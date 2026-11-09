#pragma once
int GetIslandPref(IslandPref pref) {
    switch (pref) {
        case IslandPref::Bar:
            return g_settings.islandBar ? 1 : g_settings.islandReserve ? 2 : 0;
        case IslandPref::Theme:
            return g_themeIndex;
        case IslandPref::Tray:
            return g_settings.islandTray;
        case IslandPref::TrayOpen:
            return g_settings.islandTrayOpen;
        case IslandPref::Network:
            return g_settings.islandNetwork;
        case IslandPref::Volume:
            return g_settings.islandVolume;
        case IslandPref::Battery:
            return g_settings.islandBattery;
        case IslandPref::Bell:
            return g_settings.islandBell;
        case IslandPref::Media:
            return g_settings.islandMedia;
        case IslandPref::MediaButtons:
            return g_mediaButtonsInPill;
        case IslandPref::MediaTitle:
            return g_mediaTitleInPill;
        case IslandPref::Banner:
            return g_bannerEnabled;
        case IslandPref::BannerMinimized:
            return g_bannerWhenMinimized;
        case IslandPref::HideWindowsBanners:
            return g_settings.islandHideWindowsBanners;
        case IslandPref::WebPictures:
            return g_settings.islandWebPictures;
        case IslandPref::ClockSeconds:
            return ClockShowsSeconds();
        case IslandPref::ClockWeekday:
            return g_settings.clockWeekday;
        case IslandPref::ClockDate:
            return g_settings.clockDate;
        case IslandPref::ClockYear:
            return g_settings.clockYear;
        case IslandPref::FreeDrag:
            return g_settings.islandFreeDrag;
        case IslandPref::AutoHide:
            return g_settings.islandAutoHide;
        case IslandPref::AllMonitors:
            return g_settings.islandAllMonitors;
        case IslandPref::VolumePercent:
            return g_settings.islandVolumePercent;
        case IslandPref::BatteryPercent:
            return g_settings.islandBatteryPercent;
        case IslandPref::Brightness:
            return g_settings.islandBrightness;
        case IslandPref::InputMethod:
            return g_settings.islandInputMethod;
        case IslandPref::Privacy:
            return g_settings.islandPrivacy;
        case IslandPref::UsbDrives:
            return g_settings.islandUsbDrives;
        case IslandPref::UsbNotify:
            return g_settings.islandUsbNotify;
        case IslandPref::Clipboard:
            return g_settings.islandClipboard;
        case IslandPref::Frosted:
            return g_settings.islandFrosted;
        case IslandPref::LiquidGlass:
            return g_settings.islandLiquidGlass;
        case IslandPref::Tips:
            return g_settings.islandTips;
        case IslandPref::HoverHighlight:
            return g_settings.islandHoverHighlight;
        case IslandPref::MinimizeButton:
            return g_settings.islandMinimizeButton;
        case IslandPref::BarCorners:
            return g_settings.islandBarCorners;
        case IslandPref::GlassTranslucent:
            return g_settings.islandGlassTranslucent;
        case IslandPref::DateOrder:
            return g_settings.clockDateOrder;
        case IslandPref::ClockFormat:
            return g_settings.clockFormat;
        case IslandPref::WeatherUnit:
            return g_settings.weatherUnit;
        case IslandPref::IconPack:
            return g_settings.islandIconPack;
        case IslandPref::IconColors:
            return g_settings.islandIconColors;
        case IslandPref::TrayStyle:
            return g_settings.islandTrayStyle;
        case IslandPref::Desktops:
            return g_settings.islandDesktops;
        case IslandPref::WindowColor:
            return g_settings.islandWindowColor;
        case IslandPref::Folders:
            return g_settings.islandFolders;
        case IslandPref::Wallpaper:
            return g_settings.islandWallpaper;
        case IslandPref::NotificationApps:
            return g_settings.islandApps;
        case IslandPref::KomorebiLayout:
            return g_settings.komorebiLayout;
        case IslandPref::FocusedApp:
            return g_settings.islandFocusedApp;
        case IslandPref::KomorebiHideEmpty:
            return g_settings.komorebiHideEmpty;
        case IslandPref::FolderDrag:
            return g_settings.islandFolderDrag;
        case IslandPref::WallpaperTransition:
            return g_settings.wallpaperTransition;
        case IslandPref::ActionNetwork:
        case IslandPref::ActionBluetooth:
        case IslandPref::ActionNightLight:
        case IslandPref::ActionHotspot:
        case IslandPref::ActionAirplane:
        case IslandPref::ActionProject:
        case IslandPref::ActionCast:
        case IslandPref::ActionCaptions:
        case IslandPref::ActionAccessibility:
        case IslandPref::ActionScreenshot:
        case IslandPref::ActionLock:
        case IslandPref::ActionEmoji:
        case IslandPref::ActionClipboard:
        case IslandPref::ActionTaskManager:
        case IslandPref::ActionCalculator:
        case IslandPref::ActionExplorer:
        case IslandPref::ActionKomorebi:
        case IslandPref::ActionShowDesktop:
        case IslandPref::ActionOnScreenKeyboard:
            return (g_settings.pillActions >> PillActionIndex(pref)) & 1;
        case IslandPref::KomorebiWhkd:
            return g_settings.komorebiWhkd;
        case IslandPref::KomorebiMasir:
            return g_settings.komorebiMasir;
        case IslandPref::ScreenshotFolder:
            return g_settings.islandScreenshotFolder;
        case IslandPref::MediaWheel:
            return g_settings.islandMediaWheel;
        case IslandPref::MediaMarquee:
            return g_settings.islandMediaMarquee;
        case IslandPref::ShortcutNotifications:
            return g_settings.islandWinN;
        case IslandPref::ShortcutControl:
            return g_settings.islandWinA;
        case IslandPref::MinimizeKey:
            return g_settings.islandMinimizeKey;
        case IslandPref::CapsLock:
            return g_settings.islandCapsLock;
        case IslandPref::HideWhen:
            return g_settings.islandHideWhen;
        case IslandPref::LineClick:
            return g_settings.islandLineClick;
        case IslandPref::LineFade:
            return g_settings.islandLineFade;
        case IslandPref::BarSide:
            return g_settings.islandBarSide;
        case IslandPref::BarInnerCorners:
            return g_settings.islandBarInnerCorners;
        case IslandPref::BannerSize:
            return g_settings.islandBannerSize;
        case IslandPref::ShelfLimit:
            return g_settings.islandShelfLimit;
        case IslandPref::BannerWidth:
            return g_settings.islandBannerWidth;
        case IslandPref::BannerCorners:
            return g_settings.islandBannerCorners;
        case IslandPref::BannerPlace:
            return g_settings.islandBannerPlace;
        case IslandPref::HideClock:
            return g_settings.islandHideClock;
        case IslandPref::NetSpeed:
            return g_settings.islandNetSpeed;
        case IslandPref::WorldClocks:
            return g_settings.islandWorldClocks;
        case IslandPref::Countdown:
            return g_settings.islandCountdown;
        case IslandPref::CountdownEvents:
            return g_settings.islandCountdownEvents;
        case IslandPref::CountdownAlarms:
            return g_settings.islandCountdownAlarms;
        case IslandPref::CountdownTimer:
            return g_settings.islandCountdownTimer;
        case IslandPref::CountdownPomodoro:
            return g_settings.islandCountdownPomodoro;
        case IslandPref::DownloadFill:
            return g_settings.islandDownloadFill;
        case IslandPref::ChargeFill:
            return g_settings.islandChargeFill;
        case IslandPref::FillColor:
            return g_settings.islandFillColor;
        case IslandPref::FillOwnColor:
            return g_settings.islandFillOwnColor;
        case IslandPref::FillCopyColor:
            return g_settings.islandFillCopyColor;
        case IslandPref::FillCopyOwnColor:
            return g_settings.islandFillCopyOwnColor;
        case IslandPref::FillChargeColor:
            return g_settings.islandFillChargeColor;
        case IslandPref::FillChargeOwnColor:
            return g_settings.islandFillChargeOwnColor;
        case IslandPref::FillStyle:
            return g_settings.islandFillStyle;
        case IslandPref::Lyrics:
            return g_settings.islandLyrics;
        case IslandPref::LyricsSize:
            return g_settings.islandLyricsSize;
        case IslandPref::LyricsColor:
            return g_settings.islandLyricsColor;
        case IslandPref::LyricsOwnColor:
            return g_settings.islandLyricsOwnColor;
        case IslandPref::LyricsInTitle:
            return g_settings.islandLyricsInTitle;
        case IslandPref::VisualizerColor:
            return g_settings.islandVisualizerColor;
        case IslandPref::VisualizerOwnColor:
            return g_settings.islandVisualizerOwnColor;
        case IslandPref::VisualizerSmooth:
            return g_settings.islandVisualizerSmooth;
        case IslandPref::WhatsNewNotice:
            return g_settings.islandWhatsNewNotice;
        case IslandPref::RecycleMinSize:
            return g_settings.islandRecycleMinSize;
        case IslandPref::LyricsWidth:
            return g_settings.islandLyricsWidth;
        case IslandPref::RecycleBin:
            return g_settings.islandRecycleBin;
        case IslandPref::StatusNotices:
            return g_settings.islandStatusNotices;
        case IslandPref::FoldApps:
            return g_settings.islandFoldApps;
        case IslandPref::Timer:
            return g_settings.islandTimer;
        case IslandPref::Visualizer:
            return g_settings.islandVisualizer;
        case IslandPref::Notches:
            return g_settings.islandNotches;
        case IslandPref::PillNotches:
            return g_settings.islandPillNotches;
        case IslandPref::BarClear:
            return g_settings.islandBarClear;
        case IslandPref::AiUsage:
            return g_settings.islandAiUsage;
        case IslandPref::AiClaude:
            return g_settings.islandAiClaude;
        case IslandPref::AiCodex:
            return g_settings.islandAiCodex;
        case IslandPref::FocusedTitle:
            return g_settings.islandFocusedTitle;
        case IslandPref::WeatherCards:
            return g_settings.islandWeatherCards;
        case IslandPref::CalendarBigDay:
            return g_settings.islandCalendarBigDay;
        case IslandPref::Commands:
            return g_settings.islandCommands;
        case IslandPref::DndSilent:
            return g_settings.islandDndSilent;
        case IslandPref::DownloadStalled:
            return g_settings.islandDownloadStalled;
        case IslandPref::FolderThumbnails:
            return g_settings.islandFolderThumbnails;
        case IslandPref::Shelf:
            return g_settings.islandShelf;
        case IslandPref::Eyes:
            return g_settings.islandEyes;
        case IslandPref::EyeTheme:
            return g_settings.islandEyeTheme;
        case IslandPref::EyeShape:
            return g_settings.islandEyeShape;
        case IslandPref::TimerLine:
            return g_settings.islandTimerLine;
        case IslandPref::ExternalBrightness:
            return g_settings.islandExternalBrightness;
        case IslandPref::GlassLight:
            return g_settings.islandGlassLight;
        case IslandPref::NotchEach:
            return g_settings.islandNotchEach;
        case IslandPref::NotchOpen:
            return g_settings.islandNotchOpen;
        case IslandPref::GroupApps:
            return g_settings.islandGroupApps;
        case IslandPref::MonthNumber:
            return g_settings.clockMonthNumber;
        case IslandPref::VerticalSides:
            return g_settings.islandVerticalSides;
        case IslandPref::Clock:
            return g_settings.islandClock;
        case IslandPref::ClockCenter:
            return g_settings.islandClockCenter;
        case IslandPref::Power:
            return g_settings.islandPower;
        case IslandPref::OwnNotifications:
            return g_settings.islandOwnsNotifications;
        case IslandPref::PanelsFollow:
            return g_settings.islandPanelsFollow;
        case IslandPref::ThemeButton:
            return g_settings.islandThemeButton;
        case IslandPref::UsageCpu:
            return g_settings.usageCpu;
        case IslandPref::UsageRam:
            return g_settings.usageRam;
        case IslandPref::UsageVram:
            return g_settings.usageVram;
        case IslandPref::Microphone:
            return g_settings.islandMicrophone;
        case IslandPref::Holidays:
            return g_settings.calendarHolidays;
        case IslandPref::Notes:
            return g_settings.islandNotes;
        case IslandPref::Tasks:
            return g_settings.islandTasks;
        case IslandPref::Shortcuts:
            return g_settings.islandShortcuts;
        case IslandPref::Weather:
            return g_settings.islandWeather;
        case IslandPref::Updates:
            return g_settings.islandUpdates;
        case IslandPref::Profiles:
            return g_settings.islandProfiles;
        case IslandPref::Animations:
            return g_animationChoice.on;
        case IslandPref::FollowMouse:
            return g_settings.islandFollowMouse;
    }
    return 0;
}

