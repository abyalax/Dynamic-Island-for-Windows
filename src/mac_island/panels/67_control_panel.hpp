// Editing a profile: its name typed in the box (`fresh`: a new one, whose
// name starts selected, so typing replaces it).
void ControlPanel::StartProfileEdit(int id, bool fresh) {
    for (const auto& profile : g_profiles) {
        if (profile.id == id) {
            m_profileEdit = id;
            m_profileName = profile.name;
            m_profileNameEdit.Reset();
            m_profileNameEdit.caret = m_profileName.size();
            if (fresh) {
                m_profileNameEdit.anchor = 0;
            }
            m_profileTyping = true;
            m_profileDeleteArmed = false;
            Layout();
            StartIslandAnimation();
            return;
        }
    }
}

// Done editing: the name typed kept (`keep`, if there's one), or not.
void ControlPanel::EndProfileEdit(bool keep) {
    if (m_profileEdit < 0) {
        return;
    }
    if (keep) {
        RenameProfile(m_profileEdit, m_profileName);
    }
    m_profileEdit = -1;
    m_profileTyping = false;
    m_profileDeleteArmed = false;
    Layout();
    StartIslandAnimation();
}

// The profiles: a chip each (the one applied last lit, a pencil to edit it;
// a new one comes from the "+" beside the title), the one being edited (its
// name, "save the current settings in it", "delete", "done"), and how it
// works.
void ControlPanel::DrawProfiles(ID2D1RenderTarget* target,
                                ID2D1SolidColorBrush* brush,
                                const D2D1_RECT_F& row) {
    const float s = m_scale;
    const D2D1_COLOR_F accent = Accent(1);
    // Another island than the main one: its monitor's profile is chosen
    // here.
    const bool monitor = g_island && !g_island->HasMainSettings();
    const int chosen = ChosenProfile();
    for (size_t i = 0; i < m_profileChips.size(); i++) {
        const D2D1_RECT_F& chip = m_profileChips[i];
        const float radius = (chip.bottom - chip.top) / 2;
        const bool hover = (m_hover.part == Part::ProfileChip ||
                            m_hover.part == Part::ProfileEditButton) &&
                           m_hover.index == (int)i;
        const float lit = Hover(HoverKey(4100, (int)i), hover);
        if (i >= g_profiles.size()) {
            const bool main = monitor && chosen < 0;
            brush->SetColor(main ? D2D1::ColorF(accent.r, accent.g, accent.b,
                                                0.85f + 0.15f * lit)
                                 : Fg(0.05f + 0.06f * lit));
            GlassFill(target, {chip, radius, radius}, brush);
            brush->SetColor(main ? AccentInk(1) : Fg(0.7f));
            DrawMarquee(target, brush,
                        Tr(L"Same as the main one", L"Igual à principal"),
                        m_smallFormat.get(), chip, true);
            continue;
        }
        const IslandProfile& profile = g_profiles[i];
        const bool active = profile.id == chosen;
        const bool editing = profile.id == m_profileEdit;
        if (active) {
            brush->SetColor(D2D1::ColorF(accent.r, accent.g, accent.b,
                                         0.85f + 0.15f * lit));
        } else {
            brush->SetColor(Fg(0.08f + 0.07f * lit + (editing ? 0.06f : 0)));
        }
        GlassFill(target, {chip, radius, radius}, brush);
        const D2D1_RECT_F& pencil = m_profilePencils[i];
        brush->SetColor(active ? AccentInk(1) : Fg(0.92f));
        DrawMarquee(target, brush,
                    editing && !m_profileName.empty() ? m_profileName
                                                      : profile.name,
                    m_smallFormat.get(),
                    {chip.left + std::round(10 * s), chip.top,
                     pencil.left + std::round(2 * s), chip.bottom});
        // The pencil, clearer under the mouse.
        const bool pencilHover = m_hover.part == Part::ProfileEditButton &&
                                 m_hover.index == (int)i;
        const float pencilLit =
            Hover(HoverKey(4101, (int)i), pencilHover || editing);
        const WCHAR glyph = 0xE70F;
        brush->SetColor(active ? AccentInk(0.55f + 0.45f * pencilLit)
                               : Fg(0.35f + 0.6f * pencilLit));
        DrawGlyph(target, &glyph, 1, m_smallIconFormat.get(), pencil, brush);
    }
    float hintTop = m_profileChips.empty() ? row.top
                                           : m_profileChips.back().bottom +
                                                 std::round(6 * s);
    if (m_profileEdit >= 0) {
        // The name, typed in its box.
        const D2D1_RECT_F& field = m_profileNameField;
        const float fieldRadius = std::round(8 * s);
        brush->SetColor(Fg(0.07f));
        GlassFill(target, {field, fieldRadius, fieldRadius}, brush);
        if (m_profileTyping) {
            brush->SetColor(accent);
            target->DrawRoundedRectangle(
                {{field.left + 0.5f, field.top + 0.5f, field.right - 0.5f,
                  field.bottom - 0.5f},
                 fieldRadius,
                 fieldRadius},
                brush, std::max(1.0f, std::round(1.5f * s)));
        }
        const D2D1_RECT_F text{field.left + std::round(10 * s), field.top,
                               field.right - std::round(10 * s), field.bottom};
        const float caret =
            DrawLineEdit(target, brush, m_dwrite, m_smallFormat.get(),
                         text, m_profileName, m_profileNameEdit, Fg(0.95f));
        if (m_profileTyping) {
            brush->SetColor(CaretColor());
            target->FillRectangle(
                {caret + 1, field.top + std::round(9 * s),
                 caret + 1 + std::max(1.0f, std::round(1.5f * s)),
                 field.bottom - std::round(9 * s)},
                brush);
        }
        // Its buttons.
        auto button = [&](const D2D1_RECT_F& rect, Part part, PCWSTR label,
                          bool danger) {
            const float buttonRadius = (rect.bottom - rect.top) / 2;
            const float lit =
                Hover(HoverKey(4102, (int)part), m_hover.part == part);
            if (danger) {
                brush->SetColor(
                    D2D1::ColorF(0.86f, 0.2f, 0.18f, 0.85f + 0.15f * lit));
            } else {
                brush->SetColor(Fg(0.09f + 0.07f * lit));
            }
            GlassFill(target, {rect, buttonRadius, buttonRadius}, brush);
            brush->SetColor(danger ? D2D1::ColorF(1, 1, 1, 1) : Fg(0.9f));
            DrawMarquee(target, brush, label, m_smallFormat.get(), rect, true);
        };
        button(m_profileUpdate, Part::ProfileUpdate,
               Tr(L"Save current here", L"Salvar o atual nele"), false);
        button(m_profileDelete, Part::ProfileDelete,
               m_profileDeleteArmed ? Tr(L"Delete it?", L"Excluir mesmo?")
                                    : Tr(L"Delete", L"Excluir"),
               m_profileDeleteArmed);
        button(m_profileDone, Part::ProfileDone, Tr(L"Done", L"Pronto"),
               false);
        hintTop = m_profileDone.bottom + std::round(6 * s);
        // What turns it on by itself: lit when it does.
        auto choice = [&](const D2D1_RECT_F& rect, int key, bool hover,
                          const std::wstring& label, bool on) {
            const float chipRadius = (rect.bottom - rect.top) / 2;
            const float lit = Hover(HoverKey(4103, key), hover);
            brush->SetColor(on ? D2D1::ColorF(accent.r, accent.g, accent.b,
                                              0.85f + 0.15f * lit)
                               : Fg(0.07f + 0.07f * lit));
            target->FillRoundedRectangle({rect, chipRadius, chipRadius},
                                         brush);
            brush->SetColor(on ? AccentInk(1) : Fg(0.85f));
            DrawMarquee(target, brush, label, m_smallFormat.get(),
                        {rect.left + std::round(8 * s), rect.top,
                         rect.right - std::round(8 * s), rect.bottom},
                        true);
            hintTop = std::max(hintTop, rect.bottom + std::round(6 * s));
        };
        for (size_t i = 0; i < m_profileStyleChips.size(); i++) {
            choice(m_profileStyleChips[i], (int)i,
                   m_hover.part == Part::ProfileStyleChip &&
                       m_hover.index == (int)i,
                   i == 0   ? Tr(L"With the Island style",
                                 L"Com o estilo Ilha")
                   : i == 1 ? Tr(L"With the Fixed style", L"Com o estilo Fixa")
                            : Tr(L"With the Bar style", L"Com o estilo Barra"),
                   AutoProfileHas(m_profileEdit, L's',
                                  std::to_wstring(StyleSegmentStyle((int)i))));
        }
        for (size_t i = 0; i < m_profileAppChips.size(); i++) {
            choice(m_profileAppChips[i], 10 + (int)i,
                   m_hover.part == Part::ProfileAppChip &&
                       m_hover.index == (int)i,
                   AppDisplayName(m_profileApps[i]) + L"  \u00D7", true);
        }
        if (!m_profileAddApp.empty()) {
            choice(m_profileAppAdd, 9, m_hover.part == Part::ProfileAppAdd,
                   L"+ " + AppDisplayName(m_profileAddApp), false);
        }
        if (m_profileKeep[0].right > m_profileKeep[0].left) {
            const bool stays = AutoProfileHas(m_profileEdit, L'k', L"1");
            choice(m_profileKeep[0], 7,
                   m_hover.part == Part::ProfileKeepChip &&
                       m_hover.index == 0,
                   Tr(L"After its app: the one before",
                      L"Depois do app: o de antes"),
                   !stays);
            choice(m_profileKeep[1], 8,
                   m_hover.part == Part::ProfileKeepChip &&
                       m_hover.index == 1,
                   Tr(L"After its app: stays", L"Depois do app: fica"), stays);
        }
    }
    const int hint = monitor              ? (g_island->HasOwnLook() ? 0 : 1)
                     : g_profiles.empty() ? 2
                     : NowSeconds() - m_profileSavedAt < 1.5 ? 3
                     : g_activeProfile < 0                   ? 4
                                                             : 5;
    brush->SetColor(Fg(0.45f));
    DrawWrapped(target, brush, ProfilesHint(hint), m_smallFormat.get(),
                {row.left + std::round(4 * s), hintTop, row.right,
                 std::max(hintTop + std::round(18 * s), row.bottom)});
}

// Where the island appears (three big buttons, each saying what it does),
// the screens as a map (big numbers, the islands marked), and with an
// island on each monitor, a card for each: its notifications, its profile.
void ControlPanel::LayoutMonitors(const D2D1_RECT_F& row) {
    const float s = m_scale;
    m_monitorParts.clear();
    m_monitorLabels.clear();
    m_monitorCards.clear();
    m_monitorTitles.clear();
    const auto monitors = ListMonitors();
    bool notches = false;
    {
        IslandScope scope(MainIsland());
        notches = NotchCountWanted() > 0;
    }
    const bool all = g_settings.islandAllMonitors;
    const bool follow = g_settings.islandFollowMouse && !all && !notches;
    const int mode = all ? 1 : follow ? 2 : 0;
    const float left = row.left + std::round(4 * s);
    const float right = row.right - std::round(4 * s);
    const float gap = std::round(8 * s);
    float top = row.top + std::round(4 * s);
    m_monitorTitles.push_back(
        {{left, top, right, top + 24 * s},
         Tr(L"Where the island appears", L"Onde a ilha aparece")});
    top += 28 * s;
    // (Its name, then a line saying what it does.)
    const float third = (right - left - gap * 2) / 3;
    for (int i = 0; i < 3; i++) {
        m_monitorParts.push_back(
            {{left + (third + gap) * i, top, left + (third + gap) * i + third,
              top + 58 * s},
             Part::MonitorMode,
             i,
             {},
             i == 0 ? std::wstring(Tr(L"On one monitor", L"Num monitor")) +
                          L"\n" +
                          Tr(L"The one chosen on the map",
                             L"A tela escolhida no mapa")
             : i == 1
                 ? std::wstring(Tr(L"On every monitor", L"Em todos os monitores")) +
                       L"\n" +
                       Tr(L"Each screen has its own island",
                          L"Cada tela tem a sua ilha")
                 : std::wstring(Tr(L"Follows the mouse", L"Segue o mouse")) +
                       L"\n" +
                       Tr(L"It goes to the screen with the mouse",
                          L"Vai para a tela onde está o mouse"),
             mode == i,
             i == 2 && notches});
    }
    top += 68 * s;
    m_monitorTitles.push_back(
        {{left, top, right, top + 24 * s}, Tr(L"Screens", L"Telas")});
    top += 26 * s;
    RECT bounds{};
    if (!monitors.empty()) {
        bounds = monitors.front().rect;
        for (const auto& monitor : monitors) {
            bounds.left = std::min(bounds.left, monitor.rect.left);
            bounds.top = std::min(bounds.top, monitor.rect.top);
            bounds.right = std::max(bounds.right, monitor.rect.right);
            bounds.bottom = std::max(bounds.bottom, monitor.rect.bottom);
        }
    }
    const float mapWidth = right - left;
    const float mapHeight = 180 * s;
    const float scale =
        std::min(mapWidth / std::max(1L, bounds.right - bounds.left),
                 mapHeight / std::max(1L, bounds.bottom - bounds.top));
    const float mapLeft =
        left + (mapWidth - (bounds.right - bounds.left) * scale) / 2;
    const float mapTop =
        top + (mapHeight - (bounds.bottom - bounds.top) * scale) / 2;
    const int chosen = ChosenMonitorIndex();
    for (size_t i = 0; i < monitors.size(); i++) {
        const auto& monitor = monitors[i];
        const D2D1_RECT_F screen{
            mapLeft + (monitor.rect.left - bounds.left) * scale + 3 * s,
            mapTop + (monitor.rect.top - bounds.top) * scale + 3 * s,
            mapLeft + (monitor.rect.right - bounds.left) * scale - 3 * s,
            mapTop + (monitor.rect.bottom - bounds.top) * scale - 3 * s};
        m_monitorParts.push_back({screen, Part::MonitorMap, (int)i,
                                  monitor.device,
                                  MonitorNumber(monitor.device),
                                  all || (int)i == chosen, follow});
        // (The main island's: the monitor chosen, not Windows' primary.)
        if ((int)i == chosen || all) {
            m_monitorParts.back().label =
                (int)i == chosen ? Tr(L"Main island", L"Ilha principal")
                                 : Tr(L"Island", L"Ilha");
        }
    }
    top += mapHeight + 8 * s;
    PCWSTR hint =
        all ? Tr(L"Click a screen to place the main island there (alerts and "
                 L"tray).",
                 L"Clique numa tela para pôr a ilha principal nela (avisos e "
                 L"bandeja).")
        : follow ? Tr(L"The island follows the mouse between screens.",
                      L"A ilha segue o mouse entre as telas.")
                 : Tr(L"Click a screen to choose where the island appears.",
                      L"Clique numa tela para escolher onde a ilha aparece.");
    m_monitorLabels.push_back({{left, top, right, top + 20 * s}, hint});
    top += 22 * s;
    if (notches) {
        m_monitorLabels.push_back({{left, top, right, top + 20 * s},
                                   Tr(L"Following the mouse is unavailable "
                                      L"with notches on the main island.",
                                      L"Segue o mouse fica indisponível com "
                                      L"entalhes na ilha principal.")});
    }
    top = row.top + kMonitorsTop * s;
    if (!all) {
        return;
    }
    // A card for the main island: whether it shows the notifications (it
    // does by default).
    const float pad = 12 * s;
    m_monitorCards.push_back({left, top, right, top + (kMonitorsMainCard - 10) * s});
    m_monitorTitles.push_back(
        {{left + pad, top + pad, right - pad, top + pad + 22 * s},
         std::wstring(Tr(L"Monitor ", L"Monitor ")) +
             (chosen >= 0 && chosen < (int)monitors.size()
                  ? MonitorNumber(monitors[chosen].device)
                  : L"") +
             Tr(L" · main island", L" · ilha principal")});
    m_monitorParts.push_back(
        {{left + pad, top + pad + 28 * s, right - pad, top + pad + 60 * s},
         Part::MonitorNotify,
         0,
         {},
         Tr(L"Shows the notifications", L"Mostra as notificações"),
         g_notifyMain,
         false});
    top += kMonitorsMainCard * s;
    // A card for each other island: its notifications, then its profile.
    const float chipThird = (right - left - pad * 2 - gap * 2) / 3;
    for (Island* other : OtherIslands()) {
        const std::wstring& device = other->OwnDevice();
        const float rows = (float)((g_profiles.size() + 3) / 3);
        const float bottom = top + (kMonitorsCardBase + rows * 38 - 10) * s;
        m_monitorCards.push_back({left, top, right, bottom});
        // (The island these settings belong to, marked.)
        const bool own = other == Owner();
        m_monitorTitles.push_back(
            {{left + pad, top + pad, right - pad, top + pad + 22 * s},
             std::wstring(Tr(L"Monitor ", L"Monitor ")) +
                 MonitorNumber(device) +
                 (own ? Tr(L" · this island", L" · esta ilha") : L"")});
        float y = top + pad + 28 * s;
        m_monitorParts.push_back(
            {{left + pad, y, right - pad, y + 32 * s},
             Part::MonitorNotify,
             0,
             device,
             Tr(L"Shows the notifications", L"Mostra as notificações"),
             other->ShowsNotifications(),
             false});
        y += 40 * s;
        m_monitorLabels.push_back(
            {{left + pad, y, right - pad, y + 20 * s},
             Tr(L"Its look (profile):", L"A aparência dela (perfil):")});
        y += 24 * s;
        for (size_t i = 0; i <= g_profiles.size(); i++) {
            const int id = i == 0 ? -1 : g_profiles[i - 1].id;
            const float x = left + pad + (chipThird + gap) * (float)(i % 3);
            const float chipY = y + 38 * s * (float)(i / 3);
            m_monitorParts.push_back(
                {{x, chipY, x + chipThird, chipY + 32 * s},
                 Part::MonitorProfile,
                 id,
                 device,
                 i == 0 ? std::wstring(
                              Tr(L"Same as the main one", L"Igual à principal"))
                        : g_profiles[i - 1].name,
                 other->MonitorProfile() == id,
                 false});
        }
        top = bottom + 10 * s;
    }
}

