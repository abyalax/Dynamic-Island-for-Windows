#pragma once
// A pinned icon's place: -1 with the tray, a group's number after that
// group, or TrayAnchorBefore(group) before it.
int TrayAnchorBefore(PillGroup group) {
    return -(int)group - 2;
}

int TrayAnchor(const std::wstring& key) {
    for (const auto& [anchored, anchor] : g_trayAnchors) {
        if (anchored == key) {
            return anchor;
        }
    }
    return -1;
}

std::vector<std::wstring> DecodeTrayKeys(std::wstring_view text) {
    std::vector<std::wstring> keys;
    while (!text.empty()) {
        const size_t end = text.find(L'\n');
        if (const std::wstring_view key = text.substr(0, end); !key.empty()) {
            keys.emplace_back(key);
        }
        if (end == std::wstring_view::npos) {
            break;
        }
        text.remove_prefix(end + 1);
    }
    return keys;
}

std::wstring EncodeTrayKeys(const std::vector<std::wstring>& keys) {
    std::wstring text;
    for (const auto& key : keys) {
        text += (text.empty() ? L"" : L"\n") + key;
    }
    return text;
}

// "anchor<TAB>key" lines.
std::vector<std::pair<std::wstring, int>> DecodeTrayAnchors(
    std::wstring_view text) {
    std::vector<std::pair<std::wstring, int>> anchors;
    for (const auto& line : DecodeTrayKeys(text)) {
        const size_t tab = line.find(L'\t');
        const int anchor = _wtoi(line.c_str());
        // (Beside the tray's own items: with the tray.)
        if (tab != std::wstring::npos && anchor != -1 &&
            anchor != (int)PillGroup::Tray &&
            anchor != TrayAnchorBefore(PillGroup::Tray) &&
            anchor >= TrayAnchorBefore((PillGroup)(kPillGroups - 1)) &&
            anchor < kPillGroups) {
            anchors.push_back({line.substr(tab + 1), anchor});
        }
    }
    return anchors;
}

std::wstring EncodeTrayAnchors(
    const std::vector<std::pair<std::wstring, int>>& anchors) {
    std::wstring text;
    for (const auto& [key, anchor] : anchors) {
        text += (text.empty() ? L"" : L"\n") + std::to_wstring(anchor) + L"\t" +
                key;
    }
    return text;
}

void LoadTrayArrangement() {
    g_trayPinned = DecodeTrayKeys(GetWholeStringValue(L"trayPinned", 8192));
    g_trayOrder = DecodeTrayKeys(GetWholeStringValue(L"trayOrder", 8192));
    g_trayAnchors =
        DecodeTrayAnchors(GetWholeStringValue(L"trayAnchors", 8192));
}

void SaveTrayArrangement() {
    StoreProfileString(L"trayPinned", EncodeTrayKeys(g_trayPinned).c_str());
    StoreProfileString(L"trayOrder", EncodeTrayKeys(g_trayOrder).c_str());
    StoreProfileString(L"trayAnchors",
                       EncodeTrayAnchors(g_trayAnchors).c_str());
}

// Another app's tray icon dragged to another place (on the pill, or in the
// list of the ones not pinned): where it would go.
struct TrayDrop {
    // The island it goes on (none: back where it was).
    Island* island = nullptr;
    bool pinned = false;
    int anchor = -1;
    // Put before this icon, or after `after`, or after the ones there.
    std::wstring before;
    std::wstring after;
    // The line showing where, in that island's window.
    D2D1_RECT_F mark{};
};

// The icon held (see Island::DragTrayIcon and TrayListPanel::DragIcon): its
// key, the island whose icons they are, from its panel's list or not, where
// it was pressed, and whether it moved far enough to be dragged.
struct TrayDrag {
    std::wstring key;
    Island* owner = nullptr;
    bool fromList = false;
    POINT from{};
    bool moved = false;
    TrayDrop drop;
};
TrayDrag g_trayDrag;

