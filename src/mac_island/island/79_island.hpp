#pragma once
void Island::ReadWindowColor() {
    bool wanted = false;
    ForEachIsland(
        [&](Island&) { wanted = wanted || g_settings.islandWindowColor; });
    if (!wanted || !m_appWindow || !IsWindow(m_appWindow)) {
        return;
    }
    const int color = WindowColor(m_appWindow);
    if (color != g_windowColor) {
        g_windowColor = color;
        ForEachIsland([](Island& island) {
            ApplyThemeIndex(true);
            island.StartAnimating();
        });
    }
}

void Island::OnForegroundChanged() {
    HWND foreground = GetForegroundWindow();
    if (!foreground) {
        return;
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(foreground, &processId);
    if (processId == GetCurrentProcessId()) {
        WCHAR className[64] = L"";
        GetClassName(foreground, className, ARRAYSIZE(className));
        if (_wcsicmp(className, kIslandClassName) == 0 ||
            _wcsicmp(className, kPanelClassName) == 0 ||
            _wcsicmp(className, L"Shell_TrayWnd") == 0 ||
            _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0) {
            return;
        }
    }
    NoteFocusedApp(foreground);
    // (Nor an app's menu: the app in front, its colors and its place stay.)
    if (IsShellFlyout(foreground)) {
        return;
    }
    // The desktop: the general places and icons, not the last app's
    // (#6692). The taskbar, or a window not shown yet: looked at again in
    // a moment (switching apps from the taskbar passes through it).
    if (IsDesktopWindow(foreground)) {
        KillTimer(m_hwnd, kPlaceRecheckTimerId);
        SetPlaceApp({});
        return;
    }
    if (!IsAppWindow(foreground)) {
        SetTimer(m_hwnd, kPlaceRecheckTimerId, kPlaceRecheckMs, nullptr);
        return;
    }
    KillTimer(m_hwnd, kPlaceRecheckTimerId);
    m_appWindow = foreground;
    // Its color, blending in.
    if (m_main) {
        ReadWindowColor();
    }
    // (Taken first: reading them takes them off every island's list.)
    std::vector<std::wstring> seen;
    for (const auto& app : m_unreadApps) {
        if (WindowMatchesApp(foreground, app.id)) {
            seen.push_back(app.id);
        }
    }
    for (const auto& appId : seen) {
        MarkAppRead(appId);
        m_notifications.ClearApp(appId);
    }
    SetPlaceApp(PerAppPlacesOn() ? WindowAppKey(foreground) : std::wstring());
}

void Island::SetPlaceApp(const std::wstring& appId) {
    if (appId == m_placeApp) {
        return;
    }
    const IslandPlace* before = FindAppPlace(m_placeApp);
    m_placeApp = appId;
    if (before || FindAppPlace(appId)) {
        OnPlacesChanged();
        StartAnimating();
    }
}

// (Once, see kPlaceRecheckTimerId: an app's window now is taken as usual;
// still the taskbar or the desktop, the general places; anything else, an
// app's dialog or tool window, keeps its app's.)
void Island::RecheckPlaceApp() {
    HWND foreground = GetForegroundWindow();
    if (foreground && IsAppWindow(foreground)) {
        OnForegroundChanged();
        return;
    }
    WCHAR className[64] = L"";
    if (foreground) {
        GetClassName(foreground, className, ARRAYSIZE(className));
    }
    if (!foreground || IsDesktopWindow(foreground) ||
        _wcsicmp(className, L"Shell_TrayWnd") == 0 ||
        _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0) {
        SetPlaceApp({});
    }
}

// Let go after dragging: it snaps to a ready place nearby (the middle or the
// corners of each edge), or stays where it is; kept for the app in front if
// it has its own place.
void Island::FinishDrag() {
    double x = m_placeX;
    double y = m_placeY;
    const double snap = 64.0 * m_scale;
    double best = snap;
    for (double px : {0.0, 0.5, 1.0}) {
        for (double py : {0.0, 0.5, 1.0}) {
            const double distance = std::hypot((m_placeX - px) * m_roomWidth,
                                               (m_placeY - py) * m_roomHeight);
            if (distance < best) {
                best = distance;
                x = px;
                y = py;
            }
        }
    }
    SetIslandPlace(FindAppPlace(m_placeApp) ? m_placeApp : std::wstring(), x,
                   y);
}

// Whether a group is shown now: turned on (the ones with a switch that's
// only for whether they're there), and not hidden for the app in front.
bool Island::GroupWanted(PillGroup group) const {
    if (!OnlineGroupAllowed(group) || !InThisNotch(group)) {
        return false;
    }
    if (group == PillGroup::Microphone && m_microphoneGlyph.empty()) {
        return false;
    }
    // What's playing's cover and title, and its buttons: their own options
    // (an app's own choice too, see MediaPartWanted).
    if (group == PillGroup::MediaTitle || group == PillGroup::MediaButtons) {
        return MediaPartWanted(group == PillGroup::MediaTitle
                                   ? kAppMediaTitle
                                   : kAppMediaButtons);
    }
    if (const IslandPlace* place = FindAppPlace(m_placeApp)) {
        if (place->hidden & (1ull << (int)group)) {
            return false;
        }
        if (place->own) {
            return true;
        }
    }
    switch (group) {
        case PillGroup::Apps:
            return g_settings.islandApps;
        case PillGroup::Network:
            return g_settings.islandNetwork;
        case PillGroup::Volume:
            return g_settings.islandVolume;
        case PillGroup::Battery:
            return g_settings.islandBattery;
        case PillGroup::Clock: {
            const IslandPlace* place = FindAppPlace(m_placeApp);
            const int parts = place && place->clock >= 0 ? place->clock
                                                         : GlobalClockParts();
            return g_settings.islandClock || (parts & (2 | 4 | 8));
        }
        case PillGroup::Bell:
            return g_settings.islandBell;
        case PillGroup::Power:
            return g_settings.islandPower;
        case PillGroup::Usage:
            return AnyUsageShown();
        case PillGroup::Theme:
            return g_settings.islandThemeButton;
        case PillGroup::Microphone:
            return g_settings.islandMicrophone;
        case PillGroup::Notes:
            return g_settings.islandNotes;
        case PillGroup::Tasks:
            return g_settings.islandTasks;
        case PillGroup::Shortcuts:
            return g_settings.islandShortcuts;
        case PillGroup::Weather:
            return g_settings.islandWeather;
        case PillGroup::Updates:
            return g_settings.islandUpdates;
        case PillGroup::Profiles:
            return g_settings.islandProfiles;
        case PillGroup::Brightness:
            // (Not without a monitor that tells its brightness.)
            return g_settings.islandBrightness && m_brightness >= 0;
        case PillGroup::InputMethod:
            return g_settings.islandInputMethod && !m_inputMethodText.empty();
        case PillGroup::Privacy:
            // (Only while an app uses one.)
            return g_settings.islandPrivacy && m_privacy.Any();
        case PillGroup::Drives:
            // (Only while there are some, or their list is open with an
            // ejected one still in it.)
            return g_settings.islandUsbDrives &&
                   (!m_drives.empty() || m_drivesPanel.IsOpen());
        case PillGroup::Desktops:
            return g_settings.islandDesktops && !m_desktops.names.empty();
        case PillGroup::Folders:
            return g_settings.islandFolders;
        case PillGroup::Wallpaper:
            return g_settings.islandWallpaper;
        case PillGroup::KomorebiLayout:
            // (Only while komorebi runs and manages this monitor.)
            return g_settings.komorebiLayout &&
                   KomorebiMonitorOf(m_monitorHandle);
        case PillGroup::FocusedApp:
            return g_settings.islandFocusedApp &&
                   (m_focusWindow || m_focusDesktop);
        case PillGroup::NetSpeed:
            return g_settings.islandNetSpeed;
        case PillGroup::WorldClocks:
            return g_settings.islandWorldClocks;
        case PillGroup::Countdown:
            // (Only with an event in the next hours.)
            return g_settings.islandCountdown && !m_countdownText.empty();
        case PillGroup::RecycleBin:
            return RecycleBinShown();
        case PillGroup::Timer:
            return g_settings.islandTimer;
        case PillGroup::Visualizer:
            // (Only while something plays: told by Windows' media controls,
            // or heard.)
            return g_settings.islandVisualizer &&
                   (m_mediaShown.playing || m_audioPlaying);
        case PillGroup::Lyrics:
            // (Only with a line being sung, not in the title's place; not
            // upright either, like the title: a line doesn't fit across.)
            return g_settings.islandLyrics && !m_lyricsLine.empty() &&
                   !LyricsInTitle() && !m_vertical;
        case PillGroup::AiUsage:
            return g_settings.islandAiUsage;
        case PillGroup::Commands:
            return g_settings.islandCommands;
        case PillGroup::Shelf:
            return g_settings.islandShelf && !Shelf().empty();
        case PillGroup::Eyes:
            return g_settings.islandEyes;
        default: {
            // A control center button: its own option.
            const int action = PillActionOfGroup(group);
            return action < 0 || GetIslandPref(kPillActions[action].pref) != 0;
        }
    }
}

// The usage thread runs while some of it is shown (for all apps, or for the
// app in front), or its panel is open.
void Island::UpdateUsageWatch() {
    // (Measured by the main island for all of them: while one of them shows
    // some, a monitor with a profile of its own too.)
    if (!m_main) {
        if (Island* main = MainIsland(); main && main != this) {
            IslandScope scope(main);
            main->UpdateUsageWatch();
        }
        return;
    }
    bool wanted = false;
    bool gpu = false;
    bool fast = false;
    ForEachIsland([&](Island& island) {
        if (!island.m_hwnd) {
            return;
        }
        // The GPU too when its video memory is shown (or all of it, shown
        // for an app), or its panel is open; every second with the panel
        // open (even when none of it is on the pill).
        const bool shown = AnyUsageShown();
        if (shown || island.PlaceShows(PillGroup::Usage)) {
            wanted = true;
            gpu = gpu || g_settings.usageVram || !shown;
        }
        if (island.m_usagePanel.IsOpen()) {
            wanted = true;
            fast = true;
            gpu = gpu ||
                  island.m_usagePanel.GetKind() == UsagePanel::Kind::Gpu;
        }
    });
    g_usageGpuWanted = gpu;
    g_usageFast = fast;
    if (!wanted || !m_hwnd) {
        StopUsageWatch();
        return;
    }
    if (m_usageThread) {
        return;
    }
    if (!m_usageStop) {
        m_usageStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    }
    if (!m_usageWake) {
        m_usageWake = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }
    if (!m_usageStop || !m_usageWake) {
        return;
    }
    ResetEvent(m_usageStop);
    auto* watch = new UsageWatch{m_hwnd, m_usageStop, m_usageWake};
    m_usageThread = CreateThread(nullptr, 0, UsageThreadProc, watch, 0, nullptr);
    if (!m_usageThread) {
        delete watch;
    }
}

// The usage panel opened, closed or shows another one: what's measured
// follows, and right away while it's open.
void Island::OnUsagePanelChanged() {
    UpdateUsageWatch();
    Island* main = MainIsland();
    if (main && main->m_usageWake && m_usagePanel.IsOpen()) {
        SetEvent(main->m_usageWake);
    }
}

void Island::StopUsageWatch() {
    if (m_usageThread) {
        SetEvent(m_usageStop);
        WaitForSingleObject(m_usageThread, INFINITE);
        CloseHandle(m_usageThread);
        m_usageThread = nullptr;
    }
    for (HANDLE* event : {&m_usageStop, &m_usageWake}) {
        if (*event) {
            CloseHandle(*event);
            *event = nullptr;
        }
    }
    AcquireSRWLockExclusive(&g_usageLock);
    g_usage = {};
    ReleaseSRWLockExclusive(&g_usageLock);
}

// New values measured: shown if they changed.
void Island::OnUsage() {
    const UsageDetails usage = GetUsage();
    const int cpu = usage.cpu;
    const int ram = usage.ram;
    const long long vram = usage.vramUsed;
    m_usagePanel.Refresh();
    // Back to every 2 seconds once the panels are closed.
    if (m_main) {
        g_usageFast = AnyIslandLook(
            [](Island& island) { return island.m_usagePanel.IsOpen(); });
    }
    // The video memory in megabytes.
    const double values[3] = {(double)cpu, (double)ram,
                              vram < 0 ? -1.0 : vram / (1024.0 * 1024)};
    bool changed = false;
    for (int i = 0; i < 3; i++) {
        if (values[i] == m_usageTo[i]) {
            continue;
        }
        changed = true;
        // From where it is now (still counting, maybe); unknown ones at once.
        const double now = NowSeconds();
        const float done = UsageChange(i);
        const double shown =
            m_usageFrom[i] < 0 || m_usageTo[i] < 0
                ? m_usageTo[i]
                : m_usageFrom[i] +
                      (m_usageTo[i] - m_usageFrom[i]) * EaseInOutCubic(done);
        m_usageFrom[i] = shown;
        m_usageTo[i] = values[i];
        m_usageChangedAt[i] = shown < 0 || values[i] < 0 ? 0 : now;
    }
    if (changed) {
        StartAnimating();
        Render();
    }
}

// The value as it's shown now: "23%", "1.6G", "850M", counting.
std::wstring Island::UsageText(int index) const {
    if (m_usageTo[index] < 0) {
        return L"\u2013";
    }
    const float done = UsageChange(index);
    const double value =
        m_usageFrom[index] < 0
            ? m_usageTo[index]
            : m_usageFrom[index] + (m_usageTo[index] - m_usageFrom[index]) *
                                       EaseInOutCubic(done);
    WCHAR text[16];
    if (index < 2) {
        swprintf_s(text, L"%d%%", (int)std::lround(value));
    } else if (value < 1000) {
        swprintf_s(text, L"%dM", (int)std::lround(value));
    } else {
        swprintf_s(text, L"%.1fG", value / 1024);
    }
    return text;
}

// A command added, changed, pinned or deleted: each island's pill laid out
// again (see PillGroup::Commands).
void CommandsChanged() {
    ForEachIsland([](Island& island) { island.OnOrderChanged(); });
}

// The items slide from where they were to their new places: each group's
// shift starts as the distance it moved, and springs back to nothing (see
// AnimationFrame).
void Island::OnOrderChanged() {
    // (Not taken by Layout as an order of its own to fade to.)
    m_layoutOrder.clear();
    float before[kPillGroups];
    for (int i = 0; i < kPillGroups; i++) {
        before[i] = m_groupBase[i] + (float)m_groupShift[i];
    }
    Layout();
    for (int i = 0; i < kPillGroups; i++) {
        if (before[i] >= 0 && m_groupBase[i] >= 0) {
            m_groupShift[i] = before[i] - m_groupBase[i];
        }
    }
    StartAnimating();
    Render(true);
}

// Another option chosen in the island: shown right away.
void Island::OnPreferencesChanged() {
    RefreshLook();
    UpdateClock();
    if (g_settings.pillActions) {
        m_nightLightState = GetNightLight();
        // (The radios' state comes a moment later, to light their buttons.)
        m_control.ReadRadios();
    }
    m_brightnessReadAt = 0;
    ReadBrightnessSoon();
    // (Kept open only in the pill: see islandTrayStyle.)
    m_trayKeepOpen =
        g_settings.islandTrayOpen && g_settings.islandTrayStyle == 0;
    m_trayOpen = m_trayKeepOpen;
    KillTimer(m_hwnd, kTrayCloseTimerId);
    if (g_settings.islandTrayStyle == 0) {
        m_trayListPanel.Close();
    }
    if (g_settings.islandMedia || g_mediaButtonsInPill || g_mediaTitleInPill) {
        StartMediaWatch();
    }
    UpdateUsageWatch();
    UpdateWeatherWatch();
    UpdateUpdatesWatch();
    UpdateNetSpeedWatch();
    UpdateRecycleWatch();
    UpdateAiUsageWatch();
    // (The fill's options and color, and whether explorer reads the apps'
    // progress for it.)
    UpdateFill();
    TellExplorerProgress();
    // (Lyrics turned on or off, another size.)
    m_lyricsFormat = nullptr;
    UpdateLyrics();
    m_extrasKey = -1;
    StartAnimating();
    Render(true);
    // The auto-hide, and the room it keeps (its size or its room from the
    // edges changed).
    UpdateAutoHide();
    UpdateClipboardWatch();
    UpdateShortcutKeys();
    UpdateInputMethodWatch();
    UpdatePrivacyWatch();
    UpdateDrivesWatch();
    UpdateDesktopsWatch();
    UpdateWorldClocksWatch();
    UpdateKomorebiWatch();
    // The app in focus, as its option is turned on or off (its title too).
    if (!g_settings.islandFocusedApp) {
        ClearFocusedApp();
    } else if (!m_focusWindow && !m_focusDesktop) {
        NoteFocusedApp(FindTopAppWindow(m_monitorHandle));
    } else if (ReadFocusTitle()) {
        Relayout();
    }
}

// Updates are looked for while their item is on the pill: 20 seconds after
// it's on (or the island starts), then as often as chosen (see
// UpdatesCheckSeconds).
void Island::UpdateUpdatesWatch() {
    if (!m_hwnd) {
        return;
    }
    // (Looked for by the main island, which tells the others: while one of
    // them shows them, a monitor with a profile of its own too.)
    if (!m_main) {
        if (Island* main = MainIsland(); main && main != this) {
            IslandScope scope(main);
            main->UpdateUpdatesWatch();
        }
        return;
    }
    if (!AnyIslandLook([](Island&) { return g_settings.islandUpdates; })) {
        KillTimer(m_hwnd, kUpdatesTimerId);
        m_updatesWatched = false;
        return;
    }
    if (!m_updatesWatched) {
        m_updatesWatched = true;
        // (A moment after the island starts, then as often as chosen.)
        SetTimer(m_hwnd, kUpdatesTimerId, 20 * 1000, nullptr);
    }
}

// The network's speed measured once a second by the main island while some
// island shows it (a monitor with a profile of its own too); no event tells
// it, so only then.
void Island::UpdateNetSpeedWatch() {
    if (!m_hwnd) {
        return;
    }
    if (Island* main = MainIsland(); main != this) {
        if (main) {
            IslandScope scope(main);
            main->UpdateNetSpeedWatch();
        }
        return;
    }
    const bool wanted =
        AnyIslandLook([](Island&) { return g_settings.islandNetSpeed; });
    if (wanted == m_netSpeedWatched) {
        return;
    }
    m_netSpeedWatched = wanted;
    g_netSpeed = {};
    if (!wanted) {
        KillTimer(m_hwnd, kNetSpeedTimerId);
        return;
    }
    // (The first look only counts: the speed comes with the next.)
    OnNetSpeedTimer();
    SetTimer(m_hwnd, kNetSpeedTimerId, kNetSpeedMs, nullptr);
}

void Island::OnNetSpeedTimer() {
    ULONG64 in = 0;
    ULONG64 out = 0;
    const ULONGLONG now = GetTickCount64();
    if (!ReadNetworkBytes(&in, &out)) {
        return;
    }
    const std::wstring down = g_netSpeed.down;
    const std::wstring up = g_netSpeed.up;
    // (An adapter gone makes the totals smaller: that look only counts.)
    if (g_netSpeed.at && now > g_netSpeed.at && in >= g_netSpeed.in &&
        out >= g_netSpeed.out) {
        const double seconds = (now - g_netSpeed.at) / 1000.0;
        g_netSpeed.down = FormatRate((in - g_netSpeed.in) / seconds);
        g_netSpeed.up = FormatRate((out - g_netSpeed.out) / seconds);
    }
    g_netSpeed.in = in;
    g_netSpeed.out = out;
    g_netSpeed.at = now;
    if (down != g_netSpeed.down || up != g_netSpeed.up) {
        ForEachIsland([](Island& island) {
            if (g_settings.islandNetSpeed) {
                island.Relayout();
            }
        });
    }
}

// The AI agents' files watched by the main island while some island shows
// them (a monitor with a profile of its own too): read when one changes,
// and once now.
void Island::UpdateAiUsageWatch() {
    if (!m_hwnd) {
        return;
    }
    if (Island* main = MainIsland(); main != this) {
        if (main) {
            IslandScope scope(main);
            main->UpdateAiUsageWatch();
        }
        return;
    }
    if (!AnyIslandLook([](Island&) { return g_settings.islandAiUsage; })) {
        StopAiUsageWatch();
        return;
    }
    if (m_aiUsageThread) {
        return;
    }
    m_aiUsageStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_aiUsageStop) {
        return;
    }
    auto* job = new AiUsageJob{m_hwnd, m_aiUsageStop};
    // (Its file only this account's before Claude Code writes there.)
    if (g_settings.islandAiClaude) {
        PrepareClaudeUsageFile();
    }
    const std::wstring status = ClaudeUsageFile();
    job->folders[(int)AiAgent::Claude] = status.substr(0, status.rfind(L'\\'));
    const std::wstring codex = CodexSessionsFolder();
    m_aiCodexWatched = !codex.empty() && GetFileAttributes(codex.c_str()) !=
                                             INVALID_FILE_ATTRIBUTES;
    if (m_aiCodexWatched) {
        job->folders[(int)AiAgent::Codex] = codex;
    }
    m_aiUsageThread =
        CreateThread(nullptr, 0, AiUsageThreadProc, job, 0, nullptr);
    if (!m_aiUsageThread) {
        delete job;
        CloseHandle(m_aiUsageStop);
        m_aiUsageStop = nullptr;
        return;
    }
    SetTimer(m_hwnd, kAiUsageTimerId, kAiUsageMinuteMs, nullptr);
}

