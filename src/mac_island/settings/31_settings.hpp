#pragma once
void ResetIslandSettings();

std::wstring ChoiceOption(int id, int index) {
    switch (id) {
        case kChoiceAnimationLevel:
            return index == 0   ? Tr(L"All", L"Todas")
                   : index == 1 ? Tr(L"Partial", L"Parciais")
                                : Tr(L"None", L"Nenhuma");
        case kChoiceFrameRate:
            return index == 0 ? std::wstring(Tr(L"Screen", L"Tela"))
                              : std::to_wstring(kFrameRates[index]);
        case kChoiceBackup:
            return index == 0   ? Tr(L"Export", L"Exportar")
                   : index == 1 ? Tr(L"Import", L"Importar")
                   : NowSeconds() - g_resetAskedAt < kResetAskSeconds
                       ? Tr(L"Sure?", L"Certeza?")
                       : Tr(L"Reset", L"Redefinir");
        case kChoiceColors:
            return index == kColorBackground ? Tr(L"Fill", L"Fundo")
                   : index == kColorText     ? Tr(L"Text", L"Texto")
                                             : Tr(L"Icons", L"Ícones");
        case kChoiceDateOrder:
            return index == 0   ? Tr(L"Auto", L"Auto")
                   : index == 1 ? Tr(L"D M Y", L"D M A")
                   : index == 2 ? Tr(L"M D Y", L"M D A")
                                : Tr(L"Y M D", L"A M D");
        case kChoiceClockFormat:
            return index == 0   ? Tr(L"Windows", L"Windows")
                   : index == 1 ? Tr(L"24 h", L"24 h")
                                : Tr(L"12 h AM/PM", L"12 h AM/PM");
        case kChoiceWeatherUnit:
            return index == 0   ? Tr(L"Windows", L"Windows")
                   : index == 1 ? Tr(L"°C", L"°C")
                                : Tr(L"°F", L"°F");
        case kChoiceIconPack:
            // (The folder chosen by its name.)
            return index == kIconPackWindows     ? Tr(L"Windows", L"Windows")
                   : index == kIconPackWindows10 ? Tr(L"Windows 10",
                                                      L"Windows 10")
                   : index == kIconPackEmoji     ? Tr(L"Emoji", L"Emoji")
                   : index == kIconPackFluent    ? Tr(L"Fluent (online)",
                                                      L"Fluent (online)")
                   : index == kIconPackIonicons  ? Tr(L"Ionicons (online)",
                                                      L"Ionicons (online)")
                   : index == kIconPackLucide    ? Tr(L"Lucide (online)",
                                                      L"Lucide (online)")
                   : index == kIconPackTabler    ? Tr(L"Tabler (online)",
                                                      L"Tabler (online)")
                   : index == kIconPackPhosphor  ? Tr(L"Phosphor (online)",
                                                      L"Phosphor (online)")
                   : index == kIconPackMaterial  ? Tr(L"Material (online)",
                                                      L"Material (online)")
                   : index == kIconPackBootstrap ? Tr(L"Bootstrap (online)",
                                                      L"Bootstrap (online)")
                   : index == kIconPackIconoir   ? Tr(L"Iconoir (online)",
                                                      L"Iconoir (online)")
                   : index == kIconPackHeroicons ? Tr(L"Heroicons (online)",
                                                      L"Heroicons (online)")
                   : g_iconFolder.empty()
                       ? Tr(L"Your folder…", L"Sua pasta…")
                       : std::wstring(PathFindFileName(g_iconFolder.c_str()));
        case kChoiceIconFolder:
            return index == 0   ? Tr(L"Choose it…", L"Escolher…")
                   : index == 1 ? Tr(L"Open it", L"Abrir")
                                : Tr(L"Save Windows' icons",
                                     L"Salvar ícones do Windows");
        case kChoiceIconColors:
            return index == 0 ? Tr(L"Like the island", L"Como a ilha")
                              : Tr(L"Their own", L"As próprias");
        case kChoiceTrayStyle:
            return index == 0   ? Tr(L"In the pill", L"Na pílula")
                   : index == 1 ? Tr(L"Grid", L"Grade")
                                : Tr(L"With names", L"Com nomes");
        case kChoiceWallpaper:
            return index == 0   ? Tr(L"Fade", L"Esmaecer")
                   : index == 1 ? Tr(L"Circle", L"Círculo")
                   : index == 2 ? Tr(L"Slide", L"Deslizar")
                                : Tr(L"Zoom", L"Zoom");
        case kChoiceBarSide:
            return index == 0   ? Tr(L"Top", L"Topo")
                   : index == 1 ? Tr(L"Left", L"Esquerda")
                                : Tr(L"Right", L"Direita");
        case kChoiceBannerSize:
            return kBannerSizeNames[std::clamp(
                index, 0, (int)std::size(kBannerSizeNames) - 1)];
        case kChoiceShelfLimit: {
            // (Numbers only: the same in every language.)
            static const PCWSTR kNames[] = {L"5", L"10", L"20", L"30"};
            static_assert(std::size(kNames) == std::size(kShelfLimits));
            return kNames[std::clamp(index, 0, (int)std::size(kNames) - 1)];
        }
        case kChoiceEyeTheme:
            return index == 0   ? Tr(L"Emerald (Toothless)", L"Esmeralda (Banguela)")
                   : index == 1 ? Tr(L"Ocean blue", L"Azul oceano")
                   : index == 2 ? Tr(L"Sunset amber", L"Âmbar do pôr do sol")
                   : index == 3 ? Tr(L"Mystic violet", L"Violeta místico")
                   : index == 4 ? Tr(L"Crimson", L"Carmesim")
                                : Tr(L"Arctic ice", L"Gelo do Ártico");
        case kChoiceEyeShape:
            return index == 0   ? Tr(L"Toothless", L"Banguela")
                   : index == 1 ? Tr(L"Round", L"Redondo")
                   : index == 2 ? Tr(L"Wide oval", L"Oval largo")
                   : index == 3 ? Tr(L"Almond", L"Amêndoa")
                                : Tr(L"Soft triangle", L"Triângulo suave");
        case kChoiceBannerWidth:
            return index == 0   ? Tr(L"Narrow", L"Estreito")
                   : index == 1 ? Tr(L"Normal", L"Normal")
                   : index == 2 ? Tr(L"Wide", L"Largo")
                                : Tr(L"Extra wide", L"Bem largo");
        case kChoiceBannerCorners:
            return index == 0   ? Tr(L"Like the panels", L"Como os painéis")
                   : index == 1 ? Tr(L"Less round", L"Menos redondos")
                                : Tr(L"Square", L"Quadrados");
        case kChoiceBannerPlace:
            return index == 0   ? Tr(L"Following the bell", L"Seguindo o sino")
                   : index == 1 ? Tr(L"Top left", L"Em cima à esquerda")
                   : index == 2 ? Tr(L"Top", L"Em cima")
                   : index == 3 ? Tr(L"Top right", L"Em cima à direita")
                   : index == 4 ? Tr(L"Left", L"Esquerda")
                   : index == 5 ? Tr(L"Middle", L"Meio")
                   : index == 6 ? Tr(L"Right", L"Direita")
                   : index == 7 ? Tr(L"Bottom left", L"Embaixo à esquerda")
                   : index == 8 ? Tr(L"Bottom", L"Embaixo")
                                : Tr(L"Bottom right", L"Embaixo à direita");
        case kChoiceBarInnerCorners:
            return index == 0   ? Tr(L"None", L"Nenhum")
                   : index == 1 ? Tr(L"Small", L"Pequenos")
                   : index == 2 ? Tr(L"Medium", L"Médios")
                                : Tr(L"Large", L"Grandes");
        case kChoiceLineClick:
            return index == 0 ? Tr(L"With the mouse", L"Com o mouse")
                              : Tr(L"With a click", L"Com um clique");
        case kChoiceLineFade:
            return index == 0   ? Tr(L"Never", L"Nunca")
                   : index == 1 ? Tr(L"30 s", L"30 s")
                   : index == 2 ? Tr(L"2 min", L"2 min")
                                : Tr(L"10 min", L"10 min");
        case kChoiceHideWhen:
            return index == 0   ? Tr(L"Mouse away", L"Mouse longe")
                   : index == 1 ? Tr(L"Window maximized", L"Janela maximizada")
                                : Tr(L"Window over it", L"Janela por cima");
        case kChoiceNotchOpen:
            return index == 0   ? Tr(L"All", L"Todas")
                   : index == 1 ? Tr(L"Left", L"Esquerda")
                   : index == 2 ? Tr(L"Middle", L"Meio")
                                : Tr(L"Right", L"Direita");
        case kChoiceNotches:
            return index == 0   ? Tr(L"None", L"Nenhum")
                   : index == 1 ? Tr(L"Middle", L"Meio")
                   : index == 2 ? Tr(L"Corners", L"Cantos")
                                : Tr(L"Three", L"Três");
        case kChoiceFolderDrag:
            return index == 0   ? Tr(L"Ask the first time",
                                     L"Perguntar na primeira vez")
                   : index == 1 ? Tr(L"Ask every time", L"Perguntar sempre")
                   : index == 2 ? Tr(L"Copy", L"Copiar")
                                : Tr(L"Move", L"Mover");
        case kChoiceLanguage:
        case kChoiceNotesLanguage: {
            // (Each language by its own name.)
            const int language = id == kChoiceLanguage ? index : index + 1;
            return language == 0   ? Tr(L"Automatic", L"Automático")
                   : language == 1 ? L"English"
                   : language == 2 ? L"Português"
                   : language == 3 ? L"中文"
                   : language == 4 ? L"Deutsch"
                   : language == 5 ? L"Español"
                   : language == 6 ? L"Русский"
                   : language == 7 ? L"日本語"
                                   : L"Tiếng Việt";
        }
        case kChoiceFillColor:
        case kChoiceFillCopyColor:
        case kChoiceFillChargeColor:
            return index == 0   ? Tr(L"Green", L"Verde")
                   : index == 1 ? Tr(L"Blue", L"Azul")
                   : index == 2 ? Tr(L"Windows accent", L"Destaque do Windows")
                                : Tr(L"Customize…", L"Personalizar…");
        case kChoiceLyricsSize:
            return index == 0   ? Tr(L"Small", L"Pequena")
                   : index == 1 ? Tr(L"Medium", L"Média")
                                : Tr(L"Large", L"Grande");
        case kChoiceLyricsWidth:
            return index == kLyricsWidthFollows
                       ? Tr(L"Follows the line", L"Segue a linha")
                   : index == 1 ? Tr(L"Short", L"Curta")
                   : index == 2 ? Tr(L"Medium", L"Média")
                                : Tr(L"Long", L"Longa");
        case kChoiceLyricsColor:
            return index == 0   ? Tr(L"The text's", L"A do texto")
                   : index == 1 ? Tr(L"Windows accent", L"Destaque do Windows")
                                : Tr(L"Customize…", L"Personalizar…");
        case kChoiceVisualizerColor:
            return index == 0   ? Tr(L"The icons'", L"A dos ícones")
                   : index == 1 ? Tr(L"Windows accent", L"Destaque do Windows")
                   : index == kVisualizerRainbow ? Tr(L"Rainbow", L"Arco-íris")
                   : index == kVisualizerCover ? Tr(L"The cover's", L"A da capa")
                                : Tr(L"Customize…", L"Personalizar…");
        case kChoiceRecycleMinSize: {
            // (Always, or the size: in GB from 1 GB.)
            if (index <= 0 ||
                index >= (int)std::size(kRecycleMinMegabytes)) {
                return Tr(L"Always", L"Sempre");
            }
            const int megabytes = kRecycleMinMegabytes[index];
            return megabytes >= 1024
                       ? std::to_wstring(megabytes / 1024) + L" GB"
                       : std::to_wstring(megabytes) + L" MB";
        }
        case kChoiceFillStyle:
            return index == 0   ? Tr(L"Simple", L"Simples")
                   : index == 1 ? Tr(L"Liquid wave", L"Onda líquida")
                                : Tr(L"Glow and particles",
                                     L"Brilho e partículas");
        case kChoiceSetupStyle:
            return index == 0   ? Tr(L"Island", L"Ilha")
                   : index == 1 ? Tr(L"Fixed", L"Fixa")
                                : Tr(L"Bar", L"Barra");
        case kChoiceSetupItems:
            return index == 0   ? Tr(L"Essential", L"Essencial")
                   : index == 1 ? Tr(L"Everyday", L"Dia a dia")
                   : index == 2 ? Tr(L"Everything", L"Tudo")
                                : Tr(L"Keep as it is", L"Manter como está");
        case kChoiceSetupHide:
            return index == 0 ? Tr(L"Always shown", L"Sempre visível")
                   : index == 1
                       ? Tr(L"A line with the mouse away",
                            L"Uma linha com o mouse longe")
                       : Tr(L"A line while a window is maximized",
                            L"Uma linha com uma janela maximizada");
        case kChoiceSetupTheme:
            return index == 0   ? Tr(L"Like Windows", L"Como o Windows")
                   : index == 1 ? Tr(L"Light", L"Claro")
                                : Tr(L"Dark", L"Escuro");
        default:
            return {};
    }
}

