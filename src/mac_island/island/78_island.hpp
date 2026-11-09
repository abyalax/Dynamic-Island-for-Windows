#pragma once
RECT Island::ReservedInsets() const {
    MONITORINFO info{sizeof(info)};
    if (!GetMonitorInfo(CurrentMonitor(), &info)) {
        return {};
    }
    RECT insets{info.rcWork.left - info.rcMonitor.left,
                info.rcWork.top - info.rcMonitor.top,
                info.rcMonitor.right - info.rcWork.right,
                info.rcMonitor.bottom - info.rcWork.bottom};
    // (The room the island keeps itself isn't another bar's; a notch's is
    // its base island's.)
    const Island& keeper = m_notchBase ? *m_notchBase : *this;
    if (keeper.m_appBar && keeper.m_appBarThickness > 0 &&
        EqualRect(&keeper.m_appBarMonitor, &info.rcMonitor)) {
        LONG& side = keeper.m_appBarEdge == ABE_LEFT    ? insets.left
                     : keeper.m_appBarEdge == ABE_RIGHT  ? insets.right
                     : keeper.m_appBarEdge == ABE_BOTTOM ? insets.bottom
                                                         : insets.top;
        side = std::max(0L, side - keeper.m_appBarThickness);
    }
    return insets;
}

RECT Island::TaskbarInsets() const {
    HWND taskbar = FindTaskbarWnd();
    RECT rect;
    if (!taskbar || !GetWindowRect(taskbar, &rect)) {
        return ReservedInsets();
    }
    // Its monitor: the one with the middle of its inner edge (on the screen
    // even while it's hidden).
    const UINT edge = TaskbarEdge();
    const POINT inner =
        edge == ABE_LEFT ? POINT{rect.right - 1, (rect.top + rect.bottom) / 2}
        : edge == ABE_RIGHT ? POINT{rect.left, (rect.top + rect.bottom) / 2}
        : edge == ABE_TOP ? POINT{(rect.left + rect.right) / 2, rect.bottom - 1}
                          : POINT{(rect.left + rect.right) / 2, rect.top};
    // Another monitor: its own taskbar's room, as Windows leaves it.
    if (MonitorFromPoint(inner, MONITOR_DEFAULTTONEAREST) != CurrentMonitor()) {
        return ReservedInsets();
    }
    RECT insets{};
    switch (edge) {
        case ABE_LEFT:
            insets.left = rect.right - rect.left;
            break;
        case ABE_RIGHT:
            insets.right = rect.right - rect.left;
            break;
        case ABE_TOP:
            insets.top = rect.bottom - rect.top;
            break;
        default:
            insets.bottom = rect.bottom - rect.top;
            break;
    }
    return insets;
}

// (Before it's placed on one, or after its monitor went: the one it goes
// on.)
HMONITOR Island::CurrentMonitor() const {
    MONITORINFO info{sizeof(info)};
    if (m_monitorHandle && GetMonitorInfo(m_monitorHandle, &info)) {
        return m_monitorHandle;
    }
    return WantedMonitor();
}

// The monitor with the mouse (if chosen, with several), the one chosen, or
// the taskbar's.
HMONITOR Island::WantedMonitor() const {
    if (!m_main) {
        return m_ownMonitor;
    }
    if (g_settings.islandFollowMouse && !g_settings.islandAllMonitors &&
        NotchCountWanted() == 0 && MonitorCount() > 1) {
        POINT pt;
        if (GetCursorPos(&pt)) {
            return MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        }
    }
    if (!g_islandMonitor.empty()) {
        for (const auto& monitor : ListMonitors()) {
            if (monitor.device == g_islandMonitor) {
                return monitor.handle;
            }
        }
    }
    return TaskbarMonitor(FindTaskbarWnd());
}

void Island::UpdateMonitorWatch() {
    if (m_main && g_settings.islandFollowMouse &&
        !g_settings.islandAllMonitors && NotchCountWanted() == 0 &&
        MonitorCount() > 1) {
        SetTimer(m_hwnd, kMonitorTimerId, kMonitorCheckMs, nullptr);
    } else {
        KillTimer(m_hwnd, kMonitorTimerId);
    }
}

