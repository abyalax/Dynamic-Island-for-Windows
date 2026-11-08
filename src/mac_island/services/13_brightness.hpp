// DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO_2 (Windows 11's SDK), declared here
// because the toolchain's headers only have it for a newer Windows than the
// mod builds for. Its layout is the SDK's: the flags are bits (advanced
// color supported, active...), the mode is 0 for SDR, 1 for wide color
// gamut, 2 for HDR.
struct AdvancedColorInfo2 {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    UINT32 flags;
    UINT32 colorEncoding;
    UINT32 bitsPerColorChannel;
    UINT32 activeColorMode;
};
constexpr UINT32 kColorModeHdr = 2;

// Whether HDR is on for a display target. The newer query tells HDR from
// wide color gamut (which is also "advanced color"); Windows versions that
// lack it only have the older one, where advanced color on is HDR.
bool IsHdrOn(const DISPLAYCONFIG_PATH_TARGET_INFO& target) {
    AdvancedColorInfo2 info{};
    info.header = {DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO_2,
                   sizeof(info), target.adapterId, target.id};
    if (DisplayConfigGetDeviceInfo(&info.header) == ERROR_SUCCESS) {
        return info.activeColorMode == kColorModeHdr;
    }
    DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO older{};
    older.header = {DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO,
                    sizeof(older), target.adapterId, target.id};
    return DisplayConfigGetDeviceInfo(&older.header) == ERROR_SUCCESS &&
           older.advancedColorEnabled;
}

// The display target of an external screen that has HDR on, or false. The
// screen's device ("\\.\DISPLAY1") is matched to the paths' sources, as for
// the monitors' names.
bool HdrTarget(HMONITOR monitor, DISPLAYCONFIG_PATH_TARGET_INFO& target) {
    MONITORINFOEX info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfo(monitor, &info)) {
        return false;
    }
    for (const auto& path : ActiveDisplayPaths()) {
        if (!IsBuiltInPath(path) &&
            _wcsicmp(DisplayPathDevice(path).c_str(), info.szDevice) == 0 &&
            IsHdrOn(path.targetInfo)) {
            target = path.targetInfo;
            return true;
        }
    }
    return false;
}

// Set once Windows refused the undocumented request that sets the SDR content
// brightness (see WriteHdrBrightness): from then on a screen with HDR on is
// read and set like any other (DDC/CI, or WMI), so its slider shows and
// changes the same thing. Written by the brightness threads.
std::atomic<bool> g_hdrWriteFailed{false};

// An HDR screen's SDR content brightness as a slider's level, 0 to 100, or
// -1 (not an HDR screen, Windows didn't tell, or setting it doesn't work).
// Windows reports it in 1000ths of 80 nits: 1000 is its slider's 0, and each
// step of it adds 50.
int HdrBrightness(HMONITOR monitor) {
    DISPLAYCONFIG_PATH_TARGET_INFO target;
    if (g_hdrWriteFailed || !HdrTarget(monitor, target)) {
        return -1;
    }
    DISPLAYCONFIG_SDR_WHITE_LEVEL white{};
    white.header = {DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL,
                    sizeof(white), target.adapterId, target.id};
    if (DisplayConfigGetDeviceInfo(&white.header) != ERROR_SUCCESS) {
        return -1;
    }
    return std::clamp(((int)white.SDRWhiteLevel - 1000 + 25) / 50, 0, 100);
}

// The request that sets the SDR content brightness. Windows has no
// documented call for it: this one was found by others (Twinkle Tray uses
// it) and is in no header, so it may change with a Windows version. That's
// why its result is always checked: when it fails once, it's logged and the
// screen goes back to its own brightness (see g_hdrWriteFailed).
constexpr UINT32 kSetSdrWhiteLevelRequest = 0xFFFFFFEE;
struct SetSdrWhiteLevel {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    ULONG level;
    // 1, as the known callers send it.
    BYTE finalValue;
};

