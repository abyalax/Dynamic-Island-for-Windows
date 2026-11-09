#pragma once
void Island::CheckAlarms() {
    if (!m_main || m_notchBase) {
        return;
    }
    SYSTEMTIME now;
    GetLocalTime(&now);
    const long long minuteKey =
        (long long)DayNumber(now.wYear, now.wMonth, now.wDay) * 24 * 60 +
        now.wHour * 60 + now.wMinute;
    if (minuteKey <= g_alarmsMinute) {
        return;
    }
    const long long from =
        g_alarmsMinute < 0
            ? minuteKey + 1
            : std::max(g_alarmsMinute + 1, minuteKey - kAlarmsMostMinutes + 1);
    // The minutes after the last look that are further back than the ones
    // looked at now (a long sleep): their alarms rang for nobody.
    const long long skippedFrom =
        std::max(g_alarmsMinute + 1, from - 7 * 24 * 60);
    const long long skippedTo = g_alarmsMinute < 0 ? -1 : from - 1;
    g_alarmsMinute = minuteKey;
    const long long today = DayNumber(now.wYear, now.wMonth, now.wDay);
    bool onceRang = false;
    for (auto& alarm : Alarms()) {
        if (!alarm.on) {
            continue;
        }
        if (!AlarmDueOn(alarm, from, minuteKey, today, now.wDayOfWeek)) {
            // (A once alarm whose minute went by unseen is off, not rung a
            // day late: it has no day to wait for.)
            if (!alarm.days &&
                AlarmDueOn(alarm, skippedFrom, skippedTo, today,
                           now.wDayOfWeek)) {
                alarm.on = false;
                onceRang = true;
            }
            continue;
        }
        // (Once: off now, like the iPhone's.)
        if (!alarm.days) {
            alarm.on = false;
            onceRang = true;
        }
        IslandToast toast;
        toast.glyph = 0xE823;
        toast.glyphColor = 0xFF9F0A;
        toast.sticky = true;
        toast.title = alarm.name.empty() ? Tr(L"Alarm", L"Alarme") : alarm.name;
        toast.text = AlarmTimeText(alarm.minute);
        toast.reply.actions.push_back(
            {Tr(L"Stop", L"Parar"), L"alarm-stop", L"island"});
        // (Over and over until its Stop, a minute at most.)
        if (alarm.sound && !QuietSilencesAll() &&
            PlaySound(L"Notification.Looping.Alarm", nullptr,
                      SND_ALIAS | SND_ASYNC | SND_NODEFAULT | SND_LOOP)) {
            m_alarmRinging = true;
            SetTimer(m_hwnd, kAlarmSoundStopTimerId, kUrgentSoundMaxMs,
                     nullptr);
        }
        AcquireSRWLockExclusive(&g_islandToastsLock);
        g_islandToasts.push_back(std::move(toast));
        ReleaseSRWLockExclusive(&g_islandToastsLock);
        ShowBanner();
    }
    if (onceRang) {
        SaveAlarms();
        m_timerPanel.Changed();
    }
}

// The alarm's sound, not a call's ringing that came after it (one sound at
// a time: that one took its place).
void Island::StopAlarmSound() {
    KillTimer(m_hwnd, kAlarmSoundStopTimerId);
    if (m_alarmRinging) {
        m_alarmRinging = false;
        if (!m_urgentRinging) {
            PlaySound(nullptr, nullptr, 0);
        }
    }
}

void Island::CheckTimer() {
    CheckAlarms();
    // The timer on the minimized line coming or going (see TimerLineWanted).
    if (HiddenClockWanted() != m_hiddenClockWanted) {
        StartAnimating();
    }
    if (g_settings.islandTimer && AnyTimerActive() && m_visible) {
        // Their time on the pill, and in their panel.
        Relayout();
        m_timerPanel.Refresh();
    }
    if (!m_main || m_notchBase) {
        return;
    }
    // Each countdown's end its own notice (they queue, see ShowBanner).
    bool ended = false;
    for (TimerMode mode : {TimerMode::Timer, TimerMode::Pomodoro}) {
        TimerClock& clock = TimerClockOf(mode);
        const int length = TimerLength(mode);
        if (!clock.running || !length || clock.Counted() < length) {
            continue;
        }
        IslandToast toast;
        toast.glyph = 0xE916;
        toast.glyphColor = 0xFF9F0A;
        toast.sticky = true;
        // Its buttons, like the iPhone's (see OnOwnNoticeAction).
        const auto button = [&](PCWSTR label, PCWSTR arguments) {
            toast.reply.actions.push_back({label, arguments, L"island"});
        };
        if (mode == TimerMode::Pomodoro) {
            button(Tr(L"Stop", L"Parar"), L"pomodoro-stop");
            button(Tr(L"OK", L"OK"), L"");
            // On to the next part, from when this one ended (noticed a
            // moment later; not after a long sleep of the computer).
            const double over = clock.Counted() - length;
            if (!g_timer.onBreak) {
                g_timer.rounds++;
            }
            g_timer.onBreak = !g_timer.onBreak;
            clock.counted = 0;
            clock.startedAt =
                NowSeconds() - (over > 0 && over < 60 ? over : 0);
            toast.title = g_timer.onBreak
                              ? Tr(L"Time for a break", L"Hora da pausa")
                              : Tr(L"Back to focus", L"De volta ao foco");
            toast.text = TimerModeName(mode, g_timer.onBreak) +
                         std::wstring(L" · ") + TimerText(mode);
        } else {
            button(Tr(L"Repeat", L"Repetir"), L"timer-repeat");
            button(Tr(L"Stop", L"Parar"), L"");
            toast.title = Tr(L"Time's up", L"Acabou o tempo");
            toast.text = DurationText(length, false);
            clock.Reset();
        }
        AcquireSRWLockExclusive(&g_islandToastsLock);
        g_islandToasts.push_back(std::move(toast));
        ReleaseSRWLockExclusive(&g_islandToastsLock);
        ended = true;
    }
    if (!ended) {
        return;
    }
    MessageBeep(MB_ICONASTERISK);
    ShowBanner();
    ForEachIsland([](Island& island) {
        island.Relayout();
        island.m_timerPanel.Changed();
    });
}

