#pragma once
// A choice's longest name (with what's drawn before it: a color's dot, a
// language's flag), in the settings' text.
float ControlPanel::ChoiceWidest(int id) const {
    const float s = m_scale;
    const int language = (int)IslandTextLanguage();
    if (m_choiceWidestScale != s || m_choiceWidestLanguage != language ||
        m_choiceWidest.size() != kChoiceCount) {
        m_choiceWidest.assign(kChoiceCount, -1);
        m_choiceWidestScale = s;
        m_choiceWidestLanguage = language;
    }
    // (Not these: the icon pack's last name is the folder chosen, and the
    // backup's last says "Sure?" or "Reset"; they change with no new scale
    // or language.)
    const bool kept = id != kChoiceIconPack && id != kChoiceBackup;
    if (kept && m_choiceWidest[id] >= 0) {
        return m_choiceWidest[id];
    }
    float widest = 0;
    if (m_dwrite && m_linkFormat) {
        for (int i = 0; i < ChoiceCount(id); i++) {
            const std::wstring name = ChoiceOption(id, i);
            winrt::com_ptr<IDWriteTextLayout> layout;
            DWRITE_TEXT_METRICS metrics{};
            if (SUCCEEDED(m_dwrite->CreateTextLayout(
                    name.c_str(), (UINT32)name.size(), m_linkFormat.get(),
                    2000, 100, layout.put())) &&
                SUCCEEDED(layout->GetMetrics(&metrics))) {
                widest = std::max(widest, metrics.width);
            }
        }
    }
    const float before =
        FillChoiceKind(id) >= 0 || id == kChoiceVisualizerColor ? 22
        : id == kChoiceLanguage || id == kChoiceNotesLanguage   ? 30
        : id == kChoiceColors                                   ? 14
                                                                : 0;
    widest = std::ceil(widest) + std::round((24 + before) * s);
    // (Kept only once measured: before its text is made, it's measured again.)
    if (kept && m_dwrite && m_linkFormat) {
        m_choiceWidest[id] = widest;
    }
    return widest;
}

// A choice's segments: each as wide as it was made to be, or as its
// longest name needs (no name cut), in as many rows as they need (at least
// the rows chosen for it, see ChoiceRows). The icon packs' list as it is.
ControlPanel::ChoiceFit ControlPanel::FitChoice(int id, float width) const {
    const float s = m_scale;
    const int count = std::max(1, ChoiceCount(id));
    const float room = std::max(1.0f, width - std::round(8 * s));
    // (The notches' four and the fill's colors: wider, their names are
    // longer; the icon packs' list as wide as it can be.)
    const float made = std::round(
        (id == kChoiceNotches          ? 96
         : id == kChoiceIconPack       ? 480
         : FillChoiceKind(id) >= 0 || id == kChoiceVisualizerColor ? 150
         : id == kChoiceFillStyle || id == kChoiceLyricsColor     ? 140
         : id == kChoiceLanguage || id == kChoiceNotesLanguage    ? 120
                                                                    : 80) *
        s);
    const float need = std::max(
        1.0f, id == kChoiceIconPack ? made : std::max(made, ChoiceWidest(id)));
    const int least = std::max(1, ChoiceRows(id));
    int across = (int)std::floor(room / std::min(need, room));
    across = std::clamp(across, 1, (count + least - 1) / least);
    const int rows = (count + across - 1) / across;
    return {std::min(need, std::floor(room / across)), across, rows};
}

float ControlPanel::ChoiceControlHeight(int id, float width) const {
    if (id == kChoiceBannerPlace) {
        return std::round(132 * m_scale);
    }
    if (PictureChoice(id)) {
        return std::round(84 * m_scale);
    }
    return std::round((40 + 36 * (FitChoice(id, width).rows - 1)) * m_scale);
}

float ControlPanel::SettingHeight(const SettingEntry& entry,
                                  float width) const {
    const float s = m_scale;
    // The name beside its control (as tall as it needs), or over it (with
    // what it does under it, in the window).
    auto beside = [&](float control, float least) {
        const float room =
            width - std::round(36 * s) - std::round(8 * s) - control;
        const float text =
            WrappedHeight(SettingName(entry), m_subtitleFormat.get(), room) +
            DescriptionHeight(entry, room);
        return std::max(std::round(least * s), text + std::round(14 * s));
    };
    switch (entry.kind) {
        case SettingKind::Profiles: {
            // Its chips, three on a line (the profiles; "+" on its title
            // adds one), the one being edited, and a line of help (as tall
            // as its longest).
            // (Another island than the main one: a chip for the main one's
            // settings; on the main one, a line for each other monitor.)
            const bool main = !g_island || g_island->HasMainSettings();
            const size_t chips = g_profiles.size() + (main ? 0 : 1);
            float hint = 0;
            for (int i = 0; i < kProfilesHints; i++) {
                hint = std::max(
                    hint, WrappedHeight(ProfilesHint(i), m_smallFormat.get(),
                                        width - std::round(4 * s)));
            }
            // (Editing on the main one: a line of styles, its apps' lines,
            // and staying or not.)
            const float automatic =
                m_profileEdit >= 0 && main
                    ? 38 * (float)(2 + ProfileAppRows())
                    : 0;
            return std::round((4 + (float)((chips + 2) / 3) * 38 +
                               (m_profileEdit >= 0 ? 80 : 0) + automatic) *
                              s) +
                   std::max(std::round(22 * s), hint + std::round(4 * s));
        }
        case SettingKind::Monitors: {
            // (See LayoutMonitors: where it appears, the map, then with an
            // island on each monitor, a card for each.)
            const float rows = (float)((g_profiles.size() + 3) / 3);
            return std::round(
                (kMonitorsTop +
                 (g_settings.islandAllMonitors
                      ? kMonitorsMainCard +
                            (float)OtherIslands().size() *
                                (kMonitorsCardBase + rows * 38)
                      : 0)) *
                s);
        }
        case SettingKind::Label:
            return std::round(26 * s);
        case SettingKind::Style:
        case SettingKind::Theme:
        case SettingKind::Size:
        case SettingKind::Choice:
            // The name, then its selector or slider under it (a choice's
            // buttons in rows when they don't fit one, see FitChoice).
            return SettingNameHeight(entry, width) +
                   (entry.kind == SettingKind::Choice
                        ? ChoiceControlHeight(entry.value, width)
                        : std::round(40 * s));
        case SettingKind::Place:
            return beside(std::round(60 * s), 44);
        case SettingKind::AppPlaces:
            return beside(std::round(28 * s), 34);
        case SettingKind::Icons: {
            // Its name over its icons' cells, as many on a line as fit.
            const size_t count = IconGroupSize(entry.value);
            const size_t columns = IconCellColumns(width, s);
            return SettingNameHeight(entry, width) +
                   std::round((4 + (float)((count + columns - 1) / columns) *
                                       (kIconCellHeight + kIconCellGap)) *
                              s);
        }
        case SettingKind::LyricsApps: {
            // Its name over its cards, one or two on a line (one line saying
            // there's none yet).
            const size_t columns = LyricsAppsColumns(width, s);
            const size_t rows = std::max<size_t>(
                1, (LyricsApps().size() + columns - 1) / columns);
            return SettingNameHeight(entry, width) +
                   std::round((4 + (float)rows * (kLyricsAppCard + 6)) * s);
        }
        case SettingKind::Order:
            return std::round(66 * s);
        case SettingKind::Pref:
            return beside(std::round(52 * s), 34);
        case SettingKind::Shortcut:
            return beside(std::round(176 * s), 34);
        case SettingKind::Dock:
            return beside(std::round(150 * s), 44);
        case SettingKind::About:
            return beside(std::round(30 * s), 44);
        case SettingKind::Logs:
            return entry.value == 1
                       ? WrappedHeight(m_logsShown, m_smallFormat.get(),
                                       width - std::round(8 * s)) +
                             std::round(12 * s)
                       : beside(std::round(236 * s), 44);
        case SettingKind::WhatsNew:
        case SettingKind::Setup: {
            // (Each item's text leaves room for its option's small arrow,
            // see kNoteArrow.)
            float height = std::round(34 * s);
            bool first = true;
            for (const auto& [line, heading, link] :
                 entry.kind == SettingKind::Setup ? SetupLines()
                                                  : NotesLines(entry.value)) {
                const float text = WrappedHeight(
                    line,
                    heading ? m_sectionFormat.get() : m_subtitleFormat.get(),
                    width - std::round((heading ? 28 : 28 + kNoteArrow) * s));
                height += std::round(NoteGap(heading, first) * s) + text;
                first = heading;
            }
            return height + std::round(10 * s);
        }
        default:
            return std::round(34 * s);
    }
}

// The name over a selector or a slider: across the whole row.
float ControlPanel::SettingNameHeight(const SettingEntry& entry,
                                      float width) const {
    const float s = m_scale;
    const float room = width - std::round(36 * s) - std::round(8 * s);
    const float text =
        WrappedHeight(SettingName(entry), m_subtitleFormat.get(), room) +
        DescriptionHeight(entry, room);
    return std::max(std::round(30 * s), text + std::round(10 * s));
}