// Sets an HDR screen's SDR content brightness (0 to 100), checking that the
// screen still has HDR on (the mode can change meanwhile); false if it
// doesn't, or Windows refused.
bool WriteHdrBrightness(HMONITOR monitor, int level) {
    DISPLAYCONFIG_PATH_TARGET_INFO target;
    if (g_hdrWriteFailed || !HdrTarget(monitor, target)) {
        return false;
    }
    SetSdrWhiteLevel set{};
    set.header = {(DISPLAYCONFIG_DEVICE_INFO_TYPE)kSetSdrWhiteLevelRequest,
                  sizeof(set), target.adapterId, target.id};
    set.level = 1000 + 50 * std::clamp(level, 0, 100);
    set.finalValue = 1;
    const LONG result = DisplayConfigSetDeviceInfo(&set.header);
    if (result != ERROR_SUCCESS && !g_hdrWriteFailed.exchange(true)) {
        IslandLog(L"Setting the SDR content brightness failed (%ld): HDR "
                  L"screens use their own brightness now",
                  result);
    }
    return result == ERROR_SUCCESS;
}

// The monitors whose brightness can be read or set: the physical ones
// (ForEachPhysicalMonitor, asked over DDC/CI) or, with g_externalBrightness
// off, only the ones that don't need the cable: the built-in screen (WMI) and
// the HDR ones (see HdrBrightness). Those come with no handle
// (kNoPhysicalMonitor), so whoever uses it must not ask them over DDC/CI. (A
// null handle can't tell: Windows gives real ones that are null.)
const HANDLE kNoPhysicalMonitor = INVALID_HANDLE_VALUE;
bool HasPhysicalMonitor(const PHYSICAL_MONITOR& one) {
    return one.hPhysicalMonitor != kNoPhysicalMonitor;
}
template <typename F>
void ForEachBrightnessMonitor(F&& f) {
    if (g_externalBrightness) {
        ForEachPhysicalMonitor(f);
        return;
    }
    const HMONITOR builtIn = BuiltInScreen();
    std::vector<HMONITOR> monitors;
    EnumDisplayMonitors(nullptr, nullptr, CollectMonitorProc,
                        (LPARAM)&monitors);
    // (Each monitor its own index, also one left out: an HDR screen left
    // out once setting its SDR brightness failed doesn't hand its index, and
    // the level asked for it, to the next one.)
    int index = 0;
    for (HMONITOR monitor : monitors) {
        const int own = index++;
        DISPLAYCONFIG_PATH_TARGET_INFO target;
        if (monitor == builtIn ||
            (!g_hdrWriteFailed && HdrTarget(monitor, target))) {
            PHYSICAL_MONITOR none{};
            none.hPhysicalMonitor = kNoPhysicalMonitor;
            f(none, own, monitor, 0);
        }
    }
}

// WMI's namespace with the built-in screen's brightness (root\wmi), or
// null. On a brightness thread, with COM started.
winrt::com_ptr<IWbemServices> ConnectBrightnessWmi() {
    winrt::com_ptr<IWbemLocator> locator;
    winrt::com_ptr<IWbemServices> services;
    BSTR space = SysAllocString(L"ROOT\\WMI");
    if (!space ||
        FAILED(CoCreateInstance(__uuidof(WbemLocator), nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(locator.put()))) ||
        FAILED(locator->ConnectServer(space, nullptr, nullptr, nullptr, 0,
                                      nullptr, nullptr, services.put())) ||
        FAILED(CoSetProxyBlanket(services.get(), RPC_C_AUTHN_WINNT,
                                 RPC_C_AUTHZ_NONE, nullptr,
                                 RPC_C_AUTHN_LEVEL_CALL,
                                 RPC_C_IMP_LEVEL_IMPERSONATE, nullptr,
                                 EOAC_NONE))) {
        services = nullptr;
    }
    SysFreeString(space);
    return services;
}

