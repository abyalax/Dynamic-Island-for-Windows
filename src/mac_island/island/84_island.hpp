// Watched (by the main island) while its item is on: the input method's
// windows showing and changing, the app in front (see OnForegroundChanged),
// Windows' own indicator on the taskbar, whose name changes with the mode,
// and, for what tells nothing else (Shift with some input methods, a
// keyboard switched in the same app), the keyboard and the mode looked at
// again every moment (see StartInputMethodQuery).
void Island::UpdateInputMethodWatch() {
    if (!m_hwnd || !m_main) {
        return;
    }
    if (!AnyIslandLook([](Island&) { return g_settings.islandInputMethod; })) {
        for (HWINEVENTHOOK& hook : m_inputMethodHooks) {
            if (hook) {
                UnhookWinEvent(hook);
                hook = nullptr;
            }
        }
        KillTimer(m_hwnd, kInputMethodTimerId);
        KillTimer(m_hwnd, kInputMethodPollTimerId);
        StopInputMethodThread();
        ForEachIsland(
            [](Island& island) { island.m_inputMethodText.clear(); });
        return;
    }
    if (!m_inputMethodHooks[0]) {
        m_inputMethodHooks[0] = SetWinEventHook(
            kEventObjectImeShow, kEventObjectImeChange, nullptr,
            InputMethodEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    }
    if (!m_inputMethodHooks[1]) {
        DWORD explorer = 0;
        if (HWND taskbar = FindWindow(L"Shell_TrayWnd", nullptr)) {
            GetWindowThreadProcessId(taskbar, &explorer);
        }
        if (explorer) {
            m_inputMethodHooks[1] = SetWinEventHook(
                EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, nullptr,
                InputMethodEventProc, explorer, 0, WINEVENT_OUTOFCONTEXT);
        }
    }
    CheckInputMethodSoon();
}

// Another keyboard is told right away; an input method's mode is asked on
// its thread, one question at a time (another asked meanwhile is asked once
// it answers). Both are looked at again every moment (see
// kInputMethodPollTimerId) while an input method is in front, or while
// there are keyboards to switch between (switched in the same app, newer
// Windows builds tell no event either, #6816).
void Island::StartInputMethodQuery() {
    const LANGID language = ForegroundLanguage(GetForegroundWindow());
    if (!HasInputModes(language)) {
        const int layouts = GetKeyboardLayoutList(0, nullptr);
        if (layouts > 1) {
            SetTimer(m_hwnd, kInputMethodPollTimerId, kInputMethodPollMs,
                     nullptr);
        } else {
            KillTimer(m_hwnd, kInputMethodPollTimerId);
        }
        // (An answer still coming from a thread is an old one now.)
        m_inputMethodId++;
        PostMessage(m_hwnd, WM_APP_INPUT_METHOD, MAKEWPARAM(language, 0),
                    layouts);
        return;
    }
    SetTimer(m_hwnd, kInputMethodPollTimerId, kInputMethodPollMs, nullptr);
    if (!m_inputMethodThread) {
        auto job = std::make_unique<InputMethodJob>();
        job->island = m_hwnd;
        job->ask = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        job->stop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (job->ask && job->stop) {
            m_inputMethodThread = CreateThread(
                nullptr, 0, InputMethodThreadProc, job.get(), 0, nullptr);
        }
        if (!m_inputMethodThread) {
            for (HANDLE event : {job->ask, job->stop}) {
                if (event) {
                    CloseHandle(event);
                }
            }
            return;
        }
        m_inputMethodJob = std::move(job);
    }
    m_inputMethodJob->id = ++m_inputMethodId;
    SetEvent(m_inputMethodJob->ask);
}

// Its thread ended (the item off, the island closing).
void Island::StopInputMethodThread() {
    if (!m_inputMethodThread) {
        return;
    }
    SetEvent(m_inputMethodJob->stop);
    WaitForSingleObject(m_inputMethodThread, INFINITE);
    CloseHandle(m_inputMethodThread);
    m_inputMethodThread = nullptr;
    CloseHandle(m_inputMethodJob->ask);
    CloseHandle(m_inputMethodJob->stop);
    m_inputMethodJob.reset();
}

// Its short text: the input method's letters ("中" or "英", "あ" or "A",
// "가" or "A"), or the keyboard's language ("PT") when there are others to
// switch to. Changed while the pill is minimized: said once in its bubble
// (over the line, by the island with its item), only when it changed in the
// same app (each window keeps its own: opening Start or going to another
// app isn't a change). A window in front that isn't an app's (an input
// method's question as the mode switches, like Microsoft Pinyin's) keeps
// the app behind it; Start and the search count as their own.
void Island::OnInputMethod(WPARAM state, LPARAM layouts) {
    const LANGID language = LOWORD(state);
    const WORD mode = HIWORD(state);
    std::wstring text;
    switch (PRIMARYLANGID(language)) {
        case LANG_CHINESE:
            text = mode == 1 ? L"\u4E2D" : L"\u82F1";
            break;
        case LANG_JAPANESE:
            text = mode == 1 ? L"\u3042" : L"A";
            break;
        case LANG_KOREAN:
            text = mode == 1 ? L"\uAC00" : L"A";
            break;
        default:
            if (layouts > 1) {
                WCHAR name[16] = L"";
                if (GetLocaleInfo(MAKELCID(language, SORT_DEFAULT),
                                  LOCALE_SISO639LANGNAME, name,
                                  ARRAYSIZE(name))) {
                    CharUpperBuff(name, (DWORD)wcslen(name));
                    text = name;
                }
            }
            break;
    }
    // Told only when the same window changed it (Shift, the indicator):
    // another window coming in front (an app opening or closing, the
    // desktop after the last one) has its own mode, not a change.
    HWND window = GetForegroundWindow();
    if (HWND owner = window ? GetAncestor(window, GA_ROOTOWNER) : nullptr) {
        window = owner;
    }
    const bool sameWindow = window && window == m_inputMethodWindow;
    m_inputMethodWindow = window;
    if (text == m_inputMethodText) {
        return;
    }
    const bool told = !m_inputMethodText.empty() && sameWindow;
    m_inputMethodText = text;
    // (Notches: once, from the middle one, wherever the item is, #6199; with
    // two, from the bell's.)
    const bool bubbleHere = m_notch < 0         ? true
                            : m_notchCount == 2 ? !m_notchBase
                                                : m_notch == 1;
    if (told && !text.empty() && m_main && m_minimized && !m_bar &&
        HasMainSettings() && bubbleHere) {
        WCHAR native[64] = L"";
        GetLocaleInfo(MAKELCID(language, SORT_DEFAULT),
                      LOCALE_SNATIVELANGUAGENAME, native, ARRAYSIZE(native));
        // ("中文(简体)" alone: it starts with its letter already.)
        ShowTextBubble(mode == 2 ? text + L"  " +
                                       Tr(L"English letters",
                                          L"Letras em inglês")
                       : std::wstring_view{native}.starts_with(text)
                           ? std::wstring(native)
                           : text + L"  " + native,
                       -1);
    }
    Relayout();
}

void Island::UpdatePrivacyWatch() {
    if (!m_hwnd || !m_main) {
        return;
    }
    const bool wanted =
        AnyIslandLook([](Island&) { return g_settings.islandPrivacy; });
    if (wanted == (g_privacyThread != nullptr)) {
        return;
    }
    if (!wanted) {
        // (The islands hear it's empty now.)
        StopPrivacyWatch();
        PostMessage(m_hwnd, WM_APP_PRIVACY, 0, 0);
        return;
    }
    g_privacyStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    g_privacyThread =
        g_privacyStop
            ? CreateThread(nullptr, 0, PrivacyThreadProc, m_hwnd, 0, nullptr)
            : nullptr;
    if (!g_privacyThread) {
        StopPrivacyWatch();
    }
}

void Island::OnPrivacy() {
    AcquireSRWLockShared(&g_privacyLock);
    PrivacyUse use = g_privacy;
    ReleaseSRWLockShared(&g_privacyLock);
    if (use == m_privacy) {
        return;
    }
    if (use.camera.empty()) {
        m_cameraSince = 0;
    } else if (m_privacy.camera.empty()) {
        m_cameraSince = NowSeconds();
        StartAnimating();
    }
    m_privacy = std::move(use);
    Relayout();
    // Its tip shown: with the apps now (or gone with the item).
    if (m_tooltipKey.starts_with(kItemTipPrefix) &&
        IsPrivacyItem(
            (IslandItem)_wtoi(m_tooltipKey.c_str() + wcslen(kItemTipPrefix)))) {
        const std::wstring key = m_tooltipKey;
        m_tooltipKey.clear();
        ShowTrayTip(key);
    }
}

std::wstring Island::ItemTip(IslandItem item, const std::wstring& text) const {
    if (IsPrivacyItem(item)) {
        return PrivacyText();
    }
    // What's playing: its whole title and its artist (the pill may cut
    // them), on more lines when long.
    if (item == IslandItem::Media) {
        std::wstring tip = MediaTitleText();
        if (!m_mediaShown.title.empty() && !m_mediaShown.artist.empty()) {
            tip += L"\n" + m_mediaShown.artist;
        }
        return tip;
    }
    if (item == IslandItem::Desktops) {
        return DesktopName(m_desktops, m_desktops.current);
    }
    if (item == IslandItem::Folders) {
        return Tr(L"Folders and places", L"Pastas e locais");
    }
    if (item == IslandItem::KomorebiLayout) {
        const KomorebiMonitor* monitor = KomorebiMonitorOf(m_monitorHandle);
        return monitor ? L"komorebi \u00B7 " +
                             KomorebiLayoutName(*monitor,
                                                g_komorebiShown.paused)
                       : L"komorebi";
    }
    if (item == IslandItem::FocusedApp) {
        return FocusedText();
    }
    if (item == IslandItem::Eyes) {
        return Tr(L"Dragon eyes (click to blink)",
                  L"Olhos de dragão (clique para piscar)");
    }
    if (item == IslandItem::Visualizer) {
        return Tr(L"Audio visualizer", L"Visualizador de áudio");
    }
    if (item == IslandItem::Lyrics) {
        return Tr(L"Song lyrics", L"Letra da música");
    }
    if (item == IslandItem::Timer) {
        return Tr(L"Timer, stopwatch and Pomodoro",
                  L"Timer, cronômetro e Pomodoro");
    }
    if (item == IslandItem::AppsFold) {
        return Tr(L"Apps with notifications", L"Apps com notificações");
    }
    // (The figures are on the item, changing every second.)
    if (item == IslandItem::NetSpeed) {
        return Tr(L"Network speed (up and down)",
                  L"Velocidade da rede (envio e recebimento)");
    }
    if (item == IslandItem::WorldClock) {
        if (m_worldClocks.empty()) {
            return Tr(L"Add clocks in Windows' date and time settings "
                      L"(Additional clocks)",
                      L"Adicione relógios nas configurações de data e hora "
                      L"do Windows (Relógios adicionais)");
        }
        std::wstring tip;
        for (const auto& clock : m_worldClocks) {
            tip += (tip.empty() ? L"" : L"\n") + clock.name + L"  " +
                   clock.time;
        }
        return tip;
    }
    if (item == IslandItem::Countdown) {
        return m_countdownTitle + L"\n" + m_countdownText;
    }
    if (item == IslandItem::Shelf) {
        // (A count after a label: no plural forms to get wrong.)
        return std::wstring(Tr(L"Shelf", L"Prateleira")) + L"\n" +
               Tr(L"Files", L"Arquivos") + L": " +
               std::to_wstring(Shelf().size());
    }
    if (item == IslandItem::Commands) {
        // A pinned one's name, or the panel's.
        const auto& commands = Commands();
        // (The panel's icon has its glyph as its key, not a number.)
        const size_t index = text.empty() || !iswdigit(text[0])
                                 ? commands.size()
                                 : _wtoi(text.c_str());
        return index < commands.size() ? commands[index].name
                                       : Tr(L"Your commands", L"Seus comandos");
    }
    if (item == IslandItem::AiUsage) {
        // Each agent's limits.
        const long long now = UnixNow();
        std::wstring tip = Tr(L"AI usage", L"Uso de IA");
        for (int agent = 0; agent < kAiAgents; agent++) {
            const AiUsage& usage = g_aiUsage[agent];
            if (!AiAgentOn(agent) || !AiUsageKnown(usage)) {
                continue;
            }
            tip += L"\n" + std::wstring(AiAgentName(agent));
            for (const AiLimit* limit : {&usage.session, &usage.week}) {
                const double used = AiUsedNow(*limit, now);
                if (used < 0) {
                    continue;
                }
                WCHAR percent[16];
                swprintf_s(percent, L"%.0f%%", used);
                tip += std::wstring(L" \u00B7 ") +
                       (limit == &usage.session ? L"5 h"
                                                : Tr(L"week", L"semana")) +
                       L" " + percent;
            }
        }
        return tip;
    }
    if (item == IslandItem::RecycleBin) {
        return std::wstring(Tr(L"Recycle Bin", L"Lixeira")) +
               (g_recycleItems >= 0
                    ? L"\n" + RecycleBinText(g_recycleItems,
                                              g_recycleKilobytes)
                    : std::wstring());
    }
    if (item == IslandItem::KomorebiWorkspace) {
        const KomorebiMonitor* monitor = KomorebiMonitorOf(m_monitorHandle);
        return monitor ? L"komorebi \u00B7 " +
                             KomorebiWorkspaceName(*monitor, _wtoi(text.c_str()))
                       : std::wstring();
    }
    // The others: their group's name (with the percentage for the volume
    // and the battery).
    PillGroup group = PillGroup::Count;
    switch (item) {
        case IslandItem::Overflow:
            group = PillGroup::Tray;
            break;
        case IslandItem::Network:
            group = PillGroup::Network;
            break;
        case IslandItem::Volume:
            group = PillGroup::Volume;
            break;
        case IslandItem::Battery:
            group = PillGroup::Battery;
            break;
        case IslandItem::Bell:
            group = PillGroup::Bell;
            break;
        case IslandItem::Power:
            group = PillGroup::Power;
            break;
        case IslandItem::ThemeSwitch:
            group = PillGroup::Theme;
            break;
        case IslandItem::Microphone:
            group = PillGroup::Microphone;
            break;
        case IslandItem::Notes:
            group = PillGroup::Notes;
            break;
        case IslandItem::Tasks:
            group = PillGroup::Tasks;
            break;
        case IslandItem::Shortcuts:
            group = PillGroup::Shortcuts;
            break;
        case IslandItem::Weather:
            group = PillGroup::Weather;
            break;
        case IslandItem::Updates:
            group = PillGroup::Updates;
            break;
        case IslandItem::Profiles:
            group = PillGroup::Profiles;
            break;
        case IslandItem::Brightness:
            group = PillGroup::Brightness;
            break;
        case IslandItem::InputMethod:
            group = PillGroup::InputMethod;
            break;
        case IslandItem::Drives:
            group = PillGroup::Drives;
            break;
        default:
            break;
    }
    if (group != PillGroup::Count) {
        const PillGroupInfo& info = kPillGroupInfo[(int)group];
        std::wstring name = Tr(info.english, info.portuguese);
        const std::wstring& percent =
            item == IslandItem::Volume    ? m_volumeText.text
            : item == IslandItem::Battery ? m_batteryText.text
                                          : std::wstring();
        if (!percent.empty()) {
            name += L" " + percent;
        }
        return name;
    }
    if (item == IslandItem::Wallpaper) {
        return Tr(L"Wallpaper (the wheel: the next one)",
                  L"Papel de parede (a roda: o próximo)");
    }
    const int action = item == IslandItem::QuickAction && !text.empty()
                           ? _wtoi(text.c_str())
                           : -1;
    if (action >= 0 && action < (int)ARRAYSIZE(kPillActions)) {
        const ControlInfo& info = GetControlInfo(kPillActions[action].id);
        return Tr(info.english, info.portuguese);
    }
    return {};
}

// Which apps use them, for the bubble.
std::wstring Island::PrivacyText() const {
    std::wstring text;
    auto add = [&](const std::wstring& apps, PCWSTR kind) {
        if (!apps.empty()) {
            text += (text.empty() ? L"" : L"  \u00B7  ") + std::wstring(kind) +
                    L": " + apps;
        }
    };
    add(m_privacy.microphone, Tr(L"Microphone", L"Microfone"));
    add(m_privacy.camera, Tr(L"Camera", L"Câmera"));
    add(m_privacy.location, Tr(L"Location", L"Localização"));
    return text;
}

void Island::OnDeviceChange(WPARAM event, LPARAM data) {
    const auto* header = (const DEV_BROADCAST_HDR*)data;
    if ((event != DBT_DEVICEARRIVAL && event != DBT_DEVICEREMOVECOMPLETE) ||
        !header || header->dbch_devicetype != DBT_DEVTYP_VOLUME) {
        return;
    }
    const auto* volume = (const DEV_BROADCAST_VOLUME*)header;
    // (Not network drives.)
    if ((volume->dbcv_flags & DBTF_NET) || !volume->dbcv_unitmask) {
        return;
    }
    ListDrives(event == DBT_DEVICEARRIVAL && g_settings.islandUsbNotify
                   ? volume->dbcv_unitmask
                   : 0);
}

// The desktops are read on a thread by the main island while their item is
// on (see DesktopsThreadProc).
void Island::UpdateDesktopsWatch() {
    if (!m_hwnd || !m_main) {
        return;
    }
    if (!AnyIslandLook([](Island&) { return g_settings.islandDesktops; })) {
        StopDesktopsWatch();
        if (!m_desktops.names.empty()) {
            ForEachIsland([](Island& island) {
                island.m_desktops = {};
                island.Relayout();
            });
        }
        return;
    }
    if (m_desktopsThread) {
        return;
    }
    m_desktopsStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_desktopsStop) {
        return;
    }
    // (Told again: a new thread starts from what's there.)
    AcquireSRWLockExclusive(&g_desktopsLock);
    g_desktops = {};
    ReleaseSRWLockExclusive(&g_desktopsLock);
    auto* job = new DesktopsJob{m_hwnd, m_desktopsStop};
    m_desktopsThread =
        CreateThread(nullptr, 0, DesktopsThreadProc, job, 0, nullptr);
    if (!m_desktopsThread) {
        delete job;
        CloseHandle(m_desktopsStop);
        m_desktopsStop = nullptr;
    }
}