int ChoiceValue(int id) {
    switch (id) {
        case kChoiceAnimationLevel:
            return g_animationChoice.level;
        case kChoiceFrameRate:
            return g_animationChoice.rate;
        case kChoiceBackup:
        case kChoiceColors:
            return -1;
        case kChoiceDateOrder:
            return g_settings.clockDateOrder;
        case kChoiceClockFormat:
            return g_settings.clockFormat;
        case kChoiceWeatherUnit:
            return g_settings.weatherUnit;
        case kChoiceIconPack:
            return g_settings.islandIconPack;
        case kChoiceIconFolder:
            return -1;
        case kChoiceIconColors:
            return g_settings.islandIconColors;
        case kChoiceNotchOpen:
            return g_settings.islandNotchOpen;
        case kChoiceHideWhen:
            return g_settings.islandHideWhen;
        case kChoiceLineClick:
            return g_settings.islandLineClick;
        case kChoiceLineFade:
            return g_settings.islandLineFade;
        case kChoiceBarSide:
            return g_settings.islandBarSide;
        case kChoiceBarInnerCorners:
            return g_settings.islandBarInnerCorners;
        case kChoiceBannerSize:
            return g_settings.islandBannerSize;
        case kChoiceShelfLimit:
            return g_settings.islandShelfLimit;
        case kChoiceEyeTheme:
            return g_settings.islandEyeTheme;
        case kChoiceEyeShape:
            return g_settings.islandEyeShape;
        case kChoiceBannerWidth:
            return g_settings.islandBannerWidth;
        case kChoiceBannerCorners:
            return g_settings.islandBannerCorners;
        case kChoiceBannerPlace:
            return g_settings.islandBannerPlace;
        case kChoiceTrayStyle:
            return g_settings.islandTrayStyle;
        case kChoiceWallpaper:
            return g_settings.wallpaperTransition;
        case kChoiceNotches:
            return g_settings.islandReserve ? g_settings.islandNotches
                                            : g_settings.islandPillNotches;
        case kChoiceFolderDrag:
            return (int)(std::find(std::begin(kFolderDragShown),
                                   std::end(kFolderDragShown),
                                   g_settings.islandFolderDrag) -
                         std::begin(kFolderDragShown));
        case kChoiceLanguage:
            return g_textLanguage;
        case kChoiceNotesLanguage:
            return g_notesLanguage >= 0 ? g_notesLanguage
                                        : (int)IslandTextLanguage();
        case kChoiceFillColor:
            return g_settings.islandFillColor;
        case kChoiceFillCopyColor:
            return g_settings.islandFillCopyColor;
        case kChoiceFillChargeColor:
            return g_settings.islandFillChargeColor;
        case kChoiceFillStyle:
            return g_settings.islandFillStyle;
        case kChoiceLyricsSize:
            return g_settings.islandLyricsSize;
        case kChoiceLyricsWidth:
            return g_settings.islandLyricsWidth;
        case kChoiceLyricsColor:
            return g_settings.islandLyricsColor;
        case kChoiceVisualizerColor:
            return g_settings.islandVisualizerColor;
        case kChoiceRecycleMinSize:
            return g_settings.islandRecycleMinSize;
        case kChoiceSetupStyle:
            return StyleSegmentStyle(GetIslandPref(IslandPref::Bar));
        case kChoiceSetupItems:
            return SetupItemSet();
        case kChoiceSetupHide:
            // (Hiding while a window is over it: none of these.)
            return !g_settings.islandAutoHide ? 0
                   : g_settings.islandHideWhen == 0 ||
                           ChoiceCount(kChoiceSetupHide) < 3
                       ? 1
                   : g_settings.islandHideWhen == 1 ? 2
                                                    : -1;
        case kChoiceSetupTheme:
            return g_themeIndex == (int)ThemeId::Auto    ? 0
                   : g_themeIndex == (int)ThemeId::Light ? 1
                   : g_themeIndex == (int)ThemeId::Black ? 2
                                                         : -1;
        default:
            return -1;
    }
}

