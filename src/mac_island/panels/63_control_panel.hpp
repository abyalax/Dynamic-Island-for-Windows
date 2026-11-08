void ControlPanel::UpdateWifi(bool scan) {
    if (!m_wlan || !m_hasWifi || !m_hwnd) {
        m_networks.clear();
        return;
    }
    // One at a time; the list shown stays until the new one comes.
    if (m_wifiListThread) {
        if (WaitForSingleObject(m_wifiListThread, 0) != WAIT_OBJECT_0) {
            m_wifiListAgain = true;
            m_wifiScanAgain = m_wifiScanAgain || scan;
            return;
        }
        CloseHandle(m_wifiListThread);
        m_wifiListThread = nullptr;
    }
    auto* job =
        new WifiListJob{m_hwnd, m_wlan, m_wifiInterface, scan, ++m_wifiListId};
    m_wifiListThread =
        CreateThread(nullptr, 0, WifiListThreadProc, job, 0, nullptr);
    if (!m_wifiListThread) {
        delete job;
    }
}

// The networks in reach, one per name: the connected one first, then by
// signal. Refused by Windows (see kWifiListRefused): access denied, or
// only networks with their names hidden (what it does for apps without the
// location on newer Windows 11 builds).
DWORD WINAPI ControlPanel::WifiListThreadProc(LPVOID parameter) {
    std::unique_ptr<WifiListJob> job((WifiListJob*)parameter);
    if (job->scan) {
        WlanScan(job->wlan, &job->wifiInterface, nullptr, nullptr, nullptr);
    }
    PWLAN_AVAILABLE_NETWORK_LIST list = nullptr;
    if (const DWORD error = WlanGetAvailableNetworkList(
            job->wlan, &job->wifiInterface, 0, nullptr, &list);
        error != ERROR_SUCCESS) {
        IslandLog(L"The Wi-Fi networks couldn't be listed: 0x%08X", error);
        // (Told anyway: the panel waits for this thread's answer.)
        PostMessage(job->notify, WM_APP_WIFI_LIST, job->id,
                    error == ERROR_ACCESS_DENIED ? kWifiListRefused : 0);
        return 0;
    }
    bool unnamed = false;
    auto* found = new std::vector<WifiNetwork>;
    std::vector<WifiNetwork>& networks = *found;
    for (DWORD i = 0; i < list->dwNumberOfItems; i++) {
        const WLAN_AVAILABLE_NETWORK& available = list->Network[i];
        // Hidden networks have no name to show.
        if (available.dot11Ssid.uSSIDLength == 0 ||
            available.dot11BssType != dot11_BSS_type_infrastructure) {
            unnamed = unnamed || available.dot11Ssid.uSSIDLength == 0;
            continue;
        }
        WifiNetwork network;
        network.ssid = available.dot11Ssid;
        network.name = SsidName(available.dot11Ssid);
        network.signal = (int)available.wlanSignalQuality;
        network.secure = available.bSecurityEnabled;
        network.connected =
            (available.dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED) != 0;
        if (available.dwFlags & WLAN_AVAILABLE_NETWORK_HAS_PROFILE) {
            network.profile = available.strProfileName;
        }
        network.auth = available.dot11DefaultAuthAlgorithm;
        network.cipher = available.dot11DefaultCipherAlgorithm;

        auto same = std::find_if(
            networks.begin(), networks.end(), [&](const WifiNetwork& n) {
                return n.ssid.uSSIDLength == network.ssid.uSSIDLength &&
                       memcmp(n.ssid.ucSSID, network.ssid.ucSSID,
                              network.ssid.uSSIDLength) == 0;
            });
        if (same == networks.end()) {
            networks.push_back(std::move(network));
            continue;
        }
        same->signal = std::max(same->signal, network.signal);
        same->connected = same->connected || network.connected;
        if (same->profile.empty()) {
            same->profile = network.profile;
        }
    }
    WlanFreeMemory(list);
    std::stable_sort(networks.begin(), networks.end(),
                     [](const WifiNetwork& a, const WifiNetwork& b) {
                         if (a.connected != b.connected) {
                             return a.connected;
                         }
                         return a.signal > b.signal;
                     });
    if (networks.size() > kMaxNetworks) {
        networks.resize(kMaxNetworks);
    }
    if (networks.empty() && unnamed) {
        delete found;
        PostMessage(job->notify, WM_APP_WIFI_LIST, job->id, kWifiListRefused);
        return 0;
    }
    if (!PostMessage(job->notify, WM_APP_WIFI_LIST, job->id, (LPARAM)found)) {
        delete found;
    }
    return 0;
}

