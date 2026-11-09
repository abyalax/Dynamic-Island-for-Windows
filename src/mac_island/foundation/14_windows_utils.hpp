#pragma once
// Whether an island's process runs: its mutex exists while it does (one it
// can't open exists too). In the island's own process once it released its
// own (see WhTool_ModUninit): a new version's.
bool IslandProcessRunning() {
    if (HANDLE running =
            OpenMutex(SYNCHRONIZE, FALSE, L"windhawk-tool-mod_" WH_MOD_ID)) {
        CloseHandle(running);
        return true;
    }
    return GetLastError() == ERROR_ACCESS_DENIED;
}

// In the island's process, on the island's thread (COM is ready there).
void UpdateWindowsQuiet(bool islandRunning) {
    const bool wanted = islandRunning && g_settings.islandOwnsNotifications &&
                        g_toastWatchReady && !g_unloading;
    // Not wanted (the island stopping, or the option off): it going off now
    // is the island's own doing, not Windows'.
    if (!wanted) {
        g_dndSeenOn = false;
    }
    // Turned off from Windows meanwhile: left off (not fought over), and
    // the island works as usual meanwhile (see WindowsQuietForIsland).
    if (wanted && (g_turnedDndOn || g_dndSeenOn)) {
        if (IsDoNotDisturbOn()) {
            g_dndSeenOn = true;
        } else if (!g_turnedDndOn || NowSeconds() - g_turnedDndOnAt > 5) {
            if (g_turnedDndOn) {
                g_turnedDndOn = false;
                SetUserIntValue(L"turnedDndOn", 0);
            }
            g_dndSeenOn = false;
            g_dndSwitchFailed = true;
            IslandLog(L"\"Do not disturb\" was turned off from Windows: left off");
            return;
        }
    }
    if (wanted && !g_turnedDndOn) {
        if (g_dndSwitchFailed) {
            return;
        }
        // Already on: left alone.
        if (IsDoNotDisturbOn()) {
            g_dndSeenOn = true;
            return;
        }
        if (SetDoNotDisturb(true, g_islandQuiet)) {
            g_turnedDndOn = true;
            g_turnedDndOnAt = NowSeconds();
            g_turnedDndAlarms = g_islandQuiet;
            SetUserIntValue(L"turnedDndOn", 1);
        } else {
            g_dndSwitchFailed = true;
        }
    } else if (wanted && g_turnedDndOn && g_turnedDndAlarms != g_islandQuiet) {
        // The island's own "Do not disturb" switched (or not known yet):
        // Windows' follows it.
        if (SetDoNotDisturb(true, g_islandQuiet)) {
            g_turnedDndAlarms = g_islandQuiet;
        }
    } else if (!wanted && g_turnedDndOn) {
        // Stopping for a new version's island (it started meanwhile, and goes
        // on with it): left on.
        if (g_unloading && IslandProcessRunning()) {
            return;
        }
        if (SetDoNotDisturb(false)) {
            g_turnedDndOn = false;
            g_turnedDndAlarms = -1;
            SetUserIntValue(L"turnedDndOn", 0);
        }
    }
}

// Whether the island has the notifications to itself now: its own
// notifications, with Windows' "Do not disturb" on (turned on by the island,
// or already on). If it's off (turned off from Windows, or it couldn't be
// switched), Windows shows its banners and plays its sounds, so the island
// works as usual (taking the banners, not playing sounds).
bool WindowsQuietForIsland() {
    return g_settings.islandOwnsNotifications && IsDoNotDisturbOn();
}

// "Do not disturb" as the island shows it and switches it: its own with its
// own notifications, otherwise Windows'.
bool IsIslandQuiet() {
    return g_settings.islandOwnsNotifications ? g_islandQuiet.load()
                                              : IsDoNotDisturbOn();
}

// The island's "Do not disturb" keeping calls, alarms and reminders silent
// too (islandDndSilent; they still show on the pill).
bool QuietSilencesAll() {
    return g_settings.islandOwnsNotifications && g_settings.islandDndSilent &&
           g_islandQuiet.load();
}

bool SetIslandQuiet(bool on) {
    if (g_settings.islandOwnsNotifications) {
        g_islandQuiet = on;
        Wh_SetIntValue(L"islandQuiet", on);
        return true;
    }
    return SetDoNotDisturb(on);
}

