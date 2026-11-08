// One control of the main view, at its place (sections at theirs).
ID2D1Bitmap* ControlPanel::GetControlTile(ID2D1RenderTarget* target,
                                          ID2D1SolidColorBrush* brush,
                                          ControlId id,
                                          const D2D1_RECT_F& rect,
                                          float margin) {
    const D2D1_SIZE_F size{
        std::ceil(rect.right - rect.left + margin * 2),
        std::ceil(rect.bottom - rect.top + margin * 2)};
    ControlTile* tile = nullptr;
    for (auto& existing : m_tiles) {
        if (existing.id == id) {
            tile = &existing;
        }
    }
    if (tile && tile->size.width == size.width &&
        tile->size.height == size.height && tile->icons == IconsGeneration()) {
        return tile->bitmap.get();
    }
    winrt::com_ptr<ID2D1BitmapRenderTarget> tileTarget;
    if (FAILED(target->CreateCompatibleRenderTarget(size, tileTarget.put()))) {
        return nullptr;
    }
    tileTarget->SetTextAntialiasMode(target->GetTextAntialiasMode());
    tileTarget->BeginDraw();
    tileTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
    tileTarget->SetTransform(D2D1::Matrix3x2F::Translation(
        margin - rect.left, margin - rect.top));
    DrawControl(tileTarget.get(), brush, id, rect);
    winrt::com_ptr<ID2D1Bitmap> bitmap;
    if (FAILED(tileTarget->EndDraw()) ||
        FAILED(tileTarget->GetBitmap(bitmap.put()))) {
        return nullptr;
    }
    if (!tile) {
        tile = &m_tiles.emplace_back();
        tile->id = id;
    }
    tile->size = size;
    tile->icons = IconsGeneration();
    tile->bitmap = std::move(bitmap);
    return tile->bitmap.get();
}

