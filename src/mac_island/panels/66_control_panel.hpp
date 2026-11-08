// What finds an entry: its names in both languages, a few words for what it
// does, and its section's title.
std::wstring ControlPanel::SettingText(const SettingEntry& entry) const {
    // (In Chinese too, when the island's texts are: see Tr.)
    std::wstring text = std::wstring(SettingSectionTitle(entry.section, true)) +
                        L" " + SettingSectionTitle(entry.section, false) +
                        L" " +
                        Tr(SettingSectionTitle(entry.section, true),
                           SettingSectionTitle(entry.section, false));
    switch (entry.kind) {
        case SettingKind::Label:
        case SettingKind::Setup:
            break;
        case SettingKind::Style:
            text += L" Style pill fixed keep room reserve maximized bar Estilo "
                    L"pílula fixa reservar espaço maximizadas barra";
            break;
        case SettingKind::Theme:
            text +=
                L" Colors theme black graphite light accent glass auto dark "
                L"mode inverted own custom Cores tema preto grafite claro "
                L"destaque vidro automático escuro modo invertido próprias "
                L"personalizadas";
            break;
        case SettingKind::Size:
            text += entry.value == 18
                        ? L" Visualizer sensitivity bars sound audio level "
                          L"Sensibilidade do visualizador barras som áudio"
                    : entry.value == 16
                        ? L" Glass blur blurred frosted strength Liquid "
                          L"Desfoque do vidro desfocado fosco força"
                    : entry.value == 15
                        ? L" Edge around border outline line glow rim "
                          L"Borda em volta contorno linha brilho"
                    : entry.value == 14
                        ? L" Room at the pill's ends width wide narrow "
                          L"Espaço nas pontas da pílula largura larga "
                          L"estreita"
                    : entry.value == 13
                        ? L" Pill corners round square radius Cantos da "
                          L"pílula redondos quadrados"
                    : entry.value == 12
                        ? L" Pill height thin fat tall Altura da pílula "
                          L"fina gorda alta"
                    : entry.value == 11
                        ? L" Title width length song scroll What's playing "
                          L"Largura do título comprimento música rolar"
                    : entry.value == 10
                        ? L" Panels size settings text zoom bigger 4K "
                          L"Tamanho dos painéis configurações texto maior"
                    : entry.value == 9
                        ? L" Area that brings it back activation hot corner "
                          L"line auto-hide Área que traz de volta ativação "
                          L"canto linha"
                    : entry.value == 8
                        ? L" Line opacity transparent invisible auto-hide "
                          L"Opacidade da linha transparente invisível"
                    : entry.value == 19
                        ? L" Text size texts font clock date percentage "
                          L"small big Tamanho dos textos fonte relógio data "
                          L"percentual pequenos grandes"
                    : entry.value == 7
                        ? L" Icon size icons symbols buttons small big "
                          L"Tamanho dos ícones símbolos botões pequenos "
                          L"grandes"
                    : entry.value == 6
                        ? L" Notch curves round corners MacBook Curvas dos "
                          L"entalhes cantos redondos"
                    : entry.value == 5
                        ? L" Hide after seconds delay auto-hide line Esconder "
                          L"depois segundos linha"
                    : entry.value == 4
                        ? L" Opacity transparency transparent see-through "
                          L"Opacidade transparência transparente"
                    : entry.value == 3
                        ? L" Tray icons list scroll how many Ícones da "
                          L"bandeja lista rolar quantos"
                    : entry.value == 2
                        ? L" Animation speed faster slower Velocidade das "
                          L"animações mais rápido devagar"
                    : entry.value
                        ? L" Distance from the edges gap margin room space "
                          L"Distância das bordas espaço margem"
                        : L" Size bigger smaller Tamanho maior menor";
            break;
        case SettingKind::Place:
            text += L" Place where position Posição onde lugar";
            break;
        case SettingKind::Choice:
            text += entry.value == kChoiceAnimationLevel
                        ? L" Level how many all partial none slower computer "
                          L"Nível quantas todas parciais nenhuma computador "
                          L"lento"
                    : entry.value == kChoiceFrameRate
                        ? L" Frame rate limit FPS Limite de quadros por "
                          L"segundo"
                    : entry.value == kChoiceBackup
                        ? L" Export import backup file save restore Exportar "
                          L"importar arquivo salvar restaurar cópia"
                    : entry.value == kChoiceColors
                        ? L" Your own colors custom background text icons "
                          L"Cores próprias personalizadas fundo texto ícones"
                    : entry.value == kChoiceDateOrder
                        ? L" Date order day month year Ordem da data dia mês "
                          L"ano"
                    : entry.value == kChoiceClockFormat
                        ? L" Time format 24 12 hours AM PM Formato da hora "
                          L"horas"
                    : entry.value == kChoiceWeatherUnit
                        ? L" Temperature unit Celsius Fahrenheit weather "
                          L"Unidade de temperatura clima"
                    : entry.value == kChoiceIconPack
                        ? L" Icon pack icons theme emoji Ionicons Lucide "
                          L"iPhone Mac online download folder Pacote de "
                          L"ícones tema baixar pasta"
                    : entry.value == kChoiceIconColors
                        ? L" Icon colors tint original monochrome Cores dos "
                          L"ícones originais"
                    : entry.value == kChoiceIconFolder
                        ? L" Icon folder own pictures custom SVG PNG save "
                          L"Pasta de ícones imagens próprias salvar"
                    : entry.value == kChoiceTrayStyle
                        ? L" Tray icons hidden list grid names Ícones "
                          L"bandeja ocultos lista grade nomes"
                    : entry.value == kChoiceWallpaper
                        ? L" Wallpaper transition fade circle slide zoom "
                          L"Papel de parede transição esmaecer círculo "
                          L"deslizar"
                    : entry.value == kChoiceNotchOpen
                        ? L" Line opens island notch left middle right "
                          L"Linha abre ilha entalhe esquerda meio direita"
                    : entry.value == kChoiceLineClick
                        ? L" Line click open mouse hover pill Linha clique "
                          L"abrir mouse pílula"
                    : entry.value == kChoiceLineFade
                        ? L" Fade line OLED burn-in screen minimized Apagar "
                          L"linha tela queimar minimizada"
                    : entry.value == kChoiceBarSide
                        ? L" Bar vertical side left right upright Barra "
                          L"vertical lado esquerda direita em pé"
                    : entry.value == kChoiceBarInnerCorners
                        ? L" Bar corners round radius Barra cantos "
                          L"arredondados raio"
                    : entry.value == kChoiceShelfLimit
                        ? L" Shelf files how many limit Prateleira arquivos "
                          L"quantos limite"
                    : entry.value == kChoiceEyeTheme ||
                            entry.value == kChoiceEyeShape
                        ? L" Eyes dragon Toothless color shape Olhos dragão "
                          L"Banguela cor formato"
                    : entry.value == kChoiceBannerSize ||
                            entry.value == kChoiceBannerWidth ||
                            entry.value == kChoiceBannerCorners ||
                            entry.value == kChoiceBannerPlace
                        ? L" Banner notification size width corners place "
                          L"position screen Aviso notificação tamanho "
                          L"largura cantos lugar posição tela"
                    : entry.value == kChoiceHideWhen
                        ? L" When hides smart auto-hide maximized window over "
                          L"covering Quando esconder automático maximizada "
                          L"janela por cima cobrindo"
                    : entry.value == kChoiceNotches
                        ? L" Notches notch MacBook corners middle three Fixed "
                          L"Entalhes entalhe cantos meio três Fixa"
                    : entry.value == kChoiceFolderDrag
                        ? L" Folders drag files copy move ask Pastas arrastar "
                          L"arquivos copiar mover perguntar"
                    : entry.value == kChoiceLanguage
                        ? L" Language English Portuguese Chinese German "
                          L"Spanish Russian Japanese Vietnamese translation "
                          L"Idioma inglês português chinês alemão espanhol "
                          L"russo japonês vietnamita tradução 语言 Sprache "
                          L"Язык 言語 Tiếng Việt ngôn ngữ"
                    : FillChoiceKind(entry.value) >= 0
                        ? L" Fill color progress download copy charge green "
                          L"blue accent customize Cor preenchimento progresso "
                          L"cópia carga verde azul destaque personalizar"
                    : entry.value == kChoiceFillStyle
                        ? L" Fill animation liquid wave glow particles simple "
                          L"Animação preenchimento onda líquida brilho "
                          L"partículas simples"
                    : entry.value == kChoiceLyricsSize ||
                            entry.value == kChoiceLyricsColor ||
                            entry.value == kChoiceLyricsWidth
                        ? L" Lyrics song words size color karaoke Letra "
                          L"música tamanho cor karaokê"
                    : entry.value == kChoiceVisualizerColor
                        ? L" Visualizer audio bars color rainbow accent "
                          L"customize Visualizador áudio barras cor arco-íris "
                          L"destaque personalizar"
                    : entry.value == kChoiceRecycleMinSize
                        ? L" Recycle Bin size MB GB always Lixeira tamanho "
                          L"peso sempre"
                        : L"";
            break;
        case SettingKind::AppPlaces:
            text += L" Place and icons per app Posição e ícones por app";
            break;
        case SettingKind::Icons:
            text += L" Icons pictures own Ícones imagens próprias";
            for (const auto& icon : kIcons) {
                if ((int)icon.group == entry.value) {
                    text += L" " + std::wstring(icon.name);
                }
            }
            break;
        case SettingKind::LyricsApps:
            text += L" Lyrics apps songs lrclib Letras apps músicas";
            for (const auto& app : LyricsApps()) {
                text += L" " + AppDisplayName(app.appId);
            }
            break;
        case SettingKind::Monitors:
            text +=
                L" Monitors screens follow mouse all profile notifications "
                L"Monitor telas segue mouse todos perfil notificações";
            break;
        case SettingKind::Profiles:
            text += L" Profiles save apply rename delete Perfis salvar aplicar "
                    L"renomear excluir app style estilo automatic "
                    L"automático";
            for (const auto& profile : g_profiles) {
                text += L" " + profile.name;
            }
            break;
        case SettingKind::Order:
            text += L" Order of the items move Ordem dos itens mover";
            for (const auto& info : kPillGroupInfo) {
                text += std::wstring(L" ") + info.english + L" " +
                        info.portuguese + L" " +
                        Tr(info.english, info.portuguese);
            }
            break;
        case SettingKind::Pref: {
            const IslandPrefInfo& info = kIslandPrefs[entry.value];
            text += std::wstring(L" ") + info.english + L" " + info.portuguese +
                    L" " + Tr(info.english, info.portuguese);
            break;
        }
        case SettingKind::Dock:
            text += L" Dock Windhawk taskbar barra de tarefas";
            break;
        case SettingKind::About:
            text += L" About version author Instagram GitHub site bugs ideas "
                    L"Sobre versão autor ideias " +
                    SettingName(entry);
            break;
        case SettingKind::Logs:
            text += L" Logs log diagnostic copy save registro diagnóstico "
                    L"copiar salvar";
            break;
        case SettingKind::WhatsNew:
            text += L" What's new changelog version notes Novidades versão "
                    L"notas " +
                    SettingName(entry);
            for (const auto& note : NotesLines(entry.value)) {
                text += L" " + note.text;
            }
            break;
        case SettingKind::Shortcut:
            text += L" Keys shortcut minimize record Win+J Teclas atalho "
                    L"minimizar gravar " +
                    ShortcutText(g_minimizeShortcut) + L" " +
                    Tr(L"Keys", L"Teclas");
            break;
    }
    return text;
}

ControlPanel::OrderChip* ControlPanel::FindOrderChip(PillGroup group,
                                                    int strip) {
    for (auto& chip : m_orderChips) {
        if (chip.group == group && chip.strip == strip) {
            return &chip;
        }
    }
    return nullptr;
}

std::vector<PillGroup> ControlPanel::StripOrder(int strip) const {
    return strip >= 0 && strip < (int)g_appPlaces.size()
               ? PlaceOrder(g_appPlaces[strip])
               : g_pillOrder;
}

bool ControlPanel::StripShows(int strip, PillGroup group) const {
    if (strip < 0 || strip >= (int)g_appPlaces.size()) {
        return PillGroupOn(group);
    }
    const IslandPlace& place = g_appPlaces[strip];
    return !((place.own ? place.hidden
                        : place.hidden | GloballyHiddenGroups()) &
             (1ull << (int)group)) &&
           OnlineGroupAllowed(group);
}

void ControlPanel::SetStripOrder(int strip,
                                 const std::vector<PillGroup>& order) {
    if (strip < 0 || strip >= (int)g_appPlaces.size()) {
        SetPillOrder(order);
        return;
    }
    g_appPlaces[strip].order = order;
    if (g_island) {
        ForEachIsland([&](Island& island) { island.Relayout(); });
    }
}

void ControlPanel::ToggleAppGroup(size_t entry, int group) {
    if (entry >= g_appPlaces.size()) {
        return;
    }
    IslandPlace& place = g_appPlaces[entry];
    if (!place.own) {
        place.hidden |= GloballyHiddenGroups();
        place.own = true;
    }
    place.hidden ^= 1ull << group;
    SavePlaces();
    if (g_island) {
        ForEachIsland([&](Island& island) { island.OnPlacesChanged(); });
        ForEachIsland([&](Island& island) { island.Relayout(); });
    }
}

// The order strip's items, side by side in the chosen order, centered.
void ControlPanel::LayoutOrderChips(int strip) {
    const D2D1_RECT_F& stripRect = StripRect(strip);
    // (The settings' strip with notches: a part for each.)
    const int notches = strip < 0 ? NotchCountWanted() : 0;
    // Only the items on the pill (the others are turned on in the list).
    std::vector<PillGroup> shown;
    for (PillGroup group : StripOrder(strip)) {
        // (Not the clock while it's kept in the middle: it goes there; with
        // notches, it's moved to one like the others.)
        if (StripShows(strip, group) &&
            !(group == PillGroup::Clock && notches == 0 &&
              PlaceClockCenter(strip >= 0 && strip < (int)g_appPlaces.size()
                                   ? &g_appPlaces[strip]
                                   : nullptr))) {
            shown.push_back(group);
        }
    }
    std::erase_if(m_orderChips, [&](const OrderChip& chip) {
        return chip.strip == strip &&
               std::find(shown.begin(), shown.end(), chip.group) == shown.end();
    });
    if (strip < 0) {
        m_notchParts.clear();
    }
    if (notches == 0) {
        PlaceOrderChips(shown, stripRect, strip);
        return;
    }
    // The notches side by side (one: in the middle), each with its items;
    // a notch with many takes more of the strip (they're alike otherwise).
    const float s = m_scale;
    const std::vector<int> places = NotchPlaces(notches);
    const float between = std::round(12 * s);
    const float room = stripRect.right - stripRect.left;
    // (Their curves inside their part.)
    const float curve = NotchShoulder(stripRect.bottom - stripRect.top);
    std::vector<std::vector<PillGroup>> groups(places.size());
    std::vector<float> wanted(places.size());
    float available = room - between * (places.size() - 1);
    float total = 0;
    for (size_t i = 0; i < places.size(); i++) {
        float need = std::round(6 * s) + curve * 2;
        for (PillGroup group : shown) {
            if (NotchOfGroup(group, notches) == places[i]) {
                groups[i].push_back(group);
                need += std::round((group == PillGroup::Clock ? 50 : 30) * s) +
                        std::round(2 * s);
            }
        }
        wanted[i] = std::max(need, available / places.size() * 0.6f);
        total += wanted[i];
    }
    float left = stripRect.left;
    if (places.size() == 1) {
        const float width = std::round(room * 0.85f);
        left = std::round(stripRect.left + (room - width) / 2);
        wanted[0] = total = available = width;
    }
    for (size_t i = 0; i < places.size(); i++) {
        const float width = std::floor(available * wanted[i] / total);
        const D2D1_RECT_F part{left + curve, stripRect.top,
                               left + width - curve, stripRect.bottom};
        m_notchParts.push_back({places[i], part});
        PlaceOrderChips(groups[i], part, strip);
        left += width + between;
    }
}

void ControlPanel::PlaceOrderChips(const std::vector<PillGroup>& groups,
                                   const D2D1_RECT_F& rect,
                                   int strip) {
    const float s = m_scale;
    const float gap = std::round(2 * s);
    const float inset = std::round(3 * s);
    // Narrower when they don't all fit.
    float full = -gap;
    for (PillGroup group : groups) {
        full += std::round((group == PillGroup::Clock ? 50 : 30) * s) + gap;
    }
    const float room = rect.right - rect.left - inset * 2;
    const float squeeze =
        full > room && groups.size() > 1
            ? (room - gap * (groups.size() - 1)) /
                  (full - gap * (groups.size() - 1))
            : 1.0f;
    auto widthOf = [s, squeeze](PillGroup group) {
        return std::floor((group == PillGroup::Clock ? 50 : 30) * s * squeeze);
    };
    float total = -gap;
    for (PillGroup group : groups) {
        total += widthOf(group) + gap;
    }
    float x = std::round((rect.left + rect.right - total) / 2);
    for (PillGroup group : groups) {
        const float width = widthOf(group);
        const D2D1_RECT_F chipRect{x, rect.top + inset, x + width,
                                   rect.bottom - inset};
        if (OrderChip* chip = FindOrderChip(group, strip)) {
            chip->rect = chipRect;
        } else {
            m_orderChips.push_back({group, chipRect, chipRect.left, 0, strip});
        }
        x += width + gap;
    }
}