void Island::StopAiUsageWatch() {
    if (m_hwnd) {
        KillTimer(m_hwnd, kAiUsageTimerId);
    }
    if (m_aiUsageThread) {
        SetEvent(m_aiUsageStop);
        WaitForSingleObject(m_aiUsageThread, INFINITE);
        CloseHandle(m_aiUsageThread);
        m_aiUsageThread = nullptr;
        // (What it read and posted, not taken yet.)
        MSG msg;
        while (m_hwnd && PeekMessage(&msg, m_hwnd, WM_APP_AI_USAGE,
                                     WM_APP_AI_USAGE, PM_REMOVE)) {
            delete (AiUsageRead*)msg.lParam;
        }
    }
    if (m_aiUsageStop) {
        CloseHandle(m_aiUsageStop);
        m_aiUsageStop = nullptr;
    }
}

// Each minute: the time left goes down (laid out again, nothing read); Codex's
// sessions watched once they're there, and the watch started again if its
// thread ended by itself (a folder it watched gone).
void Island::OnAiUsageMinute() {
    if (m_aiUsageThread &&
        WaitForSingleObject(m_aiUsageThread, 0) == WAIT_OBJECT_0) {
        StopAiUsageWatch();
        UpdateAiUsageWatch();
        return;
    }
    if (!m_aiCodexWatched) {
        const std::wstring codex = CodexSessionsFolder();
        if (!codex.empty() &&
            GetFileAttributes(codex.c_str()) != INVALID_FILE_ATTRIBUTES) {
            StopAiUsageWatch();
            UpdateAiUsageWatch();
            return;
        }
    }
    ForEachIsland([](Island& island) {
        if (g_settings.islandAiUsage) {
            island.Relayout();
            island.m_aiUsagePanel.Refresh();
        }
    });
}

