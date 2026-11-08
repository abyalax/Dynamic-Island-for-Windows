// The last lines the island's process logged (with their time), kept for
// the settings' Logs page whether Windhawk's logging is on or not (see
// IslandLog). Only there (g_keepLog, set in WhTool_ModInit): explorer's
// lines go to Windhawk's log alone.
constexpr size_t kKeptLogLines = 300;
SRWLOCK g_keptLogLock = SRWLOCK_INIT;
std::wstring g_keptLog[kKeptLogLines];
size_t g_keptLogNext = 0;
size_t g_keptLogCount = 0;
bool g_keepLog = false;

void KeptLog(int line, PCSTR function, PCWSTR format, ...) {
    WCHAR text[1024];
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(text, ARRAYSIZE(text), _TRUNCATE, format, args);
    va_end(args);
    SYSTEMTIME now;
    GetLocalTime(&now);
    WCHAR head[160];
    swprintf_s(head, L"%02u:%02u:%02u.%03u [%d:%S]: ", now.wHour, now.wMinute,
               now.wSecond, now.wMilliseconds, line, function);
    AcquireSRWLockExclusive(&g_keptLogLock);
    g_keptLog[g_keptLogNext] = std::wstring(head) + text;
    g_keptLogNext = (g_keptLogNext + 1) % kKeptLogLines;
    g_keptLogCount = std::min(g_keptLogCount + 1, kKeptLogLines);
    ReleaseSRWLockExclusive(&g_keptLogLock);
}

// The lines kept, oldest first.
std::wstring KeptLogText() {
    std::wstring text;
    AcquireSRWLockShared(&g_keptLogLock);
    for (size_t i = 0; i < g_keptLogCount; i++) {
        text += g_keptLog[(g_keptLogNext + kKeptLogLines - g_keptLogCount + i) %
                          kKeptLogLines] +
                L"\r\n";
    }
    ReleaseSRWLockShared(&g_keptLogLock);
    return text;
}

