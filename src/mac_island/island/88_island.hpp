// The island read a notification: its Windows banner goes (`take`) or
// stays (see m_bannersOwed).
void Island::OweWindowsBanner(bool take) {
    const double now = NowSeconds();
    // It joined the stack showing now (or started it, just before).
    if (m_stackShowing && WindowsBannerShowing()) {
        if (!take) {
            KeepWindowsBannersShown();
        } else if (m_stackWaiting && !m_stackMustShow) {
            // The one it waited for: it stays off the screen.
            m_stackWaiting = false;
            KillTimer(m_hwnd, kBannerRestoreTimerId);
        } else if (!m_stackHidden && !m_stackMustShow &&
                   now - m_bannerShownAt < BannerRestoreSeconds() + 2 &&
                   CanTakeBanners()) {
            // Put back a moment ago, still waiting for it: off again.
            m_stackHidden = true;
            if (g_windowsBanner) {
                OnWindowsBanner(g_windowsBanner, false);
            }
        }
        return;
    }
    m_bannersOwed.push_back({now + kBannerOwedSeconds, take});
}

// How long a Windows banner hidden ahead of the pill waits for it: longer
// when the pill may be downloading the notification's picture first.
double Island::BannerRestoreSeconds() const {
    return (g_settings.islandWebPictures ? 9000 : kBannerRestoreMs) / 1000.0;
}

