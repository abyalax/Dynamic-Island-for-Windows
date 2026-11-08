void Island::OpenFromShortcut(bool control) {
    // Hidden (a full-screen app): Windows' own panel, as without the island
    // (sent as a new shortcut, which the hook lets through).
    if (!m_visible) {
        PressKeys({VK_LWIN, (WORD)(control ? 'A' : 'N')});
        return;
    }
    if (m_minimized) {
        SetMinimized(false);
    }
    HideGearBubble();
    if (!control) {
        TogglePanel(m_notifications, IslandItem::Bell);
    } else if (!m_control.SetOnly(ControlPanel::Only::All) ||
               !m_control.IsOpen()) {
        TogglePanel(m_control, IslandItem::Network);
    }
}

void Island::OpenNotifications() {
    if (!m_notifications.IsOpen()) {
        TogglePanel(m_notifications, IslandItem::Bell);
    }
}

void Island::OnNetworkChanged() {
    UpdateNetwork();
    Render();
}

void Island::OnVolumeChanged() {
    // A microphone the island muted, unmuted elsewhere: its level back.
    if (m_main && m_devices && !MicrophoneLevels().empty()) {
        RestoreMicrophoneLevels(m_devices.get(), false, false);
    }
    m_volumeGlyph = VolumeGlyph();
    m_microphoneGlyph = MicrophoneGlyph();
    UpdateVolumePercent();
    Render();
}

void Island::StartBrightnessThread(LPTHREAD_START_ROUTINE proc) {
    std::erase_if(m_brightnessThreads, [](HANDLE thread) {
        if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0) {
            CloseHandle(thread);
            return true;
        }
        return false;
    });
    m_brightnessJob->notify = m_hwnd;
    auto* held = new std::shared_ptr<BrightnessJob>(m_brightnessJob);
    if (HANDLE thread = CreateThread(nullptr, 0, proc, held, 0, nullptr)) {
        m_brightnessThreads.push_back(thread);
    } else {
        delete held;
        if (proc == BrightnessThreadProc) {
            m_brightnessJob->running = false;
        }
    }
}

// A screen's brightness changed here: every island on it and every control
// center showing it take it.
void Island::ShareBrightness(HMONITOR screen, int level) {
    ForEachIsland([&](Island& island) {
        bool changed = island.m_control.TakeBrightness(screen, level);
        if (island.m_monitorHandle == screen && island.m_brightness >= 0 &&
            island.m_brightness != level) {
            island.m_brightness = level;
            changed = true;
        }
        if (changed) {
            StartIslandAnimation();
        }
    });
}

// (Changed elsewhere meanwhile, like with the keyboard's keys.)
void Island::ReadBrightnessSoon() {
    if (!g_settings.islandBrightness || m_brightnessJob->running ||
        NowSeconds() - m_brightnessReadAt < 10) {
        return;
    }
    m_brightnessReadAt = NowSeconds();
    m_brightnessJob->screen = m_monitorHandle;
    StartBrightnessThread(BrightnessQueryProc);
}

void Island::SetBrightness(int level) {
    m_brightness = std::clamp(level, 0, 100);
    m_brightnessReadAt = NowSeconds();
    // (Its own monitor's: an island on each monitor changes each one.)
    m_brightnessJob->AskScreen(m_monitorHandle, m_brightness);
    if (!m_brightnessJob->running.exchange(true)) {
        StartBrightnessThread(BrightnessThreadProc);
    }
    ShareBrightness(m_monitorHandle, m_brightness);
    ShowTextBubble(std::wstring(Tr(L"Brightness", L"Brilho")) + L" " +
                       std::to_wstring(m_brightness) + L"%",
                   SlotMiddle(IslandItem::Brightness));
}

// The volume's percentage ("79%"), laid out again when it changes.
void Island::UpdateVolumePercent() {
    float level = 0;
    std::wstring text;
    if (auto volume = GetVolume(nullptr);
        volume && SUCCEEDED(volume->GetMasterVolumeLevelScalar(&level))) {
        text = std::to_wstring((int)std::lround(level * 100)) + L"%";
    }
    if (text != m_volumeText.text) {
        m_volumeText.text = text;
        if (g_settings.islandVolumePercent) {
            Relayout();
        }
    }
}

// The battery's charge ("83%"; nothing without a battery).
void Island::UpdateBatteryPercent() {
    SYSTEM_POWER_STATUS power;
    std::wstring text;
    if (GetSystemPowerStatus(&power) && !(power.BatteryFlag & 128) &&
        power.BatteryFlag != 255 && power.BatteryLifePercent <= 100) {
        text = std::to_wstring(power.BatteryLifePercent) + L"%";
    }
    if (text != m_batteryText.text) {
        m_batteryText.text = text;
        if (g_settings.islandBatteryPercent) {
            Relayout();
        }
    }
}

// Nothing on a computer without a battery.
std::wstring Island::BatteryGlyph() {
    SYSTEM_POWER_STATUS power;
    if (!GetSystemPowerStatus(&power) || (power.BatteryFlag & 128) ||
        power.BatteryFlag == 255 || power.BatteryLifePercent > 100) {
        return {};
    }
    const int tenth = (power.BatteryLifePercent + 5) / 10;
    const bool charging = power.ACLineStatus == 1;
    if (tenth >= 10) {
        return {charging ? (WCHAR)0xEA93 : (WCHAR)0xE83F};
    }
    return {(WCHAR)((charging ? 0xE85A : 0xE850) + tenth)};
}

void Island::UpdateStatus() {
    UpdateNetwork();
    m_volumeGlyph = VolumeGlyph();
    UpdateVolumePercent();
    UpdateBatteryPercent();
    m_microphoneGlyph = MicrophoneGlyph();
    m_batteryGlyph = BatteryGlyph();
    // A moon while "Do not disturb" is on, like the macOS Focus icon.
    m_bellGlyph = {IsIslandQuiet() ? kGlyphMoon : kGlyphBell};
}

// A short text (a count) in the middle of `rect` by its own ink, not the
// font's line box (which puts figures lower), and not snapped to pixels like
// the shape around it (the Fixed style's notches sit between pixels).
void DrawInkCentered(IDWriteFactory* dwrite,
                     ID2D1RenderTarget* target,
                     ID2D1Brush* brush,
                     const std::wstring& text,
                     IDWriteTextFormat* format,
                     const D2D1_RECT_F& rect) {
    const float width = rect.right - rect.left;
    const float height = rect.bottom - rect.top;
    winrt::com_ptr<IDWriteTextLayout> layout;
    if (!dwrite || FAILED(dwrite->CreateTextLayout(
                       text.c_str(), (UINT32)text.size(), format, width,
                       height, layout.put()))) {
        return;
    }
    D2D1_POINT_2F origin{rect.left, rect.top};
    DWRITE_OVERHANG_METRICS overhang{};
    if (SUCCEEDED(layout->GetOverhangMetrics(&overhang))) {
        // The ink's box: from -left to width + right (and likewise down).
        origin.x += (overhang.left - overhang.right) / 2;
        origin.y += (overhang.top - overhang.bottom) / 2;
    }
    target->DrawTextLayout(origin, layout.get(), brush,
                           D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);
}

