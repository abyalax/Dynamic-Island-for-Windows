#pragma once
// The island's sliders' values (0 size, 1 room from the edges, 2 animation
// speed, 3 tray icons, 4 the backgrounds' opacity, 5 the auto-hide's delay,
// 6 the notches' curves, 7 the icons' size, 8 the minimized line's
// opacity, 9 the strip that brings it back, 10 the panels' size, 11 the
// playing title's width, 12 the pill's height, 13 its corners, 14 the room at
// its ends, 15 the edge around, 16 the glass's blur, 17 the tray grid's
// columns, 18 the visualizer's sensitivity, 19 the texts' size).
constexpr int kIslandSliderCount = 20;
int IslandSliderValue(int slider);

// How many tray icons the open list shows before it scrolls (see
// Island::m_trayScroll), chosen in the island's settings.
constexpr int kTrayListMin = 4;
constexpr int kTrayListMax = 40;
constexpr int kTrayListDefault = 12;
constexpr int kTrayListStep = 1;
int g_trayListIcons = kTrayListDefault;

void LoadTrayListSetting() {
    int value = Wh_GetIntValue(L"trayListIsland", 0);
    if (value <= 0) {
        // Up to 1.0 it was a Windhawk setting: its value, if Windhawk still
        // has it, is kept as the island's once.
        value = Wh_GetIntSetting(L"trayListIcons");
        value = value > 0 ? value : kTrayListDefault;
        Wh_SetIntValue(L"trayListIsland", value);
    }
    g_trayListIcons = std::clamp(value, kTrayListMin, kTrayListMax);
}

void SetTrayListIcons(int icons, bool keep);

// The media buttons on the pill (the setting, switched in the media panel).
std::atomic<bool> g_mediaButtonsInPill{false};
// What's playing on the pill: its cover and its title, instead of the note.
std::atomic<bool> g_mediaTitleInPill{false};

// The clock's parts shown for all apps (bits as IslandPlace::clock).
int GlobalClockParts();
// The per-app settings' chips (each app's taking kAppChipStride): its
// groups of icons, what's playing's buttons and its cover and title, the
// clock's parts, and its room from the edges and size (each its row, its
// slider, and a button back to the one for all apps).
constexpr int kMediaChipFirst = 64;
constexpr int kClockChipFirst = 66;
constexpr int kClockParts = 4;
// After the clock's parts: the clock kept in the middle.
constexpr int kClockCenterChip = kClockChipFirst + kClockParts;
constexpr int kStepChipFirst = 72;
constexpr int kAppChipStride = 80;
static_assert(kPillGroups <= 64 && kPillGroups <= kMediaChipFirst &&
              kMediaChipFirst + 2 <= kClockChipFirst &&
              kClockCenterChip < kStepChipFirst &&
              kStepChipFirst + 6 <= kAppChipStride);

// The groups turned off for all apps (bits by PillGroup).
uint64_t GloballyHiddenGroups() {
    uint64_t mask = 0;
    for (int group = 0; group < kPillGroups; group++) {
        // (What's playing's group is its note: its buttons and its cover and
        // title are chosen on their own, see Island::MediaPartWanted.)
        const int pref = PillGroupPref((PillGroup)group);
        if (pref >= 0 && !GetIslandPref((IslandPref)pref)) {
            mask |= 1ull << group;
        }
        if ((PillGroup)group == PillGroup::Usage && !AnyUsageShown()) {
            mask |= 1ull << group;
        }
    }
    return mask;
}

// Where the pill sits on its monitor: fractions of the room it can move in
// (x: 0 left, 0.5 middle, 1 right; y: 0 top, 0.5 middle, 1 bottom). One for
// all apps, and others for some apps, used while they're in front.
struct IslandPlace {
    std::wstring appId;
    double x = 0.5;
    double y = 0;
    // For an app: the groups of items it hides while it's in front (bits
    // by PillGroup). Once its icons are chosen (`own`), they're all its
    // own: the ones not hidden show even if they're off for all apps.
    uint64_t hidden = 0;
    bool own = false;
    // How many groups there were when its own choice was kept: the ones
    // added since follow the choice for all apps (see LoadPlaces).
    int known = kPillGroups;
    // Its own clock (bits: 1 seconds, 2 the day of the week, 4 the day and
    // month, 8 the year), or -1 for the one of all apps.
    int clock = -1;
    // What's playing: its buttons (kAppMediaButtons) and its cover and title
    // (kAppMediaTitle) on the pill, or -1 as for all apps.
    int media = -1;
    // Its own room from the screen's edges (pixels) and size of the island
    // (percent), or -1 as for all apps.
    int gap = -1;
    int size = -1;
    // The clock kept in the middle (1) or not (0) for it, or -1 as for all
    // apps.
    int center = -1;
    // Its own order of the pill's items, or empty as for all apps.
    std::vector<PillGroup> order;
};
constexpr int kAppMediaButtons = 1;
constexpr int kAppMediaTitle = 2;
IslandPlace g_islandPlace;
std::vector<IslandPlace> g_appPlaces;
// With several monitors, the place for all apps is kept for each (by its
// device name): the island on another monitor takes that one's (see
// UseMonitorPlace), or the last one used on a monitor without its own.
struct MonitorPlace {
    std::wstring device;
    double x;
    double y;
};
std::vector<MonitorPlace> g_monitorPlaces;
// The monitor g_islandPlace is for (the island's).
std::wstring g_placeMonitor;
constexpr size_t kMaxMonitorPlaces = 8;
constexpr size_t kMaxAppPlaces = 24;

IslandPlace* FindAppPlace(const std::wstring& appId) {
    if (appId.empty()) {
        return nullptr;
    }
    for (auto& place : g_appPlaces) {
        if (_wcsicmp(place.appId.c_str(), appId.c_str()) == 0) {
            return &place;
        }
    }
    return nullptr;
}

// The clock kept in the middle for an app (its own choice, or the one for
// all apps).
bool PlaceClockCenter(const IslandPlace* place) {
    return place && place->center >= 0 ? place->center != 0
                                       : g_settings.islandClockCenter;
}

// An app's order of the pill's items: its own (complete, see
// DecodePillOrder), or the one for all apps.
const std::vector<PillGroup>& PlaceOrder(const IslandPlace& place) {
    return place.order.empty() ? g_pillOrder : place.order;
}

