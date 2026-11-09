#pragma once
void ControlPanel::UpdateApps() {
    ReadMixerApps(m_apps, kMaxMixerApps);
}

void ControlPanel::SetView(View view) {
    // The settings open with an empty search, at their top (not coming back
    // from the places per app).
    const bool freshSettings =
        view == View::Settings && m_view != View::AppPlaces;
    if (freshSettings) {
        m_search.clear();
        m_searchEdit.Reset();
        m_settingsScroll.Reset();
        m_orderDrag = -1;
        m_orderMoved = false;
    }
    // The new view slides in: from the right going in, from the left coming
    // back to the main one (or the Wi-Fi list from the password).
    if (view != m_view) {
        const bool back = view == View::Main ||
                          (view == View::Wifi && m_view == View::Password) ||
                          (view == View::Settings && m_view == View::AppPlaces);
        m_viewDirection = back ? -1 : 1;
        m_viewIn = 0;
        m_viewInVelocity = 0;
        StartIslandAnimation();
    }
    if (view != m_view) {
        m_forgetProfile.clear();
    }
    // (The hotspot's password isn't kept once its page is left.)
    if (m_view == View::Hotspot && view != View::Hotspot) {
        m_hotspotName.clear();
        m_password.clear();
        m_hotspotNameEdit.Reset();
        m_passwordEdit.Reset();
        m_hotspotStatus.clear();
    }
    m_view = view;
    m_hover = {};
    // Windows looks for networks again (on the thread reading them); the
    // list is read again when it's done (WM_APP_WLAN).
    m_wifiScanAgain = view == View::Wifi;
    Update();
    Layout();
    if (freshSettings) {
        SettleSettings();
    }
}