// The notches (the Fixed style's, see islandNotches): one in the middle,
// two at the screen's corners, or three. Each group goes in the left (0),
// middle (1) or right (2) one, kept as a figure per group in
// "islandNotchSides"; with two, the middle's go in the left one; with one,
// all of them in it. In each notch, the groups keep the pill's order.
constexpr int kMaxNotches = 3;
uint8_t g_notchSides[kPillGroups];

// The media's parts (new groups, see PillGroup::MediaTitle) in the note's
// notch, for sides kept before them (`length`: how many were kept).
void FollowMediaNotch(size_t length) {
    for (PillGroup part : {PillGroup::MediaTitle, PillGroup::MediaButtons}) {
        if ((size_t)part >= length) {
            g_notchSides[(int)part] = g_notchSides[(int)PillGroup::Media];
        }
    }
}

uint8_t DefaultNotchSide(PillGroup group) {
    switch (group) {
        case PillGroup::Clock:
        case PillGroup::Bell:
        case PillGroup::Media:
            return 1;
        case PillGroup::Apps:
        case PillGroup::Usage:
        case PillGroup::Notes:
        case PillGroup::Tasks:
        case PillGroup::Shortcuts:
        case PillGroup::Weather:
        case PillGroup::Updates:
        case PillGroup::Profiles:
        case PillGroup::Desktops:
        case PillGroup::Folders:
        case PillGroup::Wallpaper:
        case PillGroup::KomorebiLayout:
        case PillGroup::FocusedApp:
        case PillGroup::WorldClocks:
        case PillGroup::Countdown:
            return 0;
        default:
            return 2;
    }
}

void LoadNotchSides() {
    const std::wstring value =
        GetWholeStringValue(L"islandNotchSides", kPillGroups + 1);
    const size_t length = value.size();
    for (int i = 0; i < kPillGroups; i++) {
        const WCHAR c = (size_t)i < length ? value[i] : 0;
        g_notchSides[i] = c >= L'0' && c <= L'2'
                              ? (uint8_t)(c - L'0')
                              : DefaultNotchSide((PillGroup)i);
    }
    FollowMediaNotch(length);
}

void SaveNotchSides() {
    std::wstring text;
    for (int i = 0; i < kPillGroups; i++) {
        text += (WCHAR)(L'0' + g_notchSides[i]);
    }
    StoreProfileString(L"islandNotchSides", text.c_str());
}

// How many notches the island has (none: not the Fixed style, or its pill).
int NotchCountWanted() {
    return g_settings.islandBar       ? 0
           : g_settings.islandReserve ? g_settings.islandNotches
                                      : g_settings.islandPillNotches;
}

// The places, icons and order per app: only for the pill (the Fixed style
// keeps its strip, the bar its own look).
bool PerAppPlacesOn() {
    return !g_settings.islandBar && !g_settings.islandReserve &&
           NotchCountWanted() == 0;
}

// Where the notches are (0 left, 1 middle, 2 right), left to right.
std::vector<int> NotchPlaces(int count) {
    return count >= 3   ? std::vector<int>{0, 1, 2}
           : count == 2 ? std::vector<int>{0, 2}
           : count == 1 ? std::vector<int>{1}
                        : std::vector<int>{};
}

// The notch a group is in, with that many.
int NotchOfGroup(PillGroup group, int count) {
    const int side = g_notchSides[(int)group];
    return count <= 1 ? 1 : count == 2 ? (side == 2 ? 2 : 0) : side;
}

void SetPillOrder(const std::vector<PillGroup>& order);

// How big the notches' curves are, in percent: the round corners under them
// (a quarter of their height times the strength, at 100%), and the curves
// beside them, growing faster (wide and sweeping at 200%). The strength
// makes 100% half again as big as a quarter of their height (the default,
// 75%, a little more than that quarter).
constexpr int kNotchCurveMin = 0;
constexpr int kNotchCurveMax = 200;
constexpr int kNotchCurveDefault = 75;
constexpr int kNotchCurveStep = 5;
constexpr float kNotchCurveStrength = 1.5f;