void Island::UpdateFormats(float scale) {
    if (scale == m_scale && g_settings.islandIconSize == m_formatIconSize &&
        g_settings.islandTextSize == m_formatTextSize &&
        g_settings.islandBannerSize == m_formatBannerSize &&
        m_vertical == m_formatVertical && m_iconFormat && m_smallIconFormat &&
        m_textFormat && m_badgeFormat) {
        return;
    }
    m_scale = scale;
    m_formatIconSize = g_settings.islandIconSize;
    m_formatTextSize = g_settings.islandTextSize;
    m_formatBannerSize = g_settings.islandBannerSize;
    m_formatVertical = m_vertical;
    m_textScale =
        m_vertical ? scale : scale * g_settings.islandTextSize / 100.0f;
    m_usageScale = std::min(m_textScale, scale * kUsageMostScale);
    m_iconFormat = nullptr;
    m_smallIconFormat = nullptr;
    m_textFormat = nullptr;
    m_badgeFormat = nullptr;
    m_dwrite->CreateTextFormat(L"Segoe UI Variable Text", nullptr,
                               DWRITE_FONT_WEIGHT_BOLD,
                               DWRITE_FONT_STYLE_NORMAL,
                               DWRITE_FONT_STRETCH_NORMAL, 8.5f * scale, L"",
                               m_badgeFormat.put());
    m_dwrite->CreateTextFormat(IconFont(), nullptr,
                               DWRITE_FONT_WEIGHT_NORMAL,
                               DWRITE_FONT_STYLE_NORMAL,
                               DWRITE_FONT_STRETCH_NORMAL,
                               kIslandIconSize * IconScale(), L"",
                               m_iconFormat.put());
    m_dwrite->CreateTextFormat(IconFont(), nullptr,
                               DWRITE_FONT_WEIGHT_NORMAL,
                               DWRITE_FONT_STYLE_NORMAL,
                               DWRITE_FONT_STRETCH_NORMAL,
                               kIslandSmallIconSize * scale, L"",
                               m_smallIconFormat.put());
    m_dwrite->CreateTextFormat(
        L"Segoe UI Variable Text", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        kIslandTextSize * m_textScale, L"", m_textFormat.put());
    for (auto* format : {m_iconFormat.get(), m_smallIconFormat.get(),
                         m_textFormat.get(), m_badgeFormat.get()}) {
        if (format) {
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
    }
    if (!m_tabular &&
        SUCCEEDED(m_dwrite->CreateTypography(m_tabular.put()))) {
        m_tabular->AddFontFeature({DWRITE_FONT_FEATURE_TAG_TABULAR_FIGURES, 1});
    }

    // The banner's texts (and the pill's names): one line each, ending with
    // "..." when too long; `size` in pixels.
    auto make = [&](DWRITE_FONT_WEIGHT weight, float size,
                    DWRITE_TEXT_ALIGNMENT alignment) {
        winrt::com_ptr<IDWriteTextFormat> format;
        if (SUCCEEDED(m_dwrite->CreateTextFormat(
                L"Segoe UI Variable Text", nullptr, weight,
                DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"",
                format.put()))) {
            format->SetTextAlignment(alignment);
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            const DWRITE_TRIMMING trimming{
                DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
            winrt::com_ptr<IDWriteInlineObject> ellipsis;
            m_dwrite->CreateEllipsisTrimmingSign(format.get(), ellipsis.put());
            format->SetTrimming(&trimming, ellipsis.get());
        }
        return format;
    };
    const float bannerScale = scale * BannerSizeFactor();
    m_bannerAppFormat = make(DWRITE_FONT_WEIGHT_NORMAL, 11.5f * bannerScale,
                             DWRITE_TEXT_ALIGNMENT_LEADING);
    m_bannerTimeFormat = make(DWRITE_FONT_WEIGHT_NORMAL, 11.5f * bannerScale,
                              DWRITE_TEXT_ALIGNMENT_TRAILING);
    m_bannerTitleFormat = make(DWRITE_FONT_WEIGHT_SEMI_BOLD, 13 * bannerScale,
                               DWRITE_TEXT_ALIGNMENT_LEADING);
    m_bannerTextFormat = make(DWRITE_FONT_WEIGHT_NORMAL, 12.5f * bannerScale,
                              DWRITE_TEXT_ALIGNMENT_LEADING);
    m_clockDateFormat = make(DWRITE_FONT_WEIGHT_NORMAL, 11.5f * scale,
                             DWRITE_TEXT_ALIGNMENT_CENTER);
    m_mediaTitleFormat = make(DWRITE_FONT_WEIGHT_NORMAL, 12.5f * m_textScale,
                              DWRITE_TEXT_ALIGNMENT_LEADING);
    m_itemNameFormat =
        make(DWRITE_FONT_WEIGHT_SEMI_BOLD, kIslandTextSize * m_textScale,
             DWRITE_TEXT_ALIGNMENT_LEADING);
    m_usageLabelFormat = make(DWRITE_FONT_WEIGHT_SEMI_BOLD, 7.5f * m_usageScale,
                              DWRITE_TEXT_ALIGNMENT_CENTER);
    m_usageValueFormat = make(DWRITE_FONT_WEIGHT_SEMI_BOLD, 11 * m_usageScale,
                              DWRITE_TEXT_ALIGNMENT_CENTER);
    // As wide as the widest value (figures of equal width) or name.
    m_usageWidth = std::round(34 * m_usageScale);
    for (PCWSTR sample : {L"100%", L"88.8G", L"888M"}) {
        winrt::com_ptr<IDWriteTextLayout> layout;
        if (m_usageValueFormat &&
            SUCCEEDED(m_dwrite->CreateTextLayout(
                sample, (UINT32)wcslen(sample), m_usageValueFormat.get(),
                1000 * scale, 100 * scale, layout.put()))) {
            if (m_tabular) {
                layout->SetTypography(m_tabular.get(),
                                      {0, (UINT32)wcslen(sample)});
            }
            DWRITE_TEXT_METRICS metrics;
            layout->GetMetrics(&metrics);
            m_usageWidth =
                std::max(m_usageWidth,
                         std::ceil(metrics.widthIncludingTrailingWhitespace +
                                   6 * m_usageScale));
        }
    }
}

// Places the items and sizes the island around them: centered in the pill,
// at the right end of the bar (like the macOS menu bar's status items).
void Island::Layout() {
    const HMONITOR monitor = CurrentMonitor();
    MONITORINFO info{sizeof(info)};
    GetMonitorInfo(monitor, &info);
    m_monitor = info.rcMonitor;
    m_workArea = info.rcWork;
    // The screen's scaling, and the island's own size on top of it (the
    // panels keep the screen's).
    UINT dpiX = 96;
    UINT dpiY = 96;
    if (FAILED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
        HWND taskbar = FindTaskbarWnd();
        dpiX = taskbar ? GetDpiForWindow(taskbar) : 96;
    }
    m_dpiScale = dpiX / 96.0f;
    const float scale = m_dpiScale * SizeWanted() / 100.0f;
    UpdateFormats(scale);
    if (m_bannerWanted || m_bannerAmount > 0) {
        MeasureBanner();
    }

    // Upright on the left or right edge, if chosen (not for the bar). Turning
    // from one to the other, it closes into its dot first (see m_reshape).
    {
        const IslandPlace& place = CurrentPlace();
        const bool vertical =
            m_bar ? g_settings.islandBarSide > 0
                  : g_settings.islandVerticalSides &&
                        (place.x < 0.001 || place.x > 0.999) &&
                        place.y > 0.001 && place.y < 0.999;
        if (!m_verticalKnown || m_reshape == 2 || !m_visible || m_away ||
            AnimationLevel() >= 2) {
            m_vertical = vertical;
            m_verticalKnown = true;
            m_reshape = 0;
        } else if (vertical != m_vertical && !m_reshape) {
            m_reshape = 1;
            StartAnimating();
        } else if (vertical == m_vertical && m_reshape == 1) {
            m_reshape = 0;
        }
        if (m_vertical == vertical) {
            m_sideLeft =
                m_bar ? g_settings.islandBarSide == 1 : place.x < 0.5;
        }
    }
    // Across the pill (its height, or its width when upright).
    const float height =
        std::round((m_bar ? (m_vertical ? kVerticalPillWidth : kBarHeight)
                    : m_vertical ? kVerticalPillWidth * PillHeightScale()
                                 : kPillHeight * PillHeightScale()) *
                   scale);
    const float padding =
        std::round(kIslandPadding * scale * g_settings.islandPillEnds / 100);
    const float spacing = std::round(kIslandSpacing * scale);
    const float iconSlot = std::round(kIslandIconSlot * scale *
                                      g_settings.islandIconSize / 100.0f);

    // The clock's width, measured with every figure as a 0, so that it stays
    // the same as the time changes.
    // (Measured again only when that or the size changes: this runs with
    // every frame.)
    std::wstring widest = m_clock;
    for (auto& c : widest) {
        if (c >= L'0' && c <= L'9') {
            c = L'0';
        }
    }
    const std::wstring clockKey = widest + L"|" + std::to_wstring(m_textScale);
    if (clockKey != m_clockMeasuredKey) {
        m_clockMeasuredKey = clockKey;
        m_clockMeasured = 100 * scale;
        winrt::com_ptr<IDWriteTextLayout> clockLayout;
        if (m_textFormat &&
            SUCCEEDED(m_dwrite->CreateTextLayout(
                widest.c_str(), (UINT32)widest.size(), m_textFormat.get(),
                1000 * scale, height, clockLayout.put()))) {
            if (m_tabular) {
                clockLayout->SetTypography(m_tabular.get(),
                                           {0, (UINT32)widest.size()});
            }
            SmallClockMarker(clockLayout.get(), widest);
            DWRITE_TEXT_METRICS metrics;
            clockLayout->GetMetrics(&metrics);
            m_clockMeasured =
                std::ceil(metrics.widthIncludingTrailingWhitespace) +
                12 * scale;
        } else {
            m_clockMeasuredKey.clear();
        }
    }
    float clockWidth = m_clockMeasured;
    // Upright: two short lines.
    if (m_vertical) {
        clockWidth = std::round(40 * scale);
    }

    m_slots.clear();
    float x = 0;
    auto addIcon = [&](IslandItem item, const std::wstring& glyph) {
        if (glyph.empty()) {
            return;
        }
        m_slots.push_back({item, {x, 0, x + iconSlot, height}, glyph, 1});
        x += iconSlot + spacing;
    };
    // A percentage beside an item, as part of it (clicked, scrolled).
    auto addSideText = [&](IslandItem item, SideText& side) {
        if (side.text.empty() || !m_textFormat) {
            return;
        }
        if (side.text != side.measured || m_textScale != side.scale) {
            side.measured = side.text;
            side.scale = m_textScale;
            side.width = 0;
            winrt::com_ptr<IDWriteTextLayout> layout;
            if (SUCCEEDED(m_dwrite->CreateTextLayout(
                    side.text.c_str(), (UINT32)side.text.size(),
                    m_textFormat.get(), 1000, 100, layout.put()))) {
                DWRITE_TEXT_METRICS metrics{};
                layout->GetMetrics(&metrics);
                side.width = std::ceil(metrics.width);
            }
        }
        if (side.width <= 0) {
            return;
        }
        // Close to its icon's glyph (centered in its slot, see
        // kIslandIconSize), not to the slot.
        x -= std::round(spacing +
                        iconSlot * (1 - kIslandIconSize / kIslandIconSlot) / 2 -
                        3 * scale);
        m_slots.push_back({item, {x, 0, x + side.width, height}, side.text, 0});
        x += side.width + spacing;
    };
    // Two short lines (see kStackedFont).
    auto addStacked = [&](IslandItem item, const std::wstring& label,
                          const std::wstring& value,
                          const std::wstring& widest = {}) {
        const float width = StackedWidth(label, value, widest);
        m_slots.push_back(
            {item, {x, 0, x + width, height}, label + L"\n" + value,
             kStackedFont});
        x += width + spacing;
    };
    // Where each group was laid out (from, to and its slots), for the clock
    // kept in the middle.
    struct GroupSpan {
        int group;
        float start;
        float end;
        size_t firstSlot;
        size_t endSlot;
        // Pinned tray icons beside that group's items (see addAnchored).
        bool tray = false;
    };
    std::vector<GroupSpan> spans;
    // Pinned tray icons put beside another item (see TrayAnchor): in a span
    // of their own before or after its group's, sliding with it, there even
    // while it's hidden (on its notch), whenever the tray's icons show.
    const bool trayShown = g_settings.islandTray || PlaceShows(PillGroup::Tray);
    auto addAnchored = [&](int anchor, PillGroup group) {
        if (!trayShown || !InThisNotch(group)) {
            return;
        }
        const size_t first = m_slots.size();
        const float start = x;
        for (const auto& icon : m_trayIcons) {
            if (TrayAnchor(icon.key) != anchor) {
                continue;
            }
            const float iconShown = (float)std::clamp(icon.shown, 0.0, 1.15);
            const float pinned =
                PinAmount(icon.key) * std::min(iconShown, 1.0f);
            if (pinned <= 0.001f) {
                continue;
            }
            IslandSlot slot{IslandItem::TrayIcon,
                            {x, 0, x + iconSlot * pinned, height},
                            icon.key,
                            4};
            slot.scale = PinAmount(icon.key) * iconShown;
            slot.group = (int)group;
            m_slots.push_back(slot);
            x += (iconSlot + spacing) * pinned;
        }
        if (m_slots.size() > first) {
            spans.push_back(
                {(int)group, start, x, first, m_slots.size(), true});
        }
    };
    // The groups of items, in the order chosen (see g_pillOrder; the app in
    // front may have its own), each squeezed by how much it's shown.
    const IslandPlace* orderPlace = m_bar ? nullptr : FindAppPlace(m_placeApp);
    const std::vector<PillGroup> order =
        orderPlace ? PlaceOrder(*orderPlace) : g_pillOrder;
    const bool orderMoved = !m_layoutOrder.empty() && m_layoutOrder != order;
    m_layoutOrder = order;
    for (PillGroup group : order) {
        addAnchored(TrayAnchorBefore(group), group);
        const float shown =
            (float)std::clamp(m_groupShown[(int)group], 0.0, 1.15);
        if (shown <= 0.001f) {
            addAnchored((int)group, group);
            continue;
        }
        const size_t firstSlot = m_slots.size();
        const float groupStart = x;
        switch (group) {
            case PillGroup::Apps: {
                // Apps with notifications, like the icons on the left
                // of a phone's status bar. They take room as they grow
                // in, so the island widens with them. Folded (see
                // islandFoldApps): an arrow, and the apps as far out as
                // m_appsAmount.
                const bool folded =
                    g_settings.islandFoldApps &&
                    std::any_of(m_appEntries.begin(), m_appEntries.end(),
                                [](const AppEntry& entry) {
                                    return entry.present;
                                });
                if (folded) {
                    addIcon(IslandItem::AppsFold,
                            {m_appsOpen ? kGlyphPrevious : kGlyphNext});
                }
                const float out =
                    folded ? (float)std::clamp(m_appsAmount, 0.0, 1.0) : 1;
                float appsShown = 0;
                for (const auto& entry : m_appEntries) {
                    const float shown =
                        (float)std::clamp(entry.shown, 0.0, 1.15) * out;
                    const float room = std::min(shown, 1.0f);
                    IslandSlot slot{IslandItem::App,
                                    {x, 0, x + iconSlot * room, height},
                                    entry.id,
                                    3};
                    slot.scale = shown;
                    m_slots.push_back(slot);
                    x += (iconSlot + spacing) * room;
                    appsShown = std::max(appsShown, room);
                }
                x += spacing * appsShown;
                break;
            }
            case PillGroup::Tray: {
                // The tray: the arrow (when some icons aren't pinned), the
                // others opening sideways after it, then the pinned ones.
                if (g_settings.islandTray || PlaceShows(PillGroup::Tray)) {
                    bool unpinned = false;
                    for (const auto& icon : m_trayIcons) {
                        unpinned = unpinned ||
                                   (!icon.leaving && !IsTrayPinned(icon.key));
                    }
                    if (unpinned) {
                        addIcon(IslandItem::Overflow, {kGlyphNext});
                    }
                    // The room grows without going past it (no jolt at the
                    // end); the icons bounce in on their own.
                    const float room =
                        (float)std::clamp(m_trayAmount, 0.0, 1.0);
                    const float open =
                        (float)std::clamp(m_trayBounce, 0.0, 1.15);
                    // Many icons: the list stops at the chosen number (see
                    // g_trayListIcons; never more than ~60% of the screen)
                    // and the wheel scrolls the rest; an icon cut at an
                    // edge, fading, shows there are more that way.
                    const float iconStep = iconSlot + spacing;
                    const float along =
                        (float)(m_vertical ? m_monitor.bottom - m_monitor.top
                                           : m_monitor.right - m_monitor.left);
                    const int fits =
                        std::max(1, std::min(g_trayListIcons,
                                             (int)(along * 0.6f / iconStep)));
                    const float listMax = (fits + 0.5f) * iconStep;
                    float listFull = 0;
                    for (const auto& icon : m_trayIcons) {
                        listFull +=
                            iconStep * (1 - PinAmount(icon.key)) *
                            std::min((float)std::clamp(icon.shown, 0.0, 1.15),
                                     1.0f);
                    }
                    m_trayIconStep = iconStep;
                    m_trayOverflow = std::max(0.0f, listFull - listMax);
                    m_trayScrollTarget = std::clamp(m_trayScrollTarget, 0.0,
                                                    (double)m_trayOverflow);
                    m_trayScroll =
                        std::clamp(m_trayScroll, 0.0, (double)m_trayOverflow);
                    const float scroll = (float)m_trayScroll;
                    m_trayMoreBefore = scroll > 0.5f;
                    m_trayMoreAfter = scroll < m_trayOverflow - 0.5f;
                    const float listStart = x;
                    const float listEnd =
                        listStart + std::min(listFull, listMax) * room;
                    x = listStart - scroll * room;
                    // An icon being pinned shrinks out of the list as it grows
                    // among the pinned ones (and back when unpinned).
                    for (const auto& icon : m_trayIcons) {
                        const float unpinned = 1 - PinAmount(icon.key);
                        const float iconShown =
                            (float)std::clamp(icon.shown, 0.0, 1.15);
                        const float iconRoom =
                            room * unpinned * std::min(iconShown, 1.0f);
                        if (iconRoom <= 0.001f) {
                            continue;
                        }
                        IslandSlot slot{IslandItem::TrayIcon,
                                        {x, 0, x + iconSlot * iconRoom, height},
                                        icon.key,
                                        4};
                        slot.scale = open * unpinned * iconShown;
                        slot.inList = true;
                        if (slot.rect.right > listStart &&
                            slot.rect.left < listEnd) {
                            m_slots.push_back(slot);
                        }
                        x += (iconSlot + spacing) * iconRoom;
                    }
                    x = listEnd;
                    if (m_trayOverflow > 0 && listEnd - listStart >= 1) {
                        m_slots.push_back({IslandItem::None,
                                           {listStart, 0, listEnd, height},
                                           L"",
                                           kTrayListFont});
                    }
                    // With the list open, a faint line between it and the
                    // pinned ones.
                    bool anyPinned = false;
                    for (const auto& icon : m_trayIcons) {
                        anyPinned = anyPinned || (TrayAnchor(icon.key) == -1 &&
                                                  PinAmount(icon.key) > 0.001f);
                    }
                    if (unpinned && anyPinned && room > 0.001f) {
                        const float line = std::round(6 * scale);
                        IslandSlot slot{IslandItem::TraySeparator,
                                        {x, 0, x + line * room, height},
                                        L"",
                                        5};
                        slot.scale = room;
                        m_slots.push_back(slot);
                        x += (line + spacing) * room;
                    }
                    for (const auto& icon : m_trayIcons) {
                        // (Not the ones put elsewhere, see addAnchored.)
                        if (TrayAnchor(icon.key) != -1) {
                            continue;
                        }
                        const float iconShown =
                            (float)std::clamp(icon.shown, 0.0, 1.15);
                        const float pinned =
                            PinAmount(icon.key) * std::min(iconShown, 1.0f);
                        if (pinned <= 0.001f) {
                            continue;
                        }
                        IslandSlot slot{IslandItem::TrayIcon,
                                        {x, 0, x + iconSlot * pinned, height},
                                        icon.key,
                                        4};
                        slot.scale = PinAmount(icon.key) * iconShown;
                        m_slots.push_back(slot);
                        x += (iconSlot + spacing) * pinned;
                    }
                }
                break;
            }
            case PillGroup::Network:
                addIcon(IslandItem::Network, {kGlyphControlCenter});
                break;
            case PillGroup::Volume:
                addIcon(IslandItem::Volume, m_volumeGlyph);
                if (g_settings.islandVolumePercent) {
                    addSideText(IslandItem::Volume, m_volumeText);
                }
                break;
            case PillGroup::Battery:
                addIcon(IslandItem::Battery, m_batteryGlyph);
                if (g_settings.islandBatteryPercent &&
                    !m_batteryGlyph.empty()) {
                    addSideText(IslandItem::Battery, m_batteryText);
                }
                break;
            case PillGroup::Clock:
                // (A little more room on its sides: see below.)
                {
                    // Growing or shrinking to its new width.
                    m_clockWidthTarget = clockWidth;
                    if (m_clockWidth < 0 || m_vertical) {
                        m_clockWidth = clockWidth;
                        m_clockWidthVelocity = 0;
                    } else if (std::fabs(m_clockWidth - clockWidth) > 0.5) {
                        StartAnimating();
                    }
                    const float shownWidth = (float)m_clockWidth;
                    m_slots.push_back({IslandItem::Clock,
                                       {x, 0, x + shownWidth, height},
                                       m_clock,
                                       0});
                    x += shownWidth + spacing;
                }
                break;
            case PillGroup::Bell:
                addIcon(IslandItem::Bell, m_bellGlyph);
                break;
            case PillGroup::Media:
            case PillGroup::MediaTitle:
            case PillGroup::MediaButtons: {
                // What's playing: its note, its cover and title, and its
                // buttons, each its own group (placed apart, or together
                // as they were); they take room as they grow in, so the
                // pill widens with them.
                auto addGrowing = [&](IslandItem item,
                                      const std::wstring& glyph, float room,
                                      float amount) {
                    if (room <= 0.001f && amount <= 0.001f) {
                        return;
                    }
                    room = std::clamp(room, 0.0f, 1.0f);
                    IslandSlot entry{item,
                                     {x, 0, x + iconSlot * room, height},
                                     glyph,
                                     1};
                    entry.scale = std::max(amount, 0.0f);
                    m_slots.push_back(entry);
                    x += (iconSlot + spacing) * room;
                };
                const float mediaRoom =
                    (float)std::clamp(m_mediaRoom, 0.0, 1.0);
                const float media = (float)std::clamp(m_mediaAmount, 0.0, 1.15);
                const float titleRoom =
                    (float)std::clamp(m_mediaTitleRoom, 0.0, 1.0);
                const bool titleShown = !m_vertical && titleRoom > 0.001f &&
                                        (mediaRoom > 0.001f || media > 0.001f);
                if (group == PillGroup::Media) {
                    if (MediaIconWanted()) {
                        addGrowing(IslandItem::Media, {kGlyphMusic},
                                   mediaRoom, media);
                    }
                    break;
                }
                if (group == PillGroup::MediaTitle) {
                    if (titleShown) {
                        // The cover and the title, as wide as the title (up to a
                        // limit), opening out of the note; the line sung in
                        // its place, swapping in like another title (see
                        // LyricsInTitle).
                        const bool lyric = LyricsInTitle();
                        const bool holding = PanelShownFromPill();
                        const std::wstring titleText =
                            lyric ? m_lyricsLine : MediaTitleText();
                        const std::wstring measuredKey =
                            titleText + L"\n" + m_mediaShown.artist + L"\n" +
                            std::to_wstring(m_textScale) + L"\n" +
                            std::to_wstring(g_settings.islandMediaTitleLimit) +
                            (holding ? L"\nheld" : L"");
                        if (measuredKey != m_mediaTitleMeasured) {
                            const float widthBefore = m_mediaTitleWidth;
                            m_mediaTitleMeasured = measuredKey;
                            auto measure = [&](const std::wstring& text) {
                                winrt::com_ptr<IDWriteTextLayout> layout;
                                if (!m_mediaTitleFormat ||
                                    FAILED(m_dwrite->CreateTextLayout(
                                        text.c_str(), (UINT32)text.size(),
                                        m_mediaTitleFormat.get(), 10000, height,
                                        layout.put()))) {
                                    return 0.0f;
                                }
                                DWRITE_TEXT_METRICS metrics;
                                layout->GetMetrics(&metrics);
                                return metrics.widthIncludingTrailingWhitespace;
                            };
                            const std::wstring before = m_mediaTitleShown;
                            m_mediaTitleShown = titleText;
                            m_mediaTitleWidth = measure(m_mediaTitleShown);
                            if (m_mediaTitleWidth < kMediaTitleShortWidth * scale &&
                                !lyric && !m_mediaShown.title.empty() &&
                                !m_mediaShown.artist.empty()) {
                                m_mediaTitleShown +=
                                    L" \u00B7 " + m_mediaShown.artist;
                                m_mediaTitleWidth = measure(m_mediaTitleShown);
                            }
                            m_mediaTitleFullWidth = m_mediaTitleWidth;
                            m_mediaTitleWidth = std::clamp(
                                m_mediaTitleWidth,
                                std::round(kMediaTitleMinWidth * scale),
                                std::round(g_settings.islandMediaTitleLimit *
                                           scale));
                            // (The song's lines in its place: as wide as the
                            // widest since the title was last there, see
                            // m_lyricsHeldWidth.)
                            const bool held =
                                lyric && g_settings.islandLyricsWidth !=
                                             kLyricsWidthFollows;
                            m_titleLyricsWidth =
                                held ? std::max(m_titleLyricsWidth,
                                                m_mediaTitleWidth)
                                     : 0;
                            if (held) {
                                m_mediaTitleWidth = m_titleLyricsWidth;
                            }
                            const bool released =
                                !holding && m_titleHeldWidth > 0;
                            if (holding) {
                                if (m_titleHeldWidth <= 0) {
                                    m_titleHeldWidth = widthBefore > 0
                                        ? widthBefore : m_mediaTitleWidth;
                                }
                                m_mediaTitleWidth = m_titleHeldWidth;
                            } else {
                                m_titleHeldWidth = 0;
                            }
                            // Another title on the pill: it swaps in, and the
                            // width follows; the same one (another size of the
                            // island) or the first one is just there.
                            const bool swapping = !before.empty() &&
                                                  before != m_mediaTitleShown &&
                                                  titleRoom > 0.01f &&
                                                  media > 0.01f;
                            if (swapping) {
                                m_mediaTitleBefore = before;
                                m_mediaSwap = 1;
                                m_mediaSwapVelocity = 0;
                                StartAnimating();
                            } else if (released) {
                                // (Its width goes to the title's smoothly.)
                                StartAnimating();
                            } else {
                                m_mediaTitleWidthShown = m_mediaTitleWidth;
                                m_mediaTitleWidthVelocity = 0;
                            }
                        }
                        const std::wstring& title = m_mediaTitleShown;
                        const float full = std::round(18 * scale) +
                                           std::round(8 * scale) +
                                           (float)m_mediaTitleWidthShown +
                                           std::round(4 * scale);
                        const float width =
                            (iconSlot +
                             (std::max(full, iconSlot) - iconSlot) * titleRoom) *
                            mediaRoom;
                        IslandSlot entry{
                            IslandItem::Media, {x, 0, x + width, height}, title, 6};
                        entry.scale = std::max(media, 0.0f);
                        m_slots.push_back(entry);
                        x += width + spacing * mediaRoom;
                    }
                    break;
                }
                const float buttonsRoom =
                    mediaRoom * (float)std::clamp(m_mediaButtonsRoom, 0.0, 1.0);
                const float buttons =
                    std::min(media, 1.0f) *
                    (float)std::clamp(m_mediaButtonsAmount, 0.0, 1.15);
                addGrowing(IslandItem::MediaPrevious, {kGlyphPreviousTrack},
                           buttonsRoom, buttons);
                addGrowing(IslandItem::MediaPlayPause,
                           {m_mediaShown.playing ? kGlyphPause : kGlyphPlay},
                           buttonsRoom, buttons);
                addGrowing(IslandItem::MediaNext, {kGlyphNextTrack},
                           buttonsRoom, buttons);
                break;
            }
            case PillGroup::Power:
                addIcon(IslandItem::Power, {kGlyphPower});
                break;
            case PillGroup::Usage: {
                // A short name over each value; all of them when shown for
                // the app in front without any chosen.
                const bool all = !AnyUsageShown();
                const struct {
                    IslandItem item;
                    bool on;
                    PCWSTR name;
                } kUsages[] = {
                    {IslandItem::UsageCpu, g_settings.usageCpu, L"CPU"},
                    {IslandItem::UsageRam, g_settings.usageRam, L"RAM"},
                    {IslandItem::UsageVram, g_settings.usageVram, L"VRAM"},
                };
                for (int i = 0; i < 3; i++) {
                    if (!kUsages[i].on && !all) {
                        continue;
                    }
                    const std::wstring value = UsageText(i);
                    m_slots.push_back(
                        {kUsages[i].item,
                         {x, 0, x + m_usageWidth, height},
                         std::wstring(kUsages[i].name) + L"\n" +
                             (value.empty() ? L"\u2013" : value),
                         7});
                    x += m_usageWidth + spacing;
                }
                break;
            }
            case PillGroup::Microphone:
                // (Nothing without a microphone: its icon shrinks away.)
                if (!m_microphoneGlyph.empty()) {
                    m_microphoneShown = m_microphoneGlyph;
                }
                addIcon(IslandItem::Microphone, m_microphoneShown);
                break;
            case PillGroup::Notes:
                addIcon(IslandItem::Notes, {kGlyphNote});
                break;
            case PillGroup::Tasks:
                addIcon(IslandItem::Tasks, {kGlyphTasks});
                break;
            case PillGroup::Shortcuts:
                addIcon(IslandItem::Shortcuts, {kGlyphKeyboardShortcuts});
                break;
            case PillGroup::Updates:
                addIcon(IslandItem::Updates, {(WCHAR)0xE895});
                break;
            case PillGroup::Profiles:
                addIcon(IslandItem::Profiles, {kGlyphProfiles});
                break;
            case PillGroup::Brightness:
                if (m_brightness >= 0) {
                    addIcon(IslandItem::Brightness, {kGlyphBrightness});
                }
                break;
            case PillGroup::InputMethod:
                if (!m_inputMethodText.empty()) {
                    m_slots.push_back({IslandItem::InputMethod,
                                       {x, 0, x + iconSlot, height},
                                       m_inputMethodText,
                                       0});
                    x += iconSlot + spacing;
                }
                break;
            case PillGroup::Privacy:
                // Each in use, in its color (see Render).
                if (!m_privacy.microphone.empty()) {
                    addIcon(IslandItem::PrivacyMicrophone, {(WCHAR)0xE720});
                }
                if (!m_privacy.camera.empty()) {
                    addIcon(IslandItem::PrivacyCamera, {(WCHAR)0xE714});
                }
                if (!m_privacy.location.empty()) {
                    addIcon(IslandItem::PrivacyLocation, {(WCHAR)0xE81D});
                }
                break;
            case PillGroup::Desktops:
                // A dot each (up to 9; then "2/12").
                if (g_settings.islandDesktops && !m_desktops.names.empty()) {
                    const size_t count = m_desktops.names.size();
                    const float width =
                        count > 9 ? std::round(40 * scale)
                                  : std::round((count * 8 + 8) * scale);
                    m_slots.push_back({IslandItem::Desktops,
                                       {x, 0, x + width, height},
                                       L"",
                                       kDesktopsFont});
                    x += width + spacing;
                }
                break;
            case PillGroup::Folders:
                if (g_settings.islandFolders) {
                    addIcon(IslandItem::Folders, {(WCHAR)0xE8B7});
                }
                break;
            case PillGroup::Wallpaper:
                if (g_settings.islandWallpaper) {
                    addIcon(IslandItem::Wallpaper, {(WCHAR)0xE8B9});
                }
                break;
            case PillGroup::Drives:
                if (!m_drives.empty()) {
                    addIcon(IslandItem::Drives, {(WCHAR)0xE88E});
                }
                break;
            case PillGroup::NetSpeed:
                // Up over down (bytes a second).
                addStacked(IslandItem::NetSpeed,
                           L"\u2191 " + (g_netSpeed.up.empty()
                                             ? std::wstring(L"\u2013")
                                             : g_netSpeed.up),
                           L"\u2193 " + (g_netSpeed.down.empty()
                                             ? std::wstring(L"\u2013")
                                             : g_netSpeed.down),
                           L"\u2193 888M");
                break;
            case PillGroup::WorldClocks:
                // Each of Windows' additional clocks (a day ahead or behind
                // marked); a globe until one is added there.
                if (m_worldClocks.empty()) {
                    addIcon(IslandItem::WorldClock, {(WCHAR)0xE909});
                }
                for (const auto& clock : m_worldClocks) {
                    addStacked(IslandItem::WorldClock, clock.name,
                               clock.time + (clock.days > 0   ? L" +1"
                                             : clock.days < 0 ? L" \u22121"
                                                              : L""));
                }
                break;
            case PillGroup::Countdown:
                addStacked(IslandItem::Countdown, m_countdownTitle,
                           m_countdownText);
                break;
            case PillGroup::AiUsage: {
                // Each agent with its limits: its app's icon (only its text
                // without the app), and beside it the tighter one's use with
                // the time left (in its color, see Render); the robot alone
                // while none has them (its panel says how).
                const long long now = UnixNow();
                bool any = false;
                for (int agent = 0; agent < kAiAgents; agent++) {
                    if (!AiAgentOn(agent) || !AiUsageKnown(g_aiUsage[agent])) {
                        continue;
                    }
                    if (const std::wstring app = AiAgentApp(agent);
                        !app.empty()) {
                        m_slots.push_back({IslandItem::AiUsage,
                                           {x, 0, x + iconSlot, height},
                                           app,
                                           kAppIconFont});
                        m_slots.back().action = agent;
                        x += iconSlot + spacing;
                    }
                    SideText& side = m_aiTexts[agent];
                    side.text = AiPillValue(g_aiUsage[agent], now);
                    const size_t slots = m_slots.size();
                    addSideText(IslandItem::AiUsage, side);
                    if (m_slots.size() > slots) {
                        m_slots.back().action = agent;
                    }
                    any = true;
                }
                if (!any) {
                    addIcon(IslandItem::AiUsage, {kGlyphAiUsage});
                }
                break;
            }
            case PillGroup::Commands: {
                // Their panel's icon, then each one chosen as a button: the
                // icon of what it opens (its kind's without one); a click
                // runs it, its name under the mouse.
                addIcon(IslandItem::Commands, {kGlyphCommand});
                const auto& commands = Commands();
                for (size_t i = 0; i < commands.size(); i++) {
                    if (!commands[i].pinned) {
                        continue;
                    }
                    if (!commands[i].iconPath.empty()) {
                        m_slots.push_back({IslandItem::Commands,
                                           {x, 0, x + iconSlot, height},
                                           commands[i].iconPath,
                                           kAppIconFont});
                        x += iconSlot + spacing;
                    } else {
                        addIcon(IslandItem::Commands,
                                {CommandGlyph(commands[i])});
                    }
                    m_slots.back().action = (int)i;
                }
                break;
            }
            case PillGroup::Shelf:
                // How many files it holds beside it (shown only with
                // some).
                addIcon(IslandItem::Shelf, {(WCHAR)0xE7B8});
                m_shelfText.text = std::to_wstring(Shelf().size());
                addSideText(IslandItem::Shelf, m_shelfText);
                break;
            case PillGroup::Eyes: {
                // Two eyes, as wide as their shape asks (see EyesWidth).
                const float eyesWidth = EyesWidth(ChosenEyeShape(), height);
                m_slots.push_back({IslandItem::Eyes,
                                   {x, 0, x + eyesWidth, height},
                                   L"",
                                   kEyesFont});
                x += eyesWidth + spacing;
                break;
            }
            case PillGroup::Visualizer:
                m_slots.push_back({IslandItem::Visualizer,
                                   {x, 0, x + iconSlot, height},
                                   L"",
                                   kVisualizerFont});
                x += iconSlot + spacing;
                break;
            case PillGroup::Timer: {
                // The one ending first (with any ending in the same second),
                // or the stopwatch going alone: its time; the icon with
                // none going. (Each one's mode in its slot, for its tab.)
                const std::vector<TimerMode> timers = PillTimers();
                for (TimerMode mode : timers) {
                    addStacked(IslandItem::Timer,
                               TimerModeName(mode, g_timer.onBreak),
                               TimerText(mode), L"88:88");
                    m_slots.back().action = (int)mode;
                }
                if (timers.empty()) {
                    addIcon(IslandItem::Timer, {(WCHAR)0xE916});
                }
                break;
            }
            case PillGroup::RecycleBin:
                // Its count beside it (nothing when empty).
                addIcon(IslandItem::RecycleBin, {(WCHAR)0xE74D});
                addSideText(IslandItem::RecycleBin, m_recycleText);
                break;
            case PillGroup::Weather:
                // Its sky and temperature (a cloud until it comes); its room
                // follows what it shows.
                if (m_weatherShown.empty()) {
                    m_weatherWidthTarget = iconSlot;
                    if (m_weatherWidth < 0) {
                        m_weatherWidth = iconSlot;
                    }
                    const float room = (float)m_weatherWidth;
                    m_slots.push_back({IslandItem::Weather,
                                       {x, 0, x + room, height},
                                       {(WCHAR)0xE753},
                                       1});
                    x += room + spacing;
                } else {
                    // The sky, and the temperature as wide as it is
                    // (measured again only when it changes).
                    const std::wstring weatherKey =
                        m_weatherShown + L"|" + std::to_wstring(m_textScale);
                    if (weatherKey != m_weatherMeasuredKey) {
                        m_weatherMeasuredKey = weatherKey;
                        m_weatherMeasured = std::round(26 * m_textScale);
                        const size_t tab = m_weatherShown.find(L'\t');
                        winrt::com_ptr<IDWriteTextLayout> measure;
                        if (tab != std::wstring::npos && m_textFormat &&
                            SUCCEEDED(m_dwrite->CreateTextLayout(
                                m_weatherShown.c_str() + tab + 1,
                                (UINT32)(m_weatherShown.size() - tab - 1),
                                m_textFormat.get(), 1000, height,
                                measure.put()))) {
                            DWRITE_TEXT_METRICS metrics{};
                            measure->GetMetrics(&metrics);
                            m_weatherMeasured = std::ceil(metrics.width);
                        }
                    }
                    const float temperatureWidth = m_weatherMeasured;
                    m_weatherWidthTarget = std::round(20 * m_textScale) +
                                           temperatureWidth +
                                           std::round(4 * m_textScale);
                    if (m_weatherWidth < 0) {
                        m_weatherWidth = m_weatherWidthTarget;
                    }
                    const float weatherWidth = (float)m_weatherWidth;
                    m_slots.push_back({IslandItem::Weather,
                                       {x, 0, x + weatherWidth, height},
                                       m_weatherShown,
                                       9});
                    x += weatherWidth + spacing;
                }
                break;
            case PillGroup::KomorebiLayout:
            case PillGroup::FocusedApp: {
                // komorebi's layout on this monitor, or the app in focus on
                // it: an icon and the name, as wide as the name up to a
                // limit (the whole name in its tip); upright, the icon.
                const bool komorebi = group == PillGroup::KomorebiLayout;
                ItemWidth& width = komorebi ? m_komorebiWidth : m_focusWidth;
                const KomorebiMonitor* monitor =
                    komorebi ? KomorebiMonitorOf(m_monitorHandle) : nullptr;
                if (komorebi ? !monitor : !m_focusWindow && !m_focusDesktop) {
                    // (Back later: at its width at once, nothing swapping.)
                    width.shown = -1;
                    width.name.clear();
                    break;
                }
                // komorebi's workspaces of this monitor first, a dot each
                // (like yasb's), when there's more than one: the one shown
                // ("*"), ones with windows ("+"), empty ones (left out if
                // chosen).
                if (komorebi && !m_vertical && monitor->workspaces.size() > 1) {
                    // (Spaced like the other items: their shadows don't
                    // meet.)
                    const float dot = std::round(iconSlot * 0.7f);
                    for (int i = 0; i < (int)monitor->workspaces.size(); i++) {
                        const bool here = i == monitor->workspace;
                        const bool windows = monitor->workspaces[i].windows;
                        if (g_settings.komorebiHideEmpty && !here && !windows) {
                            continue;
                        }
                        m_slots.push_back(
                            {IslandItem::KomorebiWorkspace,
                             {x, 0, x + dot, height},
                             std::to_wstring(i) + (here      ? L"*"
                                                   : windows ? L"+"
                                                             : L""),
                             kWorkspaceDotFont});
                        x += dot + spacing;
                    }
                }
                const std::wstring name =
                    komorebi
                        ? KomorebiLayoutName(*monitor, g_komorebiShown.paused)
                        : FocusedText();
                // (komorebi's: "\x1" and its icon's number, see
                // DrawKomorebiIcon; the desktop's: its glyph.)
                const int komorebiIcon =
                    komorebi ? KomorebiIconOf(*monitor, g_komorebiShown.paused)
                             : -1;
                const std::wstring icon =
                    komorebi ? (komorebiIcon >= 0
                                    ? L"\x1" + std::to_wstring(komorebiIcon)
                                    : std::wstring())
                    : m_focusDesktop ? std::wstring()
                                     : m_focusKey;
                // Another name: the one before slides up and out.
                if (name != width.name || icon != width.icon) {
                    if (!width.name.empty() && width.shown >= 0 &&
                        !m_vertical) {
                        width.nameBefore = width.name;
                        width.iconBefore = width.icon;
                        width.swap = 1;
                        width.swapVelocity = 0;
                        StartAnimating();
                    }
                    width.name = name;
                    width.icon = icon;
                }
                // (With the window's title, twice as wide at most.)
                const float most =
                    kItemNameMaxWidth *
                    (!komorebi && !m_focusTitle.empty() ? 2 : 1);
                width.target =
                    m_vertical ? iconSlot
                               : std::round(22 * scale) +
                                     std::min(
                                         MeasureItemName(width, name, height),
                                         std::round(most * scale)) +
                                     std::round(6 * scale);
                if (width.shown < 0 || m_vertical) {
                    width.shown = width.target;
                    width.velocity = 0;
                } else if (std::fabs(width.shown - width.target) > 0.5) {
                    StartAnimating();
                }
                const float room = (float)width.shown;
                m_slots.push_back({komorebi ? IslandItem::KomorebiLayout
                                            : IslandItem::FocusedApp,
                                   {x, 0, x + room, height},
                                   name,
                                   kNamedItemFont});
                x += room + spacing;
                break;
            }
            case PillGroup::Lyrics:
                // The line sung now (or the one before, while it goes), as
                // wide as its words up to a limit.
                if (IDWriteTextFormat* format = LyricsFormat()) {
                    const std::wstring& line =
                        m_lyricsLine.empty() ? m_lyricsBefore : m_lyricsLine;
                    float width = 0;
                    // (Measured again only when the line or its size
                    // changes, not each frame the pill is laid out.)
                    const std::wstring measureKey =
                        line + L"|" +
                        std::to_wstring(g_settings.islandLyricsSize) + L"|" +
                        std::to_wstring(scale);
                    if (measureKey != m_lyricsMeasuredKey) {
                        m_lyricsMeasuredKey = measureKey;
                        m_lyricsMeasured = 0;
                        winrt::com_ptr<IDWriteTextLayout> layout;
                        DWRITE_TEXT_METRICS metrics{};
                        if (!line.empty() &&
                            SUCCEEDED(m_dwrite->CreateTextLayout(
                                line.c_str(), (UINT32)line.size(), format,
                                2000, 100, layout.put())) &&
                            SUCCEEDED(layout->GetMetrics(&metrics))) {
                            m_lyricsMeasured = std::ceil(metrics.width);
                        }
                    }
                    if (m_lyricsMeasured > 0) {
                        const int longest = kLyricsMaxWidths[std::clamp(
                            g_settings.islandLyricsWidth, 0,
                            (int)std::size(kLyricsMaxWidths) - 1)];
                        width = std::min(m_lyricsMeasured +
                                             std::round(8 * scale),
                                         std::round(longest * scale));
                    }
                    // (As wide as the widest line since the last pause:
                    // lines one after another don't resize the pill, and a
                    // notch's window, each time.)
                    if (width > 0 && !m_lyricsLine.empty() &&
                        g_settings.islandLyricsWidth != kLyricsWidthFollows) {
                        m_lyricsHeldWidth = std::max(m_lyricsHeldWidth, width);
                        width = m_lyricsHeldWidth;
                    }
                    if (width > 0) {
                        m_lyricsWidth = width;
                        m_slots.push_back({IslandItem::Lyrics,
                                           {x, 0, x + width, height},
                                           line,
                                           kLyricsFont});
                        x += width + spacing;
                    }
                }
                break;
            case PillGroup::Theme:
                // Drawn turning (see m_themeTurn).
                m_slots.push_back({IslandItem::ThemeSwitch,
                                   {x, 0, x + iconSlot, height},
                                   L"",
                                   8});
                x += iconSlot + spacing;
                break;
            default:
                // A control center button (see kPillActions); Wi-Fi as the
                // network (see DrawNetwork).
                if (const int action = PillActionOfGroup(group); action >= 0) {
                    const ControlId id = kPillActions[action].id;
                    addIcon(IslandItem::QuickAction,
                            id == ControlId::Network && !m_networkGlyph.empty()
                                ? m_networkGlyph
                                : std::wstring(1, GetControlInfo(id).glyph));
                    m_slots.back().action = action;
                }
                break;
        }
        // Going (its items gone with its option): the ones it had, as it
        // shrinks; wanted: kept for then.
        const int index = (int)group;
        const bool wanted = GroupWanted(group);
        if (wanted && m_slots.size() > firstSlot) {
            m_groupKept[index].assign(m_slots.begin() + firstSlot,
                                      m_slots.end());
            for (auto& kept : m_groupKept[index]) {
                kept.rect.left -= groupStart;
                kept.rect.right -= groupStart;
            }
            m_groupKeptWidth[index] = x - groupStart;
        } else if (!wanted && m_slots.size() == firstSlot &&
                   !m_groupKept[index].empty()) {
            for (IslandSlot kept : m_groupKept[index]) {
                kept.rect.left += groupStart;
                kept.rect.right += groupStart;
                m_slots.push_back(kept);
            }
            x = groupStart + m_groupKeptWidth[index];
        }
        // Its width following its items' (see m_groupWidthShown); first
        // laid out: as it is.
        const float natural = x - groupStart;
        if (m_groupWidthWanted[index] <= 0 || !wanted) {
            m_groupWidthShown[index] = natural;
            m_groupWidthVelocity[index] = 0;
        } else if (std::fabs(m_groupWidthWanted[index] - natural) > 0.5f) {
            StartAnimating();
        }
        m_groupWidthWanted[index] = natural;
        const float stretch =
            natural > 0.5f ? (float)m_groupWidthShown[index] / natural : 1.0f;
        const float room = std::min(shown, 1.0f) * stretch;
        for (size_t i = firstSlot; i < m_slots.size(); i++) {
            IslandSlot& slot = m_slots[i];
            slot.group = (int)group;
            if (shown < 0.999f || shown > 1.001f) {
                slot.scale *= shown;
            }
            if (std::fabs(room - 1) > 0.001f) {
                slot.rect.left =
                    groupStart + (slot.rect.left - groupStart) * room;
                slot.rect.right =
                    groupStart + (slot.rect.right - groupStart) * room;
            }
        }
        x = groupStart + (x - groupStart) * room;
        if (m_slots.size() > firstSlot) {
            spans.push_back(
                {(int)group, groupStart, x, firstSlot, m_slots.size()});
        }
        addAnchored((int)group, group);
    }
    // The clock goes among the other groups, which keep their order around
    // it, with a little more room on its sides. Kept in the middle (not on
    // the bar, where it's in the middle of the screen), it goes where both
    // sides are the most alike, and stays there until another place is
    // clearly better (no hopping back and forth as items grow and shrink);
    // otherwise where it is in the order. The groups slide to their new
    // places.
    // (Notches hiding together: one button, on the bell's, which keeps the
    // line too.)
    const float tail = MinimizableAt(CurrentPlace()) &&
                               g_settings.islandMinimizeButton &&
                               (!m_notchBase || g_settings.islandNotchEach)
                           ? iconSlot + spacing
                           : 0;
    const bool centered = PlaceClockCenter(orderPlace);
    bool slideGroups = false;
    float groupsBefore[kPillGroups];
    {
        size_t clockSpan = spans.size();
        for (size_t i = 0; i < spans.size(); i++) {
            if (spans[i].group == (int)PillGroup::Clock && !spans[i].tray) {
                clockSpan = i;
            }
        }
        if (clockSpan < spans.size()) {
            // (Tray icons put beside the clock go with it.)
            auto withClock = [&](size_t i) {
                return spans[i].tray && spans[i].group == (int)PillGroup::Clock;
            };
            std::vector<GroupSpan> others;
            for (size_t i = 0; i < spans.size(); i++) {
                if (i != clockSpan && !withClock(i)) {
                    others.push_back(spans[i]);
                }
            }
            const size_t count = others.size();
            auto groupAt = [&](size_t index) {
                return index < count ? others[index].group : kPillGroups;
            };
            // How much one side is longer than the other, with the clock
            // before others[index].
            float total = tail;
            for (const auto& span : others) {
                total += span.end - span.start;
            }
            auto unevenness = [&](size_t index) {
                float before = 0;
                for (size_t i = 0; i < index; i++) {
                    before += others[i].end - others[i].start;
                }
                return std::fabs(before - (total - before));
            };
            size_t at = clockSpan;
            if (centered) {
                size_t best = 0;
                for (size_t i = 1; i <= count; i++) {
                    if (unevenness(i) < unevenness(best)) {
                        best = i;
                    }
                }
                at = best;
                for (size_t i = 0; i <= count; i++) {
                    if (groupAt(i) == m_clockAfter &&
                        unevenness(i) <= unevenness(best) + iconSlot * 0.6f) {
                        at = i;
                    }
                }
                // The bar: the clock keeps its place among the items while
                // both sides fit in its halves (an item coming and going,
                // like the microphone in use, doesn't make the others hop
                // to its other side).
                if (m_bar) {
                    const float half =
                        BarLength(ReservedInsets()) / 2 - height;
                    const float clockHalf =
                        (spans[clockSpan].end - spans[clockSpan].start) / 2;
                    float before = 0;
                    for (size_t i = 0; i <= count; i++) {
                        if (groupAt(i) == m_clockAfter &&
                            before + clockHalf <= half &&
                            total - before + clockHalf <= half) {
                            at = i;
                        }
                        if (i < count) {
                            before += others[i].end - others[i].start;
                        }
                    }
                }
            }
            if (m_clockAfter >= 0 && groupAt(at) != m_clockAfter) {
                slideGroups = true;
            }
            m_clockAfter = groupAt(at);
            // Laid out again in that order.
            std::vector<IslandSlot> slots(m_slots.begin(),
                                          m_slots.begin() +
                                              (spans.empty()
                                                   ? 0
                                                   : spans.front().firstSlot));
            float place = 0;
            auto put = [&](const GroupSpan& span, float before) {
                const float shift = place + before - span.start;
                for (size_t i = span.firstSlot; i < span.endSlot; i++) {
                    IslandSlot slot = m_slots[i];
                    slot.rect.left += shift;
                    slot.rect.right += shift;
                    slots.push_back(slot);
                }
                place += before + span.end - span.start;
            };
            // (The room after it too, kept in the middle: alike on its sides.)
            for (size_t i = 0; i <= count; i++) {
                if (i == at) {
                    float room = i > 0 ? spacing : 0;
                    for (size_t j = 0; j < spans.size(); j++) {
                        if (j < clockSpan && withClock(j)) {
                            put(spans[j], room);
                            room = 0;
                        }
                    }
                    put(spans[clockSpan], room);
                    for (size_t j = clockSpan + 1; j < spans.size(); j++) {
                        if (withClock(j)) {
                            put(spans[j], 0);
                        }
                    }
                    if (i < count && centered) {
                        place += spacing;
                    }
                }
                if (i < count) {
                    put(others[i], 0);
                }
            }
            m_slots = std::move(slots);
            x = place;
        }
    }
    if (orderMoved) {
        slideGroups = true;
    }
    if (slideGroups) {
        for (int i = 0; i < kPillGroups; i++) {
            groupsBefore[i] = m_groupBase[i] < 0
                                  ? -1
                                  : m_groupBase[i] + (float)m_groupShift[i];
        }
    }
    // Minimized to a line against its edge: the arrow points to it.
    if (tail > 0) {
        const IslandPlace& place = CurrentPlace();
        WCHAR glyph = place.y > 0.999 ? kGlyphCollapseDown : kGlyphCollapse;
        if (place.y > 0.001 && place.y < 0.999) {
            glyph = place.x < 0.5 ? kGlyphPrevious : kGlyphNext;
        }
        addIcon(IslandItem::Minimize, {glyph});
    }
    float contentWidth = x - spacing;
    float clockMiddle = -1;
    for (const auto& slot : m_slots) {
        if (slot.item == IslandItem::Clock) {
            clockMiddle = (slot.rect.left + slot.rect.right) / 2;
        }
    }
    // Where each group starts, and each still sliding from where it was.
    for (int i = 0; i < kPillGroups; i++) {
        m_groupBase[i] = -1;
    }
    for (const auto& slot : m_slots) {
        if (slot.group >= 0 && m_groupBase[slot.group] < 0) {
            m_groupBase[slot.group] = slot.rect.left;
        }
    }
    // Another order (the app in front's own): the groups fade from their
    // places to the new ones, instead of sliding over each other. The clock
    // moving in the middle (items growing, the tray icons' list opening):
    // they slide.
    if (slideGroups) {
        // (A group going to the clock's other side fades there too, rather
        // than sliding over the clock.)
        const int clock = (int)PillGroup::Clock;
        const bool clockThere =
            groupsBefore[clock] >= 0 && m_groupBase[clock] >= 0;
        for (int i = 0; i < kPillGroups; i++) {
            if (groupsBefore[i] >= 0 && m_groupBase[i] >= 0 &&
                std::fabs(groupsBefore[i] - m_groupBase[i]) > 1) {
                m_groupShift[i] = groupsBefore[i] - m_groupBase[i];
                m_groupShiftVelocity[i] = 0;
                const bool crossed =
                    clockThere && i != clock &&
                    (groupsBefore[i] < groupsBefore[clock]) !=
                        (m_groupBase[i] < m_groupBase[clock]);
                if (orderMoved || crossed) {
                    m_groupSwap[i] = 1;
                    m_groupSwapVelocity[i] = 0;
                }
            }
        }
        StartAnimating();
    }
    // A group fading to its place (see m_groupSwap): drawn there coming in,
    // and where it was going (a copy); otherwise sliding (see
    // OnOrderChanged).
    std::vector<IslandSlot> leaving;
    for (auto& slot : m_slots) {
        if (slot.group < 0) {
            continue;
        }
        const double swap = m_groupSwap[slot.group];
        if (swap > 0) {
            IslandSlot old = slot;
            old.rect.left += (float)m_groupShift[slot.group];
            old.rect.right += (float)m_groupShift[slot.group];
            old.alpha = (float)std::clamp(swap, 0.0, 1.0);
            leaving.push_back(old);
            slot.alpha = 1 - old.alpha;
            continue;
        }
        const float shift = (float)m_groupShift[slot.group];
        slot.rect.left += shift;
        slot.rect.right += shift;
    }
    m_slots.insert(m_slots.end(), leaving.begin(), leaving.end());
    // Longer than its screen has room for (many items, a big size): it stops
    // short of the screen's edges, and the wheel slides along the rest (see
    // ScrollPill); the items cut at its ends fade there (see Render).
    m_pillOverflow = 0;
    if (!m_bar) {
        const RECT insets = TaskbarInsets();
        float along =
            m_vertical ? (float)(m_monitor.bottom - m_monitor.top - insets.top -
                                 insets.bottom)
                       : (float)(m_monitor.right - m_monitor.left -
                                 insets.left - insets.right);
        // (Its items show between its round ends, where they're cut.)
        const float ends = std::max(0.0f, height / 2 - padding);
        float room = along - std::round(GapShown() * m_dpiScale) * 2 -
                     padding * 2 - ends * 2 - std::round(24 * scale);
        // A notch: as long as its neighbors leave (they almost touch).
        if (m_notch >= 0) {
            const float wanted = std::max(contentWidth + padding * 2,
                                          std::round(kNotchMinWidth * scale));
            if (std::fabs(wanted - m_notchWanted) > 0.5f) {
                m_notchWanted = wanted;
                // (The others' room changes too.)
                for (Island* notch : NotchSet()) {
                    if (notch != this) {
                        notch->StartAnimating();
                    }
                }
            }
            room = NotchBodyMax(scale) - padding * 2 - ends * 2;
        }
        if (room > iconSlot * 2 && contentWidth > room) {
            m_pillOverflow = contentWidth - room;
            m_pillScrollTarget =
                std::clamp(m_pillScrollTarget, 0.0, (double)m_pillOverflow);
            m_pillScroll =
                std::clamp(m_pillScroll, 0.0, (double)m_pillOverflow);
            const float scroll = (float)m_pillScroll;
            std::erase_if(m_slots, [room, scroll](const IslandSlot& slot) {
                return slot.rect.right - scroll <= 0 ||
                       slot.rect.left - scroll >= room;
            });
            for (auto& slot : m_slots) {
                slot.rect.left += ends - scroll;
                slot.rect.right += ends - scroll;
            }
            contentWidth = room + ends * 2;
        }
    }
    if (m_pillOverflow <= 0 && !m_bar) {
        m_pillScroll = m_pillScrollTarget = m_pillScrollVelocity = 0;
    }

    if (m_bar) {
        const RECT insets = ReservedInsets();
        const float width = BarLength(insets);
        // More items than the bar has room for: like a long pill, they slide
        // along under the wheel between its ends (see ScrollPill), and fade
        // at an end with more beyond it.
        const float barEnds = height / 2;
        const float barRoom = width - barEnds * 2;
        if (barRoom > iconSlot * 2 && contentWidth > barRoom) {
            m_pillOverflow = contentWidth - barRoom;
            m_pillScrollTarget =
                std::clamp(m_pillScrollTarget, 0.0, (double)m_pillOverflow);
            m_pillScroll =
                std::clamp(m_pillScroll, 0.0, (double)m_pillOverflow);
            const float scroll = (float)m_pillScroll;
            std::erase_if(m_slots, [barRoom, scroll](const IslandSlot& slot) {
                return slot.rect.right - scroll <= 0 ||
                       slot.rect.left - scroll >= barRoom;
            });
            for (auto& slot : m_slots) {
                slot.rect.left += barEnds - scroll;
                slot.rect.right += barEnds - scroll;
            }
        } else {
            m_pillScroll = m_pillScrollTarget = m_pillScrollVelocity = 0;
        }
        const float shift =
            centered && clockMiddle >= 0
                // In the screen's middle, as far as the items fit.
                ? std::clamp(std::round(width / 2 - clockMiddle), padding,
                             std::max(padding, width - padding - contentWidth))
                : width - padding - contentWidth;
        // At the screen's corners (an option): the items before the clock
        // against the left edge, the ones after it against the right.
        // (Off: nothing is before or after it.)
        float clockLeft = std::numeric_limits<float>::lowest();
        float clockRight = std::numeric_limits<float>::max();
        if (centered && clockMiddle >= 0 && g_settings.islandBarCorners) {
            clockLeft = std::numeric_limits<float>::max();
            clockRight = std::numeric_limits<float>::lowest();
            for (const auto& slot : m_slots) {
                if (slot.group == (int)PillGroup::Clock) {
                    clockLeft = std::min(clockLeft, slot.rect.left);
                    clockRight = std::max(clockRight, slot.rect.right);
                }
            }
        }
        for (auto& slot : m_slots) {
            if (m_pillOverflow > 0) {
                break;
            }
            const float slotShift =
                slot.rect.right <= clockLeft ? padding
                : slot.rect.left >= clockRight
                    ? std::max(shift, width - padding - contentWidth)
                    : shift;
            slot.rect.left += slotShift;
            slot.rect.right += slotShift;
        }
        const float corner = BarInnerCorner();
        // Upright on a side: its items from top to bottom, the window
        // against its edge, wider for a banner opening beside it (towards
        // the middle) and for the inner corners. (Its strip as wide as an
        // upright pill: the clock's two lines fit.)
        if (m_vertical) {
            float windowWidth = height + corner;
            if ((m_bannerWanted || m_bannerAmount > 0) && !BannerPlaced()) {
                windowWidth = std::max(
                    windowWidth,
                    height + std::round(kPanelGap * scale) +
                        std::ceil(BannerWidthAt(scale) * 1.08f + 2));
            }
            const float pillLeft = m_sideLeft ? 0 : windowWidth - height;
            for (auto& slot : m_slots) {
                slot.rect = {pillLeft, slot.rect.left, pillLeft + height,
                             slot.rect.right};
            }
            m_size = {(LONG)windowWidth, (LONG)width};
            m_position = {m_sideLeft ? m_monitor.left + insets.left
                                     : m_monitor.right - insets.right -
                                           (LONG)windowWidth,
                          m_monitor.top + insets.top};
            m_pill = {pillLeft, 0, pillLeft + height, width};
            return;
        }
        // A banner: the window grows down for it, out of the bar's middle
        // (its transparent parts let clicks through; the bar keeps its
        // strip, see SetAppBarPosition).
        float windowHeight = height + corner;
        if ((m_bannerWanted || m_bannerAmount > 0) && !BannerPlaced()) {
            const float bannerHeight =
                kBannerHeight +
                (m_banner.reply.CanReply() || BannerHasActions()
                     ? kBannerReplyHeight
                     : 0);
            windowHeight = std::max(
                windowHeight,
                std::ceil(height + std::round(kPanelGap * scale) +
                          (bannerHeight * scale * BannerSizeFactor() +
                           BannerExtraRoom()) *
                              1.08f +
                          2));
        }
        m_size = {(LONG)width, (LONG)windowHeight};
        m_position = {m_monitor.left + insets.left, m_monitor.top + insets.top};
        m_pill = {0, 0, width, height};
        return;
    }

    if (m_vertical) {
        // Upright: the items laid out along its length (so far along x) go
        // from top to bottom; it sits against its edge, and the window is
        // big enough for a banner opening towards the middle. Between it and
        // its edge, the room for the minimized line (as above the pill).
        const float length = contentWidth + padding * 2;
        const float edgeGap = std::round(kPillTopGap * scale);
        float windowWidth = height + edgeGap;
        // (Long enough for the strip that brings the line back: its
        // transparent parts let clicks through.)
        float windowHeight =
            std::ceil(std::max(length, ActivationLength(scale)) / 2) * 2 + 2;
        if ((m_bannerWanted || m_bannerAmount > 0) && !BannerPlaced()) {
            const float bannerHeight =
                kBannerHeight +
                (m_banner.reply.CanReply() || BannerHasActions()
                     ? kBannerReplyHeight
                     : 0);
            windowWidth =
                std::max(windowWidth,
                         std::ceil(BannerWidthAt(scale) * 1.08f + 2) + edgeGap);
            windowHeight = std::max(
                windowHeight,
                std::ceil((bannerHeight * scale * BannerSizeFactor() +
                           BannerExtraRoom()) *
                          1.08f / 2) * 2 +
                    2);
        }
        if (m_gearShownAt) {
            windowWidth = std::max(
                windowWidth,
                height + edgeGap +
                    std::ceil((std::round(kGearBubbleGap * scale) +
                               GearBubbleWidth()) *
                              1.1f) +
                    2);
        }
        const float pillLeft =
            m_sideLeft ? edgeGap : windowWidth - height - edgeGap;
        const float shift = (windowHeight - length) / 2;
        for (auto& slot : m_slots) {
            slot.rect = {pillLeft, slot.rect.left + padding + shift,
                         pillLeft + height, slot.rect.right + padding + shift};
        }
        m_size = {(LONG)windowWidth, (LONG)windowHeight};
        m_pill = {pillLeft, shift, pillLeft + height, shift + length};
        const float margin = std::round(GapShown() * m_dpiScale);
        const RECT insets = TaskbarInsets();
        m_roomLeft = (float)(m_monitor.left + insets.left) + margin;
        m_roomTop = (float)(m_monitor.top + insets.top) + margin;
        m_roomWidth = std::max(
            0.0f, (float)(m_monitor.right - m_monitor.left - insets.left -
                          insets.right) -
                      margin * 2 - height);
        m_roomHeight = std::max(
            0.0f, (float)(m_monitor.bottom - m_monitor.top - insets.top -
                          insets.bottom) -
                      margin * 2 - length);
        float pillX = m_roomLeft + (float)m_placeX * m_roomWidth;
        const float pillY = m_roomTop + (float)m_placeY * m_roomHeight;
        // Minimizing, the window goes to the screen's edge, where the line
        // is.
        const float folded = 1 - (float)std::clamp(m_expand, 0.0, 1.0);
        pillX += (m_sideLeft ? 1 : -1) * (edgeGap - margin) * folded;
        m_position = {(LONG)std::lround(pillX - pillLeft),
                      (LONG)std::lround(pillY - shift)};
        return;
    }

    if (m_notch >= 0) {
        LayoutNotch(contentWidth, height, padding, scale);
        return;
    }

    for (auto& slot : m_slots) {
        slot.rect.left += padding;
        slot.rect.right += padding;
    }
    const float pillWidth = contentWidth + padding * 2;
    // The window is as big as the expanded pill (plus the gap above it, or
    // under it on the lower half, where the minimized line is), or the banner
    // with its bounce; its transparent parts let clicks through.
    const float top = std::round(kPillTopGap * scale);
    // (Wide enough for the strip that brings the line back: its transparent
    // parts let clicks through.)
    float windowWidth =
        std::ceil(std::max(pillWidth, ActivationLength(scale)) / 2) * 2 + 2;
    float windowHeight = top + height;
    if ((m_bannerWanted || m_bannerAmount > 0) && !BannerPlaced()) {
        windowWidth = std::max(
            windowWidth, std::ceil(BannerWidthAt(scale) * 1.08f / 2) * 2 + 2);
        const float bannerHeight =
            kBannerHeight +
            (m_banner.reply.CanReply() || BannerHasActions()
                 ? kBannerReplyHeight
                 : 0);
        windowHeight = std::max(
            windowHeight,
            std::ceil(top + (bannerHeight * scale * BannerSizeFactor() +
                             BannerExtraRoom()) *
                                1.08f +
                      2));
    }
    if (m_gearShownAt) {
        // (As wide as its text too: a long one isn't cut.)
        windowWidth = std::max(windowWidth,
                               std::ceil(GearBubbleWidth() * 1.1f / 2) * 2 + 2);
        if (!m_minimized) {
            windowHeight = std::max(
                windowHeight,
                top + height +
                    std::ceil((kGearBubbleGap + kGearBubbleHeight) * scale *
                              1.1f) +
                    2);
        } else {
            // Minimized, it comes out over the line (see GearBubbleRect):
            // all of it, also above a low pill.
            windowHeight = std::max(
                windowHeight,
                top + std::ceil(kGearBubbleHeight * scale * 1.1f) + 2);
        }
    }
    const float shift = (windowWidth - pillWidth) / 2;
    // On the lower half, the pill is at the window's bottom (banners open
    // upwards), with the line's room under it.
    const float pillTop = AtBottom() ? windowHeight - height - top : top;
    for (auto& slot : m_slots) {
        slot.rect.left += shift;
        slot.rect.right += shift;
        slot.rect.top += pillTop;
        slot.rect.bottom += pillTop;
    }
    m_size = {(LONG)windowWidth, (LONG)windowHeight};
    m_pill = {shift, pillTop, shift + pillWidth, pillTop + height};

    // Its place: from the top (where it's always been) to the bottom, left to
    // right, inside the monitor, the chosen room from its edges (see
    // islandGap). Minimizing, the window goes to the screen's top (or
    // bottom), where the line is.
    const float margin = std::round(GapShown() * m_dpiScale);
    const RECT insets = TaskbarInsets();
    m_roomLeft = (float)(m_monitor.left + insets.left) + margin;
    m_roomTop = (float)(m_monitor.top + insets.top) + margin;
    m_roomWidth =
        std::max(0.0f, (float)(m_monitor.right - m_monitor.left - insets.left -
                               insets.right) -
                           margin * 2 - pillWidth);
    m_roomHeight =
        std::max(0.0f, (float)(m_monitor.bottom - m_monitor.top - insets.top -
                               insets.bottom) -
                           margin * 2 - height);
    const float pillX = m_roomLeft + (float)m_placeX * m_roomWidth;
    float pillY = m_roomTop + (float)m_placeY * m_roomHeight;
    const float folded = 1 - (float)std::clamp(m_expand, 0.0, 1.0);
    pillY += (AtBottom() ? -1 : 1) * (top - margin) * folded;
    m_position = {(LONG)std::lround(pillX - shift),
                  (LONG)std::lround(pillY - pillTop)};
    // Cut at its monitor's edge (the gap over the pill is taller than the
    // room from the edge): past it, another monitor there would show the
    // window's rows (over its taskbar or dock). The pill stays where it is.
    if (const LONG over = m_monitor.top - m_position.y; over > 0) {
        m_position.y += over;
        m_size.cy -= over;
        m_pill.top -= over;
        m_pill.bottom -= over;
        for (auto& slot : m_slots) {
            slot.rect.top -= over;
            slot.rect.bottom -= over;
        }
    }
    if (const LONG over = m_position.y + m_size.cy - m_monitor.bottom;
        over > 0) {
        m_size.cy -= over;
    }
}

// A notch's longest body: as long as its items want, until it almost
// touches its neighbors (a small gap between their curves); when they all
// want more, each keeps its share of the screen (the middle one stays in
// the middle). From the others' wanted lengths (their last layouts).
float Island::NotchBodyMax(float scale) {
    const RECT insets = TaskbarInsets();
    const float width =
        (float)(m_monitor.right - m_monitor.left - insets.left - insets.right);
    const float curve = NotchShoulderWidth();
    const float gap = std::round(kNotchTouchGap * scale);
    float wanted[3] = {0, 0, 0};
    for (Island* notch : NotchSet()) {
        if (notch->m_notch >= 0 && notch->m_notch < 3) {
            wanted[notch->m_notch] = notch->m_notchWanted;
        }
    }
    if (m_notchCount <= 1) {
        return std::max(0.0f, width - curve * 2 - gap * 2);
    }
    if (m_notchCount == 2) {
        // Left and right: what the other one takes of its share is left.
        const float both = width - curve * 2 - gap;
        const float other = wanted[m_notch == 0 ? 2 : 0];
        return std::max(0.0f, both - std::min(other, both / 2));
    }
    // Three: the middle one between the sides' needs (up to their shares),
    // the sides on what the middle one leaves of each half.
    const float all = width - curve * 4 - gap * 2;
    const float share = all / 3;
    const float sides =
        std::max(std::min(wanted[0], share), std::min(wanted[2], share));
    const float middleMax = std::max(0.0f, all - sides * 2);
    if (m_notch == 1) {
        return middleMax;
    }
    const float middle = std::min(std::max(wanted[1], 0.0f), middleMax);
    return std::max(0.0f, (width - middle) / 2 - curve * 2 - gap);
}

// A notch: against the top edge (the left and right ones touching the
// screen's corners), as wide as its items (small when it has none), its
// curves beside it. The window is as big as a banner or the settings bubble
// coming out of it too, kept on the screen; its transparent parts let clicks
// through.
void Island::LayoutNotch(float contentWidth,
                         float height,
                         float padding,
                         float scale) {
    const float pillWidth = std::max(contentWidth + padding * 2,
                                     std::round(kNotchMinWidth * scale));
    const float inside =
        padding + std::round((pillWidth - contentWidth - padding * 2) / 2);
    const float curve = NotchShoulderWidth();
    const RECT insets = TaskbarInsets();
    const float screenLeft = (float)(m_monitor.left + insets.left);
    const float screenRight = (float)(m_monitor.right - insets.right);
    const float screenTop = (float)(m_monitor.top + insets.top);
    const float bodyLeft =
        m_notch == 0   ? screenLeft
        : m_notch == 2 ? screenRight - pillWidth
                       : std::round((screenLeft + screenRight - pillWidth) / 2);
    float left = bodyLeft - (m_notch == 0 ? 0 : curve);
    float right = bodyLeft + pillWidth + (m_notch == 2 ? 0 : curve);
    // (At a corner, its curve goes down the screen's side.)
    float windowHeight = height + (m_notch == 1 ? 0 : curve);
    // Something wider coming out of it: in its middle, as far as the screen
    // lets it.
    auto makeRoom = [&](float width) {
        const float from = std::clamp(
            std::round(bodyLeft + pillWidth / 2 - width / 2), screenLeft,
            std::max(screenLeft, screenRight - width));
        left = std::min(left, from);
        right = std::max(right, from + width);
    };
    // The strip that brings the line back (cut at the screen's corners).
    makeRoom(ActivationLength(scale));
    if ((m_bannerWanted || m_bannerAmount > 0) && !BannerPlaced()) {
        const float bannerHeight =
            kBannerHeight +
            (m_banner.reply.CanReply() || BannerHasActions()
                 ? kBannerReplyHeight
                 : 0);
        makeRoom(std::ceil(std::max(BannerWidthAt(scale), pillWidth) * 1.08f) +
                 2);
        windowHeight = std::max(
            windowHeight,
            std::ceil((bannerHeight * scale * BannerSizeFactor() +
                       BannerExtraRoom()) *
                          1.08f +
                      2) +
                curve);
    }
    if (m_gearShownAt) {
        makeRoom(std::ceil(GearBubbleWidth() * 1.1f) + 2);
        if (!m_minimized) {
            windowHeight = std::max(
                windowHeight,
                height + std::ceil((kGearBubbleGap + kGearBubbleHeight) *
                                   scale * 1.1f) +
                    2);
        } else {
            // (Minimized: over the line, see GearBubbleRect.)
            windowHeight = std::max(
                windowHeight, std::ceil(kGearBubbleHeight * scale * 1.1f) + 2);
        }
    }
    left = std::floor(left);
    right = std::ceil(right);
    const float pillLeft = bodyLeft - left;
    for (auto& slot : m_slots) {
        slot.rect.left += pillLeft + inside;
        slot.rect.right += pillLeft + inside;
    }
    m_size = {(LONG)(right - left), (LONG)std::ceil(windowHeight)};
    m_position = {(LONG)left, (LONG)screenTop};
    m_pill = {pillLeft, 0, pillLeft + pillWidth, height};
    m_notchEdgeLeft = screenLeft - left;
    m_notchEdgeRight = screenRight - left;
    // (Not dragged: it stays against the edge.)
    m_roomLeft = screenLeft;
    m_roomTop = screenTop;
    m_roomWidth = 0;
    m_roomHeight = 0;
}

// Tells the taskbar where each tray icon is (see OnTrayIconRectRequest): its
// slot, or the arrow while it's in the closed list.
void Island::PublishTrayPlaces() {
    // (Until the mouse comes, the main island's; with notches, the main
    // monitor's one with the tray.)
    const bool usual = m_notch < 0
                           ? m_main
                           : InThisNotch(PillGroup::Tray) &&
                                 (m_main || m_notchBase == MainIsland());
    if (g_trayIsland ? g_trayIsland != this : !usual) {
        return;
    }
    D2D1_RECT_F arrow = m_pill;
    for (const auto& slot : m_slots) {
        if (slot.item == IslandItem::Overflow) {
            arrow = slot.rect;
        }
    }
    auto toScreen = [this](const D2D1_RECT_F& r) {
        return RECT{m_position.x + (LONG)r.left, m_position.y + (LONG)r.top,
                    m_position.x + (LONG)r.right,
                    m_position.y + (LONG)r.bottom};
    };
    std::vector<TrayIconPlace> places;
    for (const auto& icon : m_trayIcons) {
        RECT rect = toScreen(arrow);
        for (const auto& slot : m_slots) {
            if (slot.item == IslandItem::TrayIcon && slot.text == icon.key &&
                slot.rect.right - slot.rect.left > 4) {
                rect = toScreen(slot.rect);
            }
        }
        // (Put beside an item in another notch: there.)
        for (const Island* other : g_islands) {
            if (other == this || !EqualRect(&other->m_monitor, &m_monitor)) {
                continue;
            }
            for (const auto& slot : other->m_slots) {
                if (slot.item == IslandItem::TrayIcon && !slot.inList &&
                    slot.text == icon.key &&
                    slot.rect.right - slot.rect.left > 4) {
                    rect = {other->m_position.x + (LONG)slot.rect.left,
                            other->m_position.y + (LONG)slot.rect.top,
                            other->m_position.x + (LONG)slot.rect.right,
                            other->m_position.y + (LONG)slot.rect.bottom};
                }
            }
        }
        places.push_back(
            {icon.owner, icon.id, icon.guid, icon.hasGuid, rect});
    }
    // To explorer (only when they changed: this runs with every frame).
    IpcWriter writer;
    writer.Put<UINT32>((UINT32)places.size());
    for (const auto& place : places) {
        writer.Put<UINT64>((UINT64)(ULONG_PTR)place.owner);
        writer.Put<UINT32>(place.id);
        writer.Put<GUID>(place.guid);
        writer.Put<BYTE>(place.hasGuid);
        writer.Put<RECT>(place.rect);
    }
    if (writer.bytes != m_sentTrayPlaces) {
        m_sentTrayPlaces = writer.bytes;
        QueueTrayPlaces(m_hwnd, writer.bytes);
    }
}

// The banner, open, in the window.
D2D1_RECT_F Island::BannerRect() const {
    // Its own width (the pill shrinks into it when wider), or the pill's when
    // the panels follow the island's width (not the bar's).
    const float width =
        g_settings.islandPanelsFollow && !m_bar
            ? std::max(std::round(BannerWidthAt(m_scale)),
                       m_pill.right - m_pill.left)
            : std::round(BannerWidthAt(m_scale));
    const float open = (float)std::clamp(m_bannerOpen, 0.0, 1.05);
    const float height =
        std::round((kBannerHeight + kBannerReplyHeight * open) *
                   BannerScale()) +
        BannerTextExtra() + BannerReplyExtra() * std::min(open, 1.0f);
    if (BannerPlaced()) {
        return PlacedBannerRect(std::round(BannerWidthAt(m_scale)), height);
    }
    // Upright: towards the middle of the screen, from the pill's edge (from
    // beside the bar).
    if (m_vertical) {
        const float bannerWidth = std::round(BannerWidthAt(m_scale));
        const float gap = m_bar ? std::round(kPanelGap * m_scale) : 0;
        const float left =
            m_sideLeft ? (m_bar ? m_pill.right + gap : m_pill.left)
                       : (m_bar ? m_pill.left - gap : m_pill.right) -
                             bannerWidth;
        const float top = std::clamp(
            std::round((m_pill.top + m_pill.bottom - height) / 2), 0.0f,
            std::max(0.0f, (float)m_size.cy - height));
        return {left, top, left + bannerWidth, top + height};
    }
    // (Kept in the window: a notch at a screen's corner has it beside.)
    const float centerX =
        m_bar ? (m_pill.left + m_pill.right) / 2
              : std::clamp((m_pill.left + m_pill.right) / 2, width / 2,
                           std::max(width / 2, (float)m_size.cx - width / 2));
    // Under the bar, in its middle; upwards on the lower half of the screen.
    const float top = m_bar ? m_pill.bottom + std::round(kPanelGap * m_scale)
                      : AtBottom() ? m_pill.bottom - height
                                   : m_pill.top;
    return {std::round(centerX - width / 2), top,
            std::round(centerX - width / 2) + width, top + height};
}

// The banner's contents (the app's icon, its name and the time, the title
// and the text, the reply box and the buttons), as opaque as `contents`: in
// the island's window, or in its own (see RenderPlacedBanner).
void Island::DrawBannerContents(ID2D1RenderTarget* target,
                                ID2D1SolidColorBrush* brush,
                                float contents) {
    if (!m_bannerAppFormat || !m_bannerTimeFormat || !m_bannerTitleFormat ||
        !m_bannerTextFormat) {
        return;
    }
    // At its own size (islandBannerSize); the icons of the font, made
    // for the pill, grown around their middles.
    const float bannerScale = BannerScale();
    auto bannerGlyph = [&](WCHAR glyph, IDWriteTextFormat* format,
                           const D2D1_RECT_F& rect) {
        const float grow = BannerSizeFactor();
        D2D1_MATRIX_3X2_F before;
        target->GetTransform(&before);
        target->SetTransform(
            D2D1::Matrix3x2F::Scale(
                grow, grow,
                {(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2}) *
            before);
        DrawGlyph(target, &glyph, 1, format, rect, brush);
        target->SetTransform(before);
    };
    const D2D1_RECT_F open = BannerRect();
    const float inner = std::round(16 * bannerScale);
    const float iconSize = std::round(38 * bannerScale);
    const float middle = open.top + std::round(kBannerHeight * bannerScale) / 2;
    const D2D1_RECT_F icon{open.left + inner, middle - iconSize / 2,
                           open.left + inner + iconSize, middle + iconSize / 2};
    if (m_banner.appId.empty()) {
        // A reminder: a calendar in a red circle; a drive: a USB plug in
        // a blue one; a status notice: its glyph in its color.
        const bool drive = !m_banner.openPath.empty();
        const bool notice = m_banner.glyph != 0;
        brush->SetColor(notice  ? RgbColor(m_banner.glyphColor, contents)
                        : drive ? Accent(contents)
                                : D2D1::ColorF(1.0f, 0.27f, 0.23f, contents));
        target->FillEllipse(
            {{(icon.left + icon.right) / 2, (icon.top + icon.bottom) / 2},
             iconSize / 2,
             iconSize / 2},
            brush);
        const WCHAR glyph = notice  ? m_banner.glyph
                            : drive ? (WCHAR)0xE88E
                                    : kGlyphCalendar;
        brush->SetColor(notice  ? D2D1::ColorF(1, 1, 1, contents)
                        : drive ? AccentInk(contents)
                                : Fg(contents));
        bannerGlyph(glyph, m_iconFormat.get(), icon);
    } else if (ID2D1Bitmap* image = GetBannerImage()) {
        // The picture, filling its place (round, or with round corners),
        // and the app's icon small at its corner, like Windows shows it.
        const D2D1_SIZE_F imageSize = image->GetSize();
        const float fill =
            iconSize /
            std::max(1.0f, std::min(imageSize.width, imageSize.height));
        if (winrt::com_ptr<ID2D1BitmapBrush> imageBrush =
                SmoothBitmapBrush(target, image)) {
            imageBrush->SetOpacity(contents);
            imageBrush->SetTransform(
                D2D1::Matrix3x2F::Scale(fill, fill) *
                D2D1::Matrix3x2F::Translation(
                    (icon.left + icon.right - imageSize.width * fill) / 2,
                    (icon.top + icon.bottom - imageSize.height * fill) / 2));
            const float radius = m_banner.imageCircle
                                     ? iconSize / 2
                                     : std::round(8 * bannerScale);
            target->FillRoundedRectangle({icon, radius, radius},
                                         imageBrush.get());
        }
        if (ID2D1Bitmap* bitmap = GetAppBitmap(m_banner.appId)) {
            const float small = std::round(iconSize * 0.45f);
            const float outside = std::round(3 * bannerScale);
            const D2D1_RECT_F corner{
                icon.right - small + outside, icon.bottom - small + outside,
                icon.right + outside, icon.bottom + outside};
            DrawBitmapSmooth(target, bitmap, corner, contents);
        }
    } else if (ID2D1Bitmap* bitmap = GetAppBitmap(m_banner.appId)) {
        DrawBitmapSmooth(target, bitmap, icon, contents);
    }
    const float left = icon.right + std::round(12 * bannerScale);
    const float right = open.right - inner - std::round(4 * bannerScale);
    const float line = std::round(18 * bannerScale);
    // The title and the text (two lines) centered by the icon.
    const float top = middle - line;
    // The time gives way to the "x" under the mouse (fading, the "x"
    // growing in); the "x" lights up under the mouse.
    const float closeShown =
        m_hoverFades.Get(HoverKey(1000, 1), BannerHovered() || m_bannerTyping);
    const float closeLit =
        m_hoverFades.Get(HoverKey(1000, 2), m_hover == IslandItem::BannerClose);
    if (closeShown > 0.01f) {
        const D2D1_RECT_F close = BannerCloseRect();
        const float closeRadius =
            (close.right - close.left) / 2 * (0.6f + 0.4f * closeShown);
        brush->SetColor(Fg((0.16f + 0.16f * closeLit) * contents * closeShown));
        GlassFillEllipse(
            target,
            {{(close.left + close.right) / 2, (close.top + close.bottom) / 2},
             closeRadius,
             closeRadius},
            brush);
        brush->SetColor(Fg(0.95f * contents * closeShown));
        bannerGlyph(kGlyphClose, m_smallIconFormat.get(), close);
        // The arrow that minimizes it.
        const D2D1_RECT_F minimize = BannerMinimizeRect();
        const float minimizeLit = m_hoverFades.Get(
            HoverKey(1000, 7), m_hover == IslandItem::BannerMinimize);
        brush->SetColor(
            Fg((0.16f + 0.16f * minimizeLit) * contents * closeShown));
        GlassFillEllipse(target,
                         {{(minimize.left + minimize.right) / 2,
                           (minimize.top + minimize.bottom) / 2},
                          closeRadius,
                          closeRadius},
                         brush);
        brush->SetColor(Fg(0.95f * contents * closeShown));
        bannerGlyph(kGlyphCollapse, m_smallIconFormat.get(), minimize);
    }
    if (closeShown < 0.99f) {
        brush->SetColor(Fg(0.55f * contents * (1 - closeShown)));
        PCWSTR now = Tr(L"now", L"agora");
        target->DrawText(now, (UINT32)wcslen(now), m_bannerTimeFormat.get(),
                         {left, top, right, top + line}, brush);
    }
    // Only that there's one, when the content is hidden.
    const bool content = g_showNotificationContent;
    const std::wstring title =
        content ? m_banner.title
                : std::wstring(Tr(L"New notification", L"Nova notificação"));
    const std::wstring text =
        content ? m_banner.text
                : std::wstring(Tr(L"Content hidden", L"Conteúdo oculto"));
    brush->SetColor(Fg(0.95f * contents));
    // A title too long: cut with "...", and under the mouse it scrolls
    // to its end and back (#6200; see m_bannerMarquee).
    const D2D1_RECT_F titleRect{left, top, right - std::round(50 * bannerScale),
                                top + line};
    winrt::com_ptr<IDWriteTextLayout> scrolling;
    m_bannerMarqueeDistance = 0;
    if (!BannerHovered()) {
        m_bannerMarqueeStart = 0;
    } else if (AnimationLevel() == 0 && contents > 0.99f &&
               SUCCEEDED(m_dwrite->CreateTextLayout(
                   title.c_str(), (UINT32)title.size(),
                   m_bannerTitleFormat.get(), 10000, line, scrolling.put()))) {
        scrolling->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        DWRITE_TEXT_METRICS metrics{};
        scrolling->GetMetrics(&metrics);
        m_bannerMarqueeDistance =
            std::max(0.0f, metrics.widthIncludingTrailingWhitespace -
                               (titleRect.right - titleRect.left));
        if (!m_bannerMarqueeStart) {
            m_bannerMarqueeStart = NowSeconds();
        }
    }
    if (m_bannerMarqueeDistance > 0.5f) {
        m_bannerMarquee = true;
        const float shift =
            (float)MarqueeShift(NowSeconds() - m_bannerMarqueeStart,
                                m_bannerMarqueeDistance, bannerScale);
        target->PushAxisAlignedClip(titleRect, D2D1_ANTIALIAS_MODE_ALIASED);
        target->DrawTextLayout({titleRect.left - shift, titleRect.top},
                               scrolling.get(), brush,
                               D2D1_DRAW_TEXT_OPTIONS_NO_SNAP |
                                   D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        target->PopAxisAlignedClip();
    } else {
        target->DrawText(title.c_str(), (UINT32)title.size(),
                         m_bannerTitleFormat.get(), titleRect, brush,
                         D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    }
    brush->SetColor(Fg((content ? 0.75f : 0.45f) * contents));
    // Under the mouse, on up to three lines (opening as it grows).
    const float textExtra = BannerTextExtra();
    winrt::com_ptr<IDWriteTextLayout> wrapped =
        content && textExtra > 0.5f
            ? WrappedLayout(m_dwrite.get(), m_bannerTextFormat.get(),
                            m_banner.fullText, right - left, line * 3, line)
            : nullptr;
    if (wrapped) {
        target->PushAxisAlignedClip(
            {left, top + line, right, top + line * 2 + textExtra},
            D2D1_ANTIALIAS_MODE_ALIASED);
        target->DrawTextLayout({left, top + line}, wrapped.get(), brush,
                               D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        target->PopAxisAlignedClip();
    } else {
        target->DrawText(text.c_str(), (UINT32)text.size(),
                         m_bannerTextFormat.get(),
                         {left, top + line, right, top + line * 2}, brush,
                         D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    }

    // The reply box and "Open", under the mouse.
    const float row =
        contents * (float)std::clamp((m_bannerOpen - 0.3) / 0.7, 0.0, 1.0);
    if (row > 0.01f && BannerCanReply()) {
        D2D1_RECT_F field, send, openButton;
        BannerParts(&field, &send, &openButton);
        const float fieldRadius = std::min((field.bottom - field.top) / 2,
                                           std::round(16 * bannerScale));
        brush->SetColor(
            Fg((0.12f + 0.06f * m_hoverFades.Get(
                                    HoverKey(1000, 3),
                                    m_bannerTyping ||
                                        m_hover == IslandItem::BannerReply)) *
               row));
        GlassFill(target, {field, fieldRadius, fieldRadius}, brush);
        if (m_bannerTyping) {
            brush->SetColor(Accent(row));
            target->DrawRoundedRectangle(
                {{field.left + 0.75f, field.top + 0.75f, field.right - 0.75f,
                  field.bottom - 0.75f},
                 fieldRadius,
                 fieldRadius},
                brush, 1.5f);
        }
        // What's typed, wrapping onto more lines (the box grows, up
        // to five; then the end stays in view); the hint on one line.
        const float textLeft = field.left + std::round(12 * bannerScale);
        const float base = std::round(32 * bannerScale);
        const float replyLine = std::round(17 * bannerScale);
        const float padTop = std::round((base - replyLine) / 2);
        const bool empty = m_bannerReplyText.empty();
        float caretX = textLeft;
        float caretTop = field.top + padTop;
        float caretHeight = replyLine;
        if (empty) {
            // What was typed before (see IslandToast::draft), or the
            // hint.
            std::wstring hint = !m_banner.draft.empty() ? m_banner.draft
                                : m_banner.reply.placeholder.empty()
                                    ? std::wstring(Tr(L"Reply", L"Responder"))
                                    : m_banner.reply.placeholder;
            std::replace(hint.begin(), hint.end(), L'\n', L' ');
            brush->SetColor(Fg(0.45f * row));
            target->DrawText(
                hint.c_str(), (UINT32)hint.size(), m_bannerTextFormat.get(),
                {textLeft, field.top, send.left, field.top + base}, brush);
        } else {
            const D2D1_RECT_F textRect{textLeft, field.top + padTop, send.left,
                                       field.bottom - padTop};
            const WrappedField wrapped = LayOutEdit(
                m_dwrite.get(), m_bannerTextFormat.get(), m_bannerReplyText,
                m_bannerEdit, textRect.right - textRect.left,
                textRect.bottom - textRect.top, replyLine);
            if (wrapped.layout) {
                target->PushAxisAlignedClip(textRect,
                                            D2D1_ANTIALIAS_MODE_ALIASED);
                DrawTextSelection(
                    target, brush, wrapped.layout.get(),
                    {textRect.left, textRect.top - wrapped.scroll},
                    m_bannerEdit, row);
                brush->SetColor(Fg(0.95f * row));
                target->DrawTextLayout(
                    {textRect.left, textRect.top - wrapped.scroll},
                    wrapped.layout.get(), brush,
                    D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
                target->PopAxisAlignedClip();
                caretX = textRect.left + wrapped.caretX;
                caretTop = textRect.top - wrapped.scroll + wrapped.caretY;
                caretHeight = wrapped.caretHeight;
                m_bannerEdit.origin = {textRect.left,
                                       textRect.top - wrapped.scroll};
            }
        }

        // The caret, only where the box shows it (scrolled away, not).
        const float caretBottom = caretTop + caretHeight;
        if (m_bannerTyping && caretTop >= field.top &&
            caretBottom <= field.bottom + 1) {
            brush->SetColor(CaretColor(row));
            target->FillRectangle(
                {caretX + 1, caretTop + 1, caretX + 2.5f, caretBottom - 1},
                brush);
        }
        // Send, blue, growing in once something is typed, and lighter
        // under the mouse.
        POINT cursor{};
        GetCursorPos(&cursor);
        const bool onSend = PointInRect(
            {cursor.x - m_position.x, cursor.y - m_position.y}, send);
        const float sendShown = m_hoverFades.Get(HoverKey(1000, 5), !empty);
        const float sendLit =
            m_hoverFades.Get(HoverKey(1000, 6), onSend && !empty);
        if (sendShown > 0.01f) {
            const float sendRadius =
                ((send.bottom - send.top) / 2 - std::round(3 * bannerScale)) *
                (0.5f + 0.5f * sendShown);
            const D2D1_POINT_2F sendCenter{(send.left + send.right) / 2,
                                           (send.top + send.bottom) / 2};
            brush->SetColor(AccentLit(row * sendShown, sendLit));
            target->FillEllipse({sendCenter, sendRadius, sendRadius}, brush);
            brush->SetColor(AccentInk(row * sendShown));
            bannerGlyph(kGlyphSend, m_smallIconFormat.get(), send);
        }
        const float openRadius = (openButton.bottom - openButton.top) / 2;
        brush->SetColor(
            Fg((0.16f +
                0.1f * m_hoverFades.Get(HoverKey(1000, 4),
                                        m_hover == IslandItem::BannerOpen)) *
               row));
        GlassFill(target, {openButton, openRadius, openRadius}, brush);
        PCWSTR openText = Tr(L"Open", L"Abrir");
        brush->SetColor(Fg(0.95f * row));
        m_bannerTimeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        target->DrawText(openText, (UINT32)wcslen(openText),
                         m_bannerTimeFormat.get(), openButton, brush);
        m_bannerTimeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    }
    // A call's, an alarm's or a reminder's buttons, lighter under the
    // mouse.
    if (row > 0.01f && BannerHasActions()) {
        m_bannerTimeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        for (size_t i = 0; i < m_banner.reply.actions.size(); i++) {
            const D2D1_RECT_F button = BannerActionRect(i);
            const float radius = (button.bottom - button.top) / 2;
            const float lit =
                m_hoverFades.Get(HoverKey(1001, (long long)i),
                                 m_hover == IslandItem::BannerAction &&
                                     m_hoverApp == std::to_wstring(i));
            brush->SetColor(Fg((0.16f + 0.1f * lit) * row));
            GlassFill(target, {button, radius, radius}, brush);
            const std::wstring& label = m_banner.reply.actions[i].label;
            brush->SetColor(Fg(0.95f * row));
            target->DrawText(
                label.c_str(), (UINT32)label.size(), m_bannerTimeFormat.get(),
                {button.left + std::round(6 * bannerScale), button.top,
                 button.right - std::round(6 * bannerScale), button.bottom},
                brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
        m_bannerTimeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    }
}

// The banner at its point of the screen (BannerPlaced): its window, drawn
// after the island's with each of its frames, by the island's own target
// (the pictures it shows are made for it), like a panel: the island's color,
// its thin edge, then its contents. Its mouse goes to the island (see
// HandleBannerWindow), where BannerRect is, outside the island's window.
void Island::RenderPlacedBanner() {
    if (!BannerPlaced() || m_bannerAmount <= 0.01 || m_away) {
        if (m_bannerWnd && IsWindowVisible(m_bannerWnd)) {
            ShowWindow(m_bannerWnd, SW_HIDE);
        }
        return;
    }
    if (!m_bannerWnd) {
        m_bannerWnd = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            kBannerClassName, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
            g_module, nullptr);
        if (!m_bannerWnd) {
            return;
        }
        SetWindowLongPtr(m_bannerWnd, GWLP_USERDATA, (LONG_PTR)this);
    }
    const D2D1_RECT_F rect = BannerRect();
    // (A pixel around it for its antialiased edge.)
    const POINT at{m_position.x + (LONG)std::floor(rect.left) - 1,
                   m_position.y + (LONG)std::floor(rect.top) - 1};
    const SIZE size{
        (LONG)std::ceil(rect.right) - (LONG)std::floor(rect.left) + 2,
        (LONG)std::ceil(rect.bottom) - (LONG)std::floor(rect.top) + 2};
    if (!m_canvas.Begin(size, 1)) {
        return;
    }
    m_bannerWndAt = at;
    ID2D1RenderTarget* target = m_canvas.target.get();
    target->SetTransform(D2D1::Matrix3x2F::Translation(
        (float)(m_position.x - at.x), (float)(m_position.y - at.y)));
    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(target->CreateSolidColorBrush(Bg(1), brush.put()))) {
        const float radius = std::min(
            BannerRadius(),
            std::min(rect.bottom - rect.top, rect.right - rect.left) / 2);
        target->FillRoundedRectangle({rect, radius, radius}, brush.get());
        brush->SetColor(Fg(0.10f * EdgeAmount()));
        target->DrawRoundedRectangle({{rect.left + 0.5f, rect.top + 0.5f,
                                       rect.right - 0.5f, rect.bottom - 0.5f},
                                      radius,
                                      radius},
                                     brush.get(), 1);
        const float contents =
            (float)std::clamp((m_bannerAmount - 0.45) / 0.55, 0.0, 1.0) *
            (1 - (float)std::clamp(m_bannerSwap, 0.0, 1.0));
        if (contents > 0.01f) {
            DrawBannerContents(target, brush.get(), contents);
        }
    }
    if (m_canvas.End(
            m_bannerWnd, at,
            std::clamp((float)m_bannerAmount * 2, 0.0f, 1.0f) * AppearFade()) &&
        !IsWindowVisible(m_bannerWnd)) {
        SetWindowPos(m_bannerWnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

LRESULT Island::HandleBannerWindow(HWND hWnd,
                                   UINT msg,
                                   WPARAM wParam,
                                   LPARAM lParam) {
    IslandScope scope(this);
    if (msg == WM_MOUSEACTIVATE) {
        return MA_NOACTIVATE;
    }
    // (The wheel's point is on the screen: nothing for it here.)
    if ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST && msg != WM_MOUSEWHEEL &&
         msg != WM_MOUSEHWHEEL) ||
        msg == WM_MOUSELEAVE) {
        const POINT pt{(short)LOWORD(lParam) + m_bannerWndAt.x - m_position.x,
                       (short)HIWORD(lParam) + m_bannerWndAt.y - m_position.y};
        return HandleMessage(hWnd, msg, wParam, MAKELPARAM(pt.x, pt.y));
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK BannerWndProc(HWND hWnd,
                               UINT msg,
                               WPARAM wParam,
                               LPARAM lParam) {
    if (auto* island = (Island*)GetWindowLongPtr(hWnd, GWLP_USERDATA)) {
        return island->HandleBannerWindow(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// The banner at its point of the screen (islandBannerPlace 1-9: the work
// area's top left to its bottom right, a gap from its edges, not over the
// pill against the same edge), in the island's coordinates. Coming in
// (m_bannerAmount under 1, a little past it as it bounces), from the side it's
// against (the left or the right, the top or the bottom in the middle
// column); the one in the middle grows out of its middle.
D2D1_RECT_F Island::PlacedBannerRect(float width, float height) const {
    const int point = std::clamp(g_settings.islandBannerPlace - 1, 0, 8);
    const int column = point % 3;
    const int row = point / 3;
    const float gap = std::round(16 * m_scale);
    const RECT& work = m_workArea;
    const float left =
        column == 0   ? work.left + gap
        : column == 1 ? std::round((work.left + work.right - width) / 2)
                      : work.right - gap - width;
    float top = row == 0   ? work.top + gap
                : row == 1 ? std::round((work.top + work.bottom - height) / 2)
                           : work.bottom - gap - height;
    if (row == 0 && !AtBottom()) {
        top = std::max(top, m_position.y + m_pill.bottom + gap);
    } else if (row == 2 && AtBottom()) {
        top = std::min(top, m_position.y + m_pill.top - gap - height);
    }
    const float away = 1 - (float)std::clamp(m_bannerAmount, 0.0, 1.08);
    float dx = 0;
    float dy = 0;
    if (column != 1) {
        dx = (column == 0 ? -1 : 1) * (width + gap) * away;
    } else if (row != 1) {
        dy = (row == 0 ? -1 : 1) * (height + gap) * away;
    }
    D2D1_RECT_F rect{left + dx - m_position.x, top + dy - m_position.y,
                     left + dx + width - m_position.x,
                     top + dy + height - m_position.y};
    if (point == 4) {
        const float inX = width * 0.1f * std::max(away, 0.0f);
        const float inY = height * 0.1f * std::max(away, 0.0f);
        rect = {rect.left + inX, rect.top + inY, rect.right - inX,
                rect.bottom - inY};
    }
    return rect;
}

void Island::BannerParts(D2D1_RECT_F* field,
                         D2D1_RECT_F* send,
                         D2D1_RECT_F* open) const {
    const D2D1_RECT_F banner = BannerRect();
    const float inner = std::round(14 * BannerScale());
    const float top = banner.top + std::round(kBannerHeight * BannerScale()) +
                      BannerTextExtra();
    const float height = std::round(32 * BannerScale());
    const float openWidth = std::round(76 * BannerScale());
    *open = {banner.right - inner - openWidth, top, banner.right - inner,
             top + height};
    // Taller with a reply on more lines; its send button at the bottom.
    *field = {banner.left + inner, top,
              open->left - std::round(8 * BannerScale()),
              top + height + BannerReplyExtra()};
    *send = {field->right - height, field->bottom - height, field->right,
             field->bottom};
}

D2D1_RECT_F Island::BannerActionRect(size_t index) const {
    const D2D1_RECT_F banner = BannerRect();
    const float inner = std::round(14 * BannerScale());
    const float gap = std::round(8 * BannerScale());
    const float top = banner.top + std::round(kBannerHeight * BannerScale()) +
                      BannerTextExtra();
    const float height = std::round(32 * BannerScale());
    const float count =
        (float)std::max<size_t>(1, m_banner.reply.actions.size());
    const float width =
        (banner.right - banner.left - inner * 2 - gap * (count - 1)) / count;
    const float left = banner.left + inner + (width + gap) * index;
    return {std::round(left), top, std::round(left + width), top + height};
}

float Island::BannerTextExtra() const {
    return (m_bannerTextLines - 1) * std::round(18 * BannerScale()) *
           (float)std::clamp(m_bannerExpand, 0.0, 1.05);
}

float Island::BannerReplyExtra() const {
    return (m_bannerReplyLines - 1) * std::round(17 * BannerScale());
}

// How many lines the banner's text and its reply take, as wide as they're
// shown (measured again only when they change).
void Island::MeasureBanner() {
    const bool content = g_showNotificationContent;
    // As wide as it's shown (see BannerRect).
    const float width =
        g_settings.islandPanelsFollow && !m_vertical && !m_bar &&
                !BannerPlaced()
            ? std::max(std::round(BannerWidthAt(m_scale)),
                       m_pill.right - m_pill.left)
            : std::round(BannerWidthAt(m_scale));
    const std::wstring key = (content ? m_banner.fullText : std::wstring()) +
                             L"\n" + m_bannerReplyText + L"\n" +
                             std::to_wstring(width) + L"\n" +
                             std::to_wstring(BannerScale());
    if (key == m_bannerMeasured) {
        return;
    }
    m_bannerMeasured = key;
    m_bannerTextLines =
        content && !m_banner.fullText.empty()
            ? std::min(3, WrappedLineCount(m_dwrite.get(),
                                           m_bannerTextFormat.get(),
                                           m_banner.fullText,
                                           width -
                                               std::round(86 * BannerScale()),
                                           std::round(18 * BannerScale())))
            : 1;
    // A new line just started counts (the caret goes there).
    const std::wstring reply =
        m_bannerReplyText +
        (!m_bannerReplyText.empty() && m_bannerReplyText.back() == L'\n'
             ? L" "
             : L"");
    m_bannerReplyLines =
        reply.empty()
            ? 1
            : std::min(5, WrappedLineCount(m_dwrite.get(),
                                           m_bannerTextFormat.get(), reply,
                                           width -
                                               std::round(156 * BannerScale()),
                                           std::round(17 * BannerScale())));
}

D2D1_RECT_F Island::BannerCloseRect() const {
    const D2D1_RECT_F banner = BannerRect();
    const float size = std::round(20 * BannerScale());
    const float inner = std::round(12 * BannerScale());
    return {banner.right - inner - size, banner.top + inner,
            banner.right - inner, banner.top + inner + size};
}

D2D1_RECT_F Island::EdgeLineRect(float length, float thickness) const {
    if (m_vertical) {
        const float middle = (m_pill.top + m_pill.bottom) / 2;
        const float left = m_sideLeft ? 0 : (float)m_size.cx - thickness;
        return {left, middle - length / 2, left + thickness,
                middle + length / 2};
    }
    const float middle = (m_pill.left + m_pill.right) / 2;
    const float top = AtBottom() ? (float)m_size.cy - thickness : 0;
    return {middle - length / 2, top, middle + length / 2, top + thickness};
}

bool Island::MinimizableAt(const IslandPlace& place) const {
    if (m_bar) {
        return false;
    }
    if (place.y < 0.001 || place.y > 0.999) {
        return true;
    }
    return g_settings.islandVerticalSides &&
           (place.x < 0.001 || place.x > 0.999);
}

D2D1_RECT_F Island::BannerMinimizeRect() const {
    const D2D1_RECT_F close = BannerCloseRect();
    const float gap = std::round(6 * BannerScale());
    const float size = close.right - close.left;
    return {close.left - gap - size, close.top, close.left - gap,
            close.bottom};
}

// Typing a reply: the island takes the keyboard, and the banner stays until
// clicking somewhere else.
void Island::StartBannerTyping() {
    if (m_bannerTyping || !BannerCanReply()) {
        return;
    }
    m_bannerTyping = true;
    // What was typed before a call, an alarm or a reminder took the pill.
    m_bannerReplyText = std::move(m_banner.draft);
    m_banner.draft.clear();
    m_bannerEdit.Reset();
    m_bannerEdit.caret = m_bannerReplyText.size();
    KillTimer(m_hwnd, kBannerTimerId);
    SetWindowLongPtr(m_hwnd, GWL_EXSTYLE,
                     GetWindowLongPtr(m_hwnd, GWL_EXSTYLE) & ~WS_EX_NOACTIVATE);
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);
    StartAnimating();
    Render(true);
}

void Island::EndBannerTyping() {
    if (!m_bannerTyping) {
        return;
    }
    m_bannerTyping = false;
    m_bannerReplyText.clear();
    m_bannerEdit.Reset();
    SetWindowLongPtr(m_hwnd, GWL_EXSTYLE,
                     GetWindowLongPtr(m_hwnd, GWL_EXSTYLE) | WS_EX_NOACTIVATE);
    StartAnimating();
    // The notifications that came meanwhile show now, one by one.
    if (!m_waitingToasts.empty()) {
        SetTimer(m_hwnd, kBannerNextTimerId, kBannerNextMs, nullptr);
    }
}

void Island::SendBannerReply() {
    if (m_bannerReplyText.empty()) {
        return;
    }
    const bool sent =
        ActivateToast(m_banner.appId, m_banner.reply.arguments,
                      m_banner.reply.activationType, m_banner.reply.inputId,
                      m_bannerReplyText);
    IslandLog(L"Reply from the pill: %s", sent ? L"sent" : L"no activator");
    EndBannerTyping();
    HideBanner();
}

// Typing in the reply box: Enter sends, Escape cancels, Ctrl+V pastes.
bool Island::OnBannerChar(WCHAR c) {
    if (!m_bannerTyping) {
        return false;
    }
    constexpr size_t kMaxReply = 2000;
    // Enter sends; Shift+Enter starts a new line.
    if (c == L'\r' && GetKeyState(VK_SHIFT) >= 0) {
        SendBannerReply();
    } else if (c == 0x1B) {
        EndBannerTyping();
        HideBanner();
    } else {
        EditTextChar(m_bannerReplyText, m_bannerEdit,
                     c == L'\r' ? L'\n' : c, kMaxReply, m_hwnd);
    }
    Render(true);
    return true;
}

// Defined further down.
bool AppShowsBanners(const std::wstring& appId);
bool WindowsShowsBanners();
void NoteToastsSeen(const std::vector<IslandToast>& toasts);