// "Forget" on a saved network, clicked twice: its saved profile (with its
// password) is deleted, like Windows' "Forget".
void ControlPanel::ForgetWifi(int index) {
    if (index < 0 || index >= (int)m_networks.size() || !m_wlan ||
        !m_hasWifi) {
        return;
    }
    const std::wstring profile = m_networks[index].profile;
    if (profile.empty()) {
        return;
    }
    if (m_forgetProfile != profile) {
        m_forgetProfile = profile;
        return;
    }
    m_forgetProfile.clear();
    const DWORD result = WlanDeleteProfile(m_wlan, &m_wifiInterface,
                                           profile.c_str(), nullptr);
    if (result != ERROR_SUCCESS) {
        IslandLog(L"WlanDeleteProfile failed: %u", result);
        m_wifiStatus = Tr(L"Couldn't forget the network.",
                          L"Não foi possível esquecer a rede.");
        return;
    }
    m_networks[index].profile.clear();
    UpdateWifi();
}

// The paired devices, the connected ones first. Only what Windows already
// knows: no search for new devices.
void ControlPanel::UpdateBluetooth() {
    m_bluetoothDevices.clear();
    if (m_bluetoothRadio != 1) {
        return;
    }
    BLUETOOTH_DEVICE_SEARCH_PARAMS params{sizeof(params)};
    params.fReturnAuthenticated = TRUE;
    params.fReturnRemembered = TRUE;
    params.fReturnConnected = TRUE;
    BLUETOOTH_DEVICE_INFO info{sizeof(info)};
    HBLUETOOTH_DEVICE_FIND find = BluetoothFindFirstDevice(&params, &info);
    if (!find) {
        return;
    }
    do {
        WCHAR glyph = kGlyphBluetooth;
        const ULONG major = GET_COD_MAJOR(info.ulClassofDevice);
        if (major == COD_MAJOR_AUDIO) {
            glyph = kGlyphHeadphones;
        } else if (major == COD_MAJOR_PERIPHERAL) {
            glyph = (GET_COD_MINOR(info.ulClassofDevice) & 0x20) ? kGlyphMouse
                                                                 : kGlyphKeyboard;
        }
        m_bluetoothDevices.push_back({info.szName, info.fConnected != FALSE,
                                      glyph, info.Address.ullLong,
                                      major == COD_MAJOR_AUDIO});
        info.dwSize = sizeof(info);
    } while (BluetoothFindNextDevice(find, &info));
    BluetoothFindDeviceClose(find);
    std::stable_sort(m_bluetoothDevices.begin(), m_bluetoothDevices.end(),
                     [](const BluetoothDevice& a, const BluetoothDevice& b) {
                         return a.connected && !b.connected;
                     });
    if (m_bluetoothDevices.size() > kMaxBluetoothDevices) {
        m_bluetoothDevices.resize(kMaxBluetoothDevices);
    }
}

// A device's row clicked: headphones or speakers connect (or disconnect),
// showing it on the row; other devices open Windows' Bluetooth settings.
void ControlPanel::ClickBluetoothDevice(int index) {
    if (index < 0 || index >= (int)m_bluetoothDevices.size()) {
        return;
    }
    const BluetoothDevice& device = m_bluetoothDevices[index];
    if (!device.audio) {
        OpenUri(L"ms-settings:bluetooth");
        return;
    }
    // (One at a time.)
    const BluetoothStep step =
        LiveBluetoothStep(m_bluetoothStep, m_bluetoothStepAt);
    if (step == BluetoothStep::Connecting ||
        step == BluetoothStep::Disconnecting) {
        return;
    }
    m_bluetoothAddress = device.address;
    m_bluetoothStep = device.connected ? BluetoothStep::Disconnecting
                                       : BluetoothStep::Connecting;
    m_bluetoothStepAt = GetTickCount64();
    auto* request = new BluetoothConnectRequest{m_hwnd, device.address,
                                                !device.connected};
    if (!StartWorker(BluetoothConnectProc, request)) {
        delete request;
        m_bluetoothStep = BluetoothStep::Failed;
    }
    SetTimer(m_hwnd, kBluetoothTimerId, kBluetoothCheckMs, nullptr);
}

// The device asked to connect or disconnect, looked at again: done when it
// is (or not after a while: "Couldn't connect" for a moment).
void ControlPanel::CheckBluetoothStep() {
    UpdateBluetooth();
    const ULONGLONG since = GetTickCount64() - m_bluetoothStepAt;
    auto it = std::find_if(m_bluetoothDevices.begin(),
                           m_bluetoothDevices.end(),
                           [&](const BluetoothDevice& device) {
                               return device.address == m_bluetoothAddress;
                           });
    const bool connected = it != m_bluetoothDevices.end() && it->connected;
    switch (m_bluetoothStep) {
        case BluetoothStep::Connecting:
            if (connected) {
                m_bluetoothStep = BluetoothStep::None;
            } else if (since > kBluetoothConnectMs) {
                m_bluetoothStep = BluetoothStep::Failed;
                m_bluetoothStepAt = GetTickCount64();
            }
            break;
        case BluetoothStep::Disconnecting:
            if (!connected || since > kBluetoothConnectMs) {
                m_bluetoothStep = BluetoothStep::None;
            }
            break;
        case BluetoothStep::Failed:
            if (since > kBluetoothFailedMs) {
                m_bluetoothStep = BluetoothStep::None;
            }
            break;
        default:
            break;
    }
    if (m_bluetoothStep == BluetoothStep::None) {
        KillTimer(m_hwnd, kBluetoothTimerId);
    }
}