// Dimmed (and not pressable): the level without animations, the date's
// order without the date or the year, the icons that aren't pinned without
// the tray, the wallpaper's transition without it, the notches without the
// Fixed style, and the fill's color without a fill.
bool ChoiceDisabled(int id) {
    switch (id) {
        case kChoiceAnimationLevel:
            return !g_animationChoice.on;
        case kChoiceFrameRate:
        case kChoiceBackup:
        case kChoiceColors:
        case kChoiceLineClick:
        case kChoiceClockFormat:
        case kChoiceLanguage:
        case kChoiceNotesLanguage:
        case kChoiceIconPack:
            return false;
        case kChoiceIconFolder:
            return g_settings.islandIconPack != kIconPackFolder;
        case kChoiceDateOrder:
            return !g_settings.clockDate && !g_settings.clockYear;
        case kChoiceWeatherUnit:
            return !g_settings.islandWeather;
        case kChoiceIconColors:
            return !IconColorsMatter();
        case kChoiceTrayStyle:
            return !g_settings.islandTray;
        case kChoiceWallpaper:
            return !g_settings.islandWallpaper;
        case kChoiceNotches:
            return g_settings.islandBar;
        case kChoiceNotchOpen:
            return NotchCountWanted() == 0;
        case kChoiceHideWhen:
            return !g_settings.islandAutoHide || g_settings.islandBar ||
                   g_settings.islandReserve || NotchCountWanted() > 0;
        case kChoiceLineFade:
            return g_settings.islandBar;
        case kChoiceBarSide:
        case kChoiceBarInnerCorners:
            return !g_settings.islandBar;
        case kChoiceBannerSize:
        case kChoiceBannerWidth:
        case kChoiceBannerCorners:
        case kChoiceBannerPlace:
            return !g_bannerEnabled;
        case kChoiceShelfLimit:
            return !g_settings.islandShelf;
        case kChoiceEyeTheme:
        case kChoiceEyeShape:
            return !g_settings.islandEyes;
        case kChoiceFolderDrag:
            return !g_settings.islandFolders;
        case kChoiceFillColor:
        case kChoiceFillCopyColor:
            return !g_settings.islandDownloadFill;
        case kChoiceFillChargeColor:
            return !g_settings.islandChargeFill;
        case kChoiceFillStyle:
            return !g_settings.islandDownloadFill &&
                   !g_settings.islandChargeFill;
        case kChoiceLyricsSize:
        case kChoiceLyricsColor:
        case kChoiceLyricsWidth:
            return !g_settings.islandLyrics;
        case kChoiceVisualizerColor:
            return !g_settings.islandVisualizer;
        case kChoiceRecycleMinSize:
            return !g_settings.islandRecycleBin;
        case kChoiceSetupStyle:
        case kChoiceSetupItems:
        case kChoiceSetupTheme:
            return false;
        case kChoiceSetupHide:
            return g_settings.islandBar;
        default:
            return true;
    }
}

