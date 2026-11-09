#pragma once
// Listed by the main island while one of the options is on: when it starts,
// and as drives come and go.
void Island::UpdateDrivesWatch() {
    if (!m_hwnd || !m_main) {
        return;
    }
    if (UsbDrivesWatched() && !m_drivesListed) {
        m_drivesListed = true;
        ListDrives(0);
    } else if (!UsbDrivesWatched() && m_drivesListed) {
        m_drivesListed = false;
        AcquireSRWLockExclusive(&g_drivesLock);
        g_drives.clear();
        ReleaseSRWLockExclusive(&g_drivesLock);
        PostMessage(m_hwnd, WM_APP_DRIVES, 0, 0);
    }
}

void Island::ListDrives(DWORD arrived) {
    if (m_driveThread) {
        if (WaitForSingleObject(m_driveThread, 0) != WAIT_OBJECT_0) {
            m_drivesAgain = true;
            m_drivesArrived |= arrived;
            return;
        }
        CloseHandle(m_driveThread);
        m_driveThread = nullptr;
    }
    auto* job = new DriveJob{m_hwnd, arrived, ++m_drivesId};
    m_driveThread = CreateThread(nullptr, 0, DrivesThreadProc, job, 0, nullptr);
    if (!m_driveThread) {
        delete job;
    }
}

void Island::UpdateClipboardWatch() {
    // (Not a notch without the bell: it doesn't show what's copied.)
    if (!m_hwnd || !ShowsNotices() ||
        g_settings.islandClipboard == m_clipboardWatched) {
        return;
    }
    if (g_settings.islandClipboard) {
        m_clipboardWatched = AddClipboardFormatListener(m_hwnd) != FALSE;
    } else {
        RemoveClipboardFormatListener(m_hwnd);
        KillTimer(m_hwnd, kClipboardTimerId);
        m_clipboardWatched = false;
    }
}

// How much of a copied text the bubble shows.
constexpr size_t kClipboardChars = 32;

// What was copied, in the bubble: a file's name (or how many), the text's
// start on one line, or a picture. Not what asks not to be watched (password
// managers mark their copies so), and only "Copied" while the
// notifications' contents are hidden. Nothing is kept.
void Island::OnClipboardChanged() {
    if (!g_settings.islandClipboard || m_away || !ShowsNotices()) {
        return;
    }
    if (!OpenClipboard(m_hwnd)) {
        // The app copying still has it.
        if (++m_clipboardTries < 5) {
            SetTimer(m_hwnd, kClipboardTimerId, kClipboardRetryMs, nullptr);
        }
        return;
    }
    static const UINT kExcluded = RegisterClipboardFormat(
        L"ExcludeClipboardContentFromMonitorProcessing");
    const bool content = g_showNotificationContent;
    std::wstring what;
    if (kExcluded && IsClipboardFormatAvailable(kExcluded)) {
        // Not shown.
    } else if (HANDLE drop = IsClipboardFormatAvailable(CF_HDROP)
                                 ? GetClipboardData(CF_HDROP)
                                 : nullptr) {
        const UINT count = DragQueryFile((HDROP)drop, 0xFFFFFFFF, nullptr, 0);
        WCHAR path[MAX_PATH] = L"";
        if (count == 1 && content &&
            DragQueryFile((HDROP)drop, 0, path, ARRAYSIZE(path))) {
            what = std::wstring(Tr(L"Copied", L"Copiado")) + L": " +
                   PathFindFileName(path);
        } else if (count > 1) {
            what = std::wstring(Tr(L"Copied", L"Copiado")) + L": " +
                   std::to_wstring(count) + Tr(L" files", L" arquivos");
        }
    } else if (HANDLE data = IsClipboardFormatAvailable(CF_UNICODETEXT)
                                 ? GetClipboardData(CF_UNICODETEXT)
                                 : nullptr) {
        std::wstring line;
        if (const auto* text = (PCWSTR)GlobalLock(data)) {
            // One line, without runs of spaces.
            const size_t room = GlobalSize(data) / sizeof(WCHAR);
            for (size_t i = 0;
                 i < room && text[i] && line.size() <= kClipboardChars; i++) {
                const WCHAR c = iswspace(text[i]) ? L' ' : text[i];
                if (c != L' ' || (!line.empty() && line.back() != L' ')) {
                    line += c;
                }
            }
            GlobalUnlock(data);
        }
        if (line.size() > kClipboardChars) {
            line.resize(kClipboardChars);
            line += L"\u2026";
        }
        what = Tr(L"Copied", L"Copiado");
        if (content && !line.empty()) {
            what += L": " + line;
        }
    } else if (IsClipboardFormatAvailable(CF_DIB) ||
               IsClipboardFormatAvailable(CF_BITMAP)) {
        // Not the Snipping Tool's (Win+Shift+S): its own banner says it.
        const std::wstring owner = WindowAppKey(GetClipboardOwner());
        if (StrStrI(owner.c_str(), L"ScreenSketch") == nullptr &&
            StrStrI(owner.c_str(), L"ScreenClipping") == nullptr &&
            StrStrI(owner.c_str(), L"SnippingTool") == nullptr) {
            what = Tr(L"Picture copied", L"Imagem copiada");
        }
    }
    CloseClipboard();
    if (what.empty()) {
        return;
    }
    // Apps often put the same copy there again at once.
    const double now = NowSeconds();
    if (what == m_clipboardShown && now - m_clipboardShownAt < 1) {
        return;
    }
    m_clipboardShown = what;
    m_clipboardShownAt = now;
    ShowTextBubble(what, -1);
}

void Island::StepShiftChoice(ShiftChoice kind, int direction) {
    if (kind != m_shiftChoice) {
        m_shiftChoice = kind;
        m_shiftStep = -1;
        m_shiftOutputs.clear();
        if (kind == ShiftChoice::Output && m_devices) {
            m_shiftOutputs = ActiveOutputs(m_devices.get());
        }
    }
    // The names to choose from, and the one in use.
    std::vector<std::wstring> names;
    int current = -1;
    if (kind == ShiftChoice::Profile) {
        for (const auto& profile : g_profiles) {
            if (profile.id == (m_main ? g_activeProfile : m_profileId)) {
                current = (int)names.size();
            }
            names.push_back(profile.name);
        }
    } else {
        for (const auto& output : m_shiftOutputs) {
            if (output.isDefault) {
                current = (int)names.size();
            }
            names.push_back(output.name);
        }
    }
    const int count = (int)names.size();
    if (!count) {
        m_shiftChoice = ShiftChoice::None;
        return;
    }
    int index = m_shiftStep >= 0 && m_shiftStep < count ? m_shiftStep : current;
    index = index < 0 ? (direction > 0 ? 0 : count - 1)
                      : (index + direction + count) % count;
    m_shiftStep = index;
    ShowTextBubble(names[index], SlotMiddle(kind == ShiftChoice::Profile
                                                ? IslandItem::Profiles
                                                : IslandItem::Volume));
    SetTimer(m_hwnd, kShiftStepTimerId, kShiftStepCheckMs, nullptr);
}