void ControlPanel::DrawControl(ID2D1RenderTarget* target,
                               ID2D1SolidColorBrush* brush,
                               ControlId id,
                               const D2D1_RECT_F& rect) {
    const float s = m_scale;
    const ControlInfo& info = GetControlInfo(id);
    const PCWSTR name = Tr(info.english, info.portuguese);
    // While editing, a section that isn't available here (no brightness
    // control, no battery) still shows its place.
    const bool missing =
        (id == ControlId::Display && m_display.right <= m_display.left) ||
        (id == ControlId::Sound && m_sound.right <= m_sound.left) ||
        (id == ControlId::Battery &&
         m_batteryTile.right <= m_batteryTile.left) ||
        (id == ControlId::PowerMode &&
         m_powerModeRect.right <= m_powerModeRect.left);
    if (missing) {
        const float radius = ControlRadius(rect, s);
        brush->SetColor(Fg(0.08f));
        GlassFill(target, {rect, radius, radius}, brush);
        const float icon = std::round(40 * s);
        brush->SetColor(Fg(0.6f));
        DrawGlyph(target, &info.glyph, 1, m_iconFormat.get(),
                  {rect.left, rect.top, rect.left + icon, rect.bottom},
                  brush);
        // Said only when it really isn't (a removed one shrinking away has
        // no place either).
        const bool unavailable =
            (id == ControlId::Display && m_brightness < 0) ||
            (id == ControlId::Battery && !m_hasBattery) ||
            (id == ControlId::PowerMode && m_powerPlans.empty());
        const std::wstring text =
            unavailable ? std::wstring(name) + L"  ·  " +
                              Tr(L"Not available here", L"Indisponível aqui")
                        : std::wstring(name);
        brush->SetColor(Fg(0.6f));
        target->DrawText(text.c_str(), (UINT32)text.size(),
                         m_subtitleFormat.get(),
                         {rect.left + icon, rect.top, rect.right, rect.bottom},
                         brush);
        return;
    }
    switch (id) {
        case ControlId::Network: {
            // Wi-Fi with its name, or the cable.
            WCHAR networkGlyph = kGlyphEthernet;
            std::wstring networkName = Tr(L"Ethernet", L"Cabo");
            if (m_network.wifi) {
                networkGlyph =
                    m_network.signal >= 0
                        ? kGlyphWifi[std::clamp(m_network.signal / 25, 0, 3)]
                        : kGlyphWifi[3];
                networkName = m_network.wifiName.empty()
                                  ? L"Wi-Fi"
                                  : m_network.wifiName;
            } else if (!m_network.connected) {
                networkGlyph = kGlyphNoInternet;
                networkName = Tr(L"Network", L"Rede");
            }
            std::wstring networkState =
                g_island && g_island->IsNetworkConnecting()
                    ? Tr(L"Connecting...", L"Conectando...")
                : m_network.internet  ? Tr(L"Connected", L"Conectado")
                : m_network.connected ? Tr(L"No internet", L"Sem internet")
                                      : Tr(L"Disconnected", L"Desconectado");
            if (m_wifiRadio == 0 && !m_network.wifi && !m_network.connected) {
                networkGlyph = kGlyphWifi[3];
                networkName = L"Wi-Fi";
                networkState = Tr(L"Off", L"Desligado");
            }
            DrawTile(target, brush, rect, networkGlyph,
                     m_network.internet || (m_wifiRadio == 1 && m_network.wifi),
                     networkName.c_str(), networkState,
                     m_hover.part == Part::Network ||
                         m_hover.part == Part::WifiToggle);
            break;
        }
        case ControlId::Bluetooth:
            DrawTile(target, brush, rect, kGlyphBluetooth,
                     m_bluetoothRadio == 1, L"Bluetooth",
                     m_bluetoothRadio == 1   ? Tr(L"On", L"Ligado")
                     : m_bluetoothRadio == 0 ? Tr(L"Off", L"Desligado")
                     : m_bluetoothRadio == kRadioMissing
                         ? Tr(L"Unavailable", L"Indisponível")
                         : Tr(L"Devices", L"Dispositivos"),
                     m_hover.part == Part::Bluetooth ||
                         m_hover.part == Part::BluetoothToggle);
            break;
        case ControlId::DoNotDisturb:
            DrawTile(target, brush, rect, kGlyphMoon, m_doNotDisturb, name,
                     m_doNotDisturb ? Tr(L"On", L"Ligado")
                                    : Tr(L"Off", L"Desligado"),
                     m_hover.part == Part::DoNotDisturb ||
                         m_hover.part == Part::DndToggle);
            break;
        case ControlId::NightLight:
            DrawTile(target, brush, rect, kGlyphNightLight,
                     m_nightLight == 1, name,
                     m_nightLight == 1   ? Tr(L"On", L"Ligada")
                     : m_nightLight == 0 ? Tr(L"Off", L"Desligada")
                                         : Tr(L"Unavailable",
                                              L"Indisponível"),
                     m_hover.part == Part::NightLight && m_nightLight >= 0);
            break;
        case ControlId::Hotspot:
            DrawTile(target, brush, rect, kGlyphHotspot, m_hotspot == 1,
                     name,
                     m_hotspot == 1   ? Tr(L"On", L"Ligado")
                     : m_hotspot == 0 ? Tr(L"Off", L"Desligado")
                                      : Tr(L"Unavailable", L"Indisponível"),
                     m_hover.part == Part::HotspotToggle ||
                         m_hover.part == Part::HotspotEdit);
            break;
        case ControlId::Airplane:
            DrawTile(target, brush, rect, kGlyphAirplane, m_airplane == 1,
                     name,
                     m_airplane == 1   ? Tr(L"On", L"Ligado")
                     : m_airplane == 0 ? Tr(L"Off", L"Desligado")
                                       : Tr(L"Unavailable", L"Indisponível"),
                     m_hover.part == Part::AirplaneToggle);
            break;
        case ControlId::Display:
        case ControlId::Sound:
        case ControlId::Battery:
        case ControlId::PowerMode:
        case ControlId::Komorebi:
            // Below.
            break;
        case ControlId::Microphone:
            DrawShortcut(target, brush, rect,
                         m_microphoneMuted == 1 ? (WCHAR)0xF781 : info.glyph,
                         name, m_microphoneMuted == 0,
                         m_hover.part == Part::Shortcut &&
                             m_hover.index == (int)id);
            break;
        default:
            DrawShortcut(target, brush, rect, info.glyph, name,
                         id == ControlId::DarkMode && m_darkMode,
                         m_hover.part == Part::Shortcut &&
                             m_hover.index == (int)id);
            break;
    }
    // The display: its brightness, like the sound.
    if (id == ControlId::Display && m_display.right > m_display.left) {
        const float sectionRadius = ControlRadius(m_display, s);
        brush->SetColor(Fg(0.08f));
        GlassFill(target, {m_display, sectionRadius, sectionRadius}, brush);
        const float sectionInner = std::round(12 * s);
        const D2D1_RECT_F displayTitle{
            m_display.left + sectionInner, m_display.top + 6 * s,
            m_display.right - sectionInner, m_display.top + 30 * s};
        PCWSTR title = Tr(L"Display", L"Tela");
        brush->SetColor(Fg(0.95f));
        target->DrawText(title, (UINT32)wcslen(title), m_sectionFormat.get(),
                         displayTitle, brush);
        const bool rows = m_brightnessSliders.size() > 1;
        for (size_t row = 0; row < m_brightnessSliders.size(); row++) {
            const D2D1_RECT_F& slider = m_brightnessSliders[row];
            const int value = row < m_brightnessMonitors.size()
                                  ? m_brightnessMonitors[row].level
                                  : m_brightness;
            // The level on the title; with more monitors, on each one's line
            // with its name.
            const D2D1_RECT_F line =
                rows ? D2D1_RECT_F{slider.left, slider.top - 20 * s,
                                   slider.right, slider.top - 2 * s}
                     : displayTitle;
            if (rows && row < m_brightnessMonitors.size()) {
                const std::wstring& name = m_brightnessMonitors[row].name;
                brush->SetColor(Fg(0.75f));
                target->DrawText(name.c_str(), (UINT32)name.size(),
                                 m_subtitleFormat.get(),
                                 {line.left, line.top, line.right - 44 * s,
                                  line.bottom},
                                 brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
            const std::wstring level = std::to_wstring(value) + L"%";
            brush->SetColor(Fg(0.55f));
            target->DrawText(level.c_str(), (UINT32)level.size(),
                             m_percentFormat.get(), line, brush);
            DrawSlider(target, brush, slider, value / 100.0f, kGlyphBrightness,
                       false);
        }
    }

    // Sound: title, percentage, the slider, the output device and the mixer.
    if (id == ControlId::Sound && m_sound.right > m_sound.left) {
        const float radius = ControlRadius(m_sound, s);
        brush->SetColor(Fg(0.08f));
        GlassFill(target, {m_sound, radius, radius}, brush);
        const float inner = std::round(12 * s);
        brush->SetColor(Fg(0.95f));
        const D2D1_RECT_F titleRect{m_sound.left + inner, m_sound.top + 6 * s,
                                    m_sound.right - inner, m_slider.top - 4 * s};
        PCWSTR soundTitle = Tr(L"Sound", L"Som");
        target->DrawText(soundTitle, (UINT32)wcslen(soundTitle),
                         m_sectionFormat.get(), titleRect, brush);
        const std::wstring percent =
            m_hasVolume ? (m_muted ? std::wstring(Tr(L"Muted", L"Mudo"))
                                   : std::to_wstring((int)std::lround(
                                         m_volume * 100)) +
                                         L"%")
                        : L"";
        brush->SetColor(Fg(0.55f));
        target->DrawText(percent.c_str(), (UINT32)percent.size(),
                         m_percentFormat.get(), titleRect, brush);

        DrawSlider(target, brush, m_slider,
                   m_hasVolume && !m_muted ? m_volume : 0,
                   m_hasVolume ? VolumeGlyphFor(m_volume, m_muted) : kGlyphMute,
                   m_hover.part == Part::VolumeIcon);

        // The output device, opening the list of devices: its name (cut
        // short when long) and the arrow right after it, always seen.
        const std::wstring device = m_device.empty()
                                        ? std::wstring(Tr(L"Output", L"Saída"))
                                        : m_device;
        const float arrowWidth = std::round(16 * s);
        float nameWidth = m_deviceRow.right - m_deviceRow.left - arrowWidth;
        winrt::com_ptr<IDWriteTextLayout> deviceLayout;
        if (SUCCEEDED(m_dwrite->CreateTextLayout(
                device.c_str(), (UINT32)device.size(), m_subtitleFormat.get(),
                10000, 100, deviceLayout.put()))) {
            DWRITE_TEXT_METRICS metrics{};
            if (SUCCEEDED(deviceLayout->GetMetrics(&metrics))) {
                nameWidth = std::min(nameWidth, std::ceil(metrics.width));
            }
        }
        brush->SetColor(
            Fg(0.55f + 0.35f * Hover(m_deviceRow, m_hover.part == Part::Device)));
        const D2D1_RECT_F nameRect{m_deviceRow.left, m_deviceRow.top,
                                   m_deviceRow.left + nameWidth,
                                   m_deviceRow.bottom};
        target->DrawText(device.c_str(), (UINT32)device.size(),
                         m_subtitleFormat.get(), nameRect, brush);
        const D2D1_RECT_F arrowRect{nameRect.right + std::round(5 * s),
                                    m_deviceRow.top,
                                    nameRect.right + arrowWidth,
                                    m_deviceRow.bottom};
        target->DrawText(L"›", 1, m_subtitleFormat.get(), arrowRect, brush);

        // The mixer button.
        const float buttonRadius = (m_mixerButton.bottom - m_mixerButton.top) / 2;
        brush->SetColor(Fg(
            0.1f + 0.1f * Hover(m_mixerButton, m_hover.part == Part::MixerButton)));
        GlassFill(target, {m_mixerButton, buttonRadius, buttonRadius}, brush);
        PCWSTR mixer = L"Mixer";
        brush->SetColor(Fg(0.9f));
        target->DrawText(mixer, (UINT32)wcslen(mixer), m_linkFormat.get(),
                         m_mixerButton, brush);
    }

    // Power: its title, the power plans (the active one checked), and the
    // power modes (the chosen one blue).
    if (id == ControlId::PowerMode &&
        m_powerModeRect.right > m_powerModeRect.left) {
        const float sectionRadius = ControlRadius(m_powerModeRect, s);
        brush->SetColor(Fg(0.08f));
        GlassFill(target, {m_powerModeRect, sectionRadius, sectionRadius},
                  brush);
        const float sectionInner = std::round(12 * s);
        const D2D1_RECT_F titleRect{
            m_powerModeRect.left + sectionInner, m_powerModeRect.top,
            m_powerModeRect.right - sectionInner,
            m_powerModeRect.top + std::round(34 * s)};
        PCWSTR title = Tr(L"Power", L"Energia");
        brush->SetColor(Fg(0.95f));
        target->DrawText(title, (UINT32)wcslen(title), m_sectionFormat.get(),
                         titleRect, brush);
        // The active plan, with "›" (its list).
        if (m_powerPlanRow.right > m_powerPlanRow.left) {
            const D2D1_RECT_F& row = m_powerPlanRow;
            const float radius = std::round(8 * s);
            brush->SetColor(
                Fg(0.06f + 0.08f * Hover(row, m_hover.part == Part::PowerPlan)));
            GlassFill(target, {row, radius, radius}, brush);
            const float arrow = std::round(28 * s);
            // (The Windows power plan, not the mode under it.)
            const std::wstring name =
                m_activePlan >= 0 && m_activePlan < (int)m_powerPlans.size()
                    ? Tr(L"Plan: ", L"Plano: ") +
                          m_powerPlans[m_activePlan].name
                    : std::wstring(Tr(L"Choose a power plan",
                                      L"Escolha um plano de energia"));
            brush->SetColor(Fg(0.92f));
            target->DrawText(name.c_str(), (UINT32)name.size(),
                             m_subtitleFormat.get(),
                             {row.left + std::round(10 * s), row.top,
                              row.right - arrow, row.bottom},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            brush->SetColor(Fg(0.6f));
            DrawGlyph(target, &kGlyphNext, 1, m_smallIconFormat.get(),
                      {row.right - arrow, row.top, row.right,
                       row.bottom},
                      brush);
        }
        static const PCWSTR kModeNames[3][2] = {
            {L"Efficiency", L"Eficiência"},
            {L"Balanced", L"Equilibrado"},
            {L"Performance", L"Desempenho"}};
        for (int i = 0; i < 3 && m_powerMode >= 0; i++) {
            const D2D1_RECT_F& choice = m_powerModeChoices[i];
            const float radius = (choice.bottom - choice.top) / 2;
            const bool chosen = m_powerMode == i;
            const float lit =
                Hover(choice, m_hover.part == Part::PowerModeSegment &&
                                  m_hover.index == i);
            brush->SetColor(chosen ? Accent(1.0f - 0.15f * lit)
                                   : Fg(0.1f + 0.08f * lit));
            GlassFill(target, {choice, radius, radius}, brush);
            PCWSTR name = Tr(kModeNames[i][0], kModeNames[i][1]);
            brush->SetColor(chosen ? AccentInk(1) : Fg(0.9f));
            target->DrawText(name, (UINT32)wcslen(name), m_linkFormat.get(),
                             choice, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }

    // komorebi: its three buttons (dimmed when they can't be used), and its
    // version or why they can't.
    if (id == ControlId::Komorebi) {
        const float sectionRadius = ControlRadius(rect, s);
        brush->SetColor(Fg(0.08f));
        GlassFill(target, {rect, sectionRadius, sectionRadius}, brush);
        static const WCHAR kGlyphs[3] = {kGlyphPlay, kGlyphStop, kGlyphReload};
        const float radius = std::round(10 * s);
        for (int i = 0; i < 3; i++) {
            const D2D1_RECT_F& button = m_komorebiButtons[i];
            const bool enabled = KomorebiEnabled(i);
            const float lit =
                enabled ? Hover(button, m_hover.part == Part::KomorebiButton &&
                                            m_hover.index == i)
                        : 0;
            brush->SetColor(Fg(0.06f + 0.1f * lit));
            GlassFill(target, {button, radius, radius}, brush);
            brush->SetColor(Fg(enabled ? 0.95f : 0.3f));
            DrawGlyph(target, &kGlyphs[i], 1, m_komorebiIconFormat.get(),
                      button, brush);
        }
        std::wstring text;
        if (m_komorebic.empty()) {
            text = Tr(L"komorebi isn't installed",
                      L"O komorebi não está instalado");
        } else {
            text = L"komorebi";
            if (!m_komorebiVersion.empty()) {
                text += L" v" + m_komorebiVersion;
            }
            if (m_komorebiBusy >= 0) {
                text +=
                    std::wstring(L"  ·  ") + Tr(L"Working…", L"Executando…");
            } else if (m_komorebiFailed) {
                text += std::wstring(L"  ·  ") +
                        Tr(L"It failed (see the terminal)",
                           L"Falhou (veja o terminal)");
            } else if (!m_komorebiRunning) {
                text += std::wstring(L"  ·  ") + Tr(L"Stopped", L"Parado");
            }
        }
        brush->SetColor(Fg(0.6f));
        target->DrawText(text.c_str(), (UINT32)text.size(),
                         m_smallFormat.get(),
                         {rect.left, m_komorebiButtons[0].bottom, rect.right,
                          rect.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    if (id == ControlId::Battery &&
        m_batteryTile.right > m_batteryTile.left) {
        const int tenth = (m_batteryPercent + 5) / 10;
        const WCHAR glyph =
            tenth >= 10 ? (m_charging ? (WCHAR)0xEA93 : (WCHAR)0xE83F)
                        : (WCHAR)((m_charging ? 0xE85A : 0xE850) + tenth);
        DrawTile(target, brush, m_batteryTile, glyph, m_charging,
                 (std::to_wstring(m_batteryPercent) + L"%").c_str(),
                 m_charging ? Tr(L"Charging", L"Carregando")
                            : Tr(L"On battery", L"Na bateria"),
                 m_hover.part == Part::Battery);
    }
}

// The "-" on a control's corner while editing, like on the iPhone.
void ControlPanel::DrawRemoveBadge(ID2D1RenderTarget* target,
                                   ID2D1SolidColorBrush* brush,
                                   const D2D1_RECT_F& rect,
                                   float amount) {
    if (amount <= 0.01f || !m_badgeFormat) {
        return;
    }
    const float s = m_scale;
    const float radius = std::round(9 * s) * std::clamp(amount, 0.0f, 1.1f);
    const D2D1_POINT_2F center{rect.left + std::round(3 * s),
                               rect.top + std::round(3 * s)};
    const float opacity = std::clamp(amount, 0.0f, 1.0f);
    const float lit = Hover(HoverKey(4003, 0), m_hover.part == Part::EditRemove);
    brush->SetColor(D2D1::ColorF(0.42f + 0.13f * lit, 0.42f + 0.13f * lit,
                                 0.44f + 0.13f * lit, opacity));
    target->FillEllipse({center, radius, radius}, brush);
    brush->SetColor(D2D1::ColorF(1, 1, 1, opacity));
    const WCHAR minus = 0xE738;
    DrawGlyph(target, &minus, 1, m_badgeFormat.get(),
              {center.x - radius, center.y - radius, center.x + radius,
               center.y + radius},
              brush);
}

// The chosen controls, each where it's shown (sliding to its place); while
// editing each has a "-", and the dragged one follows the mouse,
// a bit bigger. Below: the footer, or the controls to add and "Done".
void ControlPanel::DrawMain(ID2D1RenderTarget* target,
                           ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    const float edit = (float)std::clamp(m_editAmount, 0.0, 1.0);
    if (edit <= 0.001f) {
        m_tiles.clear();
    }
    D2D1_MATRIX_3X2_F base;
    target->GetTransform(&base);
    const D2D1::Matrix3x2F& baseMatrix =
        *D2D1::Matrix3x2F::ReinterpretBaseType(&base);

    // Removed ones, shrinking and fading away.
    for (const auto& ghost : m_ghosts) {
        const float amount = (float)std::clamp(ghost.amount, 0.0, 1.0);
        const D2D1_POINT_2F center{(ghost.rect.left + ghost.rect.right) / 2,
                                   (ghost.rect.top + ghost.rect.bottom) / 2};
        winrt::com_ptr<ID2D1Layer> layer;
        target->CreateLayer(nullptr, layer.put());
        target->PushLayer(
            D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                  D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                  D2D1::IdentityMatrix(), amount),
            layer.get());
        const float scale = 0.6f + 0.4f * amount;
        target->SetTransform(D2D1::Matrix3x2F::Scale(scale, scale, center) *
                             baseMatrix);
        DrawControl(target, brush, ghost.id, ghost.rect);
        target->SetTransform(base);
        target->PopLayer();
    }

    auto drawAt = [&](ControlId id, const D2D1_RECT_F& rect, bool dragged) {
        const Motion* motion = FindMotion(id);
        const float dx = motion ? (float)motion->x - rect.left : 0;
        const float dy = motion ? (float)motion->y - rect.top : 0;
        float scale = motion ? (float)motion->scale : 1;
        if (dragged) {
            scale *= 1.06f;
        }
        const D2D1_POINT_2F center{(rect.left + rect.right) / 2,
                                   (rect.top + rect.bottom) / 2};
        target->SetTransform(D2D1::Matrix3x2F::Scale(scale, scale, center) *
                             D2D1::Matrix3x2F::Translation(dx, dy) *
                             baseMatrix);
        const float margin = std::round(4 * s);
        if (ID2D1Bitmap* tile = edit > 0.001f ? GetControlTile(target, brush,
                                                               id, rect, margin)
                                              : nullptr) {
            const D2D1_SIZE_F size = tile->GetSize();
            DrawBitmapSmooth(target, tile,
                             {rect.left - margin, rect.top - margin,
                              rect.left - margin + size.width,
                              rect.top - margin + size.height});
        } else {
            DrawControl(target, brush, id, rect);
        }
        DrawRemoveBadge(target, brush, rect, edit);
        target->SetTransform(base);
    };
    for (const auto& [id, rect] : m_controlRects) {
        if (m_dragMoved && (int)id == m_dragId) {
            continue;
        }
        drawAt(id, rect, false);
    }
    // The dragged one, on top.
    for (const auto& [id, rect] : m_controlRects) {
        if (m_dragMoved && (int)id == m_dragId) {
            drawAt(id, rect, true);
        }
    }

    if (m_editing) {
        // The controls to add.
        if (!m_addRects.empty()) {
            PCWSTR add = Tr(L"Add controls", L"Adicionar controles");
            brush->SetColor(Fg(0.5f * edit));
            target->DrawText(add, (UINT32)wcslen(add), m_smallFormat.get(),
                             m_addHeader, brush);
        }
        for (const auto& [id, rect] : m_addRects) {
            const ControlInfo& info = GetControlInfo(id);
            const bool hover =
                m_hover.part == Part::EditAdd && m_hover.index == (int)id;
            const float radius = std::round(10 * s);
            brush->SetColor(Fg((0.07f + 0.07f * Hover(rect, hover)) * edit));
            GlassFill(target, {rect, radius, radius}, brush);
            const float icon = std::round(28 * s);
            brush->SetColor(Fg(0.85f * edit));
            DrawGlyph(target, &info.glyph, 1, m_iconFormat.get(),
                      {rect.left + std::round(2 * s), rect.top,
                       rect.left + icon, rect.bottom},
                      brush);
            brush->SetColor(Fg(0.85f * edit));
            DrawMarquee(target, brush, Tr(info.english, info.portuguese),
                        m_smallFormat.get(),
                        {rect.left + icon, rect.top,
                         rect.right - std::round(6 * s), rect.bottom});
        }
        // "Done", blue.
        const float doneRadius = (m_done.bottom - m_done.top) / 2;
        brush->SetColor(Accent(
            1.0f - 0.15f * Hover(m_done, m_hover.part == Part::EditDone)));
        target->FillRoundedRectangle({m_done, doneRadius, doneRadius}, brush);
        PCWSTR done = Tr(L"Done", L"Concluído");
        brush->SetColor(AccentInk(1));
        target->DrawText(done, (UINT32)wcslen(done), m_linkFormat.get(),
                         m_done, brush);
        return;
    }
    if (m_only != Only::All) {
        return;
    }

    PCWSTR more = Tr(L"More Windows controls", L"Mais controles do Windows");
    brush->SetColor(
        Fg(0.55f + 0.35f * Hover(m_more, m_hover.part == Part::More)));
    DrawMarquee(target, brush, more, m_linkFormat.get(), m_more, true);
    // "Edit", with a pencil.
    const float editRadius = (m_edit.bottom - m_edit.top) / 2 - 2;
    const D2D1_RECT_F editBack{m_edit.left, m_edit.top + 2, m_edit.right,
                               m_edit.bottom - 2};
    brush->SetColor(
        Fg(0.1f + 0.1f * Hover(m_edit, m_hover.part == Part::EditButton)));
    GlassFill(target, {editBack, editRadius, editRadius}, brush);
    const std::wstring editText =
        std::wstring(1, kGlyphEdit) + L"  " + Tr(L"Edit", L"Editar");
    brush->SetColor(Fg(0.9f));
    winrt::com_ptr<IDWriteTextLayout> editLayout;
    if (SUCCEEDED(m_dwrite->CreateTextLayout(
            editText.c_str(), (UINT32)editText.size(), m_linkFormat.get(),
            m_edit.right - m_edit.left, m_edit.bottom - m_edit.top,
            editLayout.put()))) {
        // The pencil in the icon font.
        editLayout->SetFontFamilyName(IconFont(), {0, 1});
        target->DrawTextLayout({m_edit.left, m_edit.top}, editLayout.get(),
                               brush);
    }
    // The island's settings: a gear, round.
    const D2D1_POINT_2F gearCenter{
        (m_settingsButton.left + m_settingsButton.right) / 2,
        (m_settingsButton.top + m_settingsButton.bottom) / 2};
    brush->SetColor(
        Fg(0.1f + 0.1f * Hover(m_settingsButton,
                               m_hover.part == Part::SettingsButton)));
    GlassFillEllipse(target, {gearCenter, editRadius, editRadius}, brush);
    brush->SetColor(Fg(0.9f));
    DrawGlyph(target, &kGlyphSettings, 1, m_iconFormat.get(), m_settingsButton,
              brush);
}

void ControlPanel::DrawPowerPlans(ID2D1RenderTarget* target,
                                 ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    DrawHeader(target, brush, Tr(L"Power plan", L"Plano de energia"));
    for (size_t i = 0; i < m_powerPlans.size() && i < m_rows.size(); i++) {
        const D2D1_RECT_F& row = m_rows[i];
        const bool active = (int)i == m_activePlan;
        const float lit = Hover(
            row, m_hover.part == Part::PowerPlanRow && m_hover.index == (int)i);
        if (lit > 0.01f || active) {
            const float radius = std::round(10 * s);
            const float rest = active ? 0.08f : 0.0f;
            brush->SetColor(Fg(rest + (0.14f - rest) * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        const float check = std::round(36 * s);
        const std::wstring& name = m_powerPlans[i].name;
        brush->SetColor(Fg(0.95f));
        target->DrawText(name.c_str(), (UINT32)name.size(),
                         m_subtitleFormat.get(),
                         {row.left + std::round(12 * s), row.top,
                          row.right - check, row.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        if (active) {
            brush->SetColor(Accent(1));
            DrawGlyph(target, &kGlyphCheck, 1, m_iconFormat.get(),
                      {row.right - check, row.top, row.right,
                       row.bottom},
                      brush);
        }
    }
}

void ControlPanel::DrawOutputs(ID2D1RenderTarget* target,
                              ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    DrawHeader(target, brush, Tr(L"Sound output", L"Saída de som"));
    for (size_t i = 0; i < m_outputs.size() && i < m_rows.size(); i++) {
        const auto& output = m_outputs[i];
        const D2D1_RECT_F& row = m_rows[i];
        const bool hover = m_hover.part == Part::Output && m_hover.index == (int)i;
        const float lit = Hover(row, hover);
        if (lit > 0.01f || output.isDefault) {
            const float radius = std::round(10 * s);
            const float rest = output.isDefault ? 0.08f : 0.0f;
            brush->SetColor(Fg(rest + (0.14f - rest) * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        const float icon = std::round(36 * s);
        brush->SetColor(Fg(0.95f));
        DrawGlyph(target, &output.glyph, 1, m_iconFormat.get(),
                  {row.left, row.top, row.left + icon, row.bottom},
                  brush);
        target->DrawText(output.name.c_str(), (UINT32)output.name.size(),
                         m_subtitleFormat.get(),
                         {row.left + icon, row.top, row.right - icon,
                          row.bottom},
                         brush);
        if (output.isDefault) {
            brush->SetColor(Accent(1));
            DrawGlyph(target, &kGlyphCheck, 1, m_iconFormat.get(),
                      {row.right - icon, row.top, row.right,
                       row.bottom},
                      brush);
        }
    }
}

// The app's icon as a Direct2D bitmap, made once.
ID2D1Bitmap* MixerAppBitmap(ID2D1RenderTarget* target,
                           IWICImagingFactory* wic, MixerApp& app) {
    if (app.bitmap || !app.icon || !wic) {
        return app.bitmap.get();
    }
    winrt::com_ptr<IWICBitmap> wicBitmap;
    winrt::com_ptr<IWICFormatConverter> converter;
    if (SUCCEEDED(wic->CreateBitmapFromHICON(app.icon, wicBitmap.put())) &&
        SUCCEEDED(wic->CreateFormatConverter(converter.put())) &&
        SUCCEEDED(converter->Initialize(
            wicBitmap.get(), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0,
            WICBitmapPaletteTypeMedianCut))) {
        target->CreateBitmapFromWicBitmap(converter.get(), nullptr,
                                          app.bitmap.put());
    }
    return app.bitmap.get();
}