// The main island watches Windows' additional clocks while an island shows
// them (a monitor with a profile of its own too: its change is passed on to
// the main one, like UpdateNetSpeedWatch).
void Island::UpdateWorldClocksWatch() {
    if (!m_hwnd) {
        return;
    }
    if (Island* main = MainIsland(); main != this) {
        if (main) {
            IslandScope scope(main);
            main->UpdateWorldClocksWatch();
        }
        return;
    }
    if (!AnyIslandLook([](Island&) { return g_settings.islandWorldClocks; })) {
        StopWorldClocksWatch();
        return;
    }
    if (m_clocksThread) {
        return;
    }
    m_clocksStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_clocksStop) {
        return;
    }
    auto* job = new WorldClocksJob{m_hwnd, m_clocksStop};
    m_clocksThread =
        CreateThread(nullptr, 0, WorldClocksThreadProc, job, 0, nullptr);
    if (!m_clocksThread) {
        delete job;
        CloseHandle(m_clocksStop);
        m_clocksStop = nullptr;
    }
}

void Island::StopWorldClocksWatch() {
    if (m_clocksThread) {
        SetEvent(m_clocksStop);
        WaitForSingleObject(m_clocksThread, INFINITE);
        CloseHandle(m_clocksThread);
        m_clocksThread = nullptr;
    }
    if (m_clocksStop) {
        CloseHandle(m_clocksStop);
        m_clocksStop = nullptr;
    }
}