void LoadPlaces() {
    g_islandPlace.x = std::clamp(Wh_GetIntValue(L"placeX", 500), 0, 1000) / 1000.0;
    g_islandPlace.y = std::clamp(Wh_GetIntValue(L"placeY", 0), 0, 1000) / 1000.0;
    // Per monitor: x, y (thousandths) and the device, one per line.
    g_monitorPlaces.clear();
    {
        const std::wstring monitors =
            GetWholeStringValue(L"monitorPlaces", 2048);
        for (PCWSTR line = monitors.c_str(); *line;) {
            PCWSTR end = wcschr(line, L'\n');
            const std::wstring text =
                end ? std::wstring(line, end) : std::wstring(line);
            const size_t tab1 = text.find(L'\t');
            const size_t tab2 = tab1 == std::wstring::npos
                                    ? tab1
                                    : text.find(L'\t', tab1 + 1);
            if (tab2 != std::wstring::npos && tab2 + 1 < text.size() &&
                g_monitorPlaces.size() < kMaxMonitorPlaces) {
                g_monitorPlaces.push_back(
                    {text.substr(tab2 + 1),
                     std::clamp(_wtoi(text.c_str()), 0, 1000) / 1000.0,
                     std::clamp(_wtoi(text.c_str() + tab1 + 1), 0, 1000) /
                         1000.0});
            }
            if (!end) {
                break;
            }
            line = end + 1;
        }
    }
    // Still on its monitor (the settings were read again).
    for (const auto& place : g_monitorPlaces) {
        if (place.device == g_placeMonitor) {
            g_islandPlace.x = place.x;
            g_islandPlace.y = place.y;
        }
    }
    g_appPlaces.clear();
    const std::wstring value = GetWholeStringValue(L"appPlaces", 16 * 1024);
    // One per line: x, y (thousandths), optionally "h" and the hidden
    // groups, then the app's ID (tab-separated).
    for (PCWSTR line = value.data(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        const std::wstring text =
            end ? std::wstring(line, end) : std::wstring(line);
        const size_t tab1 = text.find(L'\t');
        const size_t tab2 =
            tab1 == std::wstring::npos ? tab1 : text.find(L'\t', tab1 + 1);
        if (tab2 != std::wstring::npos && tab2 + 1 < text.size() &&
            g_appPlaces.size() < kMaxAppPlaces) {
            IslandPlace place;
            place.x = std::clamp(_wtoi(text.c_str()), 0, 1000) / 1000.0;
            place.y =
                std::clamp(_wtoi(text.c_str() + tab1 + 1), 0, 1000) / 1000.0;
            place.appId = text.substr(tab2 + 1);
            // Before the ID: "a" its own choice of icons ("h" only some
            // hidden, before) and "k" how many groups there were then, "c"
            // its own clock, "e" its clock in the middle or not, "m" what's
            // playing, "g" its room from the edges, "s" its size, "o" its
            // order (as EncodePillOrder).
            bool knownKept = false;
            for (size_t tab3 = place.appId.find(L'\t');
                 place.appId.size() > 1 &&
                 wcschr(L"hacemgsok", place.appId[0]) &&
                 (iswdigit(place.appId[1]) || place.appId[0] == L'o') &&
                 tab3 != std::wstring::npos;
                 tab3 = place.appId.find(L'\t')) {
                const int number = _wtoi(place.appId.c_str() + 1);
                if (place.appId[0] == L'c') {
                    place.clock = std::clamp(number, 0, 15);
                } else if (place.appId[0] == L'e') {
                    place.center = number ? 1 : 0;
                } else if (place.appId[0] == L'o') {
                    place.order = DecodePillOrder(
                        std::wstring_view(place.appId).substr(1, tab3 - 1));
                } else if (place.appId[0] == L'm') {
                    place.media =
                        number & (kAppMediaButtons | kAppMediaTitle);
                } else if (place.appId[0] == L'g') {
                    place.gap =
                        std::clamp(number, kIslandGapMin, kIslandGapMax);
                } else if (place.appId[0] == L's') {
                    place.size =
                        std::clamp(number, kIslandSizeMin, kIslandSizeMax);
                } else if (place.appId[0] == L'k') {
                    place.known = std::clamp(number, 0, kPillGroups);
                    knownKept = true;
                } else {
                    place.own = place.appId[0] == L'a';
                    place.hidden =
                        _wcstoui64(place.appId.c_str() + 1, nullptr, 10) &
                        ((1ull << kPillGroups) - 1);
                }
                place.appId.erase(0, tab3 + 1);
            }
            // (Kept before the count was: from 2.1 if one of 2.1's groups is
            // hidden in it, else from 2.0, which had fewer.)
            if (!knownKept) {
                place.known = place.hidden >> (int)PillGroup::NetSpeed
                                  ? (int)PillGroup::AiUsage
                                  : (int)PillGroup::NetSpeed;
            }
            g_appPlaces.push_back(std::move(place));
        }
        if (!end) {
            break;
        }
        line = end + 1;
    }
    // A choice kept before some groups existed: they follow the choice for
    // all apps (hidden when off there), instead of showing for the app just
    // because it never heard of them (#6785, #6759).
    const uint64_t off = GloballyHiddenGroups();
    for (auto& place : g_appPlaces) {
        if (place.own && place.known < kPillGroups) {
            place.hidden |= off & ~((1ull << place.known) - 1);
        }
    }
}

void SavePlaces() {
    StoreProfileInt(L"placeX", (int)std::lround(g_islandPlace.x * 1000));
    StoreProfileInt(L"placeY", (int)std::lround(g_islandPlace.y * 1000));
    if (!g_placeMonitor.empty()) {
        std::erase_if(g_monitorPlaces, [](const MonitorPlace& place) {
            return place.device == g_placeMonitor;
        });
        g_monitorPlaces.insert(g_monitorPlaces.begin(),
                               {g_placeMonitor, g_islandPlace.x,
                                g_islandPlace.y});
        if (g_monitorPlaces.size() > kMaxMonitorPlaces) {
            g_monitorPlaces.resize(kMaxMonitorPlaces);
        }
        std::wstring monitors;
        for (const auto& place : g_monitorPlaces) {
            monitors += std::to_wstring(std::lround(place.x * 1000)) + L"\t" +
                        std::to_wstring(std::lround(place.y * 1000)) + L"\t" +
                        place.device + L"\n";
        }
        StoreProfileString(L"monitorPlaces", monitors.c_str());
    }
    std::wstring value;
    for (const auto& place : g_appPlaces) {
        value +=
            std::to_wstring(std::lround(place.x * 1000)) + L"\t" +
            std::to_wstring(std::lround(place.y * 1000)) + L"\t" +
            (place.own      ? L"a" + std::to_wstring(place.hidden) + L"\t" +
                                  L"k" + std::to_wstring(kPillGroups) + L"\t"
             : place.hidden ? L"h" + std::to_wstring(place.hidden) + L"\t"
                            : std::wstring()) +
            (place.clock >= 0 ? L"c" + std::to_wstring(place.clock) + L"\t"
                              : std::wstring()) +
            (place.center >= 0 ? L"e" + std::to_wstring(place.center) + L"\t"
                               : std::wstring()) +
            (place.media >= 0 ? L"m" + std::to_wstring(place.media) + L"\t"
                              : std::wstring()) +
            (place.gap >= 0 ? L"g" + std::to_wstring(place.gap) + L"\t"
                            : std::wstring()) +
            (place.size >= 0 ? L"s" + std::to_wstring(place.size) + L"\t"
                             : std::wstring()) +
            (place.order.empty()
                 ? std::wstring()
                 : L"o" + EncodePillOrder(place.order) + L"\t") +
            place.appId + L"\n";
    }
    StoreProfileString(L"appPlaces", value.c_str());
}

// The island went to another monitor: its place for all apps is that
// monitor's own, if it has one (otherwise the one it had stays).
void UseMonitorPlace(const std::wstring& device) {
    g_placeMonitor = device;
    for (const auto& place : g_monitorPlaces) {
        if (place.device == device) {
            g_islandPlace.x = place.x;
            g_islandPlace.y = place.y;
        }
    }
}

// The monitors, left to right (then top to bottom); the island's settings
// show each with Windows' own number (see MonitorNumber).
struct MonitorEntry {
    HMONITOR handle;
    std::wstring device;
    RECT rect;
};

BOOL CALLBACK ListMonitorProc(HMONITOR monitor, HDC, LPRECT, LPARAM param) {
    MONITORINFOEX info{};
    info.cbSize = sizeof(info);
    if (GetMonitorInfo(monitor, &info)) {
        ((std::vector<MonitorEntry>*)param)
            ->push_back({monitor, info.szDevice, info.rcMonitor});
    }
    return TRUE;
}

std::vector<MonitorEntry> ListMonitors() {
    std::vector<MonitorEntry> monitors;
    EnumDisplayMonitors(nullptr, nullptr, ListMonitorProc, (LPARAM)&monitors);
    std::sort(monitors.begin(), monitors.end(),
              [](const MonitorEntry& a, const MonitorEntry& b) {
                  return a.rect.left != b.rect.left ? a.rect.left < b.rect.left
                                                    : a.rect.top < b.rect.top;
              });
    return monitors;
}

// The Windows display device's number, independent of its position.
std::wstring MonitorNumber(const std::wstring& device) {
    const size_t at = device.rfind(L"DISPLAY");
    if (at != std::wstring::npos) {
        const std::wstring number = device.substr(at + 7);
        if (!number.empty() &&
            number.find_first_not_of(L"0123456789") == std::wstring::npos) {
            return number;
        }
    }
    return device;
}

int MonitorCount() {
    return GetSystemMetrics(SM_CMONITORS);
}

// The monitor chosen for the island (its device name), or empty for the
// taskbar's (the main one).
std::wstring g_islandMonitor;

// Changed in the settings, or by dragging the pill: kept, and the pill
// slides there (see Island::OnPlacesChanged).
void SetIslandPlace(const std::wstring& appId, double x, double y);

// Windows' dark mode (apps and the system), switched like its settings do:
// the two registry values, then telling the windows.
bool IsDarkMode() {
    DWORD light = 1;
    DWORD size = sizeof(light);
    RegGetValue(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\"
                L"Personalize",
                L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size);
    return light == 0;
}

// The two values are written at once, so quick clicks end in the last one;
// only telling the windows (which may wait for them) is on a thread.
void SetDarkMode(bool dark) {
    const DWORD light = dark ? 0 : 1;
    for (PCWSTR name : {L"AppsUseLightTheme", L"SystemUsesLightTheme"}) {
        RegSetKeyValue(HKEY_CURRENT_USER,
                       L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\"
                       L"Personalize",
                       name, REG_DWORD, &light, sizeof(light));
    }
}

// Windows' transparency effects (Personalization > Colors): without them,
// its blur is a flat color, so the frosted glass shows only while they're on
// (the mod never changes them). Read again when Windows' colors change.
bool g_transparencyEffects = true;

bool TransparencyEffectsOn() {
    DWORD on = 1;
    DWORD size = sizeof(on);
    RegGetValue(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\"
                L"Personalize",
                L"EnableTransparency", RRF_RT_REG_DWORD, nullptr, &on, &size);
    return on != 0;
}

// Tells the windows the colors changed (dark mode, transparency effects).
// The parameter: how long each window has to answer, in milliseconds (none:
// half a second).
DWORD WINAPI DarkModeThreadProc(LPVOID parameter) {
    const UINT timeout = parameter ? (UINT)(UINT_PTR)parameter : 500;
    // Windows that don't answer are skipped.
    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                       (LPARAM)L"ImmersiveColorSet", SMTO_ABORTIFHUNG, timeout,
                       nullptr);
    return 0;
}