// An agent's app (its Start menu ID), whose icon stands for it on the pill:
// the first of its apps installed (Claude's from the Store or its
// installer; OpenAI's Codex app); empty without one.
std::wstring Island::AiAgentApp(int agent) {
    static constexpr PCWSTR kApps[kAiAgents][2] = {
        {L"Claude_pzs8sxrjxfjjc!Claude",
         L"com.squirrel.AnthropicClaude.claude"},
        {L"OpenAI.Codex_2p2nqsd0c76g0!App", nullptr}};
    for (PCWSTR app : kApps[agent]) {
        if (app && GetAppBitmap(app)) {
            return app;
        }
    }
    return {};
}

// Both agents as the watching thread read them (another island's look may
// show the one this one's doesn't), and the islands showing them laid out
// again.
void Island::OnAiUsageRead(const AiUsageRead& read) {
    if (read.claudeRead) {
        g_aiUsage[(int)AiAgent::Claude] = read.claude;
    }
    g_aiUsage[(int)AiAgent::Codex] = read.codex;
    ForEachIsland([](Island& island) {
        if (g_settings.islandAiUsage) {
            island.Relayout();
            island.m_aiUsagePanel.Refresh();
        }
    });
}

// The Recycle Bin watched by the main island while some island shows it:
// Windows tells it each change (see WM_APP_RECYCLE_CHANGED), and it's
// counted again a moment later.
void Island::UpdateRecycleWatch() {
    if (!m_hwnd) {
        return;
    }
    if (Island* main = MainIsland(); main != this) {
        if (main) {
            IslandScope scope(main);
            main->UpdateRecycleWatch();
        }
        return;
    }
    const bool wanted =
        AnyIslandLook([](Island&) { return g_settings.islandRecycleBin; });
    if (wanted == (m_recycleNotify != 0)) {
        return;
    }
    if (!wanted) {
        SHChangeNotifyDeregister(m_recycleNotify);
        m_recycleNotify = 0;
        KillTimer(m_hwnd, kRecycleTimerId);
        return;
    }
    PIDLIST_ABSOLUTE bin = nullptr;
    if (FAILED(SHGetKnownFolderIDList(FOLDERID_RecycleBinFolder, 0, nullptr,
                                      &bin))) {
        return;
    }
    SHChangeNotifyEntry entry{bin, TRUE};
    m_recycleNotify = SHChangeNotifyRegister(
        m_hwnd,
        SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery,
        SHCNE_ALLEVENTS, WM_APP_RECYCLE_CHANGED, 1, &entry);
    CoTaskMemFree(bin);
    if (!m_recycleNotify) {
        IslandLog(L"The Recycle Bin can't be watched");
    }
    StartRecycleCount();
}