void SetIslandPlace(const std::wstring& appId, double x, double y) {
    IslandPlace* place = appId.empty() ? &g_islandPlace : FindAppPlace(appId);
    if (!place) {
        return;
    }
    place->x = std::clamp(x, 0.0, 1.0);
    place->y = std::clamp(y, 0.0, 1.0);
    SavePlaces();
    if (g_island) {
        ForEachIsland([&](Island& island) { island.OnPlacesChanged(); });
    }
}

// Changed in the island's settings (its thread): shown right away, and kept
// when `keep` (once the slider is let go).
void SetIslandGap(int pixels, bool keep) {
    pixels = std::clamp(pixels, kIslandGapMin, kIslandGapMax);
    if (keep) {
        StoreProfileInt(L"islandGap", pixels);
    }
    if (pixels == g_settings.islandGap) {
        return;
    }
    g_settings.islandGap = pixels;
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

void SetTrayListIcons(int icons, bool keep) {
    icons = std::clamp(icons, kTrayListMin, kTrayListMax);
    if (keep) {
        StoreProfileInt(L"trayListIsland", icons);
    }
    if (icons == g_trayListIcons) {
        return;
    }
    g_trayListIcons = icons;
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

void SetNotchCurve(int percent, bool keep) {
    percent = std::clamp(percent, kNotchCurveMin, kNotchCurveMax);
    if (keep) {
        StoreProfileInt(L"islandNotchCurve", percent);
    }
    if (percent == g_settings.islandNotchCurve) {
        return;
    }
    g_settings.islandNotchCurve = percent;
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

void SetIconSize(int percent, bool keep) {
    percent = std::clamp(percent, kIconSizeMin, kIconSizeMax);
    if (keep) {
        StoreProfileInt(L"islandIconSize", percent);
    }
    if (percent == g_settings.islandIconSize) {
        return;
    }
    g_settings.islandIconSize = percent;
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

void SetLineOpacity(int percent, bool keep) {
    percent = std::clamp(percent, kLineOpacityMin, kLineOpacityMax);
    if (keep) {
        StoreProfileInt(L"islandLineOpacity", percent);
    }
    if (percent == g_settings.islandLineOpacity) {
        return;
    }
    g_settings.islandLineOpacity = percent;
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

void SetActivationWidth(int pixels, bool keep) {
    pixels = std::clamp(pixels, kActivationWidthMin, kActivationWidthMax);
    if (keep) {
        StoreProfileInt(L"islandActivationWidth", pixels);
    }
    if (pixels == g_settings.islandActivationWidth) {
        return;
    }
    g_settings.islandActivationWidth = pixels;
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

void SetPanelSize(int percent, bool keep) {
    percent = std::clamp(percent, kPanelSizeMin, kPanelSizeMax);
    if (keep) {
        StoreProfileInt(L"islandPanelSize", percent);
    }
    g_settings.islandPanelSize = percent;
    // (The open panels laid out again once the slider is let go, not under
    // the mouse while it moves.)
    if (keep && g_island && !g_applyingProfile) {
        ForEachIsland([](Island& island) { island.RescalePanels(); });
    }
}

void SetMediaTitleLimit(int pixels, bool keep) {
    pixels = std::clamp(pixels, kMediaTitleLimitMin, kMediaTitleLimitMax);
    if (keep) {
        StoreProfileInt(L"islandMediaTitleLimit", pixels);
    }
    if (pixels == g_settings.islandMediaTitleLimit) {
        return;
    }
    g_settings.islandMediaTitleLimit = pixels;
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

// The pill's height, corners, the room at its ends, the edge around, the
// glass's blur, the tray grid's columns, the visualizer's sensitivity or the
// texts' size (sliders 12 to 19): kept in the profile, every island laid out
// again.
void SetPillLook(int slider, int value, bool keep) {
    const IslandSlider& info = kIslandSliders[slider];
    value = std::clamp(value, info.min, info.max);
    int& setting = slider == 12   ? g_settings.islandPillHeight
                   : slider == 13 ? g_settings.islandPillCorners
                   : slider == 14 ? g_settings.islandPillEnds
                   : slider == 15 ? g_settings.islandEdge
                   : slider == 16 ? g_settings.islandGlassBlur
                   : slider == 17 ? g_settings.islandTrayColumns
                   : slider == 18 ? g_settings.islandVizSensitivity
                                  : g_settings.islandTextSize;
    if (keep) {
        StoreProfileInt(slider == 12   ? L"islandPillHeight"
                        : slider == 13 ? L"islandPillCorners"
                        : slider == 14 ? L"islandPillEnds"
                        : slider == 15 ? L"islandEdge"
                        : slider == 16 ? L"islandGlassBlur"
                        : slider == 17 ? L"islandTrayColumns"
                        : slider == 18 ? L"islandVizSensitivity"
                                       : L"islandTextSize",
                        value);
    }
    if (value == setting) {
        return;
    }
    setting = value;
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

// The icon pack's folder chosen (empty: Windows' icons), kept in the
// profile; every island drawn again with it.
void SetIconFolder(std::wstring folder) {
    // (A drive's root keeps its backslash.)
    while (folder.size() > 3 && folder.back() == L'\\') {
        folder.pop_back();
    }
    if (folder == g_iconFolder) {
        return;
    }
    g_iconFolder = folder;
    g_iconLook++;
    StoreProfileString(L"islandIconFolder", folder.c_str());
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

// The pictures and colors chosen for single icons kept in the profile, and
// every island drawn again with them.
void StoreIconFiles(const std::vector<std::pair<WCHAR, std::wstring>>& files,
                    const std::vector<std::pair<WCHAR, int>>& colors) {
    g_iconFiles = DecodeIconFiles(EncodeIconFiles(files, colors));
    g_iconLook++;
    StoreProfileString(L"islandIconFiles", g_iconFiles.text.c_str());
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

// A picture chosen for one icon (empty: the pack's again), kept in the
// profile; every island drawn again with it.
void SetIconFile(WCHAR glyph, const std::wstring& path) {
    auto files = g_iconFiles.files;
    std::erase_if(files,
                  [&](const auto& file) { return file.first == glyph; });
    if (!path.empty()) {
        files.push_back({glyph, path});
    }
    StoreIconFiles(files, g_iconFiles.colors);
}

// A color chosen for one icon (0xRRGGBB; -1: the island's again), kept in
// the profile beside the pictures; every island drawn again with it.
void SetIconColor(WCHAR glyph, int color) {
    auto colors = g_iconFiles.colors;
    std::erase_if(colors,
                  [&](const auto& own) { return own.first == glyph; });
    if (color >= 0) {
        colors.push_back({glyph, color & 0xFFFFFF});
    }
    StoreIconFiles(g_iconFiles.files, colors);
}

// The island's texts in another language (see g_textLanguage), for every
// island: they're kept in many places, so it starts again with them.
void SetTextLanguage(int choice) {
    choice = std::clamp(choice, 0, kTextLanguages);
    if (choice == g_textLanguage) {
        return;
    }
    g_textLanguage = choice;
    Wh_SetIntValue(L"islandLanguage", choice);
    RestartIslandSoon();
}

void SetAutoHideDelay(int tenths, bool keep) {
    tenths = std::clamp(tenths, kAutoHideDelayMin, kAutoHideDelayMax);
    if (keep) {
        StoreProfileInt(L"islandAutoHideDelay", tenths);
    }
    g_settings.islandAutoHideDelay = tenths;
}

void SetCustomColor(int color, int value) {
    if (color < 0 || color >= kCustomColors) {
        return;
    }
    g_customColors[color] = value & 0xFFFFFF;
    StoreProfileInt(kCustomColorKeys[color], g_customColors[color]);
    // Chosen: the custom colors are the ones shown.
    if (g_themeIndex != (int)ThemeId::Custom) {
        SetIslandPref(IslandPref::Theme, (int)ThemeId::Custom);
        return;
    }
    ApplyThemeIndex(true);
    StartIslandAnimation();
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

void SetIslandOpacity(int percent, bool keep) {
    percent = std::clamp(percent, kIslandOpacityMin, kIslandOpacityMax);
    if (keep) {
        StoreProfileInt(L"islandOpacity", percent);
    }
    if (percent != g_islandOpacity) {
        g_islandOpacity = percent;
        if (g_island) {
            ForEachIsland(
                [&](Island& island) { island.OnPreferencesChanged(); });
        }
    }
}

void SetIslandSize(int percent, bool keep) {
    percent = std::clamp(percent, kIslandSizeMin, kIslandSizeMax);
    if (keep) {
        StoreProfileInt(L"islandSize", percent);
    }
    if (percent == g_settings.islandSize) {
        return;
    }
    g_settings.islandSize = percent;
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