// The first object a WMI query finds (waiting up to 2 s), or null.
winrt::com_ptr<IWbemClassObject> FirstWmiObject(IWbemServices* services,
                                                PCWSTR query) {
    winrt::com_ptr<IEnumWbemClassObject> found;
    winrt::com_ptr<IWbemClassObject> object;
    BSTR language = SysAllocString(L"WQL");
    BSTR text = SysAllocString(query);
    ULONG count = 0;
    if (language && text &&
        SUCCEEDED(services->ExecQuery(
            language, text,
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
            found.put()))) {
        found->Next(2000, 1, object.put(), &count);
    }
    SysFreeString(language);
    SysFreeString(text);
    if (!count) {
        object = nullptr;
    }
    return object;
}

// The built-in screen's brightness, 0 to 100, or -1.
int ReadBuiltInBrightness() {
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    int level = -1;
    if (const auto services = ConnectBrightnessWmi()) {
        if (const auto object = FirstWmiObject(
                services.get(),
                L"SELECT CurrentBrightness FROM WmiMonitorBrightness "
                L"WHERE Active=TRUE")) {
            VARIANT value;
            VariantInit(&value);
            if (SUCCEEDED(object->Get(L"CurrentBrightness", 0, &value,
                                      nullptr, nullptr)) &&
                value.vt == VT_UI1) {
                level = std::min<int>(value.bVal, 100);
            }
            VariantClear(&value);
        }
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    return level;
}

// Sets the built-in screen's brightness (0 to 100); false if it can't.
bool WriteBuiltInBrightness(int level) {
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool written = false;
    if (const auto services = ConnectBrightnessWmi()) {
        const auto instance = FirstWmiObject(
            services.get(),
            L"SELECT * FROM WmiMonitorBrightnessMethods WHERE Active=TRUE");
        winrt::com_ptr<IWbemClassObject> methods, inputs, call;
        BSTR className = SysAllocString(L"WmiMonitorBrightnessMethods");
        BSTR method = SysAllocString(L"WmiSetBrightness");
        VARIANT path;
        VariantInit(&path);
        VARIANT timeout;
        VariantInit(&timeout);
        timeout.vt = VT_I4;
        timeout.lVal = 0;
        VARIANT brightness;
        VariantInit(&brightness);
        brightness.vt = VT_UI1;
        brightness.bVal = (BYTE)std::clamp(level, 0, 100);
        written =
            instance && className && method &&
            SUCCEEDED(instance->Get(L"__PATH", 0, &path, nullptr, nullptr)) &&
            path.vt == VT_BSTR &&
            SUCCEEDED(services->GetObject(className, 0, nullptr,
                                          methods.put(), nullptr)) &&
            SUCCEEDED(methods->GetMethod(L"WmiSetBrightness", 0,
                                         inputs.put(), nullptr)) &&
            SUCCEEDED(inputs->SpawnInstance(0, call.put())) &&
            SUCCEEDED(call->Put(L"Timeout", 0, &timeout, CIM_UINT32)) &&
            SUCCEEDED(call->Put(L"Brightness", 0, &brightness, CIM_UINT8)) &&
            SUCCEEDED(services->ExecMethod(path.bstrVal, method, 0, nullptr,
                                           call.get(), nullptr, nullptr));
        VariantClear(&path);
        SysFreeString(className);
        SysFreeString(method);
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    return written;
}

// A physical monitor's brightness, 0 to 100, or -1: the SDR content
// brightness of a screen with HDR on (`hdr` says so, see HdrBrightness), or
// over DDC/CI, or, for the first one on `builtIn` (BuiltInScreen) without it,
// through WMI (`builtIn` is then cleared: one built-in screen). One without a
// handle (see ForEachBrightnessMonitor) is never asked over DDC/CI.
int PhysicalBrightness(const PHYSICAL_MONITOR& one,
                       HMONITOR monitor,
                       HMONITOR& builtIn,
                       bool* hdr = nullptr) {
    if (const int level = HdrBrightness(monitor); level >= 0) {
        if (hdr) {
            *hdr = true;
        }
        return level;
    }
    DWORD minimum, current, maximum;
    if (HasPhysicalMonitor(one) &&
        GetMonitorBrightness(one.hPhysicalMonitor, &minimum, &current,
                             &maximum) &&
        maximum > minimum) {
        return (int)std::lround(100.0 * (current - minimum) /
                                (maximum - minimum));
    }
    if (builtIn && monitor == builtIn) {
        builtIn = nullptr;
        return ReadBuiltInBrightness();
    }
    return -1;
}

// The whole brightness steps a wheel turn makes, 5% a notch (WHEEL_DELTA).
// Touchpads and smooth wheels send smaller turns: what's left of a step is
// kept in `rest` for the next turn instead of being lost.
int BrightnessWheelSteps(float& rest, int delta) {
    rest += 5.0f * delta / WHEEL_DELTA;
    const int steps = (int)rest;
    rest -= steps;
    return steps;
}

// A monitor whose brightness can be changed.
struct BrightnessMonitor {
    // Its place in ForEachPhysicalMonitor's order (see BrightnessJob).
    int index = 0;
    std::wstring name;
    // From 0 to 100.
    int level = 0;
    // The screen it shows (an island's monitor).
    HMONITOR screen = nullptr;
    // Its slider is the SDR content brightness (the screen has HDR on).
    bool hdr = false;
};

// The same names numbered ("DELL P2419H (2)"); a missing one is
// "Monitor 1", "Monitor 2"...
void NameBrightnessMonitors(std::vector<BrightnessMonitor>& list) {
    std::vector<std::wstring> names;
    for (size_t i = 0; i < list.size(); i++) {
        std::wstring name = list[i].name;
        if (name.empty()) {
            name = std::wstring(Tr(L"Monitor", L"Monitor")) + L" " +
                   std::to_wstring(i + 1);
        } else {
            int same = 0;
            int place = 0;
            for (size_t j = 0; j < list.size(); j++) {
                if (list[j].name == list[i].name) {
                    same++;
                    place += j < i;
                }
            }
            if (same > 1) {
                name += L" (" + std::to_wstring(place + 1) + L")";
            }
        }
        names.push_back(std::move(name));
    }
    for (size_t i = 0; i < list.size(); i++) {
        list[i].name = std::move(names[i]);
    }
}

// The monitors that tell their brightness, with their names.
std::vector<BrightnessMonitor> ReadBrightnessMonitors() {
    const auto names = MonitorFriendlyNames();
    std::vector<BrightnessMonitor> list;
    const HMONITOR builtInScreen = BuiltInScreen();
    HMONITOR builtIn = builtInScreen;
    ForEachBrightnessMonitor([&](const PHYSICAL_MONITOR& one, int index,
                                 HMONITOR monitor, int within) {
        const bool wasBuiltIn = builtIn != nullptr;
        bool hdr = false;
        const int level = PhysicalBrightness(one, monitor, builtIn, &hdr);
        if (level < 0) {
            return;
        }
        BrightnessMonitor entry;
        entry.index = index;
        entry.screen = monitor;
        entry.level = level;
        entry.hdr = hdr;
        MONITORINFOEX info{};
        info.cbSize = sizeof(info);
        if (GetMonitorInfo(monitor, &info)) {
            int k = 0;
            for (const auto& [device, name] : names) {
                if (_wcsicmp(device.c_str(), info.szDevice) == 0 &&
                    k++ == within) {
                    entry.name = name;
                    break;
                }
            }
        }
        // (The built-in screen read through WMI says so; others get
        // Windows' generic name, like "Generic PnP Monitor".)
        if (entry.name.empty()) {
            entry.name = wasBuiltIn && !builtIn && monitor == builtInScreen
                             ? Tr(L"Built-in display", L"Tela integrada")
                             : one.szPhysicalMonitorDescription;
        }
        list.push_back(std::move(entry));
    });
    NameBrightnessMonitors(list);
    // (With HDR on its slider isn't the panel's brightness: said in its name,
    // which shows when there's more than one.)
    for (auto& entry : list) {
        if (entry.hdr) {
            entry.name += Tr(L" · HDR: SDR content", L" · HDR: conteúdo SDR");
        }
    }
    return list;
}

// From 0 to 100 (the screen's monitor's, or without a screen the first
// one's), -1 if none answers. A screen that doesn't answer (no DDC/CI) has
// none: not another monitor's, which WriteBrightness wouldn't change there.
// Only the monitors asked about are asked over DDC/CI (slow, and some
// flicker).
int ReadBrightness(HMONITOR screen) {
    int level = -1;
    HMONITOR builtIn = BuiltInScreen();
    for (const bool any : {false, true}) {
        if (any != !screen) {
            continue;
        }
        ForEachBrightnessMonitor(
            [&](const PHYSICAL_MONITOR& one, int, HMONITOR monitor, int) {
                if (level < 0 && (any || monitor == screen)) {
                    level = PhysicalBrightness(one, monitor, builtIn);
                }
            });
        if (level >= 0) {
            break;
        }
    }
    return level;
}

// Changes of the displays (WM_DISPLAYCHANGE), so a list of monitors read
// before one is known to be old.
std::atomic<int> g_displayChanges;

constexpr int kMaxBrightnessMonitors = 8;

// The levels asked for, by monitor (-1: left as it is), and for the
// monitors of a screen. False if one asked didn't answer (turned off: still
// a display for Windows, but its DDC/CI is gone).
bool WriteBrightness(const int (&levels)[kMaxBrightnessMonitors],
                     HMONITOR screen,
                     int screenLevel) {
    bool answered = true;
    HMONITOR builtIn = BuiltInScreen();
    ForEachBrightnessMonitor(
        [&](const PHYSICAL_MONITOR& one, int index, HMONITOR monitor, int) {
            const int level =
                screenLevel >= 0 && monitor == screen ? screenLevel
                : index < kMaxBrightnessMonitors      ? levels[index]
                                                      : -1;
            if (level < 0) {
                return;
            }
            // A screen with HDR on (checked again: the mode can change) has
            // its SDR content brightness set; if Windows refuses, the
            // monitor's own is the way left, when it has one.
            if (WriteHdrBrightness(monitor, level)) {
                return;
            }
            DWORD minimum, current, maximum;
            if (!HasPhysicalMonitor(one) ||
                !GetMonitorBrightness(one.hPhysicalMonitor, &minimum,
                                      &current, &maximum) ||
                maximum <= minimum) {
                // The built-in screen through WMI (see BuiltInScreen).
                if (builtIn && monitor == builtIn) {
                    builtIn = nullptr;
                    answered = WriteBuiltInBrightness(level) && answered;
                } else {
                    answered = false;
                }
                return;
            }
            SetMonitorBrightness(
                one.hPhysicalMonitor,
                minimum +
                    (DWORD)std::lround((maximum - minimum) * level / 100.0));
        });
    return answered;
}

void OpenUri(PCWSTR uri) {
    ShellExecute(nullptr, L"open", uri, nullptr, nullptr, SW_SHOWNORMAL);
}

// Whether "Do not disturb" is on. Windows publishes it as a notification
// state (WNF_SHEL_QUIETHOURS_ACTIVE_PROFILE_CHANGED): 0 when off, otherwise
// the active profile (priority only, alarms only).
bool IsDoNotDisturbOn() {
    using NtQueryWnfStateData_t = LONG(NTAPI*)(
        const ULONGLONG* stateName, const void* typeId,
        const void* explicitScope, ULONG* changeStamp, void* buffer,
        ULONG* bufferSize);
    static const auto query = (NtQueryWnfStateData_t)GetProcAddress(
        GetModuleHandle(L"ntdll.dll"), "NtQueryWnfStateData");
    if (!query) {
        return false;
    }
    constexpr ULONGLONG kQuietHoursState = 0x0D83063EA3BF1C75;
    DWORD profile = 0;
    ULONG size = sizeof(profile);
    ULONG changeStamp;
    return query(&kQuietHoursState, nullptr, nullptr, &changeStamp, &profile,
                 &size) >= 0 &&
           size == sizeof(profile) && profile != 0;
}

// Turns "Do not disturb" on or off, with the interface the Windows
// notification center uses (the quiet hours settings). It isn't documented;
// "Do not disturb" on is the "priority only" profile (or, `alarmsOnly`, the
// stricter "alarms only" one, which lets no app through). Returns whether it
// worked.
struct IQuietHoursSettings : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE get_UserSelectedProfile(LPWSTR* id) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_UserSelectedProfile(LPCWSTR id) = 0;
};

constexpr GUID kCLSID_QuietHoursSettings = {
    0xf53321fa,
    0x34f8,
    0x4b7f,
    {0xb9, 0xa3, 0x36, 0x18, 0x77, 0xcb, 0x94, 0xcf}};
constexpr GUID kIID_IQuietHoursSettings = {
    0x6bff4732,
    0x81ec,
    0x4ffb,
    {0xae, 0x67, 0xb6, 0xc1, 0xbc, 0x29, 0x63, 0x1f}};

bool SetDoNotDisturb(bool on, bool alarmsOnly = false) {
    winrt::com_ptr<IQuietHoursSettings> settings;
    if (FAILED(CoCreateInstance(kCLSID_QuietHoursSettings, nullptr,
                                CLSCTX_LOCAL_SERVER, kIID_IQuietHoursSettings,
                                settings.put_void()))) {
        IslandLog(L"The quiet hours settings aren't available");
        return false;
    }
    const HRESULT result = settings->put_UserSelectedProfile(
        !on          ? L"Microsoft.QuietHoursProfile.Unrestricted"
        : alarmsOnly ? L"Microsoft.QuietHoursProfile.AlarmsOnly"
                     : L"Microsoft.QuietHoursProfile.PriorityOnly");
    if (FAILED(result)) {
        IslandLog(L"Switching \"Do not disturb\" failed: %08X", (unsigned)result);
    }
    return SUCCEEDED(result);
}

////////////////////////////////////////////////////////////////////////////////
// The island's own notifications (an option, off unless chosen): Windows'
// "Do not disturb" is kept on while the island runs, so Windows shows no
// banners and plays no sounds (the notifications still go to the
// notification center and its database, where the island reads them, and
// the new-notification state still changes); the island shows each one and
// plays its sound as it comes. Its own "Do not disturb" (g_islandQuiet) then
// silences only it, and makes Windows' "alarms only" (an app on Windows'
// priority list would show Windows' banner and play its sound; calls,
// alarms and reminders still come through the island). Windows' setting
// goes back when the island stops, the
// option is turned off, at the next start after a crash ("turnedDndOn"), and
// from explorer if the island's process ends without doing it (see
// WatchIslandProcess). If "Do not disturb" was already on, it's left alone.

std::atomic<bool> g_islandQuiet;
bool g_turnedDndOn;
// When the island turned it on (NowSeconds).
double g_turnedDndOnAt;
// Not switched on again until the option is turned on again: switching
// failed (the interface is missing on this build), or it was turned off
// from Windows (by the user, or a Focus rule ending).
bool g_dndSwitchFailed;
// It was seen on while the island wanted it (turned on by the island, or
// already on), so it going off is Windows' doing.
bool g_dndSeenOn;
// The profile the island set while it has it on: 1 "alarms only" (with its
// own "Do not disturb"), 0 "priority only", -1 not known (just started).
int g_turnedDndAlarms = -1;

