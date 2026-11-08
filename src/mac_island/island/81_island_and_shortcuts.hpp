// Muted or not, as the default one is: every microphone plugged in (an app
// may record from another one than the default, like a webcam's while a
// headset is the default). Some apps record a muted microphone anyway (they
// take its sound before Windows mutes it): muted, its level goes to 0 too,
// while the island runs, and back to what it was when it's unmuted or the
// island stops (still muted then). One muted on purpose before is
// left alone (and stays muted).
void Island::ToggleMicrophone() {
    const int muted = MicrophoneMuted();
    if (muted < 0 || !m_devices) {
        return;
    }
    if (muted) {
        RestoreMicrophoneLevels(m_devices.get(), true, true);
        // (The default ones too, if they were muted elsewhere.)
        for (const auto& microphone : GetMicrophones()) {
            microphone->SetMute(FALSE, nullptr);
        }
        OnVolumeChanged();
        return;
    }
    winrt::com_ptr<IMMDeviceCollection> devices;
    UINT count = 0;
    if (FAILED(m_devices->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE,
                                             devices.put())) ||
        FAILED(devices->GetCount(&count))) {
        return;
    }
    auto& levels = MicrophoneLevels();
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
        BOOL wasMuted = FALSE;
        if (FAILED(volume->GetMute(&wasMuted)) || wasMuted) {
            continue;
        }
        // Its level (never a 0 over one kept from before).
        float level = 0;
        volume->GetMasterVolumeLevelScalar(&level);
        auto it = std::find_if(levels.begin(), levels.end(),
                               [&](const auto& entry) {
                                   return entry.first == deviceId;
                               });
        const int kept = level >= 0.0005f ? (int)std::lround(level * 1000)
                         : it != levels.end() ? it->second
                                              : -1;
        if (it == levels.end()) {
            if (levels.size() >= 32) {
                levels.erase(levels.begin());
            }
            levels.push_back({deviceId, kept});
        } else {
            it->second = kept;
        }
        volume->SetMute(TRUE, nullptr);
        volume->SetMasterVolumeLevelScalar(0, nullptr);
    }
    SaveMicrophoneLevels();
    OnVolumeChanged();
}

// From the microphone already watched (the default one), not looked up
// again on every change.
std::wstring Island::MicrophoneGlyph() {
    winrt::com_ptr<IAudioEndpointVolume> microphone = m_watchedMicrophone;
    // (A notch without the bell watches none: the default one, read now.)
    if (!microphone && !m_statusEvents) {
        const auto microphones = GetMicrophones();
        if (!microphones.empty()) {
            microphone = microphones.front();
        }
    }
    BOOL muted = FALSE;
    if (!microphone || FAILED(microphone->GetMute(&muted))) {
        return {};
    }
    return {muted ? (WCHAR)0xF781 : (WCHAR)0xE720};
}

void Island::WatchMicrophone() {
    if (!m_statusEvents) {
        return;
    }
    if (m_watchedMicrophone) {
        m_watchedMicrophone->UnregisterControlChangeNotify(m_statusEvents);
        m_watchedMicrophone = nullptr;
    }
    const auto microphones = GetMicrophones();
    if (!microphones.empty() &&
        SUCCEEDED(
            microphones[0]->RegisterControlChangeNotify(m_statusEvents))) {
        m_watchedMicrophone = microphones[0];
    }
}