// A line to Windhawk's log (Wh_Log), and in the island's process kept for
// its Logs page too (see KeptLog).
#define IslandLog(message, ...)                                        \
    do {                                                               \
        Wh_Log(message, ##__VA_ARGS__);                                \
        if (g_keepLog) {                                               \
            KeptLog(__LINE__, __FUNCTION__, message, ##__VA_ARGS__);   \
        }                                                              \
    } while (0)

using namespace winrt::Windows::UI::Xaml;

enum class AttentionMode { Icon, Dock, None };

struct {
    // What the taskbar is: the mod's dock, or the Windows taskbar (all of it,
    // or only its apps).
    enum class TaskbarMode {
        Dock,
        Windows,
        WindowsApps,
        WindowsTransparent,
    } taskbarMode;
    bool autoHide;
    double cornerRadius;
    double dockIconSize;
    double dockIconGap;
    double bottomGap;
    bool dockBackground;
    double backgroundOpacity;
    int showDuration;
    int hideDuration;
    int showDelay;
    int hideDelay;
    // When the taskbar hides (hiding.hideWhen): 0 with the mouse away (as
    // Windows does), 1 only while an app's window is maximized on its
    // monitor, 2 while one is over it (see TaskbarWayWatched).
    int hideWhen;
    // The strip over the bottom edge that brings the taskbar up, in DIPs, on
    // every monitor; 0 automatic (see EdgeStripPixels).
    int revealHeight;
    bool fullscreenGuard;
    bool hideTray;
    // The Windows taskbar unchanged, but without its tray (the island's).
    bool windowsHideTray;
    bool showIsland;
    bool islandBar;
    // The pill keeping its room at its edge (windows maximize beside it).
    bool islandReserve;
    // What the island shows.
    bool islandTray;
    bool islandTrayOpen;
    // How the tray icons that aren't pinned show from the arrow: 0 opening
    // sideways in the pill, 1 a grid of icons under it (like Windows'), 2 a
    // list with their names (see TrayListPanel).
    int islandTrayStyle;
    // Files dragged out of the folders panel: 0 asked, 1 copied, 2 moved.
    int islandFolderDrag;
    bool islandNetwork;
    bool islandVolume;
    bool islandBattery;
    bool islandBell;
    bool islandMedia;
    bool islandMediaButtons;
    bool islandBanner;
    bool islandBannerMinimized;
    bool islandHideWindowsBanners;
    // Pictures of notifications that are on the web (like WhatsApp's contact
    // photos) are downloaded: off unless chosen, since it goes online.
    bool islandWebPictures;
    // A click on the Snipping Tool's notifications shows the newest
    // screenshot in Explorer instead of opening the Snipping Tool.
    bool islandScreenshotFolder;
    // The wheel on what's playing on the pill changes the song (not the
    // volume).
    bool islandMediaWheel;
    // A title too long for the pill keeps scrolling while it plays.
    bool islandMediaMarquee;
    // The island's clock: seconds (-1 like the Windows clock), the day of the
    // week, the day and month, the year.
    int clockSeconds;
    bool clockWeekday;
    bool clockDate;
    bool clockYear;
    // Moving the pill by dragging it, and standing it upright on the sides.
    bool islandFreeDrag;
    // Minimized to its line while the mouse is away (at the edges).
    bool islandAutoHide;
    // An island on each monitor (the chosen one's is the main one).
    bool islandAllMonitors;
    // The volume's and the battery's percentage beside their icons.
    bool islandVolumePercent;
    bool islandBatteryPercent;
    // The screen's brightness on the pill.
    bool islandBrightness;
    // A bubble with what was copied (see Island::OnClipboardChanged).
    bool islandClipboard;
    // The keyboard's input method on the pill (see
    // Island::UpdateInputMethodWatch), the apps using the microphone, the
    // camera or the location (see PrivacyThreadProc), and for USB drives an
    // icon to open or eject them and a notification when one is plugged in
    // (see Island::OnDeviceChange).
    bool islandInputMethod;
    bool islandPrivacy;
    bool islandUsbDrives;
    bool islandUsbNotify;
    // The virtual desktops on the pill (see DesktopsThreadProc), and the
    // control center's buttons put on it (a bit each, see kPillActions).
    bool islandDesktops;
    unsigned pillActions;
    // What komorebi's start, stop and reload buttons take along (see
    // KomorebiCommands).
    bool komorebiWhkd;
    bool komorebiMasir;
    // The island's background taking the color of the app in front (see
    // WindowColor).
    bool islandWindowColor;
    // Windows' Win+N and Win+A opening the island's notifications and
    // control center instead (see ShortcutKeyProc).
    bool islandWinN;
    bool islandWinA;
    // Win+J minimizes the islands to their line and brings them back.
    bool islandMinimizeKey;
    // A bubble when Caps Lock is turned on or off (the main island's).
    bool islandCapsLock;
    // Items added in 2.1 (#6157, #6156): the network's speed, other cities'
    // times (Windows' additional clocks), the time until the next event and
    // the Recycle Bin.
    bool islandNetSpeed;
    bool islandWorldClocks;
    bool islandCountdown;
    bool islandCountdownEvents;
    bool islandCountdownAlarms;
    bool islandCountdownTimer;
    bool islandCountdownPomodoro;
    bool islandDownloadFill;
    bool islandChargeFill;
    // The fill's colors, for downloads and other apps, file copies and the
    // battery charging: 0 green, 1 blue, 2 Windows' accent, 3 the user's own
    // (its own 0xRRGGBB).
    int islandFillColor;
    int islandFillOwnColor;
    int islandFillCopyColor;
    int islandFillCopyOwnColor;
    int islandFillChargeColor;
    int islandFillChargeOwnColor;
    // How the fill moves: 0 simple, 1 a liquid wave, 2 a glow with
    // particles (see Island::DrawFill).
    int islandFillStyle;
    // Song lyrics (from lrclib.net, see LyricsThreadProc): on, their size on
    // the pill (0 small, 1 medium, 2 large), their color (0 the text's, 1
    // Windows' accent, 2 their own, 0xRRGGBB); the line sung in the song
    // title's place (see Island::LyricsInTitle).
    bool islandLyrics;
    int islandLyricsSize;
    int islandLyricsColor;
    int islandLyricsOwnColor;
    bool islandLyricsInTitle;
    // The longest a line gets on the pill (see kLyricsMaxWidths).
    int islandLyricsWidth;
    // The Recycle Bin on the pill, from how much in it (see
    // kRecycleMinMegabytes: 0 always).
    bool islandRecycleBin;
    int islandRecycleMinSize;
    // Short notices from the pill: the charger plugged in, low battery, a
    // Bluetooth audio device connected, "Do not disturb" on or off (#6158).
    bool islandStatusNotices;
    // The apps with notifications folded into an arrow on the pill, like the
    // tray: out a moment when one comes, and under the mouse (#6129, #6204).
    bool islandFoldApps;
    // A timer, a stopwatch and Pomodoro on the pill (see TimerPanel).
    bool islandTimer;
    // Bars moving with the sound while something plays (see
    // Island::StepVisualizer), and their color (0 the icons', 1 Windows'
    // accent, 2 a rainbow, 3 their own, 0xRRGGBB).
    bool islandVisualizer;
    int islandVisualizerColor;
    int islandVisualizerOwnColor;
    // Its bars sway smoothly like the iPhone's; off, they jump with the beat.
    bool islandVisualizerSmooth;
    // Once per version, a notice from the pill opens What's new (see
    // Island::StartWhatsNewNotice).
    bool islandWhatsNewNotice;
    // Folders and places, and the wallpapers, on the pill (and how a
    // wallpaper comes in, a WallpaperTransition).
    bool islandFolders;
    bool islandWallpaper;
    int wallpaperTransition;
    // Windows' blur behind the bar and the Fixed style's strip (see
    // Island::UpdateGlass).
    bool islandFrosted;
    // The island and its panels as glass (see GlassLayer).
    bool islandLiquidGlass;
    bool islandTips;
    // The soft gray shape behind the pill's item under the mouse.
    bool islandHoverHighlight;
    // The arrow at the pill's end that minimizes it to its line.
    bool islandMinimizeButton;
    // The bar's items at the screen's corners, the clock in the middle.
    bool islandBarCorners;
    // The bar's edge: 0 the top, 1 the left, 2 the right (upright there,
    // see Island::m_vertical); its corners facing the screen rounded (an
    // index of kBarInnerCornerRadii, 0 none).
    int islandBarSide;
    int islandBarInnerCorners;
    // The banner's size, width and corners (indexes of kBannerSizes,
    // kBannerWidths and kBannerCornerShares), and where it shows: 0 out of
    // the pill (where the bell is), 1-9 a point of the screen (its top left
    // to its bottom right, see Island::BannerPlaced).
    int islandBannerSize;
    int islandBannerWidth;
    int islandBannerCorners;
    int islandBannerPlace;
    // How many files the shelf keeps (an index of kShelfLimits).
    int islandShelfLimit;
    // The glass translucent: a lighter tint and livelier colors.
    bool islandGlassTranslucent;
    // The date's order on the clock: 0 the Windows regional format's, 1 the
    // day first, 2 the month first, 3 the year first; and the month as a
    // number ("9/10").
    int clockDateOrder;
    bool clockMonthNumber;
    // The time's format: 0 the Windows regional format's, 1 24 hours, 2 12
    // hours with AM and PM (see FormatClockTime).
    int clockFormat;
    // 0 Windows' measurement system, 1 Celsius, 2 Fahrenheit.
    int weatherUnit;
    // The icon pack (see IconPackId), and its pictures' and emojis' colors
    // (pictures of one's own, see g_iconFiles, too): 0 only their shapes, in
    // the icons' colors (like the font's), 1 their own colors.
    int islandIconPack;
    int islandIconColors;
    // How long the mouse is away before the island hides as its line, in
    // tenths of a second (see islandAutoHide).
    int islandAutoHideDelay;
    // When it hides (islandAutoHide, the Island style): 0 with the mouse
    // away, 1 only while a window is maximized on its monitor, 2 only while
    // a window is over it (see Island::WindowInTheWay).
    int islandHideWhen;
    // Hidden by the auto-hide (the Island and Fixed styles), or the Bar
    // hidden by Win+J: a small pill with the time instead of the line (see
    // Island::HiddenClockWanted).
    bool islandHideClock;
    // The minimized line fading away after a while (OLED screens): an index
    // of kLineFadeSeconds (0 never).
    int islandLineFade;
    // What opens the minimized line (and the hidden Bar's clock): the
    // mouse reaching it (0), or a click on it (1: it only grows under
    // the mouse).
    int islandLineClick;
    // The Fixed style as notches against the top edge (0: the pill as it
    // is; 1 in the middle, 2 at the screen's corners, or 3), how round
    // their curves are (in percent of the usual), and whether each one hides
    // on its own (otherwise they all do together).
    int islandNotches;
    // The Island style's own (0 by default), the same way, without keeping
    // a strip (see NotchCountWanted).
    int islandPillNotches;
    // The Bar style without its background: only the items (see
    // Island::Render), the frosted glass only lightly tinted.
    bool islandBarClear;
    // The AI agents' usage limits on the pill (see AiUsagePanel), and which
    // agents.
    bool islandAiUsage;
    bool islandAiClaude;
    bool islandAiCodex;
    // The app in focus with its window's title too (see
    // Island::FocusedText).
    bool islandFocusedTitle;
    // The weather's details in its panel as small cards, like a phone's
    // (#6129, see WeatherPanel::Draw), instead of two lines.
    bool islandWeatherCards;
    // The calendar with today big at its left, with alarm, timer and
    // agendas buttons and the day's events beside it (#6165, see
    // CalendarPanel::DrawBigDay).
    bool islandCalendarBigDay;
    // The user's own commands (#6165 #6161): shortcuts to a program, a
    // file, a folder, a link or a command line, run from their panel, or
    // from the pill for the ones chosen as buttons (see CommandsPanel).
    bool islandCommands;
    // The island's Do not disturb silences calls, alarms and reminders
    // too (they still show on the pill; see Island::OnToastsChanged).
    bool islandDndSilent;
    // A browser's download standing still for 30 s shows as paused (see
    // Island::UpdateFill): browsers never tell a pause or a failure.
    bool islandDownloadStalled;
    // Files show Windows' thumbnails (pictures, videos, documents) in the
    // folders panel and on the shelf, made on a thread (see ThumbnailThreadProc).
    bool islandFolderThumbnails;
    // The shelf: files dropped on the pill kept there (their paths) to be
    // dragged out later into another app (see ShelfPanel).
    bool islandShelf;
    // The dragon's eyes on the pill: they follow the mouse, blink and glance
    // around on their own (see Island::StepEyes), in the colors of an index of
    // kEyeThemes and the shape of an index of kEyeShapes.
    bool islandEyes;
    int islandEyeTheme;
    int islandEyeShape;
    // While a timer, Pomodoro or the stopwatch runs, the minimized line is a
    // small pill with the ring and the time of the one ending first (with
    // islandHideClock, the time beside the clock; see TimerLineWanted).
    bool islandTimerLine;
    // The external monitors' brightness is read and set over DDC/CI (their
    // sliders in the control center and on the pill). Some docks and MST hubs
    // make a monitor flicker, or reset, at every call: off, nothing goes over
    // the cable (the built-in screen keeps WMI, and a screen with HDR on its
    // SDR content brightness). Shared by all the islands (see
    // g_externalBrightness).
    bool islandExternalBrightness;
    // The Liquid Glass with the light and shadow of macOS 27: a sheen, a
    // beveled edge, light along its bottom, a soft shadow around it, and the
    // panels' cards as glass too (see GlassLayer and GlassFill).
    bool islandGlassLight;
    int islandNotchCurve;
    bool islandNotchEach;
    // Whose line opens the island (0 every one's, else 1 + the notch).
    int islandNotchOpen;
    // One icon on the pill for the notifications of one app, even when they
    // come under IDs of their own (see NotificationAppKey).
    bool islandGroupApps;
    // The apps with notifications on the pill (their icons and counts).
    bool islandApps;
    // komorebi's layout on the pill (its menu changes it), and the app in
    // focus on the island's monitor.
    bool komorebiLayout;
    bool islandFocusedApp;
    // komorebi's workspaces without windows left out of its dots (the one
    // shown always there).
    bool komorebiHideEmpty;
    bool islandVerticalSides;
    // The clock on the pill (on by default), kept in its middle (or the
    // bar's), and the power button (sleep, restart, shut down) on it.
    bool islandClock;
    bool islandClockCenter;
    bool islandPower;
    // Panels opening out of the pill as wide as it, when it's wider than
    // them (otherwise they drop out of the clicked item, at their own width).
    bool islandPanelsFollow;
    // A button switching Windows between light and dark mode, and how much
    // the processor, the memory and the video memory are used, on the pill.
    bool islandThemeButton;
    bool usageCpu;
    bool usageRam;
    bool usageVram;
    // A button muting the microphone, on the pill.
    bool islandMicrophone;
    // Public holidays in the calendar (goes online, see Holiday).
    bool calendarHolidays;
    // Notes, tasks and keyboard shortcuts, on the pill.
    bool islandNotes;
    bool islandTasks;
    bool islandShortcuts;
    // The weather on the pill (goes online, see WeatherThreadProc).
    bool islandWeather;
    // Updates (winget and Windows Update) on the pill.
    bool islandUpdates;
    // A button on the pill listing the settings' profiles.
    bool islandProfiles;
    // The island handles notifications itself (see UpdateWindowsQuiet).
    bool islandOwnsNotifications;
    // The island's size (its text, icons and width), in percent.
    int islandSize;
    // Its icons' own size (their room too, not the texts nor the island's
    // height), and its texts' own size, in percent.
    int islandIconSize;
    int islandTextSize;
    // The minimized line's opacity (percent; at 0 it's unseen, and the mouse
    // still brings the island back), the length of the strip along the edge
    // that brings it back (px at 100%), the panels' size (percent of the
    // screen's scaling) and the widest the playing title gets on the pill
    // (px at 100%).
    int islandLineOpacity;
    int islandActivationWidth;
    int islandPanelSize;
    int islandMediaTitleLimit;
    // The pill's height, how round its corners are and the room at its
    // ends, in % of the usual (see kPillHeightPctMin).
    int islandPillHeight;
    int islandPillCorners;
    int islandPillEnds;
    // How much the thin edge around the island, its panels, banners and
    // bubbles shows (glass's bright one too), in %; the minimized line keeps
    // its own.
    int islandEdge;
    // More blur on the glass than Windows' own, in % (0: Windows' own).
    int islandGlassBlur;
    // How many columns the tray's grid has (see TrayListPanel).
    int islandTrayColumns;
    // How much the visualizer's bars move, in % (see StepVisualizer).
    int islandVizSensitivity;
    // The room between the island and the screen's edges (px at 100%).
    int islandGap;
    // On the monitor where the mouse is (several monitors).
    bool islandFollowMouse;
    double magnification;
    double magnificationRange;
    // The magnification, the bounces and hideWhen on the Windows taskbar
    // with only its apps too (see AppsTaskbarEffects).
    bool onlyAppsEffects;
    // The widest the dock gets, in % of its monitor (0: no limit): a box
    // past that, scrolled by the wheel (see g_dockScroll).
    int dockMaxWidth;
    bool bounceOnLaunch;
    bool keepOpenOnLaunch;
    int keepOpenDuration;
    bool dotIndicator;
    AttentionMode attentionMode;
} g_settings;

// The island's colors: its background, and its foreground (texts, icons and
// the light fills over the background, drawn with some transparency).
struct IslandTheme {
    float background[3] = {0, 0, 0};
    // How opaque the background is (glass lets the windows show through).
    float backgroundAlpha = 1;
    float foreground[3] = {1, 1, 1};
    // The pill's icons (the texts' color, but in the custom colors).
    float icon[3] = {1, 1, 1};
    // Dark text on a light background needs more of it to read well (0 to 1,
    // in between while the colors change).
    float light = 0;
};
// The colors shown (blending from one theme to another while they change).
IslandTheme g_theme;
// The colors chosen, and the ones a change started from.
IslandTheme g_themeTarget, g_themeFrom;
// When the colors started changing (NowSeconds), or 0.
double g_themeChangeStart;
// Whether the colors changed on the island's last frame (the panels draw
// again too).
bool g_themeChanging;

// The foreground with an opacity.
D2D1_COLOR_F Fg(float alpha) {
    // On the light theme, texts and icons (drawn at 0.35 and up) get darker;
    // the light fills under them stay as they are.
    if (g_theme.light > 0 && alpha >= 0.35f) {
        alpha += (std::min(1.0f, 0.55f + alpha * 0.5f) - alpha) * g_theme.light;
    }
    return {g_theme.foreground[0], g_theme.foreground[1],
            g_theme.foreground[2], alpha};
}

// How opaque the backgrounds are, in percent (the island's settings): what's
// behind shows through below 100. Texts and icons stay as they are.
constexpr int kIslandOpacityMin = 30;
constexpr int kIslandOpacityMax = 100;
constexpr int kIslandOpacityDefault = 100;
constexpr int kIslandOpacityStep = 5;
int g_islandOpacity = kIslandOpacityDefault;

// The background with an opacity (times the theme's own, and the chosen
// one).
D2D1_COLOR_F Bg(float alpha) {
    return {g_theme.background[0], g_theme.background[1],
            g_theme.background[2],
            alpha * g_theme.backgroundAlpha * g_islandOpacity / 100.0f};
}

// The background with only this opacity (not the chosen one): the bubbles
// that ask or tell something (their texts over what's behind didn't read).
D2D1_COLOR_F BgSolid(float alpha) {
    return {g_theme.background[0], g_theme.background[1],
            g_theme.background[2], alpha};
}

// The pill's icons with an opacity (like Fg, in their own color).
D2D1_COLOR_F FgIcon(float alpha) {
    const D2D1_COLOR_F text = Fg(alpha);
    return {g_theme.icon[0], g_theme.icon[1], g_theme.icon[2], text.a};
}

// The themes, in the order the island's settings show them.
// Auto: black in dark mode, light in light mode (Windows' apps mode);
// inverted, the other way around. Custom: the colors chosen (see
// g_customColors).
enum class ThemeId {
    Black,
    Graphite,
    Light,
    Accent,
    Glass,
    Auto,
    AutoInverted,
    Custom,
    Count
};
int g_themeIndex;

// The color of the app in front (0xRRGGBB, see WindowColor), or -1: the
// island's background takes it when chosen (islandWindowColor).
int g_windowColor = -1;
int WindowColor(HWND hWnd);

// The island in the app in front's colors now (its highlight follows the
// icons' tint, like with your own colors).
bool UsesWindowColor() {
    return g_settings.islandWindowColor && g_windowColor >= 0;
}

// The custom colors (0xRRGGBB): the background, the texts and the pill's
// icons, chosen in the island's settings with Windows' color dialog.
enum CustomColor { kColorBackground, kColorText, kColorIcons, kCustomColors };
constexpr PCWSTR kCustomColorKeys[kCustomColors] = {
    L"islandColorBackground", L"islandColorText", L"islandColorIcons"};
constexpr int kCustomColorDefaults[kCustomColors] = {0x1C1C1E, 0xFFFFFF,
                                                     0x64D2FF};
// (Not one of them: the color dialogs for the pill's fill's own colors, one
// a kind, see kFillDownloads; the song lyrics', the visualizer's and an
// icon's own colors.)
constexpr int kFillColorDialog = kCustomColors;
constexpr int kLyricsColorDialog = kFillColorDialog + 3;
constexpr int kVisualizerColorDialog = kLyricsColorDialog + 1;
// (And an icon's own color: this plus its code.)
constexpr int kIconColorDialog = kVisualizerColorDialog + 1;
std::array<int, kCustomColors> g_customColors = {
    kCustomColorDefaults[0], kCustomColorDefaults[1], kCustomColorDefaults[2]};

void LoadCustomColors() {
    for (int i = 0; i < kCustomColors; i++) {
        g_customColors[i] =
            Wh_GetIntValue(kCustomColorKeys[i], kCustomColorDefaults[i]) &
            0xFFFFFF;
    }
}

// A custom color's red, green and blue, from 0 to 1.
void CustomColorParts(int color, float parts[3]) {
    parts[0] = ((color >> 16) & 0xFF) / 255.0f;
    parts[1] = ((color >> 8) & 0xFF) / 255.0f;
    parts[2] = (color & 0xFF) / 255.0f;
}

float Luminance(float r, float g, float b) {
    return 0.299f * r + 0.587f * g + 0.114f * b;
}

// How easy one color is to read on another (0xRRGGBB): the contrast ratio
// of the web's accessibility guidelines (WCAG), from 1 (the same) to 21.
float ContrastRatio(int a, int b) {
    auto relative = [](int color) {
        float parts[3];
        CustomColorParts(color, parts);
        for (float& value : parts) {
            value = value <= 0.04045f ? value / 12.92f
                                      : std::pow((value + 0.055f) / 1.055f,
                                                 2.4f);
        }
        return 0.2126f * parts[0] + 0.7152f * parts[1] + 0.0722f * parts[2];
    };
    float lighter = relative(a);
    float darker = relative(b);
    if (lighter < darker) {
        std::swap(lighter, darker);
    }
    return (lighter + 0.05f) / (darker + 0.05f);
}

// One of your own colors hard to see on the background: the texts or the
// icons (the guidelines' minimum for big texts and for icons: 3 and 2).
bool CustomColorHardToRead(int color) {
    const int background = g_customColors[kColorBackground];
    return color == kColorText
               ? ContrastRatio(g_customColors[kColorText], background) < 3
           : color == kColorIcons
               ? ContrastRatio(g_customColors[kColorIcons], background) < 2
               : false;
}

bool CustomColorsHardToRead() {
    return CustomColorHardToRead(kColorText) ||
           CustomColorHardToRead(kColorIcons);
}

// The highlight (switches and buttons on, the one chosen, a field being
// typed in): Windows' blue, or with your own colors, the icons' color made
// stronger (as vivid as it goes on a dark background, deeper on a light
// one), and pushed away from the background when they're too alike.
D2D1_COLOR_F Accent(float alpha) {
    if (g_themeIndex != (int)ThemeId::Custom && !UsesWindowColor()) {
        return {0.04f, 0.52f, 1.0f, alpha};
    }
    const float* back = g_theme.background;
    const float background = Luminance(back[0], back[1], back[2]);
    float c[3] = {g_theme.icon[0], g_theme.icon[1], g_theme.icon[2]};
    const float top = std::max({c[0], c[1], c[2]});
    for (float& value : c) {
        value = background >= 0.5f ? value * 0.75f
                : top > 0.01f      ? value / top
                                   : 1.0f;
    }
    if (std::fabs(Luminance(c[0], c[1], c[2]) - background) < 0.3f) {
        const float away = background < 0.5f ? 1.0f : 0.0f;
        for (float& value : c) {
            value += (away - value) * 0.5f;
        }
    }
    return {c[0], c[1], c[2], alpha};
}

// The highlight a bit lighter under the mouse (`lit` from 0 to 1).
D2D1_COLOR_F AccentLit(float alpha, float lit) {
    D2D1_COLOR_F color = Accent(alpha);
    color.r += (1 - color.r) * 0.12f * lit;
    color.g += (1 - color.g) * 0.1f * lit;
    return color;
}

// What's written on the highlight: white, or dark on a light one.
D2D1_COLOR_F AccentInk(float alpha) {
    const D2D1_COLOR_F accent = Accent(1);
    const float ink =
        Luminance(accent.r, accent.g, accent.b) > 0.62f ? 0.08f : 1.0f;
    return {ink, ink, ink, alpha};
}

// A switch's knob: white, turning to the highlight's ink as it goes on.
D2D1_COLOR_F KnobColor(float on) {
    const D2D1_COLOR_F ink = AccentInk(1);
    const float t = std::clamp(on, 0.0f, 1.0f);
    return {1 + (ink.r - 1) * t, 1 + (ink.g - 1) * t, 1 + (ink.b - 1) * t, 1};
}

double NowSeconds();

// How the mod animates (see LoadAnimationSettings), in both processes: the
// level (0 all, 1 partial: no bounces nor decorations, 2 none), the speed in
// percent, and the frame rate limit (0 the screen's, or frames a second).
std::atomic<int> g_animationLevel{0};
std::atomic<int> g_animationSpeed{100};
std::atomic<int> g_frameRateLimit{0};

int AnimationLevel() {
    return g_animationLevel.load(std::memory_order_relaxed);
}

// A real time step, as the animations take it (faster or slower).
double AnimationDt(double dt) {
    return dt * g_animationSpeed.load(std::memory_order_relaxed) / 100.0;
}

// How long a timed animation lasts at the chosen speed: 0 without
// animations, and for a decoration (a glow, a color blend, a number
// counting...) at the "partial" level too.
double AnimationSeconds(double seconds, bool decoration = false) {
    const int level = AnimationLevel();
    if (level >= 2 || (decoration && level >= 1)) {
        return 0;
    }
    return seconds * 100.0 / g_animationSpeed.load(std::memory_order_relaxed);
}

// An animation's length in milliseconds at the chosen speed (0, at once,
// without animations).
int AnimationMs(int ms) {
    return (int)std::lround(AnimationSeconds(ms / 1000.0) * 1000);
}

// The animation settings as chosen: on, level (0 all, 1 partial, 2 none),
// speed (percent) and frame rate (0 the screen's, 1 about 90, 2 about 60, 3
// about 30).
struct AnimationChoice {
    int on = 1;
    int level = 0;
    int speed = 125;
    int rate = 0;
};
constexpr int kAnimationSpeedMin = 50;
constexpr int kAnimationSpeedMax = 200;
constexpr int kAnimationSpeedDefault = 125;
constexpr int kAnimationSpeedStep = 10;
constexpr int kFrameRates[] = {0, 90, 60, 30};
AnimationChoice g_animationChoice;

int EncodeAnimations(const AnimationChoice& choice) {
    return 1 + choice.on + choice.level * 2 + choice.rate * 8 +
           choice.speed * 32;
}

AnimationChoice DecodeAnimations(int code) {
    code--;
    return {code & 1, (code >> 1) & 3, code >> 5, (code >> 3) & 3};
}

AnimationChoice AnimationsFromWindhawk() {
    AnimationChoice choice;
    choice.on = Wh_GetIntSetting(L"animations.enabled") != 0;
    const auto level = WindhawkUtils::StringSetting::make(L"animations.level");
    choice.level = wcscmp(level, L"partial") == 0 ? 1
                   : wcscmp(level, L"none") == 0  ? 2
                                                  : 0;
    choice.speed = Wh_GetIntSetting(L"animations.speed");
    const auto rate =
        WindhawkUtils::StringSetting::make(L"animations.frameRate");
    choice.rate = wcscmp(rate, L"fps90") == 0   ? 1
                  : wcscmp(rate, L"fps60") == 0 ? 2
                  : wcscmp(rate, L"fps30") == 0 ? 3
                                                : 0;
    return choice;
}

void ApplyAnimationChoice(AnimationChoice choice) {
    choice.on = choice.on ? 1 : 0;
    choice.level = std::clamp(choice.level, 0, 2);
    choice.speed =
        std::clamp(choice.speed, kAnimationSpeedMin, kAnimationSpeedMax);
    choice.rate = std::clamp(choice.rate, 0, 3);
    g_animationChoice = choice;
    g_animationLevel = choice.on ? choice.level : 2;
    g_animationSpeed = choice.speed;
    g_frameRateLimit = kFrameRates[choice.rate];
}

// Explorer (set first thing in Wh_ModInit), rather than the island's process.
bool g_isExplorer;

// The dock animates as Windhawk's settings say, the island as its own
// settings say. The island's start as Windhawk's were (they were the same
// ones up to 1.0), kept then.
void LoadAnimationSettings() {
    if (g_isExplorer) {
        ApplyAnimationChoice(AnimationsFromWindhawk());
        return;
    }
    int island = Wh_GetIntValue(L"animationsIsland", 0);
    if (island <= 0) {
        island = EncodeAnimations(AnimationsFromWindhawk());
        Wh_SetIntValue(L"animationsIsland", island);
    }
    ApplyAnimationChoice(DecodeAnimations(island));
}

// A setting a profile keeps (see CaptureProfile), stored: the profile in use
// takes it too, a moment later (see SaveActiveProfileSoon).
void SaveActiveProfileSoon();
// Whether the current island shows a profile of its own (another monitor's,
// see Island::SaveOwnProfile), and whether a stored value is part of what it
// shows (not what every island shares, see IsSharedPref).
bool OwnLookShown();
// Whether the island shown (its settings, its panels) is another
// monitor's, not the main one (see Island::HasMainSettings).
bool OtherIslandShown();
bool IsLookValue(PCWSTR name);
// The current island's own profile saved a moment later.
void SaveOwnLookSoon();
// Windhawk's portable storage (a file of its own) loses a stored text's line
// breaks: there they're stored as kStoredLineBreak instead, and read back as
// line breaks wherever stored (see GetStringValueIn). Asked once a process,
// by a text written and read back (named after the process: the island's
// and Explorer's ask at the same time).
constexpr WCHAR kStoredLineBreak = 0x1E;
bool StorageKeepsLineBreaks() {
    static const bool keeps = [] {
        const std::wstring probe =
            L"lineBreakProbe" + std::to_wstring(GetCurrentProcessId());
        Wh_SetStringValue(probe.c_str(), L"a\nb");
        WCHAR back[8] = L"";
        Wh_GetStringValue(probe.c_str(), back, ARRAYSIZE(back));
        Wh_DeleteValue(probe.c_str());
        return wcscmp(back, L"a\nb") == 0;
    }();
    return keeps;
}

// A text stored (see StorageKeepsLineBreaks).
void SetStoredString(PCWSTR name, PCWSTR value) {
    if (!wcschr(value, L'\n') || StorageKeepsLineBreaks()) {
        Wh_SetStringValue(name, value);
        return;
    }
    std::wstring kept(value);
    std::replace(kept.begin(), kept.end(), L'\n', kStoredLineBreak);
    Wh_SetStringValue(name, kept.c_str());
}

// A value of the island's settings: in the main settings (and the profile in
// use there), or, on an island showing a profile of its own, in that
// profile instead.
void StoreProfileInt(PCWSTR name, int value) {
    if (OwnLookShown() && IsLookValue(name)) {
        SaveOwnLookSoon();
        return;
    }
    Wh_SetIntValue(name, value);
    SaveActiveProfileSoon();
}
void StoreProfileString(PCWSTR name, PCWSTR value) {
    if (OwnLookShown() &&
        (wcscmp(name, L"islandOrder") == 0 ||
         wcscmp(name, L"islandNotchSides") == 0 ||
         wcscmp(name, L"trayPinned") == 0 || wcscmp(name, L"trayOrder") == 0 ||
         wcscmp(name, L"trayAnchors") == 0 ||
         wcscmp(name, L"islandIconFolder") == 0 ||
         wcscmp(name, L"islandIconFiles") == 0)) {
        SaveOwnLookSoon();
        return;
    }
    SetStoredString(name, value);
    SaveActiveProfileSoon();
}

// Changed in the island (`keep`: saved, not only shown while sliding).
void SetAnimationChoice(const AnimationChoice& choice, bool keep) {
    ApplyAnimationChoice(choice);
    if (keep) {
        StoreProfileInt(L"animationsIsland",
                        EncodeAnimations(g_animationChoice));
    }
}

// Moves `value` towards `target` like a spring, over about `seconds`.
// `damping` 1 settles without overshooting; lower values bounce a little, like
// the iPhone's Dynamic Island. Returns true while it's still moving.
bool SpringTowards(double& value, double& velocity, double target,
                   double seconds, double dt, double damping = 1.0) {
    // Without animations, it's there at once (told as moving that once:
    // what follows it, like a layout, still goes there); at the "partial"
    // level, it doesn't bounce.
    const int level = AnimationLevel();
    if (level >= 2) {
        const bool jumped = value != target || velocity != 0;
        value = target;
        velocity = 0;
        return jumped;
    }
    if (level == 1) {
        damping = std::max(damping, 1.0);
    }
    const double omega = 8.0 / seconds;
    constexpr double kStep = 1.0 / 480;
    for (double remaining = dt; remaining > 0; remaining -= kStep) {
        const double h = std::min(remaining, kStep);
        const double acceleration = omega * omega * (target - value) -
                                    2 * damping * omega * velocity;
        velocity += acceleration * h;
        value += velocity * h;
    }
    if (std::fabs(target - value) < 0.001 && std::fabs(velocity) < 0.01) {
        value = target;
        velocity = 0;
        return false;
    }
    return true;
}

// A decoration (a glow, a crossfade, an icon turning): like SpringTowards,
// but there at once from the "partial" level on (told as moving that once,
// like SpringTowards without animations).
bool SpringDecoration(double& value, double& velocity, double target,
                      double seconds, double dt, double damping = 1.0) {
    if (AnimationLevel() >= 1) {
        const bool jumped = value != target || velocity != 0;
        value = target;
        velocity = 0;
        return jumped;
    }
    return SpringTowards(value, velocity, target, seconds, dt, damping);
}

bool IsDarkMode();

// Reads the chosen theme. Chosen in the island (or Windows' mode switched
// with the automatic one), the colors blend into it (see StepTheme);
// otherwise, they're there right away.
void ApplyThemeIndex(bool blend);

void LoadTheme(bool blend = false) {
    g_themeIndex =
        std::clamp(Wh_GetIntValue(L"islandTheme", 0), 0, (int)ThemeId::Count - 1);
    LoadCustomColors();
    ApplyThemeIndex(blend);
}

// The colors of the theme in g_themeIndex (see LoadTheme).
void ApplyThemeIndex(bool blend) {
    ThemeId theme = (ThemeId)g_themeIndex;
    if (theme == ThemeId::Auto) {
        theme = IsDarkMode() ? ThemeId::Black : ThemeId::Light;
    } else if (theme == ThemeId::AutoInverted) {
        theme = IsDarkMode() ? ThemeId::Light : ThemeId::Black;
    }
    IslandTheme& target = g_themeTarget;
    target = {};
    if (theme == ThemeId::Custom) {
        CustomColorParts(g_customColors[kColorBackground], target.background);
        CustomColorParts(g_customColors[kColorText], target.foreground);
        // A light background takes the light theme's darker texts.
        const float* b = target.background;
        target.light = Luminance(b[0], b[1], b[2]) > 0.6f;
    } else if (theme == ThemeId::Graphite) {
        target.background[0] = 0.17f;
        target.background[1] = 0.17f;
        target.background[2] = 0.19f;
    } else if (theme == ThemeId::Light) {
        target.background[0] = 0.96f;
        target.background[1] = 0.96f;
        target.background[2] = 0.97f;
        target.foreground[0] = 0.02f;
        target.foreground[1] = 0.02f;
        target.foreground[2] = 0.03f;
        target.light = 1;
    } else if (theme == ThemeId::Accent) {
        // Darkened, so white reads well on it.
        DWORD color = 0;
        BOOL opaque = FALSE;
        if (SUCCEEDED(DwmGetColorizationColor(&color, &opaque))) {
            target.background[0] = ((color >> 16) & 0xFF) / 255.0f * 0.55f;
            target.background[1] = ((color >> 8) & 0xFF) / 255.0f * 0.55f;
            target.background[2] = (color & 0xFF) / 255.0f * 0.55f;
        }
    } else if (theme == ThemeId::Glass) {
        // Frosted: a gray haze over what's behind, thick enough to read on
        // any window (a layered window can't blur what's behind it).
        target.background[0] = 0.19f;
        target.background[1] = 0.19f;
        target.background[2] = 0.21f;
        target.backgroundAlpha = 0.94f;
    }
    if (theme == ThemeId::Custom) {
        CustomColorParts(g_customColors[kColorIcons], target.icon);
    } else {
        std::copy(target.foreground, target.foreground + 3, target.icon);
    }
    // The app in front's color: the background darkened like the accent's
    // theme (so white texts read well on it), and the icons in a light tint
    // of it (white for a dark color).
    if (UsesWindowColor()) {
        float color[3];
        CustomColorParts(g_windowColor, color);
        const bool dark = Luminance(color[0], color[1], color[2]) < 0.25f;
        for (int i = 0; i < 3; i++) {
            target.background[i] = color[i] * 0.55f;
            target.foreground[i] = 1;
            target.icon[i] = dark ? 1 : color[i] + (1 - color[i]) * 0.45f;
        }
        target.light = 0;
        target.backgroundAlpha = 1;
    }
    if (blend) {
        g_themeFrom = g_theme;
        g_themeChangeStart = NowSeconds();
    } else {
        g_theme = target;
        g_themeChangeStart = 0;
    }
}

// How long the colors take to blend into another theme.
constexpr double kThemeChangeSeconds = 0.45;

// Blends the shown colors a step further into the chosen theme (eased in and
// out). Returns whether they changed.
bool StepTheme() {
    g_themeChanging = false;
    if (!g_themeChangeStart) {
        return false;
    }
    const double length = AnimationSeconds(kThemeChangeSeconds, true);
    const double t =
        length > 0 ? std::clamp((NowSeconds() - g_themeChangeStart) / length,
                                0.0, 1.0)
                   : 1.0;
    const float k = (float)(t < 0.5 ? 4 * t * t * t
                                    : 1 - std::pow(-2 * t + 2, 3) / 2);
    auto mix = [k](float a, float b) { return a + (b - a) * k; };
    for (int i = 0; i < 3; i++) {
        g_theme.background[i] =
            mix(g_themeFrom.background[i], g_themeTarget.background[i]);
        g_theme.foreground[i] =
            mix(g_themeFrom.foreground[i], g_themeTarget.foreground[i]);
        g_theme.icon[i] = mix(g_themeFrom.icon[i], g_themeTarget.icon[i]);
    }
    g_theme.backgroundAlpha =
        mix(g_themeFrom.backgroundAlpha, g_themeTarget.backgroundAlpha);
    g_theme.light = mix(g_themeFrom.light, g_themeTarget.light);
    if (t >= 1) {
        g_theme = g_themeTarget;
        g_themeChangeStart = 0;
    }
    g_themeChanging = true;
    return true;
}

// Whether the taskbar was at the bottom when the settings were read.
std::atomic<bool> g_taskbarAtBottom{true};

// The mod's dock (otherwise, the Windows taskbar, and the mod only adds the
// island).
bool IsDock() {
    return g_settings.taskbarMode == decltype(g_settings.taskbarMode)::Dock;
}

// The Windows taskbar, transparent, with only its apps.
bool IsTransparentTaskbar() {
    return g_settings.taskbarMode ==
           decltype(g_settings.taskbarMode)::WindowsTransparent;
}

// The taskbar hides itself, and slides in and out with the mod's animation:
// the dock, and the transparent Windows taskbar (at the bottom: on the other
// edges it stays as Windows has it, only transparent).
bool SlidesItself() {
    return IsDock() || (IsTransparentTaskbar() && g_taskbarAtBottom);
}

// The Windows taskbar with only its apps, with the dock's effects too
// (icons.onlyApps, at the bottom): its icons grow inside its height and
// bounce, and it hides when a window is in its way (with Windows' own
// auto-hide).
bool AppsTaskbarEffects() {
    return g_settings.taskbarMode ==
               decltype(g_settings.taskbarMode)::WindowsApps &&
           g_settings.onlyAppsEffects && g_taskbarAtBottom;
}

// The icons grow under the mouse and bounce: the dock, and the transparent
// Windows taskbar too (at the bottom), and the one with only its apps with
// that option.
bool AnimatesIcons() {
    return SlidesItself() || AppsTaskbarEffects();
}

// The transparent taskbar's soft shadow, rising from the bottom edge: a bit
// stronger over light windows (measured as it shows) and under the mouse.
std::atomic<double> g_backdropLight{0.5};

std::atomic<bool> g_unloading;

// The account's SID as text (empty if it can't be read).
const std::wstring& UserSid() {
    static const std::wstring sid = [] {
        std::wstring sid;
        HANDLE token = nullptr;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            alignas(TOKEN_USER) BYTE
                buffer[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE];
            DWORD size = 0;
            LPWSTR text = nullptr;
            if (GetTokenInformation(token, TokenUser, buffer, sizeof(buffer),
                                    &size) &&
                ConvertSidToStringSid(((TOKEN_USER*)buffer)->User.Sid,
                                      &text)) {
                sid = text;
                LocalFree(text);
            }
            CloseHandle(token);
        }
        return sid;
    }();
    return sid;
}

// The mod's storage is the computer's, shared by its accounts: what one
// account's session changed in Windows (its auto-hide, its Do not disturb),
// its explorer.exe part and what describes its own Windows (its
// notifications cleared here, its tray icons kept across a reload) are kept
// under a name with the account's SID, so another account never undoes or
// reads them. Values kept before under the plain name
// are read once and moved.
std::wstring UserValueName(PCWSTR name) {
    const std::wstring& sid = UserSid();
    return sid.empty() ? std::wstring(name) : name + (L"_" + sid);
}

// (`shared`: a value every account used before 2.1, like the timer's
// lengths: copied, not moved, so each account starts from it once.)
int GetUserIntValue(PCWSTR name, int defaultValue, bool shared = false) {
    const std::wstring userName = UserValueName(name);
    constexpr int kMissing = INT_MIN;
    const int value = Wh_GetIntValue(userName.c_str(), kMissing);
    if (value != kMissing) {
        return value;
    }
    const int legacy = Wh_GetIntValue(name, kMissing);
    if (legacy == kMissing || userName == name) {
        return defaultValue;
    }
    Wh_SetIntValue(userName.c_str(), legacy);
    if (!shared) {
        Wh_DeleteValue(name);
    }
    return legacy;
}

void SetUserIntValue(PCWSTR name, int value) {
    Wh_SetIntValue(UserValueName(name).c_str(), value);
}

// A stored text in a buffer of `size` characters (empty when it doesn't fit,
// like a missing one), its line breaks as written (see SetStoredString).
std::wstring GetStringValueIn(PCWSTR name, size_t size) {
    const auto value = std::make_unique_for_overwrite<WCHAR[]>(size);
    std::wstring text =
        Wh_GetStringValue(name, value.get(), size) ? value.get() : L"";
    std::replace(text.begin(), text.end(), kStoredLineBreak, L'\n');
    return text;
}

// A stored text, whole: Wh_GetStringValue reads a value larger than the
// buffer as empty, so a larger one is tried, up to 16 times `room` (the usual
// size; no stored text grows that far). A missing value costs those reads
// too: not for a value read often.
std::wstring GetWholeStringValue(PCWSTR name, size_t room) {
    const size_t most = std::max(room, std::min(room * 16, (size_t)1 << 20));
    for (size_t size = room; size <= most; size *= 2) {
        std::wstring value = GetStringValueIn(name, size);
        if (!value.empty()) {
            return value;
        }
    }
    return L"";
}

// Read once (on Explorer's taskbar thread too): its writers keep it within
// `room` (the kept tray icons, the cleared and dismissed notifications).
std::wstring GetUserStringValue(PCWSTR name, size_t room) {
    const std::wstring userName = UserValueName(name);
    std::wstring value = GetStringValueIn(userName.c_str(), room);
    if (!value.empty() || userName == name) {
        return value;
    }
    value = GetStringValueIn(name, room);
    if (!value.empty()) {
        SetStoredString(userName.c_str(), value.c_str());
        Wh_DeleteValue(name);
    }
    return value;
}

void SetUserStringValue(PCWSTR name, PCWSTR value) {
    SetStoredString(UserValueName(name).c_str(), value);
}

// The mark that the account has its own copy of a text every account used
// before 2.1 (see GetWholeUserStringValue), even an empty one.
std::wstring UserSeededName(PCWSTR name) {
    return UserValueName((std::wstring(name) + L"Seeded").c_str());
}

// The account's own text, whole (see GetWholeStringValue). The alarms and
// the folders' pins and looks were every account's before 2.1: the text kept
// under the plain name is copied, not moved, so each account starts from
// it, once (its mark then: an account that emptied its own doesn't get the
// old one back).
std::wstring GetWholeUserStringValue(PCWSTR name, size_t room) {
    const std::wstring userName = UserValueName(name);
    std::wstring value = GetWholeStringValue(userName.c_str(), room);
    if (!value.empty() || userName == name) {
        return value;
    }
    const std::wstring seeded = UserSeededName(name);
    if (Wh_GetIntValue(seeded.c_str(), 0)) {
        return value;
    }
    Wh_SetIntValue(seeded.c_str(), 1);
    value = GetWholeStringValue(name, room);
    if (!value.empty()) {
        SetStoredString(userName.c_str(), value.c_str());
    }
    return value;
}

// A first install, on this account: nothing of the island stored yet (its
// animations' value is stored as it first starts, see
// LoadAnimationSettings), What's new never told and the setup assistant
// never shown.
bool FirstInstall() {
    return Wh_GetIntValue(L"animationsIsland", 0) <= 0 &&
           GetUserStringValue(L"whatsNewTold", 32).empty() &&
           GetUserStringValue(L"setupDone", 32).empty();
}
// Read as the tool mod starts (before that value is stored), until the
// setup assistant opens (see Island::OpenSetup).
bool g_firstInstall;

// The mod's own module (set in Wh_ModInit), for its window classes.
HMODULE g_module;

// Runs `f`, keeping its errors from reaching the code that called it: XAML
// calls throw when an element isn't ready (the taskbar hiding for a
// full-screen app, for one), and an exception left uncaught in code that
// Windows calls (hooks, window procedures, XAML events) ends Explorer.
template <typename F>
void Safely(PCWSTR what, F&& f) {
    try {
        f();
    } catch (winrt::hresult_error const& e) {
        IslandLog(L"%s failed: %08X", what, (unsigned)e.code());
    } catch (...) {
        IslandLog(L"%s failed", what);
    }
}

double NowSeconds() {
    static const double frequency = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return (double)f.QuadPart;
    }();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart / frequency;
}