std::wstring ControlPanel::SettingName(const SettingEntry& entry) const {
    // (Shared with the other islands: said on theirs.)
    const bool otherIsland = g_island && !g_island->HasMainSettings();
    switch (entry.kind) {
        case SettingKind::Style: {
            const IslandPrefInfo& info = kIslandPrefs[(int)IslandPref::Bar];
            return Tr(info.english, info.portuguese);
        }
        case SettingKind::Theme: {
            const IslandPrefInfo& info = kIslandPrefs[(int)IslandPref::Theme];
            return std::wstring(Tr(info.english, info.portuguese)) +
                   L" \u00B7 " + ThemeName(g_themeIndex);
        }
        case SettingKind::Place:
            return g_settings.islandBar ? Tr(L"Place (the bar is at the top)",
                                             L"Posição (a barra fica no topo)")
                   : NotchCountWanted() > 0
                       ? Tr(L"Place (the notches are at the top)",
                            L"Posição (os entalhes ficam no topo)")
                   : g_settings.islandFreeDrag
                       ? Tr(L"Place (or drag the pill)",
                            L"Posição (ou arraste a pílula)")
                       : Tr(L"Place", L"Posição");
        case SettingKind::LyricsApps:
            return Tr(L"Apps that look for lyrics", L"Apps que buscam letras");
        case SettingKind::Icons:
            return IconGroupName(entry.value);
        case SettingKind::AppPlaces:
            return std::wstring(Tr(L"Place and icons per app",
                                   L"Posição e ícones por app")) +
                   (g_appPlaces.empty()
                        ? std::wstring()
                        : L"  (" + std::to_wstring(g_appPlaces.size()) + L")");
        case SettingKind::Size: {
            const int slider = entry.value;
            const int size = IslandSliderValue(slider);
            WCHAR seconds[16];
            swprintf_s(seconds, L"%.1f s", size / 10.0);
            std::wstring name =
                slider == 18
                    ? std::wstring(Tr(L"Visualizer sensitivity",
                                      L"Sensibilidade do visualizador")) +
                          L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 17
                    ? std::wstring(Tr(L"Grid columns", L"Colunas da grade")) +
                          L" \u00B7 " + std::to_wstring(size)
                : slider == 16
                    ? std::wstring(Tr(L"Glass blur", L"Desfoque do vidro")) +
                          L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 15
                    ? std::wstring(Tr(L"Edge around", L"Borda em volta")) +
                          L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 14
                    ? std::wstring(Tr(L"Room at the pill's ends",
                                      L"Espaço nas pontas da pílula")) +
                          L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 13
                    ? std::wstring(Tr(L"Pill corners", L"Cantos da pílula")) +
                          L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 12
                    ? std::wstring(Tr(L"Pill height", L"Altura da pílula")) +
                          L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 11
                    ? std::wstring(Tr(L"Title width", L"Largura do título")) +
                          L" \u00B7 " + std::to_wstring(size) + L" px"
                : slider == 10
                    ? std::wstring(Tr(L"Panels size", L"Tamanho dos painéis")) +
                          L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 9 ? std::wstring(Tr(L"Area that brings it back",
                                                L"Área que traz de volta")) +
                                    L" \u00B7 " + std::to_wstring(size) + L" px"
                : slider == 8
                    ? std::wstring(Tr(L"Line opacity", L"Opacidade da linha")) +
                          L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 19
                    ? std::wstring(Tr(L"Text size", L"Tamanho dos textos")) +
                          L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 7
                    ? std::wstring(Tr(L"Icon size", L"Tamanho dos ícones")) +
                          L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 6 ? std::wstring(Tr(L"Notch curves",
                                                L"Curvas dos entalhes")) +
                                    L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 5
                    ? std::wstring(Tr(L"Hide after", L"Esconder depois de")) +
                          L" \u00B7 " + seconds
                : slider == 4 ? std::wstring(Tr(L"Opacity", L"Opacidade")) +
                                    L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 3 ? std::wstring(Tr(
                                    L"Other apps' icons before scrolling",
                                    L"Ícones de outros apps antes de rolar")) +
                                    L" \u00B7 " + std::to_wstring(size)
                : slider == 2 ? std::wstring(Tr(L"Speed", L"Velocidade")) +
                                    L" \u00B7 " + std::to_wstring(size) + L"%"
                : slider == 1 ? std::wstring(Tr(L"Edge distance",
                                                L"Distância da borda")) +
                                    L" \u00B7 " + std::to_wstring(size) + L" px"
                              : std::wstring(Tr(L"Size", L"Tamanho")) +
                                    L" \u00B7 " + std::to_wstring(size) + L"%";
            if (slider == 2 && otherIsland) {
                name += Tr(L" (all monitors)", L" (todos os monitores)");
            }
            return name;
        }
        case SettingKind::Choice: {
            const int id = entry.value;
            // Your own colors hard to read, or how the icon pack's download
            // goes: said instead of the name.
            const std::wstring packStatus =
                id == kChoiceIconPack ? IconPackStatus() : std::wstring();
            std::wstring name =
                id == kChoiceAnimationLevel ? Tr(L"How many", L"Quantas")
                : id == kChoiceFrameRate
                    ? Tr(L"Frames a second", L"Quadros por segundo")
                : id == kChoiceBackup
                    ? (m_backupStatus.empty()
                           ? Tr(L"A file with all of it (agenda links too)",
                                L"Um arquivo com tudo (os links das agendas "
                                L"também)")
                           : m_backupStatus.c_str())
                : id == kChoiceColors && CustomColorsHardToRead()
                    ? Tr(L"Hard to read: too alike",
                         L"Difícil de ler: cores parecidas")
                : id == kChoiceColors
                    ? Tr(L"Your own colors", L"Cores próprias")
                : id == kChoiceDateOrder ? Tr(L"Date order", L"Ordem da data")
                : id == kChoiceClockFormat
                    ? Tr(L"Time format", L"Formato da hora")
                : id == kChoiceWeatherUnit
                    ? Tr(L"Temperature unit", L"Unidade de temperatura")
                : id == kChoiceIconPack
                    ? (packStatus.empty()
                           ? Tr(L"Icon pack", L"Pacote de ícones")
                           : packStatus.c_str())
                : id == kChoiceIconFolder
                    ? (!m_iconStatus.empty() ? m_iconStatus.c_str()
                       : g_iconFolder.empty()
                           ? Tr(L"Your folder (none chosen yet)",
                                L"Sua pasta (nenhuma escolhida ainda)")
                           : g_iconFolder.c_str())
                : id == kChoiceIconColors
                    ? Tr(L"Icon colors", L"Cores dos ícones")
                : id == kChoiceTrayStyle
                    ? Tr(L"Icons that aren't pinned", L"Ícones não fixados")
                : id == kChoiceWallpaper ? Tr(L"Wallpaper transition",
                                              L"Transição do papel de parede")
                : id == kChoiceNotches   ? Tr(L"Notches", L"Entalhes")
                : id == kChoiceNotchOpen
                    ? Tr(L"Line that opens the island",
                         L"Linha que abre a ilha")
                : id == kChoiceHideWhen
                    ? Tr(L"When it hides", L"Quando esconder")
                : id == kChoiceLineClick
                    ? Tr(L"Opens the line", L"Abre a linha")
                : id == kChoiceLineFade
                    ? Tr(L"Fade the line after", L"Apagar a linha depois de")
                : id == kChoiceBarSide ? Tr(L"The bar's side", L"Lado da barra")
                : id == kChoiceBarInnerCorners
                    ? Tr(L"The bar's inner corners",
                         L"Cantos de dentro da barra")
                : id == kChoiceBannerSize
                    ? Tr(L"Banner size", L"Tamanho do aviso")
                : id == kChoiceShelfLimit
                    ? Tr(L"Shelf holds", L"A prateleira guarda")
                : id == kChoiceEyeTheme
                    ? Tr(L"Eyes' colors", L"Cores dos olhos")
                : id == kChoiceEyeShape
                    ? Tr(L"Eyes' shape", L"Formato dos olhos")
                : id == kChoiceBannerWidth
                    ? Tr(L"Banner width", L"Largura do aviso")
                : id == kChoiceBannerCorners
                    ? Tr(L"Banner corners", L"Cantos do aviso")
                : id == kChoiceBannerPlace
                    ? Tr(L"Where banners show", L"Onde os avisos aparecem")
                : id == kChoiceFolderDrag
                    ? Tr(L"Files dragged out", L"Arquivos arrastados para fora")
                : id == kChoiceLanguage
                    ? Tr(L"Language of the island's texts (it starts again)",
                         L"Idioma dos textos da ilha (ela reinicia)")
                : id == kChoiceNotesLanguage
                    ? Tr(L"Language of the notes", L"Idioma das notas")
                : id == kChoiceFillColor
                    ? Tr(L"Color for downloads and other apps",
                         L"Cor dos downloads e outros apps")
                : id == kChoiceFillCopyColor
                    ? Tr(L"Color for file copies", L"Cor das cópias de arquivos")
                : id == kChoiceFillChargeColor
                    ? Tr(L"Color for the battery charging",
                         L"Cor da carga da bateria")
                : id == kChoiceFillStyle
                    ? Tr(L"Fill animation", L"Animação do preenchimento")
                : id == kChoiceLyricsSize
                    ? Tr(L"Lyrics size on the pill",
                         L"Tamanho da letra na pílula")
                : id == kChoiceLyricsWidth
                    ? Tr(L"Longest line on the pill",
                         L"Comprimento da linha na pílula")
                : id == kChoiceLyricsColor
                    ? Tr(L"Lyrics color", L"Cor da letra")
                : id == kChoiceVisualizerColor
                    ? Tr(L"Visualizer color", L"Cor do visualizador")
                : id == kChoiceRecycleMinSize
                    ? Tr(L"Recycle Bin appears", L"A lixeira aparece")
                : id == kChoiceSetupStyle ? Tr(L"Style", L"Estilo")
                : id == kChoiceSetupItems
                    ? Tr(L"Items on the pill", L"Itens na pílula")
                : id == kChoiceSetupHide  ? Tr(L"Hiding", L"Esconder")
                : id == kChoiceSetupTheme ? Tr(L"Colors", L"Cores")
                                          : L"";
            if (otherIsland &&
                (id == kChoiceAnimationLevel || id == kChoiceFrameRate ||
                 id == kChoiceLanguage)) {
                name += Tr(L" (all monitors)", L" (todos os monitores)");
            }
            return name;
        }
        case SettingKind::Pref: {
            const IslandPref pref = (IslandPref)entry.value;
            const IslandPrefInfo& info = kIslandPrefs[entry.value];
            std::wstring name = Tr(info.english, info.portuguese);
            if (otherIsland && IsSharedPref(pref) && !MainIslandOnly(pref)) {
                name += Tr(L" (all monitors)", L" (todos os monitores)");
            }
            // (Its keys as recorded, in every language.)
            if (pref == IslandPref::MinimizeKey && name.starts_with(L"Win+J")) {
                name.replace(0, 5, ShortcutText(g_minimizeShortcut, false));
            }
            // (komorebi starting with whkd, which isn't there: it'd fail.)
            // (The glass giving way to the plain color for now.)
            if (pref == IslandPref::LiquidGlass && g_settings.islandLiquidGlass &&
                g_energySaver) {
                name += Tr(L" · plain color while Windows' energy saver is on",
                           L" · cor lisa enquanto a economia de energia do Windows "
                           L"está ligada");
            } else if (pref == IslandPref::LiquidGlass &&
                       g_settings.islandLiquidGlass &&
                       !g_transparencyEffects) {
                name += Tr(L" · plain color: Windows' transparency effects are "
                           L"off (Personalization > Colors)",
                           L" · cor lisa: os efeitos de transparência do "
                           L"Windows estão desligados (Personalização > Cores)");
            }
            if (pref == IslandPref::KomorebiWhkd && g_settings.komorebiWhkd &&
                g_komorebiFound == 1 && g_whkdFound == 0) {
                name += Tr(L" · whkd isn't installed (winget install "
                           L"LGUG2Z.whkd)",
                           L" · o whkd não está instalado (winget install "
                           L"LGUG2Z.whkd)");
            }
            return name;
        }
        case SettingKind::Dock:
            return Tr(L"The dock is set up in Windhawk",
                      L"A dock é configurada no Windhawk");
        case SettingKind::Setup: {
            static constexpr PCWSTR kTitles[kSetupSteps][2] = {
                {L"Welcome to Mac Island & Dock",
                 L"Boas-vindas ao Mac Island & Dock"},
                {L"Style", L"Estilo"},
                {L"Items on the pill", L"Itens na pílula"},
                {L"Hiding and notifications", L"Esconder e notificações"},
                {L"Look and dock", L"Aparência e dock"},
                {L"All set", L"Tudo pronto"},
            };
            const int step = std::clamp(m_setup.step, 0, kSetupSteps - 1);
            return Tr(kTitles[step][0], kTitles[step][1]);
        }
        case SettingKind::About:
            switch (entry.value) {
                case 0:
                    return std::wstring(L"Mac Island & Dock " WH_MOD_VERSION) +
                           Tr(L" by caliberda", L" por caliberda");
                case 1:
                    return std::wstring(Tr(L"Website", L"Site")) +
                           L" · caliberda.com.br";
                case 2:
                    return std::wstring(Tr(L"Bugs and ideas", L"Bugs e ideias")) +
                           L" · GitHub";
                case 4:
                    return std::wstring(Tr(L"The mod's site", L"Site do mod")) +
                           L" · island.caliberda.com.br";
                default:
                    return L"Instagram · @cesar.kali";
            }
        case SettingKind::Logs:
            if (entry.value == 1) {
                return {};
            }
            return std::wstring(Tr(L"The island's logs", L"Os logs da ilha")) +
                   (m_logsStatus.empty() ? L"" : L" · " + m_logsStatus);
        case SettingKind::WhatsNew: {
            if (entry.value < 0 || entry.value >= kReleaseNotesCount) {
                return {};
            }
            const std::wstring version = kReleaseNotes[entry.value].version;
            return L"Mac Island & Dock " + version +
                   (version == std::wstring(L"" WH_MOD_VERSION)
                        ? L" \u00B7 " + std::wstring(Tr(L"installed",
                                                         L"instalada"))
                        : std::wstring());
        }
        case SettingKind::Shortcut: {
            std::wstring name = Tr(L"Keys", L"Teclas");
            if (m_recordingShortcut) {
                name += Tr(L" · press the new keys (Esc cancels)",
                           L" · pressione as novas teclas (Esc cancela)");
            }
            if (!m_shortcutNote.empty()) {
                name += L" · " + m_shortcutNote;
            }
            return name;
        }
        default:
            return {};
    }
}

