#pragma once
void ControlPanel::DrawMixer(ID2D1RenderTarget* target,
                             ID2D1SolidColorBrush* brush) {
    DrawHeader(target, brush, Tr(L"Volume mixer", L"Mixer de volume"));
    for (size_t i = 0; i < m_rows.size(); i++) {
        const int index = (int)i - 1;
        const D2D1_RECT_F& iconRect = m_rowIcons[i];
        const D2D1_RECT_F& sliderRect = m_rowSliders[i];
        const bool hoverIcon =
            (index < 0 && m_hover.part == Part::VolumeIcon) ||
            (m_hover.part == Part::AppIcon && m_hover.index == index);

        float level;
        bool muted;
        std::wstring name;
        if (index < 0) {
            level = m_volume;
            muted = m_muted;
            name = m_device.empty() ? std::wstring(Tr(L"Device", L"Dispositivo"))
                                    : m_device;
        } else {
            const MixerApp& app = m_apps[index];
            level = app.volume;
            muted = app.muted;
            name = app.name;
        }

        // The icon: the app's, or a glyph for the device and system sounds.
        if (const float lit = Hover(iconRect, hoverIcon); lit > 0.01f) {
            const float radius = (iconRect.bottom - iconRect.top) / 2;
            brush->SetColor(Fg(0.14f * lit));
            GlassFill(target, {iconRect, radius, radius}, brush);
        }
        ID2D1Bitmap* bitmap =
            index >= 0 ? MixerAppBitmap(target, m_wic.get(), m_apps[index])
                       : nullptr;
        if (bitmap) {
            const float inset = std::round(5 * m_scale);
            DrawBitmapSmooth(target, bitmap,
                             {iconRect.left + inset, iconRect.top + inset,
                              iconRect.right - inset, iconRect.bottom - inset},
                             muted ? 0.4f : 1.0f);
        } else {
            const WCHAR glyph = index < 0 ? kGlyphHeadphones : kGlyphSpeakers;
            brush->SetColor(Fg(muted ? 0.4f : 0.9f));
            DrawGlyph(target, &glyph, 1, m_iconFormat.get(), iconRect, brush);
        }
        if (muted) {
            brush->SetColor(D2D1::ColorF(1.0f, 0.27f, 0.23f, 1));
            DrawGlyph(target, &kGlyphMute, 1, m_iconFormat.get(),
                      {iconRect.right - std::round(14 * m_scale),
                       iconRect.bottom - std::round(14 * m_scale),
                       iconRect.right + std::round(2 * m_scale),
                       iconRect.bottom + std::round(2 * m_scale)},
                      brush);
        }

        // The name above the slider, its level at the end (like the Sound
        // tile's).
        const D2D1_RECT_F above{sliderRect.left,
                                iconRect.top - std::round(2 * m_scale),
                                sliderRect.right, sliderRect.top};
        const std::wstring percent =
            muted ? std::wstring(Tr(L"Muted", L"Mudo"))
                  : std::to_wstring((int)std::lround(level * 100)) + L"%";
        brush->SetColor(Fg(0.6f));
        target->DrawText(name.c_str(), (UINT32)name.size(),
                         m_subtitleFormat.get(),
                         {above.left, above.top,
                          above.right - std::round(48 * m_scale), above.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        brush->SetColor(Fg(0.55f));
        target->DrawText(percent.c_str(), (UINT32)percent.size(),
                         m_percentFormat.get(), above, brush);
        DrawSlider(target, brush, sliderRect, muted ? 0 : level, 0, false);
    }
}

void ControlPanel::Draw(ID2D1RenderTarget* target,
                        ID2D1SolidColorBrush* brush) {
    // Found again while drawing (see DrawMarquee). Not cleared between
    // drawings: frames skipped for the lower frame rate don't draw.
    m_marqueeActive = false;
    if (!m_iconFormat || !m_titleFormat || !m_subtitleFormat ||
        !m_sectionFormat || !m_percentFormat || !m_linkFormat ||
        !m_smallFormat || !m_smallIconFormat || !m_badgeFormat) {
        return;
    }
    // A view coming in: faded and moved aside a little, on top of the
    // panel's own placement.
    const float in = (float)std::clamp(m_viewIn, 0.0, 1.05);
    winrt::com_ptr<ID2D1Layer> viewLayer;
    D2D1_MATRIX_3X2_F base;
    target->GetTransform(&base);
    if (in < 0.999f) {
        target->CreateLayer(nullptr, viewLayer.put());
        target->PushLayer(
            D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                  D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                  D2D1::IdentityMatrix(),
                                  std::clamp(in * 1.3f, 0.0f, 1.0f)),
            viewLayer.get());
        target->SetTransform(
            D2D1::Matrix3x2F::Translation(
                m_viewDirection * (1 - in) * std::round(28 * m_scale), 0) *
            *D2D1::Matrix3x2F::ReinterpretBaseType(&base));
    }
    // (The places per app in the settings' window: on a card of their own.)
    if (m_view == View::AppPlaces) {
        const float radius = std::round(8 * m_scale);
        brush->SetColor(Fg(0.05f));
        GlassFill(target,
                  {{0, 0, (float)m_size.cx, (float)m_size.cy}, radius, radius},
                  brush);
    }
    DrawView(target, brush);
    if (viewLayer) {
        target->SetTransform(base);
        target->PopLayer();
    }
}

void ControlPanel::DrawView(ID2D1RenderTarget* target,
                            ID2D1SolidColorBrush* brush) {
    switch (m_view) {
        case View::Main:
            DrawMain(target, brush);
            break;
        case View::Output:
            DrawOutputs(target, brush);
            break;
        case View::PowerPlans:
            DrawPowerPlans(target, brush);
            break;
        case View::Mixer:
            DrawMixer(target, brush);
            break;
        case View::Wifi:
            DrawWifi(target, brush);
            break;
        case View::Password:
            DrawPassword(target, brush);
            break;
        case View::Hotspot:
            DrawHotspot(target, brush);
            break;
        case View::Bluetooth:
            DrawBluetooth(target, brush);
            break;
        case View::Project:
            DrawProject(target, brush);
            break;
        case View::Accessibility:
            DrawAccessibility(target, brush);
            break;
        case View::Settings:
            DrawSettings(target, brush);
            break;
        case View::AppPlaces:
            DrawAppPlaces(target, brush);
            break;
    }
}

void ControlPanel::SetVolumeAt(POINT pt) {
    if (!g_island) {
        return;
    }
    const D2D1_RECT_F& rect =
        m_view == View::Mixer && !m_rowSliders.empty() ? m_rowSliders[0]
                                                       : m_slider;
    const float knob = rect.bottom - rect.top;
    const float level = std::clamp(
        (pt.x - rect.left - knob / 2) / (rect.right - rect.left - knob), 0.0f,
        1.0f);
    if (auto volume = g_island->GetVolume(nullptr)) {
        volume->SetMasterVolumeLevelScalar(level, nullptr);
        volume->SetMute(level <= 0.001f, nullptr);
    }
    m_volume = level;
    m_muted = level <= 0.001f;
    g_island->OnVolumeChanged();
}

void ControlPanel::SetAppVolume(int index, float volume) {
    if (index < 0 || index >= (int)m_apps.size()) {
        return;
    }
    MixerApp& app = m_apps[index];
    app.volume = std::clamp(volume, 0.0f, 1.0f);
    app.muted = false;
    for (auto& session : app.volumes) {
        session->SetMasterVolume(app.volume, nullptr);
        session->SetMute(FALSE, nullptr);
    }
}

void ControlPanel::SetAppVolumeAt(int index, POINT pt) {
    if (index < 0 || index + 1 >= (int)m_rowSliders.size()) {
        return;
    }
    const D2D1_RECT_F& rect = m_rowSliders[index + 1];
    const float knob = rect.bottom - rect.top;
    SetAppVolume(index, (pt.x - rect.left - knob / 2) /
                            (rect.right - rect.left - knob));
}

void ControlPanel::DragNotchChip(float middle) {
    const int count = NotchCountWanted();
    const auto dragged = (PillGroup)m_orderDrag;
    // The notch it's over (the nearest one).
    int place = m_notchParts.front().first;
    float nearest = std::numeric_limits<float>::max();
    for (const auto& [at, part] : m_notchParts) {
        const float distance = middle < part.left    ? part.left - middle
                               : middle > part.right ? middle - part.right
                                                     : 0;
        if (distance < nearest) {
            nearest = distance;
            place = at;
        }
    }
    const bool moved = NotchOfGroup(dragged, count) != place;
    if (moved) {
        g_notchSides[(int)dragged] = (uint8_t)place;
        SaveNotchSides();
    }
    // After the last of that notch's items whose middle it has passed, or
    // before its first one (alone there: where it was).
    const std::vector<PillGroup> before = g_pillOrder;
    std::vector<PillGroup> order;
    size_t at = 0;
    for (size_t i = 0; i < before.size(); i++) {
        if (before[i] == dragged) {
            at = order.size();
        } else {
            order.push_back(before[i]);
        }
    }
    size_t first = order.size();
    bool passed = false;
    for (size_t i = 0; i < order.size(); i++) {
        const OrderChip* chip = FindOrderChip(order[i]);
        if (!chip || NotchOfGroup(order[i], count) != place) {
            continue;
        }
        first = std::min(first, i);
        if ((chip->rect.left + chip->rect.right) / 2 < middle) {
            at = i + 1;
            passed = true;
        }
    }
    if (!passed && first < order.size()) {
        at = first;
    }
    order.insert(order.begin() + std::min(at, order.size()), dragged);
    if (order != before) {
        SetPillOrder(order);
    }
    if (moved || order != before) {
        LayoutOrderChips();
        if (moved && g_island) {
            ForEachIsland([](Island& island) { island.Relayout(); });
        }
    }
}

bool ControlPanel::OnMouseMove(POINT pt) {
    if (m_barDrag) {
        DragScrollBar(pt);
        return true;
    }
    if (m_view == View::Settings) {
        const int bar =
            m_settingsScroll.Max() > 0 &&
                    PointInRect(pt, ScrollBarStrip(m_settingsViewport))
                ? 1
            : m_navScroll.Max() > 0 &&
                    PointInRect(pt, ScrollBarStrip(m_navViewport, kNavBarStrip))
                ? 2
                : 0;
        if (bar != m_barHover) {
            m_barHover = bar;
            StartIslandAnimation();
        }
        if (bar) {
            const bool changed = m_hover.part != Part::None;
            m_hover = {};
            return changed;
        }
    }
    // Dragging an item of the order strip: it follows the mouse, and the
    // others (and the island's items) make way as it passes them.
    if (m_orderDrag >= 0 && (GetKeyState(VK_LBUTTON) & 0x8000)) {
        const int threshold = (int)std::round(4 * m_scale);
        if (!m_orderMoved && std::abs(pt.x - m_orderStartX) > threshold) {
            m_orderMoved = true;
        }
        const int strip = m_orderDragStrip;
        OrderChip* chip = FindOrderChip((PillGroup)m_orderDrag, strip);
        if (!m_orderMoved || !chip) {
            return false;
        }
        const D2D1_RECT_F& stripRect = StripRect(strip);
        const float width = chip->rect.right - chip->rect.left;
        m_orderDragLeft = std::clamp(
            (float)StripPoint(strip, pt).x - m_orderGrab, stripRect.left,
            std::max(stripRect.left, stripRect.right - width));
        // Its place: after the last shown one whose middle it has passed
        // (the hidden ones keep theirs).
        const float middle = m_orderDragLeft + width / 2;
        if (strip < 0 && !m_notchParts.empty()) {
            DragNotchChip(middle);
            StartIslandAnimation();
            return false;
        }
        const std::vector<PillGroup> before = StripOrder(strip);
        std::vector<PillGroup> order;
        for (PillGroup group : before) {
            if ((int)group != m_orderDrag) {
                order.push_back(group);
            }
        }
        size_t index = 0;
        for (size_t i = 0; i < order.size(); i++) {
            const OrderChip* other = FindOrderChip(order[i], strip);
            if (other && (other->rect.left + other->rect.right) / 2 < middle) {
                index = i + 1;
            }
        }
        order.insert(order.begin() + index, (PillGroup)m_orderDrag);
        if (order != before) {
            SetStripOrder(strip, order);
            LayoutOrderChips(strip);
        }
        StartIslandAnimation();
        return false;
    }
    // Dragging a control while editing: it follows the mouse, and takes the
    // place of the one under it (the others slide out of the way).
    if (m_dragId >= 0 && (GetKeyState(VK_LBUTTON) & 0x8000)) {
        const int threshold = (int)std::round(5 * m_scale);
        if (!m_dragMoved && (std::abs(pt.x - m_dragStart.x) > threshold ||
                             std::abs(pt.y - m_dragStart.y) > threshold)) {
            m_dragMoved = true;
        }
        if (!m_dragMoved) {
            return false;
        }
        m_dragPoint = pt;
        for (const auto& [id, rect] : m_controlRects) {
            if ((int)id == m_dragId || !PointInRect(pt, rect)) {
                continue;
            }
            auto from = std::find(m_controls.begin(), m_controls.end(),
                                  (ControlId)m_dragId);
            auto to = std::find(m_controls.begin(), m_controls.end(), id);
            if (from != m_controls.end() && to != m_controls.end()) {
                const ControlId moved = *from;
                const int toIndex = (int)(to - m_controls.begin());
                m_controls.erase(from);
                m_controls.insert(m_controls.begin() + toIndex, moved);
                Layout();
            }
            break;
        }
        // Drawn by the animation, once per frame: not on every mouse move
        // too (a mouse can send hundreds a second).
        StartIslandAnimation();
        return false;
    }
    if (m_dragging) {
        if (m_dragIndex <= kDragAppSlider) {
            const int code = kDragAppSlider - m_dragIndex;
            SetAppSliderAt(code / 2, code % 2, pt, false);
        } else if (m_dragIndex <= kDragIslandSize) {
            SetIslandSizeAt(kDragIslandSize - m_dragIndex, pt, false);
        } else if (m_dragIndex == kDragBrightness) {
            SetBrightnessAt(m_brightnessRow, pt);
        } else if (m_dragIndex < 0) {
            SetVolumeAt(pt);
        } else {
            SetAppVolumeAt(m_dragIndex, pt);
        }
        return true;
    }
    const Hit hover = HitTest(pt);
    if (hover == m_hover) {
        return false;
    }
    m_hover = hover;
    return true;
}

bool ControlPanel::OnMouseLeave() {
    if (m_barHover) {
        m_barHover = 0;
        StartIslandAnimation();
    }
    if (m_hover.part == Part::None) {
        return false;
    }
    m_hover = {};
    return true;
}

bool ControlPanel::OnMouseDown(POINT pt) {
    // The settings' scroll bars: dragged by their thumb, or the list jumps
    // where the bar is clicked.
    if (m_view == View::Settings) {
        for (int bar = 1; bar <= 2; bar++) {
            const D2D1_RECT_F& viewport =
                bar == 2 ? m_navViewport : m_settingsViewport;
            const SmoothScroll& scroll =
                bar == 2 ? m_navScroll : m_settingsScroll;
            if (scroll.Max() > 0 && PointInRect(pt, ScrollBarStrip(viewport, bar == 2 ? kNavBarStrip : 14))) {
                float top = 0;
                float thumb = 0;
                ScrollThumb(viewport, scroll, &top, &thumb);
                m_barGrab = pt.y >= top && pt.y < top + thumb ? pt.y - top
                                                              : thumb / 2;
                m_barDrag = bar;
                SetCapture(m_hwnd);
                DragScrollBar(pt);
                return true;
            }
        }
    }
    const Hit hit = HitTest(pt);
    // In a field: the caret goes where it's clicked.
    if (hit.part == Part::SearchField) {
        EditTextClick(m_search, m_searchEdit, pt);
        return true;
    }
    if (hit.part == Part::ProfileName && m_profileTyping) {
        EditTextClick(m_profileName, m_profileNameEdit, ToSettingsList(pt));
        return true;
    }
    if (hit.part == Part::PasswordField) {
        m_hotspotTypingName = false;
        EditTextClick(m_password, m_passwordEdit, pt);
        return true;
    }
    if (hit.part == Part::HotspotName) {
        m_hotspotTypingName = true;
        EditTextClick(m_hotspotName, m_hotspotNameEdit, pt);
        return true;
    }
    // An item of the order strip: maybe dragged.
    if (hit.part == Part::OrderChip) {
        const int strip = hit.index / kOrderStripStride - 1;
        const int group = hit.index % kOrderStripStride;
        if (const OrderChip* chip = FindOrderChip((PillGroup)group, strip)) {
            m_orderDrag = group;
            m_orderDragStrip = strip;
            m_orderMoved = false;
            m_orderStartX = pt.x;
            m_orderGrab = (float)StripPoint(strip, pt).x - chip->rect.left;
            m_orderDragLeft = chip->rect.left;
            SetCapture(m_hwnd);
        }
        return false;
    }
    // An app's room from the edges or size: dragged along its slider.
    if (hit.part == Part::AppItemChip) {
        const int entry = hit.index / kAppChipStride;
        const int part = hit.index % kAppChipStride - kStepChipFirst;
        if (part == 1 || part == 4) {
            const int which = part / 3;
            m_dragging = true;
            m_dragIndex = kDragAppSlider - (entry * 2 + which);
            SetCapture(m_hwnd);
            SetAppSliderAt(entry, which, pt, false);
            return true;
        }
    }
    // A control while editing: maybe dragged.
    if (m_view == View::Main && m_editing && hit.part == Part::EditControl) {
        for (const auto& [id, rect] : m_controlRects) {
            if ((int)id == hit.index) {
                m_dragId = hit.index;
                m_dragMoved = false;
                m_dragStart = m_dragPoint = pt;
                m_dragGrab = {pt.x - rect.left, pt.y - rect.top};
                SetCapture(m_hwnd);
            }
        }
        return false;
    }
    if (hit.part == Part::SizeSlider) {
        m_dragging = true;
        m_dragIndex = kDragIslandSize - hit.index;
        SetCapture(m_hwnd);
        SetIslandSizeAt(hit.index, pt, false);
        return true;
    }
    if (hit.part == Part::BrightnessSlider && hit.index >= 0 &&
        hit.index < (int)m_brightnessSliders.size() &&
        PointInRect(pt, m_brightnessSliders[hit.index])) {
        m_dragging = true;
        m_dragIndex = kDragBrightness;
        m_brightnessRow = hit.index;
        SetCapture(m_hwnd);
        SetBrightnessAt(hit.index, pt);
        return true;
    }
    if (hit.part == Part::Slider ||
        (hit.part == Part::AppSlider && hit.index < 0)) {
        m_dragging = true;
        m_dragIndex = -1;
        SetCapture(m_hwnd);
        SetVolumeAt(pt);
        return true;
    }
    if (hit.part == Part::AppSlider) {
        m_dragging = true;
        m_dragIndex = hit.index;
        SetCapture(m_hwnd);
        SetAppVolumeAt(hit.index, pt);
        return true;
    }
    return false;
}

bool ControlPanel::OnMouseUp(POINT pt) {
    if (m_barDrag) {
        m_barDrag = 0;
        ReleaseCapture();
        return true;
    }
    // The end of a drag in the order strip (kept), or a click on an item
    // (shown or hidden).
    if (m_orderDrag >= 0) {
        const PillGroup group = (PillGroup)m_orderDrag;
        const int strip = m_orderDragStrip;
        const bool moved = m_orderMoved;
        m_orderDrag = -1;
        m_orderDragStrip = -1;
        m_orderMoved = false;
        if (GetCapture() == m_hwnd) {
            ReleaseCapture();
        }
        if (moved) {
            if (strip < 0) {
                SavePillOrder();
                if (!m_notchParts.empty()) {
                    SaveNotchSides();
                    // The bell in another notch: that one shows the
                    // notifications now.
                    if (Island* main = MainIsland();
                        main && main->NotchBaseMoved()) {
                        RestartIslandSoon();
                    }
                }
            } else {
                SavePlaces();
            }
            StartIslandAnimation();
            return true;
        }
        const Hit hit = HitTest(pt);
        if (hit.part != Part::OrderChip ||
            hit.index != OrderChipIndex(strip, group)) {
            return true;
        }
        if (strip >= 0) {
            ToggleAppGroup(strip, (int)group);
            Layout();
            return true;
        }
        const int pref = PillGroupPref(group);
        if (pref >= 0) {
            SetIslandPref((IslandPref)pref,
                          GetIslandPref((IslandPref)pref) ? 0 : 1);
            LayoutOrderChips();
        }
        return true;
    }
    if (m_dragId >= 0) {
        const bool moved = m_dragMoved;
        // Dropped: it springs into its place from there.
        if (Motion* motion = FindMotion((ControlId)m_dragId); motion && moved) {
            motion->x = m_dragPoint.x - m_dragGrab.x;
            motion->y = m_dragPoint.y - m_dragGrab.y;
            motion->vx = motion->vy = 0;
            motion->scale = 1.06;
            motion->vscale = 0;
        }
        m_dragId = -1;
        m_dragMoved = false;
        if (GetCapture() == m_hwnd) {
            ReleaseCapture();
        }
        if (moved) {
            SaveControls();
            Layout();
            StartIslandAnimation();
            return true;
        }
    }
    if (m_dragging) {
        m_dragging = false;
        ReleaseCapture();
        if (m_dragIndex <= kDragAppSlider) {
            const int code = kDragAppSlider - m_dragIndex;
            SetAppSliderAt(code / 2, code % 2, pt, true);
        } else if (m_dragIndex <= kDragIslandSize) {
            SetIslandSizeAt(kDragIslandSize - m_dragIndex, pt, true);
        }
        return true;
    }
    const Hit hit = HitTest(pt);
    // A click elsewhere: the keys go back to the search (the name stays as
    // typed until "Done").
    if (m_profileTyping && hit.part != Part::ProfileName) {
        m_profileTyping = false;
    }
    // ...and "Forget" waits for its second click no more.
    if (hit.part != Part::Forget) {
        m_forgetProfile.clear();
    }
    // ...and the bubble of what an option needs closes.
    if (m_needsEntry >= 0 && hit.part != Part::NeedsYes &&
        hit.part != Part::NeedsNo && hit.part != Part::NeedsBox &&
        hit.part != Part::DisabledLink) {
        m_needsEntry = -1;
        StartIslandAnimation();
    }
    switch (hit.part) {
        case Part::Network:
            SetView(View::Wifi);
            return true;
        case Part::Bluetooth:
            SetView(View::Bluetooth);
            return true;
        case Part::WifiToggle:
            // Shown right away; the radio's thread confirms it.
            m_wifiRadio = m_wifiRadio == 1 ? 0 : 1;
            SwitchRadio(0, m_wifiRadio == 1);
            if (m_wifiRadio == 0) {
                m_networks.clear();
            }
            Layout();
            return true;
        case Part::BluetoothToggle:
            m_bluetoothRadio = m_bluetoothRadio == 1 ? 0 : 1;
            SwitchRadio(1, m_bluetoothRadio == 1);
            UpdateBluetooth();
            Layout();
            return true;
        case Part::BluetoothRow:
            ClickBluetoothDevice(hit.index);
            Layout();
            return true;
        case Part::WifiRow:
            JoinWifi(hit.index);
            Layout();
            return true;
        case Part::Disconnect:
            if (m_wlan && m_hasWifi) {
                WlanDisconnect(m_wlan, &m_wifiInterface, nullptr);
            }
            return true;
        case Part::Forget:
            ForgetWifi(hit.index);
            Layout();
            return true;
        case Part::WifiConnect:
            JoinWifi(hit.index);
            Layout();
            return true;
        case Part::WifiLocation:
            ShellExecute(nullptr, L"open", L"ms-settings:privacy-location",
                         nullptr, nullptr, SW_SHOWNORMAL);
            return true;
        case Part::ShowPassword:
            m_showPassword = !m_showPassword;
            return true;
        case Part::Connect:
            SubmitPassword();
            Layout();
            return true;
        case Part::DoNotDisturb:
            // The notifications, with its switch.
            if (g_island) {
                g_island->OpenNotifications();
            }
            break;
        case Part::DndToggle:
            if (SetIslandQuiet(!m_doNotDisturb)) {
                m_doNotDisturb = !m_doNotDisturb;
                if (g_island) {
                    ForEachIsland([&](Island& island) {
                        island.OnDoNotDisturbChanged();
                    });
                }
            }
            return true;
        case Part::NightLight:
            SwitchNightLight();
            return true;
        case Part::Shortcut:
            switch ((ControlId)hit.index) {
                case ControlId::Project:
                    UpdateDisplayMode();
                    SetView(View::Project);
                    return true;
                case ControlId::Cast:
                    Close();
                    RunControlShortcut(ControlId::Cast);
                    break;
                case ControlId::Captions:
                    RunControlShortcut(ControlId::Captions);
                    break;
                case ControlId::Accessibility:
                    UpdateAccessibility();
                    SetView(View::Accessibility);
                    return true;
                case ControlId::DarkMode:
                    SwitchDarkMode(!m_darkMode);
                    return true;
                case ControlId::Weather:
                    if (g_island) {
                        g_island->OpenWeather();
                    }
                    return false;
                case ControlId::Updates:
                    if (g_island) {
                        g_island->OpenUpdates();
                    }
                    return false;
                case ControlId::Notes:
                case ControlId::Tasks:
                case ControlId::Shortcuts:
                    // Opens out of the island in its place.
                    if (g_island) {
                        const auto id = (ControlId)hit.index;
                        g_island->OpenListPanel(
                            id == ControlId::Notes   ? IslandItem::Notes
                            : id == ControlId::Tasks ? IslandItem::Tasks
                                                     : IslandItem::Shortcuts);
                    }
                    return false;
                case ControlId::Microphone:
                    if (g_island) {
                        g_island->ToggleMicrophone();
                        m_microphoneMuted = g_island->MicrophoneMuted();
                    }
                    return true;
                case ControlId::Screenshot:
                    // After the panel is gone, so it isn't in the picture.
                    Close();
                    SetTimer(m_hwnd, kScreenshotTimerId, 450, nullptr);
                    break;
                case ControlId::Lock:
                case ControlId::Emoji:
                case ControlId::Clipboard:
                case ControlId::TaskManager:
                case ControlId::Calculator:
                case ControlId::Explorer:
                case ControlId::ShowDesktop:
                case ControlId::OnScreenKeyboard:
                    Close();
                    RunControlShortcut((ControlId)hit.index);
                    break;
                case ControlId::Stopwatch:
                case ControlId::Pomodoro:
                case ControlId::Alarms:
                    // The timer panel, on its tab.
                    if (g_island) {
                        const ControlId id = (ControlId)hit.index;
                        g_island->OpenTimerTab(
                            id == ControlId::Stopwatch  ? 1
                            : id == ControlId::Pomodoro ? 2
                                                        : 3);
                        return false;
                    }
                    break;
                default:
                    // A pill item's button: what the item opens (another
                    // panel, so this one closes).
                    if (const IslandItem item =
                            ControlItem((ControlId)hit.index);
                        item != IslandItem::None && g_island) {
                        if (item == IslandItem::PrivacyMicrophone) {
                            Close();
                        }
                        g_island->OpenFromControlCenter(item);
                        return false;
                    }
                    break;
            }
            break;
        case Part::EditButton:
            SetEditing(true);
            return true;
        case Part::SettingsButton:
            Close();
            OpenIslandSettingsFromPanel();
            return false;
        case Part::EditDone:
            SetEditing(false);
            return true;
        case Part::EditRemove: {
            const ControlId id = (ControlId)hit.index;
            auto it = std::find(m_controls.begin(), m_controls.end(), id);
            if (it == m_controls.end()) {
                return true;
            }
            // It shrinks away where it's shown; the others close the gap.
            for (const auto& [shown, rect] : m_controlRects) {
                if (shown == id) {
                    D2D1_RECT_F at = rect;
                    if (const Motion* motion = FindMotion(id)) {
                        const float dx = (float)motion->x - rect.left;
                        const float dy = (float)motion->y - rect.top;
                        at = {rect.left + dx, rect.top + dy, rect.right + dx,
                              rect.bottom + dy};
                    }
                    m_ghosts.push_back({id, at});
                }
            }
            m_controls.erase(it);
            SaveControls();
            Layout();
            StartIslandAnimation();
            return true;
        }
        case Part::EditAdd: {
            const ControlId id = (ControlId)hit.index;
            D2D1_RECT_F from{};
            for (const auto& [chip, rect] : m_addRects) {
                if (chip == id) {
                    from = rect;
                }
            }
            m_controls.push_back(id);
            SaveControls();
            Layout();
            // It grows in from where it was listed.
            if (Motion* motion = FindMotion(id)) {
                motion->x = from.left;
                motion->y = from.top;
                motion->scale = 0.4;
                motion->vscale = 0;
            }
            StartIslandAnimation();
            return true;
        }
        case Part::PageButton:
            if (hit.index == kPageWindhawk) {
                OpenWindhawk();
            } else if (hit.index == kPageCopyLogs) {
                m_logsStatus = CopyTextToClipboard(m_hwnd, LogsText())
                                   ? Tr(L"\u2713 Copied", L"\u2713 Copiado")
                                   : L"";
                Layout();
            } else if (hit.index == kPageSaveLogs) {
                StartLogsDialog();
            } else {
                static constexpr PCWSTR kLinks[] = {
                    L"https://windhawk.net/mods/mac-island-dock",
                    L"https://caliberda.com.br",
                    L"https://github.com/ramensoftware/windhawk-mods/issues/"
                    L"new?title=%5BMac%20Island%20%26%20Dock%5D%20&body="
                    L"Mod%20author%3A%20%40cesarkali%0A%0A",
                    L"https://www.instagram.com/cesar.kali",
                    L"https://island.caliberda.com.br"};
                const int link = hit.index - kPageLink;
                if (link >= 0 && link < (int)std::size(kLinks)) {
                    ShellExecute(nullptr, L"open", kLinks[link], nullptr,
                                 nullptr, SW_SHOWNORMAL);
                }
            }
            return true;
        case Part::DisabledLink: {
            // What it needs, in a bubble under it (where it's somewhere else
            // that can't be changed from here: that place).
            const int entry = hit.index / kDisabledStride;
            const int disabled = hit.index % kDisabledStride;
            if (disabled == (int)IslandPref::Frosted &&
                FrostedNeedsOnlyTransparency()) {
                ShellExecute(nullptr, L"open", L"ms-settings:colors", nullptr,
                             nullptr, SW_SHOWNORMAL);
            } else {
                OpenNeeds(entry);
                if (m_needsEntry < 0) {
                    GoToSetting(DisabledTarget(disabled));
                }
            }
            return true;
        }
        case Part::SetupButton:
            PressSetupButton(hit.index);
            return true;
        case Part::NeedsYes:
            ApplyNeeds();
            return true;
        case Part::NeedsNo:
            m_needsEntry = -1;
            StartIslandAnimation();
            return true;
        case Part::NeedsBox:
            return true;
        case Part::RestoreNeeds:
            RestoreNeeds();
            return true;
        case Part::NoteLink:
            if (hit.index >= 0 && hit.index < (int)m_noteLinks.size()) {
                const SettingTarget target = m_noteLinks[hit.index].first;
                GoToSetting(target);
            }
            return true;
        case Part::DisabledSetting:
            // The glass without Windows' transparency effects: where to turn
            // them on.
            if (hit.index == (int)IslandPref::Frosted &&
                FrostedNeedsOnlyTransparency()) {
                ShellExecute(nullptr, L"open", L"ms-settings:colors", nullptr,
                             nullptr, SW_SHOWNORMAL);
            }
            return true;
        case Part::SettingToggle: {
            const IslandPref pref = (IslandPref)hit.index;
            SetIslandPref(pref, GetIslandPref(pref) ? 0 : 1);
            // The order strip follows at once (an item turned on or off, the
            // clock kept in the middle).
            LayoutOrderChips();
            StartIslandAnimation();
            return true;
        }
        case Part::SizeReset:
            SetIslandSlider(hit.index, kIslandSliders[hit.index].usual, true);
            return true;
        case Part::ShortcutRecord:
            m_shortcutNote.clear();
            StartShortcutRecording(!m_recordingShortcut);
            Layout();
            return true;
        case Part::ShortcutReset:
            StartShortcutRecording(false);
            m_shortcutNote = SetMinimizeShortcut(kDefaultMinimizeShortcut);
            Layout();
            return true;
        case Part::ChoiceSegment: {
            const int id = hit.index / 16;
            const int index = hit.index % 16;
            if (id == kChoiceBackup && index == 2) {
                // (A second click within a few seconds resets them.)
                if (NowSeconds() - g_resetAskedAt < kResetAskSeconds) {
                    g_resetAskedAt = -100;
                    ResetIslandSettings();
                    m_backupStatus = Tr(L"Settings back to their defaults",
                                        L"Configurações de volta ao padrão");
                } else {
                    g_resetAskedAt = NowSeconds();
                    m_backupStatus =
                        Tr(L"Again to put every setting back to its default "
                           L"(notes, tasks, events and profiles stay)",
                           L"De novo para voltar todas as configurações ao "
                           L"padrão (anotações, tarefas, eventos e perfis "
                           L"ficam)");
                }
                Layout();
            } else if (id == kChoiceBackup) {
                StartBackupDialog(index == 0);
            } else if (id == kChoiceColors) {
                StartColorDialog(index);
            } else if (id == kChoiceDateOrder) {
                SetIslandPref(IslandPref::DateOrder, index);
            } else if (id == kChoiceClockFormat) {
                SetIslandPref(IslandPref::ClockFormat, index);
            } else if (id == kChoiceWeatherUnit) {
                SetIslandPref(IslandPref::WeatherUnit, index);
            } else if (id == kChoiceIconPack) {
                // (Your folder, without one: chosen first; an online pack
                // not downloaded: tried again.)
                IconSourceKind kind;
                std::wstring what;
                if (index == kIconPackFolder && g_iconFolder.empty()) {
                    StartIconFolderDialog(false);
                } else if (index == g_settings.islandIconPack &&
                           CurrentIconSource(&kind, &what)) {
                    RequestIconSource(kind, what, true);
                } else {
                    SetIslandPref(IslandPref::IconPack, index);
                }
            } else if (id == kChoiceIconFolder) {
                m_iconStatus.clear();
                if (index == 0 || g_iconFolder.empty()) {
                    StartIconFolderDialog(index == 2);
                } else if (index == 1) {
                    // (Only a folder on this computer: one from a backup may
                    // be on another, or name a file, which "explore" never
                    // runs.)
                    const DWORD attributes =
                        GetFileAttributes(g_iconFolder.c_str());
                    if (IsLocalDrivePath(g_iconFolder) &&
                        attributes != INVALID_FILE_ATTRIBUTES &&
                        (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        ShellExecute(nullptr, L"explore", g_iconFolder.c_str(),
                                     nullptr, nullptr, SW_SHOWNORMAL);
                    } else {
                        m_iconStatus = Tr(L"Only a folder on this computer",
                                          L"Só uma pasta deste computador");
                        Layout();
                    }
                } else {
                    SaveWindowsIconsInto(g_iconFolder);
                }
            } else if (id == kChoiceIconColors) {
                SetIslandPref(IslandPref::IconColors, index);
            } else if (id == kChoiceTrayStyle) {
                SetIslandPref(IslandPref::TrayStyle, index);
            } else if (id == kChoiceWallpaper) {
                SetIslandPref(IslandPref::WallpaperTransition, index);
            } else if (id == kChoiceNotchOpen) {
                SetIslandPref(IslandPref::NotchOpen, index);
            } else if (id == kChoiceHideWhen) {
                SetIslandPref(IslandPref::HideWhen, index);
            } else if (id == kChoiceLineClick) {
                SetIslandPref(IslandPref::LineClick, index);
            } else if (id == kChoiceLineFade) {
                SetIslandPref(IslandPref::LineFade, index);
            } else if (id == kChoiceBarSide) {
                SetIslandPref(IslandPref::BarSide, index);
            } else if (id == kChoiceBarInnerCorners) {
                SetIslandPref(IslandPref::BarInnerCorners, index);
            } else if (id == kChoiceBannerSize) {
                SetIslandPref(IslandPref::BannerSize, index);
            } else if (id == kChoiceShelfLimit) {
                SetIslandPref(IslandPref::ShelfLimit, index);
            } else if (id == kChoiceEyeTheme) {
                SetIslandPref(IslandPref::EyeTheme, index);
            } else if (id == kChoiceEyeShape) {
                SetIslandPref(IslandPref::EyeShape, index);
            } else if (id == kChoiceBannerWidth) {
                SetIslandPref(IslandPref::BannerWidth, index);
            } else if (id == kChoiceBannerCorners) {
                SetIslandPref(IslandPref::BannerCorners, index);
            } else if (id == kChoiceBannerPlace) {
                SetIslandPref(IslandPref::BannerPlace, index);
            } else if (id == kChoiceNotches) {
                SetIslandPref(g_settings.islandReserve ? IslandPref::Notches
                                                       : IslandPref::PillNotches,
                              index);
            } else if (id == kChoiceFolderDrag) {
                SetIslandPref(IslandPref::FolderDrag,
                              kFolderDragShown[index]);
            } else if (id == kChoiceLanguage) {
                SetTextLanguage(index);
            } else if (id == kChoiceNotesLanguage) {
                g_notesLanguage = index;
                OnSearchChanged();
            } else if (id == kChoiceFillStyle) {
                SetIslandPref(IslandPref::FillStyle, index);
            } else if (id == kChoiceLyricsSize) {
                SetIslandPref(IslandPref::LyricsSize, index);
            } else if (id == kChoiceLyricsWidth) {
                SetIslandPref(IslandPref::LyricsWidth, index);
            } else if (id == kChoiceLyricsColor) {
                // (Its own: picked first, chosen once picked.)
                if (index == 2) {
                    StartColorDialog(kLyricsColorDialog);
                } else {
                    SetIslandPref(IslandPref::LyricsColor, index);
                }
            } else if (id == kChoiceRecycleMinSize) {
                SetIslandPref(IslandPref::RecycleMinSize, index);
            } else if (id == kChoiceSetupStyle) {
                // (The profile in use's style, put back by the assistant's
                // Cancel: not a profile of its own, see StyleSegment.)
                SetIslandPref(IslandPref::Bar, StyleSegmentStyle(index));
            } else if (id == kChoiceSetupItems) {
                ChooseSetupItems(index);
            } else if (id == kChoiceSetupHide) {
                SetIslandPref(IslandPref::AutoHide, index > 0);
                if (index > 0) {
                    SetIslandPref(IslandPref::HideWhen, index - 1);
                }
            } else if (id == kChoiceSetupTheme) {
                SetIslandPref(IslandPref::Theme,
                              (int)(index == 0   ? ThemeId::Auto
                                    : index == 1 ? ThemeId::Light
                                                 : ThemeId::Black));
            } else if (id == kChoiceVisualizerColor) {
                // (Its own: picked first, chosen once picked.)
                if (index == 3) {
                    StartColorDialog(kVisualizerColorDialog);
                } else {
                    SetIslandPref(IslandPref::VisualizerColor, index);
                }
            } else if (const int kind = FillChoiceKind(id); kind >= 0) {
                // (Your own: picked first, chosen once picked.)
                if (index == 3) {
                    StartColorDialog(kFillColorDialog + kind);
                } else {
                    SetIslandPref(kind == kFillCopies ? IslandPref::FillCopyColor
                                  : kind == kFillCharge
                                      ? IslandPref::FillChargeColor
                                      : IslandPref::FillColor,
                                  index);
                }
            } else {
                AnimationChoice choice = g_animationChoice;
                (id == kChoiceAnimationLevel ? choice.level : choice.rate) =
                    index;
                SetAnimationChoice(choice, true);
            }
            StartIslandAnimation();
            return true;
        }
        case Part::SearchClear:
            m_search.clear();
            m_searchEdit.Reset();
            OnSearchChanged();
            return true;
        case Part::StyleSegment:
            if (const int style = StyleSegmentStyle(hit.index);
                GetIslandPref(IslandPref::Bar) != style) {
                // Its own profile first (#6166), then the style (the one in
                // use keeps its own).
                if (const int profile =
                        !g_island || g_island->HasMainSettings()
                            ? AutoProfileFor(L's', std::to_wstring(style))
                            : -1;
                    profile >= 0 && profile != g_activeProfile) {
                    ForgetAutoProfile();
                    ApplyProfile(profile);
                }
                if (GetIslandPref(IslandPref::Bar) != style) {
                    SetIslandPref(IslandPref::Bar, style);
                }
                StartIslandAnimation();
            }
            return true;
        case Part::PlaceDot:
            SetIslandPlace({}, (hit.index % 3) / 2.0, (hit.index / 3) / 2.0);
            return true;
        case Part::AppPlacesRow:
            ListOpenApps();
            m_appPlacesScroll.Reset();
            SetView(View::AppPlaces);
            return true;
        case Part::MonitorNotify:
            if (hit.index >= 0 && hit.index < (int)m_monitorParts.size()) {
                const MonitorPart& part = m_monitorParts[hit.index];
                SetNotificationScreen(part.device, !part.chosen);
            }
            Layout();
            StartIslandAnimation();
            return true;
        case Part::MonitorMode:
        case Part::MonitorMap:
        case Part::MonitorProfile: {
            if (hit.index < 0 || hit.index >= (int)m_monitorParts.size()) {
                return true;
            }
            const MonitorPart part = m_monitorParts[hit.index];
            if (part.disabled) {
                return true;
            }
            if (hit.part == Part::MonitorMode) {
                IslandScope scope(MainIsland());
                if (part.value == 2 && NotchCountWanted() > 0) {
                    return true;
                }
                SetIslandPref(IslandPref::FollowMouse, part.value == 2);
                SetIslandPref(IslandPref::AllMonitors, part.value == 1);
            } else if (hit.part == Part::MonitorMap) {
                const auto monitors = ListMonitors();
                for (size_t i = 0; i < monitors.size(); i++) {
                    if (monitors[i].device == part.device) {
                        ChooseMonitor((int)i);
                        break;
                    }
                }
            } else {
                for (Island* other : OtherIslands()) {
                    if (other->OwnDevice() != part.device) {
                        continue;
                    }
                    IslandScope scope(other);
                    other->SetMonitorProfile(part.value);
                    break;
                }
            }
            Layout();
            StartIslandAnimation();
            return true;
        }
        case Part::ProfileChip:
            // Another island than the main one: its monitor's profile (the
            // last chip: the main island's settings).
            if (g_island && !g_island->HasMainSettings()) {
                g_island->SetMonitorProfile(
                    hit.index >= 0 && hit.index < (int)g_profiles.size()
                        ? g_profiles[hit.index].id
                        : -1);
            } else if (hit.index >= 0 && hit.index < (int)g_profiles.size()) {
                // Applied (whatever was being edited is kept first).
                const int id = g_profiles[hit.index].id;
                EndProfileEdit(true);
                ForgetAutoProfile();
                ApplyProfile(id);
            }
            Layout();
            StartIslandAnimation();
            return true;
        case Part::ProfileAdd:
            EndProfileEdit(true);
            if (const int id = AddProfile(NewProfileName()); id >= 0) {
                StartProfileEdit(id, true);
            }
            Layout();
            StartIslandAnimation();
            return true;
        case Part::SettingsTab:
            // (Searching: the search goes, the tab shows.) Its options are
            // put in place at once and fade in together.
            if (hit.index != m_settingsTab || !m_search.empty()) {
                if (hit.index == kTabWhatsNew) {
                    g_notesLanguage = -1;
                }
                m_settingsTab = hit.index;
                m_search.clear();
                m_searchEdit.Reset();
                m_settingsScroll.Reset();
                OnSearchChanged();
                SettleSettings();
                m_tabFade = 0;
                m_tabFadeVelocity = 0;
            }
            return true;
        case Part::ProfileEditButton:
            if (hit.index >= 0 && hit.index < (int)g_profiles.size()) {
                const int id = g_profiles[hit.index].id;
                if (m_profileEdit == id) {
                    EndProfileEdit(true);
                } else {
                    EndProfileEdit(true);
                    StartProfileEdit(id, false);
                }
            }
            return true;
        case Part::ProfileName:
            m_profileTyping = true;
            return true;
        case Part::ProfileUpdate:
            // Saved, and done editing it.
            if (g_island && !g_island->HasMainSettings()) {
                SaveShownInProfile(m_profileEdit);
            } else {
                UpdateProfile(m_profileEdit);
            }
            m_profileSavedAt = NowSeconds();
            EndProfileEdit(true);
            return true;
        case Part::ProfileDelete:
            // A second click deletes it.
            if (!m_profileDeleteArmed) {
                m_profileDeleteArmed = true;
                return true;
            }
            DeleteProfile(m_profileEdit);
            EndProfileEdit(false);
            return true;
        case Part::ProfileDone:
            EndProfileEdit(true);
            return true;
        case Part::ProfileStyleChip:
            if (m_profileEdit >= 0 && hit.index >= 0 && hit.index < 3) {
                const std::wstring style =
                    std::to_wstring(StyleSegmentStyle(hit.index));
                SetAutoProfileRule(m_profileEdit, L's', style,
                                   !AutoProfileHas(m_profileEdit, L's', style));
            }
            return true;
        case Part::ProfileAppChip:
            if (m_profileEdit >= 0 && hit.index >= 0 &&
                hit.index < (int)m_profileApps.size()) {
                SetAutoProfileRule(m_profileEdit, L'a',
                                   m_profileApps[hit.index], false);
                Layout();
                StartIslandAnimation();
            }
            return true;
        case Part::ProfileAppAdd:
            if (m_profileEdit >= 0 && !m_profileAddApp.empty()) {
                SetAutoProfileRule(m_profileEdit, L'a', m_profileAddApp, true);
                Layout();
                StartIslandAnimation();
            }
            return true;
        case Part::ProfileKeepChip:
            if (m_profileEdit >= 0) {
                SetAutoProfileRule(m_profileEdit, L'k', L"1", hit.index == 1);
            }
            return true;
        case Part::AppPlaceDot: {
            const size_t entry = hit.index / 9;
            if (entry < g_appPlaces.size()) {
                SetIslandPlace(g_appPlaces[entry].appId,
                               (hit.index % 9 % 3) / 2.0,
                               (hit.index % 9 / 3) / 2.0);
            }
            return true;
        }
        case Part::AppItemChip: {
            const size_t entry = hit.index / kAppChipStride;
            if (entry < g_appPlaces.size()) {
                IslandPlace& place = g_appPlaces[entry];
                const int group = hit.index % kAppChipStride;
                if (group >= kStepChipFirst) {
                    // Its room from the edges or size: back to the one for
                    // all apps (its slider is dragged, see OnMouseDown).
                    const int step = group - kStepChipFirst;
                    if (step % 3 != 2) {
                        return true;
                    }
                    (step / 3 ? place.size : place.gap) = -1;
                } else if (group == kClockCenterChip) {
                    // Its clock in the middle or not, from the usual.
                    place.center = PlaceClockCenter(&place) ? 0 : 1;
                } else if (group >= kClockChipFirst) {
                    // The clock's parts: its own clock, from the usual one.
                    if (place.clock < 0) {
                        place.clock = GlobalClockParts();
                    }
                    place.clock ^= 1 << (group - kClockChipFirst);
                } else if (group >= kMediaChipFirst) {
                    // What's playing's buttons or cover and title: its own
                    // choice, from the one for all apps.
                    if (place.media < 0) {
                        place.media =
                            (g_mediaButtonsInPill ? kAppMediaButtons : 0) |
                            (g_mediaTitleInPill ? kAppMediaTitle : 0);
                    }
                    place.media ^= group == kMediaChipFirst ? kAppMediaButtons
                                                            : kAppMediaTitle;
                } else {
                    ToggleAppGroup(entry, group);
                    Layout();
                    return true;
                }
                SavePlaces();
                if (g_island) {
                    ForEachIsland(
                        [&](Island& island) { island.OnPlacesChanged(); });
                    ForEachIsland([&](Island& island) { island.Relayout(); });
                }
            }
            return true;
        }
        case Part::AppPlaceRemove:
            if (hit.index >= 0 && hit.index < (int)g_appPlaces.size()) {
                g_appPlaces.erase(g_appPlaces.begin() + hit.index);
                // The strips after it are the next apps' now.
                std::erase_if(m_orderChips, [](const OrderChip& chip) {
                    return chip.strip >= 0;
                });
                SavePlaces();
                if (g_island) {
                    ForEachIsland(
                        [&](Island& island) { island.OnPlacesChanged(); });
                }
                ListOpenApps();
                Layout();
            }
            return true;
        case Part::IconCell:
            if (hit.index >= 0 && hit.index < (int)std::size(kIcons)) {
                m_iconStatus.clear();
                StartIconPictureDialog(kIcons[hit.index].glyph);
            }
            return true;
        case Part::IconCellReset:
            if (hit.index >= 0 && hit.index < (int)std::size(kIcons)) {
                SetIconFile(kIcons[hit.index].glyph, L"");
                SetIconColor(kIcons[hit.index].glyph, -1);
                Layout();
            }
            return true;
        case Part::IconPackDelete:
            if (hit.index >= 0 && hit.index < kIconPacks) {
                if (g_settings.islandIconPack == hit.index) {
                    SetIslandPref(IslandPref::IconPack, kIconPackWindows);
                }
                DeleteIconPack(hit.index);
                Layout();
            }
            return true;
        case Part::IconCellColor:
            if (hit.index >= 0 && hit.index < (int)std::size(kIcons)) {
                StartColorDialog(kIconColorDialog + kIcons[hit.index].glyph);
            }
            return true;
        case Part::LyricsAppChip:
            if (hit.index >= 0 && hit.index < (int)LyricsApps().size()) {
                LyricsApps()[hit.index].off = !LyricsApps()[hit.index].off;
                SaveLyricsApps();
                if (Island* main = MainIsland()) {
                    main->LyricsAppsChanged();
                }
                Render();
            }
            return true;
        case Part::AppPlaceAdd:
            if (hit.index >= 0 && hit.index < (int)m_openApps.size() &&
                g_appPlaces.size() < kMaxAppPlaces) {
                // Where the pill is for all apps, to start with.
                IslandPlace place = g_islandPlace;
                place.own = true;
                place.hidden = GloballyHiddenGroups();
                place.appId = m_openApps[hit.index];
                g_appPlaces.push_back(std::move(place));
                SavePlaces();
                ListOpenApps();
                Layout();
            }
            return true;
        case Part::ThemeSwatch:
            SetIslandPref(IslandPref::Theme, hit.index);
            return true;
        case Part::HotspotToggle:
            m_hotspot = m_hotspot == 1 ? 0 : 1;
            SwitchRadio(2, m_hotspot == 1);
            return true;
        case Part::HotspotEdit:
            OpenHotspot();
            return true;
        case Part::HotspotSave:
            SaveHotspot();
            Layout();
            return true;
        case Part::AirplaneToggle:
            m_airplane = m_airplane == 1 ? 0 : 1;
            SwitchRadio(3, m_airplane == 1);
            return true;
        case Part::ProjectRow: {
            static const UINT32 kTopologies[] = {
                SDC_TOPOLOGY_INTERNAL, SDC_TOPOLOGY_CLONE,
                SDC_TOPOLOGY_EXTEND, SDC_TOPOLOGY_EXTERNAL};
            if (hit.index >= 0 && hit.index < 4) {
                const LONG result =
                    SetDisplayConfig(0, nullptr, 0, nullptr,
                                     SDC_APPLY | kTopologies[hit.index]);
                if (result != ERROR_SUCCESS) {
                    IslandLog(L"SetDisplayConfig failed: %ld", result);
                }
                UpdateDisplayMode();
            }
            return true;
        }
        case Part::AccessibilityRow:
            ToggleAccessibility(hit.index);
            return true;
        case Part::KomorebiButton:
            if (hit.index >= 0 && hit.index < 3 &&
                KomorebiEnabled(hit.index)) {
                RunKomorebi(hit.index);
            }
            return true;
        case Part::VolumeIcon:
            if (auto volume = g_island ? g_island->GetVolume(nullptr) : nullptr) {
                volume->SetMute(!m_muted, nullptr);
                m_muted = !m_muted;
                g_island->OnVolumeChanged();
            }
            return true;
        case Part::Device:
            SetView(View::Output);
            return true;
        case Part::MixerButton:
            SetView(View::Mixer);
            return true;
        case Part::Back:
            // A view opened alone from the pill: the whole control center.
            if (m_only == Only::View && m_view != View::Password) {
                SetOnly(Only::All);
                return true;
            }
            SetView(m_view == View::Password    ? View::Wifi
                    : m_view == View::AppPlaces ? View::Settings
                                                : View::Main);
            return true;
        case Part::Output:
            if (hit.index >= 0 && hit.index < (int)m_outputs.size()) {
                SetDefaultOutput(m_outputs[hit.index].id);
                Update();
                if (g_island) {
                    g_island->OnVolumeChanged();
                }
            }
            return true;
        case Part::AppIcon:
            if (hit.index >= 0 && hit.index < (int)m_apps.size()) {
                MixerApp& app = m_apps[hit.index];
                app.muted = !app.muted;
                for (auto& session : app.volumes) {
                    session->SetMute(app.muted, nullptr);
                }
            }
            return true;
        case Part::Battery:
            Close();
            OpenUri(L"ms-settings:batterysaver");
            break;
        case Part::PowerModeSegment:
            if (hit.index >= 0 && hit.index < 3 && hit.index != m_powerMode &&
                !g_powerChanging.exchange(true)) {
                // (Shown at once; read again once it's set.)
                if (auto* change = new PowerChange{m_hwnd, hit.index, {}};
                    StartWorker(PowerChangeThreadProc, change)) {
                    m_powerMode = hit.index;
                } else {
                    delete change;
                    g_powerChanging = false;
                }
            }
            return true;
        case Part::PowerPlan:
            SetView(View::PowerPlans);
            return true;
        case Part::PowerPlanRow:
            // Another plan (the power modes show only with the balanced one).
            if (hit.index >= 0 && hit.index < (int)m_powerPlans.size() &&
                hit.index != m_activePlan && !g_powerChanging.exchange(true)) {
                if (auto* change = new PowerChange{
                        m_hwnd, -1, m_powerPlans[hit.index].guid};
                    StartWorker(PowerChangeThreadProc, change)) {
                    m_activePlan = hit.index;
                } else {
                    delete change;
                    g_powerChanging = false;
                }
            }
            return true;
        case Part::More:
            Close();
            PressWindowsKey('A');
            break;
        default:
            break;
    }
    return false;
}

bool ControlPanel::OnWheel(int delta) {
    // The settings and the places per app scroll (when they're taller than
    // the screen allows).
    if (m_view == View::Settings || m_view == View::AppPlaces) {
        // (Over the side bar: it scrolls, when the window is short.)
        POINT cursor{};
        GetCursorPos(&cursor);
        ScreenToClient(m_hwnd, &cursor);
        cursor.x -= m_contentOffset.x;
        cursor.y -= m_contentOffset.y;
        const bool nav = m_view == View::Settings &&
                         PointInRect(cursor, m_navViewport);
        SmoothScroll& scroll = nav                         ? m_navScroll
                               : m_view == View::Settings ? m_settingsScroll
                                                          : m_appPlacesScroll;
        if (scroll.Wheel(delta, std::round(96 * m_scale))) {
            m_hover = {};
            return true;
        }
        return false;
    }
    // The volume only on the main view and the sound's own (not over the
    // Wi-Fi networks, Bluetooth...), and not while editing.
    if ((m_view != View::Main && m_view != View::Output &&
         m_view != View::Mixer) ||
        (m_view == View::Main && m_editing)) {
        return false;
    }
    if (!g_island) {
        return false;
    }
    const float step = 0.02f * delta / WHEEL_DELTA;
    // Over the display, the wheel changes the brightness (the monitor's
    // under it), 5% a notch.
    if (m_view == View::Main && m_hover.part == Part::BrightnessSlider &&
        m_hover.index >= 0 &&
        m_hover.index < (int)m_brightnessMonitors.size()) {
        if (const int steps = BrightnessWheelSteps(m_brightnessWheel, delta)) {
            SetBrightness(m_hover.index,
                          m_brightnessMonitors[m_hover.index].level + steps);
        }
        return true;
    }
    // In the mixer, the wheel over an app's row changes that app.
    if (m_view == View::Mixer && m_hover.part != Part::None &&
        m_hover.index >= 0 && m_hover.index < (int)m_apps.size()) {
        SetAppVolume(m_hover.index, m_apps[m_hover.index].volume + step);
        return true;
    }
    if (auto volume = g_island->GetVolume(nullptr)) {
        const float level = std::clamp(m_volume + step, 0.0f, 1.0f);
        volume->SetMasterVolumeLevelScalar(level, nullptr);
        if (level > 0) {
            volume->SetMute(FALSE, nullptr);
        }
        m_volume = level;
        m_muted = false;
        g_island->OnVolumeChanged();
    }
    return true;
}

bool ControlPanel::OnKey(WPARAM key) {
    // In a field: the arrows, Home, End and Delete (with Shift, by letter;
    // with Ctrl, by word) move and select in it.
    std::wstring* typed = nullptr;
    TextEdit* edit = nullptr;
    if (m_view == View::Settings && m_profileTyping) {
        typed = &m_profileName;
        edit = &m_profileNameEdit;
    } else if (m_view == View::Settings) {
        typed = &m_search;
        edit = &m_searchEdit;
    } else if (m_view == View::Password ||
               (m_view == View::Hotspot && !m_hotspotTypingName)) {
        typed = &m_password;
        edit = &m_passwordEdit;
    } else if (m_view == View::Hotspot) {
        typed = &m_hotspotName;
        edit = &m_hotspotNameEdit;
    }
    // (The hotspot's page: Tab goes to the other field.)
    if (m_view == View::Hotspot && key == VK_TAB) {
        m_hotspotTypingName = !m_hotspotTypingName;
        return true;
    }
    if (typed && (key == VK_LEFT || key == VK_RIGHT || key == VK_HOME ||
                  key == VK_END || key == VK_DELETE)) {
        const std::wstring before = *typed;
        if (EditTextKey(*typed, *edit, key)) {
            if (typed == &m_search && m_search != before) {
                OnSearchChanged();
            }
            return true;
        }
    }
    // A profile's name being typed: Enter keeps it, Escape leaves it as it
    // was.
    if (m_view == View::Settings && m_profileTyping) {
        if (key == VK_RETURN || key == VK_ESCAPE) {
            EndProfileEdit(key == VK_RETURN);
            return true;
        }
    }
    // Escape and Backspace go back, like in Windows (Backspace types in the
    // password).
    // Escape leaves editing.
    if (m_view == View::Main) {
        if (m_editing && key == VK_ESCAPE) {
            SetEditing(false);
            return true;
        }
        return false;
    }
    if (key == VK_BACK && (m_view == View::Password ||
                           m_view == View::Hotspot ||
                           m_view == View::Settings)) {
        return false;
    }
    if (key == VK_ESCAPE && m_view == View::Settings && !m_search.empty()) {
        m_search.clear();
        m_searchEdit.Reset();
        OnSearchChanged();
        return true;
    }
    // In the window: Up and Down go through the tabs (nothing searched),
    // Page Up and Page Down scroll; on the setup assistant, Enter is its
    // next step and Escape its Cancel.
    if (m_windowed && m_view == View::Settings && !m_profileTyping) {
        if ((key == VK_RETURN || key == VK_ESCAPE) &&
            m_settingsTab == kTabSetup && m_search.empty()) {
            PressSetupButton(key == VK_RETURN ? kSetupNext : kSetupCancel);
            return true;
        }
        if ((key == VK_UP || key == VK_DOWN) && m_search.empty() &&
            !m_tabIds.empty()) {
            const int count = (int)m_tabIds.size();
            const int now = (int)(std::find(m_tabIds.begin(), m_tabIds.end(),
                                            m_settingsTab) -
                                  m_tabIds.begin());
            m_settingsTab =
                m_tabIds[(now + (key == VK_DOWN ? 1 : count - 1)) % count];
            m_settingsScroll.Reset();
            OnSearchChanged();
            SettleSettings();
            m_tabFade = 0;
            m_tabFadeVelocity = 0;
            return true;
        }
        if (key == VK_PRIOR || key == VK_NEXT) {
            const double page =
                (m_settingsViewport.bottom - m_settingsViewport.top) * 0.9;
            m_settingsScroll.target =
                std::clamp(m_settingsScroll.target +
                               (key == VK_NEXT ? page : -page),
                           0.0, (double)m_settingsScroll.Max());
            StartIslandAnimation();
            return true;
        }
    }
    // The settings close (nothing behind them to go back to).
    if (m_view == View::Settings) {
        return false;
    }
    if (key == VK_ESCAPE || key == VK_BACK) {
        SetView(m_view == View::Password    ? View::Wifi
                : m_view == View::AppPlaces ? View::Settings
                                            : View::Main);
        return true;
    }
    return false;
}

// Typing the password: Enter connects, Ctrl+V pastes. In the settings,
// typing searches them.
bool ControlPanel::OnChar(WCHAR c) {
    // (Enter and Escape are keys: see OnKey.)
    if (c == L'\r' && m_view == View::Password) {
        SubmitPassword();
        Layout();
        return true;
    }
    if (c == L'\r' && m_view == View::Hotspot) {
        SaveHotspot();
        Layout();
        return true;
    }
    if (m_view == View::Hotspot && m_hotspotTypingName && c != L'\t' &&
        c != L'\n' && c != 0x1B) {
        if (!EditTextChar(m_hotspotName, m_hotspotNameEdit, c, 32, m_hwnd)) {
            return false;
        }
        KeepOneLine(m_hotspotName);
        m_hotspotStatus.clear();
        Layout();
        return true;
    }
    if (c == L'\r' || c == L'\n' || c == 0x1B) {
        return false;
    }
    if (m_view == View::Settings && m_profileTyping) {
        if (!EditTextChar(m_profileName, m_profileNameEdit, c, kMaxProfileName,
                          m_hwnd)) {
            return false;
        }
        KeepOneLine(m_profileName);
        return true;
    }
    if (m_view == View::Settings) {
        constexpr size_t kMaxSearch = 40;
        // (Backspace in an empty search isn't the search's.)
        if ((c == L'\b' || c == 0x7F) && m_search.empty()) {
            return false;
        }
        const std::wstring before = m_search;
        if (!EditTextChar(m_search, m_searchEdit, c, kMaxSearch, m_hwnd)) {
            return false;
        }
        KeepOneLine(m_search);
        if (m_search != before) {
            OnSearchChanged();
        }
        return true;
    }
    if (m_view != View::Password && m_view != View::Hotspot) {
        return false;
    }
    if (c == L'\t') {
        return false;
    }
    constexpr size_t kMaxPassword = 63;
    // A hidden password isn't copied.
    if ((c == 0x03 || c == 0x18) && !m_showPassword) {
        return true;
    }
    if (!EditTextChar(m_password, m_passwordEdit, c, kMaxPassword, m_hwnd)) {
        return false;
    }
    KeepOneLine(m_password);
    if (!m_connecting) {
        m_wifiStatus.clear();
    }
    if (m_view == View::Hotspot) {
        m_hotspotStatus.clear();
    }
    Layout();
    return true;
}

bool ControlPanel::OnAppMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_APP_SHORTCUT_RECORDED) {
        if (!m_recordingShortcut) {
            return false;
        }
        OnShortcutRecorded((int)lParam);
        return true;
    }
    // komorebi's command ended: failed (shown in a terminal) or done (its
    // state read again).
    if (msg == WM_APP_KOMOREBI_DONE) {
        if (lParam) {
            m_komorebiFailed = true;
            m_komorebiBusy = -1;
        } else if (wParam != 1) {
            KickKomorebiWatch();
        }
        Refresh();
        return true;
    }
    // (Only from their threads: the job is theirs.)
    if (msg == WM_APP_COLOR_DONE) {
        if (!m_colorThread) {
            return false;
        }
        m_dialogShown = false;
        WaitForSingleObject(m_colorThread, INFINITE);
        CloseHandle(m_colorThread);
        m_colorThread = nullptr;
        std::unique_ptr<ColorJob> job((ColorJob*)lParam);
        if (job->done && job->color >= kIconColorDialog) {
            SetIconColor((WCHAR)(job->color - kIconColorDialog), job->value);
        } else if (job->done && job->color == kLyricsColorDialog) {
            SetIslandPref(IslandPref::LyricsOwnColor, job->value & 0xFFFFFF);
            SetIslandPref(IslandPref::LyricsColor, 2);
        } else if (job->done && job->color == kVisualizerColorDialog) {
            SetIslandPref(IslandPref::VisualizerOwnColor,
                          job->value & 0xFFFFFF);
            SetIslandPref(IslandPref::VisualizerColor, 3);
        } else if (job->done && job->color >= kFillColorDialog) {
            const int kind = job->color - kFillColorDialog;
            SetIslandPref(kind == kFillCopies   ? IslandPref::FillCopyOwnColor
                          : kind == kFillCharge ? IslandPref::FillChargeOwnColor
                                                : IslandPref::FillOwnColor,
                          job->value & 0xFFFFFF);
            SetIslandPref(kind == kFillCopies   ? IslandPref::FillCopyColor
                          : kind == kFillCharge ? IslandPref::FillChargeColor
                                                : IslandPref::FillColor,
                          3);
        } else if (job->done) {
            SetCustomColor(job->color, job->value);
        }
        SetForegroundWindow(m_hwnd);
        Layout();
        return true;
    }
    if (msg == WM_APP_FOLDER_PICKED) {
        if (!m_iconThread) {
            return false;
        }
        m_dialogShown = false;
        WaitForSingleObject(m_iconThread, INFINITE);
        CloseHandle(m_iconThread);
        m_iconThread = nullptr;
        std::unique_ptr<FolderPickJob> job((FolderPickJob*)lParam);
        if (!job->picture && job->purpose == kIconFolderSaved) {
            m_iconStatus =
                job->saved < 0 ? Tr(L"Couldn't save in that folder",
                                    L"Não deu para salvar nessa pasta")
                : job->saved == 0
                    ? Tr(L"\u2713 All of them are there",
                         L"\u2713 Todos já estão lá")
                    : L"\u2713 " + std::to_wstring(job->saved) + L" " +
                          Tr(L"icons saved", L"ícones salvos");
            if (job->saved >= 0) {
                ShellExecute(nullptr, L"open", job->path.c_str(), nullptr,
                             nullptr, SW_SHOWNORMAL);
            }
            Layout();
            return true;
        }
        if (job->path.empty()) {
            // (Canceled.)
        } else if (!IsLocalDrivePath(job->path)) {
            m_iconStatus = job->picture
                               ? Tr(L"Only a picture on this computer",
                                    L"Só uma imagem deste computador")
                               : Tr(L"Only a folder on this computer",
                                    L"Só uma pasta deste computador");
        } else if (job->picture) {
            SetIconFile((WCHAR)job->purpose, job->path);
        } else {
            SetIconFolder(job->path);
            if (g_settings.islandIconPack != kIconPackFolder) {
                SetIslandPref(IslandPref::IconPack, kIconPackFolder);
            }
            if (job->purpose == kIconFolderPickSave) {
                SaveWindowsIconsInto(g_iconFolder);
            }
        }
        SetForegroundWindow(m_hwnd);
        Layout();
        return true;
    }
    if (msg == WM_APP_BACKUP_DONE) {
        if (!m_backupThread) {
            return false;
        }
        m_dialogShown = false;
        WaitForSingleObject(m_backupThread, INFINITE);
        CloseHandle(m_backupThread);
        m_backupThread = nullptr;
        std::unique_ptr<BackupJob> job((BackupJob*)lParam);
        if (job->logs) {
            m_logsStatus =
                job->done ? Tr(L"\u2713 Saved", L"\u2713 Salvo") : L"";
        } else if (job->save) {
            m_backupStatus =
                job->done ? Tr(L"\u2713 Exported", L"\u2713 Exportado") : L"";
        } else if (job->done) {
            // Put back (as the main island's settings), the island starts
            // again with all of it.
            IslandScope scope(MainIsland());
            if (RestoreBackup(job->text)) {
                return true;
            }
            m_backupStatus = Tr(L"Not a backup of the island",
                                L"Não é um backup da ilha");
        }
        // Back in front (the dialog had it).
        SetForegroundWindow(m_hwnd);
        Layout();
        return true;
    }
    if (msg == WM_APP_SCREENSHOT) {
        PressKeys({VK_LWIN, VK_SHIFT, 'S'});
        return false;
    }
    // A radio request's answers (the state after it), except for what was
    // clicked again meanwhile (still shown as asked).
    if (msg == WM_APP_RADIOS) {
        if (m_radioWanted[0] < 0) {
            m_wifiRadio = (int)(INT_PTR)wParam;
        }
        if (m_radioWanted[1] < 0) {
            m_bluetoothRadio = (int)(INT_PTR)lParam;
        }
        // (Asked for the pill's buttons while closed too: they light up.)
        StartIslandAnimation();
        if (!IsOpen()) {
            return false;
        }
        Update();
        Layout();
        return true;
    }
    if (msg == WM_APP_HOTSPOT_CONFIG) {
        // The hotspot's name and password read (or changed): in its page's
        // fields, unless something was typed meanwhile.
        std::unique_ptr<HotspotAnswer> answer((HotspotAnswer*)lParam);
        m_hotspotBusy = false;
        if (!answer || m_view != View::Hotspot) {
            return true;
        }
        if (!answer->worked) {
            m_hotspotStatus =
                answer->saved || !m_hotspotName.empty()
                    ? Tr(L"Couldn't change the hotspot.",
                         L"Não foi possível mudar o hotspot.")
                    : Tr(L"The hotspot isn't available now.",
                         L"O hotspot não está disponível agora.");
        } else {
            if (m_hotspotName.empty() && m_password.empty()) {
                m_hotspotName = answer->name;
                m_password = answer->password;
                m_hotspotNameEdit.Reset();
                m_passwordEdit.Reset();
            }
            m_hotspotStatus =
                answer->saved ? Tr(L"Saved.", L"Salvo.") : std::wstring();
        }
        Layout();
        return true;
    }
    if (msg == WM_APP_HOTSPOT) {
        if (m_radioWanted[2] < 0) {
            m_hotspot = (int)(INT_PTR)wParam;
        }
        if (m_radioWanted[3] < 0) {
            m_airplane = (int)(INT_PTR)lParam;
        }
        // The request's last answer: the next one wanted goes.
        m_radioBusy = false;
        for (int kind = 0; kind < 4; kind++) {
            if (m_radioWanted[kind] >= 0) {
                const bool on = m_radioWanted[kind] == 1;
                m_radioWanted[kind] = -1;
                SwitchRadio(kind, on);
                break;
            }
        }
        StartIslandAnimation();
        Layout();
        return true;
    }
    if (msg == WM_APP_BLUETOOTH_DONE) {
        // Its driver didn't take it (no audio endpoint): not connecting.
        if (!wParam && (ULONGLONG)lParam == m_bluetoothAddress) {
            if (m_bluetoothStep == BluetoothStep::Connecting) {
                m_bluetoothStep = BluetoothStep::Failed;
                m_bluetoothStepAt = GetTickCount64();
            } else if (m_bluetoothStep == BluetoothStep::Disconnecting) {
                m_bluetoothStep = BluetoothStep::None;
            }
            Layout();
        }
        return true;
    }
    if (msg == WM_APP_POWER_SET) {
        // (What Windows has now, also when it refused.)
        m_powerMode = GetPowerMode();
        m_powerPlans = GetPowerPlans(&m_activePlan);
        m_powerReadAt = NowSeconds();
        Layout();
        return true;
    }
    if (msg == WM_APP_BRIGHTNESS_LIST) {
        std::unique_ptr<std::vector<BrightnessMonitor>> list(
            (std::vector<BrightnessMonitor>*)lParam);
        // Not while it's being changed here.
        if (!m_brightnessJob->running) {
            m_brightnessMonitors = std::move(*list);
            m_brightness = m_brightnessMonitors.empty()
                               ? -1
                               : m_brightnessMonitors.front().level;
        }
        Layout();
        return true;
    }
    if (msg == WM_APP_WIFI_LIST) {
        const bool refused = lParam == kWifiListRefused;
        std::unique_ptr<std::vector<WifiNetwork>> found(
            refused ? nullptr : (std::vector<WifiNetwork>*)lParam);
        if ((UINT)wParam != m_wifiListId || !m_wifiListThread) {
            return false;
        }
        m_wifiRefused = refused;
        WaitForSingleObject(m_wifiListThread, INFINITE);
        CloseHandle(m_wifiListThread);
        m_wifiListThread = nullptr;
        if (found && m_wlan && m_hasWifi && m_wifiRadio != 0) {
            m_networks = std::move(*found);
        }
        if (m_wifiListAgain) {
            m_wifiListAgain = false;
            const bool scan = m_wifiScanAgain;
            m_wifiScanAgain = false;
            UpdateWifi(scan);
        }
        Layout();
        return true;
    }
    if (msg == WM_APP_WLAN_CONNECTION) {
        const std::unique_ptr<WlanConnectionEvent> event(
            (WlanConnectionEvent*)lParam);
        OnWifiJoinEnded(*event);
    } else if (msg != WM_APP_WLAN ||
               (wParam != wlan_notification_acm_scan_complete &&
                wParam != wlan_notification_acm_disconnected)) {
        return false;
    }
    if (m_view == View::Wifi) {
        UpdateWifi();
    }
    if (g_island) {
        g_island->OnNetworkChanged();
    }
    Layout();
    return true;
}