// Windows' power mode (Settings > Power: best power efficiency, balanced,
// best performance): an overlay on the balanced power plan, read and set
// with the functions Settings uses (powrprof.dll, looked up: they aren't
// documented). With another plan chosen, or without them, there's none.
constexpr GUID kPowerModes[3] = {
    {0x961cc777, 0x2547, 0x4f9d, {0x81, 0x74, 0x7d, 0x86, 0x18, 0x1b, 0x8a, 0x7a}},
    {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    {0xded574b5, 0x45a0, 0x4f42, {0x87, 0x37, 0x46, 0x34, 0x5c, 0x09, 0xc2, 0x38}}};
// The balanced power plan.
constexpr GUID kBalancedPowerPlan = {
    0x381b4222, 0xf694, 0x41f0, {0x96, 0x85, 0xff, 0x5b, 0xb2, 0x60, 0xdf, 0x2e}};

// powrprof.dll, loaded once and kept: setting the power mode leaves work of
// its own going on in it, and freeing it right after crashed the island
// (an address in no module any more).
HMODULE Powrprof() {
    static const HMODULE module =
        LoadLibraryEx(L"powrprof.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    return module;
}

// The power mode (0 to 2, 3 for one in between, from Windows 10's slider),
// or -1 when there's none.
int GetPowerMode() {
    HMODULE powrprof = Powrprof();
    if (!powrprof) {
        return -1;
    }
    using GetActive_t = DWORD(WINAPI*)(HKEY, GUID**);
    using GetOverlay_t = DWORD(WINAPI*)(GUID*);
    auto getActive = (GetActive_t)GetProcAddress(powrprof, "PowerGetActiveScheme");
    auto getOverlay =
        (GetOverlay_t)GetProcAddress(powrprof, "PowerGetEffectiveOverlayScheme");
    int mode = -1;
    GUID* plan = nullptr;
    if (getActive && getOverlay && getActive(nullptr, &plan) == ERROR_SUCCESS &&
        plan) {
        GUID overlay{};
        if (IsEqualGUID(*plan, kBalancedPowerPlan) &&
            getOverlay(&overlay) == ERROR_SUCCESS) {
            mode = 3;
            for (int i = 0; i < 3; i++) {
                if (IsEqualGUID(overlay, kPowerModes[i])) {
                    mode = i;
                }
            }
        }
    }
    if (plan) {
        LocalFree(plan);
    }
    return mode;
}

// The power plans (Control Panel > Power Options), with their names, and
// the active one's index (or -1): listed and chosen with powrprof's
// documented functions (looked up, like the ones above).
struct PowerPlan {
    GUID guid;
    std::wstring name;
};

std::vector<PowerPlan> GetPowerPlans(int* active) {
    std::vector<PowerPlan> plans;
    *active = -1;
    HMODULE powrprof = Powrprof();
    if (!powrprof) {
        return plans;
    }
    using Enumerate_t = DWORD(WINAPI*)(HKEY, const GUID*, const GUID*, ULONG,
                                       ULONG, UCHAR*, DWORD*);
    using ReadName_t = DWORD(WINAPI*)(HKEY, const GUID*, const GUID*,
                                      const GUID*, UCHAR*, DWORD*);
    using GetActive_t = DWORD(WINAPI*)(HKEY, GUID**);
    auto enumerate = (Enumerate_t)GetProcAddress(powrprof, "PowerEnumerate");
    auto readName =
        (ReadName_t)GetProcAddress(powrprof, "PowerReadFriendlyName");
    auto getActive =
        (GetActive_t)GetProcAddress(powrprof, "PowerGetActiveScheme");
    if (enumerate && readName && getActive) {
        // ACCESS_SCHEME.
        constexpr ULONG kAccessScheme = 16;
        for (ULONG i = 0; i < 64; i++) {
            GUID guid;
            DWORD size = sizeof(guid);
            if (enumerate(nullptr, nullptr, nullptr, kAccessScheme, i,
                          (UCHAR*)&guid, &size) != ERROR_SUCCESS) {
                break;
            }
            WCHAR name[128] = L"";
            DWORD nameSize = sizeof(name);
            if (readName(nullptr, &guid, nullptr, nullptr, (UCHAR*)name,
                         &nameSize) != ERROR_SUCCESS) {
                continue;
            }
            name[ARRAYSIZE(name) - 1] = L'\0';
            plans.push_back({guid, name});
        }
        GUID* current = nullptr;
        if (getActive(nullptr, &current) == ERROR_SUCCESS && current) {
            for (size_t i = 0; i < plans.size(); i++) {
                if (IsEqualGUID(plans[i].guid, *current)) {
                    *active = (int)i;
                }
            }
            LocalFree(current);
        }
    }
    return plans;
}

bool SetPowerPlan(const GUID& guid) {
    HMODULE powrprof = Powrprof();
    if (!powrprof) {
        return false;
    }
    using SetActive_t = DWORD(WINAPI*)(HKEY, const GUID*);
    auto setActive =
        (SetActive_t)GetProcAddress(powrprof, "PowerSetActiveScheme");
    return setActive && setActive(nullptr, &guid) == ERROR_SUCCESS;
}

bool SetPowerMode(int mode) {
    if (mode < 0 || mode > 2) {
        return false;
    }
    HMODULE powrprof = Powrprof();
    if (!powrprof) {
        return false;
    }
    using SetOverlay_t = DWORD(WINAPI*)(GUID);
    auto setOverlay =
        (SetOverlay_t)GetProcAddress(powrprof, "PowerSetActiveOverlayScheme");
    return setOverlay && setOverlay(kPowerModes[mode]) == ERROR_SUCCESS;
}

// A power mode or plan chosen, set on a thread of its own: Windows tells
// every window of the change (the island's too) while it's set, and the
// island's thread waiting for it froze the island (#6652). Its panel is told
// once it's done (WM_APP_POWER_SET, wParam whether it worked) and reads the
// mode again.
constexpr UINT WM_APP_POWER_SET = WM_APP + 106;
struct PowerChange {
    HWND notify;
    int mode;  // 0 to 2, or -1 for `plan`.
    GUID plan;
};
// One at a time: a mode that Windows never finishes setting (the freeze was
// only with the modes, not the plans) doesn't pile threads up with each
// click. Its start is logged, so a log without its end shows it.
std::atomic<bool> g_powerChanging{false};

DWORD WINAPI PowerChangeThreadProc(LPVOID parameter) {
    const std::unique_ptr<PowerChange> change((PowerChange*)parameter);
    const double started = NowSeconds();
    IslandLog(L"Power %s being set", change->mode >= 0 ? L"mode" : L"plan");
    const bool set = change->mode >= 0 ? SetPowerMode(change->mode)
                                       : SetPowerPlan(change->plan);
    IslandLog(L"Power %s set in %.2f s%s",
              change->mode >= 0 ? L"mode" : L"plan", NowSeconds() - started,
              set ? L"" : L" (refused)");
    g_powerChanging = false;
    PostMessage(change->notify, WM_APP_POWER_SET, set, 0);
    return 0;
}

constexpr float kHeaderHeight = 40;
constexpr float kRowHeight = 40;
constexpr float kMixerRowHeight = 46;
constexpr size_t kMaxMixerApps = 8;
constexpr size_t kMaxNetworks = 8;
constexpr size_t kMaxBluetoothDevices = 8;

// The control center's messages: the radios' state (wParam Wi-Fi, lParam
// Bluetooth, see RadioThreadProc), and Wi-Fi events (wParam the event: a
// scan done or Wi-Fi disconnected; a join's end is WM_APP_WLAN_CONNECTION).
constexpr UINT WM_APP_RADIOS = WM_APP + 60;
constexpr UINT WM_APP_WLAN = WM_APP + 61;
// A join finished (lParam: a new WlanConnectionEvent, the receiver's).
constexpr UINT WM_APP_WLAN_CONNECTION = WM_APP + 86;
// The Wi-Fi networks in reach, read on a thread (lParam: a list, see
// ControlPanel::WifiListThreadProc).
constexpr UINT WM_APP_WIFI_LIST = WM_APP + 73;
// Its lParam when Windows refused the list (no location access, see
// NetworkInfo); otherwise the networks found, or none.
constexpr LPARAM kWifiListRefused = 1;
// The mobile hotspot's and airplane mode's state (wParam and lParam, like a
// radio's).
constexpr UINT WM_APP_HOTSPOT = WM_APP + 62;
// The monitors' brightness (wParam, 0 to 100, or -1).
constexpr UINT WM_APP_BRIGHTNESS = WM_APP + 63;
// Each monitor's brightness (see BrightnessListProc).
constexpr UINT WM_APP_BRIGHTNESS_LIST = WM_APP + 88;
// The screenshot shortcut, once the control center is gone.
constexpr UINT WM_APP_SCREENSHOT = WM_APP + 64;
constexpr UINT_PTR kScreenshotTimerId = kPanelScreenshotTimerId;
// Checks night light after a change (see ControlPanel::SwitchNightLight).
constexpr UINT_PTR kNightLightTimerId = 31;
// A Wi-Fi join given up on time (see ControlPanel::CheckConnecting).
constexpr UINT_PTR kConnectingTimerId = 32;
// A Bluetooth device connecting or disconnecting from its row: looked at
// this often, given up after this long, its failure shown this long (see
// ControlPanel::CheckBluetoothStep).
constexpr UINT_PTR kBluetoothTimerId = 33;
constexpr UINT kBluetoothCheckMs = 300;
constexpr ULONGLONG kBluetoothConnectMs = 12000;
constexpr ULONGLONG kBluetoothFailedMs = 4000;
// A Bluetooth device's connection asked for (wParam: whether the device
// took the request).
constexpr UINT WM_APP_BLUETOOTH_DONE = WM_APP + 89;
constexpr UINT kNightLightCheckMs = 800;

// A radio's state for the control center.
constexpr int kRadioUnknown = -1;
constexpr int kRadioMissing = -2;

// Turns the Wi-Fi or Bluetooth radio on or off, like the quick settings do
// (Windows.Devices.Radios), then reports both radios' state to `notify`. The
// calls wait, so they run on a thread of their own.
struct RadioRequest {
    HWND notify;
    // 0: Wi-Fi, 1: Bluetooth, 2: the mobile hotspot, 3: airplane mode, -1:
    // only report.
    int kind;
    bool on;
};

// Airplane mode: the interface the Windows quick settings use (the radio
// management service). It isn't documented, but it has been the same since
// Windows 8 and other tools use it; if it's missing, the switch isn't shown.
struct IRadioManager : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE IsRMSupported(DWORD* state) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetUIRadioInstances(
        IUnknown** instances) = 0;
    // `enabled`: the radios are on (airplane mode is off).
    virtual HRESULT STDMETHODCALLTYPE GetSystemRadioState(int* enabled,
                                                          int* unknown,
                                                          int* reason) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetSystemRadioState(int enabled) = 0;
};

constexpr GUID kCLSID_RadioManagementAPI = {
    0x581333F6,
    0x28DB,
    0x41BE,
    {0xBC, 0x7A, 0xFF, 0x20, 0x1F, 0x12, 0xF3, 0xF6}};
constexpr GUID kIID_IRadioManager = {
    0xDB3AFBFB,
    0x08E6,
    0x46C6,
    {0xAA, 0x70, 0xBF, 0x9A, 0x34, 0xC3, 0x0A, 0xB7}};

// The Wi-Fi radio through the WLAN API, for adapters the radio API doesn't
// list (older drivers): switched on or off first (`set` 1 or 0, -1 not),
// then its state (on when an adapter's radio is on in software and in
// hardware), or kRadioMissing without one.
int WlanRadioState(int set) {
    HANDLE wlan = nullptr;
    DWORD version = 0;
    if (WlanOpenHandle(2, nullptr, &version, &wlan) != ERROR_SUCCESS) {
        return kRadioMissing;
    }
    int state = kRadioMissing;
    PWLAN_INTERFACE_INFO_LIST interfaces = nullptr;
    if (WlanEnumInterfaces(wlan, nullptr, &interfaces) == ERROR_SUCCESS) {
        for (DWORD i = 0; i < interfaces->dwNumberOfItems; i++) {
            const GUID& adapter = interfaces->InterfaceInfo[i].InterfaceGuid;
            for (int pass = set >= 0 ? 0 : 1; pass < 2; pass++) {
                DWORD size = 0;
                PWLAN_RADIO_STATE radio = nullptr;
                if (WlanQueryInterface(wlan, &adapter,
                                       wlan_intf_opcode_radio_state, nullptr,
                                       &size, (PVOID*)&radio,
                                       nullptr) != ERROR_SUCCESS) {
                    break;
                }
                bool on = false;
                for (DWORD phy = 0; phy < radio->dwNumberOfPhys; phy++) {
                    const auto& phyState = radio->PhyRadioState[phy];
                    if (pass == 0) {
                        WLAN_PHY_RADIO_STATE wanted{};
                        wanted.dwPhyIndex = phyState.dwPhyIndex;
                        wanted.dot11SoftwareRadioState =
                            set ? dot11_radio_state_on : dot11_radio_state_off;
                        if (const DWORD result = WlanSetInterface(
                                wlan, &adapter, wlan_intf_opcode_radio_state,
                                sizeof(wanted), &wanted, nullptr);
                            result != ERROR_SUCCESS) {
                            IslandLog(L"Wi-Fi radio: %u", result);
                        }
                    }
                    on = on ||
                         (phyState.dot11SoftwareRadioState ==
                              dot11_radio_state_on &&
                          phyState.dot11HardwareRadioState ==
                              dot11_radio_state_on);
                }
                WlanFreeMemory(radio);
                if (pass == 1) {
                    state = std::max(state, on ? 1 : 0);
                }
            }
        }
        WlanFreeMemory(interfaces);
    }
    WlanCloseHandle(wlan, nullptr);
    return state;
}

DWORD WINAPI RadioThreadProc(LPVOID parameter) {
    std::unique_ptr<RadioRequest> request((RadioRequest*)parameter);
    using namespace winrt::Windows::Devices::Radios;
    int states[2] = {kRadioMissing, kRadioMissing};
    winrt::init_apartment(winrt::apartment_type::multi_threaded);

    // Airplane mode first: it switches the radios.
    int airplane = kRadioMissing;
    winrt::com_ptr<IRadioManager> radioManager;
    if (SUCCEEDED(CoCreateInstance(kCLSID_RadioManagementAPI, nullptr,
                                   CLSCTX_ALL, kIID_IRadioManager,
                                   radioManager.put_void()))) {
        if (request->kind == 3) {
            const HRESULT result =
                radioManager->SetSystemRadioState(request->on ? 0 : 1);
            if (FAILED(result)) {
                IslandLog(L"Airplane mode: %08X", (unsigned)result);
            }
        }
        int enabled = 1;
        int unknown = 0;
        int reason = 0;
        if (SUCCEEDED(radioManager->GetSystemRadioState(&enabled, &unknown,
                                                        &reason))) {
            airplane = enabled ? 0 : 1;
        }
        radioManager = nullptr;
    }

    try {
        if (Radio::RequestAccessAsync().get() == RadioAccessStatus::Allowed) {
            for (const auto& radio : Radio::GetRadiosAsync().get()) {
                const int index = radio.Kind() == RadioKind::WiFi ? 0
                                  : radio.Kind() == RadioKind::Bluetooth
                                      ? 1
                                      : -1;
                if (index < 0) {
                    continue;
                }
                if (index == request->kind) {
                    radio
                        .SetStateAsync(request->on ? RadioState::On
                                                   : RadioState::Off)
                        .get();
                }
                states[index] = std::max(
                    states[index], radio.State() == RadioState::On ? 1 : 0);
            }
        }
    } catch (winrt::hresult_error const& e) {
        IslandLog(L"Radios: %08X %s", (unsigned)e.code(), e.message().c_str());
    }
    // A Wi-Fi adapter the radio API doesn't list: through the WLAN API.
    if (states[0] == kRadioMissing) {
        states[0] = WlanRadioState(request->kind == 0 ? request->on : -1);
    }
    PostMessage(request->notify, WM_APP_RADIOS, (WPARAM)(INT_PTR)states[0],
                (LPARAM)(INT_PTR)states[1]);

    // The mobile hotspot shares the internet connection over Wi-Fi.
    int hotspot = kRadioMissing;
    try {
        using namespace winrt::Windows::Networking;
        auto profile =
            Connectivity::NetworkInformation::GetInternetConnectionProfile();
        using NetworkOperators::NetworkOperatorTetheringManager;
        if (profile &&
            NetworkOperatorTetheringManager::
                    GetTetheringCapabilityFromConnectionProfile(profile) ==
                NetworkOperators::TetheringCapability::Enabled) {
            auto manager =
                NetworkOperatorTetheringManager::CreateFromConnectionProfile(
                    profile);
            if (request->kind == 2) {
                auto result = (request->on ? manager.StartTetheringAsync()
                                           : manager.StopTetheringAsync())
                                  .get();
                if (result.Status() !=
                    NetworkOperators::TetheringOperationStatus::Success) {
                    IslandLog(L"Hotspot: status %d", (int)result.Status());
                }
            }
            hotspot = manager.TetheringOperationalState() ==
                              NetworkOperators::TetheringOperationalState::On
                          ? 1
                          : 0;
        }
    } catch (winrt::hresult_error const& e) {
        IslandLog(L"Hotspot: %08X %s", (unsigned)e.code(), e.message().c_str());
    }
    PostMessage(request->notify, WM_APP_HOTSPOT, (WPARAM)(INT_PTR)hotspot,
                (LPARAM)(INT_PTR)airplane);
    winrt::uninit_apartment();
    return 0;
}

// The mobile hotspot's network name and password, read or changed on a
// thread (the calls wait), as Windows' own Mobile hotspot page does
// (NetworkOperatorTetheringManager's access point configuration). The
// answer comes back as a new HotspotAnswer (the panel deletes it).
constexpr UINT WM_APP_HOTSPOT_CONFIG = WM_APP + 102;
struct HotspotRequest {
    HWND notify;
    // Empty: only read.
    std::wstring name;
    std::wstring password;
};
struct HotspotAnswer {
    bool worked = false;
    bool saved = false;
    std::wstring name;
    std::wstring password;
};

DWORD WINAPI HotspotConfigThreadProc(LPVOID parameter) {
    std::unique_ptr<HotspotRequest> request((HotspotRequest*)parameter);
    auto answer = std::make_unique<HotspotAnswer>();
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    try {
        using namespace winrt::Windows::Networking;
        using NetworkOperators::NetworkOperatorTetheringManager;
        auto profile =
            Connectivity::NetworkInformation::GetInternetConnectionProfile();
        if (profile &&
            NetworkOperatorTetheringManager::
                    GetTetheringCapabilityFromConnectionProfile(profile) ==
                NetworkOperators::TetheringCapability::Enabled) {
            auto manager =
                NetworkOperatorTetheringManager::CreateFromConnectionProfile(
                    profile);
            auto config = manager.GetCurrentAccessPointConfiguration();
            if (!request->name.empty()) {
                config.Ssid(request->name);
                config.Passphrase(request->password);
                manager.ConfigureAccessPointAsync(config).get();
                config = manager.GetCurrentAccessPointConfiguration();
                answer->saved = true;
            }
            answer->name = config.Ssid();
            answer->password = config.Passphrase();
            answer->worked = true;
        }
    } catch (winrt::hresult_error const& e) {
        // (Not the password: only what failed.)
        IslandLog(L"Hotspot settings: %08X", (unsigned)e.code());
    }
    winrt::uninit_apartment();
    if (PostMessage(request->notify, WM_APP_HOTSPOT_CONFIG, 0,
                    (LPARAM)answer.get())) {
        answer.release();
    }
    return 0;
}

// A paired Bluetooth audio device connected or disconnected the way
// Windows' own "Connect" does it: the one-shot reconnect (or disconnect)
// property of its audio driver (KSPROPSETID_BtAudio, ksmedia.h), asked to
// the kernel streaming filter behind one of its audio endpoints (present or
// unplugged), found by the device's address in that filter's ID.
constexpr GUID kKsPropSetBtAudio = {
    0x7fa06c40,
    0xb8f6,
    0x4c7e,
    {0x85, 0x56, 0xe8, 0xc3, 0x3a, 0x12, 0xe5, 0x4d}};
constexpr ULONG kKsPropertyOneShotReconnect = 0;
constexpr ULONG kKsPropertyOneShotDisconnect = 1;
constexpr ULONG kKsPropertyTypeGet = 1;
constexpr GUID kIID_IDeviceTopology = {
    0x2a07407e,
    0x6497,
    0x4a18,
    {0x97, 0x87, 0x32, 0xf7, 0x9b, 0xd0, 0xd9, 0x8f}};
constexpr GUID kIID_IKsControl = {
    0x28f54685,
    0x06fd,
    0x11d2,
    {0xb2, 0x7a, 0x00, 0xa0, 0xc9, 0x22, 0x31, 0x96}};

// KSPROPERTY (ks.h).
struct KsPropertyRequest {
    GUID set;
    ULONG id;
    ULONG flags;
};

// IKsControl (ksproxy.h): only its first method is called.
struct IKsControlProperty : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE KsProperty(KsPropertyRequest* property,
                                                 ULONG propertyLength,
                                                 void* data,
                                                 ULONG dataLength,
                                                 ULONG* returned) = 0;
};