void Island::StopDesktopsWatch() {
    if (m_desktopsThread) {
        SetEvent(m_desktopsStop);
        WaitForSingleObject(m_desktopsThread, INFINITE);
        CloseHandle(m_desktopsThread);
        m_desktopsThread = nullptr;
    }
    if (m_desktopsStop) {
        CloseHandle(m_desktopsStop);
        m_desktopsStop = nullptr;
    }
}

// komorebi's state is read by the main island while one of the islands
// shows its layout (each with its own look), for all of them.
void Island::UpdateKomorebiWatch() {
    if (!m_main) {
        if (Island* main = MainIsland(); main && main != this) {
            IslandScope scope(main);
            main->UpdateKomorebiWatch();
        }
        return;
    }
    if (!m_hwnd) {
        return;
    }
    if (!AnyIslandLook([](Island&) { return g_settings.komorebiLayout; })) {
        const bool shown = g_komorebiShown.connected;
        StopKomorebiWatch();
        if (shown) {
            ForEachIsland([](Island& island) {
                island.m_komorebiPanel.Close();
                island.Relayout();
            });
        }
        return;
    }
    if (m_komorebiWatch) {
        return;
    }
    // The thread gets its own copies of the events (it closes them).
    HANDLE stop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    HANDLE kick = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    HANDLE stopCopy = nullptr;
    HANDLE kickCopy = nullptr;
    const HANDLE process = GetCurrentProcess();
    if (!stop || !kick ||
        !DuplicateHandle(process, stop, process, &stopCopy, 0, FALSE,
                         DUPLICATE_SAME_ACCESS) ||
        !DuplicateHandle(process, kick, process, &kickCopy, 0, FALSE,
                         DUPLICATE_SAME_ACCESS)) {
        for (HANDLE handle : {stop, kick, stopCopy, kickCopy}) {
            if (handle) {
                CloseHandle(handle);
            }
        }
        return;
    }
    auto* job = new KomorebiWatchJob{m_hwnd, stopCopy, kickCopy};
    m_komorebiWatch =
        CreateThread(nullptr, 0, KomorebiWatchThreadProc, job, 0, nullptr);
    if (!m_komorebiWatch) {
        delete job;
        for (HANDLE handle : {stop, kick, stopCopy, kickCopy}) {
            CloseHandle(handle);
        }
        return;
    }
    m_komorebiWatchStop = stop;
    g_komorebiKick = kick;
}