// A join ended: a profile made for it stays only if it worked; the panel
// shows how it went only if it's the join it's waiting for (not an earlier
// one, left by a click on another network).
void ControlPanel::OnWifiJoinEnded(const WlanConnectionEvent& event) {
    const bool worked =
        event.code == wlan_notification_acm_connection_complete &&
        event.reason == WLAN_REASON_CODE_SUCCESS;
    if (auto it = std::find(m_createdProfiles.begin(), m_createdProfiles.end(),
                            event.profile);
        it != m_createdProfiles.end()) {
        if (!worked && m_wlan) {
            WlanDeleteProfile(m_wlan, &m_wifiInterface, event.profile.c_str(),
                              nullptr);
        }
        m_createdProfiles.erase(it);
    }
    if (!m_connecting || event.profile != m_connectingProfile) {
        return;
    }
    m_connecting = false;
    m_connectingProfile.clear();
    if (worked) {
        m_wifiStatus.clear();
        if (m_view == View::Password) {
            m_password.clear();
            m_passwordEdit.Reset();
            SetView(View::Wifi);
        }
        return;
    }
    IslandLog(L"Joining Wi-Fi failed, reason %u", (unsigned)event.reason);
    m_wifiStatus =
        m_view == View::Password
            ? Tr(L"Couldn't connect. Check the password.",
                 L"Não foi possível conectar. Confira a senha.")
            : Tr(L"Couldn't connect.", L"Não foi possível conectar.");
    // Why, as Windows says it (in its language).
    WCHAR reason[512];
    if (event.reason &&
        WlanReasonCodeToString(event.reason, ARRAYSIZE(reason), reason,
                               nullptr) == ERROR_SUCCESS &&
        *reason) {
        m_wifiStatus += L"\n";
        m_wifiStatus += reason;
    }
}

