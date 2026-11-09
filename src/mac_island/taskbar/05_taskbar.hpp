#pragma once
// The part of a taskbar it shows: on its own monitor, and before it moves to
// `next`, on it both where it is and where it goes (cut after the move, a
// frame would show it past the edge in between).
RECT TaskbarShownPart(HWND hWnd, const RECT* next) {
    RECT own = TaskbarOwnPart(hWnd);
    if (next) {
        const RECT there = TaskbarOwnPart(hWnd, next);
        if (!IntersectRect(&own, &own, &there)) {
            own = {};
        }
    }
    return own;
}

// A taskbar (not the transparent one, cut down to its apps) shows only its
// part on its own monitor while some of it is past its edge (hidden, or
// sliding: with a monitor past that edge, the rest would show there), and
// its whole window again once it's all on it. `next`: before it moves there.
void ClipToOwnMonitor(HWND hWnd, const RECT* next = nullptr) {
    RECT window;
    if (!GetWindowRect(hWnd, &window)) {
        return;
    }
    const RECT own = TaskbarShownPart(hWnd, next);
    const bool whole = own.left <= 0 && own.top <= 0 &&
                       own.right >= window.right - window.left &&
                       own.bottom >= window.bottom - window.top;
    const bool ours = std::find(g_ownClips.begin(), g_ownClips.end(), hWnd) !=
                      g_ownClips.end();
    if (whole) {
        // (Before a move, its cut stays until after it.)
        if (!next && ours) {
            std::erase(g_ownClips, hWnd);
            SetWindowRgn(hWnd, nullptr, RedrawsWithCut(hWnd));
        }
        return;
    }
    RECT current{};
    const int kind = GetWindowRgnBox(hWnd, &current);
    // (Unloading: none set again, see ClipDockWindow.)
    if (g_unloading ||
        (ours && kind == SIMPLEREGION && EqualRect(&current, &own))) {
        return;
    }
    if (HRGN region = CreateRectRgnIndirect(&own)) {
        // The system owns the region once it's set.
        if (!SetWindowRgn(hWnd, region, RedrawsWithCut(hWnd))) {
            DeleteObject(region);
        } else if (!ours) {
            g_ownClips.push_back(hWnd);
        }
    }
}

// The monitor a taskbar at `rect` belongs to (the row of it nearest the
// screen's inside, see TaskbarMonitor), and its scale: the dock's room is
// measured with it, the same whether the taskbar is shown or hidden (hidden,
// it's mostly over the next monitor, maybe with another scale, which the
// window takes then).
HMONITOR TaskbarMonitorAt(const RECT& rect) {
    return MonitorFromPoint({(rect.left + rect.right) / 2, rect.top},
                            MONITOR_DEFAULTTONEAREST);
}

double TaskbarScaleAt(const RECT& rect) {
    UINT dpiX = 96;
    UINT dpiY = 96;
    if (FAILED(GetDpiForMonitor(TaskbarMonitorAt(rect), MDT_EFFECTIVE_DPI,
                                &dpiX, &dpiY))) {
        dpiX = 96;
    }
    return dpiX / 96.0;
}

// The room of a grown taskbar, in pixels.
int GrownExtra(HWND hWnd, const GrownTaskbar& grown) {
    RECT now{};
    GetWindowRect(hWnd, &now);
    return (int)std::lround(grown.extraDips * TaskbarScaleAt(now));
}

// How close to the bottom edge the mouse brings the taskbar up (DIPs; see
// EdgeThreadProc): the height chosen (hiding.revealHeight), or as high as
// the dock's icons over its gap at the bottom (with a monitor below, the
// mouse easily goes past the last row), a few pixels for the transparent
// taskbar.
double EdgeShowDips() {
    if (g_settings.revealHeight > 0) {
        return g_settings.revealHeight;
    }
    return IsDock() ? g_settings.dockIconSize + g_settings.bottomGap : 8;
}

// The rows over the bottom edge of `monitor` (`bottom`) at `x` that bring
// the taskbar up, in pixels: the height chosen on every monitor, or
// automatically the whole strip (see EdgeShowDips) only with a monitor right
// below, where the mouse goes past the last row; otherwise the mouse stops
// at the edge and only its last rows count, like Windows' own (the dock came
// up as the mouse got near the bottom, #6472).
LONG EdgeStripPixels(LONG x, LONG bottom, UINT dpi) {
    const double dips =
        g_settings.revealHeight > 0 ||
                MonitorFromPoint({x, bottom}, MONITOR_DEFAULTTONULL)
            ? EdgeShowDips()
            : 2;
    return (LONG)std::lround(dips * dpi / 96);
}

// The mouse came down past the bottom of a monitor with another one right
// below it, under the dock's icons (or the transparent taskbar's apps): the
// first rows of the monitor below count as that edge as long as it stays in
// them (the mouse easily goes past the last row), shown and kept up (see
// EdgeThreadProc, TrayUI__ShouldAutoHideTray_Hook). Only coming from above:
// the top of the monitor below is used as it is otherwise. Set by the edge's
// thread.
std::atomic<HMONITOR> g_edgeCapture;

// The monitor right above `pt` whose bottom edge it counts for (see
// g_edgeCapture), or nullptr: `pt` in the first rows of the monitor `below`
// (as many as the strip over the edge, see EdgeShowDips), under the dock's
// icons or the apps there.
HMONITOR EdgeCaptureAbove(POINT pt, const RECT& below) {
    const HMONITOR above =
        MonitorFromPoint({pt.x, below.top - 1}, MONITOR_DEFAULTTONULL);
    MONITORINFO info{sizeof(info)};
    UINT dpiX = 96;
    UINT dpiY = 96;
    if (!above || !GetMonitorInfo(above, &info) ||
        info.rcMonitor.bottom != below.top ||
        FAILED(GetDpiForMonitor(above, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) ||
        pt.y >= below.top + (LONG)std::lround(EdgeShowDips() * dpiX / 96)) {
        return nullptr;
    }
    int appsLeft = 0;
    int appsRight = 0;
    if ((IsTransparentTaskbar() || IsDock()) &&
        GetAppsArea(above, &appsLeft, &appsRight) &&
        (pt.x < appsLeft || pt.x >= appsRight)) {
        return nullptr;
    }
    return above;
}

// Whether the mouse is in the rows past `taskbar`'s bottom edge that count
// for it now (see g_edgeCapture).
bool EdgeCaptureHolds(HWND taskbar) {
    const HMONITOR captured = g_edgeCapture;
    POINT pt{};
    MONITORINFO info{sizeof(info)};
    return captured && taskbar && TaskbarMonitor(taskbar) == captured &&
           GetCursorPos(&pt) &&
           GetMonitorInfo(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST),
                          &info) &&
           EdgeCaptureAbove(pt, info.rcMonitor) == captured;
}

// Whether another monitor is right below `monitor` at `x`.
bool MonitorBelow(const RECT& monitor, LONG x) {
    return MonitorFromPoint({x, monitor.bottom}, MONITOR_DEFAULTTONULL) !=
           nullptr;
}

// How much higher the hidden dock's window (`base` with the room `extra`
// below) is than Windows puts it, in pixels (the rows above its last 2 on
// the screen are cut away, see ClipDockWindow): Windows shows a hidden
// taskbar only with the mouse on its window. With a monitor below, more than
// half of it stays on its own monitor: it keeps that monitor's scale, or it
// would change scale on its way up (its content hidden meanwhile, see
// g_dockContentHidden) whenever the one below has another.
int HiddenLift(const RECT& base, int extra) {
    const HMONITOR monitor = TaskbarMonitorAt(base);
    UINT dpiX = 96;
    UINT dpiY = 96;
    if (FAILED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
        dpiX = 96;
    }
    int lift = (int)std::lround((EdgeShowDips() - 2) * dpiX / 96);
    MONITORINFO info{sizeof(info)};
    if (GetMonitorInfo(monitor, &info) &&
        MonitorBelow(info.rcMonitor, (base.left + base.right) / 2)) {
        lift = std::max(lift, (int)(base.bottom - base.top + extra) / 2 +
                                  MulDiv(4, dpiX, 96));
    }
    return std::max(0, lift);
}

// The room of a dock's window that hides over a monitor below, in DIPs, at
// least (`baseDips`: the taskbar's own height): hidden, more than half of its
// window stays on its own monitor (see HiddenLift), and only with this much
// room above the dock is all of the dock past the edge there, so it slides
// away past the edge like the docks with no monitor below (with only the room
// for its magnified icons, part of it stayed above the edge and was cut away
// on its way down: it went into a line above the edge).
double RoomOverMonitorBelowDips(double baseDips) {
    const DockMetrics m = GetDockMetrics(0);
    // The dock at rest, with its apps' badges sticking out of it.
    const double rest = m.bottomGap + m.Height() + 8;
    // (Past the lift, half the window and a few pixels, or the rows the mouse
    // brings it up from, and the strip the taskbar keeps on the screen.)
    return std::max(2 * rest + 12, rest + EdgeShowDips() + 6) - baseDips;
}