// False without an audio endpoint of the device, or when its driver
// refused.
bool SetBluetoothAudioConnected(ULONGLONG address, bool connect) {
    winrt::com_ptr<IMMDeviceEnumerator> devices;
    winrt::com_ptr<IMMDeviceCollection> endpoints;
    UINT count = 0;
    if (FAILED(CoCreateInstance(kCLSID_MMDeviceEnumerator, nullptr,
                                CLSCTX_ALL, kIID_IMMDeviceEnumerator,
                                devices.put_void())) ||
        FAILED(devices->EnumAudioEndpoints(
            eAll, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED,
            endpoints.put())) ||
        FAILED(endpoints->GetCount(&count))) {
        return false;
    }
    // (The address in the filter's ID: "...&0&001A7DDA7113_C00000000#...".)
    WCHAR wanted[16];
    swprintf_s(wanted, L"%012llX", address & 0xFFFFFFFFFFFFull);
    for (UINT i = 0; i < count; i++) {
        winrt::com_ptr<IMMDevice> endpoint;
        winrt::com_ptr<IDeviceTopology> topology;
        winrt::com_ptr<IConnector> connector;
        LPWSTR filterId = nullptr;
        if (FAILED(endpoints->Item(i, endpoint.put())) ||
            FAILED(endpoint->Activate(kIID_IDeviceTopology, CLSCTX_ALL,
                                      nullptr, topology.put_void())) ||
            FAILED(topology->GetConnector(0, connector.put())) ||
            FAILED(connector->GetDeviceIdConnectedTo(&filterId))) {
            continue;
        }
        std::wstring id = filterId;
        std::transform(id.begin(), id.end(), id.begin(), towupper);
        winrt::com_ptr<IMMDevice> filter;
        winrt::com_ptr<IKsControlProperty> control;
        const bool found =
            id.find(wanted) != std::wstring::npos &&
            SUCCEEDED(devices->GetDevice(filterId, filter.put())) &&
            SUCCEEDED(filter->Activate(kIID_IKsControl, CLSCTX_ALL, nullptr,
                                       control.put_void()));
        CoTaskMemFree(filterId);
        if (!found) {
            continue;
        }
        KsPropertyRequest property{kKsPropSetBtAudio,
                                   connect ? kKsPropertyOneShotReconnect
                                           : kKsPropertyOneShotDisconnect,
                                   kKsPropertyTypeGet};
        ULONG returned = 0;
        if (SUCCEEDED(control->KsProperty(&property, sizeof(property),
                                          nullptr, 0, &returned))) {
            return true;
        }
    }
    return false;
}