float NotchCurveAmount() {
    return g_settings.islandNotchCurve / 100.0f * kNotchCurveStrength;
}

float NotchRadius(float height) {
    return std::round(height / 4 * NotchCurveAmount());
}

float NotchShoulder(float height) {
    const float amount = NotchCurveAmount();
    return std::round(height / 4 * amount * amount);
}

// The island's size, in percent: the slider's ends, and the size it's
// restored to.
constexpr int kIslandSizeMin = 70;
constexpr int kIslandSizeMax = 150;
constexpr int kIslandSizeDefault = 100;
constexpr int kIslandSizeStep = 5;
// The icons' own size, in percent (see islandIconSize).
constexpr int kIconSizeMin = 70;
constexpr int kIconSizeMax = 150;
constexpr int kIconSizeDefault = 100;
constexpr int kIconSizeStep = 5;
// The pill's texts' own size, in percent (see islandTextSize; a profile
// saved before it came has its icons' size).
constexpr int kTextSizeMin = 70;
constexpr int kTextSizeMax = 150;
constexpr int kTextSizeDefault = 100;
constexpr int kTextSizeStep = 5;
constexpr int kIslandGapMin = 0;
constexpr int kIslandGapMax = 48;
constexpr int kIslandGapDefault = 6;
constexpr int kIslandGapStep = 2;
// How long the mouse is away before the island hides as its line (tenths of
// a second), not following the animations' speed.
constexpr int kAutoHideDelayMin = 2;
constexpr int kAutoHideDelayMax = 50;
constexpr int kAutoHideDelayDefault = 8;
constexpr int kAutoHideDelayStep = 1;
// The minimized line's opacity, in percent (0: unseen, see
// islandLineOpacity).
constexpr int kLineOpacityMin = 0;
constexpr int kLineOpacityMax = 100;
constexpr int kLineOpacityDefault = 100;
constexpr int kLineOpacityStep = 10;
// The strip along the edge that brings the minimized line back (px at
// 100%): at least the line under the mouse and a margin on each side.
constexpr int kActivationWidthMin = 120;
constexpr int kActivationWidthMax = 600;
constexpr int kActivationWidthDefault = 120;
constexpr int kActivationWidthStep = 20;
// The panels' size, in percent of the screen's scaling (a 4K screen at
// 100%: the settings' text was small).
constexpr int kPanelSizeMin = 100;
constexpr int kPanelSizeMax = 150;
constexpr int kPanelSizeDefault = 100;
constexpr int kPanelSizeStep = 10;
// The widest the playing title gets on the pill (px at 100%), cut or
// scrolling past it.
constexpr int kMediaTitleLimitMin = 100;
constexpr int kMediaTitleLimitMax = 400;
constexpr int kMediaTitleLimitDefault = 150;
constexpr int kMediaTitleLimitStep = 10;
// The pill's height (and the Fixed style's room, the notches'), how round
// its corners are (100: round ends) and the room at its ends, in % of the
// usual.
constexpr int kPillHeightPctMin = 70;
constexpr int kPillHeightPctMax = 130;
constexpr int kPillHeightPctDefault = 100;
constexpr int kPillHeightPctStep = 5;
constexpr int kPillCornersMin = 0;
constexpr int kPillCornersMax = 100;
constexpr int kPillCornersDefault = 100;
constexpr int kPillCornersStep = 10;
constexpr int kPillEndsMin = 50;
constexpr int kPillEndsMax = 200;
constexpr int kPillEndsDefault = 100;
constexpr int kPillEndsStep = 10;
constexpr int kEdgeMin = 0;
constexpr int kEdgeMax = 100;
constexpr int kEdgeDefault = 100;
constexpr int kEdgeStep = 10;
// More blur on the glass (see kGlassBlurMost).
constexpr int kGlassBlurMin = 0;
constexpr int kGlassBlurMax = 100;
constexpr int kGlassBlurDefault = 0;
constexpr int kGlassBlurStep = 10;
// The tray's grid of icons (see TrayListPanel).
constexpr int kTrayColumnsMin = 2;
constexpr int kTrayColumnsMax = 12;
constexpr int kTrayColumnsDefault = 6;
constexpr int kTrayColumnsStep = 1;
// The visualizer's sensitivity (see Island::StepVisualizer).
constexpr int kVizSensitivityMin = 25;
constexpr int kVizSensitivityMax = 300;
constexpr int kVizSensitivityDefault = 100;
constexpr int kVizSensitivityStep = 25;