void ControlPanel::DrawMonitors(ID2D1RenderTarget* target,
                                ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    for (const auto& card : m_monitorCards) {
        const float radius = std::round(10 * s);
        brush->SetColor(Fg(0.045f));
        GlassFill(target, {card, radius, radius}, brush);
    }
    for (const auto& [rect, text] : m_monitorTitles) {
        brush->SetColor(Fg(0.95f));
        DrawMarquee(target, brush, text, m_titleFormat.get(), rect);
    }
    for (size_t i = 0; i < m_monitorParts.size(); i++) {
        const auto& part = m_monitorParts[i];
        const float opacity = part.disabled ? 0.4f : 1.0f;
        const float lit = Hover(HoverKey(4120, (int)i),
                                !part.disabled && m_hover.part == part.part &&
                                    m_hover.index == (int)i);
        const bool screen = part.part == Part::MonitorMap;
        const float radius = std::round((screen ? 8 : 10) * s);
        const float chosen =
            Hover(HoverKey(4121, (int)i), part.chosen, part.chosen ? 1 : 0);
        // A screen: dark like a monitor, the chosen ones tinted and ringed;
        // a button: lit in the accent color's tint when chosen, ringed; an
        // on/off: its name and a switch, like the settings' rows.
        brush->SetColor(screen ? Fg((0.12f + 0.06f * lit) * opacity)
                               : Fg((0.07f + 0.06f * lit) * opacity));
        GlassFill(target, {part.rect, radius, radius}, brush);
        if (part.part == Part::MonitorNotify) {
            const float inset = 12 * s;
            const float middle = (part.rect.top + part.rect.bottom) / 2;
            const float switchWidth = std::round(36 * s);
            const float switchHeight = std::round(20 * s);
            const D2D1_RECT_F toggle{
                part.rect.right - inset - switchWidth,
                std::round(middle - switchHeight / 2),
                part.rect.right - inset,
                std::round(middle - switchHeight / 2) + switchHeight};
            brush->SetColor(Fg(0.9f * opacity));
            DrawMarquee(target, brush, part.text, m_smallFormat.get(),
                        {part.rect.left + inset, part.rect.top,
                         toggle.left - inset, part.rect.bottom});
            DrawToggle(target, brush, toggle,
                       KnobFor(9000 + (int)i, part.chosen));
            continue;
        }
        if (chosen > 0.01f) {
            brush->SetColor(Accent(0.18f * chosen * opacity));
            target->FillRoundedRectangle({part.rect, radius, radius}, brush);
            brush->SetColor(Accent(chosen * opacity));
            target->DrawRoundedRectangle(
                {{part.rect.left + 1, part.rect.top + 1, part.rect.right - 1,
                  part.rect.bottom - 1},
                 radius - 1,
                 radius - 1},
                brush, std::max(1.5f, std::round(2 * s)));
        } else if (screen) {
            brush->SetColor(Fg(0.25f * opacity));
            target->DrawRoundedRectangle({part.rect, radius, radius}, brush, 1);
        }
        if (screen) {
            // Its number, big, a little over its middle; its island under
            // it, centered.
            const float middle = (part.rect.top + part.rect.bottom) / 2;
            brush->SetColor(Fg(0.7f * opacity));
            DrawMarquee(target, brush, part.label, m_smallFormat.get(),
                        {part.rect.left + 6 * s, middle + 12 * s,
                         part.rect.right - 6 * s, middle + 30 * s},
                        true);
            winrt::com_ptr<IDWriteTextLayout> number;
            if (SUCCEEDED(m_dwrite->CreateTextLayout(
                    part.text.c_str(), (UINT32)part.text.size(),
                    m_pageTitleFormat.get(), part.rect.right - part.rect.left,
                    30 * s, number.put()))) {
                number->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                number->SetParagraphAlignment(
                    DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                brush->SetColor(Fg(0.95f * opacity));
                target->DrawTextLayout({part.rect.left, middle - 20 * s},
                                       number.get(), brush);
            }
            continue;
        }
        // Its name; a mode's line about it under the name.
        const size_t split = part.text.find(L'\n');
        if (split == std::wstring::npos) {
            brush->SetColor(Fg(0.9f * opacity));
            DrawMarquee(target, brush, part.text, m_smallFormat.get(),
                        {part.rect.left + 6 * s, part.rect.top,
                         part.rect.right - 6 * s, part.rect.bottom},
                        true);
            continue;
        }
        const float inset = 12 * s;
        brush->SetColor(Fg(0.95f * opacity));
        DrawMarquee(target, brush, part.text.substr(0, split),
                    m_titleFormat.get(),
                    {part.rect.left + inset, part.rect.top + 8 * s,
                     part.rect.right - inset, part.rect.top + 28 * s});
        brush->SetColor(Fg(0.6f * opacity));
        DrawMarquee(target, brush, part.text.substr(split + 1),
                    m_subtitleFormat.get(),
                    {part.rect.left + inset, part.rect.top + 30 * s,
                     part.rect.right - inset, part.rect.bottom - 6 * s});
    }
    for (const auto& [rect, text] : m_monitorLabels) {
        brush->SetColor(Fg(0.6f));
        DrawMarquee(target, brush, text, m_smallFormat.get(), rect);
    }
}

// A slider: its track, filled up to the knob, the usual value marked on it,
// and the knob (bigger while dragged or under the mouse).
void ControlPanel::DrawSliderTrack(ID2D1RenderTarget* target,
                                   ID2D1SolidColorBrush* brush,
                                   const D2D1_RECT_F& slider,
                                   int min,
                                   int max,
                                   int usual,
                                   int value,
                                   bool active) {
    const float s = m_scale;
    const float inset = std::round(10 * s);
    const float middle = (slider.top + slider.bottom) / 2;
    const float thickness = std::round(4 * s);
    const D2D1_RECT_F track{slider.left + inset, middle - thickness / 2,
                            slider.right - inset, middle + thickness / 2};
    const float fraction =
        std::clamp((float)(value - min) / (max - min), 0.0f, 1.0f);
    const float knobX = track.left + (track.right - track.left) * fraction;
    brush->SetColor(Fg(0.15f));
    target->FillRoundedRectangle({track, thickness / 2, thickness / 2}, brush);
    brush->SetColor(Accent(1));
    target->FillRoundedRectangle(
        {{track.left, track.top, knobX, track.bottom},
         thickness / 2,
         thickness / 2},
        brush);
    // The usual value, marked on the track.
    const float usualX = track.left + (track.right - track.left) *
                                          (float)(usual - min) / (max - min);
    brush->SetColor(Fg(0.35f));
    target->FillRectangle(
        {usualX - std::round(1 * s), middle - std::round(6 * s),
         usualX + std::round(1 * s), middle + std::round(6 * s)},
        brush);
    const float knob = std::round((active ? 9 : 8) * s);
    brush->SetColor(AccentInk(1));
    target->FillEllipse({{knobX, middle}, knob, knob}, brush);
    brush->SetColor(D2D1::ColorF(0, 0, 0, 0.25f));
    target->DrawEllipse({{knobX, middle}, knob, knob}, brush, 1);
}

void ControlPanel::SetAppSliderAt(int entry, int which, POINT pt, bool keep) {
    if (entry < 0 || entry >= (int)g_appPlaces.size()) {
        return;
    }
    const int index = entry * kAppChipStride + kStepChipFirst + which * 3 + 1;
    for (const auto& [chipIndex, slider] : m_appItemChips) {
        if (chipIndex != index) {
            continue;
        }
        const IslandSlider& info = kIslandSliders[which ? 0 : 1];
        const float inset = std::round(10 * m_scale);
        const float left = slider.left + inset;
        const float right = slider.right - inset;
        if (right <= left) {
            return;
        }
        const float fraction =
            std::clamp((pt.x - left) / (right - left), 0.0f, 1.0f);
        const int steps = (info.max - info.min) / info.step;
        IslandPlace& place = g_appPlaces[entry];
        (which ? place.size : place.gap) =
            info.min + (int)std::lround(fraction * steps) * info.step;
        if (keep) {
            SavePlaces();
        }
        if (g_island) {
            ForEachIsland([&](Island& island) { island.OnPlacesChanged(); });
            ForEachIsland([&](Island& island) { island.Relayout(); });
        }
        StartIslandAnimation();
        return;
    }
}

void ControlPanel::SetIslandSizeAt(int slider, POINT pt, bool keep) {
    pt = ToSettingsList(pt);
    const IslandSlider& info = kIslandSliders[slider];
    const float inset = std::round(10 * m_scale);
    const float left = m_sizeSlider[slider].left + inset;
    const float right = m_sizeSlider[slider].right - inset;
    if (right <= left) {
        return;
    }
    const float fraction =
        std::clamp((pt.x - left) / (right - left), 0.0f, 1.0f);
    const int steps = (info.max - info.min) / info.step;
    SetIslandSlider(slider,
                    info.min + (int)std::lround(fraction * steps) * info.step,
                    keep);
}

void ControlPanel::DrawSettings(ID2D1RenderTarget* target,
                                ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    DrawSettingsNav(target, brush);
    m_reasonLinks.clear();
    m_noteLinks.clear();

    const D2D1_RECT_F& viewport = m_settingsViewport;
    BeginScrollArea(target, viewport, m_settingsScroll,
                    (float)std::clamp(m_tabFade, 0.0, 1.0));
    const float offset = m_settingsScroll.Offset();
    const float height = viewport.bottom - viewport.top;
    D2D1_MATRIX_3X2_F list;
    target->GetTransform(&list);
    for (const auto& entry : m_settings) {
        const float alpha = (float)std::clamp(entry.alpha, 0.0, 1.0);
        const float entryHeight = entry.rect.bottom - entry.rect.top;
        if (alpha < 0.01f ||
            entry.y + entryHeight < offset - 2 - m_cardPad ||
            entry.y > offset + height + 2 + m_cardPad) {
            continue;
        }
        // Where it's drawn now, fading in or out.
        target->SetTransform(
            D2D1::Matrix3x2F::Translation((float)entry.x - entry.rect.left,
                                          (float)entry.y - entry.rect.top) *
            *D2D1::Matrix3x2F::ReinterpretBaseType(&list));
        // (Its layer only as big as it: one as big as the target for each
        // entry made fading ones slow.)
        winrt::com_ptr<ID2D1Layer> layer;
        if (alpha < 0.999f) {
            const float margin = std::round(4 * s) + m_cardPad;
            target->CreateLayer(nullptr, layer.put());
            target->PushLayer(
                D2D1::LayerParameters(
                    {entry.rect.left - margin, entry.rect.top - margin,
                     entry.rect.right + margin, entry.rect.bottom + margin},
                    nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                    D2D1::IdentityMatrix(), alpha),
                layer.get());
        }
        DrawSettingEntry(target, brush, entry);
        if (layer) {
            target->PopLayer();
        }
    }
    target->SetTransform(list);
    DrawNeeds(target, brush);
    if (m_nothingAlpha > 0.01) {
        PCWSTR nothing = Tr(L"No settings match the search",
                            L"Nenhuma configuração encontrada");
        brush->SetColor(Fg(0.5f * (float)std::clamp(m_nothingAlpha, 0.0, 1.0)));
        target->DrawText(nothing, (UINT32)wcslen(nothing), m_linkFormat.get(),
                         {0, 0, (float)m_size.cx, std::round(64 * s)}, brush);
    }
    EndScrollArea(target);
    DrawScrollBar(target, brush, viewport, m_settingsScroll, 0.6f,
                  Hover(ScrollBarStrip(viewport),
                        m_barHover == 1 || m_barDrag == 1));
    // The setup assistant's buttons under a thin line, and its step between
    // them ("2 of 6").
    const D2D1_RECT_F& cancel = m_setupButtons[kSetupCancel];
    const D2D1_RECT_F& next = m_setupButtons[kSetupNext];
    if (next.right > next.left) {
        brush->SetColor(Fg(0.07f));
        target->FillRectangle(
            {cancel.left, viewport.bottom, next.right, viewport.bottom + 1},
            brush);
        const int step = m_setup.step;
        const PCWSTR names[kSetupButtons] = {
            step == kSetupWelcome ? Tr(L"Skip", L"Pular")
                                  : Tr(L"Cancel", L"Cancelar"),
            Tr(L"Back", L"Voltar"),
            step == kSetupWelcome ? Tr(L"Start", L"Iniciar")
            : step == kSetupDone  ? Tr(L"Finish", L"Concluir")
                                  : Tr(L"Next", L"Avançar")};
        for (int i = 0; i < kSetupButtons; i++) {
            if (m_setupButtons[i].right > m_setupButtons[i].left) {
                DrawButton(target, brush, m_setupButtons[i], names[i],
                           i == kSetupNext,
                           m_hover.part == Part::SetupButton &&
                               m_hover.index == i);
            }
        }
        WCHAR where[32];
        swprintf_s(where, Tr(L"%d of %d", L"%d de %d"), step + 1, kSetupSteps);
        const D2D1_RECT_F& back = m_setupButtons[kSetupBack];
        brush->SetColor(Fg(0.55f));
        target->DrawText(where, (UINT32)wcslen(where), m_smallFormat.get(),
                         {cancel.right + std::round(8 * s), cancel.top,
                          (back.right > back.left ? back.left : next.left) -
                              std::round(8 * s),
                          cancel.bottom},
                         brush);
    }
}

// The frosted glass dimmed only for Windows' transparency effects being off
// (everything else it needs is there): its reason says where to turn them
// on, and a click opens that page.
bool FrostedNeedsOnlyTransparency() {
    return !g_transparencyEffects && !g_settings.islandLiquidGlass &&
           GetIslandPref(IslandPref::Bar) && NotchCountWanted() == 0 &&
           !(GetIslandPref(IslandPref::Bar) == 2 &&
             GetIslandPref(IslandPref::AutoHide));
}

std::wstring ControlPanel::DisabledReason(int index) {
    // "Needs <the option> on", "Off while <the option> is on", with the
    // option's own name.
    auto with = [](PCWSTR english, PCWSTR portuguese, IslandPref pref) {
        std::wstring text = Tr(english, portuguese);
        const IslandPrefInfo& info = kIslandPrefs[(int)pref];
        if (const size_t at = text.find(L"%s"); at != std::wstring::npos) {
            text.replace(at, 2, Tr(info.english, info.portuguese));
        }
        return text;
    };
    auto needs = [&](IslandPref pref) {
        return with(L"Needs “%s” on",
                    L"Precisa de “%s” ligado", pref);
    };
    auto offWhile = [&](IslandPref pref) {
        return with(L"Off while “%s” is on",
                    L"Desligado enquanto “%s” está ligado", pref);
    };
    if (index == kDisabledShortcut) {
        return OtherIslandShown() ? DisabledReason((int)IslandPref::MinimizeKey)
                                  : needs(IslandPref::MinimizeKey);
    }
    if (index >= kDisabledChoice && index < kDisabledAppPlaces) {
        switch (index - kDisabledChoice) {
            case kChoiceAnimationLevel:
                return needs(IslandPref::Animations);
            case kChoiceDateOrder:
                return Tr(L"Needs the date or the year on the clock",
                          L"Precisa da data ou do ano no relógio");
            case kChoiceWeatherUnit:
                return needs(IslandPref::Weather);
            case kChoiceIconColors:
                return Tr(L"Only for emoji and pictures of your own",
                          L"Só para emoji e imagens suas");
            case kChoiceIconFolder:
                return Tr(L"Only with Your folder as the pack",
                          L"Só com Sua pasta como pacote");
            case kChoiceTrayStyle:
                return needs(IslandPref::Tray);
            case kChoiceWallpaper:
                return needs(IslandPref::Wallpaper);
            case kChoiceNotches:
                return Tr(L"Not for the Bar style", L"Não no estilo Barra");
            case kChoiceNotchOpen:
                return g_settings.islandBar
                           ? Tr(L"Not for the Bar style", L"Não no estilo Barra")
                           : Tr(L"Needs the notches", L"Precisa dos entalhes");
            case kChoiceLineFade:
            case kChoiceSetupHide:
                return Tr(L"The bar doesn't minimize", L"A barra não minimiza");
            case kChoiceBarSide:
            case kChoiceBarInnerCorners:
                return Tr(L"Only for the Bar style", L"Só no estilo Barra");
            case kChoiceBannerSize:
            case kChoiceBannerWidth:
            case kChoiceBannerCorners:
            case kChoiceBannerPlace:
                return needs(IslandPref::Banner);
            case kChoiceShelfLimit:
                return needs(IslandPref::Shelf);
            case kChoiceEyeTheme:
            case kChoiceEyeShape:
                return needs(IslandPref::Eyes);
            case kChoiceHideWhen:
                return !g_settings.islandAutoHide
                           ? needs(IslandPref::AutoHide)
                       : g_settings.islandBar || g_settings.islandReserve
                           ? Tr(L"Only for the Island style",
                                L"Só no estilo Ilha")
                           : Tr(L"Not with the notches", L"Não com os entalhes");
            case kChoiceFolderDrag:
                return needs(IslandPref::Folders);
            case kChoiceFillColor:
            case kChoiceFillCopyColor:
                return needs(IslandPref::DownloadFill);
            case kChoiceFillChargeColor:
                return needs(IslandPref::ChargeFill);
            case kChoiceLyricsSize:
            case kChoiceLyricsColor:
            case kChoiceLyricsWidth:
                return needs(IslandPref::Lyrics);
            case kChoiceVisualizerColor:
                return needs(IslandPref::Visualizer);
            case kChoiceRecycleMinSize:
                return needs(IslandPref::RecycleBin);
            case kChoiceFillStyle:
                return Tr(L"Needs a fill on (downloads or charging)",
                          L"Precisa de um preenchimento ligado (downloads ou "
                          L"carga)");
            default:
                return {};
        }
    }
    // (The notches: chosen on their own row.)
    const std::wstring notches =
        Tr(L"Off with the notches (Fixed style)",
           L"Desligado com os entalhes (estilo Fixa)");
    if (index >= kDisabledSlider) {
        const int slider = index - kDisabledSlider;
        const std::wstring bar =
            Tr(L"Off with the bar style", L"Desligado no estilo barra");
        switch (slider) {
            case 1:
            case 13:
                return g_settings.islandBar ? bar : notches;
            case 2:
                return !g_animationChoice.on
                           ? needs(IslandPref::Animations)
                           : Tr(L"Off with no animations (“None”)",
                                L"Desligado sem animações (“Nenhuma”)");
            case 5:
                return needs(IslandPref::AutoHide);
            case 6:
                return Tr(L"Needs the notches (Fixed style)",
                          L"Precisa dos entalhes (estilo Fixa)");
            case 8:
            case 9:
                return Tr(L"The bar doesn't minimize", L"A barra não minimiza");
            case 11:
                return needs(IslandPref::MediaTitle);
            case 12:
                return bar;
            case 16:
                return needs(IslandPref::LiquidGlass);
            case 17:
                return Tr(L"Only with the Grid style",
                          L"Só com o estilo Grade");
            case 18:
                return needs(IslandPref::Visualizer);
            default:
                return {};
        }
    }
    if (index == kDisabledAppPlaces) {
        if (!g_settings.islandBar && !g_settings.islandReserve) {
            return Tr(L"Not with the notches: each one keeps its place and "
                      L"items for every app",
                      L"Não com os entalhes: cada um fica no seu lugar, com "
                      L"os mesmos itens para todos os apps");
        }
        return Tr(L"Only for the Island style: the Fixed style and the bar "
                  L"keep one place and the same items for every app",
                  L"Só no estilo Ilha: o estilo Fixa e a barra ficam num "
                  L"lugar só e com os mesmos itens para todos os apps");
    }
    if (OtherIslandShown() && MainIslandOnly((IslandPref)index)) {
        return Tr(L"Only on the main island: change it in its settings",
                  L"Só na ilha principal: mude nas configurações dela");
    }
    switch ((IslandPref)index) {
        case IslandPref::KomorebiLayout:
            return Tr(L"komorebi isn't installed",
                      L"O komorebi não está instalado");
        case IslandPref::KomorebiHideEmpty:
            return needs(IslandPref::KomorebiLayout);
        case IslandPref::FollowMouse:
            return GetIslandPref(IslandPref::AllMonitors)
                       ? offWhile(IslandPref::AllMonitors)
                       : notches;
        case IslandPref::FreeDrag:
        case IslandPref::VerticalSides:
            return notches;
        case IslandPref::NotchEach:
            return Tr(L"Needs the notches (Fixed style)",
                      L"Precisa dos entalhes (estilo Fixa)");
        case IslandPref::MinimizeButton:
            return Tr(L"The bar doesn't minimize", L"A barra não minimiza");
        case IslandPref::BarCorners:
            return Tr(L"Needs the bar style and the clock in the middle",
                      L"Precisa do estilo barra e do relógio no meio");
        case IslandPref::GlassTranslucent:
            return needs(IslandPref::LiquidGlass);
        case IslandPref::ClockSeconds:
            return needs(IslandPref::Clock);
        case IslandPref::CountdownEvents:
        case IslandPref::CountdownAlarms:
            return needs(IslandPref::Countdown);
        case IslandPref::FocusedTitle:
            return needs(IslandPref::FocusedApp);
        case IslandPref::WeatherCards:
            return needs(IslandPref::Weather);
        case IslandPref::DndSilent:
            return needs(IslandPref::OwnNotifications);
        case IslandPref::DownloadStalled:
            return needs(IslandPref::DownloadFill);
        case IslandPref::FolderThumbnails:
            return needs(IslandPref::Folders);
        case IslandPref::TimerLine:
            return needs(IslandPref::Timer);
        case IslandPref::GlassLight:
            return needs(IslandPref::LiquidGlass);
        case IslandPref::AiClaude:
        case IslandPref::AiCodex:
            return needs(IslandPref::AiUsage);
        case IslandPref::CountdownTimer:
        case IslandPref::CountdownPomodoro:
            return !GetIslandPref(IslandPref::Countdown)
                       ? needs(IslandPref::Countdown)
                       : offWhile(IslandPref::Timer);
        case IslandPref::VolumePercent:
            return needs(IslandPref::Volume);
        case IslandPref::BatteryPercent:
            return needs(IslandPref::Battery);
        case IslandPref::TrayOpen:
            return !GetIslandPref(IslandPref::Tray)
                       ? needs(IslandPref::Tray)
                       : Tr(L"Only with the icons opening in the pill",
                            L"Só com os ícones abrindo na pílula");
        case IslandPref::Frosted:
            if (FrostedNeedsOnlyTransparency()) {
                return Tr(
                    L"Needs Windows' transparency effects: click to "
                    L"open Personalization > Colors",
                    L"Precisa dos efeitos de transparência do Windows: "
                    L"clique para abrir Personalização > Cores");
            }
            return Tr(
                L"Only with the Glass off (it blurs them already), on "
                L"the bar or on the Fixed style without notches and "
                L"not hiding as a line",
                L"Só com o Vidro desligado (ele já desfoca), na barra "
                L"ou no estilo Fixa sem entalhes e sem esconder como "
                L"linha");
        case IslandPref::MonthNumber:
            return needs(IslandPref::ClockDate);
        case IslandPref::BarClear:
            return GetIslandPref(IslandPref::Bar) != 1
                       ? Tr(L"Only for the Bar style", L"Só no estilo Barra")
                       : Tr(L"Not with the Liquid Glass",
                            L"Não com o Liquid Glass");
        case IslandPref::HideClock:
            return !g_settings.islandAutoHide
                       ? needs(IslandPref::AutoHide)
                       : Tr(L"Not with the notches", L"Não com os entalhes");
        case IslandPref::VisualizerSmooth:
            return needs(IslandPref::Visualizer);
        case IslandPref::LyricsInTitle:
            return needs(GetIslandPref(IslandPref::Lyrics)
                             ? IslandPref::MediaTitle
                             : IslandPref::Lyrics);
        case IslandPref::HideWindowsBanners:
            return GetIslandPref(IslandPref::Banner)
                       ? offWhile(IslandPref::OwnNotifications)
                       : needs(IslandPref::Banner);
        case IslandPref::BannerMinimized:
        case IslandPref::WebPictures:
            return needs(IslandPref::Banner);
        default:
            return {};
    }
}

// The window's side bar: the search box, and a row for each tab (its icon
// and name; the one shown lit, with an accent mark at its left, like
// Windows' Settings; only the icons when narrow), scrolling when the window
// is short; the page's title, and the island's preview.
void ControlPanel::DrawSettingsNav(ID2D1RenderTarget* target,
                                   ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    DrawSearchField(target, brush);
    BeginScrollArea(target, m_navViewport, m_navScroll);
    // (The scroll area's coordinates start at the side bar's top.)
    const float top = m_navViewport.top;
    for (size_t i = 0; i < m_tabs.size(); i++) {
        const D2D1_RECT_F tab{m_tabs[i].left, m_tabs[i].top - top,
                              m_tabs[i].right, m_tabs[i].bottom - top};
        const int id = m_tabIds[i];
        const bool chosen = m_search.empty() && id == m_settingsTab;
        const float lit = Hover(HoverKey(4110, id),
                                m_hover.part == Part::SettingsTab &&
                                    m_hover.index == id);
        const float radius = std::round(5 * s);
        if (id == kTabDock) {
            brush->SetColor(Fg(0.08f));
            target->FillRectangle(
                {tab.left + std::round(8 * s), tab.top - std::round(8 * s),
                 tab.right - std::round(8 * s), tab.top - std::round(7 * s)},
                brush);
        }
        if (chosen || lit > 0.01f) {
            brush->SetColor(Fg(chosen ? 0.08f + 0.03f * lit : 0.06f * lit));
            GlassFill(target, {tab, radius, radius}, brush);
        }
        if (chosen) {
            const float middle = (tab.top + tab.bottom) / 2;
            const float mark = std::round(1.5f * s);
            brush->SetColor(Accent(1));
            target->FillRoundedRectangle(
                {{tab.left, middle - std::round(8 * s), tab.left + mark * 2,
                  middle + std::round(8 * s)},
                 mark,
                 mark},
                brush);
        }
        const float icon = std::round(40 * s);
        const float iconLeft =
            m_navNarrow ? std::round((tab.left + tab.right - icon) / 2)
                        : tab.left + std::round(4 * s);
        const WCHAR glyph = SettingTabGlyph(id);
        brush->SetColor(Fg(chosen ? 0.95f : 0.8f));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                  {iconLeft, tab.top, iconLeft + icon, tab.bottom},
                  brush);
        if (!m_navNarrow) {
            DrawWrapped(target, brush, SettingTabTitle(id),
                        m_subtitleFormat.get(),
                        {iconLeft + icon, tab.top, tab.right - std::round(6 * s),
                         tab.bottom});
        }
    }
    EndScrollArea(target);
    DrawScrollBar(target, brush, m_navViewport, m_navScroll, 0.6f,
                  Hover(ScrollBarStrip(m_navViewport, kNavBarStrip),
                        m_barHover == 2 || m_barDrag == 2));
    const std::wstring title =
        m_search.empty() ? std::wstring(SettingTabTitle(m_settingsTab))
                         : std::wstring(Tr(L"Search results",
                                           L"Resultados da busca"));
    brush->SetColor(Fg(0.95f));
    target->DrawText(title.c_str(), (UINT32)title.size(),
                     m_pageTitleFormat.get(), m_pageTitle, brush);
    // The island's preview: its items in their order, on a card.
    if (m_previewShown) {
        const D2D1_RECT_F card = CardOf(m_previewRow);
        const float radius = std::round(6 * s);
        brush->SetColor(Fg(0.05f));
        GlassFill(target, {card, radius, radius}, brush);
        brush->SetColor(Fg(0.07f));
        target->DrawRoundedRectangle(
            {{card.left + 0.5f, card.top + 0.5f, card.right - 0.5f,
              card.bottom - 0.5f},
             radius,
             radius},
            brush, 1);
        DrawOrderStrip(target, brush, m_previewRow);
    }
}