void Island::ApplyShiftChoice() {
    KillTimer(m_hwnd, kShiftStepTimerId);
    const ShiftChoice kind = std::exchange(m_shiftChoice, ShiftChoice::None);
    const int step = std::exchange(m_shiftStep, -1);
    std::wstring name;
    if (kind == ShiftChoice::Profile && step >= 0 &&
        step < (int)g_profiles.size()) {
        const IslandProfile profile = g_profiles[step];
        name = profile.name;
        if (profile.id != (m_main ? g_activeProfile : m_profileId)) {
            ApplyProfile(profile.id);
        }
    } else if (kind == ShiftChoice::Output && step >= 0 &&
               step < (int)m_shiftOutputs.size()) {
        name = m_shiftOutputs[step].name;
        if (!m_shiftOutputs[step].isDefault) {
            SetDefaultOutput(m_shiftOutputs[step].id);
            OnVolumeChanged();
            m_control.Refresh();
        }
    }
    m_shiftOutputs.clear();
    // What was chosen, with a check mark, under its item.
    if (!name.empty()) {
        ShowTextBubble(L"\u2713 " + name,
                       SlotMiddle(kind == ShiftChoice::Profile
                                      ? IslandItem::Profiles
                                      : IslandItem::Volume));
    }
}

void Island::ShowGearBubble() {
    m_gearText.clear();
    m_gearX = -1;
    const double now = NowSeconds();
    // Again from the start if it was going away.
    if (!m_gearShownAt || now >= m_gearHideAt) {
        m_gearShownAt = now;
    }
    m_gearHideAt = now + kGearBubbleStay;
    StartAnimating();
    Render(true);
}

void Island::HideGearBubble() {
    const double now = NowSeconds();
    if (m_gearShownAt && now < m_gearHideAt) {
        m_gearHideAt = now;
        StartAnimating();
    }
}

void Island::OpenSettingsWindow(const SettingsWindowState* restore) {
    // (One a monitor: a notch opens its base's.)
    if (m_notchBase) {
        m_notchBase->OpenSettingsWindow(restore);
        return;
    }
    IslandScope scope(this);
    HideGearBubble();
    if (!m_settingsWindow.Window() &&
        !m_settingsWindow.CreateWindowed(m_d2d.get(), m_dwrite.get())) {
        IslandLog(L"The settings' window failed: %u", GetLastError());
        return;
    }
    // Open already: brought to the front as it is (its page, search and
    // scroll).
    if (!restore && m_settingsWindow.IsOpen() &&
        IsWindowVisible(m_settingsWindow.Window())) {
        if (IsIconic(m_settingsWindow.Window())) {
            ShowWindow(m_settingsWindow.Window(), SW_RESTORE);
        }
        SetForegroundWindow(m_settingsWindow.Window());
        return;
    }
    std::wstring title = Tr(L"Island settings", L"Configurações da ilha");
    if (!HasMainSettings()) {
        title += L" · " + std::wstring(Tr(L"Monitor ", L"Monitor ")) +
                 MonitorNumber(SettingsIsland()->OwnDevice());
    }
    if (restore) {
        m_settingsWindow.PlaceWindow(*restore);
    }
    m_settingsWindow.ShowSettings();
    m_settingsWindow.OpenWindowed(g_settings.islandPanelSize / 100.0f,
                                  title.c_str());
    if (restore) {
        m_settingsWindow.PutWindowState(*restore);
    }
}

// The gear's, or as wide as its text.
float Island::GearBubbleWidth() const {
    return m_gearText.empty() ? std::round(kGearBubbleWidth * m_scale)
                              : m_gearTextWidth;
}

// Under the pill (over it on the lower half of the screen, beside it when
// upright); when minimized, where the pill would be, beside the line.
D2D1_RECT_F Island::GearBubbleRect() const {
    const float width = GearBubbleWidth();
    const float height = std::round(kGearBubbleHeight * m_scale);
    const float gap = std::round(kGearBubbleGap * m_scale);
    if (m_vertical) {
        const float middle = std::round((m_pill.top + m_pill.bottom) / 2);
        const float left = m_minimized  ? (m_sideLeft ? m_pill.left
                                                      : m_pill.right - width)
                           : m_sideLeft ? m_pill.right + gap
                                        : m_pill.left - gap - width;
        return {left, middle - height / 2, left + width, middle + height / 2};
    }
    // Under its item (inside the window), or the pill's middle.
    const float centerX =
        m_gearX >= 0
            ? std::round(std::clamp(m_gearX, width / 2,
                                    std::max(width / 2,
                                             (float)m_size.cx - width / 2)))
            : std::round((m_pill.left + m_pill.right) / 2);
    // (Minimized, from the line's edge of the screen.)
    const float top = m_minimized  ? (AtBottom() ? m_pill.bottom - height
                                                 : m_pill.top)
                      : AtBottom() ? m_pill.top - gap - height
                                   : m_pill.bottom + gap;
    return {centerX - width / 2, top, centerX + width / 2, top + height};
}

// How far it's out, from 0 to 1 (a bit more while bouncing).
float Island::GearAmount(double now) const {
    if (!m_gearShownAt) {
        return 0;
    }
    const double t = now - m_gearShownAt;
    const double in = AnimationSeconds(kGearBubbleIn);
    const double out = AnimationSeconds(kGearBubbleOut);
    float amount = t < in ? (float)SpringOut(t / in) : 1;
    if (now >= m_gearHideAt) {
        amount = std::min(
            amount, out > 0 ? 1 - (float)EaseInOutCubic(std::min(
                                      1.0, (now - m_gearHideAt) / out))
                            : 0.0f);
    }
    return std::max(amount, 0.0f);
}

float Island::PeekAmount(double now) {
    if (!m_peekStart) {
        return 0;
    }
    const double t = now - m_peekStart;
    const double in = AnimationSeconds(kPeekIn);
    const double out = AnimationSeconds(kPeekOut);
    if (t < in) {
        return (float)SpringOut(t / in);
    }
    if (t < kPeekHold) {
        return 1;
    }
    if (t < kPeekHold + out) {
        return 1 - (float)EaseInOutCubic((t - kPeekHold) / out);
    }
    return 0;
}

ID2D1Bitmap* Island::GetBannerImage() {
    if (m_banner.image.empty() || !m_wic || !m_canvas.target) {
        return nullptr;
    }
    if (m_bannerImagePath == m_banner.image) {
        return m_bannerImage.get();
    }
    m_bannerImagePath = m_banner.image;
    m_bannerImage = nullptr;
    winrt::com_ptr<IWICBitmapDecoder> decoder;
    winrt::com_ptr<IWICBitmapFrameDecode> frame;
    winrt::com_ptr<IWICFormatConverter> converter;
    winrt::com_ptr<IWICStream> stream;
    const bool decoded =
        m_banner.imageData.empty()
            ? SUCCEEDED(m_wic->CreateDecoderFromFilename(
                  m_banner.image.c_str(), nullptr, GENERIC_READ,
                  WICDecodeMetadataCacheOnDemand, decoder.put()))
            : SUCCEEDED(m_wic->CreateStream(stream.put())) &&
                  SUCCEEDED(stream->InitializeFromMemory(
                      m_banner.imageData.data(),
                      (DWORD)m_banner.imageData.size())) &&
                  SUCCEEDED(m_wic->CreateDecoderFromStream(
                      stream.get(), nullptr, WICDecodeMetadataCacheOnLoad,
                      decoder.put()));
    if (decoded && SUCCEEDED(decoder->GetFrame(0, frame.put())) &&
        SUCCEEDED(m_wic->CreateFormatConverter(converter.put())) &&
        SUCCEEDED(converter->Initialize(
            frame.get(), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0,
            WICBitmapPaletteTypeMedianCut))) {
        m_canvas.target->CreateBitmapFromWicBitmap(converter.get(), nullptr,
                                                   m_bannerImage.put());
    }
    return m_bannerImage.get();
}

