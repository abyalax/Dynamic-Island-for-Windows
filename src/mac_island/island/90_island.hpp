DWORD Island::DragOverAt(IDataObject* data,
                         POINT screen,
                         DWORD,
                         DWORD allowed) {
    IslandScope scope(this);
    POINT pt = screen;
    ScreenToClient(m_hwnd, &pt);
    std::wstring key;
    const bool over = HitTest(pt, &key) == IslandItem::Folders;
    if (over != m_dropOnFolders) {
        m_dropOnFolders = over;
        KillTimer(m_hwnd, kFoldersDropTimerId);
        if (over && !m_foldersPanel.IsOpen()) {
            SetTimer(m_hwnd, kFoldersDropTimerId, kFolderOpenMs, nullptr);
        }
        // (Lit while the files are on it.)
        if (over) {
            m_hover = IslandItem::Folders;
        } else if (m_hover == IslandItem::Folders) {
            m_hover = IslandItem::None;
        }
        StartAnimating();
    }
    // Files on the rest of the pill (the folders' item opens its panel):
    // the shelf takes them when they're let go, as a bubble says.
    const bool shelf = !over && g_settings.islandShelf &&
                       ShelfDropEffect(allowed) != DROPEFFECT_NONE &&
                       DragHasFiles(data);
    if (shelf != m_dropOnShelf) {
        m_dropOnShelf = shelf;
        if (shelf) {
            ShowTextBubble(Tr(L"Let go to put it on the shelf",
                              L"Solte para pôr na prateleira"),
                           -1);
        } else {
            HideShelfBubble();
        }
    }
    return shelf ? ShelfDropEffect(allowed) : DROPEFFECT_NONE;
}

// The bubble asking to let go goes with the files.
void Island::HideShelfBubble() {
    const double now = NowSeconds();
    if (m_gearText == Tr(L"Let go to put it on the shelf",
                         L"Solte para pôr na prateleira") &&
        m_gearHideAt > now) {
        m_gearHideAt = now;
        StartAnimating();
    }
}

void Island::DragGone() {
    IslandScope scope(this);
    KillTimer(m_hwnd, kFoldersDropTimerId);
    if (m_foldersPanel.IsOpen()) {
        SetTimer(m_hwnd, kFoldersDragLeftTimerId, kFoldersDragLeftMs, nullptr);
    }
    if (m_dropOnFolders) {
        m_dropOnFolders = false;
        if (m_hover == IslandItem::Folders) {
            m_hover = IslandItem::None;
        }
        StartAnimating();
    }
    if (m_dropOnShelf) {
        m_dropOnShelf = false;
        HideShelfBubble();
    }
}

DWORD Island::DropAt(IDataObject* data, POINT, DWORD, DWORD allowed) {
    const bool shelf = m_dropOnShelf;
    DragGone();
    if (!shelf) {
        return DROPEFFECT_NONE;
    }
    IslandScope scope(this);
    const std::vector<std::wstring> paths = DraggedPaths(data);
    if (paths.empty()) {
        return DROPEFFECT_NONE;
    }
    if (!AddToShelf(paths)) {
        return DROPEFFECT_NONE;
    }
    ShowTextBubble(std::wstring(Tr(L"On the shelf", L"Na prateleira")) +
                       L": " + std::to_wstring(Shelf().size()),
                   -1);
    ForEachIsland([](Island& island) { island.FollowShelf(true); });
    return ShelfDropEffect(allowed);
}

// Mouse buttons: on a tray icon, passed on to its app (see ClickTrayIcon).
// Elsewhere, a click (on release) is the island's own.
void Island::OnMouseButton(UINT msg, LPARAM lParam) {
    std::wstring key;
    const IslandItem item =
        HitTest({(short)LOWORD(lParam), (short)HIWORD(lParam)}, &key);
    if (item == IslandItem::TrayIcon) {
        ShowTrayTip({});
        ClickTrayIcon(key, msg);
        return;
    }
    const bool pressable = item == IslandItem::MediaPrevious ||
                           item == IslandItem::MediaPlayPause ||
                           item == IslandItem::MediaNext ||
                           item == IslandItem::Media ||
                           item == IslandItem::ThemeSwitch ||
                           item == IslandItem::Microphone;
    if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) {
        if (pressable) {
            m_press.Press((int)item);
            StartAnimating();
        }
    }
    if (msg == WM_LBUTTONUP) {
        if (m_press.held) {
            m_press.Release();
            StartAnimating();
        }
        if (item == IslandItem::TrayPin) {
            SetTrayPinned(key, !IsTrayPinned(key));
            return;
        }
        OnClick(item, key);
    }
    // The middle button: a quick action on some items (the bell switches "Do
    // not disturb", the volume and the microphone mute).
    if (msg == WM_MBUTTONUP) {
        switch (item) {
            case IslandItem::Bell:
                if (SetIslandQuiet(!IsIslandQuiet())) {
                    OnDoNotDisturbChanged();
                }
                break;
            case IslandItem::Volume:
                if (auto volume = GetVolume(nullptr)) {
                    BOOL muted = FALSE;
                    if (SUCCEEDED(volume->GetMute(&muted))) {
                        volume->SetMute(!muted, nullptr);
                    }
                }
                break;
            case IslandItem::Microphone:
                ToggleMicrophone();
                m_control.Refresh();
                break;
            default:
                break;
        }
    }
    // The bell's right click: the notifications, with their switches open
    // (the middle button still switches "Do not disturb").
    if (msg == WM_RBUTTONUP && item == IslandItem::Bell) {
        m_notifications.ShowSettings();
        if (!m_notifications.IsOpen()) {
            TogglePanel(m_notifications, item);
        }
        return;
    }
    // A right click anywhere else on the island (not on another app's icon
    // or a notification): the settings bubble.
    // (Between two items too: anywhere on the pill.)
    const bool onIsland =
        item != IslandItem::None ||
        PointInRect({(short)LOWORD(lParam), (short)HIWORD(lParam)}, m_pill);
    if (msg == WM_RBUTTONUP && onIsland &&
        item != IslandItem::Overflow && item != IslandItem::TrayIcon &&
        item != IslandItem::TrayPin && item != IslandItem::SettingsBubble &&
        item != IslandItem::Banner && item != IslandItem::BannerReply &&
        item != IslandItem::BannerOpen && item != IslandItem::BannerClose &&
        item != IslandItem::BannerMinimize &&
        item != IslandItem::BannerAction) {
        if (m_bar) {
            OpenSettingsWindow();
        } else {
            ShowGearBubble();
        }
    }
    // The arrow's right click keeps the list open, or not (in the pill).
    if (msg == WM_RBUTTONUP && item == IslandItem::Overflow &&
        g_settings.islandTrayStyle == 0) {
        m_trayKeepOpen = !m_trayKeepOpen;
        g_settings.islandTrayOpen = m_trayKeepOpen;
        StoreProfileInt(L"trayKeepOpen", m_trayKeepOpen);
        m_trayOpen = m_trayKeepOpen;
        KillTimer(m_hwnd, kTrayCloseTimerId);
        StartAnimating();
    }
}

IslandItem Island::HitTest(POINT pt, std::wstring* appId) const {
    if (!m_bar && m_gearText.empty() && GearAmount(NowSeconds()) > 0.5f &&
        PointInRect(pt, GearBubbleRect())) {
        return IslandItem::SettingsBubble;
    }
    if (m_bannerAmount > 0.5 && PointInRect(pt, BannerRect())) {
        if (PointInRect(pt, BannerCloseRect())) {
            return IslandItem::BannerClose;
        }
        if (PointInRect(pt, BannerMinimizeRect())) {
            return IslandItem::BannerMinimize;
        }
        if (m_bannerOpen > 0.5 && BannerHasActions()) {
            for (size_t i = 0; i < m_banner.reply.actions.size(); i++) {
                if (PointInRect(pt, BannerActionRect(i))) {
                    if (appId) {
                        *appId = std::to_wstring(i);
                    }
                    return IslandItem::BannerAction;
                }
            }
        }
        if (m_bannerOpen > 0.5 && BannerCanReply()) {
            D2D1_RECT_F field, send, open;
            BannerParts(&field, &send, &open);
            if (PointInRect(pt, field)) {
                return IslandItem::BannerReply;
            }
            if (PointInRect(pt, open)) {
                return IslandItem::BannerOpen;
            }
        }
        return IslandItem::Banner;
    }
    if (!m_bar && m_minimized) {
        return LineOpens() ? IslandItem::Line : IslandItem::None;
    }
    // (The Bar hidden: its small pill with the time, like the line.)
    if (m_bar && m_barHidden) {
        return IslandItem::Line;
    }
    const IslandSlot* trayList = TrayListSlot();
    for (const auto& slot : m_slots) {
        if (slot.font == kTrayListFont ||
            (slot.inList && trayList && !PointInRect(pt, trayList->rect))) {
            continue;
        }
        if (pt.x >= slot.rect.left && pt.x < slot.rect.right &&
            pt.y >= slot.rect.top && pt.y < slot.rect.bottom) {
            if (appId && (slot.item == IslandItem::App ||
                          slot.item == IslandItem::TrayIcon ||
                          HasItemTip(slot.item))) {
                *appId = SlotKey(slot);
            }
            // What's playing: its note or its cover and title (each lit on
            // its own).
            if (appId && slot.item == IslandItem::Media) {
                *appId = MediaSlotPart(slot);
            }
            if (slot.item == IslandItem::TrayIcon &&
                slot.text == m_pinShownKey &&
                PointInRect(pt, TrayPinRect(slot.rect))) {
                return IslandItem::TrayPin;
            }
            return slot.item;
        }
    }
    // In the room between two tray icons: the nearer one (a right click
    // there opens its app's menu, not the island's settings).
    const IslandSlot* nearest = nullptr;
    float nearestDistance = std::round(8 * m_scale);
    for (const auto& slot : m_slots) {
        if (slot.item != IslandItem::TrayIcon ||
            (slot.inList && trayList && !PointInRect(pt, trayList->rect))) {
            continue;
        }
        // (Upright, the icons go down the pill.)
        const float across = m_vertical ? (float)pt.x : (float)pt.y;
        const float acrossFrom = m_vertical ? slot.rect.left : slot.rect.top;
        const float acrossTo = m_vertical ? slot.rect.right : slot.rect.bottom;
        if (across < acrossFrom || across >= acrossTo) {
            continue;
        }
        const float along = m_vertical ? (float)pt.y : (float)pt.x;
        const float alongFrom = m_vertical ? slot.rect.top : slot.rect.left;
        const float alongTo = m_vertical ? slot.rect.bottom : slot.rect.right;
        const float distance =
            along < alongFrom ? alongFrom - along : along - alongTo;
        if (distance < nearestDistance) {
            nearest = &slot;
            nearestDistance = distance;
        }
    }
    if (nearest) {
        if (appId) {
            *appId = nearest->text;
        }
        return IslandItem::TrayIcon;
    }
    return IslandItem::None;
}