// Joins a network: with its saved profile, or with a new one (open, or with
// the password `key`). WPA/WPA2/WPA3 personal only; the others need the
// Windows network list.
bool ControlPanel::ConnectWifi(const WifiNetwork& network,
                               const std::wstring* key) {
    if (!m_wlan || !m_hasWifi) {
        return false;
    }
    std::wstring profile = network.profile;
    if (profile.empty() || key) {
        PCWSTR authentication;
        switch (network.auth) {
            case DOT11_AUTH_ALGO_80211_OPEN:
                authentication = L"open";
                break;
            case DOT11_AUTH_ALGO_WPA_PSK:
                authentication = L"WPAPSK";
                break;
            case DOT11_AUTH_ALGO_RSNA_PSK:
                authentication = L"WPA2PSK";
                break;
            case DOT11_AUTH_ALGO_WPA3_SAE:
                authentication = L"WPA3SAE";
                break;
            default:
                return false;
        }
        const bool open = network.auth == DOT11_AUTH_ALGO_80211_OPEN;
        if (!open && !key) {
            return false;
        }
        PCWSTR encryption = open ? L"none"
                            : network.cipher == DOT11_CIPHER_ALGO_TKIP
                                ? L"TKIP"
                                : L"AES";
        std::wstring hex;
        for (ULONG i = 0; i < network.ssid.uSSIDLength && i < 32; i++) {
            WCHAR byte[3];
            swprintf_s(byte, L"%02X", network.ssid.ucSSID[i]);
            hex += byte;
        }
        const std::wstring name = EscapeXml(network.name);
        std::wstring body =
            L"<SSIDConfig><SSID><hex>" + hex + L"</hex><name>" + name +
            L"</name></SSID></SSIDConfig>"
            L"<connectionType>ESS</connectionType>"
            L"<connectionMode>auto</connectionMode>"
            L"<MSM><security><authEncryption><authentication>" +
            authentication + L"</authentication><encryption>" + encryption +
            L"</encryption><useOneX>false</useOneX></authEncryption>";
        if (!open) {
            body += L"<sharedKey><keyType>passPhrase</keyType>"
                    L"<protected>false</protected><keyMaterial>" +
                    EscapeXml(*key) + L"</keyMaterial></sharedKey>";
        }
        body += L"</security></MSM></WLANProfile>";
        // A saved network's own profile takes the new password; a new one is
        // saved under the network's name, or "Name (2)" when a profile
        // already has that name (it isn't replaced, nor deleted if joining
        // fails).
        const bool update = !network.profile.empty();
        // Updated as the kind it is (all users', or only this user's: then
        // an all users' one of that name can't replace it).
        DWORD flags = 0;
        if (update) {
            LPWSTR saved = nullptr;
            if (WlanGetProfile(m_wlan, &m_wifiInterface,
                               network.profile.c_str(), nullptr, &saved,
                               &flags, nullptr) == ERROR_SUCCESS) {
                WlanFreeMemory(saved);
            }
            flags &= WLAN_PROFILE_USER;
        }
        DWORD result = ERROR_ALREADY_EXISTS;
        for (int copy = 1; copy <= 9 && result == ERROR_ALREADY_EXISTS;
             copy++) {
            profile = update      ? network.profile
                      : copy == 1 ? network.name
                                  : network.name + L" (" +
                                        std::to_wstring(copy) + L")";
            const std::wstring xml =
                L"<?xml version=\"1.0\"?>"
                L"<WLANProfile "
                L"xmlns=\"http://www.microsoft.com/networking/WLAN/profile/"
                L"v1\"><name>" +
                EscapeXml(profile) + L"</name>" + body;
            DWORD reason = 0;
            result = WlanSetProfile(m_wlan, &m_wifiInterface, flags,
                                    xml.c_str(), nullptr, update, nullptr,
                                    &reason);
            if (result != ERROR_SUCCESS &&
                (update || result != ERROR_ALREADY_EXISTS)) {
                IslandLog(L"WlanSetProfile failed: %u, reason %u", result,
                       reason);
            }
            if (update) {
                break;
            }
        }
        if (result != ERROR_SUCCESS) {
            return false;
        }
        if (!update) {
            m_createdProfiles.push_back(profile);
        }
    }
    WLAN_CONNECTION_PARAMETERS parameters{};
    parameters.wlanConnectionMode = wlan_connection_mode_profile;
    parameters.strProfile = profile.c_str();
    parameters.dot11BssType = dot11_BSS_type_infrastructure;
    const DWORD result =
        WlanConnect(m_wlan, &m_wifiInterface, &parameters, nullptr);
    if (result != ERROR_SUCCESS) {
        IslandLog(L"WlanConnect failed: %u", result);
        // (A profile made for it goes: nothing joins with it.)
        if (auto it = std::find(m_createdProfiles.begin(),
                                m_createdProfiles.end(), profile);
            it != m_createdProfiles.end()) {
            WlanDeleteProfile(m_wlan, &m_wifiInterface, profile.c_str(),
                              nullptr);
            m_createdProfiles.erase(it);
        }
        return false;
    }
    m_connecting = true;
    m_connectingProfile = profile;
    m_connectingSince = GetTickCount64();
    // (Given up on time even if nothing draws the panel meanwhile.)
    SetTimer(m_hwnd, kConnectingTimerId, (UINT)kConnectingMs + 100, nullptr);
    m_wifiStatus = Tr(L"Connecting...", L"Conectando...");
    return true;
}