// When a taskbar button last started flashing (an app asking for attention),
// in NowSeconds() time.
std::atomic<double> g_lastFlashTime{0};
// Where the button of the app that last asked for attention is, horizontally,
// in screen pixels, and when that was measured (see RecordAttentionButton).
std::atomic<int> g_attentionButtonX{0};
std::atomic<double> g_attentionButtonTime{0};

// Until when the dock stays up no matter what, in NowSeconds() time, while an
// app clicked in it opens (see KeepDockShown).
std::atomic<double> g_keepShownUntil{0};
// The taskbar it's for (the other monitors' hide as usual). Taskbar thread
// only.
HWND g_keepShownWnd;

// Whether `taskbar` is the dock kept up for an app opening (see
// KeepDockShown): not another monitor's.
bool IsKeptShown(HWND taskbar) {
    return taskbar && taskbar == g_keepShownWnd &&
           NowSeconds() < g_keepShownUntil;
}

// Bounces come in groups of three, each one a little lower, like on macOS.
// Opening an app and asking for attention are one group each.
constexpr double kBounceTime = 0.42;
constexpr double kBounceGroupPause = 0.25;
constexpr int kBouncesPerGroup = 3;
constexpr double kBounceGroupLength =
    kBouncesPerGroup * kBounceTime + kBounceGroupPause;