void Island::TellExplorerProgress() {
    if (!m_hwnd) {
        return;
    }
    if (Island* main = MainIsland(); main != this) {
        if (main) {
            IslandScope scope(main);
            main->TellExplorerProgress();
        }
        return;
    }
    const int wanted = AnyIslandLook([](Island&) {
                           return g_settings.islandDownloadFill;
                       })
                           ? 1
                           : 0;
    // (Told only when the taskbar's part took it: not yet there, after
    // Explorer started again, it's told again on a later tick.)
    DWORD_PTR answer = 0;
    if (wanted != m_progressTold &&
        SendIpc(FindTaskbarWnd(), m_hwnd, kIpcProgressWanted, &wanted,
                sizeof(wanted), &answer) &&
        answer == TRUE) {
        m_progressTold = wanted;
    }
}

// Counted on a thread (a bin with many files takes a moment); asked again
// while one counts (or its count is still on its way), once more after it
// (see OnRecycleCounted, which waits only for the thread that told).
void Island::StartRecycleCount() {
    if (m_recycleThread) {
        m_recycleAgain = true;
        return;
    }
    m_recycleThread =
        CreateThread(nullptr, 0, RecycleCountProc, m_hwnd, 0, nullptr);
}

void Island::OnRecycleCounted(WPARAM items, LPARAM kilobytes) {
    if (m_recycleThread) {
        // (It ends right after telling.)
        WaitForSingleObject(m_recycleThread, INFINITE);
        CloseHandle(m_recycleThread);
        m_recycleThread = nullptr;
    }
    if (m_recycleAgain) {
        m_recycleAgain = false;
        StartRecycleCount();
    }
    // (Not counted: the last count stays.)
    if ((INT_PTR)items < 0) {
        return;
    }
    g_recycleItems = (long long)items;
    g_recycleKilobytes = (long long)kilobytes;
    ForEachIsland([](Island& island) {
        island.m_recyclePanel.SetContents(g_recycleItems, g_recycleKilobytes);
        island.UpdateExtraItems();
    });
}