// A timer's end again, Pomodoro stopped, an alarm's sound stopped, What's
// new opened; the notice is gone either way.
void Island::OnOwnNoticeAction(const std::wstring& arguments) {
    if (arguments == L"alarm-stop") {
        if (Island* main = MainIsland()) {
            main->StopAlarmSound();
        }
        return;
    }
    if (arguments == L"whats-new") {
        OpenWhatsNew();
        return;
    }
    if (arguments == L"timer-repeat") {
        TimerClock& clock = TimerClockOf(TimerMode::Timer);
        clock.Reset();
        clock.Start();
    } else if (arguments == L"pomodoro-stop") {
        TimerClockOf(TimerMode::Pomodoro).Reset();
        g_timer.onBreak = false;
        g_timer.rounds = 0;
    } else {
        return;
    }
    ForEachIsland([](Island& island) {
        island.Relayout();
        island.m_timerPanel.Changed();
    });
}

void Island::ShowWhatsNewNotice() {
    if (!g_settings.islandWhatsNewNotice) {
        return;
    }
    SetUserStringValue(L"whatsNewTold", L"" WH_MOD_VERSION);
    IslandToast toast;
    toast.glyph = 0xE789;
    toast.glyphColor = 0x0A84FF;
    // (Once a version: it waits for its buttons, like an alarm's.)
    toast.sticky = true;
    toast.title = L"Mac Island & Dock " WH_MOD_VERSION;
    toast.text = Tr(L"See what's new in this version",
                    L"Veja o que há de novo nesta versão");
    toast.reply.actions.push_back(
        {Tr(L"What's new", L"Novidades"), L"whats-new", L"island"});
    toast.reply.actions.push_back({Tr(L"Later", L"Depois"), L"", L"island"});
    AcquireSRWLockExclusive(&g_islandToastsLock);
    g_islandToasts.push_back(std::move(toast));
    ReleaseSRWLockExclusive(&g_islandToastsLock);
    ShowBanner();
}

void Island::OpenWhatsNew() {
    Island& base = m_notchBase ? *m_notchBase : *this;
    base.OpenSettingsWindow();
    base.m_settingsWindow.GoToTab(kTabWhatsNew);
}

void Island::OpenSetup() {
    g_firstInstall = false;
    SetUserStringValue(L"whatsNewTold", L"" WH_MOD_VERSION);
    Island& base = m_notchBase ? *m_notchBase : *this;
    base.OpenSettingsWindow();
    base.m_settingsWindow.GoToTab(kTabSetup);
}

// A short notice from the pill (the main island's, see ShowBanner), like a
// drive's: its glyph in a circle of its color; a click only closes it.
void Island::ShowStatusNotice(WCHAR glyph,
                              int color,
                              const std::wstring& title,
                              const std::wstring& text) {
    if (!g_settings.islandStatusNotices) {
        return;
    }
    IslandToast toast;
    toast.title = title;
    toast.text = text;
    toast.glyph = glyph;
    toast.glyphColor = color;
    AcquireSRWLockExclusive(&g_islandToastsLock);
    g_islandToasts.push_back(std::move(toast));
    ReleaseSRWLockExclusive(&g_islandToastsLock);
    ShowBanner();
}

void ShowIslandNotice(WCHAR glyph,
                      int color,
                      const std::wstring& title,
                      const std::wstring& text) {
    if (Island* main = MainIsland()) {
        IslandScope scope(main);
        main->ShowStatusNotice(glyph, color, title, text);
    }
}

// The charger plugged in, and the battery going under 20% and 10% (each
// once until it charges again). Only on a computer with a battery.
void Island::CheckPowerNotices() {
    if (!m_main || m_notchBase) {
        return;
    }
    SYSTEM_POWER_STATUS power;
    if (!GetSystemPowerStatus(&power) || (power.BatteryFlag & 128) ||
        power.BatteryLifePercent > 100) {
        return;
    }
    const int percent = power.BatteryLifePercent;
    const int charging = power.ACLineStatus == 1 ? 1 : 0;
    const bool first = m_powerSeen < 0;
    const std::wstring level = std::to_wstring(percent) + L"%";
    if (charging != m_powerSeen) {
        m_powerSeen = charging;
        if (charging) {
            m_lowBatteryTold = 101;
            if (!first) {
                ShowStatusNotice(0xE945, 0x34C759,
                                 Tr(L"Charging", L"Carregando"), level);
            }
        }
    }
    if (charging) {
        return;
    }
    for (int threshold : {10, 20}) {
        if (percent <= threshold && m_lowBatteryTold > threshold) {
            m_lowBatteryTold = threshold;
            if (!first) {
                ShowStatusNotice(0xE850, 0xFF3B30,
                                 Tr(L"Low battery", L"Bateria fraca"), level);
            }
            break;
        }
    }
}

// "Do not disturb" (as the island shows it, see IsIslandQuiet) turned on
// or off.
void Island::NoteQuiet(bool quiet) {
    if (!m_main || m_notchBase || m_quietSeen == (int)quiet) {
        return;
    }
    const bool first = m_quietSeen < 0;
    m_quietSeen = quiet;
    if (!first) {
        ShowStatusNotice(0xE708, 0x5E5CE6,
                         Tr(L"Do not disturb", L"Não incomodar"),
                         quiet ? Tr(L"Turned on", L"Ativado")
                               : Tr(L"Turned off", L"Desativado"));
    }
}

