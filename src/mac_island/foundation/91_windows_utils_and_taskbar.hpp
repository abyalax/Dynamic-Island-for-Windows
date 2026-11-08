// Waits `ms`, handling the thread's messages, unless it's told to quit
// meanwhile (false then).
bool WaitUnlessQuit(DWORD ms) {
    const ULONGLONG end = GetTickCount64() + ms;
    for (;;) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        const ULONGLONG now = GetTickCount64();
        if (now >= end) {
            return true;
        }
        MsgWaitForMultipleObjects(0, nullptr, FALSE, (DWORD)(end - now),
                                  QS_ALLINPUT);
    }
}

DWORD WINAPI IslandThreadProc(LPVOID parameter) {
    MSG msg;
    PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    SetEvent((HANDLE)parameter);

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const HRESULT comResult =
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // (Files dragged out of the folders' panel and dropped on the islands:
    // Windows' drag and drop needs OLE, not only COM.)
    const HRESULT oleResult = OleInitialize(nullptr);
    g_windowsAccentColor = ReadWindowsAccentColor();
    if (GetTickCount64() < kBootUptimeMs) {
        g_progressFrom = NowSeconds() + kBootProgressSeconds;
    }

    bool made = false;
    const bool classes = RegisterIslandClasses();
    if (classes) {
        FrameTimer frames;
        // The main island, then (if chosen) one on each other monitor.
        std::vector<std::unique_ptr<Island>> islands;
        islands.push_back(std::make_unique<Island>());
        Island& main = *islands.front();
        g_island = &main;
        // The Fixed style as notches (see islandNotches): an island for each
        // on the same monitor (its look, its profile), made after the one
        // with the bell (the island made first: it shows the notifications
        // and keeps the strip).
        auto chooseNotch = [](Island& island) {
            if (const int count = NotchCountWanted(); count > 0) {
                island.SetNotch(NotchOfGroup(PillGroup::Bell, count), nullptr,
                                count);
            }
        };
        auto addNotches = [&](Island& base, const std::wstring& device) {
            for (int notch : NotchPlaces(base.NotchCount())) {
                if (notch == base.Notch()) {
                    continue;
                }
                auto sibling = std::make_unique<Island>();
                sibling->SetOtherMonitor(base.CurrentMonitor(), device,
                                         base.MonitorProfile());
                sibling->SetNotch(notch, &base, base.NotchCount());
                IslandScope scope(sibling.get());
                if (!sibling->Create()) {
                    sibling->Destroy();
                    continue;
                }
                base.AddNotchSibling(sibling.get());
                g_islands.push_back(sibling.get());
                islands.push_back(std::move(sibling));
            }
            base.OnNotchesMade();
        };
        chooseNotch(main);
        const bool created = main.Create();
        made = created;
        if (created) {
            g_islands.push_back(&main);
            if (main.Notch() >= 0) {
                MONITORINFOEX info{};
                info.cbSize = sizeof(info);
                GetMonitorInfo(main.CurrentMonitor(), &info);
                addNotches(main, info.szDevice);
            }
            if (g_settings.islandAllMonitors) {
                const HMONITOR mainMonitor = main.CurrentMonitor();
                for (const auto& monitor : ListMonitors()) {
                    if (monitor.handle == mainMonitor) {
                        continue;
                    }
                    auto other = std::make_unique<Island>();
                    other->SetOtherMonitor(
                        monitor.handle, monitor.device,
                        LoadMonitorProfile(monitor.device));
                    IslandScope scope(other.get());
                    chooseNotch(*other);
                    if (!other->Create()) {
                        other->Destroy();
                        continue;
                    }
                    Island& made = *other;
                    g_islands.push_back(other.get());
                    islands.push_back(std::move(other));
                    if (made.Notch() >= 0) {
                        addNotches(made, monitor.device);
                    }
                }
            }
            {
                IslandScope scope(&main);
                main.CatchUpToasts();
                main.RefreshWatches();
                main.StartGlassRenewals();
                main.StartWhatsNewNotice();
            }
            ForEachIsland([](Island& island) { island.Tick(); });
            ReopenSettingsWindow();
            // While one is animating, a frame per display refresh (or fewer,
            // see WaitForAnimationFrame); otherwise, only when there's
            // something to do.
            for (;;) {
                const bool animating =
                    std::any_of(g_islands.begin(), g_islands.end(),
                                [](Island* island) {
                                    return island->Animating();
                                });
                if (animating) {
                    if (!PumpMessages()) {
                        break;
                    }
                    RunIslandFrames();
                    WaitForAnimationFrame(frames);
                    continue;
                }
                if (GetMessage(&msg, nullptr, 0, 0) <= 0) {
                    break;
                }
                if (msg.message == WM_APP_ICONS && !msg.hwnd) {
                    OnIconsRead();
                    continue;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        // The main one's hidden Windows banner back first (a ringing one
        // shouldn't wait for the others' ends), then the others, then the
        // main one.
        {
            IslandScope scope(&main);
            main.ReleaseWindowsBanner();
        }
        for (size_t i = islands.size(); i-- > 1;) {
            IslandScope scope(islands[i].get());
            islands[i]->Destroy();
        }
        g_island = &main;
        main.Destroy();
        g_islands.clear();
        g_island = nullptr;
        StopUserCommands();
        ReleaseGlass();
        ReleaseIconPictures();
        UnregisterIslandClasses();
    }

    if (SUCCEEDED(oleResult)) {
        OleUninitialize();
    }
    if (SUCCEEDED(comResult)) {
        CoUninitialize();
    }
    // Not made (at sign-in, Windows may not be ready for it yet): tried
    // again 5 seconds later, a dozen times at most, unless the island is
    // stopped meanwhile.
    if (made) {
        g_islandTries = 0;
    } else if (g_settings.showIsland && ++g_islandTries < 12 &&
               WaitUnlessQuit(5000)) {
        IslandLog(L"The island couldn't be made: trying again");
        RestartIslandSoon();
    }
    return 0;
}

void StartIslandThread() {
    if (g_islandThread || !g_settings.showIsland) {
        return;
    }
    HANDLE started = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!started) {
        return;
    }
    g_islandThread = CreateThread(nullptr, 0, IslandThreadProc, started, 0,
                                  &g_islandThreadId);
    if (g_islandThread) {
        WaitForSingleObject(started, INFINITE);
    } else {
        g_islandThreadId = 0;
    }
    CloseHandle(started);
}

void StopIslandThread() {
    if (!g_islandThread) {
        return;
    }
    // Told again every half second until it ends: a WM_QUIT taken by a loop
    // inside one of its calls (a modal one, waiting for something) would
    // leave it running, and Windhawk waiting for it.
    PostThreadMessage(g_islandThreadId, WM_QUIT, 0, 0);
    for (int waits = 1;
         WaitForSingleObject(g_islandThread, 500) == WAIT_TIMEOUT; waits++) {
        if (waits == 10) {
            IslandLog(L"The island's thread hasn't ended after 5 s");
        }
        PostThreadMessage(g_islandThreadId, WM_QUIT, 0, 0);
    }
    CloseHandle(g_islandThread);
    g_islandThread = nullptr;
    g_islandThreadId = 0;
}

////////////////////////////////////////////////////////////////////////////////
// Auto-hide.

bool SetAutoHide(bool on) {
    APPBARDATA data{sizeof(data)};
    data.hWnd = FindWindow(L"Shell_TrayWnd", nullptr);
    if (!data.hWnd) {
        return false;
    }
    const UINT state = (UINT)SHAppBarMessage(ABM_GETSTATE, &data);
    if (((state & ABS_AUTOHIDE) != 0) != on) {
        data.lParam = on ? (state | ABS_AUTOHIDE) : (state & ~ABS_AUTOHIDE);
        // ABM_SETSTATE always returns TRUE. Read back the actual state,
        // rather than forgetting ownership when the shell rejected it.
        SHAppBarMessage(ABM_SETSTATE, &data);
    }
    return IsWindow(data.hWnd) &&
           (((SHAppBarMessage(ABM_GETSTATE, &data) & ABS_AUTOHIDE) != 0) == on);
}

// Another app's auto-hide bar on the taskbar's edge (a dock, a launcher, a
// taskbar tool), or null: Windows allows one per side and answers the
// taskbar's own with a dialog ("only one auto-hide toolbar per side").
HWND OtherAutoHideBar(HWND taskbar) {
    APPBARDATA position{sizeof(position)};
    position.hWnd = taskbar;
    if (!SHAppBarMessage(ABM_GETTASKBARPOS, &position)) {
        return nullptr;
    }
    MONITORINFO monitor{sizeof(monitor)};
    if (!GetMonitorInfo(MonitorFromRect(&position.rc, MONITOR_DEFAULTTONEAREST),
                        &monitor)) {
        return nullptr;
    }
    APPBARDATA asked{sizeof(asked)};
    asked.uEdge = position.uEdge;
    asked.rc = monitor.rcMonitor;
    HWND bar = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &asked);
    if (!bar || !IsWindow(bar)) {
        return nullptr;
    }
    WCHAR className[64] = L"";
    GetClassName(bar, className, ARRAYSIZE(className));
    return _wcsicmp(className, L"Shell_TrayWnd") == 0 ||
                   _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0
               ? nullptr
               : bar;
}

// A persisted flag means the user's original setting was OFF. Set it before
// changing Windows and clear it only after a confirmed restoration. If the
// user already had auto-hide on, the mod never takes ownership of it.
void UpdateAutoHide() {
    if (!OwnsTaskbar()) {
        return;
    }
    g_turnedAutoHideOn = g_turnedAutoHideOn ||
        GetUserIntValue(L"turnedAutoHideOn", 0) != 0;
    if (g_settings.autoHide && !g_unloading) {
        SetUserIntValue(L"explorerPartPid", (int)GetCurrentProcessId());
        APPBARDATA data{sizeof(data)};
        data.hWnd = FindCurrentProcessTaskbarWnd();
        if (!(SHAppBarMessage(ABM_GETSTATE, &data) & ABS_AUTOHIDE)) {
            // (Another app's auto-hide bar there: Windows would refuse with
            // its dialog; the taskbar stays as it is, and the log says
            // whose it is.)
            if (HWND other = OtherAutoHideBar(data.hWnd)) {
                WCHAR className[64] = L"";
                GetClassName(other, className, ARRAYSIZE(className));
                DWORD processId = 0;
                GetWindowThreadProcessId(other, &processId);
                IslandLog(L"Another app's auto-hide bar is on the taskbar's "
                          L"edge (%s, process %u): the taskbar's auto-hide "
                          L"is left as it is",
                          className, (unsigned)processId);
                return;
            }
            g_turnedAutoHideOn = true;
            SetUserIntValue(L"turnedAutoHideOn", 1);
            if (!SetAutoHide(true)) {
                IslandLog(L"The taskbar auto-hide setting could not be applied");
            }
        }
    } else if (g_turnedAutoHideOn) {
        if (SetAutoHide(false)) {
            g_turnedAutoHideOn = false;
            SetUserIntValue(L"turnedAutoHideOn", 0);
        } else {
            IslandLog(L"Taskbar auto-hide restoration is still pending");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Mod lifetime.

bool HookTaskbarDllSymbols() {
    HMODULE module =
        LoadLibraryEx(L"taskbar.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) {
        IslandLog(L"Failed to load taskbar.dll");
        return false;
    }

    WindhawkUtils::SYMBOL_HOOK taskbarDllHooks[] = {
        {
            {LR"(const CTaskBand::`vftable'{for `ITaskListWndSite'})"},
            &CTaskBand_ITaskListWndSite_vftable,
        },
        {
            {LR"(const CSecondaryTaskBand::`vftable'{for `ITaskListWndSite'})"},
            &CSecondaryTaskBand_ITaskListWndSite_vftable,
        },
        {
            {LR"(public: virtual class std::shared_ptr<class TaskbarHost> __cdecl CTaskBand::GetTaskbarHost(void)const )"},
            &CTaskBand_GetTaskbarHost_Original,
        },
        {
            {LR"(public: int __cdecl TaskbarHost::FrameHeight(void)const )"},
            &TaskbarHost_FrameHeight_Original,
        },
        {
            {LR"(public: virtual class std::shared_ptr<class TaskbarHost> __cdecl CSecondaryTaskBand::GetTaskbarHost(void)const )"},
            &CSecondaryTaskBand_GetTaskbarHost_Original,
        },
        {
            {LR"(public: void __cdecl std::_Ref_count_base::_Decref(void))"},
            &std__Ref_count_base__Decref_Original,
        },
        {
            {LR"(public: virtual void __cdecl TrayUI::StartTaskbar(void))"},
            &TrayUI_StartTaskbar_Original,
            TrayUI_StartTaskbar_Hook,
        },
        {
            {LR"(public: virtual struct HWND__ * __cdecl CSecondaryTray::GetTrayWindow(void))"},
            &CSecondaryTray_GetTrayWindow_Original,
        },
        {
            {LR"(public: virtual void __cdecl CSecondaryTray::InitModelAndHost(struct winrt::WindowsUdk::UI::Shell::TaskbarModel))"},
            &CSecondaryTray_InitModelAndHost_Original,
            CSecondaryTray_InitModelAndHost_Hook,
        },
        {
            {LR"(public: virtual void __cdecl TrayUI::SlideWindow(struct HWND__ *,struct tagRECT const *,struct HMONITOR__ *,bool,bool))"},
            &TrayUI_SlideWindow_Original,
            TrayUI_SlideWindow_Hook,
        },
        // Without it, the shown dock stays up with the mouse beside it.
        {
            {LR"(public: bool __cdecl TrayUI::_ShouldAutoHideTray(class AutoHideTelemetry::TaskbarAutoHideTelemetry::AutoHideOnTaskbarHide))"},
            &TrayUI__ShouldAutoHideTray_Original,
            TrayUI__ShouldAutoHideTray_Hook,
            true,
        },
        // Without these, a taskbar may still come up for an instant when an
        // app asks for attention (SlideWindow sends it back down).
        {
            {LR"(public: virtual void __cdecl TrayUI::Unhide(enum TrayCommon::TrayUnhideFlags,enum TrayCommon::UnhideRequest))"},
            &TrayUI_Unhide_Original,
            TrayUI_Unhide_Hook,
            true,
        },
        {
            {LR"(public: virtual long __cdecl CTaskBand::UnhideTray(void))"},
            &CTaskBand_UnhideTray_Original,
            CTaskBand_UnhideTray_Hook,
            true,
        },
        {
            {LR"(public: virtual long __cdecl CSecondaryTaskBand::UnhideTray(void))"},
            &CSecondaryTaskBand_UnhideTray_Original,
            CSecondaryTaskBand_UnhideTray_Hook,
            true,
        },
        {
            {LR"(public: virtual int __cdecl CTaskListWnd::PermitAutoHide(void))"},
            &CTaskListWnd_PermitAutoHide_Original,
            CTaskListWnd_PermitAutoHide_Hook,
            // Without it, the taskbar just shows itself for attention as
            // usual.
            true,
        },
        // Without these, the pill doesn't fill with downloads (see
        // PublishProgress).
        {
            {LR"(protected: void __cdecl CTaskBand::_OnSetProgressState(struct HWND__ * const,enum TBPFLAG))"},
            &CTaskBand__OnSetProgressState_Original,
            CTaskBand__OnSetProgressState_Hook,
            true,
        },
        {
            {LR"(protected: void __cdecl CTaskBand::_OnSetProgressValue(struct HWND__ * const,unsigned long))"},
            &CTaskBand__OnSetProgressValue_Original,
            CTaskBand__OnSetProgressValue_Hook,
            true,
        },
        // Without these, apps' overlay icons (like Discord's counter) don't
        // count as badges and stay as the taskbar draws them.
        {
            {LR"(protected: void __cdecl CTaskBand::_HandleOverlayIcon(struct HWND__ *,struct HICON__ *))"},
            &CTaskBand__HandleOverlayIcon_Original,
            CTaskBand__HandleOverlayIcon_Hook,
            true,
        },
        {
            {LR"(protected: void __cdecl CTaskBand::_HandleOverlayIconDescription(struct HWND__ *,void *))"},
            &CTaskBand__HandleOverlayIconDescription_Original,
            CTaskBand__HandleOverlayIconDescription_Hook,
            true,
        },
        {
            {LR"(public: virtual long __cdecl CTaskItem::SetOverlayIconDescription(unsigned short const *))"},
            &CTaskItem_SetOverlayIconDescription_Original,
            CTaskItem_SetOverlayIconDescription_Hook,
            true,
        },
        // Without these, an overlay icon set before the mod started stays as
        // the taskbar draws it until it changes (see ReadOverlayIcons).
        {
            {LR"(const CTaskBand::`vftable'{for `ITaskListManager'})"},
            &CTaskBand_ITaskListManager_vftable,
            nullptr,
            true,
        },
        {
            {LR"(const CWindowTaskItem::`vftable'{for `ITaskItem'})"},
            &CWindowTaskItem_ITaskItem_vftable,
            nullptr,
            true,
        },
        {
            {LR"(const CImmersiveTaskItem::`vftable'{for `ITaskItem'})"},
            &CImmersiveTaskItem_ITaskItem_vftable,
            nullptr,
            true,
        },
        {
            {LR"(public: virtual long __cdecl CTaskBand::GetTaskFromWindow(struct HWND__ *,struct ITaskGroup * *,struct ITaskItem * *))"},
            &CTaskBand_GetTaskFromWindow_Original,
            nullptr,
            true,
        },
        {
            {LR"(public: virtual struct HICON__ * __cdecl CTaskItem::GetOverlayIcon(void))"},
            &CTaskItem_GetOverlayIcon_Original,
            nullptr,
            true,
        },
        {
            {LR"(public: virtual unsigned short const * __cdecl CTaskItem::GetOverlayIconDescription(void))"},
            &CTaskItem_GetOverlayIconDescription_Original,
            nullptr,
            true,
        },
        // Without these, a taskbar hidden for a window in its way
        // (hiding.hideWhen) comes back only with the mouse at the bottom
        // edge (see UnhideTaskbar).
        {
            {LR"(const TrayUI::`vftable'{for `ITrayComponentHost'})"},
            &TrayUI_ITrayComponentHost_vftable,
            nullptr,
            true,
        },
        {
            {LR"(public: virtual __int64 __cdecl TrayUI::WndProc(struct HWND__ *,unsigned int,unsigned __int64,__int64,bool *))"},
            &TrayUI_WndProc_Original,
            TrayUI_WndProc_Hook,
            true,
        },
        {
            {LR"(private: void __cdecl CSecondaryTray::_Unhide(enum TrayCommon::TrayUnhideFlags,enum TrayCommon::UnhideRequest))"},
            &CSecondaryTray__Unhide_Original,
            nullptr,
            true,
        },
        {
            {LR"(private: virtual __int64 __cdecl CSecondaryTray::v_WndProc(struct HWND__ *,unsigned int,unsigned __int64,__int64))"},
            &CSecondaryTray_v_WndProc_Original,
            CSecondaryTray_v_WndProc_Hook,
            true,
        },
    };

    if (!HookSymbols(module, taskbarDllHooks, ARRAYSIZE(taskbarDllHooks))) {
        IslandLog(L"HookSymbols failed");
        return false;
    }
    return true;
}