// The option a dimmed one waits for (see DisabledReason), to go to it from
// its reason; Label: nothing to go to.
ControlPanel::SettingTarget ControlPanel::DisabledTarget(int index) {
    auto pref = [](IslandPref option) {
        return SettingTarget{SettingKind::Pref, (int)option};
    };
    const SettingTarget none{SettingKind::Label, -1};
    const SettingTarget style{SettingKind::Style, 0};
    const SettingTarget notches{SettingKind::Choice, kChoiceNotches};
    const SettingTarget trayStyle{SettingKind::Choice, kChoiceTrayStyle};
    if (index == kDisabledShortcut) {
        return OtherIslandShown() ? none : pref(IslandPref::MinimizeKey);
    }
    if (index == kDisabledAppPlaces) {
        return !g_settings.islandBar && !g_settings.islandReserve ? notches
                                                                  : style;
    }
    if (index >= kDisabledChoice && index < kDisabledAppPlaces) {
        switch (index - kDisabledChoice) {
            case kChoiceAnimationLevel:
                return pref(IslandPref::Animations);
            case kChoiceDateOrder:
                return pref(IslandPref::ClockDate);
            case kChoiceWeatherUnit:
                return pref(IslandPref::Weather);
            case kChoiceIconColors:
            case kChoiceIconFolder:
                return {SettingKind::Choice, kChoiceIconPack};
            case kChoiceTrayStyle:
                return pref(IslandPref::Tray);
            case kChoiceWallpaper:
                return pref(IslandPref::Wallpaper);
            case kChoiceNotches:
                return style;
            case kChoiceNotchOpen:
                return g_settings.islandBar ? style : notches;
            case kChoiceHideWhen:
                return !g_settings.islandAutoHide ? pref(IslandPref::AutoHide)
                       : g_settings.islandBar || g_settings.islandReserve
                           ? style
                           : notches;
            case kChoiceLineFade:
            case kChoiceBarSide:
            case kChoiceBarInnerCorners:
                return style;
            case kChoiceBannerSize:
            case kChoiceBannerWidth:
            case kChoiceBannerCorners:
            case kChoiceBannerPlace:
                return pref(IslandPref::Banner);
            case kChoiceShelfLimit:
                return pref(IslandPref::Shelf);
            case kChoiceEyeTheme:
            case kChoiceEyeShape:
                return pref(IslandPref::Eyes);
            case kChoiceFolderDrag:
                return pref(IslandPref::Folders);
            case kChoiceFillColor:
            case kChoiceFillCopyColor:
                return pref(IslandPref::DownloadFill);
            case kChoiceFillChargeColor:
                return pref(IslandPref::ChargeFill);
            case kChoiceFillStyle:
                return pref(IslandPref::DownloadFill);
            case kChoiceLyricsSize:
            case kChoiceLyricsColor:
            case kChoiceLyricsWidth:
                return pref(IslandPref::Lyrics);
            case kChoiceVisualizerColor:
                return pref(IslandPref::Visualizer);
            case kChoiceRecycleMinSize:
                return pref(IslandPref::RecycleBin);
            default:
                return none;
        }
    }
    if (index >= kDisabledSlider) {
        switch (index - kDisabledSlider) {
            case 1:
            case 13:
                return g_settings.islandBar ? style : notches;
            case 2:
                return !g_animationChoice.on
                           ? pref(IslandPref::Animations)
                           : SettingTarget{SettingKind::Choice,
                                           kChoiceAnimationLevel};
            case 5:
                return pref(IslandPref::AutoHide);
            case 6:
                return notches;
            case 8:
            case 9:
            case 12:
                return style;
            case 11:
                return pref(IslandPref::MediaTitle);
            case 16:
                return pref(IslandPref::LiquidGlass);
            case 17:
                return trayStyle;
            case 18:
                return pref(IslandPref::Visualizer);
            default:
                return none;
        }
    }
    if (OtherIslandShown() && MainIslandOnly((IslandPref)index)) {
        return none;
    }
    switch ((IslandPref)index) {
        case IslandPref::KomorebiHideEmpty:
            return pref(IslandPref::KomorebiLayout);
        case IslandPref::FollowMouse:
            return GetIslandPref(IslandPref::AllMonitors)
                       ? SettingTarget{SettingKind::Monitors, 0}
                       : notches;
        case IslandPref::FreeDrag:
        case IslandPref::VerticalSides:
        case IslandPref::NotchEach:
            return notches;
        case IslandPref::MinimizeButton:
        case IslandPref::BarCorners:
            return style;
        case IslandPref::GlassTranslucent:
            return pref(IslandPref::LiquidGlass);
        case IslandPref::ClockSeconds:
            return pref(IslandPref::Clock);
        case IslandPref::CountdownEvents:
        case IslandPref::CountdownAlarms:
            return pref(IslandPref::Countdown);
        case IslandPref::FocusedTitle:
            return pref(IslandPref::FocusedApp);
        case IslandPref::WeatherCards:
            return pref(IslandPref::Weather);
        case IslandPref::DndSilent:
            return pref(IslandPref::OwnNotifications);
        case IslandPref::DownloadStalled:
            return pref(IslandPref::DownloadFill);
        case IslandPref::FolderThumbnails:
            return pref(IslandPref::Folders);
        case IslandPref::TimerLine:
            return pref(IslandPref::Timer);
        case IslandPref::GlassLight:
            return pref(IslandPref::LiquidGlass);
        case IslandPref::AiClaude:
        case IslandPref::AiCodex:
            return pref(IslandPref::AiUsage);
        case IslandPref::CountdownTimer:
        case IslandPref::CountdownPomodoro:
            return pref(GetIslandPref(IslandPref::Countdown)
                            ? IslandPref::Timer
                            : IslandPref::Countdown);
        case IslandPref::VolumePercent:
            return pref(IslandPref::Volume);
        case IslandPref::BatteryPercent:
            return pref(IslandPref::Battery);
        case IslandPref::TrayOpen:
            return !GetIslandPref(IslandPref::Tray) ? pref(IslandPref::Tray)
                                                    : trayStyle;
        case IslandPref::Frosted:
            return g_settings.islandLiquidGlass ? pref(IslandPref::LiquidGlass)
                                                : style;
        case IslandPref::MonthNumber:
            return pref(IslandPref::ClockDate);
        case IslandPref::BarClear:
            return GetIslandPref(IslandPref::Bar) != 1
                       ? style
                       : pref(IslandPref::LiquidGlass);
        case IslandPref::HideClock:
            return !g_settings.islandAutoHide ? pref(IslandPref::AutoHide)
                                              : notches;
        case IslandPref::VisualizerSmooth:
            return pref(IslandPref::Visualizer);
        case IslandPref::LyricsInTitle:
            return pref(GetIslandPref(IslandPref::Lyrics)
                            ? IslandPref::MediaTitle
                            : IslandPref::Lyrics);
        case IslandPref::HideWindowsBanners:
            return GetIslandPref(IslandPref::Banner)
                       ? pref(IslandPref::OwnNotifications)
                       : pref(IslandPref::Banner);
        case IslandPref::BannerMinimized:
        case IslandPref::WebPictures:
            return pref(IslandPref::Banner);
        default:
            return none;
    }
}

// An entry's index for DisabledReason while it's dimmed, or -1.
int ControlPanel::DisabledIndex(const SettingEntry& entry) {
    switch (entry.kind) {
        case SettingKind::Pref:
            return PrefDisabled((IslandPref)entry.value) ? entry.value : -1;
        case SettingKind::Choice:
            return ChoiceDisabled(entry.value) ? kDisabledChoice + entry.value
                                               : -1;
        case SettingKind::Size:
            return SliderDisabled(entry.value) ? kDisabledSlider + entry.value
                                               : -1;
        case SettingKind::AppPlaces:
            return PerAppPlacesOn() ? -1 : kDisabledAppPlaces;
        case SettingKind::LyricsApps:
            // (Waits for the lyrics, as their size does.)
            return ChoiceDisabled(kChoiceLyricsSize)
                       ? kDisabledChoice + kChoiceLyricsSize
                       : -1;
        case SettingKind::Shortcut:
            return ShortcutDisabled() ? kDisabledShortcut : -1;
        default:
            return -1;
    }
}

// A dimmed option's reason, written under it (saying a click shows what to
// change, see OpenNeeds); empty when it isn't dimmed.
std::wstring ControlPanel::SettingReason(const SettingEntry& entry) {
    const int index = DisabledIndex(entry);
    if (index < 0) {
        return {};
    }
    std::wstring reason = DisabledReason(index);
    if (!reason.empty() && ReasonLinks(index)) {
        reason += std::wstring(L"  › ") +
                  Tr(L"what to change", L"o que mudar");
    }
    return reason;
}

bool ControlPanel::ReasonLinks(int index) {
    return DisabledTarget(index).kind != SettingKind::Label ||
           (index == (int)IslandPref::Frosted && FrostedNeedsOnlyTransparency());
}

// Which options are dimmed now (their reasons change the rows' heights).
std::string ControlPanel::DisabledKey() const {
    std::string key;
    key.reserve(m_settings.size());
    for (const auto& entry : m_settings) {
        key += DisabledIndex(entry) >= 0 ? '1' : '0';
    }
    return key;
}