// The pictures' places: a card for each choice, side by side in the
// middle; for the banners' place, "Following the bell" beside a little
// screen with its nine points (the chosen one's name under it).
void ControlPanel::PlaceChoicePictures(int id,
                                       const D2D1_RECT_F& rect,
                                       float top,
                                       float bottom) {
    const float s = m_scale;
    auto& segments = m_choiceSegments[id];
    if (id == kChoiceBannerPlace) {
        const float gap = std::round(16 * s);
        const float bell = std::round(150 * s);
        const float screenHeight = std::round(96 * s);
        const float screenWidth = std::min(
            std::round(168 * s),
            std::max(std::round(90 * s),
                     rect.right - rect.left - bell - gap - std::round(8 * s)));
        const float left = std::round((rect.left + rect.right) / 2 -
                                      (bell + gap + screenWidth) / 2);
        const float screenTop = top + std::round(4 * s);
        const float middle = screenTop + screenHeight / 2;
        const float button = std::round(34 * s);
        segments.push_back(
            {left, middle - button / 2, left + bell, middle + button / 2});
        const D2D1_RECT_F screen{left + bell + gap, screenTop,
                                 left + bell + gap + screenWidth,
                                 screenTop + screenHeight};
        const float inset = std::round(5 * s);
        const float cellWidth = (screen.right - screen.left - inset * 2) / 3;
        const float cellHeight = (screen.bottom - screen.top - inset * 2) / 3;
        for (int i = 0; i < 9; i++) {
            const float x = screen.left + inset + cellWidth * (float)(i % 3);
            const float y = screen.top + inset + cellHeight * (float)(i / 3);
            segments.push_back({x, y, x + cellWidth, y + cellHeight});
        }
        return;
    }
    const int count = ChoiceCount(id);
    const float gap = std::round(8 * s);
    const float card = std::min(
        std::round(112 * s),
        std::floor((rect.right - rect.left - gap * (float)(count - 1)) /
                   (float)count));
    const float left = std::round((rect.left + rect.right) / 2 -
                                  (card * count + gap * (count - 1)) / 2);
    for (int i = 0; i < count; i++) {
        const float x = left + (card + gap) * (float)i;
        segments.push_back({x, top, x + card, bottom});
    }
}

