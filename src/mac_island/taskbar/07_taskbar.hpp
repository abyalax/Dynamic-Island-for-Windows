#pragma once
// The taskbar shows what's behind it where the mod took its background away:
// the dock (its room and beside its shelf) and the transparent Windows
// taskbar.
bool TaskbarSeeThrough() {
    return IsDock() || IsTransparentTaskbar();
}

// The taskbar's own window painted black, which its glass shows as nothing,
// under its XAML too. Explorer paints it white below its top 2 rows while
// Windows hides the XAML (as the taskbar hides itself), and that white stays
// under the XAML once it's back: it showed wherever the taskbar is see-through
// (see TaskbarSeeThrough), a band as wide as the screen with the dock's room
// open.
void PaintTaskbarClear(HWND hWnd) {
    RECT client;
    // (Not clipped by its child windows, unlike its own painting: the XAML's
    // window covers it all.)
    const HDC dc =
        GetClientRect(hWnd, &client) ? GetDCEx(hWnd, nullptr, DCX_CACHE)
                                     : nullptr;
    if (!dc) {
        return;
    }
    FillRect(dc, &client, (HBRUSH)GetStockObject(BLACK_BRUSH));
    ReleaseDC(hWnd, dc);
}

// Marks the taskbar whose hide timer is being handled for as long as it lives
// (see g_hideTimerTaskbar). Taskbar thread only.
class HideTimerScope {
   public:
    HideTimerScope(HWND taskbar, bool active) : m_active(active) {
        if (active) {
            m_previous = g_hideTimerTaskbar;
            g_hideTimerTaskbar = taskbar;
        }
    }
    ~HideTimerScope() {
        if (m_active) {
            g_hideTimerTaskbar = m_previous;
        }
    }
    HideTimerScope(const HideTimerScope&) = delete;
    HideTimerScope& operator=(const HideTimerScope&) = delete;

   private:
    bool m_active;
    HWND m_previous = nullptr;
};