void Island::DrawAppIcon(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush,
                         const std::wstring& appId,
                         const D2D1_RECT_F& icon,
                         float opacity,
                         bool missingShown) {
    const float size = icon.right - icon.left;
    if (appId == kWidgetsBadgeId) {
        if (!m_iconFormat || size <= 0) {
            return;
        }
        // (The glyph at the icon's size, as it grows in.)
        const WCHAR glyph[] = {kGlyphWidgets, 0};
        D2D1_MATRIX_3X2_F transform;
        target->GetTransform(&transform);
        target->SetTransform(
            D2D1::Matrix3x2F::Scale(size / (18 * m_scale), size / (18 * m_scale),
                                    {(icon.left + icon.right) / 2,
                                     (icon.top + icon.bottom) / 2}) *
            transform);
        brush->SetColor(FgIcon(0.95f * opacity));
        DrawGlyph(target, glyph, 1, m_iconFormat.get(), icon, brush);
        target->SetTransform(transform);
        return;
    }
    if (ID2D1Bitmap* bitmap = GetAppBitmap(appId)) {
        DrawBitmapSmooth(target, bitmap, icon, opacity);
    } else if (missingShown) {
        brush->SetColor(Fg(0.3f * opacity));
        target->FillRoundedRectangle({icon, size / 4, size / 4}, brush);
    }
}

ID2D1Bitmap* Island::GetAppBitmap(const std::wstring& appId) {
    const double now = NowSeconds();
    int misses = 0;
    for (auto it = m_appBitmaps.begin(); it != m_appBitmaps.end(); ++it) {
        if (it->appId == appId) {
            if (!it->retryAt || now < it->retryAt) {
                return it->bitmap.get();
            }
            misses = it->bitmap ? 0 : it->misses + 1;
            m_appBitmaps.erase(it);
            break;
        }
    }
    bool fromExplorer = false;
    winrt::com_ptr<ID2D1Bitmap> bitmap =
        LoadAppIcon(m_canvas.target.get(), m_wic.get(), m_hwnd, appId,
                    (int)std::round(32 * m_scale), &fromExplorer);
    // Remembered even when missing, so it isn't looked up every frame (one
    // explorer didn't give, again after a while); the oldest are let go past
    // a limit (every app focused for a day: those still shown are read
    // again).
    constexpr size_t kMostAppBitmaps = 64;
    if (m_appBitmaps.size() >= kMostAppBitmaps) {
        m_appBitmaps.erase(m_appBitmaps.begin(),
                           m_appBitmaps.begin() + kMostAppBitmaps / 2);
    }
    m_appBitmaps.push_back(
        {appId, bitmap, AppIconRetryAt(fromExplorer, !!bitmap, misses, now),
         bitmap ? 0 : misses});
    return bitmap.get();
}

void Island::StartAnimating() {
    if (!m_animating) {
        m_animating = true;
        m_lastFrame = NowSeconds();
        m_freshStart = true;
    }
}

