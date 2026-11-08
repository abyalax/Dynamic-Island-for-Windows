void ResetDockOfTaskbar();

void ResetDockBehavior() {
    // The timer and the handlers that run on their own go first: a step
    // below failing mustn't leave them ticking after the unload.
    if (g_badgeTimer) {
        Safely(L"Stopping the badge timer", [&] { g_badgeTimer.Stop(); });
        g_badgeTimer = nullptr;
    }
    Safely(L"Removing the rendering handler",
           [&] { g_renderingRevoker.revoke(); });
    StopWatchingProgressBars();
    UnwatchThumbnails();
    for (auto& state : g_otherDocks) {
        DockScope scope(state->wnd);
        ResetDockOfTaskbar();
    }
    g_otherDocks.clear();
    // (A show Windows made at once still waiting, see kQuietShowDelayMs: its
    // timer mustn't tick after the unload; its cut goes with the others.)
    if (g_quietShownWnd) {
        KillTimer(g_quietShownWnd, kQuietShowTimerId);
        g_quietShownWnd = nullptr;
    }
    for (HWND hWnd : g_ownClips) {
        if (IsWindow(hWnd)) {
            SetWindowRgn(hWnd, nullptr, TRUE);
        }
    }
    g_ownClips.clear();

    AcquireSRWLockExclusive(&g_badgeAppsLock);
    g_badgeApps.clear();
    ReleaseSRWLockExclusive(&g_badgeAppsLock);
    g_stateRevokers.clear();
    g_dotRevokers.clear();
    for (const auto& watch : g_badgeWatches) {
        if (auto element = watch.element.get()) {
            Safely(L"Releasing a taskbar badge watch", [&] {
                element.UnregisterPropertyChangedCallback(watch.property,
                                                          watch.token);
            });
        }
    }
    g_badgeWatches.clear();
    g_watchedButtons.clear();
    ResetDockOfTaskbar();
}

// The mod's shelf canvases in `panel` taken out, all of them: one left by an
// earlier style, or by an earlier load of the mod (its weak reference gone),
// would keep drawing its shelf where it was, in the dock's color, seen as a
// white band whenever the dock's room opens.
void RemoveDockShelfHosts(Controls::Panel const& panel) {
    auto children = panel.Children();
    for (uint32_t i = children.Size(); i-- > 0;) {
        if (auto element = children.GetAt(i).try_as<FrameworkElement>();
            element && element.Name() == kDockShelfHostName) {
            children.RemoveAt(i);
        }
    }
}

