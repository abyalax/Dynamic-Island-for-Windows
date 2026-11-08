// An option's value put in memory only (not kept, nothing else done), for a
// monitor's look. The ones for all the islands at once (where they are,
// the animations, who handles the notifications) aren't a monitor's.
void SetIslandPrefValue(IslandPref pref, int value) {
    const bool on = value != 0;
    switch (pref) {
        case IslandPref::Bar:
            g_settings.islandBar = value == 1;
            g_settings.islandReserve = value == 2;
            break;
        case IslandPref::Theme:
            g_themeIndex = std::clamp(value, 0, (int)ThemeId::Count - 1);
            ApplyThemeIndex(false);
            break;
        case IslandPref::Tray:
            g_settings.islandTray = on;
            break;
        case IslandPref::TrayOpen:
            g_settings.islandTrayOpen = on;
            break;
        case IslandPref::Network:
            g_settings.islandNetwork = on;
            break;
        case IslandPref::Volume:
            g_settings.islandVolume = on;
            break;
        case IslandPref::Battery:
            g_settings.islandBattery = on;
            break;
        case IslandPref::Bell:
            g_settings.islandBell = on;
            break;
        case IslandPref::Media:
            g_settings.islandMedia = on;
            break;
        case IslandPref::MediaButtons:
            g_mediaButtonsInPill = on;
            break;
        case IslandPref::MediaTitle:
            g_mediaTitleInPill = on;
            break;
        case IslandPref::Banner:
            g_bannerEnabled = on;
            break;
        case IslandPref::BannerMinimized:
            g_bannerWhenMinimized = on;
            break;
        case IslandPref::WebPictures:
            g_settings.islandWebPictures = on;
            break;
        case IslandPref::ClockSeconds:
            g_settings.clockSeconds = on ? 1 : 0;
            break;
        case IslandPref::ClockWeekday:
            g_settings.clockWeekday = on;
            break;
        case IslandPref::ClockDate:
            g_settings.clockDate = on;
            break;
        case IslandPref::ClockYear:
            g_settings.clockYear = on;
            break;
        case IslandPref::FreeDrag:
            g_settings.islandFreeDrag = on;
            break;
        case IslandPref::AutoHide:
            g_settings.islandAutoHide = on;
            break;
        case IslandPref::VerticalSides:
            g_settings.islandVerticalSides = on;
            break;
        case IslandPref::Clock:
            g_settings.islandClock = on;
            break;
        case IslandPref::ClockCenter:
            g_settings.islandClockCenter = on;
            break;
        case IslandPref::Power:
            g_settings.islandPower = on;
            break;
        case IslandPref::PanelsFollow:
            g_settings.islandPanelsFollow = on;
            break;
        case IslandPref::ThemeButton:
            g_settings.islandThemeButton = on;
            break;
        case IslandPref::UsageCpu:
            g_settings.usageCpu = on;
            break;
        case IslandPref::UsageRam:
            g_settings.usageRam = on;
            break;
        case IslandPref::UsageVram:
            g_settings.usageVram = on;
            break;
        case IslandPref::Microphone:
            g_settings.islandMicrophone = on;
            break;
        case IslandPref::Holidays:
            g_settings.calendarHolidays = on;
            break;
        case IslandPref::Notes:
            g_settings.islandNotes = on;
            break;
        case IslandPref::Tasks:
            g_settings.islandTasks = on;
            break;
        case IslandPref::Shortcuts:
            g_settings.islandShortcuts = on;
            break;
        case IslandPref::Weather:
            g_settings.islandWeather = on;
            break;
        case IslandPref::Updates:
            g_settings.islandUpdates = on;
            break;
        case IslandPref::Profiles:
            g_settings.islandProfiles = on;
            break;
        case IslandPref::VolumePercent:
            g_settings.islandVolumePercent = on;
            break;
        case IslandPref::BatteryPercent:
            g_settings.islandBatteryPercent = on;
            break;
        case IslandPref::Brightness:
            g_settings.islandBrightness = on;
            break;
        case IslandPref::Clipboard:
            g_settings.islandClipboard = on;
            break;
        case IslandPref::InputMethod:
            g_settings.islandInputMethod = on;
            break;
        case IslandPref::Privacy:
            g_settings.islandPrivacy = on;
            break;
        case IslandPref::UsbDrives:
            g_settings.islandUsbDrives = on;
            break;
        case IslandPref::UsbNotify:
            g_settings.islandUsbNotify = on;
            break;
        case IslandPref::Frosted:
            g_settings.islandFrosted = on;
            break;
        case IslandPref::BarClear:
            g_settings.islandBarClear = on;
            break;
        case IslandPref::AiUsage:
            g_settings.islandAiUsage = on;
            break;
        case IslandPref::AiClaude:
            g_settings.islandAiClaude = on;
            break;
        case IslandPref::AiCodex:
            g_settings.islandAiCodex = on;
            break;
        case IslandPref::FocusedTitle:
            g_settings.islandFocusedTitle = on;
            break;
        case IslandPref::WeatherCards:
            g_settings.islandWeatherCards = on;
            break;
        case IslandPref::CalendarBigDay:
            g_settings.islandCalendarBigDay = on;
            break;
        case IslandPref::Commands:
            g_settings.islandCommands = on;
            break;
        case IslandPref::DndSilent:
            g_settings.islandDndSilent = on;
            break;
        case IslandPref::DownloadStalled:
            g_settings.islandDownloadStalled = on;
            break;
        case IslandPref::FolderThumbnails:
            g_settings.islandFolderThumbnails = on;
            break;
        case IslandPref::Shelf:
            g_settings.islandShelf = on;
            break;
        case IslandPref::Eyes:
            g_settings.islandEyes = on;
            break;
        case IslandPref::EyeTheme:
            g_settings.islandEyeTheme =
                std::clamp(value, 0, (int)std::size(kEyeThemes) - 1);
            break;
        case IslandPref::EyeShape:
            g_settings.islandEyeShape =
                std::clamp(value, 0, (int)std::size(kEyeShapes) - 1);
            break;
        case IslandPref::TimerLine:
            g_settings.islandTimerLine = on;
            break;
        case IslandPref::ExternalBrightness:
            g_settings.islandExternalBrightness = on;
            break;
        case IslandPref::GlassLight:
            g_settings.islandGlassLight = on;
            break;
        case IslandPref::LiquidGlass:
            g_settings.islandLiquidGlass = on;
            break;
        case IslandPref::Tips:
            g_settings.islandTips = on;
            break;
        case IslandPref::HoverHighlight:
            g_settings.islandHoverHighlight = on;
            break;
        case IslandPref::MinimizeButton:
            g_settings.islandMinimizeButton = on;
            break;
        case IslandPref::BarCorners:
            g_settings.islandBarCorners = on;
            break;
        case IslandPref::GlassTranslucent:
            g_settings.islandGlassTranslucent = on;
            break;
        case IslandPref::DateOrder:
            g_settings.clockDateOrder = std::clamp(value, 0, 3);
            break;
        case IslandPref::ClockFormat:
            g_settings.clockFormat = std::clamp(value, 0, 2);
            break;
        case IslandPref::WeatherUnit:
            g_settings.weatherUnit = std::clamp(value, 0, 2);
            break;
        case IslandPref::IconPack:
            g_settings.islandIconPack = std::clamp(value, 0, kIconPacks - 1);
            g_iconLook++;
            break;
        case IslandPref::IconColors:
            g_settings.islandIconColors = std::clamp(value, 0, 1);
            g_iconLook++;
            break;
        case IslandPref::TrayStyle:
            g_settings.islandTrayStyle = std::clamp(value, 0, 2);
            break;
        case IslandPref::Desktops:
            g_settings.islandDesktops = on;
            break;
        case IslandPref::Folders:
            g_settings.islandFolders = on;
            break;
        case IslandPref::Wallpaper:
            g_settings.islandWallpaper = on;
            break;
        case IslandPref::NotificationApps:
            g_settings.islandApps = on;
            break;
        case IslandPref::KomorebiLayout:
            g_settings.komorebiLayout = on;
            break;
        case IslandPref::FocusedApp:
            g_settings.islandFocusedApp = on;
            break;
        case IslandPref::KomorebiHideEmpty:
            g_settings.komorebiHideEmpty = on;
            break;
        case IslandPref::FolderDrag:
            g_settings.islandFolderDrag =
                std::clamp(value, 0, kFolderDragAskAlways);
            break;
        case IslandPref::WallpaperTransition:
            g_settings.wallpaperTransition =
                std::clamp(value, 0, (int)WallpaperTransition::Count - 1);
            break;
        case IslandPref::WindowColor:
            g_settings.islandWindowColor = on;
            // (The colors depend on it: a profile's look, put together option
            // by option, mustn't keep the colors made while it still had the
            // main island's choice.)
            ApplyThemeIndex(false);
            break;
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
            SetPillActionBit(pref, on);
            break;
        case IslandPref::KomorebiWhkd:
            g_settings.komorebiWhkd = on;
            break;
        case IslandPref::KomorebiMasir:
            g_settings.komorebiMasir = on;
            break;
        case IslandPref::ScreenshotFolder:
            g_settings.islandScreenshotFolder = on;
            break;
        case IslandPref::MediaWheel:
            g_settings.islandMediaWheel = on;
            break;
        case IslandPref::MediaMarquee:
            g_settings.islandMediaMarquee = on;
            break;
        case IslandPref::ShortcutNotifications:
            g_settings.islandWinN = on;
            break;
        case IslandPref::ShortcutControl:
            g_settings.islandWinA = on;
            break;
        case IslandPref::MinimizeKey:
            g_settings.islandMinimizeKey = on;
            break;
        case IslandPref::CapsLock:
            g_settings.islandCapsLock = on;
            break;
        case IslandPref::HideWhen:
            g_settings.islandHideWhen = std::clamp(value, 0, 2);
            break;
        case IslandPref::LineClick:
            g_settings.islandLineClick = std::clamp(value, 0, 1);
            break;
        case IslandPref::LineFade:
            g_settings.islandLineFade =
                std::clamp(value, 0, (int)std::size(kLineFadeSeconds) - 1);
            break;
        case IslandPref::BarSide:
            g_settings.islandBarSide = std::clamp(value, 0, 2);
            break;
        case IslandPref::BarInnerCorners:
            g_settings.islandBarInnerCorners = std::clamp(
                value, 0, (int)std::size(kBarInnerCornerRadii) - 1);
            break;
        case IslandPref::BannerSize:
            g_settings.islandBannerSize =
                std::clamp(value, 0, (int)std::size(kBannerSizes) - 1);
            break;
        case IslandPref::ShelfLimit:
            g_settings.islandShelfLimit =
                std::clamp(value, 0, (int)std::size(kShelfLimits) - 1);
            break;
        case IslandPref::BannerWidth:
            g_settings.islandBannerWidth =
                std::clamp(value, 0, (int)std::size(kBannerWidths) - 1);
            break;
        case IslandPref::BannerCorners:
            g_settings.islandBannerCorners =
                std::clamp(value, 0, (int)std::size(kBannerCornerShares) - 1);
            break;
        case IslandPref::BannerPlace:
            g_settings.islandBannerPlace =
                std::clamp(value, 0, kBannerPlaces - 1);
            break;
        case IslandPref::HideClock:
            g_settings.islandHideClock = on;
            break;
        case IslandPref::NetSpeed:
            g_settings.islandNetSpeed = on;
            break;
        case IslandPref::WorldClocks:
            g_settings.islandWorldClocks = on;
            break;
        case IslandPref::Countdown:
            g_settings.islandCountdown = on;
            break;
        case IslandPref::CountdownEvents:
            g_settings.islandCountdownEvents = on;
            break;
        case IslandPref::CountdownAlarms:
            g_settings.islandCountdownAlarms = on;
            break;
        case IslandPref::CountdownTimer:
            g_settings.islandCountdownTimer = on;
            break;
        case IslandPref::CountdownPomodoro:
            g_settings.islandCountdownPomodoro = on;
            break;
        case IslandPref::DownloadFill:
            g_settings.islandDownloadFill = on;
            break;
        case IslandPref::ChargeFill:
            g_settings.islandChargeFill = on;
            break;
        case IslandPref::FillColor:
            g_settings.islandFillColor = std::clamp(value, 0, 3);
            break;
        case IslandPref::FillOwnColor:
            g_settings.islandFillOwnColor = value & 0xFFFFFF;
            break;
        case IslandPref::FillCopyColor:
            g_settings.islandFillCopyColor = std::clamp(value, 0, 3);
            break;
        case IslandPref::FillCopyOwnColor:
            g_settings.islandFillCopyOwnColor = value & 0xFFFFFF;
            break;
        case IslandPref::FillChargeColor:
            g_settings.islandFillChargeColor = std::clamp(value, 0, 3);
            break;
        case IslandPref::FillChargeOwnColor:
            g_settings.islandFillChargeOwnColor = value & 0xFFFFFF;
            break;
        case IslandPref::FillStyle:
            g_settings.islandFillStyle = std::clamp(value, 0, 2);
            break;
        case IslandPref::Lyrics:
            g_settings.islandLyrics = on;
            break;
        case IslandPref::LyricsSize:
            g_settings.islandLyricsSize = std::clamp(value, 0, 2);
            break;
        case IslandPref::LyricsColor:
            g_settings.islandLyricsColor = std::clamp(value, 0, 2);
            break;
        case IslandPref::LyricsOwnColor:
            g_settings.islandLyricsOwnColor = value & 0xFFFFFF;
            break;
        case IslandPref::LyricsInTitle:
            g_settings.islandLyricsInTitle = on;
            break;
        case IslandPref::VisualizerColor:
            g_settings.islandVisualizerColor = std::clamp(value, 0, 4);
            break;
        case IslandPref::VisualizerOwnColor:
            g_settings.islandVisualizerOwnColor = value & 0xFFFFFF;
            break;
        case IslandPref::VisualizerSmooth:
            g_settings.islandVisualizerSmooth = on;
            break;
        case IslandPref::WhatsNewNotice:
            g_settings.islandWhatsNewNotice = on;
            break;
        case IslandPref::RecycleMinSize:
            g_settings.islandRecycleMinSize = std::clamp(
                value, 0, (int)std::size(kRecycleMinMegabytes) - 1);
            break;
        case IslandPref::LyricsWidth:
            g_settings.islandLyricsWidth = std::clamp(
                value, 0, (int)std::size(kLyricsMaxWidths) - 1);
            break;
        case IslandPref::RecycleBin:
            g_settings.islandRecycleBin = on;
            break;
        case IslandPref::StatusNotices:
            g_settings.islandStatusNotices = on;
            break;
        case IslandPref::FoldApps:
            g_settings.islandFoldApps = on;
            break;
        case IslandPref::Timer:
            g_settings.islandTimer = on;
            break;
        case IslandPref::Visualizer:
            g_settings.islandVisualizer = on;
            break;
        case IslandPref::MonthNumber:
            g_settings.clockMonthNumber = on;
            break;
        case IslandPref::Notches:
            g_settings.islandNotches = std::clamp(value, 0, kMaxNotches);
            break;
        case IslandPref::PillNotches:
            g_settings.islandPillNotches = std::clamp(value, 0, kMaxNotches);
            break;
        case IslandPref::NotchOpen:
            g_settings.islandNotchOpen = std::clamp(value, 0, 3);
            break;
        case IslandPref::NotchEach:
            g_settings.islandNotchEach = on;
            break;
        case IslandPref::HideWindowsBanners:
        case IslandPref::OwnNotifications:
        case IslandPref::Animations:
        case IslandPref::FollowMouse:
        case IslandPref::AllMonitors:
        case IslandPref::GroupApps:
            break;
    }
}