void Island::AnimationFrame() {
    const double now = NowSeconds();
    // A frame late (work done before it, like a panel getting ready to
    // open) doesn't make the animations jump ahead: they slow down a little
    // instead. The first frame starts them from their beginning.
    const int limit = g_frameRateLimit.load(std::memory_order_relaxed);
    const double longest =
        m_freshStart ? 0.008 : std::max(0.025, limit ? 1.5 / limit : 0.0);
    m_freshStart = false;
    const double dt = AnimationDt(std::clamp(now - m_lastFrame, 0.0, longest));
    m_lastFrame = now;

    bool expanding =
        m_minimized ? SpringTowards(m_expand, m_expandVelocity, 0,
                                    kCloseSeconds * 1.3, dt)
                    : SpringTowards(m_expand, m_expandVelocity, 1,
                                    kOpenSeconds, dt, kBounceDamping);
    // Moving to another monitor: once closed into its dot, the dot glides
    // there (see CheckMonitor).
    if (m_monitorMove == 1 &&
        (std::fabs(m_appear - kMoveDot) < 0.03 || !m_visible)) {
        m_moveFrom = {
            m_position.x + (LONG)std::lround((m_pill.left + m_pill.right) / 2),
            m_position.y + (LONG)std::lround((m_pill.top + m_pill.bottom) / 2)};
        SwitchMonitor(m_moveTarget);
        m_monitorMove = 2;
        m_moveT = 0;
        m_moveTVelocity = 0;
    }
    if (m_monitorMove == 2) {
        if (SpringTowards(m_moveT, m_moveTVelocity, 1, kOpenSeconds * 1.6,
                          dt)) {
            expanding = true;
        } else {
            m_monitorMove = 0;
        }
    }
    // Turning upright or back, once closed into its dot.
    if (m_reshape == 1 &&
        (std::fabs(m_appear - kMoveDot) < 0.03 || !m_visible)) {
        m_reshape = 2;
        expanding = true;
    }
    // Out of the dot (bouncing a little), or back into it.
    if (m_visible && (m_monitorMove || m_reshape)
            ? SpringTowards(m_appear, m_appearVelocity, kMoveDot,
                            kDisappearSeconds * 0.6, dt)
        : m_visible ? SpringTowards(m_appear, m_appearVelocity, 1,
                                    kAppearSeconds, dt, kPanelWidthDamping)
                    : SpringTowards(m_appear, m_appearVelocity, 0,
                                    kDisappearSeconds, dt)) {
        expanding = true;
    } else if (!m_visible && !m_away) {
        m_away = true;
        Render(true);
    }
    bool apps = false;
    for (auto& entry : m_appEntries) {
        if (SpringTowards(entry.shown, entry.velocity,
                          entry.present ? 1.0 : 0.0, kOpenSeconds, dt,
                          entry.present ? kBounceDamping : 1.0)) {
            apps = true;
        }
    }
    std::erase_if(m_appEntries, [](const AppEntry& entry) {
        return !entry.present && entry.shown <= 0;
    });
    // Only the clock when hidden: its small pill growing out of the line
    // (bouncing a little), or back into it.
    {
        const bool clock = HiddenClockWanted();
        m_hiddenClockWanted = clock;
        if (SpringTowards(m_hiddenClock, m_hiddenClockVelocity, clock ? 1 : 0,
                          clock ? kOpenSeconds : kCloseSeconds, dt,
                          clock ? kBounceDamping : 1.0)) {
            apps = true;
        }
    }
    // The minimized line under the mouse.
    if (SpringTowards(m_lineHover, m_lineHoverVelocity,
                      m_minimized && m_hover == IslandItem::Line ? 1 : 0,
                      kOpenSeconds * 0.8, dt, kBounceDamping)) {
        apps = true;
    }
    // The line fading away once left alone long enough (islandLineFade),
    // back at once under the mouse, with a notification or opened.
    {
        const double now = NowSeconds();
        // (Not while it shows a timer going: it changes every second.)
        if (!m_minimized || m_hover == IslandItem::Line ||
            m_bannerAmount > 0 || PeekAmount(now) > 0 || !m_lineAloneSince ||
            TimerLineWanted()) {
            m_lineAloneSince = now;
        }
        const bool due = LineFadeDue(now);
        if (SpringTowards(m_lineFade, m_lineFadeVelocity, due ? 0 : 1,
                          due ? kLineFadeOutSeconds : kOpenSeconds * 0.5,
                          dt)) {
            apps = true;
        }
    }
    // The folded apps: out like the tray's icons.
    if (SpringTowards(m_appsAmount, m_appsVelocity,
                      g_settings.islandFoldApps && m_appsOpen ? 1 : 0,
                      m_appsOpen ? kPanelWidthSeconds * 0.8
                                 : kPanelCloseSeconds,
                      dt)) {
        apps = true;
    }
    // The tray icons' list: out like a panel, back without bouncing.
    const double trayTarget = m_trayOpen ? 1 : 0;
    const bool trayRoom =
        SpringTowards(m_trayAmount, m_trayVelocity, trayTarget,
                      m_trayOpen ? kPanelWidthSeconds * 0.8 : kPanelCloseSeconds,
                      dt);
    const bool trayBounce =
        m_trayOpen ? SpringTowards(m_trayBounce, m_trayBounceVelocity, 1,
                                   kPanelWidthSeconds, dt, kPanelWidthDamping)
                   : SpringTowards(m_trayBounce, m_trayBounceVelocity, 0,
                                   kPanelCloseSeconds, dt);
    const bool trayArrow = SpringTowards(
        m_trayArrow, m_trayArrowVelocity,
        m_trayListPanel.IsOpen() && !m_vertical ? (AtBottom() ? -0.5 : 0.5)
                                                : trayTarget,
        kOpenSeconds, dt, kBounceDamping);
    if (trayRoom || trayBounce || trayArrow) {
        apps = true;
    }
    // Closed, the list starts from its beginning next time.
    if (!m_trayOpen && m_trayAmount <= 0.001) {
        m_trayScroll = m_trayScrollTarget = m_trayScrollVelocity = 0;
    } else if (SpringTowards(m_trayScroll, m_trayScrollVelocity,
                             m_trayScrollTarget, 0.35, dt)) {
        apps = true;
    }
    if (SpringTowards(m_pillScroll, m_pillScrollVelocity, m_pillScrollTarget,
                      0.35, dt)) {
        apps = true;
    }
    for (auto& state : m_pinStates) {
        if (SpringTowards(state.amount, state.velocity,
                          IsTrayPinned(state.key) ? 1 : 0, kOpenSeconds, dt)) {
            apps = true;
        }
    }
    // Tray icons coming and going.
    for (auto& icon : m_trayIcons) {
        if (SpringTowards(icon.shown, icon.shownVelocity,
                          icon.leaving ? 0.0 : 1.0, kOpenSeconds, dt,
                          icon.leaving ? 1.0 : kBounceDamping)) {
            apps = true;
        }
    }
    if (std::erase_if(m_trayIcons, [](const TrayIcon& icon) {
            return icon.leaving && icon.shown <= 0;
        })) {
        std::erase_if(m_trayBitmaps, [&](const auto& entry) {
            return !FindTrayIcon(std::get<0>(entry));
        });
    }
    std::erase_if(m_pinStates, [this](const PinState& state) {
        return state.velocity == 0 &&
               state.amount == (IsTrayPinned(state.key) ? 1 : 0);
    });
    // Sliding to its place (unless it's being dragged), straight there
    // without going past it.
    if (!m_dragMoved) {
        const IslandPlace& place = CurrentPlace();
        if (SpringTowards(m_placeX, m_placeVelocityX, place.x,
                          kOpenSeconds * 2.2, dt)) {
            apps = true;
        }
        if (SpringTowards(m_placeY, m_placeVelocityY, place.y,
                          kOpenSeconds * 2.2, dt)) {
            apps = true;
        }
    }
    // What's playing, and its buttons, growing in (bouncing) and out.
    if (SpringTowards(m_mediaAmount, m_mediaVelocity, m_mediaWanted ? 1 : 0,
                      m_mediaWanted ? kOpenSeconds * 1.25 : kCloseSeconds * 1.8,
                      dt, m_mediaWanted ? kBounceDamping : 1.0)) {
        apps = true;
    }
    const bool buttonsWanted = MediaPartWanted(kAppMediaButtons);
    const bool titleWanted = MediaPartWanted(kAppMediaTitle);
    if (SpringTowards(m_mediaButtonsAmount, m_mediaButtonsVelocity,
                      buttonsWanted ? 1 : 0,
                      buttonsWanted ? kOpenSeconds * 1.25 : kCloseSeconds * 1.8,
                      dt, buttonsWanted ? kBounceDamping : 1.0)) {
        apps = true;
    }
    // A pressed button.
    if (m_press.Animate(now)) {
        apps = true;
    }
    // The pill filling (see UpdateFill), its band sliding while it goes.
    if (SpringTowards(m_fillAmount, m_fillAmountVelocity,
                      m_fillWanted ? 1 : 0, kOpenSeconds, dt)) {
        apps = true;
    }
    if (SpringTowards(m_fillShown, m_fillShownVelocity, m_fillTarget,
                      kOpenSeconds * 1.4, dt)) {
        apps = true;
    }
    // (A download that ended: until its moment is over.)
    if (m_fillEnd != kFillEndNone) {
        UpdateFill();
    }
    // (Not minimized or hidden; the charge's on its own slower timer, see
    // UpdateFillTimer.)
    if (m_fillMoving && m_fillDownload && m_fillAmount > 0.01 &&
        !m_minimized && !m_away) {
        apps = true;
    }
    // The room for what's playing and its buttons, without bouncing.
    if (SpringTowards(m_mediaRoom, m_mediaRoomVelocity, m_mediaWanted ? 1 : 0,
                      m_mediaWanted ? kOpenSeconds * 1.1 : kCloseSeconds * 1.8,
                      dt)) {
        apps = true;
    }
    if (SpringTowards(m_mediaTitleRoom, m_mediaTitleRoomVelocity,
                      titleWanted ? 1 : 0,
                      titleWanted ? kOpenSeconds * 1.2 : kCloseSeconds * 1.8,
                      dt)) {
        apps = true;
    }
    if (SpringTowards(m_mediaButtonsRoom, m_mediaButtonsRoomVelocity,
                      buttonsWanted ? 1 : 0,
                      buttonsWanted ? kOpenSeconds * 1.1 : kCloseSeconds * 1.8,
                      dt)) {
        apps = true;
    }
    if (SpringDecoration(m_playSwap, m_playSwapVelocity, 0, 0.4, dt,
                      kBounceDamping)) {
        apps = true;
    }
    if (SpringDecoration(m_mediaSwap, m_mediaSwapVelocity, 0, 0.55, dt)) {
        apps = true;
    } else if (!m_mediaTitleBefore.empty()) {
        m_mediaTitleBefore.clear();
    }
    if (SpringDecoration(m_mediaCoverSwap, m_mediaCoverSwapVelocity, 0, 0.6,
                      dt)) {
        apps = true;
    } else if (m_mediaCoverBefore) {
        m_mediaCoverBefore = nullptr;
    }
    // Kept going while the dots wait to fade.
    if (SpringTowards(m_mediaDots, m_mediaDotsVelocity,
                      now < m_mediaDotsUntil ? 1 : 0,
                      now < m_mediaDotsUntil ? 0.3 : 0.6, dt) ||
        now < m_mediaDotsUntil) {
        apps = true;
    }
    if (SpringTowards(m_mediaTitleWidthShown, m_mediaTitleWidthVelocity,
                      m_mediaTitleWidth, 0.5, dt)) {
        apps = true;
    }
    if (SpringDecoration(m_themeTurn, m_themeTurnVelocity, m_darkMode ? 1 : 0,
                      0.5, dt, kBounceDamping)) {
        apps = true;
    }
    if (SpringDecoration(m_networkSwap, m_networkSwapVelocity, 0, 0.5, dt)) {
        apps = true;
    }
    if (SpringDecoration(m_clockSwap, m_clockSwapVelocity, 0, 0.45, dt)) {
        apps = true;
    } else if (!m_clockBefore.empty()) {
        m_clockBefore.clear();
    }
    if (m_clockWidth >= 0 &&
        SpringTowards(m_clockWidth, m_clockWidthVelocity, m_clockWidthTarget,
                      0.45, dt)) {
        apps = true;
    }
    for (double changedAt : m_usageChangedAt) {
        if (changedAt && now - changedAt < kUsageChangeSeconds) {
            apps = true;
        }
    }
    for (auto& swap : m_glyphSwaps) {
        if (SpringDecoration(swap.t, swap.velocity, 0, 0.45, dt)) {
            apps = true;
        }
    }
    if (m_updatesCount > 0) {
        m_updatesDotCount = m_updatesCount;
    }
    if (SpringTowards(m_updatesDot, m_updatesDotVelocity,
                      m_updatesCount > 0 ? 1 : 0, kOpenSeconds, dt,
                      m_updatesCount > 0 ? kBounceDamping : 1.0)) {
        apps = true;
    }
    if (SpringTowards(m_unseenDot, m_unseenDotVelocity, m_unseen ? 1 : 0,
                      kOpenSeconds, dt, m_unseen ? kBounceDamping : 1.0)) {
        apps = true;
    }
    if (m_weatherWidth >= 0 &&
        SpringTowards(m_weatherWidth, m_weatherWidthVelocity,
                      m_weatherWidthTarget, 0.45, dt)) {
        apps = true;
    }
    for (ItemWidth* width : {&m_komorebiWidth, &m_focusWidth}) {
        if (width->shown >= 0 && SpringTowards(width->shown, width->velocity,
                                               width->target, 0.45, dt)) {
            apps = true;
        }
        if (SpringDecoration(width->swap, width->swapVelocity, 0, 0.5, dt)) {
            apps = true;
        } else {
            width->nameBefore.clear();
        }
    }
    if (SpringDecoration(m_weatherSwap, m_weatherSwapVelocity, 0, 0.5, dt)) {
        apps = true;
    } else {
        m_weatherBefore.clear();
    }
    if (m_gapShown >= 0 &&
        SpringTowards(m_gapShown, m_gapVelocity, GapWanted(),
                      kOpenSeconds, dt)) {
        apps = true;
    }
    if (!m_microphoneShown.empty() && m_microphoneGlyph.empty() &&
        m_groupShown[(int)PillGroup::Microphone] <= 0) {
        m_microphoneShown.clear();
    }
    // The banner's reply row, under the mouse or while typing.
    const bool wantRow =
        m_bannerWanted && ((BannerCanReply() &&
                            (m_bannerTyping || BannerHovered())) ||
                           BannerHasActions());
    if (SpringTowards(m_bannerOpen, m_bannerOpenVelocity, wantRow ? 1 : 0,
                      kOpenSeconds, dt, wantRow ? kBounceDamping : 1.0)) {
        apps = true;
    }
    // Its text on more lines, under the mouse (smoothly, no swinging).
    const bool wantLines =
        m_bannerWanted && (m_bannerTyping || BannerHovered());
    if (SpringTowards(m_bannerExpand, m_bannerExpandVelocity,
                      wantLines ? 1 : 0, kOpenSeconds, dt)) {
        apps = true;
    }
    if (SpringDecoration(m_bannerSwap, m_bannerSwapVelocity, 0, 0.35, dt)) {
        apps = true;
    }
    // The banner: out like a panel, back without bouncing.
    const bool bannerMoving =
        m_bannerWanted
            ? SpringTowards(m_bannerAmount, m_bannerVelocity, 1,
                            kPanelWidthSeconds, dt, kPanelWidthDamping)
            : SpringTowards(m_bannerAmount, m_bannerVelocity, 0,
                            kPanelCloseSeconds, dt);
    if (bannerMoving) {
        apps = true;
    } else if (!m_bannerWanted && !m_banner.appId.empty()) {
        m_banner = {};
        m_bannerOpen = 0;
        m_bannerOpenVelocity = 0;
    }
    // The camera's icon blinking.
    if (m_cameraSince && now - m_cameraSince < kCameraBlinkSeconds) {
        apps = true;
    }
    if (m_peekStart) {
        if (now - m_peekStart >= kPeekHold + kPeekOut || !m_minimized) {
            m_peekStart = 0;
            m_peekApp.clear();
        } else {
            apps = true;
        }
    }
    if (m_gearShownAt) {
        // It stays while the mouse is on it.
        if (m_hover == IslandItem::SettingsBubble && now < m_gearHideAt &&
            m_gearHideAt - now < kGearBubbleLinger) {
            m_gearHideAt = now + kGearBubbleLinger;
        }
        if (now >= m_gearHideAt + kGearBubbleOut) {
            m_gearShownAt = 0;
            m_gearHideAt = 0;
            m_gearText.clear();
            m_gearX = -1;
        }
        apps = true;
    }
    // The icon's name growing in, or going.
    if (m_tipShownAt) {
        const bool going = m_tipHideAt != 0;
        if (going && now >= m_tipHideAt + AnimationSeconds(kTipOut)) {
            m_tipShownAt = m_tipHideAt = 0;
            m_tipLayout = nullptr;
        }
        if (going || now - m_tipShownAt < AnimationSeconds(kTipIn) + 0.05) {
            apps = true;
        }
    }
    // A pin growing in.
    if (!m_pinShownKey.empty() && now - m_pinShownAt < kPinGrowSeconds) {
        apps = true;
    }
    // (A reply being typed: its caret blinking a moment.)
    if (m_hoverFades.Animate(dt) || (m_bannerTyping && CaretBlinking())) {
        apps = true;
    }
    // Colors blending into another theme.
    if (StepTheme()) {
        apps = true;
    }
    // The visualizer's bars, while they're on the pill.
    if (m_vizRunning) {
        StepVisualizer(dt);
        apps = true;
    }
    // The dragon's eyes, only while something moves (they ask to go on).
    if (m_eyesRunning && StepEyes(dt)) {
        apps = true;
    }
    // Groups sliding to their new places (another order), and shrinking
    // away or growing in.
    for (int i = 0; i < kPillGroups; i++) {
        if (m_groupSwap[i] > 0) {
            if (SpringTowards(m_groupSwap[i], m_groupSwapVelocity[i], 0, 0.5,
                              dt)) {
                apps = true;
            } else {
                m_groupSwap[i] = 0;
                m_groupShift[i] = m_groupShiftVelocity[i] = 0;
            }
        } else if (SpringTowards(m_groupShift[i], m_groupShiftVelocity[i], 0,
                                 0.32, dt, 0.82)) {
            apps = true;
        }
        if (SpringTowards(m_groupWidthShown[i], m_groupWidthVelocity[i],
                          m_groupWidthWanted[i], 0.3, dt)) {
            apps = true;
        }
        // (The song's line, a wide text: slower and without the bounce, as
        // the items around it make room.)
        const bool lyrics = i == (int)PillGroup::Lyrics;
        if (SpringTowards(m_groupShown[i], m_groupShownVelocity[i],
                          GroupWanted((PillGroup)i) ? 1 : 0,
                          lyrics ? 0.6 : 0.38, dt,
                          lyrics ? 1.0 : kBounceDamping)) {
            apps = true;
        }
    }
    Render();
    // Started while drawing (another title measured, another cover decoded,
    // a title scrolling).
    if (m_mediaSwap > 0 || m_mediaCoverSwap > 0 || m_mediaMarquee ||
        m_bannerMarquee ||
        std::fabs(m_mediaTitleWidthShown - m_mediaTitleWidth) > 0.01) {
        apps = true;
    }
    bool panels = false;
    // The last panel closed: the notifications that came meanwhile show.
    const bool panelOpen = AnyPanelOpen();
    if (m_panelWasOpen && !panelOpen && !m_bannerWanted &&
        !m_waitingToasts.empty()) {
        SetTimer(m_hwnd, kBannerNextTimerId, kBannerNextMs, nullptr);
    }
    m_panelWasOpen = panelOpen;
    for (Panel* panel : Panels()) {
        panels = panel->Animate(dt) || panels;
    }
    panels = m_settingsWindow.Animate(dt) || panels;
    m_animating = expanding || panels || apps;
}