// The settings view's sections (see ControlPanel::SettingEntry), in their
// tabs' order (see SettingSectionTab).
enum SettingSection {
    kSectionProfiles,
    kSectionBackup,
    kSectionLanguage,
    kSectionStyle,
    kSectionPlace,
    // When and how it becomes the line (3.0: its own tab).
    kSectionHide,
    kSectionColors,
    kSectionGlass,
    kSectionShape,
    kSectionLook,
    // The Bar style's side, inner corners and items at the screen's corners.
    kSectionBar,
    kSectionDetails,
    kSectionIcons,
    kSectionAnimations,
    kSectionSystem,
    // The pill filling like a loading bar (downloads, the battery).
    kSectionFill,
    kSectionDesktop,
    kSectionTools,
    kSectionActions,
    kSectionTray,
    kSectionMedia,
    kSectionClock,
    kSectionNotifications,
    kSectionBanners,
    kSectionMonitors,
    // The pages about the mod (see SettingKind::Dock), and the setup
    // assistant (see SettingKind::Setup).
    kSectionDock,
    kSectionAbout,
    kSectionSetup,
    kSectionWhatsNew,
    kSectionLogs,
    kSectionCount
};

// The settings' tabs, in the window's side bar: the profiles, the backup
// and the language; the style (with what only it has); the place; hiding;
// the look; the icons; the animations; the system's items
// (with the loading bar); the desktop's; the apps and tools; the control
// center's buttons; the tray icons; what's playing; the clock; the
// notifications; the banners; the monitors (with two or more); and the
// pages about the mod: the dock (set up in Windhawk), about, the setup
// assistant, what's new, and the logs.
enum SettingTab {
    kTabGeneral,
    kTabStyle,
    kTabPlace,
    kTabHide,
    kTabLook,
    kTabIcons,
    kTabAnimations,
    kTabSystem,
    kTabDesktop,
    kTabTools,
    kTabControlCenter,
    kTabTray,
    kTabMedia,
    kTabClock,
    kTabNotifications,
    kTabBanners,
    kTabMonitors,
    kTabDock,
    kTabAbout,
    kTabSetup,
    kTabWhatsNew,
    kTabLogs,
    kTabCount
};

bool SettingTabShown(int tab) {
    return tab >= 0 && tab < kTabCount &&
           (tab != kTabMonitors || GetSystemMetrics(SM_CMONITORS) > 1);
}

int SettingSectionTab(int section) {
    static constexpr int kTabs[kSectionCount] = {
        kTabGeneral,       kTabGeneral,  kTabGeneral, kTabStyle,
        kTabPlace,         kTabHide,     kTabLook,    kTabLook,
        kTabLook,          kTabLook,     kTabLook,    kTabLook,
        kTabIcons,         kTabAnimations, kTabSystem,
        kTabSystem,        kTabDesktop,  kTabTools,   kTabControlCenter,
        kTabTray,          kTabMedia,    kTabClock,   kTabNotifications,
        kTabBanners,       kTabMonitors, kTabDock,    kTabAbout,
        kTabSetup,         kTabWhatsNew, kTabLogs};
    return kTabs[std::clamp(section, 0, kSectionCount - 1)];
}