// When the island last restarted for a lost device (kept across restarts).
double g_deviceRestartAt = -100;

void OnDrawingDeviceLost() {
    if (g_island) {
        g_island->OnDeviceLost();
    }
}

// The graphics driver was updated or reset (the island's drawing target
// fails from then on): the island starts over, which makes its targets and
// every picture kept for them again. Not more than every 2 seconds, while
// the driver may still be installing.
void Island::OnDeviceLost() {
    if (m_deviceRetryPending) {
        return;
    }
    const double now = NowSeconds();
    if (now - g_deviceRestartAt < kDeviceRetrySeconds) {
        m_deviceRetryPending = true;
        SetTimer(m_hwnd, kDeviceRetryTimerId,
                 (UINT)(kDeviceRetrySeconds * 1000), nullptr);
        return;
    }
    g_deviceRestartAt = now;
    m_deviceRetryPending = true;
    IslandLog(L"The graphics device was lost, restarting the island");
    RestartIslandSoon();
}

// Where a panel opens for the item clicked (the one with the key, or under
// the mouse, or the first of its kind), the same in every style: under it,
// out of the pill (over it, growing up, on the lower half of the screen);
// on the bar, dropping out of the item; upright on a side, beside it.
bool Island::PanelPlace(IslandItem item,
                        const std::wstring& key,
                        POINT* anchor,
                        D2D1_RECT_F* source,
                        Panel::Direction* direction,
                        bool* fromPill,
                        bool* behind) {
    const IslandSlot* clicked = nullptr;
    POINT cursor;
    GetCursorPos(&cursor);
    cursor.x -= m_position.x;
    cursor.y -= m_position.y;
    for (const auto& slot : m_slots) {
        if (item != IslandItem::None && slot.item == item &&
            (!clicked || (key.empty() ? PointInRect(cursor, slot.rect)
                                      : SlotKey(slot) == key))) {
            clicked = &slot;
        }
    }
    const D2D1_RECT_F pill{m_position.x + m_pill.left,
                           m_position.y + m_pill.top,
                           m_position.x + m_pill.right,
                           m_position.y + m_pill.bottom};
    const float gap = kPanelGap * m_scale;
    if (m_vertical) {
        // Upright on a side: beside the pill, towards the middle of the
        // screen, on its own (the shape that grows out of the pill is made
        // for it lying down), coming out from behind the item (not the
        // whole pill: along a bar, that's the screen's height), beside it.
        *anchor = {
            m_position.x + (LONG)(m_sideLeft ? m_pill.right + gap
                                             : m_pill.left - gap),
            m_position.y +
                (LONG)(clicked ? (clicked->rect.top + clicked->rect.bottom) / 2
                               : (m_pill.top + m_pill.bottom) / 2)};
        // (Without one, like the settings: a square in the pill's middle.)
        const float across = pill.right - pill.left;
        const float middle = (pill.top + pill.bottom) / 2;
        *source = clicked ? D2D1_RECT_F{m_position.x + clicked->rect.left,
                                        m_position.y + clicked->rect.top,
                                        m_position.x + clicked->rect.right,
                                        m_position.y + clicked->rect.bottom}
                          : D2D1_RECT_F{pill.left, middle - across / 2,
                                        pill.right, middle + across / 2};
        *direction =
            m_sideLeft ? Panel::Direction::Right : Panel::Direction::Left;
        *fromPill = false;
        *behind = true;
        return true;
    }
    if (!m_bar) {
        // Under the pill, under the item clicked (its middle for the
        // settings), coming out from behind it.
        *anchor = {
            m_position.x +
                (LONG)(clicked ? (clicked->rect.left + clicked->rect.right) / 2
                               : (m_pill.left + m_pill.right) / 2),
            m_position.y +
                (LONG)(AtBottom() ? m_pill.top - gap : m_pill.bottom + gap)};
        *source = pill;
        *direction =
            AtBottom() ? Panel::Direction::Up : Panel::Direction::Down;
        *fromPill = true;
        *behind = true;
        return true;
    }
    // Out of the item clicked (the clock if it isn't shown), or out of the
    // bar's middle.
    D2D1_RECT_F from{};
    if (clicked) {
        from = clicked->rect;
    } else if (item == IslandItem::None) {
        const float middle = (m_pill.left + m_pill.right) / 2;
        const float half = std::round(kIslandIconSlot * IconScale()) / 2;
        from = {middle - half, m_pill.top, middle + half, m_pill.bottom};
    } else {
        for (const auto& slot : m_slots) {
            if (slot.item == IslandItem::Clock) {
                from = slot.rect;
                break;
            }
        }
    }
    if (from.right <= from.left) {
        return false;
    }
    *anchor = {m_position.x + (LONG)((from.left + from.right) / 2),
               m_position.y + (LONG)(m_pill.bottom + gap)};
    *source = {m_position.x + from.left, m_position.y + from.top,
               m_position.x + from.right, m_position.y + from.bottom};
    *direction = Panel::Direction::Down;
    *fromPill = false;
    *behind = false;
    return true;
}

// Opens a panel out of the island (see PanelPlace), or closes it if it's
// open. Only one panel is open at a time.
void Island::TogglePanel(Panel& panel,
                         IslandItem item,
                         const std::wstring& key) {
    HideGearBubble();
    EndBannerTyping();
    // A banner shown: it shrinks back into the pill (behind the panel); the
    // panel opens under the pill as it'll be without it (laid out so, then
    // the banner goes on from where it was).
    double bannerLeft = 0;
    if (m_bannerWanted || m_bannerAmount > 0) {
        HideBanner();
        bannerLeft = m_bannerAmount;
        m_bannerAmount = 0;
        Render(true);
    }
    for (Panel* other : Panels()) {
        if (other != &panel) {
            other->Close();
        }
    }
    POINT anchor;
    D2D1_RECT_F source;
    Panel::Direction direction;
    bool fromPill;
    bool behind;
    if (panel.IsOpen()) {
        panel.Close();
    } else if (PanelPlace(item, key, &anchor, &source, &direction, &fromPill,
                          &behind)) {
        panel.SetDirection(direction);
        panel.Open(anchor, PanelScale(), source, fromPill, behind);
    }
    if (bannerLeft > 0) {
        m_bannerAmount = bannerLeft;
        Render(true);
    }
    StartAnimating();
}

void Island::MovePanel(Panel& panel,
                       IslandItem item,
                       const std::wstring& key) {
    POINT anchor;
    D2D1_RECT_F source;
    Panel::Direction direction;
    bool fromPill;
    bool behind;
    if (panel.IsOpen() && PanelPlace(item, key, &anchor, &source, &direction,
                                     &fromPill, &behind)) {
        panel.MoveTo(anchor, source);
    }
}