// The Bluetooth audio outputs on now (their endpoints' IDs) and their
// devices' names: an output's filter is a Bluetooth one (its ID has the
// Bluetooth enumerator's name, see SetBluetoothAudioConnected).
void BluetoothOutputs(IMMDeviceEnumerator* devices,
                      std::vector<std::wstring>* ids,
                      std::vector<std::wstring>* names) {
    // DEVPKEY_DeviceInterface_FriendlyName: the device's own name (the
    // endpoint's adds what it is, like "Headphones (...)").
    constexpr PROPERTYKEY kInterfaceName = {
        {0x026e516e,
         0xb814,
         0x414b,
         {0x83, 0xcd, 0x85, 0x6d, 0x6f, 0xef, 0x48, 0x22}},
        2};
    winrt::com_ptr<IMMDeviceCollection> endpoints;
    UINT count = 0;
    if (FAILED(devices->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE,
                                           endpoints.put())) ||
        FAILED(endpoints->GetCount(&count))) {
        return;
    }
    for (UINT i = 0; i < count; i++) {
        winrt::com_ptr<IMMDevice> endpoint;
        winrt::com_ptr<IDeviceTopology> topology;
        winrt::com_ptr<IConnector> connector;
        LPWSTR filterId = nullptr;
        LPWSTR endpointId = nullptr;
        if (FAILED(endpoints->Item(i, endpoint.put())) ||
            FAILED(endpoint->Activate(kIID_IDeviceTopology, CLSCTX_ALL,
                                      nullptr, topology.put_void())) ||
            FAILED(topology->GetConnector(0, connector.put())) ||
            FAILED(connector->GetDeviceIdConnectedTo(&filterId))) {
            continue;
        }
        std::wstring filter = filterId;
        CoTaskMemFree(filterId);
        std::transform(filter.begin(), filter.end(), filter.begin(), towupper);
        if (filter.find(L"BTHENUM") == std::wstring::npos &&
            filter.find(L"BTHHFENUM") == std::wstring::npos) {
            continue;
        }
        if (FAILED(endpoint->GetId(&endpointId))) {
            continue;
        }
        ids->push_back(endpointId);
        CoTaskMemFree(endpointId);
        std::wstring name;
        winrt::com_ptr<IPropertyStore> properties;
        if (SUCCEEDED(endpoint->OpenPropertyStore(STGM_READ,
                                                  properties.put()))) {
            for (const PROPERTYKEY& key :
                 {kInterfaceName, kPKEY_Device_FriendlyName}) {
                PROPVARIANT value;
                PropVariantInit(&value);
                if (name.empty() &&
                    SUCCEEDED(properties->GetValue(key, &value)) &&
                    value.vt == VT_LPWSTR && value.pwszVal) {
                    name = value.pwszVal;
                }
                PropVariantClear(&value);
            }
        }
        names->push_back(name);
    }
}

// A Bluetooth audio device connected: its name, once (a headset has more
// than one output: stereo and hands-free).
void Island::CheckBluetoothOutputs() {
    if (!m_main || m_notchBase || !m_devices) {
        return;
    }
    std::vector<std::wstring> ids;
    std::vector<std::wstring> names;
    BluetoothOutputs(m_devices.get(), &ids, &names);
    std::vector<std::wstring> told;
    for (size_t i = 0; m_bluetoothKnown && i < ids.size(); i++) {
        const bool before = std::find(m_bluetoothOutputs.begin(),
                                      m_bluetoothOutputs.end(),
                                      ids[i]) != m_bluetoothOutputs.end();
        if (before || names[i].empty() ||
            std::find(told.begin(), told.end(), names[i]) != told.end()) {
            continue;
        }
        told.push_back(names[i]);
        ShowStatusNotice(0xE7F6, 0x0A84FF, names[i],
                         Tr(L"Connected", L"Conectado"));
    }
    m_bluetoothKnown = true;
    m_bluetoothOutputs = std::move(ids);
}

// A reminder in the pill, like a notification (on the calendar's day when
// clicked), with the reminder sound.
void Island::ShowReminder(const CalendarEvent& event) {
    IslandLog(L"Reminder shown (island %s, notices here %d)",
              m_visible ? L"shown" : L"away until it's back",
              (int)ShowsNotices());
    MessageBeep(MB_ICONASTERISK);
    IslandToast toast;
    toast.title = event.title;
    toast.text = EventTimeText(event.minute);
    toast.sticky = true;
    m_reminderDay = {event.year, event.month, event.day};
    AcquireSRWLockExclusive(&g_islandToastsLock);
    g_islandToasts.push_back(std::move(toast));
    ReleaseSRWLockExclusive(&g_islandToastsLock);
    ShowBanner();
}

Island::UnreadApp* Island::FindUnread(const std::wstring& appId) {
    for (auto& app : m_unreadApps) {
        if (_wcsicmp(app.id.c_str(), appId.c_str()) == 0) {
            return &app;
        }
    }
    return nullptr;
}