NetworkInfo Island::QueryNetwork() {
    NetworkInfo info;
    NLM_CONNECTIVITY connectivity;
    if (m_network && SUCCEEDED(m_network->GetConnectivity(&connectivity))) {
        info.internet = connectivity & (NLM_CONNECTIVITY_IPV4_INTERNET |
                                        NLM_CONNECTIVITY_IPV6_INTERNET);
        info.connected = connectivity != NLM_CONNECTIVITY_DISCONNECTED;
    }
    // The physical adapters (cable, Wi-Fi, mobile broadband): with some and
    // none of them connected, nothing is (whatever virtual adapters say).
    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) == NO_ERROR && table) {
        bool any = false;
        bool up = false;
        for (ULONG i = 0; i < table->NumEntries; i++) {
            const MIB_IF_ROW2& row = table->Table[i];
            if (!row.InterfaceAndOperStatusFlags.HardwareInterface ||
                row.InterfaceAndOperStatusFlags.FilterInterface ||
                (row.Type != IF_TYPE_ETHERNET_CSMACD &&
                 row.Type != IF_TYPE_IEEE80211 && row.Type != 243 &&
                 row.Type != 244)) {
                continue;
            }
            any = true;
            if (row.OperStatus == IfOperStatusUp &&
                row.MediaConnectState == MediaConnectStateConnected) {
                up = true;
                if (row.Type == IF_TYPE_IEEE80211) {
                    info.wifiAdapterUp = true;
                } else if (row.Type == IF_TYPE_ETHERNET_CSMACD) {
                    info.cableUp = true;
                }
            }
        }
        FreeMibTable(table);
        info.physical = up || !any;
        if (!info.physical && !info.internet) {
            info.connected = false;
        }
    }

    // A connected Wi-Fi network shows its name and signal.
    PWLAN_INTERFACE_INFO_LIST interfaces = nullptr;
    if (m_wlan && WlanEnumInterfaces(m_wlan, nullptr, &interfaces) ==
                      ERROR_SUCCESS) {
        for (DWORD i = 0; i < interfaces->dwNumberOfItems && !info.wifi; i++) {
            const auto& adapter = interfaces->InterfaceInfo[i];
            if (adapter.isState == wlan_interface_state_associating ||
                adapter.isState == wlan_interface_state_discovering ||
                adapter.isState == wlan_interface_state_authenticating) {
                info.wifiConnecting = true;
            }
            if (adapter.isState != wlan_interface_state_connected) {
                continue;
            }
            info.wifi = true;
            DWORD size = 0;
            PWLAN_CONNECTION_ATTRIBUTES attributes = nullptr;
            const DWORD queried =
                WlanQueryInterface(m_wlan, &adapter.InterfaceGuid,
                                   wlan_intf_opcode_current_connection, nullptr,
                                   &size, (PVOID*)&attributes, nullptr);
            // (Once per answer: this runs on every network change.)
            static DWORD loggedQuery = ERROR_SUCCESS;
            if (queried != loggedQuery) {
                loggedQuery = queried;
                if (queried != ERROR_SUCCESS) {
                    IslandLog(L"The Wi-Fi network couldn't be read: 0x%08X",
                           queried);
                }
            }
            if (queried == ERROR_SUCCESS) {
                info.signal =
                    attributes->wlanAssociationAttributes.wlanSignalQuality;
                const auto& ssid = attributes->wlanAssociationAttributes.dot11Ssid;
                WCHAR name[64] = L"";
                MultiByteToWideChar(CP_UTF8, 0, (LPCCH)ssid.ucSSID,
                                    (int)std::min<ULONG>(ssid.uSSIDLength, 32),
                                    name, ARRAYSIZE(name) - 1);
                info.wifiName = name;
                WlanFreeMemory(attributes);
            }
        }
        WlanFreeMemory(interfaces);
    }
    // The WLAN API can say no Wi-Fi is connected while its adapter is (no
    // cable plugged in, #6361): the adapter table decides then (no name,
    // full bars).
    const bool wifiByTable = !info.wifi && info.wifiAdapterUp && !info.cableUp;
    static bool loggedTable = false;
    if (wifiByTable != loggedTable) {
        loggedTable = wifiByTable;
        if (wifiByTable) {
            IslandLog(L"Wi-Fi taken from the adapter table (WLAN: %s)",
                   m_wlan ? L"not connected" : L"not open");
        }
    }
    info.wifi = info.wifi || wifiByTable;
    return info;
}