// (Set on the other notches too, see SetMinimized.)
bool g_minimizingNotches = false;

// The Bar style hidden, or back (the minimize shortcut): it fades out like
// over a full-screen app (or closes into a small pill with the time, see
// islandHideClock), and gives its strip of the screen back.
void Island::SetBarHidden(bool hidden) {
    if (m_barHidden == hidden) {
        return;
    }
    m_barHidden = hidden;
    if (hidden) {
        CloseAllPanels();
        EndTakeover();
    }
    // (A window under the small pill, looked at again.)
    if (hidden && g_settings.islandHideClock) {
        m_hiddenClockLook = {};
        HiddenClockCovered();
    }
    CheckShown();
    UpdateAutoHide();
    StartAnimating();
}

void Island::SetMinimized(bool minimized, bool keep) {
    // The notches hide and show together (their one line on the bell's,
    // see Render), unless each one hides on its own.
    if (m_notch >= 0 && !g_settings.islandNotchEach && !g_minimizingNotches) {
        g_minimizingNotches = true;
        for (Island* notch : NotchSet()) {
            if (notch != this && notch->m_minimized != minimized) {
                IslandScope scope(notch);
                notch->SetMinimized(minimized, keep);
            }
        }
        g_minimizingNotches = false;
    }
    m_minimized = minimized;
    // (`keep` false: the auto-hide's, see CheckAutoHide.)
    m_autoHidden = minimized && !keep;
    if (keep) {
        Wh_SetIntValue(MinimizedKey().c_str(), minimized);
    }
    SetAppBarPosition();
    // (The strip is kept by the notch with the bell, for all of them.)
    if (m_notchBase) {
        m_notchBase->SetAppBarPosition();
    }
    if (minimized) {
        CloseAllPanels();
        EndTakeover();
    } else if (m_stackShowing && WantsTakeover()) {
        m_bannerReadRetries = 0;
        ReadWindowsBanners(100);
    }
    StartAnimating();
}