void Island::LoadUnread() {
    m_unreadApps.clear();
    const std::wstring value = GetWholeStringValue(L"unreadApps", 8192);
    for (PCWSTR line = value.data(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        std::wstring id = NotificationAppKey(
            end ? std::wstring(line, end) : std::wstring(line));
        if (!id.empty() && !FindUnread(id)) {
            m_unreadApps.push_back({std::move(id)});
        }
        if (!end) {
            break;
        }
        line = end + 1;
    }
    m_unseen = Wh_GetIntValue(L"notificationsUnseen", 0) != 0;
}

// Kept by the main island only (the others hold the same apps).
void Island::SaveUnread() {
    if (!m_main) {
        return;
    }
    std::wstring value;
    for (const auto& app : m_unreadApps) {
        value += app.id + L"\n";
    }
    SetStoredString(L"unreadApps", value.c_str());
    Wh_SetIntValue(L"notificationsUnseen", m_unseen);
}

// A new notification: its app shows on the island with the red dot (not if
// it's the app in front), and the bell gets its dot.
void Island::AddUnread(const IslandToast& toast) {
    constexpr size_t kMaxUnreadApps = 12;
    if (toast.appId.empty() || toast.sticky) {
        return;
    }
    const std::wstring appId = NotificationAppKey(toast.appId);
    std::erase_if(m_clearedBadges, [&](const std::wstring& id) {
        return _wcsicmp(id.c_str(), appId.c_str()) == 0;
    });
    // (Minimized: it peeks out of the line, on the notch that shows the
    // notifications.)
    const bool peeks = m_minimized && !m_bar && ShowsNotices();
    // From the app in front: already seen (no dot on the island or the bell).
    if (UnreadApp* app = FindUnread(appId)) {
        app->reply = toast.reply;
    } else if (WindowMatchesApp(GetForegroundWindow(), appId)) {
        return;
    } else {
        if (m_unreadApps.size() >= kMaxUnreadApps) {
            m_unreadApps.erase(m_unreadApps.begin());
        }
        m_unreadApps.push_back({appId, toast.reply});
        if (peeks) {
            m_peekApp = appId;
            m_peekStart = NowSeconds();
        }
    }
    m_unseen = true;
    SaveUnread();
    UpdateApps();
    OpenFoldedApps(3000);
    for (auto& entry : m_appEntries) {
        if (entry.present && entry.shown > 0.9 &&
            _wcsicmp(entry.id.c_str(), appId.c_str()) == 0) {
            entry.velocity += 3.5;
        }
    }
    if (peeks && (!g_bannerEnabled || !g_bannerWhenMinimized ||
                  IsSnoozed(toast.appId))) {
        m_peekApp = appId;
        m_peekStart = NowSeconds();
    }
    StartAnimating();
}

// The folded apps out of their arrow, back in after `forMs` unless the mouse
// is on them then (see kAppsFoldTimerId).
void Island::OpenFoldedApps(UINT forMs) {
    if (!g_settings.islandFoldApps) {
        return;
    }
    if (!m_appsOpen) {
        m_appsOpen = true;
        Relayout();
        StartAnimating();
    }
    SetTimer(m_hwnd, kAppsFoldTimerId, forMs, nullptr);
}

// Grouped by app or not (see islandGroupApps): the apps on the pill follow
// (taken apart again only by new notifications).
void Island::RegroupUnread() {
    std::vector<UnreadApp> apps = std::move(m_unreadApps);
    m_unreadApps.clear();
    for (auto& app : apps) {
        app.id = NotificationAppKey(app.id);
        if (UnreadApp* same = FindUnread(app.id)) {
            same->reply = app.reply;
        } else {
            m_unreadApps.push_back(std::move(app));
        }
    }
    SaveUnread();
    UpdateApps();
    StartAnimating();
    Render(true);
}

// How long an app is ignored after its banner's "x".
constexpr double kBannerSnoozeSeconds = 60;

bool Island::IsSnoozed(const std::wstring& appId) {
    const double now = NowSeconds();
    std::erase_if(m_snoozedApps,
                  [now](const auto& entry) { return entry.second <= now; });
    return std::any_of(
        m_snoozedApps.begin(), m_snoozedApps.end(), [&](const auto& entry) {
            return _wcsicmp(entry.first.c_str(), appId.c_str()) == 0;
        });
}

void Island::MarkAppRead(const std::wstring& appId) {
    // (A copy: `appId` may be one of an island's own strings.)
    const std::wstring id = appId;
    ForEachIsland([&](Island& island) { island.MarkAppReadHere(id); });
}

void Island::MarkAppReadHere(const std::wstring& appId) {
    const size_t before = m_unreadApps.size();
    const std::wstring key = NotificationAppKey(appId);
    std::erase_if(m_unreadApps, [&](const UnreadApp& app) {
        return _wcsicmp(app.id.c_str(), appId.c_str()) == 0 ||
               _wcsicmp(app.id.c_str(), key.c_str()) == 0;
    });
    if (m_unreadApps.size() != before) {
        // Nothing left to look at: the bell's dot goes too.
        if (m_unreadApps.empty()) {
            m_unseen = false;
        }
        SaveUnread();
        UpdateApps();
        StartAnimating();
        Render(true);
    }
}

void Island::ClearAppsFromList(const std::vector<std::wstring>& apps,
                               bool all) {
    auto listed = [&](const std::wstring& id) {
        return all ||
               std::any_of(apps.begin(), apps.end(),
                           [&](const std::wstring& app) {
                               return SameNotificationApp(app, id);
                           });
    };
    std::erase_if(m_unreadApps,
                  [&](const UnreadApp& app) { return listed(app.id); });
    for (const auto& id : m_badgeApps) {
        if (listed(id) &&
            std::find(m_clearedBadges.begin(), m_clearedBadges.end(), id) ==
                m_clearedBadges.end()) {
            m_clearedBadges.push_back(id);
        }
    }
    if (all || m_unreadApps.empty()) {
        m_unseen = false;
    }
    SaveUnread();
    UpdateApps();
    StartAnimating();
    Render(true);
}

void Island::OnToastsChanged() {
    const bool urgentShown = m_bannerWanted && m_banner.urgent;
    const bool urgentWaiting =
        std::any_of(m_waitingToasts.begin(), m_waitingToasts.end(),
                    [](const IslandToast& toast) { return toast.urgent; });
    if (m_unreadApps.empty() && !urgentShown && !urgentWaiting) {
        return;
    }
    std::vector<std::wstring> inCenter;
    if (!m_notifications.AppsInCenter(&inCenter)) {
        return;
    }
    auto present = [&](const std::wstring& appId) {
        return std::any_of(inCenter.begin(), inCenter.end(),
                           [&](const std::wstring& id) {
                               return _wcsicmp(id.c_str(), appId.c_str()) == 0;
                           });
    };
    // A call, an alarm or a reminder whose notification is gone (the call
    // ended, or it was handled elsewhere) leaves the pill.
    std::erase_if(m_waitingToasts, [&](const IslandToast& toast) {
        return toast.urgent && !present(toast.appId);
    });
    if (urgentShown && !present(m_banner.appId)) {
        HideBanner();
    }
    // (An app's kept ones count as there, see MergeKeptToasts.)
    for (const auto& [id, kept] : m_notifications.KeptCounts()) {
        inCenter.push_back(id);
    }
    DropGoneUnread(inCenter);
}

void Island::DropGoneUnread(const std::vector<std::wstring>& inCenter) {
    const size_t before = m_unreadApps.size();
    std::erase_if(m_unreadApps, [&](const UnreadApp& app) {
        return std::none_of(inCenter.begin(), inCenter.end(),
                            [&](const std::wstring& id) {
                                return _wcsicmp(id.c_str(), app.id.c_str()) ==
                                       0;
                            });
    });
    if (m_unreadApps.size() == before) {
        return;
    }
    if (m_unreadApps.empty()) {
        m_unseen = false;
    }
    SaveUnread();
    UpdateApps();
    m_notifications.Refresh();
    StartAnimating();
    Render(true);
}

// How long a notification snoozed on the island waits (the time Windows
// uses for one without a choice of its own).
constexpr double kToastSnoozeSeconds = 5 * 60;

void Island::SnoozeToast(IslandToast toast) {
    // Kept by the main island (it shows them again on every island).
    if (!m_main) {
        if (Island* main = MainIsland(); main && main != this) {
            IslandScope scope(main);
            main->SnoozeToast(std::move(toast));
        }
        return;
    }
    toast.draft.clear();
    // Snoozed again: from now (once).
    if (toast.id > 0) {
        std::erase_if(m_snoozedToasts, [&](const auto& entry) {
            return entry.first.id == toast.id;
        });
    }
    m_snoozedToasts.push_back({std::move(toast), NowSeconds() +
                                                     kToastSnoozeSeconds});
    SaveSnoozed();
}

// The clock of the computer, in seconds (for times kept across restarts).
double WallClockSeconds() {
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    return (double)(((ULONGLONG)now.dwHighDateTime << 32) |
                    now.dwLowDateTime) /
           1e7;
}

// A line per notification: "<ID> <when it's due>" (the island's own, like
// a calendar reminder, aren't in the database and aren't kept).
void Island::SaveSnoozed() {
    const double now = NowSeconds();
    const double wall = WallClockSeconds();
    std::wstring value;
    for (const auto& [toast, due] : m_snoozedToasts) {
        if (toast.id > 0) {
            value += std::to_wstring(toast.id) + L" " +
                     std::to_wstring((long long)(wall + (due - now))) + L"\n";
        }
    }
    SetStoredString(L"snoozedToasts", value.c_str());
}

// Read again from the notification database: the ones gone from it (cleared
// in Windows meanwhile) are dropped.
void Island::LoadSnoozed() {
    const std::wstring value = GetWholeStringValue(L"snoozedToasts", 4096);
    const double now = NowSeconds();
    const double wall = WallClockSeconds();
    bool dropped = false;
    for (PCWSTR line = value.data(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        long long id = 0;
        long long due = 0;
        IslandToast toast;
        if (swscanf(line, L"%lld %lld", &id, &due) == 2 &&
            !IsToastSnoozed(id) && ReadToastById(id, &toast)) {
            m_snoozedToasts.push_back(
                {std::move(toast), now + std::max(0.0, (double)due - wall)});
        } else {
            dropped = true;
        }
        if (!end) {
            break;
        }
        line = end + 1;
    }
    if (dropped) {
        SaveSnoozed();
    }
}

void Island::OnNotificationsSeen() {
    if (m_unseen) {
        m_unseen = false;
        SaveUnread();
        Render(true);
    }
}

// The newest notification the island has taken (by its ID in the database,
// for each user), so the ones that come while it isn't running are found
// when it starts (see CatchUpToasts).
void NoteToastsSeen(const std::vector<IslandToast>& toasts) {
    long long newest = 0;
    for (const auto& toast : toasts) {
        newest = std::max(newest, toast.id);
    }
    if (newest > GetUserIntValue(L"notificationsSeenId", 0)) {
        SetUserIntValue(L"notificationsSeenId", (int)newest);
    }
}

// The notifications that came while the island wasn't running (the computer
// off, the island turned off or starting again) and are still in Windows'
// list: their apps show on every island with the red dot, like new ones,
// without a banner or a sound. Once, as it starts (main island).
void Island::CatchUpToasts() {
    const long long seen = GetUserIntValue(L"notificationsSeenId", 0);
    long long newest = seen;
    const std::vector<IslandToast> toasts =
        m_notifications.MissedToasts(seen, &newest);
    if (newest > seen) {
        SetUserIntValue(L"notificationsSeenId", (int)newest);
    }
    if (toasts.empty()) {
        return;
    }
    IslandLog(L"%zu apps with notifications from while the island wasn't running",
           toasts.size());
    ForEachIsland([&](Island& island) {
        for (const auto& toast : toasts) {
            island.AddUnread(toast);
        }
    });
}

// The counts read again (for the apps on the island only), drawn again if
// they changed.
void Island::UpdateAppCounts() {
    // Read once, by the main island, and passed on to the others (the
    // database is read on the island thread).
    if (!m_main) {
        if (Island* main = MainIsland(); main && main != this) {
            ShowCenterCounts(main->m_centerCounts);
        }
        return;
    }
    std::vector<std::pair<std::wstring, int>> counts;
    if (!m_appEntries.empty() || !m_unreadApps.empty()) {
        counts = m_notifications.CountsInCenter();
        // (With the kept ones an app took out itself, see MergeKeptToasts.)
        for (const auto& [id, kept] : m_notifications.KeptCounts()) {
            auto it = std::find_if(counts.begin(), counts.end(),
                                   [&](const auto& entry) {
                                       return _wcsicmp(entry.first.c_str(),
                                                       id.c_str()) == 0;
                                   });
            if (it == counts.end()) {
                counts.push_back({id, kept});
            } else {
                it->second = std::max(it->second, kept);
            }
        }
    }
    ShowCenterCounts(counts);
    for (Island* other : g_islands) {
        if (other != this) {
            IslandScope scope(other);
            other->ShowCenterCounts(counts);
        }
    }
}

void Island::ShowCenterCounts(
    const std::vector<std::pair<std::wstring, int>>& counts) {
    if (counts != m_centerCounts) {
        m_centerCounts = counts;
        Render(true);
    }
}

// Its badge's count or its notifications in the center, the bigger one.
int Island::AppCount(const std::wstring& appId) const {
    int count = 0;
    for (size_t i = 0; i < m_badgeApps.size() && i < m_badgeCounts.size();
         i++) {
        if (_wcsicmp(m_badgeApps[i].c_str(), appId.c_str()) == 0) {
            count = m_badgeCounts[i];
        }
    }
    for (const auto& [id, center] : m_centerCounts) {
        if (_wcsicmp(id.c_str(), appId.c_str()) == 0) {
            count = std::max(count, center);
        }
    }
    return count;
}

// Takes the apps with a badge from the taskbar, and the ones with unread
// notifications: new ones grow in (and peek out of the line when the pill is
// minimized), the others shrink away.
void Island::UpdateApps() {
    AcquireSRWLockShared(&g_badgeAppsLock);
    m_badgeApps = g_badgeApps;
    m_badgeCounts = g_badgeCounts;
    ReleaseSRWLockShared(&g_badgeAppsLock);
    UpdateAppCounts();
    // A cleared badge counts again once it's gone (and comes back).
    std::erase_if(m_clearedBadges, [&](const std::wstring& id) {
        return std::find(m_badgeApps.begin(), m_badgeApps.end(), id) ==
               m_badgeApps.end();
    });
    std::vector<std::wstring> wanted;
    for (const auto& id : m_badgeApps) {
        if (std::find(m_clearedBadges.begin(), m_clearedBadges.end(), id) ==
            m_clearedBadges.end()) {
            wanted.push_back(id);
        }
    }
    for (const auto& app : m_unreadApps) {
        if (std::none_of(wanted.begin(), wanted.end(),
                         [&](const std::wstring& id) {
                             return _wcsicmp(id.c_str(), app.id.c_str()) == 0;
                         })) {
            wanted.push_back(app.id);
        }
    }
    // New apps grow in; apps without a badge anymore shrink away.
    bool changed = false;
    for (const auto& id : wanted) {
        auto it = std::find_if(m_appEntries.begin(), m_appEntries.end(),
                               [&](const AppEntry& e) { return e.id == id; });
        if (it == m_appEntries.end()) {
            m_appEntries.push_back({id});
            changed = true;
            if (m_minimized && !m_bar) {
                m_peekApp = id;
                m_peekStart = NowSeconds();
            }
        } else if (!it->present) {
            it->present = true;
            changed = true;
        }
    }
    for (auto& entry : m_appEntries) {
        const bool present =
            std::find(wanted.begin(), wanted.end(), entry.id) != wanted.end();
        if (entry.present && !present) {
            entry.present = false;
            changed = true;
        }
    }
    if (changed) {
        StartAnimating();
    }
}

// The Windows banner for a new notification is a window of the shell
// experience host that's uncloaked at the bottom right, starting with no
// height. The island shows notifications its own way (in the pill, or only
// its icons and the list), so it's moved off the screen as it shows (its
// sound still plays, and it still goes to the notification center). Windows
// places it again for each notification, so nothing stays changed if the mod
// stops.
HWND g_windowsBanner;
DWORD g_shellExperienceHostId;

bool IsShellExperienceHost(DWORD processId) {
    if (processId == g_shellExperienceHostId) {
        return true;
    }
    bool is = false;
    if (HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                     processId)) {
        WCHAR path[MAX_PATH];
        DWORD length = ARRAYSIZE(path);
        if (QueryFullProcessImageName(process, 0, path, &length)) {
            PCWSTR name = wcsrchr(path, L'\\');
            is = _wcsicmp(name ? name + 1 : path, L"ShellExperienceHost.exe") ==
                 0;
        }
        CloseHandle(process);
    }
    if (is) {
        g_shellExperienceHostId = processId;
    }
    return is;
}

void CALLBACK BannerWinEventProc(HWINEVENTHOOK,
                                 DWORD event,
                                 HWND hWnd,
                                 LONG idObject,
                                 LONG idChild,
                                 DWORD,
                                 DWORD) {
    if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF || !hWnd ||
        !g_island) {
        return;
    }
    IslandScope scope(MainIsland());
    if (event == kEventObjectUncloaked && hWnd != g_windowsBanner) {
        WCHAR className[64];
        RECT rect;
        DWORD processId = 0;
        MONITORINFO monitor{sizeof(monitor)};
        // Recognized when it shows: the shell experience host's window at
        // the bottom right corner of its monitor (with no height yet, or
        // already its size). Not a flyout the user opened (the notification
        // center, the calendar, the quick settings): those come to the
        // front.
        if (GetForegroundWindow() != hWnd &&
            GetClassName(hWnd, className, ARRAYSIZE(className)) &&
            wcscmp(className, L"Windows.UI.Core.CoreWindow") == 0 &&
            GetWindowRect(hWnd, &rect) &&
            GetMonitorInfo(MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST),
                           &monitor) &&
            rect.right >= monitor.rcMonitor.right - 40 &&
            rect.bottom >= monitor.rcMonitor.bottom - 200 &&
            GetWindowThreadProcessId(hWnd, &processId) &&
            IsShellExperienceHost(processId)) {
            g_windowsBanner = hWnd;
            g_island->WatchWindowsBannerMoves(processId);
        }
    }
    // The island started while a stack was already showing (no uncloaking
    // to see): known by its moves as banners join it, at the bottom right
    // of its monitor.
    if (event == EVENT_OBJECT_LOCATIONCHANGE && !g_windowsBanner) {
        WCHAR className[64];
        RECT rect;
        DWORD processId = 0;
        int cloaked = 1;
        MONITORINFO monitor{sizeof(monitor)};
        if (GetForegroundWindow() != hWnd &&
            GetClassName(hWnd, className, ARRAYSIZE(className)) &&
            wcscmp(className, L"Windows.UI.Core.CoreWindow") == 0 &&
            GetWindowRect(hWnd, &rect) && rect.bottom > rect.top &&
            SUCCEEDED(DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked,
                                            sizeof(cloaked))) &&
            !cloaked &&
            GetMonitorInfo(MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST),
                           &monitor) &&
            rect.right >= monitor.rcMonitor.right - 40 &&
            rect.bottom >= monitor.rcMonitor.bottom - 200 &&
            GetWindowThreadProcessId(hWnd, &processId) &&
            IsShellExperienceHost(processId)) {
            g_windowsBanner = hWnd;
            g_island->OnWindowsBannerSeen();
            g_island->OnWindowsBanner(hWnd, true);
        }
        return;
    }
    if (hWnd == g_windowsBanner) {
        if (event == kEventObjectCloaked) {
            g_island->OnWindowsBannersGone();
            return;
        }
        if (event == kEventObjectUncloaked) {
            g_island->OnWindowsBannerSeen();
        }
        g_island->OnWindowsBanner(hWnd, event == kEventObjectUncloaked);
    }
}