// The network's state (see NetState), and its glyph.
std::wstring Island::NetworkGlyph() {
    const NetworkInfo info = QueryNetwork();
    const double now = NowSeconds();
    const NetState before = m_netState;
    // Connected without internet right after being off (or connecting):
    // Windows is still checking for it.
    if (!info.connected || info.internet) {
        m_netLocalSince = 0;
    } else if (!m_netLocalSince && (before == NetState::Off ||
                                    before == NetState::Connecting)) {
        m_netLocalSince = now;
    }
    // Connected again from nothing: "connecting" shows a moment at least,
    // even when the internet is there at once (not at the start).
    if (info.internet && before == NetState::Off && !m_networkGlyph.empty()) {
        m_netShowConnectingUntil = now + 1.4;
    }
    NetState state;
    if (info.internet && now >= m_netShowConnectingUntil) {
        state = NetState::Online;
    } else if (info.internet) {
        state = NetState::Connecting;
    } else if (info.wifiConnecting ||
               (m_netLocalSince &&
                now - m_netLocalSince < kNetworkCheckSeconds)) {
        state = NetState::Connecting;
    } else {
        state = info.connected ? NetState::NoInternet : NetState::Off;
    }
    if (state == NetState::Connecting) {
        if (!m_netConnectingSince) {
            m_netConnectingSince = now;
        } else if (now - m_netConnectingSince > kNetworkConnectingMax) {
            state = info.connected ? NetState::NoInternet : NetState::Off;
        }
    } else {
        m_netConnectingSince = 0;
    }
    m_netWifi = info.wifi || (info.wifiConnecting && !info.connected);
    m_netBars = info.signal >= 0 ? std::clamp(info.signal / 25, 0, 3) : 3;
    m_netState = state;
    switch (state) {
        case NetState::Online:
            return {m_netWifi ? kGlyphWifi[m_netBars] : kGlyphEthernet};
        case NetState::Connecting:
            // (Drawn moving, see DrawNetwork.)
            return {m_netWifi ? kGlyphWifi[3] : kGlyphEthernet, L'~'};
        case NetState::NoInternet:
            return {m_netWifi ? kGlyphWifiWarning[m_netBars]
                              : kGlyphEthernetWarning};
        default:
            return {kGlyphNoInternet};
    }
}

// The network's state now: its glyph (changing smoothly), checked again
// every second while connecting, and what waited for the internet once it
// comes.
void Island::UpdateNetwork() {
    const NetState before = m_netState;
    SetNetworkGlyph(NetworkGlyph());
    if (m_hwnd) {
        if (m_netState == NetState::Connecting) {
            SetTimer(m_hwnd, kNetworkTimerId, 1000, nullptr);
            SetTimer(m_hwnd, kNetworkFrameTimerId, kNetworkFrameMs, nullptr);
        } else {
            KillTimer(m_hwnd, kNetworkTimerId);
            KillTimer(m_hwnd, kNetworkFrameTimerId);
        }
    }
    if (m_netState == NetState::Online && before != NetState::Online) {
        OnInternetBack();
    }
}

void Island::SetNetworkGlyph(std::wstring glyph) {
    if (glyph == m_networkGlyph) {
        return;
    }
    // The one before fades out as the new one grows in (not at the start).
    if (!m_networkGlyph.empty()) {
        m_networkFrom = m_networkGlyph;
        m_networkSwap = 1;
        m_networkSwapVelocity = 0;
        StartAnimating();
    }
    m_networkGlyph = std::move(glyph);
}

// The internet came (after the island started, or back after it went):
// the weather, if it's old or failed.
void Island::OnInternetBack() {
    if (!m_hwnd || !m_main || !AnyWeatherWanted()) {
        return;
    }
    const WeatherData weather = GetWeather();
    if (!weather.valid || m_weatherRetries > 0 ||
        NowSeconds() - weather.fetchedAt > 10 * 60) {
        KillTimer(m_hwnd, kWeatherRetryTimerId);
        StartWeather({});
    }
}