// What an option does, in a line under its name (in the window only, see
// LayoutSettingsWindow), like Windows' Settings.
std::wstring ControlPanel::SettingDescription(const SettingEntry& entry) const {
    if (!m_windowed) {
        return {};
    }
    struct Description {
        SettingKind kind;
        int value;
        PCWSTR english;
        PCWSTR portuguese;
    };
    static const Description kDescriptions[] = {
        {SettingKind::Profiles, 0,
         L"Switches, adds, renames, updates or deletes your profiles",
         L"Troca, adiciona, renomeia, atualiza ou exclui seus perfis"},
        {SettingKind::Choice, kChoiceBackup,
         L"Saves a backup file or loads one; Reset puts every setting back "
         L"to its default (clicked twice)",
         L"Salva um arquivo de backup ou carrega um; Redefinir volta todas as "
         L"configurações ao padrão (dois cliques)"},
        {SettingKind::Choice, kChoiceLanguage,
         L"Applies to every monitor, and the island restarts to apply it",
         L"Vale para todos os monitores, e a ilha reinicia para aplicar"},
        {SettingKind::Place, 0,
         L"Picks the screen spot for the pill, on a grid",
         L"Escolhe o ponto da tela para a pílula, em uma grade"},
        {SettingKind::LyricsApps, 0,
         L"The apps seen playing; a click turns one's lyrics off, and "
         L"nothing of its songs is sent to lrclib.net",
         L"Os apps vistos tocando; um clique desliga as letras de um, e nada "
         L"das músicas dele é enviado ao lrclib.net"},
        {SettingKind::AppPlaces, 0,
         L"Gives chosen apps their own spot and icons (Island style only)",
         L"Dá a apps escolhidos um lugar e ícones próprios (só no estilo "
         L"Ilha)"},
        {SettingKind::Pref, (int)IslandPref::FreeDrag,
         L"Drag the pill to any spot on the screen (not with notches)",
         L"Arraste a pílula para qualquer ponto da tela (não com entalhes)"},
        {SettingKind::Pref, (int)IslandPref::VerticalSides,
         L"Stands the pill up on the left and right edges (not with notches)",
         L"Põe a pílula em pé nas bordas esquerda e direita (não com "
         L"entalhes)"},
        {SettingKind::Pref, (int)IslandPref::AutoHide,
         L"Shrinks the pill to a thin line on the screen edge when idle",
         L"Reduz a pílula a uma linha fina na borda da tela quando parada"},
        {SettingKind::Choice, kChoiceLineClick,
         L"With a click, the line only grows under the mouse and opens when "
         L"clicked (the hidden Bar's clock too)",
         L"Com um clique, a linha só cresce sob o mouse e abre quando "
         L"clicada (o relógio da Barra escondida também)"},
        {SettingKind::Choice, kChoiceLineFade,
         L"For OLED screens: the minimized line fades away after this long, "
         L"back when the mouse reaches it",
         L"Para telas OLED: a linha minimizada some depois desse tempo e "
         L"volta quando o mouse chega nela"},
        {SettingKind::Choice, kChoiceHideWhen,
         L"Hides when the mouse leaves, or only while a window is maximized "
         L"or over it",
         L"Esconde quando o mouse sai, ou só enquanto uma janela está "
         L"maximizada ou por cima"},
        {SettingKind::Pref, (int)IslandPref::HideClock,
         L"Hidden, a small pill with the time stays instead of the line (the "
         L"line while a window is under it). The Bar hidden with Win+J closes "
         L"into it too",
         L"Escondida, fica uma pílula pequena com a hora no lugar da linha (a "
         L"linha enquanto uma janela está embaixo dela). A Barra escondida com "
         L"Win+J também vira essa pílula"},
        {SettingKind::Size, 5,
         L"Seconds the pill waits after the mouse leaves before it hides",
         L"Segundos que a pílula espera após o mouse sair antes de esconder"},
        {SettingKind::Size, 8,
         L"How visible the thin line is when the pill is hidden",
         L"Quão visível é a linha fina quando a pílula está escondida"},
        {SettingKind::Size, 9,
         L"Width of the edge zone where the mouse brings the pill back",
         L"Largura da zona na borda em que o mouse traz a pílula de volta"},
        {SettingKind::Pref, (int)IslandPref::MinimizeButton,
         L"Adds a small arrow at the pill's end that hides it to its line",
         L"Põe uma setinha na ponta da pílula que a esconde na linha"},
        {SettingKind::Size, 1,
         L"Gap between the pill and the screen edge (not with the bar or "
         L"notches)",
         L"Espaço entre a pílula e a borda da tela (não com a barra ou "
         L"entalhes)"},
        {SettingKind::Style, 0,
         L"Pill floats, Fixed keeps its room on the screen, Bar spans the "
         L"edge",
         L"Pílula flutua, Fixa guarda seu espaço na tela, Barra ocupa a borda"},
        {SettingKind::Choice, kChoiceNotches,
         L"Splits the island into notches against the top edge, around the "
         L"camera; the Island and Fixed styles each keep their own",
         L"Divide a ilha em entalhes na borda de cima, em volta da câmera; os "
         L"estilos Ilha e Fixa guardam cada um o seu"},
        {SettingKind::Choice, kChoiceNotchOpen,
         L"With the island hidden, only this notch's line shows and opens it",
         L"Com a ilha escondida, só a linha deste entalhe aparece e a abre"},
        {SettingKind::Size, 6,
         L"How round the notch curves are in the Fixed style",
         L"O quanto as curvas dos entalhes são arredondadas no estilo Fixa"},
        {SettingKind::Pref, (int)IslandPref::NotchEach,
         L"Each notch hides alone instead of all at once (Fixed only)",
         L"Cada entalhe se esconde sozinho em vez de todos juntos (só estilo "
         L"Fixa)"},
        {SettingKind::Theme, 0,
         L"Picks a color theme from the swatches",
         L"Escolhe um tema de cores entre as amostras"},
        {SettingKind::Choice, kChoiceColors,
         L"Opens pickers for the fill, text and icon colors",
         L"Abre seletores para as cores de fundo, texto e ícones"},
        {SettingKind::Pref, (int)IslandPref::WindowColor,
         L"The island takes the color of the app in front",
         L"A ilha assume a cor do app que está na frente"},
        {SettingKind::Size, 4,
         L"How solid the island's fill is, from see-through to full",
         L"Quão sólido é o fundo da ilha, de transparente a total"},
        {SettingKind::Pref, (int)IslandPref::LiquidGlass,
         L"See-through blurred pill with a bright edge, needs Windows "
         L"transparency",
         L"Pílula transparente e desfocada com borda de luz, precisa da "
         L"transparência do Windows"},
        {SettingKind::Pref, (int)IslandPref::GlassTranslucent,
         L"Lighter tint and livelier colors on the glass",
         L"Tinta mais leve e cores mais vivas no vidro"},
        {SettingKind::Size, 16,
         L"How strongly the glass blurs what is behind it",
         L"O quanto o vidro desfoca o que está atrás"},
        {SettingKind::Pref, (int)IslandPref::Frosted,
         L"Frosted look for the bar and the Fixed strip, needs Windows "
         L"transparency",
         L"Aparência fosca na barra e na faixa Fixa, precisa da transparência "
         L"do Windows"},
        {SettingKind::Pref, (int)IslandPref::BarClear,
         L"Only the bar's items over what is behind it; with the frosted look, "
         L"only the blur",
         L"Só os itens da barra sobre o que está atrás; com a aparência "
         L"fosca, só o desfoque"},
        {SettingKind::Size, 12,
         L"Makes the pill taller or shorter (not with the bar)",
         L"Deixa a pílula mais alta ou mais baixa (não com a barra)"},
        {SettingKind::Size, 13,
         L"From square to fully round corners (not with the bar or notches)",
         L"De cantos retos a totalmente redondos (não com a barra ou "
         L"entalhes)"},
        {SettingKind::Size, 14,
         L"Padding before the first item and after the last one",
         L"Margem antes do primeiro item e depois do último"},
        {SettingKind::Size, 15,
         L"How visible the thin outline around the pill is",
         L"Quão visível é o contorno fino em volta da pílula"},
        {SettingKind::Size, 0,
         L"Scales the pill, icons and panels together",
         L"Dimensiona a pílula, os ícones e os painéis juntos"},
        {SettingKind::Size, 7,
         L"Scales only the icons on the pill (their room too)",
         L"Dimensiona só os ícones da pílula (o espaço deles também)"},
        {SettingKind::Size, 19,
         L"Scales only the texts on the pill: the clock, percentages and "
         L"titles",
         L"Dimensiona só os textos da pílula: o relógio, os percentuais e "
         L"os títulos"},
        {SettingKind::Choice, kChoiceIconPack,
         L"Windows, Windows 10 and Emoji are Windows' own fonts. The online "
         L"ones (all free to use: MIT, Apache 2.0 or ISC) go online once, "
         L"when chosen: about 150 small SVG pictures are downloaded from "
         L"cdn.jsdelivr.net (a plain request for each file, nothing about you "
         L"or your PC is sent), kept in the mod's folder, and never "
         L"downloaded again. An icon a pack hasn't stays Windows'. Your "
         L"folder: a picture (PNG, SVG, ICO...) per icon, named after it",
         L"Windows, Windows 10 e Emoji são fontes do próprio Windows. Os "
         L"online (todos de uso livre: MIT, Apache 2.0 ou ISC) usam a "
         L"internet uma vez, quando escolhidos: cerca de 150 imagens SVG "
         L"pequenas são baixadas de cdn.jsdelivr.net (um pedido simples por "
         L"arquivo, nada sobre você ou o seu PC é enviado), guardadas na "
         L"pasta do mod e nunca baixadas de novo. Um ícone que o pacote não "
         L"tem fica o do Windows. Sua pasta: uma imagem (PNG, SVG, ICO...) "
         L"por ícone, com o nome dele"},
        {SettingKind::Choice, kChoiceIconColors,
         L"Like the island: only the shapes of the emojis and of your "
         L"pictures, in the island's colors. Their own: as they are",
         L"Como a ilha: só o formato dos emojis e das suas imagens, nas "
         L"cores da ilha. As próprias: como são"},
        {SettingKind::Choice, kChoiceIconFolder,
         L"Save Windows' icons puts a picture of every icon in the folder, "
         L"named after it (wifi.png, battery-5.png...): replace the ones you "
         L"want, and they show a moment later",
         L"Salvar ícones do Windows coloca na pasta uma imagem de cada "
         L"ícone, com o nome dele (wifi.png, battery-5.png...): troque as que "
         L"quiser e elas aparecem logo depois"},
        {SettingKind::Icons, (int)IconGroup::Status,
         L"Wi-Fi, sound, battery, the bell and the clock, each step of them. "
         L"Click an icon to use a picture of your own for it, its brush (top "
         L"left) for a color of its own; the x goes back to the pack's",
         L"Wi-Fi, som, bateria, o sino e o relógio, cada etapa deles. Clique "
         L"num ícone para usar uma imagem sua nele, no pincel (em cima à "
         L"esquerda) para uma cor só dele; o x volta ao do pacote"},
        {SettingKind::Icons, (int)IconGroup::Media,
         L"What's playing's buttons and pictures",
         L"Os botões e as imagens do que está tocando"},
        {SettingKind::Icons, (int)IconGroup::Controls,
         L"The control center's buttons",
         L"Os botões da central de controle"},
        {SettingKind::Icons, (int)IconGroup::Pill,
         L"The pill's other items", L"Os outros itens da pílula"},
        {SettingKind::Icons, (int)IconGroup::Buttons,
         L"The buttons on the panels", L"Os botões dos painéis"},
        {SettingKind::Icons, (int)IconGroup::Other,
         L"The settings' own icons",
         L"Os ícones das próprias configurações"},
        {SettingKind::Size, 10,
         L"Scales the control center, calendar and other panels",
         L"Dimensiona a central de controle, o calendário e outros painéis"},
        {SettingKind::Pref, (int)IslandPref::PanelsFollow,
         L"Panels open as wide as the island is",
         L"Os painéis abrem com a mesma largura da ilha"},
        {SettingKind::Pref, (int)IslandPref::Tips,
         L"Shows small bubbles with names and app notifications on hover",
         L"Mostra balões com nomes e notificações de apps ao passar o mouse"},
        {SettingKind::Pref, (int)IslandPref::HoverHighlight,
         L"Lights up the item the mouse is over",
         L"Acende o item em que o mouse está"},
        {SettingKind::Pref, (int)IslandPref::Animations,
         L"Turns the island's animations on or off, on every monitor",
         L"Liga ou desliga as animações da ilha, em todos os monitores"},
        {SettingKind::Choice, kChoiceAnimationLevel,
         L"Chooses how many animations play, on every monitor",
         L"Escolhe quantas animações tocam, em todos os monitores"},
        {SettingKind::Size, 2,
         L"Speeds up or slows down the animations, on every monitor",
         L"Acelera ou desacelera as animações, em todos os monitores"},
        {SettingKind::Choice, kChoiceFrameRate,
         L"Smoothness of the animations, matching the screen or a fixed rate",
         L"Fluidez das animações, igual à tela ou em uma taxa fixa"},
        {SettingKind::Order, 0,
         L"Drag the groups to reorder them, the minimize arrow stays last",
         L"Arraste os grupos para reordenar, a seta de minimizar fica por "
         L"último"},
        {SettingKind::Pref, (int)IslandPref::ClockCenter,
         L"Puts the clock in the middle with the other items around it",
         L"Põe o relógio no meio com os outros itens em volta"},
        {SettingKind::Pref, (int)IslandPref::BarCorners,
         L"In the bar, sends items to the screen corners, clock in the middle",
         L"Na barra, leva os itens aos cantos da tela, com o relógio no meio"},
        {SettingKind::Choice, kChoiceBarSide,
         L"The bar at the top, or upright on the left or the right edge, its "
         L"items one under the other; windows keep out of its strip",
         L"A barra no topo, ou em pé na borda esquerda ou direita, com os "
         L"itens um embaixo do outro; as janelas ficam fora da faixa dela"},
        {SettingKind::Choice, kChoiceBarInnerCorners,
         L"Rounds the corners where the bar meets the screen, like a "
         L"screen's own round corners",
         L"Arredonda os cantos onde a barra encontra a tela, como os cantos "
         L"redondos da própria tela"},
        {SettingKind::Pref, (int)IslandPref::Network,
         L"Shows the control center's icon, click for Wi-Fi, Bluetooth, "
         L"volume and more",
         L"Mostra o ícone da central de controle, clique para Wi-Fi, "
         L"Bluetooth, volume e mais"},
        {SettingKind::Pref, (int)IslandPref::Volume,
         L"Shows the volume icon, click for the volume panel",
         L"Mostra o ícone de volume, clique para abrir o painel"},
        {SettingKind::Pref, (int)IslandPref::VolumePercent,
         L"Writes the volume level as a number next to its icon",
         L"Escreve o nível do volume como número ao lado do ícone"},
        {SettingKind::Pref, (int)IslandPref::Battery,
         L"Shows the battery icon on the pill",
         L"Mostra o ícone da bateria na pílula"},
        {SettingKind::Pref, (int)IslandPref::BatteryPercent,
         L"Writes the charge level as a number next to its icon",
         L"Escreve o nível de carga como número ao lado do ícone"},
        {SettingKind::Pref, (int)IslandPref::Brightness,
         L"Shows the screen brightness, scroll the wheel on it to change",
         L"Mostra o brilho da tela, role a roda sobre ele para mudar"},
        {SettingKind::Pref, (int)IslandPref::Microphone,
         L"Adds a button that mutes and unmutes the microphone",
         L"Adiciona um botão que silencia e reativa o microfone"},
        {SettingKind::Pref, (int)IslandPref::Privacy,
         L"Shows a mark and the app name while the mic, camera or location is "
         L"used",
         L"Mostra uma marca e o nome do app enquanto microfone, câmera ou "
         L"localização estão em uso"},
        {SettingKind::Pref, (int)IslandPref::InputMethod,
         L"Shows the current keyboard language, like the Windows indicator",
         L"Mostra o idioma atual do teclado, como o indicador do Windows"},
        {SettingKind::Pref, (int)IslandPref::ThemeButton,
         L"Adds a button that switches Windows between light and dark",
         L"Adiciona um botão que alterna o Windows entre claro e escuro"},
        {SettingKind::Pref, (int)IslandPref::Power,
         L"Adds a button for sleep, restart and shut down",
         L"Adiciona um botão para suspender, reiniciar e desligar"},
        {SettingKind::Pref, (int)IslandPref::Bell,
         L"Adds the notifications bell to the pill",
         L"Adiciona o sino de notificações à pílula"},
        {SettingKind::Pref, (int)IslandPref::Desktops,
         L"Shows the virtual desktops, scroll the wheel on it to switch",
         L"Mostra as áreas de trabalho virtuais, role a roda sobre ele para "
         L"trocar"},
        {SettingKind::Pref, (int)IslandPref::FocusedApp,
         L"Shows the icon and name of the app in focus on this monitor",
         L"Mostra o ícone e o nome do app em foco neste monitor"},
        {SettingKind::Pref, (int)IslandPref::Folders,
         L"Opens a panel with your folder and place shortcuts",
         L"Abre um painel com seus atalhos de pastas e locais"},
        {SettingKind::Choice, kChoiceFolderDrag,
         L"Files dragged out of the folders panel or the shelf into an app: "
         L"copied or moved, or asked (the first time, remembered, or every "
         L"time)",
         L"Arquivos arrastados do painel de pastas ou da prateleira para um "
         L"app: copiados ou movidos, ou perguntado (na primeira vez, e "
         L"lembrado, ou sempre)"},
        {SettingKind::Pref, (int)IslandPref::Wallpaper,
         L"Cycles through the pictures of a folder as your wallpaper",
         L"Alterna as imagens de uma pasta como papel de parede"},
        {SettingKind::Choice, kChoiceWallpaper,
         L"The effect shown when the wallpaper changes",
         L"O efeito mostrado quando o papel de parede muda"},
        {SettingKind::Pref, (int)IslandPref::UsbDrives,
         L"Shows an icon to open or eject plugged-in USB drives",
         L"Mostra um ícone para abrir ou ejetar pendrives conectados"},
        {SettingKind::Pref, (int)IslandPref::UsbNotify,
         L"Shows a notice each time a USB drive is plugged in",
         L"Mostra um aviso toda vez que um pendrive é conectado"},
        {SettingKind::Pref, (int)IslandPref::RecycleBin,
         L"Adds the Recycle Bin to the pill",
         L"Adiciona a Lixeira à pílula"},
        {SettingKind::Choice, kChoiceRecycleMinSize,
         L"Always, or only once what's in it weighs this much (it comes and "
         L"goes by itself)",
         L"Sempre, ou só quando o que tem nela pesar isso (ela aparece e some "
         L"sozinha)"},
        {SettingKind::Pref, (int)IslandPref::Notes,
         L"Adds a button that opens your quick notes",
         L"Adiciona um botão que abre suas anotações rápidas"},
        {SettingKind::Pref, (int)IslandPref::Tasks,
         L"Adds a button that opens your to-do list",
         L"Adiciona um botão que abre sua lista de tarefas"},
        {SettingKind::Pref, (int)IslandPref::Shortcuts,
         L"Adds a button that lists the keyboard shortcuts",
         L"Adiciona um botão que lista os atalhos do teclado"},
        {SettingKind::Pref, (int)IslandPref::Weather,
         L"Shows the weather, it needs an internet connection",
         L"Mostra o clima, precisa de conexão com a internet"},
        {SettingKind::Choice, kChoiceWeatherUnit,
         L"Follows Windows' measurement system or forces Celsius or Fahrenheit",
         L"Segue o sistema de medidas do Windows ou força Celsius ou "
         L"Fahrenheit"},
        {SettingKind::Pref, (int)IslandPref::Updates,
         L"Shows pending app and Windows updates from winget and Windows "
         L"Update",
         L"Mostra atualizações pendentes de apps e do Windows pelo winget e "
         L"Windows Update"},
        {SettingKind::Pref, (int)IslandPref::Profiles,
         L"Adds a button to switch profiles from the pill",
         L"Adiciona um botão para trocar de perfil pela pílula"},
        {SettingKind::Pref, (int)IslandPref::UsageCpu,
         L"Shows how busy the processor is",
         L"Mostra o quanto o processador está ocupado"},
        {SettingKind::Pref, (int)IslandPref::UsageRam,
         L"Shows how much memory is in use",
         L"Mostra quanta memória está em uso"},
        {SettingKind::Pref, (int)IslandPref::UsageVram,
         L"Shows how much video memory is in use",
         L"Mostra quanta memória de vídeo está em uso"},
        {SettingKind::Pref, (int)IslandPref::NetSpeed,
         L"Shows the current upload and download speed",
         L"Mostra a velocidade atual de envio e recebimento"},
        {SettingKind::Pref, (int)IslandPref::Timer,
         L"Adds a timer, a stopwatch and a Pomodoro to the pill",
         L"Adiciona timer, cronômetro e Pomodoro à pílula"},
        {SettingKind::Pref, (int)IslandPref::Visualizer,
         L"Shows bars that move with the sound that is playing",
         L"Mostra barras que se mexem com o som que está tocando"},
        {SettingKind::Choice, kChoiceVisualizerColor,
         L"The bars' color; the rainbow gives each bar its own, going around "
         L"slowly",
         L"A cor das barras; o arco-íris dá a cada barra a sua, girando "
         L"devagar"},
        {SettingKind::Pref, (int)IslandPref::VisualizerSmooth,
         L"Each bar sways on its own, softly, as loud as the sound; off, "
         L"they jump with the beat",
         L"Cada barra balança sozinha, suave, conforme o volume; desligado, "
         L"elas pulam com a batida"},
        {SettingKind::Pref, (int)IslandPref::ActionNetwork,
         L"Puts a one-click Wi-Fi and network button on the pill",
         L"Põe na pílula um botão de Wi-Fi e rede de um clique"},
        {SettingKind::Pref, (int)IslandPref::ActionBluetooth,
         L"Puts a one-click Bluetooth button on the pill",
         L"Põe na pílula um botão de Bluetooth de um clique"},
        {SettingKind::Pref, (int)IslandPref::ActionNightLight,
         L"Puts a one-click Night light button on the pill",
         L"Põe na pílula um botão de Luz noturna de um clique"},
        {SettingKind::Pref, (int)IslandPref::ActionHotspot,
         L"Puts a one-click Mobile hotspot button on the pill",
         L"Põe na pílula um botão de Hotspot móvel de um clique"},
        {SettingKind::Pref, (int)IslandPref::ActionAirplane,
         L"Puts a one-click Airplane mode button on the pill",
         L"Põe na pílula um botão de Modo avião de um clique"},
        {SettingKind::Pref, (int)IslandPref::ActionProject,
         L"Puts a button on the pill to choose the display mode",
         L"Põe na pílula um botão para escolher o modo de exibição"},
        {SettingKind::Pref, (int)IslandPref::ActionCast,
         L"Puts a button on the pill to cast the screen",
         L"Põe na pílula um botão para transmitir a tela"},
        {SettingKind::Pref, (int)IslandPref::ActionCaptions,
         L"Puts a button on the pill that turns live captions on",
         L"Põe na pílula um botão que ativa as legendas ao vivo"},
        {SettingKind::Pref, (int)IslandPref::ActionAccessibility,
         L"Puts a button on the pill that opens the accessibility options",
         L"Põe na pílula um botão que abre as opções de acessibilidade"},
        {SettingKind::Pref, (int)IslandPref::ActionScreenshot,
         L"Puts a button on the pill that takes a screenshot",
         L"Põe na pílula um botão que captura a tela"},
        {SettingKind::Pref, (int)IslandPref::ActionLock,
         L"Puts a button on the pill that locks the PC",
         L"Põe na pílula um botão que bloqueia o PC"},
        {SettingKind::Pref, (int)IslandPref::ActionEmoji,
         L"Puts a button on the pill that opens the emoji panel",
         L"Põe na pílula um botão que abre o painel de emojis"},
        {SettingKind::Pref, (int)IslandPref::ActionClipboard,
         L"Puts a button on the pill that opens the clipboard history",
         L"Põe na pílula um botão que abre o histórico da área de "
         L"transferência"},
        {SettingKind::Pref, (int)IslandPref::ActionTaskManager,
         L"Puts a button on the pill that opens Task Manager",
         L"Põe na pílula um botão que abre o Gerenciador de Tarefas"},
        {SettingKind::Pref, (int)IslandPref::ActionCalculator,
         L"Puts a button on the pill that opens Calculator",
         L"Põe na pílula um botão que abre a Calculadora"},
        {SettingKind::Pref, (int)IslandPref::ActionExplorer,
         L"Puts a button on the pill that opens File Explorer",
         L"Põe na pílula um botão que abre o Explorador de Arquivos"},
        {SettingKind::Pref, (int)IslandPref::ActionShowDesktop,
         L"Puts a button on the pill that shows the desktop",
         L"Põe na pílula um botão que mostra a área de trabalho"},
        {SettingKind::Pref, (int)IslandPref::ActionOnScreenKeyboard,
         L"Puts a button on the pill that opens the on-screen keyboard",
         L"Põe na pílula um botão que abre o teclado virtual"},
        {SettingKind::Pref, (int)IslandPref::ActionKomorebi,
         L"Puts a button on the pill to start, stop or reload komorebi",
         L"Põe na pílula um botão para iniciar, parar ou recarregar o "
         L"komorebi"},
        {SettingKind::Pref, (int)IslandPref::KomorebiWhkd,
         L"The komorebi buttons also start and stop whkd, its shortcut helper",
         L"Os botões do komorebi também iniciam e param o whkd, o ajudante de "
         L"atalhos"},
        {SettingKind::Pref, (int)IslandPref::KomorebiMasir,
         L"The komorebi buttons also start and stop masir, so focus follows "
         L"the mouse",
         L"Os botões do komorebi também iniciam e param o masir, e o foco "
         L"segue o mouse"},
        {SettingKind::Pref, (int)IslandPref::KomorebiLayout,
         L"Shows komorebi's workspaces and layout on the pill, a menu changes "
         L"it",
         L"Mostra as áreas e o layout do komorebi na pílula, um menu o troca"},
        {SettingKind::Pref, (int)IslandPref::KomorebiHideEmpty,
         L"Leaves out komorebi workspaces that have no windows",
         L"Deixa de fora as áreas do komorebi sem janelas"},
        {SettingKind::Pref, (int)IslandPref::Tray,
         L"Adds a way to reach the tray icons of other apps from the pill",
         L"Adiciona um jeito de acessar os ícones da bandeja de outros apps "
         L"pela pílula"},
        {SettingKind::Choice, kChoiceTrayStyle,
         L"Opens the unpinned icons as a list in the pill, a grid or a named "
         L"list",
         L"Abre os ícones não fixados como lista na pílula, grade ou lista "
         L"com nomes"},
        {SettingKind::Pref, (int)IslandPref::TrayOpen,
         L"The list stays open after you click an icon (In the pill style)",
         L"A lista continua aberta depois de clicar em um ícone (estilo Na "
         L"pílula)"},
        {SettingKind::Size, 3,
         L"How many icons show before the list starts to scroll",
         L"Quantos ícones aparecem antes de a lista começar a rolar"},
        {SettingKind::Size, 18,
         L"How much the bars move with the sound (they follow the playing "
         L"app's own sound)",
         L"O quanto as barras se mexem com o som (elas seguem o som do app "
         L"que está tocando)"},
        {SettingKind::Size, 17,
         L"Number of columns in the icon grid (Grid style only)",
         L"Número de colunas da grade de ícones (só no estilo Grade)"},
        {SettingKind::Pref, (int)IslandPref::Media,
         L"Shows the playing song or video with its cover and controls",
         L"Mostra a música ou o vídeo tocando com capa e controles"},
        {SettingKind::Pref, (int)IslandPref::MediaButtons,
         L"Puts previous, play and next buttons right on the pill",
         L"Põe os botões de anterior, tocar e próxima direto na pílula"},
        {SettingKind::Pref, (int)IslandPref::MediaTitle,
         L"Shows the cover and the song title on the pill",
         L"Mostra a capa e o título da música na pílula"},
        {SettingKind::Size, 11,
         L"Longest width the song title may take on the pill",
         L"Maior largura que o título da música pode ocupar na pílula"},
        {SettingKind::Pref, (int)IslandPref::MediaMarquee,
         L"A title too long to fit scrolls across while it plays",
         L"Um título longo demais rola enquanto a música toca"},
        {SettingKind::Pref, (int)IslandPref::MediaWheel,
         L"Scrolling on the player skips songs instead of changing the volume",
         L"Rolar sobre o player troca de música em vez de mudar o volume"},
        {SettingKind::Pref, (int)IslandPref::Clock,
         L"Shows the time on the pill",
         L"Mostra a hora na pílula"},
        {SettingKind::Choice, kChoiceClockFormat,
         L"Follows Windows or forces a 24 hour or 12 hour clock",
         L"Segue o Windows ou força relógio de 24 ou de 12 horas"},
        {SettingKind::Pref, (int)IslandPref::ClockSeconds,
         L"Adds the seconds to the time",
         L"Acrescenta os segundos à hora"},
        {SettingKind::Pref, (int)IslandPref::ClockWeekday,
         L"Adds the weekday to the clock",
         L"Acrescenta o dia da semana ao relógio"},
        {SettingKind::Pref, (int)IslandPref::ClockDate,
         L"Adds the day and the month to the clock",
         L"Acrescenta o dia e o mês ao relógio"},
        {SettingKind::Pref, (int)IslandPref::ClockYear,
         L"Adds the year to the clock",
         L"Acrescenta o ano ao relógio"},
        {SettingKind::Choice, kChoiceDateOrder,
         L"Sets the order of day, month and year when the date shows",
         L"Define a ordem de dia, mês e ano quando a data aparece"},
        {SettingKind::Pref, (int)IslandPref::MonthNumber,
         L"Writes the month as a number, like 9/10, not as a name",
         L"Escreve o mês como número, como 9/10, e não como nome"},
        {SettingKind::Pref, (int)IslandPref::Holidays,
         L"Marks holidays in the calendar, it needs an internet connection",
         L"Marca os feriados no calendário, precisa de conexão com a internet"},
        {SettingKind::Pref, (int)IslandPref::WorldClocks,
         L"Shows the extra clocks you added in Windows",
         L"Mostra os relógios adicionais que você incluiu no Windows"},
        {SettingKind::Pref, (int)IslandPref::Countdown,
         L"Shows how long until the next event, alarm, timer or Pomodoro part",
         L"Mostra quanto falta para o próximo evento, alarme, timer ou parte "
         L"do Pomodoro"},
        {SettingKind::Pref, (int)IslandPref::AiUsage,
         L"How much of the AI agents' limits is used and when each starts "
         L"over, read on this PC (no login and nothing online)",
         L"Quanto dos limites dos agentes de IA já foi usado e quando cada "
         L"um recomeça, lido neste PC (sem login e nada online)"},
        {SettingKind::Pref, (int)IslandPref::AiClaude,
         L"What Claude Code in a terminal gives its status line (a line in "
         L"its script, see the item's panel). The Claude desktop app doesn't "
         L"leave its limits on this PC, so it can't show here",
         L"O que o Claude Code no terminal passa para a linha de status dele "
         L"(uma linha no script dela, veja o painel do item). O app de "
         L"desktop do Claude não deixa os limites neste PC, então ele não "
         L"aparece aqui"},
        {SettingKind::Pref, (int)IslandPref::AiCodex,
         L"What Codex writes in its logs on this PC",
         L"O que o Codex escreve nos logs dele neste PC"},
        {SettingKind::Pref, (int)IslandPref::FocusedTitle,
         L"Also shows the title of the window in focus, like the page or the "
         L"file open in it",
         L"Mostra também o título da janela em foco, como a página ou o "
         L"arquivo aberto nela"},
        {SettingKind::Pref, (int)IslandPref::WeatherCards,
         L"The weather panel shows feels like, humidity, wind, UV, rain, "
         L"pressure and clouds as small cards, like a phone's",
         L"O painel do clima mostra sensação, umidade, vento, UV, chuva, "
         L"pressão e nuvens em cartões pequenos, como no celular"},
        {SettingKind::Pref, (int)IslandPref::CalendarBigDay,
         L"The calendar opens as a card: today big at its left, the month "
         L"beside it, buttons for the alarms, the timer and your agendas, and "
         L"the day's events",
         L"O calendário abre como um cartão: o dia de hoje grande à esquerda, "
         L"o mês ao lado, botões para os alarmes, o timer e suas agendas, e "
         L"os eventos do dia"},
        {SettingKind::Pref, (int)IslandPref::Commands,
         L"Shortcuts to programs and files (or command lines) you add, with "
         L"their own icons, a click away in their panel; any of them can be a "
         L"button on the pill",
         L"Atalhos para programas e arquivos (ou linhas de comando) que você "
         L"adiciona, com os ícones deles, a um clique no painel; qualquer um "
         L"pode virar um botão na pílula"},
        {SettingKind::Pref, (int)IslandPref::DndSilent,
         L"In the island's Do not disturb, calls, alarms and reminders don't "
         L"ring either (they still show on the pill)",
         L"No Não incomodar da ilha, chamadas, alarmes e lembretes também não "
         L"tocam (continuam aparecendo na pílula)"},
        {SettingKind::Pref, (int)IslandPref::DownloadStalled,
         L"Browsers never tell the taskbar a download is paused or failed: "
         L"one whose bar stands still for 30 s turns yellow",
         L"Os navegadores nunca avisam a barra de tarefas que um download "
         L"pausou ou falhou: um com a barra parada por 30 s fica amarelo"},
        {SettingKind::Pref, (int)IslandPref::FolderThumbnails,
         L"Pictures, videos and documents show a small preview, as in "
         L"Explorer, in the folders panel and on the shelf",
         L"Fotos, vídeos e documentos mostram uma prévia pequena, como no "
         L"Explorador, no painel de pastas e na prateleira"},
        {SettingKind::Pref, (int)IslandPref::Shelf,
         L"Drop files on the pill to keep them there, then drag them out into "
         L"another app later (their places are kept, not copies)",
         L"Solte arquivos na pílula para guardá-los ali e depois arraste para "
         L"outro app (guarda onde eles estão, não cópias)"},
        {SettingKind::Pref, (int)IslandPref::Eyes,
         L"Two dragon eyes on the pill: they follow the mouse when it is "
         L"near, blink, glance around and get excited with a notification",
         L"Dois olhos de dragão na pílula: seguem o mouse quando ele está "
         L"perto, piscam, olham em volta e se animam com uma notificação"},
        {SettingKind::Choice, kChoiceEyeTheme,
         L"The eyes' colors",
         L"As cores dos olhos"},
        {SettingKind::Choice, kChoiceEyeShape,
         L"The eyes' shape",
         L"O formato dos olhos"},
        {SettingKind::Pref, (int)IslandPref::TimerLine,
         L"While a timer, Pomodoro or the stopwatch runs, the line becomes a "
         L"small pill with its ring and its time (with Only the clock, the "
         L"time goes beside the clock)",
         L"Enquanto um timer, o Pomodoro ou o cronômetro corre, a linha vira "
         L"uma pílula pequena com o anel e o tempo (com Só o relógio, o tempo "
         L"fica ao lado da hora)"},
        {SettingKind::Pref, (int)IslandPref::ExternalBrightness,
         L"Their sliders talk to the monitor over its cable. Turn off if a "
         L"monitor flickers or a dock or MST hub resets when the control "
         L"center opens",
         L"Os controles deles falam com o monitor pelo cabo. Desligue se um "
         L"monitor piscar ou se uma dock ou hub MST reiniciar ao abrir a "
         L"central de controle"},
        {SettingKind::Pref, (int)IslandPref::GlassLight,
         L"The glass gets a sheen, a beveled edge, light along its bottom and "
         L"a soft shadow around it, and the panels' cards look like glass too "
         L"(idea by Thiago Silva)",
         L"O vidro ganha brilho, borda chanfrada, luz na parte de baixo e uma "
         L"sombra suave em volta, e os cartões dos painéis também parecem "
         L"vidro (ideia de Thiago Silva)"},
        {SettingKind::Pref, (int)IslandPref::CountdownEvents,
         L"Your events and your agendas' events",
         L"Seus eventos e os das suas agendas"},
        {SettingKind::Pref, (int)IslandPref::CountdownAlarms,
         L"The next alarm that's on",
         L"O próximo alarme ligado"},
        {SettingKind::Pref, (int)IslandPref::CountdownTimer,
         L"While the timer runs",
         L"Enquanto o timer está rodando"},
        {SettingKind::Pref, (int)IslandPref::CountdownPomodoro,
         L"Its focus or break ending, while it runs",
         L"O fim do foco ou da pausa, enquanto ele roda"},
        {SettingKind::Pref, (int)IslandPref::DownloadFill,
         L"The progress apps show on their taskbar button (downloads, copies, installs)",
         L"O progresso que os apps mostram no botão da barra de tarefas (downloads, cópias, instalações)"},
        {SettingKind::Pref, (int)IslandPref::ChargeFill,
         L"On a laptop, the battery's percentage as it charges",
         L"Num notebook, a porcentagem da bateria enquanto carrega"},
        {SettingKind::Choice, kChoiceFillColor,
         L"Downloads, installs and other apps' progress; red for an error, "
         L"yellow paused",
         L"Downloads, instalações e o progresso dos outros apps; vermelho "
         L"com erro, amarelo pausado"},
        {SettingKind::Choice, kChoiceFillCopyColor,
         L"Files copied or moved by Windows (its copy window)",
         L"Arquivos copiados ou movidos pelo Windows (a janela de cópia)"},
        {SettingKind::Choice, kChoiceFillChargeColor,
         L"The battery's charge while a laptop charges",
         L"A carga da bateria enquanto o notebook carrega"},
        {SettingKind::Pref, (int)IslandPref::Lyrics,
         L"The line sung now on the pill (place it in the order above), all "
         L"of them in its panel. Each song's title and artist are sent to "
         L"lrclib.net (songs only: not videos, voice messages or the apps "
         L"turned off below)",
         L"A linha cantada agora na pílula (posicione na ordem acima), todas "
         L"no painel dela. O título e o artista de cada música são enviados "
         L"ao lrclib.net (só músicas: não vídeos, áudios de voz nem os apps "
         L"desligados abaixo)"},
        {SettingKind::Choice, kChoiceLyricsSize,
         L"How big the line is on the pill",
         L"O tamanho da linha na pílula"},
        {SettingKind::Choice, kChoiceLyricsWidth,
         L"\"Follows the line\": as wide as each line; the others keep the "
         L"widest line's width. A longer line ends with \u2026",
         L"\"Segue a linha\": da largura de cada linha; as outras mantêm a "
         L"largura da linha mais larga. Uma linha maior termina em \u2026"},
        {SettingKind::Choice, kChoiceLyricsColor,
         L"The line's color, on the pill and lit in the panel",
         L"A cor da linha, na pílula e destacada no painel"},
        {SettingKind::Pref, (int)IslandPref::LyricsInTitle,
         L"While a line is sung, it takes the title and artist's place; "
         L"between lines they come back",
         L"Enquanto uma linha é cantada, ela fica no lugar do nome e do "
         L"artista; entre as linhas eles voltam"},
        {SettingKind::Choice, kChoiceFillStyle,
         L"How it moves; paused it breathes, an error shakes, done flashes "
         L"with a check, canceled drains back",
         L"Como ele se mexe; pausado respira, erro treme, concluído brilha "
         L"com um visto, cancelado escorre de volta"},
        {SettingKind::Pref, (int)IslandPref::OwnNotifications,
         L"The island shows the notifications and plays their sounds, Windows "
         L"stays quiet",
         L"A ilha mostra as notificações e toca os sons, o Windows fica "
         L"quieto"},
        {SettingKind::Pref, (int)IslandPref::ShortcutNotifications,
         L"Pressing Win+N opens the island's notifications instead of "
         L"Windows'",
         L"Apertar Win+N abre as notificações da ilha em vez das do Windows"},
        {SettingKind::Pref, (int)IslandPref::ShortcutControl,
         L"Pressing Win+A opens the island's control center instead of "
         L"Windows'",
         L"Apertar Win+A abre a central de controle da ilha em vez da do "
         L"Windows"},
        {SettingKind::Pref, (int)IslandPref::MinimizeKey,
         L"The keys below send the island to its line, and again bring it "
         L"back",
         L"As teclas abaixo levam a ilha para a linha, e de novo a trazem de "
         L"volta"},
        {SettingKind::Shortcut, 0,
         L"Shows the minimize keys, click them to record a new combination",
         L"Mostra as teclas de minimizar, clique nelas para gravar outra "
         L"combinação"},
        {SettingKind::Pref, (int)IslandPref::CapsLock,
         L"A bubble on the island says when Caps Lock is turned on or off",
         L"Um balão na ilha avisa quando o Caps Lock é ligado ou desligado"},
        {SettingKind::Pref, (int)IslandPref::StatusNotices,
         L"Shows notices for the charger, low battery, Bluetooth audio and Do "
         L"not disturb",
         L"Mostra avisos do carregador, bateria fraca, áudio Bluetooth e Não "
         L"incomodar"},
        {SettingKind::Pref, (int)IslandPref::FoldApps,
         L"Folds the app icons into an arrow, they come out for new ones or "
         L"on hover",
         L"Recolhe os ícones dos apps numa seta, que saem com novidades ou ao "
         L"passar o mouse"},
        {SettingKind::Pref, (int)IslandPref::Banner,
         L"Shows a banner on the pill when a notification arrives",
         L"Mostra um aviso na pílula quando chega uma notificação"},
        {SettingKind::Pref, (int)IslandPref::BannerMinimized,
         L"Banners still appear while the pill is minimized to its line",
         L"Os avisos continuam aparecendo com a pílula minimizada na linha"},
        {SettingKind::Choice, kChoiceBannerSize,
         L"How big the banner is, its texts and buttons with it",
         L"O tamanho do aviso, com os textos e botões dele"},
        {SettingKind::Choice, kChoiceShelfLimit,
         L"How many files it keeps: the oldest goes when a new one comes",
         L"Quantos arquivos ela guarda: o mais antigo sai quando chega um "
         L"novo"},
        {SettingKind::Choice, kChoiceBannerWidth,
         L"How wide the banner is (a longer text fits on its lines)",
         L"A largura do aviso (um texto maior cabe nas linhas dele)"},
        {SettingKind::Choice, kChoiceBannerCorners,
         L"How round the banner's corners are",
         L"O quanto os cantos do aviso são arredondados"},
        {SettingKind::Choice, kChoiceBannerPlace,
         L"\"Following the bell\": out of the pill, as now; or at a point of "
         L"the screen, sliding in from its nearest edge",
         L"\"Seguindo o sino\": saindo da pílula, como hoje; ou num ponto da "
         L"tela, entrando pela borda mais perto"},
        {SettingKind::Pref, (int)IslandPref::HideWindowsBanners,
         L"Stops Windows' own banners so you only see the island's",
         L"Impede os avisos do próprio Windows para você ver só os da ilha"},
        {SettingKind::Pref, (int)IslandPref::WebPictures,
         L"Fetches pictures for banners from the web, it needs an internet "
         L"connection",
         L"Busca imagens dos avisos na web, precisa de conexão com a internet"},
        {SettingKind::Pref, (int)IslandPref::NotificationApps,
         L"Shows an icon on the pill for each app with notifications",
         L"Mostra na pílula um ícone para cada app com notificações"},
        {SettingKind::Pref, (int)IslandPref::GroupApps,
         L"Merges an app's notifications into one icon, Phone Link's phone "
         L"apps included",
         L"Reúne as notificações de um app em um ícone, inclusive os apps do "
         L"celular do Vincular ao Celular"},
        {SettingKind::Pref, (int)IslandPref::Clipboard,
         L"Shows a bubble under the pill with what you just copied",
         L"Mostra um balão sob a pílula com o que você acabou de copiar"},
        {SettingKind::Pref, (int)IslandPref::ScreenshotFolder,
         L"Clicking a screenshot notice opens the picture in Explorer",
         L"Clicar no aviso de captura abre a imagem no Explorador"},
        {SettingKind::Dock, 0,
         L"Its style, size, hiding and animations are in Windhawk's settings "
         L"for this mod",
         L"Estilo, tamanho, esconder e animações ficam nas configurações do "
         L"mod no Windhawk"},
        {SettingKind::About, 0,
         L"Made in Brazil, new versions bring fixes and what users ask for",
         L"Feito no Brasil, as versões novas trazem correções e o que os "
         L"usuários pedem"},
        {SettingKind::About, 4,
         L"Every option explained step by step, the roadmap and your votes",
         L"Cada opção explicada passo a passo, o roadmap e os seus votos"},
        {SettingKind::About, 1,
         L"My site, with my other projects and a Discord link",
         L"Meu site, com meus outros projetos e um link do Discord"},
        {SettingKind::About, 2,
         L"Report a bug or suggest an idea, GitHub tells you when it comes out",
         L"Relate um bug ou sugira uma ideia, o GitHub avisa quando sair"},
        {SettingKind::About, 3, L"To talk to me and follow my projects",
         L"Para falar comigo e acompanhar meus projetos"},
        {SettingKind::Pref, (int)IslandPref::WhatsNewNotice,
         L"Once per version, a notice from the pill opens this page",
         L"Uma vez por versão, um aviso da pílula abre esta página"},
        {SettingKind::Choice, kChoiceNotesLanguage,
         L"Only for the notes below; the island keeps its language",
         L"Só para as notas abaixo; a ilha mantém o idioma dela"},
        {SettingKind::Logs, 0,
         L"A summary of the PC, the island's settings and its last 300 log "
         L"lines (the dock's are in Windhawk's log)",
         L"Um resumo do PC, as configurações da ilha e as últimas 300 linhas "
         L"de log dela (as da dock ficam no log do Windhawk)"},
        {SettingKind::Monitors, 0,
         L"Chooses which monitors show the island, with a map of your screens",
         L"Escolhe quais monitores mostram a ilha, com um mapa das suas telas"},
    };
    for (const auto& description : kDescriptions) {
        if (description.kind == entry.kind && description.value == entry.value) {
            return Tr(description.english, description.portuguese);
        }
    }
    return {};
}