// The change that takes away one reason an entry is dimmed (its
// DisabledReason index): an option turned on or off; the style or the
// notches set to the first value that takes that reason away (tried in
// memory). False when it's something else (a choice, Windows' own setting).
bool ControlPanel::NeedOf(const SettingEntry& entry,
                          int disabled,
                          NeedChange* change) {
    const SettingTarget target = DisabledTarget(disabled);
    if (target.kind == SettingKind::Pref) {
        const auto pref = (IslandPref)target.value;
        const int from = GetIslandPref(pref);
        *change = {pref, from, from ? 0 : 1};
        return true;
    }
    IslandPref pref;
    std::vector<int> values;
    if (target.kind == SettingKind::Style) {
        // (The Island style first, then Fixed, then the bar.)
        pref = IslandPref::Bar;
        values = {0, 2, 1};
    } else if (target.kind == SettingKind::Choice &&
               target.value == kChoiceNotches) {
        pref = g_settings.islandReserve ? IslandPref::Notches
                                        : IslandPref::PillNotches;
        values = {1, 2, 3, 0};
    } else {
        return false;
    }
    const int from = GetIslandPref(pref);
    for (int value : values) {
        if (value == from) {
            continue;
        }
        SetIslandPrefValue(pref, value);
        const bool helps = DisabledIndex(entry) != disabled;
        SetIslandPrefValue(pref, from);
        if (helps) {
            *change = {pref, from, value};
            return true;
        }
    }
    return false;
}

// Everything an entry needs, found by making each change in memory (then
// putting every option back as it was): `complete` when that's all of it.
std::vector<ControlPanel::NeedChange> ControlPanel::NeedsFor(int index,
                                                             bool* complete) {
    std::vector<NeedChange> needs;
    *complete = false;
    if (index < 0 || index >= (int)m_settings.size()) {
        return needs;
    }
    const SettingEntry entry = m_settings[index];
    const auto saved = g_settings;
    for (int step = 0; step < 8; step++) {
        const int disabled = DisabledIndex(entry);
        if (disabled < 0) {
            *complete = true;
            break;
        }
        NeedChange change;
        if (!NeedOf(entry, disabled, &change) ||
            std::any_of(needs.begin(), needs.end(), [&](const NeedChange& n) {
                return n.pref == change.pref;
            })) {
            break;
        }
        SetIslandPrefValue(change.pref, change.to);
        // (One kept for all islands isn't set in memory: listed, the rest
        // found once it's made.)
        needs.push_back(change);
        if (GetIslandPref(change.pref) != change.to) {
            break;
        }
    }
    // (Each put back, the last first: some options (the banners') aren't in
    // g_settings, which is put back whole.)
    for (auto it = needs.rbegin(); it != needs.rend(); ++it) {
        SetIslandPrefValue(it->pref, it->from);
    }
    g_settings = saved;
    return needs;
}

void ControlPanel::OpenNeeds(int index) {
    m_needs = NeedsFor(index, &m_needsComplete);
    m_needsEntry = m_needs.empty() ? -1 : index;
    StartIslandAnimation();
}

// "Yes, change": each change made for real, found again after each one
// (some show only then), then the option itself turned on; all of them
// kept for "Restore what changed".
void ControlPanel::ApplyNeeds() {
    const int index = m_needsEntry;
    m_needsEntry = -1;
    if (index < 0 || index >= (int)m_settings.size()) {
        return;
    }
    const SettingEntry entry = m_settings[index];
    std::vector<NeedChange> made;
    for (int step = 0; step < 8; step++) {
        const int disabled = DisabledIndex(entry);
        NeedChange change;
        if (disabled < 0 || !NeedOf(entry, disabled, &change) ||
            std::any_of(made.begin(), made.end(), [&](const NeedChange& n) {
                return n.pref == change.pref;
            })) {
            break;
        }
        SetIslandPref(change.pref, change.to);
        made.push_back(change);
    }
    if (entry.kind == SettingKind::Pref && DisabledIndex(entry) < 0 &&
        !GetIslandPref((IslandPref)entry.value)) {
        SetIslandPref((IslandPref)entry.value, 1);
        made.push_back({(IslandPref)entry.value, 0, 1});
    }
    if (!made.empty()) {
        m_restore = std::move(made);
        m_restoreEntry = index;
    }
    StartIslandAnimation();
}

// Each change put back, the last one first (one changed again since, as
// chosen elsewhere, is left as it is).
void ControlPanel::RestoreNeeds() {
    for (auto it = m_restore.rbegin(); it != m_restore.rend(); ++it) {
        if (GetIslandPref(it->pref) == it->to) {
            SetIslandPref(it->pref, it->from);
        }
    }
    m_restore.clear();
    m_restoreEntry = -1;
    StartIslandAnimation();
}

// "Name: from → to", and its tab when it isn't this one.
std::wstring ControlPanel::NeedText(const NeedChange& change) const {
    auto value = [&](int v) -> std::wstring {
        if (change.pref == IslandPref::Bar) {
            return v == 1   ? Tr(L"Bar", L"Barra")
                   : v == 2 ? Tr(L"Fixed", L"Fixa")
                            : Tr(L"Island", L"Ilha");
        }
        if (change.pref == IslandPref::Notches ||
            change.pref == IslandPref::PillNotches) {
            return ChoiceOption(kChoiceNotches, v);
        }
        return v ? Tr(L"on", L"ligado") : Tr(L"off", L"desligado");
    };
    const IslandPrefInfo& info = kIslandPrefs[(int)change.pref];
    std::wstring name =
        change.pref == IslandPref::Bar ? std::wstring(Tr(L"Style", L"Estilo"))
        : change.pref == IslandPref::Notches ||
                change.pref == IslandPref::PillNotches
            ? std::wstring(Tr(L"Notches", L"Entalhes"))
            : std::wstring(Tr(info.english, info.portuguese));
    std::wstring text = L"• " + name + L": " + value(change.from) +
                        L" → " + value(change.to);
    // (Where it is, in another tab.)
    const SettingKind kind = change.pref == IslandPref::Bar
                                 ? SettingKind::Style
                                 : SettingKind::Pref;
    for (const auto& entry : m_settings) {
        if (entry.kind == kind &&
            (kind == SettingKind::Style || entry.value == (int)change.pref)) {
            const int tab = SettingSectionTab(entry.section);
            if (tab != m_settingsTab) {
                text += std::wstring(L"  (") + Tr(L"tab", L"aba") + L" " +
                        SettingTabTitle(tab) + L")";
            }
            break;
        }
    }
    return text;
}

// The bubble under the option's card, over the ones after it; and on a
// card whose needs were made, "Restore what changed".
void ControlPanel::DrawNeeds(ID2D1RenderTarget* target,
                             ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    m_restoreLink = {};
    if (m_restoreEntry >= 0 && m_restoreEntry < (int)m_settings.size() &&
        m_settings[m_restoreEntry].shown) {
        const D2D1_RECT_F card = CardOf(m_settings[m_restoreEntry].rect);
        PCWSTR restore = Tr(L"Restore what changed", L"Restaurar o que mudou");
        m_restoreLink = {card.right - std::round(190 * s), card.top + 2,
                         card.right - std::round(10 * s),
                         card.top + std::round(24 * s)};
        brush->SetColor(Accent(m_hover.part == Part::RestoreNeeds ? 1 : 0.8f));
        target->DrawText(restore, (UINT32)wcslen(restore), m_smallFormat.get(),
                         m_restoreLink, brush);
    }
    m_needsRect = m_needsYes = m_needsNo = {};
    if (m_needsEntry < 0 || m_needsEntry >= (int)m_settings.size() ||
        !m_settings[m_needsEntry].shown) {
        return;
    }
    const SettingEntry& entry = m_settings[m_needsEntry];
    const D2D1_RECT_F card = CardOf(entry.rect);
    const float pad = std::round(14 * s);
    const float left = card.left + std::round(16 * s);
    // Its lines: what's asked, each change, and what may come after.
    std::wstring name;
    if (entry.kind == SettingKind::Pref) {
        const IslandPrefInfo& info = kIslandPrefs[entry.value];
        name = Tr(info.english, info.portuguese);
    }
    std::wstring title = Tr(L"To use “%s”, change:", L"Para usar “%s”, mude:");
    if (const size_t at = title.find(L"%s"); at != std::wstring::npos) {
        title.replace(at, 2, name.empty() ? Tr(L"this", L"isto") : name);
    }
    std::vector<std::wstring> lines{title};
    for (const auto& change : m_needs) {
        lines.push_back(NeedText(change));
    }
    if (!m_needsComplete) {
        lines.push_back(Tr(L"…and then what else it says it needs",
                           L"…e depois o que mais ela disser que precisa"));
    }
    // (How to go back, under them.)
    lines.push_back(Tr(L"“Restore what changed” on this option undoes it later",
                       L"“Restaurar o que mudou”, nesta opção, desfaz depois"));
    // As wide as its longest line (no wider than the card), at least as
    // its buttons.
    float widest = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        IDWriteTextFormat* format =
            i == 0 ? m_subtitleFormat.get() : m_smallFormat.get();
        winrt::com_ptr<IDWriteTextLayout> layout;
        DWRITE_TEXT_METRICS metrics{};
        if (m_dwrite && format &&
            SUCCEEDED(m_dwrite->CreateTextLayout(
                lines[i].c_str(), (UINT32)lines[i].size(), format, 4000,
                100, layout.put())) &&
            SUCCEEDED(layout->GetMetrics(&metrics))) {
            widest = std::max(widest, metrics.widthIncludingTrailingWhitespace);
        }
    }
    const float most = std::min(card.right - card.left - std::round(32 * s),
                                std::round(560 * s));
    const float least = std::round((150 + 8 + 110) * s) + pad * 2;
    const float width = std::clamp(std::ceil(widest) + pad * 2 + std::round(4 * s),
                                   std::min(least, most), most);
    const float textWidth = width - pad * 2;
    std::vector<float> heights;
    float textHeight = 0;
    for (const auto& line : lines) {
        heights.push_back(std::max(std::round(20 * s),
                                   WrappedHeight(line, m_smallFormat.get(),
                                                 textWidth)));
        textHeight += heights.back();
    }
    const float button = std::round(32 * s);
    const float top = card.bottom - std::round(6 * s);
    m_needsRect = {left, top, left + width,
                   top + pad * 2 + textHeight + std::round(10 * s) + button};
    const float radius = std::round(12 * s);
    brush->SetColor(BgSolid(1));
    target->FillRoundedRectangle({m_needsRect, radius, radius}, brush);
    brush->SetColor(Accent(0.9f));
    target->DrawRoundedRectangle({m_needsRect, radius, radius}, brush,
                                 std::max(1.0f, 1.5f * s));
    float y = top + pad;
    for (size_t i = 0; i < lines.size(); i++) {
        brush->SetColor(Fg(i == 0 ? 0.95f : i + 1 == lines.size() ? 0.5f : 0.8f));
        DrawWrapped(target, brush, lines[i],
                    i == 0 ? m_subtitleFormat.get() : m_smallFormat.get(),
                    {left + pad, y, left + pad + textWidth, y + heights[i]});
        y += heights[i];
    }
    const float buttonsTop = y + std::round(10 * s);
    const float half = std::round(150 * s);
    m_needsYes = {left + pad, buttonsTop, left + pad + half,
                  buttonsTop + button};
    m_needsNo = {m_needsYes.right + std::round(8 * s), buttonsTop,
                 m_needsYes.right + std::round(8 * s) + std::round(110 * s),
                 buttonsTop + button};
    DrawButton(target, brush, m_needsYes, Tr(L"Yes, change", L"Sim, alterar"),
               true, m_hover.part == Part::NeedsYes);
    DrawButton(target, brush, m_needsNo, Tr(L"Cancel", L"Cancelar"), false,
               m_hover.part == Part::NeedsNo);
}

void ControlPanel::DrawButton(ID2D1RenderTarget* target,
                              ID2D1SolidColorBrush* brush,
                              const D2D1_RECT_F& rect,
                              PCWSTR text,
                              bool main,
                              bool hover) {
    const float round = (rect.bottom - rect.top) / 2;
    const float lit = Hover(rect, hover);
    brush->SetColor(main ? Accent(0.85f + 0.15f * lit)
                         : Fg(0.1f + 0.08f * lit));
    GlassFill(target, {rect, round, round}, brush);
    brush->SetColor(main ? AccentInk(1) : Fg(0.9f));
    target->DrawText(text, (UINT32)wcslen(text), m_linkFormat.get(), rect,
                     brush);
}

// Shows an option: its tab, scrolled to it, its card glowing a moment.
void ControlPanel::GoToSetting(const SettingTarget& target) {
    for (size_t i = 0; i < m_settings.size(); i++) {
        const SettingEntry& entry = m_settings[i];
        if (entry.kind != target.kind || entry.value != target.value) {
            continue;
        }
        m_settingsTab = SettingSectionTab(entry.section);
        m_search.clear();
        m_searchEdit.Reset();
        m_settingsScroll.Reset();
        OnSearchChanged();
        SettleSettings();
        m_tabFade = 0;
        m_tabFadeVelocity = 0;
        const double top = m_settings[i].rect.top - m_cardPad -
                           std::round(24 * m_scale);
        m_settingsScroll.target =
            std::clamp(top, 0.0, (double)m_settingsScroll.Max());
        m_flashEntry = (int)i;
        m_flashAt = NowSeconds();
        StartIslandAnimation();
        return;
    }
}

// What a step of the setup assistant shows: its text, its own choice, and
// a few options of other tabs as they are there.
bool ControlPanel::SetupShows(int step, const SettingEntry& entry) {
    struct Shown {
        int step;
        SettingKind kind;
        int value;
    };
    using Kind = SettingKind;
    static constexpr Shown kShown[] = {
        {kSetupStyle, Kind::Choice, kChoiceSetupStyle},
        {kSetupPill, Kind::Choice, kChoiceSetupItems},
        {kSetupHiding, Kind::Choice, kChoiceSetupHide},
        {kSetupHiding, Kind::Pref, (int)IslandPref::ShortcutNotifications},
        {kSetupHiding, Kind::Pref, (int)IslandPref::ShortcutControl},
        {kSetupHiding, Kind::Pref, (int)IslandPref::Banner},
        {kSetupHiding, Kind::Pref, (int)IslandPref::HideWindowsBanners},
        {kSetupLook, Kind::Choice, kChoiceSetupTheme},
        {kSetupLook, Kind::Pref, (int)IslandPref::WindowColor},
        {kSetupLook, Kind::Pref, (int)IslandPref::LiquidGlass},
        {kSetupLook, Kind::Dock, 0},
    };
    return entry.kind == SettingKind::Setup ||
           std::any_of(std::begin(kShown), std::end(kShown),
                       [&](const Shown& shown) {
                           return shown.step == step &&
                                  shown.kind == entry.kind &&
                                  shown.value == entry.value;
                       });
}

// The setup assistant's text for its step: what it's about, a line each,
// the last one where to find more; on the last step, what was chosen, each
// line a link to its option.
std::vector<ControlPanel::NoteLine> ControlPanel::SetupLines() const {
    std::vector<NoteLine> lines;
    auto add = [&](PCWSTR english, PCWSTR portuguese) {
        lines.push_back({Tr(english, portuguese), false, {}});
    };
    auto more = [&](std::initializer_list<int> tabs) {
        std::wstring names;
        for (int tab : tabs) {
            names += (names.empty() ? L"" : L", ") +
                     std::wstring(SettingTabTitle(tab));
        }
        std::wstring text = Tr(L"More options in: %s", L"Mais opções em: %s");
        if (const size_t at = text.find(L"%s"); at != std::wstring::npos) {
            text.replace(at, 2, names);
        }
        lines.push_back({text, false, {}});
    };
    switch (m_setup.step) {
        case kSetupWelcome:
            add(L"The island is the pill at the top of the screen: the clock, "
                L"the control center, your notifications and what's playing",
                L"A ilha é a pílula no topo da tela: o relógio, a central de "
                L"controle, suas notificações e o que está tocando");
            add(L"A click on an item opens it; when you don't use it, the "
                L"pill can shrink into a thin line",
                L"Um clique num item abre ele; quando você não a usa, a "
                L"pílula pode encolher numa linha fina");
            add(L"A right click on the island shows a gear: it opens these "
                L"settings",
                L"Um clique com o botão direito na ilha mostra uma "
                L"engrenagem: ela abre estas configurações");
            add(L"The dock is optional: it takes the taskbar's place, and "
                L"it's chosen in Windhawk",
                L"A dock é opcional: ela fica no lugar da barra de tarefas, e "
                L"é escolhida no Windhawk");
            add(L"This assistant sets the main choices in a few steps, and "
                L"each one changes the island at once; everything else is in "
                L"the tabs on the left",
                L"Este assistente define as escolhas principais em poucos "
                L"passos, e cada uma muda a ilha na hora; todo o resto fica "
                L"nas abas à esquerda");
            break;
        case kSetupStyle:
            add(L"Island: a pill floating at the top, the windows go under it",
                L"Ilha: uma pílula flutuando no topo, as janelas passam por "
                L"baixo dela");
            add(L"Fixed: joined to the top edge, keeping its room (the "
                L"windows stay below it)",
                L"Fixa: presa na borda de cima, guardando o espaço dela (as "
                L"janelas ficam abaixo)");
            add(L"Bar: along the whole top edge, like macOS' menu bar",
                L"Barra: ao longo de toda a borda de cima, como a barra de "
                L"menus do macOS");
            more({kTabStyle, kTabPlace});
            break;
        case kSetupPill:
            add(L"Essential: the clock, the battery, the control center, the "
                L"volume, the notifications' bell and what's playing",
                L"Essencial: o relógio, a bateria, a central de controle, o "
                L"volume, o sino das notificações e o que está tocando");
            add(L"Everyday: also the next event, the weather (goes online), "
                L"the timer, your folders, the apps with notifications and "
                L"the song's cover",
                L"Dia a dia: também o próximo evento, o clima (acessa a "
                L"internet), o timer, suas pastas, os apps com notificações e "
                L"a capa da música");
            add(L"Everything: most of the items, from notes and tasks to the "
                L"network's speed",
                L"Tudo: quase todos os itens, de anotações e tarefas à "
                L"velocidade da rede");
            add(L"Keep as it is: the items the pill had before",
                L"Manter como está: os itens que a pílula tinha antes");
            more({kTabSystem, kTabDesktop, kTabTools});
            break;
        case kSetupHiding:
            add(L"The pill can stay, or become a thin line while the mouse is "
                L"away or while a window is maximized; the mouse on the line "
                L"brings it back",
                L"A pílula pode ficar, ou virar uma linha fina enquanto o "
                L"mouse está longe ou enquanto uma janela está maximizada; o "
                L"mouse na linha a traz de volta");
            add(L"Banners show each new notification coming out of the pill",
                L"Os avisos mostram cada notificação nova saindo da pílula");
            add(L"Win+N and Win+A can open the island's notifications and "
                L"control center instead of Windows'",
                L"Win+N e Win+A podem abrir as notificações e a central de "
                L"controle da ilha no lugar das do Windows");
            more({kTabHide, kTabNotifications, kTabBanners});
            break;
        case kSetupLook:
            add(L"Colors: like Windows (light or dark with it), always light "
                L"or always dark, or the colors of the app in front",
                L"Cores: como o Windows (claro ou escuro junto com ele), "
                L"sempre claro ou sempre escuro, ou as cores do app da "
                L"frente");
            add(L"Glass: see-through and blurred, like on macOS",
                L"Vidro: transparente e desfocado, como no macOS");
            add(L"The dock is chosen in Windhawk, in this mod's settings: "
                L"Taskbar (the dock, the Windows taskbar transparent or with "
                L"only the apps, or unchanged); the island can't change it",
                L"A dock é escolhida no Windhawk, nas configurações deste "
                L"mod: Barra de tarefas (a dock, a barra do Windows "
                L"transparente ou só com os apps, ou sem mudanças); a ilha "
                L"não consegue mudar isso");
            more({kTabLook, kTabIcons});
            break;
        default: {
            auto chosen = [&](const std::wstring& name,
                              const std::wstring& value,
                              SettingTarget target) {
                lines.push_back({name + L": " + value, false, target});
            };
            auto onOff = [](IslandPref pref) {
                return GetIslandPref(pref) ? Tr(L"on", L"ligado")
                                           : Tr(L"off", L"desligado");
            };
            auto pref = [&](IslandPref option) {
                const IslandPrefInfo& info = kIslandPrefs[(int)option];
                chosen(Tr(info.english, info.portuguese), onOff(option),
                       {SettingKind::Pref, (int)option});
            };
            chosen(Tr(L"Style", L"Estilo"),
                   ChoiceOption(kChoiceSetupStyle,
                                ChoiceValue(kChoiceSetupStyle)),
                   {SettingKind::Style, 0});
            chosen(Tr(L"Items on the pill", L"Itens na pílula"),
                   ChoiceOption(kChoiceSetupItems,
                                ChoiceValue(kChoiceSetupItems)),
                   {SettingKind::Pref, (int)IslandPref::Network});
            const int hide = ChoiceValue(kChoiceSetupHide);
            chosen(SettingTabTitle(kTabHide),
                   ChoiceDisabled(kChoiceSetupHide)
                       ? DisabledReason(kDisabledChoice + kChoiceSetupHide)
                   : hide >= 0
                       ? ChoiceOption(kChoiceSetupHide, hide)
                       : ChoiceOption(kChoiceHideWhen,
                                      g_settings.islandHideWhen),
                   {SettingKind::Pref, (int)IslandPref::AutoHide});
            pref(IslandPref::Banner);
            pref(IslandPref::HideWindowsBanners);
            pref(IslandPref::ShortcutNotifications);
            pref(IslandPref::ShortcutControl);
            chosen(Tr(L"Colors", L"Cores"), ThemeName(g_themeIndex),
                   {SettingKind::Theme, 0});
            pref(IslandPref::WindowColor);
            chosen(Tr(L"Glass", L"Vidro"), onOff(IslandPref::LiquidGlass),
                   {SettingKind::Pref, (int)IslandPref::LiquidGlass});
            add(L"Finish keeps all of it; Cancel puts back what was there "
                L"before",
                L"Concluir mantém tudo isso; Cancelar volta o que havia "
                L"antes");
            break;
        }
    }
    return lines;
}