// The network's glyph on the pill (Wi-Fi's button): connecting moves
// (Wi-Fi's bars filling one after another over a faint full one, or the
// cable pulsing), and a change fades the glyph before out while the new one
// grows in.
void Island::DrawNetwork(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush,
                         const IslandSlot& slot,
                         float opacity) {
    IDWriteTextFormat* format = m_iconFormat.get();
    if (!format) {
        return;
    }
    const D2D1_POINT_2F center{(slot.rect.left + slot.rect.right) / 2,
                               (slot.rect.top + slot.rect.bottom) / 2};
    const float full = std::round(kIslandIconSlot * IconScale());
    const D2D1_RECT_F rect{center.x - full / 2, slot.rect.top,
                           center.x + full / 2, slot.rect.bottom};
    auto drawGlyph = [&](const std::wstring& text, float alpha, float size) {
        if (text.empty() || alpha <= 0.004f) {
            return;
        }
        const float scale = std::max(size, 0.0f);
        target->SetTransform(D2D1::Matrix3x2F::Scale(scale, scale, center));
        if (text.size() > 1 && text[1] == L'~') {
            const double now = NowSeconds();
            if (text[0] == kGlyphEthernet) {
                // The cable, pulsing.
                const float pulse =
                    0.35f + 0.65f * (0.5f + 0.5f * (float)std::cos(now * 4.2));
                brush->SetColor(FgIcon(0.95f * alpha * pulse));
                DrawGlyph(target, &kGlyphEthernet, 1, format, rect, brush);
            } else {
                // Wi-Fi: a faint full one, its bars filling one after another.
                brush->SetColor(FgIcon(0.25f * alpha));
                DrawGlyph(target, &kGlyphWifi[3], 1, format, rect, brush);
                const int bar = (int)(now / 0.32) % 4;
                brush->SetColor(FgIcon(0.95f * alpha));
                DrawGlyph(target, &kGlyphWifi[bar], 1, format, rect, brush);
            }
        } else {
            brush->SetColor(FgIcon(0.95f * alpha));
            DrawGlyph(target, text.c_str(), 1, format, rect, brush);
        }
        target->SetTransform(D2D1::Matrix3x2F::Identity());
    };
    const float shown = std::max(slot.scale, 0.0f);
    const float swap = (float)std::clamp(m_networkSwap, 0.0, 1.0);
    const float alpha = opacity * std::clamp(slot.scale, 0.0f, 1.0f);
    // The one before fades and shrinks a little as the new one grows in.
    const float grow = 1 - swap;
    if (swap > 0.001f) {
        drawGlyph(m_networkFrom, alpha * swap * swap,
                  shown * (0.8f + 0.2f * swap));
    }
    drawGlyph(m_networkGlyph, alpha * (1 - swap * swap),
              shown * (0.8f + 0.2f * grow));
}

std::wstring Island::VolumeGlyph() {
    auto volume = GetVolume(nullptr);
    float level = 0;
    BOOL muted = FALSE;
    if (!volume || FAILED(volume->GetMasterVolumeLevelScalar(&level)) ||
        FAILED(volume->GetMute(&muted))) {
        return {};
    }
    if (muted) {
        return {kGlyphMute};
    }
    if (level <= 0.001f) {
        return {kGlyphVolume[0]};
    }
    return {kGlyphVolume[std::clamp((int)(level * 3) + 1, 1, 3)]};
}

void Island::OnDoNotDisturbChanged() {
    UpdateStatus();
    Render();
}

// Win+N and Win+A for the island (options): Windows' own shortcuts, so
// only a low-level keyboard hook can take them first; the minimize
// shortcut's keys too (any app may use them). It's on only while one of
// them is chosen or new keys are being recorded, on a thread of its own
// that does nothing else (it only looks at the keys and posts to the
// island). The key's press and release are kept from the app in front, and
// with Win or Alt a key nothing uses (0xFF) goes in between, so the Windows
// key (or Alt) let go alone doesn't open Start (or the menu bar), as
// PowerToys does.
constexpr UINT WM_APP_SHORTCUT = WM_APP + 83;
// The ones chosen (bit 0 Win+N, bit 1 Win+A, bit 2 the minimize shortcut,
// see g_minimizeShortcut; bit 3 Caps Lock told, see islandCapsLock), read by
// the hook.
std::atomic<int> g_shortcutKeys;
HANDLE g_shortcutThread;
DWORD g_shortcutThreadId;

// The keys taken by the hook (Win+N's, Win+A's, the minimize shortcut's,
// and one recorded), until let go (the hook's thread only).
DWORD g_shortcutTaken[4];

// Win, Ctrl, Alt and Shift as they're held now (kShortcutWin...): every key
// before this one counted, nothing remembered here, so a Win let go where
// the hook doesn't see it, like on the lock screen, can't stay "held".
int HeldShortcutModifiers() {
    auto held = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
    return (held(VK_LWIN) || held(VK_RWIN) ? kShortcutWin : 0) |
           (held(VK_CONTROL) ? kShortcutCtrl : 0) |
           (held(VK_MENU) ? kShortcutAlt : 0) |
           (held(VK_SHIFT) ? kShortcutShift : 0);
}

void PressUnusedKey() {
    INPUT dummy[2]{};
    dummy[0].type = dummy[1].type = INPUT_KEYBOARD;
    dummy[0].ki.wVk = dummy[1].ki.wVk = 0xFF;
    dummy[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, dummy, sizeof(INPUT));
}