float ControlPanel::DescriptionHeight(const SettingEntry& entry,
                                      float width) const {
    float height = 0;
    for (const std::wstring& text :
         {SettingDescription(entry), SettingReason(entry)}) {
        if (!text.empty()) {
            height += std::round(2 * m_scale) +
                      WrappedHeight(text, m_smallFormat.get(), width);
        }
    }
    return height;
}

std::vector<ControlPanel::NoteLine> ControlPanel::NotesLines(int index) {
    std::vector<NoteLine> lines;
    if (index < 0 || index >= kReleaseNotesCount) {
        return lines;
    }
    const int language =
        std::clamp(g_notesLanguage >= 0 ? g_notesLanguage
                                        : (int)IslandTextLanguage(),
                   0, kTextLanguages - 1);
    std::wstring_view text = kReleaseNotes[index].text[language];
    while (!text.empty()) {
        const size_t end = std::min(text.find(L'\n'), text.size());
        std::wstring_view line = text.substr(0, end);
        const bool heading = !line.empty() && line[0] == L'#';
        if (heading) {
            line.remove_prefix(1);
        }
        SettingTarget link{SettingKind::Label, -1};
        if (const size_t close = line.find(L'}');
            !line.empty() && line[0] == L'{' && close != line.npos) {
            link = NoteTarget(line.substr(1, close - 1));
            line.remove_prefix(close + 1);
        }
        // (An item with its option is a button, see kNoteArrow.)
        if (!line.empty()) {
            lines.push_back({std::wstring(line), heading, link});
        }
        text.remove_prefix(std::min(end + 1, text.size()));
    }
    return lines;
}