// A join Windows never answered: the panel stops waiting for it (its end,
// if it comes later, still removes a profile made for it that failed).
void ControlPanel::CheckConnecting() {
    if (m_connecting && GetTickCount64() - m_connectingSince > kConnectingMs) {
        m_connecting = false;
        m_connectingProfile.clear();
        m_wifiStatus =
            Tr(L"Couldn't connect.", L"Não foi possível conectar.");
    }
}

// A network clicked in the list: joins it, asking for its password if needed.
void ControlPanel::JoinWifi(int index) {
    if (index < 0 || index >= (int)m_networks.size()) {
        return;
    }
    const WifiNetwork network = m_networks[index];
    if (network.connected) {
        return;
    }
    if (!network.profile.empty() || !network.secure) {
        if (!ConnectWifi(network, nullptr)) {
            m_wifiStatus = Tr(L"Couldn't connect.",
                              L"Não foi possível conectar.");
        }
        return;
    }
    switch (network.auth) {
        case DOT11_AUTH_ALGO_WPA_PSK:
        case DOT11_AUTH_ALGO_RSNA_PSK:
        case DOT11_AUTH_ALGO_WPA3_SAE:
            m_joining = network;
            m_password.clear();
            m_passwordEdit.Reset();
            m_showPassword = false;
            m_wifiStatus.clear();
            SetView(View::Password);
            break;
        default:
            // Company networks (a user name and a certificate): the Windows
            // network list asks for them.
            Close();
            OpenUri(L"ms-availablenetworks:");
            break;
    }
}

void ControlPanel::SubmitPassword() {
    CheckConnecting();
    if (m_connecting) {
        return;
    }
    if (m_password.size() < 8) {
        m_wifiStatus = Tr(L"The password has at least 8 characters.",
                          L"A senha tem pelo menos 8 caracteres.");
        return;
    }
    if (!ConnectWifi(m_joining, &m_password)) {
        m_wifiStatus =
            Tr(L"Couldn't connect.", L"Não foi possível conectar.");
    }
}

// kind: 0 Wi-Fi, 1 Bluetooth, -1 only to know their state.
void ControlPanel::SwitchRadio(int kind, bool on) {
    // Forgets the threads that are done.
    std::erase_if(m_workers, [](HANDLE thread) {
        if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0) {
            CloseHandle(thread);
            return true;
        }
        return false;
    });
    if (!m_hwnd) {
        return;
    }
    if (m_radioBusy && GetTickCount64() - m_radioBusySince < 15000) {
        if (kind >= 0) {
            m_radioWanted[kind] = on;
        }
        return;
    }
    auto* request = new RadioRequest{m_hwnd, kind, on};
    if (HANDLE thread =
            CreateThread(nullptr, 0, RadioThreadProc, request, 0, nullptr)) {
        m_workers.push_back(thread);
        m_radioBusy = true;
        m_radioBusySince = GetTickCount64();
    } else {
        delete request;
    }
}

void ControlPanel::OpenControlView(ControlId id) {
    switch (id) {
        case ControlId::Network:
            SetView(View::Wifi);
            break;
        case ControlId::Bluetooth:
            SetView(View::Bluetooth);
            break;
        case ControlId::Project:
            UpdateDisplayMode();
            SetView(View::Project);
            break;
        case ControlId::Accessibility:
            UpdateAccessibility();
            SetView(View::Accessibility);
            break;
        default:
            SetView(View::Main);
            break;
    }
}