// Moves the Windows banners off the screen when the island shows their
// notifications instead (see m_bannersOwed). A new stack is hidden right
// away when the island can show notifications, so it doesn't flash, and
// put back if the island doesn't read its notification shortly after (see
// RestoreWindowsBanner). Never a window that's in front (a flyout the user
// opened).
void Island::OnWindowsBanner(HWND banner, bool appeared) {
    if ((!g_settings.islandHideWindowsBanners && !WindowsQuietForIsland()) ||
        GetForegroundWindow() == banner) {
        return;
    }
    int cloaked = 0;
    RECT rect;
    if (FAILED(DwmGetWindowAttribute(banner, DWMWA_CLOAKED, &cloaked,
                                     sizeof(cloaked))) ||
        cloaked || !GetWindowRect(banner, &rect)) {
        return;
    }
    const double now = NowSeconds();
    const int below = GetSystemMetrics(SM_YVIRTUALSCREEN) +
                      GetSystemMetrics(SM_CYVIRTUALSCREEN) + 200;
    // A new stack: the notifications read just before are its first ones
    // (all the island's, or not); with none, it waits for them.
    if (appeared) {
        m_bannerShownAt = now;
        KillTimer(m_hwnd, kBannerRestoreTimerId);
        if (rect.top < below) {
            m_bannerHomeBottom = rect.bottom;
        }
        m_stackShowing = true;
        m_stackHidden = m_stackWaiting = m_stackMustShow = false;
        m_stackTakeover = false;
        m_stackHeight = rect.bottom - rect.top;
        m_bannerReadRetries = 0;
        std::erase_if(m_bannersOwed, [now](const OwedBanner& owed) {
            return owed.until < now;
        });
        // With the island's own notifications: the call, alarm or reminder
        // on the pill may be this one.
        if (WantsTakeover()) {
            ReadWindowsBanners(300);
        }
        if (!g_settings.islandHideWindowsBanners) {
            m_bannersOwed.clear();
        } else if (!m_bannersOwed.empty()) {
            const bool all =
                std::all_of(m_bannersOwed.begin(), m_bannersOwed.end(),
                            [](const OwedBanner& owed) { return owed.take; });
            m_stackHidden = all;
            m_stackMustShow = !all;
            m_bannersOwed.clear();
        } else if (CanTakeBanners() && !WindowsQuietForIsland()) {
            m_stackHidden = m_stackWaiting = true;
            SetTimer(m_hwnd, kBannerRestoreTimerId,
                     (UINT)(BannerRestoreSeconds() * 1000), nullptr);
        }
        // With the island's own notifications, Windows shows only calls,
        // alarms and reminders: hidden at once while it's read (see
        // OnWindowsBannersRead), back if it isn't the island's.
        m_stackChangedAt = now;
        CheckShown();
        if (CanTakeOverAhead()) {
            m_stackHidden = m_stackWaiting = true;
            SetTimer(m_hwnd, kBannerRestoreTimerId, kBannerRestoreMs, nullptr);
            ReadWindowsBanners(0);
        }
    }
    // Another notification joined the stack (or one left): read again.
    if (!appeared && m_stackShowing &&
        rect.bottom - rect.top != m_stackHeight) {
        m_stackHeight = rect.bottom - rect.top;
        m_stackChangedAt = now;
        if (WantsTakeover() || m_stackTakeover || m_stackWaiting ||
            (m_stackHidden && WindowsQuietForIsland())) {
            m_bannerReadRetries = 0;
            ReadWindowsBanners(150);
        }
    }
    // Where Windows has it, while it's on the screen.
    if (!m_stackHidden && !appeared && rect.top < below) {
        m_bannerHomeBottom = rect.bottom;
    }
    // Windows may move it back as it shows: off again.
    if (m_stackHidden && rect.top < below) {
        SetWindowPos(banner, nullptr, rect.left, below, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void Island::OnWindowsBannersGone() {
    KillTimer(m_hwnd, kBannerRestoreTimerId);
    if (m_stackTakeover && m_bannerWanted && m_banner.urgent) {
        m_banner.reply.actions = m_bannerOwnActions;
        Render(true);
    }
    m_stackTakeover = false;
    m_stackShowing = false;
    m_stackHidden = m_stackWaiting = m_stackMustShow = false;
}

bool Island::CanTakeOverAhead() const {
    return WindowsQuietForIsland() && g_bannerEnabled && m_visible &&
           !m_minimized && !AnyPanelOpen();
}

// A stack hidden while waiting for the island (to read its notification, or
// to read the stack) wasn't confirmed in time: it comes back.
void Island::OnBannerRestoreTimer() {
    KillTimer(m_hwnd, kBannerRestoreTimerId);
    if (m_stackTakeover) {
        EndTakeover();
    } else {
        RestoreWindowsBanner();
    }
}

// The call, alarm or reminder on the pill (or under the bar), with the
// island's own notifications, while the island is shown.
bool Island::WantsTakeover() const {
    return WindowsQuietForIsland() && m_bannerWanted && m_banner.urgent &&
           !m_banner.appId.empty() && m_visible && !m_minimized &&
           !AnyPanelOpen();
}

void Island::StartWindowsBannerThread(DWORD delayMs,
                                      int press,
                                      const WindowsBannerRead& fallback) {
    if (!g_windowsBanner || !m_bannerReadStop) {
        return;
    }
    std::erase_if(m_bannerReadThreads, [](HANDLE thread) {
        if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0) {
            CloseHandle(thread);
            return true;
        }
        return false;
    });
    HANDLE stop = nullptr;
    if (!DuplicateHandle(GetCurrentProcess(), m_bannerReadStop,
                         GetCurrentProcess(), &stop, 0, FALSE,
                         DUPLICATE_SAME_ACCESS)) {
        return;
    }
    auto* request = new WindowsBannerRequest{
        g_windowsBanner, stop, delayMs, m_banner.title, press, fallback};
    HANDLE thread = CreateThread(nullptr, 0, WindowsBannerThreadProc, request,
                                 0, nullptr);
    if (!thread) {
        delete request;
        return;
    }
    m_bannerReadThreads.push_back(thread);
    m_bannerReadsBusy++;
}

// Reads Windows' stack of banners (one read at a time; another one after it
// if asked meanwhile).
void Island::ReadWindowsBanners(DWORD delayMs) {
    if (!WantsTakeover() && !m_stackTakeover &&
        !(m_stackShowing && WindowsQuietForIsland())) {
        return;
    }
    if (m_bannerReadsBusy > 0) {
        m_bannerReadAgain = true;
        return;
    }
    StartWindowsBannerThread(delayMs, kReadOnly, {});
}

// Presses a button of Windows' banner for the call, alarm or reminder on
// the pill (-1: its "x"); if that fails, the island's own button with the
// same name is pressed instead (the app's own).
void Island::PressWindowsBanner(int button, const std::wstring& label) {
    WindowsBannerRead fallback;
    for (const auto& action : m_bannerOwnActions) {
        if (!label.empty() && action.label == label &&
            action.activationType != L"system") {
            fallback.fallbackAppId = m_banner.appId;
            fallback.fallbackArguments = action.arguments;
            fallback.fallbackActivationType = action.activationType;
            break;
        }
    }
    m_takeoverPressed = true;
    StartWindowsBannerThread(0, button, fallback);
}

// The takeover ends: the island's own buttons, and Windows' banner back on
// the screen.
void Island::EndTakeover() {
    if (!m_stackTakeover) {
        return;
    }
    m_stackTakeover = false;
    if (m_bannerWanted && m_banner.urgent) {
        m_banner.reply.actions = m_bannerOwnActions;
        Render(true);
    }
    if (m_stackShowing) {
        KeepWindowsBannersShown();
    }
}

void Island::OnWindowsBannersRead() {
    std::vector<WindowsBannerRead> reads;
    AcquireSRWLockExclusive(&g_windowsBannerReadsLock);
    reads.swap(g_windowsBannerReads);
    ReleaseSRWLockExclusive(&g_windowsBannerReadsLock);
    for (const auto& read : reads) {
        m_bannerReadsBusy = std::max(0, m_bannerReadsBusy - 1);
        // Pressing Windows' button failed (it's gone): the app's own.
        if (read.pressRequested && !read.pressed &&
            !read.fallbackAppId.empty()) {
            ActivateToast(read.fallbackAppId, read.fallbackArguments,
                          read.fallbackActivationType);
        }
    }
    if (reads.empty()) {
        return;
    }
    const auto& views = reads.back().views;
    // Every notification in Windows' stack is the island's: the one on the
    // pill, or one waiting for it (it shows next). Hidden, with the pill's
    // one's buttons on the pill (Windows' own names, "Snooze" too).
    const std::wstring title = BannerText(m_banner.title);
    auto islands = [&](const std::wstring& viewTitle) {
        return viewTitle == title ||
               std::any_of(m_waitingToasts.begin(), m_waitingToasts.end(),
                           [&](const IslandToast& toast) {
                               return toast.urgent &&
                                      BannerText(toast.title) == viewTitle;
                           });
    };
    const auto pillView =
        std::find_if(views.begin(), views.end(),
                     [&](const WindowsBannerView& view) {
                         return view.priority && view.title == title;
                     });
    const bool ours =
        m_stackShowing && WantsTakeover() && pillView != views.end() &&
        std::all_of(views.begin(), views.end(),
                    [&](const WindowsBannerView& view) {
                        return view.priority && !view.title.empty() &&
                               islands(view.title);
                    });
    // With the island's own notifications, an app Windows lets through "Do
    // not disturb" (a priority one, like the Snipping Tool): a stack of
    // notifications the pill shows (or will) stays hidden, like any other
    // one the island shows (the pill's own buttons kept).
    auto pillShows = [&](const std::wstring& viewTitle) {
        return (m_bannerWanted && viewTitle == title) ||
               std::any_of(m_waitingToasts.begin(), m_waitingToasts.end(),
                           [&](const IslandToast& toast) {
                               return BannerText(toast.title) == viewTitle;
                           });
    };
    const bool pills =
        !ours && !m_stackTakeover && m_stackShowing &&
        WindowsQuietForIsland() && g_settings.islandHideWindowsBanners &&
        m_visible && !views.empty() &&
        std::all_of(views.begin(), views.end(),
                    [&](const WindowsBannerView& view) {
                        return !view.title.empty() && pillShows(view.title);
                    });
    // Calls, alarms and reminders only (or any notification, a priority
    // app's coming through "Do not disturb", when the island hides Windows'
    // banners), one of them not read by the island yet, the stack having
    // just changed: it waits for it, hidden (not shown for a moment, then
    // hidden once the pill shows it).
    const bool waitForIsland =
        !ours && m_stackShowing && CanTakeOverAhead() && !views.empty() &&
        (g_settings.islandHideWindowsBanners ||
         std::all_of(views.begin(), views.end(),
                     [](const WindowsBannerView& view) {
                         return view.priority;
                     })) &&
        NowSeconds() - m_stackChangedAt < kBannerRestoreMs / 1000.0;
    if (ours) {
        KillTimer(m_hwnd, kBannerRestoreTimerId);
        m_stackWaiting = false;
        if (!m_stackTakeover) {
            m_stackTakeover = true;
            m_bannerOwnActions = m_banner.reply.actions;
        }
        std::vector<ToastReply::Action> actions;
        for (size_t i = 0; i < pillView->buttons.size() && i < 3; i++) {
            if (!pillView->buttons[i].empty()) {
                actions.push_back(
                    {pillView->buttons[i], std::to_wstring(i), L"windows"});
            }
        }
        m_banner.reply.actions = actions.empty() ? m_bannerOwnActions : actions;
        m_takeoverDismiss = pillView->dismiss;
        m_stackHidden = true;
        if (g_windowsBanner) {
            OnWindowsBanner(g_windowsBanner, false);
        }
        StartAnimating();
        Render(true);
    } else if (pills) {
        KillTimer(m_hwnd, kBannerRestoreTimerId);
        m_stackWaiting = false;
        if (!m_stackHidden) {
            m_stackHidden = true;
            if (g_windowsBanner) {
                OnWindowsBanner(g_windowsBanner, false);
            }
        }
    } else if (waitForIsland) {
        if (!m_stackHidden) {
            m_stackHidden = true;
            if (g_windowsBanner) {
                OnWindowsBanner(g_windowsBanner, false);
            }
        }
        m_stackWaiting = true;
        SetTimer(m_hwnd, kBannerRestoreTimerId,
                 (UINT)std::max(
                     1.0, (kBannerRestoreMs / 1000.0 -
                           (NowSeconds() - m_stackChangedAt)) * 1000),
                 nullptr);
    } else if (m_stackTakeover) {
        EndTakeover();
    } else if ((m_stackWaiting || m_stackHidden) && WindowsQuietForIsland() &&
               !views.empty()) {
        // Not the island's (a priority app's notification the pill doesn't
        // show, say): back now.
        RestoreWindowsBanner();
    } else if (m_stackShowing && (WantsTakeover() || m_stackWaiting) &&
               views.empty() && m_bannerReadRetries < 3) {
        // Not drawn yet: a moment later.
        m_bannerReadRetries++;
        m_bannerReadAgain = false;
        if (m_bannerReadsBusy == 0) {
            StartWindowsBannerThread(400, kReadOnly, {});
        }
        return;
    }
    if (m_bannerReadAgain && m_bannerReadsBusy == 0) {
        m_bannerReadAgain = false;
        ReadWindowsBanners(0);
    }
}

// The Windows banners come back where they were: the island didn't read
// the notification in time, or the stack has one the island doesn't show
// (see KeepWindowsBannersShown).
void Island::RestoreWindowsBanner() {
    KillTimer(m_hwnd, kBannerRestoreTimerId);
    m_stackHidden = false;
    m_stackWaiting = false;
    RECT rect;
    const int below = GetSystemMetrics(SM_YVIRTUALSCREEN) +
                      GetSystemMetrics(SM_CYVIRTUALSCREEN) + 200;
    if (g_windowsBanner && IsWindow(g_windowsBanner) &&
        GetWindowRect(g_windowsBanner, &rect) && rect.top >= below &&
        m_bannerHomeBottom) {
        SetWindowPos(g_windowsBanner, nullptr, rect.left,
                     m_bannerHomeBottom - (rect.bottom - rect.top), 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

// Moves an app's small window, opened from its tray icon at the bottom of
// the screen (where it thinks the icon is), under the island.
void CALLBACK TrayAppWinEventProc(HWINEVENTHOOK,
                                  DWORD,
                                  HWND hWnd,
                                  LONG idObject,
                                  LONG idChild,
                                  DWORD,
                                  DWORD) {
    if (idObject == OBJID_WINDOW && idChild == CHILDID_SELF && hWnd &&
        g_island) {
        Island* island = g_trayIsland ? g_trayIsland : MainIsland();
        IslandScope scope(island);
        island->OnTrayAppWindow(hWnd);
    }
}