// Its thread told to stop and waited for (every wait in it ends with the
// stop: a while at most); what it read is forgotten. Only the island that
// runs it.
void Island::StopKomorebiWatch() {
    if (!m_komorebiWatch && !m_komorebiWatchStop) {
        return;
    }
    if (m_komorebiWatchStop) {
        SetEvent(m_komorebiWatchStop);
    }
    if (m_komorebiWatch) {
        WaitForSingleObject(m_komorebiWatch, 5000);
        CloseHandle(m_komorebiWatch);
        m_komorebiWatch = nullptr;
    }
    if (m_komorebiWatchStop) {
        CloseHandle(m_komorebiWatchStop);
        m_komorebiWatchStop = nullptr;
    }
    if (g_komorebiKick) {
        CloseHandle(g_komorebiKick);
        g_komorebiKick = nullptr;
    }
    AcquireSRWLockExclusive(&g_komorebiLock);
    g_komorebiState = {};
    ReleaseSRWLockExclusive(&g_komorebiLock);
    g_komorebiShown = {};
}

// A name's width on the pill (measured again only when it or the size
// changes).
float Island::MeasureItemName(ItemWidth& width,
                              const std::wstring& name,
                              float height) {
    const std::wstring key = name + L"|" + std::to_wstring(m_textScale);
    if (key != width.measuredKey) {
        width.measuredKey = key;
        width.measured = 0;
        winrt::com_ptr<IDWriteTextLayout> layout;
        if (m_itemNameFormat &&
            SUCCEEDED(m_dwrite->CreateTextLayout(
                name.c_str(), (UINT32)name.size(), m_itemNameFormat.get(),
                10000, height, layout.put()))) {
            DWRITE_TEXT_METRICS metrics{};
            layout->GetMetrics(&metrics);
            width.measured = std::ceil(metrics.widthIncludingTrailingWhitespace);
        }
    }
    return width.measured;
}