// The pictures (see PlaceChoicePictures): the chosen one ringed in the
// accent, the one under the mouse lighter.
void ControlPanel::DrawChoicePictures(ID2D1RenderTarget* target,
                                      ID2D1SolidColorBrush* brush,
                                      int id,
                                      float opacity) {
    const float s = m_scale;
    const auto& segments = m_choiceSegments[id];
    const int value = ChoiceValue(id);
    auto lit = [&](int i) {
        return Hover(segments[i], m_hover.part == Part::ChoiceSegment &&
                                      m_hover.index == id * 16 + i);
    };
    auto card = [&](const D2D1_RECT_F& rect, bool chosen, float hover) {
        const float radius = std::round(10 * s);
        brush->SetColor(Fg((chosen ? 0.14f : 0.06f + 0.06f * hover) * opacity));
        GlassFill(target, {rect, radius, radius}, brush);
        if (chosen) {
            brush->SetColor(Accent(opacity));
            target->DrawRoundedRectangle(
                {{rect.left + 1, rect.top + 1, rect.right - 1, rect.bottom - 1},
                 radius - 1,
                 radius - 1},
                brush, std::max(1.5f, std::round(2 * s)));
        }
    };
    if (id == kChoiceBannerPlace && segments.size() == 10) {
        // "Following the bell": a button with the bell.
        const D2D1_RECT_F& bell = segments[0];
        card(bell, value == 0, lit(0));
        const WCHAR glyph = 0xEA8F;
        const float icon = bell.bottom - bell.top;
        brush->SetColor(Fg((value == 0 ? 0.95f : 0.7f) * opacity));
        DrawGlyph(target, &glyph, 1, m_smallIconFormat.get(),
                  {bell.left + std::round(4 * s), bell.top,
                   bell.left + std::round(4 * s) + icon, bell.bottom},
                  brush);
        DrawMarquee(target, brush, ChoiceOption(id, 0), m_linkFormat.get(),
                    {bell.left + icon, bell.top,
                     bell.right - std::round(8 * s), bell.bottom},
                    false);
        // The little screen, a banner at each of its nine points.
        const D2D1_RECT_F screen{segments[1].left - std::round(5 * s),
                                 segments[1].top - std::round(5 * s),
                                 segments[9].right + std::round(5 * s),
                                 segments[9].bottom + std::round(5 * s)};
        const float corner = std::round(8 * s);
        brush->SetColor(Fg(0.05f * opacity));
        GlassFill(target, {screen, corner, corner}, brush);
        brush->SetColor(Fg(0.3f * opacity));
        target->DrawRoundedRectangle({screen, corner, corner}, brush,
                                     std::max(1.0f, s));
        for (int i = 1; i <= 9; i++) {
            const D2D1_RECT_F& cell = segments[i];
            const float width = (cell.right - cell.left) * 0.62f;
            const float height = (cell.bottom - cell.top) * 0.5f;
            const int column = (i - 1) % 3;
            const int row = (i - 1) / 3;
            const float pad = std::round(2 * s);
            const float x = column == 0 ? cell.left + pad
                            : column == 2
                                ? cell.right - pad - width
                                : (cell.left + cell.right - width) / 2;
            const float y = row == 0 ? cell.top + pad
                            : row == 2 ? cell.bottom - pad - height
                                       : (cell.top + cell.bottom - height) / 2;
            const D2D1_RECT_F banner{x, y, x + width, y + height};
            const float round = height / 2;
            const float hover = lit(i);
            brush->SetColor(value == i ? Accent(opacity)
                                       : Fg((0.2f + 0.3f * hover) * opacity));
            target->FillRoundedRectangle({banner, round, round}, brush);
        }
        // The chosen point's name under the screen.
        if (value >= 1 && value <= 9) {
            const std::wstring name = ChoiceOption(id, value);
            brush->SetColor(Fg(0.75f * opacity));
            target->DrawText(name.c_str(), (UINT32)name.size(),
                             m_linkFormat.get(),
                             {screen.left - std::round(20 * s), screen.bottom,
                              screen.right + std::round(20 * s),
                              screen.bottom + std::round(26 * s)},
                             brush);
        }
        return;
    }
    for (size_t i = 0; i < segments.size(); i++) {
        const D2D1_RECT_F& rect = segments[i];
        const bool chosen = value == (int)i;
        card(rect, chosen, lit((int)i));
        // The drawing over the name: a little screen.
        const float label = std::round(24 * s);
        const float pictureBottom = rect.bottom - label;
        const float w = std::round(56 * s);
        const float h = std::round(36 * s);
        const float cx = (rect.left + rect.right) / 2;
        const float cy = (rect.top + pictureBottom) / 2 + std::round(3 * s);
        const D2D1_RECT_F screen{cx - w / 2, cy - h / 2, cx + w / 2,
                                 cy + h / 2};
        const D2D1_COLOR_F ink = chosen ? Accent(opacity) : Fg(0.7f * opacity);
        const float thick = std::round(6 * s);
        if (id == kChoiceBannerCorners) {
            // A banner with those corners.
            const float radius = i == 0   ? std::round(10 * s)
                                 : i == 1 ? std::round(5 * s)
                                          : 0.0f;
            const D2D1_RECT_F banner{screen.left, cy - std::round(12 * s),
                                     screen.right, cy + std::round(12 * s)};
            brush->SetColor(ink);
            target->FillRoundedRectangle({banner, radius, radius}, brush);
        } else {
            brush->SetColor(Fg(0.3f * opacity));
            target->DrawRectangle(screen, brush, std::max(1.0f, s));
            if (id == kChoiceBarSide) {
                // The bar along its edge.
                const D2D1_RECT_F bar =
                    i == 0   ? D2D1_RECT_F{screen.left, screen.top,
                                         screen.right, screen.top + thick}
                    : i == 1 ? D2D1_RECT_F{screen.left, screen.top,
                                           screen.left + thick, screen.bottom}
                             : D2D1_RECT_F{screen.right - thick, screen.top,
                                           screen.right, screen.bottom};
                brush->SetColor(ink);
                target->FillRectangle(bar, brush);
            } else if (id == kChoiceSetupStyle) {
                // The pill floating under the top edge, the pill joined to
                // it, or the bar along it.
                const float pill = std::round(9 * s);
                const float half = std::round(w * 0.3f);
                const D2D1_RECT_F shape =
                    i == 0   ? D2D1_RECT_F{cx - half, screen.top + thick,
                                         cx + half, screen.top + thick + pill}
                    : i == 1 ? D2D1_RECT_F{cx - half, screen.top - pill,
                                           cx + half, screen.top + pill}
                             : D2D1_RECT_F{screen.left, screen.top,
                                           screen.right, screen.top + thick};
                const float radius = i == 2 ? 0.0f : pill / 2;
                target->PushAxisAlignedClip(screen,
                                            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                brush->SetColor(ink);
                target->FillRoundedRectangle({shape, radius, radius}, brush);
                target->PopAxisAlignedClip();
            } else {
                // The bar at the top, and the screen under it with the
                // corners rounded where they meet (the bar's color in them).
                brush->SetColor(ink);
                target->FillRectangle(screen, brush);
                const float radius = std::round(4 * s) * (float)i;
                brush->SetColor(Bg(1));
                target->FillRoundedRectangle(
                    {{screen.left, screen.top + thick, screen.right,
                      screen.bottom + radius},
                     radius,
                     radius},
                    brush);
                brush->SetColor(Fg(0.3f * opacity));
                target->DrawRectangle(screen, brush, std::max(1.0f, s));
            }
        }
        const std::wstring name = ChoiceOption(id, (int)i);
        brush->SetColor(Fg((chosen ? 0.95f : 0.7f) * opacity));
        DrawMarquee(target, brush, name, m_linkFormat.get(),
                    {rect.left + std::round(6 * s), pictureBottom,
                     rect.right - std::round(6 * s),
                     rect.bottom - std::round(2 * s)},
                    false);
    }
}

void ControlPanel::LayoutSettings() {
    if (!m_toolsLooked) {
        m_toolsLooked = true;
        LookForKomorebi();
    }
    if (m_settings.empty()) {
        static const SettingSpec kSpecs[] = {
            // The setup assistant's page: its text, then its own choices
            // (the other tabs' options it shows come after them, see
            // SetupShows).
            {(int)SettingKind::Setup, 0, kSectionSetup},
            {(int)SettingKind::Choice, kChoiceSetupStyle, kSectionSetup},
            {(int)SettingKind::Choice, kChoiceSetupItems, kSectionSetup},
            {(int)SettingKind::Choice, kChoiceSetupHide, kSectionSetup},
            {(int)SettingKind::Choice, kChoiceSetupTheme, kSectionSetup},
            {(int)SettingKind::Label, 0, kSectionProfiles},
            {(int)SettingKind::Profiles, 0, kSectionProfiles},
            {(int)SettingKind::Label, 0, kSectionBackup},
            {(int)SettingKind::Choice, kChoiceBackup, kSectionBackup},
            {(int)SettingKind::Label, 0, kSectionLanguage},
            {(int)SettingKind::Choice, kChoiceLanguage, kSectionLanguage},
            {(int)SettingKind::Label, 0, kSectionStyle},
            {(int)SettingKind::Style, 0, kSectionStyle},
            {(int)SettingKind::Choice, kChoiceNotches, kSectionStyle, 1},
            {(int)SettingKind::Choice, kChoiceNotchOpen, kSectionStyle, 1},
            {(int)SettingKind::Size, 6, kSectionStyle, 1},
            {(int)SettingKind::Pref, (int)IslandPref::NotchEach,
             kSectionStyle, 1},
            {(int)SettingKind::Pref, (int)IslandPref::FreeDrag, kSectionStyle},
            {(int)SettingKind::Pref, (int)IslandPref::VerticalSides,
             kSectionStyle},
            {(int)SettingKind::Label, 0, kSectionPlace},
            {(int)SettingKind::Place, 0, kSectionPlace},
            {(int)SettingKind::AppPlaces, 0, kSectionPlace},
            {(int)SettingKind::Size, 1, kSectionPlace},
            {(int)SettingKind::Label, 0, kSectionHide},
            {(int)SettingKind::Pref, (int)IslandPref::AutoHide, kSectionHide},
            {(int)SettingKind::Choice, kChoiceHideWhen, kSectionHide, 1},
            {(int)SettingKind::Pref, (int)IslandPref::HideClock,
             kSectionHide, 1},
            {(int)SettingKind::Pref, (int)IslandPref::TimerLine, kSectionHide},
            {(int)SettingKind::Size, 5, kSectionHide, 1},
            {(int)SettingKind::Size, 8, kSectionHide},
            {(int)SettingKind::Choice, kChoiceLineFade, kSectionHide, 1},
            {(int)SettingKind::Choice, kChoiceLineClick, kSectionHide, 1},
            {(int)SettingKind::Size, 9, kSectionHide},
            {(int)SettingKind::Pref, (int)IslandPref::MinimizeButton,
             kSectionHide},
            {(int)SettingKind::Pref, (int)IslandPref::MinimizeKey,
             kSectionHide},
            {(int)SettingKind::Shortcut, 0, kSectionHide, 1},
            {(int)SettingKind::Label, 0, kSectionColors},
            {(int)SettingKind::Theme, 0, kSectionColors},
            {(int)SettingKind::Choice, kChoiceColors, kSectionColors, 1},
            {(int)SettingKind::Pref, (int)IslandPref::WindowColor,
             kSectionColors},
            {(int)SettingKind::Size, 4, kSectionColors, 1},
            {(int)SettingKind::Label, 0, kSectionGlass},
            {(int)SettingKind::Pref, (int)IslandPref::LiquidGlass,
             kSectionGlass},
            {(int)SettingKind::Pref, (int)IslandPref::GlassTranslucent,
             kSectionGlass, 1},
            {(int)SettingKind::Pref, (int)IslandPref::GlassLight,
             kSectionGlass, 1},
            {(int)SettingKind::Size, 16, kSectionGlass, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Frosted,
             kSectionGlass, 1},
            {(int)SettingKind::Pref, (int)IslandPref::BarClear,
             kSectionGlass, 1},
            {(int)SettingKind::Label, 0, kSectionShape},
            {(int)SettingKind::Size, 12, kSectionShape},
            {(int)SettingKind::Size, 13, kSectionShape, 1},
            {(int)SettingKind::Size, 14, kSectionShape, 1},
            {(int)SettingKind::Size, 15, kSectionShape, 1},
            {(int)SettingKind::Label, 0, kSectionLook},
            {(int)SettingKind::Size, 0, kSectionLook},
            {(int)SettingKind::Size, 7, kSectionLook, 1},
            {(int)SettingKind::Size, 19, kSectionLook, 1},
            {(int)SettingKind::Size, 10, kSectionLook, 1},
            {(int)SettingKind::Pref, (int)IslandPref::PanelsFollow,
             kSectionLook, 1},
            {(int)SettingKind::Label, 0, kSectionBar},
            {(int)SettingKind::Choice, kChoiceBarSide, kSectionBar},
            {(int)SettingKind::Choice, kChoiceBarInnerCorners, kSectionBar},
            {(int)SettingKind::Pref, (int)IslandPref::BarCorners, kSectionBar},
            {(int)SettingKind::Label, 0, kSectionDetails},
            {(int)SettingKind::Pref, (int)IslandPref::Tips, kSectionDetails},
            {(int)SettingKind::Pref, (int)IslandPref::HoverHighlight,
             kSectionDetails, 1},
            {(int)SettingKind::Label, 0, kSectionIcons},
            {(int)SettingKind::Choice, kChoiceIconPack, kSectionIcons},
            {(int)SettingKind::Choice, kChoiceIconColors, kSectionIcons, 1},
            {(int)SettingKind::Choice, kChoiceIconFolder, kSectionIcons, 1},
            {(int)SettingKind::Icons, (int)IconGroup::Status, kSectionIcons},
            {(int)SettingKind::Icons, (int)IconGroup::Media, kSectionIcons},
            {(int)SettingKind::Icons, (int)IconGroup::Controls, kSectionIcons},
            {(int)SettingKind::Icons, (int)IconGroup::Pill, kSectionIcons},
            {(int)SettingKind::Icons, (int)IconGroup::Buttons, kSectionIcons},
            {(int)SettingKind::Icons, (int)IconGroup::Other, kSectionIcons},
            {(int)SettingKind::Label, 0, kSectionAnimations},
            {(int)SettingKind::Pref, (int)IslandPref::Animations,
             kSectionAnimations},
            {(int)SettingKind::Choice, kChoiceAnimationLevel,
             kSectionAnimations, 1},
            {(int)SettingKind::Size, 2, kSectionAnimations, 1},
            {(int)SettingKind::Choice, kChoiceFrameRate, kSectionAnimations, 1},
            {(int)SettingKind::Label, 0, kSectionSystem},
            {(int)SettingKind::Pref, (int)IslandPref::Network, kSectionSystem},
            {(int)SettingKind::Pref, (int)IslandPref::Volume, kSectionSystem},
            {(int)SettingKind::Pref, (int)IslandPref::VolumePercent,
             kSectionSystem, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Battery, kSectionSystem},
            {(int)SettingKind::Pref, (int)IslandPref::BatteryPercent,
             kSectionSystem, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Brightness,
             kSectionSystem},
            {(int)SettingKind::Pref, (int)IslandPref::Microphone,
             kSectionSystem},
            {(int)SettingKind::Pref, (int)IslandPref::Privacy, kSectionSystem},
            {(int)SettingKind::Pref, (int)IslandPref::InputMethod,
             kSectionSystem},
            {(int)SettingKind::Pref, (int)IslandPref::ThemeButton,
             kSectionSystem},
            {(int)SettingKind::Pref, (int)IslandPref::Power, kSectionSystem},
            {(int)SettingKind::Pref, (int)IslandPref::Bell, kSectionSystem},
            {(int)SettingKind::Label, 0, kSectionFill},
            {(int)SettingKind::Pref, (int)IslandPref::DownloadFill,
             kSectionFill},
            {(int)SettingKind::Pref, (int)IslandPref::DownloadStalled,
             kSectionFill, 1},
            {(int)SettingKind::Choice, kChoiceFillColor, kSectionFill, 1},
            {(int)SettingKind::Choice, kChoiceFillCopyColor, kSectionFill, 1},
            {(int)SettingKind::Pref, (int)IslandPref::ChargeFill, kSectionFill},
            {(int)SettingKind::Choice, kChoiceFillChargeColor, kSectionFill, 1},
            {(int)SettingKind::Choice, kChoiceFillStyle, kSectionFill},
            {(int)SettingKind::Label, 0, kSectionDesktop},
            {(int)SettingKind::Pref, (int)IslandPref::Desktops,
             kSectionDesktop},
            {(int)SettingKind::Pref, (int)IslandPref::FocusedApp,
             kSectionDesktop},
            {(int)SettingKind::Pref, (int)IslandPref::FocusedTitle,
             kSectionDesktop, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Folders, kSectionDesktop},
            {(int)SettingKind::Pref, (int)IslandPref::FolderThumbnails,
             kSectionDesktop, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Shelf, kSectionDesktop},
            {(int)SettingKind::Choice, kChoiceShelfLimit, kSectionDesktop, 1},
            {(int)SettingKind::Choice, kChoiceFolderDrag, kSectionDesktop, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Eyes, kSectionDesktop},
            {(int)SettingKind::Choice, kChoiceEyeTheme, kSectionDesktop, 1},
            {(int)SettingKind::Choice, kChoiceEyeShape, kSectionDesktop, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Wallpaper,
             kSectionDesktop},
            {(int)SettingKind::Choice, kChoiceWallpaper, kSectionDesktop, 1},
            {(int)SettingKind::Pref, (int)IslandPref::UsbDrives,
             kSectionDesktop},
            {(int)SettingKind::Pref, (int)IslandPref::UsbNotify,
             kSectionDesktop, 1},
            {(int)SettingKind::Pref, (int)IslandPref::RecycleBin,
             kSectionDesktop},
            {(int)SettingKind::Choice, kChoiceRecycleMinSize,
             kSectionDesktop, 1},
            {(int)SettingKind::Label, 0, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::Notes, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::Tasks, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::Shortcuts, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::Weather, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::WeatherCards,
             kSectionTools, 1},
            {(int)SettingKind::Choice, kChoiceWeatherUnit, kSectionTools, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Updates, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::Profiles, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::UsageCpu, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::UsageRam, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::UsageVram, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::NetSpeed, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::Timer, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::AiUsage, kSectionTools},
            {(int)SettingKind::Pref, (int)IslandPref::AiClaude,
             kSectionTools, 1},
            {(int)SettingKind::Pref, (int)IslandPref::AiCodex,
             kSectionTools, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Commands, kSectionTools},
            {(int)SettingKind::Label, 0, kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionNetwork,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionBluetooth,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionNightLight,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionHotspot,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionAirplane,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionProject,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionCast,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionCaptions,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionAccessibility,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionScreenshot,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionLock,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionEmoji,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionClipboard,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionTaskManager,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionCalculator,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionExplorer,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionShowDesktop,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionOnScreenKeyboard,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::ActionKomorebi,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::KomorebiWhkd,
             kSectionActions, 1},
            {(int)SettingKind::Pref, (int)IslandPref::KomorebiMasir,
             kSectionActions, 1},
            {(int)SettingKind::Pref, (int)IslandPref::KomorebiLayout,
             kSectionActions},
            {(int)SettingKind::Pref, (int)IslandPref::KomorebiHideEmpty,
             kSectionActions, 1},
            {(int)SettingKind::Pref, (int)IslandPref::ExternalBrightness,
             kSectionActions},
            {(int)SettingKind::Label, 0, kSectionTray},
            {(int)SettingKind::Pref, (int)IslandPref::Tray, kSectionTray},
            {(int)SettingKind::Choice, kChoiceTrayStyle, kSectionTray, 1},
            {(int)SettingKind::Pref, (int)IslandPref::TrayOpen,
             kSectionTray, 1},
            {(int)SettingKind::Size, 3, kSectionTray, 1},
            {(int)SettingKind::Size, 17, kSectionTray, 1},
            {(int)SettingKind::Label, 0, kSectionMedia},
            {(int)SettingKind::Pref, (int)IslandPref::Media, kSectionMedia},
            {(int)SettingKind::Pref, (int)IslandPref::MediaButtons,
             kSectionMedia, 1},
            {(int)SettingKind::Pref, (int)IslandPref::MediaTitle,
             kSectionMedia, 1},
            {(int)SettingKind::Size, 11, kSectionMedia, 1},
            {(int)SettingKind::Pref, (int)IslandPref::MediaMarquee,
             kSectionMedia, 1},
            {(int)SettingKind::Pref, (int)IslandPref::MediaWheel,
             kSectionMedia, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Visualizer,
             kSectionMedia},
            {(int)SettingKind::Size, 18, kSectionMedia, 1},
            {(int)SettingKind::Choice, kChoiceVisualizerColor,
             kSectionMedia, 1},
            {(int)SettingKind::Pref, (int)IslandPref::VisualizerSmooth,
             kSectionMedia, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Lyrics, kSectionMedia},
            {(int)SettingKind::Choice, kChoiceLyricsSize, kSectionMedia, 1},
            {(int)SettingKind::Choice, kChoiceLyricsWidth, kSectionMedia, 1},
            {(int)SettingKind::Choice, kChoiceLyricsColor, kSectionMedia, 1},
            {(int)SettingKind::Pref, (int)IslandPref::LyricsInTitle,
             kSectionMedia, 1},
            {(int)SettingKind::LyricsApps, 0, kSectionMedia, 1},
            {(int)SettingKind::Label, 0, kSectionClock},
            // (The items' order is the preview over the tabs, not a row.)
            {(int)SettingKind::Order, 0, kSectionClock},
            {(int)SettingKind::Pref, (int)IslandPref::Clock, kSectionClock},
            {(int)SettingKind::Pref, (int)IslandPref::ClockCenter,
             kSectionClock, 1},
            {(int)SettingKind::Choice, kChoiceClockFormat, kSectionClock},
            {(int)SettingKind::Pref, (int)IslandPref::ClockSeconds,
             kSectionClock, 1},
            {(int)SettingKind::Pref, (int)IslandPref::ClockWeekday,
             kSectionClock, 1},
            {(int)SettingKind::Pref, (int)IslandPref::ClockDate,
             kSectionClock, 1},
            {(int)SettingKind::Pref, (int)IslandPref::ClockYear,
             kSectionClock, 1},
            {(int)SettingKind::Choice, kChoiceDateOrder, kSectionClock, 1},
            {(int)SettingKind::Pref, (int)IslandPref::MonthNumber,
             kSectionClock, 1},
            {(int)SettingKind::Pref, (int)IslandPref::Holidays,
             kSectionClock, 1},
            {(int)SettingKind::Pref, (int)IslandPref::CalendarBigDay,
             kSectionClock},
            {(int)SettingKind::Pref, (int)IslandPref::WorldClocks,
             kSectionClock},
            {(int)SettingKind::Pref, (int)IslandPref::Countdown, kSectionClock},
            {(int)SettingKind::Pref, (int)IslandPref::CountdownEvents,
             kSectionClock, 1},
            {(int)SettingKind::Pref, (int)IslandPref::CountdownAlarms,
             kSectionClock, 1},
            {(int)SettingKind::Pref, (int)IslandPref::CountdownTimer,
             kSectionClock, 1},
            {(int)SettingKind::Pref, (int)IslandPref::CountdownPomodoro,
             kSectionClock, 1},
            {(int)SettingKind::Label, 0, kSectionNotifications},
            {(int)SettingKind::Pref, (int)IslandPref::OwnNotifications,
             kSectionNotifications},
            {(int)SettingKind::Pref, (int)IslandPref::DndSilent,
             kSectionNotifications, 1},
            {(int)SettingKind::Pref, (int)IslandPref::ShortcutNotifications,
             kSectionNotifications, 1},
            {(int)SettingKind::Pref, (int)IslandPref::ShortcutControl,
             kSectionNotifications, 1},
            {(int)SettingKind::Pref, (int)IslandPref::FoldApps,
             kSectionNotifications},
            {(int)SettingKind::Pref, (int)IslandPref::NotificationApps,
             kSectionNotifications},
            {(int)SettingKind::Pref, (int)IslandPref::GroupApps,
             kSectionNotifications},
            {(int)SettingKind::Pref, (int)IslandPref::Clipboard,
             kSectionNotifications},
            {(int)SettingKind::Pref, (int)IslandPref::ScreenshotFolder,
             kSectionNotifications},
            {(int)SettingKind::Label, 0, kSectionBanners},
            {(int)SettingKind::Pref, (int)IslandPref::Banner, kSectionBanners},
            {(int)SettingKind::Pref, (int)IslandPref::BannerMinimized,
             kSectionBanners, 1},
            {(int)SettingKind::Choice, kChoiceBannerSize, kSectionBanners, 1},
            {(int)SettingKind::Choice, kChoiceBannerWidth, kSectionBanners, 1},
            {(int)SettingKind::Choice, kChoiceBannerCorners,
             kSectionBanners, 1},
            {(int)SettingKind::Choice, kChoiceBannerPlace, kSectionBanners, 1},
            {(int)SettingKind::Pref, (int)IslandPref::HideWindowsBanners,
             kSectionBanners},
            {(int)SettingKind::Pref, (int)IslandPref::WebPictures,
             kSectionBanners, 1},
            {(int)SettingKind::Pref, (int)IslandPref::CapsLock,
             kSectionBanners},
            {(int)SettingKind::Pref, (int)IslandPref::StatusNotices,
             kSectionBanners},
            {(int)SettingKind::Label, 0, kSectionMonitors},
            {(int)SettingKind::Monitors, 0, kSectionMonitors},
            {(int)SettingKind::Dock, 0, kSectionDock},
            {(int)SettingKind::About, 0, kSectionAbout},
            {(int)SettingKind::About, 4, kSectionAbout},
            {(int)SettingKind::About, 1, kSectionAbout},
            {(int)SettingKind::About, 2, kSectionAbout},
            {(int)SettingKind::About, 3, kSectionAbout},
            {(int)SettingKind::Pref, (int)IslandPref::WhatsNewNotice,
             kSectionWhatsNew},
            {(int)SettingKind::Choice, kChoiceNotesLanguage, kSectionWhatsNew},
            {(int)SettingKind::WhatsNew, 0, kSectionWhatsNew},
            {(int)SettingKind::WhatsNew, 1, kSectionWhatsNew},
            {(int)SettingKind::WhatsNew, 2, kSectionWhatsNew},
            {(int)SettingKind::WhatsNew, 3, kSectionWhatsNew},
            {(int)SettingKind::WhatsNew, 4, kSectionWhatsNew},
            {(int)SettingKind::Logs, 0, kSectionLogs},
            {(int)SettingKind::Logs, 1, kSectionLogs},
        };
        for (const auto& spec : kSpecs) {
            m_settings.push_back({(SettingKind)spec.kind, spec.value,
                                  spec.section, spec.follows != 0});
        }
    }

    LayoutSettingsWindow();
}

void ControlPanel::PlaceSettingParts(SettingEntry& entry,
                                     const D2D1_RECT_F& rect,
                                     float column) {
    const float s = m_scale;
    const float top = rect.top;
    const float height = rect.bottom - rect.top;
    entry.rect = rect;
    const bool under = entry.kind == SettingKind::Style ||
                       entry.kind == SettingKind::Theme ||
                       entry.kind == SettingKind::Size ||
                       entry.kind == SettingKind::Choice ||
                       entry.kind == SettingKind::LyricsApps ||
                       entry.kind == SettingKind::Icons;
    entry.head = under ? SettingNameHeight(entry, column) : 0;
    // A selector's or a slider's row, under its name.
    const float controlTop = top + entry.head;
    const float controlBottom = rect.bottom - std::round(6 * s);
    // The parts of the rows that have some.
    switch (entry.kind) {
        case SettingKind::Label:
            if (entry.section == kSectionProfiles &&
                g_profiles.size() < kMaxProfiles) {
                const float add = std::round(22 * s);
                const float addTop = top + (height - add) / 2;
                m_profileAdd = {rect.right - std::round(4 * s) - add,
                                addTop, rect.right - std::round(4 * s),
                                addTop + add};
            }
            break;
        case SettingKind::Style: {
            m_styleRow = rect;
            const float segment = std::min(
                std::round(84 * s),
                std::floor((rect.right - rect.left - std::round(8 * s)) /
                           3));
            const float left =
                std::round((rect.left + rect.right) / 2 - segment * 1.5f);
            for (int i = 0; i < 3; i++) {
                m_styleSegments[i] = {left + segment * i, controlTop,
                                      left + segment * (i + 1),
                                      controlBottom};
            }
            break;
        }
        case SettingKind::Theme: {
            m_themeRow = rect;
            const float swatch = std::round(20 * s);
            const float swatchGap = std::round(8 * s);
            const int count = (int)ThemeId::Count;
            float swatchLeft =
                std::round((rect.left + rect.right) / 2 -
                           (swatch * count + swatchGap * (count - 1)) / 2);
            const float middle = (controlTop + controlBottom) / 2;
            for (int i = 0; i < count; i++) {
                m_swatches[i] = {swatchLeft, middle - swatch / 2,
                                 swatchLeft + swatch, middle + swatch / 2};
                swatchLeft += swatch + swatchGap;
            }
            break;
        }
        case SettingKind::Size: {
            // The slider in the middle, its "restore" at the right.
            const int slider = entry.value;
            m_sizeRow[slider] = rect;
            const float reset = std::round(28 * s);
            const float middle = (controlTop + controlBottom) / 2;
            m_sizeReset[slider] = {rect.right - std::round(2 * s) - reset,
                                   middle - reset / 2,
                                   rect.right - std::round(2 * s),
                                   middle + reset / 2};
            m_sizeSlider[slider] = {
                rect.left + reset + std::round(12 * s), controlTop,
                m_sizeReset[slider].left - std::round(10 * s),
                controlBottom};
            break;
        }
        case SettingKind::Place: {
            m_placeRow = rect;
            const float gridWidth = std::round(54 * s);
            const float gridHeight = std::round(32 * s);
            m_placeGrid = {rect.right - std::round(6 * s) - gridWidth,
                           top + (height - gridHeight) / 2,
                           rect.right - std::round(6 * s),
                           top + (height + gridHeight) / 2};
            break;
        }
        case SettingKind::AppPlaces:
            m_appPlacesRow = rect;
            break;
        case SettingKind::Icons: {
            // (This group's cells made again; the other groups' kept.)
            std::erase_if(m_iconCells, [&](const IconCell& cell) {
                return (int)kIcons[cell.icon].group == entry.value;
            });
            const float cellHeight = std::round(kIconCellHeight * s);
            const float gap = std::round(kIconCellGap * s);
            const size_t columns = IconCellColumns(column, s);
            const float left = rect.left + std::round(4 * s);
            const float cellWidth = std::floor(
                (rect.right - std::round(4 * s) - left -
                 gap * (float)(columns - 1)) /
                (float)columns);
            const float reset = std::round(18 * s);
            size_t i = 0;
            for (size_t index = 0; index < std::size(kIcons); index++) {
                if ((int)kIcons[index].group != entry.value) {
                    continue;
                }
                const float cellLeft =
                    left + (cellWidth + gap) * (float)(i % columns);
                const float cellTop =
                    controlTop + (cellHeight + gap) * (float)(i / columns);
                const D2D1_RECT_F cell{cellLeft, cellTop,
                                       cellLeft + cellWidth,
                                       cellTop + cellHeight};
                m_iconCells.push_back(
                    {(int)index, cell,
                     {cell.right - reset - std::round(3 * s),
                      cell.top + std::round(3 * s),
                      cell.right - std::round(3 * s),
                      cell.top + std::round(3 * s) + reset},
                     {cell.left + std::round(3 * s),
                      cell.top + std::round(3 * s),
                      cell.left + std::round(3 * s) + reset,
                      cell.top + std::round(3 * s) + reset}});
                i++;
            }
            break;
        }
        case SettingKind::LyricsApps: {
            const float card = std::round(kLyricsAppCard * s);
            const float gap = std::round(6 * s);
            const size_t columns = LyricsAppsColumns(column, s);
            const float left = rect.left + std::round(4 * s);
            const float cardWidth = std::floor(
                (rect.right - std::round(4 * s) - left -
                 gap * (float)(columns - 1)) /
                (float)columns);
            m_lyricsAppChips.clear();
            for (size_t i = 0; i < LyricsApps().size(); i++) {
                const float cardLeft =
                    left + (cardWidth + gap) * (float)(i % columns);
                const float cardTop =
                    controlTop + (card + gap) * (float)(i / columns);
                m_lyricsAppChips.push_back(
                    {cardLeft, cardTop, cardLeft + cardWidth, cardTop + card});
            }
            PrepareLyricsAppIcons();
            break;
        }
        case SettingKind::Monitors:
            LayoutMonitors(rect);
            break;
        case SettingKind::Profiles: {
            const float chip = std::round(32 * s);
            const float gap = std::round(6 * s);
            const float chipWidth =
                std::floor((rect.right - rect.left - gap * 2) / 3);
            // On another island than the main one: its monitor's
            // profile, or the main island's settings (the last chip).
            const bool monitor = g_island && !g_island->HasMainSettings();
            const size_t count = g_profiles.size();
            const size_t chips = count + (monitor ? 1 : 0);
            m_profileChips.clear();
            m_profilePencils.clear();
            float chipsBottom = top + std::round(4 * s);
            for (size_t i = 0; i < chips; i++) {
                const float left =
                    rect.left + (chipWidth + gap) * (float)(i % 3);
                const float chipTop =
                    top + std::round(4 * s) + (chip + gap) * (float)(i / 3);
                const D2D1_RECT_F chipRect{left, chipTop, left + chipWidth,
                                           chipTop + chip};
                m_profileChips.push_back(chipRect);
                if (i < count) {
                    m_profilePencils.push_back({chipRect.right - chip,
                                                chipRect.top,
                                                chipRect.right,
                                                chipRect.bottom});
                }
                chipsBottom = chipTop + chip + gap;
            }
            m_profileNameField = m_profileUpdate = m_profileDelete =
                m_profileDone = {};
            if (m_profileEdit >= 0) {
                const float field = std::round(34 * s);
                m_profileNameField = {rect.left, chipsBottom, rect.right,
                                      chipsBottom + field};
                const float buttonsTop = chipsBottom + field + gap;
                const float third =
                    std::floor((rect.right - rect.left - gap * 2) / 3);
                m_profileUpdate = {rect.left, buttonsTop,
                                   rect.left + third, buttonsTop + field};
                m_profileDelete = {rect.left + third + gap, buttonsTop,
                                   rect.left + third * 2 + gap,
                                   buttonsTop + field};
                m_profileDone = {rect.right - third, buttonsTop,
                                 rect.right, buttonsTop + field};
            }
            m_profileStyleChips.clear();
            m_profileAppChips.clear();
            m_profileApps.clear();
            m_profileAddApp.clear();
            m_profileAppAdd = m_profileKeep[0] = m_profileKeep[1] = {};
            if (m_profileEdit >= 0 && !monitor) {
                // A line of styles, its apps (and the one to add) three on
                // a line, and staying after its app or not.
                const float third =
                    std::floor((rect.right - rect.left - gap * 2) / 3);
                float y = m_profileDone.bottom + gap;
                for (int i = 0; i < 3; i++) {
                    const float left = rect.left + (third + gap) * (float)i;
                    m_profileStyleChips.push_back(
                        {left, y, left + third, y + chip});
                }
                y += chip + gap;
                m_profileApps = AutoProfileApps(m_profileEdit);
                m_profileAddApp = AutoProfileAppToAdd(m_profileEdit);
                const size_t count =
                    m_profileApps.size() + (m_profileAddApp.empty() ? 0 : 1);
                for (size_t i = 0; i < count; i++) {
                    const float left =
                        rect.left + (third + gap) * (float)(i % 3);
                    const float chipTop = y + (chip + gap) * (float)(i / 3);
                    const D2D1_RECT_F chipRect{left, chipTop, left + third,
                                               chipTop + chip};
                    if (i < m_profileApps.size()) {
                        m_profileAppChips.push_back(chipRect);
                    } else {
                        m_profileAppAdd = chipRect;
                    }
                }
                y += (chip + gap) * (float)ProfileAppRows();
                const float half =
                    std::floor((rect.right - rect.left - gap) / 2);
                m_profileKeep[0] = {rect.left, y, rect.left + half, y + chip};
                m_profileKeep[1] = {rect.right - half, y, rect.right,
                                    y + chip};
            }
            break;
        }
        case SettingKind::Choice: {
            // Segments in the middle, each as wide as its names need (in
            // rows when they don't fit one, see FitChoice); pictures for
            // the places and shapes.
            const int id = entry.value;
            const int count = ChoiceCount(id);
            m_choiceRow[id] = rect;
            m_choiceSegments[id].clear();
            if (id == kChoiceBannerPlace || PictureChoice(id)) {
                PlaceChoicePictures(id, rect, controlTop, controlBottom);
                break;
            }
            const ChoiceFit fit = FitChoice(id, rect.right - rect.left);
            const int rows = fit.rows;
            const int across = fit.across;
            const float segment = fit.segment;
            const float first = std::round((rect.left + rect.right) / 2 -
                                           segment * across / 2);
            const float rowGap = std::round(6 * s);
            const float rowHeight =
                (controlBottom - controlTop - rowGap * (rows - 1)) / rows;
            for (int i = 0; i < count; i++) {
                const float left = first + segment * (i % across);
                const float rowTop =
                    controlTop + (rowHeight + rowGap) * (float)(i / across);
                m_choiceSegments[id].push_back(
                    {left, rowTop, left + segment, rowTop + rowHeight});
            }
            // (A downloaded pack's bin, at its row's end.)
            if (id == kChoiceIconPack) {
                const float inset = std::round(4 * s);
                const bool look = m_iconPackDiskSeen != g_iconDiskChanges;
                m_iconPackDiskSeen = g_iconDiskChanges;
                for (int i = 0; i < count; i++) {
                    const D2D1_RECT_F& row = m_choiceSegments[id][i];
                    if (look) {
                        m_iconPackOnDisk[i] = IconPackOnDisk(i);
                    }
                    const float size = row.bottom - row.top - 2 * inset;
                    m_iconPackDelete[i] =
                        m_iconPackOnDisk[i]
                            ? D2D1_RECT_F{row.right - std::round(3 * s) -
                                              inset - size,
                                          row.top + inset,
                                          row.right - std::round(3 * s) -
                                              inset,
                                          row.bottom - inset}
                            : D2D1_RECT_F{};
                }
            }
            break;
        }
        case SettingKind::Order:
            m_orderStrip = {
                rect.left + std::round(4 * s), top + std::round(6 * s),
                rect.right - std::round(4 * s), top + std::round(42 * s)};
            LayoutOrderChips();
            break;
        case SettingKind::Dock:
        case SettingKind::Logs: {
            // Its buttons at the right (not on the logs' text).
            if (entry.kind == SettingKind::Logs && entry.value == 1) {
                break;
            }
            const float button = std::round(30 * s);
            const float middle = (rect.top + rect.bottom) / 2;
            float right = rect.right - std::round(2 * s);
            const int buttons[2] = {
                entry.kind == SettingKind::Dock ? (int)kPageWindhawk
                                                : (int)kPageSaveLogs,
                entry.kind == SettingKind::Dock ? -1 : (int)kPageCopyLogs};
            for (int id : buttons) {
                if (id < 0) {
                    continue;
                }
                const float buttonWidth =
                    std::round((id == kPageWindhawk ? 140 : 110) * s);
                m_pageButtons.push_back({id,
                                         {right - buttonWidth,
                                          middle - button / 2, right,
                                          middle + button / 2}});
                right -= buttonWidth + std::round(8 * s);
            }
            break;
        }
        case SettingKind::Shortcut: {
            // The keys' chip, its "restore" at the right (like a
            // slider's).
            const float reset = std::round(28 * s);
            const float middle = (rect.top + rect.bottom) / 2;
            m_shortcutReset = {rect.right - std::round(2 * s) - reset,
                               middle - reset / 2,
                               rect.right - std::round(2 * s),
                               middle + reset / 2};
            const float chip = std::round(28 * s);
            m_shortcutChip = {
                m_shortcutReset.left - std::round(6 * s) -
                    std::round(138 * s),
                middle - chip / 2, m_shortcutReset.left - std::round(6 * s),
                middle + chip / 2};
            break;
        }
        default:
            break;
    }
}

// The settings' window, like Windows' Settings: a side bar with the search
// box and the tabs (only their icons when the window is narrow, the search
// box over the page then; it scrolls when the window is short), and the tab
// shown at the right under its title: the island's preview (its items in
// their order, dragged to change it; not for the monitors and the pages
// about the mod), then its sections' titles and its options, each on a card
// with what it does under its name (and why it's dimmed, when it is), as
// wide as the window allows up to about 1100 DIPs. The setup assistant's
// tab shows its step, with its buttons under it.
void ControlPanel::LayoutSettingsWindow() {
    const float s = m_scale;
    RECT client{};
    GetClientRect(m_hwnd, &client);
    // (Under its title bar.)
    const float width = (float)std::max(client.right, 1L);
    const float height =
        (float)std::max(client.bottom - CaptionHeight(), 1L);
    m_size = {(LONG)width, (LONG)height};
    m_back = m_profileAdd = {};
    m_pageButtons.clear();
    // (Read as the page opens, not each time it's laid out: it copies the
    // kept lines and reads the registry.)
    if (m_settingsTab != kTabLogs) {
        m_logsShown.clear();
    } else if (m_logsShown.empty()) {
        m_logsShown = LogsText();
    }
    // The setup assistant from its first step as its tab shows, with every
    // option as it is then; left for another tab (or one of its links), it's
    // done, with what was chosen.
    if (m_settingsTab == kTabSetup && m_setup.from.empty()) {
        m_setup = {kSetupWelcome, g_activeProfile, IslandPrefValues()};
    } else if (m_settingsTab != kTabSetup && !m_setup.from.empty()) {
        EndSetup(false);
    }
    const float padding = std::round(16 * s);
    const float field = std::round(32 * s);
    m_navNarrow = width < std::round(900 * s);
    const float nav =
        m_navNarrow ? std::round(60 * s)
                    : std::clamp(std::round(width * 0.21f), std::round(240 * s),
                                 std::round(320 * s));
    float navTop = padding;
    if (!m_navNarrow) {
        m_searchField = {padding, navTop, nav - padding / 2, navTop + field};
        navTop += field + std::round(14 * s);
    }
    // The tabs shown (the monitors' with two or more), the mod's own pages
    // after a line.
    m_tabs.clear();
    m_tabIds.clear();
    const float tabHeight = std::round(34 * s);
    const float tabGap = std::round(2 * s);
    if (!SettingTabShown(m_settingsTab)) {
        m_settingsTab = kTabGeneral;
    }
    float tabTop = navTop;
    for (int tab = 0; tab < kTabCount; tab++) {
        if (!SettingTabShown(tab)) {
            continue;
        }
        if (tab == kTabDock) {
            tabTop += std::round(13 * s);
        }
        m_tabs.push_back({std::round(8 * s), tabTop, nav - std::round(4 * s),
                          tabTop + tabHeight});
        m_tabIds.push_back(tab);
        tabTop += tabHeight + tabGap;
    }
    m_navViewport = {0, navTop, nav, height};
    m_navScroll.SetSizes(height - navTop, tabTop - navTop + padding);
    const float pageLeft = nav + std::round((m_navNarrow ? 8 : 12) * s);
    const float pageRight = width - padding * 1.5f;
    float pageTop = padding;
    if (m_navNarrow) {
        m_searchField = {pageLeft, pageTop, pageRight, pageTop + field};
        pageTop += field + std::round(12 * s);
    }
    m_searchClear = {m_searchField.right - field, m_searchField.top,
                     m_searchField.right, m_searchField.bottom};
    m_pageTitle = {pageLeft, pageTop, pageRight, pageTop + std::round(40 * s)};
    m_cardPad = std::round(12 * s);
    const float cardGap = std::round(4 * s);
    // In a wide window, the small options two on a line (see LineShare),
    // so the page grows wider then.
    // (Not on the setup assistant's steps.)
    const bool columns = !m_search.empty() || m_settingsTab != kTabSetup;
    const float cardWidth = std::max(
        std::round(200 * s),
        std::min(pageRight - pageLeft, std::round((columns ? 1500 : 1100) * s)));
    const float inner = cardWidth - m_cardPad * 2;
    // The island's preview, across the whole page (the pill can be long).
    m_previewShown = m_settingsTab != kTabMonitors &&
                     m_settingsTab < kTabDock;
    float listTop = m_pageTitle.bottom + std::round(10 * s);
    m_previewRow = {};
    if (m_previewShown) {
        m_previewRow = {pageLeft + m_cardPad, listTop + m_cardPad,
                        pageRight - m_cardPad,
                        listTop + m_cardPad + std::round(66 * s)};
        m_orderStrip = {m_previewRow.left + std::round(4 * s),
                        m_previewRow.top + std::round(6 * s),
                        m_previewRow.right - std::round(4 * s),
                        m_previewRow.top + std::round(42 * s)};
        LayoutOrderChips();
        listTop = m_previewRow.bottom + m_cardPad + std::round(12 * s);
    }

    // What's shown: the tab's options, or what matches the search (not the
    // setup assistant's), and the titles of their sections (the order of the
    // items is the preview); the setup assistant's step, without titles.
    const bool setupPage = m_search.empty() && m_settingsTab == kTabSetup;
    bool sectionShown[kSectionCount] = {};
    for (auto& entry : m_settings) {
        if (entry.kind == SettingKind::Label) {
            continue;
        }
        entry.shown = entry.kind != SettingKind::Order &&
                      (setupPage ? SetupShows(m_setup.step, entry)
                       : m_search.empty()
                           ? SettingSectionTab(entry.section) == m_settingsTab
                           : entry.section != kSectionSetup &&
                                 MatchesSearch(SettingText(entry), m_search));
        if (entry.section == kSectionMonitors && MonitorCount() < 2) {
            entry.shown = false;
        }
        sectionShown[entry.section] |= entry.shown;
    }
    float y = 0;
    bool any = false;
    // Two on a line (as wide as half the page each) only with room for it:
    // a switch beside a switch (the line as tall as the taller one), a
    // slider or a choice beside another (each as tall as it is). One left
    // alone takes the whole line.
    const float columnGap = std::round(8 * s);
    const float half = std::floor((cardWidth - columnGap) / 2);
    const float halfInner = half - m_cardPad * 2;
    const bool paired = columns && cardWidth >= std::round(1000 * s);
    SettingEntry* waiting = nullptr;
    int waitingShare = 0;
    auto placeWhole = [&](SettingEntry& entry) {
        const float row = SettingHeight(entry, inner);
        PlaceSettingParts(entry,
                          {pageLeft + m_cardPad, y + m_cardPad,
                           pageLeft + cardWidth - m_cardPad,
                           y + m_cardPad + row},
                          inner);
        y += row + m_cardPad * 2 + cardGap;
    };
    auto placeWaiting = [&] {
        if (waiting) {
            placeWhole(*waiting);
            waiting = nullptr;
        }
    };
    for (auto& entry : m_settings) {
        if (entry.kind == SettingKind::Label) {
            entry.shown = sectionShown[entry.section] && !setupPage;
        }
        if (!entry.shown) {
            continue;
        }
        if (entry.kind == SettingKind::Label) {
            placeWaiting();
            // (Room over a section's title, but at the top.)
            if (y > 0) {
                y += std::round(14 * s);
            }
            const float label = std::round(30 * s);
            PlaceSettingParts(entry,
                              {pageLeft, y, pageLeft + cardWidth, y + label},
                              cardWidth);
            y += label;
            continue;
        }
        any = true;
        const int share = paired ? LineShare(entry, halfInner) : 0;
        if (share && waiting && share == waitingShare) {
            const float leftRow = SettingHeight(*waiting, halfInner);
            const float rightRow = SettingHeight(entry, halfInner);
            const float line = std::max(leftRow, rightRow);
            const float right = pageLeft + half + columnGap;
            PlaceSettingParts(*waiting,
                              {pageLeft + m_cardPad, y + m_cardPad,
                               pageLeft + half - m_cardPad,
                               y + m_cardPad + (share == 1 ? line : leftRow)},
                              halfInner);
            PlaceSettingParts(entry,
                              {right + m_cardPad, y + m_cardPad,
                               pageLeft + cardWidth - m_cardPad,
                               y + m_cardPad + (share == 1 ? line : rightRow)},
                              halfInner);
            waiting = nullptr;
            y += line + m_cardPad * 2 + cardGap;
            continue;
        }
        placeWaiting();
        if (share) {
            waiting = &entry;
            waitingShare = share;
            continue;
        }
        placeWhole(entry);
    }
    placeWaiting();
    m_nothingFound = !any;
    m_disabledKey = DisabledKey();
    const float content = any ? y + padding : std::round(64 * s);
    // The setup assistant's buttons on a line under its page, as wide as its
    // cards: Skip or Cancel at the left, Back and the next step at the
    // right (see PressSetupButton).
    float listBottom = height;
    for (auto& button : m_setupButtons) {
        button = {};
    }
    if (setupPage) {
        const float button = std::round(32 * s);
        const float gap = std::round(8 * s);
        const float right = pageLeft + cardWidth;
        const float buttonWidth = std::min(
            std::round(120 * s), std::floor((cardWidth - gap * 3) / 4));
        const float top = height - padding - button;
        m_setupButtons[kSetupCancel] = {pageLeft, top, pageLeft + buttonWidth,
                                        top + button};
        m_setupButtons[kSetupNext] = {right - buttonWidth, top, right,
                                      top + button};
        if (m_setup.step > kSetupWelcome) {
            m_setupButtons[kSetupBack] = {right - buttonWidth * 2 - gap, top,
                                          right - buttonWidth - gap,
                                          top + button};
        }
        listBottom = top - padding;
    }
    m_settingsViewport = {nav, listTop, width, listBottom};
    m_settingsScroll.SetSizes(listBottom - listTop, content);

    // An entry coming back slides in a little from below, the others glide
    // to their new places.
    bool moving = false;
    for (auto& entry : m_settings) {
        if (entry.shown && entry.alpha < 0.05) {
            entry.x = entry.rect.left;
            entry.y = entry.rect.top + std::round(10 * s);
            entry.vx = entry.vy = 0;
        }
        moving = moving || (entry.shown ? 1.0 : 0.0) != entry.alpha ||
                 std::fabs(entry.x - entry.rect.left) > 0.5 ||
                 std::fabs(entry.y - entry.rect.top) > 0.5;
    }
    if (moving) {
        StartIslandAnimation();
    }
}

int ControlPanel::LineShare(const SettingEntry& entry, float width) const {
    switch (entry.kind) {
        case SettingKind::Pref:
        case SettingKind::Shortcut:
            return 1;
        case SettingKind::Size:
            return 2;
        case SettingKind::Choice:
            return !PictureChoice(entry.value) &&
                           entry.value != kChoiceBannerPlace &&
                           FitChoice(entry.value, width).rows == 1
                       ? 2
                       : 0;
        default:
            return 0;
    }
}

void ControlPanel::DragScrollBar(POINT pt) {
    const D2D1_RECT_F& viewport =
        m_barDrag == 2 ? m_navViewport : m_settingsViewport;
    SmoothScroll& scroll = m_barDrag == 2 ? m_navScroll : m_settingsScroll;
    float top = 0;
    float thumb = 0;
    ScrollThumb(viewport, scroll, &top, &thumb);
    const float inset = std::round(3 * m_scale);
    const float travel = viewport.bottom - viewport.top - inset * 2 - thumb;
    const double at =
        travel > 0 ? (pt.y - m_barGrab - viewport.top - inset) / travel : 0;
    scroll.target = scroll.offset = std::clamp(at, 0.0, 1.0) * scroll.Max();
    scroll.velocity = 0;
    scroll.lastWheel = NowSeconds();
    m_hover = {};
    StartIslandAnimation();
}

void ControlPanel::SettleSettings() {
    for (auto& entry : m_settings) {
        entry.x = entry.rect.left;
        entry.y = entry.rect.top;
        entry.vx = entry.vy = entry.valpha = 0;
        entry.alpha = entry.shown ? 1 : 0;
    }
    for (auto& chip : m_orderChips) {
        chip.x = chip.rect.left;
        chip.vx = 0;
    }
    m_nothingAlpha = m_nothingFound ? 1 : 0;
    m_nothingAlphaVelocity = 0;
}

bool ControlPanel::AnimateSettings(double dt) {
    // (An option dimmed or back: its reason comes or goes.)
    if (m_view == View::Settings && DisabledKey() != m_disabledKey) {
        Layout();
    }
    bool moving = m_settingsScroll.Animate(dt);
    moving |= m_flashEntry >= 0 && NowSeconds() - m_flashAt < kFlashSeconds;
    moving |= m_appPlacesScroll.Animate(dt);
    for (auto& entry : m_settings) {
        moving |= SpringTowards(entry.alpha, entry.valpha, entry.shown ? 1 : 0,
                                0.24, dt);
        if (entry.shown) {
            moving |= SpringTowards(entry.x, entry.vx, entry.rect.left, 0.42,
                                    dt, 0.86);
            moving |= SpringTowards(entry.y, entry.vy, entry.rect.top, 0.42, dt,
                                    0.86);
        }
    }
    for (auto& chip : m_orderChips) {
        if (m_orderMoved && (int)chip.group == m_orderDrag &&
            chip.strip == m_orderDragStrip) {
            // Under the mouse.
            chip.x = m_orderDragLeft;
            chip.vx = 0;
            moving = true;
            continue;
        }
        moving |= SpringTowards(chip.x, chip.vx, chip.rect.left, 0.36, dt, 0.8);
    }
    moving |= m_navScroll.Animate(dt);
    moving |= SpringTowards(m_nothingAlpha, m_nothingAlphaVelocity,
                            m_nothingFound ? 1 : 0, 0.3, dt);
    moving |= SpringTowards(m_tabFade, m_tabFadeVelocity, 1, 0.24, dt);
    return moving;
}

void ControlPanel::OnSearchChanged() {
    m_settingsScroll.target = 0;
    m_hover = {};
    Layout();
    StartIslandAnimation();
}

ControlPanel::Motion* ControlPanel::FindMotion(ControlId id) {
    for (auto& motion : m_motions) {
        if (motion.id == id) {
            return &motion;
        }
    }
    return nullptr;
}

void ControlPanel::SetEditing(bool editing) {
    m_editing = editing;
    m_dragId = -1;
    m_dragMoved = false;
    m_hover = {};
    Layout();
    StartIslandAnimation();
}

// The controls chosen, or the usual ones.
void ControlPanel::LoadControls() {
    if (m_controlsLoaded) {
        return;
    }
    m_controlsLoaded = true;
    m_controls.clear();
    const std::wstring value = GetWholeStringValue(L"controls", 1024);
    if (value.empty()) {
        for (const auto& info : kControls) {
            if (info.shownByDefault) {
                m_controls.push_back(info.id);
            }
        }
        return;
    }
    for (PCWSTR p = value.c_str(); *p;) {
        PCWSTR end = wcschr(p, L',');
        const std::wstring key = end ? std::wstring(p, end) : std::wstring(p);
        for (const auto& info : kControls) {
            if (key == info.key &&
                std::find(m_controls.begin(), m_controls.end(), info.id) ==
                    m_controls.end()) {
                m_controls.push_back(info.id);
            }
        }
        if (!end) {
            break;
        }
        p = end + 1;
    }
}

void ControlPanel::SaveControls() {
    std::wstring value;
    for (ControlId id : m_controls) {
        value += (value.empty() ? L"" : L",") +
                 std::wstring(GetControlInfo(id).key);
    }
    // Nothing chosen is still something chosen.
    StoreProfileString(L"controls", value.empty() ? L"none" : value.c_str());
}

ControlPanel::Hit ControlPanel::HitTest(POINT pt) const {
    if (m_view == View::Wifi || m_view == View::Bluetooth) {
        if (ShowsBack() && PointInRect(pt, m_back)) {
            return {Part::Back};
        }
        const bool wifi = m_view == View::Wifi;
        if ((wifi ? m_wifiRadio : m_bluetoothRadio) >= 0 &&
            PointInRect(pt, m_toggle)) {
            return {wifi ? Part::WifiToggle : Part::BluetoothToggle};
        }
        for (size_t i = 0; i < m_rows.size(); i++) {
            if (!PointInRect(pt, m_rows[i])) {
                continue;
            }
            if (!wifi) {
                return {Part::BluetoothRow, (int)i};
            }
            // Its buttons (all shown with the mouse on the row).
            for (const auto& button : m_rowButtons) {
                if (button.row == i && PointInRect(pt, button.rect)) {
                    return {button.part, (int)i};
                }
            }
            return {Part::WifiRow, (int)i};
        }
        if (wifi && m_wifiRefused && m_networks.empty() &&
            m_wifiStatus.empty() && PointInRect(pt, m_status)) {
            return {Part::WifiLocation};
        }
        if (wifi && m_hotspot >= 0 && PointInRect(pt, m_hotspotRow)) {
            return {Part::HotspotToggle};
        }
        if (wifi && m_airplane >= 0 && PointInRect(pt, m_airplaneRow)) {
            return {Part::AirplaneToggle};
        }
        return {};
    }
    if (m_view == View::Settings) {
        if (!m_search.empty() && PointInRect(pt, m_searchClear)) {
            return {Part::SearchClear};
        }
        // (The side bar scrolls: its tabs at their place in it.)
        if (PointInRect(pt, m_navViewport)) {
            const POINT nav{pt.x,
                            (LONG)std::lround(pt.y + m_navScroll.Offset())};
            for (size_t i = 0; i < m_tabs.size(); i++) {
                if (PointInRect(nav, m_tabs[i])) {
                    return {Part::SettingsTab, m_tabIds[i]};
                }
            }
        }
        // The island's preview (it doesn't scroll).
        if (m_previewShown && PointInRect(pt, m_previewRow)) {
            for (const auto& chip : m_orderChips) {
                if (chip.strip < 0 && PointInRect(pt, chip.rect)) {
                    return {Part::OrderChip, OrderChipIndex(-1, chip.group)};
                }
            }
            return {};
        }
        if (PointInRect(pt, m_searchField)) {
            return {Part::SearchField};
        }
        // The setup assistant's buttons, under its page.
        for (int i = 0; i < kSetupButtons; i++) {
            if (PointInRect(pt, m_setupButtons[i])) {
                return {Part::SetupButton, i};
            }
        }
        if (!PointInRect(pt, m_settingsViewport)) {
            return {};
        }
        const POINT list = ToSettingsList(pt);
        if (m_needsEntry >= 0 && PointInRect(list, m_needsRect)) {
            return {PointInRect(list, m_needsYes)  ? Part::NeedsYes
                    : PointInRect(list, m_needsNo) ? Part::NeedsNo
                                                   : Part::NeedsBox};
        }
        if (m_restoreEntry >= 0 && PointInRect(list, m_restoreLink)) {
            return {Part::RestoreNeeds};
        }
        for (const auto& entry : m_settings) {
            // (Anywhere on its card; a section's title has none.)
            if (!entry.shown || entry.alpha < 0.5 ||
                !PointInRect(list, entry.kind == SettingKind::Label
                                       ? entry.rect
                                       : CardOf(entry.rect))) {
                continue;
            }
            for (const auto& [linked, rect] : m_reasonLinks) {
                if (linked == (int)(&entry - m_settings.data()) &&
                    PointInRect(list, rect)) {
                    return {Part::DisabledLink,
                            linked * kDisabledStride + DisabledIndex(entry)};
                }
            }
            if (entry.kind == SettingKind::WhatsNew ||
                entry.kind == SettingKind::Setup) {
                for (size_t i = 0; i < m_noteLinks.size(); i++) {
                    if (PointInRect(list, m_noteLinks[i].second)) {
                        return {Part::NoteLink, (int)i};
                    }
                }
            }
            switch (entry.kind) {
                case SettingKind::Style:
                    for (int i = 0; i < 3; i++) {
                        if (PointInRect(list, m_styleSegments[i])) {
                            return {Part::StyleSegment, i};
                        }
                    }
                    break;
                case SettingKind::Theme: {
                    const float grow = std::round(3 * m_scale);
                    for (int i = 0; i < (int)ThemeId::Count; i++) {
                        const D2D1_RECT_F& r = m_swatches[i];
                        if (PointInRect(list,
                                        {r.left - grow, r.top - grow,
                                         r.right + grow, r.bottom + grow})) {
                            return {Part::ThemeSwatch, i};
                        }
                    }
                    break;
                }
                case SettingKind::Size: {
                    const int slider = entry.value;
                    if (PointInRect(list, m_sizeReset[slider])) {
                        return {Part::SizeReset, slider};
                    }
                    if (PointInRect(list, m_sizeSlider[slider])) {
                        return {Part::SizeSlider, slider};
                    }
                    if (SliderDisabled(slider)) {
                        return {Part::DisabledSetting,
                                kDisabledSlider + slider};
                    }
                    break;
                }
                case SettingKind::Place:
                    if (!g_settings.islandBar && NotchCountWanted() == 0) {
                        const int dot = PlaceDotAt(m_placeGrid, list);
                        if (dot >= 0) {
                            return {Part::PlaceDot, dot};
                        }
                    }
                    break;
                case SettingKind::AppPlaces:
                    if (PerAppPlacesOn()) {
                        return {Part::AppPlacesRow};
                    }
                    return {Part::DisabledSetting, kDisabledAppPlaces};
                case SettingKind::Icons:
                    for (const auto& cell : m_iconCells) {
                        if ((int)kIcons[cell.icon].group != entry.value ||
                            !PointInRect(list, cell.rect)) {
                            continue;
                        }
                        return {HasOwnIconLook(kIcons[cell.icon].glyph) &&
                                        PointInRect(list, cell.reset)
                                    ? Part::IconCellReset
                                : PointInRect(list, cell.paint)
                                    ? Part::IconCellColor
                                    : Part::IconCell,
                                cell.icon};
                    }
                    break;
                case SettingKind::LyricsApps:
                    if (const int disabled = DisabledIndex(entry);
                        disabled >= 0) {
                        return {Part::DisabledSetting, disabled};
                    }
                    for (size_t i = 0; i < m_lyricsAppChips.size(); i++) {
                        if (PointInRect(list, m_lyricsAppChips[i])) {
                            return {Part::LyricsAppChip, (int)i};
                        }
                    }
                    break;
                case SettingKind::Monitors:
                    for (size_t i = 0; i < m_monitorParts.size(); i++) {
                        const auto& part = m_monitorParts[i];
                        if (!part.disabled && PointInRect(list, part.rect)) {
                            return {part.part, (int)i};
                        }
                    }
                    break;
                case SettingKind::Profiles:
                    for (size_t i = 0; i < m_profilePencils.size(); i++) {
                        if (PointInRect(list, m_profilePencils[i])) {
                            return {Part::ProfileEditButton, (int)i};
                        }
                    }
                    for (size_t i = 0; i < m_profileChips.size(); i++) {
                        if (PointInRect(list, m_profileChips[i])) {
                            return {Part::ProfileChip, (int)i};
                        }
                    }
                    if (m_profileEdit >= 0) {
                        if (PointInRect(list, m_profileNameField)) {
                            return {Part::ProfileName};
                        }
                        if (PointInRect(list, m_profileUpdate)) {
                            return {Part::ProfileUpdate};
                        }
                        if (PointInRect(list, m_profileDelete)) {
                            return {Part::ProfileDelete};
                        }
                        if (PointInRect(list, m_profileDone)) {
                            return {Part::ProfileDone};
                        }
                        for (size_t i = 0; i < m_profileStyleChips.size();
                             i++) {
                            if (PointInRect(list, m_profileStyleChips[i])) {
                                return {Part::ProfileStyleChip, (int)i};
                            }
                        }
                        for (size_t i = 0; i < m_profileAppChips.size(); i++) {
                            if (PointInRect(list, m_profileAppChips[i])) {
                                return {Part::ProfileAppChip, (int)i};
                            }
                        }
                        if (PointInRect(list, m_profileAppAdd)) {
                            return {Part::ProfileAppAdd};
                        }
                        for (int i = 0; i < 2; i++) {
                            if (PointInRect(list, m_profileKeep[i])) {
                                return {Part::ProfileKeepChip, i};
                            }
                        }
                    }
                    break;
                case SettingKind::Order:
                    for (const auto& chip : m_orderChips) {
                        if (chip.strip < 0 && PointInRect(list, chip.rect)) {
                            return {Part::OrderChip,
                                    OrderChipIndex(-1, chip.group)};
                        }
                    }
                    break;
                case SettingKind::Pref:
                    if (!PrefDisabled((IslandPref)entry.value)) {
                        return {Part::SettingToggle, entry.value};
                    }
                    return {Part::DisabledSetting, entry.value};
                case SettingKind::Choice:
                    if (ChoiceDisabled(entry.value)) {
                        return {Part::DisabledSetting,
                                kDisabledChoice + entry.value};
                    }
                    if (entry.value == kChoiceIconPack) {
                        for (int i = 0; i < kIconPacks; i++) {
                            if (m_iconPackOnDisk[i] &&
                                IconPackProgress(i) < 0 &&
                                PointInRect(list, m_iconPackDelete[i])) {
                                return {Part::IconPackDelete, i};
                            }
                        }
                    }
                    {
                        const auto& segments = m_choiceSegments[entry.value];
                        for (size_t i = 0; i < segments.size(); i++) {
                            if (PointInRect(list, segments[i])) {
                                return {Part::ChoiceSegment,
                                        entry.value * 16 + (int)i};
                            }
                        }
                    }
                    break;
                case SettingKind::Label:
                    if (PointInRect(list, m_profileAdd)) {
                        return {Part::ProfileAdd};
                    }
                    break;
                case SettingKind::Dock:
                case SettingKind::Logs:
                    for (const auto& [id, rect] : m_pageButtons) {
                        if (PointInRect(list, rect)) {
                            return {Part::PageButton, id};
                        }
                    }
                    break;
                case SettingKind::About:
                    // (All of its card.)
                    return {Part::PageButton, kPageLink + entry.value};
                case SettingKind::WhatsNew:
                case SettingKind::Setup:
                    // (Only read, but its items with an option, above.)
                    break;
                case SettingKind::Shortcut:
                    if (ShortcutDisabled()) {
                        return {Part::DisabledSetting, kDisabledShortcut};
                    }
                    if (PointInRect(list, m_shortcutReset)) {
                        return {Part::ShortcutReset};
                    }
                    if (PointInRect(list, m_shortcutChip)) {
                        return {Part::ShortcutRecord};
                    }
                    break;
            }
            return {};
        }
        return {};
    }
    if (m_view == View::AppPlaces) {
        if (ShowsBack() && PointInRect(pt, m_back)) {
            return {Part::Back};
        }
        if (!PointInRect(pt, m_appPlacesViewport)) {
            return {};
        }
        pt = ToAppPlacesList(pt);
        for (const auto& chip : m_orderChips) {
            if (chip.strip >= 0 && PointInRect(pt, chip.rect)) {
                return {Part::OrderChip,
                        OrderChipIndex(chip.strip, chip.group)};
            }
        }
        for (const auto& [index, rect] : m_appItemChips) {
            if (PointInRect(pt, rect)) {
                return {Part::AppItemChip, index};
            }
        }
        for (size_t i = 0; i < m_appPlaceRows.size(); i++) {
            if (PointInRect(pt, m_appPlaceRemoves[i])) {
                return {Part::AppPlaceRemove, (int)i};
            }
            const int dot = PlaceDotAt(m_appPlaceGrids[i], pt);
            if (dot >= 0) {
                return {Part::AppPlaceDot, (int)i * 9 + dot};
            }
        }
        for (size_t i = 0; i < m_openAppChips.size(); i++) {
            if (PointInRect(pt, m_openAppChips[i])) {
                return {Part::AppPlaceAdd, (int)i};
            }
        }
        return {};
    }
    if (m_view == View::Project || m_view == View::Accessibility) {
        if (ShowsBack() && PointInRect(pt, m_back)) {
            return {Part::Back};
        }
        for (size_t i = 0; i < m_rows.size(); i++) {
            if (PointInRect(pt, m_rows[i])) {
                return {m_view == View::Project ? Part::ProjectRow
                                                : Part::AccessibilityRow,
                        (int)i};
            }
        }
        return {};
    }
    if (m_view == View::Password || m_view == View::Hotspot) {
        if (ShowsBack() && PointInRect(pt, m_back)) {
            return {Part::Back};
        }
        if (PointInRect(pt, m_eye)) {
            return {Part::ShowPassword};
        }
        if (PointInRect(pt, m_field)) {
            return {Part::PasswordField};
        }
        if (m_view == View::Hotspot && PointInRect(pt, m_hotspotField)) {
            return {Part::HotspotName};
        }
        if (PointInRect(pt, m_connect)) {
            return {m_view == View::Hotspot ? Part::HotspotSave
                                            : Part::Connect};
        }
        return {};
    }
    if (m_view != View::Main) {
        if (ShowsBack() && PointInRect(pt, m_back)) {
            return {Part::Back};
        }
        for (size_t i = 0; i < m_rows.size(); i++) {
            if (!PointInRect(pt, m_rows[i])) {
                continue;
            }
            if (m_view == View::Output) {
                return {Part::Output, (int)i};
            }
            if (m_view == View::PowerPlans) {
                return {Part::PowerPlanRow, (int)i};
            }
            // Row 0 is the device's volume (index -1).
            const int index = (int)i - 1;
            if (PointInRect(pt, m_rowIcons[i])) {
                return index < 0 ? Hit{Part::VolumeIcon} : Hit{Part::AppIcon,
                                                                index};
            }
            return {Part::AppSlider, index};
        }
        return {};
    }
    // Editing: the "-" first (it sticks out of the corner), the controls, the
    // ones to add, "Done".
    if (m_editing) {
        const float badge = std::round(11 * m_scale);
        for (const auto& [id, rect] : m_controlRects) {
            if (PointInRect(pt, {rect.left - badge, rect.top - badge,
                                 rect.left + badge * 1.4f,
                                 rect.top + badge * 1.4f})) {
                return {Part::EditRemove, (int)id};
            }
        }
        for (const auto& [id, rect] : m_controlRects) {
            if (PointInRect(pt, rect)) {
                return {Part::EditControl, (int)id};
            }
        }
        for (const auto& [id, rect] : m_addRects) {
            if (PointInRect(pt, rect)) {
                return {Part::EditAdd, (int)id};
            }
        }
        if (PointInRect(pt, m_done)) {
            return {Part::EditDone};
        }
        return {};
    }
    for (const auto& [id, rect] : m_controlRects) {
        if (!PointInRect(pt, rect)) {
            continue;
        }
        switch (id) {
            case ControlId::Network:
                return {m_wifiRadio >= 0 && PointInRect(pt, m_networkIcon)
                            ? Part::WifiToggle
                            : Part::Network};
            case ControlId::Bluetooth:
                return {m_bluetoothRadio >= 0 &&
                                PointInRect(pt, m_bluetoothIcon)
                            ? Part::BluetoothToggle
                            : Part::Bluetooth};
            case ControlId::DoNotDisturb:
                return {PointInRect(pt, m_dndIcon) ? Part::DndToggle
                                                   : Part::DoNotDisturb};
            case ControlId::NightLight:
                return {Part::NightLight};
            case ControlId::Hotspot:
                // (Its icon switches it, the rest opens its page.)
                return {m_hotspot < 0 ? Part::None
                        : PointInRect(pt, m_hotspotIcon) ? Part::HotspotToggle
                                                         : Part::HotspotEdit};
            case ControlId::Airplane:
                return {m_airplane >= 0 ? Part::AirplaneToggle : Part::None};
            case ControlId::Display: {
                // (The row under the mouse: its name and its slider.)
                int row = 0;
                for (size_t i = 1; i < m_brightnessSliders.size(); i++) {
                    if (pt.y >= m_brightnessSliders[i - 1].bottom) {
                        row = (int)i;
                    }
                }
                return {Part::BrightnessSlider, row};
            }
            case ControlId::Sound:
                if (PointInRect(pt, m_volumeIcon)) {
                    return {Part::VolumeIcon};
                }
                if (PointInRect(pt, m_slider)) {
                    return {Part::Slider};
                }
                if (PointInRect(pt, m_deviceRow)) {
                    return {Part::Device};
                }
                if (PointInRect(pt, m_mixerButton)) {
                    return {Part::MixerButton};
                }
                return {};
            case ControlId::Battery:
                return {Part::Battery};
            case ControlId::PowerMode:
                for (int i = 0; i < 3; i++) {
                    if (PointInRect(pt, m_powerModeChoices[i])) {
                        return {Part::PowerModeSegment, i};
                    }
                }
                if (PointInRect(pt, m_powerPlanRow)) {
                    return {Part::PowerPlan};
                }
                return {};
            case ControlId::Komorebi:
                for (int i = 0; i < 3; i++) {
                    if (PointInRect(pt, m_komorebiButtons[i])) {
                        return {KomorebiEnabled(i) ? Part::KomorebiButton
                                                   : Part::None,
                                i};
                    }
                }
                return {};
            default:
                return {Part::Shortcut, (int)id};
        }
    }
    if (PointInRect(pt, m_edit)) {
        return {Part::EditButton};
    }
    if (PointInRect(pt, m_settingsButton)) {
        return {Part::SettingsButton};
    }
    if (PointInRect(pt, m_more)) {
        return {Part::More};
    }
    return {};
}

void ControlPanel::DrawTile(ID2D1RenderTarget* target,
                            ID2D1SolidColorBrush* brush,
                            const D2D1_RECT_F& rect,
                            WCHAR glyph,
                            bool active,
                            PCWSTR title,
                            const std::wstring& subtitle,
                            bool hover) {
    const float s = m_scale;
    const float radius = ControlRadius(rect, s);
    brush->SetColor(Fg(0.08f + 0.06f * Hover(rect, hover)));
    GlassFill(target, {rect, radius, radius}, brush);

    // The icon in a circle, blue when it's on.
    const float circle = std::round(17 * s);
    const D2D1_POINT_2F center{rect.left + std::round(12 * s) + circle,
                               (rect.top + rect.bottom) / 2};
    brush->SetColor(active ? Accent(1) : Fg(0.18f));
    GlassFillEllipse(target, {center, circle, circle}, brush);
    brush->SetColor(Fg(0.95f));
    DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
              {center.x - circle, center.y - circle, center.x + circle,
               center.y + circle},
              brush);

    const float textLeft = center.x + circle + std::round(9 * s);
    const float middle = (rect.top + rect.bottom) / 2;
    // Long names scroll (under the mouse) instead of being cut.
    DrawMarquee(target, brush, title, m_titleFormat.get(),
                {textLeft, middle - std::round(17 * s),
                 rect.right - std::round(8 * s), middle});
    brush->SetColor(Fg(0.55f));
    DrawMarquee(target, brush, subtitle, m_subtitleFormat.get(),
                {textLeft, middle, rect.right - std::round(8 * s),
                 middle + std::round(16 * s)});
}