// (Each one still there, and the main one still on the monitor it goes on:
// the taskbar's, or the one chosen.)
bool Island::IslandMonitorsChanged() const {
    const std::vector<MonitorEntry> monitors = ListMonitors();
    const size_t islands = (size_t)std::count_if(
        g_islands.begin(), g_islands.end(),
        [](const Island* island) { return !island->m_notchBase; });
    if (g_settings.islandAllMonitors && monitors.size() != islands) {
        return true;
    }
    return std::any_of(
        g_islands.begin(), g_islands.end(), [&](const Island* island) {
            // (A notch goes where the one with the bell goes.)
            if (island->m_notchBase &&
                island->m_ownMonitor != island->m_notchBase->WantedMonitor()) {
                return true;
            }
            return island->m_monitorHandle != island->WantedMonitor() ||
                   std::none_of(monitors.begin(), monitors.end(),
                                [&](const MonitorEntry& monitor) {
                                    return monitor.handle ==
                                           island->m_monitorHandle;
                                });
        });
}

void Island::OnMonitorChoiceChanged() {
    UpdateMonitorWatch();
    CheckMonitor();
}

int ChosenMonitorIndex() {
    const std::vector<MonitorEntry> monitors = ListMonitors();
    HMONITOR chosen = nullptr;
    for (const auto& monitor : monitors) {
        if (monitor.device == g_islandMonitor) {
            chosen = monitor.handle;
        }
    }
    if (!chosen) {
        chosen = TaskbarMonitor(FindTaskbarWnd());
    }
    for (size_t i = 0; i < monitors.size(); i++) {
        if (monitors[i].handle == chosen) {
            return (int)i;
        }
    }
    return 0;
}

void ChooseMonitor(int index) {
    IslandScope scope(MainIsland());
    const std::vector<MonitorEntry> monitors = ListMonitors();
    if (index < 0 || index >= (int)monitors.size()) {
        return;
    }
    g_islandMonitor = monitors[index].device;
    StoreProfileString(L"islandMonitor", g_islandMonitor.c_str());
    // On each monitor: the main one is another, they start again (the
    // notches too, made on its monitor).
    if (g_settings.islandAllMonitors || NotchCountWanted() > 0) {
        RestartIslandSoon();
        return;
    }
    if (g_island) {
        ForEachIsland([&](Island& island) { island.OnMonitorChoiceChanged(); });
    }
}

// On another monitor now: at its place there (that monitor's own), without
// sliding to it.
void Island::SwitchMonitor(HMONITOR monitor) {
    m_monitorHandle = monitor;
    MONITORINFOEX info{};
    info.cbSize = sizeof(info);
    if (GetMonitorInfo(monitor, &info)) {
        m_placeDevice = info.szDevice;
        UseMonitorPlace(info.szDevice);
    }
    const IslandPlace& place = CurrentPlace();
    m_placeX = place.x;
    m_placeY = place.y;
    m_placeVelocityX = m_placeVelocityY = 0;
    if (m_minimized && !MinimizableAt(place)) {
        SetMinimized(false);
    }
    Layout();
    SetAppBarPosition();
    m_drawnKey.clear();
}

// Goes to the monitor it should be on: closing into its dot, which glides
// there and opens again (at once without animations, or while not shown).
// Not while it's being used (a panel open, a reply typed, dragged, a banner
// shown): checked again later.
void Island::CheckMonitor() {
    const HMONITOR wanted = WantedMonitor();
    if (!m_monitorHandle) {
        SwitchMonitor(wanted);
        UpdateMonitorWatch();
        return;
    }
    if (m_monitorMove == 1) {
        if (wanted == m_monitorHandle) {
            // Back where it is: it opens again there.
            m_monitorMove = 0;
            StartAnimating();
        } else {
            m_moveTarget = wanted;
        }
        return;
    }
    MONITORINFO info{sizeof(info)};
    const bool gone = !GetMonitorInfo(m_monitorHandle, &info);
    if (m_monitorMove || (wanted == m_monitorHandle && !gone)) {
        m_wantedMonitor = nullptr;
        return;
    }
    if (wanted != m_wantedMonitor) {
        m_wantedMonitor = wanted;
        m_wantedSince = NowSeconds();
    }
    if (!gone && g_settings.islandFollowMouse &&
        NowSeconds() - m_wantedSince < 0.4) {
        return;
    }
    if (!gone && (AnyPanelOpen() || m_bannerTyping || m_dragArmed ||
                  m_bannerWanted)) {
        return;
    }
    if (gone || !m_visible || m_away || !m_shownOnce ||
        AnimationLevel() >= 2) {
        SwitchMonitor(wanted);
        Render(true);
        return;
    }
    CloseAllPanels();
    HideGearBubble();
    m_monitorMove = 1;
    m_moveTarget = wanted;
    StartAnimating();
}