void ControlPanel::SwitchNightLight() {
    if (m_nightLight < 0) {
        return;
    }
    m_nightLight = m_nightLight == 1 ? 0 : 1;
    if (m_nightLightChecking) {
        return;
    }
    SetNightLight(m_nightLight == 1);
    if (!m_hwnd) {
        return;
    }
    m_nightLightChecking = true;
    m_nightLightCheckSince = GetTickCount64();
    m_nightLightTries = 0;
    SetTimer(m_hwnd, kNightLightTimerId, kNightLightCheckMs, nullptr);
}

bool ControlPanel::OnTimer(UINT_PTR id) {
    if (id == kConnectingTimerId) {
        CheckConnecting();
        Layout();
        return true;
    }
    if (id == kBluetoothTimerId) {
        CheckBluetoothStep();
        Layout();
        return true;
    }
    if (id != kNightLightTimerId || !m_nightLightChecking) {
        return false;
    }
    const int now = GetNightLight();
    if (now >= 0 && now != m_nightLight && ++m_nightLightTries < 4) {
        SetNightLight(m_nightLight == 1);
        SetTimer(m_hwnd, kNightLightTimerId, kNightLightCheckMs, nullptr);
        return false;
    }
    m_nightLightChecking = false;
    if (now >= 0) {
        m_nightLight = now;
    }
    Layout();
    return true;
}

// (False when the thread couldn't start: the caller still owns `parameter`.)
bool ControlPanel::StartWorker(LPTHREAD_START_ROUTINE proc, void* parameter) {
    std::erase_if(m_workers, [](HANDLE thread) {
        if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0) {
            CloseHandle(thread);
            return true;
        }
        return false;
    });
    HANDLE thread = CreateThread(nullptr, 0, proc, parameter, 0, nullptr);
    if (!thread) {
        return false;
    }
    m_workers.push_back(thread);
    return true;
}

bool ControlPanel::StartBrightnessWorker(LPTHREAD_START_ROUTINE proc) {
    m_brightnessJob->notify = m_hwnd;
    auto* held = new std::shared_ptr<BrightnessJob>(m_brightnessJob);
    if (!StartWorker(proc, held)) {
        delete held;
        return false;
    }
    return true;
}

// The display mode, like Windows+P: the PC's screen only, duplicated,
// extended, or the second screen only.
void ControlPanel::UpdateDisplayMode() {
    UINT32 paths = 0;
    UINT32 modes = 0;
    m_displayMode = 0;
    if (GetDisplayConfigBufferSizes(QDC_DATABASE_CURRENT, &paths, &modes) !=
        ERROR_SUCCESS) {
        return;
    }
    std::vector<DISPLAYCONFIG_PATH_INFO> pathInfo(paths);
    std::vector<DISPLAYCONFIG_MODE_INFO> modeInfo(modes);
    DISPLAYCONFIG_TOPOLOGY_ID topology{};
    if (QueryDisplayConfig(QDC_DATABASE_CURRENT, &paths, pathInfo.data(),
                           &modes, modeInfo.data(),
                           &topology) == ERROR_SUCCESS) {
        m_displayMode = (int)topology;
    }
}

// The accessibility features of the Windows quick settings: the magnifier,
// the narrator and the on-screen keyboard (started from a button), color
// filters (only when their shortcut is on, which is how they're switched)
// and sticky keys (switches).
// Windows' Magnifier started itself (its Win+Plus shortcut can be turned off
// in Windows' settings; see StartWithUiAccess), or with that shortcut: also
// when it isn't running a moment later (Explorer started, the magnifier not;
// see CheckMagnifierStarted).
double g_magnifierCheckAt = 0;
constexpr double kMagnifierCheckSeconds = 3;

void StartMagnifier() {
    if (!StartWithUiAccess(L"Magnify.exe")) {
        PressKeys({VK_LWIN, VK_ADD});
        return;
    }
    g_magnifierCheckAt = NowSeconds() + kMagnifierCheckSeconds;
}

// (Each second, see Island::Tick.)
void CheckMagnifierStarted() {
    if (!g_magnifierCheckAt || NowSeconds() < g_magnifierCheckAt) {
        return;
    }
    g_magnifierCheckAt = 0;
    if (!SessionProcessId(L"Magnify.exe")) {
        PressKeys({VK_LWIN, VK_ADD});
    }
}