void Island::OnClick(IslandItem item, const std::wstring& appId) {
    switch (item) {
        case IslandItem::App: {
            // An app with a badge on the taskbar opens; one with unread
            // notifications opens at its latest one, or, with nothing to
            // open, its notifications are shown in the list. The Widgets
            // button's badge opens the Widgets board (Win+W, its own
            // shortcut).
            if (appId == kWidgetsBadgeId) {
                PressKeys({VK_LWIN, 'W'});
                break;
            }
            const UnreadApp* unread = FindUnread(appId);
            const bool badge = std::find(m_badgeApps.begin(), m_badgeApps.end(),
                                         appId) != m_badgeApps.end();
            if (badge || !unread) {
                OpenApp(appId);
                MarkAppRead(appId);
                m_notifications.ClearApp(appId);
                break;
            }
            const ToastReply reply = unread->reply;
            if (OpenNotification(appId, reply)) {
                MarkAppRead(appId);
                m_notifications.ClearApp(appId);
            } else {
                m_notifications.FocusApp(appId);
                if (!m_notifications.IsOpen()) {
                    TogglePanel(m_notifications, IslandItem::Bell);
                }
            }
            break;
        }
        case IslandItem::Banner:
        case IslandItem::BannerOpen:
            if (!m_banner.appId.empty()) {
                const std::wstring bannerApp = m_banner.appId;
                const ToastReply reply = m_banner.reply;
                CloseBannerElsewhere();
                // Windows' banner for it goes too.
                if (m_stackTakeover && m_takeoverDismiss) {
                    PressWindowsBanner(-1, L"");
                }
                EndBannerTyping();
                HideBanner();
                if (OpenNotification(bannerApp, reply)) {
                    MarkAppRead(bannerApp);
                    m_notifications.ClearApp(bannerApp);
                } else {
                    // Nothing to open: the list, at its notifications.
                    m_notifications.FocusApp(bannerApp);
                    TogglePanel(m_notifications, IslandItem::Bell);
                }
            } else if (m_banner.glyph) {
                // A status notice: it only goes.
                HideBanner();
            } else if (!m_banner.openPath.empty()) {
                // A drive: opened in Explorer.
                const std::wstring path = m_banner.openPath;
                HideBanner();
                ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr,
                             SW_SHOWNORMAL);
            } else {
                // A reminder: the calendar, on its day.
                m_calendar.SelectDay(m_reminderDay.year, m_reminderDay.month,
                                     m_reminderDay.day);
                HideBanner();
                TogglePanel(m_calendar, IslandItem::Clock);
            }
            break;
        case IslandItem::BannerClose: {
            // Deleted: gone from the pill and the list (and its app from the
            // island, if nothing else of it is left). Windows' banner for a
            // call, an alarm or a reminder goes to the notification center.
            if (m_stackTakeover && m_takeoverDismiss) {
                PressWindowsBanner(-1, L"");
            }
            const long long id = m_banner.id;
            const std::wstring bannerApp = m_banner.appId;
            CloseBannerElsewhere();
            EndBannerTyping();
            HideBanner();
            m_notifications.DismissById(id);
            if (!bannerApp.empty()) {
                MarkAppRead(bannerApp);
            }
            break;
        }
        case IslandItem::BannerMinimize:
            // Minimized: it stays in the list, and its app is ignored for a
            // moment (only its icon on the island moves); a call's, an
            // alarm's or a reminder's Windows banner goes to the
            // notification center (it stops ringing), not ignored.
            if (m_stackTakeover && m_takeoverDismiss) {
                PressWindowsBanner(-1, L"");
            }
            // (On every island: the others ignore it too.)
            if (!m_banner.appId.empty() && !m_banner.urgent) {
                const std::wstring app = m_banner.appId;
                const double until = NowSeconds() + kBannerSnoozeSeconds;
                ForEachIsland([&](Island& island) {
                    if (!island.IsSnoozed(app)) {
                        island.m_snoozedApps.push_back({app, until});
                    }
                    std::erase_if(island.m_waitingToasts,
                                  [&](const IslandToast& t) {
                                      return _wcsicmp(t.appId.c_str(),
                                                      app.c_str()) == 0;
                                  });
                });
            }
            CloseBannerElsewhere();
            EndBannerTyping();
            HideBanner();
            break;
        case IslandItem::BannerAction: {
            // A call's, an alarm's or a reminder's button: pressed like
            // Windows does ("Dismiss" just closes it).
            const size_t index = (size_t)_wtoi(appId.c_str());
            if (index < m_banner.reply.actions.size()) {
                const ToastReply::Action action =
                    m_banner.reply.actions[index];
                const std::wstring bannerApp = m_banner.appId;
                // Windows' own button (taken over), or the island's.
                if (action.activationType == L"windows") {
                    PressWindowsBanner(_wtoi(action.arguments.c_str()),
                                       action.label);
                }
                const IslandToast snoozed = m_banner;
                HideBanner();
                if (action.activationType == L"system" &&
                    action.arguments == L"snooze") {
                    SnoozeToast(snoozed);
                } else if (action.activationType == L"island") {
                    OnOwnNoticeAction(action.arguments);
                } else if (action.activationType != L"system" &&
                           action.activationType != L"windows") {
                    ActivateToast(bannerApp, action.arguments,
                                  action.activationType);
                }
                MarkAppRead(bannerApp);
            }
            break;
        }
        case IslandItem::BannerReply: {
            // In the box: typing; on the send button: sending.
            D2D1_RECT_F field, send, open;
            BannerParts(&field, &send, &open);
            POINT pt;
            GetCursorPos(&pt);
            pt.x -= m_position.x;
            pt.y -= m_position.y;
            if (m_bannerTyping && !m_bannerReplyText.empty() &&
                PointInRect(pt, send)) {
                SendBannerReply();
            } else {
                const bool typing = m_bannerTyping;
                StartBannerTyping();
                // Already typing: the caret goes where it's clicked.
                if (typing) {
                    EditTextClick(m_bannerReplyText, m_bannerEdit, pt);
                    Render(true);
                }
            }
            break;
        }
        case IslandItem::Overflow:
            // In a list of their own, or opening sideways (and staying, if
            // kept open).
            if (g_settings.islandTrayStyle != 0) {
                UpdateTrayList();
                TogglePanel(m_trayListPanel, item);
                break;
            }
            m_trayOpen = !m_trayOpen;
            if (!m_trayOpen) {
                m_trayKeepOpen = false;
                Wh_SetIntValue(L"trayKeepOpen", 0);
            }
            StartAnimating();
            break;
        case IslandItem::TrayIcon:
        case IslandItem::TrayPin:
            break;
        case IslandItem::SettingsBubble:
            OpenSettingsWindow();
            break;
        case IslandItem::InputMethod: {
            // The keyboards' list, for the app in front.
            HWND foreground = GetForegroundWindow();
            DWORD processId = 0;
            GetWindowThreadProcessId(foreground, &processId);
            m_keyboardPanel.SetTarget(processId == GetCurrentProcessId()
                                          ? m_appWindow
                                          : foreground);
            TogglePanel(m_keyboardPanel, item);
            break;
        }
        case IslandItem::PrivacyMicrophone:
        case IslandItem::PrivacyCamera:
        case IslandItem::PrivacyLocation:
            // (Which apps use it shows under the mouse.) Windows' page for
            // it, to choose which apps may.
            ShowTrayTip({});
            ShellExecute(nullptr, L"open",
                         item == IslandItem::PrivacyMicrophone
                             ? L"ms-settings:privacy-microphone"
                         : item == IslandItem::PrivacyCamera
                             ? L"ms-settings:privacy-webcam"
                             : L"ms-settings:privacy-location",
                         nullptr, nullptr, SW_SHOWNORMAL);
            break;
        case IslandItem::Network:
        case IslandItem::Volume:
        case IslandItem::Battery:
        case IslandItem::Brightness:
            // The volume opens only the sound, the battery only itself and
            // the power mode, the brightness only itself; the control
            // center's item, all of it (open already the other way: it
            // switches, going under this item).
            if (!m_control.SetOnly(item == IslandItem::Volume
                                       ? ControlPanel::Only::Sound
                                   : item == IslandItem::Battery
                                       ? ControlPanel::Only::Battery
                                   : item == IslandItem::Brightness
                                       ? ControlPanel::Only::Display
                                       : ControlPanel::Only::All) ||
                !m_control.IsOpen()) {
                TogglePanel(m_control, item);
            } else {
                MovePanel(m_control, item);
            }
            break;
        case IslandItem::Clock:
            TogglePanel(m_calendar, item);
            break;
        case IslandItem::Lyrics:
            m_lyricsPanel.SetSong(m_mediaShown, m_lyrics, m_lyricsSearching);
            TogglePanel(m_lyricsPanel, item);
            break;
        case IslandItem::Media:
        case IslandItem::Visualizer:
            m_mediaPanel.SetState(m_mediaState.present ? m_mediaState
                                                       : m_mediaShown);
            TogglePanel(m_mediaPanel, item);
            break;
        case IslandItem::MediaPrevious:
            SendMediaCommand(MediaCommand::Previous);
            break;
        case IslandItem::MediaPlayPause:
            SendMediaCommand(MediaCommand::PlayPause);
            break;
        case IslandItem::MediaNext:
            SendMediaCommand(MediaCommand::Next);
            break;
        case IslandItem::Bell:
            TogglePanel(m_notifications, item);
            break;
        case IslandItem::Minimize:
            SetMinimized(true);
            break;
        case IslandItem::Line:
            if (m_bar) {
                SetBarHidden(false);
            } else {
                // (Hidden by the auto-hide: open until the mouse leaves, as
                // when the mouse opens it.)
                SetMinimized(false, !m_autoHidden);
            }
            break;
        case IslandItem::Power:
            TogglePanel(m_power, item);
            break;
        case IslandItem::Microphone:
            ToggleMicrophone();
            m_control.Refresh();
            break;
        case IslandItem::Notes:
        case IslandItem::Tasks:
        case IslandItem::Shortcuts:
            OpenListPanel(item);
            break;
        case IslandItem::Weather:
            OpenWeather();
            break;
        case IslandItem::Updates:
            OpenUpdates();
            break;
        case IslandItem::Profiles:
            TogglePanel(m_profilesPanel, item);
            break;
        case IslandItem::Drives:
            TogglePanel(m_drivesPanel, item);
            break;
        case IslandItem::AppsFold:
            OpenFoldedApps(kAppsFoldMs);
            break;
        case IslandItem::Timer:
            // (A timer's time: its Live Activity; the icon: the panel.)
            if (!appId.empty()) {
                m_timerPanel.ShowActivity(
                    (TimerMode)std::clamp(_wtoi(appId.c_str()), 0,
                                          kTimerModes - 1));
            } else {
                m_timerPanel.ShowTab(-1);
            }
            TogglePanel(m_timerPanel, item);
            break;
        case IslandItem::NetSpeed:
            // Windows' own view of the network (Task Manager).
            OpenTaskManager();
            break;
        case IslandItem::WorldClock:
            // Windows' own list of them, to add or change one.
            ShellExecute(nullptr, L"open", L"control.exe", L"timedate.cpl,,1",
                         nullptr, SW_SHOWNORMAL);
            break;
        case IslandItem::Countdown:
            // An alarm or a timer: their panel, on its tab.
            if (!m_countdownText.empty() &&
                m_countdownKind != kCountdownEvent) {
                m_timerPanel.ShowTab(m_countdownKind == kCountdownAlarm   ? 3
                                     : m_countdownKind == kCountdownTimer ? 0
                                                                          : 2);
                TogglePanel(m_timerPanel, item);
                break;
            }
            // The calendar, on the event's day.
            if (!m_calendar.IsOpen()) {
                m_calendar.SelectDay(m_countdownDay[0], m_countdownDay[1],
                                     m_countdownDay[2]);
            }
            TogglePanel(m_calendar, item);
            break;
        case IslandItem::RecycleBin:
            m_recyclePanel.SetContents(g_recycleItems, g_recycleKilobytes);
            TogglePanel(m_recyclePanel, item);
            break;
        case IslandItem::AiUsage:
            TogglePanel(m_aiUsagePanel, item);
            break;
        case IslandItem::Shelf:
            TogglePanel(m_shelfPanel, item);
            break;
        case IslandItem::Eyes:
            BlinkEyes();
            break;
        case IslandItem::Commands: {
            // A pinned one runs; the icon opens their panel.
            const auto& commands = Commands();
            const size_t index = appId.empty() || !iswdigit(appId[0])
                                     ? commands.size()
                                     : _wtoi(appId.c_str());
            if (index < commands.size()) {
                if (!RunUserCommand(commands[index])) {
                    IslandLog(L"A pinned command couldn't start");
                }
            } else {
                TogglePanel(m_commandsPanel, item);
            }
            break;
        }
        case IslandItem::Desktops:
            TogglePanel(m_desktopsPanel, item);
            break;
        case IslandItem::Folders:
            TogglePanel(m_foldersPanel, item);
            break;
        case IslandItem::Wallpaper:
            TogglePanel(m_wallpaperPanel, item);
            break;
        case IslandItem::KomorebiLayout:
            m_komorebiPanel.SetScreen(m_monitorHandle);
            TogglePanel(m_komorebiPanel, item);
            break;
        case IslandItem::KomorebiWorkspace:
            // Another workspace on this monitor (the one shown: nothing).
            if (const KomorebiMonitor* monitor =
                    KomorebiMonitorOf(m_monitorHandle);
                monitor && !appId.ends_with(L"*")) {
                m_komorebiPanel.RunCommand(KomorebiWorkspaceCommand(
                    g_komorebiShown.komorebic, *monitor, _wtoi(appId.c_str())));
            }
            break;
        case IslandItem::FocusedApp:
            // (The desktop: nothing to bring.)
            if (m_focusWindow) {
                ActivateApp(m_focusWindow);
            }
            break;
        case IslandItem::QuickAction: {
            const int action = appId.empty() ? -1 : _wtoi(appId.c_str());
            if (action >= 0 && action < (int)ARRAYSIZE(kPillActions)) {
                ShowTrayTip({});
                RunPillAction(action);
            }
            break;
        }
        case IslandItem::ThemeSwitch:
            m_darkMode = !m_darkMode;
            m_control.SwitchDarkMode(m_darkMode);
            FollowWindowsMode();
            StartAnimating();
            break;
        case IslandItem::UsageCpu:
        case IslandItem::UsageRam:
        case IslandItem::UsageVram: {
            const auto kind = item == IslandItem::UsageCpu
                                  ? UsagePanel::Kind::Cpu
                              : item == IslandItem::UsageRam
                                  ? UsagePanel::Kind::Ram
                                  : UsagePanel::Kind::Gpu;
            // Open on another one: it shows that one.
            if (m_usagePanel.IsOpen() && m_usagePanel.GetKind() != kind) {
                m_usagePanel.SetKind(kind);
            } else {
                m_usagePanel.SetKind(kind);
                TogglePanel(m_usagePanel, item);
            }
            OnUsagePanelChanged();
            break;
        }
        case IslandItem::TraySeparator:
        case IslandItem::None:
            break;
    }
}