struct BluetoothConnectRequest {
    HWND notify;
    ULONGLONG address;
    bool connect;
};

DWORD WINAPI BluetoothConnectProc(LPVOID parameter) {
    std::unique_ptr<BluetoothConnectRequest> request(
        (BluetoothConnectRequest*)parameter);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool done =
        SetBluetoothAudioConnected(request->address, request->connect);
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    PostMessage(request->notify, WM_APP_BLUETOOTH_DONE, done,
                (LPARAM)request->address);
    return 0;
}

// Where a device connecting or disconnecting from its row is.
enum class BluetoothStep { None, Connecting, Disconnecting, Failed };

// The step as it stands, `step` having started at `since` (GetTickCount64):
// one whose checks stopped (the panel closed meanwhile, see
// ControlPanel::CheckBluetoothStep) is over once its time is, or it would
// stay on the row and keep the device from being clicked again.
BluetoothStep LiveBluetoothStep(BluetoothStep step, ULONGLONG since) {
    const ULONGLONG elapsed = GetTickCount64() - since;
    if (((step == BluetoothStep::Connecting ||
          step == BluetoothStep::Disconnecting) &&
         elapsed > kBluetoothConnectMs + kBluetoothCheckMs) ||
        (step == BluetoothStep::Failed &&
         elapsed > kBluetoothFailedMs + kBluetoothCheckMs)) {
        return BluetoothStep::None;
    }
    return step;
}