// Turned on by an island that ended without turning it back off (the
// computer turned off meanwhile), and no island takes it over now: back off
// (as the island's process starts).
void RestoreLeftoverDnd() {
    if (!g_turnedDndOn ||
        (g_settings.showIsland && g_settings.islandOwnsNotifications)) {
        return;
    }
    const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SetDoNotDisturb(false)) {
        g_turnedDndOn = false;
        SetUserIntValue(L"turnedDndOn", 0);
        IslandLog(L"\"Do not disturb\" left on by the island: turned back off");
    }
    if (SUCCEEDED(init)) {
        CoUninitialize();
    }
}

// Explorer keeps an eye on the island's process (told by its hello): if it
// ends while the island's own notifications have Windows' "Do not disturb"
// on (it crashed), explorer turns it back off.
HANDLE g_islandProcess;
HANDLE g_islandProcessWait;
DWORD g_islandProcessId;
SRWLOCK g_islandWatchLock = SRWLOCK_INIT;

// (On a thread of its own: explorer's threads have their own apartments.)
DWORD WINAPI TurnLeftoverDndOffThreadProc(LPVOID) {
    const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (GetUserIntValue(L"turnedDndOn", 0) && SetDoNotDisturb(false)) {
        SetUserIntValue(L"turnedDndOn", 0);
        IslandLog(L"The island stopped: \"Do not disturb\" turned back off");
    }
    if (SUCCEEDED(init)) {
        CoUninitialize();
    }
    return 0;
}

VOID CALLBACK IslandProcessEndedCallback(PVOID, BOOLEAN) {
    // A new version's island may still be starting (the old one's process
    // can end before the new one's is up): given a few seconds to take it
    // over. (In short steps: StopWatchingIslandProcess waits for this.)
    constexpr int kTakeOverSteps = 30;
    for (int step = 0; step < kTakeOverSteps; step++) {
        if (g_unloading || IslandProcessRunning()) {
            return;
        }
        Sleep(100);
    }
    if (g_unloading || IslandProcessRunning()) {
        return;
    }
    TurnLeftoverDndOffThreadProc(nullptr);
}

// The explorer.exe part unloading (the mod turned off or removed) while no
// island's process runs (it ended without turning "Do not disturb" back off,
// and didn't start again): turned back off here, since nothing else would.
// Not while the island's process is still ending (it releases its mutex
// first, see WhTool_ModUninit): it turns it off itself, or hands it to a new
// version's. Only by the taskbar's explorer.exe (the one the island talks
// to): another one (a folder window's) never knew the island's process.
void RestoreDndWithoutIsland() {
    if (!OwnsTaskbar() || !GetUserIntValue(L"turnedDndOn", 0) ||
        IslandProcessRunning()) {
        return;
    }
    AcquireSRWLockShared(&g_islandWatchLock);
    const bool ending = g_islandProcess &&
                        WaitForSingleObject(g_islandProcess, 0) == WAIT_TIMEOUT;
    ReleaseSRWLockShared(&g_islandWatchLock);
    if (ending) {
        return;
    }
    if (HANDLE thread = CreateThread(nullptr, 0, TurnLeftoverDndOffThreadProc,
                                     nullptr, 0, nullptr)) {
        // (Its code is the mod's, so it ends before the mod unloads.)
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
}

void StopWatchingIslandProcess() {
    AcquireSRWLockExclusive(&g_islandWatchLock);
    HANDLE wait = g_islandProcessWait;
    HANDLE process = g_islandProcess;
    g_islandProcessWait = nullptr;
    g_islandProcess = nullptr;
    g_islandProcessId = 0;
    ReleaseSRWLockExclusive(&g_islandWatchLock);
    if (wait) {
        // Waits for a callback that's running.
        UnregisterWaitEx(wait, INVALID_HANDLE_VALUE);
    }
    if (process) {
        CloseHandle(process);
    }
}

// On the taskbar's thread.
void WatchIslandProcess(HWND island) {
    DWORD processId = 0;
    GetWindowThreadProcessId(island, &processId);
    AcquireSRWLockShared(&g_islandWatchLock);
    const bool watching = processId == g_islandProcessId;
    ReleaseSRWLockShared(&g_islandWatchLock);
    if (!processId || watching) {
        return;
    }
    StopWatchingIslandProcess();
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, processId);
    if (!process) {
        return;
    }
    HANDLE wait = nullptr;
    if (!RegisterWaitForSingleObject(
            &wait, process, IslandProcessEndedCallback, nullptr, INFINITE,
            WT_EXECUTEONLYONCE | WT_EXECUTELONGFUNCTION)) {
        CloseHandle(process);
        return;
    }
    AcquireSRWLockExclusive(&g_islandWatchLock);
    g_islandProcess = process;
    g_islandProcessWait = wait;
    g_islandProcessId = processId;
    ReleaseSRWLockExclusive(&g_islandWatchLock);
}