void ControlPanel::UpdateAccessibility() {
    m_accessibility.clear();
    m_accessibility.push_back({Feature::Magnifier, kGlyphMagnifier,
                               Tr(L"Magnifier", L"Lupa"), false, true});
    m_accessibility.push_back({Feature::Narrator, kGlyphNarrator,
                               Tr(L"Narrator", L"Narrador"), false, true});
    m_accessibility.push_back({Feature::OnScreenKeyboard, kGlyphKeyboard,
                               Tr(L"On-screen keyboard", L"Teclado virtual"),
                               false, true});
    DWORD hotkey = 0;
    DWORD active = 0;
    DWORD size = sizeof(hotkey);
    RegGetValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\ColorFiltering",
                L"HotkeyEnabled", RRF_RT_REG_DWORD, nullptr, &hotkey, &size);
    size = sizeof(active);
    RegGetValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\ColorFiltering",
                L"Active", RRF_RT_REG_DWORD, nullptr, &active, &size);
    if (hotkey) {
        m_accessibility.push_back({Feature::ColorFilters, kGlyphColorFilters,
                                   Tr(L"Color filters", L"Filtros de cor"),
                                   active != 0});
    }
    STICKYKEYS sticky{sizeof(sticky)};
    SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(sticky), &sticky, 0);
    m_accessibility.push_back({Feature::StickyKeys, kGlyphKeyboard,
                               Tr(L"Sticky keys", L"Teclas de aderência"),
                               (sticky.dwFlags & SKF_STICKYKEYSON) != 0});
}

void ControlPanel::ToggleAccessibility(int index) {
    if (index < 0 || index >= (int)m_accessibility.size()) {
        return;
    }
    AccessibilityItem& item = m_accessibility[index];
    switch (item.feature) {
        case Feature::Magnifier:
            // (Ended with its own Win+Esc or its window.)
            StartMagnifier();
            break;
        case Feature::Narrator:
            // (Its shortcut starts it, or ends it when it's on.)
            PressKeys({VK_LWIN, VK_CONTROL, VK_RETURN});
            break;
        case Feature::OnScreenKeyboard:
            StartWithUiAccess(L"osk.exe");
            break;
        case Feature::ColorFilters:
            PressKeys({VK_LWIN, VK_CONTROL, 'C'});
            break;
        case Feature::StickyKeys: {
            STICKYKEYS sticky{sizeof(sticky)};
            if (SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(sticky),
                                     &sticky, 0)) {
                sticky.dwFlags ^= SKF_STICKYKEYSON;
                SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(sticky),
                                     &sticky,
                                     SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
            }
            break;
        }
    }
    // Shown right away; the next update confirms it.
    if (!item.button) {
        item.on = !item.on;
    }
}

// A monitor's brightness (its row; the only one with a single monitor).
void ControlPanel::SetBrightness(int row, int level) {
    if (row < 0 || row >= (int)m_brightnessMonitors.size()) {
        return;
    }
    BrightnessMonitor& monitor = m_brightnessMonitors[row];
    monitor.level = std::clamp(level, 0, 100);
    m_brightness = m_brightnessMonitors.front().level;
    m_brightnessJob->Ask(monitor.index, monitor.level);
    if (!m_brightnessJob->running.exchange(true) &&
        !StartBrightnessWorker(BrightnessThreadProc)) {
        m_brightnessJob->running = false;
    }
    Island::ShareBrightness(monitor.screen, monitor.level);
}

// A screen's brightness changed elsewhere (an island's wheel): shown here
// too. Whether it changed.
bool ControlPanel::TakeBrightness(HMONITOR screen, int level) {
    bool changed = false;
    for (auto& monitor : m_brightnessMonitors) {
        if (monitor.screen == screen && monitor.level != level) {
            monitor.level = level;
            changed = true;
        }
    }
    if (!m_brightnessMonitors.empty()) {
        m_brightness = m_brightnessMonitors.front().level;
    }
    return changed;
}

void ControlPanel::SetBrightnessAt(int row, POINT pt) {
    if (row < 0 || row >= (int)m_brightnessSliders.size()) {
        return;
    }
    const D2D1_RECT_F& rect = m_brightnessSliders[row];
    const float knob = rect.bottom - rect.top;
    SetBrightness(row, (int)std::lround(
        100 * std::clamp((pt.x - rect.left - knob / 2) /
                             (rect.right - rect.left - knob),
                         0.0f, 1.0f)));
}

// Each worker owns what it uses (a request, the shared brightness job), so
// the wait is bounded: a radio or a monitor stuck in a slow call can't hold
// the island's restart or the unload.
void ControlPanel::WaitForWorkers() {
    m_brightnessJob->notify = nullptr;
    JoinOwningThreads(m_workers, 5000);
    // (The hotspot's answers not taken yet.)
    MSG msg;
    while (m_hwnd && PeekMessage(&msg, m_hwnd, WM_APP_HOTSPOT_CONFIG,
                                 WM_APP_HOTSPOT_CONFIG, PM_REMOVE)) {
        delete (HotspotAnswer*)msg.lParam;
    }
}

