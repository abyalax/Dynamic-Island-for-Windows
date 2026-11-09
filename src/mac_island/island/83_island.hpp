#pragma once
// Takes the latest new notification and shows it: in the pill, or peeking out
// of the line when minimized. Not with "Do not disturb", in the bar, or over
// an open panel.
void Island::ShowBanner() {
    CheckShown();
    AcquireSRWLockExclusive(&g_islandToastsLock);
    std::vector<IslandToast> toasts = std::move(g_islandToasts);
    g_islandToasts.clear();
    ReleaseSRWLockExclusive(&g_islandToastsLock);
    // Each island counts them; the main one shows them, plays their sounds
    // and handles the Windows banners.
    if (m_main) {
        NoteToastsSeen(toasts);
        for (Island* other : g_islands) {
            if (other != this) {
                IslandScope scope(other);
                other->ShowToasts(toasts);
            }
        }
    }
    ShowToasts(std::move(toasts));
}

void Island::ShowToasts(std::vector<IslandToast> toasts) {
    for (const auto& toast : toasts) {
        AddUnread(toast);
    }
    // (A notch without the bell, or another monitor's island not chosen to
    // show them: only its apps' icons. The main island also handles Windows'
    // banners and the sounds, see g_notifyScreens.)
    if (!ShowsNotices() || (!m_main && !ShowsNotifications())) {
        return;
    }
    // Their Windows banners (see m_bannersOwed): the island's go (in the
    // pill, or as its app's icon and in the list); a call's, an alarm's or a
    // reminder's, or one the island can't show now, stays, and the others
    // with it. Only while Windows shows banners (with its "Do not disturb",
    // only calls, alarms and reminders show), and not for an app whose
    // banners are off (Windows shows none for it).
    if (m_main && g_settings.islandHideWindowsBanners &&
        !WindowsQuietForIsland() && WindowsShowsBanners()) {
        const bool canTake = AnyIslandTakesBanners();
        for (const auto& toast : toasts) {
            if (!toast.appId.empty() && !toast.sticky &&
                AppShowsBanners(toast.appId)) {
                OweWindowsBanner(canTake && !toast.urgent &&
                                 !toast.reply.windowsButtons);
            }
        }
    }
    // The island's own notifications, with Windows kept quiet: the newest
    // one's sound, as it comes (not in full screen, nor with the island's
    // "Do not disturb", which only keeps them, nor over a call ringing).
    // Calls, alarms and reminders show in the pill too, ahead of the
    // others, and ring (see QueueUrgentSound); the island's "Do not disturb"
    // lets them through, like Windows' does (unless chosen, see
    // QuietSilencesAll).
    if (WindowsQuietForIsland()) {
        const IslandToast* newest = nullptr;
        const IslandToast* newestUrgent = nullptr;
        for (const auto& toast : toasts) {
            if (toast.appId.empty() || toast.sticky) {
                continue;
            }
            if (toast.urgent) {
                newestUrgent = &toast;
            } else {
                newest = &toast;
            }
        }
        if (m_main && newest && !g_islandQuiet && m_visible &&
            !m_urgentRinging) {
            PlayToastSound(newest->appId, newest->sound);
        }
        if (m_main && newestUrgent && !QuietSilencesAll()) {
            QueueUrgentSound(*newestUrgent);
        }
    } else {
        // Otherwise Windows' banner shows them, with their buttons (also
        // Windows' own ones, see ToastReply::windowsButtons): not in the
        // pill too.
        std::erase_if(toasts, [](const IslandToast& toast) {
            return toast.urgent || toast.reply.windowsButtons;
        });
    }
    if (g_settings.islandOwnsNotifications && g_islandQuiet) {
        std::erase_if(toasts, [](const IslandToast& toast) {
            return !toast.sticky && !toast.urgent && !toast.glyph;
        });
    }
    // Apps ignored for a moment (see m_snoozedApps): not in the pill (their
    // sound plays, and their icon pops, see AddUnread).
    std::erase_if(toasts, [this](const IslandToast& toast) {
        return !toast.sticky && !toast.urgent && IsSnoozed(toast.appId);
    });
    if (!g_bannerEnabled) {
        std::erase_if(toasts, [](const IslandToast& toast) {
            return !toast.sticky && !toast.glyph;
        });
    }
    // Another monitor's island shows the apps' notifications; the main one,
    // with its own turned off, still the calls, alarms and reminders and the
    // island's own notices (its timers' and alarms' ends, and its status
    // ones with a glyph: charger, battery, Bluetooth, Do not disturb; see
    // g_notifyScreens).
    if (!m_main) {
        std::erase_if(toasts, [](const IslandToast& toast) {
            return toast.urgent || toast.sticky || toast.glyph;
        });
    } else if (!g_notifyMain) {
        std::erase_if(toasts, [](const IslandToast& toast) {
            return !toast.urgent && !toast.sticky && !toast.glyph;
        });
    }
    if (toasts.empty() && m_waitingToasts.empty()) {
        return;
    }
    // Calls, alarms and reminders apart: they go first.
    const auto firstUrgent = std::stable_partition(
        toasts.begin(), toasts.end(),
        [](const IslandToast& toast) { return !toast.urgent; });
    std::vector<IslandToast> urgent(std::make_move_iterator(firstUrgent),
                                    std::make_move_iterator(toasts.end()));
    toasts.erase(firstUrgent, toasts.end());
    const bool anyNew = !toasts.empty() || !urgent.empty();
    // One of those on the pill: new ones wait behind it.
    if (m_bannerWanted && m_banner.urgent) {
        if (anyNew) {
            const bool urgentQueued = !urgent.empty();
            QueueToasts(std::move(urgent), std::move(toasts));
            // Windows' stack may have it too: read again.
            if (urgentQueued && m_stackShowing && WantsTakeover()) {
                m_bannerReadRetries = 0;
                ReadWindowsBanners(100);
            }
            StartAnimating();
            Render(true);
        }
        return;
    }
    // The one on the pill gives way to a call, an alarm or a reminder, and
    // shows again after it (with what was being typed).
    IslandToast interrupted;
    bool interrupting = false;
    if (!urgent.empty() && m_bannerWanted) {
        interrupted = m_banner;
        if (m_bannerTyping) {
            interrupted.draft = m_bannerReplyText;
        }
        interrupting = true;
    }
    // Typing a reply: a new message of the same chat updates the banner (what
    // was typed stays, and the focus), others wait until the reply is done.
    // The pill takes them all, so the Windows banner goes.
    if (m_bannerTyping) {
        if (urgent.empty()) {
            for (auto& toast : toasts) {
                if (toast.appId == m_banner.appId &&
                    toast.title == m_banner.title) {
                    m_banner.text = std::move(toast.text);
                    m_banner.fullText = std::move(toast.fullText);
                    m_banner.image = std::move(toast.image);
                    m_banner.imageCircle = toast.imageCircle;
                    m_banner.imageData = std::move(toast.imageData);
                    if (toast.reply.CanReply()) {
                        m_banner.reply = std::move(toast.reply);
                    }
                } else {
                    m_waitingToasts.push_back(std::move(toast));
                }
            }
            StartAnimating();
            Render(true);
            return;
        }
        EndBannerTyping();
    }
    // Not shown in the pill (or under the bar) now, with a panel open: it's
    // on the island anyway (its app with the red dot, the bell's dot, and the
    // list, shown again if open), so the Windows banner still goes. In full
    // screen, it stays (waiting ones keep waiting). With a panel open (not
    // the list, which shows it), it waits for the panel to close (its sound
    // played as it came, not again then; see AnimationFrame).
    if (!m_visible || AnyPanelOpen()) {
        if (anyNew) {
            if (!CanTakeBanners()) {
                KeepWindowsBannersShown();
            }
            m_notifications.Refresh();
            if (!m_visible) {
                // (Hidden for a full-screen app: the island's own notices
                // that wait for a button, like an alarm's Stop while its
                // sound repeats, show once it's back; see CheckShown.)
                std::vector<IslandToast> sticky;
                for (auto& toast : toasts) {
                    if (toast.sticky) {
                        sticky.push_back(std::move(toast));
                    }
                }
                QueueToasts({}, std::move(sticky));
            } else if (!m_notifications.IsOpen()) {
                QueueToasts(std::move(urgent), std::move(toasts));
                // Only the latest few (all of them are in the list).
                while (m_waitingToasts.size() > kMaxWaitingToasts) {
                    const auto oldest = std::find_if(
                        m_waitingToasts.begin(), m_waitingToasts.end(),
                        [](const IslandToast& toast) { return !toast.urgent; });
                    if (oldest == m_waitingToasts.end()) {
                        break;
                    }
                    m_waitingToasts.erase(oldest);
                }
            }
        }
        return;
    }
    // The newest notification shows at once (a call, an alarm or a reminder
    // first, the others then waiting behind it); a waiting one only when no
    // banner is shown.
    IslandToast next;
    if (!urgent.empty()) {
        next = std::move(urgent.back());
        urgent.pop_back();
        std::vector<IslandToast> others;
        if (!toasts.empty()) {
            others.push_back(std::move(toasts.back()));
        }
        QueueToasts(std::move(urgent), std::move(others));
        if (interrupting) {
            m_waitingToasts.insert(
                std::find_if(m_waitingToasts.begin(), m_waitingToasts.end(),
                             [](const IslandToast& toast) {
                                 return !toast.urgent;
                             }),
                std::move(interrupted));
        }
    } else if (!toasts.empty()) {
        next = std::move(toasts.back());
    } else if (!m_bannerWanted) {
        next = std::move(m_waitingToasts.front());
        m_waitingToasts.erase(m_waitingToasts.begin());
    } else {
        return;
    }
    // (The Bar hidden by Win+J, into its clock or not, like minimized; it has
    // no line to peek out of.)
    if ((m_minimized || m_barHidden) && !g_bannerWhenMinimized &&
        !next.appId.empty()) {
        if (anyNew) {
            KeepWindowsBannersShown();
        }
        if (m_minimized) {
            m_peekApp = next.appId;
            m_peekStart = NowSeconds();
            StartAnimating();
        }
        return;
    }
    // Over one shown: the new one's contents fade in.
    if (m_bannerWanted && m_bannerAmount > 0.5) {
        m_bannerSwap = 1;
        m_bannerSwapVelocity = 0;
    }
    m_banner = std::move(next);
    m_bannerWanted = true;
    // Calls, alarms and reminders stay until closed, answered, or gone from
    // the notification center (see OnToastsChanged); Windows' banner for
    // one may be taken over (see m_stackTakeover).
    if (!m_banner.sticky && !m_banner.urgent) {
        SetTimer(m_hwnd, kBannerTimerId, kBannerShownMs, nullptr);
    } else {
        KillTimer(m_hwnd, kBannerTimerId);
    }
    // Windows' stack may have it too (a call's, or a priority app's with the
    // island's own notifications).
    if (m_stackShowing && (WantsTakeover() || WindowsQuietForIsland())) {
        m_bannerReadRetries = 0;
        ReadWindowsBanners(200);
    }
    StartAnimating();
}

void Island::QueueToasts(std::vector<IslandToast> urgent,
                         std::vector<IslandToast> others) {
    m_waitingToasts.insert(
        std::find_if(m_waitingToasts.begin(), m_waitingToasts.end(),
                     [](const IslandToast& toast) { return !toast.urgent; }),
        std::make_move_iterator(urgent.begin()),
        std::make_move_iterator(urgent.end()));
    m_waitingToasts.insert(m_waitingToasts.end(),
                           std::make_move_iterator(others.begin()),
                           std::make_move_iterator(others.end()));
}

// A call's, an alarm's or a reminder's sound. Windows lets those through its
// "Do not disturb" by default (showing its banner and playing their sound
// itself), so the island waits a moment and plays it only if no Windows
// banner showed meanwhile, or just before. One that repeats rings while it's
// on the pill, up to a minute.
void Island::QueueUrgentSound(const IslandToast& toast) {
    m_urgentSoundApp = toast.appId;
    m_urgentSound = toast.sound;
    m_urgentSoundLoop = toast.loop;
    m_urgentSoundAt = NowSeconds();
    SetTimer(m_hwnd, kUrgentSoundTimerId, kUrgentSoundWaitMs, nullptr);
}

void Island::PlayUrgentSound() {
    KillTimer(m_hwnd, kUrgentSoundTimerId);
    if (m_windowsBannerSeenAt > m_urgentSoundAt - 4) {
        return;
    }
    const bool onPill =
        m_bannerWanted && m_banner.urgent &&
        _wcsicmp(m_banner.appId.c_str(), m_urgentSoundApp.c_str()) == 0;
    const bool loop = m_urgentSoundLoop && onPill;
    if (m_urgentRinging) {
        m_urgentRinging = false;
        StopToastSound();
    }
    PlayToastSound(m_urgentSoundApp, m_urgentSound, loop);
    if (loop) {
        m_urgentRinging = true;
        SetTimer(m_hwnd, kUrgentSoundStopTimerId, kUrgentSoundMaxMs, nullptr);
    }
}

void Island::StopUrgentSound() {
    KillTimer(m_hwnd, kUrgentSoundTimerId);
    KillTimer(m_hwnd, kUrgentSoundStopTimerId);
    if (m_urgentRinging) {
        m_urgentRinging = false;
        StopToastSound();
    }
}

// An alarm's own notice (see CheckAlarms: its Stop button).
bool IsAlarmNotice(const IslandToast& toast) {
    return toast.appId.empty() &&
           std::any_of(toast.reply.actions.begin(), toast.reply.actions.end(),
                       [](const ToastReply::Action& action) {
                           return action.arguments == L"alarm-stop";
                       });
}