// The song's lyrics looked for (the main island; the others wait for it, see
// WM_APP_LYRICS), when they're on and another song plays.
void Island::UpdateLyrics() {
    if (!g_settings.islandLyrics || !m_mediaShown.present ||
        m_mediaShown.title.empty()) {
        m_lyricsKey.clear();
        m_lyrics = {};
        m_lyricsSearching = false;
        StepLyrics();
        return;
    }
    // (Listed in the settings, to turn its lyrics off there.)
    if (NoteLyricsApp(m_mediaShown.appId)) {
        RefreshLyricsAppsSetting();
    }
    const std::wstring key = LyricsKey(m_mediaShown);
    // Not a song, or its app's lyrics off: nothing looked for (or sent).
    const int skip =
        LyricsSkipFor(m_mediaShown, AppDisplayName(m_mediaShown.appId),
                      LyricsAppOff(m_mediaShown.appId));
    if (skip != kLyricsSkipNone) {
        if (key != m_lyricsKey || m_lyrics.skip != skip) {
            // (Why, for an app whose songs get no lyrics, #6691.)
            if (m_main) {
                IslandLog(L"Lyrics not looked for (%d): length %.0f s, "
                          L"artist %d, from %s",
                          skip, m_mediaShown.duration,
                          !m_mediaShown.artist.empty(),
                          m_mediaShown.appId.c_str());
            }
            m_lyricsKey = key;
            m_lyrics = {};
            m_lyrics.key = key;
            m_lyrics.skip = skip;
        }
        m_lyricsSearching = false;
        m_lyricsPanel.SetSong(m_mediaShown, m_lyrics, false);
        StepLyrics();
        return;
    }
    // (Not reached last time: again, a while later.)
    const bool retry = key == m_lyricsKey && m_lyrics.failed &&
                       !m_lyricsSearching &&
                       NowSeconds() - m_lyrics.failedAt > kLyricsRetrySeconds;
    // (Or its app's lyrics turned on again.)
    if (key != m_lyricsKey || retry || m_lyrics.skip != kLyricsSkipNone) {
        m_lyricsKey = key;
        m_lyrics = {};
        m_lyrics.key = key;
        m_lyricsSearching = !CopyLyrics(key, &m_lyrics);
        // (The main island's thread looks, for every island: its own
        // lyrics may be off.)
        Island* main = MainIsland();
        if (m_lyricsSearching &&
            (!main || !main->StartLyricsSearch(m_mediaShown, key))) {
            m_lyricsSearching = false;
        }
    }
    m_lyricsPanel.SetSong(m_mediaShown, m_lyrics, m_lyricsSearching);
    StepLyrics();
}