// The option a "What's new" item tells about, by the key in braces at its
// start (the same in every language).
ControlPanel::SettingTarget ControlPanel::NoteTarget(std::wstring_view key) {
    auto pref = [](IslandPref option) {
        return SettingTarget{SettingKind::Pref, (int)option};
    };
    auto choice = [](int id) { return SettingTarget{SettingKind::Choice, id}; };
    static const std::pair<std::wstring_view, SettingTarget> kKeys[] = {
        {L"IconPack", choice(kChoiceIconPack)},
        {L"HideWhen", choice(kChoiceHideWhen)},
        {L"HideClock", pref(IslandPref::HideClock)},
        {L"Dock", {SettingKind::Dock, 0}},
        {L"Setup", {SettingKind::Setup, 0}},
        {L"LineFade", choice(kChoiceLineFade)},
        {L"LineClick", choice(kChoiceLineClick)},
        {L"BarSide", choice(kChoiceBarSide)},
        {L"DndSilent", pref(IslandPref::DndSilent)},
        {L"BannerSize", choice(kChoiceBannerSize)},
        {L"BannerPlace", choice(kChoiceBannerPlace)},
        {L"Shelf", pref(IslandPref::Shelf)},
        {L"Style", {SettingKind::Style, 0}},
        {L"FolderDrag", choice(kChoiceFolderDrag)},
        {L"Lyrics", pref(IslandPref::Lyrics)},
        {L"Timer", pref(IslandPref::Timer)},
        {L"Visualizer", pref(IslandPref::Visualizer)},
        {L"NetSpeed", pref(IslandPref::NetSpeed)},
        {L"WorldClocks", pref(IslandPref::WorldClocks)},
        {L"Countdown", pref(IslandPref::Countdown)},
        {L"RecycleBin", pref(IslandPref::RecycleBin)},
        {L"DownloadFill", pref(IslandPref::DownloadFill)},
        {L"StatusNotices", pref(IslandPref::StatusNotices)},
        {L"Language", choice(kChoiceLanguage)},
        {L"Folders", pref(IslandPref::Folders)},
        {L"Glass", pref(IslandPref::LiquidGlass)},
        {L"PillShape", {SettingKind::Size, 12}},
        {L"BarCorners", pref(IslandPref::BarCorners)},
        {L"Updates", pref(IslandPref::Updates)},
        {L"IconSize", {SettingKind::Size, 7}},
        {L"OwnNotifications", pref(IslandPref::OwnNotifications)},
        {L"FolderThumbnails", pref(IslandPref::FolderThumbnails)},
        {L"TimerLine", pref(IslandPref::TimerLine)},
        {L"GlassLight", pref(IslandPref::GlassLight)},
        {L"ExternalBrightness", pref(IslandPref::ExternalBrightness)},
        {L"DownloadStalled", pref(IslandPref::DownloadStalled)},
        {L"NotchOpen", choice(kChoiceNotchOpen)},
        {L"Notches", choice(kChoiceNotches)},
        {L"TextSize", {SettingKind::Size, 19}},
        {L"BarClear", pref(IslandPref::BarClear)},
        {L"WeatherUnit", choice(kChoiceWeatherUnit)},
        {L"CapsLock", pref(IslandPref::CapsLock)},
        {L"MinimizeKey", pref(IslandPref::MinimizeKey)},
        {L"AiUsage", pref(IslandPref::AiUsage)},
        {L"FocusedTitle", pref(IslandPref::FocusedTitle)},
        {L"Commands", pref(IslandPref::Commands)},
        {L"WeatherCards", pref(IslandPref::WeatherCards)},
        {L"CalendarBigDay", pref(IslandPref::CalendarBigDay)},
        {L"Profiles", {SettingKind::Profiles, 0}},
        {L"Reset", choice(kChoiceBackup)},
    };
    for (const auto& [name, target] : kKeys) {
        if (name == key) {
            return target;
        }
    }
    return {SettingKind::Label, -1};
}