// The main island's settings with a profile's put over them, in memory (the
// main settings are back in memory after); only the options the profile
// lacks are written into it.
IslandLook LookFromProfile(int id) {
    const IslandLook main = CaptureLook();
    const std::wstring value = ReadProfileText(id);
    std::unordered_map<std::wstring, int> numbers;
    std::unordered_map<std::wstring, std::wstring> texts;
    ParseProfileLines(value.data(), &numbers, &texts);
    // (The custom colors before the theme, which may be them.)
    for (int i = 0; i < kCustomColors; i++) {
        if (const auto it = numbers.find(kCustomColorKeys[i]);
            it != numbers.end()) {
            g_customColors[i] = it->second & 0xFFFFFF;
        }
    }
    // An option the profile doesn't have (saved before the option came):
    // what it shows now (the main island's) is written into it, so the main
    // island changing it later doesn't change this profile's monitor.
    std::wstring missing;
    for (const auto& info : kIslandPrefs) {
        if (!info.key || IsSharedPref(info.pref)) {
            continue;
        }
        if (const auto it = numbers.find(info.key); it != numbers.end()) {
            SetIslandPrefValue(info.pref, it->second);
        } else if (!value.empty()) {
            missing += L"i" + std::wstring(info.key) + L"=" +
                       std::to_wstring(GetIslandPref(info.pref)) + L"\n";
        }
    }
    if (!missing.empty()) {
        std::wstring text = value;
        if (text.back() != L'\n') {
            text += L'\n';
        }
        SetStoredString(ProfileValueName(id).c_str(),
                        (text + missing).c_str());
    }
    if (const auto it = numbers.find(L"islandSize"); it != numbers.end()) {
        g_settings.islandSize =
            std::clamp(it->second, kIslandSizeMin, kIslandSizeMax);
    }
    if (const auto it = numbers.find(L"islandGap"); it != numbers.end()) {
        g_settings.islandGap =
            std::clamp(it->second, kIslandGapMin, kIslandGapMax);
    }
    if (const auto it = numbers.find(L"islandOpacity"); it != numbers.end()) {
        g_islandOpacity =
            std::clamp(it->second, kIslandOpacityMin, kIslandOpacityMax);
    }
    if (const auto it = numbers.find(L"trayListIsland"); it != numbers.end()) {
        g_trayListIcons = std::clamp(it->second, kTrayListMin, kTrayListMax);
    }
    if (const auto it = numbers.find(L"islandAutoHideDelay");
        it != numbers.end()) {
        g_settings.islandAutoHideDelay =
            std::clamp(it->second, kAutoHideDelayMin, kAutoHideDelayMax);
    }
    if (const auto it = numbers.find(L"islandNotchCurve");
        it != numbers.end()) {
        g_settings.islandNotchCurve =
            std::clamp(it->second, kNotchCurveMin, kNotchCurveMax);
    }
    if (const auto it = numbers.find(L"islandIconSize"); it != numbers.end()) {
        g_settings.islandIconSize =
            std::clamp(it->second, kIconSizeMin, kIconSizeMax);
    }
    if (const auto it = numbers.find(L"islandTextSize"); it != numbers.end()) {
        g_settings.islandTextSize =
            std::clamp(it->second, kTextSizeMin, kTextSizeMax);
    } else if (numbers.contains(L"islandIconSize")) {
        g_settings.islandTextSize =
            std::clamp(g_settings.islandIconSize, kTextSizeMin, kTextSizeMax);
    }
    if (const auto it = numbers.find(L"islandLineOpacity");
        it != numbers.end()) {
        g_settings.islandLineOpacity =
            std::clamp(it->second, kLineOpacityMin, kLineOpacityMax);
    }
    if (const auto it = numbers.find(L"islandActivationWidth");
        it != numbers.end()) {
        g_settings.islandActivationWidth =
            std::clamp(it->second, kActivationWidthMin, kActivationWidthMax);
    }
    if (const auto it = numbers.find(L"islandPanelSize");
        it != numbers.end()) {
        g_settings.islandPanelSize =
            std::clamp(it->second, kPanelSizeMin, kPanelSizeMax);
    }
    if (const auto it = numbers.find(L"islandMediaTitleLimit");
        it != numbers.end()) {
        g_settings.islandMediaTitleLimit =
            std::clamp(it->second, kMediaTitleLimitMin, kMediaTitleLimitMax);
    }
    if (const auto it = numbers.find(L"islandPillHeight");
        it != numbers.end()) {
        g_settings.islandPillHeight =
            std::clamp(it->second, kPillHeightPctMin, kPillHeightPctMax);
    }
    if (const auto it = numbers.find(L"islandPillCorners");
        it != numbers.end()) {
        g_settings.islandPillCorners =
            std::clamp(it->second, kPillCornersMin, kPillCornersMax);
    }
    if (const auto it = numbers.find(L"islandPillEnds"); it != numbers.end()) {
        g_settings.islandPillEnds =
            std::clamp(it->second, kPillEndsMin, kPillEndsMax);
    }
    if (const auto it = numbers.find(L"islandEdge"); it != numbers.end()) {
        g_settings.islandEdge = std::clamp(it->second, kEdgeMin, kEdgeMax);
    }
    if (const auto it = numbers.find(L"islandGlassBlur"); it != numbers.end()) {
        g_settings.islandGlassBlur =
            std::clamp(it->second, kGlassBlurMin, kGlassBlurMax);
    }
    if (const auto it = numbers.find(L"islandTrayColumns");
        it != numbers.end()) {
        g_settings.islandTrayColumns =
            std::clamp(it->second, kTrayColumnsMin, kTrayColumnsMax);
    }
    if (const auto it = numbers.find(L"islandVizSensitivity");
        it != numbers.end()) {
        g_settings.islandVizSensitivity =
            std::clamp(it->second, kVizSensitivityMin, kVizSensitivityMax);
    }
    if (const auto it = texts.find(L"islandOrder"); it != texts.end()) {
        if (std::vector<PillGroup> order = DecodePillOrder(it->second);
            !order.empty()) {
            g_pillOrder = std::move(order);
        }
    }
    if (const auto it = texts.find(L"islandNotchSides"); it != texts.end()) {
        for (size_t i = 0; i < kPillGroups; i++) {
            const WCHAR c = i < it->second.size() ? it->second[i] : 0;
            g_notchSides[i] = c >= L'0' && c <= L'2'
                                  ? (uint8_t)(c - L'0')
                                  : DefaultNotchSide((PillGroup)i);
        }
    }
    // (A profile saved before it kept them: the main look's.)
    if (const auto it = texts.find(L"trayPinned"); it != texts.end()) {
        g_trayPinned = DecodeTrayKeys(it->second);
    }
    if (const auto it = texts.find(L"trayOrder"); it != texts.end()) {
        g_trayOrder = DecodeTrayKeys(it->second);
    }
    if (const auto it = texts.find(L"trayAnchors"); it != texts.end()) {
        g_trayAnchors = DecodeTrayAnchors(it->second);
    }
    if (const auto it = texts.find(L"islandIconFolder"); it != texts.end()) {
        g_iconFolder = it->second;
    }
    if (const auto it = texts.find(L"islandIconFiles"); it != texts.end()) {
        g_iconFiles = DecodeIconFiles(it->second);
    }
    IslandLook look = CaptureLook();
    PutLook(main);
    return look;
}

