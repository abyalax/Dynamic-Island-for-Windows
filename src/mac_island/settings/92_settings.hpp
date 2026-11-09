#pragma once
void LoadSettings() {
    LoadTheme();
    LoadNotificationScreens();
    LoadAnimationSettings();
    LoadTrayListSetting();
    const auto taskbarMode = WindhawkUtils::StringSetting::make(L"taskbarMode");
    g_settings.taskbarMode =
        wcscmp(taskbarMode, L"windows") == 0
            ? decltype(g_settings.taskbarMode)::Windows
        : wcscmp(taskbarMode, L"windowsApps") == 0
            ? decltype(g_settings.taskbarMode)::WindowsApps
        : wcscmp(taskbarMode, L"windowsTransparent") == 0
            ? decltype(g_settings.taskbarMode)::WindowsTransparent
        : wcscmp(taskbarMode, L"dock") == 0
            ? decltype(g_settings.taskbarMode)::Dock
            : decltype(g_settings.taskbarMode)::Windows;
    // The dock is made for the bottom: with the taskbar elsewhere, it stays
    // as Windows has it (the island still works), until it's back at the
    // bottom (see WatchTaskbarPlace). The transparent taskbar stays
    // transparent there, without hiding itself or growing its icons (see
    // SlidesItself).
    g_taskbarAtBottom = IsTaskbarAtBottom();
    if (!g_taskbarAtBottom &&
        g_settings.taskbarMode == decltype(g_settings.taskbarMode)::Dock) {
        IslandLog(L"The taskbar isn't at the bottom: leaving it as it is");
        g_settings.taskbarMode = decltype(g_settings.taskbarMode)::Windows;
    }
    g_settings.autoHide = Wh_GetIntSetting(L"hiding.autoHide") != 0;
    g_settings.cornerRadius =
        std::clamp(Wh_GetIntSetting(L"dock.cornerRadius"), 0, 32);
    g_settings.dockIconSize =
        std::clamp(Wh_GetIntSetting(L"dock.iconSize"), 24, 64);
    g_settings.dockIconGap =
        std::clamp(Wh_GetIntSetting(L"dock.iconGap"), 0, 32);
    g_settings.bottomGap = std::clamp(Wh_GetIntSetting(L"dock.bottomGap"), 0, 24);
    g_settings.dockBackground = Wh_GetIntSetting(L"dock.background") != 0;
    g_settings.backgroundOpacity =
        std::clamp(Wh_GetIntSetting(L"dock.backgroundOpacity"), 0, 100) / 100.0;
    g_settings.showDuration =
        std::clamp(Wh_GetIntSetting(L"hiding.showDuration"), 0, 1000);
    g_settings.hideDuration =
        std::clamp(Wh_GetIntSetting(L"hiding.hideDuration"), 0, 1000);
    g_settings.showDelay = std::clamp(Wh_GetIntSetting(L"hiding.showDelay"), 0, 2000);
    g_settings.hideDelay = std::clamp(Wh_GetIntSetting(L"hiding.hideDelay"), 0, 5000);
    {
        const auto hideWhen =
            WindhawkUtils::StringSetting::make(L"hiding.hideWhen");
        g_settings.hideWhen = wcscmp(hideWhen, L"maximized") == 0    ? 1
                              : wcscmp(hideWhen, L"windowOver") == 0 ? 2
                                                                    : 0;
    }
    g_settings.revealHeight =
        std::clamp(Wh_GetIntSetting(L"hiding.revealHeight"), 0, 120);
    g_settings.fullscreenGuard = Wh_GetIntSetting(L"hiding.fullscreenGuard") != 0;
    g_settings.hideTray = Wh_GetIntSetting(L"dock.hideTray") != 0;
    g_settings.windowsHideTray = Wh_GetIntSetting(L"windowsHideTray") != 0;
    g_settings.showIsland = Wh_GetIntSetting(L"showIsland") != 0;
    // The island's own options, chosen in it (see kIslandPrefs).
    // 0 the pill, 1 the bar, 2 the pill keeping its room.
    const int style = Wh_GetIntValue(L"islandBar", 0);
    g_settings.islandBar = style == 1;
    g_settings.islandReserve = style == 2;
    g_settings.islandTray = Wh_GetIntValue(L"islandTray", 1) != 0;
    g_settings.islandTrayOpen = Wh_GetIntValue(L"trayKeepOpen", 0) != 0;
    g_settings.islandNetwork = Wh_GetIntValue(L"islandNetwork", 1) != 0;
    g_settings.islandVolume = Wh_GetIntValue(L"islandVolume", 1) != 0;
    g_settings.islandBattery = Wh_GetIntValue(L"islandBattery", 1) != 0;
    g_settings.islandBell = Wh_GetIntValue(L"islandBell", 1) != 0;
    g_settings.islandMedia = Wh_GetIntValue(L"islandMedia", 1) != 0;
    g_settings.islandMediaButtons = Wh_GetIntValue(L"mediaButtons", 0) != 0;
    g_settings.islandBanner = Wh_GetIntValue(L"bannerEnabled", 1) != 0;
    g_settings.islandBannerMinimized =
        Wh_GetIntValue(L"bannerMinimized", 0) != 0;
    g_settings.islandHideWindowsBanners =
        Wh_GetIntValue(L"hideWindowsBanners", 1) != 0;
    g_settings.islandWebPictures =
        Wh_GetIntValue(L"bannerWebPictures", 0) != 0;
    g_settings.clockSeconds = Wh_GetIntValue(L"clockSeconds", -1);
    g_settings.clockWeekday = Wh_GetIntValue(L"clockWeekday", 1) != 0;
    g_settings.clockDate = Wh_GetIntValue(L"clockDate", 1) != 0;
    g_settings.clockYear = Wh_GetIntValue(L"clockYear", 0) != 0;
    g_settings.islandFreeDrag = Wh_GetIntValue(L"islandFreeDrag", 0) != 0;
    g_settings.islandAutoHide = Wh_GetIntValue(L"islandAutoHide", 0) != 0;
    g_settings.islandAllMonitors =
        Wh_GetIntValue(L"islandAllMonitors", 0) != 0;
    g_settings.islandVolumePercent =
        Wh_GetIntValue(L"islandVolumePercent", 0) != 0;
    g_settings.islandBatteryPercent =
        Wh_GetIntValue(L"islandBatteryPercent", 0) != 0;
    g_settings.islandBrightness = Wh_GetIntValue(L"islandBrightness", 0) != 0;
    g_settings.islandClipboard = Wh_GetIntValue(L"islandClipboard", 0) != 0;
    g_settings.islandInputMethod =
        Wh_GetIntValue(L"islandInputMethod", 0) != 0;
    g_settings.islandPrivacy = Wh_GetIntValue(L"islandPrivacy", 0) != 0;
    g_settings.islandUsbDrives = Wh_GetIntValue(L"islandUsbDrives", 1) != 0;
    g_settings.islandUsbNotify = Wh_GetIntValue(L"islandUsbNotify", 1) != 0;
    g_settings.islandFrosted = Wh_GetIntValue(L"islandFrosted", 0) != 0;
    g_settings.islandBarClear = Wh_GetIntValue(L"islandBarClear", 0) != 0;
    g_settings.islandAiUsage = Wh_GetIntValue(L"islandAiUsage", 0) != 0;
    g_settings.islandAiClaude = Wh_GetIntValue(L"islandAiClaude", 1) != 0;
    g_settings.islandAiCodex = Wh_GetIntValue(L"islandAiCodex", 1) != 0;
    g_settings.islandFocusedTitle =
        Wh_GetIntValue(L"islandFocusedTitle", 0) != 0;
    g_settings.islandWeatherCards =
        Wh_GetIntValue(L"islandWeatherCards", 0) != 0;
    g_settings.islandCalendarBigDay =
        Wh_GetIntValue(L"islandCalendarBigDay", 0) != 0;
    g_settings.islandCommands = Wh_GetIntValue(L"islandCommands", 0) != 0;
    g_settings.islandDndSilent = Wh_GetIntValue(L"islandDndSilent", 0) != 0;
    g_settings.islandDownloadStalled =
        Wh_GetIntValue(L"islandDownloadStalled", 1) != 0;
    g_settings.islandFolderThumbnails =
        Wh_GetIntValue(L"islandFolderThumbnails", 1) != 0;
    g_settings.islandShelf = Wh_GetIntValue(L"islandShelf", 0) != 0;
    g_settings.islandEyes = Wh_GetIntValue(L"islandEyes", 0) != 0;
    g_settings.islandEyeTheme =
        std::clamp(Wh_GetIntValue(L"islandEyeTheme", 0), 0,
                   (int)std::size(kEyeThemes) - 1);
    g_settings.islandEyeShape =
        std::clamp(Wh_GetIntValue(L"islandEyeShape", 0), 0,
                   (int)std::size(kEyeShapes) - 1);
    g_settings.islandTimerLine = Wh_GetIntValue(L"islandTimerLine", 1) != 0;
    g_settings.islandExternalBrightness =
        Wh_GetIntValue(L"islandExternalBrightness", 1) != 0;
    g_settings.islandGlassLight = Wh_GetIntValue(L"islandGlassLight", 0) != 0;
    g_settings.islandLiquidGlass =
        Wh_GetIntValue(L"islandLiquidGlass", 0) != 0;
    // (Read for the threads too; the control centers' lists read before are
    // old if it changed.)
    if (g_externalBrightness.exchange(g_settings.islandExternalBrightness) !=
        g_settings.islandExternalBrightness) {
        g_displayChanges++;
    }
    g_settings.islandTips = Wh_GetIntValue(L"islandTips", 1) != 0;
    g_settings.islandHoverHighlight =
        Wh_GetIntValue(L"islandHoverHighlight", 1) != 0;
    g_settings.islandMinimizeButton =
        Wh_GetIntValue(L"islandMinimizeButton", 1) != 0;
    g_settings.islandBarCorners = Wh_GetIntValue(L"islandBarCorners", 0) != 0;
    g_settings.islandGlassTranslucent =
        Wh_GetIntValue(L"islandGlassTranslucent", 0) != 0;
    g_textLanguage =
        std::clamp(Wh_GetIntValue(L"islandLanguage", 0), 0, kTextLanguages);
    g_settings.clockDateOrder =
        std::clamp(Wh_GetIntValue(L"islandDateOrder", 0), 0, 3);
    g_settings.clockFormat =
        std::clamp(Wh_GetIntValue(L"islandClockFormat", 0), 0, 2);
    g_settings.weatherUnit =
        std::clamp(Wh_GetIntValue(L"islandWeatherUnit", 0), 0, 2);
    g_settings.islandIconPack = std::clamp(
        Wh_GetIntValue(L"islandIconPack", kIconPackWindows), 0, kIconPacks - 1);
    g_settings.islandIconColors =
        std::clamp(Wh_GetIntValue(L"islandIconColors", 0), 0, 1);
    // (Only the island draws with them: not read in explorer, where a value
    // that isn't there costs five reads each.)
    if (!g_isExplorer) {
        g_iconFolder = GetWholeStringValue(L"islandIconFolder", 1024);
        g_iconFiles = DecodeIconFiles(
            GetWholeStringValue(L"islandIconFiles", kIconFilesRoom));
    }
    g_settings.islandTrayStyle =
        std::clamp(Wh_GetIntValue(L"islandTrayStyle", 0), 0, 2);
    g_settings.islandDesktops = Wh_GetIntValue(L"islandDesktops", 0) != 0;
    g_settings.islandWindowColor =
        Wh_GetIntValue(L"islandWindowColor", 0) != 0;
    g_settings.islandFolders = Wh_GetIntValue(L"islandFolders", 0) != 0;
    g_settings.islandWinN = Wh_GetIntValue(L"islandWinN", 0) != 0;
    g_settings.islandWinA = Wh_GetIntValue(L"islandWinA", 0) != 0;
    g_settings.islandMinimizeKey =
        Wh_GetIntValue(L"islandMinimizeKey", 0) != 0;
    g_settings.islandCapsLock = Wh_GetIntValue(L"islandCapsLock", 0) != 0;
    g_settings.islandHideWhen =
        std::clamp(Wh_GetIntValue(L"islandHideWhen", 0), 0, 2);
    g_settings.islandHideClock = Wh_GetIntValue(L"islandHideClock", 0) != 0;
    g_settings.islandLineClick =
        std::clamp(Wh_GetIntValue(L"islandLineClick", 0), 0, 1);
    g_settings.islandLineFade =
        std::clamp(Wh_GetIntValue(L"islandLineFade", 0), 0,
                   (int)std::size(kLineFadeSeconds) - 1);
    g_settings.islandBarSide =
        std::clamp(Wh_GetIntValue(L"islandBarSide", 0), 0, 2);
    g_settings.islandBarInnerCorners =
        std::clamp(Wh_GetIntValue(L"islandBarInnerCorners", 0), 0,
                   (int)std::size(kBarInnerCornerRadii) - 1);
    g_settings.islandBannerSize =
        std::clamp(Wh_GetIntValue(L"islandBannerSize", kBannerSizeNormal), 0,
                   (int)std::size(kBannerSizes) - 1);
    g_settings.islandShelfLimit =
        std::clamp(Wh_GetIntValue(L"islandShelfLimit", kShelfLimitNormal), 0,
                   (int)std::size(kShelfLimits) - 1);
    g_settings.islandBannerWidth =
        std::clamp(Wh_GetIntValue(L"islandBannerWidth", kBannerWidthNormal), 0,
                   (int)std::size(kBannerWidths) - 1);
    g_settings.islandBannerCorners =
        std::clamp(Wh_GetIntValue(L"islandBannerCorners", 0), 0,
                   (int)std::size(kBannerCornerShares) - 1);
    g_settings.islandBannerPlace = std::clamp(
        Wh_GetIntValue(L"islandBannerPlace", 0), 0, kBannerPlaces - 1);
    // (Keys without Win, Ctrl or Alt are never recorded: Win+J then.)
    const int minimizeShortcut =
        Wh_GetIntValue(L"islandMinimizeShortcut", kDefaultMinimizeShortcut);
    g_minimizeShortcut =
        (minimizeShortcut & 0xFF) && (minimizeShortcut >> 8 & 7) &&
                !(minimizeShortcut >> 12)
            ? minimizeShortcut
            : kDefaultMinimizeShortcut;
    g_settings.islandNetSpeed = Wh_GetIntValue(L"islandNetSpeed", 0) != 0;
    g_settings.islandWorldClocks =
        Wh_GetIntValue(L"islandWorldClocks", 0) != 0;
    g_settings.islandCountdown = Wh_GetIntValue(L"islandCountdown", 0) != 0;
    g_settings.islandCountdownEvents = Wh_GetIntValue(L"islandCountdownEvents", 1) != 0;
    g_settings.islandCountdownAlarms = Wh_GetIntValue(L"islandCountdownAlarms", 1) != 0;
    g_settings.islandCountdownTimer = Wh_GetIntValue(L"islandCountdownTimer", 1) != 0;
    g_settings.islandCountdownPomodoro = Wh_GetIntValue(L"islandCountdownPomodoro", 1) != 0;
    g_settings.islandDownloadFill = Wh_GetIntValue(L"islandDownloadFill", 0) != 0;
    g_settings.islandChargeFill = Wh_GetIntValue(L"islandChargeFill", 0) != 0;
    g_settings.islandFillColor =
        std::clamp(Wh_GetIntValue(L"islandFillColor", 0), 0, 3);
    g_settings.islandFillOwnColor =
        Wh_GetIntValue(L"islandFillOwnColor", 0x30D158) & 0xFFFFFF;
    g_settings.islandFillCopyColor =
        std::clamp(Wh_GetIntValue(L"islandFillCopyColor", 1), 0, 3);
    g_settings.islandFillCopyOwnColor =
        Wh_GetIntValue(L"islandFillCopyOwnColor", 0x0A84FF) & 0xFFFFFF;
    g_settings.islandFillChargeColor =
        std::clamp(Wh_GetIntValue(L"islandFillChargeColor", 0), 0, 3);
    g_settings.islandFillChargeOwnColor =
        Wh_GetIntValue(L"islandFillChargeOwnColor", 0x30D158) & 0xFFFFFF;
    g_settings.islandFillStyle =
        std::clamp(Wh_GetIntValue(L"islandFillStyle", 1), 0, 2);
    g_settings.islandLyrics = Wh_GetIntValue(L"islandLyrics", 0) != 0;
    g_settings.islandLyricsSize =
        std::clamp(Wh_GetIntValue(L"islandLyricsSize", 1), 0, 2);
    g_settings.islandLyricsColor =
        std::clamp(Wh_GetIntValue(L"islandLyricsColor", 0), 0, 2);
    g_settings.islandLyricsOwnColor =
        Wh_GetIntValue(L"islandLyricsOwnColor", 0x64D2FF) & 0xFFFFFF;
    g_settings.islandLyricsInTitle =
        Wh_GetIntValue(L"islandLyricsInTitle", 0) != 0;
    g_settings.islandVisualizerColor =
        std::clamp(Wh_GetIntValue(L"islandVisualizerColor", 0), 0, 4);
    g_settings.islandVisualizerOwnColor =
        Wh_GetIntValue(L"islandVisualizerOwnColor", 0x64D2FF) & 0xFFFFFF;
    g_settings.islandVisualizerSmooth =
        Wh_GetIntValue(L"islandVisualizerSmooth", 0) != 0;
    g_settings.islandWhatsNewNotice =
        Wh_GetIntValue(L"islandWhatsNewNotice", 1) != 0;
    g_settings.islandRecycleMinSize = std::clamp(
        Wh_GetIntValue(L"islandRecycleMinSize", 0), 0,
        (int)std::size(kRecycleMinMegabytes) - 1);
    g_settings.islandLyricsWidth =
        std::clamp(Wh_GetIntValue(L"islandLyricsWidth", kLyricsWidthFollows), 0,
                   (int)std::size(kLyricsMaxWidths) - 1);
    g_settings.islandRecycleBin = Wh_GetIntValue(L"islandRecycleBin", 0) != 0;
    g_settings.islandStatusNotices =
        Wh_GetIntValue(L"islandStatusNotices", 0) != 0;
    g_settings.islandFoldApps = Wh_GetIntValue(L"islandFoldApps", 0) != 0;
    g_settings.islandTimer = Wh_GetIntValue(L"islandTimer", 0) != 0;
    g_settings.islandVisualizer =
        Wh_GetIntValue(L"islandVisualizer", 0) != 0;
    g_settings.islandWallpaper = Wh_GetIntValue(L"islandWallpaper", 0) != 0;
    g_settings.wallpaperTransition =
        std::clamp(Wh_GetIntValue(L"islandWallpaperTransition", 1), 0,
                   (int)WallpaperTransition::Count - 1);
    g_settings.komorebiWhkd = Wh_GetIntValue(L"komorebiWhkd", 0) != 0;
    g_settings.komorebiMasir = Wh_GetIntValue(L"komorebiMasir", 0) != 0;
    g_settings.pillActions = 0;
    for (const auto& action : kPillActions) {
        for (const auto& info : kIslandPrefs) {
            if (info.pref == action.pref && Wh_GetIntValue(info.key, 0)) {
                SetPillActionBit(action.pref, true);
            }
        }
    }
    g_settings.clockMonthNumber =
        Wh_GetIntValue(L"islandMonthNumber", 0) != 0;
    g_settings.islandAutoHideDelay =
        std::clamp(Wh_GetIntValue(L"islandAutoHideDelay",
                                  kAutoHideDelayDefault),
                   kAutoHideDelayMin, kAutoHideDelayMax);
    g_settings.islandNotches =
        std::clamp(Wh_GetIntValue(L"islandNotches", 0), 0, kMaxNotches);
    g_settings.islandPillNotches =
        std::clamp(Wh_GetIntValue(L"islandPillNotches", 0), 0, kMaxNotches);
    g_settings.islandNotchCurve =
        std::clamp(Wh_GetIntValue(L"islandNotchCurve", kNotchCurveDefault),
                   kNotchCurveMin, kNotchCurveMax);
    g_settings.islandIconSize =
        std::clamp(Wh_GetIntValue(L"islandIconSize", kIconSizeDefault),
                   kIconSizeMin, kIconSizeMax);
    // (Before it came, the texts grew with the icons.)
    g_settings.islandTextSize = std::clamp(
        Wh_GetIntValue(L"islandTextSize", g_settings.islandIconSize),
        kTextSizeMin, kTextSizeMax);
    g_settings.islandLineOpacity =
        std::clamp(Wh_GetIntValue(L"islandLineOpacity", kLineOpacityDefault),
                   kLineOpacityMin, kLineOpacityMax);
    g_settings.islandActivationWidth = std::clamp(
        Wh_GetIntValue(L"islandActivationWidth", kActivationWidthDefault),
        kActivationWidthMin, kActivationWidthMax);
    g_settings.islandPanelSize =
        std::clamp(Wh_GetIntValue(L"islandPanelSize", kPanelSizeDefault),
                   kPanelSizeMin, kPanelSizeMax);
    g_settings.islandMediaTitleLimit = std::clamp(
        Wh_GetIntValue(L"islandMediaTitleLimit", kMediaTitleLimitDefault),
        kMediaTitleLimitMin, kMediaTitleLimitMax);
    g_settings.islandPillHeight =
        std::clamp(Wh_GetIntValue(L"islandPillHeight", kPillHeightPctDefault),
                   kPillHeightPctMin, kPillHeightPctMax);
    g_settings.islandPillCorners =
        std::clamp(Wh_GetIntValue(L"islandPillCorners", kPillCornersDefault),
                   kPillCornersMin, kPillCornersMax);
    g_settings.islandPillEnds =
        std::clamp(Wh_GetIntValue(L"islandPillEnds", kPillEndsDefault),
                   kPillEndsMin, kPillEndsMax);
    g_settings.islandEdge = std::clamp(
        Wh_GetIntValue(L"islandEdge", kEdgeDefault), kEdgeMin, kEdgeMax);
    g_settings.islandGlassBlur =
        std::clamp(Wh_GetIntValue(L"islandGlassBlur", kGlassBlurDefault),
                   kGlassBlurMin, kGlassBlurMax);
    g_settings.islandTrayColumns =
        std::clamp(Wh_GetIntValue(L"islandTrayColumns", kTrayColumnsDefault),
                   kTrayColumnsMin, kTrayColumnsMax);
    g_settings.islandVizSensitivity = std::clamp(
        Wh_GetIntValue(L"islandVizSensitivity", kVizSensitivityDefault),
        kVizSensitivityMin, kVizSensitivityMax);
    g_settings.islandNotchEach = Wh_GetIntValue(L"islandNotchEach", 0) != 0;
    g_settings.islandNotchOpen =
        std::clamp(Wh_GetIntValue(L"islandNotchOpen", 0), 0, 3);
    g_settings.islandGroupApps = Wh_GetIntValue(L"islandGroupApps", 1) != 0;
    g_settings.islandApps = Wh_GetIntValue(L"islandApps", 1) != 0;
    g_settings.komorebiLayout =
        Wh_GetIntValue(L"islandKomorebiLayout", 0) != 0;
    g_settings.islandFocusedApp = Wh_GetIntValue(L"islandFocusedApp", 0) != 0;
    g_settings.komorebiHideEmpty =
        Wh_GetIntValue(L"islandKomorebiHideEmpty", 1) != 0;
    g_settings.islandFolderDrag = std::clamp(
        Wh_GetIntValue(L"islandFolderDrag", 0), 0, kFolderDragAskAlways);
    g_settings.islandScreenshotFolder =
        Wh_GetIntValue(L"islandScreenshotFolder", 0) != 0;
    g_settings.islandMediaWheel = Wh_GetIntValue(L"islandMediaWheel", 0) != 0;
    g_settings.islandMediaMarquee =
        Wh_GetIntValue(L"islandMediaMarquee", 0) != 0;
    LoadNotchSides();
    g_settings.islandVerticalSides =
        Wh_GetIntValue(L"islandVerticalSides", 1) != 0;
    g_settings.islandClock = Wh_GetIntValue(L"islandClock", 1) != 0;
    g_settings.islandClockCenter = Wh_GetIntValue(L"islandClockCenter", 0) != 0;
    g_settings.islandPower = Wh_GetIntValue(L"islandPower", 0) != 0;
    g_settings.islandPanelsFollow =
        Wh_GetIntValue(L"islandPanelsFollow", 0) != 0;
    g_settings.islandThemeButton =
        Wh_GetIntValue(L"islandThemeButton", 0) != 0;
    g_settings.usageCpu = Wh_GetIntValue(L"usageCpu", 0) != 0;
    g_settings.usageRam = Wh_GetIntValue(L"usageRam", 0) != 0;
    g_settings.usageVram = Wh_GetIntValue(L"usageVram", 0) != 0;
    g_settings.islandMicrophone = Wh_GetIntValue(L"islandMicrophone", 0) != 0;
    g_settings.calendarHolidays = Wh_GetIntValue(L"calendarHolidays", 0) != 0;
    g_hidePastAfter =
        std::clamp(Wh_GetIntValue(L"calendarHidePastAfter", 0), 0, 3);
    g_calendarIcs.clear();
    for (int i = 0; i < (int)kMaxAgendas; i++) {
        const auto value =
            WindhawkUtils::StringSetting::make(L"calendarIcs[%d]", i);
        const std::wstring source = AgendaSource(value.get());
        if (!source.empty() &&
            std::find(g_calendarIcs.begin(), g_calendarIcs.end(), source) ==
                g_calendarIcs.end()) {
            g_calendarIcs.push_back(source);
        }
    }
    g_settings.islandNotes = Wh_GetIntValue(L"islandNotes", 0) != 0;
    g_settings.islandTasks = Wh_GetIntValue(L"islandTasks", 0) != 0;
    g_settings.islandShortcuts = Wh_GetIntValue(L"islandShortcuts", 0) != 0;
    g_settings.islandWeather = Wh_GetIntValue(L"islandWeather", 0) != 0;
    g_settings.islandUpdates = Wh_GetIntValue(L"islandUpdates", 0) != 0;
    g_updatesEvery =
        std::clamp(Wh_GetIntValue(L"updatesCheckEvery", 0), 0,
                   (int)std::size(kUpdatesEveryHours) - 1);
    g_settings.islandProfiles = Wh_GetIntValue(L"islandProfiles", 0) != 0;
    LoadProfiles();
    LoadIgnoredUpdates();
    g_settings.islandOwnsNotifications =
        Wh_GetIntValue(L"islandOwnsNotifications", 0) != 0;
    g_islandQuiet = Wh_GetIntValue(L"islandQuiet", 0) != 0;
    LoadPillOrder();
    LoadTrayArrangement();
    // (After the options: the places hide the groups that are off.)
    LoadPlaces();
    g_settings.islandSize = std::clamp(
        Wh_GetIntValue(L"islandSize", kIslandSizeDefault), kIslandSizeMin,
        kIslandSizeMax);
    g_islandOpacity = std::clamp(
        Wh_GetIntValue(L"islandOpacity", kIslandOpacityDefault),
        kIslandOpacityMin, kIslandOpacityMax);
    g_settings.islandGap =
        std::clamp(Wh_GetIntValue(L"islandGap", kIslandGapDefault),
                   kIslandGapMin, kIslandGapMax);
    g_settings.islandFollowMouse =
        Wh_GetIntValue(L"islandFollowMouse", 0) != 0;
    {
        g_islandMonitor = GetWholeStringValue(L"islandMonitor", 64);
    }
    g_settings.magnification =
        std::clamp(Wh_GetIntSetting(L"icons.magnification"), 100, 200) / 100.0;
    g_settings.magnificationRange =
        std::clamp(Wh_GetIntSetting(L"icons.magnificationRange"), 40, 300);
    g_settings.onlyAppsEffects = Wh_GetIntSetting(L"icons.onlyApps") != 0;
    {
        const int widest = Wh_GetIntSetting(L"dock.maxWidth");
        g_settings.dockMaxWidth = widest <= 0 ? 0 : std::clamp(widest, 30, 100);
    }
    g_settings.bounceOnLaunch = Wh_GetIntSetting(L"icons.bounceOnLaunch") != 0;
    g_settings.keepOpenOnLaunch = Wh_GetIntSetting(L"icons.keepOpenOnLaunch") != 0;
    g_settings.keepOpenDuration =
        std::clamp(Wh_GetIntSetting(L"icons.keepOpenDuration"), 0, 5000);
    g_settings.dotIndicator = Wh_GetIntSetting(L"dock.dotIndicator") != 0;
    // Options that only work with another one: as if off without it.
    if (!g_settings.autoHide) {
        g_settings.keepOpenOnLaunch = false;
    }
    const auto attentionMode =
        WindhawkUtils::StringSetting::make(L"dock.attentionMode");
    g_settings.attentionMode = wcscmp(attentionMode, L"dock") == 0
                                   ? AttentionMode::Dock
                               : wcscmp(attentionMode, L"none") == 0
                                   ? AttentionMode::None
                                   : AttentionMode::Icon;
}