void Island::RefreshLyricsAppsSetting() {
    ForEachIsland([](Island& island) {
        island.m_control.RefreshSettings();
        island.m_settingsWindow.RefreshSettings();
    });
}

void Island::LyricsAppsChanged() {
    ForEachIsland([](Island& island) {
        island.UpdateLyrics();
        island.m_control.RefreshSettings();
        island.m_settingsWindow.RefreshSettings();
    });
}

// On the main island: a song's lyrics looked for on a thread, one at a time
// (another one asked meanwhile: looked for after it, see WM_APP_LYRICS).
// False if it can't be.
bool Island::StartLyricsSearch(const MediaState& media,
                               const std::wstring& key) {
    if (m_lyricsThread) {
        return true;
    }
    g_lyricsStop = false;
    AcquireSRWLockExclusive(&g_lyricsCancel.lock);
    g_lyricsCancel.canceled = false;
    ReleaseSRWLockExclusive(&g_lyricsCancel.lock);
    auto* request = new LyricsRequest{m_hwnd, key, media.title,
                                      LyricsArtist(media.artist),
                                      media.duration};
    m_lyricsThread =
        CreateThread(nullptr, 0, LyricsThreadProc, request, 0, nullptr);
    if (!m_lyricsThread) {
        delete request;
        return false;
    }
    return true;
}

// The line sung now on the pill, and a timer for the next one; paused, none
// (it goes, and comes back as the song does).
void Island::StepLyrics() {
    KillTimer(m_hwnd, kLyricsTimerId);
    std::wstring line;
    if (g_settings.islandLyrics && m_lyrics.synced) {
        const double now = m_mediaShown.Now() + kLyricsLead;
        const int index = LyricLineAt(m_lyrics, now);
        if (index >= 0 && m_mediaShown.playing) {
            line = m_lyrics.lines[index].text;
        }
        if (m_mediaShown.playing && index + 1 < (int)m_lyrics.lines.size()) {
            const double next = m_lyrics.lines[index + 1].at - now;
            SetTimer(m_hwnd, kLyricsTimerId,
                     (UINT)std::clamp(next * 1000 + 30, 30.0, 2000.0),
                     nullptr);
        }
    }
    if (line != m_lyricsLine) {
        if (line.empty()) {
            m_lyricsHeldWidth = 0;
        }
        m_lyricsBefore = m_lyricsLine;
        m_lyricsLine = line;
        m_lyricsChangedAt = NowSeconds();
        Relayout();
        StartAnimating();
    }
}

// The pill's line's text, as big as chosen (islandLyricsSize).
IDWriteTextFormat* Island::LyricsFormat() {
    const float size = (g_settings.islandLyricsSize == 0   ? 11.5f
                        : g_settings.islandLyricsSize == 2 ? 15.0f
                                                           : 13.0f) *
                       m_textScale;
    if (m_lyricsFormat && size == m_lyricsFormatSize) {
        return m_lyricsFormat.get();
    }
    m_lyricsFormat = nullptr;
    m_lyricsFormatSize = size;
    if (SUCCEEDED(m_dwrite->CreateTextFormat(
            L"Segoe UI Variable Text", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"",
            m_lyricsFormat.put()))) {
        m_lyricsFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_lyricsFormat->SetParagraphAlignment(
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        m_lyricsFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        const DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0,
                                       0};
        winrt::com_ptr<IDWriteInlineObject> ellipsis;
        m_dwrite->CreateEllipsisTrimmingSign(m_lyricsFormat.get(),
                                             ellipsis.put());
        m_lyricsFormat->SetTrimming(&trimming, ellipsis.get());
    }
    return m_lyricsFormat.get();
}

// What's playing shows on the pill a moment longer than it plays, so media
// starting and stopping quickly (or a page changing songs) doesn't flash.
void Island::UpdateMediaShown() {
    const double now = NowSeconds();
    if (m_mediaState.present) {
        KillTimer(m_hwnd, kMediaTimerId);
        m_mediaGoneAt = 0;
        if (m_mediaShown.present && m_mediaShown.playing != m_mediaState.playing) {
            // Play turning into pause: it shrinks and grows again.
            m_playSwap = 1;
            m_playSwapVelocity = 0;
        }
        const bool newText = m_mediaShown.title != m_mediaState.title ||
                             m_mediaShown.artist != m_mediaState.artist ||
                             m_mediaShown.appId != m_mediaState.appId;
        // A long title scrolls from its start again (see
        // MediaTitleScrolls).
        if (newText || (m_mediaState.playing && !m_mediaShown.playing)) {
            m_mediaMarqueeStart = now;
        }
        m_mediaShown = m_mediaState;
        // Its tip shown: with the new title.
        if (newText && m_tooltipKey.starts_with(kItemTipPrefix) &&
            (IslandItem)_wtoi(m_tooltipKey.c_str() + wcslen(kItemTipPrefix)) ==
                IslandItem::Media) {
            const std::wstring key = m_tooltipKey;
            m_tooltipKey.clear();
            ShowTrayTip(key);
        }
        if (!m_mediaWanted) {
            m_mediaWanted = true;
            m_mediaSince = now;
        }
        UpdateLyrics();
        StartAnimating();
        return;
    }
    if (!m_mediaWanted) {
        return;
    }
    if (!m_mediaGoneAt) {
        m_mediaGoneAt = now;
    }
    const double until =
        std::max(m_mediaGoneAt + kMediaLinger, m_mediaSince + kMediaMinShown);
    if (now + 0.01 >= until) {
        m_mediaWanted = false;
        m_mediaShown.present = false;
        // Nothing playing anymore: its panels close, its line goes.
        m_mediaPanel.Close();
        m_lyricsPanel.Close();
        UpdateLyrics();
        StartAnimating();
        return;
    }
    SetTimer(m_hwnd, kMediaTimerId, (UINT)((until - now) * 1000) + 10, nullptr);
}

// Whether a visible window of another app is above the island where it is.
// Never while a menu is open above it (a tray icon's menu, opened from the
// island, or the app in front's own popups): the island isn't raised over
// it then.
bool Island::IsCovered() const {
    RECT island;
    if (!GetWindowRect(m_hwnd, &island)) {
        return false;
    }
    const DWORD self = GetCurrentProcessId();
    DWORD foregroundProcess = 0;
    if (HWND foreground = GetForegroundWindow()) {
        GetWindowThreadProcessId(foreground, &foregroundProcess);
    }
    bool covered = false;
    for (HWND above = GetWindow(m_hwnd, GW_HWNDPREV); above;
         above = GetWindow(above, GW_HWNDPREV)) {
        DWORD processId = 0;
        GetWindowThreadProcessId(above, &processId);
        if (processId == self || !IsWindowVisible(above)) {
            continue;
        }
        int cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(above, DWMWA_CLOAKED, &cloaked,
                                            sizeof(cloaked))) &&
            cloaked) {
            continue;
        }
        WCHAR className[16] = L"";
        GetClassName(above, className, ARRAYSIZE(className));
        // (The app in front's small windows without a title bar: its menus
        // and flyouts, like a tray app's panel opened from the island.)
        const LONG style = GetWindowLong(above, GWL_STYLE);
        RECT popup{};
        GetWindowRect(above, &popup);
        const bool menuLike =
            processId == foregroundProcess &&
            (style & WS_CAPTION) != WS_CAPTION &&
            (popup.right - popup.left) * 2 < m_monitor.right - m_monitor.left &&
            (popup.bottom - popup.top) * 2 < m_monitor.bottom - m_monitor.top;
        if (wcscmp(className, L"#32768") == 0 || menuLike) {
            return false;
        }
        RECT rect;
        RECT overlap;
        if (GetWindowRect(above, &rect) &&
            IntersectRect(&overlap, &rect, &island)) {
            covered = true;
        }
    }
    return covered;
}