// A button pressed: it goes down quickly (staying down while held), and once
// all the way down, springs back with a little overshoot. Timed rather than a
// spring, so even a quick click shows the whole press.
constexpr double kPressDown = 0.09;
constexpr double kPressBack = 0.42;

struct PressAnimation {
    // What's pressed (an item or a part, as a number), or 0.
    int target = 0;
    bool held = false;
    double start = 0;
    double releaseAt = 0;

    void Press(int what) {
        target = what;
        held = true;
        start = NowSeconds();
    }
    void Release() {
        if (!held) {
            return;
        }
        held = false;
        releaseAt = std::max(NowSeconds(), start + kPressDown);
    }
    // From 0 (up) to 1 (down); a bit under 0 while springing back.
    double Amount(double now) const {
        const double downSeconds = AnimationSeconds(kPressDown, true);
        const double backSeconds = AnimationSeconds(kPressBack, true);
        if (!target || downSeconds <= 0) {
            return 0;
        }
        const double down = std::min((now - start) / downSeconds, 1.0);
        const double eased = 1 - (1 - down) * (1 - down) * (1 - down);
        if (held || now < releaseAt) {
            return eased;
        }
        const double back = (now - releaseAt) / backSeconds;
        return back >= 1 ? 0 : 1 - SpringOut(back);
    }
    // Returns true while it moves (and forgets it when done).
    bool Animate(double now) {
        if (!target) {
            return false;
        }
        if (!held && now >= releaseAt + AnimationSeconds(kPressBack, true)) {
            target = 0;
            return true;
        }
        return true;
    }
};

// Opening things bounces a little; closing doesn't.
constexpr double kBounceDamping = 0.72;
constexpr double kOpenSeconds = 0.42;
constexpr double kCloseSeconds = 0.26;
// A panel opening out of the pill: sideways quick, then downwards a moment
// later, a bit softer, straight to its size (no overshoot). Closing goes back
// the same way. (The banner and the tray list still bounce a little.)
constexpr double kPanelOpenDamping = 1.0;
constexpr double kPanelWidthSeconds = 0.55;
constexpr double kPanelWidthDamping = 0.62;
constexpr double kPanelHeightSeconds = 0.65;
constexpr double kPanelHeightDelay = 0.05;
constexpr double kPanelCloseSeconds = 0.42;
// How far down it has to be for its contents to show (they fade in after).
constexpr float kPanelContentsFrom = 0.45f;
// The island opening out of its dot, and closing back into it.
constexpr double kAppearSeconds = 1.25;
constexpr double kDisappearSeconds = 0.95;
// The pill's corners, opened into a panel.
constexpr float kIslandPanelRadius = 26;
// How much of their radius the panels' and the banner's corners keep: as
// round as the pill's (islandPillCorners, in %; the bar's and the notches'
// have their own look).
int NotchCountWanted();
float PanelCornerShare() {
    return g_settings.islandBar || NotchCountWanted() > 0
               ? 1.0f
               : g_settings.islandPillCorners / 100.0f;
}
// A new notification in the pill: its size, and how long it stays.
constexpr float kBannerWidth = 380;
constexpr float kBannerHeight = 70;
// A window of its own: its title bar's height (at 96 DPI), room for
// Windows' caption buttons (see Panel::CaptionHeight).
constexpr int kCaptionHeight = 32;