////////////////////////////////////////////////////////////////////////////////
// The explorer.exe part: the dock, and what the taskbar knows for the island.

BOOL ExplorerModInit() {
    IslandLog(L">");

    LoadSettings();
    // Turned on by the mod before Explorer last stopped (maybe a crash).
    g_turnedAutoHideOn = GetUserIntValue(L"turnedAutoHideOn", 0) != 0;

    if (!HookTaskbarDllSymbols()) {
        return FALSE;
    }

    WindhawkUtils::SetFunctionHook(SetTimer, SetTimer_Hook,
                                   &SetTimer_Original);
    return TRUE;
}

void ExplorerModAfterInit() {
    IslandLog(L">");

    g_shellHookMessage = RegisterWindowMessage(L"SHELLHOOK");
    StartAttentionThread();
    ApplySettings();
    SubclassTaskbars();
    UpdateAutoHide();
    RefreshDockRoom();
    StartEdgeWatch();
    RequestTrayIcons();
    ReadOverlayIcons();
    StartLateStart();
}

void ExplorerModBeforeUninit() {
    IslandLog(L">");

    g_unloading = true;
    StopWatchingTaskbarPlace();
    StopLateStart();
    StopEdgeWatch();
    StopAttentionThread();
    // Restore Windows before XAML cleanup, even if a retired element fails.
    UpdateAutoHide();
    StopBigIcons();
    ApplySettings();
    // (Stopped by the settings' apply on the taskbars' thread, which also
    // starts their hide timers again; here too in case it didn't run.)
    StopTaskbarWayThread();
    UnsubclassTaskbars();
    // (One may have started in the subclass meanwhile.)
    StopWatchingTaskbarPlace();
    KeepTrayIcons();
    // (While it still knows the island's process, see there.)
    RestoreDndWithoutIsland();
    StopWatchingIslandProcess();
    FreeIslandAppIcons();
    if (GetUserIntValue(L"explorerPartPid", 0) == (int)GetCurrentProcessId()) {
        SetUserIntValue(L"explorerPartPid", 0);
    }
}