// Shown, or hidden: not over a full-screen app (a game or a video), nor over
// the screenshot's selection after a moment (it's in the picture taken, and
// then out of the way of the selection's buttons). On each tick, and as a
// notification comes (the selection may be gone already).
void Island::CheckShown() {
    const double now = NowSeconds();
    if (!IsSnippingOn(CurrentMonitor())) {
        m_snippingSince = 0;
    } else if (!m_snippingSince) {
        m_snippingSince = now;
    }
    // (The Bar hidden showing only the clock: shown, but not over a window.)
    const bool barGone =
        m_barHidden && !(g_settings.islandHideClock && !m_hiddenClockCovered);
    const bool show = !barGone && !IsFullscreenAppOn(CurrentMonitor()) &&
                      !(m_snippingSince &&
                        now - m_snippingSince >= kSnippingHideSeconds);
    if (show != m_visible) {
        m_visible = show;
        // Hidden once it has closed into the dot and faded (see
        // AnimationFrame).
        if (show) {
            m_away = false;
            // Shown once, still fully transparent; never hidden again.
            if (!m_shownOnce) {
                m_shownOnce = true;
                ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
            }
            // The notices that waited for it (see ShowToasts).
            if (!m_bannerWanted && !m_waitingToasts.empty()) {
                SetTimer(m_hwnd, kBannerNextTimerId, kBannerNextMs, nullptr);
            }
        } else {
            CloseAllPanels();
            EndTakeover();
        }
        StartAnimating();
    }
}

// Whether the minimized line has been left alone long enough to fade away
// (islandLineFade; not the bar's).
bool Island::LineFadeDue(double now) const {
    const int seconds = kLineFadeSeconds[std::clamp(
        g_settings.islandLineFade, 0, (int)std::size(kLineFadeSeconds) - 1)];
    return seconds > 0 && !m_bar && m_minimized && m_lineAloneSince > 0 &&
           now - m_lineAloneSince >= seconds;
}

void Island::Tick() {
    // (The line's time to fade, see islandLineFade: the frames start it.)
    if (m_lineFade > 0 && LineFadeDue(NowSeconds())) {
        StartAnimating();
    }
    // Windows' "Do not disturb" follows the island's own notifications (the
    // notification watch may become ready after the island).
    UpdateWindowsQuiet(true);
    // (Any app's sound brings the visualizer.)
    CheckAudioPlaying();
    // (The battery's charge, now and then.)
    UpdateFill();
    // The taskbar moved to another edge (hiding itself, it doesn't change
    // the screen's work area): the pill's room is another.
    if (const UINT edge = TaskbarEdge(); edge != m_taskbarEdge) {
        m_taskbarEdge = edge;
        Relayout();
    }
    ReadBrightnessSoon();
    // A display message can be missed: the monitors are looked at here too
    // (one plugged in or out, moved, or another resolution), and the
    // taskbars told.
    if (m_main) {
        const auto monitors = ListMonitors();
        const bool changed =
            !m_displayMonitors.empty() &&
            (monitors.size() != m_displayMonitors.size() ||
             !std::equal(monitors.begin(), monitors.end(),
                         m_displayMonitors.begin(),
                         [](const MonitorEntry& a, const MonitorEntry& b) {
                             return a.device == b.device &&
                                    a.handle == b.handle &&
                                    EqualRect(&a.rect, &b.rect);
                         }));
        m_displayMonitors = monitors;
        if (changed) {
            PostMessage(FindTaskbarWnd(), DisplayChangedMessage(), 0, 0);
            if (g_settings.islandAllMonitors || m_notchCount > 0) {
                RestartIslandSoon();
                return;
            }
            m_control.ReloadControls();
            Relayout();
        }
    }
    // On the monitor it should be on (another one chosen, unplugged...).
    CheckMonitor();
    CheckShown();
    CheckFocusedApp();
    // (Also hidden over a full-screen app: an alarm rings, a timer ends.)
    CheckTimer();
    if (m_main) {
        CheckMagnifierStarted();
        // (Not taken yet: Explorer starting again.)
        TellExplorerProgress();
    }
    if (!m_visible) {
        return;
    }
    const double now = NowSeconds();
    if (++m_ticks >= kIslandStatusTicks) {
        m_ticks = 0;
        UpdateStatus();
    }
    // Snoozed notifications due again (only the main island keeps them).
    if (!m_snoozedToasts.empty()) {
        std::vector<IslandToast> due;
        std::erase_if(m_snoozedToasts, [&](auto& entry) {
            if (entry.second > now) {
                return false;
            }
            due.push_back(std::move(entry.first));
            return true;
        });
        if (!due.empty()) {
            SaveSnoozed();
            AcquireSRWLockExclusive(&g_islandToastsLock);
            g_islandToasts.insert(g_islandToasts.end(),
                                  std::make_move_iterator(due.begin()),
                                  std::make_move_iterator(due.end()));
            ReleaseSRWLockExclusive(&g_islandToastsLock);
            ShowBanner();
        }
    }
    if (++m_raiseTicks >= kIslandRaiseTicks) {
        m_raiseTicks = 0;
        m_control.Refresh();
        m_notifications.Refresh();
        // Other always-on-top windows can cover it: raised again only then (and
        // an open panel stays above it), so it doesn't fight over the top.
        if (IsCovered()) {
            SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            for (Panel* panel : Panels()) {
                panel->Raise();
            }
            // The frosted strip (or the Liquid Glass) right under it again.
            if (m_glassStrip && IsWindowVisible(m_glassStrip)) {
                SetWindowPos(m_glassStrip, m_hwnd, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            m_liquid.FollowOwner();
        }
    }
    UpdateClock();
    UpdateExtraItems();
    if (g_settings.islandOwnsNotifications) {
        NoteQuiet(g_islandQuiet);
    }
    m_calendar.Refresh();
    m_mediaPanel.Refresh();
    // Reminders due now (the main island's; ShowBanner passes them on).
    if (m_main) {
        for (const auto& event : TakeDueReminders()) {
            ShowReminder(event);
        }
    }
    Render();
}

// The taskbar's download state as the pill shows it: a browser's standing
// still is paused, as far as can be told (islandDownloadStalled).
int TaskbarDownloadState(double now) {
    return g_downloadState == kProgressNormal && g_downloadBrowsers &&
                   g_settings.islandDownloadStalled &&
                   now - g_downloadMovedAt >= kDownloadStalledSeconds
               ? kProgressPaused
               : g_downloadState;
}