// The taskbar in the globals (see DockScope): its handlers, icons, shadow
// and cut go. A dock shown for a notification goes back down, and its timer
// stops (a started DispatcherTimer would tick on after the unload).
void ResetDockOfTaskbar() {
    if (g_notificationTimer) {
        g_notificationTimer.Stop();
        g_notificationTimer = nullptr;
    }
    if (g_shownForNotification) {
        g_shownForNotification = false;
        if (g_dockWnd) {
            const RECT& hidden = g_notificationHiddenRect;
            SetWindowPos(g_dockWnd, nullptr, hidden.left, hidden.top,
                         hidden.right - hidden.left,
                         hidden.bottom - hidden.top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    g_layoutRevoker.revoke();
    // (Each part on its own: a retired element failing mustn't leave the
    // rest, its timers above all, running after the unload.)
    if (auto target = g_pointerTarget.get()) {
        Safely(L"Removing the taskbar pointer handlers", [&] {
            if (g_pointerHandler) {
                target.RemoveHandler(UIElement::PointerMovedEvent(),
                                     g_pointerHandler);
            }
            if (g_pressedHandler) {
                target.RemoveHandler(UIElement::PointerPressedEvent(),
                                     g_pressedHandler);
            }
            if (g_wheelHandler) {
                target.RemoveHandler(UIElement::PointerWheelChangedEvent(),
                                     g_wheelHandler);
            }
        });
    }
    g_pointerHandler = nullptr;
    g_pressedHandler = nullptr;
    g_wheelHandler = nullptr;
    g_pointerTarget = nullptr;
    for (auto& icon : g_dockIcons) {
        Safely(L"Restoring a taskbar icon", [&] { RestoreDockIcon(icon); });
    }
    g_dockIcons.clear();
    if (g_dockHovered == g_dockWnd) {
        g_dockHovered = nullptr;
    }
    if (g_dockWnd) {
        KillTimer(g_dockWnd, kAnimatorTimerId);
    }
    Safely(L"Resuming the dock's animations", [] { ResumeRepeaterAnimations(); });
    if (auto frame = g_dockFrame.get(); frame && g_dockContentHidden) {
        Safely(L"Restoring the dock's opacity",
               [&] { RestoreDockOpacity(frame); });
    }
    g_dockContentHidden = false;
    g_dockScaleSettled = true;
    g_dockScaleMoved = false;
    g_dockOpacityKept = nullptr;
    g_dockFrame = nullptr;
    g_dockRepeater = nullptr;
    // (The mod's own canvas, see ApplyStyleUnscoped.)
    if (auto background = g_dockBackground.get()) {
        Safely(L"Removing the dock's canvas", [&] {
            if (background.Name() == kDockShelfHostName) {
                if (auto panel = Media::VisualTreeHelper::GetParent(background)
                                     .try_as<Controls::Panel>()) {
                    RemoveDockShelfHosts(panel);
                }
            }
        });
    }
    g_dockShelf = {};
    g_dockShelfRest = {};
    g_dockShelfNow = {};
    g_dockRoomOpen = false;
    g_dockBackground = nullptr;
    if (auto fill = g_shadowFill.get(); fill && g_shadowVisual) {
        Safely(L"Removing the dock's shadow", [&] {
            Hosting::ElementCompositionPreview::SetElementChildVisual(fill,
                                                                     nullptr);
        });
    }
    g_shadowVisual = nullptr;
    g_shadowFill = nullptr;
    ResetAppsArea();
    g_dockItemCount = -1;
    g_dockWnd = nullptr;
}

bool ApplyStyleUnscoped(XamlRoot xamlRoot, HWND hWnd, bool mainTaskbar);

// Another monitor's taskbar gets a dock state of its own (see DockState).
bool ApplyStyle(XamlRoot xamlRoot, HWND hWnd, bool mainTaskbar) {
    if (mainTaskbar) {
        return ApplyStyleUnscoped(xamlRoot, hWnd, true);
    }
    // (Styled again, as when its monitor comes back: its old handlers go.)
    const bool again = FindOtherDock(hWnd) != nullptr;
    if (!again) {
        auto state = std::make_unique<DockState>();
        state->wnd = hWnd;
        g_otherDocks.push_back(std::move(state));
    }
    DockScope scope(hWnd);
    if (again) {
        ResetDockOfTaskbar();
        g_dockWnd = hWnd;
    }
    return ApplyStyleUnscoped(xamlRoot, hWnd, false);
}

bool ApplyStyleUnscoped(XamlRoot xamlRoot, HWND hWnd, bool mainTaskbar) {
    auto content = xamlRoot.Content().try_as<FrameworkElement>();
    if (!content) {
        return false;
    }

    auto frame = FindByClassName(content, L"Taskbar.TaskbarFrame");
    if (!frame) {
        IslandLog(L"TaskbarFrame not found");
        return false;
    }
    auto rootGrid = FindByName(frame, L"RootGrid");
    if (!rootGrid) {
        IslandLog(L"RootGrid not found");
        return false;
    }

    // The Windows taskbar: as it is (maybe without its tray), or without what
    // the island shows (the tray on the right, the widgets on the left). The
    // apps' badges are still watched, for the island.
    if (!IsDock()) {
        const bool onlyApps =
            g_settings.taskbarMode ==
                decltype(g_settings.taskbarMode)::WindowsApps ||
            IsTransparentTaskbar();
        if (onlyApps ||
            (g_settings.windowsHideTray && g_settings.showIsland)) {
            if (auto tray =
                    FindByClassName(content, L"SystemTray.SystemTrayFrame")) {
                SetStyled(tray, UIElement::VisibilityProperty(),
                          winrt::box_value(Visibility::Collapsed));
            }
        }
        if (onlyApps) {
            std::vector<FrameworkElement> widgets;
            FindAllByClassName(frame, {L"Taskbar.AugmentedEntryPointButton"},
                               widgets);
            for (const auto& widget : widgets) {
                SetStyled(widget, UIElement::VisibilityProperty(),
                          winrt::box_value(Visibility::Collapsed));
            }
        }
        // Transparent: no background and no line on top, only a soft shadow
        // rising from the bottom edge, so the icons still stand out over
        // light windows.
        if (IsTransparentTaskbar()) {
            if (auto background =
                    FindByClassName(rootGrid, L"Taskbar.TaskbarBackground")) {
                if (auto fill = FindByName(background, L"BackgroundFill")) {
                    SetStyled(fill, Shapes::Shape::FillProperty(),
                              Media::SolidColorBrush(
                                  winrt::Windows::UI::Colors::Transparent()));
                    SetStyled(fill, UIElement::OpacityProperty(),
                              winrt::box_value(g_shadowStrength));
                    // (The shadow rises from the bottom edge: not on the
                    // other edges.)
                    if (g_taskbarAtBottom) {
                        g_shadowFill = winrt::make_weak(fill.as<UIElement>());
                        MakeShadow(fill);
                    }
                }
                if (auto stroke = FindByName(background, L"BackgroundStroke")) {
                    SetStyled(stroke, UIElement::VisibilityProperty(),
                              winrt::box_value(Visibility::Collapsed));
                }
            } else {
                IslandLog(L"TaskbarBackground not found");
            }
        }
        SetupDockBehavior(hWnd, frame, nullptr);
        return true;
    }

    // Only as wide as the icons, centered, at the bottom of the window (its
    // room above is the dock's, see GrownTaskbar).
    SetStyled(frame, FrameworkElement::HorizontalAlignmentProperty(),
              winrt::box_value(HorizontalAlignment::Center));
    SetStyled(frame, FrameworkElement::VerticalAlignmentProperty(),
              winrt::box_value(VerticalAlignment::Bottom));
    SetStyled(frame, FrameworkElement::WidthProperty(),
              winrt::box_value(std::numeric_limits<double>::quiet_NaN()));
    WatchFrameWidth(frame);

    // The dock has its own height, at the bottom of the taskbar. Whatever the
    // taskbar has above it stays transparent: room for the magnified icons,
    // and the strip that stays on screen while the taskbar is hidden.
    SetStyled(rootGrid, Controls::Grid::PaddingProperty(),
              winrt::box_value(ThicknessHelper::FromLengths(8, 0, 8, 0)));

    // The dock's shelf is drawn on the taskbar's own background rectangle,
    // which lies behind the icons (see DockShelf). Rounding the panel that
    // holds the icons would also cut off the magnified icons at its edges.
    Shapes::Rectangle fill{nullptr};
    if (auto background =
            FindByClassName(rootGrid, L"Taskbar.TaskbarBackground")) {
        fill = FindByName(background, L"BackgroundFill")
                   .try_as<Shapes::Rectangle>();
        if (auto stroke = FindByName(background, L"BackgroundStroke")) {
            SetStyled(stroke, UIElement::VisibilityProperty(),
                      winrt::box_value(Visibility::Collapsed));
        }
    }
    // The buttons keep their place, and their icons are lined up with the
    // shelf (see OnRendering).
    FrameworkElement dockBackground{nullptr};
    if (fill) {
        StyleDockBackground(fill);
    }
    // The shelf goes on a canvas of its own, behind everything in the
    // taskbar's XAML and as big as it: the background is cut to the
    // taskbar's own height, the shelf isn't (see DockShelf).
    if (auto panel = content.try_as<Controls::Panel>(); fill && panel) {
        RemoveDockShelfHosts(panel);
        Controls::Canvas shelfHost;
        shelfHost.Name(kDockShelfHostName);
        shelfHost.IsHitTestVisible(false);
        panel.Children().InsertAt(0, shelfHost);
        g_dockShelf = MakeDockShelf(shelfHost);
        dockBackground = shelfHost;
    } else {
        IslandLog(L"BackgroundFill not found, rounding the panel instead");
        StylePill(rootGrid, ThicknessHelper::FromLengths(
                                0, 0, 0, g_settings.bottomGap));
        SetDockGeometry(rootGrid, GetDockMetrics(0).Height(), 0);
    }

    if (auto tray = FindByClassName(content, L"SystemTray.SystemTrayFrame")) {
        if (g_settings.hideTray || g_settings.showIsland) {
            SetStyled(tray, UIElement::VisibilityProperty(),
                      winrt::box_value(Visibility::Collapsed));
        } else if (auto trayGrid = FindByName(tray, L"SystemTrayFrameGrid")) {
            SetStyled(tray, FrameworkElement::VerticalAlignmentProperty(),
                      winrt::box_value(VerticalAlignment::Bottom));
            StylePill(trayGrid, ThicknessHelper::FromLengths(
                                    0, 0, 6, g_settings.bottomGap));
            SetDockGeometry(trayGrid, kDockTrayHeight, 6);
        }
    }

    // Magnification and bouncing follow the mouse, on every taskbar.
    SetupDockBehavior(hWnd, frame, dockBackground);
    return true;
}

// Whether the main taskbar got its style (it may not be ready yet when the
// mod loads with Explorer; see LateStartThreadProc).
std::atomic<bool> g_mainTaskbarStyled;

void ApplyToTaskbar(HWND hWnd, XamlRoot xamlRoot, bool mainTaskbar) {
    if (!xamlRoot) {
        IslandLog(L"Getting XamlRoot failed for %p", hWnd);
        return;
    }
    bool applied = false;
    Safely(L"Styling the taskbar",
           [&] { applied = ApplyStyle(xamlRoot, hWnd, mainTaskbar); });
    if (!applied) {
        IslandLog(L"ApplyStyle failed for %p", hWnd);
        return;
    }
    if (mainTaskbar) {
        g_mainTaskbarStyled = true;
    }
}

using RunFromWindowThreadProc_t = void(WINAPI*)(void* parameter);

// Runs a function on a window's thread (with a hook on that thread for the
// message sent to the window), and returns once it ran.
const UINT g_runFromWindowThreadMessage =
    RegisterWindowMessage(L"Windhawk_RunFromWindowThread_" WH_MOD_ID);

struct RunFromWindowThreadParam {
    RunFromWindowThreadProc_t proc;
    void* procParam;
};

LRESULT CALLBACK RunFromWindowThreadHookProc(int nCode,
                                             WPARAM wParam,
                                             LPARAM lParam) {
    if (nCode == HC_ACTION) {
        const CWPSTRUCT* cwp = (const CWPSTRUCT*)lParam;
        if (cwp->message == g_runFromWindowThreadMessage) {
            auto* param = (RunFromWindowThreadParam*)cwp->lParam;
            Safely(L"Running on the taskbar thread", [&] {
                param->proc(param->procParam);
            });
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool RunFromWindowThread(HWND hWnd,
                         RunFromWindowThreadProc_t proc,
                         void* procParam) {
    DWORD dwThreadId = GetWindowThreadProcessId(hWnd, nullptr);
    if (dwThreadId == 0) {
        return false;
    }

    if (dwThreadId == GetCurrentThreadId()) {
        Safely(L"Running on the taskbar thread", [&] { proc(procParam); });
        return true;
    }

    HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, RunFromWindowThreadHookProc,
                                  nullptr, dwThreadId);
    if (!hook) {
        return false;
    }

    RunFromWindowThreadParam param{proc, procParam};
    SendMessage(hWnd, g_runFromWindowThreadMessage, 0, (LPARAM)&param);

    UnhookWindowsHookEx(hook);
    return true;
}

// Styles a taskbar window of this thread.
BOOL CALLBACK ApplyToTaskbarWindowProc(HWND hWnd, LPARAM) {
    WCHAR className[32];
    if (!GetClassName(hWnd, className, ARRAYSIZE(className))) {
        return TRUE;
    }
    if (_wcsicmp(className, L"Shell_TrayWnd") == 0) {
        ApplyToTaskbar(hWnd, GetTaskbarXamlRoot(hWnd), true);
    } else if (_wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0) {
        ApplyToTaskbar(hWnd, GetSecondaryTaskbarXamlRoot(hWnd), false);
    }
    return TRUE;
}

// Restyles every taskbar from the taskbar thread, or removes the style while
// unloading.
// Unloading: what the XAML globals still hold goes, on the taskbar's thread
// (their destructors never run, see g_styledProperties).
void FreeXamlGlobals() {
    g_otherDocks.clear();
    g_dockShelf = {};
    g_bigIcons.reset();
    g_overlaySources.reset();
    // (The overlay icons' copies too.)
    AcquireSRWLockExclusive(&g_overlayIconsLock);
    for (const auto& overlay : g_overlayIcons) {
        DestroyIcon(overlay.icon);
    }
    std::vector<OverlayIcon>().swap(g_overlayIcons);
    ReleaseSRWLockExclusive(&g_overlayIconsLock);
    g_animatorKept = nullptr;
    g_animatorProperty = nullptr;
    g_animatorPropertyTried = false;
    std::vector<StyledProperty>().swap(g_styledProperties);
    std::vector<WidthWatch>().swap(g_widthWatches);
    std::vector<DockIcon>().swap(g_dockIcons);
    std::vector<VisualStateGroup::CurrentStateChanged_revoker>().swap(
        g_stateRevokers);
    std::vector<FrameworkElement::SizeChanged_revoker>().swap(g_dotRevokers);
    std::vector<BadgeWatch>().swap(g_badgeWatches);
    StopWatchingProgressBars();
    g_layoutRevoker = {};
    g_renderingRevoker = {};
}

// The dock's room above the taskbars made again (see GrownTaskbar), defined
// further down.
void WINAPI RefreshDockRoomProc(void*);

void ApplySettingsFromTaskbarThread() {
    IslandLog(L"Applying settings");

    // Without the dock now (or unloading), the taskbars get their own height
    // back first: the styles going would otherwise move a window still
    // grown for a dock that isn't there.
    if (!IsDock() || g_unloading) {
        Safely(L"Giving the taskbars their height back",
               [] { RefreshDockRoomProc(nullptr); });
    }
    // (Whatever fails in there, the globals below are still freed.)
    Safely(L"Removing the taskbar styles", [] { ClearStyles(); });
    g_mainTaskbarStyled = false;
    if (g_unloading) {
        // (Its timer is this thread's: stopped here, not by the unloading
        // thread.)
        if (HWND taskbar = FindCurrentProcessTaskbarWnd()) {
            KillTimer(taskbar, kDisplayRestyleTimerId);
        }
        FreeXamlGlobals();
        return;
    }

    EnumThreadWindows(GetCurrentThreadId(), ApplyToTaskbarWindowProc, 0);
    RefreshDockRoomProc(nullptr);
}

void WINAPI ApplySettingsProc(void*) {
    ApplySettingsFromTaskbarThread();
}

void ApplySettings() {
    if (HWND hTaskbarWnd = FindCurrentProcessTaskbarWnd()) {
        RunFromWindowThread(
            hTaskbarWnd, ApplySettingsProc, nullptr);
    }
}

using TrayUI_StartTaskbar_t = void(WINAPI*)(void* pThis);
TrayUI_StartTaskbar_t TrayUI_StartTaskbar_Original;
void WINAPI TrayUI_StartTaskbar_Hook(void* pThis) {
    IslandLog(L">");
    TrayUI_StartTaskbar_Original(pThis);
    Safely(L"Styling the new taskbar", [] { ApplySettingsFromTaskbarThread(); });
}

using CSecondaryTray_GetTrayWindow_t = HWND(WINAPI*)(void* pThis);
CSecondaryTray_GetTrayWindow_t CSecondaryTray_GetTrayWindow_Original;

using CSecondaryTray_InitModelAndHost_t = void(WINAPI*)(void* pThis,
                                                        void* taskbarModel);
CSecondaryTray_InitModelAndHost_t CSecondaryTray_InitModelAndHost_Original;
// Defined further down.
void SubclassTaskbars();

void WINAPI CSecondaryTray_InitModelAndHost_Hook(void* pThis,
                                                 void* taskbarModel) {
    IslandLog(L">");
    CSecondaryTray_InitModelAndHost_Original(pThis, taskbarModel);
    if (g_unloading) {
        return;
    }
    HWND taskbarWnd = CSecondaryTray_GetTrayWindow_Original(pThis);
    ApplyToTaskbar(taskbarWnd, GetSecondaryTaskbarXamlRoot(taskbarWnd), false);
    SubclassTaskbars();
}

////////////////////////////////////////////////////////////////////////////////
// Showing and hiding (taskbar thread).

// A slightly springy settle, like the macOS dock: fast at first, a hint of
// overshoot, then at rest. `t` goes from 0 to 1 over the animation.
double SpringOut(double t) {
    constexpr double kDamping = 0.78;
    // Settles within the duration.
    constexpr double kOmega = 5.2 / kDamping;
    const double dampedOmega = kOmega * std::sqrt(1 - kDamping * kDamping);
    return 1 - std::exp(-kDamping * kOmega * t) *
                   (std::cos(dampedOmega * t) +
                    kDamping * kOmega / dampedOmega * std::sin(dampedOmega * t));
}

double EaseInOutCubic(double t) {
    const double u = -2 * t + 2;
    return t < 0.5 ? 4 * t * t * t : 1 - u * u * u / 2;
}

int LerpInt(int a, int b, double t) {
    return a + (int)std::lround((b - a) * t);
}

// Waits for the next frame of an animation: one refresh of the screen after
// the last one (or the frame rate limit's interval), on a high-resolution
// timer. Not DwmFlush: waiting for the compositor after drawing puts the
// drawing and the refresh one after the other, which halves the frames on
// fast screens (as macOS minimize animation found too). A frame running late
// starts the next interval from now, not from when it should have started.
// Each animation loop has its own timer (a local: none is left behind on
// explorer's threads when the mod unloads).
struct FrameTimer {
    HANDLE handle = nullptr;
    double next = 0;
    double rate = 60;
    double rateReadAt = -100;
    FrameTimer() = default;
    FrameTimer(const FrameTimer&) = delete;
    FrameTimer& operator=(const FrameTimer&) = delete;
    ~FrameTimer() {
        if (handle) {
            CloseHandle(handle);
        }
    }
};

void WaitForAnimationFrame(FrameTimer& timer) {
    if (!timer.handle) {
        timer.handle = CreateWaitableTimerExW(
            nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
            TIMER_MODIFY_STATE | SYNCHRONIZE);
    }
    const double now = NowSeconds();
    if (now - timer.rateReadAt > 5) {
        timer.rateReadAt = now;
        DWM_TIMING_INFO timing{sizeof(timing)};
        if (SUCCEEDED(DwmGetCompositionTimingInfo(nullptr, &timing)) &&
            timing.rateRefresh.uiDenominator &&
            timing.rateRefresh.uiNumerator) {
            timer.rate = (double)timing.rateRefresh.uiNumerator /
                         timing.rateRefresh.uiDenominator;
        }
    }
    const int limit = g_frameRateLimit.load(std::memory_order_relaxed);
    const double interval =
        1.0 / (limit ? std::min((double)limit, timer.rate) : timer.rate);
    timer.next = std::max(timer.next + interval, now);
    const double wait = timer.next - now;
    if (wait <= 0.0005) {
        return;
    }
    LARGE_INTEGER due;
    due.QuadPart = -(LONGLONG)(wait * 10'000'000);
    if (timer.handle && SetWaitableTimer(timer.handle, &due, 0, nullptr,
                                         nullptr, FALSE)) {
        WaitForSingleObject(timer.handle, 100);
    } else {
        Sleep((DWORD)std::ceil(wait * 1000));
    }
}