// A step of the setup assistant, shown like another tab: its options in
// place at once, fading in together.
void ControlPanel::ShowSetupStep(int step) {
    m_setup.step = std::clamp(step, 0, kSetupSteps - 1);
    m_settingsScroll.Reset();
    OnSearchChanged();
    SettleSettings();
    m_tabFade = 0;
    m_tabFadeVelocity = 0;
}

// A set of the pill's items (see kSetupItems), or "Keep as it is": the
// items as the assistant found them.
void ControlPanel::ChooseSetupItems(int set) {
    for (size_t i = 0; i < std::size(kSetupItems); i++) {
        const IslandPref pref = kSetupItems[i];
        const int value = set < (int)std::size(kSetupItemSets)
                              ? (int)(i < kSetupItemSets[set])
                              : m_setup.from[(int)pref];
        if (GetIslandPref(pref) != value) {
            SetIslandPref(pref, value);
        }
    }
}

// The setup assistant's buttons (Enter is its next one, Escape its
// Cancel): a step back or on; Finish keeps what was chosen, Skip and Cancel
// put it back, both then on the first tab.
void ControlPanel::PressSetupButton(int button) {
    if (button == kSetupBack && m_setup.step > kSetupWelcome) {
        ShowSetupStep(m_setup.step - 1);
        return;
    }
    if (button == kSetupNext && m_setup.step < kSetupDone) {
        ShowSetupStep(m_setup.step + 1);
        return;
    }
    if (button != kSetupBack) {
        EndSetup(button == kSetupCancel);
        GoToTab(kTabGeneral);
    }
}

// The setup assistant ended: with `cancel`, every option it found otherwise
// is put back, the last first (not into another profile that took over
// meanwhile, by its app or by hand). Either way, it's done on this account:
// it doesn't open by itself again.
void ControlPanel::EndSetup(bool cancel) {
    if (cancel && m_setup.profile == g_activeProfile) {
        for (size_t i = m_setup.from.size(); i-- > 0;) {
            if (GetIslandPref((IslandPref)i) != m_setup.from[i]) {
                SetIslandPref((IslandPref)i, m_setup.from[i]);
            }
        }
        StartIslandAnimation();
    }
    m_setup.from.clear();
    SetUserStringValue(L"setupDone", L"" WH_MOD_VERSION);
}

// The search box: a magnifier, what's typed (or a hint) with the caret, and
// an "x" to clear it. Typing anywhere in the settings goes there.
void ControlPanel::DrawSearchField(ID2D1RenderTarget* target,
                                   ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    const D2D1_RECT_F& field = m_searchField;
    const float radius = (field.bottom - field.top) / 2;
    brush->SetColor(
        Fg(0.08f + 0.03f * Hover(field, m_hover.part == Part::SearchField)));
    GlassFill(target, {field, radius, radius}, brush);
    if (!m_search.empty()) {
        brush->SetColor(Accent(0.9f));
        target->DrawRoundedRectangle(
            {{field.left + 0.75f, field.top + 0.75f, field.right - 0.75f,
              field.bottom - 0.75f},
             radius,
             radius},
            brush, 1.5f);
    }
    const float icon = std::round(30 * s);
    const WCHAR magnifier = 0xE721;
    brush->SetColor(Fg(0.5f));
    DrawGlyph(target, &magnifier, 1, m_smallIconFormat.get(),
              {field.left + std::round(2 * s), field.top,
               field.left + std::round(2 * s) + icon, field.bottom},
              brush);
    const D2D1_RECT_F textRect{
        field.left + icon, field.top,
        m_search.empty() ? field.right - radius : m_searchClear.left,
        field.bottom};
    float caretX = textRect.left;
    if (m_search.empty()) {
        PCWSTR hint = Tr(L"Search", L"Buscar");
        brush->SetColor(Fg(0.4f));
        target->DrawText(hint, (UINT32)wcslen(hint), m_subtitleFormat.get(),
                         textRect, brush);
        m_searchEdit.layout = nullptr;
    } else {
        caretX = DrawLineEdit(target, brush, m_dwrite,
                              m_subtitleFormat.get(), textRect, m_search,
                              m_searchEdit, Fg(0.95f));
        const float circle = std::round(8 * s);
        const D2D1_POINT_2F center{
            (m_searchClear.left + m_searchClear.right) / 2,
            (m_searchClear.top + m_searchClear.bottom) / 2};
        brush->SetColor(Fg(
            0.2f + 0.15f * Hover(m_searchClear, m_hover.part == Part::SearchClear)));
        GlassFillEllipse(target, {center, circle, circle}, brush);
        const WCHAR clear = 0xE894;
        brush->SetColor(Fg(0.95f));
        DrawGlyph(target, &clear, 1, m_badgeFormat.get(),
                  {center.x - circle, center.y - circle,
                   center.x + circle, center.y + circle},
                  brush);
    }
    const float middle = (field.top + field.bottom) / 2;
    brush->SetColor(CaretColor());
    target->FillRectangle({caretX + 1, middle - std::round(7 * s),
                           caretX + 2.5f, middle + std::round(7 * s)},
                          brush);
}