WCHAR SettingTabGlyph(int tab) {
    static constexpr WCHAR kGlyphs[kTabCount] = {
        0xE713, 0xE8A1, 0xE81E, 0xED1A, 0xE771, 0xE91B, 0xE9E9,
        0xE770, 0xE7C4, 0xEC7A, kGlyphControlCenter, 0xE712, 0xE8D6, 0xE823,
        0xEA8F, 0xE8BD, 0xE7F4, 0xE75B, 0xE946, 0xE82F, 0xE789, 0xE9F9};
    return kGlyphs[std::clamp(tab, 0, kTabCount - 1)];
}

// The Monitors page's parts' heights (DIPs): where the island appears and
// the map; the main island's card; another island's card without its
// profiles (38 a line of three).
constexpr float kMonitorsTop = 372;
constexpr float kMonitorsMainCard = 96;
constexpr float kMonitorsCardBase = 138;

// The settings rows with a few choices.
enum ChoiceId {
    kChoiceAnimationLevel,
    kChoiceFrameRate,
    // Two buttons, none chosen: exporting and importing a backup.
    kChoiceBackup,
    // Three buttons, none chosen: the custom colors (see CustomColor).
    kChoiceColors,
    kChoiceDateOrder,
    kChoiceClockFormat,
    kChoiceTrayStyle,
    // How a wallpaper comes in (see WallpaperTransition).
    kChoiceWallpaper,
    // The Fixed style's notches (see islandNotches).
    kChoiceNotches,
    // Files dragged out of the folders (see islandFolderDrag).
    kChoiceFolderDrag,
    // The island's texts' language (see g_textLanguage).
    kChoiceLanguage,
    // The pill's fill's colors: downloads and other apps, file copies, the
    // battery charging (see FillChoiceKind).
    kChoiceFillColor,
    kChoiceFillCopyColor,
    kChoiceFillChargeColor,
    // How the fill moves (see islandFillStyle).
    kChoiceFillStyle,
    // The song lyrics' size and color on the pill (see islandLyricsSize).
    kChoiceLyricsSize,
    kChoiceLyricsColor,
    // The audio visualizer's color (see islandVisualizerColor).
    kChoiceVisualizerColor,
    // From how much in it the Recycle Bin shows (islandRecycleMinSize).
    kChoiceRecycleMinSize,
    // The longest a song's line gets on the pill (islandLyricsWidth).
    kChoiceLyricsWidth,
    // The language What's new shows its notes in (see g_notesLanguage).
    kChoiceNotesLanguage,
    // °C or °F on the weather (see weatherUnit).
    kChoiceWeatherUnit,
    // Which notch's line opens the island (see islandNotchOpen).
    kChoiceNotchOpen,
    // When it hides as its line (see islandHideWhen).
    kChoiceHideWhen,
    // When its line fades away (see islandLineFade).
    kChoiceLineFade,
    // What opens the line (see islandLineClick).
    kChoiceLineClick,
    // The bar's edge and its inner corners (see islandBarSide and
    // islandBarInnerCorners).
    kChoiceBarSide,
    kChoiceBarInnerCorners,
    // The banner's size, width, corners and place (see islandBannerSize).
    kChoiceBannerSize,
    kChoiceBannerWidth,
    kChoiceBannerCorners,
    kChoiceBannerPlace,
    // How many files the shelf keeps (islandShelfLimit).
    kChoiceShelfLimit,
    // The icon pack (islandIconPack), its pictures' colors
    // (islandIconColors), and the folder's actions (choose it, open it, save
    // Windows' icons into it, see g_iconFolder).
    kChoiceIconPack,
    kChoiceIconColors,
    kChoiceIconFolder,
    // The setup assistant's own (see SetupShows): the style as pictures, a
    // set of the pill's items (see kSetupItems), hiding as the line (the
    // auto-hide and when, together) and the colors.
    kChoiceSetupStyle,
    kChoiceSetupItems,
    kChoiceSetupHide,
    kChoiceSetupTheme,
    // The dragon eyes' colors and shape (islandEyeTheme, islandEyeShape).
    kChoiceEyeTheme,
    kChoiceEyeShape,
    kChoiceCount
};