// The islands chosen to show the notifications (see g_notifyScreens).
void LoadNotificationScreens() {
    g_notifyMain = Wh_GetIntValue(L"notifyMainIsland", 1) != 0;
    g_notifyScreens.clear();
    // (Kept under 1500 characters, see SetNotificationScreen: one read.)
    const std::wstring value = GetStringValueIn(L"notifyScreens", 2048);
    for (size_t start = 0; start < value.size();) {
        size_t end = value.find(L'\n', start);
        if (end == std::wstring::npos) {
            end = value.size();
        }
        if (end > start) {
            g_notifyScreens.push_back(value.substr(start, end - start));
        }
        start = end + 1;
    }
}

// An island chosen to show them or not: a monitor's by its device, the main
// one's with none.
void SetNotificationScreen(const std::wstring& device, bool on) {
    if (device.empty()) {
        g_notifyMain = on;
        Wh_SetIntValue(L"notifyMainIsland", on ? 1 : 0);
        return;
    }
    std::erase(g_notifyScreens, device);
    if (on) {
        g_notifyScreens.push_back(device);
    }
    std::wstring value;
    for (const auto& screen : g_notifyScreens) {
        if (value.size() + screen.size() < 1500) {
            value += screen + L"\n";
        }
    }
    SetStoredString(L"notifyScreens", value.c_str());
}