LRESULT CALLBACK ShortcutKeyProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code != HC_ACTION) {
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }
    const auto* key = (const KBDLLHOOKSTRUCT*)lParam;
    if (key->flags & LLKHF_INJECTED) {
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }
    const bool down = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
    // Caps Lock let go: the island says how it is now (the key goes on).
    if (!down && key->vkCode == VK_CAPITAL && (g_shortcutKeys.load() & 8)) {
        if (HWND island = g_islandWnd) {
            PostMessage(island, WM_APP_SHORTCUT, 3, 0);
        }
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }
    // A key taken: its repeats and its release too, until it's let go (the
    // app in front never sees it held).
    for (DWORD& taken : g_shortcutTaken) {
        if (taken && taken == key->vkCode) {
            if (!down) {
                taken = 0;
            }
            return 1;
        }
    }
    if (!down || IsModifierKey(key->vkCode)) {
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }
    const int keys = g_shortcutKeys.load();
    const int minimize = g_minimizeShortcut.load();
    // (The modifiers read only for a key that may be a shortcut's: every key
    // typed comes here.)
    if (!g_shortcutRecording && !((keys & 1) && key->vkCode == 'N') &&
        !((keys & 2) && key->vkCode == 'A') &&
        !((keys & 4) && key->vkCode == (DWORD)(minimize & 0xFF))) {
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }
    const int held = HeldShortcutModifiers();
    // Recording: Escape alone cancels; any other key goes to the settings
    // (recorded with Win, Ctrl or Alt; without, they say it needs one and
    // wait for another). Another window in front (the settings left): it
    // stops, and the keys go there as usual.
    if (g_shortcutRecording) {
        const HWND recorder = g_shortcutRecorder;
        if (!recorder || GetAncestor(GetForegroundWindow(), GA_ROOT) !=
                             GetAncestor(recorder, GA_ROOT)) {
            g_shortcutRecording = false;
            if (recorder) {
                PostMessage(recorder, WM_APP_SHORTCUT_RECORDED, 0, -1);
            }
            return CallNextHookEx(nullptr, code, wParam, lParam);
        }
        const bool escape = key->vkCode == VK_ESCAPE && !held;
        if (escape || (held & (kShortcutWin | kShortcutCtrl | kShortcutAlt))) {
            g_shortcutRecording = false;
        }
        g_shortcutTaken[3] = key->vkCode;
        if (held & (kShortcutWin | kShortcutAlt)) {
            PressUnusedKey();
        }
        PostMessage(recorder, WM_APP_SHORTCUT_RECORDED, 0,
                    escape ? -1 : held << 8 | (int)key->vkCode);
        return 1;
    }
    const int which =
        (keys & 1) && held == kShortcutWin && key->vkCode == 'N'   ? 0
        : (keys & 2) && held == kShortcutWin && key->vkCode == 'A' ? 1
        : (keys & 4) && held == minimize >> 8 &&
                key->vkCode == (DWORD)(minimize & 0xFF)
            ? 2
            : -1;
    if (which < 0) {
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }
    g_shortcutTaken[which] = key->vkCode;
    if (held & (kShortcutWin | kShortcutAlt)) {
        PressUnusedKey();
    }
    if (HWND island = g_islandWnd) {
        PostMessage(island, WM_APP_SHORTCUT, which, 0);
    }
    return 1;
}

DWORD WINAPI ShortcutThreadProc(LPVOID parameter) {
    std::fill(std::begin(g_shortcutTaken), std::end(g_shortcutTaken), 0);
    // (Its queue made before it says it's ready, for its WM_QUIT.)
    MSG msg;
    PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);
    SetEvent((HANDLE)parameter);
    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, ShortcutKeyProc,
                                  g_module, 0);
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
    }
    if (hook) {
        UnhookWindowsHookEx(hook);
    }
    return 0;
}

void StopShortcutKeys() {
    if (!g_shortcutThread) {
        return;
    }
    PostThreadMessage(g_shortcutThreadId, WM_QUIT, 0, 0);
    WaitForSingleObject(g_shortcutThread, INFINITE);
    CloseHandle(g_shortcutThread);
    g_shortcutThread = nullptr;
}