// The line under a list: why it's empty, or how joining went.
std::wstring ControlPanel::StatusText() const {
    if (m_view == View::Wifi) {
        if (m_wifiRadio == 0) {
            return Tr(L"Wi-Fi is off", L"Wi-Fi desligado");
        }
        if (!m_hasWifi) {
            return Tr(L"No Wi-Fi adapter", L"Sem adaptador de Wi-Fi");
        }
        if (!m_wifiStatus.empty()) {
            return m_wifiStatus;
        }
        if (m_networks.empty() && m_wifiRefused) {
            return Tr(
                L"Windows shows the networks only to apps allowed to "
                L"use the location: click to turn on \"Let desktop "
                L"apps access your location\"",
                L"O Windows só mostra as redes para apps com acesso à "
                L"localização: clique para ligar \"Permitir que "
                L"aplicativos da área de trabalho acessem sua "
                L"localização\"");
        }
        if (m_networks.empty()) {
            return Tr(L"Looking for networks...", L"Procurando redes...");
        }
    } else if (m_view == View::Bluetooth) {
        if (m_bluetoothRadio == kRadioMissing) {
            return Tr(L"No Bluetooth", L"Sem Bluetooth");
        }
        if (m_bluetoothRadio == 0) {
            return Tr(L"Bluetooth is off", L"Bluetooth desligado");
        }
        if (m_bluetoothDevices.empty()) {
            return Tr(L"No paired devices", L"Nenhum dispositivo pareado");
        }
    } else if (m_view == View::Password) {
        return m_wifiStatus;
    } else if (m_view == View::Hotspot) {
        return m_hotspotStatus;
    }
    return {};
}

void ControlPanel::Update() {
    if (!g_island) {
        return;
    }
    m_network = g_island->QueryNetwork();
    m_device.clear();
    m_hasVolume = false;
    if (auto volume = g_island->GetVolume(&m_device)) {
        BOOL muted = FALSE;
        m_hasVolume = SUCCEEDED(volume->GetMasterVolumeLevelScalar(&m_volume)) &&
                      SUCCEEDED(volume->GetMute(&muted));
        m_muted = muted;
    }
    m_doNotDisturb = IsIslandQuiet();
    if (KomorebiShown()) {
        UpdateKomorebi();
    }
    if (m_nightLightChecking &&
        GetTickCount64() - m_nightLightCheckSince > 5000) {
        m_nightLightChecking = false;
    }
    if (!m_nightLightChecking) {
        m_nightLight = GetNightLight();
    }

    SYSTEM_POWER_STATUS power;
    m_hasBattery = GetSystemPowerStatus(&power) && !(power.BatteryFlag & 128) &&
                   power.BatteryFlag != 255 && power.BatteryLifePercent <= 100;
    if (m_hasBattery) {
        m_batteryPercent = power.BatteryLifePercent;
        m_charging = power.ACLineStatus == 1;
    }
    // (Not on every refresh: reading them loads powrprof.dll and every
    // plan's name.)
    if (!m_powerReadAt || NowSeconds() - m_powerReadAt > 10) {
        m_powerReadAt = NowSeconds();
        m_powerMode = GetPowerMode();
        m_powerPlans = GetPowerPlans(&m_activePlan);
    }
    m_microphoneMuted = g_island ? g_island->MicrophoneMuted() : -1;

    if (m_view == View::Output) {
        UpdateOutputs();
    } else if (m_view == View::Mixer) {
        UpdateApps();
    } else if (m_view == View::Wifi) {
        // The network cable: a physical Ethernet adapter, and its link.
        m_wired = {};
        PMIB_IF_TABLE2 table = nullptr;
        if (GetIfTable2(&table) == NO_ERROR && table) {
            for (ULONG i = 0; i < table->NumEntries; i++) {
                const MIB_IF_ROW2& row = table->Table[i];
                if (!row.InterfaceAndOperStatusFlags.HardwareInterface ||
                    row.InterfaceAndOperStatusFlags.FilterInterface ||
                    row.Type != IF_TYPE_ETHERNET_CSMACD) {
                    continue;
                }
                m_wired.present = true;
                if (row.OperStatus == IfOperStatusUp &&
                    row.MediaConnectState == MediaConnectStateConnected) {
                    m_wired.connected = true;
                    m_wired.speed =
                        std::max(m_wired.speed, row.ReceiveLinkSpeed);
                }
            }
            FreeMibTable(table);
        }
        const bool scan = m_wifiScanAgain;
        m_wifiScanAgain = false;
        UpdateWifi(scan);
    } else if (m_view == View::Bluetooth) {
        UpdateBluetooth();
    } else if (m_view == View::Project) {
        UpdateDisplayMode();
    } else if (m_view == View::Accessibility) {
        UpdateAccessibility();
    }
}

// The active output devices, the default one marked.
void ControlPanel::UpdateOutputs() {
    IMMDeviceEnumerator* devices = g_island ? g_island->Devices() : nullptr;
    m_outputs = devices ? ActiveOutputs(devices) : std::vector<AudioOutput>{};
}