// Each monitor's own profile: "id\tdevice" lines.
int LoadMonitorProfile(const std::wstring& device) {
    const std::wstring value = GetWholeStringValue(L"monitorProfiles", 2048);
    for (PCWSTR line = value.c_str(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        const std::wstring text =
            end ? std::wstring(line, end) : std::wstring(line);
        const size_t tab = text.find(L'\t');
        if (tab != std::wstring::npos && text.substr(tab + 1) == device) {
            const int id = _wtoi(text.c_str());
            return std::any_of(g_profiles.begin(), g_profiles.end(),
                               [id](const IslandProfile& profile) {
                                   return profile.id == id;
                               })
                       ? id
                       : -1;
        }
        if (!end) {
            break;
        }
        line = end + 1;
    }
    return -1;
}

void SaveMonitorProfile(const std::wstring& device, int id) {
    const std::wstring value = GetWholeStringValue(L"monitorProfiles", 2048);
    std::wstring kept;
    for (PCWSTR line = value.c_str(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        const std::wstring text =
            end ? std::wstring(line, end) : std::wstring(line);
        const size_t tab = text.find(L'\t');
        if (tab != std::wstring::npos && text.substr(tab + 1) != device &&
            kept.size() < 1500) {
            kept += text + L"\n";
        }
        if (!end) {
            break;
        }
        line = end + 1;
    }
    if (id >= 0) {
        kept += std::to_wstring(id) + L"\t" + device + L"\n";
    }
    SetStoredString(L"monitorProfiles", kept.c_str());
}

void Island::SetMonitorProfile(int id) {
    // (A notch: its monitor's island's profile.)
    if (m_notchBase) {
        IslandScope scope(m_notchBase);
        m_notchBase->SetMonitorProfile(id);
        return;
    }
    if (m_main) {
        return;
    }
    // (Its own changes still waiting go in the profile it had, its notches'
    // too.)
    if (m_profileSavePending) {
        SaveOwnProfile();
    }
    for (Island* notch : m_notchSiblings) {
        if (notch->m_profileSavePending) {
            IslandScope scope(notch);
            notch->SaveOwnProfile();
        }
    }
    SaveMonitorProfile(m_ownDevice, id);
    m_profileId = id;
    // Its notches show it too.
    for (Island* notch : m_notchSiblings) {
        notch->m_profileId = id;
    }
    ReloadLook();
    for (Island* notch : m_notchSiblings) {
        IslandScope scope(notch);
        notch->ReloadLook();
    }
}

void Island::ReloadLook() {
    RefreshLook();
    // Another style (the pill, the fixed pill, the bar), or other notches:
    // they start again. (A notch beside its base keeps no strip: the base
    // does, see RegisterAppBar.)
    if (m_bar != g_settings.islandBar ||
        m_appBar != ((g_settings.islandBar || g_settings.islandReserve) &&
                     !m_notchBase) ||
        m_notchCount != NotchCountWanted()) {
        RestartIslandSoon();
        return;
    }
    OnOrderChanged();
    OnPlacesChanged();
    UpdateClock();
    Render(true);
}

void Island::SaveOwnProfile(bool show) {
    CancelProfileSave();
    const int id = m_profileId;
    if (m_main || id < 0 ||
        std::none_of(g_profiles.begin(), g_profiles.end(),
                     [id](const IslandProfile& profile) {
                         return profile.id == id;
                     })) {
        return;
    }
    {
        IslandScope scope(this);
        SetStoredString(ProfileValueName(id).c_str(),
                        CaptureShownProfile(id).c_str());
    }
    if (show) {
        ShowSavedProfile(id, this);
    }
}

void Island::RefreshLook() {
    g_windowsAccentColor = ReadWindowsAccentColor();
    if (m_main) {
        return;
    }
    // (Its own changes still waiting are kept first.)
    if (m_profileSavePending) {
        SaveOwnProfile();
    }
    // (The colors shown, to keep blending from them.)
    const bool shown = g_island == this || m_profileId >= 0;
    const IslandLook before = g_island == this ? CaptureLook() : m_look;
    // (Made from the main settings: those go in memory meanwhile.)
    const bool applied = m_lookApplied;
    if (applied) {
        PutLook(g_mainLook);
        m_lookApplied = false;
    }
    if (m_profileId >= 0 &&
        std::none_of(g_profiles.begin(), g_profiles.end(),
                     [this](const IslandProfile& profile) {
                         return profile.id == m_profileId;
                     })) {
        // Its profile was deleted: the main settings again.
        m_profileId = -1;
        SaveMonitorProfile(m_ownDevice, -1);
    }
    if (m_profileId >= 0) {
        m_look = LookFromProfile(m_profileId);
        if (shown && KeepThemeBlend(before, m_look)) {
            StartAnimating();
        }
    }
    if ((applied || g_island == this) && m_profileId >= 0) {
        g_mainLook = CaptureLook();
        PutLook(m_look);
        m_lookApplied = true;
    }
}

void Island::OnProfileApplied() {
    // The bell moved to another notch, or the profile chose another monitor
    // while there's an island on each one (or notches, made on the chosen
    // one): they start again, as ChooseMonitor does (two islands would end
    // up on the same monitor otherwise).
    if (NotchBaseMoved() ||
        (m_main &&
         (g_settings.islandAllMonitors || NotchCountWanted() > 0) &&
         IslandMonitorsChanged())) {
        RestartIslandSoon();
        return;
    }
    m_control.ReloadControls();
    OnOrderChanged();
    OnPlacesChanged();
    OnMonitorChoiceChanged();
    OnPreferencesChanged();
}

void OpenIslandSettingsFromPanel() {
    if (g_island) {
        g_island->OpenSettingsWindow();
    }
}

////////////////////////////////////////////////////////////////////////////////
// The island.

////////////////////////////////////////////////////////////////////////////////
// The island is told when what it shows changes, instead of asking: the
// volume and the default output device (Core Audio), the network (the network
// list manager, and Wi-Fi's own notifications for its signal), the battery
// (power setting notifications) and "Do not disturb" (its WNF state, see
// IsDoNotDisturbOn). Each posts WM_APP_STATUS to the island's window, from
// whatever thread it comes on.

constexpr GUID kIID_IAudioEndpointVolumeCallback = {
    0x657804fa,
    0xd6ad,
    0x4496,
    {0x8a, 0x60, 0x35, 0x27, 0x52, 0xaf, 0x4f, 0x89}};
constexpr GUID kIID_IMMNotificationClient = {
    0x7991eec9,
    0x7e89,
    0x4d85,
    {0x83, 0x90, 0x6c, 0x70, 0x3c, 0xec, 0x60, 0xc0}};
constexpr GUID kIID_INetworkListManagerEvents = {
    0xdcb00001,
    0x570f,
    0x4a9b,
    {0x8d, 0x69, 0x19, 0x9f, 0xdb, 0xa5, 0x72, 0x3b}};
constexpr GUID kIID_IConnectionPointContainer = {
    0xb196b284,
    0xbab4,
    0x101a,
    {0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07}};
constexpr GUID kGUID_BatteryPercentageRemaining = {
    0xa7ad8041,
    0xb45a,
    0x4cae,
    {0x87, 0xa3, 0xee, 0xcb, 0xb4, 0x68, 0xa9, 0xe1}};
constexpr GUID kGUID_AcDcPowerSource = {
    0x5d3e9a59,
    0xe9d5,
    0x4b00,
    {0xa6, 0xbd, 0xff, 0x34, 0xff, 0x51, 0x65, 0x48}};
// GUID_POWER_SAVING_STATUS: Windows' energy saver (battery saver) on or off.
constexpr GUID kGUID_PowerSavingStatus = {
    0xe00958c0,
    0xc213,
    0x4ace,
    {0xac, 0x77, 0xfe, 0xcc, 0xed, 0x2e, 0xee, 0xa5}};
constexpr ULONGLONG kWnfQuietHoursState = 0x0D83063EA3BF1C75;

class StatusEvents final : public IAudioEndpointVolumeCallback,
                           public IMMNotificationClient,
                           public INetworkListManagerEvents {
   public:
    explicit StatusEvents(HWND hwnd) : m_hwnd(hwnd) {}

    // IUnknown (the three interfaces share it).
    IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override {
        if (riid == IID_IUnknown || riid == kIID_IAudioEndpointVolumeCallback) {
            *object = static_cast<IAudioEndpointVolumeCallback*>(this);
        } else if (riid == kIID_IMMNotificationClient) {
            *object = static_cast<IMMNotificationClient*>(this);
        } else if (riid == kIID_INetworkListManagerEvents) {
            *object = static_cast<INetworkListManagerEvents*>(this);
        } else {
            *object = nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&m_refs);
    }
    IFACEMETHODIMP_(ULONG) Release() override {
        const ULONG refs = InterlockedDecrement(&m_refs);
        if (!refs) {
            delete this;
        }
        return refs;
    }

    // The volume or the mute changed.
    IFACEMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA data) override {
        if (data && IsEqualGUID(data->guidEventContext, kMediaVolumeContext)) {
            const unsigned packed =
                (unsigned)std::lround(std::clamp(data->fMasterVolume, 0.0f, 1.0f) * 10000) |
                (data->bMuted ? 0x10000u : 0u);
            m_mediaVolume.store(packed);
            if (!m_mediaVolumePending.exchange(true)) {
                Post(kStatusMediaVolume);
            }
            return S_OK;
        }
        Post(kStatusVolume);
        return S_OK;
    }

    // Another default output device, or one added or removed.
    IFACEMETHODIMP OnDefaultDeviceChanged(EDataFlow flow,
                                          ERole role,
                                          LPCWSTR) override {
        if (flow == eRender && role == eMultimedia) {
            Post(kStatusAudioDevice);
        }
        if (flow == eCapture && role == eConsole) {
            Post(kStatusMicrophoneDevice);
        }
        return S_OK;
    }
    IFACEMETHODIMP OnDeviceAdded(LPCWSTR) override { return S_OK; }
    IFACEMETHODIMP OnDeviceRemoved(LPCWSTR) override { return S_OK; }
    IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR, DWORD) override {
        Post(kStatusAudioDevices);
        return S_OK;
    }
    IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR,
                                          const PROPERTYKEY) override {
        return S_OK;
    }

    // Connected, disconnected, with or without internet.
    IFACEMETHODIMP ConnectivityChanged(NLM_CONNECTIVITY) override {
        Post(kStatusNetwork);
        return S_OK;
    }

    // From the island's thread before it closes its window.
    void Detach() { m_hwnd = nullptr; }
    unsigned TakeMediaVolume() {
        m_mediaVolumePending = false;
        return m_mediaVolume.load();
    }

   private:
    std::atomic<unsigned> m_mediaVolume{0};
    std::atomic<bool> m_mediaVolumePending{false};
    void Post(StatusChange change) {
        if (HWND hwnd = m_hwnd) {
            PostMessage(hwnd, WM_APP_STATUS, change, 0);
        }
    }

    LONG m_refs = 1;
    std::atomic<HWND> m_hwnd;
};