// An app's name, as Windows' Start lists it, or as its program file
// describes it ("Google Chrome").
std::wstring FocusedAppName(const std::wstring& key) {
    return key.find(L'\\') != std::wstring::npos ? GetProgramName(key.c_str())
                                                 : AppDisplayName(key);
}

// The app in focus as the taskbar knows it: by its ID when Windows lists the
// app under it (Start, or a notification sender's registration); otherwise
// (like Steam's window: "Valve.Steam.Client", listed nowhere, in its helper
// steamwebhelper.exe) by the program the window says starts it again and the
// name it gives (`name`, empty if none), or by its process's file.
std::wstring FocusedAppKey(HWND window, std::wstring* name) {
    name->clear();
    std::wstring key = GetWindowAppId(window);
    if (!key.empty()) {
        winrt::com_ptr<IShellItem> item;
        bool registered = false;
        // (A packaged app's: its package's icon if not its own.)
        if (key.find(L'!') != std::wstring::npos ||
            SUCCEEDED(SHCreateItemInKnownFolder(kFOLDERID_AppsFolder, 0,
                                                key.c_str(),
                                                IID_PPV_ARGS(item.put()))) ||
            (RegisteredAppValue(key, L"DisplayName", &registered),
             registered)) {
            return key;
        }
    }
    // (Its relaunch program, see WindowRelaunchProgram.)
    if (const std::wstring program = WindowRelaunchProgram(window);
        !program.empty()) {
        *name = GetWindowAppText(window, 4);
        if (!name->empty() && (*name)[0] == L'@') {
            WCHAR resolved[256];
            *name = SUCCEEDED(SHLoadIndirectString(name->c_str(), resolved,
                                                   ARRAYSIZE(resolved),
                                                   nullptr))
                        ? resolved
                        : L"";
        }
        return program;
    }
    // (Its process's file, as for a window without an ID.)
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (HANDLE process =
            OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId)) {
        WCHAR path[MAX_PATH];
        DWORD length = ARRAYSIZE(path);
        if (QueryFullProcessImageName(process, 0, path, &length)) {
            key = path;
        }
        CloseHandle(process);
    }
    return key;
}