// Text that doesn't fit scrolls to its end and back while the mouse is over
// it, with a pause at each end, like on macOS (otherwise its start shows);
// text that fits is drawn as it is. Uses the brush's color.
void ControlPanel::DrawMarquee(ID2D1RenderTarget* target,
                               ID2D1SolidColorBrush* brush,
                               const std::wstring& text,
                               IDWriteTextFormat* format,
                               const D2D1_RECT_F& rect,
                               bool centered) {
    if (text.empty() || !format) {
        return;
    }
    const float width = rect.right - rect.left;
    const float height = rect.bottom - rect.top;
    winrt::com_ptr<IDWriteTextLayout> layout;
    float textWidth = 0;
    for (const auto& cached : m_marqueeLayouts) {
        // The size too: a format made again (another scaling) can have the
        // same address.
        if (cached.format == format && cached.height == height &&
            cached.fontSize == format->GetFontSize() && cached.text == text) {
            layout = cached.layout;
            textWidth = cached.width;
            break;
        }
    }
    if (!layout) {
        if (FAILED(m_dwrite->CreateTextLayout(text.c_str(),
                                              (UINT32)text.size(), format,
                                              10000, height, layout.put()))) {
            return;
        }
        layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        DWRITE_TEXT_METRICS metrics;
        layout->GetMetrics(&metrics);
        textWidth = metrics.widthIncludingTrailingWhitespace;
        // Texts change (like a network's name): the old ones go. (The
        // settings' window shows more of them at once: the icons' names.)
        if (m_marqueeLayouts.size() >= (m_windowed ? 512u : 96u)) {
            m_marqueeLayouts.clear();
        }
        m_marqueeLayouts.push_back(
            {text, format, format->GetFontSize(), height, layout, textWidth});
    }
    float x = rect.left;
    if (textWidth <= width) {
        if (centered) {
            x += (width - textWidth) / 2;
        }
    } else if (ScrollsHere(target, rect) && AnimationLevel() == 0) {
        m_marqueeActive = true;
        x -= (float)MarqueeShift(NowSeconds() - m_scrollSince,
                                 textWidth - width, m_scale);
    }
    target->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
    target->DrawTextLayout({x, rect.top}, layout.get(), brush,
                           D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);
    target->PopAxisAlignedClip();
}