// Once the banner is known, its moves (only that process's windows).
void Island::WatchWindowsBannerMoves(DWORD processId) {
    if (m_bannerMoveHook) {
        UnhookWinEvent(m_bannerMoveHook);
    }
    m_bannerMoveHook = SetWinEventHook(
        EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, nullptr,
        BannerWinEventProc, processId, 0, WINEVENT_OUTOFCONTEXT);
}

constexpr GUID kCLSID_CUIAutomation8 = {
    0xe22ad333,
    0xb25f,
    0x460c,
    {0x83, 0xd0, 0x05, 0x81, 0x10, 0x73, 0x95, 0xc9}};
constexpr GUID kCLSID_CUIAutomation = {
    0xff48dba4,
    0x60ef,
    0x4201,
    {0xaa, 0x87, 0x54, 0x10, 0x3e, 0xef, 0x59, 0x4e}};
constexpr GUID kIID_IUIAutomation = {
    0x30cbe57d,
    0xd9d0,
    0x452a,
    {0xab, 0x13, 0x7a, 0xc5, 0xac, 0x48, 0x25, 0xee}};
constexpr GUID kIID_IUIAutomation2 = {
    0x34723aff,
    0x0c9d,
    0x49d0,
    {0x98, 0x96, 0x7a, 0xb5, 0x2d, 0xf8, 0xcd, 0x8a}};