// One entry of the settings, at its place in the list.
void ControlPanel::DrawSettingEntry(ID2D1RenderTarget* target,
                                    ID2D1SolidColorBrush* brush,
                                    const SettingEntry& entry) {
    const float s = m_scale;
    const float icon = std::round(36 * s);
    // The name on its lines: over a selector or a slider (entry.head), or
    // beside its control (up to `right`).
    const std::wstring entryName = SettingName(entry);
    const std::wstring entryDescription = SettingDescription(entry);
    const std::wstring entryReason = SettingReason(entry);
    // Each option on a card of its own (the one gone to glowing a moment).
    if (entry.kind != SettingKind::Label) {
        const D2D1_RECT_F card = CardOf(entry.rect);
        const float radius = std::round(6 * s);
        brush->SetColor(Fg(0.05f));
        GlassFill(target, {card, radius, radius}, brush);
        brush->SetColor(Fg(0.07f));
        target->DrawRoundedRectangle(
            {{card.left + 0.5f, card.top + 0.5f, card.right - 0.5f,
              card.bottom - 0.5f},
             radius,
             radius},
            brush, 1);
        const double since = NowSeconds() - m_flashAt;
        if (m_flashEntry == (int)(&entry - m_settings.data()) &&
            since < kFlashSeconds) {
            brush->SetColor(Accent(1 - (float)(since / kFlashSeconds)));
            target->DrawRoundedRectangle(
                {{card.left + 1, card.top + 1, card.right - 1,
                  card.bottom - 1},
                 radius,
                 radius},
                brush, 2);
        }
    }
    auto label = [&](const D2D1_RECT_F& row, WCHAR glyph, float right,
                     D2D1_COLOR_F color) {
        const D2D1_RECT_F head{
            row.left, row.top, entry.head > 0 ? row.right : right,
            entry.head > 0 ? row.top + entry.head : row.bottom};
        brush->SetColor(color);
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                  {head.left, head.top, head.left + icon, head.bottom},
                  brush);
        const D2D1_RECT_F text{head.left + icon, head.top,
                               head.right - std::round(8 * s), head.bottom};
        if (entryDescription.empty() && entryReason.empty()) {
            DrawWrapped(target, brush, entryName, m_subtitleFormat.get(), text);
            return;
        }
        // The name with what it does under it, and why it's dimmed (a link
        // to what it waits for), together in the middle.
        const float width = text.right - text.left;
        const float gap = std::round(2 * s);
        const float nameHeight =
            WrappedHeight(entryName, m_subtitleFormat.get(), width);
        const float descriptionHeight =
            entryDescription.empty()
                ? 0
                : gap + WrappedHeight(entryDescription, m_smallFormat.get(),
                                      width);
        const float reasonHeight =
            entryReason.empty()
                ? 0
                : gap + WrappedHeight(entryReason, m_smallFormat.get(), width);
        float top = std::round((text.top + text.bottom - nameHeight -
                                descriptionHeight - reasonHeight) /
                               2);
        DrawWrapped(target, brush, entryName, m_subtitleFormat.get(),
                    {text.left, top, text.right, top + nameHeight});
        top += nameHeight;
        if (!entryDescription.empty()) {
            brush->SetColor({color.r, color.g, color.b, color.a * 0.62f});
            DrawWrapped(target, brush, entryDescription, m_smallFormat.get(),
                        {text.left, top + gap, text.right,
                         top + descriptionHeight});
            top += descriptionHeight;
        }
        if (!entryReason.empty()) {
            // (In the accent color when a click goes somewhere, brighter
            // under the mouse; never dimmed with its row.)
            const int disabled = DisabledIndex(entry);
            const bool link = ReasonLinks(disabled);
            const D2D1_RECT_F reason{text.left, top + gap, text.right,
                                     top + reasonHeight};
            const int linkIndex =
                (int)(&entry - m_settings.data()) * kDisabledStride + disabled;
            const float lit =
                link ? Hover(reason, m_hover.part == Part::DisabledLink &&
                                         m_hover.index == linkIndex)
                     : 0.0f;
            brush->SetColor(link ? Accent(0.8f + 0.2f * lit) : Fg(0.6f));
            DrawWrapped(target, brush, entryReason, m_smallFormat.get(),
                        reason);
            if (link) {
                m_reasonLinks.push_back(
                    {(int)(&entry - m_settings.data()), reason});
            }
        }
    };
    switch (entry.kind) {
        case SettingKind::Label: {
            PCWSTR text = Tr(SettingSectionTitle(entry.section, true),
                             SettingSectionTitle(entry.section, false));
            const D2D1_RECT_F& row = entry.rect;
            // (In the window: a group's title, like Windows' Settings.)
            brush->SetColor(Fg(m_windowed ? 0.9f : 0.5f));
            target->DrawText(
                text, (UINT32)wcslen(text),
                m_windowed ? m_sectionFormat.get() : m_smallFormat.get(),
                {row.left + std::round(6 * s), row.top + std::round(6 * s),
                 row.right, row.bottom},
                brush);
            // The profiles' "+".
            if (entry.section == kSectionProfiles &&
                m_profileAdd.right > m_profileAdd.left) {
                const D2D1_POINT_2F center{
                    (m_profileAdd.left + m_profileAdd.right) / 2,
                    (m_profileAdd.top + m_profileAdd.bottom) / 2};
                const float radius =
                    (m_profileAdd.right - m_profileAdd.left) / 2;
                const float lit = Hover(m_profileAdd,
                                        m_hover.part == Part::ProfileAdd);
                brush->SetColor(Fg(0.08f + 0.1f * lit));
                GlassFillEllipse(target, {center, radius, radius}, brush);
                const WCHAR plus = 0xE710;
                brush->SetColor(Fg(0.9f));
                DrawGlyph(target, &plus, 1, m_badgeFormat.get(), m_profileAdd,
                          brush);
            }
            break;
        }
        case SettingKind::Style:
            // The style: the pill, the pill keeping its room or the bar, the
            // chosen one lit (sliding).
            {
                const IslandPrefInfo& info = kIslandPrefs[(int)IslandPref::Bar];
                label(m_styleRow, info.glyph, m_styleRow.right, Fg(0.95f));
                const D2D1_RECT_F track{
                    m_styleSegments[0].left, m_styleSegments[0].top,
                    m_styleSegments[2].right, m_styleSegments[2].bottom};
                const float radius = (track.bottom - track.top) / 2;
                brush->SetColor(Fg(0.08f));
                GlassFill(target, {track, radius, radius}, brush);
                const int chosenSegment =
                    StyleSegmentStyle(GetIslandPref(IslandPref::Bar));
                const float knob = (float)std::clamp(
                    m_styleKnob < 0 ? (double)chosenSegment : m_styleKnob,
                    -0.05, 2.05);
                const float segmentWidth =
                    m_styleSegments[0].right - m_styleSegments[0].left;
                const float inset = std::round(2 * s);
                const D2D1_RECT_F lit{
                    m_styleSegments[0].left + segmentWidth * knob + inset,
                    track.top + inset,
                    m_styleSegments[0].right + segmentWidth * knob - inset,
                    track.bottom - inset};
                const float litRadius = (lit.bottom - lit.top) / 2;
                brush->SetColor(
                    Fg(m_hover.part == Part::StyleSegment ? 0.26f : 0.2f));
                GlassFill(target, {lit, litRadius, litRadius}, brush);
                for (int i = 0; i < 3; i++) {
                    PCWSTR name = i == 0   ? Tr(L"Pill", L"Pílula")
                                  : i == 1 ? Tr(L"Fixed", L"Fixa")
                                           : Tr(L"Bar", L"Barra");
                    const bool chosen = chosenSegment == i;
                    brush->SetColor(Fg(chosen ? 0.95f : 0.55f));
                    target->DrawText(name, (UINT32)wcslen(name),
                                     m_linkFormat.get(), m_styleSegments[i],
                                     brush);
                }
            }
            break;
        case SettingKind::Choice:
            // A few choices, the chosen one lit (sliding), like the style.
            {
                const int id = entry.value;
                const auto& segments = m_choiceSegments[id];
                if (segments.empty()) {
                    break;
                }
                const float opacity = ChoiceDisabled(id) ? 0.4f : 1.0f;
                // Your own colors hard to read: said instead of the name.
                const bool hardToRead =
                    id == kChoiceColors && CustomColorsHardToRead();
                const WCHAR glyph = hardToRead                  ? 0xE7BA
                                    : id == kChoiceAnimationLevel ? 0xE9E9
                                    : id == kChoiceFrameRate    ? 0xE916
                                    : id == kChoiceBackup       ? 0xE8F1
                                    : id == kChoiceColors       ? 0xE790
                                    : id == kChoiceDateOrder    ? 0xE787
                                    : id == kChoiceClockFormat  ? 0xE823
                                    : id == kChoiceWeatherUnit  ? 0xE753
                                    : id == kChoiceIconPack     ? 0xE91B
                                    : id == kChoiceIconColors   ? 0xE790
                                    : id == kChoiceIconFolder   ? 0xE8B7
                                    : id == kChoiceTrayStyle    ? 0xE712
                                    : id == kChoiceWallpaper    ? 0xE8B9
                                    : id == kChoiceNotches      ? 0xECA5
                                    : id == kChoiceNotchOpen    ? 0xE8A7
                                    : id == kChoiceHideWhen     ? 0xE89F
                                    : id == kChoiceLineClick    ? 0xE962
                                    : id == kChoiceLineFade     ? 0xE7B3
                                    : id == kChoiceBarSide      ? 0xE8E4
                                    : id == kChoiceBarInnerCorners ? 0xE71A
                                    : id == kChoiceBannerSize ||
                                              id == kChoiceBannerWidth
                                        ? 0xE8E9
                                    : id == kChoiceBannerCorners ? 0xE71A
                                    : id == kChoiceBannerPlace  ? 0xE81E
                                    : id == kChoiceShelfLimit   ? 0xE7B8
                                    : id == kChoiceEyeTheme ||
                                              id == kChoiceEyeShape
                                        ? 0xE7B3
                                    : id == kChoiceFolderDrag   ? 0xE8C8
                                    : id == kChoiceLanguage ||
                                              id == kChoiceNotesLanguage
                                        ? 0xE774
                                    : id == kChoiceFillColor    ? 0xE896
                                    : id == kChoiceFillCopyColor ? 0xE8C8
                                    : id == kChoiceFillChargeColor ? 0xE83E
                                    : id == kChoiceFillStyle    ? 0xE9E9
                                    : id == kChoiceLyricsSize   ? 0xE8E9
                                    : id == kChoiceLyricsWidth  ? 0xE8E9
                                    : id == kChoiceLyricsColor  ? 0xE790
                                    : id == kChoiceVisualizerColor ? 0xE790
                                    : id == kChoiceRecycleMinSize ? 0xE74D
                                    : id == kChoiceSetupStyle   ? 0xE7C4
                                    : id == kChoiceSetupItems   ? 0xE8FD
                                    : id == kChoiceSetupHide    ? 0xE89F
                                    : id == kChoiceSetupTheme   ? 0xE790
                                                                : 0;
                // (Hard to read: in orange, like a warning.)
                label(m_choiceRow[id], glyph, m_choiceRow[id].right,
                      hardToRead ? D2D1::ColorF(1.0f, 0.62f, 0.04f, opacity)
                                 : Fg(0.95f * opacity));
                if (id == kChoiceBannerPlace || PictureChoice(id)) {
                    DrawChoicePictures(target, brush, id, opacity);
                    break;
                }
                // A fill's colors: buttons side by side, each with a big
                // dot of its color and its whole name, the chosen one ringed;
                // the languages and the icon packs too, in rows, without dots;
                // and any choice whose names took more than one row.
                if (const int kind = FillChoiceKind(id);
                    kind >= 0 || id == kChoiceLanguage ||
                    id == kChoiceNotesLanguage ||
                    id == kChoiceVisualizerColor || id == kChoiceIconPack ||
                    segments.back().top > segments.front().top) {
                    const float between = std::round(6 * s);
                    for (size_t i = 0; i < segments.size(); i++) {
                        const D2D1_RECT_F button{
                            segments[i].left + between / 2, segments[i].top,
                            segments[i].right - between / 2,
                            segments[i].bottom};
                        const float radius = (button.bottom - button.top) / 2;
                        const bool chosen = ChoiceValue(id) == (int)i;
                        const float hover = Hover(
                            button, m_hover.part == Part::ChoiceSegment &&
                                        m_hover.index == id * 16 + (int)i);
                        brush->SetColor(Fg((chosen ? 0.16f : 0.08f + 0.06f * hover) *
                                           opacity));
                        GlassFill(target, {button, radius, radius}, brush);
                        // An icon pack downloading: filled as far as it is,
                        // in the island's download color, a sheen going over
                        // it (like the pill's fill).
                        if (const float progress =
                                id == kChoiceIconPack
                                    ? IconPackProgress((int)i)
                                    : -1.0f;
                            progress >= 0) {
                            const D2D1_COLOR_F fill = RgbColor(
                                FillColorRgb(kFillDownloads), opacity);
                            const float width =
                                std::max(2 * radius,
                                         (button.right - button.left) *
                                             progress);
                            const D2D1_RECT_F done{button.left, button.top,
                                                   button.left + width,
                                                   button.bottom};
                            brush->SetColor({fill.r, fill.g, fill.b,
                                             0.35f * opacity});
                            target->FillRoundedRectangle({done, radius, radius},
                                                         brush);
                            const float band = std::round(60 * s);
                            const float at =
                                done.left - band +
                                (width + band) *
                                    (float)std::fmod(NowSeconds() / 1.4, 1.0);
                            target->PushAxisAlignedClip(
                                done, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                            brush->SetColor({1, 1, 1, 0.18f * opacity});
                            target->FillRectangle(
                                {at, done.top, at + band, done.bottom}, brush);
                            target->PopAxisAlignedClip();
                        }
                        const float chosenAmount =
                            Hover(HoverKey(6800, id * 16 + (int)i), chosen,
                                  chosen ? 1 : 0);
                        if (chosenAmount > 0.01f) {
                            brush->SetColor(Accent(chosenAmount * opacity));
                            target->DrawRoundedRectangle(
                                {{button.left + 1, button.top + 1,
                                  button.right - 1, button.bottom - 1},
                                 radius - 1,
                                 radius - 1},
                                brush, std::max(1.5f, std::round(2 * s)));
                        }
                        float textLeft = button.left + std::round(8 * s);
                        if (kind >= 0 || id == kChoiceVisualizerColor) {
                            const float dot = std::round(7 * s);
                            const D2D1_POINT_2F at{
                                textLeft + dot,
                                (button.top + button.bottom) / 2};
                            // (The rainbow: its hues across the dot.)
                            winrt::com_ptr<ID2D1GradientStopCollection> stops;
                            winrt::com_ptr<ID2D1LinearGradientBrush> rainbow;
                            if (id == kChoiceVisualizerColor &&
                                (int)i == kVisualizerRainbow) {
                                D2D1_GRADIENT_STOP hues[6];
                                for (int h = 0; h < 6; h++) {
                                    hues[h] = {h / 5.0f,
                                               RgbColor(RainbowRgb(h / 6.0),
                                                        opacity)};
                                }
                                const HRESULT made =
                                    target->CreateGradientStopCollection(
                                        hues, 6, stops.put());
                                if (SUCCEEDED(made)) {
                                    target->CreateLinearGradientBrush(
                                        {{at.x - dot, at.y - dot},
                                         {at.x + dot, at.y + dot}},
                                        stops.get(), rainbow.put());
                                }
                            }
                            brush->SetColor(RgbColor(
                                kind >= 0 ? FillChoiceRgb(kind, (int)i)
                                          : VisualizerChoiceRgb((int)i),
                                opacity));
                            if (rainbow) {
                                target->FillEllipse({at, dot, dot},
                                                    rainbow.get());
                            } else {
                                target->FillEllipse({at, dot, dot}, brush);
                            }
                            brush->SetColor(Fg(0.35f * opacity));
                            target->DrawEllipse({at, dot, dot}, brush, 1);
                            textLeft = at.x + dot + std::round(6 * s);
                        }
                        // A language's flag (none for "Automatic").
                        if (const int language =
                                id == kChoiceNotesLanguage ? (int)i
                                : id == kChoiceLanguage    ? (int)i - 1
                                                           : -1;
                            language >= 0) {
                            const D2D1_RECT_F flag =
                                ChoiceFlagRect(textLeft, button, s);
                            DrawFlag(target, brush, flag,
                                     (TextLanguage)language, opacity);
                            textLeft = flag.right + std::round(6 * s);
                        }
                        const std::wstring option = ChoiceOption(id, (int)i);
                        // (An icon pack's: what it is beside its name, see
                        // IconPackDetail.)
                        const float nameRight =
                            id == kChoiceIconPack
                                ? std::round((button.left + button.right) / 2)
                                : button.right - std::round(8 * s);
                        brush->SetColor(Fg((chosen ? 0.95f : 0.7f) * opacity));
                        DrawMarquee(target, brush, option, m_linkFormat.get(),
                                    {textLeft, button.top, nameRight,
                                     button.bottom},
                                    false);
                        if (id == kChoiceIconPack) {
                            // (A downloaded one's bin at the end, but while
                            // it downloads.)
                            const bool bin = m_iconPackOnDisk[i] &&
                                             IconPackProgress((int)i) < 0;
                            const D2D1_RECT_F& del = m_iconPackDelete[i];
                            brush->SetColor(Fg(0.5f * opacity));
                            DrawMarquee(target, brush,
                                        IconPackDetail((int)i),
                                        m_linkFormat.get(),
                                        {nameRight, button.top,
                                         bin ? del.left - std::round(6 * s)
                                             : button.right -
                                                   std::round(12 * s),
                                         button.bottom},
                                        false);
                            if (bin) {
                                const float glow = Hover(
                                    HoverKey(6810, (int)i),
                                    m_hover.part == Part::IconPackDelete &&
                                        m_hover.index == (int)i);
                                const float round = (del.bottom - del.top) / 2;
                                brush->SetColor(
                                    Fg((0.06f + 0.14f * glow) * opacity));
                                GlassFill(target, {del, round, round}, brush);
                                const WCHAR trash = 0xE74D;
                                brush->SetColor(
                                    Fg((0.6f + 0.35f * glow) * opacity));
                                DrawGlyph(target, &trash, 1,
                                          m_smallIconFormat.get(), del, brush);
                            }
                        }
                    }
                    break;
                }
                const D2D1_RECT_F track{segments.front().left,
                                        segments.front().top,
                                        segments.back().right,
                                        segments.back().bottom};
                const float radius = (track.bottom - track.top) / 2;
                brush->SetColor(Fg(0.08f * opacity));
                GlassFill(target, {track, radius, radius}, brush);
                const bool hover = m_hover.part == Part::ChoiceSegment &&
                                   m_hover.index / 16 == id;
                // Buttons (none chosen): lit under the mouse only.
                const bool buttons = ChoiceValue(id) < 0;
                const float knob =
                    buttons ? (float)(m_hover.index % 16)
                            : (float)std::clamp(
                                  m_choiceKnob[id] < 0 ? (double)ChoiceValue(id)
                                                       : m_choiceKnob[id],
                                  -0.05, segments.size() - 0.95);
                const float segmentWidth =
                    segments.front().right - segments.front().left;
                const float inset = std::round(2 * s);
                const D2D1_RECT_F lit{
                    segments.front().left + segmentWidth * knob + inset,
                    track.top + inset,
                    segments.front().right + segmentWidth * knob - inset,
                    track.bottom - inset};
                const float litRadius = (lit.bottom - lit.top) / 2;
                if (!buttons || hover) {
                    brush->SetColor(Fg((hover ? 0.26f : 0.2f) * opacity));
                    GlassFill(target, {lit, litRadius, litRadius}, brush);
                }
                for (size_t i = 0; i < segments.size(); i++) {
                    const std::wstring option = ChoiceOption(id, (int)i);
                    const bool chosen = buttons || ChoiceValue(id) == (int)i;
                    D2D1_RECT_F textRect = segments[i];
                    // The custom colors: a dot of each before its name.
                    if (id == kChoiceColors) {
                        const float dot = std::round(4 * s);
                        const D2D1_POINT_2F at{
                            segments[i].left + std::round(9 * s),
                            (segments[i].top + segments[i].bottom) / 2};
                        float parts[3];
                        CustomColorParts(g_customColors[i], parts);
                        brush->SetColor(D2D1::ColorF(parts[0], parts[1],
                                                     parts[2], opacity));
                        target->FillEllipse({at, dot, dot}, brush);
                        brush->SetColor(Fg(0.4f * opacity));
                        target->DrawEllipse({at, dot, dot}, brush, 1);
                        textRect.left = at.x + dot;
                        // The one hard to read on the fill: ringed in orange.
                        if (CustomColorHardToRead((int)i)) {
                            const float inset = std::round(2 * s);
                            const D2D1_RECT_F ring{
                                segments[i].left + inset,
                                segments[i].top + inset,
                                segments[i].right - inset,
                                segments[i].bottom - inset};
                            const float ringRadius =
                                (ring.bottom - ring.top) / 2;
                            brush->SetColor(
                                D2D1::ColorF(1.0f, 0.62f, 0.04f, opacity));
                            target->DrawRoundedRectangle(
                                {ring, ringRadius, ringRadius}, brush,
                                std::max(1.0f, std::round(1.5f * s)));
                        }
                    }
                    brush->SetColor(Fg((chosen ? 0.95f : 0.55f) * opacity));
                    target->DrawText(option.c_str(), (UINT32)option.size(),
                                     m_linkFormat.get(), textRect, brush);
                }
            }
            break;
        case SettingKind::Theme:
            // The colors: a swatch each, the chosen one ringed in blue.
            {
                const IslandPrefInfo& info =
                    kIslandPrefs[(int)IslandPref::Theme];
                label(m_themeRow, info.glyph, m_themeRow.right, Fg(0.95f));
                for (int i = 0; i < (int)ThemeId::Count; i++) {
                    const D2D1_RECT_F& r = m_swatches[i];
                    const D2D1_POINT_2F center{(r.left + r.right) / 2,
                                               (r.top + r.bottom) / 2};
                    const float radius = (r.right - r.left) / 2;
                    const bool hover =
                        m_hover.part == Part::ThemeSwatch && m_hover.index == i;
                    const float grow = std::round(1.5f * s) * Hover(r, hover);
                    brush->SetColor(ThemeSwatchColor(i));
                    target->FillEllipse({center, radius + grow, radius + grow},
                                        brush);
                    // The automatic ones: half black, half light (the other
                    // way around for the inverted one).
                    if (i == (int)ThemeId::Auto ||
                        i == (int)ThemeId::AutoInverted) {
                        brush->SetColor(ThemeSwatchColor(
                            (int)(i == (int)ThemeId::Auto ? ThemeId::Light
                                                          : ThemeId::Black)));
                        target->PushAxisAlignedClip(
                            {center.x, r.top - grow, r.right + grow,
                             r.bottom + grow},
                            D2D1_ANTIALIAS_MODE_ALIASED);
                        target->FillEllipse(
                            {center, radius + grow, radius + grow}, brush);
                        target->PopAxisAlignedClip();
                    }
                    // The custom one: its background with a dot of its
                    // icons' color.
                    if (i == (int)ThemeId::Custom) {
                        float parts[3];
                        CustomColorParts(g_customColors[kColorIcons], parts);
                        brush->SetColor(
                            D2D1::ColorF(parts[0], parts[1], parts[2], 1));
                        target->FillEllipse(
                            {center, radius * 0.45f, radius * 0.45f}, brush);
                    }
                    brush->SetColor(Fg(0.3f));
                    target->DrawEllipse({center, radius + grow, radius + grow},
                                        brush, 1);
                    if (i == g_themeIndex) {
                        brush->SetColor(Accent(1));
                        target->DrawEllipse(
                            {center, radius + std::round(3.5f * s),
                             radius + std::round(3.5f * s)},
                            brush, 2);
                    }
                }
            }
            break;
        case SettingKind::Place:
            // The place, for all apps (not for the bar or the notches,
            // always at the top).
            {
                const bool bar = g_settings.islandBar;
                const bool notches = NotchCountWanted() > 0;
                const float opacity = bar || notches ? 0.4f : 1.0f;
                label(m_placeRow, 0xE81E, m_placeGrid.left,
                      Fg(0.95f * opacity));
                DrawPlaceGrid(
                    target, brush, m_placeGrid, g_islandPlace.x,
                    g_islandPlace.y,
                    m_hover.part == Part::PlaceDot ? m_hover.index : -1,
                    opacity);
            }
            break;
        case SettingKind::AppPlaces:
            // The places per app (only for the pill, see PerAppPlacesOn).
            {
                const bool bar = !PerAppPlacesOn();
                const float opacity = bar ? 0.4f : 1.0f;
                if (const float lit = Hover(
                        m_appPlacesRow,
                        !bar && m_hover.part == Part::AppPlacesRow);
                    lit > 0.01f) {
                    const float radius = std::round(10 * s);
                    brush->SetColor(Fg(0.08f * lit));
                    GlassFill(target, {m_appPlacesRow, radius, radius}, brush);
                }
                label(m_appPlacesRow, 0xE8A9,
                      m_appPlacesRow.right - std::round(24 * s),
                      Fg(0.95f * opacity));
                brush->SetColor(Fg(0.5f * opacity));
                const WCHAR chevron = kGlyphNext;
                DrawGlyph(target, &chevron, 1, m_iconFormat.get(),
                          {m_appPlacesRow.right - std::round(28 * s),
                           m_appPlacesRow.top, m_appPlacesRow.right,
                           m_appPlacesRow.bottom},
                          brush);
            }
            break;
        case SettingKind::Icons: {
            // Each icon as the island draws it now (its pack's, or a
            // picture of its own: ringed, with an "x" back to the pack's),
            // a bit bigger, its name under it.
            label(entry.rect, IconGroupGlyph(entry.value), entry.rect.right,
                  Fg(0.95f));
            for (const auto& cell : m_iconCells) {
                const IconInfo& icon = kIcons[cell.icon];
                if ((int)icon.group != entry.value) {
                    continue;
                }
                const bool own = HasOwnIconLook(icon.glyph);
                const bool hover = (m_hover.part == Part::IconCell ||
                                    m_hover.part == Part::IconCellReset ||
                                    m_hover.part == Part::IconCellColor) &&
                                   m_hover.index == cell.icon;
                const float lit = Hover(HoverKey(4150, cell.icon), hover);
                const float radius = std::round(8 * s);
                brush->SetColor(Fg(0.05f + 0.06f * lit));
                GlassFill(target, {cell.rect, radius, radius}, brush);
                if (own) {
                    brush->SetColor(Accent(0.6f));
                    target->DrawRoundedRectangle(
                        {{cell.rect.left + 0.5f, cell.rect.top + 0.5f,
                          cell.rect.right - 0.5f, cell.rect.bottom - 0.5f},
                         radius - 0.5f,
                         radius - 0.5f},
                        brush, 1);
                }
                const float iconSize = std::round(32 * s);
                const D2D1_POINT_2F middle{
                    std::round((cell.rect.left + cell.rect.right) / 2),
                    cell.rect.top + std::round(6 * s) + iconSize / 2};
                D2D1_MATRIX_3X2_F before;
                target->GetTransform(&before);
                target->SetTransform(
                    D2D1::Matrix3x2F::Scale(kIconCellZoom, kIconCellZoom,
                                            middle) *
                    before);
                brush->SetColor(Fg(0.92f));
                DrawGlyph(target, &icon.glyph, 1, m_iconFormat.get(),
                          {middle.x - iconSize / 2, middle.y - iconSize / 2,
                           middle.x + iconSize / 2, middle.y + iconSize / 2},
                          brush);
                target->SetTransform(before);
                brush->SetColor(Fg(0.6f));
                DrawMarquee(target, brush, icon.name, m_smallFormat.get(),
                            {cell.rect.left + std::round(4 * s),
                             cell.rect.bottom - std::round(20 * s),
                             cell.rect.right - std::round(4 * s),
                             cell.rect.bottom - std::round(4 * s)},
                            true);
                // Its brush: under the mouse, or with a color chosen (a dot
                // of it).
                const int color = IconColorOf(icon.glyph);
                if (lit > 0.01f || color >= 0) {
                    const float glow = Hover(
                        HoverKey(4152, cell.icon),
                        m_hover.part == Part::IconCellColor &&
                            m_hover.index == cell.icon);
                    const float round =
                        (cell.paint.bottom - cell.paint.top) / 2;
                    const float shown = color >= 0 ? 1.0f : lit;
                    brush->SetColor(Fg((0.12f + 0.12f * glow) * shown));
                    GlassFill(target, {cell.paint, round, round}, brush);
                    if (color >= 0) {
                        const float dot = std::round(4 * s);
                        brush->SetColor(D2D1::ColorF(
                            ((color >> 16) & 0xFF) / 255.0f,
                            ((color >> 8) & 0xFF) / 255.0f,
                            (color & 0xFF) / 255.0f));
                        target->FillEllipse(
                            {{(cell.paint.left + cell.paint.right) / 2,
                              (cell.paint.top + cell.paint.bottom) / 2},
                             dot,
                             dot},
                            brush);
                    } else {
                        // (Personalize's brush.)
                        const WCHAR paint = 0xE771;
                        brush->SetColor(Fg(0.85f * shown));
                        DrawGlyph(target, &paint, 1, m_smallIconFormat.get(),
                                  cell.paint, brush);
                    }
                }
                if (own) {
                    const float glow = Hover(
                        HoverKey(4151, cell.icon),
                        m_hover.part == Part::IconCellReset &&
                            m_hover.index == cell.icon);
                    const float round =
                        (cell.reset.bottom - cell.reset.top) / 2;
                    brush->SetColor(Fg(0.12f + 0.12f * glow));
                    GlassFill(target, {cell.reset, round, round}, brush);
                    const WCHAR close = kGlyphClose;
                    brush->SetColor(Fg(0.85f));
                    DrawGlyph(target, &close, 1, m_smallIconFormat.get(),
                              cell.reset, brush);
                }
            }
            break;
        }
        case SettingKind::LyricsApps: {
            const bool disabled = DisabledIndex(entry) >= 0;
            const float opacity = disabled ? 0.4f : 1.0f;
            label(entry.rect, 0xEC4F, entry.rect.right, Fg(0.95f * opacity));
            const auto& apps = LyricsApps();
            if (apps.empty()) {
                PCWSTR none = Tr(L"An app appears here once it plays something",
                                 L"Um app aparece aqui quando tocar algo");
                brush->SetColor(Fg(0.5f * opacity));
                DrawMarquee(target, brush, none, m_smallFormat.get(),
                            {entry.rect.left + std::round(10 * s),
                             entry.rect.top + entry.head,
                             entry.rect.right - std::round(8 * s),
                             entry.rect.top + entry.head + std::round(32 * s)},
                            true);
                break;
            }
            // A card each: the app's icon, its name, what happens to its
            // songs, and a switch. Turned on or off, the switch slides, the
            // card's tint and ring and the icon fade, and the line under the
            // name changes over; a new app's card rises in. (Keyed by the
            // app, not its place: one let go moves the others up.)
            for (size_t i = 0; i < m_lyricsAppChips.size() && i < apps.size();
                 i++) {
                const D2D1_RECT_F& card = m_lyricsAppChips[i];
                const long long id =
                    (long long)(std::hash<std::wstring>{}(apps[i].appId) &
                                0xFFFFFFFFFFull);
                const bool hover = !disabled &&
                                   m_hover.part == Part::LyricsAppChip &&
                                   m_hover.index == (int)i;
                const float lit = Hover(HoverKey(4140, id), hover);
                const float on = Hover(HoverKey(4141, id), !apps[i].off,
                                       apps[i].off ? 0.0f : 1.0f);
                const float in = Hover(HoverKey(4142, id), true);
                const float rise = std::round((1 - in) * 8 * s);
                const D2D1_RECT_F shown{card.left, card.top + rise, card.right,
                                        card.bottom + rise};
                winrt::com_ptr<ID2D1Layer> layer;
                if (opacity * in < 0.999f) {
                    target->CreateLayer(nullptr, layer.put());
                    target->PushLayer(
                        D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                              D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                              D2D1::IdentityMatrix(),
                                              opacity * in),
                        layer.get());
                }
                const float radius = std::round(12 * s);
                brush->SetColor(Fg(0.06f + 0.05f * lit));
                GlassFill(target, {shown, radius, radius}, brush);
                if (on > 0.01f) {
                    brush->SetColor(Accent(0.12f * on));
                    target->FillRoundedRectangle({shown, radius, radius},
                                                 brush);
                    brush->SetColor(Accent(0.45f * on));
                    target->DrawRoundedRectangle(
                        {{shown.left + 0.5f, shown.top + 0.5f,
                          shown.right - 0.5f, shown.bottom - 0.5f},
                         radius - 0.5f,
                         radius - 0.5f},
                        brush, 1);
                }
                const float inset = std::round(10 * s);
                const float middle = std::round((shown.top + shown.bottom) / 2);
                const float iconSize = std::round(24 * s);
                const D2D1_RECT_F icon{shown.left + inset,
                                       middle - iconSize / 2,
                                       shown.left + inset + iconSize,
                                       middle + iconSize / 2};
                if (ID2D1Bitmap* bitmap = LyricsAppBitmap(apps[i].appId)) {
                    DrawBitmapSmooth(target, bitmap, icon, 0.45f + 0.55f * on);
                } else {
                    brush->SetColor(Fg(0.12f));
                    GlassFill(target,
                              {icon, std::round(6 * s), std::round(6 * s)},
                              brush);
                    const WCHAR note = 0xEC4F;
                    brush->SetColor(Fg(0.4f + 0.4f * on));
                    DrawGlyph(target, &note, 1, m_smallIconFormat.get(), icon,
                              brush);
                }
                const float switchWidth = std::round(36 * s);
                const float switchHeight = std::round(20 * s);
                const D2D1_RECT_F toggle{
                    shown.right - inset - switchWidth,
                    std::round(middle - switchHeight / 2),
                    shown.right - inset,
                    std::round(middle - switchHeight / 2) + switchHeight};
                const float textLeft = icon.right + inset;
                const float textRight = toggle.left - inset;
                brush->SetColor(Fg(0.6f + 0.35f * on));
                DrawMarquee(target, brush, AppDisplayName(apps[i].appId),
                            m_subtitleFormat.get(),
                            {textLeft, shown.top + std::round(4 * s), textRight,
                             middle + std::round(1 * s)},
                            false);
                // (What it does now, the old line sliding out as the new
                // one comes in.)
                const float lineHeight = middle - shown.top - std::round(4 * s);
                const auto line = [&](PCWSTR text, float amount, float from) {
                    if (amount < 0.01f) {
                        return;
                    }
                    const float shift = std::round((1 - amount) * from * s);
                    brush->SetColor(Fg(0.55f * amount));
                    DrawMarquee(target, brush, text, m_smallFormat.get(),
                                {textLeft, middle + shift, textRight,
                                 middle + lineHeight + shift},
                                false);
                };
                line(Tr(L"Looks for lyrics", L"Busca letras"), on, -6);
                line(Tr(L"Off: nothing is sent",
                        L"Desligado: nada \u00e9 enviado"),
                     1 - on, 6);
                DrawToggle(target, brush, toggle, on);
                if (layer) {
                    target->PopLayer();
                }
            }
            break;
        }
        case SettingKind::Size:
            // The size (or the room from the edges): its value, a slider
            // (the knob in steps), and a button to restore it, dimmed while
            // it's the usual one.
            {
                const int slider = entry.value;
                const IslandSlider& info = kIslandSliders[slider];
                const D2D1_RECT_F& sizeRow = m_sizeRow[slider];
                const D2D1_RECT_F& sizeSlider = m_sizeSlider[slider];
                const D2D1_RECT_F& sizeReset = m_sizeReset[slider];
                const int size = IslandSliderValue(slider);
                const bool bar = SliderDisabled(slider);
                label(sizeRow,
                      slider == 19   ? 0xE8D2
                      : slider == 18 ? 0xE8D6
                      : slider == 17 ? 0xF0E2
                      : slider == 16 ? 0xEDA8
                      : slider == 15 ? 0xEA3A
                      : slider == 14 ? 0xE90C
                      : slider == 13 ? 0xE71A
                      : slider == 12 ? 0xE9A6
                      : slider == 11 ? 0xE8D6
                      : slider == 10 ? 0xE8A3
                      : slider == 9  ? 0xE7C9
                      : slider == 8  ? 0xE7B3
                      : slider == 7  ? 0xE8E9
                      : slider == 6  ? 0xE8B4
                      : slider == 5  ? 0xE916
                      : slider == 4  ? 0xE790
                      : slider == 3  ? 0xE8FD
                      : slider == 2  ? 0xEC4A
                      : slider == 1  ? 0xE9A6
                                     : 0xE740,
                      sizeRow.right, Fg(bar ? 0.38f : 0.95f));
                DrawSliderTrack(
                    target, brush, sizeSlider, info.min, info.max, info.usual,
                    size,
                    (m_dragging && m_dragIndex == kDragIslandSize - slider) ||
                        (m_hover.part == Part::SizeSlider &&
                         m_hover.index == slider));

                const bool usualSize = size == info.usual;
                if (const float lit = Hover(
                        sizeReset, !usualSize &&
                                       m_hover.part == Part::SizeReset &&
                                       m_hover.index == slider);
                    lit > 0.01f) {
                    const float radius = (sizeReset.right - sizeReset.left) / 2;
                    brush->SetColor(Fg(0.12f * lit));
                    GlassFill(target, {sizeReset, radius, radius}, brush);
                }
                brush->SetColor(Fg(usualSize ? 0.25f : 0.85f));
                const WCHAR restore = 0xE72C;
                DrawGlyph(target, &restore, 1, m_iconFormat.get(), sizeReset,
                          brush);
            }
            break;
        case SettingKind::Order:
            DrawOrderStrip(target, brush, entry.rect);
            break;
        case SettingKind::Monitors:
            DrawMonitors(target, brush);
            break;
        case SettingKind::Profiles:
            DrawProfiles(target, brush, entry.rect);
            break;
        case SettingKind::Pref:
            // An option's switch; one that depends on another being on is
            // dimmed while it's off.
            {
                const int item = entry.value;
                const D2D1_RECT_F& row = entry.rect;
                const IslandPref pref = (IslandPref)item;
                const IslandPrefInfo& info = kIslandPrefs[item];
                const bool disabled = PrefDisabled(pref);
                const bool hover = !disabled &&
                                   m_hover.part == Part::SettingToggle &&
                                   m_hover.index == item;
                // (In the window, all of its card.)
                if (const float lit = Hover(row, hover); lit > 0.01f) {
                    const float radius = std::round((m_windowed ? 6 : 10) * s);
                    brush->SetColor(Fg(0.08f * lit));
                    GlassFill(target, {CardOf(row), radius, radius}, brush);
                }
                const float toggleWidth = std::round(36 * s);
                const float toggleHeight = std::round(20 * s);
                const D2D1_RECT_F toggle{
                    row.right - std::round(8 * s) - toggleWidth,
                    std::round((row.top + row.bottom - toggleHeight) / 2),
                    row.right - std::round(8 * s),
                    std::round((row.top + row.bottom - toggleHeight) / 2) +
                        toggleHeight};
                const float opacity = disabled ? 0.4f : 1.0f;
                label(row, info.glyph, toggle.left, Fg(0.95f * opacity));
                winrt::com_ptr<ID2D1Layer> layer;
                if (disabled) {
                    target->CreateLayer(nullptr, layer.put());
                    target->PushLayer(
                        D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                              D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                              D2D1::IdentityMatrix(), opacity),
                        layer.get());
                }
                DrawToggle(target, brush, toggle,
                           KnobFor(100 + item, GetIslandPref(pref) != 0));
                if (layer) {
                    target->PopLayer();
                }
            }
            break;
        case SettingKind::WhatsNew:
        case SettingKind::Setup: {
            const D2D1_RECT_F& row = entry.rect;
            brush->SetColor(Fg(0.95f));
            target->DrawText(entryName.c_str(), (UINT32)entryName.size(),
                             m_titleFormat.get(),
                             {row.left + std::round(4 * s), row.top,
                              row.right - std::round(4 * s),
                              row.top + std::round(30 * s)},
                             brush);
            float top = row.top + std::round(34 * s);
            const float left = row.left + std::round(4 * s);
            const float textLeft = left + std::round(24 * s);
            const float arrow = std::round(kNoteArrow * s);
            // (One line's height: the dot beside an item's first line.)
            const float lineHeight =
                WrappedHeight(L"A", m_subtitleFormat.get(), 1000 * s);
            bool first = true;
            for (const auto& [line, heading, link] :
                 entry.kind == SettingKind::Setup ? SetupLines()
                                                  : NotesLines(entry.value)) {
                const float gap = std::round(NoteGap(heading, first) * s);
                // A thin line between two items.
                if (!heading && !first) {
                    brush->SetColor(Fg(0.06f));
                    target->FillRectangle(
                        {textLeft, std::round(top + gap / 2),
                         row.right - std::round(4 * s),
                         std::round(top + gap / 2) + 1},
                        brush);
                }
                top += gap;
                first = heading;
                IDWriteTextFormat* format =
                    heading ? m_sectionFormat.get() : m_subtitleFormat.get();
                const float textRight =
                    heading ? row.right : row.right - arrow;
                const float height =
                    WrappedHeight(line, format, textRight - textLeft);
                if (heading) {
                    brush->SetColor(Accent(1));
                    DrawWrapped(target, brush, line, format,
                                {left, top, row.right, top + height});
                    top += height;
                    continue;
                }
                // An item with its option: all of it a button, lit under
                // the mouse, its small arrow at the end of its first line
                // sliding a little to the right; a click shows the option.
                if (link.kind != SettingKind::Label) {
                    const D2D1_RECT_F item{left, top - std::round(5 * s),
                                           row.right,
                                           top + height + std::round(5 * s)};
                    const int index = (int)m_noteLinks.size();
                    const float lit =
                        Hover(item, m_hover.part == Part::NoteLink &&
                                        m_hover.index == index);
                    if (lit > 0.01f) {
                        const float radius = std::round(8 * s);
                        brush->SetColor(Accent(0.1f * lit));
                        target->FillRoundedRectangle({item, radius, radius},
                                                     brush);
                    }
                    const WCHAR go = 0xE76C;
                    const float slide = std::round(4 * s) * lit;
                    brush->SetColor(Accent(0.55f + 0.45f * lit));
                    DrawGlyph(target, &go, 1, m_smallIconFormat.get(),
                              {textRight + slide, top,
                               row.right - std::round(4 * s) + slide,
                               top + lineHeight},
                              brush);
                    m_noteLinks.push_back({link, item});
                }
                brush->SetColor(Fg(0.5f));
                target->FillEllipse(
                    {{left + std::round(10 * s), top + lineHeight / 2},
                     std::round(2 * s), std::round(2 * s)},
                    brush);
                brush->SetColor(Fg(0.85f));
                DrawWrapped(target, brush, line, format,
                            {textLeft, top, textRight, top + height});
                top += height;
            }
            break;
        }
        case SettingKind::Dock:
        case SettingKind::About:
        case SettingKind::Logs: {
            const D2D1_RECT_F& row = entry.rect;
            // The logs themselves.
            if (entry.kind == SettingKind::Logs && entry.value == 1) {
                brush->SetColor(Fg(0.75f));
                DrawWrapped(target, brush, m_logsShown, m_smallFormat.get(),
                            {row.left + std::round(4 * s),
                             row.top + std::round(6 * s),
                             row.right - std::round(4 * s),
                             row.bottom - std::round(6 * s)});
                break;
            }
            // About's rows open a page: all of the card, an arrow at the
            // right.
            float right = row.right;
            if (entry.kind == SettingKind::About) {
                const float lit =
                    Hover(row, m_hover.part == Part::PageButton &&
                                   m_hover.index == kPageLink + entry.value);
                if (lit > 0.01f) {
                    const float radius = std::round(6 * s);
                    brush->SetColor(Fg(0.08f * lit));
                    GlassFill(target, {CardOf(row), radius, radius}, brush);
                }
                const WCHAR open = 0xE8A7;
                brush->SetColor(Fg(0.6f));
                DrawGlyph(target, &open, 1, m_smallIconFormat.get(),
                          {row.right - std::round(28 * s), row.top,
                           row.right, row.bottom},
                          brush);
                right = row.right - std::round(30 * s);
            }
            // (The mod's and Instagram's own logos are drawn, no font has
            // them.)
            static constexpr WCHAR kAboutGlyphs[] = {L' ', 0xE774, 0xEBE8,
                                                     L' ', 0xE909};
            const WCHAR glyph =
                entry.kind == SettingKind::Dock    ? (WCHAR)0xE7C4
                : entry.kind == SettingKind::Logs ? (WCHAR)0xE9F9
                                                  : kAboutGlyphs[std::clamp(
                                                        entry.value, 0, 4)];
            for (const auto& [id, rect] : m_pageButtons) {
                if (rect.top >= row.top && rect.bottom <= row.bottom) {
                    right = std::min(right, rect.left);
                }
            }
            label(row, glyph, right, Fg(0.95f));
            if (entry.kind == SettingKind::About && entry.value == 0) {
                const float logo = std::round(24 * s);
                const float x = row.left + (icon - logo) / 2;
                const float y = std::round((row.top + row.bottom - logo) / 2);
                DrawModLogo(target, {x, y, x + logo, y + logo});
            }
            if (entry.kind == SettingKind::About && entry.value == 3) {
                brush->SetColor(Fg(0.95f));
                DrawInstagramLogo(target, brush,
                                  {row.left + icon / 2,
                                   (row.top + row.bottom) / 2},
                                  m_iconFormat->GetFontSize() * 0.86f);
            }
            for (const auto& [id, rect] : m_pageButtons) {
                if (rect.top < row.top || rect.bottom > row.bottom) {
                    continue;
                }
                const float radius = (rect.bottom - rect.top) / 2;
                const float lit = Hover(rect, m_hover.part == Part::PageButton &&
                                                  m_hover.index == id);
                brush->SetColor(Fg(0.08f + 0.06f * lit));
                GlassFill(target, {rect, radius, radius}, brush);
                PCWSTR name = id == kPageWindhawk
                                  ? Tr(L"Open Windhawk", L"Abrir o Windhawk")
                              : id == kPageCopyLogs ? Tr(L"Copy", L"Copiar")
                                                    : Tr(L"Save...", L"Salvar...");
                brush->SetColor(Fg(0.95f));
                target->DrawText(name, (UINT32)wcslen(name), m_linkFormat.get(),
                                 rect, brush);
            }
            break;
        }
        case SettingKind::Shortcut:
            // The keys in a chip (ringed while recording), and a button to
            // put Win+J back, dimmed while it's Win+J.
            {
                const bool disabled = ShortcutDisabled();
                const float opacity = disabled ? 0.4f : 1.0f;
                label(entry.rect, 0xE765, m_shortcutChip.left,
                      Fg(0.95f * opacity));
                const D2D1_RECT_F& chip = m_shortcutChip;
                const float radius = (chip.bottom - chip.top) / 2;
                const float lit =
                    Hover(chip, !disabled && m_hover.part == Part::ShortcutRecord);
                brush->SetColor(Fg((0.08f + 0.06f * lit) * opacity));
                GlassFill(target, {chip, radius, radius}, brush);
                if (m_recordingShortcut) {
                    brush->SetColor(Accent(0.9f));
                    target->DrawRoundedRectangle(
                        {{chip.left + 0.75f, chip.top + 0.75f,
                          chip.right - 0.75f, chip.bottom - 0.75f},
                         radius,
                         radius},
                        brush, 1.5f);
                }
                const std::wstring keys =
                    m_recordingShortcut
                        ? std::wstring(Tr(L"Press the keys",
                                          L"Pressione as teclas"))
                        : ShortcutText(g_minimizeShortcut);
                brush->SetColor(Fg((m_recordingShortcut ? 0.6f : 0.95f) *
                                   opacity));
                target->DrawText(keys.c_str(), (UINT32)keys.size(),
                                 m_linkFormat.get(),
                                 {chip.left + std::round(6 * s), chip.top,
                                  chip.right - std::round(6 * s), chip.bottom},
                                 brush);
                const bool usual =
                    g_minimizeShortcut == kDefaultMinimizeShortcut;
                if (const float resetLit = Hover(
                        m_shortcutReset,
                        !usual && !disabled &&
                            m_hover.part == Part::ShortcutReset);
                    resetLit > 0.01f) {
                    const float resetRadius =
                        (m_shortcutReset.right - m_shortcutReset.left) / 2;
                    brush->SetColor(Fg(0.12f * resetLit));
                    GlassFill(target,
                              {m_shortcutReset, resetRadius, resetRadius},
                              brush);
                }
                brush->SetColor(Fg((usual ? 0.25f : 0.85f) * opacity));
                const WCHAR restore = 0xE72C;
                DrawGlyph(target, &restore, 1, m_iconFormat.get(),
                          m_shortcutReset, brush);
            }
            break;
    }
}