// A text this long (the logs) isn't kept whole for each width it's wrapped
// at (see DrawWrapped, WrappedHeight): resizing the window makes many.
constexpr size_t kLongWrappedText = 2048;

void ControlPanel::DrawWrapped(ID2D1RenderTarget* target,
                               ID2D1SolidColorBrush* brush,
                               const std::wstring& text,
                               IDWriteTextFormat* format,
                               const D2D1_RECT_F& rect,
                               bool centered) {
    const float width = rect.right - rect.left;
    const float height = rect.bottom - rect.top;
    if (text.empty() || !format || width <= 1 || height <= 1) {
        return;
    }
    winrt::com_ptr<IDWriteTextLayout> layout;
    for (const auto& cached : m_wrappedLayouts) {
        if (cached.format == format && cached.width == width &&
            cached.height == height && cached.centered == centered &&
            cached.fontSize == format->GetFontSize() && cached.text == text) {
            layout = cached.layout;
            break;
        }
    }
    if (!layout) {
        if (FAILED(m_dwrite->CreateTextLayout(text.c_str(), (UINT32)text.size(),
                                              format, width, height,
                                              layout.put()))) {
            return;
        }
        layout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        layout->SetTextAlignment(centered ? DWRITE_TEXT_ALIGNMENT_CENTER
                                          : DWRITE_TEXT_ALIGNMENT_LEADING);
        layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        if (m_wrappedLayouts.size() >= (m_windowed ? 512u : 96u)) {
            m_wrappedLayouts.clear();
        } else if (text.size() > kLongWrappedText) {
            std::erase_if(m_wrappedLayouts, [&](const auto& cached) {
                return cached.text == text;
            });
        }
        m_wrappedLayouts.push_back({text, format, format->GetFontSize(), width,
                                    height, centered, layout});
    }
    target->DrawTextLayout({rect.left, rect.top}, layout.get(), brush);
}