// Whether the app in front shows a group by its own choice.
bool Island::PlaceShows(PillGroup group) const {
    const IslandPlace* place = FindAppPlace(m_placeApp);
    return place && place->own && !(place->hidden & (1ull << (int)group)) &&
           OnlineGroupAllowed(group);
}

void Island::OnPlacesChanged() {
    // Its own clock, maybe.
    UpdateClock();
    // What's playing, shown for this app only: watched then too.
    if (PlaceShows(PillGroup::Media) || MediaPartWanted(kAppMediaButtons) ||
        MediaPartWanted(kAppMediaTitle)) {
        StartMediaWatch();
    }
    UpdateUsageWatch();
    // Away from the edges, it can't stay a line.
    if (m_minimized && !MinimizableAt(CurrentPlace())) {
        SetMinimized(false);
    }
    SetAppBarPosition();
    StartAnimating();
}

// Windows' own passing surfaces (Start, search, the quick settings and the
// notification center, task view, Alt+Tab, the screenshot's selection): not
// an app coming to the front. The island stays as it is for the app behind
// them, instead of moving away and back (while Start opens, for example, or
// in the colors and the profile of no app for the moment a screenshot is
// taken).
bool IsShellFlyout(HWND hwnd) {
    if (IsSnippingSelection(hwnd,
                            MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST))) {
        return true;
    }
    WCHAR className[64] = L"";
    GetClassName(hwnd, className, ARRAYSIZE(className));
    for (PCWSTR shellClass :
         {L"XamlExplorerHostIslandWindow", L"MultitaskingViewFrame",
          L"TaskSwitcherWnd", L"ForegroundStaging"}) {
        if (_wcsicmp(className, shellClass) == 0) {
            return true;
        }
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process) {
        return false;
    }
    WCHAR path[MAX_PATH];
    DWORD size = ARRAYSIZE(path);
    const bool named = QueryFullProcessImageName(process, 0, path, &size);
    CloseHandle(process);
    if (!named) {
        return false;
    }
    PCWSTR name = wcsrchr(path, L'\\');
    name = name ? name + 1 : path;
    for (PCWSTR host :
         {L"StartMenuExperienceHost.exe", L"SearchHost.exe", L"SearchApp.exe",
          L"ShellExperienceHost.exe", L"ShellHost.exe",
         L"ScreenClippingHost.exe"}) {
        if (_wcsicmp(name, host) == 0) {
            return true;
        }
    }
    return false;
}

// Whether a window in front is an app's own window, not one of its menus
// (Windows', or a popup without a title bar it owns, like a tray icon's
// menu drawn by the app) nor a hidden or tool window it takes the focus
// with for one.
bool IsAppWindow(HWND hWnd) {
    if (!IsWindowVisible(hWnd)) {
        return false;
    }
    WCHAR className[16] = L"";
    GetClassName(hWnd, className, ARRAYSIZE(className));
    if (wcscmp(className, L"#32768") == 0) {
        return false;
    }
    const LONG style = GetWindowLong(hWnd, GWL_STYLE);
    const LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    if ((exStyle & WS_EX_TOOLWINDOW) && !(exStyle & WS_EX_APPWINDOW)) {
        return false;
    }
    return !(GetWindow(hWnd, GW_OWNER) && (style & WS_POPUP) &&
             (style & WS_CAPTION) != WS_CAPTION);
}