// The Recycle Bin's count, the world clocks and the next event: laid out
// again when they change (the clocks and the event looked at each minute,
// right after their options change, and the clocks when Windows' change,
// see WM_APP_WORLD_CLOCKS).
void Island::UpdateExtraItems() {
    bool changed = false;
    const std::wstring bin = g_settings.islandRecycleBin && g_recycleItems > 0
                                 ? std::to_wstring(g_recycleItems)
                                 : std::wstring();
    if (bin != m_recycleText.text) {
        m_recycleText.text = bin;
        changed = true;
    }
    if (RecycleBinShown() != m_recycleShown) {
        m_recycleShown = !m_recycleShown;
        changed = true;
    }
    SYSTEMTIME now;
    GetLocalTime(&now);
    // (Again each minute, when an option changes, and when a timer starts,
    // stops or passes a minute, or the alarms change.)
    // (A hash: unsigned, so it may wrap.)
    unsigned long long timers = 0;
    for (TimerMode mode : {TimerMode::Timer, TimerMode::Pomodoro}) {
        timers = timers * 2003 +
                 (TimerClockOf(mode).running
                      ? (unsigned long long)std::ceil(TimerLeft(mode) / 60) + 1
                      : 0);
    }
    for (const auto& alarm : Alarms()) {
        timers = timers * 31 + (unsigned long long)alarm.minute * 2 +
                 (alarm.on ? 1 : 0);
        timers = timers * 131 + (unsigned long long)alarm.days;
    }
    const unsigned long long minute =
        (unsigned long long)DayNumber(now.wYear, now.wMonth, now.wDay) * 24 *
            60 +
        now.wHour * 60 + now.wMinute;
    const long long key = (long long)(
        (minute * 128 + (g_settings.islandWorldClocks ? 1 : 0) +
         (g_settings.islandCountdown ? 2 : 0) +
         (g_settings.islandCountdownEvents ? 4 : 0) +
         (g_settings.islandCountdownAlarms ? 8 : 0) +
         (g_settings.islandCountdownTimer ? 16 : 0) +
         (g_settings.islandCountdownPomodoro ? 32 : 0) +
         (g_settings.islandTimer ? 64 : 0)) ^
        (timers * 1000003ull));
    if (key != m_extrasKey) {
        m_extrasKey = key;
        std::vector<WorldClock> clocks;
        if (g_settings.islandWorldClocks) {
            clocks = ReadWorldClocks();
        }
        if (clocks != m_worldClocks) {
            m_worldClocks = std::move(clocks);
            changed = true;
        }
        std::wstring title;
        std::wstring text;
        if (g_settings.islandCountdown) {
            // The soonest of what's chosen: an event (yours or an
            // agenda's), an alarm that's on, the timer's end, Pomodoro's
            // next part (the ones running; within 12 hours).
            int best = 12 * 60 + 1;
            int minutes = 0;
            int year = 0;
            int month = 0;
            int day = 0;
            std::wstring name;
            if (g_settings.islandCountdownEvents &&
                NextTimedEvent(&name, &minutes, &year, &month, &day) &&
                minutes < best) {
                best = minutes;
                title = name;
                m_countdownKind = kCountdownEvent;
                m_countdownDay[0] = year;
                m_countdownDay[1] = month;
                m_countdownDay[2] = day;
            }
            if (g_settings.islandCountdownAlarms) {
                for (const auto& alarm : Alarms()) {
                    // (Its minute now: it rang already; on its days.)
                    const int until = AlarmMinutesUntil(alarm, now);
                    if (alarm.on && until < best) {
                        best = until;
                        title = alarm.name.empty() ? Tr(L"Alarm", L"Alarme")
                                                   : alarm.name;
                        m_countdownKind = kCountdownAlarm;
                    }
                }
            }
            // (Not with the timer's own item on the pill: it shows them
            // already, to the second.)
            for (TimerMode mode : {TimerMode::Timer, TimerMode::Pomodoro}) {
                const bool chosen = !g_settings.islandTimer &&
                    (mode == TimerMode::Timer
                         ? g_settings.islandCountdownTimer
                         : g_settings.islandCountdownPomodoro);
                if (!chosen || !TimerClockOf(mode).running) {
                    continue;
                }
                const int until = (int)std::ceil(TimerLeft(mode) / 60);
                if (until <= best) {
                    best = until;
                    title = TimerModeName(mode, g_timer.onBreak);
                    m_countdownKind = mode == TimerMode::Timer
                                          ? kCountdownTimer
                                          : kCountdownPomodoro;
                }
            }
            if (best <= 12 * 60) {
                text = UntilText(best);
            } else {
                title.clear();
            }
        }
        if (title != m_countdownTitle || text != m_countdownText) {
            m_countdownTitle = title;
            m_countdownText = text;
            changed = true;
        }
    }
    if (changed) {
        Relayout();
    }
}