float ControlPanel::WrappedHeight(const std::wstring& text,
                                  IDWriteTextFormat* format,
                                  float width) const {
    winrt::com_ptr<IDWriteTextLayout> layout;
    if (!format) {
        return 0;
    }
    // (Measured once: the settings measure every row as they're laid out
    // and drawn.)
    WCHAR tail[48];
    swprintf_s(tail, L"\x1F%p\x1F%.1f\x1F%.2f", (void*)format,
               format->GetFontSize(), width);
    const std::wstring key =
        (text.size() > kLongWrappedText
             ? L"\x1E" + std::to_wstring(std::hash<std::wstring>{}(text))
             : text) +
        tail;
    if (const auto it = m_wrappedHeights.find(key);
        it != m_wrappedHeights.end()) {
        return it->second;
    }
    float height;
    if (text.empty() || width <= 1 ||
        FAILED(m_dwrite->CreateTextLayout(text.c_str(), (UINT32)text.size(),
                                          format, width, 10000,
                                          layout.put()))) {
        height = std::ceil(format->GetFontSize() * 1.33f);
    } else {
        layout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        DWRITE_TEXT_METRICS metrics{};
        layout->GetMetrics(&metrics);
        height = std::ceil(metrics.height);
    }
    if (m_wrappedHeights.size() >= 4096) {
        m_wrappedHeights.clear();
    }
    m_wrappedHeights.emplace(key, height);
    return height;
}

// Scrolling texts move while the control center is open; so do switches,
// the controls while editing (sliding to their places), and
// removed ones shrinking away.
bool ControlPanel::AnimateContents(double dt) {
    // A text scrolling under the mouse never stops (see m_idleMotion), nor
    // a text box's caret while it blinks (a few seconds after typing).
    // (Nor an icon pack's download in the settings, see IconPackProgress.)
    const bool idle = m_marqueeActive ||
                      ((m_view == View::Settings || m_view == View::Password ||
                        m_view == View::Hotspot) &&
                       CaretBlinking()) ||
                      (m_view == View::Settings && IconPacksDownloading());
    bool moving = false;
    if (SpringTowards(m_viewIn, m_viewInVelocity, 1, 0.42, dt)) {
        moving = true;
    }
    if (SpringTowards(m_editAmount, m_editAmountVelocity, m_editing ? 1 : 0,
                      0.32, dt)) {
        moving = true;
    }
    for (auto& knob : m_knobs) {
        if (SpringTowards(knob.value, knob.velocity, knob.target, 0.3, dt,
                          kBounceDamping)) {
            moving = true;
        }
    }
    for (int id = 0; id < kChoiceCount; id++) {
        const double chosen = ChoiceValue(id);
        // Buttons only (the backup's): no knob.
        if (chosen < 0) {
            continue;
        }
        if (m_choiceKnob[id] < 0) {
            m_choiceKnob[id] = chosen;
        }
        if (SpringTowards(m_choiceKnob[id], m_choiceKnobVelocity[id], chosen,
                          0.35, dt, kBounceDamping)) {
            moving = true;
        }
    }
    const double style = StyleSegmentStyle(GetIslandPref(IslandPref::Bar));
    if (m_styleKnob < 0) {
        m_styleKnob = style;
    }
    if (SpringTowards(m_styleKnob, m_styleKnobVelocity, style, 0.35, dt,
                      kBounceDamping)) {
        moving = true;
    }
    for (auto& motion : m_motions) {
        if (m_dragMoved && (int)motion.id == m_dragId) {
            // Under the mouse.
            motion.x = m_dragPoint.x - m_dragGrab.x;
            motion.y = m_dragPoint.y - m_dragGrab.y;
            motion.vx = motion.vy = 0;
            moving = true;
            continue;
        }
        for (const auto& [id, rect] : m_controlRects) {
            if (id != motion.id) {
                continue;
            }
            if (SpringTowards(motion.x, motion.vx, rect.left, 0.42, dt, 0.8)) {
                moving = true;
            }
            if (SpringTowards(motion.y, motion.vy, rect.top, 0.42, dt, 0.8)) {
                moving = true;
            }
        }
        if (SpringTowards(motion.scale, motion.vscale, 1, 0.38, dt,
                          kBounceDamping)) {
            moving = true;
        }
    }
    for (auto& ghost : m_ghosts) {
        if (SpringTowards(ghost.amount, ghost.velocity, 0, 0.26, dt)) {
            moving = true;
        }
    }
    std::erase_if(m_ghosts,
                  [](const Ghost& ghost) { return ghost.amount <= 0; });
    if ((m_view == View::Settings || m_view == View::AppPlaces) &&
        AnimateSettings(dt)) {
        moving = true;
    }
    // A profile just saved says so for a moment.
    if (m_view == View::Settings && NowSeconds() - m_profileSavedAt < 1.6) {
        moving = true;
    }
    m_idleMotion = idle && !moving;
    return moving || idle;
}

float ControlPanel::KnobFor(int key, bool on) {
    const double target = on ? 1 : 0;
    for (auto& knob : m_knobs) {
        if (knob.key == key) {
            if (knob.target != target) {
                knob.target = target;
                StartIslandAnimation();
            }
            return (float)knob.value;
        }
    }
    m_knobs.push_back({key, target, 0, target});
    return (float)target;
}

// A small button: an icon in a circle (blue when it's on), its name under it.
void ControlPanel::DrawShortcut(ID2D1RenderTarget* target,
                                ID2D1SolidColorBrush* brush,
                                const D2D1_RECT_F& rect,
                                WCHAR glyph,
                                PCWSTR name,
                                bool active,
                                bool hover) {
    const float s = m_scale;
    const float tileRadius = ControlRadius(rect, s);
    brush->SetColor(Fg(0.08f + 0.06f * Hover(rect, hover)));
    GlassFill(target, {rect, tileRadius, tileRadius}, brush);
    const float circle = std::round(14 * s);
    const D2D1_POINT_2F center{(rect.left + rect.right) / 2,
                               rect.top + std::round(8 * s) + circle};
    brush->SetColor(active ? Accent(1) : Fg(0.18f));
    GlassFillEllipse(target, {center, circle, circle}, brush);
    brush->SetColor(Fg(0.95f));
    DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
              {center.x - circle, center.y - circle, center.x + circle,
               center.y + circle},
              brush);
    brush->SetColor(Fg(0.75f));
    DrawMarquee(target, brush, name, m_smallFormat.get(),
                {rect.left + std::round(4 * s), center.y + circle,
                 rect.right - std::round(4 * s),
                 rect.bottom - std::round(2 * s)},
                true);
}

// A thick slider like on macOS: filled up to the level, with a round knob,
// and optionally an icon inside its start.
void ControlPanel::DrawSlider(ID2D1RenderTarget* target,
                              ID2D1SolidColorBrush* brush,
                              const D2D1_RECT_F& rect,
                              float level,
                              WCHAR glyph,
                              bool hoverIcon) {
    const float radius = (rect.bottom - rect.top) / 2;
    brush->SetColor(Fg(0.16f));
    target->FillRoundedRectangle({rect, radius, radius}, brush);
    const float knob = radius * 2;
    const float fillRight =
        rect.left + knob + (rect.right - rect.left - knob) *
                               std::clamp(level, 0.0f, 1.0f);
    brush->SetColor(Fg(0.92f));
    target->FillRoundedRectangle(
        {{rect.left, rect.top, fillRight, rect.bottom}, radius, radius}, brush);
    brush->SetColor(D2D1::ColorF(1, 1, 1, 1));
    target->FillEllipse(
        {{fillRight - radius, rect.top + radius}, radius - 1, radius - 1},
        brush);
    if (glyph) {
        brush->SetColor(Bg(hoverIcon ? 0.95f : 0.7f));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                  {rect.left, rect.top, rect.left + knob * 1.2f,
                   rect.bottom},
                  brush);
    }
}