constexpr GUID kIID_IUIAutomationInvokePattern = {
    0xfb377fbe,
    0x8ea6,
    0x46d5,
    {0x9c, 0x73, 0x64, 0x99, 0x64, 0x2d, 0x30, 0x59}};

// The elements under `parent` with an automation ID.
winrt::com_ptr<IUIAutomationElementArray> FindByAutomationId(
    IUIAutomation* uia,
    IUIAutomationElement* parent,
    PCWSTR id) {
    winrt::com_ptr<IUIAutomationElementArray> found;
    VARIANT value;
    VariantInit(&value);
    value.vt = VT_BSTR;
    value.bstrVal = SysAllocString(id);
    winrt::com_ptr<IUIAutomationCondition> condition;
    if (value.bstrVal &&
        SUCCEEDED(uia->CreatePropertyCondition(UIA_AutomationIdPropertyId,
                                               value, condition.put()))) {
        parent->FindAll(TreeScope_Descendants, condition.get(), found.put());
    }
    VariantClear(&value);
    return found;
}

std::vector<winrt::com_ptr<IUIAutomationElement>> ElementsOf(
    IUIAutomationElementArray* array) {
    std::vector<winrt::com_ptr<IUIAutomationElement>> elements;
    int count = 0;
    if (array && SUCCEEDED(array->get_Length(&count))) {
        for (int i = 0; i < count; i++) {
            winrt::com_ptr<IUIAutomationElement> element;
            if (SUCCEEDED(array->GetElement(i, element.put())) && element) {
                elements.push_back(std::move(element));
            }
        }
    }
    return elements;
}