// As wide as the wider line, up to kItemNameMaxWidth (`widest` measured
// for the value when given: figures changing every second keep their room).
float Island::StackedWidth(const std::wstring& label,
                           const std::wstring& value,
                           const std::wstring& widest) {
    if (m_stackedScale != m_usageScale) {
        m_stackedScale = m_usageScale;
        m_stackedWidths.clear();
    }
    const std::wstring& measured = widest.empty() ? value : widest;
    const std::wstring key = label + L"\n" + measured;
    for (const auto& [text, width] : m_stackedWidths) {
        if (text == key) {
            return width;
        }
    }
    float width = std::round(30 * m_usageScale);
    auto measure = [&](IDWriteTextFormat* format, const std::wstring& text) {
        winrt::com_ptr<IDWriteTextLayout> layout;
        if (format && !text.empty() &&
            SUCCEEDED(m_dwrite->CreateTextLayout(
                text.c_str(), (UINT32)text.size(), format, 2000, 100,
                layout.put()))) {
            DWRITE_TEXT_METRICS metrics{};
            layout->GetMetrics(&metrics);
            width = std::max(
                width, std::ceil(metrics.widthIncludingTrailingWhitespace +
                                 8 * m_usageScale));
        }
    };
    measure(m_usageLabelFormat.get(), label);
    measure(m_usageValueFormat.get(), measured);
    // (A long name, like an event's, is cut with "...".)
    width = std::min(width, std::round(kItemNameMaxWidth * m_usageScale));
    // (Texts change, like the next event's: the old ones go.)
    if (m_stackedWidths.size() >= 32) {
        m_stackedWidths.clear();
    }
    m_stackedWidths.push_back({key, width});
    return width;
}

void Island::StartUpdatesJob() {
    // (The main island looks, and tells the others.)
    if (!m_main) {
        if (Island* main = MainIsland(); main && main != this) {
            IslandScope scope(main);
            main->StartUpdatesJob();
        }
        return;
    }
    if (!m_updatesStop) {
        return;
    }
    std::erase_if(m_updatesThreads, [](HANDLE thread) {
        if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0) {
            CloseHandle(thread);
            return true;
        }
        return false;
    });
    // One look at a time.
    AcquireSRWLockExclusive(&g_updatesLock);
    const bool busy = g_updates.checking;
    g_updates.checking = true;
    ReleaseSRWLockExclusive(&g_updatesLock);
    if (busy) {
        return;
    }
    // The thread gets its own handle of the stop event, so it stays valid
    // even if the island gives up waiting for it.
    HANDLE stop = nullptr;
    HANDLE thread = nullptr;
    if (DuplicateHandle(GetCurrentProcess(), m_updatesStop,
                        GetCurrentProcess(), &stop, 0, FALSE,
                        DUPLICATE_SAME_ACCESS)) {
        auto* job = new UpdatesJob{m_hwnd, stop};
        thread = CreateThread(nullptr, 0, UpdatesThreadProc, job, 0, nullptr);
        if (thread) {
            m_updatesThreads.push_back(thread);
        } else {
            delete job;
            CloseHandle(stop);
        }
    }
    if (!thread) {
        AcquireSRWLockExclusive(&g_updatesLock);
        g_updates.checking = false;
        ReleaseSRWLockExclusive(&g_updatesLock);
    }
    m_updatesPanel.Refresh();
}

void Island::OnUpdates() {
    const UpdatesData updates = GetUpdates();
    int count = (int)updates.windows.size();
    count += (int)std::count_if(updates.apps.begin(), updates.apps.end(),
                                [](const AppUpdate& app) {
                                    return !app.ignored;
                                });
    m_updatesCount = count;
    m_updatesPanel.Refresh();
    Render();
}

bool Island::WeatherWanted() const {
    // (An app's own icons can't turn it on: see OnlineGroupAllowed.)
    return g_settings.islandWeather;
}

bool Island::AnyWeatherWanted() const {
    return AnyIslandLook(
        [](Island& island) { return island.WeatherWanted(); });
}

// The weather is measured while it's on the pill: now (if it's old), and
// every 30 minutes.
void Island::UpdateWeatherWatch() {
    if (!m_hwnd) {
        return;
    }
    const WeatherData weather = GetWeather();
    if (WeatherWanted()) {
        ShowWeather(weather);
    }
    // (Measured by the main island, which tells the others: while one of
    // them shows it, a monitor with a profile of its own too.)
    if (!m_main) {
        if (Island* main = MainIsland(); main && main != this) {
            IslandScope scope(main);
            main->UpdateWeatherWatch();
        }
        return;
    }
    if (!AnyWeatherWanted()) {
        KillTimer(m_hwnd, kWeatherTimerId);
        KillTimer(m_hwnd, kWeatherRetryTimerId);
        return;
    }
    if (!weather.valid ||
        NowSeconds() - weather.fetchedAt > kWeatherRefreshSeconds) {
        StartWeather({});
    }
    SetTimer(m_hwnd, kWeatherTimerId, (UINT)(kWeatherRefreshSeconds * 1000),
             nullptr);
}

// The sky and the temperature now, on the pill.
void Island::ShowWeather(const WeatherData& weather) {
    const std::wstring shown =
        weather.valid ? WeatherEmoji(weather.code, weather.day) + L"\t" +
                            Degrees(weather.temperature)
                      : std::wstring();
    if (shown != m_weatherShown) {
        m_weatherBefore = m_weatherShown;
        m_weatherSwap = 1;
        m_weatherSwapVelocity = 0;
        StartAnimating();
    }
    m_weatherShown = shown;
}