// The ones chosen (the main island's).
void Island::UpdateShortcutKeys() {
    if (!m_main) {
        return;
    }
    g_shortcutKeys = (g_settings.islandWinN ? 1 : 0) |
                     (g_settings.islandWinA ? 2 : 0) |
                     (g_settings.islandMinimizeKey ? 4 : 0) |
                     (g_settings.islandCapsLock ? 8 : 0);
    UpdateShortcutHook();
}

std::wstring ShortcutText(int combo, bool spaced) {
    static constexpr std::pair<int, PCWSTR> kModifiers[] = {
        {kShortcutWin, L"Win"},
        {kShortcutCtrl, L"Ctrl"},
        {kShortcutAlt, L"Alt"},
        {kShortcutShift, L"Shift"},
    };
    std::wstring text;
    for (const auto& [bit, name] : kModifiers) {
        if ((combo >> 8) & bit) {
            text += std::wstring(name) + (spaced ? L" + " : L"+");
        }
    }
    return text + KeyName(combo & 0xFF);
}

// Whether Windows or another app uses the keys too: registered by it
// (RegisterHotKey refuses them), or one of Windows' own Win+key shortcuts
// that it doesn't register.
bool ShortcutTaken(int combo) {
    const int modifiers = combo >> 8;
    const UINT vk = combo & 0xFF;
    // (Any id of the thread's own; let go at once.)
    constexpr int kProbeHotKey = 0x4D49;
    const UINT flags = MOD_NOREPEAT |
                       (modifiers & kShortcutWin ? MOD_WIN : 0) |
                       (modifiers & kShortcutCtrl ? MOD_CONTROL : 0) |
                       (modifiers & kShortcutAlt ? MOD_ALT : 0) |
                       (modifiers & kShortcutShift ? MOD_SHIFT : 0);
    if (!RegisterHotKey(nullptr, kProbeHotKey, flags, vk)) {
        return GetLastError() == ERROR_HOTKEY_ALREADY_REGISTERED;
    }
    UnregisterHotKey(nullptr, kProbeHotKey);
    if (modifiers != kShortcutWin) {
        return false;
    }
    if ((vk >= 'A' && vk <= 'Z' && wcschr(L"ABCDEFGHIKLMNPQRSTUVWXZ", (WCHAR)vk)) ||
        (vk >= '0' && vk <= '9')) {
        return true;
    }
    switch (vk) {
        case VK_OEM_PERIOD:
        case VK_OEM_COMMA:
        case VK_OEM_PLUS:
        case VK_OEM_MINUS:
        case VK_TAB:
        case VK_SPACE:
        case VK_HOME:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_PAUSE:
        case VK_SNAPSHOT:
            return true;
        default:
            return false;
    }
}

// New keys for the minimize shortcut, kept (and taken by the hook from then
// on) unless Windows keeps them (Win+L) or the island's Win+N or Win+A has
// them; what to tell about them (empty: nothing).
std::wstring SetMinimizeShortcut(int combo) {
    const int modifiers = combo >> 8;
    const UINT vk = combo & 0xFF;
    if (modifiers == kShortcutWin && vk == 'L') {
        return Tr(L"Win+L stays Windows' (it locks the PC)",
                  L"Win+L fica com o Windows (ele bloqueia o PC)");
    }
    if (modifiers == kShortcutWin && ((vk == 'N' && g_settings.islandWinN) ||
                                      (vk == 'A' && g_settings.islandWinA))) {
        return Tr(L"taken by the island's Win+N or Win+A",
                  L"já usado pelo Win+N ou Win+A da ilha");
    }
    g_minimizeShortcut = combo;
    Wh_SetIntValue(L"islandMinimizeShortcut", combo);
    return combo != kDefaultMinimizeShortcut && ShortcutTaken(combo)
               ? Tr(L"Windows or another app uses them too, the island takes "
                    L"them first",
                    L"o Windows ou outro app também usa, a ilha pega antes")
               : L"";
}

void UpdateShortcutHook() {
    if (!g_shortcutKeys && !g_shortcutRecording) {
        StopShortcutKeys();
        return;
    }
    if (g_shortcutThread) {
        return;
    }
    HANDLE ready = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!ready) {
        return;
    }
    g_shortcutThread = CreateThread(nullptr, 0, ShortcutThreadProc, ready, 0,
                                    &g_shortcutThreadId);
    if (g_shortcutThread) {
        WaitForSingleObject(ready, INFINITE);
    }
    CloseHandle(ready);
}