std::wstring ElementName(IUIAutomationElement* element) {
    std::wstring name;
    BSTR value = nullptr;
    if (SUCCEEDED(element->get_CurrentName(&value)) && value) {
        name = value;
    }
    SysFreeString(value);
    return name;
}

std::wstring ElementAutomationId(IUIAutomationElement* element) {
    std::wstring id;
    BSTR value = nullptr;
    if (SUCCEEDED(element->get_CurrentAutomationId(&value)) && value) {
        id = value;
    }
    SysFreeString(value);
    return id;
}

// The notifications in a stack: its "window" elements (one per banner).
// Windows numbers the ones after the first ("PriorityToastView3"), so
// they're told apart by their ID's start; one with another ID counts as not
// a call's, an alarm's or a reminder's.
winrt::com_ptr<IUIAutomationElementArray> FindBannerViews(
    IUIAutomation* uia,
    IUIAutomationElement* root) {
    winrt::com_ptr<IUIAutomationElementArray> found;
    VARIANT value;
    VariantInit(&value);
    value.vt = VT_I4;
    value.lVal = UIA_WindowControlTypeId;
    winrt::com_ptr<IUIAutomationCondition> condition;
    if (SUCCEEDED(uia->CreatePropertyCondition(UIA_ControlTypePropertyId,
                                               value, condition.put()))) {
        root->FindAll(TreeScope_Descendants, condition.get(), found.put());
    }
    return found;
}

// A title as both sides show it: one line, spaces collapsed, trimmed.
std::wstring BannerText(const std::wstring& text) {
    std::wstring result;
    for (WCHAR c : text) {
        const bool space = iswspace(c);
        if (space && (result.empty() || result.back() == L' ')) {
            continue;
        }
        result += space ? L' ' : c;
    }
    while (!result.empty() && result.back() == L' ') {
        result.pop_back();
    }
    return result;
}

struct WindowsBannerRequest {
    ~WindowsBannerRequest() {
        if (stop) {
            CloseHandle(stop);
        }
    }
    HWND banner;
    // The thread's own handle of the island's stop event.
    HANDLE stop;
    DWORD delayMs;
    // The notification's title, and its button to press (-1: its "x"), or
    // none (kReadOnly).
    std::wstring title;
    int press;
    WindowsBannerRead fallback;
};
constexpr int kReadOnly = -2;