// The order strip: a little pill with the items' icons (the hidden ones
// dimmed), the one under the mouse lit, the one dragged lifted; under it, what
// the one under the mouse is, or how it works.
void ControlPanel::DrawOrderStrip(ID2D1RenderTarget* target,
                                  ID2D1SolidColorBrush* brush,
                                  const D2D1_RECT_F& row,
                                  int stripIndex) {
    const float s = m_scale;
    const D2D1_RECT_F& strip = StripRect(stripIndex);
    const float radius = (strip.bottom - strip.top) / 2;
    const bool notches = stripIndex < 0 && !m_notchParts.empty();
    if (notches) {
        // The notches, shaped like on the screen: the strip's top is the
        // screen's edge.
        winrt::com_ptr<ID2D1Factory> factory;
        target->GetFactory(factory.put());
        const float curve = NotchShoulder(strip.bottom - strip.top);
        const float corner = NotchRadius(strip.bottom - strip.top);
        brush->SetColor(Fg(0.04f));
        target->FillRectangle({strip.left, strip.top, strip.right,
                               strip.top + std::round(2 * s)},
                              brush);
        brush->SetColor(Fg(0.09f));
        for (const auto& entry : m_notchParts) {
            if (auto shape = NotchGeometry(factory.get(), entry.second, corner,
                                           curve, false, false, true)) {
                target->FillGeometry(shape.get(), brush);
            }
        }
    } else {
        brush->SetColor(Fg(0.07f));
        GlassFill(target, {strip, radius, radius}, brush);
    }
    const OrderChip* dragged = nullptr;
    auto drawChip = [&](const OrderChip& chip) {
        const int pref = PillGroupPref(chip.group);
        const bool shown =
            stripIndex >= 0 ? true
            : pref >= 0     ? GetIslandPref((IslandPref)pref) != 0
            : chip.group == PillGroup::Usage ? AnyUsageShown()
                                             : true;
        const bool lifted = m_orderMoved && (int)chip.group == m_orderDrag &&
                            chip.strip == m_orderDragStrip;
        const bool hover =
            m_hover.part == Part::OrderChip &&
            m_hover.index == OrderChipIndex(stripIndex, chip.group);
        const float dx = (float)chip.x - chip.rect.left;
        D2D1_RECT_F rect{chip.rect.left + dx, chip.rect.top,
                         chip.rect.right + dx, chip.rect.bottom};
        if (lifted) {
            const float grow = std::round(2 * s);
            rect = {rect.left - grow, rect.top - grow, rect.right + grow,
                    rect.bottom + grow};
        }
        const float chipRadius = (rect.bottom - rect.top) / 2;
        const float lit = Hover(
            HoverKey(4002, OrderChipIndex(stripIndex, chip.group)), hover);
        if (lit > 0.01f || lifted) {
            brush->SetColor(Fg(lifted ? 0.22f : 0.12f * lit));
            GlassFill(target, {rect, chipRadius, chipRadius}, brush);
        }
        brush->SetColor(Fg(shown ? 0.95f : 0.3f));
        if (chip.group == PillGroup::Clock) {
            target->DrawText(L"12:00", 5, m_linkFormat.get(), rect, brush);
        } else {
            const WCHAR glyph = kPillGroupInfo[(int)chip.group].glyph;
            DrawGlyph(target, &glyph, 1, m_smallIconFormat.get(), rect, brush);
        }
    };
    for (const auto& chip : m_orderChips) {
        if (chip.strip != stripIndex) {
            continue;
        }
        if (m_orderMoved && (int)chip.group == m_orderDrag &&
            chip.strip == m_orderDragStrip) {
            dragged = &chip;
            continue;
        }
        drawChip(chip);
    }
    if (dragged) {
        drawChip(*dragged);
    }
    // (An app's strip has no line under it.)
    if (stripIndex >= 0) {
        return;
    }
    std::wstring hint;
    if (m_hover.part == Part::OrderChip && m_hover.index >= 0 &&
        m_hover.index < kPillGroups) {
        const PillGroupInfo& info = kPillGroupInfo[m_hover.index];
        hint = Tr(info.english, info.portuguese);
        const int pref = PillGroupPref((PillGroup)m_hover.index);
        // (Any of them is dragged, shown or hidden.)
        hint += Tr(L" · drag to move", L" · arraste para mover");
        if (pref >= 0) {
            hint += GetIslandPref((IslandPref)pref)
                        ? Tr(L" · click to hide", L" · clique para esconder")
                        : Tr(L" · hidden, click to show",
                             L" · escondido, clique para mostrar");
        }
    } else if (notches) {
        hint = Tr(L"Drag from one notch to another · click to hide",
                  L"Arraste de um entalhe para outro · clique para esconder");
    } else {
        hint = Tr(L"Drag to change the order · click to hide",
                  L"Arraste para mudar a ordem · clique para esconder");
    }
    brush->SetColor(Fg(0.6f));
    DrawMarquee(target, brush, hint, m_smallFormat.get(),
                {row.left + std::round(6 * s), strip.bottom + std::round(3 * s),
                 row.right - std::round(6 * s), row.bottom - std::round(2 * s)},
                true);
}