// The taskbar moved to another edge: once it has settled, the settings
// apply again (the dock goes away, or comes back at the bottom). On a thread
// of its own, since applying them waits for the taskbar's thread.
HANDLE g_taskbarPlaceThread;
HANDLE g_taskbarPlaceStop;
// (Started on the taskbar's thread, stopped by the unloading one. The lock
// only guards the handles: the thread is waited for outside it, since it may
// be sending to the taskbar's thread, which may be waiting for the lock.)
SRWLOCK g_taskbarPlaceLock = SRWLOCK_INIT;

void ExplorerModSettingsChanged();

// (Its stop event is its parameter.)
DWORD WINAPI TaskbarPlaceThreadProc(LPVOID parameter) {
    // (Soon: the dock's look on a taskbar at another edge makes Windows lay
    // it out wrong, see GrowTaskbarWindow.)
    if (WaitForSingleObject((HANDLE)parameter, 150) == WAIT_TIMEOUT &&
        !g_unloading && IsTaskbarAtBottom() != g_taskbarAtBottom) {
        ExplorerModSettingsChanged();
    }
    return 0;
}

void StopWatchingTaskbarPlace() {
    AcquireSRWLockExclusive(&g_taskbarPlaceLock);
    const HANDLE thread = std::exchange(g_taskbarPlaceThread, nullptr);
    const HANDLE stop = std::exchange(g_taskbarPlaceStop, nullptr);
    ReleaseSRWLockExclusive(&g_taskbarPlaceLock);
    if (thread) {
        SetEvent(stop);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
    if (stop) {
        CloseHandle(stop);
    }
}

// On the taskbar's thread (not once unloading has begun: g_unloading is set
// before the first StopWatchingTaskbarPlace, so none starts after it).
void WatchTaskbarPlace() {
    AcquireSRWLockExclusive(&g_taskbarPlaceLock);
    // (One that has ended is only closed.)
    if (g_taskbarPlaceThread &&
        WaitForSingleObject(g_taskbarPlaceThread, 0) == WAIT_OBJECT_0) {
        CloseHandle(std::exchange(g_taskbarPlaceThread, nullptr));
        CloseHandle(std::exchange(g_taskbarPlaceStop, nullptr));
    }
    if (!g_unloading && !g_taskbarPlaceThread) {
        g_taskbarPlaceStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (g_taskbarPlaceStop) {
            g_taskbarPlaceThread =
                CreateThread(nullptr, 0, TaskbarPlaceThreadProc,
                             g_taskbarPlaceStop, 0, nullptr);
            if (!g_taskbarPlaceThread) {
                CloseHandle(std::exchange(g_taskbarPlaceStop, nullptr));
            }
        }
    }
    ReleaseSRWLockExclusive(&g_taskbarPlaceLock);
}

// Windhawk's settings change and the taskbar moving to another edge (see
// TaskbarPlaceThreadProc) apply them one at a time.
SRWLOCK g_explorerSettingsLock = SRWLOCK_INIT;

void ExplorerModSettingsChanged() {
    IslandLog(L">");

    AcquireSRWLockExclusive(&g_explorerSettingsLock);

    StopLateStart();
    LoadSettings();
    ApplySettings();
    SubclassTaskbars();
    UpdateAutoHide();
    // The bottom edge is only watched for the dock.
    StopEdgeWatch();
    StartEdgeWatch();
    RequestTrayIcons();
    StartLateStart();
    ReleaseSRWLockExclusive(&g_explorerSettingsLock);
}

////////////////////////////////////////////////////////////////////////////////
// The island's process (windhawk.exe): the island, and the new notifications
// (for the island, and for the dock's icons to bounce).

// Keeps the multithreaded apartment alive while the island's process runs.
// Its threads (media, radios, notifications) come and go, and when the last
// one leaves it, COM unloads Windows' DLLs while C++/WinRT still has their
// factories cached: the next call (opening the control center after the
// island restarted) would run code that's gone.
// Looked up at run time (not in every import library).
CO_MTA_USAGE_COOKIE g_mtaUsage;

FARPROC GetComBaseProc(PCSTR name) {
    HMODULE combase = GetModuleHandle(L"combase.dll");
    return combase ? GetProcAddress(combase, name) : nullptr;
}

BOOL WhTool_ModInit() {
    g_keepLog = true;
    IslandLog(L">");

    using CoIncrementMTAUsage_t = HRESULT(WINAPI*)(CO_MTA_USAGE_COOKIE*);
    if (auto increment = (CoIncrementMTAUsage_t)GetComBaseProc(
            "CoIncrementMTAUsage");
        !increment || FAILED(increment(&g_mtaUsage))) {
        g_mtaUsage = nullptr;
    }
    g_firstInstall = FirstInstall();
    LoadSettings();
    g_turnedDndOn = GetUserIntValue(L"turnedDndOn", 0) != 0;
    RestoreLeftoverDnd();
    // (Levels left at 0 by a process that ended without its uninit.)
    RestoreMicrophonesNow();
    StartToastWatch();
    StartIslandThread();
    return TRUE;
}

// The tool mod's process mutex (see Wh_ModInit).
extern HANDLE g_toolModProcessMutex;

void WhTool_ModUninit() {
    IslandLog(L">");

    // (No restart starts once this is set.)
    g_unloading = true;
    // Released first: a new version's process starts while this one still
    // tears the island down (it takes a moment), and would leave on finding
    // the mutex held.
    if (g_toolModProcessMutex) {
        CloseHandle(g_toolModProcessMutex);
        g_toolModProcessMutex = nullptr;
    }
    WaitForIslandRestart();
    AcquireSRWLockExclusive(&g_islandLifeLock);
    StopIslandThread();
    ReleaseSRWLockExclusive(&g_islandLifeLock);
    RestoreMicrophonesNow();
    StopToastWatch();
    // Auto-hide turned on by the explorer.exe part is turned back off by
    // that part when it unloads; from here too, but only when no explorer.exe
    // part looks after it now (Explorer was restarted without the mod). Not
    // when one does (a new version, or the mod turned off and on quickly:
    // the new part may have turned it on already).
    DWORD trayProcessId = 0;
    if (HWND tray = FindWindow(L"Shell_TrayWnd", nullptr)) {
        GetWindowThreadProcessId(tray, &trayProcessId);
    }
    if (GetUserIntValue(L"turnedAutoHideOn", 0) && trayProcessId &&
        (DWORD)GetUserIntValue(L"explorerPartPid", 0) != trayProcessId) {
        if (SetAutoHide(false)) {
            SetUserIntValue(L"turnedAutoHideOn", 0);
        } else {
            IslandLog(L"Taskbar auto-hide restoration is still pending");
        }
    }
    winrt::clear_factory_cache();
    using CoDecrementMTAUsage_t = HRESULT(WINAPI*)(CO_MTA_USAGE_COOKIE);
    if (auto decrement = (CoDecrementMTAUsage_t)GetComBaseProc(
            "CoDecrementMTAUsage");
        g_mtaUsage && decrement) {
        decrement(g_mtaUsage);
    }
    g_mtaUsage = nullptr;
}

void WhTool_ModSettingsChanged() {
    IslandLog(L">");

    WaitForIslandRestart();
    AcquireSRWLockExclusive(&g_islandLifeLock);
    // The island still on: it reads them again on its own thread and goes on
    // as it is (a panel open, a reply being typed, an animation running).
    // Turned on or off, it starts again (or stops).
    const HWND island = g_islandWnd;
    if (g_islandThread && island &&
        (Wh_GetIntSetting(L"showIsland") != 0) == g_settings.showIsland) {
        PostMessage(island, WM_APP_RELOAD_SETTINGS, 0, 0);
    } else {
        StopIslandThread();
        LoadSettings();
        // (The island turned off: the microphones it muted come back.)
        if (!g_settings.showIsland) {
            RestoreMicrophonesNow();
        }
        StartIslandThread();
    }
    ReleaseSRWLockExclusive(&g_islandLifeLock);
}

bool IsExplorerProcess() {
    WCHAR path[MAX_PATH];
    const DWORD length = GetModuleFileName(nullptr, path, ARRAYSIZE(path));
    if (!length || length == ARRAYSIZE(path)) {
        return false;
    }
    PCWSTR name = wcsrchr(path, L'\\');
    return _wcsicmp(name ? name + 1 : path, L"explorer.exe") == 0;
}

////////////////////////////////////////////////////////////////////////////////
// Windhawk tool mod implementation for mods which don't need to inject to other
// processes or hook other functions. Context:
// https://github.com/ramensoftware/windhawk/wiki/Mods-as-tools:-Running-mods-in-a-dedicated-process
//
// The mod will load and run in a dedicated windhawk.exe process.
//
// Paste the code below as part of the mod code, and use these callbacks:
// * WhTool_ModInit
// * WhTool_ModSettingsChanged
// * WhTool_ModUninit
//
// Currently, other callbacks are not supported.
//
// This mod also targets explorer.exe: there, each callback calls the explorer
// part instead (the lines marked "explorer.exe part").

bool g_isToolModProcessLauncher;
HANDLE g_toolModProcessMutex;

void WINAPI EntryPoint_Hook() {
    Wh_Log(L">");
    ExitThread(0);
}

BOOL Wh_ModInit() {
    // The mod's own module, for its window classes (both processes).
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      (LPCWSTR)&Wh_ModInit, &g_module);

    // explorer.exe part.
    if (IsExplorerProcess()) {
        g_isExplorer = true;
        return ExplorerModInit();
    }

    DWORD sessionId;
    if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId) &&
        sessionId == 0) {
        return FALSE;
    }

    bool isExcluded = false;
    bool isToolModProcess = false;
    bool isCurrentToolModProcess = false;
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (!argv) {
        Wh_Log(L"CommandLineToArgvW failed");
        return FALSE;
    }

    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"-service") == 0 ||
            wcscmp(argv[i], L"-service-start") == 0 ||
            wcscmp(argv[i], L"-service-stop") == 0) {
            isExcluded = true;
            break;
        }
    }

    for (int i = 1; i < argc - 1; i++) {
        if (wcscmp(argv[i], L"-tool-mod") == 0) {
            isToolModProcess = true;
            if (wcscmp(argv[i + 1], WH_MOD_ID) == 0) {
                isCurrentToolModProcess = true;
            }
            break;
        }
    }

    LocalFree(argv);

    if (isExcluded) {
        return FALSE;
    }

    if (isCurrentToolModProcess) {
        g_toolModProcessMutex =
            CreateMutex(nullptr, TRUE, L"windhawk-tool-mod_" WH_MOD_ID);
        if (!g_toolModProcessMutex) {
            Wh_Log(L"CreateMutex failed");
            ExitProcess(1);
        }

        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            Wh_Log(L"Tool mod already running (%s)", WH_MOD_ID);
            ExitProcess(1);
        }

        if (!WhTool_ModInit()) {
            ExitProcess(1);
        }

        IMAGE_DOS_HEADER* dosHeader =
            (IMAGE_DOS_HEADER*)GetModuleHandle(nullptr);
        IMAGE_NT_HEADERS* ntHeaders =
            (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);

        DWORD entryPointRVA = ntHeaders->OptionalHeader.AddressOfEntryPoint;
        void* entryPoint = (BYTE*)dosHeader + entryPointRVA;

        Wh_SetFunctionHook(entryPoint, (void*)EntryPoint_Hook, nullptr);
        return TRUE;
    }

    if (isToolModProcess) {
        return FALSE;
    }

    g_isToolModProcessLauncher = true;
    return TRUE;
}