// A switch like on macOS: blue with the knob on the right when on.
void ControlPanel::DrawToggle(ID2D1RenderTarget* target,
                              ID2D1SolidColorBrush* brush,
                              const D2D1_RECT_F& rect,
                              float on) {
    const float radius = (rect.bottom - rect.top) / 2;
    // The track goes from gray to blue; the knob slides, a bit springy.
    const float t = std::clamp(on, 0.0f, 1.0f);
    const D2D1_COLOR_F off = Fg(0.22f);
    const D2D1_COLOR_F accent = Accent(1);
    brush->SetColor(D2D1::ColorF(off.r + (accent.r - off.r) * t,
                                 off.g + (accent.g - off.g) * t,
                                 off.b + (accent.b - off.b) * t,
                                 off.a + (1.0f - off.a) * t));
    GlassFill(target, {rect, radius, radius}, brush);
    const float knob = radius - std::round(2 * m_scale);
    const float travel = rect.right - rect.left - radius * 2;
    brush->SetColor(KnobColor(t));
    target->FillEllipse({{rect.left + radius + travel * std::clamp(on, -0.1f, 1.1f),
                          rect.top + radius},
                         knob,
                         knob},
                        brush);
}

winrt::com_ptr<IDWriteTextLayout> ControlPanel::StatusLayout(
    const std::wstring& status,
    float width) const {
    winrt::com_ptr<IDWriteTextLayout> layout;
    if (!m_linkFormat ||
        FAILED(m_dwrite->CreateTextLayout(status.c_str(), (UINT32)status.size(),
                                          m_linkFormat.get(), width, 10000,
                                          layout.put()))) {
        return nullptr;
    }
    layout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    return layout;
}

void ControlPanel::DrawStatus(ID2D1RenderTarget* target,
                              ID2D1SolidColorBrush* brush) {
    const std::wstring status = StatusText();
    if (status.empty()) {
        return;
    }
    auto layout = StatusLayout(status, m_status.right - m_status.left);
    DWRITE_TEXT_METRICS metrics{};
    if (!layout || FAILED(layout->GetMetrics(&metrics))) {
        return;
    }
    // (The line that opens the location's page: a link.)
    if (m_hover.part == Part::WifiLocation) {
        layout->SetUnderline(true, {0, (UINT32)status.size()});
    }
    brush->SetColor(m_view == View::Wifi && m_wifiRefused &&
                            m_networks.empty() && m_wifiStatus.empty()
                        ? AccentInk(1)
                        : Fg(0.55f));
    target->DrawTextLayout(
        {m_status.left,
         (m_status.top + m_status.bottom - metrics.height) / 2},
        layout.get(), brush);
}

void ControlPanel::DrawHeader(ID2D1RenderTarget* target,
                              ID2D1SolidColorBrush* brush,
                              PCWSTR title,
                              int toggle) {
    const bool back = ShowsBack();
    if (const float lit =
            back ? Hover(m_back, m_hover.part == Part::Back) : 0.0f;
        lit > 0.01f) {
        const float inset = std::round(6 * m_scale);
        const D2D1_RECT_F back{m_back.left + inset, m_back.top + inset,
                               m_back.right - inset, m_back.bottom - inset};
        const float radius = (back.bottom - back.top) / 2;
        brush->SetColor(Fg(0.14f * lit));
        GlassFill(target, {back, radius, radius}, brush);
    }
    brush->SetColor(Fg(0.95f));
    if (back) {
        DrawGlyph(target, &kGlyphBack, 1, m_iconFormat.get(), m_back, brush);
    }
    const float titleRight = toggle >= 0
                                 ? m_toggle.left - std::round(8 * m_scale)
                                 : (float)m_size.cx - m_back.left;
    target->DrawText(title, (UINT32)wcslen(title), m_sectionFormat.get(),
                     {back ? m_back.right + std::round(4 * m_scale)
                           : m_back.left + std::round(6 * m_scale),
                      m_back.top, titleRight, m_back.bottom},
                     brush);
    if (toggle >= 0) {
        DrawToggle(target, brush, m_toggle,
                   KnobFor(1000 + (int)m_view, toggle == 1));
    }
}

void ControlPanel::DrawWifi(ID2D1RenderTarget* target,
                            ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    DrawHeader(target, brush, L"Wi-Fi", m_wifiRadio);
    const float icon = std::round(36 * s);
    // The network cable: connected (and its speed) or not.
    if (m_wiredRow.bottom > m_wiredRow.top) {
        const D2D1_RECT_F& row = m_wiredRow;
        if (m_wired.connected) {
            const float radius = std::round(10 * s);
            brush->SetColor(Fg(0.08f));
            GlassFill(target, {row, radius, radius}, brush);
        }
        const WCHAR glyph = 0xE839;
        brush->SetColor(m_wired.connected ? Accent(1) : Fg(0.6f));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                  {row.left, row.top, row.left + icon, row.bottom},
                  brush);
        std::wstring text = Tr(L"Network cable", L"Cabo de rede");
        if (!m_wired.connected) {
            text += L" \u00B7 ";
            text += Tr(L"not connected", L"desconectado");
        } else if (m_wired.speed) {
            const double megabits = m_wired.speed / 1e6;
            WCHAR speed[32];
            if (megabits >= 1000) {
                swprintf_s(speed, L" \u00B7 %g Gbps", megabits / 1000);
            } else {
                swprintf_s(speed, L" \u00B7 %g Mbps", megabits);
            }
            text += std::wstring(L" \u00B7 ") +
                    Tr(L"Connected", L"Conectado") + speed;
        }
        brush->SetColor(Fg(m_wired.connected ? 0.95f : 0.6f));
        target->DrawText(text.c_str(), (UINT32)text.size(),
                         m_subtitleFormat.get(),
                         {row.left + icon, row.top, row.right, row.bottom},
                         brush);
    }
    for (size_t i = 0; i < m_networks.size() && i < m_rows.size(); i++) {
        const WifiNetwork& network = m_networks[i];
        const D2D1_RECT_F& row = m_rows[i];
        const bool hover =
            (m_hover.part == Part::WifiRow ||
             m_hover.part == Part::Disconnect || m_hover.part == Part::Forget ||
             m_hover.part == Part::WifiConnect) &&
            m_hover.index == (int)i;
        const float lit = Hover(row, hover);
        if (lit > 0.01f || network.connected) {
            const float radius = std::round(10 * s);
            const float rest = network.connected ? 0.08f : 0.0f;
            brush->SetColor(Fg(rest + (0.14f - rest) * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        // The signal, blue when connected.
        const WCHAR glyph = kGlyphWifi[std::clamp(network.signal / 25, 0, 3)];
        brush->SetColor(network.connected ? Accent(1) : Fg(0.95f));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                  {row.left, row.top, row.left + icon, row.bottom},
                  brush);
        // Its buttons: the connected one's always, the others' under the
        // mouse (and "Forget it?" while it waits for its second click).
        float right = row.right - icon;
        for (const auto& button : m_rowButtons) {
            if (button.row != i || !RowButtonShown(button, hover)) {
                continue;
            }
            right = std::min(right, button.rect.left - std::round(6 * s));
            const bool armed = button.part == Part::Forget &&
                               network.profile == m_forgetProfile;
            const float buttonLit =
                Hover(HoverKey(4006, (long long)i * 8 + (int)button.part),
                      m_hover.part == button.part && m_hover.index == (int)i);
            const float radius = (button.rect.bottom - button.rect.top) / 2;
            brush->SetColor(armed ? D2D1::ColorF(0.86f, 0.2f, 0.18f,
                                                 0.85f + 0.15f * buttonLit)
                            : button.part == Part::WifiConnect
                                ? AccentLit(0.9f, buttonLit)
                                : Fg(0.1f + 0.08f * buttonLit));
            target->FillRoundedRectangle({button.rect, radius, radius}, brush);
            PCWSTR label =
                button.part == Part::Forget
                    ? (armed ? Tr(L"Forget it?", L"Esquecer mesmo?")
                             : Tr(L"Forget", L"Esquecer"))
                : button.part == Part::Disconnect
                    ? Tr(L"Disconnect", L"Desconectar")
                    : Tr(L"Connect", L"Conectar");
            brush->SetColor(armed ? D2D1::ColorF(1, 1, 1, 1)
                            : button.part == Part::WifiConnect ? AccentInk(1)
                                                               : Fg(0.9f));
            // (m_smallFormat is centered.)
            target->DrawText(label, (UINT32)wcslen(label), m_smallFormat.get(),
                             button.rect, brush);
        }
        brush->SetColor(Fg(0.95f));
        target->DrawText(network.name.c_str(), (UINT32)network.name.size(),
                         m_subtitleFormat.get(),
                         {row.left + icon, row.top, right, row.bottom}, brush);
        if (right >= row.right - icon && network.secure &&
            network.profile.empty()) {
            brush->SetColor(Fg(0.5f));
            DrawGlyph(target, &kGlyphLock, 1, m_iconFormat.get(),
                      {row.right - icon, row.top, row.right,
                       row.bottom},
                      brush);
        }
    }
    DrawStatus(target, brush);
    if (m_hotspot >= 0 && m_hotspotRow.bottom > m_hotspotRow.top) {
        DrawRowToggle(target, brush, m_hotspotRow, kGlyphHotspot,
                      Tr(L"Mobile hotspot", L"Hotspot móvel"), m_hotspot == 1,
                      m_hover.part == Part::HotspotToggle);
    }
    if (m_airplane >= 0 && m_airplaneRow.bottom > m_airplaneRow.top) {
        DrawRowToggle(target, brush, m_airplaneRow, kGlyphAirplane,
                      Tr(L"Airplane mode", L"Modo avião"), m_airplane == 1,
                      m_hover.part == Part::AirplaneToggle);
    }
}

// A network's button: Disconnect always (on the connected one), the others
// with the mouse on their row, and "Forget it?" while it waits.
bool ControlPanel::RowButtonShown(const RowButton& button,
                                  bool overRow) const {
    if (overRow) {
        return true;
    }
    const WifiNetwork& network = m_networks[button.row];
    return (button.part == Part::Disconnect && network.connected) ||
           (button.part == Part::Forget && !network.profile.empty() &&
            network.profile == m_forgetProfile);
}

// A row with an icon, a name and a switch on the right.
void ControlPanel::DrawRowToggle(ID2D1RenderTarget* target,
                                 ID2D1SolidColorBrush* brush,
                                 const D2D1_RECT_F& row,
                                 WCHAR glyph,
                                 PCWSTR name,
                                 bool on,
                                 bool hover,
                                 bool toggle) {
    const float s = m_scale;
    if (const float lit = Hover(row, hover); lit > 0.01f) {
        const float radius = std::round(10 * s);
        brush->SetColor(Fg(0.08f * lit));
        GlassFill(target, {row, radius, radius}, brush);
    }
    const float icon = std::round(36 * s);
    brush->SetColor(Fg(0.95f));
    DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
              {row.left, row.top, row.left + icon, row.bottom}, brush);
    // The switch (or the button's arrow) at the end.
    const float toggleWidth = std::round(36 * s);
    const float toggleHeight = std::round(20 * s);
    const D2D1_RECT_F area{
        row.right - std::round(8 * s) - toggleWidth,
        std::round((row.top + row.bottom - toggleHeight) / 2),
        row.right - std::round(8 * s),
        std::round((row.top + row.bottom - toggleHeight) / 2) + toggleHeight};
    brush->SetColor(Fg(0.95f));
    target->DrawText(name, (UINT32)wcslen(name), m_subtitleFormat.get(),
                     {row.left + icon, row.top, area.left - std::round(8 * s),
                      row.bottom},
                     brush);
    if (toggle) {
        DrawToggle(target, brush, area, KnobFor(2000 + glyph, on));
    } else {
        // A button: an arrow saying it opens.
        const WCHAR open = 0xE8A7;
        brush->SetColor(Fg(0.5f));
        DrawGlyph(target, &open, 1, m_smallIconFormat.get(), area, brush);
    }
}