// A fill color's row's kind (kFillDownloads...), -1 for the other rows.
int FillChoiceKind(int id) {
    return id == kChoiceFillColor         ? kFillDownloads
           : id == kChoiceFillCopyColor   ? kFillCopies
           : id == kChoiceFillChargeColor ? kFillCharge
                                          : -1;
}

int MonitorCount();
bool IsSharedPref(IslandPref pref);
void SaveShownInProfile(int id);
// The monitor chosen for the island (when it doesn't follow the mouse), as
// an index in MonitorCount's order.
int ChosenMonitorIndex();
void ChooseMonitor(int index);

// The style row's segments, in order: the pill, the pill keeping its room,
// the bar; the style (IslandPref::Bar) of a segment, and the segment of a
// style (the same swap both ways).
int StyleSegmentStyle(int segment) {
    return segment == 1 ? 2 : segment == 2 ? 1 : 0;
}

// Files dragged out of the folders panel and the shelf (islandFolderDrag):
// 0 asked the first time (the answer remembered unless unticked), 1 copied,
// 2 moved, 3 asked every time. Shown in that order: the asks first.
constexpr int kFolderDragAskAlways = 3;
constexpr int kFolderDragShown[] = {0, kFolderDragAskAlways, 1, 2};

bool FolderDragAsks(int choice) {
    return choice == 0 || choice == kFolderDragAskAlways;
}

// The pill's items the setup assistant's sets switch (kChoiceSetupItems):
// Essential the first ones, Everyday more, Everything all of them; each
// set's items on and the others of the list off (the rest as they are).
constexpr IslandPref kSetupItems[] = {
    // Essential.
    IslandPref::Clock, IslandPref::Battery, IslandPref::Network,
    IslandPref::Volume, IslandPref::Bell, IslandPref::Media,
    // Everyday.
    IslandPref::Countdown, IslandPref::Weather, IslandPref::Timer,
    IslandPref::Folders, IslandPref::NotificationApps, IslandPref::MediaTitle,
    // Everything.
    IslandPref::ThemeButton, IslandPref::Microphone, IslandPref::Notes,
    IslandPref::Tasks, IslandPref::Shortcuts, IslandPref::Brightness,
    IslandPref::InputMethod, IslandPref::Privacy, IslandPref::UsbDrives,
    IslandPref::Desktops, IslandPref::NetSpeed, IslandPref::RecycleBin,
    IslandPref::Visualizer, IslandPref::MediaButtons, IslandPref::Power};
constexpr size_t kSetupItemSets[] = {6, 12, std::size(kSetupItems)};

// The set the pill's items are now (see kSetupItems), or the choice after
// the sets ("Keep as it is") when they're none of them.
int SetupItemSet() {
    for (size_t set = 0; set < std::size(kSetupItemSets); set++) {
        bool same = true;
        for (size_t i = 0; i < std::size(kSetupItems); i++) {
            same = same && (GetIslandPref(kSetupItems[i]) != 0) ==
                               (i < kSetupItemSets[set]);
        }
        if (same) {
            return (int)set;
        }
    }
    return (int)std::size(kSetupItemSets);
}

// Every option's value now, by IslandPref (see SetupState).
std::vector<int> IslandPrefValues() {
    std::vector<int> values(std::size(kIslandPrefs));
    for (const auto& info : kIslandPrefs) {
        values[(int)info.pref] = GetIslandPref(info.pref);
    }
    return values;
}