void ClearMixerApps(std::vector<MixerApp>& apps) {
    for (auto& app : apps) {
        if (app.icon) {
            DestroyIcon(app.icon);
        }
    }
    apps.clear();
}

void ControlPanel::ClearApps() {
    ClearMixerApps(m_apps);
}

// Whether a session (of the system sounds, or of a process and its program
// file) goes in the app's entry: the same program, or the same process when
// its file can't be read.
bool SameMixerApp(const MixerApp& app,
                                bool system,
                                const std::wstring& path,
                                DWORD processId) {
    if (app.system != system) {
        return false;
    }
    if (system) {
        return true;
    }
    return path.empty() || app.path.empty()
               ? app.processId == processId
               : _wcsicmp(app.path.c_str(), path.c_str()) == 0;
}

// The programs playing sound on the default device, one entry per program
// (like Windows' mixer: Spotify, for one, plays from more than one of its
// processes).
void ReadMixerApps(std::vector<MixerApp>& apps, size_t maxApps,
                   std::wstring* deviceId) {
    if (deviceId) {
        deviceId->clear();
    }
    // Icons and bitmaps are kept for the programs still there.
    std::vector<MixerApp> previous = std::move(apps);
    apps.clear();

    IMMDeviceEnumerator* devices = g_island ? g_island->Devices() : nullptr;
    winrt::com_ptr<IMMDevice> device;
    winrt::com_ptr<IAudioSessionManager2> manager;
    winrt::com_ptr<IAudioSessionEnumerator> sessions;
    int count = 0;
    if (devices &&
        SUCCEEDED(devices->GetDefaultAudioEndpoint(eRender, eMultimedia,
                                                   device.put())) &&
        SUCCEEDED(device->Activate(kIID_IAudioSessionManager2, CLSCTX_ALL,
                                   nullptr, manager.put_void())) &&
        SUCCEEDED(manager->GetSessionEnumerator(sessions.put())) &&
        SUCCEEDED(sessions->GetCount(&count))) {
        if (deviceId) {
            LPWSTR id = nullptr;
            if (SUCCEEDED(device->GetId(&id))) {
                *deviceId = id;
                CoTaskMemFree(id);
            }
        }
        for (int i = 0; i < count; i++) {
            winrt::com_ptr<IAudioSessionControl> control;
            if (FAILED(sessions->GetSession(i, control.put()))) {
                continue;
            }
            AudioSessionState state;
            if (FAILED(control->GetState(&state)) ||
                state == AudioSessionStateExpired) {
                continue;
            }
            auto control2 = control.try_as<IAudioSessionControl2>();
            auto volume = control.try_as<ISimpleAudioVolume>();
            if (!control2 || !volume) {
                continue;
            }
            const bool system = control2->IsSystemSoundsSession() == S_OK;
            DWORD processId = 0;
            control2->GetProcessId(&processId);
            const std::wstring path =
                system ? std::wstring{} : ProcessImagePath(processId);

            auto it = std::find_if(apps.begin(), apps.end(),
                                   [&](const MixerApp& app) {
                                       return SameMixerApp(app, system, path,
                                                           processId);
                                   });
            if (it == apps.end()) {
                if (apps.size() >= maxApps) {
                    continue;
                }
                MixerApp app;
                app.processId = processId;
                app.path = path;
                app.system = system;
                float level = 1;
                BOOL muted = FALSE;
                volume->GetMasterVolume(&level);
                volume->GetMute(&muted);
                app.volume = level;
                app.muted = muted;
                apps.push_back(std::move(app));
                it = apps.end() - 1;
            }
            it->volumes.push_back(volume);
            if (deviceId) {
                LPWSTR id = nullptr;
                if (SUCCEEDED(control2->GetSessionInstanceIdentifier(&id))) {
                    it->sessionIds.emplace_back(id);
                    CoTaskMemFree(id);
                }
            }
        }
    }

    for (auto& app : apps) {
        if (app.system) {
            app.name = Tr(L"System sounds (notifications)",
                          L"Sons do sistema (notificações)");
            continue;
        }
        auto old = std::find_if(previous.begin(), previous.end(),
                                [&](const MixerApp& other) {
                                    return SameMixerApp(other, app.system,
                                                        app.path,
                                                        app.processId);
                                });
        if (old != previous.end()) {
            app.name = old->name;
            app.icon = std::exchange(old->icon, nullptr);
            app.bitmap = old->bitmap;
            continue;
        }
        if (!app.path.empty()) {
            app.name = GetProgramName(app.path.c_str());
            ExtractIconEx(app.path.c_str(), 0, &app.icon, nullptr, 1);
        }
        if (app.name.empty()) {
            app.name = Tr(L"App", L"App");
        }
    }
    for (auto& old : previous) {
        if (old.icon) {
            DestroyIcon(old.icon);
        }
    }
}