constexpr int kLaunchBounceGroups = 1;
constexpr int kAttentionBounceGroups = 1;

// Height of a bounce sequence `elapsed` seconds into it, from 0 (resting) to
// 1 (highest). Each bounce is a parabola, like a thrown object. Sets `done`
// when the sequence is over.
double BounceSequence(double elapsed, int groups, bool* done) {
    *done = false;
    const int group = (int)(elapsed / kBounceGroupLength);
    if (group >= groups) {
        *done = true;
        return 0;
    }
    const double inGroup = elapsed - group * kBounceGroupLength;
    const int bounce = (int)(inGroup / kBounceTime);
    if (bounce >= kBouncesPerGroup) {
        return 0;  // The pause between groups.
    }
    const double phase = (inGroup - bounce * kBounceTime) / kBounceTime;
    return (1.0 - 0.18 * bounce) * 4 * phase * (1 - phase);
}

// Whether the mod turned auto-hide on, so it's turned off again on unload.
bool g_turnedAutoHideOn;

// The taskbar's internal timers: hiding after the mouse leaves, and showing
// after the mouse reaches the edge.
constexpr UINT_PTR kHideTimerId = 2;
constexpr UINT_PTR kUnhideTimerId = 3;

BOOL CALLBACK FindTaskbarProc(HWND hWnd, LPARAM lParam) {
    DWORD dwProcessId;
    WCHAR className[32];
    if (GetWindowThreadProcessId(hWnd, &dwProcessId) &&
        dwProcessId == GetCurrentProcessId() &&
        GetClassName(hWnd, className, ARRAYSIZE(className)) &&
        _wcsicmp(className, L"Shell_TrayWnd") == 0) {
        *reinterpret_cast<HWND*>(lParam) = hWnd;
        return FALSE;
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Finding the taskbar's XAML elements.

HWND FindCurrentProcessTaskbarWnd() {
    HWND hTaskbarWnd = nullptr;
    EnumWindows(
        FindTaskbarProc,
        reinterpret_cast<LPARAM>(&hTaskbarWnd));
    return hTaskbarWnd;
}

// There can be more than one explorer.exe (folders can open in a process of
// their own): only the one with the taskbar runs the watchers and the
// auto-hide setting.
// This process's main taskbar window, remembered (the sender of the
// island's messages).
std::atomic<HWND> g_ownTaskbarWnd;

HWND OwnTaskbarWnd() {
    HWND hWnd = g_ownTaskbarWnd;
    if (!hWnd || !IsWindow(hWnd)) {
        hWnd = FindCurrentProcessTaskbarWnd();
        g_ownTaskbarWnd = hWnd;
    }
    return hWnd;
}

bool OwnsTaskbar() {
    return FindCurrentProcessTaskbarWnd() != nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// The mod runs in two processes (see the tool mod implementation at the end):
// in explorer.exe, the dock and what only the taskbar knows (the apps' badges,
// the tray icons the apps send it); in a windhawk.exe process of its own, the
// island, so that nothing in it can take the shell down. They tell each other
// what changed with WM_COPYDATA, tagged, never waiting long for each other.

constexpr WCHAR kIslandClassName[] = L"WindhawkMacIslandDockIsland";
constexpr WCHAR kPanelClassName[] = L"WindhawkMacIslandDockPanel";
// The notification watcher's message-only window (the sender of the apps
// with new notifications, with the island off too).
constexpr WCHAR kNotifierClassName[] = L"WindhawkMacIslandDockNotifier";

enum IpcKind : ULONG_PTR {
    // Explorer to the island: the apps with a badge ('\n'-separated IDs).
    kIpcBadgeApps = 0x4D494401,
    // Explorer to the island: the tray icons (see SendTrayIcons).
    kIpcTrayIcons,
    // The island to explorer: it started, send everything.
    kIpcHello,
    // The island to explorer: where the tray icons are on the screen.
    kIpcTrayPlaces,
    // The island to explorer: apps with a new notification ('\n'-separated
    // IDs), so their icons bounce.
    kIpcNotifiedApps,
    // The island to explorer: an app's icon (an int size, then its ID). The
    // answer is the icon (an HICON), kept by explorer: the island copies it.
    // The shell gives packaged apps' icons in explorer, not always in the
    // island's process.
    kIpcAppIcon,
    // The island to explorer: whether an island fills with the apps'
    // progress (an int, 1 or 0): the taskbar's bars are watched only then.
    kIpcProgressWanted,
};
constexpr UINT kIpcTimeoutMs = 400;
// The ID the Widgets button's badge goes to the island under (its
// AutomationId; the Widgets board has news): it isn't an app, so the island
// shows it with its own name and icon, and a click opens the board.
constexpr WCHAR kWidgetsBadgeId[] = L"WidgetsButton";

// In explorer (the taskbar's thread), messages sent to it from other threads
// wait meanwhile (SMTO_BLOCK): the mod's own unloading sends it some (see
// ExplorerModBeforeUninit), and they must not run while this code is still
// on its stack. True once delivered; what the other side answered in
// `answer` (TRUE when the mod's part there took it: the taskbar not yet
// subclassed, after Explorer started again, answers something else).
bool SendIpc(HWND to,
             HWND from,
             IpcKind kind,
             const void* data,
             DWORD size,
             DWORD_PTR* answer = nullptr) {
    if (!to) {
        return false;
    }
    COPYDATASTRUCT copyData{kind, size, (PVOID)data};
    DWORD_PTR result = 0;
    const bool sent =
        SendMessageTimeout(to, WM_COPYDATA, (WPARAM)from, (LPARAM)&copyData,
                           SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT |
                               (g_isExplorer ? SMTO_BLOCK : 0),
                           kIpcTimeoutMs, &result) != 0;
    if (answer) {
        *answer = sent ? result : 0;
    }
    return sent;
}

// The island's tray icon places, to explorer from a thread of their own: the
// taskbar may be busy for a moment (a window coming to the front, a panel
// opening), and the island's frames don't wait for it. Only the latest
// places are sent.
HWND FindTaskbarWnd();

struct TrayPlacesSender {
    SRWLOCK lock = SRWLOCK_INIT;
    CONDITION_VARIABLE wake = CONDITION_VARIABLE_INIT;
    std::vector<BYTE> pending;
    HWND from = nullptr;
    bool queued = false;
    bool stopping = false;
    HANDLE thread = nullptr;
};
TrayPlacesSender g_trayPlacesSender;

DWORD WINAPI TrayPlacesThreadProc(LPVOID) {
    TrayPlacesSender& sender = g_trayPlacesSender;
    for (;;) {
        AcquireSRWLockExclusive(&sender.lock);
        while (!sender.queued && !sender.stopping) {
            SleepConditionVariableSRW(&sender.wake, &sender.lock, INFINITE, 0);
        }
        if (sender.stopping) {
            ReleaseSRWLockExclusive(&sender.lock);
            return 0;
        }
        std::vector<BYTE> bytes = std::move(sender.pending);
        const HWND from = sender.from;
        sender.queued = false;
        ReleaseSRWLockExclusive(&sender.lock);
        SendIpc(FindTaskbarWnd(), from, kIpcTrayPlaces, bytes.data(),
                (DWORD)bytes.size());
    }
}

// (Not once stopped, until the main island is created again: an island
// going away after the main one doesn't start it again.)
void AllowTrayPlaces() {
    TrayPlacesSender& sender = g_trayPlacesSender;
    AcquireSRWLockExclusive(&sender.lock);
    sender.stopping = false;
    ReleaseSRWLockExclusive(&sender.lock);
}

void QueueTrayPlaces(HWND from, const std::vector<BYTE>& bytes) {
    TrayPlacesSender& sender = g_trayPlacesSender;
    AcquireSRWLockExclusive(&sender.lock);
    if (sender.stopping) {
        ReleaseSRWLockExclusive(&sender.lock);
        return;
    }
    sender.pending = bytes;
    sender.from = from;
    sender.queued = true;
    if (!sender.thread) {
        sender.thread =
            CreateThread(nullptr, 0, TrayPlacesThreadProc, nullptr, 0, nullptr);
    }
    ReleaseSRWLockExclusive(&sender.lock);
    WakeConditionVariable(&sender.wake);
}

void StopTrayPlacesSender() {
    TrayPlacesSender& sender = g_trayPlacesSender;
    AcquireSRWLockExclusive(&sender.lock);
    const HANDLE thread = sender.thread;
    sender.thread = nullptr;
    sender.stopping = true;
    sender.queued = false;
    sender.pending.clear();
    ReleaseSRWLockExclusive(&sender.lock);
    WakeConditionVariable(&sender.wake);
    if (thread) {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
}

// Whether a WM_COPYDATA came from the other part of the mod: the island's
// window (in explorer), or the taskbar (in the island's process).
bool IsIpcSender(HWND sender, PCWSTR className) {
    WCHAR name[64];
    return sender && GetClassName(sender, name, ARRAYSIZE(name)) &&
           wcscmp(name, className) == 0;
}

HWND FindIslandWnd() {
    return FindWindow(kIslandClassName, nullptr);
}

// The main taskbar, from any process.
HWND FindTaskbarWnd() {
    return FindWindow(L"Shell_TrayWnd", nullptr);
}

// Told to the taskbar by the island when it sees the monitors changed (see
// Island::Tick): restyled as after WM_DISPLAYCHANGE, which can be missed.
UINT DisplayChangedMessage() {
    static const UINT message =
        RegisterWindowMessage(L"WindhawkMacIslandDock_DisplayChanged");
    return message;
}

// Where Windows keeps the taskbar's edge (as for TaskbarEdge), read without
// asking the taskbar. Read again at most twice a second: the island lays
// itself out, and the taskbars find their monitor, with every frame.
UINT StoredTaskbarEdge() {
    static std::atomic<ULONGLONG> readAt{0};
    static std::atomic<UINT> stored{ABE_BOTTOM};
    const ULONGLONG now = GetTickCount64();
    const ULONGLONG last = readAt.load(std::memory_order_relaxed);
    if (last && now - last < 500) {
        return stored.load(std::memory_order_relaxed);
    }
    UINT edge = ABE_BOTTOM;
    BYTE data[64];
    DWORD size = sizeof(data);
    if (RegGetValue(HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                    L"StuckRects3",
                    L"Settings", RRF_RT_REG_BINARY, nullptr, data,
                    &size) == ERROR_SUCCESS &&
        size > 12 && data[12] <= ABE_BOTTOM) {
        edge = data[12];
    }
    stored.store(edge, std::memory_order_relaxed);
    readAt.store(now, std::memory_order_relaxed);
    return edge;
}

// The edge last seen from the taskbar's window all on its monitor (see
// TaskbarEdge), or UINT_MAX before. Moved in Settings, the taskbar keeps the
// edge stored for it as it was until explorer restarts.
std::atomic<UINT> g_seenTaskbarEdge{UINT_MAX};

// The edge last seen, or the stored one before it's been seen.
UINT KnownTaskbarEdge() {
    const UINT seen = g_seenTaskbarEdge.load(std::memory_order_relaxed);
    return seen != UINT_MAX ? seen : StoredTaskbarEdge();
}

// The edge of `monitor` a taskbar at `rect` is on, also while it's hidden
// (mostly past that edge): from its shape and which half its middle is in.
UINT RectSide(const RECT& rect, const RECT& monitor) {
    if (rect.right - rect.left < rect.bottom - rect.top) {
        return rect.left + rect.right < monitor.left + monitor.right
                   ? ABE_LEFT
                   : ABE_RIGHT;
    }
    return rect.top + rect.bottom < monitor.top + monitor.bottom ? ABE_TOP
                                                                 : ABE_BOTTOM;
}

// The edge a taskbar at `rect` is on, if it's all on a monitor (shown).
bool RectEdge(const RECT& rect, UINT* edge) {
    MONITORINFO info{sizeof(info)};
    if (!GetMonitorInfo(MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST),
                        &info) ||
        rect.left < info.rcMonitor.left || rect.top < info.rcMonitor.top ||
        rect.right > info.rcMonitor.right ||
        rect.bottom > info.rcMonitor.bottom) {
        return false;
    }
    // (Not a bar: almost square for a moment on its way to another edge.)
    const LONG width = rect.right - rect.left;
    const LONG height = rect.bottom - rect.top;
    if (height * 2 > width && width * 2 > height) {
        return false;
    }
    if (width < height) {
        *edge = rect.left + rect.right <
                        info.rcMonitor.left + info.rcMonitor.right
                    ? ABE_LEFT
                    : ABE_RIGHT;
    } else {
        *edge = rect.top + rect.bottom <
                        info.rcMonitor.top + info.rcMonitor.bottom
                    ? ABE_TOP
                    : ABE_BOTTOM;
    }
    return true;
}

// The edge of its monitor the taskbar is on (ABE_LEFT, ABE_TOP, ABE_RIGHT or
// ABE_BOTTOM; Windows 11 can put it on any). From where its window is only
// while it's all on its monitor: hidden or sliding, most of it is past the
// edge, maybe over another monitor (which made the dock and the taskbar as
// it is take turns). Otherwise from where Windows keeps it (asking the
// taskbar waits for it while it's busy, and from its own process could wait
// for a thread waiting for it), as last seen.
UINT TaskbarEdge() {
    HWND taskbar = FindTaskbarWnd();
    RECT rect;
    UINT edge;
    if (taskbar && GetWindowRect(taskbar, &rect) && RectEdge(rect, &edge)) {
        g_seenTaskbarEdge.store(edge, std::memory_order_relaxed);
        return edge;
    }
    return KnownTaskbarEdge();
}

// The monitor a taskbar belongs to: the one under its row next to the
// screen's inside (all of it when shown, the 2 pixels left of it when
// auto-hidden). Not MonitorFromWindow: a hidden taskbar is mostly past its
// edge, maybe over the next monitor (the one below a bottom taskbar), which
// that would take.
HMONITOR TaskbarMonitor(HWND taskbar) {
    RECT rect;
    if (!taskbar || !GetWindowRect(taskbar, &rect)) {
        return MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
    }
    const LONG middleX = (rect.left + rect.right) / 2;
    const LONG middleY = (rect.top + rect.bottom) / 2;
    POINT inner;
    switch (KnownTaskbarEdge()) {
        case ABE_LEFT:
            inner = {rect.right - 1, middleY};
            break;
        case ABE_RIGHT:
            inner = {rect.left, middleY};
            break;
        case ABE_TOP:
            inner = {middleX, rect.bottom - 1};
            break;
        default:
            inner = {middleX, rect.top};
            break;
    }
    return MonitorFromPoint(inner, MONITOR_DEFAULTTONEAREST);
}

// The part of a taskbar's window on its own monitor, in the window's
// pixels (empty if none); `at`: with the window there.
RECT TaskbarOwnPart(HWND taskbar, const RECT* at = nullptr) {
    RECT window{};
    RECT part{};
    MONITORINFO info{sizeof(info)};
    if ((at ? (window = *at, true) : GetWindowRect(taskbar, &window)) &&
        GetMonitorInfo(TaskbarMonitor(taskbar), &info) &&
        IntersectRect(&part, &window, &info.rcMonitor)) {
        OffsetRect(&part, -window.left, -window.top);
    }
    return part;
}

// Whether the taskbar is at the bottom of its monitor.
bool IsTaskbarAtBottom() {
    return TaskbarEdge() == ABE_BOTTOM;
}

std::wstring JoinLines(const std::vector<std::wstring>& lines) {
    std::wstring joined;
    for (const auto& line : lines) {
        joined += line + L"\n";
    }
    return joined;
}

std::vector<std::wstring> SplitLines(const COPYDATASTRUCT* copyData) {
    std::vector<std::wstring> lines;
    const auto* text = (const WCHAR*)copyData->lpData;
    const size_t length = copyData->cbData / sizeof(WCHAR);
    size_t start = 0;
    for (size_t i = 0; i < length; i++) {
        if (text[i] == L'\n') {
            if (i > start) {
                lines.emplace_back(text + start, i - start);
            }
            start = i + 1;
        }
    }
    return lines;
}

bool SendLines(HWND to, HWND from, IpcKind kind,
               const std::vector<std::wstring>& lines) {
    const std::wstring joined = JoinLines(lines);
    return SendIpc(to, from, kind, joined.data(),
                   (DWORD)(joined.size() * sizeof(WCHAR)));
}

// Plain values and strings, one after another.
struct IpcWriter {
    std::vector<BYTE> bytes;
    template <typename T>
    void Put(const T& value) {
        const BYTE* data = (const BYTE*)&value;
        bytes.insert(bytes.end(), data, data + sizeof(T));
    }
    void PutString(const std::wstring& text) {
        Put<UINT32>((UINT32)text.size());
        const BYTE* data = (const BYTE*)text.data();
        bytes.insert(bytes.end(), data, data + text.size() * sizeof(WCHAR));
    }
};

struct IpcReader {
    const BYTE* at;
    const BYTE* end;
    explicit IpcReader(const COPYDATASTRUCT* copyData)
        : at((const BYTE*)copyData->lpData), end(at + copyData->cbData) {}
    template <typename T>
    bool Get(T* value) {
        if (end - at < (ptrdiff_t)sizeof(T)) {
            return false;
        }
        memcpy(value, at, sizeof(T));
        at += sizeof(T);
        return true;
    }
    bool GetString(std::wstring* text) {
        UINT32 length = 0;
        if (!Get(&length) || length > (size_t)(end - at) / sizeof(WCHAR)) {
            return false;
        }
        text->assign((const WCHAR*)at, length);
        at += length * sizeof(WCHAR);
        return true;
    }
};

// Breadth-first search, so the shallowest match wins.
FrameworkElement FindDescendant(
    FrameworkElement root,
    std::function<bool(FrameworkElement const&)> predicate,
    int maxDepth = 12) {
    std::vector<FrameworkElement> level{root};
    for (int depth = 0; depth < maxDepth && !level.empty(); depth++) {
        std::vector<FrameworkElement> next;
        for (const auto& element : level) {
            const int count = Media::VisualTreeHelper::GetChildrenCount(element);
            for (int i = 0; i < count; i++) {
                auto child = Media::VisualTreeHelper::GetChild(element, i)
                                 .try_as<FrameworkElement>();
                if (!child) {
                    continue;
                }
                if (predicate(child)) {
                    return child;
                }
                next.push_back(child);
            }
        }
        level = std::move(next);
    }
    return nullptr;
}

FrameworkElement FindByClassName(FrameworkElement root, PCWSTR className) {
    return FindDescendant(root, [className](FrameworkElement const& element) {
        return winrt::get_class_name(element) == className;
    });
}

FrameworkElement FindByName(FrameworkElement root, PCWSTR name) {
    return FindDescendant(root, [name](FrameworkElement const& element) {
        return element.Name() == name;
    });
}

void* CTaskBand_ITaskListWndSite_vftable;
void* CSecondaryTaskBand_ITaskListWndSite_vftable;

using CTaskBand_GetTaskbarHost_t = void*(WINAPI*)(void* pThis, void** result);
CTaskBand_GetTaskbarHost_t CTaskBand_GetTaskbarHost_Original;

void* TaskbarHost_FrameHeight_Original;

using CSecondaryTaskBand_GetTaskbarHost_t = void*(WINAPI*)(void* pThis,
                                                           void** result);
CSecondaryTaskBand_GetTaskbarHost_t CSecondaryTaskBand_GetTaskbarHost_Original;

using std__Ref_count_base__Decref_t = void(WINAPI*)(void* pThis);
std__Ref_count_base__Decref_t std__Ref_count_base__Decref_Original;

XamlRoot XamlRootFromTaskbarHostSharedPtr(void* taskbarHostSharedPtr[2]) {
    if (!taskbarHostSharedPtr[0] && !taskbarHostSharedPtr[1]) {
        return nullptr;
    }

    size_t taskbarElementIUnknownOffset = 0x10;

#if defined(_M_X64)
    {
        // 48:83EC 28 | sub rsp,28
        // 48:83C1 48 | add rcx,48
        const BYTE* b = (const BYTE*)TaskbarHost_FrameHeight_Original;
        if (b[0] == 0x48 && b[1] == 0x83 && b[2] == 0xEC && b[4] == 0x48 &&
            b[5] == 0x83 && b[6] == 0xC1 && b[7] <= 0x7F) {
            taskbarElementIUnknownOffset = b[7];
        } else {
            IslandLog(L"Unsupported TaskbarHost::FrameHeight");
        }
    }
#elif defined(_M_ARM64)
    {
        // 7f2303d5 pacibsp
        // fd7bbfa9 stp     fp, lr, [sp, #-0x10]!
        // fd030091 mov     fp, sp
        // 080c41f8 ldr     x8, [x0, #0x10]!
        const DWORD* p = (const DWORD*)TaskbarHost_FrameHeight_Original;
        if (p[0] == 0xD503237F && (p[1] & 0xFFC07FFF) == 0xA9807BFD &&
            p[2] == 0x910003FD && (p[3] & 0xFFF00FE0) == 0xF8400C00) {
            taskbarElementIUnknownOffset = (p[3] >> 12) & 0xFF;
        } else {
            IslandLog(L"Unsupported TaskbarHost::FrameHeight");
        }
    }
#else
    // 32-bit: only the island's process (windhawk.exe) is, and the taskbar
    // part never runs there (Windows 11's explorer.exe is 64-bit).
#endif

    auto* taskbarElementIUnknown =
        *(IUnknown**)((BYTE*)taskbarHostSharedPtr[0] +
                      taskbarElementIUnknownOffset);

    FrameworkElement taskbarElement = nullptr;
    taskbarElementIUnknown->QueryInterface(winrt::guid_of<FrameworkElement>(),
                                           winrt::put_abi(taskbarElement));

    auto result = taskbarElement ? taskbarElement.XamlRoot() : nullptr;

    std__Ref_count_base__Decref_Original(taskbarHostSharedPtr[1]);

    return result;
}

XamlRoot GetTaskbarXamlRoot(HWND hTaskbarWnd) {
    HWND hTaskSwWnd = (HWND)GetProp(hTaskbarWnd, L"TaskbandHWND");
    if (!hTaskSwWnd) {
        return nullptr;
    }

    void* taskBand = (void*)GetWindowLongPtr(hTaskSwWnd, 0);
    void* taskBandForTaskListWndSite = taskBand;
    for (int i = 0; *(void**)taskBandForTaskListWndSite !=
                    CTaskBand_ITaskListWndSite_vftable;
         i++) {
        if (i == 20) {
            return nullptr;
        }
        taskBandForTaskListWndSite = (void**)taskBandForTaskListWndSite + 1;
    }

    void* taskbarHostSharedPtr[2]{};
    CTaskBand_GetTaskbarHost_Original(taskBandForTaskListWndSite,
                                      taskbarHostSharedPtr);
    return XamlRootFromTaskbarHostSharedPtr(taskbarHostSharedPtr);
}

XamlRoot GetSecondaryTaskbarXamlRoot(HWND hSecondaryTaskbarWnd) {
    HWND hTaskSwWnd =
        (HWND)FindWindowEx(hSecondaryTaskbarWnd, nullptr, L"WorkerW", nullptr);
    if (!hTaskSwWnd) {
        return nullptr;
    }

    void* taskBand = (void*)GetWindowLongPtr(hTaskSwWnd, 0);
    void* taskBandForTaskListWndSite = taskBand;
    for (int i = 0; *(void**)taskBandForTaskListWndSite !=
                    CSecondaryTaskBand_ITaskListWndSite_vftable;
         i++) {
        if (i == 20) {
            return nullptr;
        }
        taskBandForTaskListWndSite = (void**)taskBandForTaskListWndSite + 1;
    }

    void* taskbarHostSharedPtr[2]{};
    CSecondaryTaskBand_GetTaskbarHost_Original(taskBandForTaskListWndSite,
                                               taskbarHostSharedPtr);
    return XamlRootFromTaskbarHostSharedPtr(taskbarHostSharedPtr);
}

////////////////////////////////////////////////////////////////////////////////
// The dock style (taskbar thread).