LRESULT Island::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam,
                              LPARAM lParam) {
    // The minimize shortcut (Win+J unless other keys were recorded): every
    // island that can be minimized goes to its line, or (all of them there
    // already) comes back, like a click on its line.
    if (msg == WM_APP_SHORTCUT && wParam == 3) {
        SetTimer(m_hwnd, kCapsLockTimerId, kCapsLockWaitMs, nullptr);
        return 0;
    }
    if (msg == WM_APP_SHORTCUT && wParam == 2) {
        // (A Bar has no line: it hides altogether, see SetBarHidden.)
        bool anyOpen = false;
        ForEachIsland([&](Island& island) {
            anyOpen = anyOpen ||
                      (island.m_bar ? !island.m_barHidden
                                    : !island.m_minimized &&
                                          island.MinimizableAt(
                                              island.CurrentPlace()));
        });
        ForEachIsland([&](Island& island) {
            if (island.m_bar) {
                island.SetBarHidden(anyOpen);
            } else if (island.m_minimized != anyOpen &&
                       island.MinimizableAt(island.CurrentPlace())) {
                island.SetMinimized(anyOpen);
            }
        });
        return 0;
    }
    // Win+N or Win+A (see ShortcutKeyProc): the main island opens its
    // notifications or its control center (or closes them, like Windows'
    // own), out of its line if it's minimized, wherever the mouse is.
    if (msg == WM_APP_SHORTCUT) {
        const Island* main = MainIsland();
        const RECT monitor = main ? main->m_monitor : m_monitor;
        // (Of the notches there, the one with the bell or the control
        // center.)
        const PillGroup group =
            wParam == 1 ? PillGroup::Network : PillGroup::Bell;
        Island* target = nullptr;
        Island* fallback = this;
        ForEachIsland([&](Island& island) {
            if (!EqualRect(&island.m_monitor, &monitor)) {
                return;
            }
            if (!island.m_notchBase) {
                fallback = &island;
            }
            if (island.InThisNotch(group)) {
                target = &island;
            }
        });
        Island* chosen = target ? target : fallback;
        IslandScope scope(chosen);
        chosen->OpenFromShortcut(wParam == 1);
        return 0;
    }
    // Explorer restarted (or asked the apps for their tray icons): it's told
    // the island is here.
    static const UINT taskbarCreated = RegisterWindowMessage(L"TaskbarCreated");
    if (msg == taskbarCreated && taskbarCreated) {
        if (m_main) {
            SayHello();
            // (Windows' notices of the Recycle Bin's changes come through
            // explorer: watched again from the new one, and counted again;
            // after a restart of the PC the island may have started first.)
            if (m_recycleNotify) {
                SHChangeNotifyDeregister(m_recycleNotify);
                m_recycleNotify = 0;
            }
            UpdateRecycleWatch();
        }
        m_appBarRetries = 0;
        RegisterAppBar();
        // (Windows' input indicator is in the new explorer.)
        if (m_inputMethodHooks[1]) {
            UnhookWinEvent(m_inputMethodHooks[1]);
            m_inputMethodHooks[1] = nullptr;
        }
        UpdateInputMethodWatch();
        return 0;
    }
    // What the main island hears for all of them (from explorer, the shared
    // jobs, Windhawk): the others hear it too.
    if (m_main && g_islands.size() > 1 &&
        (msg == WM_APP_USAGE || msg == WM_APP_WEATHER ||
         msg == WM_APP_UPDATES || msg == WM_APP_BADGES || msg == WM_APP_TRAY ||
         msg == WM_APP_MEDIA || msg == WM_APP_TOASTS_CHANGED ||
         msg == WM_APP_INPUT_METHOD || msg == WM_APP_PRIVACY ||
         msg == WM_APP_LYRICS ||
         msg == WM_APP_DRIVES || msg == WM_APP_RELOAD_SETTINGS ||
         (msg == WM_APP_STATUS && wParam == kStatusDoNotDisturb))) {
        for (Island* other : g_islands) {
            if (other != this) {
                PostMessage(other->m_hwnd, msg, wParam, lParam);
            }
        }
    }
    switch (msg) {
        case WM_APP_USAGE:
            OnUsage();
            return 0;
        case WM_APP_USAGE_PANEL:
            OnUsagePanelChanged();
            return 0;
        case WM_APP_BRIGHTNESS:
            // Read (-1: no monitor tells it, the item isn't shown), unless
            // it's being changed here.
            if (!m_brightnessJob->running &&
                (int)(INT_PTR)wParam != m_brightness) {
                m_brightness = (int)(INT_PTR)wParam;
                Relayout();
            }
            return 0;
        case WM_APP_WEATHER:
            OnWeather((int)wParam);
            return 0;
        case WM_APP_UPDATES:
            OnUpdates();
            return 0;
        case WM_APP_RELOAD_SETTINGS:
            LoadSettings();
            // (Its own monitor's place, read again.)
            if (!m_placeDevice.empty()) {
                UseMonitorPlace(m_placeDevice);
            }
            OnPreferencesChanged();
            UpdateMonitorWatch();
            CheckMonitor();
            return 0;
        case WM_DISPLAYCHANGE:
            if (m_main) {
                g_displayChanges++;
            }
            // A monitor plugged in or out (or its handle changed): with one
            // on each monitor, they start again for the monitors there are
            // now. Another resolution only lays them out again.
            if (m_main &&
                ((g_settings.islandAllMonitors &&
                  (MonitorCount() > 1 || g_islands.size() > 1)) ||
                 m_notchCount > 0) &&
                IslandMonitorsChanged()) {
                RestartIslandSoon();
            }
            UpdateMonitorWatch();
            CheckMonitor();
            m_control.ReloadControls();
            Relayout();
            if (m_main) {
                RenewAllGlass();
            }
            break;
        case WM_DWMCOLORIZATIONCOLORCHANGED:
        case WM_THEMECHANGED:
            g_windowsAccentColor = ReadWindowsAccentColor();
            UpdateFill();
            StartIslandAnimation();
            break;
        case WM_SETTINGCHANGE:
            // (The week's first day, or the language of Windows' dates.)
            ForgetWeekdays();
            // The taskbar moved to another edge (or a bar came or went): the
            // pill's room is another.
            if (wParam == SPI_SETWORKAREA) {
                CheckAppBarRoom();
                Relayout();
            }
            // Light or dark mode switched (here or elsewhere): the button
            // turns to it.
            if (lParam &&
                wcscmp((PCWSTR)lParam, L"ImmersiveColorSet") == 0) {
                if (IsDarkMode() != m_darkMode) {
                    m_darkMode = !m_darkMode;
                    StartAnimating();
                }
                g_windowsAccentColor = ReadWindowsAccentColor();
                UpdateFill();
                StartIslandAnimation();
                FollowWindowsMode();
                // Windows' transparency effects (the frosted glass needs
                // them) may have been switched.
                // (Back on: Windows' blur too, see RenewAllGlass. This comes
                // often with nothing switched, like as the work area changes.)
                const bool effects = TransparencyEffectsOn();
                const bool effectsBack = effects && !g_transparencyEffects;
                g_transparencyEffects = effects;
                Render(true);
                if (effectsBack) {
                    RenewAllGlass();
                }
            }
            break;
        // The icons' thread read pictures (see TellIconsRead).
        case WM_APP_ICONS:
            OnIconsRead();
            return 0;
        case WM_APP_INPUT_METHOD:
            // The thread's answer. (One asked before another question, or
            // before the app in front had no input modes: left, it would
            // show the old app's.)
            if ((lParam & kInputMethodAsked) &&
                (((UINT)lParam >> kInputMethodIdShift) & kInputMethodIdMask) !=
                    (m_inputMethodId & kInputMethodIdMask)) {
                return 0;
            }
            // (Each island showing the item, as itself.)
            ForEachIsland([&](Island& island) {
                if (g_settings.islandInputMethod) {
                    island.OnInputMethod(wParam, lParam & 0xFFFF);
                }
            });
            return 0;
        case WM_APP_RECYCLE_CHANGED: {
            // (Windows' shared memory for the change, given back; what
            // changed doesn't matter: the bin is counted again.)
            PIDLIST_ABSOLUTE* items = nullptr;
            LONG event = 0;
            if (HANDLE lock = SHChangeNotification_Lock(
                    (HANDLE)wParam, (DWORD)lParam, &items, &event)) {
                SHChangeNotification_Unlock(lock);
            }
            SetTimer(m_hwnd, kRecycleTimerId, kRecycleSettleMs, nullptr);
            return 0;
        }
        case WM_APP_RECYCLE_BIN:
            OnRecycleCounted(wParam, lParam);
            return 0;
        case WM_APP_TIMER_CHANGED:
            ForEachIsland([](Island& island) {
                island.Relayout();
                island.m_timerPanel.Changed();
            });
            return 0;
        case WM_APP_PRIVACY:
            // (Read by the main island's thread; the others follow.)
            ForEachIsland([](Island& island) { island.OnPrivacy(); });
            return 0;
        case WM_APP_WALLPAPER_LIST: {
            std::unique_ptr<WallpaperListJob> job((WallpaperListJob*)lParam);
            if (m_wallpaperListThread) {
                WaitForSingleObject(m_wallpaperListThread, INFINITE);
                CloseHandle(m_wallpaperListThread);
                m_wallpaperListThread = nullptr;
            }
            // No pictures there (or no folder chosen yet): its panel, to
            // choose one.
            if (job->paths.empty()) {
                TogglePanel(m_wallpaperPanel, IslandItem::Wallpaper);
                return 0;
            }
            m_wallpapers = std::move(job->paths);
            m_wallpaperIndex = -1;
            StepWallpaper(job->step);
            return 0;
        }
        case WM_APP_WALLPAPER_DONE:
            if (m_wallpaperThread) {
                WaitForSingleObject(m_wallpaperThread, INFINITE);
                CloseHandle(m_wallpaperThread);
                m_wallpaperThread = nullptr;
            }
            return 0;
        case WM_APP_KOMOREBI: {
            // (Read by the main island's thread; the others follow.)
            AcquireSRWLockShared(&g_komorebiLock);
            const KomorebiState state = g_komorebiState;
            ReleaseSRWLockShared(&g_komorebiLock);
            if (!(state == g_komorebiShown)) {
                g_komorebiShown = state;
                ForEachIsland([](Island& island) {
                    island.m_komorebiPanel.Refresh();
                    island.Relayout();
                });
            }
            return 0;
        }
        case WM_APP_PROGRESS: {
            // From explorer (see PublishProgress): checked, every island.
            const int state = std::clamp((int)(wParam & 0xFF), 0, 8);
            const int permille = std::clamp((int)lParam, 0, 1000);
            if (state != g_downloadState || permille != g_downloadPermille) {
                g_downloadMovedAt = NowSeconds();
            }
            g_downloadState = state;
            g_downloadKind = (int)(wParam >> 8 & 0xF);
            g_downloadBrowsers = (wParam >> 8 & kProgressFromBrowsers) != 0;
            g_downloadPermille = permille;
            ForEachIsland([](Island& island) { island.UpdateFill(); });
            return 0;
        }
        case WM_APP_LYRICS:
            // A song's lyrics came (the main island's search, see
            // UpdateLyrics); another song waiting: looked for now.
            if (m_lyricsThread) {
                WaitForSingleObject(m_lyricsThread, INFINITE);
                CloseHandle(m_lyricsThread);
                m_lyricsThread = nullptr;
            }
            // (Every island looking takes them; one looking for another
            // song asks for it now.)
            ForEachIsland([](Island& island) {
                if (!island.m_lyricsSearching) {
                    return;
                }
                if (!island.m_lyricsKey.empty()) {
                    island.m_lyricsSearching =
                        !CopyLyrics(island.m_lyricsKey, &island.m_lyrics);
                }
                if (island.m_lyricsSearching) {
                    island.m_lyricsKey.clear();
                }
                island.UpdateLyrics();
            });
            return 0;
        case WM_APP_AI_USAGE: {
            // The agents' limits, read by the thread watching their files.
            std::unique_ptr<AiUsageRead> read((AiUsageRead*)lParam);
            if (read) {
                OnAiUsageRead(*read);
            }
            return 0;
        }
        case WM_APP_WORLD_CLOCKS:
            // Windows' additional clocks changed: read again now.
            ForEachIsland([](Island& island) {
                island.m_extrasKey = -1;
                island.UpdateExtraItems();
            });
            return 0;
        case WM_APP_DESKTOPS: {
            // (Read by the main island's thread; the others follow.)
            AcquireSRWLockShared(&g_desktopsLock);
            const VirtualDesktops desktops = g_desktops;
            ReleaseSRWLockShared(&g_desktopsLock);
            ForEachIsland([&](Island& island) {
                if (!(island.m_desktops == desktops)) {
                    island.m_desktops = desktops;
                    if (HWND panel = island.m_desktopsPanel.Window()) {
                        PostMessage(panel, WM_APP_DESKTOPS, 0, 0);
                    }
                    island.Relayout();
                }
            });
            // A desktop closed from the list: once on it, closed (see
            // g_desktopCloseAt); then back to where the user was.
            if (g_desktopCloseAt >= 0) {
                if (NowSeconds() > g_desktopReturnUntil) {
                    g_desktopCloseAt = -1;
                    g_desktopReturnTo = -1;
                } else if (desktops.current == g_desktopCloseAt) {
                    g_desktopCloseAt = -1;
                    PressKeys({VK_CONTROL, VK_LWIN, VK_F4});
                }
            }
            if (g_desktopReturnTo >= 0 &&
                (NowSeconds() > g_desktopReturnUntil ||
                 (desktops.names.size() == g_desktopReturnCount &&
                  desktops.current >= 0))) {
                if (NowSeconds() <= g_desktopReturnUntil) {
                    SwitchVirtualDesktop(g_desktopReturnTo);
                }
                g_desktopReturnTo = -1;
            }
            return 0;
        }
        case WM_APP_DRIVES: {
            // The listing's end: the one asked meanwhile runs now.
            if (m_main && wParam == kDrivesListed && m_driveThread &&
                (UINT)lParam == m_drivesId) {
                WaitForSingleObject(m_driveThread, INFINITE);
                CloseHandle(m_driveThread);
                m_driveThread = nullptr;
                if (m_drivesAgain && UsbDrivesWatched()) {
                    const DWORD arrived = m_drivesArrived;
                    m_drivesAgain = false;
                    m_drivesArrived = 0;
                    ListDrives(arrived);
                }
            }
            AcquireSRWLockShared(&g_drivesLock);
            const std::vector<RemovableDrive> drives = g_drives;
            ReleaseSRWLockShared(&g_drivesLock);
            // (Listed by the main island; the others follow.)
            ForEachIsland([&](Island& island) {
                island.m_drives = drives;
                island.m_drivesPanel.SetDrives(drives);
                island.Relayout();
            });
            return 0;
        }
        case WM_DEVICECHANGE:
            if (m_main && UsbDrivesWatched()) {
                OnDeviceChange(wParam, lParam);
            }
            break;
        case WM_CLIPBOARDUPDATE:
            m_clipboardTries = 0;
            OnClipboardChanged();
            return 0;
        case WM_TIMER:
            if (wParam == kIslandTimerId) {
                Tick();
                ScheduleTick();
                return 0;
            }
            if (wParam == kMonitorTimerId) {
                CheckMonitor();
                return 0;
            }
            if (wParam == kInputMethodTimerId) {
                KillTimer(m_hwnd, kInputMethodTimerId);
                StartInputMethodQuery();
                return 0;
            }
            if (wParam == kInputMethodPollTimerId) {
                // (Not while no island showing it is there: a full-screen
                // app; the app in front changing asks again.)
                if (AnyIslandLook([](Island& island) {
                        return island.m_visible &&
                               g_settings.islandInputMethod;
                    })) {
                    StartInputMethodQuery();
                }
                return 0;
            }
            if (wParam == kFoldersDropTimerId) {
                KillTimer(m_hwnd, kFoldersDropTimerId);
                if (m_dropOnFolders && !m_foldersPanel.IsOpen()) {
                    TogglePanel(m_foldersPanel, IslandItem::Folders);
                }
                return 0;
            }
            if (wParam == kFoldersDragLeftTimerId) {
                KillTimer(m_hwnd, kFoldersDragLeftTimerId);
                m_foldersPanel.CloseIfDragLeft();
                return 0;
            }
            if (wParam == kProfileSaveTimerId) {
                if (m_main) {
                    SaveActiveProfile();
                } else {
                    SaveOwnProfile();
                }
                return 0;
            }
            if (wParam == kAutoHideTimerId) {
                CheckAutoHide();
                return 0;
            }
            if (wParam == kAutoProfileTimerId) {
                KillTimer(m_hwnd, kAutoProfileTimerId);
                CheckAutoProfile();
                return 0;
            }
            if (wParam == kCapsLockTimerId) {
                KillTimer(m_hwnd, kCapsLockTimerId);
                // (Each island, as itself: a notch's from the middle one.)
                ForEachIsland([](Island& island) { island.ShowCapsLock(); });
                return 0;
            }
            if (wParam == kDeviceRetryTimerId) {
                KillTimer(m_hwnd, kDeviceRetryTimerId);
                m_deviceRetryPending = false;
                OnDeviceLost();
                return 0;
            }
            if (wParam == kGlassRenewTimerId) {
                KillTimer(m_hwnd, kGlassRenewTimerId);
                RenewAllGlass();
                if (++m_glassRenewals < (int)std::size(kGlassRenewMs)) {
                    SetTimer(m_hwnd, kGlassRenewTimerId,
                             kGlassRenewMs[m_glassRenewals], nullptr);
                }
                return 0;
            }
            if (wParam == kTrayAppTimerId) {
                StopWatchingTrayAppWindows();
                return 0;
            }
            if (wParam == kLyricsTimerId) {
                StepLyrics();
                return 0;
            }
            if (wParam == kPlaceRecheckTimerId) {
                KillTimer(m_hwnd, kPlaceRecheckTimerId);
                RecheckPlaceApp();
                return 0;
            }
            if (wParam == kChargeFillTimerId) {
                StartAnimating();
                return 0;
            }
            if (wParam == kEyesTimerId) {
                OnEyesTimer();
                return 0;
            }
            if (wParam == kAlarmSoundStopTimerId) {
                StopAlarmSound();
                return 0;
            }
            if (wParam == kWhatsNewTimerId) {
                KillTimer(m_hwnd, kWhatsNewTimerId);
                if (g_firstInstall) {
                    OpenSetup();
                } else {
                    ShowWhatsNewNotice();
                }
                return 0;
            }
            if (wParam == kNetSpeedTimerId) {
                OnNetSpeedTimer();
                return 0;
            }
            if (wParam == kAiUsageTimerId) {
                OnAiUsageMinute();
                return 0;
            }
            if (wParam == kRecycleTimerId) {
                KillTimer(m_hwnd, kRecycleTimerId);
                StartRecycleCount();
                return 0;
            }
            if (wParam == kWeatherTimerId) {
                StartWeather({});
                return 0;
            }
            if (wParam == kWeatherRetryTimerId) {
                KillTimer(m_hwnd, kWeatherRetryTimerId);
                if (AnyWeatherWanted()) {
                    StartWeather({});
                }
                return 0;
            }
            if (wParam == kNetworkTimerId) {
                OnNetworkChanged();
                return 0;
            }
            if (wParam == kNetworkFrameTimerId) {
                if (m_visible) {
                    Render();
                }
                return 0;
            }
            if (wParam == kUpdatesTimerId) {
                // Looked for a moment ago (before a restart of the island):
                // not again until it's due.
                const UpdatesData updates = GetUpdates();
                const double since = NowSeconds() - updates.checkedAt;
                if (updates.checked && since >= 0 &&
                    since < UpdatesCheckSeconds()) {
                    SetTimer(m_hwnd, kUpdatesTimerId,
                             (UINT)((UpdatesCheckSeconds() - since) * 1000) +
                                 1000,
                             nullptr);
                    return 0;
                }
                SetTimer(m_hwnd, kUpdatesTimerId,
                         (UINT)(UpdatesCheckSeconds() * 1000), nullptr);
                StartUpdatesJob();
                return 0;
            }
            if (wParam == kMediaTimerId) {
                KillTimer(m_hwnd, kMediaTimerId);
                UpdateMediaShown();
                return 0;
            }
            if (wParam == kMediaOpenTimerId) {
                // Its window came; or the app didn't answer its tray icon's
                // click (like Spotify from the Store): started by its ID.
                const bool came = FindMediaWindow(m_mediaOpenApp) != nullptr;
                if (came || NowSeconds() - m_mediaOpenAt >= kMediaOpenSeconds) {
                    KillTimer(m_hwnd, kMediaOpenTimerId);
                    if (!came) {
                        LaunchMediaApp(m_mediaOpenApp);
                    }
                    m_mediaOpenApp.clear();
                }
                return 0;
            }
            if (wParam == kBannerNextTimerId) {
                KillTimer(m_hwnd, kBannerNextTimerId);
                ShowBanner();
                return 0;
            }
            if (wParam == kClipboardTimerId) {
                KillTimer(m_hwnd, kClipboardTimerId);
                OnClipboardChanged();
                return 0;
            }
            if (wParam == kShiftStepTimerId) {
                // Shift let go: what was chosen is applied.
                if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                    ApplyShiftChoice();
                }
                return 0;
            }
            if (wParam == kUrgentSoundTimerId) {
                PlayUrgentSound();
                return 0;
            }
            if (wParam == kUrgentSoundStopTimerId) {
                KillTimer(m_hwnd, kUrgentSoundStopTimerId);
                if (m_urgentRinging) {
                    m_urgentRinging = false;
                    StopToastSound();
                }
                return 0;
            }
            if (wParam == kBannerRestoreTimerId) {
                OnBannerRestoreTimer();
                return 0;
            }
            if (wParam == kPinTimerId) {
                KillTimer(m_hwnd, kPinTimerId);
                if (m_hover == IslandItem::TrayIcon && !m_hoverApp.empty()) {
                    m_pinShownKey = m_hoverApp;
                    m_pinShownAt = NowSeconds();
                    StartAnimating();
                }
                return 0;
            }
            if (wParam == kTipTimerId) {
                KillTimer(m_hwnd, kTipTimerId);
                if (!m_pendingTipKey.empty() && !TextBubbleShown() &&
                    ((m_hover == IslandItem::TrayIcon &&
                      m_hoverApp == m_pendingTipKey) ||
                     (m_hover == IslandItem::App &&
                      kAppTipPrefix + m_hoverApp == m_pendingTipKey) ||
                     (HasItemTip(m_hover) &&
                      ItemTipKey(m_hover, m_hoverApp) == m_pendingTipKey))) {
                    ShowTrayTip(m_pendingTipKey);
                }
                m_pendingTipKey.clear();
                return 0;
            }
            if (wParam == kAppsFoldTimerId) {
                // (The mouse still on them: again later.)
                if (m_hover == IslandItem::AppsFold ||
                    m_hover == IslandItem::App) {
                    return 0;
                }
                KillTimer(m_hwnd, kAppsFoldTimerId);
                m_appsOpen = false;
                Relayout();
                StartAnimating();
                return 0;
            }
            if (wParam == kTrayCloseTimerId) {
                KillTimer(m_hwnd, kTrayCloseTimerId);
                if (m_trayOpen) {
                    m_trayOpen = false;
                    StartAnimating();
                }
                return 0;
            }
            if (wParam == kBannerTimerId) {
                // Stays while the mouse is on it, or while typing.
                if (m_bannerTyping || BannerHovered()) {
                    return 0;
                }
                HideBanner();
                return 0;
            }
            break;

        case WM_APP_BADGES: {
            // A count changed with the same apps: drawn again too.
            const std::vector<int> counts = m_badgeCounts;
            UpdateApps();
            Render(counts != m_badgeCounts);
            return 0;
        }

        // From explorer: the apps with a badge, and the tray icons.
        case WM_COPYDATA: {
            const auto* copyData = (const COPYDATASTRUCT*)lParam;
            if (!copyData || !IsIpcSender((HWND)wParam, L"Shell_TrayWnd")) {
                break;
            }
            // Taken now (the icons are copied while explorer waits), shown
            // a moment later: this may arrive while the island is itself
            // sending explorer something. Explorer finds any island's window
            // (a notch's, another monitor's): told to the main one, which
            // tells them all.
            const Island* main = MainIsland();
            const HWND told = main && main->m_hwnd ? main->m_hwnd : hWnd;
            if (copyData->dwData == kIpcBadgeApps) {
                std::vector<std::wstring> apps;
                std::vector<int> counts;
                for (const auto& line : SplitLines(copyData)) {
                    const size_t tab = line.find(L'\t');
                    apps.push_back(line.substr(0, tab));
                    counts.push_back(
                        tab == std::wstring::npos
                            ? 0
                            : std::max(0, _wtoi(line.c_str() + tab + 1)));
                }
                AcquireSRWLockExclusive(&g_badgeAppsLock);
                g_badgeApps = std::move(apps);
                g_badgeCounts = std::move(counts);
                ReleaseSRWLockExclusive(&g_badgeAppsLock);
                PostMessage(told, WM_APP_BADGES, 0, 0);
                return TRUE;
            }
            if (copyData->dwData == kIpcTrayIcons) {
                ReceiveTrayIcons(copyData);
                PostMessage(told, WM_APP_TRAY, 0, 0);
                return TRUE;
            }
            break;
        }

        case WM_APP_STATUS:
            if (wParam == kStatusMediaVolume && m_statusEvents) {
                lParam = m_statusEvents->TakeMediaVolume();
            }
            // (Its notches without the bell hear it from here; "Do not
            // disturb" goes to every island already, see above.)
            if (wParam != kStatusDoNotDisturb) {
                for (Island* notch : m_notchSiblings) {
                    PostMessage(notch->m_hwnd, msg, wParam, lParam);
                }
            }
            switch (wParam) {
                case kStatusMediaVolume: {
                    const float level = (lParam & 0xFFFF) / 10000.0f;
                    const bool muted = (lParam & 0x10000) != 0;
                    m_volumeGlyph = muted ? std::wstring{kGlyphMute}
                        : std::wstring{kGlyphVolume[level <= 0.001f ? 0 :
                            std::clamp((int)(level * 3) + 1, 1, 3)]};
                    const std::wstring text =
                        std::to_wstring((int)std::lround(level * 100)) + L"%";
                    if (m_volumeText.text != text) {
                        m_volumeText.text = text;
                        if (g_settings.islandVolumePercent) {
                            Relayout();
                        }
                    }
                    StartAnimating();
                    break;
                }
                case kStatusMicrophoneDevice:
                    WatchMicrophone();
                    OnVolumeChanged();
                    m_control.Refresh();
                    break;
                case kStatusAudioDevice:
                    WatchVolume();
                    // (The visualizer's meter: the new output's.)
                    m_vizMeter = nullptr;
                    m_vizSessionMeters.clear();
                    m_vizApp.clear();
                    m_vizLookAt = 0;
                    [[fallthrough]];
                case kStatusVolume:
                    // (The microphone's mute too.)
                    OnVolumeChanged();
                    m_control.Refresh();
                    break;
                case kStatusNetwork:
                    OnNetworkChanged();
                    m_control.Refresh();
                    break;
                case kStatusBattery:
                    m_batteryGlyph = BatteryGlyph();
                    UpdateBatteryPercent();
                    Render();
                    m_control.Refresh();
                    CheckPowerNotices();
                    break;
                case kStatusDoNotDisturb:
                    OnDoNotDisturbChanged();
                    m_control.Refresh();
                    m_notifications.Refresh();
                    if (!g_settings.islandOwnsNotifications) {
                        NoteQuiet(IsIslandQuiet());
                    }
                    break;
                case kStatusAudioDevices:
                    CheckBluetoothOutputs();
                    break;
            }
            return 0;

        case WM_POWERBROADCAST:
            if (wParam == PBT_POWERSETTINGCHANGE) {
                const auto* setting = (const POWERBROADCAST_SETTING*)lParam;
                if (setting &&
                    IsEqualGUID(setting->PowerSetting,
                                kGUID_PowerSavingStatus) &&
                    setting->DataLength >= sizeof(DWORD)) {
                    const bool saver = *(const DWORD*)setting->Data != 0;
                    if (saver != g_energySaver) {
                        g_energySaver = saver;
                        ForEachIsland([](Island& island) { island.Relayout(); });
                        RenewAllGlass();
                        StartIslandAnimation();
                    }
                    return TRUE;
                }
                PostMessage(hWnd, WM_APP_STATUS, kStatusBattery, 0);
                return TRUE;
            }
            break;

        case WM_APP_TOAST:
            ShowBanner();
            return 0;

        case WM_APP_TOASTS_CHANGED:
            UpdateAppCounts();
            OnToastsChanged();
            return 0;

        case WM_APP_BANNER_READ:
            OnWindowsBannersRead();
            return 0;

        case WM_APP_MEDIA:
            AcquireSRWLockShared(&g_mediaLock);
            m_mediaState = g_media;
            ReleaseSRWLockShared(&g_mediaLock);
            m_mediaPanel.SetState(m_mediaState);
            UpdateMediaShown();
            Render();
            return 0;

        case WM_APP_APPBAR:
            if (wParam == ABN_POSCHANGED) {
                SetAppBarPosition(true);
            }
            return 0;

        case WM_MOUSEACTIVATE:
            return m_bannerTyping ? MA_ACTIVATE : MA_NOACTIVATE;

        case WM_ACTIVATE:
            // Clicked somewhere else while typing a reply.
            if (LOWORD(wParam) == WA_INACTIVE && m_bannerTyping) {
                EndBannerTyping();
                HideBanner();
            }
            return 0;

        // Alt+F4 while typing a reply: the reply is left, the island stays.
        case WM_CLOSE:
            if (m_bannerTyping) {
                EndBannerTyping();
                HideBanner();
            }
            return 0;

        case WM_CHAR:
            g_lastTyped = NowSeconds();
            if (OnBannerChar((WCHAR)wParam)) {
                return 0;
            }
            break;

        case WM_KEYDOWN:
            g_lastTyped = NowSeconds();
            // The arrows, Home, End and Delete in the reply.
            if (m_bannerTyping &&
                EditTextKey(m_bannerReplyText, m_bannerEdit, wParam)) {
                Render(true);
                return 0;
            }
            break;

        case WM_MOUSEMOVE: {
            if (!m_tracking) {
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hWnd, 0};
                m_tracking = TrackMouseEvent(&track);
            }
            if (!g_trayDrag.key.empty() && !g_trayDrag.fromList &&
                g_trayDrag.owner == this) {
                if (wParam & MK_LBUTTON) {
                    DragTrayIcon();
                }
                return 0;
            }
            // Selecting in the reply box.
            if (m_replyPressed && (wParam & MK_LBUTTON)) {
                if (EditTextDrag(hWnd, {(short)LOWORD(lParam),
                                        (short)HIWORD(lParam)})) {
                    Render(true);
                }
                return 0;
            }
            // Its tray icons are the ones explorer is told about now (a
            // notch's, when they're in it).
            if (g_trayIsland != this && InThisNotch(PillGroup::Tray)) {
                g_trayIsland = this;
                m_sentTrayPlaces.clear();
            }
            // Dragging the pill: it follows the mouse once it has moved a
            // little.
            if (m_dragArmed && (wParam & MK_LBUTTON)) {
                POINT cursor;
                GetCursorPos(&cursor);
                const int threshold = (int)std::round(6 * m_scale);
                if (!m_dragMoved &&
                    (std::abs(cursor.x - m_dragFrom.x) > threshold ||
                     std::abs(cursor.y - m_dragFrom.y) > threshold)) {
                    m_dragMoved = true;
                    CloseAllPanels();
                    ShowTrayTip({});
                    m_press.Release();
                    m_hover = IslandItem::None;
                    m_hoverApp.clear();
                }
                if (m_dragMoved) {
                    if (m_roomWidth > 0) {
                        m_placeX = std::clamp(
                            (cursor.x - m_dragGrab.x - m_roomLeft) / m_roomWidth,
                            0.0f, 1.0f);
                    }
                    if (m_roomHeight > 0) {
                        m_placeY = std::clamp(
                            (cursor.y - m_dragGrab.y - m_roomTop) / m_roomHeight,
                            0.0f, 1.0f);
                    }
                    m_placeVelocityX = m_placeVelocityY = 0;
                    Render();
                    return 0;
                }
            }
            std::wstring hoverApp;
            const IslandItem hover = HitTest(
                {(short)LOWORD(lParam), (short)HIWORD(lParam)}, &hoverApp);
            if (hover != m_hover || hoverApp != m_hoverApp) {
                m_hover = hover;
                m_hoverApp = hoverApp;
                // The folded apps out under the mouse, back a moment after
                // it leaves them.
                if (hover == IslandItem::AppsFold || hover == IslandItem::App) {
                    OpenFoldedApps(kAppsFoldMs);
                }
                // Another icon (or none): its pin waits.
                const bool onTrayIcon = hover == IslandItem::TrayIcon ||
                                        hover == IslandItem::TrayPin;
                if (!onTrayIcon || hoverApp != m_pinShownKey) {
                    m_pinShownKey.clear();
                    KillTimer(m_hwnd, kPinTimerId);
                    if (onTrayIcon) {
                        SetTimer(m_hwnd, kPinTimerId, kPinDelayMs, nullptr);
                    }
                }
                // The name shows after a moment, or right away when moving
                // from one icon to the next.
                const std::wstring tipKey =
                    hover == IslandItem::TrayIcon ? hoverApp
                    : hover == IslandItem::App    ? kAppTipPrefix + hoverApp
                    : HasItemTip(hover) ? ItemTipKey(hover, hoverApp)
                                        : std::wstring{};
                KillTimer(m_hwnd, kTipTimerId);
                if (TextBubbleShown()) {
                    m_pendingTipKey.clear();
                    ShowTrayTip({});
                } else if (tipKey.empty() || !m_tooltipKey.empty()) {
                    m_pendingTipKey.clear();
                    ShowTrayTip(tipKey);
                } else {
                    m_pendingTipKey = tipKey;
                    SetTimer(m_hwnd, kTipTimerId, GetDoubleClickTime(),
                             nullptr);
                }
                Render();
            }
            KillTimer(m_hwnd, kTrayCloseTimerId);
            // The line and the banner's reply row follow the mouse.
            if (m_bannerWanted || m_minimized) {
                StartAnimating();
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            m_tracking = false;
            // The banner goes a moment after the mouse leaves it, and the
            // tray icons' list a bit later.
            if (m_bannerWanted && !m_bannerTyping && !m_banner.sticky) {
                SetTimer(m_hwnd, kBannerTimerId, 2000, nullptr);
                StartAnimating();
            }
            if (m_trayOpen && !m_trayKeepOpen) {
                SetTimer(m_hwnd, kTrayCloseTimerId, 4000, nullptr);
            }
            if (m_appsOpen) {
                SetTimer(m_hwnd, kAppsFoldTimerId, kAppsFoldMs, nullptr);
            }
            KillTimer(m_hwnd, kTipTimerId);
            m_pendingTipKey.clear();
            ShowTrayTip({});
            if (m_press.held) {
                m_press.Release();
                StartAnimating();
            }
            if (m_hover != IslandItem::None) {
                m_hover = IslandItem::None;
                m_hoverApp.clear();
                StartAnimating();
                Render();
            }
            return 0;

        case WM_LBUTTONDOWN: {
            if (PressBannerReply({(short)LOWORD(lParam),
                                  (short)HIWORD(lParam)})) {
                return 0;
            }
            // Another app's tray icon: its app gets the click as the button
            // comes up; moved a little, it's dragged to another place (see
            // DragTrayIcon).
            std::wstring itemKey;
            const IslandItem item = HitTest(
                {(short)LOWORD(lParam), (short)HIWORD(lParam)}, &itemKey);
            if (item == IslandItem::TrayIcon) {
                g_trayDrag = {itemKey, this};
                GetCursorPos(&g_trayDrag.from);
                SetCapture(m_hwnd);
                return 0;
            }
            // The pill can be dragged from anywhere but the tray icons (their
            // apps get the click) and the line; a click still clicks.
            if (g_settings.islandFreeDrag && !m_bar && m_notch < 0 &&
                !m_bannerTyping && m_bannerAmount <= 0.01 && !m_minimized &&
                item != IslandItem::TrayIcon && item != IslandItem::TrayPin) {
                m_dragArmed = true;
                m_dragMoved = false;
                GetCursorPos(&m_dragFrom);
                m_dragGrab = {
                    m_dragFrom.x - (m_position.x + (LONG)std::lround(m_pill.left)),
                    m_dragFrom.y - (m_position.y + (LONG)std::lround(m_pill.top))};
                SetCapture(m_hwnd);
            }
            OnMouseButton(msg, lParam);
            return 0;
        }

        case WM_LBUTTONUP:
            if (!g_trayDrag.key.empty() && !g_trayDrag.fromList &&
                g_trayDrag.owner == this) {
                FinishTrayDrag(true);
                return 0;
            }
            // (The press in the reply box was its click.)
            if (m_replyPressed) {
                m_replyPressed = false;
                EndEditDrag(hWnd);
                return 0;
            }
            if (m_dragArmed) {
                m_dragArmed = false;
                const bool moved = m_dragMoved;
                if (GetCapture() == m_hwnd) {
                    ReleaseCapture();
                }
                if (moved) {
                    m_dragMoved = false;
                    FinishDrag();
                    return 0;
                }
            }
            OnMouseButton(msg, lParam);
            return 0;

        case WM_CAPTURECHANGED:
            if (!g_trayDrag.key.empty() && !g_trayDrag.fromList &&
                g_trayDrag.owner == this && (HWND)lParam != m_hwnd) {
                FinishTrayDrag(false);
            }
            if (m_replyPressed && (HWND)lParam != m_hwnd) {
                m_replyPressed = false;
                EndEditDrag(m_hwnd);
            }
            if (m_dragArmed && (HWND)lParam != m_hwnd) {
                m_dragArmed = false;
                if (m_dragMoved) {
                    m_dragMoved = false;
                    FinishDrag();
                }
            }
            break;

        case WM_LBUTTONDBLCLK:
            // (A word in the reply box: see EditTextClick.)
            if (PressBannerReply({(short)LOWORD(lParam),
                                  (short)HIWORD(lParam)})) {
                return 0;
            }
            OnMouseButton(msg, lParam);
            return 0;

        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            OnMouseButton(msg, lParam);
            return 0;

        case WM_APP_TRAY:
            UpdateTrayIcons();
            Render();
            return 0;

        case WM_MOUSEHWHEEL:
            if (ScrollTrayList(GET_WHEEL_DELTA_WPARAM(wParam) / 120.0)) {
                return 0;
            }
            if (m_hover == IslandItem::Media &&
                m_mediaShown.sources.size() > 1) {
                StepMediaSource(GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1 : -1);
                return 0;
            }
            if (!m_bannerWanted) {
                ScrollPill(GET_WHEEL_DELTA_WPARAM(wParam) / 120.0);
            }
            return 0;
        case WM_MOUSEWHEEL:
            // The wheel on a long tray list: it scrolls.
            if (ScrollTrayList(-GET_WHEEL_DELTA_WPARAM(wParam) / 120.0)) {
                return 0;
            }
            // Shift and the wheel on what's playing: another app's media.
            if ((GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT) &&
                m_hover == IslandItem::Media &&
                m_mediaShown.sources.size() > 1) {
                StepMediaSource(GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? 1 : -1);
                return 0;
            }
            // Shift and the wheel on the profiles' button: another profile;
            // on the volume, another output device.
            if ((GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT) &&
                ((m_hover == IslandItem::Profiles && !g_profiles.empty()) ||
                 m_hover == IslandItem::Volume)) {
                StepShiftChoice(m_hover == IslandItem::Profiles
                                    ? ShiftChoice::Profile
                                    : ShiftChoice::Output,
                                GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? 1 : -1);
                return 0;
            }
            // A pill longer than its screen: the wheel slides along it (with
            // Shift anywhere, or off the items whose wheel does something;
            // not while a notification is open on it).
            if (!m_bannerWanted &&
                ((GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT) ||
                 (m_hover != IslandItem::Volume &&
                  m_hover != IslandItem::Desktops &&
                  m_hover != IslandItem::Wallpaper &&
                  m_hover != IslandItem::Brightness &&
                  m_hover != IslandItem::Media &&
                  m_hover != IslandItem::MediaPrevious &&
                  m_hover != IslandItem::MediaPlayPause &&
                  m_hover != IslandItem::MediaNext &&
                  m_hover != IslandItem::BannerReply)) &&
                ScrollPill(-GET_WHEEL_DELTA_WPARAM(wParam) / 120.0)) {
                return 0;
            }
            // Over a reply on more lines than shown: it scrolls.
            if (m_hover == IslandItem::BannerReply && m_bannerReplyLines >= 5) {
                EditTextWheel(m_bannerEdit, GET_WHEEL_DELTA_WPARAM(wParam));
                Render(true);
                return 0;
            }
            // The wheel on the wallpaper: the folder's next or previous one.
            if (m_hover == IslandItem::Wallpaper) {
                StepWallpaper(GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? 1 : -1);
                return 0;
            }
            // The wheel on the desktops goes to the next or previous one.
            if (m_hover == IslandItem::Desktops) {
                StepVirtualDesktop(GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? 1
                                                                      : -1);
                return 0;
            }
            // The wheel on the brightness changes it, 5% a notch.
            if (m_hover == IslandItem::Brightness && m_brightness >= 0) {
                if (const int steps = BrightnessWheelSteps(
                        m_brightnessWheel, GET_WHEEL_DELTA_WPARAM(wParam))) {
                    SetBrightness(m_brightness + steps);
                }
                return 0;
            }
            // The wheel on what's playing, when chosen: the next song (down)
            // or the one before (up), once a notch however fast it turns.
            if (g_settings.islandMediaWheel &&
                (m_hover == IslandItem::Media ||
                 m_hover == IslandItem::MediaPrevious ||
                 m_hover == IslandItem::MediaPlayPause ||
                 m_hover == IslandItem::MediaNext)) {
                const double now = NowSeconds();
                if (now - m_mediaWheelAt > kMediaWheelSeconds) {
                    m_mediaWheelDelta = 0;
                }
                m_mediaWheelAt = now;
                m_mediaWheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
                if (std::abs(m_mediaWheelDelta) >= WHEEL_DELTA &&
                    now - m_mediaSkippedAt >= kMediaWheelSeconds) {
                    SendMediaCommand(m_mediaWheelDelta < 0
                                         ? MediaCommand::Next
                                         : MediaCommand::Previous);
                    m_mediaSkippedAt = now;
                    m_mediaWheelDelta = 0;
                }
                return 0;
            }
            // The wheel on the volume changes it, 2% a notch.
            if (m_hover == IslandItem::Volume || m_hover == IslandItem::Media ||
                m_hover == IslandItem::MediaPrevious ||
                m_hover == IslandItem::MediaPlayPause ||
                m_hover == IslandItem::MediaNext) {
                if (auto volume = GetVolume(nullptr)) {
                    float level = 0;
                    volume->GetMasterVolumeLevelScalar(&level);
                    const float step =
                        0.02f * GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
                    volume->SetMasterVolumeLevelScalar(
                        std::clamp(level + step, 0.0f, 1.0f), nullptr);
                    if (level + step > 0) {
                        volume->SetMute(FALSE, nullptr);
                    }
                }
                OnVolumeChanged();
                m_control.Refresh();
                // Without the percentage beside it: shown under it a moment.
                if (m_hover == IslandItem::Volume &&
                    !g_settings.islandVolumePercent &&
                    !m_volumeText.text.empty()) {
                    ShowTextBubble(m_volumeText.text,
                                   SlotMiddle(IslandItem::Volume));
                }
            }
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void StartPanelAnimation(Island* owner) {
    if (owner) {
        owner->StartAnimating();
    }
}

void StartIslandAnimation() {
    g_frameRequests++;
    ForEachIsland([](Island& island) { island.StartAnimating(); });
}

void RunIslandFrames() {
    for (Island* island : std::vector<Island*>(g_islands)) {
        if (island->Animating()) {
            IslandScope scope(island);
            island->AnimationFrame();
        }
    }
}

// The icons' thread read pictures (see IconThreadProc): every island drawn
// again (a message to the thread, not to a window: see PumpMessages).
void OnIconsRead() {
    ForEachIsland([](Island& island) { island.RedrawIcons(); });
}

// Handles messages, and returns false on WM_QUIT.
bool PumpMessages() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        if (msg.message == WM_APP_ICONS && !msg.hwnd) {
            OnIconsRead();
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

// The islands' and their panels' window classes, registered once for all of
// them by the island's thread (and unregistered after the last one is gone).
bool RegisterIslandClasses() {
    WNDCLASS windowClass{};
    windowClass.hInstance = g_module;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpfnWndProc = IslandWndProc;
    windowClass.lpszClassName = kIslandClassName;
    // Double clicks are passed on to the tray icons.
    windowClass.style = CS_DBLCLKS;
    if (!RegisterClass(&windowClass)) {
        IslandLog(L"RegisterClass failed: %u", GetLastError());
        return false;
    }
    windowClass.style = 0;
    windowClass.lpfnWndProc = PanelWndProc;
    windowClass.lpszClassName = kPanelClassName;
    if (!RegisterClass(&windowClass)) {
        IslandLog(L"RegisterClass failed: %u", GetLastError());
        UnregisterClass(kIslandClassName, g_module);
        return false;
    }
    // (Double clicks select a word of the reply, as on the pill.)
    windowClass.style = CS_DBLCLKS;
    windowClass.lpfnWndProc = BannerWndProc;
    windowClass.lpszClassName = kBannerClassName;
    if (!RegisterClass(&windowClass)) {
        IslandLog(L"RegisterClass failed: %u", GetLastError());
        UnregisterClass(kIslandClassName, g_module);
        UnregisterClass(kPanelClassName, g_module);
        return false;
    }
    windowClass.style = 0;
    windowClass.hCursor = nullptr;
    windowClass.lpfnWndProc = GlassWndProc;
    windowClass.lpszClassName = kGlassClassName;
    if (!RegisterClass(&windowClass)) {
        IslandLog(L"RegisterClass failed: %u", GetLastError());
        UnregisterClass(kIslandClassName, g_module);
        UnregisterClass(kPanelClassName, g_module);
        UnregisterClass(kBannerClassName, g_module);
        return false;
    }
    return true;
}

void UnregisterIslandClasses() {
    UnregisterClass(kIslandClassName, g_module);
    UnregisterClass(kPanelClassName, g_module);
    UnregisterClass(kBannerClassName, g_module);
    UnregisterClass(kGlassClassName, g_module);
}

// The settings' window open as the islands started again (a moment ago):
// back, on the island of the same monitor (the main one if it's gone).
void ReopenSettingsWindow() {
    const std::vector<SettingsWindowState> states =
        std::move(g_settingsWindowStates);
    g_settingsWindowStates.clear();
    for (const SettingsWindowState& state : states) {
        if (NowSeconds() - state.at > 10) {
            continue;
        }
        Island* chosen = state.device.empty() ? MainIsland() : nullptr;
        ForEachIsland([&](Island& island) {
            if (!state.device.empty() && !island.IsNotchSibling() &&
                island.OwnDevice() == state.device) {
                chosen = &island;
            }
        });
        if (!chosen) {
            chosen = MainIsland();
        }
        if (chosen) {
            chosen->OpenSettingsWindow(&state);
        }
    }
}

// Times in a row the main island couldn't be made (see IslandThreadProc).
int g_islandTries = 0;