// Wi-Fi: connected, disconnected, or its signal changed.
void WINAPI IslandWlanCallback(PWLAN_NOTIFICATION_DATA data, PVOID context) {
    const bool msm =
        data->NotificationSource == WLAN_NOTIFICATION_SOURCE_MSM &&
        (data->NotificationCode == wlan_notification_msm_signal_quality_change ||
         data->NotificationCode == wlan_notification_msm_associating ||
         data->NotificationCode == wlan_notification_msm_authenticating ||
         data->NotificationCode == wlan_notification_msm_connected ||
         data->NotificationCode == wlan_notification_msm_disconnected);
    const bool acm =
        data->NotificationSource == WLAN_NOTIFICATION_SOURCE_ACM &&
        (data->NotificationCode == wlan_notification_acm_connection_start ||
         data->NotificationCode == wlan_notification_acm_connection_complete ||
         data->NotificationCode ==
             wlan_notification_acm_connection_attempt_fail ||
         data->NotificationCode == wlan_notification_acm_disconnected);
    if (msm || acm) {
        PostMessage((HWND)context, WM_APP_STATUS, kStatusNetwork, 0);
    }
}

// "Do not disturb" switched (on any thread).
LONG NTAPI IslandDndCallback(ULONGLONG, ULONG, void*, void*, const void*,
                             ULONG) {
    if (HWND island = g_islandWnd) {
        PostMessage(island, WM_APP_STATUS, kStatusDoNotDisturb, 0);
    }
    return 0;
}