// A device's line on its row: what's happening to it (`step`, when it's the
// one), or that it's connected.
PCWSTR BluetoothStatusText(bool connected, BluetoothStep step) {
    switch (step) {
        case BluetoothStep::Connecting:
            return Tr(L"Connecting...", L"Conectando...");
        case BluetoothStep::Disconnecting:
            return Tr(L"Disconnecting...", L"Desconectando...");
        case BluetoothStep::Failed:
            return Tr(L"Couldn't connect.", L"Não foi possível conectar.");
        default:
            return connected ? Tr(L"Connected", L"Conectado") : nullptr;
    }
}

// Waits for threads that own their jobs, all of them together for at most
// `ms`: one stuck in a slow call (a monitor, a radio, a media app) ends by
// itself later, and the tool process exits at unload anyway.
void JoinOwningThreads(std::vector<HANDLE>& threads, DWORD ms) {
    const ULONGLONG until = GetTickCount64() + ms;
    for (HANDLE thread : threads) {
        const ULONGLONG now = GetTickCount64();
        WaitForSingleObject(thread, now < until ? (DWORD)(until - now) : 0);
        CloseHandle(thread);
    }
    threads.clear();
}

// Brightness: reads it (BrightnessQueryProc, the first monitor's;
// BrightnessListProc, each monitor's), or sets the latest levels asked for
// until no newer ones come (BrightnessThreadProc), so dragging a slider
// doesn't queue up slow calls. Each thread holds the job too (its parameter
// is a new std::shared_ptr to it), so it can outlive its window; `notify` is
// cleared when that window goes.
struct BrightnessJob {
    std::atomic<HWND> notify{nullptr};
    // The level asked for each monitor (by its place in
    // ForEachPhysicalMonitor's order), -1 once written (or none asked).
    std::atomic<int> targets[kMaxBrightnessMonitors];
    // An island's screen (read for it, see BrightnessQueryProc), and the
    // level asked for its monitors (-1 once written).
    std::atomic<HMONITOR> screen{nullptr};
    std::atomic<int> screenTarget{-1};
    std::atomic<bool> running{false};
    // A monitor asked didn't answer: its list is read again (see OnOpen).
    std::atomic<bool> missed{false};