// Also swallows the "show" timer while a full-screen app is in front, so the
// mouse at the bottom edge doesn't bring the taskbar up. Other ways of showing
// it, like the Windows key, aren't affected.
LRESULT CALLBACK TaskbarSubclassProc(HWND hWnd,
                                     UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam,
                                     DWORD_PTR) {
    // What the taskbar asks while it handles its hide timer is about it (see
    // CTaskListWnd_PermitAutoHide_Hook).
    HideTimerScope hideTimerScope(hWnd,
                                  uMsg == WM_TIMER && wParam == kHideTimerId);
    const bool displayChanged =
        DisplayChangedMessage() && uMsg == DisplayChangedMessage();
    // (Windows' transparency effects or its light or dark mode switched: the
    // dock's glass and colors follow.)
    // (Its own hiding switched too: the dock's room follows, see
    // GrownTaskbar.)
    const bool colorsChanged =
        uMsg == WM_SETTINGCHANGE && lParam && IsDock() &&
        (wcscmp((PCWSTR)lParam, L"ImmersiveColorSet") == 0 ||
         wcscmp((PCWSTR)lParam, L"TraySettings") == 0);
    if ((uMsg == WM_DISPLAYCHANGE || displayChanged || colorsChanged) &&
        !g_unloading) {
        if (hWnd == FindCurrentProcessTaskbarWnd()) {
            SetTimer(hWnd, kDisplayRestyleTimerId, 200, nullptr);
        }
    }
    // (The mod's own: not for explorer.)
    if (displayChanged) {
        return 0;
    }
    // See-through, its own window stays black (see PaintTaskbarClear): instead
    // of Explorer's paint, and again once it moved or its cut changed (what
    // was there before shows where the cut opens). (Also while unloading:
    // the background comes back after.)
    if (TaskbarSeeThrough()) {
        if (uMsg == WM_ERASEBKGND) {
            return 1;
        }
        if (uMsg == WM_PAINT) {
            PAINTSTRUCT ps;
            if (BeginPaint(hWnd, &ps)) {
                EndPaint(hWnd, &ps);
            }
            PaintTaskbarClear(hWnd);
            return 0;
        }
        // (Not with each frame of the slide hiding it, see SlideTo.)
        if (uMsg == WM_WINDOWPOSCHANGED &&
            !(hWnd == g_slidingTaskbar && g_slidingHides)) {
            PaintTaskbarClear(hWnd);
        }
    }
    if (uMsg == WM_TIMER && wParam == kDisplayRestyleTimerId) {
        KillTimer(hWnd, kDisplayRestyleTimerId);
        if (!g_unloading) {
            Safely(L"Restyling taskbars after a display change", [] {
                ApplySettingsFromTaskbarThread();
                SubclassTaskbars();
            });
        }
        return 0;
    }
    if (uMsg == WM_DPICHANGED) {
        DockScope scope(hWnd);
        if (hWnd == g_dockWnd) {
            g_dockScaleChangedAt = NowSeconds();
            g_dockScaleSettled = false;
            g_dockScaleMoved = false;
            // (Not as it turns to another orientation, see below.)
            const auto* suggested = (const RECT*)lParam;
            RECT now;
            const bool turning =
                suggested && GetWindowRect(hWnd, &now) &&
                (suggested->right - suggested->left <
                 suggested->bottom - suggested->top) !=
                    (now.right - now.left < now.bottom - now.top);
            if (!turning) {
                Safely(L"Pausing the taskbar's animator",
                       [] { PauseRepeaterAnimations(); });
            }
            StartRendering();
            if (!turning &&
                !SetTimer(hWnd, kAnimatorTimerId, kAnimatorPauseMs, nullptr)) {
                Safely(L"Resuming the taskbar's animator",
                       [] { ResumeRepeaterAnimations(); });
            }
        }
    }
    // Moved to another edge (in Settings) while its animator is out: put
    // back first, or explorer lays the taskbar out there without it and
    // crashes.
    if (uMsg == WM_WINDOWPOSCHANGING && lParam && !g_unloading &&
        (((const WINDOWPOS*)lParam)->flags & (SWP_NOMOVE | SWP_NOSIZE)) !=
            (SWP_NOMOVE | SWP_NOSIZE)) {
        const auto* pos = (const WINDOWPOS*)lParam;
        DockScope scope(hWnd);
        RECT now;
        if (hWnd == g_dockWnd && g_animatorPaused &&
            GetWindowRect(hWnd, &now)) {
            RECT next = now;
            if (!(pos->flags & SWP_NOMOVE)) {
                OffsetRect(&next, pos->x - now.left, pos->y - now.top);
            }
            if (!(pos->flags & SWP_NOSIZE)) {
                next.right = next.left + pos->cx;
                next.bottom = next.top + pos->cy;
            }
            // (Hidden too: on its own monitor's halves.)
            MONITORINFO info{sizeof(info)};
            if (GetMonitorInfo(TaskbarMonitor(hWnd), &info) &&
                RectSide(next, info.rcMonitor) !=
                    RectSide(now, info.rcMonitor)) {
                KillTimer(hWnd, kAnimatorTimerId);
                Safely(L"Resuming the taskbar's animator",
                       [] { ResumeRepeaterAnimations(); });
            }
        }
    }
    if (uMsg == WM_TIMER && wParam == kAnimatorTimerId) {
        KillTimer(hWnd, kAnimatorTimerId);
        DockScope scope(hWnd);
        if (hWnd == g_dockWnd) {
            Safely(L"Resuming the taskbar's animator",
                   [] { ResumeRepeaterAnimations(); });
        }
        return 0;
    }
    // Windows sometimes takes a window for a full-screen app when it isn't
    // (one it once saw covering its monitor keeps that mark until it closes)
    // and keeps the taskbar under it, even while it's shown: its own handler
    // turns a request to be on top into one to go under. Kept on top while
    // it's shown (or asked to be) and no app on its monitor is in full
    // screen (see IsFullscreenAppOn). Only in the modes where the
    // mod shows and hides it (the unchanged taskbar is left as it is).
    if (uMsg == WM_WINDOWPOSCHANGING && lParam && !g_unloading &&
        SlidesItself()) {
        auto* pos = (WINDOWPOS*)lParam;
        const bool askedTop = !(pos->flags & SWP_NOZORDER) &&
                              pos->hwndInsertAfter == HWND_TOPMOST;
        const LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        // The dock's room above the taskbar (see GrownTaskbar), once the
        // taskbar placed itself.
        if (IsDock()) {
            GrowTaskbarWindow(hWnd, pos);
        }
        bool goesUnder = askedTop;
        if (!(pos->flags & SWP_NOZORDER)) {
            const HWND after = pos->hwndInsertAfter;
            goesUnder = after == HWND_NOTOPMOST || after == HWND_BOTTOM ||
                        (after != HWND_TOPMOST && after != HWND_TOP &&
                         !(GetWindowLongPtr(after, GWL_EXSTYLE) &
                           WS_EX_TOPMOST));
        }
        MONITORINFO info{sizeof(info)};
        if (goesUnder &&
            (askedTop || (GetMonitorInfo(TaskbarMonitor(hWnd), &info) &&
                          !IsTaskbarHidden(hWnd, info.rcMonitor))) &&
            !IsFullscreenAppOnMonitor(hWnd)) {
            pos->hwndInsertAfter = HWND_TOPMOST;
            pos->flags &= ~SWP_NOZORDER;
        }
        return result;
    }
    // Brought up while under a window (Windows shows it where it was, behind
    // the one in front): on top as it shows, kept there by the above.
    if (uMsg == WM_WINDOWPOSCHANGED && !g_unloading && SlidesItself() &&
        !(GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST)) {
        static bool raising;
        MONITORINFO info{sizeof(info)};
        if (!raising && GetMonitorInfo(TaskbarMonitor(hWnd), &info) &&
            !IsTaskbarHidden(hWnd, info.rcMonitor) &&
            !IsFullscreenAppOnMonitor(hWnd)) {
            raising = true;
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            raising = false;
        }
    }
    // Moved to another edge of the screen (or back to the bottom): before
    // the dock's own handling below, which ends here.
    // (Not during the mod's own slide: it can't change the edge.)
    if (uMsg == WM_WINDOWPOSCHANGED && lParam && !g_unloading &&
        hWnd != g_slidingTaskbar &&
        (((const WINDOWPOS*)lParam)->flags & (SWP_NOMOVE | SWP_NOSIZE)) !=
            (SWP_NOMOVE | SWP_NOSIZE) &&
        IsTaskbarAtBottom() != g_taskbarAtBottom) {
        WatchTaskbarPlace();
    }
    // The dock: never shown past its monitor, nor taking the mouse in its
    // room at rest (see ClipDockWindow), whoever moved it.
    if (uMsg == WM_WINDOWPOSCHANGED && IsDock() && !g_unloading) {
        const LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        // (Not during the mod's own slide, which cuts it before and after
        // each move itself, see SlideTo.)
        if (hWnd != g_slidingTaskbar) {
            ClipDockWindow(hWnd);
        }
        return result;
    }
    // An app closed and the taskbar got the focus back (see HandFocusBack):
    // once the focus moved, it tries to hide again right after.
    if (uMsg == WM_TIMER && wParam == kHideTimerId && HandFocusBack(hWnd)) {
        const LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        ScheduleTaskbarHide(hWnd, 50);
        return result;
    }
    // Up on the desktop (hiding.hideWhen): its hide timer stopped while
    // nothing is in its way (or it fires again and again), and started again
    // once a window is (see CheckTaskbarWay).
    if (uMsg == WM_TIMER && wParam == kHideTimerId && TaskbarWayWatched() &&
        !WindowInTaskbarWay(hWnd)) {
        KillTimer(hWnd, kHideTimerId);
        return 0;
    }
    if (uMsg == WM_TIMER && wParam == kTaskbarWayTimerId) {
        CheckTaskbarWay(hWnd);
        return 0;
    }
    // Shown at once by Windows and still up a moment later: the dock shows
    // now (see kQuietShowDelayMs).
    if (uMsg == WM_TIMER && wParam == kQuietShowTimerId) {
        KillTimer(hWnd, kQuietShowTimerId);
        if (hWnd == g_quietShownWnd) {
            g_quietShownWnd = nullptr;
        }
        // (Also one that another monitor's show took the place of while it
        // waited: it's cut to nothing until here.)
        ClipSlideStep(hWnd, nullptr);
        return 0;
    }
    if (uMsg == WM_TIMER && wParam == kUnhideTimerId && SlidesItself() &&
        g_settings.fullscreenGuard && IsFullscreenAppOnMonitor(hWnd)) {
        static ULONGLONG lastLogged;
        if (GetTickCount64() - lastLogged > 5000) {
            lastLogged = GetTickCount64();
            IslandLog(L"Full-screen app in front, not showing the taskbar");
        }
        // (The taskbar's own handler is what stops it: stopped here, or it
        // fires every few milliseconds for as long as the app stays.)
        KillTimer(hWnd, kUnhideTimerId);
        return 0;
    }
    // The mouse is where it brings the taskbar up, from the edge's thread
    // (hidden still: it may have come up meanwhile).
    if (uMsg == g_edgeUnhideMessage && g_edgeUnhideMessage) {
        MONITORINFO info{sizeof(info)};
        if (!g_unloading && GetMonitorInfo(TaskbarMonitor(hWnd), &info) &&
            IsTaskbarHidden(hWnd, info.rcMonitor)) {
            UnhideTaskbar(hWnd);
        }
        return 0;
    }
    // The other apps' tray icons, for the island (the taskbar still gets
    // them).
    // From the island (in its own process): send everything later (not
    // while it waits for the answer), where the tray icons are, and apps with
    // a new notification.
    if (uMsg == g_sendTrayIconsMessage && g_sendTrayIconsMessage) {
        g_trayIconsSendPending = false;
        SendTrayIcons();
        return 0;
    }
    if (uMsg == g_requestTrayIconsMessage && g_requestTrayIconsMessage) {
        RequestTrayIcons();
        return 0;
    }
    if (uMsg == g_sendToIslandMessage && g_sendToIslandMessage) {
        SendBadgeApps();
        SendTrayIcons();
        Safely(L"Following the progress", [] { PublishProgress(true); });
        return 0;
    }
    // Big icons read (see BigIconJob): the dock drawn again with them.
    if (uMsg == g_bigIconsReadMessage && g_bigIconsReadMessage) {
        if (!g_unloading) {
            StartRendering();
        }
        return 0;
    }
    if (uMsg == WM_COPYDATA && lParam &&
        (IsIpcSender((HWND)wParam, kIslandClassName) ||
         IsIpcSender((HWND)wParam, kNotifierClassName))) {
        const auto* copyData = (const COPYDATASTRUCT*)lParam;
        switch (copyData->dwData) {
            case kIpcHello:
                if (IsIpcSender((HWND)wParam, kIslandClassName)) {
                    WatchIslandProcess((HWND)wParam);
                }
                // The island shows the tray icons now: they're asked for if
                // they haven't been yet.
                g_settings.islandTray = Wh_GetIntValue(L"islandTray", 1) != 0;
                g_settings.showIsland = true;
                PostMessage(hWnd, g_requestTrayIconsMessage, 0, 0);
                PostMessage(hWnd, g_sendToIslandMessage, 0, 0);
                return TRUE;
            case kIpcTrayPlaces:
                ReceiveTrayPlaces(copyData);
                return TRUE;
            case kIpcNotifiedApps: {
                std::vector<std::wstring> apps = SplitLines(copyData);
                Safely(L"Notified apps", [&] { NotifyToastApps(&apps); });
                return TRUE;
            }
            case kIpcAppIcon:
                return (LRESULT)AppIconForIsland(copyData);
            case kIpcProgressWanted: {
                IpcReader reader(copyData);
                int wanted = 0;
                if (reader.Get(&wanted)) {
                    Safely(L"Following the progress",
                           [&] { SetProgressWanted(wanted != 0); });
                }
                return TRUE;
            }
        }
    }
    if (uMsg == WM_COPYDATA) {
        // Where an icon is: on the island.
        if (const LRESULT place =
                OnTrayIconRectRequest((const COPYDATASTRUCT*)lParam)) {
            return place;
        }
        const LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        OnTrayNotify((const COPYDATASTRUCT*)lParam);
        return result;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// The shell hook message, and its code for a flashing window (lParam is the
// window).
UINT g_shellHookMessage;
constexpr WPARAM kShellHookFlash = 0x8006;  // HSHELL_FLASH

void OnAppFlashed(HWND hWnd);

LRESULT CALLBACK TaskbandSubclassProc(HWND hWnd,
                                      UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam,
                                      DWORD_PTR) {
    // Recorded before the taskbar handles it, since that's when it decides to
    // show itself (see TrayUI_SlideWindow_Hook).
    if (g_shellHookMessage && uMsg == g_shellHookMessage &&
        wParam == kShellHookFlash) {
        g_lastFlashTime = NowSeconds();
        OnAppFlashed((HWND)lParam);
    }
    // An app's window closed: if that leaves the focus on the shown taskbar,
    // its hide timer hands it on (see HandFocusBack), after any wait for an
    // app opening.
    if (g_shellHookMessage && uMsg == g_shellHookMessage &&
        wParam == HSHELL_WINDOWDESTROYED && SlidesItself() && g_trayShown) {
        g_appClosedAt = NowSeconds();
        if (HWND taskbar = GetAncestor(hWnd, GA_ROOT)) {
            const double keep =
                taskbar == g_keepShownWnd ? g_keepShownUntil - NowSeconds()
                                          : 0.0;
            ScheduleTaskbarHide(
                taskbar, std::max(300u, (UINT)(std::max(keep, 0.0) * 1000) +
                                            150));
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// Subclassed from several threads (the engine's, the late start, the taskbar's
// for a new secondary taskbar): the lists are only touched under this lock,
// never held while subclassing (that waits for the taskbar's thread).
SRWLOCK g_subclassedLock = SRWLOCK_INIT;
std::vector<HWND> g_subclassedTaskbars;
std::vector<HWND> g_subclassedTaskbands;

bool AnyTaskbarSubclassed() {
    AcquireSRWLockShared(&g_subclassedLock);
    const bool any = !g_subclassedTaskbars.empty();
    ReleaseSRWLockShared(&g_subclassedLock);
    return any;
}

bool IsListed(const std::vector<HWND>& list, HWND hWnd) {
    AcquireSRWLockShared(&g_subclassedLock);
    const bool listed = std::find(list.begin(), list.end(), hWnd) != list.end();
    ReleaseSRWLockShared(&g_subclassedLock);
    return listed;
}

// Every subclassed taskbar told to look at its way (see CheckTaskbarWay).
void PostTaskbarWayChecks() {
    AcquireSRWLockShared(&g_subclassedLock);
    const std::vector<HWND> taskbars = g_subclassedTaskbars;
    ReleaseSRWLockShared(&g_subclassedLock);
    for (HWND taskbar : taskbars) {
        PostMessage(taskbar, WM_TIMER, kTaskbarWayTimerId, 0);
    }
}

// Windows' events about the apps' windows (see kWindowEventRanges), on a
// thread of its own (not the taskbars': moving windows send many), while
// the taskbars' way is watched: the taskbars told once for a burst of them,
// kTaskbarWayBatchMs after the first.
// (Started on the taskbars' thread, also stopped by the unloading one: the
// lock only guards the handles, the thread is waited for outside it.)
HANDLE g_taskbarWayThread;
DWORD g_taskbarWayThreadId;
SRWLOCK g_taskbarWayLock = SRWLOCK_INIT;
// (Its thread's only.)
UINT_PTR g_taskbarWayBatch;

void CALLBACK TaskbarWayEventProc(HWINEVENTHOOK,
                                  DWORD,
                                  HWND hWnd,
                                  LONG idObject,
                                  LONG idChild,
                                  DWORD,
                                  DWORD) {
    if (!g_taskbarWayBatch && IsWindowEventForApp(hWnd, idObject, idChild)) {
        g_taskbarWayBatch = SetTimer(nullptr, 0, kTaskbarWayBatchMs, nullptr);
    }
}

DWORD WINAPI TaskbarWayThreadProc(LPVOID parameter) {
    // Create the message queue before the creator continues.
    MSG msg;
    PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    SetEvent((HANDLE)parameter);

    HWINEVENTHOOK hooks[std::size(kWindowEventRanges)] = {};
    for (size_t i = 0; i < std::size(kWindowEventRanges); i++) {
        hooks[i] = SetWinEventHook(
            kWindowEventRanges[i].first, kWindowEventRanges[i].second, nullptr,
            TaskbarWayEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    }
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_TIMER && !msg.hwnd && g_taskbarWayBatch &&
            msg.wParam == g_taskbarWayBatch) {
            KillTimer(nullptr, g_taskbarWayBatch);
            g_taskbarWayBatch = 0;
            PostTaskbarWayChecks();
            continue;
        }
        DispatchMessage(&msg);
    }
    if (g_taskbarWayBatch) {
        KillTimer(nullptr, g_taskbarWayBatch);
        g_taskbarWayBatch = 0;
    }
    for (HWINEVENTHOOK hook : hooks) {
        if (hook) {
            UnhookWinEvent(hook);
        }
    }
    return 0;
}

void StartTaskbarWayThread() {
    AcquireSRWLockExclusive(&g_taskbarWayLock);
    HANDLE started = g_taskbarWayThread || g_unloading
                         ? nullptr
                         : CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (started) {
        g_taskbarWayThread = CreateThread(nullptr, 0, TaskbarWayThreadProc,
                                          started, 0, &g_taskbarWayThreadId);
        if (g_taskbarWayThread) {
            // (It takes no lock.)
            WaitForSingleObject(started, INFINITE);
        } else {
            g_taskbarWayThreadId = 0;
        }
        CloseHandle(started);
    }
    ReleaseSRWLockExclusive(&g_taskbarWayLock);
}

void StopTaskbarWayThread() {
    AcquireSRWLockExclusive(&g_taskbarWayLock);
    const HANDLE thread = g_taskbarWayThread;
    const DWORD threadId = g_taskbarWayThreadId;
    g_taskbarWayThread = nullptr;
    g_taskbarWayThreadId = 0;
    ReleaseSRWLockExclusive(&g_taskbarWayLock);
    if (thread) {
        PostThreadMessage(threadId, WM_QUIT, 0, 0);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
}

// The taskbars' way watched or not with the settings (on the taskbars'
// thread): each one looks once now; no longer watched, their own hide
// timers start again (stopped while nothing was in the way), so they hide
// with the mouse away as before.
void WatchTaskbarWay() {
    if (!TaskbarWayWatched()) {
        AcquireSRWLockShared(&g_taskbarWayLock);
        const bool watched = g_taskbarWayThread != nullptr;
        ReleaseSRWLockShared(&g_taskbarWayLock);
        if (!watched) {
            return;
        }
        StopTaskbarWayThread();
        AcquireSRWLockShared(&g_subclassedLock);
        const std::vector<HWND> taskbars = g_subclassedTaskbars;
        ReleaseSRWLockShared(&g_subclassedLock);
        for (HWND taskbar : taskbars) {
            ScheduleTaskbarHide(taskbar, kTaskbarWayHideMs);
        }
        return;
    }
    StartTaskbarWayThread();
    PostTaskbarWayChecks();
}

void AddToList(std::vector<HWND>& list, HWND hWnd) {
    AcquireSRWLockExclusive(&g_subclassedLock);
    if (std::find(list.begin(), list.end(), hWnd) == list.end()) {
        list.push_back(hWnd);
    }
    ReleaseSRWLockExclusive(&g_subclassedLock);
}

BOOL CALLBACK SubclassTaskbarProc(HWND hWnd, LPARAM) {
    DWORD processId = 0;
    WCHAR className[32] = L"";
    if (GetWindowThreadProcessId(hWnd, &processId) &&
        processId == GetCurrentProcessId() &&
        GetClassName(hWnd, className, ARRAYSIZE(className)) &&
        (_wcsicmp(className, L"Shell_TrayWnd") == 0 ||
         _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0) &&
        !IsListed(g_subclassedTaskbars, hWnd)) {
        if (WindhawkUtils::SetWindowSubclassFromAnyThread(
                hWnd, TaskbarSubclassProc, 0)) {
            AddToList(g_subclassedTaskbars, hWnd);
        }
    }
    // (What Explorer painted on it before goes, see PaintTaskbarClear: also
    // once the settings make it see-through.)
    if (TaskbarSeeThrough() && IsListed(g_subclassedTaskbars, hWnd)) {
        InvalidateRect(hWnd, nullptr, FALSE);
    }
    // The main taskbar's buttons live in its taskband window (made a
    // moment after the taskbar, when Explorer starts).
    if (processId == GetCurrentProcessId() &&
        _wcsicmp(className, L"Shell_TrayWnd") == 0) {
        HWND taskband = (HWND)GetProp(hWnd, L"TaskbandHWND");
        if (taskband && !IsListed(g_subclassedTaskbands, taskband) &&
            WindhawkUtils::SetWindowSubclassFromAnyThread(
                taskband, TaskbandSubclassProc, 0)) {
            AddToList(g_subclassedTaskbands, taskband);
        }
    }
    return TRUE;
}

void SubclassTaskbars() {
    EnumWindows(
        SubclassTaskbarProc,
        0);
}

void UnsubclassTaskbars() {
    AcquireSRWLockExclusive(&g_subclassedLock);
    std::vector<HWND> taskbars = std::move(g_subclassedTaskbars);
    std::vector<HWND> taskbands = std::move(g_subclassedTaskbands);
    g_subclassedTaskbars.clear();
    g_subclassedTaskbands.clear();
    ReleaseSRWLockExclusive(&g_subclassedLock);
    for (HWND hWnd : taskbars) {
        if (IsWindow(hWnd)) {
            WindhawkUtils::RemoveWindowSubclassFromAnyThread(
                hWnd, TaskbarSubclassProc);
            // (Painted by Windows again: in the see-through modes the
            // subclass painted it clear, see PaintTaskbarClear.)
            RedrawWindow(hWnd, nullptr, nullptr,
                         RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        }
    }

    for (HWND hWnd : taskbands) {
        if (IsWindow(hWnd)) {
            WindhawkUtils::RemoveWindowSubclassFromAnyThread(
                hWnd, TaskbandSubclassProc);
        }
    }
}

// Loaded with Explorer (or while it restarts), the taskbar isn't ready yet:
// tries again every second, for up to two minutes, until it's styled, then
// asks the apps for their tray icons again (they sent them before the mod
// could see them).
HANDLE g_lateStartThread;
HANDLE g_lateStartStop;

// The parts that need the taskbar's process (see OwnsTaskbar): started with
// the mod, or once the taskbar is ready.
void StartAttentionThread();
void StartEdgeWatch();
void StartTaskbarParts() {
    StartAttentionThread();
    StartEdgeWatch();
}

void UpdateAutoHide();

DWORD WINAPI LateStartThreadProc(LPVOID) {
    for (int tries = 0; tries < 120; tries++) {
        if (WaitForSingleObject(g_lateStartStop, 1000) != WAIT_TIMEOUT) {
            return 0;
        }
        if (!FindCurrentProcessTaskbarWnd()) {
            continue;
        }
        SubclassTaskbars();
        if (!g_mainTaskbarStyled) {
            ApplySettings();
        }
        if (g_mainTaskbarStyled) {
            IslandLog(L"Taskbar ready after %d s", tries + 1);
            UpdateAutoHide();
            RefreshDockRoom();
            StartTaskbarParts();
            RequestTrayIcons();
            return 0;
        }
    }
    IslandLog(L"Taskbar still not ready, giving up");
    return 0;
}

// Started when the taskbar wasn't ready.
void StartLateStart() {
    if (g_lateStartThread || g_mainTaskbarStyled) {
        return;
    }
    g_lateStartStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!g_lateStartStop) {
        return;
    }
    g_lateStartThread =
        CreateThread(nullptr, 0, LateStartThreadProc, nullptr, 0, nullptr);
    if (!g_lateStartThread) {
        CloseHandle(g_lateStartStop);
        g_lateStartStop = nullptr;
    }
}

void StopLateStart() {
    if (g_lateStartThread) {
        SetEvent(g_lateStartStop);
        WaitForSingleObject(g_lateStartThread, INFINITE);
        CloseHandle(g_lateStartThread);
        g_lateStartThread = nullptr;
    }
    if (g_lateStartStop) {
        CloseHandle(g_lateStartStop);
        g_lateStartStop = nullptr;
    }
}

// Whether an app's jump list is in front, opened from the dock: Windows
// shows them from its Start menu's process (or the shell experience host on
// older builds), after a right-click on the icon.
constexpr UINT kJumpListRetryMs = 300;

bool JumpListOpen(HWND dock) {
    // (Only the dock whose icon was right-clicked: the other monitors' hide.)
    if (dock != g_dockRightPressedWnd ||
        NowSeconds() - g_dockRightPressedAt > 60) {
        return false;
    }
    DWORD processId = 0;
    HWND foreground = GetForegroundWindow();
    if (!foreground || !GetWindowThreadProcessId(foreground, &processId)) {
        return false;
    }
    HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process) {
        return false;
    }
    WCHAR path[MAX_PATH];
    DWORD length = ARRAYSIZE(path);
    const bool named = QueryFullProcessImageName(process, 0, path, &length);
    CloseHandle(process);
    const PCWSTR name = named ? PathFindFileName(path) : L"";
    return _wcsicmp(name, L"StartMenuExperienceHost.exe") == 0 ||
           _wcsicmp(name, L"ShellExperienceHost.exe") == 0;
}

// Whether the taskbar may hide now. A button asking for attention keeps it
// up; unless the whole dock is wanted for that, it hides as usual. An app
// clicked in the dock that's still opening keeps it up too.
using CTaskListWnd_PermitAutoHide_t = BOOL(WINAPI*)(void* pThis);
CTaskListWnd_PermitAutoHide_t CTaskListWnd_PermitAutoHide_Original;
BOOL WINAPI CTaskListWnd_PermitAutoHide_Hook(void* pThis) {
    if (!SlidesItself()) {
        return CTaskListWnd_PermitAutoHide_Original(pThis);
    }
    // Each taskbar answers for itself: the one whose hide timer asked (the
    // main one when something else did), with its own dock's state, so the
    // mouse on one monitor's dock doesn't keep the other monitor's up.
    const HWND asking = g_hideTimerTaskbar ? g_hideTimerTaskbar : g_dockWnd;
    // (A taskbar the mod hasn't styled yet has no state of its own: the main
    // one's would answer for it, so Windows' own answer stands.)
    if (asking != g_dockWnd && !FindOtherDock(asking)) {
        return CTaskListWnd_PermitAutoHide_Original(pThis);
    }
    DockScope scope(asking);
    const HWND dock = g_dockWnd;
    // An app clicked in the dock is opening (see KeepDockShown), the mouse
    // is on the dock (its magnified icons may be above the taskbar's own
    // height), or an app's jump list is open (its icon right-clicked): tried
    // again a moment later, as Windows' own taskbar stays up for it.
    if (!g_unloading && IsDock() && JumpListOpen(dock)) {
        ScheduleTaskbarHide(dock, kJumpListRetryMs);
        return FALSE;
    }
    // (The mouse is off its window already: under the thumbnails' window, as
    // big as the monitor, the taskbar stops drawing the frames that notice
    // it, see OnRenderingUnsafe.)
    if (g_dockHovered) {
        RECT window{};
        POINT pt{};
        if (GetWindowRect(g_dockHovered, &window) && GetCursorPos(&pt) &&
            !PtInRect(&window, pt)) {
            g_dockHovered = nullptr;
        }
    }
    if (!g_unloading && IsKeptShown(dock)) {
        // Windows doesn't ask again: tried right after the wait.
        if (!UserIsReachingForTaskbar(dock)) {
            ScheduleTaskbarHide(
                dock,
                (UINT)std::max(0.0, (g_keepShownUntil - NowSeconds()) * 1000) +
                    100);
        }
        return FALSE;
    }
    if (!g_unloading && IsDock() && g_dockHovered == dock) {
        return FALSE;
    }
    if (!g_unloading && IsDock() &&
        g_settings.attentionMode != AttentionMode::Dock &&
        std::any_of(g_dockIcons.begin(), g_dockIcons.end(),
                    [](const DockIcon& icon) { return icon.attention; })) {
        return TRUE;
    }
    // On the transparent taskbar, Windows would keep it up as long as a
    // button asks for attention (until that app is looked at): it hides as
    // usual, unless the user is on it or in something of the taskbar's (a
    // jump list, a thumbnail).
    if (!g_unloading && !IsDock() && g_dockWnd &&
        std::any_of(g_dockIcons.begin(), g_dockIcons.end(),
                    [](const DockIcon& icon) { return icon.attention; }) &&
        !UserIsReachingForTaskbar(g_dockWnd)) {
        POINT pt;
        HWND under = GetCursorPos(&pt) ? WindowFromPoint(pt) : nullptr;
        DWORD underProcess = 0;
        DWORD foregroundProcess = 0;
        if (under) {
            GetWindowThreadProcessId(under, &underProcess);
        }
        if (HWND foreground = GetForegroundWindow()) {
            GetWindowThreadProcessId(foreground, &foregroundProcess);
        }
        if (underProcess != GetCurrentProcessId() &&
            foregroundProcess != GetCurrentProcessId()) {
            return TRUE;
        }
    }
    return CTaskListWnd_PermitAutoHide_Original(pThis);
}

////////////////////////////////////////////////////////////////////////////////
// Bottom edge.
//
// While hidden, the taskbar keeps a thin strip on screen to notice the mouse.
// With the dock style that strip is transparent, and transparent parts don't
// always receive the mouse, so the bottom edge is watched here as well.
// Only while the mod slides the taskbar itself, and only often when the mouse
// is near the bottom (10 times a second otherwise).

HANDLE g_edgeThread;
HANDLE g_edgeStopEvent;

constexpr DWORD kEdgePollMs = 15;
// Far from the bottom edge, the mouse is looked at less often: it takes more
// than this long to get there.
constexpr DWORD kEdgeIdlePollMs = 100;
constexpr int kEdgeNearPixels = 300;
// While the mouse stays at the edge, the request is repeated in case it got
// lost.
constexpr DWORD kEdgeRetryMs = 150;

// Looking for the taskbar of a monitor.
struct TaskbarSearch {
    HMONITOR monitor;
    HWND result;
};

BOOL CALLBACK FindTaskbarForMonitorProc(HWND hWnd, LPARAM lParam) {
    auto* context = reinterpret_cast<TaskbarSearch*>(lParam);
    DWORD processId = 0;
    WCHAR className[32];
    if (GetWindowThreadProcessId(hWnd, &processId) &&
        processId == GetCurrentProcessId() &&
        GetClassName(hWnd, className, ARRAYSIZE(className)) &&
        (_wcsicmp(className, L"Shell_TrayWnd") == 0 ||
         _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0) &&
        TaskbarMonitor(hWnd) == context->monitor) {
        context->result = hWnd;
        return FALSE;
    }
    return TRUE;
}

HWND FindTaskbarForMonitor(HMONITOR monitor) {
    TaskbarSearch context{monitor, nullptr};
    EnumWindows(
        FindTaskbarForMonitorProc,
        reinterpret_cast<LPARAM>(&context));
    return context.result;
}

// A taskbar showing itself for an app asking for attention: it slides up on
// its own (before TrayUI::SlideWindow), so it's stopped here, or it would
// come up and go back down with every flash (see KeepsDownForAttention).
using CTaskBand_UnhideTray_t = HRESULT(WINAPI*)(void* pThis);
CTaskBand_UnhideTray_t CTaskBand_UnhideTray_Original;
HRESULT WINAPI CTaskBand_UnhideTray_Hook(void* pThis) {
    if (HWND taskbar = FindCurrentProcessTaskbarWnd();
        taskbar && KeepsDownForAttention(taskbar)) {
        LogKeptDown();
        return S_OK;
    }
    return CTaskBand_UnhideTray_Original(pThis);
}

// The same for the other monitors' taskbars (the one where the mouse is,
// for whether the user is reaching for it).
CTaskBand_UnhideTray_t CSecondaryTaskBand_UnhideTray_Original;
HRESULT WINAPI CSecondaryTaskBand_UnhideTray_Hook(void* pThis) {
    POINT pt{};
    GetCursorPos(&pt);
    HWND taskbar =
        FindTaskbarForMonitor(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST));
    if (!taskbar) {
        taskbar = FindCurrentProcessTaskbarWnd();
    }
    if (taskbar && KeepsDownForAttention(taskbar)) {
        LogKeptDown();
        return S_OK;
    }
    return CSecondaryTaskBand_UnhideTray_Original(pThis);
}

// Hidden: less than half of the taskbar is on its monitor, or the hidden
// dock's window, higher (see HiddenLift), cut down to its last pixels.
bool IsTaskbarHidden(HWND hTaskbarWnd, const RECT& monitorRect) {
    RECT rect, visible;
    if (!GetWindowRect(hTaskbarWnd, &rect)) {
        return false;
    }
    if (!IntersectRect(&visible, &rect, &monitorRect)) {
        return true;
    }
    if (!EqualRect(&visible, &rect)) {
        RECT cut{};
        if (GetWindowRgnBox(hTaskbarWnd, &cut) == SIMPLEREGION &&
            cut.bottom - cut.top <=
                MulDiv(6, GetDpiForWindow(hTaskbarWnd), 96)) {
            return true;
        }
    }
    return (visible.bottom - visible.top) < (rect.bottom - rect.top) / 2;
}

DWORD WINAPI EdgeThreadProc(LPVOID) {
    ULONGLONG atEdgeSince = 0;
    ULONGLONG lastRequest = 0;
    // Where the mouse was last time (see g_edgeCapture).
    HMONITOR lastMonitor = nullptr;

    DWORD wait = kEdgeIdlePollMs;
    while (WaitForSingleObject(g_edgeStopEvent, wait) == WAIT_TIMEOUT) {
        wait = kEdgeIdlePollMs;
        POINT pt;
        if (!GetCursorPos(&pt)) {
            continue;
        }
        HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        MONITORINFO info{sizeof(info)};
        if (!GetMonitorInfo(monitor, &info)) {
            atEdgeSince = 0;
            g_edgeCapture = nullptr;
            continue;
        }
        // Just past the bottom edge of the monitor above, come from it: that
        // edge (see g_edgeCapture).
        HMONITOR above = EdgeCaptureAbove(pt, info.rcMonitor);
        if (above && above != g_edgeCapture && above != lastMonitor) {
            above = nullptr;
        }
        g_edgeCapture = above;
        lastMonitor = monitor;
        if (above) {
            wait = kEdgePollMs;
            monitor = above;
            if (!GetMonitorInfo(monitor, &info)) {
                atEdgeSince = 0;
                continue;
            }
        } else {
            if (pt.y >= info.rcMonitor.bottom - kEdgeNearPixels) {
                wait = kEdgePollMs;
            }
            UINT dpiX = 96;
            UINT dpiY = 96;
            if (FAILED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX,
                                        &dpiY))) {
                dpiX = 96;
            }
            if (pt.y < info.rcMonitor.bottom -
                           EdgeStripPixels(pt.x, info.rcMonitor.bottom,
                                           dpiX)) {
                atEdgeSince = 0;
                continue;
            }
            // The transparent taskbar: only under its apps (that monitor's);
            // the dock, under its icons (see ClipDockWindow).
            int appsLeft = 0;
            int appsRight = 0;
            if ((IsTransparentTaskbar() || IsDock()) &&
                GetAppsArea(monitor, &appsLeft, &appsRight) &&
                (pt.x < appsLeft || pt.x >= appsRight)) {
                atEdgeSince = 0;
                continue;
            }
        }

        const ULONGLONG now = GetTickCount64();
        if (!atEdgeSince) {
            atEdgeSince = now;
        }
        if (now - atEdgeSince < (ULONGLONG)g_settings.showDelay ||
            now - lastRequest < kEdgeRetryMs) {
            continue;
        }

        HWND hTaskbarWnd = FindTaskbarForMonitor(monitor);
        if (!hTaskbarWnd || !IsTaskbarHidden(hTaskbarWnd, info.rcMonitor) ||
            (g_settings.fullscreenGuard &&
             IsFullscreenAppOnMonitor(hTaskbarWnd))) {
            continue;
        }

        // The same message the taskbar's own "show" timer sends; asked again
        // (still hidden), as the mod shows it (see g_edgeUnhideMessage).
        if (lastRequest >= atEdgeSince) {
            PostMessage(hTaskbarWnd, g_edgeUnhideMessage, 0, 0);
        } else {
            PostMessage(hTaskbarWnd, WM_TIMER, kUnhideTimerId, 0);
        }
        lastRequest = now;
    }
    return 0;
}

void StartEdgeWatch() {
    // A transparent taskbar, hidden, doesn't get the mouse either.
    if (g_edgeThread || !SlidesItself() || !OwnsTaskbar()) {
        return;
    }
    g_edgeStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!g_edgeStopEvent) {
        return;
    }
    g_edgeThread = CreateThread(nullptr, 0, EdgeThreadProc, nullptr, 0, nullptr);
    if (!g_edgeThread) {
        CloseHandle(g_edgeStopEvent);
        g_edgeStopEvent = nullptr;
    }
}