// The island's settings bubble (see Island::ShowGearBubble): its size and
// gap from the pill, and how long it takes to come out, stays, stays after
// the mouse leaves it, and takes to go.
constexpr float kGearBubbleWidth = 46;
constexpr float kGearBubbleHeight = 30;
constexpr float kGearBubbleGap = 6;
constexpr double kGearBubbleIn = 0.4;
constexpr double kGearBubbleStay = 4;
constexpr double kGearBubbleLinger = 1.5;
constexpr double kGearBubbleOut = 0.25;
// The icon's name under the mouse (see Island::ShowTrayTip): how long it
// takes to grow in and to fade.
constexpr double kTipIn = 0.3;
constexpr double kTipOut = 0.15;
// The row with the reply box and "Open", shown under the mouse.
constexpr float kBannerReplyHeight = 46;
constexpr UINT kBannerShownMs = 5000;
constexpr UINT_PTR kBannerTimerId = 3;
constexpr UINT_PTR kTrayCloseTimerId = 4;
constexpr UINT_PTR kTipTimerId = 5;
constexpr UINT_PTR kTrayAppTimerId = 6;
constexpr UINT_PTR kMediaTimerId = 7;
// A tray icon's pin shows after the mouse stays on it this long, so it isn't
// clicked by accident; it then grows in.
constexpr UINT_PTR kPinTimerId = 8;
// A Windows banner hidden right away is put back if the pill hasn't shown the
// notification by then (see OnWindowsBanner).
constexpr UINT_PTR kBannerRestoreTimerId = 9;
constexpr UINT kBannerRestoreMs = 1500;
// After a banner goes, the next waiting one (see m_waitingToasts) comes this
// much later.
constexpr UINT_PTR kBannerNextTimerId = 10;
constexpr UINT kBannerNextMs = 600;
// The notifications kept for the pill while a panel is open, at most.
constexpr size_t kMaxWaitingToasts = 8;
// A call's, an alarm's or a reminder's sound waits this long for Windows'
// own (see Island::QueueUrgentSound), and rings at most this long.
constexpr UINT_PTR kUrgentSoundTimerId = 11;
constexpr UINT kUrgentSoundWaitMs = 1000;
constexpr UINT_PTR kUrgentSoundStopTimerId = 12;
// The weather, measured again.
constexpr UINT_PTR kWeatherTimerId = 14;
// The weather asked again soon, when the service didn't answer (offline at
// startup, for example).
constexpr UINT_PTR kWeatherRetryTimerId = 16;
// The network checked again while it's connecting, and its glyph moving
// meanwhile (a few frames a second, not the whole animation loop).
constexpr UINT_PTR kNetworkTimerId = 17;
constexpr UINT_PTR kNetworkFrameTimerId = 18;
constexpr UINT kNetworkFrameMs = 80;
// Updates, looked for again.
constexpr UINT_PTR kUpdatesTimerId = 15;
// Where the mouse is, checked this often while the island follows it (several
// monitors only).
constexpr UINT_PTR kMonitorTimerId = 20;
// Shift and the wheel on the profiles' button or the volume: the profile or
// the output chosen is applied once Shift is let go, checked this often (see
// Island::StepShiftChoice).
constexpr UINT_PTR kShiftStepTimerId = 21;
constexpr UINT kShiftStepCheckMs = 40;
// The clipboard was busy as it changed: read again this much later (see
// Island::OnClipboardChanged).
constexpr UINT_PTR kClipboardTimerId = 22;
constexpr UINT kClipboardRetryMs = 100;
// Hiding as its line while the mouse is away: checked this often, and it
// goes this long after the mouse left (see Island::CheckAutoHide).
constexpr UINT_PTR kAutoHideTimerId = 23;
constexpr UINT kAutoHideCheckMs = 120;
// How long the minimized line stays before it fades away (islandLineFade,
// 0 never), and how long it takes.
constexpr int kLineFadeSeconds[] = {0, 30, 120, 600};
// The bar's inner corners (islandBarInnerCorners), in DIPs.
constexpr float kBarInnerCornerRadii[] = {0, 8, 14, 22};
// The banner's size and width in % (islandBannerSize, islandBannerWidth;
// 100 by default), and its corners' roundness in % of the panels'
// (islandBannerCorners).
constexpr int kBannerSizes[] = {50, 75, 100, 125, 150, 200};
// How many files the shelf keeps (islandShelfLimit): the oldest goes as a
// new one comes.
constexpr int kShelfLimits[] = {5, 10, 20, 30};
constexpr int kShelfLimitNormal = 1;
constexpr int kBannerSizeNormal = 2;
constexpr PCWSTR kBannerSizeNames[] = {L"50%",  L"75%",  L"100%",
                                       L"125%", L"150%", L"200%"};
constexpr int kBannerWidths[] = {80, 100, 130, 160};
constexpr int kBannerWidthNormal = 1;
constexpr int kBannerCornerShares[] = {100, 50, 0};
// Its places: out of the pill, or one of 9 points of the screen.
constexpr int kBannerPlaces = 10;
constexpr double kLineFadeOutSeconds = 2.0;
// The apps' windows changed, as Windows tells the main island (see
// Island::UpdateWindowEvents): counted, so the windows in an island's way
// (islandHideWhen, islandHideClock) are looked at again only after a change.
// (On the islands' thread only.)
unsigned g_windowEvents = 0;