// The app window on top on a screen (not the island's own, hidden,
// minimized or on another desktop), for the app in focus before any focus
// change says so; none if there isn't one.
HWND FindTopAppWindow(HMONITOR screen) {
    if (!screen) {
        return nullptr;
    }
    HWND window = GetTopWindow(nullptr);
    for (int i = 0; window && i < 1000;
         i++, window = GetWindow(window, GW_HWNDNEXT)) {
        DWORD processId = 0;
        GetWindowThreadProcessId(window, &processId);
        if (processId != GetCurrentProcessId() && !IsIconic(window) &&
            IsAppWindow(window) && !IsCloakedWindow(window) &&
            !IsDesktopWindow(window) &&
            !(GetWindowLong(window, GWL_EXSTYLE) & WS_EX_TOPMOST) &&
            MonitorFromWindow(window, MONITOR_DEFAULTTONULL) == screen) {
            return window;
        }
    }
    return nullptr;
}

void Island::ClearFocusedApp() {
    if (!m_focusWindow && !m_focusDesktop) {
        return;
    }
    m_focusWindow = nullptr;
    m_focusDesktop = false;
    m_focusKey.clear();
    m_focusName.clear();
    m_focusTitle.clear();
    Relayout();
}

// The app in focus on this island's monitor (an option): the window in
// front when it's on this monitor (not Windows' menus, Start and flyouts),
// or the desktop when it's clicked here. Kept until it closes, is minimized,
// or goes to another monitor or desktop (see CheckFocusedApp).
void Island::NoteFocusedApp(HWND window) {
    if (!g_settings.islandFocusedApp || !window) {
        return;
    }
    if (IsDesktopWindow(window)) {
        // (One window for every monitor: the one clicked is the mouse's.)
        POINT cursor;
        if (m_focusDesktop || !GetCursorPos(&cursor) ||
            MonitorFromPoint(cursor, MONITOR_DEFAULTTONULL) !=
                m_monitorHandle) {
            return;
        }
        m_focusWindow = nullptr;
        m_focusDesktop = true;
        m_focusKey.clear();
        m_focusName = Tr(L"Desktop", L"Área de trabalho");
        m_focusTitle.clear();
        Relayout();
        return;
    }
    if (window == m_focusWindow || window == m_focusSkipped ||
        MonitorFromWindow(window, MONITOR_DEFAULTTONULL) != m_monitorHandle) {
        return;
    }
    // (Not the mod's own windows: a dialog of the island's, like its color
    // picker or a backup's file dialog.)
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    std::wstring key;
    std::wstring name;
    if (processId == GetCurrentProcessId() || IsShellFlyout(window) ||
        !IsAppWindow(window) || IsCloakedWindow(window) ||
        (key = FocusedAppKey(window, &name)).empty()) {
        m_focusSkipped = window;
        return;
    }
    m_focusWindow = window;
    m_focusSkipped = nullptr;
    if (key == m_focusKey && !m_focusDesktop) {
        // (Another window of the same app: its title.)
        if (ReadFocusTitle()) {
            Relayout();
        }
        return;
    }
    m_focusDesktop = false;
    m_focusKey = key;
    m_focusName = name.empty() ? FocusedAppName(key) : name;
    ReadFocusTitle();
    // (Its icon read now, not while drawing a frame.)
    if (m_canvas.target) {
        GetAppBitmap(m_focusKey);
    }
    Relayout();
}

// The window in focus's title (islandFocusedTitle), read with
// InternalGetWindowText (no message to a window that may hang), without the
// app's name it ends with ("Page - Google Chrome"); whether it changed.
bool Island::ReadFocusTitle() {
    std::wstring title;
    if (g_settings.islandFocusedTitle && m_focusWindow) {
        WCHAR text[256];
        const int length =
            InternalGetWindowText(m_focusWindow, text, ARRAYSIZE(text));
        title.assign(text, std::max(length, 0));
        for (PCWSTR between : {L" - ", L" — ", L" – "}) {
            const std::wstring end = between + m_focusName;
            if (!m_focusName.empty() && title.size() > end.size() &&
                title.ends_with(end)) {
                title.resize(title.size() - end.size());
                break;
            }
        }
        if (title == m_focusName) {
            title.clear();
        }
    }
    if (title == m_focusTitle) {
        return false;
    }
    m_focusTitle = std::move(title);
    return true;
}

// The app in focus as the pill shows it: its name, and its window's title
// when chosen.
std::wstring Island::FocusedText() const {
    return m_focusTitle.empty() ? m_focusName
                                : m_focusName + L" · " + m_focusTitle;
}