// Asks for the weather (of a city typed, or of the one chosen), on its
// thread; nothing without a city.
void Island::StartWeather(const std::wstring& search) {
    // (The main island asks, and tells the others.)
    if (!m_main) {
        if (Island* main = MainIsland(); main && main != this) {
            IslandScope scope(main);
            main->StartWeather(search);
        }
        return;
    }
    if (m_weatherThread) {
        if (WaitForSingleObject(m_weatherThread, 0) != WAIT_OBJECT_0) {
            if (!search.empty()) {
                m_weatherPendingSearch = search;
            }
            return;
        }
        CloseHandle(m_weatherThread);
        m_weatherThread = nullptr;
    }
    // (A city typed: back to it, from "Use my location".)
    if (!search.empty()) {
        Wh_SetIntValue(L"weatherMyLocation", 0);
    }
    auto* request = new WeatherRequest{m_hwnd, search, LoadWeatherPlace(),
                                       search.empty() &&
                                           Wh_GetIntValue(L"weatherMyLocation",
                                                          0) != 0,
                                       g_weatherLocateClicks};
    if (search.empty() && request->place.name.empty() && !request->locate) {
        delete request;
        return;
    }
    g_weatherLocateAgain = false;
    g_weatherStop = false;
    m_weatherThread =
        CreateThread(nullptr, 0, WeatherThreadProc, request, 0, nullptr);
    if (!m_weatherThread) {
        delete request;
    }
}

void Island::OnWeather(int problem) {
    // The service didn't answer (no internet yet, at startup): asked again
    // in 15 s, then waiting longer each time (up to 5 minutes), and as soon
    // as the internet comes (see OnInternetBack). The same when the location
    // wasn't found yet (4): "Use my location" stays chosen.
    if ((problem == 2 || problem == 4) && m_main && AnyWeatherWanted()) {
        static constexpr UINT kRetrySeconds[] = {15, 30, 60, 120, 300};
        const UINT seconds =
            kRetrySeconds[std::min(m_weatherRetries,
                                   (int)ARRAYSIZE(kRetrySeconds) - 1)];
        m_weatherRetries++;
        SetTimer(m_hwnd, kWeatherRetryTimerId, seconds * 1000, nullptr);
    } else if (problem == 0) {
        m_weatherRetries = 0;
        KillTimer(m_hwnd, kWeatherRetryTimerId);
    } else if (problem == 3 && m_main && AnyWeatherWanted() &&
               !LoadWeatherPlace().name.empty()) {
        // My location not found (refused or off): the city typed, at once.
        SetTimer(m_hwnd, kWeatherRetryTimerId, 1000, nullptr);
    }
    ShowWeather(GetWeather());
    // A city typed meanwhile: looked for now (the panel keeps waiting for
    // it).
    if (!m_weatherPendingSearch.empty()) {
        const std::wstring search = std::move(m_weatherPendingSearch);
        m_weatherPendingSearch.clear();
        if (m_weatherThread) {
            // It's ending (it just told this).
            WaitForSingleObject(m_weatherThread, 2000);
        }
        StartWeather(search);
        Relayout();
        return;
    }
    // "Use my location" clicked meanwhile: asked now, not lost.
    if (g_weatherLocateAgain && m_main) {
        if (m_weatherThread) {
            WaitForSingleObject(m_weatherThread, 2000);
        }
        StartWeather({});
        Relayout();
        return;
    }
    // (Every island's panel: one searching on another monitor waits for
    // this answer too.)
    ForEachIsland([&](Island& island) {
        IslandScope scope(&island);
        island.m_weatherPanel.OnWeather(problem);
    });
    // Its emojis made now, not as it opens.
    if (problem == 0) {
        m_weatherPanel.WarmUp(PanelScale());
    }
    Relayout();
}

// Windhawk run as administrator: the island's process is elevated, and
// Windows keeps explorer's messages from its windows unless each is allowed
// (nothing changes for one that isn't elevated): the apps' badges and tray
// icons (WM_COPYDATA, still checked against its sender), a download's
// progress, the bar's strip moved, and the taskbar started again.
void AllowExplorerMessages(HWND hwnd) {
    static const UINT taskbarCreated = RegisterWindowMessage(L"TaskbarCreated");
    for (UINT message :
         {(UINT)WM_COPYDATA, WM_APP_PROGRESS, WM_APP_APPBAR, taskbarCreated}) {
        if (message) {
            ChangeWindowMessageFilterEx(hwnd, message, MSGFLT_ALLOW, nullptr);
        }
    }
}

