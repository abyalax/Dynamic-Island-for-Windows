#pragma once
void Island::ReleaseWindowsBanner() {
    m_stackTakeover = false;
    if (m_stackHidden) {
        RestoreWindowsBanner();
    }
    for (HWINEVENTHOOK* hook :
         {&m_bannerHook, &m_bannerMoveHook, &m_foregroundHook}) {
        if (*hook) {
            UnhookWinEvent(*hook);
            *hook = nullptr;
        }
    }
}

void Island::Destroy() {
    // A tray icon held on it or over it: let go (its window, about to go,
    // loses the mouse with nothing held).
    if (g_trayDrag.owner == this || g_trayDrag.drop.island == this) {
        g_trayDrag = {};
    }
    // A Windows banner moved off screen (one could be ringing) comes back
    // first, before the waits below. Only the main island takes it over.
    ReleaseWindowsBanner();
    if (m_main) {
        g_windowsBanner = nullptr;
        if (m_profileSavePending) {
            SaveActiveProfile();
        }
        StopTrayPlacesSender();
        UpdateWindowsQuiet(false);
        g_islandWnd = nullptr;
    } else if (m_profileSavePending) {
        // (Its own changes, see SaveOwnProfile.)
        SaveOwnProfile(false);
    }
    if (g_trayIsland == this) {
        g_trayIsland = nullptr;
    }
    StopUrgentSound();
    // (An alarm's sound repeats on its own: it'd go on after the island,
    // with nothing left to stop it.)
    if (m_main) {
        StopAlarmSound();
    }
    FreeSoundPlayer();
    WaitForToastActivations();
    if (m_hwnd) {
        KillTimer(m_hwnd, kBannerTimerId);
        KillTimer(m_hwnd, kMediaTimerId);
        KillTimer(m_hwnd, kEyesTimerId);
    }
    if (m_bannerWnd) {
        DestroyWindow(m_bannerWnd);
        m_bannerWnd = nullptr;
    }
    StopStatusWatch();
    if (m_main) {
        g_lyricsStop = true;
        CancelHttps(&g_lyricsCancel);
        StopMediaWatch();
        StopUsageWatch();
    }
    // Everything waited for below is told to stop first, so the waits run
    // together.
    if (m_updatesStop) {
        SetEvent(m_updatesStop);
    }
    if (m_bannerReadStop) {
        SetEvent(m_bannerReadStop);
    }
    if (m_weatherThread) {
        // The requests give up after a few seconds, and the location it may
        // be waiting for is canceled (g_weatherStop), so this is short; the
        // limit is for a network call that doesn't give up.
        g_weatherStop = true;
        WaitForSingleObject(m_weatherThread, 20000);
        CloseHandle(m_weatherThread);
        m_weatherThread = nullptr;
    }
    // (They hold the brightness job too.)
    m_brightnessJob->notify = nullptr;
    JoinOwningThreads(m_brightnessThreads, 5000);
    // The lyrics worker shares its cache and stop flag: finish before restart
    // (at once: its request was closed above).
    if (m_lyricsThread) {
        g_lyricsStop = true;
        WaitForSingleObject(m_lyricsThread, INFINITE);
        CloseHandle(m_lyricsThread);
        m_lyricsThread = nullptr;
    }
    for (HANDLE thread : m_updatesThreads) {
        WaitForSingleObject(thread, 10000);
        CloseHandle(thread);
    }
    m_updatesThreads.clear();
    if (m_updatesStop) {
        CloseHandle(m_updatesStop);
        m_updatesStop = nullptr;
    }
    for (Panel* panel : Panels()) {
        panel->Destroy();
    }
    if (m_settingsWindow.Window()) {
        if (m_settingsWindow.IsOpen() && !g_unloading) {
            SettingsWindowState state;
            state.at = NowSeconds();
            state.device = m_main ? L"" : OwnDevice();
            m_settingsWindow.KeepWindowState(&state);
            std::erase_if(g_settingsWindowStates,
                          [&](const SettingsWindowState& kept) {
                              return kept.device == state.device;
                          });
            g_settingsWindowStates.push_back(std::move(state));
        }
        m_settingsWindow.Destroy();
    }
    // (After the panels and the settings: closing them ends a shortcut
    // recording, which would start the keyboard hook again. Nothing wanted
    // anymore, so another island going after this one doesn't either.)
    if (m_main) {
        g_shortcutKeys = 0;
        g_shortcutRecording = false;
        StopShortcutKeys();
    }
    // (The reads of Windows banners only read them.)
    for (HANDLE thread : m_bannerReadThreads) {
        WaitForSingleObject(thread, 10000);
        CloseHandle(thread);
    }
    m_bannerReadThreads.clear();
    if (m_bannerReadStop) {
        CloseHandle(m_bannerReadStop);
        m_bannerReadStop = nullptr;
    }
    AcquireSRWLockExclusive(&g_windowsBannerReadsLock);
    g_windowsBannerReads.clear();
    ReleaseSRWLockExclusive(&g_windowsBannerReadsLock);
    StopWatchingTrayAppWindows();
    m_trayIcons.clear();
    m_trayBitmaps.clear();
    if (m_appBar) {
        APPBARDATA data{sizeof(data)};
        data.hWnd = m_hwnd;
        SHAppBarMessage(ABM_REMOVE, &data);
        m_appBar = false;
    }
    if (m_clipboardWatched) {
        RemoveClipboardFormatListener(m_hwnd);
        m_clipboardWatched = false;
    }
    for (HWINEVENTHOOK& hook : m_inputMethodHooks) {
        if (hook) {
            UnhookWinEvent(hook);
            hook = nullptr;
        }
    }
    StopInputMethodThread();
    StopDesktopsWatch();
    StopWorldClocksWatch();
    StopAiUsageWatch();
    StopWindowEvents();
    StopKomorebiWatch();
    if (m_recycleNotify) {
        SHChangeNotifyDeregister(m_recycleNotify);
        m_recycleNotify = 0;
    }
    if (m_hwnd) {
        KillTimer(m_hwnd, kNetSpeedTimerId);
        KillTimer(m_hwnd, kRecycleTimerId);
    }
    // (They own their jobs: one still busy, on a sleeping network drive or
    // a drive slow to answer, is let go.)
    for (HANDLE* thread : {&m_wallpaperThread, &m_wallpaperListThread,
                           &m_driveThread, &m_recycleThread}) {
        if (*thread) {
            WaitForSingleObject(*thread, 5000);
            CloseHandle(*thread);
            *thread = nullptr;
        }
    }
    MSG listed;
    while (m_hwnd && PeekMessage(&listed, m_hwnd, WM_APP_WALLPAPER_LIST,
                                 WM_APP_WALLPAPER_LIST, PM_REMOVE)) {
        delete (WallpaperListJob*)listed.lParam;
    }
    if (m_main) {
        StopPrivacyWatch();
    }
    for (HWND* window : {&m_bubbleWnd, &m_glassStrip, &m_tipWnd}) {
        if (*window) {
            DestroyWindow(*window);
            *window = nullptr;
        }
    }
    m_liquid.Destroy();
    m_bubbleCanvas.Destroy();
    m_tipCanvas.Destroy();
    m_tipLayout = nullptr;
    m_tipShownAt = m_tipHideAt = 0;
    m_tipDrawnKey.clear();
    m_barBlurred = false;
    if (m_hwnd) {
        m_drop.Revoke();
        KillTimer(m_hwnd, kFoldersDropTimerId);
        KillTimer(m_hwnd, kFoldersDragLeftTimerId);
        KillTimer(m_hwnd, kIslandTimerId);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    m_canvas.Destroy();
    if (m_wlan) {
        WlanCloseHandle(m_wlan, nullptr);
        m_wlan = nullptr;
    }
}

// The bar's strip; for the pill keeping its room, a strip along the edge its
// place is at (its own place, not an app's: windows don't change size as
// apps come to the front), as deep as the pill and its distance from the
// edge twice. None while it's away from the edges or minimized. Told to the
// system only when it changes (or when asked again).
void Island::SetAppBarPosition(bool force) {
    if (!m_appBar) {
        return;
    }
    UINT edge = ABE_TOP;
    // (The bar's window is taller while it shows a banner; on a side, its
    // strip is as wide as the bar.)
    int thickness = (int)std::ceil(m_pill.bottom - m_pill.top);
    if (m_bar && m_vertical) {
        edge = m_sideLeft ? ABE_LEFT : ABE_RIGHT;
        thickness = (int)std::ceil(m_pill.right - m_pill.left);
    }
    if (m_notch >= 0) {
        // The notches: their height, while one isn't a line.
        edge = ABE_TOP;
        thickness = 0;
        const std::vector<Island*> notches = NotchSet();
        if (!g_settings.islandAutoHide &&
            std::any_of(
                notches.begin(), notches.end(),
                [](const Island* notch) { return !notch->m_minimized; })) {
            thickness =
                (int)std::lround(kPillHeight * PillHeightScale() * m_scale);
        }
    } else if (!m_bar) {
        const IslandPlace& place = g_islandPlace;
        const bool upright = g_settings.islandVerticalSides &&
                             (place.x < 0.001 || place.x > 0.999) &&
                             place.y > 0.001 && place.y < 0.999;
        edge = upright ? (place.x < 0.5 ? ABE_LEFT : ABE_RIGHT)
               : place.y > 0.5 ? ABE_BOTTOM
                               : ABE_TOP;
        thickness = 0;
        if (MinimizableAt(place) && !m_minimized &&
            !g_settings.islandAutoHide) {
            const float across = (upright ? kVerticalPillWidth : kPillHeight) *
                                 PillHeightScale() * m_scale;
            thickness = (int)std::lround(across + GapShown() * m_dpiScale * 2);
        }
    }
    // (A hidden bar gives its strip back.)
    if (m_bar && m_barHidden) {
        thickness = 0;
    }
    if (!force && edge == m_appBarEdge && thickness == m_appBarThickness &&
        EqualRect(&m_monitor, &m_appBarMonitor)) {
        return;
    }
    // Another edge: the strip is told again from the start (ABM_SETPOS on a
    // new edge kept the old strip out of the work area too: the bar moved
    // from the top to a side reserved both).
    if (edge != m_appBarEdge) {
        APPBARDATA data{sizeof(data)};
        data.hWnd = m_hwnd;
        SHAppBarMessage(ABM_REMOVE, &data);
        data.uCallbackMessage = WM_APP_APPBAR;
        if (!SHAppBarMessage(ABM_NEW, &data)) {
            m_appBar = false;
            return;
        }
    }
    m_appBarEdge = edge;
    m_appBarThickness = thickness;
    m_appBarMonitor = m_monitor;
    auto fit = [&](RECT* rc) {
        switch (edge) {
            case ABE_BOTTOM:
                rc->top = rc->bottom - thickness;
                break;
            case ABE_LEFT:
                rc->right = rc->left + thickness;
                break;
            case ABE_RIGHT:
                rc->left = rc->right - thickness;
                break;
            default:
                rc->bottom = rc->top + thickness;
                break;
        }
    };
    APPBARDATA data{sizeof(data)};
    data.hWnd = m_hwnd;
    data.uEdge = edge;
    data.rc = m_monitor;
    fit(&data.rc);
    SHAppBarMessage(ABM_QUERYPOS, &data);
    fit(&data.rc);
    SHAppBarMessage(ABM_SETPOS, &data);
    m_appBarRect = data.rc;
    UpdateGlass();
}

// The bar reserves its strip at the top of the screen, like the taskbar
// does at the bottom; the pill keeping its room, a strip along its edge.
// Told to the system when the island starts, and again when the taskbar
// starts (a new one knows no strips; at sign-in the island may start before
// the taskbar is ready). One that knows it already says no, and its place
// is only told again.
void Island::RegisterAppBar() {
    if ((!m_bar && !g_settings.islandReserve) || m_notchBase) {
        return;
    }
    APPBARDATA data{sizeof(data)};
    data.hWnd = m_hwnd;
    data.uCallbackMessage = WM_APP_APPBAR;
    if (SHAppBarMessage(ABM_NEW, &data)) {
        m_appBar = true;
    } else if (!m_appBar) {
        return;
    }
    SetAppBarPosition(true);
}

// The work area changed: when the strip is inside it (the taskbar, still
// starting, set it without the strip, or didn't take it yet), maximized
// windows would go under the bar, so it's told again. A few times in a row
// at most, so a system that doesn't keep it can't make it loop.
void Island::CheckAppBarRoom() {
    if ((!m_bar && !g_settings.islandReserve) || m_notchBase) {
        m_appBarRetries = 0;
        return;
    }
    if (m_appBar) {
        MONITORINFO info{sizeof(info)};
        RECT overlap;
        if (m_appBarThickness <= 0 ||
            !GetMonitorInfo(
                MonitorFromRect(&m_appBarRect, MONITOR_DEFAULTTONEAREST),
                &info) ||
            !IntersectRect(&overlap, &info.rcWork, &m_appBarRect)) {
            m_appBarRetries = 0;
            return;
        }
    }
    if (m_appBarRetries < 3) {
        m_appBarRetries++;
        RegisterAppBar();
    }
}

// The automatic colors (this island's: maybe its monitor's own) turn with
// Windows' mode, switched here or elsewhere: light ones in light mode (dark
// ones for the inverted).
void Island::FollowWindowsMode() {
    const bool inverted = g_themeIndex == (int)ThemeId::AutoInverted;
    if ((g_themeIndex == (int)ThemeId::Auto || inverted) &&
        (g_themeTarget.light > 0.5f) != (inverted == IsDarkMode())) {
        ApplyThemeIndex(true);
        StartAnimating();
    }
}

void Island::UpdateAutoHide() {
    if (!m_hwnd) {
        return;
    }
    m_autoHideAwaySince = 0;
    // (Notches hiding together: the one with the bell watches them all.)
    // (The Bar hidden showing only the clock: the mouse on it and the
    // windows under it, see CheckAutoHide.)
    if ((g_settings.islandAutoHide && !m_bar &&
         !(m_notchBase && !g_settings.islandNotchEach)) ||
        (m_bar && m_barHidden && g_settings.islandHideClock)) {
        SetTimer(m_hwnd, kAutoHideTimerId, kAutoHideCheckMs, nullptr);
    } else {
        KillTimer(m_hwnd, kAutoHideTimerId);
    }
    SetAppBarPosition();
    UpdateWindowEvents();
}

// Open while the mouse is on it (or a panel, a notification or a drag is
// going on); its line a moment after the mouse leaves; open again as the
// mouse reaches the line (the mouse is only told to the island over what
// it draws: the line and its strip).
void Island::CheckAutoHide() {
    // The Bar hidden showing only the clock: back as the mouse reaches it,
    // like the line; gone while a window is under it.
    if (m_bar) {
        if (m_barHidden && g_settings.islandHideClock) {
            // (With a click instead, see islandLineClick and OnClick.)
            if (m_hover == IslandItem::Line && !g_settings.islandLineClick) {
                SetBarHidden(false);
                return;
            }
            const bool covered = m_hiddenClockCovered;
            if (HiddenClockCovered() != covered) {
                CheckShown();
            }
        }
        return;
    }
    // (Notches hiding together: the bell's one watches them all.)
    const bool together = m_notch >= 0 && !g_settings.islandNotchEach;
    if (!g_settings.islandAutoHide || m_bar ||
        !MinimizableAt(CurrentPlace()) || (together && m_notchBase)) {
        return;
    }
    const std::vector<Island*> set =
        together ? NotchSet() : std::vector<Island*>{this};
    // Only while a window is in its way (islandHideWhen): shown on the
    // desktop.
    const bool smart = SmartHideOn();
    const bool inTheWay = smart && WindowInTheWay();
    if (m_minimized) {
        // (Notches hiding together: any one's line brings them all back.)
        // (With a click instead, islandLineClick: it only grows under the
        // mouse, see OnClick.)
        for (Island* island : set) {
            if (island->m_hover == IslandItem::Line &&
                !g_settings.islandLineClick) {
                SetMinimized(false, false);
                return;
            }
        }
        // Only the clock (islandHideClock): the line while a window is
        // under it.
        if (g_settings.islandHideClock && m_autoHidden) {
            HiddenClockCovered();
        }
        if (HiddenClockWanted() != m_hiddenClockWanted) {
            m_hiddenClockWanted = HiddenClockWanted();
            StartAnimating();
        }
        // The window gone: back, when it hid by itself (the user's
        // minimizing stays).
        if (smart && m_autoHidden && !inTheWay) {
            SetMinimized(false, false);
        }
        return;
    }
    // Where its open pill is, for the windows over it while it's a line.
    m_wayRect = {m_position.x + (LONG)m_pill.left,
                 m_position.y + (LONG)m_pill.top,
                 m_position.x + (LONG)std::ceil(m_pill.right),
                 m_position.y + (LONG)std::ceil(m_pill.bottom)};
    POINT cursor;
    GetCursorPos(&cursor);
    bool used = m_bannerWanted || m_dragMoved;
    // On it: on its window, or (notches hiding together) on the edge between
    // them too, so going from one to another doesn't hide them on the way.
    RECT span{};
    for (Island* island : set) {
        const RECT window{island->m_position.x, island->m_position.y,
                          island->m_position.x + island->m_size.cx,
                          island->m_position.y + island->m_size.cy};
        UnionRect(&span, &span, &window);
        used = used || island->AnyPanelOpen();
    }
    used = used || PtInRect(&span, cursor);
    if (used || (smart && !inTheWay)) {
        m_autoHideAwaySince = 0;
        return;
    }
    const double now = NowSeconds();
    if (!m_autoHideAwaySince) {
        m_autoHideAwaySince = now;
    } else if (now - m_autoHideAwaySince >=
               g_settings.islandAutoHideDelay / 10.0) {
        m_autoHideAwaySince = 0;
        SetMinimized(true, false);
    }
}

// Only the clock when hidden (islandHideClock): hidden by the auto-hide
// (the user's minimizing shows the line), the Island or the Fixed style
// along the top or bottom edge, without a window under it. The Bar, hidden
// by the minimize shortcut, closes into it (a window under it: it goes
// altogether, see CheckShown).
bool Island::HiddenClockWanted() const {
    return HiddenTimeWanted() || TimerLineWanted();
}

bool Island::HiddenTimeWanted() const {
    if (m_bar) {
        return g_settings.islandHideClock && m_barHidden;
    }
    return g_settings.islandHideClock && g_settings.islandAutoHide &&
           m_minimized && m_autoHidden && !m_bar && m_notch < 0 &&
           !m_vertical && !m_hiddenClockCovered;
}

// A timer, Pomodoro or the stopwatch going while minimized (islandTimerLine,
// Discord): the line becomes the small pill with its ring and time, however
// it was minimized and with windows under it too; the hidden Bar's clock
// gets it beside the time. Not on the notches nor upright: their line has
// no room for it.
bool Island::TimerLineWanted() const {
    if (!g_settings.islandTimerLine || !g_settings.islandTimer ||
        m_vertical || !AnyTimerActive()) {
        return false;
    }
    if (m_bar) {
        return g_settings.islandHideClock && m_barHidden;
    }
    return m_minimized && m_notch < 0;
}

// What the small pill says: the time, the timer's (see TimerLineWanted), or
// both; the time as it goes away.
std::wstring Island::HiddenPillText() const {
    const bool time = HiddenTimeWanted();
    std::wstring timer;
    if (TimerLineWanted()) {
        const std::vector<TimerMode> timers = PillTimers();
        if (!timers.empty()) {
            timer = TimerText(timers[0]);
        }
    }
    if (timer.empty()) {
        return m_hiddenClockText;
    }
    return time ? m_hiddenClockText + L"   " + timer : timer;
}

// The small pill's text in the middle of `rect`, the timer's ring before
// it when one shows (orange, like the timer panel's).
void Island::DrawHiddenPillContents(ID2D1RenderTarget* target,
                                    ID2D1SolidColorBrush* brush,
                                    const D2D1_RECT_F& rect,
                                    float opacity,
                                    float scale) {
    if (!m_hiddenClockLayout) {
        return;
    }
    DWRITE_TEXT_METRICS metrics{};
    m_hiddenClockLayout->GetMetrics(&metrics);
    const std::vector<TimerMode> timers =
        TimerLineWanted() ? PillTimers() : std::vector<TimerMode>{};
    const float ring = timers.empty() ? 0 : HiddenRingSize(scale);
    const float gap = timers.empty() ? 0 : std::round(6 * scale);
    const float left =
        std::round((rect.left + rect.right - metrics.width - ring - gap) / 2);
    const float middleY = (rect.top + rect.bottom) / 2;
    if (!timers.empty()) {
        const TimerMode mode = timers[0];
        const D2D1_POINT_2F center{left + ring / 2, middleY};
        const float radius = ring / 2 - scale;
        const float stroke = std::max(1.5f, 2 * scale);
        brush->SetColor(Fg(0.22f * opacity));
        target->DrawEllipse({center, radius, radius}, brush, stroke);
        const int length = TimerLength(mode);
        const float share =
            mode == TimerMode::Stopwatch || length <= 0
                ? 1.0f
                : (float)std::clamp(TimerLeft(mode) / length, 0.0, 1.0);
        brush->SetColor(D2D1::ColorF(1.0f, 0.62f, 0.04f, opacity));
        winrt::com_ptr<ID2D1Factory> factory;
        target->GetFactory(factory.put());
        winrt::com_ptr<ID2D1PathGeometry> arc;
        winrt::com_ptr<ID2D1GeometrySink> sink;
        if (share >= 0.999f) {
            target->DrawEllipse({center, radius, radius}, brush, stroke);
        } else if (share > 0.001f && factory &&
                   SUCCEEDED(factory->CreatePathGeometry(arc.put())) &&
                   SUCCEEDED(arc->Open(sink.put()))) {
            // From the top, clockwise, as much as is left.
            const float angle = share * 2 * 3.14159265f;
            sink->BeginFigure({center.x, center.y - radius},
                              D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddArc({{center.x + radius * std::sin(angle),
                           center.y - radius * std::cos(angle)},
                          {radius, radius},
                          0,
                          D2D1_SWEEP_DIRECTION_CLOCKWISE,
                          share > 0.5f ? D2D1_ARC_SIZE_LARGE
                                       : D2D1_ARC_SIZE_SMALL});
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
            if (SUCCEEDED(sink->Close())) {
                target->DrawGeometry(arc.get(), brush, stroke);
            }
        }
    }
    brush->SetColor(Fg(opacity));
    target->DrawTextLayout(
        {left + ring + gap - metrics.left,
         std::round((rect.top + rect.bottom - metrics.height) / 2 -
                    metrics.top)},
        m_hiddenClockLayout.get(), brush);
}

// Its small pill, in the window: the pill's height, as wide as the time and
// rounded ends, in the pill's middle.
D2D1_RECT_F Island::HiddenClockRect(float scale) {
    const float height = m_pill.bottom - m_pill.top;
    // (Made again for another text, text size or scale.)
    const std::wstring text = HiddenPillText();
    if (!m_hiddenClockLayout || m_hiddenClockLaidOut != text ||
        m_hiddenClockFormat != m_textFormat.get()) {
        m_hiddenClockLayout = nullptr;
        m_hiddenClockLaidOut = text;
        m_hiddenClockFormat = m_textFormat.get();
        if (m_dwrite && m_textFormat) {
            m_dwrite->CreateTextLayout(text.c_str(), (UINT32)text.size(),
                                       m_textFormat.get(), 1000, height,
                                       m_hiddenClockLayout.put());
        }
    }
    DWRITE_TEXT_METRICS metrics{};
    if (m_hiddenClockLayout) {
        m_hiddenClockLayout->GetMetrics(&metrics);
    }
    // (The Bar upright: as wide as the bar, in the middle of its side.)
    if (m_vertical) {
        const float along =
            std::max(std::ceil(metrics.height) + 16 * scale,
                     (m_pill.right - m_pill.left) * 0.75f);
        const float centerY = (m_pill.top + m_pill.bottom) / 2;
        return {m_pill.left, centerY - along / 2, m_pill.right,
                centerY + along / 2};
    }
    // (A timer's ring before the text, see DrawHiddenPillContents.)
    const float ring = TimerLineWanted()
                           ? HiddenRingSize(scale) + std::round(6 * scale)
                           : 0;
    const float width =
        std::min(std::ceil(metrics.widthIncludingTrailingWhitespace) + ring +
                     std::max(height, 24 * scale),
                 m_pill.right - m_pill.left);
    const float centerX = (m_pill.left + m_pill.right) / 2;
    return {centerX - width / 2, m_pill.top, centerX + width / 2,
            m_pill.bottom};
}

// An app's window under the small pill: looked at again only after Windows'
// events, like WindowInTheWay.
bool Island::HiddenClockCovered() {
    WindowInTheWaySearch search{CurrentMonitor(), m_hiddenClockScreen, false,
                                GetCurrentProcessId(), false};
    if (WayLookStale(m_hiddenClockLook, search)) {
        if (!IsRectEmpty(&search.place)) {
            EnumWindows(WindowInTheWayProc, reinterpret_cast<LPARAM>(&search));
        }
        m_hiddenClockCovered = search.found;
    }
    return m_hiddenClockCovered;
}

// Hiding only for a window in the way (islandHideWhen): the Island style
// (the Fixed one and the bar keep their room: nothing goes under them).
bool Island::SmartHideOn() const {
    return g_settings.islandHideWhen > 0 && !m_bar && m_notch < 0 &&
           !g_settings.islandReserve;
}

// An app's window maximized on its monitor (islandHideWhen 1), or over the
// place its open pill takes (2, maximized ones too). Not the island's own
// windows, the desktop and the taskbars, minimized ones nor ones on another
// virtual desktop (cloaked). Looked at again only after Windows told a
// window changed (see UpdateWindowEvents), or for another place.
bool Island::WindowInTheWay() {
    WindowInTheWaySearch search{CurrentMonitor(), m_wayRect,
                                g_settings.islandHideWhen == 1,
                                GetCurrentProcessId(), false};
    if (IsRectEmpty(&search.place)) {
        search.place = {m_position.x, m_position.y, m_position.x + m_size.cx,
                        m_position.y + m_size.cy};
    }
    if (WayLookStale(m_wayLook, search)) {
        EnumWindows(WindowInTheWayProc, reinterpret_cast<LPARAM>(&search));
        m_windowInTheWay = search.found;
    }
    return m_windowInTheWay;
}

// Windows' events about the apps' windows (see kWindowEventRanges), watched
// by the main island while an island hides for a window in its way or shows
// only the clock (the island's own windows left out: they move as it
// animates).
void Island::UpdateWindowEvents() {
    if (!m_hwnd) {
        return;
    }
    if (Island* main = MainIsland(); main != this) {
        if (main) {
            IslandScope scope(main);
            main->UpdateWindowEvents();
        }
        return;
    }
    if (!AnyIslandLook([](Island& island) {
            return island.m_bar
                       ? island.m_barHidden && g_settings.islandHideClock
                       : g_settings.islandAutoHide &&
                             (g_settings.islandHideWhen > 0 ||
                              g_settings.islandHideClock);
        })) {
        StopWindowEvents();
        return;
    }
    if (std::any_of(std::begin(m_windowEventHooks),
                    std::end(m_windowEventHooks),
                    [](HWINEVENTHOOK hook) { return hook != nullptr; })) {
        return;
    }
    for (size_t i = 0; i < std::size(kWindowEventRanges); i++) {
        m_windowEventHooks[i] = SetWinEventHook(
            kWindowEventRanges[i].first, kWindowEventRanges[i].second, nullptr,
            WindowEventsProc, 0, 0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
        if (!m_windowEventHooks[i]) {
            IslandLog(L"SetWinEventHook failed: %u", GetLastError());
        }
    }
    // (What was seen before it watched, looked at again.)
    g_windowEvents++;
}

void Island::StopWindowEvents() {
    for (HWINEVENTHOOK& hook : m_windowEventHooks) {
        if (hook) {
            UnhookWinEvent(hook);
            hook = nullptr;
        }
    }
}

// Like on macOS: "sex 3 out 23:59:12", with seconds when the taskbar clock
// shows them.
// "sáb 3 out 2026  20:01:50": the parts chosen in the island's settings.
void Island::UpdateClock() {
    SYSTEMTIME now;
    GetLocalTime(&now);

    std::wstring format;
    auto add = [&](PCWSTR part) {
        if (!format.empty()) {
            format += L" ";
        }
        format += part;
    };
    // The app in front's own clock, or the usual one.
    const IslandPlace* place = FindAppPlace(m_placeApp);
    const int parts =
        place && place->clock >= 0 ? place->clock : GlobalClockParts();
    // Chinese, Japanese and Korean dates go from the year to the day
    // ("2026年10月9日"), the day of the week after them; or in the order
    // chosen. (In the island's language, when one is chosen.)
    const LANGID language =
        PRIMARYLANGID(LANGIDFROMLCID(LocaleNameToLCID(DateLocale(), 0)));
    const int order = g_settings.clockDateOrder;
    const bool eastAsian = language == LANG_KOREAN ||
                           language == LANG_CHINESE ||
                           language == LANG_JAPANESE;
    const bool number = g_settings.clockMonthNumber;
    const bool marked = !number && eastAsian && (order == 0 || order == 3);
    const bool korean = marked && language == LANG_KOREAN;
    const bool yearFirst = order == 3 || (order == 0 && eastAsian);
    // Other languages: the day and month as the Windows regional format
    // writes them ("MMMM d", "d 'de' MMMM"), short ("Oct 9", "9 out"):
    // without its words, with the month's short name.
    std::wstring regional = order >= 2 ? L"MMM d" : L"d MMM";
    if (order == 0 && !eastAsian) {
        WCHAR pattern[64];
        if (GetLocaleInfoEx(DateLocale(), LOCALE_SMONTHDAY, pattern,
                            ARRAYSIZE(pattern))) {
            std::wstring kept;
            bool quoted = false;
            for (PCWSTR c = pattern; *c; c++) {
                if (*c == L'\'') {
                    quoted = !quoted;
                } else if (!quoted &&
                           (*c != L' ' ||
                            (!kept.empty() && kept.back() != L' '))) {
                    kept += *c;
                }
            }
            while (!kept.empty() && kept.back() == L' ') {
                kept.pop_back();
            }
            if (const size_t month = kept.find(L"MMMM");
                month != std::wstring::npos) {
                kept.erase(month, 1);
            }
            if (kept.find(L'd') != std::wstring::npos &&
                kept.find(L'M') != std::wstring::npos) {
                regional = kept;
            }
        }
    }
    // In numbers: "9/10", "10/9", "2026/10/9".
    if (number) {
        regional = !yearFirst && regional.find(L'd') < regional.find(L'M')
                       ? L"d/M"
                       : L"M/d";
    }
    PCWSTR monthDay = korean   ? L"M'\uC6D4' d'\uC77C'"
                      : marked ? L"M'\u6708'd'\u65E5'"
                               : regional.c_str();
    if (yearFirst) {
        if (parts & 8) {
            format = korean   ? L"yyyy'\uB144'"
                     : marked ? L"yyyy'\u5E74'"
                              : L"yyyy";
        }
        if (parts & 4) {
            format += (!marked || korean) && !format.empty()
                          ? (number ? L"/" : L" ")
                          : L"";
            format += monthDay;
        }
        if (parts & 2) {
            add(L"ddd");
        }
    } else {
        if (parts & 2) {
            add(L"ddd");
        }
        if (parts & 4) {
            add(monthDay);
        }
        if ((parts & 8) && number && (parts & 4)) {
            format += L"/yyyy";
        } else if (parts & 8) {
            add(L"yyyy");
        }
    }
    WCHAR date[64] = L"";
    if (!format.empty()) {
        GetDateFormatEx(DateLocale(), 0, &now, format.c_str(), date,
                        ARRAYSIZE(date), nullptr);
    }
    // (With 12 hours, AM or PM after the time, drawn smaller: see
    // SmallClockMarker. The time only while it's on: the date alone
    // otherwise.)
    std::wstring marker;
    std::wstring time;
    if (g_settings.islandClock) {
        time = FormatClockTime(now, parts & 1, &marker);
        if (!marker.empty()) {
            time += L" " + marker;
        }
    }
    const std::wstring before = m_clock;
    m_clock = date[0] && !time.empty() ? std::wstring(date) + L"  " + time
              : date[0]                ? std::wstring(date)
                                       : time;
    // Other parts (chosen, or the app in front's own): they fade over.
    if (m_clockParts >= 0 && parts != m_clockParts && !before.empty()) {
        m_clockBefore = before;
        m_clockSwap = 1;
        m_clockSwapVelocity = 0;
        StartAnimating();
    }
    m_clockParts = parts;
    // Upright: the time without seconds, and the day and month under it
    // (after AM or PM, with 12 hours: the pill is narrow).
    std::wstring shortMarker;
    m_clockTime = g_settings.islandClock
                      ? FormatClockTime(now, false, &shortMarker)
                      : std::wstring();
    {
        std::wstring hiddenMarker;
        m_hiddenClockText = FormatClockTime(now, false, &hiddenMarker);
        // (Not on the upright Bar: it's narrow.)
        if (!hiddenMarker.empty() && !m_vertical) {
            m_hiddenClockText += L" " + hiddenMarker;
        }
    }
    WCHAR shortDate[32] = L"";
    GetDateFormatEx(DateLocale(), 0, &now, monthDay, shortDate,
                    ARRAYSIZE(shortDate), nullptr);
    m_clockDate =
        shortMarker.empty() ? shortDate : shortMarker + L" " + shortDate;
}

// The default device can change (headphones), so it's looked up each time.
winrt::com_ptr<IAudioEndpointVolume> Island::GetVolume(
    std::wstring* deviceName) {
    winrt::com_ptr<IMMDevice> device;
    winrt::com_ptr<IAudioEndpointVolume> volume;
    if (!m_devices ||
        FAILED(m_devices->GetDefaultAudioEndpoint(eRender, eMultimedia,
                                                  device.put())) ||
        FAILED(device->Activate(kIID_IAudioEndpointVolume, CLSCTX_ALL, nullptr,
                                volume.put_void()))) {
        return nullptr;
    }
    if (deviceName) {
        winrt::com_ptr<IPropertyStore> properties;
        if (SUCCEEDED(device->OpenPropertyStore(STGM_READ,
                                                properties.put()))) {
            PROPVARIANT value;
            PropVariantInit(&value);
            if (SUCCEEDED(properties->GetValue(kPKEY_Device_FriendlyName,
                                               &value)) &&
                value.vt == VT_LPWSTR && value.pwszVal) {
                *deviceName = value.pwszVal;
            }
            PropVariantClear(&value);
        }
    }
    return volume;
}

std::vector<winrt::com_ptr<IAudioEndpointVolume>> Island::GetMicrophones() {
    std::vector<winrt::com_ptr<IAudioEndpointVolume>> volumes;
    std::vector<std::wstring> ids;
    for (ERole role : {eConsole, eCommunications}) {
        winrt::com_ptr<IMMDevice> device;
        winrt::com_ptr<IAudioEndpointVolume> volume;
        LPWSTR id = nullptr;
        if (!m_devices ||
            FAILED(m_devices->GetDefaultAudioEndpoint(eCapture, role,
                                                      device.put())) ||
            FAILED(device->GetId(&id))) {
            continue;
        }
        const std::wstring deviceId = id;
        CoTaskMemFree(id);
        if (std::find(ids.begin(), ids.end(), deviceId) != ids.end() ||
            FAILED(device->Activate(kIID_IAudioEndpointVolume, CLSCTX_ALL,
                                    nullptr, volume.put_void()))) {
            continue;
        }
        ids.push_back(deviceId);
        volumes.push_back(volume);
    }
    return volumes;
}

int Island::MicrophoneMuted() {
    const auto microphones = GetMicrophones();
    BOOL muted = FALSE;
    if (microphones.empty() || FAILED(microphones[0]->GetMute(&muted))) {
        return -1;
    }
    return muted ? 1 : 0;
}

// The microphones the island muted (see ToggleMicrophone), each with the
// level it had (-1: not known, only its sound comes back): a passing state,
// not in the backup, kept as "level\tid" lines in "microphoneLevels". Read
// once; the island's thread or, once it's stopped, the unloading one.
std::vector<std::pair<std::wstring, int>> g_microphoneLevels;
bool g_microphoneLevelsRead = false;

std::vector<std::pair<std::wstring, int>>& MicrophoneLevels() {
    if (!g_microphoneLevelsRead) {
        g_microphoneLevelsRead = true;
        const std::wstring value =
            GetWholeStringValue(L"microphoneLevels", 4096);
        for (PCWSTR line = value.c_str(); *line;) {
            PCWSTR end = wcschr(line, L'\n');
            const std::wstring text =
                end ? std::wstring(line, end) : std::wstring(line);
            if (const size_t tab = text.find(L'\t');
                tab != std::wstring::npos) {
                g_microphoneLevels.push_back(
                    {text.substr(tab + 1), _wtoi(text.c_str())});
            }
            if (!end) {
                break;
            }
            line = end + 1;
        }
    }
    return g_microphoneLevels;
}

void SaveMicrophoneLevels() {
    std::wstring value;
    for (const auto& [id, level] : g_microphoneLevels) {
        value += std::to_wstring(level) + L"\t" + id + L"\n";
    }
    SetStoredString(L"microphoneLevels", value.c_str());
}

// The microphones the island muted get their level back, when it's still
// at 0 (the user may have chosen another since, the island running or
// not): all of them (`all`: the island's button, the island stopping, or a
// level left at 0 by a process that ended without stopping it), or only the
// ones unmuted elsewhere meanwhile (Windows, another app, a key). Only the
// island's own button (`unmute`) unmutes them too: stopping leaves the
// user's mute as it is (a plain Windows mute from then on) and keeps the
// ones still muted listed, so that button still unmutes them later; one
// unmuted elsewhere leaves the list. Unplugged ones are kept for when they
// come back.
void RestoreMicrophoneLevels(IMMDeviceEnumerator* enumerator,
                             bool all,
                             bool unmute) {
    auto& levels = MicrophoneLevels();
    winrt::com_ptr<IMMDeviceCollection> devices;
    UINT count = 0;
    if (levels.empty() || !enumerator ||
        FAILED(enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE,
                                              devices.put())) ||
        FAILED(devices->GetCount(&count))) {
        return;
    }
    bool changed = false;
    for (UINT i = 0; i < count; i++) {
        winrt::com_ptr<IMMDevice> device;
        winrt::com_ptr<IAudioEndpointVolume> volume;
        LPWSTR id = nullptr;
        if (FAILED(devices->Item(i, device.put())) ||
            FAILED(device->Activate(kIID_IAudioEndpointVolume, CLSCTX_ALL,
                                    nullptr, volume.put_void())) ||
            FAILED(device->GetId(&id))) {
            continue;
        }
        const std::wstring deviceId = id;
        CoTaskMemFree(id);
        auto it = std::find_if(levels.begin(), levels.end(),
                               [&](const auto& entry) {
                                   return entry.first == deviceId;
                               });
        BOOL muted = FALSE;
        const bool mutedNow =
            it != levels.end() && SUCCEEDED(volume->GetMute(&muted)) && muted;
        if (it == levels.end() || (!all && mutedNow)) {
            continue;
        }
        float level = 0;
        if (it->second >= 0 &&
            SUCCEEDED(volume->GetMasterVolumeLevelScalar(&level)) &&
            level < 0.0005f) {
            volume->SetMasterVolumeLevelScalar(
                std::clamp(it->second, 0, 1000) / 1000.0f, nullptr);
        }
        if (unmute) {
            volume->SetMute(FALSE, nullptr);
        }
        if (unmute || !mutedNow) {
            levels.erase(it);
            changed = true;
        }
    }
    if (changed) {
        SaveMicrophoneLevels();
    }
}

DWORD WINAPI RestoreMicrophonesThreadProc(LPVOID) {
    const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    winrt::com_ptr<IMMDeviceEnumerator> enumerator;
    if (SUCCEEDED(CoCreateInstance(kCLSID_MMDeviceEnumerator, nullptr,
                                   CLSCTX_ALL, kIID_IMMDeviceEnumerator,
                                   enumerator.put_void()))) {
        RestoreMicrophoneLevels(enumerator.get(), true, false);
    }
    enumerator = nullptr;
    if (SUCCEEDED(result)) {
        CoUninitialize();
    }
    return 0;
}

// The island stopping (the mod turned off, or the island alone), or the
// process starting after one that ended without it (a crash, signing out):
// the microphones it muted get their level back and stay muted (on a thread
// with COM of its own: the caller's may have none).
void RestoreMicrophonesNow() {
    if (MicrophoneLevels().empty()) {
        return;
    }
    if (HANDLE thread = CreateThread(nullptr, 0, RestoreMicrophonesThreadProc,
                                     nullptr, 0, nullptr)) {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
}