// Each second: the app shown gone (closed, minimized, hidden, on another
// monitor or desktop), or the window in front moved onto this monitor
// (no focus change says so).
void Island::CheckFocusedApp() {
    if (!g_settings.islandFocusedApp) {
        return;
    }
    if (m_focusWindow &&
        (!IsWindow(m_focusWindow) || !IsWindowVisible(m_focusWindow) ||
         IsIconic(m_focusWindow) || IsCloakedWindow(m_focusWindow) ||
         MonitorFromWindow(m_focusWindow, MONITOR_DEFAULTTONULL) !=
             m_monitorHandle)) {
        ClearFocusedApp();
    }
    HWND foreground = GetForegroundWindow();
    DWORD processId = 0;
    if (foreground && foreground != m_focusWindow &&
        !IsDesktopWindow(foreground) &&
        GetWindowThreadProcessId(foreground, &processId) &&
        processId != GetCurrentProcessId()) {
        NoteFocusedApp(foreground);
    }
    // (Its title changes without a focus change: another page or file.)
    if (g_settings.islandFocusedTitle && m_focusWindow && ReadFocusTitle()) {
        Relayout();
    }
}

void ChooseWallpaper(Island* island,
                     const std::vector<std::wstring>& paths,
                     int index) {
    if (island) {
        island->ChangeWallpaper(paths, index);
    }
}

void OpenTimerTabOf(Island* island, int tab) {
    if (island) {
        IslandScope scope(island);
        island->OpenTimerTab(tab);
    }
}

// A picture of the folder comes in as this monitor's wallpaper, with the
// transition chosen, out of the pill (one at a time).
void Island::ChangeWallpaper(const std::vector<std::wstring>& paths,
                             int index) {
    if (index < 0 || index >= (int)paths.size() || m_wallpaperThread) {
        return;
    }
    m_wallpapers = paths;
    m_wallpaperIndex = index;
    POINT origin{m_position.x + (LONG)((m_pill.left + m_pill.right) / 2),
                 m_position.y + (LONG)((m_pill.top + m_pill.bottom) / 2)};
    for (const auto& slot : m_slots) {
        if (slot.item == IslandItem::Wallpaper) {
            origin = {
                m_position.x + (LONG)((slot.rect.left + slot.rect.right) / 2),
                m_position.y + (LONG)((slot.rect.top + slot.rect.bottom) / 2)};
        }
    }
    auto* change = new WallpaperChange{
        m_hwnd, paths[index], m_monitor,
        (WallpaperTransition)g_settings.wallpaperTransition, origin};
    m_wallpaperThread =
        CreateThread(nullptr, 0, WallpaperChangeThreadProc, change, 0, nullptr);
    if (!m_wallpaperThread) {
        delete change;
    }
}

// What's playing's app was asked to come back by its tray icon: looked for
// a moment, then started by its ID if its window didn't come.
void Island::WatchMediaAppOpen(const std::wstring& appId) {
    m_mediaOpenApp = appId;
    m_mediaOpenAt = NowSeconds();
    SetTimer(m_hwnd, kMediaOpenTimerId, kMediaOpenCheckMs, nullptr);
}

// The wheel on the item: the next picture (the folder read again the first
// time, and when it ran out).
void Island::StepWallpaper(int step) {
    // (Listed on a thread: the step is taken when the list comes, see
    // WM_APP_WALLPAPER_LIST.)
    if (m_wallpapers.empty()) {
        if (m_wallpaperListThread) {
            return;
        }
        const std::wstring folder =
            GetWholeStringValue(L"wallpaperFolder", MAX_PATH);
        auto* job = new WallpaperListJob{m_hwnd, folder, step, {}};
        m_wallpaperListThread =
            CreateThread(nullptr, 0, WallpaperListThreadProc, job, 0, nullptr);
        if (!m_wallpaperListThread) {
            delete job;
        }
        return;
    }
    const int count = (int)m_wallpapers.size();
    const int index = m_wallpaperIndex < 0
                          ? (step > 0 ? 0 : count - 1)
                          : ((m_wallpaperIndex + step) % count + count) % count;
    ChangeWallpaper(m_wallpapers, index);
}

bool Island::PillActionLit(ControlId id) const {
    switch (id) {
        case ControlId::NightLight:
            return m_nightLightState == 1;
        case ControlId::Bluetooth:
            return m_control.RadioState(1) == 1;
        case ControlId::Hotspot:
            return m_control.RadioState(2) == 1;
        case ControlId::Airplane:
            return m_control.RadioState(3) == 1;
        default:
            return false;
    }
}