void ControlPanel::DrawProject(ID2D1RenderTarget* target,
                               ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    DrawHeader(target, brush, Tr(L"Project", L"Projetar"));
    static const int kModes[] = {
        DISPLAYCONFIG_TOPOLOGY_INTERNAL, DISPLAYCONFIG_TOPOLOGY_CLONE,
        DISPLAYCONFIG_TOPOLOGY_EXTEND, DISPLAYCONFIG_TOPOLOGY_EXTERNAL};
    const PCWSTR names[] = {
        Tr(L"PC screen only", L"Somente tela do PC"),
        Tr(L"Duplicate", L"Duplicar"), Tr(L"Extend", L"Estender"),
        Tr(L"Second screen only", L"Somente segunda tela")};
    const float icon = std::round(36 * s);
    for (size_t i = 0; i < 4 && i < m_rows.size(); i++) {
        const D2D1_RECT_F& row = m_rows[i];
        const bool hover =
            m_hover.part == Part::ProjectRow && m_hover.index == (int)i;
        const bool current = m_displayMode == kModes[i];
        const float lit = Hover(row, hover);
        if (lit > 0.01f || current) {
            const float radius = std::round(10 * s);
            const float rest = current ? 0.08f : 0.0f;
            brush->SetColor(Fg(rest + (0.14f - rest) * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        brush->SetColor(Fg(0.95f));
        DrawGlyph(target, &kGlyphProject, 1, m_iconFormat.get(),
                  {row.left, row.top, row.left + icon, row.bottom},
                  brush);
        target->DrawText(names[i], (UINT32)wcslen(names[i]),
                         m_subtitleFormat.get(),
                         {row.left + icon, row.top, row.right - icon,
                          row.bottom},
                         brush);
        if (current) {
            brush->SetColor(Accent(1));
            DrawGlyph(target, &kGlyphCheck, 1, m_iconFormat.get(),
                      {row.right - icon, row.top, row.right,
                       row.bottom},
                      brush);
        }
    }
}

// A theme's color, for its swatch.
D2D1_COLOR_F ThemeSwatchColor(int index) {
    switch ((ThemeId)index) {
        case ThemeId::Graphite:
            return D2D1::ColorF(0.17f, 0.17f, 0.19f, 1);
        case ThemeId::Light:
            return D2D1::ColorF(0.96f, 0.96f, 0.97f, 1);
        case ThemeId::Accent: {
            DWORD color = 0;
            BOOL opaque = FALSE;
            if (SUCCEEDED(DwmGetColorizationColor(&color, &opaque))) {
                return D2D1::ColorF(((color >> 16) & 0xFF) / 255.0f * 0.55f,
                                    ((color >> 8) & 0xFF) / 255.0f * 0.55f,
                                    (color & 0xFF) / 255.0f * 0.55f, 1);
            }
            return D2D1::ColorF(0.1f, 0.25f, 0.45f, 1);
        }
        case ThemeId::Glass:
            return D2D1::ColorF(0.19f, 0.19f, 0.21f, 0.6f);
        case ThemeId::AutoInverted:
            return ThemeSwatchColor((int)ThemeId::Light);
        case ThemeId::Custom: {
            float parts[3];
            CustomColorParts(g_customColors[kColorBackground], parts);
            return D2D1::ColorF(parts[0], parts[1], parts[2], 1);
        }
        default:
            return D2D1::ColorF(0, 0, 0, 1);
    }
}

PCWSTR ThemeName(int index) {
    switch ((ThemeId)index) {
        case ThemeId::Graphite:
            return Tr(L"Graphite", L"Grafite");
        case ThemeId::Light:
            return Tr(L"Light", L"Claro");
        case ThemeId::Accent:
            return Tr(L"Accent color", L"Cor de destaque");
        case ThemeId::Glass:
            return Tr(L"Glass", L"Vidro");
        case ThemeId::Auto:
            return Tr(L"Auto (light or dark)",
                      L"Automático (claro ou escuro)");
        case ThemeId::AutoInverted:
            return Tr(L"Auto, inverted (the other way from Windows)",
                      L"Automático invertido (o contrário do Windows)");
        case ThemeId::Custom:
            return Tr(L"Your own colors", L"Cores próprias");
        default:
            return Tr(L"Black", L"Preto");
    }
}

// A little screen with the 9 ready places as dots: the chosen one blue, or,
// for a place of its own (dragged there), a blue dot where it is.
void ControlPanel::DrawPlaceGrid(ID2D1RenderTarget* target,
                                 ID2D1SolidColorBrush* brush,
                                 const D2D1_RECT_F& grid,
                                 double x,
                                 double y,
                                 int hover,
                                 float opacity) {
    const float s = m_scale;
    const float radius = std::round(5 * s);
    brush->SetColor(Fg(0.1f * opacity));
    GlassFill(target, {grid, radius, radius}, brush);
    brush->SetColor(Fg(0.25f * opacity));
    target->DrawRoundedRectangle(
        {{grid.left + 0.5f, grid.top + 0.5f, grid.right - 0.5f,
          grid.bottom - 0.5f},
         radius,
         radius},
        brush, 1);
    const float inset = std::round(6 * s);
    auto at = [&](double fx, double fy) {
        return D2D1_POINT_2F{
            grid.left + inset + (float)fx * (grid.right - grid.left - inset * 2),
            grid.top + inset + (float)fy * (grid.bottom - grid.top - inset * 2)};
    };
    bool ready = false;
    for (int i = 0; i < 9; i++) {
        const double fx = (i % 3) / 2.0;
        const double fy = (i / 3) / 2.0;
        const bool chosen = std::fabs(fx - x) < 0.01 && std::fabs(fy - y) < 0.01;
        ready = ready || chosen;
        const float dot = (chosen ? 3.2f : 2.2f) * s *
                          (hover == i ? 1.35f : 1.0f);
        brush->SetColor(chosen ? Accent(opacity)
                               : Fg((hover == i ? 0.8f : 0.4f) * opacity));
        target->FillEllipse({at(fx, fy), dot, dot}, brush);
    }
    // Somewhere of its own.
    if (!ready) {
        brush->SetColor(Accent(opacity));
        target->FillEllipse({at(x, y), 3.2f * s, 3.2f * s}, brush);
    }
}

int ControlPanel::PlaceDotAt(const D2D1_RECT_F& grid, POINT pt) {
    if (!PointInRect(pt, grid)) {
        return -1;
    }
    const int column = std::clamp(
        (int)((pt.x - grid.left) * 3 / (grid.right - grid.left)), 0, 2);
    const int row = std::clamp(
        (int)((pt.y - grid.top) * 3 / (grid.bottom - grid.top)), 0, 2);
    return row * 3 + column;
}

BOOL CALLBACK ListOpenAppsProc(HWND hWnd, LPARAM lParam) {
    auto* apps = reinterpret_cast<std::vector<std::wstring>*>(lParam);
    if (apps->size() >= 20 || !IsWindowVisible(hWnd) ||
        GetWindow(hWnd, GW_OWNER) ||
        (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) ||
        !GetWindowTextLength(hWnd)) {
        return TRUE;
    }
    BOOL cloaked = FALSE;
    DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked,
                          sizeof(cloaked));
    if (cloaked) {
        return TRUE;
    }
    // Not the island's own windows.
    DWORD processId = 0;
    GetWindowThreadProcessId(hWnd, &processId);
    WCHAR className[64] = L"";
    GetClassName(hWnd, className, ARRAYSIZE(className));
    if (processId == GetCurrentProcessId() &&
        (_wcsicmp(className, kIslandClassName) == 0 ||
         _wcsicmp(className, kPanelClassName) == 0)) {
        return TRUE;
    }
    std::wstring appId = WindowAppKey(hWnd);
    if (appId.empty() || FindAppPlace(appId) ||
        std::find(apps->begin(), apps->end(), appId) != apps->end()) {
        return TRUE;
    }
    apps->push_back(std::move(appId));
    return TRUE;
}

// The apps open now that have no place of their own yet.
void ControlPanel::ListOpenApps() {
    m_openApps.clear();
    EnumWindows(
        ListOpenAppsProc,
        (LPARAM)&m_openApps);
}

// The island's sliders: what each one changes, its range and steps.
struct IslandSlider {
    int min;
    int max;
    int usual;
    int step;
};
constexpr IslandSlider kIslandSliders[kIslandSliderCount] = {
    {kIslandSizeMin, kIslandSizeMax, kIslandSizeDefault, kIslandSizeStep},
    {kIslandGapMin, kIslandGapMax, kIslandGapDefault, kIslandGapStep},
    {kAnimationSpeedMin, kAnimationSpeedMax, kAnimationSpeedDefault,
     kAnimationSpeedStep},
    {kTrayListMin, kTrayListMax, kTrayListDefault, kTrayListStep},
    {kIslandOpacityMin, kIslandOpacityMax, kIslandOpacityDefault,
     kIslandOpacityStep},
    {kAutoHideDelayMin, kAutoHideDelayMax, kAutoHideDelayDefault,
     kAutoHideDelayStep},
    {kNotchCurveMin, kNotchCurveMax, kNotchCurveDefault, kNotchCurveStep},
    {kIconSizeMin, kIconSizeMax, kIconSizeDefault, kIconSizeStep},
    {kLineOpacityMin, kLineOpacityMax, kLineOpacityDefault, kLineOpacityStep},
    {kActivationWidthMin, kActivationWidthMax, kActivationWidthDefault,
     kActivationWidthStep},
    {kPanelSizeMin, kPanelSizeMax, kPanelSizeDefault, kPanelSizeStep},
    {kMediaTitleLimitMin, kMediaTitleLimitMax, kMediaTitleLimitDefault,
     kMediaTitleLimitStep},
    {kPillHeightPctMin, kPillHeightPctMax, kPillHeightPctDefault,
     kPillHeightPctStep},
    {kPillCornersMin, kPillCornersMax, kPillCornersDefault, kPillCornersStep},
    {kPillEndsMin, kPillEndsMax, kPillEndsDefault, kPillEndsStep},
    {kEdgeMin, kEdgeMax, kEdgeDefault, kEdgeStep},
    {kGlassBlurMin, kGlassBlurMax, kGlassBlurDefault, kGlassBlurStep},
    {kTrayColumnsMin, kTrayColumnsMax, kTrayColumnsDefault, kTrayColumnsStep},
    {kVizSensitivityMin, kVizSensitivityMax, kVizSensitivityDefault,
     kVizSensitivityStep},
    {kTextSizeMin, kTextSizeMax, kTextSizeDefault, kTextSizeStep},
};

// The apps with a place of their own, and the open ones to add.
void ControlPanel::DrawAppPlaces(ID2D1RenderTarget* target,
                                 ID2D1SolidColorBrush* brush) {
    const float s = m_scale;
    DrawHeader(target, brush,
               Tr(L"Place and icons per app", L"Posição e ícones por app"));
    // The list is laid out where it starts: moved up by its scroll only.
    const D2D1_RECT_F& viewport = m_appPlacesViewport;
    BeginScrollArea(target, viewport, m_appPlacesScroll);
    D2D1_MATRIX_3X2_F list;
    target->GetTransform(&list);
    target->SetTransform(D2D1::Matrix3x2F::Translation(0, -viewport.top) *
                         *D2D1::Matrix3x2F::ReinterpretBaseType(&list));
    for (size_t i = 0; i < m_appPlaceRows.size() && i < g_appPlaces.size();
         i++) {
        const D2D1_RECT_F& row = m_appPlaceRows[i];
        const IslandPlace& place = g_appPlaces[i];
        const bool hover = (m_hover.part == Part::AppPlaceDot &&
                            m_hover.index / 9 == (int)i) ||
                           (m_hover.part == Part::AppPlaceRemove &&
                            m_hover.index == (int)i);
        if (const float lit = Hover(row, hover); lit > 0.01f) {
            const float radius = std::round(10 * s);
            brush->SetColor(Fg(0.06f * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        const float nameBottom = m_appPlaceRemoves[i].bottom;
        brush->SetColor(Fg(0.92f));
        DrawMarquee(target, brush, AppDisplayName(place.appId),
                    m_subtitleFormat.get(),
                    {row.left + std::round(10 * s), row.top,
                     m_appPlaceGrids[i].left - std::round(8 * s), nameBottom});
        // Its order, then its icons: lit when shown while it's in front,
        // dimmed and struck through when hidden (fading between the two).
        if (i < m_appOrderStrips.size()) {
            DrawOrderStrip(target, brush, {}, (int)i);
        }
        for (const auto& [index, chip] : m_appItemChips) {
            if (index / kAppChipStride != (int)i) {
                continue;
            }
            const int group = index % kAppChipStride;
            const bool lit =
                m_hover.part == Part::AppItemChip && m_hover.index == index;
            const float round = std::round(8 * s);
            if (group >= kStepChipFirst) {
                // Its room from the edges and size: its name and value (its
                // own, or the one for all apps), the slider, and the button
                // back to the one for all apps (dimmed while it is).
                const int step = group - kStepChipFirst;
                const int which = step / 3;
                const int value = which ? place.size : place.gap;
                const IslandSlider& info = kIslandSliders[which ? 0 : 1];
                const int usual = IslandSliderValue(which ? 0 : 1);
                const bool own = value >= 0;
                const int shownValue = own ? value : usual;
                if (step % 3 == 0) {
                    const std::wstring text =
                        (which ? std::wstring(Tr(L"Size", L"Tamanho")) +
                                     L" \u00B7 " + std::to_wstring(shownValue) +
                                     L"%"
                               : std::wstring(Tr(L"Edge", L"Borda")) +
                                     L" \u00B7 " + std::to_wstring(shownValue) +
                                     L" px") +
                        (own ? L"" : Tr(L" (all apps)", L" (todos)"));
                    brush->SetColor(Fg(own ? 0.9f : 0.55f));
                    D2D1_RECT_F name = chip;
                    for (const auto& [other, otherRect] : m_appItemChips) {
                        if (other == index + 1) {
                            name.right = otherRect.left - std::round(6 * s);
                        }
                    }
                    DrawMarquee(target, brush, text, m_smallFormat.get(), name);
                } else if (step % 3 == 1) {
                    DrawSliderTrack(target, brush, chip, info.min, info.max,
                                    info.usual, shownValue,
                                    lit || (m_dragging &&
                                            m_dragIndex ==
                                                kDragAppSlider -
                                                    ((int)i * 2 + which)));
                } else {
                    if (const float glow = Hover(chip, own && lit);
                        glow > 0.01f) {
                        const float radius = (chip.bottom - chip.top) / 2;
                        brush->SetColor(Fg(0.12f * glow));
                        GlassFill(target, {chip, radius, radius}, brush);
                    }
                    brush->SetColor(Fg(own ? 0.85f : 0.25f));
                    const WCHAR restore = 0xE72C;
                    DrawGlyph(target, &restore, 1, m_smallIconFormat.get(),
                              chip, brush);
                }
                continue;
            }
            if (group >= kMediaChipFirst && group < kClockChipFirst) {
                // What's playing's buttons, or its cover and title: lit when
                // shown for this app (each on its own, like its note).
                const int part = group == kMediaChipFirst ? kAppMediaButtons
                                                          : kAppMediaTitle;
                const int parts =
                    place.media >= 0
                        ? place.media
                        : (g_mediaButtonsInPill ? kAppMediaButtons : 0) |
                              (g_mediaTitleInPill ? kAppMediaTitle : 0);
                const float on = std::clamp(
                    KnobFor(5000 + index, (parts & part) != 0), 0.0f, 1.0f);
                brush->SetColor(
                    Fg((0.08f + 0.08f * Hover(chip, lit)) * (0.5f + on / 2)));
                GlassFill(target, {chip, round, round}, brush);
                const WCHAR glyph = part == kAppMediaButtons ? kGlyphPlay
                                                             : (WCHAR)0xE93C;
                brush->SetColor(Fg(0.3f + 0.65f * on));
                DrawGlyph(target, &glyph, 1, m_smallIconFormat.get(), chip,
                          brush);
                if (on < 0.99f) {
                    const float strike = std::round(8 * s);
                    brush->SetColor(Fg(0.6f * (1 - on)));
                    target->DrawLine({chip.left + strike, chip.bottom - strike},
                                     {chip.right - strike, chip.top + strike},
                                     brush,
                                     std::max(1.0f, std::round(1.5f * s)));
                }
                continue;
            }
            if (group >= kClockChipFirst) {
                // The clock's parts and the clock in the middle: lit when so
                // for this app.
                const int part = group - kClockChipFirst;
                const int parts =
                    place.clock >= 0 ? place.clock : GlobalClockParts();
                const bool partOn = group == kClockCenterChip
                                        ? PlaceClockCenter(&place)
                                        : ((parts >> part) & 1) != 0;
                const float on =
                    std::clamp(KnobFor(5000 + index, partOn), 0.0f, 1.0f);
                const bool partHover = m_hover.part == Part::AppItemChip &&
                                       m_hover.index == index;
                const float partRadius = std::round(8 * s);
                brush->SetColor(Fg((0.08f + 0.08f * Hover(chip, partHover)) *
                                   (0.5f + on / 2)));
                GlassFill(target, {chip, partRadius, partRadius}, brush);
                static const PCWSTR kNames[kClockParts + 1][2] = {
                    {L"Seconds", L"Segundos"},
                    {L"Weekday", L"Dia sem."},
                    {L"Date", L"Data"},
                    {L"Year", L"Ano"},
                    {L"Middle", L"No meio"}};
                PCWSTR name = Tr(kNames[part][0], kNames[part][1]);
                brush->SetColor(Fg(0.3f + 0.65f * on));
                target->DrawText(name, (UINT32)wcslen(name), m_smallFormat.get(),
                                 chip, brush);
                continue;
            }
            const float shown = std::clamp(
                KnobFor(5000 + index,
                        !((place.own ? place.hidden
                                     : place.hidden | GloballyHiddenGroups()) &
                          (1ull << group)) &&
                            OnlineGroupAllowed((PillGroup)group)),
                0.0f,
                1.0f);
            const bool chipHover =
                m_hover.part == Part::AppItemChip && m_hover.index == index;
            const float chipRadius = std::round(8 * s);
            brush->SetColor(
                Fg((0.08f + 0.08f * Hover(chip, chipHover)) * (0.5f + shown / 2)));
            GlassFill(target, {chip, chipRadius, chipRadius}, brush);
            const WCHAR glyph = kPillGroupInfo[group].glyph;
            brush->SetColor(Fg(0.3f + 0.65f * shown));
            DrawGlyph(target, &glyph, 1, m_smallIconFormat.get(), chip, brush);
            if (shown < 0.99f) {
                const float inset = std::round(8 * s);
                brush->SetColor(Fg(0.6f * (1 - shown)));
                target->DrawLine({chip.left + inset, chip.bottom - inset},
                                 {chip.right - inset, chip.top + inset}, brush,
                                 std::max(1.0f, std::round(1.5f * s)));
            }
        }
        DrawPlaceGrid(target, brush, m_appPlaceGrids[i], place.x, place.y,
                      m_hover.part == Part::AppPlaceDot &&
                              m_hover.index / 9 == (int)i
                          ? m_hover.index % 9
                          : -1,
                      1);
        const WCHAR close = kGlyphEventDelete;
        brush->SetColor(Fg(0.45f + 0.5f * Hover(m_appPlaceRemoves[i],
                                                m_hover.part ==
                                                        Part::AppPlaceRemove &&
                                                    m_hover.index == (int)i)));
        DrawGlyph(target, &close, 1, m_iconFormat.get(), m_appPlaceRemoves[i],
                  brush);
    }
    PCWSTR add = m_openApps.empty()
                     ? Tr(L"Open an app to give it a place",
                          L"Abra um app para dar uma posição a ele")
                     : Tr(L"Add an open app", L"Adicionar um app aberto");
    brush->SetColor(Fg(0.5f));
    target->DrawText(add, (UINT32)wcslen(add), m_smallFormat.get(),
                     m_addAppsHeader, brush);
    for (size_t i = 0; i < m_openAppChips.size() && i < m_openApps.size();
         i++) {
        const D2D1_RECT_F& chip = m_openAppChips[i];
        const bool hover =
            m_hover.part == Part::AppPlaceAdd && m_hover.index == (int)i;
        const float radius = std::round(10 * s);
        brush->SetColor(Fg(0.07f + 0.07f * Hover(chip, hover)));
        GlassFill(target, {chip, radius, radius}, brush);
        brush->SetColor(Fg(0.85f));
        const std::wstring name = L"+  " + AppDisplayName(m_openApps[i]);
        DrawMarquee(target, brush, name, m_smallFormat.get(),
                    {chip.left + std::round(10 * s), chip.top,
                     chip.right - std::round(8 * s), chip.bottom});
    }
    PCWSTR hint =
        g_settings.islandFreeDrag
            ? Tr(L"While the app is in front, the pill moves there and shows "
                 L"only the lit icons. Dragging it then changes its place too.",
                 L"Com o app na frente, a pílula vai para lá e mostra só os "
                 L"ícones acesos. Arrastar a pílula também muda a posição.")
            : Tr(L"While the app is in front, the pill moves there and shows "
                 L"only the lit icons.",
                 L"Com o app na frente, a pílula vai para lá e mostra só os "
                 L"ícones acesos.");
    brush->SetColor(Fg(0.45f));
    DrawMarquee(target, brush, hint, m_smallFormat.get(), m_status, true);
    EndScrollArea(target);
    DrawScrollBar(target, brush, viewport, m_appPlacesScroll);
}

// The island's own options: its style, its colors, and what it shows.
// A slider moved: its value set (kept when `keep`).
void SetIslandSlider(int slider, int value, bool keep) {
    if (slider >= 12) {
        SetPillLook(slider, value, keep);
    } else if (slider == 11) {
        SetMediaTitleLimit(value, keep);
    } else if (slider == 10) {
        SetPanelSize(value, keep);
    } else if (slider == 9) {
        SetActivationWidth(value, keep);
    } else if (slider == 8) {
        SetLineOpacity(value, keep);
    } else if (slider == 7) {
        SetIconSize(value, keep);
    } else if (slider == 6) {
        SetNotchCurve(value, keep);
    } else if (slider == 5) {
        SetAutoHideDelay(value, keep);
    } else if (slider == 4) {
        SetIslandOpacity(value, keep);
    } else if (slider == 3) {
        SetTrayListIcons(value, keep);
    } else if (slider == 2) {
        AnimationChoice choice = g_animationChoice;
        choice.speed = value;
        SetAnimationChoice(choice, keep);
        StartIslandAnimation();
    } else if (slider == 1) {
        SetIslandGap(value, keep);
    } else {
        SetIslandSize(value, keep);
    }
}

float PillHeightScale() {
    return g_settings.islandPillHeight / 100.0f;
}

float EdgeAmount() {
    return g_settings.islandEdge / 100.0f;
}

int IslandSliderValue(int slider) {
    return slider == 19   ? g_settings.islandTextSize
           : slider == 18 ? g_settings.islandVizSensitivity
           : slider == 17 ? g_settings.islandTrayColumns
           : slider == 16 ? g_settings.islandGlassBlur
           : slider == 15 ? g_settings.islandEdge
           : slider == 14 ? g_settings.islandPillEnds
           : slider == 13 ? g_settings.islandPillCorners
           : slider == 12 ? g_settings.islandPillHeight
           : slider == 11 ? g_settings.islandMediaTitleLimit
           : slider == 10 ? g_settings.islandPanelSize
           : slider == 9  ? g_settings.islandActivationWidth
           : slider == 8  ? g_settings.islandLineOpacity
           : slider == 7  ? g_settings.islandIconSize
           : slider == 6  ? g_settings.islandNotchCurve
           : slider == 5  ? g_settings.islandAutoHideDelay
           : slider == 4  ? g_islandOpacity
           : slider == 3  ? g_trayListIcons
           : slider == 2  ? g_animationChoice.speed
           : slider == 1  ? g_settings.islandGap
                          : g_settings.islandSize;
}