////////////////////////////////////////////////////////////////////////////////
// The notifications.

// The texts of a toast's XML: <text> elements, in order, with entities
// turned back into characters.
// XML text (or an attribute's value) as a wide string, with its entities
// ("&amp;", "&#233;"...) turned back into characters.
std::wstring DecodeXml(std::string_view raw) {
    std::string text;
    for (size_t i = 0; i < raw.size(); i++) {
        if (raw[i] != '&') {
            text += raw[i];
            continue;
        }
        const size_t semicolon = raw.find(';', i);
        if (semicolon == std::string_view::npos || semicolon - i > 10) {
            text += raw[i];
            continue;
        }
        const std::string_view entity = raw.substr(i + 1, semicolon - i - 1);
        unsigned code = 0;
        if (entity == "amp") {
            code = '&';
        } else if (entity == "lt") {
            code = '<';
        } else if (entity == "gt") {
            code = '>';
        } else if (entity == "quot") {
            code = '"';
        } else if (entity == "apos") {
            code = '\'';
        } else if (entity.size() > 1 && entity[0] == '#') {
            code = entity[1] == 'x'
                       ? strtoul(std::string(entity.substr(2)).c_str(),
                                 nullptr, 16)
                       : strtoul(std::string(entity.substr(1)).c_str(),
                                 nullptr, 10);
        }
        if (!code) {
            text += raw[i];
            continue;
        }
        // As UTF-8.
        if (code < 0x80) {
            text += (char)code;
        } else if (code < 0x800) {
            text += (char)(0xC0 | (code >> 6));
            text += (char)(0x80 | (code & 0x3F));
        } else if (code < 0x10000) {
            text += (char)(0xE0 | (code >> 12));
            text += (char)(0x80 | ((code >> 6) & 0x3F));
            text += (char)(0x80 | (code & 0x3F));
        } else {
            text += (char)(0xF0 | (code >> 18));
            text += (char)(0x80 | ((code >> 12) & 0x3F));
            text += (char)(0x80 | ((code >> 6) & 0x3F));
            text += (char)(0x80 | (code & 0x3F));
        }
        i = semicolon;
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(),
                                           (int)text.size(), nullptr, 0);
    std::wstring wide(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(),
                        wide.data(), length);
    return wide;
}

// An attribute's value in a tag ("<action content="Reply" ...").
std::wstring XmlAttribute(std::string_view tag, std::string_view name) {
    for (size_t at = tag.find(name); at != std::string_view::npos;
         at = tag.find(name, at + 1)) {
        const size_t equals = at + name.size();
        if ((at > 0 && !isspace((unsigned char)tag[at - 1])) ||
            equals + 1 >= tag.size() || tag[equals] != '=') {
            continue;
        }
        const char quote = tag[equals + 1];
        if (quote != '"' && quote != '\'') {
            continue;
        }
        const size_t end = tag.find(quote, equals + 2);
        if (end == std::string_view::npos) {
            return {};
        }
        return DecodeXml(tag.substr(equals + 2, end - equals - 2));
    }
    return {};
}