void Island::StartStatusWatch() {
    // A notch without the bell: told by the one with it (see WM_APP_STATUS).
    if (m_notchBase) {
        m_microphoneGlyph = MicrophoneGlyph();
        return;
    }
    m_statusEvents = new StatusEvents(m_hwnd);

    // The volume, the microphone, and the default devices changing.
    if (m_devices) {
        m_devices->RegisterEndpointNotificationCallback(m_statusEvents);
        WatchVolume();
        WatchMicrophone();
        m_microphoneGlyph = MicrophoneGlyph();
    }

    // The network.
    winrt::com_ptr<IConnectionPointContainer> container;
    if (m_network &&
        SUCCEEDED(m_network->QueryInterface(kIID_IConnectionPointContainer,
                                            container.put_void())) &&
        SUCCEEDED(container->FindConnectionPoint(
            kIID_INetworkListManagerEvents, m_networkPoint.put())) &&
        FAILED(m_networkPoint->Advise(
            static_cast<INetworkListManagerEvents*>(m_statusEvents),
            &m_networkCookie))) {
        m_networkPoint = nullptr;
        m_networkCookie = 0;
    }
    if (m_wlan &&
        WlanRegisterNotification(
            m_wlan, WLAN_NOTIFICATION_SOURCE_MSM | WLAN_NOTIFICATION_SOURCE_ACM,
            TRUE, IslandWlanCallback, m_hwnd, nullptr,
            nullptr) == ERROR_SUCCESS) {
        m_wlanWatched = true;
    }

    // The battery, if there's one.
    if (!BatteryGlyph().empty()) {
        m_powerNotifications[0] = RegisterPowerSettingNotification(
            m_hwnd, &kGUID_BatteryPercentageRemaining,
            DEVICE_NOTIFY_WINDOW_HANDLE);
        m_powerNotifications[1] = RegisterPowerSettingNotification(
            m_hwnd, &kGUID_AcDcPowerSource, DEVICE_NOTIFY_WINDOW_HANDLE);
    }
    // The energy saver (which the Liquid Glass gives way to), told at once.
    m_powerNotifications[2] = RegisterPowerSettingNotification(
        m_hwnd, &kGUID_PowerSavingStatus, DEVICE_NOTIFY_WINDOW_HANDLE);

    // "Do not disturb".
    using Subscribe_t = LONG(NTAPI*)(void**, ULONGLONG, ULONG, void*, void*,
                                     const void*, ULONG, ULONG);
    using Query_t = LONG(NTAPI*)(const ULONGLONG*, const void*, const void*,
                                 ULONG*, void*, ULONG*);
    HMODULE ntdll = GetModuleHandle(L"ntdll.dll");
    auto subscribe = (Subscribe_t)GetProcAddress(
        ntdll, "RtlSubscribeWnfStateChangeNotification");
    auto query = (Query_t)GetProcAddress(ntdll, "NtQueryWnfStateData");
    if (subscribe && query) {
        ULONG changeStamp = 0;
        BYTE buffer[16];
        ULONG size = sizeof(buffer);
        query(&kWnfQuietHoursState, nullptr, nullptr, &changeStamp, buffer,
              &size);
        if (subscribe(&m_dndSubscription, kWnfQuietHoursState, changeStamp,
                      (void*)IslandDndCallback, nullptr, nullptr, 0, 0) < 0) {
            m_dndSubscription = nullptr;
        }
    }
}