// The taskbar's window with the dock's room (see GrownTaskbar), from where
// the taskbar puts itself: the room up while all of it is on its monitor
// (shown), down otherwise (hidden, past the screen's edge with the rest, a
// little higher: see HiddenLift).
RECT GrownRect(const RECT& base, int extra, bool* up) {
    MONITORINFO info{sizeof(info)};
    const HMONITOR monitor = TaskbarMonitorAt(base);
    *up = GetMonitorInfo(monitor, &info) && base.bottom <= info.rcMonitor.bottom;
    RECT grown = base;
    if (*up) {
        grown.top -= extra;
    } else {
        grown.bottom += extra;
        OffsetRect(&grown, 0, -HiddenLift(base, extra));
    }
    return grown;
}

// The taskbar placing itself (after its own handling of
// WM_WINDOWPOSCHANGING): with the dock's room added. A move with the
// window's size as it is (the room in it) is the taskbar's own height there.
// A size with the room already in it (the mod's own moves, or Windows' for a
// new scaling) is left as it is.
void GrowTaskbarWindow(HWND hWnd, WINDOWPOS* pos) {
    RECT now;
    if (!GetWindowRect(hWnd, &now)) {
        return;
    }
    if ((pos->flags & (SWP_NOMOVE | SWP_NOSIZE)) == (SWP_NOMOVE | SWP_NOSIZE)) {
        // Not grown yet (see RegrowTaskbarProc): the main taskbar's own
        // handler turns a move it didn't ask for into none at all (measured:
        // after the mod loaded, only the other monitors' taskbars grew).
        if (FindGrownTaskbar(hWnd)) {
            return;
        }
        pos->x = now.left;
        pos->y = now.top;
        pos->cx = now.right - now.left;
        pos->cy = now.bottom - now.top;
        pos->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
    }
    // (Upright or almost square: on its way to the left or right edge, see
    // below.)
    if (!(pos->flags & SWP_NOSIZE) && pos->cy * 2 > pos->cx) {
        // Its thickness: its width upright, or its own height (without the
        // room) at the bottom.
        LONG thickness = now.bottom - now.top;
        if (now.right - now.left < now.bottom - now.top) {
            thickness = now.right - now.left;
        } else if (const GrownTaskbar* grown = FindGrownTaskbar(hWnd)) {
            thickness -= GrownExtra(hWnd, *grown);
        }
        std::erase_if(g_grownTaskbars, [hWnd](const GrownTaskbar& taskbar) {
            return taskbar.wnd == hWnd;
        });
        // Never wider than a third of its monitor, nor higher than it: until
        // the dock's look goes (see WatchTaskbarPlace), Windows makes the
        // main taskbar as big as the screen on its way to a side (1920 by
        // 1920), keeps that size for the bottom edge too, and everything
        // freezes.
        MONITORINFO info{sizeof(info)};
        if (thickness > 0 &&
            GetMonitorInfo(TaskbarMonitor(hWnd), &info)) {
            const RECT& monitor = info.rcMonitor;
            if (pos->cx > (monitor.right - monitor.left) / 3) {
                // (On the right: against the monitor's right edge.)
                if (!(pos->flags & SWP_NOMOVE) && pos->x > monitor.left) {
                    pos->x = monitor.right - thickness;
                }
                pos->cx = thickness;
            }
            if (pos->cy > monitor.bottom - monitor.top) {
                if (!(pos->flags & SWP_NOMOVE)) {
                    pos->y = monitor.top;
                }
                pos->cy = monitor.bottom - monitor.top;
            }
        }
        return;
    }
    const double scale = TaskbarScaleAt(now);
    GrownTaskbar* grown = FindGrownTaskbar(hWnd);
    const int extraNow = grown ? GrownExtra(hWnd, *grown) : 0;
    RECT base;
    if (!(pos->flags & SWP_NOSIZE)) {
        if (grown && pos->cy / scale > grown->baseDips + grown->extraDips / 2) {
            return;
        }
        base.left = (pos->flags & SWP_NOMOVE) ? now.left : pos->x;
        base.top = (pos->flags & SWP_NOMOVE)
                       ? now.top + (!grown       ? 0
                                    : grown->up ? extraNow
                                                : grown->lift)
                       : pos->y;
        base.right = base.left + pos->cx;
        base.bottom = base.top + pos->cy;
    } else {
        const int height = now.bottom - now.top - extraNow;
        base = {pos->x, pos->y, pos->x + (now.right - now.left),
                pos->y + height};
    }
    // Moved to another edge (the dock goes away once that's noticed, see
    // WatchTaskbarPlace, which needs it all on its monitor): no room, and the
    // size the taskbar chose for that edge left as it is (`base` is measured
    // the bottom's way).
    // (Nor a shape that isn't a bar: on its way to a side, Windows sizes it
    // almost square for a moment.)
    MONITORINFO info{sizeof(info)};
    if ((base.bottom - base.top) * 2 > base.right - base.left ||
        (GetMonitorInfo(TaskbarMonitorAt(base), &info) &&
         RectSide(base, info.rcMonitor) != ABE_BOTTOM)) {
        std::erase_if(g_grownTaskbars, [hWnd](const GrownTaskbar& taskbar) {
            return taskbar.wnd == hWnd;
        });
        return;
    }
    const double baseDips = (base.bottom - base.top) / scale;
    double extraDips = DockRoomDips(baseDips);
    if (extraDips <= 0) {
        std::erase_if(g_grownTaskbars, [hWnd](const GrownTaskbar& taskbar) {
            return taskbar.wnd == hWnd;
        });
        if (pos->flags & SWP_NOSIZE) {
            pos->cy = base.bottom - base.top;
            pos->cx = base.right - base.left;
            pos->flags &= ~SWP_NOSIZE;
        }
        return;
    }
    if (g_taskbarAutoHides && GetMonitorInfo(TaskbarMonitorAt(base), &info) &&
        MonitorBelow(info.rcMonitor, (base.left + base.right) / 2)) {
        extraDips = std::max(extraDips, RoomOverMonitorBelowDips(baseDips));
    }
    bool up = false;
    const RECT target =
        GrownRect(base, (int)std::lround(extraDips * scale), &up);
    const int lift = up ? 0 : base.top - target.top;
    if (!grown) {
        g_grownTaskbars.push_back({hWnd, baseDips, extraDips, up, lift});
    } else {
        *grown = {hWnd, baseDips, extraDips, up, lift};
    }
    pos->x = target.left;
    pos->y = target.top;
    pos->cx = target.right - target.left;
    pos->cy = target.bottom - target.top;
    pos->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
}

// The taskbars' room made again for the settings now (or taken away: another
// taskbar mode, or the mod unloading): each window is put back at its own
// height, and grows again through GrowTaskbarWindow, like the taskbar's own
// moves.
BOOL CALLBACK RegrowTaskbarProc(HWND hWnd, LPARAM) {
    WCHAR className[32];
    RECT now;
    if (!GetClassName(hWnd, className, ARRAYSIZE(className)) ||
        (_wcsicmp(className, L"Shell_TrayWnd") != 0 &&
         _wcsicmp(className, L"Shell_SecondaryTrayWnd") != 0) ||
        !GetWindowRect(hWnd, &now)) {
        return TRUE;
    }
    RECT base = now;
    if (const GrownTaskbar* grown = FindGrownTaskbar(hWnd)) {
        const int extra = GrownExtra(hWnd, *grown);
        if (grown->up) {
            base.top += extra;
        } else {
            base.bottom -= extra;
            OffsetRect(&base, 0, grown->lift);
        }
        std::erase_if(g_grownTaskbars, [hWnd](const GrownTaskbar& taskbar) {
            return taskbar.wnd == hWnd;
        });
    } else if (!IsDock()) {
        return TRUE;
    }
    SetWindowPos(hWnd, nullptr, base.left, base.top, base.right - base.left,
                 base.bottom - base.top, SWP_NOZORDER | SWP_NOACTIVATE);
    return TRUE;
}

// Whether the taskbar hides itself (Windows' setting): see
// g_taskbarAutoHides. On the taskbar's thread.
void ReadTaskbarAutoHide() {
    APPBARDATA data{sizeof(data)};
    g_taskbarAutoHides =
        (SHAppBarMessage(ABM_GETSTATE, &data) & ABS_AUTOHIDE) != 0;
}