void Wh_ModAfterInit() {
    // explorer.exe part.
    if (g_isExplorer) {
        ExplorerModAfterInit();
        return;
    }

    if (!g_isToolModProcessLauncher) {
        return;
    }

    WCHAR currentProcessPath[MAX_PATH];
    switch (GetModuleFileName(nullptr, currentProcessPath,
                              ARRAYSIZE(currentProcessPath))) {
        case 0:
        case ARRAYSIZE(currentProcessPath):
            Wh_Log(L"GetModuleFileName failed");
            return;
    }

    WCHAR
    commandLine[MAX_PATH + 2 +
                (sizeof(L" -tool-mod \"" WH_MOD_ID "\"") / sizeof(WCHAR)) - 1];
    swprintf_s(commandLine, L"\"%s\" -tool-mod \"%s\"", currentProcessPath,
               WH_MOD_ID);

    HMODULE kernelModule = GetModuleHandle(L"kernelbase.dll");
    if (!kernelModule) {
        kernelModule = GetModuleHandle(L"kernel32.dll");
        if (!kernelModule) {
            Wh_Log(L"No kernelbase.dll/kernel32.dll");
            return;
        }
    }

    using CreateProcessInternalW_t = BOOL(WINAPI*)(
        HANDLE hUserToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes, WINBOOL bInheritHandles,
        DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
        LPSTARTUPINFOW lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation,
        PHANDLE hRestrictedUserToken);
    CreateProcessInternalW_t pCreateProcessInternalW =
        (CreateProcessInternalW_t)GetProcAddress(kernelModule,
                                                 "CreateProcessInternalW");
    if (!pCreateProcessInternalW) {
        Wh_Log(L"No CreateProcessInternalW");
        return;
    }

    STARTUPINFO si{
        .cb = sizeof(STARTUPINFO),
        .dwFlags = STARTF_FORCEOFFFEEDBACK,
    };
    PROCESS_INFORMATION pi;
    if (!pCreateProcessInternalW(nullptr, currentProcessPath, commandLine,
                                 nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS,
                                 nullptr, nullptr, &si, &pi, nullptr)) {
        Wh_Log(L"CreateProcess failed");
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

// explorer.exe part (not part of the tool mod snippet).
void Wh_ModBeforeUninit() {
    if (g_isExplorer) {
        ExplorerModBeforeUninit();
    }
}

void Wh_ModSettingsChanged() {
    // explorer.exe part.
    if (g_isExplorer) {
        ExplorerModSettingsChanged();
        return;
    }

    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModSettingsChanged();
}

void Wh_ModUninit() {
    // explorer.exe part.
    if (g_isExplorer) {
        return;
    }

    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModUninit();
    ExitProcess(0);
}