// The default output device's volume (again when the device changes).
void Island::WatchVolume() {
    if (!m_statusEvents) {
        return;
    }
    if (m_watchedVolume) {
        m_watchedVolume->UnregisterControlChangeNotify(m_statusEvents);
        m_watchedVolume = nullptr;
    }
    if (auto volume = GetVolume(nullptr);
        volume && SUCCEEDED(volume->RegisterControlChangeNotify(m_statusEvents))) {
        m_watchedVolume = volume;
    }
}

void Island::StopStatusWatch() {
    if (m_dndSubscription) {
        using Unsubscribe_t = LONG(NTAPI*)(void*);
        if (auto unsubscribe = (Unsubscribe_t)GetProcAddress(
                GetModuleHandle(L"ntdll.dll"),
                "RtlUnsubscribeWnfStateChangeNotification")) {
            unsubscribe(m_dndSubscription);
        }
        m_dndSubscription = nullptr;
    }
    for (HPOWERNOTIFY& notification : m_powerNotifications) {
        if (notification) {
            UnregisterPowerSettingNotification(notification);
            notification = nullptr;
        }
    }
    if (m_wlanWatched && m_wlan) {
        WlanRegisterNotification(m_wlan, WLAN_NOTIFICATION_SOURCE_NONE, TRUE,
                                 nullptr, nullptr, nullptr, nullptr);
        m_wlanWatched = false;
    }
    if (m_networkPoint) {
        m_networkPoint->Unadvise(m_networkCookie);
        m_networkPoint = nullptr;
        m_networkCookie = 0;
    }
    if (!m_statusEvents) {
        return;
    }
    if (m_watchedVolume) {
        m_watchedVolume->UnregisterControlChangeNotify(m_statusEvents);
        m_watchedVolume = nullptr;
    }
    if (m_watchedMicrophone) {
        m_watchedMicrophone->UnregisterControlChangeNotify(m_statusEvents);
        m_watchedMicrophone = nullptr;
    }
    if (m_devices) {
        m_devices->UnregisterEndpointNotificationCallback(m_statusEvents);
    }
    m_statusEvents->Detach();
    m_statusEvents->Release();
    m_statusEvents = nullptr;
}