// Reads the notifications in Windows' stack of banners; presses one's
// button first if asked (and reads again a moment later).
DWORD WINAPI WindowsBannerThreadProc(LPVOID parameter) {
    std::unique_ptr<WindowsBannerRequest> request(
        (WindowsBannerRequest*)parameter);
    if (request->delayMs &&
        WaitForSingleObject(request->stop, request->delayMs) == WAIT_OBJECT_0) {
        return 0;
    }
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    WindowsBannerRead read = std::move(request->fallback);
    {
        winrt::com_ptr<IUIAutomation> uia;
        if (FAILED(CoCreateInstance(kCLSID_CUIAutomation8, nullptr,
                                    CLSCTX_INPROC_SERVER, kIID_IUIAutomation,
                                    uia.put_void()))) {
            CoCreateInstance(kCLSID_CUIAutomation, nullptr,
                             CLSCTX_INPROC_SERVER, kIID_IUIAutomation,
                             uia.put_void());
        }
        // Not waiting long for a busy shell.
        winrt::com_ptr<IUIAutomation2> uia2;
        if (uia && SUCCEEDED(uia->QueryInterface(kIID_IUIAutomation2,
                                                 uia2.put_void()))) {
            uia2->put_ConnectionTimeout(1500);
            uia2->put_TransactionTimeout(1500);
        }
        auto readViews = [&](bool press) {
            read.views.clear();
            // What would be pressed, in each view titled like the pill's.
            std::vector<winrt::com_ptr<IUIAutomationElement>> targets;
            winrt::com_ptr<IUIAutomationElement> root;
            if (!uia || FAILED(uia->ElementFromHandle(request->banner,
                                                      root.put())) ||
                !root) {
                return;
            }
            {
                for (const auto& view :
                     ElementsOf(FindBannerViews(uia.get(), root.get()).get())) {
                    WindowsBannerView entry;
                    entry.priority =
                        ElementAutomationId(view.get())
                            .starts_with(L"PriorityToastView");
                    const auto titles = ElementsOf(
                        FindByAutomationId(uia.get(), view.get(), L"Title")
                            .get());
                    if (!titles.empty()) {
                        entry.title = BannerText(ElementName(titles[0].get()));
                    }
                    const auto buttons = ElementsOf(
                        FindByAutomationId(uia.get(), view.get(),
                                           L"VerbButton")
                            .get());
                    for (const auto& button : buttons) {
                        entry.buttons.push_back(ElementName(button.get()));
                    }
                    const auto dismiss = ElementsOf(
                        FindByAutomationId(uia.get(), view.get(),
                                           L"DismissButton")
                            .get());
                    entry.dismiss = !dismiss.empty();
                    // To press: the notification's own button, or its "x".
                    if (press && entry.priority &&
                        entry.title == BannerText(request->title)) {
                        targets.push_back(
                            request->press >= 0
                                ? (request->press < (int)buttons.size()
                                       ? buttons[request->press]
                                       : nullptr)
                                : (dismiss.empty() ? nullptr : dismiss[0]));
                    }
                    read.views.push_back(std::move(entry));
                }
            }
            // Pressed only when one notification matches: with two alike
            // ("Meeting" twice), Windows' own isn't guessed (the island's
            // button is pressed instead).
            winrt::com_ptr<IUIAutomationInvokePattern> invoke;
            if (targets.size() == 1 && targets[0] &&
                SUCCEEDED(targets[0]->GetCurrentPatternAs(
                    UIA_InvokePatternId, kIID_IUIAutomationInvokePattern,
                    invoke.put_void())) &&
                invoke && SUCCEEDED(invoke->Invoke())) {
                read.pressed = true;
            }
        };
        if (request->press != kReadOnly) {
            read.pressRequested = true;
            readViews(true);
            // Windows takes it away: read again once it's gone.
            if (read.pressed &&
                WaitForSingleObject(request->stop, 700) == WAIT_OBJECT_0) {
                uia2 = nullptr;
                uia = nullptr;
                if (SUCCEEDED(comResult)) {
                    CoUninitialize();
                }
                return 0;
            }
        }
        readViews(false);
    }
    if (SUCCEEDED(comResult)) {
        CoUninitialize();
    }
    AcquireSRWLockExclusive(&g_windowsBannerReadsLock);
    g_windowsBannerReads.push_back(std::move(read));
    ReleaseSRWLockExclusive(&g_windowsBannerReadsLock);
    if (HWND island = g_islandWnd) {
        PostMessage(island, WM_APP_BANNER_READ, 0, 0);
    }
    return 0;
}

// Whether Windows shows banners now: not with its "Do not disturb" (or a
// Focus rule), in full screen, or while presenting.
bool WindowsShowsBanners() {
    QUERY_USER_NOTIFICATION_STATE state;
    return !IsDoNotDisturbOn() &&
           SUCCEEDED(SHQueryUserNotificationState(&state)) &&
           state == QUNS_ACCEPTS_NOTIFICATIONS;
}

// Whether Windows shows an app's notifications as banners (its own
// notification settings, read only).
bool AppShowsBanners(const std::wstring& appId) {
    const std::wstring key =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\"
        L"Settings\\" +
        appId;
    for (PCWSTR value : {L"Enabled", L"ShowBanner"}) {
        DWORD data = 1;
        DWORD size = sizeof(data);
        if (RegGetValue(HKEY_CURRENT_USER, key.c_str(), value,
                        RRF_RT_REG_DWORD, nullptr, &data,
                        &size) == ERROR_SUCCESS &&
            data == 0) {
            return false;
        }
    }
    return true;
}

// Whether a Windows banner is on now (its window uncloaked): new ones wait
// their turn behind it.
bool WindowsBannerShowing() {
    int cloaked = 1;
    return g_windowsBanner && IsWindow(g_windowsBanner) &&
           SUCCEEDED(DwmGetWindowAttribute(g_windowsBanner, DWMWA_CLOAKED,
                                           &cloaked, sizeof(cloaked))) &&
           !cloaked;
}

// Whether the island takes new notifications from the Windows banner now
// (see ShowBanner): in the pill, or as its app's icon and the bell's dot (the
// bar, a panel open, typing a reply). Not with its banners off, minimized
// (or the Bar hidden) without them, when new notifications can't be read, or
// when Windows shows no banners (with its "Do not disturb", in full screen).
bool Island::CanTakeBanners() const {
    return g_settings.islandHideWindowsBanners && g_bannerEnabled &&
           g_toastWatchReady &&
           !((m_minimized || m_barHidden) && !g_bannerWhenMinimized) &&
           WindowsShowsBanners();
}

bool Island::ShowsNotifications() const {
    if (!ShowsNotices()) {
        return false;
    }
    return m_main ? g_notifyMain
                  : std::find(g_notifyScreens.begin(), g_notifyScreens.end(),
                              m_ownDevice) != g_notifyScreens.end();
}

void Island::CloseBannerElsewhere() {
    const IslandToast shown = m_banner;
    for (Island* other : g_islands) {
        if (other == this || !other->m_bannerWanted ||
            other->m_banner.appId != shown.appId ||
            (shown.id > 0 ? other->m_banner.id != shown.id
                          : other->m_banner.title != shown.title)) {
            continue;
        }
        IslandScope scope(other);
        other->EndBannerTyping();
        other->HideBanner();
    }
}

bool Island::AnyIslandTakesBanners() const {
    return std::any_of(g_islands.begin(), g_islands.end(),
                       [](const Island* island) {
                           return island->ShowsNotifications() &&
                                  island->CanTakeBanners();
                       });
}

// How long a notification the island read while no stack showed waits for
// the next stack (Windows shows its banner at once, or not at all).
constexpr double kBannerOwedSeconds = 6;