    BrightnessJob() {
        for (auto& target : targets) {
            target = -1;
        }
    }
    // A monitor's level asked for (-1: every monitor's).
    void Ask(int monitor, int level) {
        for (int i = 0; i < kMaxBrightnessMonitors; i++) {
            if (monitor < 0 || monitor == i) {
                targets[i] = level;
            }
        }
    }
    void AskScreen(HMONITOR monitor, int level) {
        screen = monitor;
        screenTarget = level;
    }
    bool Asked() const {
        return screenTarget >= 0 ||
               std::any_of(std::begin(targets), std::end(targets),
                           [](const std::atomic<int>& target) {
                               return target >= 0;
                           });
    }
};


DWORD WINAPI BrightnessQueryProc(LPVOID parameter) {
    const std::unique_ptr<std::shared_ptr<BrightnessJob>> held(
        (std::shared_ptr<BrightnessJob>*)parameter);
    const int level = ReadBrightness((*held)->screen);
    if (HWND notify = (*held)->notify) {
        PostMessage(notify, WM_APP_BRIGHTNESS, (WPARAM)(INT_PTR)level, 0);
    }
    return 0;
}

// Each monitor's (WM_APP_BRIGHTNESS_LIST, lParam: a new list its window
// deletes).
DWORD WINAPI BrightnessListProc(LPVOID parameter) {
    const std::unique_ptr<std::shared_ptr<BrightnessJob>> held(
        (std::shared_ptr<BrightnessJob>*)parameter);
    auto list = std::make_unique<std::vector<BrightnessMonitor>>(
        ReadBrightnessMonitors());
    if (HWND notify = (*held)->notify;
        notify &&
        PostMessage(notify, WM_APP_BRIGHTNESS_LIST, 0, (LPARAM)list.get())) {
        list.release();
    }
    return 0;
}

DWORD WINAPI BrightnessThreadProc(LPVOID parameter) {
    const std::unique_ptr<std::shared_ptr<BrightnessJob>> held(
        (std::shared_ptr<BrightnessJob>*)parameter);
    BrightnessJob* job = held->get();
    for (;;) {
        int levels[kMaxBrightnessMonitors];
        const int screenLevel = job->screenTarget.exchange(-1);
        bool any = screenLevel >= 0;
        for (int i = 0; i < kMaxBrightnessMonitors; i++) {
            levels[i] = job->targets[i].exchange(-1);
            any = any || levels[i] >= 0;
        }
        if (any) {
            if (!WriteBrightness(levels, job->screen, screenLevel)) {
                job->missed = true;
            }
            continue;
        }
        job->running = false;
        // A level asked for meanwhile starts it again.
        if (job->Asked() && !job->running.exchange(true)) {
            continue;
        }
        break;
    }
    return 0;
}

// A join that finished: how, and which profile it was for (a click on
// another network meanwhile makes the first one's end come later).
struct WlanConnectionEvent {
    DWORD code;
    DWORD reason;
    std::wstring profile;
};