void ControlPanel::Layout() {
    const float s = m_scale;
    // (Wider for Wi-Fi: the networks' names and their buttons.)
    float width = std::round(
        (m_view == View::Wifi || m_view == View::Password ? kControlWidth + 60
                                                           : kControlWidth) *
        s);
    const float padding = std::round(kControlPadding * s);
    const float gap = std::round(kControlGap * s);
    float y = padding;

    m_rows.clear();
    m_rowIcons.clear();
    m_rowSliders.clear();

    if (m_view == View::Settings) {
        LayoutSettings();
        return;
    }

    if (m_view != View::Main) {
        // "<  Title" header (with an on/off switch for the radios), then a row
        // per device, app or network.
        const float header = std::round(kHeaderHeight * s);
        m_back = {padding, y, padding + header, y + header};
        const float toggleWidth = std::round(36 * s);
        const float toggleHeight = std::round(20 * s);
        const float toggleRight = width - padding - std::round(4 * s);
        m_toggle = {toggleRight - toggleWidth,
                    y + std::round((header - toggleHeight) / 2),
                    toggleRight,
                    y + std::round((header - toggleHeight) / 2) +
                        toggleHeight};
        y += header;
        m_rowButtons.clear();
        m_wiredRow = {};
        if (m_view == View::Wifi || m_view == View::Bluetooth) {
            const size_t count = m_view == View::Wifi
                                     ? m_networks.size()
                                     : m_bluetoothDevices.size();
            const float row = std::round(kRowHeight * s);
            if (m_view == View::Wifi && m_wired.present) {
                m_wiredRow = {padding, y, width - padding, y + row};
                y += row + std::round(4 * s);
            }
            for (size_t i = 0; i < count; i++) {
                m_rows.push_back({padding, y, width - padding, y + row});
                y += row;
            }
            // Each network's buttons, from the right: Disconnect (the
            // connected one) or Connect, and Forget for a saved one.
            if (m_view == View::Wifi) {
                const float buttonHeight = std::round(26 * s);
                const float buttonGap = std::round(6 * s);
                auto measure = [&](PCWSTR text) {
                    winrt::com_ptr<IDWriteTextLayout> layout;
                    DWRITE_TEXT_METRICS metrics{};
                    if (m_smallFormat &&
                        SUCCEEDED(m_dwrite->CreateTextLayout(
                            text, (UINT32)wcslen(text), m_smallFormat.get(),
                            1000, 100, layout.put())) &&
                        SUCCEEDED(layout->GetMetrics(&metrics))) {
                        return std::ceil(metrics.width) + std::round(20 * s);
                    }
                    return std::round(90 * s);
                };
                for (size_t i = 0; i < m_networks.size() && i < m_rows.size();
                     i++) {
                    const WifiNetwork& network = m_networks[i];
                    const D2D1_RECT_F& rowRect = m_rows[i];
                    const float top = std::round(
                        (rowRect.top + rowRect.bottom - buttonHeight) / 2);
                    float right = rowRect.right - std::round(8 * s);
                    auto add = [&](Part part, PCWSTR text) {
                        const float buttonWidth = measure(text);
                        m_rowButtons.push_back(
                            {i, part,
                             {right - buttonWidth, top, right,
                              top + buttonHeight}});
                        right -= buttonWidth + buttonGap;
                    };
                    if (!network.profile.empty()) {
                        add(Part::Forget,
                            network.profile == m_forgetProfile
                                ? Tr(L"Forget it?", L"Esquecer mesmo?")
                                : Tr(L"Forget", L"Esquecer"));
                    }
                    if (network.connected) {
                        add(Part::Disconnect,
                            Tr(L"Disconnect", L"Desconectar"));
                    } else {
                        add(Part::WifiConnect, Tr(L"Connect", L"Conectar"));
                    }
                }
            }
            m_status = {padding, y, width - padding, y + row};
            if (const std::wstring status = StatusText(); !status.empty()) {
                DWRITE_TEXT_METRICS metrics{};
                if (auto layout =
                        StatusLayout(status, m_status.right - m_status.left);
                    layout && SUCCEEDED(layout->GetMetrics(&metrics))) {
                    m_status.bottom = std::max(
                        m_status.bottom,
                        y + std::ceil(metrics.height) + std::round(16 * s));
                }
                y = m_status.bottom;
            }
            // The mobile hotspot and airplane mode, under the networks.
            m_hotspotRow = m_airplaneRow = {};
            if (m_view == View::Wifi &&
                (m_hotspot >= 0 || m_airplane >= 0)) {
                y += std::round(4 * s);
                if (m_hotspot >= 0) {
                    m_hotspotRow = {padding, y, width - padding, y + row};
                    y += row;
                }
                if (m_airplane >= 0) {
                    m_airplaneRow = {padding, y, width - padding, y + row};
                    y += row;
                }
            }
        } else if (m_view == View::AppPlaces) {
            // A row per app with its place and its "x"; under it, its icons
            // (all but the clock, what's playing with its two parts), the
            // clock's parts, and its room from the edges and size; then the
            // open apps to add. As wide as all its icons on one line (if the
            // screen has room), or as the pill it came out of.
            const float listTop = y;
            const float row = std::round(44 * s);
            const float gridWidth = std::round(50 * s);
            const float gridHeight = std::round(30 * s);
            const float remove = std::round(28 * s);
            const float chips = std::round(32 * s);
            // Its order: a strip like the one in the settings.
            const float stripLine = std::round(40 * s);
            const float itemGap = std::round(4 * s);
            const float itemChip = std::round(30 * s);
            const float inset = std::round(8 * s);
            const int itemCount = kPillGroups - 1 + 2;
            const float screen =
                (float)(m_monitorRect.right - m_monitorRect.left);
            width = std::max(
                {width, m_minWidth,
                 std::min(padding * 2 + inset * 2 +
                              itemCount * (itemChip + itemGap) - itemGap,
                          screen - std::round(48 * s))});
            m_appPlaceRows.clear();
            m_appPlaceGrids.clear();
            m_appPlaceRemoves.clear();
            m_appItemChips.clear();
            // As many on a line as fit, then on more lines.
            const int perLine = std::max(
                1, (int)((width - padding * 2 - inset * 2 + itemGap) /
                         (itemChip + itemGap)));
            const int iconLines = (itemCount + perLine - 1) / perLine;
            // Its room from the edges and size: side by side when there's
            // room, otherwise one under the other.
            const float lineWidth = width - padding * 2 - inset * 2;
            const bool sideBySide = lineWidth >= std::round(520 * s);
            const int sliderLines = sideBySide ? 1 : 2;
            m_appOrderStrips.clear();
            std::erase_if(m_orderChips, [](const OrderChip& chip) {
                return chip.strip >= (int)g_appPlaces.size();
            });
            for (size_t i = 0; i < g_appPlaces.size(); i++) {
                const D2D1_RECT_F rect{
                    padding, y, width - padding,
                    y + row + stripLine +
                        chips * (iconLines + 1 + sliderLines)};
                m_appPlaceRows.push_back(rect);
                m_appPlaceRemoves.push_back(
                    {rect.right - remove, y, rect.right, y + row});
                m_appPlaceGrids.push_back(
                    {rect.right - remove - std::round(6 * s) - gridWidth,
                     y + (row - gridHeight) / 2,
                     rect.right - remove - std::round(6 * s),
                     y + (row + gridHeight) / 2});
                const int base = (int)i * kAppChipStride;
                m_appOrderStrips.push_back(
                    {rect.left + inset, y + row - std::round(4 * s),
                     rect.right - inset, y + row + std::round(32 * s)});
                const float chipsTop = y + row + stripLine;
                float chipLeft = rect.left + inset;
                int shown = 0;
                auto addChip = [&](int index) {
                    if (shown && shown % perLine == 0) {
                        chipLeft = rect.left + inset;
                    }
                    const float top = chipsTop - std::round(2 * s) +
                                      chips * (shown / perLine);
                    m_appItemChips.push_back(
                        {base + index,
                         {chipLeft, top, chipLeft + itemChip,
                          top + itemChip - itemGap}});
                    chipLeft += itemChip + itemGap;
                    shown++;
                };
                for (int group = 0; group < kPillGroups; group++) {
                    // (The media's parts: their chips follow the note's.)
                    if ((PillGroup)group == PillGroup::Clock ||
                        (PillGroup)group == PillGroup::MediaTitle ||
                        (PillGroup)group == PillGroup::MediaButtons) {
                        continue;
                    }
                    addChip(group);
                    if ((PillGroup)group == PillGroup::Media) {
                        addChip(kMediaChipFirst);
                        addChip(kMediaChipFirst + 1);
                    }
                }
                // The clock's parts, and the clock in the middle.
                const float clockChip = std::floor(
                    (lineWidth - itemGap * kClockParts) / (kClockParts + 1));
                chipLeft = rect.left + inset;
                float top = chipsTop - std::round(2 * s) + chips * iconLines;
                for (int part = 0; part <= kClockParts; part++) {
                    m_appItemChips.push_back(
                        {base + kClockChipFirst + part,
                         {chipLeft, top, chipLeft + clockChip,
                          top + itemChip - itemGap}});
                    chipLeft += clockChip + itemGap;
                }
                // Its room from the edges and size: the row (its name and
                // value), the slider, and the button back to the one for all
                // apps.
                top += chips;
                const float sliderWidth =
                    sideBySide ? std::floor((lineWidth - itemGap) / 2)
                               : lineWidth;
                for (int which = 0; which < 2; which++) {
                    const float left =
                        rect.left + inset +
                        (sideBySide ? (sliderWidth + itemGap) * which : 0);
                    const float lineTop =
                        top + (sideBySide ? 0 : chips * which);
                    const float bottom = lineTop + itemChip - itemGap;
                    const float reset = itemChip;
                    const int first = kStepChipFirst + which * 3;
                    const D2D1_RECT_F line{left, lineTop, left + sliderWidth,
                                           bottom};
                    m_appItemChips.push_back(
                        {base + first + 1,
                         {left + std::round(sliderWidth * 0.5f), lineTop,
                          line.right - reset - itemGap, bottom}});
                    m_appItemChips.push_back(
                        {base + first + 2,
                         {line.right - reset, lineTop, line.right, bottom}});
                    m_appItemChips.push_back({base + first, line});
                }
                y += row + stripLine + chips * (iconLines + 1 + sliderLines);
            }
            for (size_t i = 0; i < g_appPlaces.size(); i++) {
                LayoutOrderChips((int)i);
            }
            const float label = std::round(28 * s);
            m_addAppsHeader = {padding, y, width - padding, y + label};
            y += label;
            // The open apps to add, as many on a line as fit.
            const float chip = std::round(34 * s);
            const float chipGap = std::round(6 * s);
            const int appColumns = std::clamp(
                (int)((width - padding * 2 + chipGap) /
                      (std::round(150 * s) + chipGap)),
                2, 4);
            const float chipWidth =
                (width - padding * 2 - chipGap * (appColumns - 1)) /
                appColumns;
            m_openAppChips.clear();
            for (size_t i = 0; i < m_openApps.size(); i++) {
                const float left =
                    padding + (chipWidth + chipGap) * (i % appColumns);
                m_openAppChips.push_back(
                    {std::round(left), y, std::round(left + chipWidth), y + chip});
                if (i % appColumns == (size_t)appColumns - 1 ||
                    i + 1 == m_openApps.size()) {
                    y += chip + chipGap;
                }
            }
            m_status = {padding, y, width - padding, y + label * 1.5f};
            y += label * 1.5f;
            // As tall as the list, up to the room on the screen (then it
            // scrolls, like the settings).
            const float content = y - listTop;
            const float viewport = std::min(
                content,
                std::max(std::round(160 * s), MaxHeight() - listTop - padding));
            m_appPlacesViewport = {0, listTop, width, listTop + viewport};
            m_appPlacesScroll.SetSizes(viewport, content);
            y = listTop + viewport;
        } else if (m_view == View::Project ||
                   m_view == View::Accessibility) {
            const size_t count =
                m_view == View::Project ? 4 : m_accessibility.size();
            const float row = std::round(kRowHeight * s);
            for (size_t i = 0; i < count; i++) {
                m_rows.push_back({padding, y, width - padding, y + row});
                y += row;
            }
        } else if (m_view == View::Password) {
            y += std::round(4 * s);
            const float field = std::round(36 * s);
            m_field = {padding, y, width - padding, y + field};
            m_eye = {m_field.right - field, m_field.top, m_field.right,
                     m_field.bottom};
            y += field + gap;
            const float button = std::round(32 * s);
            m_connect = {width - padding - std::round(100 * s), y,
                         width - padding, y + button};
            m_status = {padding, y, m_connect.left - gap, y + button};
            y += button;
            // Too long beside the button (why joining failed): on lines of
            // its own under it.
            DWRITE_TEXT_METRICS metrics{};
            if (const std::wstring status = StatusText(); !status.empty()) {
                if (auto layout =
                        StatusLayout(status, m_status.right - m_status.left);
                    layout && SUCCEEDED(layout->GetMetrics(&metrics)) &&
                    metrics.lineCount > 1) {
                    layout = StatusLayout(status, width - padding * 2);
                    if (layout && SUCCEEDED(layout->GetMetrics(&metrics))) {
                        y += std::round(4 * s);
                        m_status = {padding, y, width - padding,
                                    y + std::ceil(metrics.height) +
                                        std::round(8 * s)};
                        y = m_status.bottom;
                    }
                }
            }
        } else if (m_view == View::Hotspot) {
            // Its name's field, its password's (with the eye), "Save" and
            // what happened beside it.
            y += std::round(4 * s);
            const float field = std::round(36 * s);
            m_hotspotField = {padding, y, width - padding, y + field};
            y += field + gap;
            m_field = {padding, y, width - padding, y + field};
            m_eye = {m_field.right - field, m_field.top, m_field.right,
                     m_field.bottom};
            y += field + gap;
            const float button = std::round(32 * s);
            m_connect = {width - padding - std::round(100 * s), y,
                         width - padding, y + button};
            m_status = {padding, y, m_connect.left - gap, y + button};
            y += button;
        } else if (m_view == View::PowerPlans) {
            const float row = std::round(34 * s);
            for (size_t i = 0; i < m_powerPlans.size(); i++) {
                m_rows.push_back({padding, y, width - padding, y + row});
                y += row;
            }
        } else if (m_view == View::Output) {
            const float row = std::round(kRowHeight * s);
            for (size_t i = 0; i < m_outputs.size(); i++) {
                m_rows.push_back({padding, y, width - padding, y + row});
                y += row;
            }
        } else {
            const float row = std::round(kMixerRowHeight * s);
            const float icon = std::round(30 * s);
            const float slider = std::round(18 * s);
            // The device's volume first, then one row per app.
            for (size_t i = 0; i < m_apps.size() + 1; i++) {
                const D2D1_RECT_F rect{padding, y, width - padding, y + row};
                m_rows.push_back(rect);
                const float middle = (rect.top + rect.bottom) / 2;
                m_rowIcons.push_back({rect.left + std::round(4 * s),
                                      middle - icon / 2,
                                      rect.left + std::round(4 * s) + icon,
                                      middle + icon / 2});
                const float sliderLeft =
                    rect.left + std::round(4 * s) + icon + std::round(12 * s);
                const float sliderTop = middle + std::round(1 * s);
                m_rowSliders.push_back({sliderLeft, sliderTop,
                                        rect.right - std::round(6 * s),
                                        sliderTop + slider});
                y += row;
            }
        }
        y += padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
        return;
    }

    // The chosen controls, in order: tiles two by two, buttons four by four,
    // sections across. Taller than the screen has room for (many controls):
    // laid out again wider, with twice (three times...) as many on a line,
    // as long as the monitor is wide enough.
    LoadControls();
    const float baseWidth = width;
    const float widest = std::max(
        baseWidth,
        (float)(m_monitorRect.right - m_monitorRect.left) - std::round(48 * s));
    for (int wide = 1;; wide++) {
    width = padding * 2 + (baseWidth - padding * 2) * wide + gap * (wide - 1);
    y = padding;
    m_controlRects.clear();
    m_networkIcon = m_bluetoothIcon = m_dndIcon = m_hotspotIcon = {};
    m_display = m_sound = m_slider = m_volumeIcon = m_deviceRow =
        m_mixerButton = m_batteryTile = m_powerModeRect = {};
    m_brightnessSliders.clear();
    const int tileColumns = 2 * wide;
    const int buttonColumns = 4 * wide;
    const float tile = std::round(kTileHeight * s);
    const float half =
        (width - padding * 2 - gap * (tileColumns - 1)) / tileColumns;
    const float shortcut = std::round(kShortcutHeight * s);
    const float shortcutWidth =
        (width - padding * 2 - gap * (buttonColumns - 1)) / buttonColumns;
    // The tiles' round icons switch them (see DrawTile).
    auto iconOf = [s](const D2D1_RECT_F& rect) {
        const float circle = std::round(17 * s);
        const float x = rect.left + std::round(12 * s) + circle;
        const float y = (rect.top + rect.bottom) / 2;
        return D2D1_RECT_F{x - circle, y - circle, x + circle, y + circle};
    };
    ControlKind rowKind = ControlKind::Section;
    int column = 0;
    auto endRow = [&]() {
        if (column > 0) {
            y += (rowKind == ControlKind::Tile ? tile : shortcut) + gap;
        }
        column = 0;
    };
    // Opened from the volume, the battery or the brightness: only those.
    static const std::vector<ControlId> kSoundOnly{ControlId::Sound};
    static const std::vector<ControlId> kBatteryOnly{ControlId::Battery,
                                                     ControlId::PowerMode};
    static const std::vector<ControlId> kDisplayOnly{ControlId::Display};
    // (komorebi's button on the pill: only its section.)
    static const std::vector<ControlId> kKomorebiOnly{ControlId::Komorebi};
    for (ControlId id : m_only == Only::Sound      ? kSoundOnly
                        : m_only == Only::Battery  ? kBatteryOnly
                        : m_only == Only::Display  ? kDisplayOnly
                        : m_only == Only::Komorebi ? kKomorebiOnly
                                                   : m_controls) {
        const ControlInfo& info = GetControlInfo(id);
        if (info.kind != rowKind) {
            endRow();
            rowKind = info.kind;
        }
        D2D1_RECT_F rect{};
        if (info.kind == ControlKind::Tile) {
            const float left = padding + (half + gap) * column;
            rect = {std::round(left), y, std::round(left + half), y + tile};
            if (++column == tileColumns) {
                endRow();
            }
        } else if (info.kind == ControlKind::Button) {
            const float left = padding + (shortcutWidth + gap) * column;
            rect = {std::round(left), y, std::round(left + shortcutWidth),
                    y + shortcut};
            if (++column == buttonColumns) {
                endRow();
            }
        } else if (id == ControlId::Display) {
            // Only when a monitor lets it be changed (while editing, its
            // place is shown anyway). One slider; with more monitors, one
            // for each, under its name.
            if (m_brightness < 0 && !m_editing) {
                continue;
            }
            const size_t rows =
                std::max<size_t>(1, m_brightnessMonitors.size());
            const float rowHeight = std::round(kBrightnessRowHeight * s);
            const float display =
                rows > 1 ? std::round(40 * s) + rowHeight * rows
                         : std::round(kDisplayHeight * s);
            rect = {padding, y, width - padding, y + display};
            if (m_brightness >= 0) {
                m_display = rect;
                const float inner = std::round(12 * s);
                for (size_t row = 0; row < rows; row++) {
                    const float sliderTop =
                        y + std::round(34 * s) +
                        (rows > 1 ? rowHeight * row + std::round(20 * s) : 0);
                    m_brightnessSliders.push_back(
                        {padding + inner, sliderTop, width - padding - inner,
                         sliderTop + std::round(26 * s)});
                }
            }
            y += display + gap;
        } else if (id == ControlId::Sound) {
            const float sound = std::round(kSoundHeight * s);
            rect = m_sound = {padding, y, width - padding, y + sound};
            const float inner = std::round(12 * s);
            const float sliderHeight = std::round(26 * s);
            const float sliderTop = y + std::round(34 * s);
            m_slider = {padding + inner, sliderTop, width - padding - inner,
                        sliderTop + sliderHeight};
            m_volumeIcon = {m_slider.left, m_slider.top,
                            m_slider.left + sliderHeight * 1.2f,
                            m_slider.bottom};
            const float bottomTop = m_slider.bottom + std::round(4 * s);
            const float bottomBottom = y + sound - std::round(4 * s);
            const float mixerWidth = std::round(64 * s);
            m_mixerButton = {width - padding - inner - mixerWidth, bottomTop,
                             width - padding - inner, bottomBottom};
            m_deviceRow = {padding + inner, bottomTop,
                           m_mixerButton.left - std::round(8 * s),
                           bottomBottom};
            y += sound + gap;
        } else if (id == ControlId::Battery) {
            if (!m_hasBattery && !m_editing) {
                continue;
            }
            const float battery = std::round(kBatteryHeight * s);
            rect = {padding, y, width - padding, y + battery};
            if (m_hasBattery) {
                m_batteryTile = rect;
            }
            y += battery + gap;
        } else if (id == ControlId::PowerMode) {
            if (m_powerPlans.empty() && !m_editing) {
                continue;
            }
            // Its title, the active power plan (which opens their list),
            // then (with the balanced plan) the three power modes side by
            // side.
            const float title = std::round(34 * s);
            const float planRow = std::round(32 * s);
            const float modes = m_powerMode >= 0 ? std::round(38 * s) : 0;
            const float height =
                m_powerPlans.empty()
                    ? std::round(kPowerModeHeight * s)
                    : title + planRow + modes + std::round(8 * s);
            rect = {padding, y, width - padding, y + height};
            m_powerPlanRow = {};
            for (auto& choice : m_powerModeChoices) {
                choice = {};
            }
            if (!m_powerPlans.empty()) {
                m_powerModeRect = rect;
                const float inner = std::round(6 * s);
                float top = y + title;
                m_powerPlanRow = {padding + inner, top,
                                  width - padding - inner, top + planRow};
                top += planRow;
                if (m_powerMode >= 0) {
                    const float modeInner = std::round(10 * s);
                    const float between = std::round(6 * s);
                    const float choice = (width - padding * 2 - modeInner * 2 -
                                          between * 2) /
                                         3;
                    top += std::round(4 * s);
                    for (int i = 0; i < 3; i++) {
                        const float left =
                            padding + modeInner + (choice + between) * i;
                        m_powerModeChoices[i] = {std::round(left), top,
                                                 std::round(left + choice),
                                                 top + std::round(30 * s)};
                    }
                }
            }
            y += height + gap;
        } else if (id == ControlId::Komorebi) {
            // Its three buttons side by side (start, stop, reload), its
            // version under them (like yasb's).
            const float inner = std::round(10 * s);
            const float between = std::round(8 * s);
            const float button = std::round(50 * s);
            const float height = inner + button + std::round(26 * s);
            rect = {padding, y, width - padding, y + height};
            const float each =
                (width - padding * 2 - inner * 2 - between * 2) / 3;
            for (int i = 0; i < 3; i++) {
                const float left = padding + inner + (each + between) * i;
                m_komorebiButtons[i] = {std::round(left), y + inner,
                                        std::round(left + each),
                                        y + inner + button};
            }
            y += height + gap;
        }
        switch (id) {
            case ControlId::Network:
                m_networkIcon = iconOf(rect);
                break;
            case ControlId::Bluetooth:
                m_bluetoothIcon = iconOf(rect);
                break;
            case ControlId::DoNotDisturb:
                m_dndIcon = iconOf(rect);
                break;
            case ControlId::Hotspot:
                m_hotspotIcon = iconOf(rect);
                break;
            default:
                break;
        }
        m_controlRects.push_back({id, rect});
    }
    endRow();
    y -= gap / 2;

    const float footer = std::round(kFooterHeight * s);
    m_addRects.clear();
    if (m_only != Only::All) {
        // Only the sound, the battery or the brightness: no footer.
        m_done = m_addHeader = m_more = m_edit = m_settingsButton = {};
        y += padding / 2;
    } else if (m_editing) {
        // The controls not shown, to add, three by three (more, wider);
        // then "Done".
        const float label = std::round(26 * s);
        m_addHeader = {padding, y, width - padding, y + label};
        y += label;
        const float chip = std::round(34 * s);
        const float chipGap = std::round(6 * s);
        const int chipColumns = 3 * wide;
        const float chipWidth =
            (width - padding * 2 - chipGap * (chipColumns - 1)) / chipColumns;
        int index = 0;
        for (const auto& info : kControls) {
            if (std::find(m_controls.begin(), m_controls.end(), info.id) !=
                m_controls.end()) {
                continue;
            }
            const float left =
                padding + (chipWidth + chipGap) * (index % chipColumns);
            m_addRects.push_back(
                {info.id,
                 {std::round(left), y, std::round(left + chipWidth), y + chip}});
            if (++index % chipColumns == 0) {
                y += chip + chipGap;
            }
        }
        if (index % chipColumns) {
            y += chip + chipGap;
        }
        m_status = {};
        y += std::round(4 * s);
        const float doneWidth = std::round(110 * s);
        m_done = {width - padding - doneWidth, y, width - padding, y + footer};
        m_more = m_edit = m_settingsButton = {};
    } else {
        // "More Windows controls", the island's settings (a gear, like the
        // bubble of a right click on the island) and "Edit".
        m_done = m_addHeader = {};
        const float editWidth = std::round(84 * s);
        m_edit = {width - padding - editWidth, y, width - padding, y + footer};
        m_settingsButton = {m_edit.left - gap - footer, y, m_edit.left - gap,
                            y + footer};
        m_more = {padding, y, m_settingsButton.left - gap, y + footer};
    }
    if (m_only == Only::All) {
        y += footer + padding / 2;
    }
    const float wider =
        padding * 2 + (baseWidth - padding * 2) * (wide + 1) + gap * wide;
    if (m_only != Only::All || y <= MaxHeight() || wider > widest) {
        break;
    }
    }

    // Each control is shown where it is, following it with a spring (see
    // AnimateContents); a new one starts at its place.
    for (const auto& [id, rect] : m_controlRects) {
        if (!FindMotion(id)) {
            m_motions.push_back({id, rect.left, rect.top});
        }
    }
    std::erase_if(m_motions, [this](const Motion& motion) {
        return std::none_of(m_controlRects.begin(), m_controlRects.end(),
                            [&](const auto& entry) {
                                return entry.first == motion.id;
                            });
    });
    // Something moved (a section shown, another order): it slides there.
    for (const auto& [id, rect] : m_controlRects) {
        const Motion* motion = FindMotion(id);
        if (motion && (std::fabs(motion->x - rect.left) > 0.5 ||
                       std::fabs(motion->y - rect.top) > 0.5)) {
            StartIslandAnimation();
            break;
        }
    }

    m_size = {(LONG)width, (LONG)std::ceil(y)};
}

// The settings' entries, in order: a section's title, then its options.
struct SettingSpec {
    int kind;
    int value;
    int section;
    // Kept right under the option before (which it depends on).
    int follows = 0;
};

PCWSTR ThemeName(int index);

// Whether a choice is shown as pictures: a card for each, a drawing over
// its name (the bar's side and inner corners, the banner's corners, the
// setup assistant's styles); the banners' place is a little screen of its
// own (see PlaceChoicePictures).
bool PictureChoice(int id) {
    return id == kChoiceBarSide || id == kChoiceBarInnerCorners ||
           id == kChoiceBannerCorners || id == kChoiceSetupStyle;
}

