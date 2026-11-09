#pragma once
// Changed in the island (its thread): kept, and shown right away.
void SetIslandPref(IslandPref pref, int value) {
    // What every island shares is the main island's, also when changed on
    // one showing a profile of its own (they all take it again).
    if (Island* main = MainIsland();
        IsSharedPref(pref) && OwnLookShown() && main && main != g_island) {
        g_island->SaveOwnProfile();
        {
            IslandScope scope(main);
            SetIslandPref(pref, value);
        }
        ForEachIsland([](Island& island) { island.RefreshLook(); });
        return;
    }
    for (const auto& info : kIslandPrefs) {
        if (info.pref == pref && info.key) {
            StoreProfileInt(info.key, value);
        }
    }
    const bool on = value != 0;
    switch (pref) {
        case IslandPref::Animations: {
            AnimationChoice choice = g_animationChoice;
            choice.on = on;
            SetAnimationChoice(choice, true);
            StartIslandAnimation();
            return;
        }
        case IslandPref::FollowMouse:
            g_settings.islandFollowMouse = on;
            if (g_island) {
                ForEachIsland(
                    [&](Island& island) { island.OnMonitorChoiceChanged(); });
            }
            return;
        case IslandPref::Bar:
            if (g_settings.islandBar != (value == 1) ||
                g_settings.islandReserve != (value == 2)) {
                g_settings.islandBar = value == 1;
                g_settings.islandReserve = value == 2;
                // Another kind of window: the island starts again.
                RestartIslandSoon();
            }
            return;
        case IslandPref::Theme:
            // (The value itself: on an island showing its own look, the
            // stored theme and colors are the main island's.)
            g_themeIndex = std::clamp(value, 0, (int)ThemeId::Count - 1);
            ApplyThemeIndex(true);
            StartIslandAnimation();
            break;
        case IslandPref::Tray:
            g_settings.islandTray = on;
            // Explorer asks the apps for their icons if it hasn't yet.
            if (on && g_island) {
                g_island->SayHello();
            }
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
            g_settings.islandMediaButtons = on;
            g_mediaButtonsInPill = on;
            break;
        case IslandPref::MediaTitle:
            g_mediaTitleInPill = on;
            break;
        case IslandPref::Banner:
            g_settings.islandBanner = on;
            g_bannerEnabled = on;
            break;
        case IslandPref::BannerMinimized:
            g_settings.islandBannerMinimized = on;
            g_bannerWhenMinimized = on;
            break;
        case IslandPref::HideWindowsBanners:
            g_settings.islandHideWindowsBanners = on;
            break;
        case IslandPref::WebPictures:
            g_settings.islandWebPictures = on;
            break;
        case IslandPref::ClockSeconds:
            g_settings.clockSeconds = on;
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
            // (The monitors' lists read so far are old: read again.)
            if (g_externalBrightness.exchange(on) != on && g_island) {
                ForEachIsland(
                    [](Island& island) { island.ReloadBrightness(); });
            }
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
            if (g_settings.islandWindowColor != on) {
                g_settings.islandWindowColor = on;
                ApplyThemeIndex(true);
                // (Read now: no island may have followed it so far.)
                if (Island* main = MainIsland(); on && main) {
                    main->ReadWindowColor();
                }
            }
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
        case IslandPref::Notches:
            if (g_settings.islandNotches != value) {
                g_settings.islandNotches = std::clamp(value, 0, kMaxNotches);
                // Islands made or gone: they start again.
                RestartIslandSoon();
            }
            return;
        case IslandPref::PillNotches:
            if (g_settings.islandPillNotches != value) {
                g_settings.islandPillNotches =
                    std::clamp(value, 0, kMaxNotches);
                RestartIslandSoon();
            }
            return;
        case IslandPref::NotchOpen:
            g_settings.islandNotchOpen = std::clamp(value, 0, 3);
            break;
        case IslandPref::NotchEach:
            g_settings.islandNotchEach = on;
            if (!on && g_island) {
                ForEachIsland([](Island& island) { island.MatchNotchBase(); });
            }
            break;
        case IslandPref::GroupApps:
            if (g_settings.islandGroupApps != on) {
                g_settings.islandGroupApps = on;
                if (g_island) {
                    ForEachIsland(
                        [](Island& island) { island.RegroupUnread(); });
                }
            }
            break;
        case IslandPref::MonthNumber:
            g_settings.clockMonthNumber = on;
            break;
        case IslandPref::AllMonitors:
            if (g_settings.islandAllMonitors != on) {
                g_settings.islandAllMonitors = on;
                // Islands made or gone: they start again.
                RestartIslandSoon();
            }
            return;
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
        case IslandPref::OwnNotifications:
            g_settings.islandOwnsNotifications = on;
            g_dndSwitchFailed = false;
            g_dndSeenOn = false;
            UpdateWindowsQuiet(true);
            if (g_island) {
                ForEachIsland(
                    [&](Island& island) { island.OnDoNotDisturbChanged(); });
            }
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
    }
    if (g_island && !g_applyingProfile) {
        ForEachIsland([&](Island& island) { island.OnPreferencesChanged(); });
    }
}

// Another order of the pill's items, chosen in its settings: the items slide
// to their new places (kept with SavePillOrder: an own look a moment after
// the last step of a drag, not at every step).
void SetPillOrder(const std::vector<PillGroup>& order) {
    if (order == g_pillOrder) {
        return;
    }
    g_pillOrder = order;
    SavePillOrder();
    if (g_island) {
        ForEachIsland([&](Island& island) { island.OnOrderChanged(); });
    }
}

////////////////////////////////////////////////////////////////////////////////
// Profiles (see IslandProfile).