// Wi-Fi events, on a system thread: passed on to the control center.
void WINAPI WlanNotificationCallback(PWLAN_NOTIFICATION_DATA data,
                                     PVOID context) {
    if (!data || data->NotificationSource != WLAN_NOTIFICATION_SOURCE_ACM) {
        return;
    }
    if ((data->NotificationCode == wlan_notification_acm_connection_complete ||
         data->NotificationCode ==
             wlan_notification_acm_connection_attempt_fail) &&
        data->pData &&
        data->dwDataSize >= sizeof(WLAN_CONNECTION_NOTIFICATION_DATA)) {
        const auto* connection =
            (PWLAN_CONNECTION_NOTIFICATION_DATA)data->pData;
        auto* event = new WlanConnectionEvent{
            data->NotificationCode, connection->wlanReasonCode,
            std::wstring(connection->strProfileName,
                         wcsnlen(connection->strProfileName,
                                 WLAN_MAX_NAME_LENGTH))};
        if (!PostMessage((HWND)context, WM_APP_WLAN_CONNECTION, 0,
                         (LPARAM)event)) {
            delete event;
        }
        return;
    }
    PostMessage((HWND)context, WM_APP_WLAN, data->NotificationCode, 0);
}

std::wstring EscapeXml(const std::wstring& text) {
    std::wstring escaped;
    for (WCHAR c : text) {
        switch (c) {
            case L'&':
                escaped += L"&amp;";
                break;
            case L'<':
                escaped += L"&lt;";
                break;
            case L'>':
                escaped += L"&gt;";
                break;
            case L'"':
                escaped += L"&quot;";
                break;
            case L'\'':
                escaped += L"&apos;";
                break;
            default:
                escaped += c;
        }
    }
    return escaped;
}

std::wstring SsidName(const DOT11_SSID& ssid) {
    const int length = (int)std::min<ULONG>(ssid.uSSIDLength, 32);
    WCHAR name[64] = L"";
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                             (LPCCH)ssid.ucSSID, length, name,
                             ARRAYSIZE(name) - 1)) {
        MultiByteToWideChar(CP_ACP, 0, (LPCCH)ssid.ucSSID, length, name,
                            ARRAYSIZE(name) - 1);
    }
    return name;
}

// A program's name, as its file describes it ("Google Chrome"), or its file
// name.
std::wstring GetProgramName(PCWSTR path) {
    std::wstring name;
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSize(path, &handle);
    if (size) {
        std::vector<BYTE> data(size);
        struct Translation {
            WORD language;
            WORD codePage;
        }* translations = nullptr;
        UINT length = 0;
        if (GetFileVersionInfo(path, 0, size, data.data()) &&
            VerQueryValue(data.data(), L"\\VarFileInfo\\Translation",
                          (void**)&translations, &length) &&
            length >= sizeof(Translation)) {
            WCHAR query[64];
            swprintf_s(query, L"\\StringFileInfo\\%04x%04x\\FileDescription",
                       translations[0].language, translations[0].codePage);
            PCWSTR description = nullptr;
            if (VerQueryValue(data.data(), query, (void**)&description,
                              &length) &&
                description && *description) {
                name = description;
            }
        }
    }
    if (name.empty()) {
        PCWSTR file = wcsrchr(path, L'\\');
        name = file ? file + 1 : path;
        if (name.size() > 4 &&
            _wcsicmp(name.c_str() + name.size() - 4, L".exe") == 0) {
            name.resize(name.size() - 4);
        }
    }
    return name;
}

// An active output device (speakers, headphones, a display's sound).
struct AudioOutput {
    std::wstring id;
    std::wstring name;
    WCHAR glyph;
    bool isDefault;
};

// The active output devices, the default one marked.
std::vector<AudioOutput> ActiveOutputs(IMMDeviceEnumerator* devices) {
    std::vector<AudioOutput> outputs;
    std::wstring defaultId;
    winrt::com_ptr<IMMDevice> defaultDevice;
    if (SUCCEEDED(devices->GetDefaultAudioEndpoint(eRender, eMultimedia,
                                                   defaultDevice.put()))) {
        LPWSTR id = nullptr;
        if (SUCCEEDED(defaultDevice->GetId(&id))) {
            defaultId = id;
            CoTaskMemFree(id);
        }
    }

    winrt::com_ptr<IMMDeviceCollection> collection;
    UINT count = 0;
    if (FAILED(devices->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE,
                                           collection.put())) ||
        FAILED(collection->GetCount(&count))) {
        return outputs;
    }
    for (UINT i = 0; i < count; i++) {
        winrt::com_ptr<IMMDevice> device;
        LPWSTR id = nullptr;
        if (FAILED(collection->Item(i, device.put())) ||
            FAILED(device->GetId(&id))) {
            continue;
        }
        AudioOutput output{id, L"", kGlyphSpeakers, defaultId == id};
        CoTaskMemFree(id);

        winrt::com_ptr<IPropertyStore> properties;
        if (SUCCEEDED(device->OpenPropertyStore(STGM_READ,
                                                properties.put()))) {
            PROPVARIANT value;
            PropVariantInit(&value);
            if (SUCCEEDED(properties->GetValue(kPKEY_Device_FriendlyName,
                                               &value)) &&
                value.vt == VT_LPWSTR && value.pwszVal) {
                output.name = value.pwszVal;
            }
            PropVariantClear(&value);
            PropVariantInit(&value);
            // Form factor: headphones (3) and headsets (5), displays (9).
            if (SUCCEEDED(properties->GetValue(kPKEY_AudioEndpoint_FormFactor,
                                               &value)) &&
                value.vt == VT_UI4) {
                if (value.ulVal == 3 || value.ulVal == 5) {
                    output.glyph = kGlyphHeadphones;
                } else if (value.ulVal == 9) {
                    output.glyph = kGlyphMonitor;
                }
            }
            PropVariantClear(&value);
        }
        outputs.push_back(std::move(output));
    }
    return outputs;
}

// Makes a device the default output, for everything (like the Windows sound
// settings do).
void SetDefaultOutput(const std::wstring& id) {
    winrt::com_ptr<IPolicyConfig> policy;
    if (SUCCEEDED(CoCreateInstance(kCLSID_PolicyConfig, nullptr, CLSCTX_ALL,
                                   kIID_IPolicyConfig, policy.put_void()))) {
        for (ERole role : {eConsole, eMultimedia, eCommunications}) {
            policy->SetDefaultEndpoint(id.c_str(), role);
        }
    }
}

// The sessions of one program playing sound.
struct MixerApp {
    DWORD processId = 0;
    // Its program (its sessions from any of its processes go together).
    std::wstring path;
    bool system = false;
    std::wstring name;
    float volume = 1;
    bool muted = false;
    std::vector<winrt::com_ptr<ISimpleAudioVolume>> volumes;
    std::vector<std::wstring> sessionIds;
    HICON icon = nullptr;
    winrt::com_ptr<ID2D1Bitmap> bitmap;
};

void ClearMixerApps(std::vector<MixerApp>& apps);
bool SameMixerApp(const MixerApp& app, bool system,
                  const std::wstring& path, DWORD processId);
void ReadMixerApps(std::vector<MixerApp>& apps,
                   size_t maxApps = std::numeric_limits<size_t>::max(),
                   std::wstring* deviceId = nullptr);
ID2D1Bitmap* MixerAppBitmap(ID2D1RenderTarget* target,
                           IWICImagingFactory* wic, MixerApp& app);