void Island::HideBanner() {
    KillTimer(m_hwnd, kBannerTimerId);
    // A call, an alarm or a reminder stops ringing as it goes, and Windows'
    // banner for it comes back, unless one of its buttons was pressed (it
    // goes by itself then; checked a moment later).
    if (m_banner.urgent) {
        StopUrgentSound();
        if (m_stackTakeover && !m_takeoverPressed) {
            EndTakeover();
        }
        m_takeoverPressed = false;
    }
    // An alarm's notice closed any way (its x, a click away), not only by
    // its Stop: its sound stops too.
    if (m_bannerWanted && IsAlarmNotice(m_banner)) {
        if (Island* main = MainIsland()) {
            main->StopAlarmSound();
        }
    }
    if (m_bannerWanted) {
        m_bannerWanted = false;
        StartAnimating();
    }
    // The next waiting notification, once this one has gone.
    if (!m_waitingToasts.empty()) {
        SetTimer(m_hwnd, kBannerNextTimerId, kBannerNextMs, nullptr);
    }
}

void Island::Render(bool force) {
    if (!m_hwnd) {
        return;
    }
    Layout();
    // The bar turned upright (or back, or to the other side) once closed
    // into its dot: its strip goes to that edge now (it was told when the
    // option changed, before the layout turned).
    if (m_bar && m_appBar &&
        m_appBarEdge != (m_vertical ? (m_sideLeft ? ABE_LEFT : ABE_RIGHT)
                                    : ABE_TOP)) {
        SetAppBarPosition();
    }
    UpdateVisualizerTimer();
    UpdateFillTimer();
    UpdateEyesTimer();
    UpdateGlass();
    RenderTrayTip();
    // Gliding to another monitor: from where the dot was.
    if (m_monitorMove == 2) {
        const double left = 1 - std::clamp(m_moveT, 0.0, 1.0);
        const POINT to{
            m_position.x + (LONG)std::lround((m_pill.left + m_pill.right) / 2),
            m_position.y + (LONG)std::lround((m_pill.top + m_pill.bottom) / 2)};
        m_position.x += (LONG)std::lround((m_moveFrom.x - to.x) * left);
        m_position.y += (LONG)std::lround((m_moveFrom.y - to.y) * left);
    }

    // Everything drawn, as a key: nothing to do if it didn't change.
    std::wstring key =
        std::to_wstring(m_size.cx) + L"|" + std::to_wstring(m_position.x) +
        L"|" + std::to_wstring((int)m_hover) + m_hoverApp + L"|" +
        std::to_wstring((int)(m_expand * 1000)) + L"|" +
        std::to_wstring((int)(m_appear * 1000)) + std::to_wstring(m_away) +
        L"|" + std::to_wstring(m_position.y) + L"|" +
        std::to_wstring((int)(PeekAmount(NowSeconds()) * 1000)) + L"|" +
        std::to_wstring((int)(GearAmount(NowSeconds()) * 1000)) + m_gearText +
        L"|" + std::to_wstring((int)(m_bannerAmount * 1000)) + L"|" +
        std::to_wstring((int)(m_bannerSwap * 1000)) + L"|" + m_banner.appId +
        m_banner.title + m_banner.text + L"|" +
        std::to_wstring((int)(m_bannerOpen * 1000)) + L"|" +
        std::to_wstring((int)(m_bannerExpand * 1000)) + L"|" +
        std::to_wstring(m_bannerTyping) + m_bannerReplyText + L"|" +
        std::to_wstring(m_bannerEdit.caret) + L"|" +
        (m_bannerTyping ? std::to_wstring((int)(CaretColor().a * 24))
                        : std::wstring()) +
        L"|" + std::to_wstring((int)m_bannerEdit.scroll) + L"|" +
        std::to_wstring(g_trayIconSerial) + L"|" +
        std::to_wstring((int)(m_trayAmount * 1000)) + L"|" +
        std::to_wstring((int)(m_appsAmount * 1000)) + L"|" +
        std::to_wstring((int)(m_trayBounce * 1000)) + L"|" +
        std::to_wstring((int)(m_trayArrow * 1000)) + L"|" +
        std::to_wstring((int)(m_lineHover * 1000)) + L"|" +
        std::to_wstring((int)(m_lineFade * 1000)) + L"," +
        std::to_wstring((int)(m_hiddenClock * 1000)) + HiddenPillText() +
        L"|" + std::to_wstring(m_pinStates.size()) + L"|" + m_pinShownKey +
        std::to_wstring((int)(PinShown(NowSeconds()) * 100)) + L"|" +
        std::to_wstring(m_trayKeepOpen) +
        std::to_wstring((int)g_showNotificationContent.load()) + L"|" +
        std::to_wstring(m_press.target) + L"|" +
        std::to_wstring((int)(m_press.Amount(NowSeconds()) * 1000)) + L"|" +
        std::to_wstring((int)(m_playSwap * 1000)) + L"|" +
        std::to_wstring((int)(m_themeTurn * 1000)) + L"|" +
        std::to_wstring((int)(m_networkSwap * 1000)) + L"|" +
        std::to_wstring((int)(m_clockSwap * 1000)) + L"|" +
        std::to_wstring((int)(m_mediaSwap * 1000)) + L"|" +
        std::to_wstring((int)(m_mediaCoverSwap * 1000)) + L"|" +
        std::to_wstring((int)(m_mediaTitleWidthShown * 10)) + L"|" +
        (m_netState == NetState::Connecting
             ? std::to_wstring((int)(NowSeconds() * 1000 / kNetworkFrameMs))
             : std::wstring()) +
        L"|" +
        // The camera's rings spreading (every frame while they do).
        (m_cameraSince && NowSeconds() - m_cameraSince < kCameraBlinkSeconds
             ? std::to_wstring((int)((NowSeconds() - m_cameraSince) * 1000))
             : std::wstring()) +
        L"|" +

        std::to_wstring(m_mediaShown.thumbnailSerial) + L"|" +
        std::to_wstring(m_mediaShown.sourceIndex) + L"/" +
        std::to_wstring(m_mediaShown.sources.size()) + L"|" +
        std::to_wstring(m_vertical) + m_clockTime + L"|" +
        std::to_wstring(m_unseen) + L"|" + m_hoverFades.Signature() + L"|" +
        std::to_wstring(g_themeChangeStart) + L"|" +
        std::to_wstring(g_theme.background[0] + g_theme.background[1] * 3 +
                        g_theme.background[2] * 7 + g_theme.foreground[0] * 11 +
                        g_theme.backgroundAlpha * 13 + g_theme.light +
                        g_theme.foreground[1] * 17 +
                        g_theme.foreground[2] * 19 + g_theme.icon[0] * 23 +
                        g_theme.icon[1] * 29 + g_theme.icon[2] * 31) +
        std::to_wstring(m_barBlurred) + std::to_wstring(LiquidGlassOn()) +
        std::to_wstring(m_liquid.Shown()) + L"|";
    for (const auto& slot : m_slots) {
        key += slot.text + L"|" + std::to_wstring((int)(slot.scale * 1000)) +
               L"|" + std::to_wstring((int)std::round(slot.rect.left)) + L"|" +
               std::to_wstring((int)(slot.alpha * 100)) + L"|";
        // An icon changing: the one before goes as this one comes. (Only
        // the icon: the volume's and the battery's percentages beside them
        // are slots of the same item, see addSideText.)
        if (slot.font == 1 && (slot.item == IslandItem::Volume ||
                               slot.item == IslandItem::Battery ||
                               slot.item == IslandItem::Bell ||
                               slot.item == IslandItem::Microphone)) {
            auto swap = std::find_if(
                m_glyphSwaps.begin(), m_glyphSwaps.end(),
                [&](const GlyphSwap& entry) { return entry.item == slot.item; });
            if (swap == m_glyphSwaps.end()) {
                m_glyphSwaps.push_back({slot.item, slot.text, L"", 0, 0});
            } else if (swap->shown != slot.text) {
                swap->from = swap->shown;
                swap->shown = slot.text;
                swap->t = 1;
                swap->velocity = 0;
                StartAnimating();
            }
        }
    }
    for (const auto& swap : m_glyphSwaps) {
        key += std::to_wstring((int)(swap.t * 1000)) + L"|";
    }
    key += std::to_wstring((int)(m_updatesDot * 1000)) + L"|" +
           std::to_wstring((int)(m_unseenDot * 1000)) + L"|" +
           std::to_wstring((int)(m_weatherSwap * 1000)) + L"|";
    // A title too long scrolling (see m_mediaMarquee): drawn again as it
    // moves (not while it pauses at its ends). The banner's too.
    if (MediaTitleScrolls()) {
        key += std::to_wstring(
            (int)(MarqueeShift(NowSeconds() - m_mediaMarqueeStart,
                               m_mediaMarqueeDistance, m_scale) *
                  4));
    }
    if (m_vizRunning) {
        for (float level : m_vizLevels) {
            key += std::to_wstring((int)(level * 100)) + L",";
        }
        // (The rainbow going around: drawn again each degree it turns.)
        if (g_settings.islandVisualizerColor == kVisualizerRainbow) {
            key += L"r" + std::to_wstring((long long)(NowSeconds() *
                                                       kRainbowTurns * 360));
        }
    }
    // The eyes' look (the colors and shape too), when they're on the pill.
    if (EyesSlot()) {
        key += m_eyes.Signature(NowSeconds());
    }
    if (m_fillAmount > 0.001) {
        key += L"f" + std::to_wstring((int)(m_fillAmount * 1000)) + L"," +
               std::to_wstring((int)(m_fillShown * 1000)) + L"," +
               std::to_wstring((int)(m_fillColor.r * 255) << 16 |
                               (int)(m_fillColor.g * 255) << 8 |
                               (int)(m_fillColor.b * 255)) +
               (m_fillUnknown ? L"u," : L",") +
               std::to_wstring(g_settings.islandFillStyle) +
               std::to_wstring(m_fillEnd) + L"," +
               (m_fillMoving && !m_minimized && !m_away
                    ? std::to_wstring((int)(std::fmod(NowSeconds() /
                                                              1.6,
                                                          1.0) *
                                                 400))
                             : std::wstring());
    }
    if (m_bannerMarquee) {
        key += L"b" + std::to_wstring((int)(
                          MarqueeShift(NowSeconds() - m_bannerMarqueeStart,
                                       m_bannerMarqueeDistance, m_scale) *
                          4));
    }
    if (!force && key == m_drawnKey) {
        return;
    }
    if (!m_canvas.Begin(m_size)) {
        return;
    }
    m_drawnKey = key;
    // (A title scrolling says so as it's drawn; kept while nothing's drawn,
    // in its pauses.)
    m_mediaMarquee = false;
    m_bannerMarquee = false;
    auto* target = m_canvas.target.get();
    const float scale = m_scale;
    const float iconScale = IconScale();

    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    // Opaque (only the glass theme lets what's behind show, by its own
    // opacity).
    target->CreateSolidColorBrush(Bg(1), brush.put());
    // Over the Liquid Glass (once it shows), the shapes are only there to
    // take the mouse: the glass has the color (see GlassLayer).
    const bool liquid = LiquidGlassOn() && !m_away;
    const bool overLiquid = liquid && m_liquid.Shown();
    m_liquidShapes.clear();
    auto addLiquid = [&](const D2D1_RECT_F& rect, float radius,
                         float opacity) {
        GlassShape shape;
        shape.rect = rect;
        shape.radius = radius;
        shape.tint = Bg(GlassTint(LiquidTint()));
        shape.opacity = opacity;
        m_liquidShapes.push_back(shape);
    };

    if (m_bar) {
        // Hidden with only the clock (islandHideClock): the bar closes into
        // a small pill with the time in its edge's middle (where the mouse
        // brings it back, as on the line).
        const float clock = (float)std::clamp(m_hiddenClock, -0.05, 1.1);
        if (g_settings.islandHideClock && !m_away) {
            const D2D1_RECT_F small = HiddenClockRect(scale);
            // (Where it is on the screen, for a window under it.)
            m_hiddenClockScreen = {m_position.x + (LONG)small.left,
                                   m_position.y + (LONG)small.top,
                                   m_position.x + (LONG)std::ceil(small.right),
                                   m_position.y + (LONG)std::ceil(small.bottom)};
        }
        // Over the frosted glass, its strip has the color (see UpdateGlass);
        // the bar only takes the mouse. See-through (islandBarClear), only
        // its items show.
        const bool clear = BarClear();
        if (m_barBlurred || overLiquid || clear) {
            brush->SetColor(D2D1::ColorF(0, 0, 0, 1.0f / 255));
        }
        if (clock > 0.001f) {
            const D2D1_RECT_F small = HiddenClockRect(scale);
            const float k = std::min(clock, 1.0f);
            const D2D1_RECT_F rect{
                m_pill.left + (small.left - m_pill.left) * clock,
                m_pill.top + (small.top - m_pill.top) * clock,
                m_pill.right + (small.right - m_pill.right) * clock,
                m_pill.bottom + (small.bottom - m_pill.bottom) * clock};
            const float radius =
                std::min(rect.bottom - rect.top, rect.right - rect.left) / 2 *
                std::max(k, 0.0f);
            // (A see-through or frosted bar: its pill colored as it closes.)
            D2D1_COLOR_F fill = Bg(1);
            if (clear || (g_settings.islandFrosted && g_transparencyEffects)) {
                fill.a *= std::clamp(clock * 2, 0.0f, 1.0f);
            }
            if (liquid) {
                addLiquid(rect, radius, (float)std::clamp(m_appear, 0.0, 1.0));
            }
            if (overLiquid) {
                fill = D2D1::ColorF(0, 0, 0, 1.0f / 255);
            }
            brush->SetColor(fill);
            target->FillRoundedRectangle({rect, radius, radius}, brush.get());
            if (!overLiquid) {
                brush->SetColor(Fg(0.10f * EdgeAmount() * std::max(k, 0.0f)));
                target->DrawRoundedRectangle(
                    {{rect.left + 0.5f, rect.top + 0.5f, rect.right - 0.5f,
                      rect.bottom - 0.5f},
                     radius,
                     radius},
                    brush.get(), 1);
            }
            if (clock > 0.05f && BannerMorph() < 0.01f) {
                DrawHiddenPillContents(
                    target, brush.get(), rect,
                    std::clamp((clock - 0.3f) / 0.5f, 0.0f, 1.0f), scale);
            }
        } else {
            if (liquid) {
                addLiquid(m_pill, 0, (float)std::clamp(m_appear, 0.0, 1.0));
            }
            target->FillRectangle(m_pill, brush.get());
        }
        if (const float corner = BarInnerCorner();
            corner > 0 && !clear && !liquid && clock <= 0.001f) {
            if (m_vertical) {
                const float x = m_sideLeft ? m_pill.right : m_pill.left;
                const float out = m_sideLeft ? 1.0f : -1.0f;
                FillInnerCorner(target, brush.get(), {x, m_pill.top},
                                {0, 1}, {out, 0}, corner);
                FillInnerCorner(target, brush.get(), {x, m_pill.bottom},
                                {0, -1}, {out, 0}, corner);
            } else {
                FillInnerCorner(target, brush.get(),
                                {m_pill.left, m_pill.bottom}, {1, 0}, {0, 1},
                                corner);
                FillInnerCorner(target, brush.get(),
                                {m_pill.right, m_pill.bottom}, {-1, 0}, {0, 1},
                                corner);
            }
        }
        if (!clear && clock <= 0.001f) {
            brush->SetColor(Fg(0.08f));
            target->FillRectangle(
                !m_vertical ? D2D1_RECT_F{m_pill.left, m_pill.bottom - 1,
                                          m_pill.right, m_pill.bottom}
                : m_sideLeft ? D2D1_RECT_F{m_pill.right - 1, m_pill.top,
                                           m_pill.right, m_pill.bottom}
                             : D2D1_RECT_F{m_pill.left, m_pill.top,
                                           m_pill.left + 1, m_pill.bottom},
                brush.get());
        }
        // A new notification: a banner drops out of the bar's middle, like a
        // panel out of an item.
        const float banner = BannerMorph();
        // A download's progress or the battery charging: the whole bar
        // filling (not with a banner open, nor closed into the clock).
        if (const float fill = (float)std::clamp(m_fillAmount, 0.0, 1.0) *
                               (1 - std::clamp(banner * 3, 0.0f, 1.0f)) *
                               (1 - std::clamp(clock * 3, 0.0f, 1.0f));
            fill > 0.01f) {
            DrawFill(target, brush.get(), m_pill, 0, nullptr, fill);
        }
        if (banner > 0) {
            const D2D1_RECT_F open = BannerRect();
            const float middle = (open.left + open.right) / 2;
            const float from = std::round(30 * scale);
            const D2D1_RECT_F start{middle - from, m_pill.bottom, middle + from,
                                    m_pill.bottom};
            const D2D1_RECT_F rect{
                start.left + (open.left - start.left) * banner,
                start.top + (open.top - start.top) * banner,
                start.right + (open.right - start.right) * banner,
                start.bottom + (open.bottom - start.bottom) * banner};
            const float radius =
                std::min(BannerRadius(),
                         std::min(rect.bottom - rect.top,
                                  rect.right - rect.left) /
                             2);
            const float opacity = std::clamp(banner * 3, 0.0f, 1.0f);
            D2D1_COLOR_F fill = Bg(1);
            fill.a *= opacity;
            if (liquid) {
                addLiquid(rect, radius,
                          opacity * (float)std::clamp(m_appear, 0.0, 1.0));
            }
            if (overLiquid) {
                fill = D2D1::ColorF(0, 0, 0, 1.0f / 255);
            }
            brush->SetColor(fill);
            target->FillRoundedRectangle({rect, radius, radius}, brush.get());
            if (!overLiquid) {
                brush->SetColor(Fg(0.10f * EdgeAmount() * opacity));
                target->DrawRoundedRectangle(
                    {{rect.left + 0.5f, rect.top + 0.5f, rect.right - 0.5f,
                      rect.bottom - 0.5f},
                     radius,
                     radius},
                    brush.get(), 1);
            }
        }
    } else {
        // Between the thin line (0) and the pill (1).
        const float e = (float)std::clamp(m_expand, 0.0, 1.1);
        const float centerX = (m_pill.left + m_pill.right) / 2;
        const float centerY = (m_pill.top + m_pill.bottom) / 2;
        const float hover = (float)std::clamp(m_lineHover, 0.0, 1.2);
        // An invisible strip along the edge (as long as chosen, see
        // islandActivationWidth), so the mouse pushed against it reaches the
        // line rather than the window behind it; also with the line unseen.
        // Only as thick as the line under the mouse at a screen's edge:
        // thicker, the mouse passing near the edge (a browser's tabs) brought
        // the island back (#6468); with another monitor across that edge the
        // mouse goes past it, so a few rows more there. (Notches hiding
        // together: each one has its line, any one bringing them all back.)
        // (A line that doesn't open the island has none: the mouse goes to
        // the windows below.)
        if (e < 0.5f && BannerMorph() <= 0 && LineOpens()) {
            const LONG alongX = m_position.x + (LONG)centerX;
            const LONG alongY = m_position.y + (LONG)centerY;
            const POINT across =
                m_vertical ? POINT{m_sideLeft ? m_monitor.left - 1
                                              : m_monitor.right,
                                   alongY}
                           : POINT{alongX, AtBottom() ? m_monitor.bottom
                                                      : m_monitor.top - 1};
            const float extra =
                MonitorFromPoint(across, MONITOR_DEFAULTTONULL) ? 8 : 0;
            brush->SetColor(D2D1::ColorF(0, 0, 0, 1.0f / 255));
            target->FillRectangle(
                EdgeLineRect(ActivationLength(scale),
                             (kLineHeight + kLineHoverGrowth + extra) * scale),
                brush.get());
        }
        const D2D1_RECT_F line = EdgeLineRect(
            (kLineWidth + (kLineHoverWidth - kLineWidth) * hover) * scale,
            (kLineHeight + kLineHoverGrowth * hover) * scale);
        D2D1_RECT_F rect{line.left + (m_pill.left - line.left) * e,
                         line.top + (m_pill.top - line.top) * e,
                         line.right + (m_pill.right - line.right) * e,
                         line.bottom + (m_pill.bottom - line.bottom) * e};
        float lineMix = 1 - std::clamp(e * 2.5f, 0.0f, 1.0f);
        // Only the clock when hidden: the line becomes a small pill with the
        // time (where the mouse brings the island back, as on the line).
        const float clock = (float)std::clamp(m_hiddenClock, -0.05, 1.1) *
                            std::clamp(1 - e, 0.0f, 1.0f);
        if (clock > 0 || (m_minimized && g_settings.islandHideClock)) {
            const D2D1_RECT_F small = HiddenClockRect(scale);
            // (Where it is on the screen, for a window under it.)
            m_hiddenClockScreen = {m_position.x + (LONG)small.left,
                                   m_position.y + (LONG)small.top,
                                   m_position.x + (LONG)std::ceil(small.right),
                                   m_position.y + (LONG)std::ceil(small.bottom)};
            if (clock > 0) {
                rect = {rect.left + (small.left - rect.left) * clock,
                        rect.top + (small.top - rect.top) * clock,
                        rect.right + (small.right - rect.right) * clock,
                        rect.bottom + (small.bottom - rect.bottom) * clock};
                lineMix *= 1 - std::clamp(clock, 0.0f, 1.0f);
            }
        }
        // A new app peeking out: the line becomes a small black pill.
        const float peek = PeekAmount(NowSeconds());
        if (peek > 0) {
            // Against the edge, where the pill would be.
            const float peekLength = 46 * scale;
            const float peekThickness = 30 * scale;
            D2D1_RECT_F small;
            if (m_vertical) {
                const float left =
                    m_sideLeft ? m_pill.left : m_pill.right - peekThickness;
                small = {left, centerY - peekLength / 2, left + peekThickness,
                         centerY + peekLength / 2};
            } else {
                const float top =
                    AtBottom() ? m_pill.bottom - peekThickness : m_pill.top;
                small = {centerX - peekLength / 2, top,
                         centerX + peekLength / 2, top + peekThickness};
            }
            rect = {rect.left + (small.left - rect.left) * peek,
                    rect.top + (small.top - rect.top) * peek,
                    rect.right + (small.right - rect.right) * peek,
                    rect.bottom + (small.bottom - rect.bottom) * peek};
            lineMix *= 1 - std::clamp(peek, 0.0f, 1.0f);
        }
        // A new notification: the pill stretches into the banner.
        const float banner = BannerMorph();
        if (banner > 0) {
            const D2D1_RECT_F open = BannerRect();
            rect = {rect.left + (open.left - rect.left) * banner,
                    rect.top + (open.top - rect.top) * banner,
                    rect.right + (open.right - rect.right) * banner,
                    rect.bottom + (open.bottom - rect.bottom) * banner};
        }
        // Black, not the line's gray, as the banner opens.
        lineMix *= 1 - std::clamp(banner * 3, 0.0f, 1.0f);
        // Showing or hiding: out of (or into) a dot in its middle.
        const float open = AppearOpen();
        if (open < 1.12f && std::fabs(open - 1) > 0.0005f) {
            const float dot = std::round(10 * scale);
            const float dotY = (rect.top + rect.bottom) / 2;
            const D2D1_RECT_F from{centerX - dot / 2, dotY - dot / 2,
                                   centerX + dot / 2, dotY + dot / 2};
            rect = {from.left + (rect.left - from.left) * open,
                    from.top + (rect.top - from.top) * open,
                    from.right + (rect.right - from.right) * open,
                    from.bottom + (rect.bottom - from.bottom) * open};
            // Not smaller than the dot while bouncing back.
            if (rect.right - rect.left < dot) {
                rect.left = centerX - dot / 2;
                rect.right = centerX + dot / 2;
            }
        }
        // Round ends, whichever way it stands (a notch: its curves' size).
        const float pillRadius =
            std::min(rect.bottom - rect.top, rect.right - rect.left) / 2;
        // (Corners chosen less round: the pill's, not its line's.)
        const float shapeRadius =
            m_notch >= 0
                ? std::min(pillRadius, NotchCurve())
                : pillRadius *
                      (g_settings.islandPillCorners +
                       (100 - g_settings.islandPillCorners) * lineMix) /
                      100;
        const float bannerRadius = BannerRadius();
        const float radius = std::min(
            pillRadius,
            shapeRadius + (bannerRadius - shapeRadius) *
                              std::clamp(banner, 0.0f, 1.0f) *
                              std::clamp(open, 0.0f, 1.0f));
        // The line is light, to be seen over dark and light windows alike.
        // (Over the Liquid Glass, only the line has a color, fading as the
        // pill opens, and the glass shows as much as it's gone.)
        const D2D1_COLOR_F background = Bg(1);
        brush->SetColor(
            overLiquid
                ? D2D1::ColorF(0.55f, 0.55f, 0.55f,
                               std::max(0.74f * lineMix, 1.0f / 255))
                : D2D1::ColorF(
                      background.r + (0.55f - background.r) * lineMix,
                      background.g + (0.55f - background.g) * lineMix,
                      background.b + (0.55f - background.b) * lineMix,
                      background.a + (0.74f - background.a) * lineMix));
        // A notch against the top edge: square there, its curves growing
        // out of the line as it opens (and staying with a banner).
        ID2D1PathGeometry* notchFill = nullptr;
        ID2D1PathGeometry* notchLine = nullptr;
        if (m_notch >= 0 && rect.top < 0.5f) {
            const float shoulder = NotchShoulderWidth() *
                                   std::clamp(e, 0.0f, 1.0f) *
                                   std::clamp(open, 0.0f, 1.0f);
            const bool edgeLeft = rect.left <= m_notchEdgeLeft + 0.5f;
            const bool edgeRight = rect.right >= m_notchEdgeRight - 0.5f;
            const std::array<float, 8> key{
                rect.left, rect.top, rect.right,      rect.bottom,
                radius,    shoulder, (float)edgeLeft, (float)edgeRight};
            if (key != m_notchShapeKey || !m_notchFill || !m_notchLine) {
                m_notchShapeKey = key;
                m_notchFill = NotchGeometry(m_d2d.get(), rect, radius, shoulder,
                                            edgeLeft, edgeRight, true);
                m_notchLine = NotchGeometry(m_d2d.get(), rect, radius, shoulder,
                                            edgeLeft, edgeRight, false);
            }
            notchFill = m_notchFill.get();
            notchLine = m_notchLine.get();
            if (liquid) {
                GlassShape shape;
                shape.rect = rect;
                shape.radius = radius;
                shape.shoulder = shoulder;
                shape.edgeLeft = edgeLeft;
                shape.edgeRight = edgeRight;
                shape.tint = Bg(GlassTint(LiquidTint()));
                shape.opacity = (1 - lineMix) * AppearFade();
                m_liquidShapes.push_back(shape);
            }
        } else if (liquid) {
            addLiquid(rect, radius, (1 - lineMix) * AppearFade());
        }
        // The line as see-through as chosen (islandLineOpacity), not shown
        // when it doesn't open the island (only the chosen notch's does,
        // #6468); the pill, a peeking app and a banner opening out of it as
        // usual.
        // (Faded away after a while, see islandLineFade.)
        const float lineOpacity =
            LineOpens()
                ? g_settings.islandLineOpacity / 100.0f * (float)m_lineFade
                : 0.0f;
        brush->SetOpacity(1 - lineMix * (1 - lineOpacity));
        // (Over the Liquid Glass, its own bright edge; only the line's.)
        const float edge =
            (overLiquid ? 0.0f : 0.10f * EdgeAmount()) + 0.3f * lineMix;
        if (notchFill && notchLine) {
            target->FillGeometry(notchFill, brush.get());
            brush->SetColor(Fg(edge));
            target->DrawGeometry(notchLine, brush.get(), 1);
        } else {
            target->FillRoundedRectangle({rect, radius, radius}, brush.get());
            brush->SetColor(Fg(edge));
            target->DrawRoundedRectangle(
                {{rect.left + 0.5f, rect.top + 0.5f, rect.right - 0.5f,
                  rect.bottom - 0.5f},
                 radius,
                 radius},
                brush.get(), 1);
        }
        brush->SetOpacity(1);
        // A download's progress or the battery charging: the pill filling
        // (not as its line, nor with a banner open).
        if (const float fill = (float)std::clamp(m_fillAmount, 0.0, 1.0) *
                               (1 - lineMix) *
                               (1 - std::clamp(banner * 3, 0.0f, 1.0f));
            fill > 0.01f) {
            DrawFill(target, brush.get(), rect, radius, notchFill, fill);
        }
        if (clock > 0.05f && peek < 0.05f && banner < 0.01f) {
            DrawHiddenPillContents(
                target, brush.get(), rect,
                std::clamp((clock - 0.3f) / 0.5f, 0.0f, 1.0f) * AppearFade(),
                scale);
        }
        if (peek > 0.05f && !m_peekApp.empty() && banner < 0.01f) {
            const float size = 18 * scale * std::clamp(peek, 0.0f, 1.1f);
            const float middleX = (rect.left + rect.right) / 2;
            const float middleY = (rect.top + rect.bottom) / 2;
            const D2D1_RECT_F icon{middleX - size / 2, middleY - size / 2,
                                   middleX + size / 2, middleY + size / 2};
            const float opacity = std::clamp((peek - 0.3f) / 0.5f, 0.0f, 1.0f);
            DrawAppIcon(target, brush.get(), m_peekApp, icon, opacity, false);
            const float dot = 3.5f * scale * std::clamp(peek, 0.0f, 1.0f);
            brush->SetColor(D2D1::ColorF(1.0f, 0.27f, 0.23f, opacity));
            target->FillEllipse({{icon.right - dot / 2, icon.top + dot / 2},
                                 dot,
                                 dot},
                                brush.get());
        }
        // The settings bubble: grows out of the island's edge (out of the
        // line when minimized), away while a banner opens.
        const float gear = GearAmount(NowSeconds()) *
                           (1 - std::clamp(banner * 3, 0.0f, 1.0f));
        if (gear > 0.001f && m_iconFormat) {
            const D2D1_RECT_F full = GearBubbleRect();
            D2D1_POINT_2F anchor{(full.left + full.right) / 2, full.top};
            if (m_vertical) {
                anchor = {m_sideLeft ? full.left : full.right,
                          (full.top + full.bottom) / 2};
            } else if (AtBottom()) {
                anchor.y = full.bottom;
            }
            const float k = std::clamp(gear, 0.0f, 1.1f);
            const D2D1_RECT_F bubble{anchor.x + (full.left - anchor.x) * k,
                                     anchor.y + (full.top - anchor.y) * k,
                                     anchor.x + (full.right - anchor.x) * k,
                                     anchor.y + (full.bottom - anchor.y) * k};
            const float opacity = std::clamp(gear * 1.6f, 0.0f, 1.0f);
            const float bubbleRadius = std::min(bubble.bottom - bubble.top,
                                                bubble.right - bubble.left) /
                                       2;
            D2D1_COLOR_F fill = BgSolid(1);
            fill.a *= opacity;
            brush->SetColor(fill);
            target->FillRoundedRectangle({bubble, bubbleRadius, bubbleRadius},
                                         brush.get());
            if (m_hover == IslandItem::SettingsBubble) {
                brush->SetColor(Fg(0.12f * opacity));
                GlassFill(target, {bubble, bubbleRadius, bubbleRadius},
                          brush.get());
            }
            brush->SetColor(Fg(0.10f * EdgeAmount() * opacity));
            target->DrawRoundedRectangle(
                {{bubble.left + 0.5f, bubble.top + 0.5f, bubble.right - 0.5f,
                  bubble.bottom - 0.5f},
                 bubbleRadius,
                 bubbleRadius},
                brush.get(), 1);
            brush->SetColor(Fg(0.9f * std::clamp((gear - 0.3f) / 0.5f, 0.0f,
                                                 1.0f)));
            if (m_gearText.empty()) {
                DrawGlyph(target, &kGlyphSettings, 1, m_iconFormat.get(),
                          bubble, brush.get());
            } else if (m_textFormat) {
                target->DrawText(m_gearText.c_str(), (UINT32)m_gearText.size(),
                                 m_textFormat.get(), bubble, brush.get(),
                                 D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
    }

    // The contents fade in near the end of expanding, and away as the banner
    // opens.
    // The contents come at the end of opening out of the dot.
    const float appearContents =
        m_bar ? 1.0f : std::clamp((AppearOpen() - 0.7f) / 0.3f, 0.0f, 1.0f);
    // (The Bar's, away as it closes into the clock.)
    const float contentOpacity =
        appearContents *
        (m_bar ? 1.0f - (float)std::clamp(m_hiddenClock * 3, 0.0, 1.0)
               : (float)std::clamp((m_expand - 0.6) / 0.4, 0.0, 1.0) *
                     std::clamp(1.0f - BannerMorph() * 3, 0.0f, 1.0f));

    // The banner's contents: the app's icon, its name and the time, the
    // title and the text.
    const float bannerContents =
        (float)std::clamp((m_bannerAmount - 0.45) / 0.55, 0.0, 1.0) *
        appearContents *
        (1 - (float)std::clamp(m_bannerSwap, 0.0, 1.0));
    if (bannerContents > 0.01f && !BannerPlaced()) {
        DrawBannerContents(target, brush.get(), bannerContents);
    }
    if (contentOpacity > 0.01f) {
        // A list icon cut at its edges is clipped to the list (see
        // TrayListSlot), until the end of its drawing.
        const IslandSlot* trayList = TrayListSlot();
        struct ListClip {
            ID2D1RenderTarget* target;
            bool on;
            ~ListClip() {
                if (on) {
                    target->PopAxisAlignedClip();
                }
            }
        };
        // A pill longer than its screen: its items are clipped to it, and
        // fade at an end with more beyond it.
        // (Inside its straight part: its round ends stay as they are.)
        const float pillEnd =
            std::min(m_pill.right - m_pill.left, m_pill.bottom - m_pill.top) /
            2;
        const D2D1_RECT_F pillView =
            m_vertical ? D2D1_RECT_F{m_pill.left, m_pill.top + pillEnd,
                                     m_pill.right, m_pill.bottom - pillEnd}
                       : D2D1_RECT_F{m_pill.left + pillEnd, m_pill.top,
                                     m_pill.right - pillEnd, m_pill.bottom};
        const bool pillClipped = m_pillOverflow > 0;
        if (pillClipped) {
            target->PushAxisAlignedClip(pillView, D2D1_ANTIALIAS_MODE_ALIASED);
        }
        for (const auto& slot : m_slots) {
            if (slot.font == kTrayListFont) {
                continue;
            }
            const bool clipped = slot.inList && trayList;
            if (clipped) {
                target->PushAxisAlignedClip(trayList->rect,
                                            D2D1_ANTIALIAS_MODE_ALIASED);
            }
            const ListClip listClip{target, clipped};
            // Fading (its group going to another place).
            if (slot.alpha < 0.01f) {
                continue;
            }
            struct SlotFade {
                ID2D1RenderTarget* target;
                bool on;
                ~SlotFade() {
                    if (on) {
                        target->PopLayer();
                    }
                }
            };
            winrt::com_ptr<ID2D1Layer> fadeLayer;
            if (slot.alpha < 0.999f) {
                target->CreateLayer(nullptr, fadeLayer.put());
                target->PushLayer(
                    D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                          D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                          D2D1::IdentityMatrix(), slot.alpha),
                    fadeLayer.get());
            }
            const SlotFade slotFade{target, slot.alpha < 0.999f};
            // (An app's or a tray icon's by its key, what's playing's by its
            // part.)
            // (komorebi's workspace dots each on its own too.)
            const bool byText = slot.item == IslandItem::App ||
                                slot.item == IslandItem::TrayIcon ||
                                slot.item == IslandItem::QuickAction ||
                                slot.item == IslandItem::KomorebiWorkspace;
            const bool byPart = slot.item == IslandItem::Media;
            const bool hovered =
                (slot.item == m_hover ||
                 (slot.item == IslandItem::TrayIcon &&
                  m_hover == IslandItem::TrayPin)) &&
                (byText   ? SlotKey(slot) == m_hoverApp
                 : byPart ? MediaSlotPart(slot) == m_hoverApp
                          : true);
            // Lighting up smoothly (by the item, wherever it moves).
            const float lit = m_hoverFades.Get(
                HoverKey((int)slot.item,
                         byText ? (long long)(std::hash<std::wstring>{}(
                                                  SlotKey(slot)) &
                                              0xFFFFFFFFFF)
                         : byPart ? (slot.font == 6 ? 1 : 2)
                                  : 0),
                hovered || (slot.item == IslandItem::Overflow && m_trayOpen));
            // An item drawn in parts side by side (the battery's or the
            // volume's icon and its %): one highlight over all of them,
            // drawn with the first part (two overlapped into a darker
            // middle).
            D2D1_RECT_F area = slot.rect;
            bool laterPart = false;
            if (lit > 0.01f && !byText && !byPart) {
                const float reach = std::round(8 * scale);
                for (const auto& other : m_slots) {
                    if (&other == &slot || other.item != slot.item ||
                        other.group != slot.group || other.alpha < 0.01f ||
                        other.font == kTrayListFont) {
                        continue;
                    }
                    const bool touches =
                        m_vertical ? other.rect.top <= area.bottom + reach &&
                                         other.rect.bottom >= area.top - reach
                                   : other.rect.left <= area.right + reach &&
                                         other.rect.right >= area.left - reach;
                    if (!touches) {
                        continue;
                    }
                    if (&other < &slot) {
                        laterPart = true;
                        break;
                    }
                    area = {std::min(area.left, other.rect.left),
                            std::min(area.top, other.rect.top),
                            std::max(area.right, other.rect.right),
                            std::max(area.bottom, other.rect.bottom)};
                }
            }
            if (lit > 0.01f && !laterPart && g_settings.islandHoverHighlight) {
                const float inset = std::round(4 * scale);
                const D2D1_RECT_F rect =
                    m_vertical
                        ? D2D1_RECT_F{area.left + inset, area.top - inset / 2,
                                      area.right - inset,
                                      area.bottom + inset / 2}
                        : D2D1_RECT_F{area.left - inset / 2, area.top + inset,
                                      area.right + inset / 2,
                                      area.bottom - inset};
                const float radius = (rect.bottom - rect.top) / 2;
                brush->SetColor(Fg(0.16f * contentOpacity * lit));
                GlassFill(target, {rect, radius, radius}, brush.get());
            }
            if (slot.item == IslandItem::Overflow && m_iconFormat) {
                // ‹ turning into › as the list opens (into ⌄ for a list of
                // their own, see AnimationFrame).
                const D2D1_POINT_2F center{
                    (slot.rect.left + slot.rect.right) / 2,
                    (slot.rect.top + slot.rect.bottom) / 2};
                // Pointing up when upright, down once open.
                target->SetTransform(D2D1::Matrix3x2F::Rotation(
                    180 - (float)(m_trayArrow * 180) +
                        (m_vertical ? 90.0f : 0.0f),
                    center));
                brush->SetColor(Fg(0.95f * contentOpacity));
                DrawGlyph(target, slot.text.c_str(), (UINT32)slot.text.size(),
                          m_smallIconFormat.get(), slot.rect,
                          brush.get());
                target->SetTransform(D2D1::Matrix3x2F::Identity());
                // A dot under it while it's kept open.
                if (m_trayKeepOpen) {
                    const float dot = 1.6f * scale;
                    brush->SetColor(
                        Fg(0.7f * contentOpacity));
                    target->FillEllipse(
                        {{center.x, slot.rect.bottom - 5 * scale}, dot, dot},
                        brush.get());
                }
                continue;
            }
            if (slot.font == 6) {
                // What's playing: the note turning into the cover, and the
                // title next to it.
                const float shown =
                    contentOpacity * std::clamp(slot.scale, 0.0f, 1.0f);
                const float title = (float)std::clamp(m_mediaTitleRoom, 0.0, 1.0);
                const float press =
                    m_press.target == (int)IslandItem::Media
                        ? (float)m_press.Amount(NowSeconds())
                        : 0.0f;
                const float size = std::round(kIslandPictureSize * iconScale) *
                                   (1 - 0.15f * std::clamp(press, -0.3f, 1.0f));
                const float middle = (slot.rect.top + slot.rect.bottom) / 2;
                // Where the cover sits: from the note's middle to the left.
                const float noteCenter =
                    slot.rect.left + std::round(kIslandIconSlot * iconScale) / 2;
                const float coverCenter =
                    noteCenter + (slot.rect.left + std::round(2 * scale) +
                                  size / 2 - noteCenter) *
                                     title;
                const D2D1_RECT_F cover{coverCenter - size / 2, middle - size / 2,
                                        coverCenter + size / 2,
                                        middle + size / 2};
                target->PushAxisAlignedClip(slot.rect,
                                            D2D1_ANTIALIAS_MODE_ALIASED);
                ID2D1Bitmap* bitmap = MediaCover();
                // Rounded, filled keeping its proportions.
                auto drawCover = [&](ID2D1Bitmap* image, float alpha) {
                    if (!image || alpha <= 0.004f) {
                        return;
                    }
                    winrt::com_ptr<ID2D1RoundedRectangleGeometry> clip;
                    m_d2d->CreateRoundedRectangleGeometry(
                        {cover, std::round(4 * scale), std::round(4 * scale)},
                        clip.put());
                    winrt::com_ptr<ID2D1Layer> layer;
                    target->CreateLayer(nullptr, layer.put());
                    target->PushLayer(
                        D2D1::LayerParameters(D2D1::InfiniteRect(), clip.get(),
                                              D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                              D2D1::IdentityMatrix(), alpha),
                        layer.get());
                    const D2D1_SIZE_F bitmapSize = image->GetSize();
                    const float fill = std::max(
                        size / std::max(bitmapSize.width, 1.0f),
                        size / std::max(bitmapSize.height, 1.0f));
                    const float w = bitmapSize.width * fill;
                    const float h = bitmapSize.height * fill;
                    DrawBitmapSmooth(target, image,
                                     {coverCenter - w / 2, middle - h / 2,
                                      coverCenter + w / 2, middle + h / 2});
                    target->PopLayer();
                };
                const float coverSwap =
                    m_mediaCoverBefore
                        ? (float)std::clamp(m_mediaCoverSwap, 0.0, 1.0)
                        : 0.0f;
                if (title > 0.01f) {
                    drawCover(m_mediaCoverBefore.get(),
                              shown * title * coverSwap);
                    drawCover(bitmap, shown * title * (1 - coverSwap));
                }
                // The note, fading as the cover comes (or staying, without a
                // cover).
                const float note = bitmap ? 1 - title : 1.0f;
                if (note > 0.01f && m_iconFormat) {
                    brush->SetColor(Fg(0.95f * shown * note));
                    DrawGlyph(target, &kGlyphMusic, 1, m_iconFormat.get(),
                              cover, brush.get());
                }
                if (title > 0.01f && m_mediaTitleFormat) {
                    const float textLeft = slot.rect.left + std::round(2 * scale) +
                                           size + std::round(8 * scale);
                    const float textRight = std::max(
                        textLeft, slot.rect.right - std::round(4 * scale));
                    // The title before goes up and away as the new one comes
                    // up from below.
                    const float swap =
                        (float)std::clamp(m_mediaSwap, 0.0, 1.0);
                    const float rise = std::round(10 * scale);
                    if (swap > 0.004f && !m_mediaTitleBefore.empty()) {
                        brush->SetColor(Fg(0.92f * shown * title * swap));
                        target->DrawText(
                            m_mediaTitleBefore.c_str(),
                            (UINT32)m_mediaTitleBefore.size(),
                            m_mediaTitleFormat.get(),
                            {textLeft, slot.rect.top - rise * (1 - swap),
                             std::max(textRight,
                                      textLeft + (float)m_mediaTitleWidth),
                             slot.rect.bottom - rise * (1 - swap)},
                            brush.get());
                    }
                    brush->SetColor(Fg(0.92f * shown * title * (1 - swap)));
                    // Too long, while it plays (see MediaTitleScrolls): it
                    // scrolls to its end and back.
                    winrt::com_ptr<IDWriteTextLayout> scrolling;
                    if (MediaTitleScrolls() && swap <= 0.004f &&
                        title > 0.99f &&
                        m_mediaTitleFullWidth > textRight - textLeft + 1 &&
                        SUCCEEDED(m_dwrite->CreateTextLayout(
                            slot.text.c_str(), (UINT32)slot.text.size(),
                            m_mediaTitleFormat.get(), 10000,
                            slot.rect.bottom - slot.rect.top,
                            scrolling.put()))) {
                        m_mediaMarquee = true;
                        scrolling->SetTextAlignment(
                            DWRITE_TEXT_ALIGNMENT_LEADING);
                        m_mediaMarqueeDistance =
                            m_mediaTitleFullWidth - (textRight - textLeft);
                        const float shift = (float)MarqueeShift(
                            NowSeconds() - m_mediaMarqueeStart,
                            m_mediaMarqueeDistance, scale);
                        target->PushAxisAlignedClip(
                            {textLeft, slot.rect.top, textRight,
                             slot.rect.bottom},
                            D2D1_ANTIALIAS_MODE_ALIASED);
                        // (Not snapped to whole pixels: a slow scroll
                        // would move in jerks.)
                        target->DrawTextLayout(
                            {textLeft - shift, slot.rect.top}, scrolling.get(),
                            brush.get(), D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);
                        target->PopAxisAlignedClip();
                    } else {
                        target->DrawText(
                            slot.text.c_str(), (UINT32)slot.text.size(),
                            m_mediaTitleFormat.get(),
                            {textLeft, slot.rect.top + rise * swap, textRight,
                             slot.rect.bottom + rise * swap},
                            brush.get());
                    }
                }
                // Several apps with media, switched between: a dot each under
                // the cover for a moment, the one shown lit.
                const int sources = (int)m_mediaShown.sources.size();
                const float dotsShown =
                    title * (float)std::clamp(m_mediaDots, 0.0, 1.0);
                if (sources > 1 && dotsShown > 0.01f) {
                    const float dot = std::max(1.0f, 1.1f * scale);
                    const float step = std::round(4 * scale);
                    const float dotsY = std::min(
                        cover.bottom + std::round(3 * scale),
                        slot.rect.bottom - dot - 1);
                    const float dotsLeft =
                        coverCenter - step * (std::min(sources, 5) - 1) / 2.0f;
                    for (int i = 0; i < std::min(sources, 5); i++) {
                        brush->SetColor(
                            Fg((i == m_mediaShown.sourceIndex ? 0.9f : 0.35f) *
                               shown * dotsShown));
                        target->FillEllipse(
                            {{dotsLeft + step * i, dotsY}, dot, dot},
                            brush.get());
                    }
                }
                target->PopAxisAlignedClip();
                continue;
            }
            if (slot.font == kEyesFont) {
                DrawEyes(target, m_d2d.get(), brush.get(), slot.rect, m_eyes,
                         ChosenEyeTheme(), ChosenEyeShape(),
                         contentOpacity * std::clamp(slot.scale, 0.0f, 1.0f),
                         NowSeconds());
                continue;
            }
            if (slot.font == kVisualizerFont) {
                // Rounded bars from the middle, as high as the sound.
                const float shown =
                    contentOpacity * std::clamp(slot.scale, 0.0f, 1.0f);
                const int count = (int)m_vizLevels.size();
                const float bar = std::max(2.0f, std::round(2.5f * iconScale));
                const float gap = std::max(1.0f, std::round(1.5f * iconScale));
                const float most = std::round(15 * iconScale);
                const float least = bar;
                const float total = count * bar + (count - 1) * gap;
                const float middle = (slot.rect.top + slot.rect.bottom) / 2;
                float left = (slot.rect.left + slot.rect.right - total) / 2;
                // In the color chosen; the rainbow: a hue a bar, the whole
                // going around slowly.
                const float alpha = FgIcon(0.95f * shown).a;
                const int color = g_settings.islandVisualizerColor;
                const double turn = NowSeconds() * kRainbowTurns;
                brush->SetColor(color == 0 ? FgIcon(0.95f * shown)
                                           : RgbColor(VisualizerChoiceRgb(color),
                                                      alpha));
                for (int i = 0; i < count; i++) {
                    if (color == kVisualizerRainbow) {
                        brush->SetColor(RgbColor(
                            RainbowRgb(turn + (double)i / std::max(count, 1)),
                            alpha));
                    }
                    const float high = least + (most - least) * m_vizLevels[i];
                    const D2D1_RECT_F rect{left, middle - high / 2,
                                           left + bar, middle + high / 2};
                    target->FillRoundedRectangle({rect, bar / 2, bar / 2},
                                                 brush.get());
                    left += bar + gap;
                }
                continue;
            }
            if (slot.font == kLyricsFont && m_lyricsFormat) {
                // The new line comes up from below as the one before goes up
                // and away. Drawn as wide as the line (see m_lyricsWidth),
                // centered: coming in or going, its room uncovers or covers
                // it, fading, rather than squeezing it.
                // (The first line after a pause, or the last before one:
                // it just comes or goes with its room, without rising.)
                const float amount = std::clamp(slot.scale, 0.0f, 1.0f);
                const float shown = contentOpacity * amount * amount;
                const bool coming = m_lyricsBefore.empty();
                const bool going = m_lyricsLine.empty();
                const float t =
                    going    ? 0.0f
                    : coming ? 1.0f
                             : (float)std::clamp(
                                   (NowSeconds() - m_lyricsChangedAt) /
                                       std::max(AnimationSeconds(0.3), 0.001),
                                   0.0, 1.0);
                const float rise = std::round(10 * m_scale);
                const float center = (slot.rect.left + slot.rect.right) / 2;
                const float half =
                    std::max(slot.rect.right - slot.rect.left, m_lyricsWidth) /
                    2;
                target->PushAxisAlignedClip(slot.rect,
                                            D2D1_ANTIALIAS_MODE_ALIASED);
                if (t < 1 && !m_lyricsBefore.empty()) {
                    brush->SetColor(LyricsColor(0.95f * shown * (1 - t)));
                    target->DrawText(
                        m_lyricsBefore.c_str(), (UINT32)m_lyricsBefore.size(),
                        m_lyricsFormat.get(),
                        {center - half, slot.rect.top - rise * t,
                         center + half, slot.rect.bottom - rise * t},
                        brush.get());
                }
                if (!m_lyricsLine.empty()) {
                    brush->SetColor(LyricsColor(0.95f * shown * t));
                    target->DrawText(
                        m_lyricsLine.c_str(), (UINT32)m_lyricsLine.size(),
                        m_lyricsFormat.get(),
                        {center - half, slot.rect.top + rise * (1 - t),
                         center + half, slot.rect.bottom + rise * (1 - t)},
                        brush.get());
                }
                target->PopAxisAlignedClip();
                if (t < 1 && !going) {
                    StartAnimating();
                }
                continue;
            }
            if (slot.font == kStackedFont && m_usageLabelFormat &&
                m_usageValueFormat) {
                // A small dim label over its value (see StackedWidth).
                const size_t split = slot.text.find(L'\n');
                const std::wstring label = slot.text.substr(0, split);
                const std::wstring value = split == std::wstring::npos
                                               ? std::wstring()
                                               : slot.text.substr(split + 1);
                const float shown =
                    contentOpacity * std::clamp(slot.scale, 0.0f, 1.0f);
                const float middle = (slot.rect.top + slot.rect.bottom) / 2;
                const float labelHeight = std::round(9 * m_usageScale);
                const float valueHeight = std::round(13 * m_usageScale);
                const float top =
                    std::round(middle - (labelHeight + valueHeight) / 2);
                brush->SetColor(Fg(0.55f * shown));
                target->DrawText(label.c_str(), (UINT32)label.size(),
                                 m_usageLabelFormat.get(),
                                 {slot.rect.left, top, slot.rect.right,
                                  top + labelHeight},
                                 brush.get());
                // (A countdown's or the stopwatch's time: orange, green on
                // a break, like the iPhone's; paused, dimmer.)
                if (slot.item == IslandItem::Timer && slot.action >= 0 &&
                    slot.action < kTimerModes) {
                    const TimerMode mode = (TimerMode)slot.action;
                    const bool rest =
                        mode == TimerMode::Pomodoro && g_timer.onBreak;
                    brush->SetColor(RgbColor(
                        rest ? 0x30D158 : 0xFF9F0A,
                        (TimerClockOf(mode).running ? 0.95f : 0.6f) * shown));
                } else {
                    brush->SetColor(Fg(0.95f * shown));
                }
                target->DrawText(value.c_str(), (UINT32)value.size(),
                                 m_usageValueFormat.get(),
                                 {slot.rect.left, top + labelHeight,
                                  slot.rect.right,
                                  top + labelHeight + valueHeight},
                                 brush.get());
                continue;
            }
            if (slot.font == 7 && m_usageLabelFormat && m_usageValueFormat) {
                // A use: a small icon and its short name, dim, over its
                // value; a new value comes up from below as the one before
                // goes up and away.
                const int index = (int)slot.item - (int)IslandItem::UsageCpu;
                static constexpr WCHAR kUsageGlyphs[3] = {0xE950, 0xE964,
                                                          0xE7F4};
                const size_t split = slot.text.find(L'\n');
                const std::wstring name =
                    std::wstring(1, kUsageGlyphs[index]) + L" " +
                    slot.text.substr(0, split);
                const std::wstring value = split == std::wstring::npos
                                               ? std::wstring()
                                               : slot.text.substr(split + 1);
                const float shown =
                    contentOpacity * std::clamp(slot.scale, 0.0f, 1.0f);
                const float middle = (slot.rect.top + slot.rect.bottom) / 2;
                const float nameHeight = std::round(9 * m_usageScale);
                const float valueHeight = std::round(13 * m_usageScale);
                const float top =
                    std::round(middle - (nameHeight + valueHeight) / 2);
                const float width = slot.rect.right - slot.rect.left;
                winrt::com_ptr<IDWriteTextLayout> nameLayout;
                if (SUCCEEDED(m_dwrite->CreateTextLayout(
                        name.c_str(), (UINT32)name.size(),
                        m_usageLabelFormat.get(), width, nameHeight,
                        nameLayout.put()))) {
                    nameLayout->SetFontFamilyName(IconFont(),
                                                  {0, 1});
                    nameLayout->SetFontSize(7 * m_usageScale, {0, 1});
                    brush->SetColor(Fg(0.55f * shown));
                    target->DrawTextLayout({slot.rect.left, top},
                                           nameLayout.get(), brush.get());
                }
                const D2D1_RECT_F valueRect{slot.rect.left, top + nameHeight,
                                            slot.rect.right,
                                            top + nameHeight + valueHeight};
                target->PushAxisAlignedClip(
                    {valueRect.left, valueRect.top - 1, valueRect.right,
                     valueRect.bottom + 1},
                    D2D1_ANTIALIAS_MODE_ALIASED);
                auto drawValue = [&](const std::wstring& text, float dy,
                                     float opacity) {
                    winrt::com_ptr<IDWriteTextLayout> layout;
                    if (opacity <= 0.01f ||
                        FAILED(m_dwrite->CreateTextLayout(
                            text.c_str(), (UINT32)text.size(),
                            m_usageValueFormat.get(), width, valueHeight,
                            layout.put()))) {
                        return;
                    }
                    if (m_tabular) {
                        layout->SetTypography(m_tabular.get(),
                                              {0, (UINT32)text.size()});
                    }
                    brush->SetColor(Fg(0.95f * shown * opacity));
                    target->DrawTextLayout({valueRect.left, valueRect.top + dy},
                                           layout.get(), brush.get());
                };
                // (It counts to a new value: see UsageText.)
                drawValue(value, 0, 1);
                target->PopAxisAlignedClip();
                continue;
            }
            if (slot.font == kNamedItemFont && m_itemNameFormat) {
                // komorebi's grid, the desktop's glyph or the app's own icon,
                // then the name (upright, the icon alone); another one comes
                // up from below as the one before slides up and out.
                const bool komorebi = slot.item == IslandItem::KomorebiLayout;
                const ItemWidth& width = komorebi ? m_komorebiWidth
                                                  : m_focusWidth;
                const float shown =
                    contentOpacity * std::clamp(slot.scale, 0.0f, 1.0f);
                const float iconWidth = std::round(22 * scale);
                const float iconLeft =
                    m_vertical
                        ? (slot.rect.left + slot.rect.right - iconWidth) / 2
                        : slot.rect.left;
                auto drawNamed = [&](const std::wstring& iconKey,
                                     const std::wstring& name, float alpha,
                                     float shift) {
                    if (alpha <= 0.004f) {
                        return;
                    }
                    const D2D1_RECT_F icon{iconLeft, slot.rect.top + shift,
                                           iconLeft + iconWidth,
                                           slot.rect.bottom + shift};
                    if (iconKey.starts_with(L'\x1')) {
                        brush->SetColor(FgIcon(0.95f * alpha));
                        DrawKomorebiIcon(
                            target, brush.get(),
                            {(icon.left + icon.right) / 2,
                             (icon.top + icon.bottom) / 2},
                            iconScale, _wtoi(iconKey.c_str() + 1));
                    } else if (!iconKey.empty()) {
                        const float size = kIslandPictureSize * iconScale;
                        const float middleX = (icon.left + icon.right) / 2;
                        const float middleY = (icon.top + icon.bottom) / 2;
                        DrawAppIcon(target, brush.get(), iconKey,
                                    {middleX - size / 2, middleY - size / 2,
                                     middleX + size / 2, middleY + size / 2},
                                    alpha, true);
                    } else if (m_smallIconFormat) {
                        const WCHAR glyph =
                            komorebi ? kGlyphKomorebiLayout : kGlyphDesktop;
                        brush->SetColor(FgIcon(0.95f * alpha));
                        DrawGlyph(target, &glyph, 1, m_smallIconFormat.get(),
                                  icon, brush.get());
                    }
                    if (!m_vertical) {
                        brush->SetColor(Fg(0.95f * alpha));
                        target->DrawText(
                            name.c_str(), (UINT32)name.size(),
                            m_itemNameFormat.get(),
                            {icon.right, icon.top,
                             slot.rect.right - std::round(4 * scale),
                             icon.bottom},
                            brush.get());
                    }
                };
                const float swap = (float)std::clamp(width.swap, 0.0, 1.0);
                if (swap > 0.004f && !width.nameBefore.empty()) {
                    drawNamed(width.iconBefore, width.nameBefore,
                              shown * swap * swap, -(1 - swap) * 8 * scale);
                }
                drawNamed(width.name.empty() ? std::wstring() : width.icon,
                          width.name.empty() ? slot.text : width.name,
                          shown * (1 - swap * swap), swap * 8 * scale);
                continue;
            }
            if (slot.font == kWorkspaceDotFont) {
                // A workspace's dot: lit for the one shown, faint for one
                // with windows, a ring for an empty one; brighter under the
                // mouse.
                const float shown =
                    contentOpacity * std::clamp(slot.scale, 0.0f, 1.0f);
                const bool lit = slot.text.ends_with(L"*");
                const bool windows = slot.text.ends_with(L"+");
                const bool hover = m_hover == IslandItem::KomorebiWorkspace &&
                                   m_hoverApp == slot.text;
                const D2D1_ELLIPSE dot{{(slot.rect.left + slot.rect.right) / 2,
                                        (slot.rect.top + slot.rect.bottom) / 2},
                                       3.5f * scale,
                                       3.5f * scale};
                if (lit || windows) {
                    brush->SetColor(Fg((lit ? 0.95f : hover ? 0.65f : 0.45f) *
                                       shown));
                    target->FillEllipse(dot, brush.get());
                } else {
                    brush->SetColor(Fg((hover ? 0.65f : 0.4f) * shown));
                    target->DrawEllipse(
                        {dot.point, dot.radiusX - 0.6f * scale,
                         dot.radiusY - 0.6f * scale},
                        brush.get(), 1.2f * scale);
                }
                continue;
            }
            if (slot.font == 9 && m_textFormat) {
                const size_t tab = slot.text.find(L'\t');
                const std::wstring emoji = slot.text.substr(0, tab);
                const std::wstring temperature =
                    tab == std::wstring::npos ? std::wstring()
                                              : slot.text.substr(tab + 1);
                const float shown =
                    contentOpacity * std::clamp(slot.scale, 0.0f, 1.0f);
                if (!m_weatherEmojiFormat ||
                    m_weatherEmojiScale != m_textScale) {
                    m_weatherEmojiScale = m_textScale;
                    m_weatherEmojiFormat = nullptr;
                    m_dwrite->CreateTextFormat(
                        L"Segoe UI Emoji", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                        13 * m_textScale, L"", m_weatherEmojiFormat.put());
                    if (m_weatherEmojiFormat) {
                        m_weatherEmojiFormat->SetTextAlignment(
                            DWRITE_TEXT_ALIGNMENT_CENTER);
                        m_weatherEmojiFormat->SetParagraphAlignment(
                            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                    }
                }
                const float emojiWidth = std::round(20 * m_textScale);
                // The weather before fading out, sliding up, as this one
                // comes up from below.
                const float swap =
                    (float)std::clamp(m_weatherSwap, 0.0, 1.0);
                auto drawWeather = [&](const std::wstring& emojiText,
                                       const std::wstring& degrees,
                                       float alpha, float shift) {
                    if (alpha <= 0.004f) {
                        return;
                    }
                    if (m_weatherEmojiFormat) {
                        brush->SetColor(Fg(alpha));
                        target->DrawText(
                            emojiText.c_str(), (UINT32)emojiText.size(),
                            m_weatherEmojiFormat.get(),
                            {slot.rect.left, slot.rect.top + shift,
                             slot.rect.left + emojiWidth,
                             slot.rect.bottom + shift},
                            brush.get(),
                            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
                    }
                    brush->SetColor(Fg(0.95f * alpha));
                    target->DrawText(degrees.c_str(), (UINT32)degrees.size(),
                                     m_textFormat.get(),
                                     {slot.rect.left + emojiWidth,
                                      slot.rect.top + shift, slot.rect.right,
                                      slot.rect.bottom + shift},
                                     brush.get(),
                                     D2D1_DRAW_TEXT_OPTIONS_CLIP);
                };
                if (swap > 0.004f && !m_weatherBefore.empty()) {
                    const size_t beforeTab = m_weatherBefore.find(L'\t');
                    drawWeather(
                        m_weatherBefore.substr(0, beforeTab),
                        beforeTab == std::wstring::npos
                            ? std::wstring()
                            : m_weatherBefore.substr(beforeTab + 1),
                        shown * swap * swap, -(1 - swap) * 8 * scale);
                }
                drawWeather(emoji, temperature, shown * (1 - swap * swap),
                            swap * 8 * scale);
                continue;
            }
            if (slot.font == 8 && m_iconFormat) {
                // Light and dark mode: a half-filled circle, turning half a
                // turn as it's switched (and pressed in like the others).
                const D2D1_POINT_2F center{
                    (slot.rect.left + slot.rect.right) / 2,
                    (slot.rect.top + slot.rect.bottom) / 2};
                float size = std::max(slot.scale, 0.0f);
                if (m_press.target == (int)IslandItem::ThemeSwitch) {
                    size *= 1 - 0.24f * (float)std::clamp(
                                            m_press.Amount(NowSeconds()), -0.3,
                                            1.0);
                }
                target->SetTransform(
                    D2D1::Matrix3x2F::Scale(size, size, center) *
                    D2D1::Matrix3x2F::Rotation((float)m_themeTurn * 180,
                                               center));
                const float full = std::round(kIslandIconSlot * iconScale);
                const WCHAR glyph = 0xE7A1;
                brush->SetColor(FgIcon(0.95f * contentOpacity *
                                       std::clamp(slot.scale, 0.0f, 1.0f)));
                DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                          {center.x - full / 2, slot.rect.top,
                           center.x + full / 2, slot.rect.bottom},
                          brush.get());
                target->SetTransform(D2D1::Matrix3x2F::Identity());
                continue;
            }
            if (slot.font == 5) {
                // The line between the open list and the pinned icons.
                const float lineX = std::round((slot.rect.left + slot.rect.right) / 2);
                const float half = std::round(8 * scale);
                const float middle = std::round((slot.rect.top + slot.rect.bottom) / 2);
                brush->SetColor(
                    Fg(0.2f * contentOpacity * std::clamp(slot.scale, 0.0f, 1.0f)));
                // Across the pill, whichever way it stands.
                target->FillRectangle(
                    m_vertical ? D2D1_RECT_F{lineX - half, middle - 0.5f,
                                             lineX + half, middle + 0.5f}
                               : D2D1_RECT_F{lineX - 0.5f, middle - half,
                                             lineX + 0.5f, middle + half},
                    brush.get());
                continue;
            }
            if (slot.font == 4) {
                const TrayIcon* icon = FindTrayIcon(slot.text);
                const float shown = std::max(slot.scale, 0.0f);
                const float size = kIslandPictureSize * iconScale * shown;
                const float centerX = (slot.rect.left + slot.rect.right) / 2;
                const float centerY = (slot.rect.top + slot.rect.bottom) / 2;
                const D2D1_RECT_F rect{centerX - size / 2, centerY - size / 2,
                                       centerX + size / 2, centerY + size / 2};
                float opacity = contentOpacity * std::clamp(shown, 0.0f, 1.0f);
                // The one dragged: faint where it was.
                if (g_trayDrag.moved && slot.text == g_trayDrag.key) {
                    opacity *= 0.3f;
                }
                // Fading towards an edge with more icons past it.
                if (slot.inList && trayList) {
                    const float center = m_vertical ? centerY : centerX;
                    const float start =
                        m_vertical ? trayList->rect.top : trayList->rect.left;
                    const float end = m_vertical ? trayList->rect.bottom
                                                 : trayList->rect.right;
                    const float fade = kIslandIconSlot * iconScale * 1.2f;
                    if (m_trayMoreBefore) {
                        opacity *= std::clamp((center - start) / fade, 0.15f,
                                              1.0f);
                    }
                    if (m_trayMoreAfter) {
                        opacity *=
                            std::clamp((end - center) / fade, 0.15f, 1.0f);
                    }
                }
                if (ID2D1Bitmap* bitmap = icon ? GetTrayBitmap(*icon) : nullptr) {
                    DrawBitmapSmooth(target, bitmap, rect, opacity);
                }
                // The pin, on the icon under the mouse after a moment
                // (growing in): filled when pinned.
                const float pinShown =
                    hovered && slot.text == m_pinShownKey
                        ? PinShown(NowSeconds())
                        : 0.0f;
                if (pinShown > 0.01f && opacity > 0.5f) {
                    const D2D1_RECT_F fullPin = TrayPinRect(slot.rect);
                    const D2D1_POINT_2F pinCenter{
                        (fullPin.left + fullPin.right) / 2,
                        (fullPin.top + fullPin.bottom) / 2};
                    const float pinRadius =
                        (fullPin.right - fullPin.left) / 2 * pinShown;
                    const bool pinned = IsTrayPinned(slot.text);
                    brush->SetColor(
                        pinned ? Accent(opacity)
                               : D2D1::ColorF(
                                     0.25f, 0.25f, 0.25f,
                                     (m_hover == IslandItem::TrayPin ? 1.0f
                                                                     : 0.9f) *
                                         opacity));
                    target->FillEllipse({pinCenter, pinRadius, pinRadius},
                                        brush.get());
                    // The glyph grows with it.
                    target->SetTransform(D2D1::Matrix3x2F::Scale(
                        pinShown, pinShown, pinCenter));
                    brush->SetColor(Fg(opacity));
                    DrawGlyph(target, &kGlyphPin, 1, m_smallIconFormat.get(),
                              fullPin, brush.get());
                    target->SetTransform(D2D1::Matrix3x2F::Identity());
                }
                continue;
            }
            if (slot.font == kAppIconFont) {
                const float size = kIslandPictureSize * iconScale;
                const float centerX = (slot.rect.left + slot.rect.right) / 2;
                const float centerY = (slot.rect.top + slot.rect.bottom) / 2;
                DrawAppIcon(target, brush.get(), slot.text,
                            {centerX - size / 2, centerY - size / 2,
                             centerX + size / 2, centerY + size / 2},
                            contentOpacity, true);
                continue;
            }
            if (slot.font == 3) {
                const float shown = std::max(slot.scale, 0.0f);
                const float size = kIslandPictureSize * iconScale * shown;
                const float centerX = (slot.rect.left + slot.rect.right) / 2;
                const float centerY = (slot.rect.top + slot.rect.bottom) / 2;
                const D2D1_RECT_F icon{centerX - size / 2, centerY - size / 2,
                                       centerX + size / 2, centerY + size / 2};
                const float opacity =
                    contentOpacity * std::clamp(shown, 0.0f, 1.0f);
                DrawAppIcon(target, brush.get(), slot.text, icon, opacity,
                            true);
                const int count = AppCount(slot.text);
                if (count > 0 && m_badgeFormat) {
                    // A red bubble with the count, over the icon's corner;
                    // the number white, or black on the light theme.
                    const std::wstring number =
                        count > 99 ? L"99+" : std::to_wstring(count);
                    const float bubbleHeight = 12 * scale * shown;
                    const float bubbleWidth =
                        std::max(bubbleHeight,
                                 (number.size() * 5.5f + 5) * scale * shown);
                    const D2D1_RECT_F bubble{
                        icon.right - bubbleWidth * 0.6f,
                        icon.top - bubbleHeight * 0.35f,
                        icon.right + bubbleWidth * 0.4f,
                        icon.top + bubbleHeight * 0.65f};
                    brush->SetColor(D2D1::ColorF(1.0f, 0.27f, 0.23f, opacity));
                    target->FillRoundedRectangle(
                        {bubble, bubbleHeight / 2, bubbleHeight / 2},
                        brush.get());
                    const float ink = g_theme.light > 0.5f ? 0.0f : 1.0f;
                    brush->SetColor(D2D1::ColorF(ink, ink, ink, opacity));
                    if (shown > 0.6f) {
                        DrawInkCentered(m_dwrite.get(), target, brush.get(),
                                        number, m_badgeFormat.get(), bubble);
                    }
                    continue;
                }
                const float dot = 3.5f * scale * shown;
                brush->SetColor(D2D1::ColorF(1.0f, 0.27f, 0.23f, opacity));
                target->FillEllipse(
                    {{icon.right - dot / 2, icon.top + dot / 2}, dot, dot},
                    brush.get());
                continue;
            }
            if (slot.item == IslandItem::QuickAction && slot.action >= 0 &&
                kPillActions[slot.action].id == ControlId::Network) {
                DrawNetwork(target, brush.get(), slot, contentOpacity);
                continue;
            }
            if (slot.item == IslandItem::InputMethod && m_textFormat) {
                // (One line, upright too.)
                const float shown = std::clamp(slot.scale, 0.0f, 1.0f);
                brush->SetColor(Fg(0.95f * contentOpacity * shown));
                target->DrawText(slot.text.c_str(), (UINT32)slot.text.size(),
                                 m_textFormat.get(), slot.rect, brush.get());
                continue;
            }
            if (slot.font == kDesktopsFont) {
                const float shown = std::clamp(slot.scale, 0.0f, 1.0f);
                const float alpha = contentOpacity * shown;
                const int count = (int)m_desktops.names.size();
                const float middleY = (slot.rect.top + slot.rect.bottom) / 2;
                if (count > 9 && m_textFormat) {
                    const std::wstring text =
                        std::to_wstring(m_desktops.current + 1) + L"/" +
                        std::to_wstring(count);
                    brush->SetColor(Fg(0.95f * alpha));
                    target->DrawText(text.c_str(), (UINT32)text.size(),
                                     m_textFormat.get(), slot.rect,
                                     brush.get());
                    continue;
                }
                // The current one longer and brighter, like the iPhone's
                // pages.
                float dotX = slot.rect.left + 4 * scale;
                for (int i = 0; i < count; i++) {
                    const bool current = i == m_desktops.current;
                    const float length = (current ? 10.0f : 4.0f) * scale;
                    const float thick = 4 * scale;
                    brush->SetColor(FgIcon((current ? 0.95f : 0.38f) * alpha));
                    target->FillRoundedRectangle(
                        {{dotX, middleY - thick / 2, dotX + length,
                          middleY + thick / 2},
                         thick / 2,
                         thick / 2},
                        brush.get());
                    dotX += length + 4 * scale;
                }
                continue;
            }
            if (IsPrivacyItem(slot.item) && m_iconFormat) {
                // In use: the icon in macOS's colors (an orange microphone,
                // a green camera, a blue location), at the pill icons' size,
                // on a faint glow of its color; when the camera starts, soft
                // rings spread out of it a few times.
                const float shown = std::max(slot.scale, 0.0f);
                const float alpha =
                    contentOpacity * std::clamp(shown, 0.0f, 1.0f);
                const D2D1_COLOR_F color =
                    slot.item == IslandItem::PrivacyMicrophone
                        ? D2D1::ColorF(1.0f, 0.62f, 0.04f)
                    : slot.item == IslandItem::PrivacyCamera
                        ? D2D1::ColorF(0.19f, 0.82f, 0.35f)
                        : D2D1::ColorF(0.04f, 0.52f, 1.0f);
                const D2D1_POINT_2F center{
                    (slot.rect.left + slot.rect.right) / 2,
                    (slot.rect.top + slot.rect.bottom) / 2};
                const float radius = 8 * scale * shown;
                if (slot.item == IslandItem::PrivacyCamera && m_cameraSince) {
                    const double t = NowSeconds() - m_cameraSince;
                    if (t < kCameraBlinkSeconds) {
                        // Each ring comes out of the glow (fading in, so it
                        // never pops), slows down as it grows, and fades
                        // away before the next one.
                        const float ring =
                            (float)std::fmod(t, kCameraRingSeconds) /
                            (float)kCameraRingSeconds;
                        const float eased = 1 - std::pow(1 - ring, 3.0f);
                        const float ringRadius = radius * (1 + 0.75f * eased);
                        const float fade = std::min(1.0f, ring * 5) *
                                           (1 - ring) * (1 - ring);
                        brush->SetColor({color.r, color.g, color.b,
                                         0.7f * fade * alpha});
                        target->DrawEllipse({center, ringRadius, ringRadius},
                                            brush.get(), 1.5f * scale);
                    }
                }
                brush->SetColor({color.r, color.g, color.b, 0.2f * alpha});
                target->FillEllipse({center, radius * 1.4f, radius * 1.4f},
                                    brush.get());
                brush->SetColor({color.r, color.g, color.b, alpha});
                DrawGlyph(target, slot.text.c_str(), (UINT32)slot.text.size(),
                          m_iconFormat.get(), slot.rect, brush.get());
                continue;
            }
            auto* format =
                slot.font == 1 ? m_iconFormat.get() : m_textFormat.get();
            if (!format) {
                continue;
            }
            // Growing in or out, pressed, or play turning into pause.
            float glyphScale = std::max(slot.scale, 0.0f);
            if (m_press.target && (int)slot.item == m_press.target) {
                glyphScale *=
                    1 - 0.24f * (float)std::clamp(
                                    m_press.Amount(NowSeconds()), -0.3, 1.0);
            }
            if (slot.item == IslandItem::MediaPlayPause) {
                glyphScale *=
                    1 - 0.55f * (float)std::clamp(m_playSwap, 0.0, 1.0);
            }
            // The battery's glyph is low and wide, smaller-looking than the
            // others: a bit bigger, as big as them (at every icon size).
            if (slot.item == IslandItem::Battery && slot.font == 1) {
                glyphScale *= 1.2f;
            }
            float glyphOpacity = std::clamp(slot.scale, 0.0f, 1.0f);
            // Another icon for it (see m_glyphSwaps): the one before shrinks
            // and fades out as this one grows in.
            if (slot.font == 1) {
                for (const auto& swap : m_glyphSwaps) {
                    if (swap.item != slot.item || swap.t <= 0.004 ||
                        swap.from.empty()) {
                        continue;
                    }
                    const float t = (float)std::clamp(swap.t, 0.0, 1.0);
                    const D2D1_POINT_2F middle{
                        (slot.rect.left + slot.rect.right) / 2,
                        (slot.rect.top + slot.rect.bottom) / 2};
                    const float full = std::round(kIslandIconSlot * iconScale);
                    const float beforeScale =
                        std::max(glyphScale, 0.0f) * (0.6f + 0.4f * t);
                    target->SetTransform(D2D1::Matrix3x2F::Scale(
                        beforeScale, beforeScale, middle));
                    brush->SetColor(
                        FgIcon(0.95f * contentOpacity * glyphOpacity * t * t));
                    DrawGlyph(target, swap.from.c_str(),
                              (UINT32)swap.from.size(), format,
                              {middle.x - full / 2, slot.rect.top,
                               middle.x + full / 2, slot.rect.bottom},
                              brush.get());
                    target->SetTransform(D2D1::Matrix3x2F::Identity());
                    glyphScale *= 1 - 0.4f * t;
                    glyphOpacity *= 1 - t * t;
                }
            }
            const bool scaled = std::fabs(glyphScale - 1) > 0.001f;
            if (scaled) {
                target->SetTransform(D2D1::Matrix3x2F::Scale(
                    glyphScale, glyphScale,
                    {(slot.rect.left + slot.rect.right) / 2,
                     (slot.rect.top + slot.rect.bottom) / 2}));
            }
            // (Icons in their own color, see FgIcon.)
            auto* ink = slot.font == 1 ? FgIcon : Fg;
            brush->SetColor(
                ink((slot.item == IslandItem::Minimize ? 0.6f : 0.95f) *
                    contentOpacity * glyphOpacity));
            // Charging (its glyph with the bolt, see BatteryGlyph): green,
            // like a phone's.
            if (slot.item == IslandItem::Battery && !slot.text.empty() &&
                (slot.text[0] == 0xEA93 ||
                 (slot.text[0] >= 0xE85A && slot.text[0] <= 0xE863))) {
                brush->SetColor(
                    D2D1::ColorF(0.19f, 0.82f, 0.35f,
                                 0.95f * contentOpacity * glyphOpacity));
            }
            // An AI agent's use: yellow, then red as it fills.
            if (slot.item == IslandItem::AiUsage && slot.font == 0 &&
                slot.action >= 0 && slot.action < kAiAgents) {
                const long long now = UnixNow();
                const int rgb = AiUsageRgb(
                    AiUsedNow(AiTightLimit(g_aiUsage[slot.action], now), now));
                if (rgb >= 0) {
                    brush->SetColor(
                        RgbColor(rgb, 0.95f * contentOpacity * glyphOpacity));
                }
            }
            // A control center button whose thing is on: its glyph in the
            // accent color with a small dot under it, like the macOS menu
            // bar's (a fill behind it read as a white button).
            if (slot.item == IslandItem::QuickAction && slot.action >= 0 &&
                slot.action < (int)ARRAYSIZE(kPillActions) &&
                PillActionLit(kPillActions[slot.action].id)) {
                const float lit = contentOpacity * glyphOpacity;
                const float dot = std::max(1.0f, std::round(1.5f * scale));
                const float size = std::min(slot.rect.right - slot.rect.left,
                                            slot.rect.bottom - slot.rect.top);
                brush->SetColor(Accent(lit));
                target->FillEllipse(
                    {{(slot.rect.left + slot.rect.right) / 2,
                      (slot.rect.top + slot.rect.bottom) / 2 + size * 0.38f},
                     dot,
                     dot},
                    brush.get());
                brush->SetColor(Accent(0.95f * lit));
            }
            // Upright, the clock is two lines: the time, the date under it.
            if (slot.font == 0 && m_vertical && m_clockDateFormat) {
                const float middle = (slot.rect.top + slot.rect.bottom) / 2;
                const D2D1_RECT_F timeRect{slot.rect.left, slot.rect.top,
                                           slot.rect.right, middle + 2 * scale};
                const D2D1_RECT_F dateRect{slot.rect.left, middle,
                                           slot.rect.right, slot.rect.bottom};
                target->DrawText(m_clockTime.c_str(),
                                 (UINT32)m_clockTime.size(), format, timeRect,
                                 brush.get());
                brush->SetColor(Fg(0.6f * contentOpacity));
                target->DrawText(m_clockDate.c_str(),
                                 (UINT32)m_clockDate.size(),
                                 m_clockDateFormat.get(), dateRect,
                                 brush.get());
                if (scaled) {
                    target->SetTransform(D2D1::Matrix3x2F::Identity());
                }
                continue;
            }
            // Text with figures of equal width; icons as they are.
            if (slot.font == 0 && m_tabular) {
                // (Another set of parts: the one before fades out as this one
                // fades in.)
                const float swap =
                    slot.item == IslandItem::Clock && !m_clockBefore.empty()
                        ? (float)std::clamp(m_clockSwap, 0.0, 1.0)
                        : 0.0f;
                const D2D1_COLOR_F ink = brush->GetColor();
                auto drawText = [&](const std::wstring& text, float alpha) {
                    winrt::com_ptr<IDWriteTextLayout> layout;
                    if (alpha <= 0.004f ||
                        FAILED(m_dwrite->CreateTextLayout(
                            text.c_str(), (UINT32)text.size(), format,
                            slot.rect.right - slot.rect.left,
                            slot.rect.bottom - slot.rect.top, layout.put()))) {
                        return;
                    }
                    layout->SetTypography(m_tabular.get(),
                                          {0, (UINT32)text.size()});
                    if (slot.item == IslandItem::Clock) {
                        SmallClockMarker(layout.get(), text);
                    }
                    brush->SetColor(
                        D2D1::ColorF(ink.r, ink.g, ink.b, ink.a * alpha));
                    target->DrawTextLayout({slot.rect.left, slot.rect.top},
                                           layout.get(), brush.get());
                };
                if (swap > 0.004f) {
                    drawText(m_clockBefore, swap * swap);
                }
                drawText(slot.text, 1 - swap * swap);
                if (scaled) {
                    target->SetTransform(D2D1::Matrix3x2F::Identity());
                }
                continue;
            }
            // Icons growing in are drawn at their full size (scaled), not
            // squeezed into their room.
            D2D1_RECT_F glyphRect = slot.rect;
            if (slot.scale < 1) {
                const float full = std::round(kIslandIconSlot * iconScale);
                const float centerX = (slot.rect.left + slot.rect.right) / 2;
                glyphRect.left = centerX - full / 2;
                glyphRect.right = centerX + full / 2;
            }
            DrawGlyph(target, slot.text.c_str(), (UINT32)slot.text.size(),
                      format, glyphRect, brush.get());
            // Updates waiting: how many, on a red dot.
            if (slot.item == IslandItem::Updates && m_updatesDot > 0.004 &&
                m_updatesDotCount > 0) {
                const float grow = (float)std::max(m_updatesDot, 0.0);
                const float dot = 6 * scale;
                const D2D1_POINT_2F at{
                    (slot.rect.left + slot.rect.right) / 2 + 6 * scale,
                    (slot.rect.top + slot.rect.bottom) / 2 - 5 * scale};
                const float dotOpacity = contentOpacity * glyphOpacity *
                                         std::min(grow, 1.0f);
                D2D1_MATRIX_3X2_F before;
                target->GetTransform(&before);
                target->SetTransform(D2D1::Matrix3x2F::Scale(grow, grow, at) *
                                     before);
                brush->SetColor(D2D1::ColorF(1.0f, 0.27f, 0.23f, dotOpacity));
                target->FillEllipse({at, dot, dot}, brush.get());
                if (m_usageLabelFormat) {
                    const std::wstring count =
                        m_updatesDotCount > 9
                            ? L"9+"
                            : std::to_wstring(m_updatesDotCount);
                    brush->SetColor(D2D1::ColorF(1, 1, 1, dotOpacity));
                    target->DrawText(count.c_str(), (UINT32)count.size(),
                                     m_usageLabelFormat.get(),
                                     {at.x - dot, at.y - dot, at.x + dot,
                                      at.y + dot},
                                     brush.get());
                }
                target->SetTransform(before);
            }
            // New notifications not looked at yet: a red dot on the bell.
            if (slot.item == IslandItem::Bell && m_unseenDot > 0.004) {
                const float dot =
                    3 * scale * (float)std::max(m_unseenDot, 0.0);
                const float centerX = (slot.rect.left + slot.rect.right) / 2;
                const float centerY = (slot.rect.top + slot.rect.bottom) / 2;
                brush->SetColor(D2D1::ColorF(1.0f, 0.27f, 0.23f,
                                             contentOpacity * glyphOpacity));
                target->FillEllipse(
                    {{centerX + 5.5f * scale, centerY - 5.5f * scale},
                     dot,
                     dot},
                    brush.get());
            }
            if (scaled) {
                target->SetTransform(D2D1::Matrix3x2F::Identity());
            }
        }
        if (pillClipped) {
            target->PopAxisAlignedClip();
            const float fade = std::round(22 * scale);
            const D2D1_COLOR_F background = Bg(1);
            auto fadeEnd = [&](bool start) {
                D2D1_RECT_F band = pillView;
                D2D1_POINT_2F from{};
                D2D1_POINT_2F to{};
                // (Not over its outline.)
                const float line = 1.5f;
                if (m_vertical) {
                    band = start ? D2D1_RECT_F{pillView.left + line,
                                               pillView.top,
                                               pillView.right - line,
                                               pillView.top + fade}
                                 : D2D1_RECT_F{pillView.left + line,
                                               pillView.bottom - fade,
                                               pillView.right - line,
                                               pillView.bottom};
                    from = {band.left, start ? band.top : band.bottom};
                    to = {band.left, start ? band.bottom : band.top};
                } else {
                    band = start ? D2D1_RECT_F{pillView.left,
                                               pillView.top + line,
                                               pillView.left + fade,
                                               pillView.bottom - line}
                                 : D2D1_RECT_F{pillView.right - fade,
                                               pillView.top + line,
                                               pillView.right,
                                               pillView.bottom - line};
                    from = {start ? band.left : band.right, band.top};
                    to = {start ? band.right : band.left, band.top};
                }
                const D2D1_GRADIENT_STOP stops[] = {
                    {0, background},
                    {1, D2D1::ColorF(background.r, background.g, background.b,
                                     0)}};
                winrt::com_ptr<ID2D1GradientStopCollection> collection;
                winrt::com_ptr<ID2D1LinearGradientBrush> gradient;
                if (SUCCEEDED(target->CreateGradientStopCollection(
                        stops, 2, collection.put())) &&
                    SUCCEEDED(target->CreateLinearGradientBrush(
                        {from, to}, collection.get(), gradient.put()))) {
                    target->FillRectangle(band, gradient.get());
                }
            };
            if (m_pillScroll > 0.5) {
                fadeEnd(true);
            }
            if (m_pillScroll < m_pillOverflow - 0.5) {
                fadeEnd(false);
            }
        }
    }

    DrawTrayDrag(target, brush.get());

    // Off the screen while away (over a full-screen app). Not drawn: tried
    // again next time, even with nothing changed.
    if (!m_canvas.End(m_hwnd, m_away ? POINT{-32000, -32000} : m_position,
                      m_away ? 0.0f
                      : m_bar ? (float)std::clamp(m_appear, 0.0, 1.0)
                              : AppearFade())) {
        m_drawnKey.clear();
    }
    // (Its shapes already fade with the island, see addLiquid.)
    if (liquid) {
        m_liquid.Show(m_hwnd, m_position, m_size, m_liquidShapes);
        // Drawn for the glass, which is gone (Windows refused its blur):
        // drawn again with its own background.
        if (overLiquid && !m_liquid.Shown()) {
            Render(true);
            return;
        }
    } else {
        m_liquid.Hide();
    }
    RenderBarBubble();
    RenderPlacedBanner();
    PublishTrayPlaces();
}

// How far the peeking app's little pill is out, from 0 to 1 (a bit more
// while bouncing): it springs out, stays a moment, and goes back.
constexpr double kPeekIn = 0.45;
constexpr double kPeekHold = 1.9;
constexpr double kPeekOut = 0.45;

// How Caps Lock is now, in a bubble (islandCapsLock, see ShortcutKeyProc).
void Island::ShowCapsLock() {
    // (Notches: from the middle one, like the keyboard language's bubble.)
    const bool bubbleHere = m_notch < 0         ? true
                            : m_notchCount == 2 ? !m_notchBase
                                                : m_notch == 1;
    if (!HasMainSettings() || !m_visible || !g_settings.islandCapsLock ||
        !bubbleHere) {
        return;
    }
    const bool on = GetKeyState(VK_CAPITAL) & 1;
    ShowTextBubble(std::wstring(L"\u21EA  ") +
                       (on ? Tr(L"Caps Lock on", L"Caps Lock ligado")
                           : Tr(L"Caps Lock off", L"Caps Lock desligado")),
                   -1);
}

// (On the bar, under it: see RenderBarBubble.)
void Island::ShowTextBubble(const std::wstring& text, float x) {
    // (The item's name goes: this says more.)
    KillTimer(m_hwnd, kTipTimerId);
    m_pendingTipKey.clear();
    ShowTrayTip({});
    m_gearText = text;
    m_gearX = x;
    m_gearTextWidth = std::round(80 * m_scale);
    winrt::com_ptr<IDWriteTextLayout> layout;
    if (m_textFormat &&
        SUCCEEDED(m_dwrite->CreateTextLayout(text.c_str(), (UINT32)text.size(),
                                             m_textFormat.get(), 2000, 100,
                                             layout.put()))) {
        DWRITE_TEXT_METRICS metrics{};
        layout->GetMetrics(&metrics);
        m_gearTextWidth = std::ceil(metrics.width) + std::round(28 * m_scale);
    }
    const double now = NowSeconds();
    if (!m_gearShownAt || now >= m_gearHideAt) {
        m_gearShownAt = now;
    }
    m_gearHideAt = now + 1.6;
    StartAnimating();
    Render(true);
}

void Island::RenderBarBubble() {
    const float gear = m_bar && !m_away ? GearAmount(NowSeconds()) : 0.0f;
    if (gear <= 0.001f || !m_textFormat || !m_iconFormat) {
        if (m_bubbleWnd && IsWindowVisible(m_bubbleWnd)) {
            ShowWindow(m_bubbleWnd, SW_HIDE);
        }
        return;
    }
    if (!m_bubbleWnd) {
        // It doesn't take the mouse.
        m_bubbleWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT |
                                         WS_EX_TOPMOST | WS_EX_TOOLWINDOW |
                                         WS_EX_NOACTIVATE,
                                     L"STATIC", nullptr, WS_POPUP, 0, 0, 0, 0,
                                     nullptr, nullptr, nullptr, nullptr);
        if (!m_bubbleWnd) {
            return;
        }
        if (!m_bubbleCanvas.Create(m_d2d.get())) {
            DestroyWindow(m_bubbleWnd);
            m_bubbleWnd = nullptr;
            return;
        }
    }
    // Where the pill's would be (under the bar, out of its window), growing
    // out of the bar's edge, with room to bounce.
    const D2D1_RECT_F full = GearBubbleRect();
    const float width = full.right - full.left;
    const float height = full.bottom - full.top;
    const SIZE size{(LONG)std::ceil(width * 1.1f) + 2,
                    (LONG)std::ceil(height * 1.1f) + 2};
    if (!m_bubbleCanvas.Begin(size)) {
        return;
    }
    auto* target = m_bubbleCanvas.target.get();
    const float k = std::clamp(gear, 0.0f, 1.1f);
    const float middle = size.cx / 2.0f;
    const D2D1_RECT_F bubble{middle - width / 2 * k, 0, middle + width / 2 * k,
                             height * k};
    const float opacity = std::clamp(gear * 1.6f, 0.0f, 1.0f);
    const float radius =
        std::min(bubble.bottom - bubble.top, bubble.right - bubble.left) / 2;
    D2D1_COLOR_F fill = BgSolid(1);
    fill.a *= opacity;
    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(target->CreateSolidColorBrush(fill, brush.put()))) {
        target->FillRoundedRectangle({bubble, radius, radius}, brush.get());
        brush->SetColor(Fg(0.10f * EdgeAmount() * opacity));
        target->DrawRoundedRectangle(
            {{bubble.left + 0.5f, bubble.top + 0.5f, bubble.right - 0.5f,
              bubble.bottom - 0.5f},
             radius,
             radius},
            brush.get(), 1);
        brush->SetColor(
            Fg(0.9f * std::clamp((gear - 0.3f) / 0.5f, 0.0f, 1.0f)));
        if (m_gearText.empty()) {
            DrawGlyph(target, &kGlyphSettings, 1, m_iconFormat.get(), bubble,
                      brush.get());
        } else {
            target->DrawText(m_gearText.c_str(), (UINT32)m_gearText.size(),
                             m_textFormat.get(), bubble, brush.get(),
                             D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }
    const POINT position{
        m_position.x + (LONG)std::lround((full.left + full.right) / 2 - middle),
        m_position.y + (LONG)std::lround(full.top)};
    if (m_bubbleCanvas.End(m_bubbleWnd, position, 1) &&
        !IsWindowVisible(m_bubbleWnd)) {
        SetWindowPos(m_bubbleWnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                         SWP_SHOWWINDOW);
    }
}

// The bar see-through (islandBarClear; the Liquid Glass has its own look).
bool Island::BarClear() const {
    return m_bar && g_settings.islandBarClear && !g_settings.islandLiquidGlass;
}

// The frosted glass (islandFrosted): Windows' blur behind the bar, or behind
// the strip the pill keeps, on a window of its own under the island (a
// rectangle as big as the bar or the strip, tinted with the island's
// background: the bar's window grows for a banner). Only with Windows'
// transparency effects on, and not while the island is away (over a
// full-screen app): it fades with the bar, or as the pill closes into its
// dot, going away and coming back.
void Island::UpdateGlass() {
    const bool wanted = g_settings.islandFrosted &&
                        !g_settings.islandLiquidGlass &&
                        g_transparencyEffects && !m_away && m_notch < 0;
    // (Not while the Bar closes into its small pill with the time.)
    m_barBlurred = wanted && m_bar && m_hiddenClock <= 0.001;
    RECT strip{};
    if (m_barBlurred) {
        strip = {m_position.x + (LONG)std::floor(m_pill.left),
                 m_position.y + (LONG)std::floor(m_pill.top),
                 m_position.x + (LONG)std::ceil(m_pill.right),
                 m_position.y + (LONG)std::ceil(m_pill.bottom)};
    } else if (wanted && !m_bar && m_appBar && m_appBarThickness > 0) {
        strip = m_appBarRect;
    }
    if (IsRectEmpty(&strip)) {
        if (m_glassStrip && IsWindowVisible(m_glassStrip)) {
            ShowWindow(m_glassStrip, SW_HIDE);
        }
        return;
    }
    if (!m_glassStrip) {
        // It doesn't take the mouse.
        m_glassStrip = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT |
                                          WS_EX_TOPMOST | WS_EX_TOOLWINDOW |
                                          WS_EX_NOACTIVATE,
                                      L"STATIC", nullptr, WS_POPUP, 0, 0, 0, 0,
                                      nullptr, nullptr, nullptr, nullptr);
        if (!m_glassStrip) {
            return;
        }
        SetWindowBlur(m_glassStrip, true);
    }
    D2D1_COLOR_F color = Bg(kFrostedFill);
    if (BarClear()) {
        color.a *= kClearFrostedTint;
    }
    const BYTE opacity = (BYTE)std::lround(
        255 * (m_bar ? std::clamp(m_appear, 0.0, 1.0)
                     : std::clamp(AppearOpen(), 0.0f, 1.0f)));
    const bool painted =
        !IsWindowVisible(m_glassStrip) || !EqualRect(&strip, &m_glassRect) ||
        color.r != m_glassColor.r || color.g != m_glassColor.g ||
        color.b != m_glassColor.b || color.a != m_glassColor.a;
    if (painted) {
        m_glassRect = strip;
        m_glassColor = color;
        PaintFlatLayeredWindow(m_glassStrip, strip, color);
        m_glassOpacity = 255;
    }
    // (The whole window's opacity, the blur's too: nothing painted again.)
    if (opacity != m_glassOpacity) {
        m_glassOpacity = opacity;
        BLENDFUNCTION blend{AC_SRC_OVER, 0, opacity, AC_SRC_ALPHA};
        UpdateLayeredWindow(m_glassStrip, nullptr, nullptr, nullptr, nullptr,
                            nullptr, 0, &blend, ULW_ALPHA);
    }
    if (painted) {
        // Right under the island.
        SetWindowPos(m_glassStrip, m_hwnd, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

void CALLBACK InputMethodEventProc(HWINEVENTHOOK,
                                   DWORD,
                                   HWND,
                                   LONG,
                                   LONG,
                                   DWORD,
                                   DWORD) {
    if (Island* main = MainIsland()) {
        main->CheckInputMethodSoon();
    }
}