// Defined further down.
void WatchTaskbarWay();

void WINAPI RefreshDockRoomProc(void*) {
    std::erase_if(g_grownTaskbars, [](const GrownTaskbar& taskbar) {
        return !IsWindow(taskbar.wnd);
    });
    if (!g_unloading) {
        ReadTaskbarAutoHide();
    }
    EnumThreadWindows(GetCurrentThreadId(), RegrowTaskbarProc, 0);
    WatchTaskbarWay();
    StartRendering();
}

void RefreshDockRoom() {
    if (HWND hTaskbarWnd = FindCurrentProcessTaskbarWnd()) {
        RunFromWindowThread(hTaskbarWnd, RefreshDockRoomProc, nullptr);
    }
}

// Whether an app's jump list is open, opened from this dock (defined further
// down).
bool JumpListOpen(HWND dock);

// Whether the dock shows the tray (its other end, see ClipDockWindow).
bool DockTrayShown() {
    return !g_settings.hideTray && !g_settings.showIsland;
}

// The shown dock's window may hide with the mouse still on it but off what
// takes the mouse (see ClipDockWindow): beside the dock, over what's behind.
// Only while the taskbar hides itself: Windows asks this also with its
// auto-hide off (as Start closes), and the dock would go.
bool DockMayHide(HWND hWnd) {
    if (g_unloading || !g_taskbarAutoHides || !IsDock() || hWnd != g_dockWnd ||
        g_dockRoomOpen ||
        g_dockHovered || IsRectEmpty(&g_dockShelfRest) || DockTrayShown() ||
        IsKeptShown(hWnd) || JumpListOpen(hWnd)) {
        return false;
    }
    RECT window{};
    RECT cut{};
    POINT pt{};
    if (!GetWindowRect(hWnd, &window) || !GetCursorPos(&pt) ||
        !PtInRect(&window, pt) ||
        GetWindowRgnBox(hWnd, &cut) == ERROR) {
        return false;
    }
    // Not in the strip along the edge under its icons, which brings it up
    // (see EdgeThreadProc): it would come up and go again, over and over.
    const HMONITOR monitor = TaskbarMonitor(hWnd);
    MONITORINFO info{sizeof(info)};
    int appsLeft = 0;
    int appsRight = 0;
    if (GetMonitorInfo(monitor, &info) &&
        GetAppsArea(monitor, &appsLeft, &appsRight) && pt.x >= appsLeft &&
        pt.x < appsRight &&
        pt.y >= info.rcMonitor.bottom -
                    EdgeStripPixels(pt.x, info.rcMonitor.bottom,
                                    GetDpiForWindow(hWnd))) {
        return false;
    }
    HRGN region = CreateRectRgn(0, 0, 0, 0);
    if (!region) {
        return false;
    }
    const bool off = GetWindowRgn(hWnd, region) != ERROR &&
                     !PtInRegion(region, pt.x - window.left, pt.y - window.top);
    DeleteObject(region);
    return off;
}

// The dock's window cut to what it shows: its part on its own monitor (see
// ClipToOwnMonitor), and once all of it is there, the taskbar's own height
// and the dock's shelf at rest, unless the mouse is on the dock or its icons
// move (they grow into its whole room then). Windows' own moves drop a
// region: cut again after them.
void ClipDockWindow(HWND hWnd, const RECT* next) {
    // (Unloading: the clips were taken off, none is set again: a last frame
    // would leave one the next load doesn't know, see ResetDockBehavior.)
    if (g_unloading) {
        return;
    }
    // Shown at once by Windows a moment ago: nothing of it shows (nor takes
    // the mouse) yet, see kQuietShowDelayMs.
    if (hWnd == g_quietShownWnd) {
        RECT current{};
        if (GetWindowRgnBox(hWnd, &current) != NULLREGION) {
            if (HRGN none = CreateRectRgn(0, 0, 0, 0)) {
                // (The system owns the region once it's set.)
                if (!SetWindowRgn(hWnd, none, RedrawsWithCut(hWnd))) {
                    DeleteObject(none);
                } else if (std::find(g_ownClips.begin(), g_ownClips.end(),
                                     hWnd) == g_ownClips.end()) {
                    g_ownClips.push_back(hWnd);
                }
            }
        }
        return;
    }
    DockScope scope(hWnd);
    RECT window;
    const GrownTaskbar* grown = FindGrownTaskbar(hWnd);
    if (!grown || hWnd != g_dockWnd || !GetWindowRect(hWnd, &window)) {
        ClipToOwnMonitor(hWnd, next);
        return;
    }
    const LONG width = window.right - window.left;
    const LONG height = window.bottom - window.top;
    const RECT own = TaskbarShownPart(hWnd, next);
    if (own.left > 0 || own.top > 0 || own.right < width ||
        own.bottom < height) {
        // (Hidden, the mouse isn't on its room: shown again, it's cut to the
        // taskbar's height until a frame finds the mouse on it. Left open,
        // its whole room showed with the dock up, painted white above it.)
        g_dockRoomOpen = false;
        // Hidden (only the strip at the edge left): it shows with the mouse
        // at the edge only under the dock's icons, like the transparent
        // taskbar under its apps (with the tray shown, all along the edge:
        // the tray is at the other end).
        // (No thumbnails with the dock going away: their window goes back.)
        if (!next) {
            PlaceThumbnailWindows();
        }
        // Only its last 2 pixels show (the rows above, see HiddenLift, take
        // no clicks), once it's there.
        const UINT dpi = GetDpiForWindow(hWnd);
        const bool trayShown = DockTrayShown();
        if (!next && !grown->up &&
            own.bottom - own.top <= grown->lift + MulDiv(4, dpi, 96)) {
            RECT under = own;
            under.top = std::max(own.top, own.bottom - MulDiv(2, dpi, 96));
            const bool onlyIcons =
                !trayShown && !IsRectEmpty(&g_dockShelfRest);
            if (onlyIcons) {
                const int margin = MulDiv((int)kAppsAreaMargin, dpi, 96);
                under.left = std::max(own.left, g_dockShelfRest.left - margin);
                under.right =
                    std::min(own.right, g_dockShelfRest.right + margin);
            }
            RECT current{};
            const bool same = GetWindowRgnBox(hWnd, &current) == SIMPLEREGION &&
                              EqualRect(&current, &under);
            if (under.right > under.left && !same) {
                if (HRGN region = CreateRectRgnIndirect(&under)) {
                    // (The system owns the region once it's set.)
                    if (SetWindowRgn(hWnd, region, RedrawsWithCut(hWnd))) {
                        if (std::find(g_ownClips.begin(), g_ownClips.end(),
                                      hWnd) == g_ownClips.end()) {
                            g_ownClips.push_back(hWnd);
                        }
                    } else {
                        DeleteObject(region);
                    }
                }
            }
            // (For the edge's own check, see EdgeThreadProc.)
            if (onlyIcons) {
                g_appsAreaMonitor = TaskbarMonitor(hWnd);
                SetAppsArea(g_appsAreaMonitor, window.left + under.left,
                            window.left + under.right);
            }
            return;
        }
        ClipToOwnMonitor(hWnd, next);
        return;
    }
    // (Shown, with the mouse on it or its icons moving: its whole room. Not
    // while hidden, above: its rows above the edge are on screen then, an
    // app opening or asking for attention would show the dock there.)
    if (g_dockRoomOpen) {
        ClipToOwnMonitor(hWnd, next);
        return;
    }
    const int extra = GrownExtra(hWnd, *grown);
    // The taskbar's own height: all along with the tray shown (it's at the
    // other end), otherwise only under the shelf, so the mouse gets past the
    // dock's sides to what's behind (see DockMayHide).
    RECT band{0, extra, width, height};
    if (!IsRectEmpty(&g_dockShelfRest) && !DockTrayShown()) {
        const int margin =
            MulDiv((int)kAppsAreaMargin, GetDpiForWindow(hWnd), 96);
        band.left = std::max(0L, g_dockShelfRest.left - margin);
        band.right = std::min(width, g_dockShelfRest.right + margin);
    }
    HRGN region = CreateRectRgnIndirect(&band);
    if (!region) {
        return;
    }
    if (!IsRectEmpty(&g_dockShelfRest)) {
        if (HRGN shelf = CreateRectRgnIndirect(&g_dockShelfRest)) {
            CombineRgn(region, region, shelf, RGN_OR);
            DeleteObject(shelf);
        }
    }
    // Already cut so.
    HRGN current = CreateRectRgn(0, 0, 0, 0);
    const bool same = current && GetWindowRgn(hWnd, current) != ERROR &&
                      EqualRgn(current, region);
    if (current) {
        DeleteObject(current);
    }
    // (The system owns the region once it's set.)
    if (!same && !SetWindowRgn(hWnd, region, RedrawsWithCut(hWnd))) {
        DeleteObject(region);
        return;
    }
    if (same) {
        DeleteObject(region);
    }
    // (Kept as the mod's also when it was already cut so, like by the mod
    // before it was turned off and on: the whole room opens again with the
    // mouse on it, see ClipToOwnMonitor.)
    if (std::find(g_ownClips.begin(), g_ownClips.end(), hWnd) ==
        g_ownClips.end()) {
        g_ownClips.push_back(hWnd);
    }
}