void CALLBACK WindowEventsProc(HWINEVENTHOOK,
                               DWORD,
                               HWND hWnd,
                               LONG idObject,
                               LONG idChild,
                               DWORD,
                               DWORD) {
    if (IsWindowEventForApp(hWnd, idObject, idChild)) {
        g_windowEvents++;
    }
}

// The last look for an app's window in an island's way (see
// WindowInTheWayProc): whether the windows have to be looked at again for
// this search (Windows told a change, or it's for another place).
struct WayLook {
    bool done = false;
    unsigned events = 0;
    HMONITOR monitor = nullptr;
    RECT place{};
    bool maximizedOnly = false;
};

bool WayLookStale(WayLook& look, const WindowInTheWaySearch& search) {
    const bool stale = !look.done || look.events != g_windowEvents ||
                       look.monitor != search.monitor ||
                       !EqualRect(&look.place, &search.place) ||
                       look.maximizedOnly != search.maximizedOnly;
    look = {true, g_windowEvents, search.monitor, search.place,
            search.maximizedOnly};
    return stale;
}

// The profile in use saved this long after its settings last changed (see
// SaveActiveProfileSoon).
constexpr UINT_PTR kProfileSaveTimerId = 24;
constexpr UINT kProfileSaveMs = 800;
// What's playing's app asked to come back from the tray (its cover
// clicked): looked for this often, for this long, before it's started by
// its ID (see Island::WatchMediaAppOpen).
constexpr UINT_PTR kMediaOpenTimerId = 26;
constexpr UINT kMediaOpenCheckMs = 100;
constexpr double kMediaOpenSeconds = 1;
// Files held on the folders' item while dragged: its panel opens; and a
// moment after they left the item (gone to the panel, or not).
constexpr UINT_PTR kFoldersDropTimerId = 27;
constexpr UINT_PTR kFoldersDragLeftTimerId = 28;
constexpr UINT kFoldersDragLeftMs = 300;
constexpr UINT kMonitorCheckMs = 250;
// How far the island stays open while it glides to another monitor (its
// dot, see Island::AppearFade).
constexpr double kMoveDot = 0.2;
// The island restarted again after a lost drawing device, while the driver
// may still be installing (at most this often).
constexpr UINT_PTR kDeviceRetryTimerId = 19;
constexpr double kDeviceRetrySeconds = 2;
// Every glass made again this long after the island starts, then after each
// next wait (see RenewAllGlass): at sign-in, Windows may not be ready for
// its blur in the first moments.
constexpr UINT_PTR kGlassRenewTimerId = 29;
// The network's speed measured again (see Island::OnNetSpeedTimer), and the
// Recycle Bin counted again a moment after Windows says it changed (many
// changes at once, like a folder deleted, count once).
constexpr UINT_PTR kNetSpeedTimerId = 50;
constexpr UINT kNetSpeedMs = 1000;
constexpr UINT_PTR kRecycleTimerId = 51;
// The AI agents' limits: the islands laid out again each minute (see
// Island::OnAiUsageMinute).
constexpr UINT_PTR kAiUsageTimerId = 59;
constexpr UINT kRecycleSettleMs = 600;
// The folded apps go back into their arrow (see Island::OpenFoldedApps).
constexpr UINT_PTR kAppsFoldTimerId = 52;
constexpr UINT kAppsFoldMs = 1200;
// The visualizer reads the sound this often while something plays (see
// Island::StepVisualizer).
constexpr double kVizStepSeconds = 1.0 / 30;
// The bars drawn at most this often (see Island::StepVisualizer).
constexpr double kVizFrameSeconds = 1.0 / 60;
// A meter's peak at most this low is silence.
constexpr float kVizSilentPeak = 0.001f;
// Its six bars. Smooth, like the iPhone's (islandVisualizerSmooth): the
// middle ones taller, each one's two waves at its own speed (radians a
// second, none a multiple of another, so they never move together).
constexpr size_t kVizBars = 6;
constexpr float kVizShape[kVizBars] = {0.55f, 0.8f, 1.0f, 0.95f, 0.75f, 0.5f};
constexpr float kVizSpeeds[kVizBars] = {4.3f, 5.9f, 3.7f, 6.4f, 4.9f, 5.3f};