// What's playing, as the pill shows it: its title, or its app's name.
std::wstring Island::MediaTitleText() const {
    if (!m_mediaShown.title.empty()) {
        return m_mediaShown.title;
    }
    return m_mediaShown.appId.empty() ? std::wstring()
                                      : AppDisplayName(m_mediaShown.appId);
}

// Whether the line sung now shows in the song title's place (its option,
// the title shown on the pill: on any of its notches, the line's item on
// another one steps aside too), rather than as its own item; between lines
// the title comes back.
bool Island::LyricsInTitle() const {
    return g_settings.islandLyrics && g_settings.islandLyricsInTitle &&
           !m_lyricsLine.empty() && !m_vertical &&
           MediaPartWanted(kAppMediaTitle);
}

// The cover of what's playing, decoded once per cover.
ID2D1Bitmap* Island::MediaCover() {
    if (m_mediaCoverSerial == m_mediaShown.thumbnailSerial) {
        return m_mediaCover.get();
    }
    m_mediaCoverSerial = m_mediaShown.thumbnailSerial;
    if (m_mediaCover && m_mediaTitleRoom > 0.01 && m_mediaAmount > 0.01) {
        m_mediaCoverBefore = m_mediaCover;
        m_mediaCoverSwap = 1;
        m_mediaCoverSwapVelocity = 0;
        StartAnimating();
    }
    m_mediaCover = nullptr;
    m_mediaCoverRgb = -1;
    if (m_mediaShown.thumbnail.empty() || !m_wic || !m_canvas.target) {
        return nullptr;
    }
    winrt::com_ptr<IStream> stream;
    stream.attach(SHCreateMemStream(m_mediaShown.thumbnail.data(),
                                    (UINT)m_mediaShown.thumbnail.size()));
    winrt::com_ptr<IWICBitmapDecoder> decoder;
    winrt::com_ptr<IWICBitmapFrameDecode> frame;
    winrt::com_ptr<IWICFormatConverter> converter;
    if (stream &&
        SUCCEEDED(m_wic->CreateDecoderFromStream(
            stream.get(), nullptr, WICDecodeMetadataCacheOnLoad,
            decoder.put())) &&
        SUCCEEDED(decoder->GetFrame(0, frame.put())) &&
        SUCCEEDED(m_wic->CreateFormatConverter(converter.put())) &&
        SUCCEEDED(converter->Initialize(
            frame.get(), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0,
            WICBitmapPaletteTypeMedianCut))) {
        m_canvas.target->CreateBitmapFromWicBitmap(converter.get(), nullptr,
                                                   m_mediaCover.put());
        // (As bright as it goes, its hue kept: a dark cover's bars still
        // show on the pill.)
        const D2D1_COLOR_F color = AverageColor(m_wic.get(), converter.get());
        const float most = std::max({color.r, color.g, color.b});
        if (color.a > 0 && most > 0.02f) {
            m_mediaCoverRgb = (int)std::lround(color.r / most * 255) << 16 |
                              (int)std::lround(color.g / most * 255) << 8 |
                              (int)std::lround(color.b / most * 255);
        }
    }
    return m_mediaCover.get();
}

// The playing song's cover's color on the island drawn now (see
// kVisualizerCover), -1 without a cover.
int MediaCoverRgb() {
    if (!g_island) {
        return -1;
    }
    g_island->MediaCover();
    return g_island->m_mediaCoverRgb;
}