// Each step of a slide shows only the part of the taskbar on its own monitor,
// as Windows' own slide does: cut down to its apps (the transparent taskbar),
// or to that part. `next`: before the step, to where it goes.
void ClipSlideStep(HWND hWnd, const RECT* next) {
    DockScope scope(hWnd);
    if (hWnd == g_dockWnd && IsTransparentTaskbar() &&
        g_appsRight > g_appsLeft) {
        const RECT shown = TaskbarShownPart(hWnd, next);
        UpdateAppsArea(g_appsLeft, g_appsRight, false, &shown);
        return;
    }
    if (IsDock()) {
        ClipDockWindow(hWnd, next);
        return;
    }
    ClipToOwnMonitor(hWnd, next);
}

// The wait of a show Windows made at once (see kQuietShowDelayMs) ends: the
// caller cuts the window again, as it shows now or goes.
void EndQuietShow(HWND hWnd) {
    if (hWnd && hWnd == g_quietShownWnd) {
        g_quietShownWnd = nullptr;
        KillTimer(hWnd, kQuietShowTimerId);
    }
}

// Defined further down.
bool TaskbarSeeThrough();
void PaintTaskbarClear(HWND hWnd);

// Slides the window from where it is to `target`, one step per display frame,
// timed by the clock so a slow frame doesn't slow the whole animation down.
// Showing settles like a spring; hiding eases in and out.
void SlideTo(HWND hWnd, const RECT& target, int durationMs, bool show) {
    RECT start;
    if (!GetWindowRect(hWnd, &start) || EqualRect(&start, &target)) {
        return;
    }
    // A slide asked for during another one's step (the taskbar reacting to
    // its own move, like a new scale over another monitor): there at once,
    // or each would start another inside it, and the taskbar would never get
    // back to its messages.
    static int sliding = 0;
    if (sliding) {
        durationMs = 0;
    }
    sliding++;
    struct Done {
        ~Done() { sliding--; }
    } done;
    // Without an animation, it's there at once.
    if (durationMs <= 0) {
        ClipSlideStep(hWnd, &target);
        SetWindowPos(hWnd, nullptr, target.left, target.top,
                     target.right - target.left, target.bottom - target.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        ClipSlideStep(hWnd, nullptr);
        return;
    }

    // The taskbar's handlers leave to the slide what it does itself with each
    // frame (see g_slidingTaskbar). Its window is painted black (see
    // PaintTaskbarClear) as the taskbar's moves do: with every frame only
    // while it shows, where each frame opens more of the window (the paint
    // reaches only what's cut open); hiding, once before the first and once
    // after the last, since no row comes out that wasn't there.
    struct Sliding {
        HWND wnd;
        HWND previousWnd;
        bool previousHides;
        Sliding(HWND wnd, bool hides)
            : wnd(wnd), previousWnd(g_slidingTaskbar),
              previousHides(g_slidingHides) {
            g_slidingTaskbar = wnd;
            g_slidingHides = hides;
        }
        ~Sliding() {
            g_slidingTaskbar = previousWnd;
            g_slidingHides = previousHides;
            // (What the frames' cuts and moves left to draw, once.)
            if (TaskbarSeeThrough()) {
                PaintTaskbarClear(wnd);
                RedrawWindow(wnd, nullptr, nullptr,
                             RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
            }
        }
        Sliding(const Sliding&) = delete;
        Sliding& operator=(const Sliding&) = delete;
    };
    if (!show && TaskbarSeeThrough()) {
        PaintTaskbarClear(hWnd);
    }
    Sliding slidingNow(hWnd, !show);

    const double begin = NowSeconds();
    const double duration = durationMs / 1000.0;
    FrameTimer frames;
    for (;;) {
        const double t =
            std::min((NowSeconds() - begin) / duration, 1.0);
        const double p = t >= 1 ? 1 : show ? SpringOut(t) : EaseInOutCubic(t);
        const int left = LerpInt(start.left, target.left, p);
        const int top = LerpInt(start.top, target.top, p);
        const RECT step{left, top,
                        left + LerpInt(start.right - start.left,
                                       target.right - target.left, p),
                        top + LerpInt(start.bottom - start.top,
                                      target.bottom - target.top, p)};
        ClipSlideStep(hWnd, &step);
        SetWindowPos(hWnd, nullptr, step.left, step.top,
                     step.right - step.left, step.bottom - step.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        ClipSlideStep(hWnd, nullptr);
        if (t >= 1) {
            break;
        }
        WaitForAnimationFrame(frames);
    }
}

// How long after an app asks for attention a "show" counts as the taskbar
// showing itself for it, in seconds.
constexpr double kAttentionShowWindow = 1.0;

// Whether the user is reaching for the taskbar right now: the mouse on it or
// at the bottom edge, or the Windows key held.
bool UserIsReachingForTaskbar(HWND hWnd) {
    if (((GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) & 0x8000) ||
        EdgeCaptureHolds(hWnd)) {
        return true;
    }
    POINT pt;
    RECT rect;
    if (!GetCursorPos(&pt) || !GetWindowRect(hWnd, &rect)) {
        return true;
    }
    // (Cut down to a part, like the hidden dock's window to the edge under
    // its icons: only that part's width counts, all the way up the window,
    // where the mouse brings the dock, see EdgeShowDips.)
    RECT cut{};
    if (GetWindowRgnBox(hWnd, &cut) != ERROR && !IsRectEmpty(&cut)) {
        rect.left = std::max(rect.left, rect.left + cut.left);
        rect.right = std::min(rect.right, rect.left + (cut.right - cut.left));
    }
    // Only on its own monitor: hidden, the rest of its window is past the
    // edge, over the top of a monitor below, which counts for it only coming
    // from above (see EdgeCaptureHolds). The mouse resting there let an app
    // asking for attention bring it up (see KeepsDownForAttention).
    MONITORINFO info{sizeof(info)};
    const bool monitorKnown = GetMonitorInfo(TaskbarMonitor(hWnd), &info);
    if (monitorKnown) {
        IntersectRect(&rect, &rect, &info.rcMonitor);
        // (Hidden, its window is higher than that, see HiddenLift.)
        if (const GrownTaskbar* grown = FindGrownTaskbar(hWnd);
            grown && !grown->up) {
            rect.top = std::max(
                rect.top,
                info.rcMonitor.bottom -
                    (LONG)std::lround(EdgeShowDips() *
                                      GetDpiForWindow(hWnd) / 96.0));
        }
    }
    if (PtInRect(&rect, pt)) {
        return true;
    }
    // (At the bottom of its own monitor: another monitor can be below it.)
    return monitorKnown && PtInRect(&info.rcMonitor, pt) &&
           pt.y >= info.rcMonitor.bottom - 2;
}

// How light the screen is where the taskbar is about to show (0 dark, 1
// light), for the transparent taskbar's shadow: the area, shrunk to a few
// pixels, averaged.
void SampleBackdrop(const RECT& rect) {
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0) {
        return;
    }
    constexpr int kSampleWidth = 16;
    constexpr int kSampleHeight = 2;
    HDC screen = GetDC(nullptr);
    if (!screen) {
        return;
    }
    HDC memory = CreateCompatibleDC(screen);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = kSampleWidth;
    info.bmiHeader.biHeight = -kSampleHeight;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP bitmap =
        memory ? CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits,
                                  nullptr, 0)
               : nullptr;
    if (bitmap && bits) {
        HGDIOBJ old = SelectObject(memory, bitmap);
        SetStretchBltMode(memory, HALFTONE);
        if (StretchBlt(memory, 0, 0, kSampleWidth, kSampleHeight, screen,
                       rect.left, rect.top, width, height, SRCCOPY)) {
            GdiFlush();
            double total = 0;
            const auto* pixels = (const BYTE*)bits;
            for (int i = 0; i < kSampleWidth * kSampleHeight; i++) {
                const BYTE* pixel = pixels + i * 4;
                total += (0.0722 * pixel[0] + 0.7152 * pixel[1] +
                          0.2126 * pixel[2]) /
                         255;
            }
            g_backdropLight = total / (kSampleWidth * kSampleHeight);
        }
        SelectObject(memory, old);
    }
    if (bitmap) {
        DeleteObject(bitmap);
    }
    if (memory) {
        DeleteDC(memory);
    }
    ReleaseDC(nullptr, screen);
    StartRendering();
}

// A taskbar kept down for an app asking for attention, logged once in a
// while (the app asks again every second).
void LogKeptDown() {
    static std::atomic<double> logged{-100.0};
    const double now = NowSeconds();
    if (now - logged >= 10) {
        logged = now;
        IslandLog(L"Not showing the taskbar for an app asking for attention");
    }
}

// TrayUI::Unhide's request (TrayCommon::UnhideRequest) for an app asking for
// attention: measured, the one that comes right after a flash.
constexpr int kUnhideForAttention = 17;
// When a taskbar was last asked to show itself for something else (the main
// one, see TrayUI_Unhide_Hook, or any by the mod, see UnhideTaskbar), which
// one, and for how long that wins: for that one only, or the other monitor's
// taskbar with the app's button came up too whenever the mouse brought the
// main one up (measured: with the mouse resting at the main one's edge, half
// a second after it showed).
std::atomic<double> g_otherUnhideAt{-100.0};
std::atomic<HWND> g_otherUnhideWnd{nullptr};
constexpr double kOtherUnhideSeconds = 1.0;

// Another request than kUnhideForAttention right after a flash, logged once
// in a while (like LogKeptDown).
void LogUnhideAfterFlash(int flags, int request) {
    static std::atomic<double> logged{-100.0};
    const double now = NowSeconds();
    if (now - logged >= 10) {
        logged = now;
        IslandLog(L"Taskbar asked up right after a flash: request %d, flags 0x%X",
               request, (unsigned)flags);
    }
}

// Unless the whole dock is wanted, the taskbar doesn't show itself when an
// app asks for attention: only the app's icon peeks out (or nothing). The
// transparent taskbar never does (its button shows it, once up). Not when
// the user is reaching for it, nor when it was just asked up for something
// else (like Start opening).
bool KeepsDownForAttention(HWND hTaskbarWnd) {
    // (A button asking for attention counts too: an app like Discord gets
    // the taskbar up before its flash reaches the mod.)
    return !g_unloading && SlidesItself() &&
           (!IsDock() || g_settings.attentionMode != AttentionMode::Dock) &&
           (NowSeconds() - g_lastFlashTime < kAttentionShowWindow ||
            std::any_of(g_dockIcons.begin(), g_dockIcons.end(),
                        [](const DockIcon& icon) { return icon.attention; })) &&
           (hTaskbarWnd != g_otherUnhideWnd ||
            NowSeconds() - g_otherUnhideAt >= kOtherUnhideSeconds) &&
           !UserIsReachingForTaskbar(hTaskbarWnd);
}

// The main taskbar asked to show itself: not for an app asking for attention
// (see KeepsDownForAttention); any other reason goes through, and its show
// too (see TrayUI_SlideWindow_Hook).
using TrayUI_Unhide_t = void(WINAPI*)(void* pThis, int flags, int request);
TrayUI_Unhide_t TrayUI_Unhide_Original;
void WINAPI TrayUI_Unhide_Hook(void* pThis, int flags, int request) {
    if (request != kUnhideForAttention) {
        g_otherUnhideWnd = OwnTaskbarWnd();
        g_otherUnhideAt = NowSeconds();
        // (kUnhideForAttention was measured on one build: another one that
        // numbers it otherwise shows here, right after a flash.)
        if (NowSeconds() - g_lastFlashTime < kAttentionShowWindow) {
            LogUnhideAfterFlash(flags, request);
        }
    } else if (HWND taskbar = FindCurrentProcessTaskbarWnd();
               taskbar && KeepsDownForAttention(taskbar)) {
        LogKeptDown();
        return;
    }
    TrayUI_Unhide_Original(pThis, flags, request);
}

// To show a taskbar from the mod (see UnhideTaskbar), as the
// taskbar-auto-hide-when-maximized mod does: the main one's TrayUI (its
// ITrayComponentHost part, found by that vtable) and each secondary one's
// CSecondaryTray, noted from their window procedures. (On the taskbars'
// thread.)
void* TrayUI_ITrayComponentHost_vftable;
void* g_trayUiHost;
std::vector<std::pair<HWND, void*>> g_secondaryTrays;

using CSecondaryTray__Unhide_t = void(WINAPI*)(void* pThis,
                                               int flags,
                                               int request);
CSecondaryTray__Unhide_t CSecondaryTray__Unhide_Original;

using TrayUI_WndProc_t = LRESULT(WINAPI*)(void* pThis,
                                          HWND hWnd,
                                          UINT Msg,
                                          WPARAM wParam,
                                          LPARAM lParam,
                                          bool* flag);
TrayUI_WndProc_t TrayUI_WndProc_Original;
LRESULT WINAPI TrayUI_WndProc_Hook(void* pThis,
                                   HWND hWnd,
                                   UINT Msg,
                                   WPARAM wParam,
                                   LPARAM lParam,
                                   bool* flag) {
    if (Msg == WM_NCDESTROY) {
        g_trayUiHost = nullptr;
    } else if (!g_trayUiHost && TrayUI_ITrayComponentHost_vftable) {
        void* part = pThis;
        for (int i = 0; i < 64; i++, part = (void**)part + 1) {
            if (*(void**)part == TrayUI_ITrayComponentHost_vftable) {
                g_trayUiHost = part;
                break;
            }
        }
    }
    return TrayUI_WndProc_Original(pThis, hWnd, Msg, wParam, lParam, flag);
}

using CSecondaryTray_v_WndProc_t = LRESULT(WINAPI*)(void* pThis,
                                                    HWND hWnd,
                                                    UINT Msg,
                                                    WPARAM wParam,
                                                    LPARAM lParam);
CSecondaryTray_v_WndProc_t CSecondaryTray_v_WndProc_Original;
LRESULT WINAPI CSecondaryTray_v_WndProc_Hook(void* pThis,
                                             HWND hWnd,
                                             UINT Msg,
                                             WPARAM wParam,
                                             LPARAM lParam) {
    auto it = std::find_if(g_secondaryTrays.begin(), g_secondaryTrays.end(),
                           [hWnd](const auto& tray) {
                               return tray.first == hWnd;
                           });
    if (Msg == WM_NCDESTROY) {
        if (it != g_secondaryTrays.end()) {
            g_secondaryTrays.erase(it);
        }
    } else if (it == g_secondaryTrays.end()) {
        g_secondaryTrays.push_back({hWnd, pThis});
    }
    return CSecondaryTray_v_WndProc_Original(pThis, hWnd, Msg, wParam, lParam);
}

// A taskbar shown by the mod as the taskbar itself shows it (flags and
// request 0, as taskbar-auto-hide-when-maximized calls it). Without what
// that needs on this build: the message its own "show" timer sends (taken
// only with the mouse over the hidden taskbar).
void UnhideTaskbar(HWND taskbar) {
    WCHAR className[32] = L"";
    GetClassName(taskbar, className, ARRAYSIZE(className));
    // (Up for the mod's own reason, like nothing in its way anymore: not
    // kept down for an app asking for attention meanwhile, see
    // KeepsDownForAttention.)
    g_otherUnhideWnd = taskbar;
    g_otherUnhideAt = NowSeconds();
    if (_wcsicmp(className, L"Shell_TrayWnd") == 0) {
        if (g_trayUiHost && TrayUI_Unhide_Original) {
            TrayUI_Unhide_Original(g_trayUiHost, 0, 0);
            return;
        }
    } else if (CSecondaryTray__Unhide_Original) {
        for (const auto& [wnd, tray] : g_secondaryTrays) {
            if (wnd == taskbar) {
                CSecondaryTray__Unhide_Original(tray, 0, 0);
                return;
            }
        }
    }
    PostMessage(taskbar, WM_TIMER, kUnhideTimerId, 0);
}

// From the edge's thread to a taskbar's: the mouse is still where it brings
// the taskbar up, and the message of its own "show" timer didn't (see
// EdgeThreadProc): it's shown as the mod shows it (see UnhideTaskbar). That
// message is taken only with the mouse over the hidden taskbar as Windows
// saw it come there, not after sliding in along the edge from beside the
// strip under the dock's icons (#6816).
const UINT g_edgeUnhideMessage =
    RegisterWindowMessage(L"WindhawkMacIslandDock_EdgeUnhide");

// Whether the taskbar hides now (its hide timer): also with the mouse
// beside the shown dock, over what's behind it (see DockMayHide). The
// argument is forwarded as it came (two registers for an object passed by
// value on ARM64).
using TrayUI__ShouldAutoHideTray_t = bool(WINAPI*)(void* pThis,
                                                   void* telemetry,
                                                   void* telemetryMore);
TrayUI__ShouldAutoHideTray_t TrayUI__ShouldAutoHideTray_Original;
bool WINAPI TrayUI__ShouldAutoHideTray_Hook(void* pThis,
                                            void* telemetry,
                                            void* telemetryMore) {
    if (SlidesItself() && g_dockWnd && DockMayHide(g_dockWnd)) {
        return true;
    }
    // The mouse just past its bottom edge, on the monitor below (see
    // g_edgeCapture): it stays up.
    if (SlidesItself() && g_edgeCapture.load() &&
        EdgeCaptureHolds(FindCurrentProcessTaskbarWnd())) {
        return false;
    }
    return TrayUI__ShouldAutoHideTray_Original(pThis, telemetry,
                                               telemetryMore);
}

using TrayUI_SlideWindow_t = void(WINAPI*)(void* pThis,
                                           HWND hWnd,
                                           const RECT* rect,
                                           HMONITOR monitor,
                                           bool show,
                                           bool animate);
TrayUI_SlideWindow_t TrayUI_SlideWindow_Original;
// Defined further down.
bool IsFullscreenAppOnMonitor(HWND hTaskbarWnd);
void WINAPI TrayUI_SlideWindow_Hook(void* pThis,
                                    HWND hWnd,
                                    const RECT* rect,
                                    HMONITOR monitor,
                                    bool show,
                                    bool animate) {
    if (!SlidesItself()) {
        TrayUI_SlideWindow_Original(pThis, hWnd, rect, monitor, show, animate);
        return;
    }
    // (Usually stopped earlier, see CTaskBand_UnhideTray_Hook.)
    if (show && KeepsDownForAttention(hWnd)) {
        LogKeptDown();
        // The taskbar now takes itself for shown. Its hide timer puts that
        // right, without moving anything.
        PostMessage(hWnd, WM_TIMER, kHideTimerId, 0);
        return;
    }

    if (hWnd == g_dockWnd) {
        g_trayShown = show;
        // The taskbar takes over from a dock shown for a notification.
        g_shownForNotification = false;
    }

    if (show && IsTransparentTaskbar()) {
        SampleBackdrop(*rect);
    }

    // The window has the dock's room (see GrownTaskbar): it slides with it.
    RECT target = *rect;
    if (const GrownTaskbar* grown = FindGrownTaskbar(hWnd);
        grown && IsDock() && !g_unloading) {
        bool up = false;
        target = GrownRect(*rect, GrownExtra(hWnd, *grown), &up);
    }

    // After a full screen app (like a video in a browser), Windows sometimes
    // leaves the taskbar below other windows, so it would show behind them,
    // even with its "topmost" style still set: on top whenever it shows.
    if (show && !g_unloading && !IsFullscreenAppOnMonitor(hWnd)) {
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    // A hide while a show made at once still waits: nothing of the dock
    // showed, it goes at once. Any other move ends the wait too (a show: up
    // already, it shows now).
    const bool quietlyShown = hWnd == g_quietShownWnd;
    EndQuietShow(hWnd);
    if (!animate || g_unloading || (!show && quietlyShown)) {
        // A show at once with the user not reaching for the dock waits a
        // moment, cut to nothing (see kQuietShowDelayMs).
        if (show && !animate && !g_unloading && IsDock() &&
            FindGrownTaskbar(hWnd) && !UserIsReachingForTaskbar(hWnd)) {
            g_quietShownWnd = hWnd;
            SetTimer(hWnd, kQuietShowTimerId, kQuietShowDelayMs, nullptr);
        }
        TrayUI_SlideWindow_Original(pThis, hWnd, rect, monitor, show,
                                    animate && !quietlyShown);
        if (!g_unloading) {
            ClipSlideStep(hWnd, nullptr);
        }
        return;
    }

    // The window is moved by the mod's own animation, then the taskbar
    // finishes the move without its animation, which keeps its state right.
    SlideTo(hWnd, target,
            AnimationMs(show ? g_settings.showDuration
                             : g_settings.hideDuration),
            show);
    TrayUI_SlideWindow_Original(pThis, hWnd, rect, monitor, show, false);
    // Still cut down to its apps, or to its monitor (the taskbar's own move
    // drops a region), and to its strip once it's hidden.
    ClipSlideStep(hWnd, nullptr);
}

using SetTimer_t = decltype(&SetTimer);
SetTimer_t SetTimer_Original;
UINT_PTR WINAPI SetTimer_Hook(HWND hWnd,
                              UINT_PTR nIDEvent,
                              UINT uElapse,
                              TIMERPROC lpTimerFunc) {
    if (hWnd && !g_unloading && SlidesItself() &&
        (nIDEvent == kHideTimerId || nIDEvent == kUnhideTimerId)) {
        WCHAR className[32];
        if (GetClassName(hWnd, className, ARRAYSIZE(className)) &&
            (_wcsicmp(className, L"Shell_TrayWnd") == 0 ||
             _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0)) {
            int delay = nIDEvent == kUnhideTimerId ? g_settings.showDelay
                                                   : g_settings.hideDelay;
            // While the dock is kept up, hiding is tried right after that
            // (the other monitors' taskbars hide as usual).
            const double keep = g_keepShownUntil - NowSeconds();
            if (nIDEvent == kHideTimerId && hWnd == g_keepShownWnd &&
                keep > 0) {
                delay = std::max(delay, (int)(keep * 1000) + 100);
            }
            // A timer can't be shorter than 10 ms anyway.
            uElapse = (UINT)std::max(delay, 10);
        }
    }
    return SetTimer_Original(hWnd, nIDEvent, uElapse, lpTimerFunc);
}

// Starts the taskbar's own hide timer with a delay of the mod's choosing.
void ScheduleTaskbarHide(HWND hTaskbarWnd, UINT delayMs) {
    SetTimer_Original(hTaskbarWnd, kHideTimerId, delayMs, nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// Full-screen apps.

bool IsShellWindow(HWND hWnd) {
    WCHAR className[32];
    if (!GetClassName(hWnd, className, ARRAYSIZE(className))) {
        return false;
    }
    for (PCWSTR shellClass : {L"Progman", L"WorkerW", L"Shell_TrayWnd",
                              L"Shell_SecondaryTrayWnd"}) {
        if (_wcsicmp(className, shellClass) == 0) {
            return true;
        }
    }
    return false;
}

// What WindowInTheWayProc looks for (the island's way, see
// Island::WindowInTheWay, or a taskbar's, see WindowInTaskbarWay), and the
// window found. `self`: a process left out (the island's), or 0.
struct WindowInTheWaySearch {
    HMONITOR monitor;
    RECT place;
    bool maximizedOnly;
    DWORD self;
    bool found;
};

bool IsAppWindow(HWND hWnd);
bool IsCloakedWindow(HWND window);

BOOL CALLBACK WindowInTheWayProc(HWND hWnd, LPARAM lParam) {
    auto& search = *reinterpret_cast<WindowInTheWaySearch*>(lParam);
    DWORD processId = 0;
    GetWindowThreadProcessId(hWnd, &processId);
    if ((search.self && processId == search.self) || !IsAppWindow(hWnd) ||
        IsIconic(hWnd) || IsCloakedWindow(hWnd) || IsShellWindow(hWnd)) {
        return TRUE;
    }
    if (search.maximizedOnly) {
        search.found =
            IsZoomed(hWnd) &&
            MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL) == search.monitor;
    } else {
        // (Its visible frame: not the invisible resize borders.)
        RECT rect;
        RECT overlap;
        search.found = (SUCCEEDED(DwmGetWindowAttribute(
                            hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect,
                            sizeof(rect))) ||
                        GetWindowRect(hWnd, &rect)) &&
                       IntersectRect(&overlap, &rect, &search.place);
    }
    return !search.found;
}

// Windows' events that can change what's in the way (an app's window made,
// closed, shown, hidden, moved or sized, cloaked or uncloaked, brought to
// the front), as the taskbar-auto-hide-when-maximized mod watches them: the
// windows are looked at again only after one (see TaskbarWayThreadProc and
// Island::UpdateWindowEvents), not every few moments.
constexpr std::pair<DWORD, DWORD> kWindowEventRanges[] = {
    {EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND},
    {EVENT_OBJECT_CREATE, EVENT_OBJECT_HIDE},
    {EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE},
    {0x8017, 0x8018},  // EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED
};

// Whether an event is about an app's window: a top-level one, not the
// desktop's nor a taskbar's.
bool IsWindowEventForApp(HWND hWnd, LONG idObject, LONG idChild) {
    if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF || !hWnd ||
        (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)) {
        return false;
    }
    const HWND parent = GetAncestor(hWnd, GA_PARENT);
    return (!parent || parent == GetDesktopWindow()) && !IsShellWindow(hWnd);
}

// "When it hides" (hiding.hideWhen, #6202 #6368): the dock, or the
// transparent taskbar, stays up on the desktop and hides only while an app's
// window is in its way, like the island (see Island::WindowInTheWay); the
// mouse at the bottom edge still brings it up over the window. Each taskbar
// looks again after Windows' events, once for a burst of them (see
// TaskbarWayThreadProc and CheckTaskbarWay).
// (Not one of the taskbar's own timers: posted to it.)
constexpr UINT_PTR kTaskbarWayTimerId = 0x4D4B;
constexpr UINT kTaskbarWayBatchMs = 200;
// Its own hide timer started again once a window is in its way (it decides
// with the mouse, as always), this long after.
constexpr UINT kTaskbarWayHideMs = 50;

bool TaskbarWayWatched() {
    return g_settings.hideWhen > 0 &&
           (SlidesItself() || AppsTaskbarEffects()) && g_taskbarAutoHides &&
           !g_unloading;
}

// An app's window in the taskbar's way: maximized on its monitor (hideWhen
// 1), or over where it shows (2, maximized ones too); a full-screen app
// always.
bool WindowInTaskbarWay(HWND taskbar) {
    // (This taskbar's own dock, not the main one's.)
    DockScope scope(taskbar);
    const HMONITOR monitor = TaskbarMonitor(taskbar);
    MONITORINFO info{sizeof(info)};
    RECT window;
    if (!GetMonitorInfo(monitor, &info) || !GetWindowRect(taskbar, &window)) {
        return false;
    }
    // Where it shows: at the bottom edge, as tall as its window, over its
    // apps; the dock, only its shelf (not the room above it the magnified
    // icons take).
    const LONG height = window.bottom - window.top;
    RECT place{window.left, info.rcMonitor.bottom - height, window.right,
               info.rcMonitor.bottom};
    int appsLeft = 0;
    int appsRight = 0;
    if (taskbar == g_dockWnd && IsDock() && !IsRectEmpty(&g_dockShelfRest)) {
        place.left = window.left + g_dockShelfRest.left;
        place.right = window.left + g_dockShelfRest.right;
        // (Its shelf's top as last drawn up: with a monitor below, its window
        // goes past its monitor's bottom, see RoomOverMonitorBelowDips.)
        const GrownTaskbar* grown = FindGrownTaskbar(taskbar);
        place.top = grown && grown->shelfTop > 0
                        ? grown->shelfTop
                        : info.rcMonitor.bottom - (height - g_dockShelfRest.top);
    } else if (GetAppsArea(monitor, &appsLeft, &appsRight)) {
        place.left = appsLeft;
        place.right = appsRight;
    }
    WindowInTheWaySearch search{monitor, place, g_settings.hideWhen == 1, 0,
                                IsFullscreenAppOnMonitor(taskbar)};
    if (!search.found) {
        EnumWindows(WindowInTheWayProc, reinterpret_cast<LPARAM>(&search));
    }
    return search.found;
}

// After Windows' events (see TaskbarWayThreadProc): back up once nothing is
// in its way (as the mouse at the bottom edge brings it, see
// EdgeThreadProc), or its own hide timer started again once a window is
// (stopped while nothing was, see TaskbarSubclassProc).
void CheckTaskbarWay(HWND taskbar) {
    MONITORINFO info{sizeof(info)};
    if (!TaskbarWayWatched() ||
        !GetMonitorInfo(TaskbarMonitor(taskbar), &info)) {
        return;
    }
    const bool hidden = IsTaskbarHidden(taskbar, info.rcMonitor);
    const bool inTheWay = WindowInTaskbarWay(taskbar);
    if (hidden && !inTheWay) {
        UnhideTaskbar(taskbar);
    } else if (!hidden && inTheWay) {
        ScheduleTaskbarHide(taskbar, kTaskbarWayHideMs);
    }
}

// Whether a process's program is one of these ("name.exe").
bool ProcessIsOneOf(DWORD processId, std::initializer_list<PCWSTR> programs) {
    HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process) {
        return false;
    }
    WCHAR path[MAX_PATH];
    DWORD length = ARRAYSIZE(path);
    const bool named = QueryFullProcessImageName(process, 0, path, &length);
    CloseHandle(process);
    if (!named) {
        return false;
    }
    PCWSTR name = PathFindFileName(path);
    for (PCWSTR program : programs) {
        if (_wcsicmp(name, program) == 0) {
            return true;
        }
    }
    return false;
}

// The screenshot's selection (Win+Shift+S) (see Island::Tick).
constexpr std::initializer_list<PCWSTR> kSnippingPrograms = {
    L"ScreenClippingHost.exe", L"SnippingTool.exe"};

// Windows' own screens over everything for a moment, not apps: the
// screenshot's selection, the desktops' switch and task view, Start, search
// and the quick settings.
bool IsShellOverlay(DWORD processId) {
    return ProcessIsOneOf(processId, kSnippingPrograms) ||
           ProcessIsOneOf(processId,
                          {L"explorer.exe", L"ShellExperienceHost.exe",
                           L"StartMenuExperienceHost.exe", L"SearchHost.exe"});
}

// Whether a window is the screenshot's selection covering a monitor.
bool IsSnippingSelection(HWND window, HMONITOR monitor) {
    window = GetAncestor(window, GA_ROOT);
    MONITORINFO info{sizeof(info)};
    RECT rect;
    DWORD processId = 0;
    return window && IsWindowVisible(window) &&
           GetMonitorInfo(monitor, &info) &&
           GetWindowRect(window, &rect) && !IsZoomed(window) &&
           rect.left <= info.rcMonitor.left && rect.top <= info.rcMonitor.top &&
           rect.right >= info.rcMonitor.right &&
           rect.bottom >= info.rcMonitor.bottom &&
           GetWindowThreadProcessId(window, &processId) &&
           ProcessIsOneOf(processId, kSnippingPrograms);
}

// Whether the screenshot's selection covers a monitor (in front).
bool IsSnippingOn(HMONITOR monitor) {
    return IsSnippingSelection(GetForegroundWindow(), monitor);
}

// The island's process, told by its hello (see WatchIslandProcess).
extern DWORD g_islandProcessId;

// Whether a window covers the whole monitor like a full-screen app (games
// in exclusive or borderless full screen, videos in full screen).
// `windowsSaysBusy`: Windows itself takes a full-screen app to be running (a
// browser's video in full screen, for one), so a window with a title bar
// counts even if its size isn't exactly the screen's.
bool CoversMonitor(HWND hWnd,
                   HMONITOR monitor,
                   const RECT& monitorRect,
                   bool windowsSaysBusy = false) {
    if (!hWnd || !IsWindowVisible(hWnd) || IsShellWindow(hWnd) ||
        IsCloakedWindow(hWnd) ||
        MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL) != monitor) {
        return false;
    }
    DWORD processId = 0;
    // Not the mod's own windows (the dock's here; the island's, its panels
    // and the wallpaper's transition, which covers the whole monitor).
    if (GetWindowThreadProcessId(hWnd, &processId) &&
        (processId == GetCurrentProcessId() ||
         processId == g_islandProcessId)) {
        return false;
    }
    RECT rect;
    if (!GetWindowRect(hWnd, &rect) || rect.left > monitorRect.left ||
        rect.top > monitorRect.top || rect.right < monitorRect.right ||
        rect.bottom < monitorRect.bottom || IsShellOverlay(processId)) {
        return false;
    }
    // Exactly the screen: full screen, whatever its style (a browser keeps
    // its window "maximized" while it plays a video in full screen).
    if (EqualRect(&rect, &monitorRect)) {
        return true;
    }
    // Bigger than the screen: a maximized window (with the taskbar hidden,
    // its borders go past the screen's edges) isn't full screen, and neither
    // is one with a title bar stretched over it (like File Explorer), unless
    // Windows itself takes it for a full-screen app.
    if (IsZoomed(hWnd)) {
        return false;
    }
    if ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION) {
        return windowsSaysBusy;
    }
    return true;
}