// Whether `text` has every word of `query` in it, ignoring case and accents
// (so "notificacoes" finds "Notificações").
bool MatchesSearch(const std::wstring& text, const std::wstring& query) {
    size_t at = 0;
    while (at < query.size()) {
        if (query[at] == L' ') {
            at++;
            continue;
        }
        size_t end = query.find(L' ', at);
        if (end == std::wstring::npos) {
            end = query.size();
        }
        if (FindNLSStringEx(LOCALE_NAME_USER_DEFAULT,
                            FIND_FROMSTART | LINGUISTIC_IGNORECASE |
                                LINGUISTIC_IGNOREDIACRITIC,
                            text.c_str(), (int)text.size(), query.c_str() + at,
                            (int)(end - at), nullptr, nullptr, nullptr,
                            0) < 0) {
            return false;
        }
        at = end;
    }
    return true;
}

int GetIslandPref(IslandPref pref);
void SetIslandPref(IslandPref pref, int value);
void SetIslandPrefValue(IslandPref pref, int value);
// The island starts again a moment later (its windows made anew).
void RestartIslandSoon();
void SetIslandSize(int percent, bool keep);
void SetIslandGap(int pixels, bool keep);
void SetIslandOpacity(int percent, bool keep);
void SetAutoHideDelay(int tenths, bool keep);
void SetNotchCurve(int percent, bool keep);
void SetLineOpacity(int percent, bool keep);
void SetActivationWidth(int pixels, bool keep);
void SetPanelSize(int percent, bool keep);
void SetMediaTitleLimit(int pixels, bool keep);
void SetPillLook(int slider, int value, bool keep);
void SetTextLanguage(int choice);
void SetIconFolder(std::wstring folder);
void SetIconFile(WCHAR glyph, const std::wstring& path);
void SetIconColor(WCHAR glyph, int color);
void SetIconSize(int percent, bool keep);
// A custom color chosen (see CustomColor): kept, and the custom colors
// shown.
void SetCustomColor(int color, int value);