void FrontLastApp() {
    DWORD processId = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &processId);
    if (processId != GetCurrentProcessId() || !g_island) {
        return;
    }
    const HWND app = g_island->LastAppWindow();
    if (app && IsWindow(app) && IsWindowVisible(app) && !IsIconic(app)) {
        SetForegroundWindow(app);
    }
}

// The app in front with a profile of its own (#6166): applied, the one in
// use before kept (its settings, if none was) to go back to when another
// app comes in front, unless the app's profile stays ("k"). Looked at once
// the app stayed in front a moment (kAutoProfileWaitMs): passing through
// apps with Alt+Tab doesn't apply each one's (a style of its own restarts
// the islands).
std::wstring g_autoProfileApp;
int g_autoProfileBefore = -1;
std::wstring g_autoProfileBeforeSettings;

void ForgetAutoProfile() {
    g_autoProfileApp.clear();
    g_autoProfileBeforeSettings.clear();
}

void CheckAutoProfile() {
    Island* main = MainIsland();
    const HWND foreground = GetForegroundWindow();
    if (!main || !main->Window() || !foreground || g_profiles.empty()) {
        return;
    }
    // (No app has a profile, and none is applied: nothing to look up.)
    if (g_autoProfileApp.empty() &&
        std::none_of(AutoProfileRules().begin(), AutoProfileRules().end(),
                     [](const AutoProfileRule& rule) {
                         return rule.kind == L'a';
                     })) {
        return;
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(foreground, &processId);
    if (processId == GetCurrentProcessId() || IsShellFlyout(foreground) ||
        !IsAppWindow(foreground)) {
        return;
    }
    const std::wstring appId = WindowAppKey(foreground);
    if (appId.empty() ||
        _wcsicmp(appId.c_str(), g_autoProfileApp.c_str()) == 0) {
        return;
    }
    IslandScope scope(main);
    if (const int wanted = AutoProfileFor(L'a', appId); wanted >= 0) {
        if (g_autoProfileApp.empty()) {
            g_autoProfileBefore = g_activeProfile;
            g_autoProfileBeforeSettings =
                g_activeProfile < 0 ? CaptureProfile() : std::wstring();
        }
        g_autoProfileApp = appId;
        if (wanted != g_activeProfile) {
            IslandLog(L"The profile of the app in front");
            ApplyProfile(wanted);
        }
        return;
    }
    if (g_autoProfileApp.empty()) {
        return;
    }
    const bool stays = AutoProfileHas(g_activeProfile, L'k', L"1");
    const std::wstring settings = std::move(g_autoProfileBeforeSettings);
    ForgetAutoProfile();
    if (stays) {
        return;
    }
    IslandLog(L"Back to the profile before the app's");
    if (ProfileExists(g_autoProfileBefore)) {
        ApplyProfile(g_autoProfileBefore);
    } else if (!settings.empty()) {
        // (No profile was in use: its settings back, the app's profile
        // keeping its own.)
        SaveActiveProfile();
        std::unordered_map<std::wstring, int> numbers;
        std::unordered_map<std::wstring, std::wstring> texts;
        ParseProfileLines(settings.data(), &numbers, &texts);
        if (ApplyProfileSettings(numbers, texts)) {
            g_activeProfile = -1;
            SaveProfiles();
            ForEachIsland([](Island& island) { island.OnProfileApplied(); });
        }
    }
}

// The app in front, for its own place. The island's own windows (and the
// taskbar) don't count.
void CALLBACK IslandForegroundProc(HWINEVENTHOOK,
                                   DWORD,
                                   HWND,
                                   LONG,
                                   LONG,
                                   DWORD,
                                   DWORD) {
    if (g_island) {
        ForEachIsland([](Island& island) { island.OnForegroundChanged(); });
        if (Island* main = MainIsland(); main && main->Window()) {
            SetTimer(main->Window(), kAutoProfileTimerId, kAutoProfileWaitMs,
                     nullptr);
        }
    }
    if (Island* main = MainIsland()) {
        main->CheckInputMethodSoon();
    }
}