void ControlPanel::DrawAccessibility(ID2D1RenderTarget* target,
                                     ID2D1SolidColorBrush* brush) {
    DrawHeader(target, brush, Tr(L"Accessibility", L"Acessibilidade"));
    for (size_t i = 0; i < m_accessibility.size() && i < m_rows.size(); i++) {
        const AccessibilityItem& item = m_accessibility[i];
        DrawRowToggle(target, brush, m_rows[i], item.glyph, item.name, item.on,
                      m_hover.part == Part::AccessibilityRow &&
                          m_hover.index == (int)i,
                      !item.button);
    }
}

void ControlPanel::DrawPassword(ID2D1RenderTarget* target,
                                ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    DrawHeader(target, brush, m_joining.name.c_str());

    // The field: dots (or the text), a caret, and the eye to show it.
    const float radius = std::round(10 * s);
    brush->SetColor(Fg(0.10f));
    GlassFill(target, {m_field, radius, radius}, brush);
    brush->SetColor(Accent(1));
    target->DrawRoundedRectangle(
        {{m_field.left + 0.5f, m_field.top + 0.5f, m_field.right - 0.5f,
          m_field.bottom - 0.5f},
         radius,
         radius},
        brush, 1.5f);
    const D2D1_RECT_F textRect{m_field.left + std::round(12 * s), m_field.top,
                               m_eye.left, m_field.bottom};
    const std::wstring shown =
        m_showPassword ? m_password
                       : std::wstring(m_password.size(), L'\u2022');
    float caretX = textRect.left;
    if (shown.empty()) {
        PCWSTR placeholder = Tr(L"Password", L"Senha");
        brush->SetColor(Fg(0.4f));
        target->DrawText(placeholder, (UINT32)wcslen(placeholder),
                         m_subtitleFormat.get(), textRect, brush);
        m_passwordEdit.layout = nullptr;
    } else {
        caretX = DrawLineEdit(target, brush, m_dwrite,
                              m_subtitleFormat.get(), textRect, shown,
                              m_passwordEdit, Fg(0.95f));
    }
    brush->SetColor(CaretColor());
    const float middle = (m_field.top + m_field.bottom) / 2;
    target->FillRectangle({caretX + 1, middle - std::round(8 * s), caretX + 2.5f,
                           middle + std::round(8 * s)},
                          brush);
    const WCHAR eye = m_showPassword ? kGlyphHide : kGlyphShow;
    brush->SetColor(
        Fg(0.55f + 0.4f * Hover(m_eye, m_hover.part == Part::ShowPassword)));
    DrawGlyph(target, &eye, 1, m_iconFormat.get(), m_eye, brush);

    // "Connect", blue.
    const float buttonRadius = (m_connect.bottom - m_connect.top) / 2;
    brush->SetColor(Accent(
        m_connecting
            ? 0.5f
            : 1.0f - 0.15f * Hover(m_connect, m_hover.part == Part::Connect)));
    target->FillRoundedRectangle({m_connect, buttonRadius, buttonRadius},
                                 brush);
    PCWSTR connect = Tr(L"Connect", L"Conectar");
    brush->SetColor(AccentInk(1));
    target->DrawText(connect, (UINT32)wcslen(connect), m_linkFormat.get(),
                     m_connect, brush);
    DrawStatus(target, brush);
}

// The hotspot's page: its network name and its password (dots, or the
// text with the eye), the one typed in framed in the accent color with the
// caret, and "Save".
void ControlPanel::DrawHotspot(ID2D1RenderTarget* target,
                               ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    DrawHeader(target, brush, Tr(L"Mobile hotspot", L"Hotspot móvel"));
    const float radius = std::round(10 * s);
    auto field = [&](const D2D1_RECT_F& box, const std::wstring& shown,
                     TextEdit& edit, PCWSTR placeholder, bool typing,
                     float right) {
        brush->SetColor(Fg(0.10f));
        GlassFill(target, {box, radius, radius}, brush);
        brush->SetColor(typing ? Accent(1) : Fg(0.12f));
        target->DrawRoundedRectangle(
            {{box.left + 0.5f, box.top + 0.5f, box.right - 0.5f,
              box.bottom - 0.5f},
             radius,
             radius},
            brush, typing ? 1.5f : 1.0f);
        const D2D1_RECT_F textRect{box.left + std::round(12 * s), box.top,
                                   right, box.bottom};
        float caretX = textRect.left;
        if (shown.empty()) {
            brush->SetColor(Fg(0.4f));
            target->DrawText(placeholder, (UINT32)wcslen(placeholder),
                             m_subtitleFormat.get(), textRect, brush);
            edit.layout = nullptr;
        } else {
            caretX = DrawLineEdit(target, brush, m_dwrite,
                                  m_subtitleFormat.get(), textRect, shown,
                                  edit, Fg(0.95f));
        }
        if (typing) {
            brush->SetColor(CaretColor());
            const float middle = (box.top + box.bottom) / 2;
            target->FillRectangle(
                {caretX + 1, middle - std::round(8 * s), caretX + 2.5f,
                 middle + std::round(8 * s)},
                brush);
        }
    };
    field(m_hotspotField, m_hotspotName, m_hotspotNameEdit,
          Tr(L"Network name", L"Nome da rede"), m_hotspotTypingName,
          m_hotspotField.right - std::round(12 * s));
    field(m_field,
          m_showPassword ? m_password
                         : std::wstring(m_password.size(), L'•'),
          m_passwordEdit, Tr(L"Password", L"Senha"), !m_hotspotTypingName,
          m_eye.left);
    const WCHAR eye = m_showPassword ? kGlyphHide : kGlyphShow;
    brush->SetColor(
        Fg(0.55f + 0.4f * Hover(m_eye, m_hover.part == Part::ShowPassword)));
    DrawGlyph(target, &eye, 1, m_iconFormat.get(), m_eye, brush);

    // "Save", blue (dimmed while a read or a change is under way).
    const float buttonRadius = (m_connect.bottom - m_connect.top) / 2;
    const float lit = Hover(m_connect, m_hover.part == Part::HotspotSave);
    brush->SetColor(Accent(m_hotspotBusy ? 0.5f : 1.0f - 0.15f * lit));
    target->FillRoundedRectangle({m_connect, buttonRadius, buttonRadius},
                                 brush);
    PCWSTR save = Tr(L"Save", L"Salvar");
    brush->SetColor(AccentInk(1));
    target->DrawText(save, (UINT32)wcslen(save), m_linkFormat.get(),
                     m_connect, brush);
    DrawStatus(target, brush);
}

// The hotspot's page opened: its name and password read first (the fields
// empty until then).
void ControlPanel::OpenHotspot() {
    m_hotspotName.clear();
    m_hotspotNameEdit.Reset();
    m_password.clear();
    m_passwordEdit.Reset();
    m_showPassword = false;
    m_hotspotTypingName = true;
    m_hotspotStatus = Tr(L"Reading…", L"Lendo…");
    SetView(View::Hotspot);
    StartHotspotWorker(new HotspotRequest{m_hwnd});
}

// Its name (1 to 32 bytes in UTF-8, as a network's name is kept) and
// password (8 to 63 letters, numbers or symbols without accents, as WPA2
// takes them) checked, then changed.
void ControlPanel::SaveHotspot() {
    if (m_hotspotBusy) {
        return;
    }
    const int bytes = WideCharToMultiByte(
        CP_UTF8, 0, m_hotspotName.data(), (int)m_hotspotName.size(), nullptr,
        0, nullptr, nullptr);
    if (m_hotspotName.empty() || bytes < 1 || bytes > 32) {
        m_hotspotStatus =
            Tr(L"The name has 1 to 32 characters (a letter outside English, "
               L"like an accented one, counts as two or three).",
               L"O nome tem de 1 a 32 caracteres (uma letra fora do inglês, "
               L"como uma acentuada, conta como duas ou três).");
        return;
    }
    if (m_password.size() < 8 || m_password.size() > 63 ||
        std::any_of(m_password.begin(), m_password.end(),
                    [](WCHAR c) { return c < 0x20 || c > 0x7E; })) {
        m_hotspotStatus =
            Tr(L"The password has 8 to 63 letters, numbers or symbols, "
               L"without accents.",
               L"A senha tem de 8 a 63 letras, números ou símbolos, sem "
               L"acentos.");
        return;
    }
    m_hotspotStatus = Tr(L"Saving…", L"Salvando…");
    StartHotspotWorker(new HotspotRequest{m_hwnd, m_hotspotName, m_password});
}

void ControlPanel::StartHotspotWorker(HotspotRequest* request) {
    std::erase_if(m_workers, [](HANDLE thread) {
        if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0) {
            CloseHandle(thread);
            return true;
        }
        return false;
    });
    if (HANDLE thread = CreateThread(nullptr, 0, HotspotConfigThreadProc,
                                     request, 0, nullptr)) {
        m_workers.push_back(thread);
        m_hotspotBusy = true;
    } else {
        delete request;
        m_hotspotStatus = Tr(L"Couldn't change the hotspot.",
                             L"Não foi possível mudar o hotspot.");
    }
    Layout();
}

// The "Disconnect" button at a device row's end (see DrawBluetooth).
D2D1_RECT_F ControlPanel::BluetoothButtonRect(const D2D1_RECT_F& row) const {
    const float s = m_scale;
    const float height = std::round(26 * s);
    const float middle = (row.top + row.bottom) / 2;
    return {row.right - std::round(102 * s), std::round(middle - height / 2),
            row.right - std::round(8 * s), std::round(middle + height / 2)};
}

void ControlPanel::DrawBluetooth(ID2D1RenderTarget* target,
                                 ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    DrawHeader(target, brush, L"Bluetooth", m_bluetoothRadio);
    const float icon = std::round(36 * s);
    for (size_t i = 0; i < m_bluetoothDevices.size() && i < m_rows.size();
         i++) {
        const BluetoothDevice& device = m_bluetoothDevices[i];
        const D2D1_RECT_F& row = m_rows[i];
        const bool hover =
            m_hover.part == Part::BluetoothRow && m_hover.index == (int)i;
        if (const float lit = Hover(row, hover); lit > 0.01f) {
            const float radius = std::round(10 * s);
            brush->SetColor(Fg(0.08f * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        brush->SetColor(device.connected ? Accent(1) : Fg(0.95f));
        DrawGlyph(target, &device.glyph, 1, m_iconFormat.get(),
                  {row.left, row.top, row.left + icon, row.bottom},
                  brush);
        const float right = row.right - std::round(96 * s);
        brush->SetColor(Fg(0.95f));
        target->DrawText(device.name.c_str(), (UINT32)device.name.size(),
                         m_subtitleFormat.get(),
                         {row.left + icon, row.top, right, row.bottom}, brush);
        const BluetoothStep step =
            device.address == m_bluetoothAddress
                ? LiveBluetoothStep(m_bluetoothStep, m_bluetoothStepAt)
                : BluetoothStep::None;
        // Headphones or speakers connected: a button that disconnects them
        // (as the row's click does).
        if (device.audio && device.connected && step == BluetoothStep::None) {
            PCWSTR text = Tr(L"Disconnect", L"Desconectar");
            const D2D1_RECT_F button = BluetoothButtonRect(row);
            const float round = (button.bottom - button.top) / 2;
            brush->SetColor(Fg(0.10f + 0.08f * Hover(button, hover)));
            GlassFill(target, {button, round, round}, brush);
            brush->SetColor(Fg(0.95f));
            target->DrawText(text, (UINT32)wcslen(text), m_linkFormat.get(),
                             button, brush);
            continue;
        }
        if (PCWSTR status = BluetoothStatusText(device.connected, step)) {
            brush->SetColor(Fg(0.5f));
            target->DrawText(status, (UINT32)wcslen(status),
                             m_linkFormat.get(),
                             {right, row.top, row.right - std::round(8 * s),
                              row.bottom},
                             brush);
        }
    }
    DrawStatus(target, brush);
}

WCHAR VolumeGlyphFor(float level, bool muted) {
    if (muted) {
        return kGlyphMute;
    }
    if (level <= 0.001f) {
        return kGlyphVolume[0];
    }
    return kGlyphVolume[std::clamp((int)(level * 3) + 1, 1, 3)];
}