bool Island::Create() {
    m_bar = g_settings.islandBar;
    m_bannerReadStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    LoadUnread();
    if (m_main) {
        LoadSnoozed();
        AllowTrayPlaces();
    }
    for (int i = 0; i < kPillGroups; i++) {
        m_groupShown[i] = GroupWanted((PillGroup)i) ? 1 : 0;
    }
    m_placeApp =
        PerAppPlacesOn() ? WindowAppKey(GetForegroundWindow()) : std::wstring();
    m_placeX = m_bar ? 0.5 : CurrentPlace().x;
    m_placeY = m_bar ? 0 : CurrentPlace().y;
    m_minimized = MinimizableAt(CurrentPlace()) &&
                  Wh_GetIntValue(MinimizedKey().c_str(), 0) != 0;
    // (Notches hiding together: as the bell's, made first.)
    if (m_notchBase && !g_settings.islandNotchEach) {
        m_minimized = m_notchBase->m_minimized;
    }
    m_expand = m_minimized ? 0 : 1;

    m_hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        kIslandClassName, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, g_module,
        nullptr);
    if (!m_hwnd) {
        IslandLog(L"CreateWindowEx failed: %u", GetLastError());
        return false;
    }
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
    AllowExplorerMessages(m_hwnd);
    m_drop.Register(m_hwnd, this);

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                 m_d2d.put())) ||
        FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                   __uuidof(IDWriteFactory),
                                   (IUnknown**)m_dwrite.put())) ||
        !m_canvas.Create(m_d2d.get())) {
        IslandLog(L"Direct2D/DirectWrite failed");
        return false;
    }
    if (!m_calendar.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_control.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_notifications.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_mediaPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_power.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_profilesPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_drivesPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_recyclePanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_aiUsagePanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_commandsPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_shelfPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_lyricsPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_timerPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_trayListPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_keyboardPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_desktopsPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_foldersPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_wallpaperPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_komorebiPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_usagePanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_notesPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_tasksPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_shortcutsPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_weatherPanel.Create(m_d2d.get(), m_dwrite.get()) ||
        !m_updatesPanel.Create(m_d2d.get(), m_dwrite.get())) {
        IslandLog(L"A panel couldn't be created");
    }

    CoCreateInstance(kCLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL,
                     kIID_IMMDeviceEnumerator, m_devices.put_void());
    CoCreateInstance(kCLSID_NetworkListManager, nullptr, CLSCTX_ALL,
                     kIID_INetworkListManager, m_network.put_void());
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARGS(m_wic.put()));
    DWORD version;
    if (WlanOpenHandle(2, nullptr, &version, &m_wlan) != ERROR_SUCCESS) {
        m_wlan = nullptr;
    }

    m_weatherPanel.search = [this](const std::wstring& city) {
        StartWeather(city);
    };
    m_weatherPanel.locate = [this] {
        Wh_SetIntValue(L"weatherMyLocation", 1);
        // (Counted, so a fetch that ends later doesn't undo this click; and
        // asked again if one is still going, see OnWeather.)
        g_weatherLocateClicks++;
        g_weatherLocateMisses = 0;
        g_weatherLocateAgain = true;
        StartWeather({});
    };
    m_updatesPanel.check = [this] { StartUpdatesJob(); };
    m_updatesPanel.changed = [this] { OnUpdates(); };
    // (Another interval: the timer looks at once when the next look is due.)
    m_updatesPanel.everyChanged = [] {
        if (Island* main = MainIsland(); main && main->m_updatesWatched) {
            SetTimer(main->m_hwnd, kUpdatesTimerId, 1000, nullptr);
        }
    };
    m_updatesStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    UpdateClock();
    UpdateStatus();
    m_darkMode = IsDarkMode();
    m_themeTurn = m_darkMode ? 1 : 0;
    g_transparencyEffects = TransparencyEffectsOn();
    Layout();
    StartStatusWatch();
    // (What they are now: notices only for what changes later.)
    CheckPowerNotices();
    CheckBluetoothOutputs();
    NoteQuiet(IsIslandQuiet());
    UpdateUsageWatch();
    UpdateWeatherWatch();
    UpdateUpdatesWatch();
    UpdateNetSpeedWatch();
    UpdateRecycleWatch();
    UpdateAiUsageWatch();
    UpdateWindowEvents();

    RegisterAppBar();
    UpdateAutoHide();
    UpdateClipboardWatch();
    UpdateShortcutKeys();
    UpdateInputMethodWatch();
    UpdatePrivacyWatch();
    UpdateDrivesWatch();
    UpdateDesktopsWatch();
    UpdateWorldClocksWatch();
    UpdateKomorebiWatch();
    NoteFocusedApp(FindTopAppWindow(m_monitorHandle));

    m_trayKeepOpen = Wh_GetIntValue(L"trayKeepOpen",
                                    g_settings.islandTrayOpen ? 1 : 0) != 0 &&
                     g_settings.islandTrayStyle == 0;
    // (An island showing a profile of its own has them from its look.)
    if (!HasOwnLook()) {
        g_bannerEnabled = Wh_GetIntValue(L"bannerEnabled",
                                         g_settings.islandBanner ? 1 : 0) != 0;
        g_bannerWhenMinimized =
            Wh_GetIntValue(L"bannerMinimized",
                           g_settings.islandBannerMinimized ? 1 : 0) != 0;
    }
    // The app in front (told to every island), and the Windows banner: when
    // it's uncloaked and cloaked, and as it moves (the main island's).
    if (m_main) {
        m_foregroundHook = SetWinEventHook(
            EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
            IslandForegroundProc, 0, 0, WINEVENT_OUTOFCONTEXT);
        m_bannerHook = SetWinEventHook(kEventObjectCloaked,
                                       kEventObjectUncloaked, nullptr,
                                       BannerWinEventProc, 0, 0,
                                       WINEVENT_OUTOFCONTEXT);
    }
    // A stack of banners may be showing already: its process's moves are
    // watched from the start, to know it as banners join it.
    if (HANDLE snapshot =
            m_main ? CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
                   : INVALID_HANDLE_VALUE;
        snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 entry{sizeof(entry)};
        for (BOOL more = Process32First(snapshot, &entry); more;
             more = Process32Next(snapshot, &entry)) {
            if (_wcsicmp(entry.szExeFile, L"ShellExperienceHost.exe") == 0 &&
                IsShellExperienceHost(entry.th32ProcessID)) {
                WatchWindowsBannerMoves(entry.th32ProcessID);
                break;
            }
        }
        CloseHandle(snapshot);
    }
    m_trayOpen = m_trayKeepOpen;
    m_trayAmount = m_trayBounce = m_trayArrow = m_trayOpen ? 1 : 0;
    g_showNotificationContent =
        Wh_GetIntValue(L"notificationContent", 1) != 0;

    Render(true);
    // Explorer sends the badges and the tray icons when it hears from it
    // (and again whenever it restarts, see "TaskbarCreated"), to the main
    // island (which tells the others).
    if (m_main) {
        SayHello();
    }
    // Shown right away, still off the screen and transparent: Windows' own
    // animation for showing a window draws it as a plain rectangle, so it
    // runs where it can't be seen (see m_away).
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    m_shownOnce = true;
    if (m_main) {
        g_islandWnd = m_hwnd;
    }
    UpdateApps();
    UpdateTrayIcons();
    if (!HasOwnLook()) {
        g_mediaButtonsInPill =
            Wh_GetIntValue(L"mediaButtons",
                           g_settings.islandMediaButtons ? 1 : 0) != 0;
        g_mediaTitleInPill = Wh_GetIntValue(L"mediaTitle", 0) != 0;
    }
    m_mediaButtonsAmount = m_mediaButtonsRoom = g_mediaButtonsInPill ? 1 : 0;
    m_mediaTitleRoom = g_mediaTitleInPill ? 1 : 0;
    if (m_main && (g_settings.islandMedia || g_mediaButtonsInPill ||
                   g_mediaTitleInPill)) {
        StartMediaWatch();
    }
    // A download going on while the islands started again (another style):
    // filling at once, from what explorer last told.
    UpdateFill();
    ScheduleTick();
    return true;
}

// The next tick, right after the clock's next second.
void Island::ScheduleTick() {
    SYSTEMTIME now;
    GetLocalTime(&now);
    SetTimer(m_hwnd, kIslandTimerId, 1000 - now.wMilliseconds + 15, nullptr);
}

// (See UpdateShortcutKeys.)
void StopShortcutKeys();
extern std::atomic<int> g_shortcutKeys;