// What a control center button does on the pill: switched right there
// (night light, the hotspot, airplane mode; lit while on), done (the
// shortcuts), or its own list opened under it, alone (Wi-Fi, Bluetooth,
// projecting, accessibility; a second click closes it).
void Island::RunPillAction(int action) {
    const ControlId id = kPillActions[action].id;
    // (Its panel opens under this button, not the first one.)
    const std::wstring key = std::to_wstring(action);
    switch (id) {
        case ControlId::NightLight: {
            const int now = GetNightLight();
            if (now >= 0 && SetNightLight(now != 1)) {
                m_nightLightState = now != 1;
            } else {
                ShowTextBubble(Tr(L"Night light isn't available here",
                                  L"Luz noturna não está disponível aqui"),
                               SlotMiddle(IslandItem::QuickAction, key));
            }
            m_control.Refresh();
            StartAnimating();
            break;
        }
        case ControlId::Hotspot:
        case ControlId::Airplane:
            if (!m_control.ToggleRadio(id == ControlId::Hotspot ? 2 : 3)) {
                // Not known yet (or not on this PC): asked again.
                m_control.ReadRadios();
                ShowTextBubble(Tr(L"Not available on this PC",
                                  L"Não disponível neste PC"),
                               SlotMiddle(IslandItem::QuickAction, key));
            }
            StartAnimating();
            break;
        case ControlId::Screenshot:
            PressKeys({VK_LWIN, VK_SHIFT, 'S'});
            break;
        case ControlId::Komorebi:
            // Its own small panel: the control center with only its
            // buttons (clicked again: closed; open for another item: it
            // goes under this one).
            HideGearBubble();
            if (!m_control.SetOnly(ControlPanel::Only::Komorebi) ||
                !m_control.IsOpen()) {
                TogglePanel(m_control, IslandItem::QuickAction, key);
            } else {
                MovePanel(m_control, IslandItem::QuickAction, key);
            }
            break;
        case ControlId::Network:
        case ControlId::Bluetooth:
        case ControlId::Project:
        case ControlId::Accessibility: {
            HideGearBubble();
            if (m_control.IsOpen() && m_control.ShowingControl(id)) {
                TogglePanel(m_control, IslandItem::QuickAction, key);
                break;
            }
            m_control.SetOnly(ControlPanel::Only::View);
            m_control.ShowControl(id);
            if (!m_control.IsOpen()) {
                TogglePanel(m_control, IslandItem::QuickAction, key);
            } else {
                MovePanel(m_control, IslandItem::QuickAction, key);
            }
            break;
        }
        default:
            RunControlShortcut(id);
            break;
    }
}

// An app window's color, for the island to take (never read from the
// screen): its title bar's, when the app gives it one (Windows 11), or its
// icon's main color (the average of its colorful pixels); -1 for gray ones.
int WindowColor(HWND hWnd) {
    COLORREF caption = 0;
    // (DWMWA_CAPTION_COLOR; 0xFFFFFFFF and 0xFFFFFFFE are Windows' own.)
    if (SUCCEEDED(DwmGetWindowAttribute(hWnd, 35, &caption,
                                        sizeof(caption))) &&
        caption < 0xFFFFFFFE) {
        IslandLog(L"WindowColor: DWM caption = %06X", caption);
        return (GetRValue(caption) << 16) | (GetGValue(caption) << 8) |
               GetBValue(caption);
    }
    IslandLog(L"WindowColor: falling back to window icon");
    DWORD_PTR result = 0;
    HICON icon = nullptr;
    if (SendMessageTimeout(hWnd, WM_GETICON, ICON_BIG, 0,
                           SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &result)) {
        icon = (HICON)result;
    }
    if (!icon) {
        icon = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
    }
    ICONINFO info{};
    if (!icon || !GetIconInfo(icon, &info)) {
        return -1;
    }
    int color = -1;
    BITMAP bitmap{};
    if (info.hbmColor &&
        GetObject(info.hbmColor, sizeof(bitmap), &bitmap) &&
        bitmap.bmWidth > 0 && bitmap.bmWidth <= 256 && bitmap.bmHeight > 0 &&
        bitmap.bmHeight <= 256) {
        BITMAPINFO header{};
        header.bmiHeader.biSize = sizeof(header.bmiHeader);
        header.bmiHeader.biWidth = bitmap.bmWidth;
        header.bmiHeader.biHeight = -bitmap.bmHeight;
        header.bmiHeader.biPlanes = 1;
        header.bmiHeader.biBitCount = 32;
        header.bmiHeader.biCompression = BI_RGB;
        std::vector<DWORD> pixels((size_t)bitmap.bmWidth * bitmap.bmHeight);
        HDC dc = GetDC(nullptr);
        if (GetDIBits(dc, info.hbmColor, 0, bitmap.bmHeight, pixels.data(),
                      &header, DIB_RGB_COLORS)) {
            // Weighted by how colorful each pixel is; only clearly
            // colorful icons give a color.
            double sum[3] = {}, weight = 0;
            for (DWORD pixel : pixels) {
                const int a = pixel >> 24;
                const int r = (pixel >> 16) & 0xFF;
                const int g = (pixel >> 8) & 0xFF;
                const int b = pixel & 0xFF;
                const int top = std::max({r, g, b});
                const int bottom = std::min({r, g, b});
                const double saturation =
                    top ? (top - bottom) / (double)top : 0;
                if (a < 128 || saturation < 0.3 || top < 60) {
                    continue;
                }
                sum[0] += r * saturation;
                sum[1] += g * saturation;
                sum[2] += b * saturation;
                weight += saturation;
            }
            if (weight > pixels.size() * 0.05) {
                color = ((int)(sum[0] / weight) << 16) |
                        ((int)(sum[1] / weight) << 8) | (int)(sum[2] / weight);
            }
        }
        ReleaseDC(nullptr, dc);
    }
    if (info.hbmColor) {
        DeleteObject(info.hbmColor);
    }
    if (info.hbmMask) {
        DeleteObject(info.hbmMask);
    }
    return color;
}

// Whether USB drives are watched: for their icon, or to tell when one is
// plugged in (by any island, each with its own look).
bool UsbDrivesWatched() {
    return AnyIslandLook([](Island&) {
        return g_settings.islandUsbDrives || g_settings.islandUsbNotify;
    });
}