// Whether a full-screen app (a game, a video) is on this taskbar's monitor,
// whichever app is in front (a video full screen on one monitor while the
// user works on another): see IsFullscreenAppOn.
bool IsFullscreenAppOn(HMONITOR monitor);

bool IsFullscreenAppOnMonitor(HWND hTaskbarWnd) {
    return IsFullscreenAppOn(TaskbarMonitor(hTaskbarWnd));
}

// Windows sometimes leaves a taskbar under a window with its "topmost" style
// still set (seen at sign-in: an app opening maximized on the second monitor
// came out above that monitor's taskbar until Start opened), and the guards
// that keep it on top (TrayUI_SlideWindow_Hook, TaskbarSubclassProc) only run
// where the mod hides it: the magnified icons in the dock's room would show
// behind that window. Raised as the mouse comes in, unless an app is in full
// screen there.
void RaiseOverWindows(HWND hTaskbarWnd) {
    RECT bar{};
    if (!GetWindowRect(hTaskbarWnd, &bar) ||
        IsFullscreenAppOnMonitor(hTaskbarWnd)) {
        return;
    }
    for (HWND w = GetWindow(hTaskbarWnd, GW_HWNDPREV); w;
         w = GetWindow(w, GW_HWNDPREV)) {
        RECT rect{}, both{};
        if (!(GetWindowLongPtr(w, GWL_EXSTYLE) & WS_EX_TOPMOST) &&
            IsWindowVisible(w) && GetWindowRect(w, &rect) &&
            IntersectRect(&both, &rect, &bar)) {
            SetWindowPos(hTaskbarWnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            return;
        }
    }
}

// A full-screen app on a monitor: the window in front, the one at the top of
// the screen (in front may be something small, like a browser's "press Esc"
// hint), or the window covering the monitor's middle (the app in front is on
// another monitor, or small).
bool IsFullscreenAppOn(HMONITOR monitor) {
    // Exclusive full screen (Direct3D) and presentations, as reported by
    // Windows.
    QUERY_USER_NOTIFICATION_STATE state = QUNS_ACCEPTS_NOTIFICATIONS;
    if (SUCCEEDED(SHQueryUserNotificationState(&state)) &&
        (state == QUNS_RUNNING_D3D_FULL_SCREEN ||
         state == QUNS_PRESENTATION_MODE)) {
        return true;
    }
    // "Busy": Windows sees a full-screen app (not only Direct3D ones).
    const bool busy = state == QUNS_BUSY;

    MONITORINFO info{sizeof(info)};
    if (!GetMonitorInfo(monitor, &info)) {
        return false;
    }
    HWND foreground = GetForegroundWindow();
    if (CoversMonitor(GetAncestor(foreground, GA_ROOT), monitor,
                      info.rcMonitor, busy)) {
        return true;
    }
    // Only when the window in front is that one's (a hint over a video),
    // not any window behind the one being used.
    const POINT top{(info.rcMonitor.left + info.rcMonitor.right) / 2,
                    info.rcMonitor.top + 1};
    HWND atTop = GetAncestor(WindowFromPoint(top), GA_ROOT);
    if (atTop && atTop != foreground) {
        DWORD topProcess = 0, foregroundProcess = 0;
        GetWindowThreadProcessId(atTop, &topProcess);
        GetWindowThreadProcessId(foreground, &foregroundProcess);
        if (topProcess == foregroundProcess &&
            CoversMonitor(atTop, monitor, info.rcMonitor, busy)) {
            return true;
        }
    }
    const POINT middle{(info.rcMonitor.left + info.rcMonitor.right) / 2,
                       (info.rcMonitor.top + info.rcMonitor.bottom) / 2};
    return CoversMonitor(GetAncestor(WindowFromPoint(middle), GA_ROOT), monitor,
                         info.rcMonitor);
}

////////////////////////////////////////////////////////////////////////////////
// The other apps' tray icons, for the island.
//
// Apps add, change and remove their tray icons (Shell_NotifyIcon) by sending
// the data to the taskbar window (WM_COPYDATA). The mod reads it there, as it
// passes, keeps its own copy of each icon, and the island draws them. Clicks
// on the island are passed on to the app the same way the taskbar does: as
// the icon's callback message. To learn about the icons added before the mod
// started, it asks the apps to add them again, with the message Windows sends
// them when the taskbar is restarted ("TaskbarCreated").

// The data the taskbar gets (with 32-bit handles, from 32 and 64-bit apps).
#pragma pack(push, 4)
struct NotifyIconData32 {
    DWORD cbSize;
    DWORD hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    DWORD hIcon;
    WCHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR szInfo[256];
    UINT uVersion;
    WCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
    GUID guidItem;
    DWORD hBalloonIcon;
};
struct TrayNotifyData {
    DWORD signature;
    DWORD message;
    NotifyIconData32 nid;
};
#pragma pack(pop)
constexpr DWORD kTrayNotifySignature = 0x34753423;

// An icon handle the mod owns, destroyed when the last copy goes.
struct OwnedIcon {
    HICON icon;
    explicit OwnedIcon(HICON icon) : icon(icon) {}
    ~OwnedIcon() {
        if (icon) {
            DestroyIcon(icon);
        }
    }
    OwnedIcon(const OwnedIcon&) = delete;
    OwnedIcon& operator=(const OwnedIcon&) = delete;
};

struct TrayIcon {
    HWND owner = nullptr;
    UINT id = 0;
    GUID guid{};
    bool hasGuid = false;
    UINT callback = 0;
    UINT version = 0;
    std::shared_ptr<OwnedIcon> icon;
    // Changes with the icon, so the island draws it again.
    UINT serial = 0;
    std::wstring tip;
    bool hidden = false;
    DWORD processId = 0;
    // Stays the same across restarts, to remember it pinned: the program
    // and the icon's ID.
    std::wstring key;
    // On the island: how much it's shown (growing in when it comes,
    // shrinking away when it goes, kept meanwhile as `leaving`).
    double shown = 1;
    double shownVelocity = 0;
    bool leaving = false;
};

// The version of an icon until its app tells it (NIM_SETVERSION): it's sent
// clicks in both forms. Some apps never tell it when they add their icon
// again (Qt's, asked when the mod starts), or added it before the mod saw.
constexpr UINT kTrayVersionUnknown = UINT_MAX;

// Whether an app takes the newer mouse messages (NOTIFYICON_VERSION_4).
bool TrayIconNewer(const TrayIcon& icon) {
    return icon.version >= 4 && icon.version != kTrayVersionUnknown;
}

SRWLOCK g_trayIconsLock = SRWLOCK_INIT;
std::vector<TrayIcon> g_trayIcons;
// Windows asked directly to add their icons again (see OnTrayNotify), once
// each. Under g_trayIconsLock.
std::vector<HWND> g_trayOwnersAsked;
UINT g_trayIconSerial;
// Tells the island they changed.
constexpr UINT WM_APP_TRAY = WM_APP + 53;

// A process's program file when the process can't be opened (a service's,
// like NVIDIA's display container, or an elevated app's): Windows tells any
// process's file by its ID (the system information class
// SystemProcessIdInformation), as a device's path
// ("\Device\HarddiskVolume3\..."), turned here into its drive's. Empty if
// not found.
std::wstring ProcessImagePathById(DWORD processId) {
    using NtQuerySystemInformation_t = LONG(NTAPI*)(ULONG, PVOID, ULONG,
                                                    PULONG);
    static const auto query =
        (NtQuerySystemInformation_t)GetProcAddress(
            GetModuleHandle(L"ntdll.dll"), "NtQuerySystemInformation");
    constexpr ULONG kSystemProcessIdInformation = 88;
    struct {
        HANDLE processId;
        USHORT length;
        USHORT room;
        PWSTR buffer;
    } info{};
    WCHAR name[MAX_PATH * 2];
    info.processId = (HANDLE)(ULONG_PTR)processId;
    info.room = sizeof(name);
    info.buffer = name;
    if (!query ||
        query(kSystemProcessIdInformation, &info, sizeof(info), nullptr) < 0) {
        return {};
    }
    const std::wstring device(name, info.length / sizeof(WCHAR));
    WCHAR drive[] = L"A:";
    WCHAR target[MAX_PATH];
    for (WCHAR letter = L'A'; letter <= L'Z'; letter++) {
        drive[0] = letter;
        if (!QueryDosDevice(drive, target, ARRAYSIZE(target))) {
            continue;
        }
        const size_t length = wcslen(target);
        if (device.size() > length && device[length] == L'\\' &&
            device.compare(0, length, target) == 0) {
            return drive + device.substr(length);
        }
    }
    return {};
}