// The app last in front, to add to profile `id` (none: empty), from the
// island whose settings are open.
std::wstring AutoProfileAppToAdd(int id) {
    const HWND app = g_island ? g_island->LastAppWindow() : nullptr;
    if (!app || !IsWindow(app)) {
        return {};
    }
    const std::wstring appId = WindowAppKey(app);
    return appId.empty() || AutoProfileHas(id, L'a', appId) ? std::wstring()
                                                            : appId;
}

int ControlPanel::ProfileAppRows() const {
    const size_t count = AutoProfileApps(m_profileEdit).size() +
                         (AutoProfileAppToAdd(m_profileEdit).empty() ? 0 : 1);
    return (int)((count + 2) / 3);
}

PCWSTR ControlPanel::ProfilesHint(int hint) {
    switch (hint) {
        case 0:
            return Tr(
                L"This monitor's profile: changes here apply to every "
                L"monitor using it",
                L"Perfil deste monitor: as mudanças aqui valem para todo "
                L"monitor que usa ele");
        case 1:
            return Tr(
                L"Same as main: changes here apply to it too · + creates "
                L"this monitor's profile",
                L"Igual à principal: as mudanças aqui valem para ela "
                L"também · + cria o perfil deste monitor");
        case 2:
            return Tr(
                L"+ saves the current settings as a profile, to switch "
                L"back to them later.",
                L"+ salva as configurações atuais como um perfil, para "
                L"voltar a elas depois.");
        case 3:
            return Tr(L"Saved \u2713", L"Salvo \u2713");
        case 4:
            return Tr(L"No profile in use: click one to apply it",
                      L"Nenhum perfil em uso: clique num para aplicar");
        default:
            return Tr(
                L"Changes are saved in the one in use · the pencil "
                L"renames, saves or deletes",
                L"As mudanças ficam salvas no perfil em uso · o lápis "
                L"renomeia, salva ou exclui");
    }
}