void StopEdgeWatch() {
    if (g_edgeThread) {
        SetEvent(g_edgeStopEvent);
        WaitForSingleObject(g_edgeThread, INFINITE);
        CloseHandle(g_edgeThread);
        g_edgeThread = nullptr;
    }
    g_edgeCapture = nullptr;
    if (g_edgeStopEvent) {
        CloseHandle(g_edgeStopEvent);
        g_edgeStopEvent = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////
// New notifications (their own thread).
//
// A new message in a chat that's already unread doesn't change WhatsApp's
// badge, so the badge alone misses it. Windows keeps the notifications it shows
// in its notification database, and updates a shell state (WNF) with each one.
// When that state changes, the mod reads the new notifications from the
// database (read-only, with the system's SQLite, winsqlite3.dll), and each new
// one of an app on the dock counts as news (see OnAppNotified).

// Changes with each notification shown (and dismissed). Undocumented: if it
// doesn't exist on a Windows build, subscribing fails and only the badges are
// watched.
constexpr ULONGLONG kWnfNotificationsChanged = 0x0D83063EA3BC1035;

void StopToastWatch();

HANDLE g_toastThread;
HANDLE g_toastStopEvent;
HANDLE g_toastChangedEvent;
void* g_toastSubscription;

LONG NTAPI ToastStateCallback(ULONGLONG, ULONG, void*, void*, const void*,
                              ULONG) {
    if (HANDLE event = g_toastChangedEvent) {
        SetEvent(event);
    }
    return 0;
}

// The few functions of winsqlite3.dll that are used.
struct Sqlite {
    using open_v2_t = int(WINAPI*)(const char*, void**, int, const char*);
    using prepare_v2_t =
        int(WINAPI*)(void*, const char*, int, void**, const char**);
    using bind_int64_t = int(WINAPI*)(void*, int, long long);
    using step_t = int(WINAPI*)(void*);
    using column_int64_t = long long(WINAPI*)(void*, int);
    using column_text16_t = const void*(WINAPI*)(void*, int);
    using column_blob_t = const void*(WINAPI*)(void*, int);
    using column_bytes_t = int(WINAPI*)(void*, int);
    using finalize_t = int(WINAPI*)(void*);
    using close_t = int(WINAPI*)(void*);
    using busy_timeout_t = int(WINAPI*)(void*, int);

    static constexpr int kOpenReadOnly = 0x00000001;
    static constexpr int kRow = 100;

    HMODULE module = nullptr;
    open_v2_t open_v2 = nullptr;
    prepare_v2_t prepare_v2 = nullptr;
    bind_int64_t bind_int64 = nullptr;
    step_t step = nullptr;
    column_int64_t column_int64 = nullptr;
    column_text16_t column_text16 = nullptr;
    column_blob_t column_blob = nullptr;
    column_bytes_t column_bytes = nullptr;
    finalize_t finalize = nullptr;
    close_t close = nullptr;
    busy_timeout_t busy_timeout = nullptr;

    bool Load() {
        module = LoadLibraryEx(L"winsqlite3.dll", nullptr,
                               LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module) {
            return false;
        }
        open_v2 = (open_v2_t)GetProcAddress(module, "sqlite3_open_v2");
        prepare_v2 = (prepare_v2_t)GetProcAddress(module, "sqlite3_prepare_v2");
        bind_int64 = (bind_int64_t)GetProcAddress(module, "sqlite3_bind_int64");
        step = (step_t)GetProcAddress(module, "sqlite3_step");
        column_int64 =
            (column_int64_t)GetProcAddress(module, "sqlite3_column_int64");
        column_text16 =
            (column_text16_t)GetProcAddress(module, "sqlite3_column_text16");
        column_blob = (column_blob_t)GetProcAddress(module, "sqlite3_column_blob");
        column_bytes =
            (column_bytes_t)GetProcAddress(module, "sqlite3_column_bytes");
        finalize = (finalize_t)GetProcAddress(module, "sqlite3_finalize");
        close = (close_t)GetProcAddress(module, "sqlite3_close");
        busy_timeout =
            (busy_timeout_t)GetProcAddress(module, "sqlite3_busy_timeout");
        return open_v2 && prepare_v2 && bind_int64 && step && column_int64 &&
               column_text16 && column_blob && column_bytes && finalize &&
               close && busy_timeout;
    }

    void Unload() {
        if (module) {
            FreeLibrary(module);
            module = nullptr;
        }
    }
};

// Whether notifications show their title and text (on the island), or only
// their app. Switched in the island's notifications, and remembered.
std::atomic<bool> g_showNotificationContent{true};
// New notifications in the pill (instead of the Windows banner), and also
// with the pill minimized. Their settings, switched in the island too.
std::atomic<bool> g_bannerEnabled{true};
std::atomic<bool> g_bannerWhenMinimized{false};
// The islands that show new notifications (chosen in the Monitors section,
// with islands on all monitors): the main one unless turned off
// ("notifyMainIsland"), and other monitors' ones by their device
// ("notifyScreens", one a line). Calls, alarms and reminders stay on the
// main island (it rings them and takes Windows' banner over for their
// buttons).
bool g_notifyMain = true;
std::vector<std::wstring> g_notifyScreens;
void SetNotificationScreen(const std::wstring& device, bool on);

// What a notification offers besides its text: opening it (its own
// arguments, for the app to open the right place, like a chat), and a reply
// box with the button that sends it (like messaging apps' notifications).
struct ToastReply {
    std::wstring launch;
    // How it opens ("protocol": `launch` is a link to open).
    std::wstring launchType;
    std::wstring inputId;
    std::wstring placeholder;
    // The button that sends the reply.
    std::wstring arguments;
    std::wstring activationType;
    std::wstring label;
    // Its other buttons that the island can press too (shown on a call, an
    // alarm or a reminder, see IslandToast::urgent, and in the list): the
    // app's own, a link, or Windows' "Snooze" and "Dismiss".
    struct Action {
        std::wstring label;
        std::wstring arguments;
        std::wstring activationType;
    };
    std::vector<Action> actions;
    // Windows' own notification (like Nearby sharing's) with buttons the
    // island can't press (Windows runs them): its banner stays, and a
    // click opens Windows' notification center, where they work.
    bool windowsButtons = false;
    bool CanReply() const { return !inputId.empty() && !arguments.empty(); }
};

// Defined further down.
ToastReply ParseToastReply(const std::wstring& appId,
                           const char* xml,
                           int size);

// A new notification, for the island to show (see Island::ShowBanner).
struct IslandToast {
    std::wstring appId;
    // Its ID in the notification database (0 for the island's own, like a
    // calendar reminder), and when it came there (its ArrivalTime).
    long long id = 0;
    long long arrival = 0;
    std::wstring title;
    std::wstring text;
    // With its own lines (see SplitToastTexts).
    std::wstring fullText;
    ToastReply reply;
    // Stays until closed or clicked (a reminder).
    bool sticky = false;
    // The picture replacing the app's icon (like a contact's photo), as a
    // file, and whether it's cut round.
    std::wstring image;
    bool imageCircle = false;
    // A picture on the web, downloaded (see islandWebPictures).
    std::vector<BYTE> imageData;
    // A call, an alarm or a reminder (its scenario). Usually Windows' banner
    // shows it, with its buttons; with the island's own notifications, the
    // pill shows it first, ahead of the others (see Island::ShowBanner).
    bool urgent = false;
    // Its sound (its <audio> element): a Windows sound
    // ("ms-winsoundevent:..."), a file in its app's package ("ms-appx:///"),
    // "silent", or empty for the usual one; and whether it repeats (a call
    // ringing, an alarm).
    std::wstring sound;
    bool loop = false;
    // A reply being typed when a call, an alarm or a reminder took the pill:
    // kept for when it shows again.
    std::wstring draft;
    // The island's own about a drive plugged in: the drive, opened when it's
    // clicked (see Island::OnDeviceChange).
    std::wstring openPath;
    // A status notice (see Island::ShowStatusNotice): its glyph in a circle
    // of its color (0xRRGGBB).
    WCHAR glyph = 0;
    int glyphColor = 0;
};
SRWLOCK g_islandToastsLock = SRWLOCK_INIT;
std::vector<IslandToast> g_islandToasts;
// The notifications as they came, kept for when their app takes them out of
// Windows' list itself (like Discord) while its taskbar button still counts
// them: they stay in the island's list as long as that count does (see
// NotificationPanel::MergeKeptToasts). With when they were kept.
struct KeptToast {
    IslandToast toast;
    double since;
};
SRWLOCK g_keptToastsLock = SRWLOCK_INIT;
std::vector<KeptToast> g_keptToasts;
// Tells the island there are new ones.
constexpr UINT WM_APP_TOAST = WM_APP + 52;
// Tells the island the notification center changed (some may be gone).
constexpr UINT WM_APP_TOASTS_CHANGED = WM_APP + 56;

// Defined further down.
std::vector<std::wstring> ToastTexts(const char* xml, int size);

// A toast's texts as shown: the title and the text on one line each, and the
// text with its own lines (a line per <text>, and the message's), for when
// it opens on more lines.
void SplitToastTexts(const std::vector<std::wstring>& texts,
                     std::wstring* title,
                     std::wstring* text,
                     std::wstring* fullText) {
    auto oneLine = [](std::wstring line) {
        std::replace(line.begin(), line.end(), L'\n', L' ');
        return line;
    };
    if (!texts.empty()) {
        *title = oneLine(texts[0]);
    }
    for (size_t i = 1; i < texts.size(); i++) {
        *fullText += (i > 1 ? L"\n" : L"") + texts[i];
    }
    *text = oneLine(*fullText);
}
bool ToastImage(const std::wstring& appId,
                const char* xml,
                int size,
                std::wstring* path,
                bool* circle);
bool DownloadPicture(const std::wstring& url, std::vector<BYTE>* data);
bool PackageGoesOnline(const std::wstring& appId);
std::wstring ToastSound(const char* xml, int size);
bool ToastSoundLoops(const char* xml, int size);
bool IsUrgentToast(const char* xml, int size);
std::wstring PackageFolder(const std::wstring& appId);
void StartToastShortcutsRead();
void WaitForToastShortcutsRead();