int ChoiceCount(int id) {
    switch (id) {
        case kChoiceAnimationLevel:
            return 3;
        case kChoiceFrameRate:
            return 4;
        case kChoiceBackup:
            return 3;
        case kChoiceColors:
            return kCustomColors;
        case kChoiceDateOrder:
            return 4;
        case kChoiceClockFormat:
        case kChoiceWeatherUnit:
        case kChoiceTrayStyle:
        case kChoiceIconFolder:
            return 3;
        case kChoiceFolderDrag:
            return (int)std::size(kFolderDragShown);
        case kChoiceIconPack:
            return kIconPacks;
        case kChoiceLineClick:
        case kChoiceIconColors:
            return 2;
        case kChoiceLanguage:
            return 1 + kTextLanguages;
        case kChoiceNotesLanguage:
            return kTextLanguages;
        case kChoiceHideWhen:
        case kChoiceBarSide:
            return 3;
        case kChoiceWallpaper:
        case kChoiceNotches:
        case kChoiceNotchOpen:
        case kChoiceLineFade:
        case kChoiceBarInnerCorners:
        case kChoiceFillColor:
        case kChoiceFillCopyColor:
        case kChoiceFillChargeColor:
            return 4;
        case kChoiceVisualizerColor:
            return 5;
        case kChoiceRecycleMinSize:
            return (int)std::size(kRecycleMinMegabytes);
        case kChoiceFillStyle:
        case kChoiceLyricsSize:
        case kChoiceLyricsColor:
            return 3;
        case kChoiceLyricsWidth:
            return 4;
        case kChoiceBannerSize:
            return (int)std::size(kBannerSizes);
        case kChoiceShelfLimit:
            return (int)std::size(kShelfLimits);
        case kChoiceEyeTheme:
            return (int)std::size(kEyeThemes);
        case kChoiceEyeShape:
            return (int)std::size(kEyeShapes);
        case kChoiceBannerWidth:
            return (int)std::size(kBannerWidths);
        case kChoiceBannerCorners:
            return (int)std::size(kBannerCornerShares);
        case kChoiceBannerPlace:
            return kBannerPlaces;
        case kChoiceSetupStyle:
        case kChoiceSetupTheme:
            return 3;
        case kChoiceSetupItems:
            return (int)std::size(kSetupItemSets) + 1;
        case kChoiceSetupHide:
            // (Only while a window is maximized: the Island style without
            // notches, see kChoiceHideWhen.)
            return !g_settings.islandReserve && NotchCountWanted() == 0 ? 3
                                                                        : 2;
        default:
            return 0;
    }
}

// Whether the icons' colors choice (islandIconColors) changes anything: the
// emojis', the folder's pictures, or ones chosen for single icons.
bool IconColorsMatter() {
    return g_settings.islandIconPack == kIconPackEmoji ||
           g_settings.islandIconPack == kIconPackFolder ||
           !g_iconFiles.files.empty();
}

// The rows a choice's segments are in: the languages (nine, and the notes'
// eight) and the visualizer's five colors in two rows of buttons, the icon
// packs a list (one a row), the others in one.
int ChoiceRows(int id) {
    return id == kChoiceIconPack ? kIconPacks
           : id == kChoiceLanguage || id == kChoiceNotesLanguage ||
                   id == kChoiceVisualizerColor
               ? 2
               : 1;
}

// What an icon pack's row in the list says beside its name: whose it is,
// its license and how many of the icons it has (an online one's), or what
// it is.
std::wstring IconPackDetail(int pack) {
    switch (pack) {
        case kIconPackWindows:
            return Tr(L"Windows' own icons", L"Os ícones do próprio Windows");
        case kIconPackWindows10:
            return Tr(L"Windows 10's, where it has them",
                      L"Os do Windows 10, onde ele tem");
        case kIconPackEmoji:
            return L"Segoe UI Emoji";
        case kIconPackFolder:
            return L"PNG, SVG, ICO…";
    }
    const OnlineIconPack* online = FindOnlineIconPack(pack);
    if (!online) {
        return {};
    }
    const auto has = std::count_if(
        std::begin(kIcons), std::end(kIcons),
        [&](const IconInfo& icon) { return icon.*online->name != nullptr; });
    return std::wstring(online->by) + L" · " + online->license +
           L" · " + std::to_wstring(has) + L"/" +
           std::to_wstring(std::size(kIcons));
}

// "Reset" beside the backup (see ResetIslandSettings): when it was clicked
// first; a second click within kResetAskSeconds resets.
double g_resetAskedAt = -100;
constexpr double kResetAskSeconds = 4;