PCWSTR SettingSectionTitle(int section, bool english) {
    static const PCWSTR kTitles[kSectionCount][2] = {
        {L"Profiles", L"Perfis"},
        {L"Backup (notes, tasks, profiles and settings)",
         L"Backup (notas, tarefas, perfis e configurações)"},
        {L"Language", L"Idioma"},
        {L"Style (the options of the style chosen under it)",
         L"Estilo (as opções do estilo escolhido embaixo dele)"},
        {L"Place", L"Posição"},
        {L"Hiding: when and how it becomes the line",
         L"Esconder: quando e como ela vira a linha"},
        {L"Colors", L"Cores"},
        {L"Glass", L"Vidro"},
        {L"The pill's shape", L"Forma da pílula"},
        {L"Sizes", L"Tamanhos"},
        {L"The Bar style", L"O estilo Barra"},
        {L"Details", L"Detalhes"},
        {L"Icons", L"Ícones"},
        {L"Animations", L"Animações"},
        {L"System", L"Sistema"},
        {L"The pill as a loading bar", L"A pílula como barra de carregamento"},
        {L"Desktop", L"Área de trabalho"},
        {L"Apps and tools", L"Apps e ferramentas"},
        {L"Control center buttons on the pill", L"Botões da central na pílula"},
        {L"Other apps' tray icons", L"Ícones dos outros apps"},
        {L"What's playing", L"O que está tocando"},
        {L"Clock and calendar (agendas: the calendar's gear)",
         L"Relógio e calendário (agendas: engrenagem do calendário)"},
        {L"Notifications", L"Notificações"},
        {L"Banners (notifications on the pill)",
         L"Avisos (as notificações na pílula)"},
        {L"Monitors", L"Monitores"},
        {L"Dock", L"Dock"},
        {L"About", L"Sobre"},
        {L"Setup assistant", L"Assistente de configuração"},
        {L"What's new", L"Novidades"},
        {L"Logs", L"Logs"},
    };
    return kTitles[std::clamp(section, 0, kSectionCount - 1)][english ? 0 : 1];
}

PCWSTR SettingTabTitle(int tab) {
    static const PCWSTR kTitles[kTabCount][2] = {
        {L"General", L"Geral"},
        {L"Style", L"Estilo"},
        {L"Place", L"Posição"},
        {L"Hiding", L"Esconder"},
        {L"Look", L"Aparência"},
        {L"Icons", L"Ícones"},
        {L"Animations", L"Animações"},
        {L"System", L"Sistema"},
        {L"Desktop", L"Área de trabalho"},
        {L"Apps and tools", L"Apps e ferramentas"},
        {L"Control center", L"Central de controle"},
        {L"Tray icons", L"Ícones da bandeja"},
        {L"What's playing", L"O que está tocando"},
        {L"Clock and calendar", L"Relógio e calendário"},
        {L"Notifications", L"Notificações"},
        {L"Banners", L"Avisos"},
        {L"Monitors", L"Monitores"},
        {L"Dock", L"Dock"},
        {L"About", L"Sobre"},
        {L"Setup assistant", L"Assistente de configuração"},
        {L"What's new", L"Novidades"},
        {L"Logs", L"Logs"},
    };
    const int i = std::clamp(tab, 0, kTabCount - 1);
    return Tr(kTitles[i][0], kTitles[i][1]);
}

