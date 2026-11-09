#pragma once
// Told by the audio service when a recording starts or stops, a session
// comes, or a microphone is plugged in or out: the thread looks again.
class PrivacyAudioEvents final : public IAudioSessionNotification,
                                 public IAudioSessionEvents,
                                 public IMMNotificationClient {
   public:
    explicit PrivacyAudioEvents(HANDLE changed) : m_changed(changed) {}

    // IUnknown (the three interfaces share it).
    IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override {
        if (riid == IID_IUnknown ||
            riid == __uuidof(IAudioSessionNotification)) {
            *object = static_cast<IAudioSessionNotification*>(this);
        } else if (riid == __uuidof(IAudioSessionEvents)) {
            *object = static_cast<IAudioSessionEvents*>(this);
        } else if (riid == __uuidof(IMMNotificationClient)) {
            *object = static_cast<IMMNotificationClient*>(this);
        } else {
            *object = nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&m_refs);
    }
    IFACEMETHODIMP_(ULONG) Release() override {
        const ULONG refs = InterlockedDecrement(&m_refs);
        if (!refs) {
            delete this;
        }
        return refs;
    }

    IFACEMETHODIMP OnSessionCreated(IAudioSessionControl*) override {
        return Changed();
    }
    IFACEMETHODIMP OnStateChanged(AudioSessionState) override {
        return Changed();
    }
    IFACEMETHODIMP OnSessionDisconnected(
        AudioSessionDisconnectReason) override {
        return Changed();
    }
    IFACEMETHODIMP OnDisplayNameChanged(LPCWSTR, LPCGUID) override {
        return S_OK;
    }
    IFACEMETHODIMP OnIconPathChanged(LPCWSTR, LPCGUID) override { return S_OK; }
    IFACEMETHODIMP OnSimpleVolumeChanged(float, BOOL, LPCGUID) override {
        return S_OK;
    }
    IFACEMETHODIMP OnChannelVolumeChanged(DWORD, float[], DWORD, LPCGUID)
        override {
        return S_OK;
    }
    IFACEMETHODIMP OnGroupingParamChanged(LPCGUID, LPCGUID) override {
        return S_OK;
    }
    IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR, DWORD) override {
        return Changed();
    }
    IFACEMETHODIMP OnDeviceAdded(LPCWSTR) override { return Changed(); }
    IFACEMETHODIMP OnDeviceRemoved(LPCWSTR) override { return Changed(); }
    IFACEMETHODIMP OnDefaultDeviceChanged(EDataFlow, ERole, LPCWSTR) override {
        return S_OK;
    }
    IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override {
        return S_OK;
    }

    // Told nothing more (its event is about to be closed; a call already
    // on its way can still come).
    void Forget() {
        AcquireSRWLockExclusive(&m_lock);
        m_changed = nullptr;
        ReleaseSRWLockExclusive(&m_lock);
    }

   private:
    HRESULT Changed() {
        AcquireSRWLockShared(&m_lock);
        if (m_changed) {
            SetEvent(m_changed);
        }
        ReleaseSRWLockShared(&m_lock);
        return S_OK;
    }
    SRWLOCK m_lock = SRWLOCK_INIT;
    HANDLE m_changed;
    LONG m_refs = 1;
};

// The microphones' sessions watched (each told to the events above), to
// stop watching them.
struct MicrophoneWatch {
    std::vector<winrt::com_ptr<IAudioSessionManager2>> managers;
    std::vector<winrt::com_ptr<IAudioSessionControl>> sessions;
};

void UnwatchMicrophones(MicrophoneWatch& watch, PrivacyAudioEvents* events) {
    for (auto& session : watch.sessions) {
        session->UnregisterAudioSessionNotification(events);
    }
    for (auto& manager : watch.managers) {
        manager->UnregisterSessionNotification(events);
    }
    watch = {};
}

// Every microphone's sessions watched again, and the apps recording now.
std::wstring WatchMicrophones(IMMDeviceEnumerator* devices,
                              PrivacyAudioEvents* events,
                              MicrophoneWatch& watch) {
    UnwatchMicrophones(watch, events);
    std::vector<std::wstring> names;
    winrt::com_ptr<IMMDeviceCollection> list;
    UINT count = 0;
    if (FAILED(devices->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE,
                                           list.put())) ||
        FAILED(list->GetCount(&count))) {
        return {};
    }
    for (UINT i = 0; i < count; i++) {
        winrt::com_ptr<IMMDevice> device;
        winrt::com_ptr<IAudioSessionManager2> manager;
        winrt::com_ptr<IAudioSessionEnumerator> sessions;
        // (Its sessions are listed before its notifications are asked for:
        // they only start then.)
        if (FAILED(list->Item(i, device.put())) ||
            FAILED(device->Activate(kIID_IAudioSessionManager2, CLSCTX_ALL,
                                    nullptr, manager.put_void())) ||
            FAILED(manager->GetSessionEnumerator(sessions.put()))) {
            continue;
        }
        if (SUCCEEDED(manager->RegisterSessionNotification(events))) {
            watch.managers.push_back(manager);
        }
        int sessionCount = 0;
        sessions->GetCount(&sessionCount);
        for (int j = 0; j < sessionCount; j++) {
            winrt::com_ptr<IAudioSessionControl> control;
            if (FAILED(sessions->GetSession(j, control.put()))) {
                continue;
            }
            if (SUCCEEDED(control->RegisterAudioSessionNotification(events))) {
                watch.sessions.push_back(control);
            }
            AudioSessionState state;
            DWORD processId = 0;
            auto control2 = control.try_as<IAudioSessionControl2>();
            if (SUCCEEDED(control->GetState(&state)) &&
                state == AudioSessionStateActive && control2 &&
                control2->IsSystemSoundsSession() != S_OK &&
                SUCCEEDED(control2->GetProcessId(&processId)) && processId) {
                names.push_back(ProcessProgramName(processId));
            }
        }
    }
    return JoinNames(names);
}

// Media Foundation's sensor activity monitor (mfidl.h, not in the mod's
// headers; its functions are looked up, so Windows without Media Foundation,
// like the N editions, just shows no camera).
struct IMFSensorProcessActivity : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetProcessId(ULONG* processId) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStreamingState(BOOL* streaming) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStreamingMode(int* mode) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetReportTime(FILETIME* time) = 0;
};
struct IMFSensorActivityReport : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetFriendlyName(LPWSTR,
                                                      ULONG,
                                                      ULONG*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSymbolicLink(LPWSTR,
                                                      ULONG,
                                                      ULONG*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcessCount(ULONG* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetProcessActivity(ULONG index, IMFSensorProcessActivity** activity) = 0;
};
struct IMFSensorActivitiesReport : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetCount(ULONG* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetActivityReport(ULONG index, IMFSensorActivityReport** report) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetActivityReportByDeviceName(LPCWSTR name,
                                  IMFSensorActivityReport** report) = 0;
};
struct IMFSensorActivitiesReportCallback : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE
    OnActivitiesReport(IMFSensorActivitiesReport* reports) = 0;
};
struct IMFSensorActivityMonitor : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Start() = 0;
    virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
};
constexpr GUID kIID_IMFSensorActivitiesReportCallback = {
    0xde5072ee,
    0xdbe3,
    0x46dc,
    {0x8a, 0x87, 0xb6, 0xf6, 0x31, 0x19, 0x47, 0x51}};
// MF_VERSION (MF_SDK_VERSION 2, MF_API_VERSION 0x70) and MFSTARTUP_LITE.
constexpr ULONG kMfVersion = 0x00020070;
constexpr DWORD kMfStartupLite = 1;

// The apps streaming from a camera, as the monitor last told them (on a
// Media Foundation thread).
class CameraActivity final : public IMFSensorActivitiesReportCallback {
   public:
    explicit CameraActivity(HANDLE changed) : m_changed(changed) {}

    IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override {
        if (riid == IID_IUnknown ||
            riid == kIID_IMFSensorActivitiesReportCallback) {
            *object = static_cast<IMFSensorActivitiesReportCallback*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&m_refs);
    }
    IFACEMETHODIMP_(ULONG) Release() override {
        const ULONG refs = InterlockedDecrement(&m_refs);
        if (!refs) {
            delete this;
        }
        return refs;
    }

    IFACEMETHODIMP OnActivitiesReport(
        IMFSensorActivitiesReport* reports) override {
        std::vector<std::wstring> names;
        ULONG count = 0;
        if (reports && SUCCEEDED(reports->GetCount(&count))) {
            for (ULONG i = 0; i < count; i++) {
                winrt::com_ptr<IMFSensorActivityReport> report;
                ULONG processes = 0;
                if (FAILED(reports->GetActivityReport(i, report.put())) ||
                    FAILED(report->GetProcessCount(&processes))) {
                    continue;
                }
                for (ULONG j = 0; j < processes; j++) {
                    winrt::com_ptr<IMFSensorProcessActivity> activity;
                    BOOL streaming = FALSE;
                    ULONG processId = 0;
                    if (SUCCEEDED(report->GetProcessActivity(
                            j, activity.put())) &&
                        SUCCEEDED(activity->GetStreamingState(&streaming)) &&
                        streaming &&
                        SUCCEEDED(activity->GetProcessId(&processId))) {
                        names.push_back(ProcessProgramName(processId));
                    }
                }
            }
        }
        AcquireSRWLockExclusive(&m_lock);
        m_apps = JoinNames(names);
        if (m_changed) {
            SetEvent(m_changed);
        }
        ReleaseSRWLockExclusive(&m_lock);
        return S_OK;
    }

    std::wstring Apps() {
        AcquireSRWLockShared(&m_lock);
        std::wstring apps = m_apps;
        ReleaseSRWLockShared(&m_lock);
        return apps;
    }
    // Told nothing more (its event is about to be closed).
    void Forget() {
        AcquireSRWLockExclusive(&m_lock);
        m_changed = nullptr;
        ReleaseSRWLockExclusive(&m_lock);
    }

   private:
    SRWLOCK m_lock = SRWLOCK_INIT;
    HANDLE m_changed;
    std::wstring m_apps;
    LONG m_refs = 1;
};

// The parameter: the main island's window, told of each change.
DWORD WINAPI PrivacyThreadProc(LPVOID parameter) {
    const HWND island = (HWND)parameter;
    HANDLE changed = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!changed) {
        return 0;
    }
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // The microphones.
    auto* events = new PrivacyAudioEvents(changed);
    winrt::com_ptr<IMMDeviceEnumerator> devices;
    if (SUCCEEDED(CoCreateInstance(kCLSID_MMDeviceEnumerator, nullptr,
                                   CLSCTX_ALL, kIID_IMMDeviceEnumerator,
                                   devices.put_void()))) {
        devices->RegisterEndpointNotificationCallback(events);
    }
    MicrophoneWatch microphones;

    // The cameras.
    HMODULE mfplat =
        LoadLibraryEx(L"mfplat.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    HMODULE sensors = LoadLibraryEx(L"mfsensorgroup.dll", nullptr,
                                    LOAD_LIBRARY_SEARCH_SYSTEM32);
    using Startup_t = HRESULT(WINAPI*)(ULONG, DWORD);
    using Shutdown_t = HRESULT(WINAPI*)();
    using Create_t = HRESULT(WINAPI*)(IMFSensorActivitiesReportCallback*,
                                      IMFSensorActivityMonitor**);
    auto startup =
        mfplat ? (Startup_t)GetProcAddress(mfplat, "MFStartup") : nullptr;
    auto shutdown =
        mfplat ? (Shutdown_t)GetProcAddress(mfplat, "MFShutdown") : nullptr;
    auto create = sensors ? (Create_t)GetProcAddress(
                                sensors, "MFCreateSensorActivityMonitor")
                          : nullptr;
    const bool started = startup && shutdown && create &&
                         SUCCEEDED(startup(kMfVersion, kMfStartupLite));
    auto* camera = new CameraActivity(changed);
    winrt::com_ptr<IMFSensorActivityMonitor> monitor;
    if (started && SUCCEEDED(create(camera, monitor.put()))) {
        monitor->Start();
    }

    // The location.
    HKEY store = nullptr;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, kConsentStoreKey, 0,
                     KEY_READ | KEY_NOTIFY, &store) != ERROR_SUCCESS) {
        store = nullptr;
    }

    PrivacyUse last;
    bool told = false;
    while (true) {
        // Watched before it's read, so no change in between is missed.
        if (store) {
            RegNotifyChangeKeyValue(
                store, TRUE,
                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, changed,
                TRUE);
        }
        PrivacyUse use;
        if (devices) {
            use.microphone =
                WatchMicrophones(devices.get(), events, microphones);
        }
        use.camera = camera->Apps();
        if (store) {
            use.location = AppsUsing(store, L"location", RunningPrograms());
        }
        if (!told || !(use == last)) {
            told = true;
            AcquireSRWLockExclusive(&g_privacyLock);
            g_privacy = use;
            ReleaseSRWLockExclusive(&g_privacyLock);
            PostMessage(island, WM_APP_PRIVACY, 0, 0);
            last = std::move(use);
        }
        const HANDLE waits[] = {g_privacyStop, changed};
        if (WaitForMultipleObjects(ARRAYSIZE(waits), waits, FALSE, INFINITE) !=
            WAIT_OBJECT_0 + 1) {
            break;
        }
        // Several changes come at once (an app opening its microphone):
        // looked at once they're done.
        if (WaitForSingleObject(g_privacyStop, 150) == WAIT_OBJECT_0) {
            break;
        }
    }

    if (monitor) {
        monitor->Stop();
        monitor = nullptr;
    }
    camera->Forget();
    camera->Release();
    if (started) {
        shutdown();
    }
    UnwatchMicrophones(microphones, events);
    if (devices) {
        devices->UnregisterEndpointNotificationCallback(events);
        devices = nullptr;
    }
    events->Forget();
    events->Release();
    if (store) {
        RegCloseKey(store);
    }
    for (HMODULE module : {mfplat, sensors}) {
        if (module) {
            FreeLibrary(module);
        }
    }
    CoUninitialize();
    CloseHandle(changed);
    return 0;
}

void StopPrivacyWatch() {
    if (g_privacyThread) {
        SetEvent(g_privacyStop);
        WaitForSingleObject(g_privacyThread, INFINITE);
        CloseHandle(g_privacyThread);
        g_privacyThread = nullptr;
    }
    if (g_privacyStop) {
        CloseHandle(g_privacyStop);
        g_privacyStop = nullptr;
    }
    AcquireSRWLockExclusive(&g_privacyLock);
    g_privacy = {};
    ReleaseSRWLockExclusive(&g_privacyLock);
}

////////////////////////////////////////////////////////////////////////////////
// The keyboard's input method (an option, see Island::UpdateInputMethodWatch):
// the keyboard of the app in front, and for Chinese, Japanese and Korean,
// whether its input method types its own letters ("中") or English ones
// ("英"): asked to the app's input method window (WM_IME_CONTROL), on a
// thread of its own while the item is watched (an app that's busy would
// hold it), over and over while it's in front: Shift tells no event with
// some input methods (Microsoft Pinyin's newest version, #6460).

// Tells the island (wParam: the language in its low word, 0 for another
// keyboard, 1 for the input method's own letters, 2 for English ones in its
// high word; lParam: how many keyboards there are, with kInputMethodAsked
// when it comes from InputMethodThreadProc).
constexpr UINT WM_APP_INPUT_METHOD = WM_APP + 74;
constexpr LPARAM kInputMethodAsked = 0x10000;
// Above it, the question's number (an older answer still queued isn't taken
// for the current thread's).
constexpr int kInputMethodIdShift = 17;
constexpr UINT kInputMethodIdMask = 0x7FFF;

// The thread's: it asks each time `ask` is set (once more if it's set
// again meanwhile), with the question's number then, until `stop`.
struct InputMethodJob {
    HWND island;
    HANDLE ask;
    HANDLE stop;
    std::atomic<UINT> id{0};
};
constexpr UINT_PTR kInputMethodTimerId = 25;
constexpr UINT_PTR kInputMethodPollTimerId = 30;
constexpr UINT kInputMethodPollMs = 500;
// Caps Lock told (see ShortcutKeyProc): read once Windows has toggled it.
constexpr UINT_PTR kCapsLockTimerId = 58;
constexpr UINT kCapsLockWaitMs = 50;
constexpr UINT kInputMethodWaitMs = 100;
constexpr DWORD kEventObjectImeShow = 0x8027;
constexpr DWORD kEventObjectImeChange = 0x8029;
constexpr WPARAM kImcGetConversionMode = 0x0001;
constexpr WPARAM kImcGetOpenStatus = 0x0005;
// TSF's compartment of the keyboard's conversion mode (the IME_CMODE_ bits).
constexpr GUID kCompartmentInputModeConversion = {
    0xccf05dd8,
    0x4a87,
    0x11d7,
    {0xa6, 0xe2, 0x00, 0x06, 0x5b, 0x84, 0x43, 0x5c}};

// The keyboard's language of the app in front.
LANGID ForegroundLanguage(HWND foreground) {
    return LOWORD((UINT_PTR)GetKeyboardLayout(
        foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0));
}

// Whether it's typed through an input method that has its own letters and
// English ones.
bool HasInputModes(LANGID language) {
    const WORD primary = PRIMARYLANGID(language);
    return primary == LANG_CHINESE || primary == LANG_JAPANESE ||
           primary == LANG_KOREAN;
}

// The mode as TSF keeps it for Windows' own indicator, in global
// compartments (the same for every app): for the apps whose input method
// window answers nothing (Windows Terminal, Store apps). false if TSF has
// none (the open one is then taken as on).
bool ReadTsfInputMode(DWORD* conversion, DWORD* open) {
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool read = false;
    {
        winrt::com_ptr<ITfThreadMgr> threadMgr;
        winrt::com_ptr<ITfCompartmentMgr> compartments;
        const auto get = [&](REFGUID guid, DWORD* value) {
            winrt::com_ptr<ITfCompartment> compartment;
            VARIANT variant;
            VariantInit(&variant);
            const bool got =
                SUCCEEDED(compartments->GetCompartment(guid,
                                                       compartment.put())) &&
                SUCCEEDED(compartment->GetValue(&variant)) &&
                variant.vt == VT_I4;
            if (got) {
                *value = (DWORD)variant.lVal;
            }
            VariantClear(&variant);
            return got;
        };
        read = SUCCEEDED(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr,
                                          CLSCTX_INPROC_SERVER,
                                          IID_PPV_ARGS(threadMgr.put()))) &&
               SUCCEEDED(threadMgr->GetGlobalCompartment(compartments.put())) &&
               get(kCompartmentInputModeConversion, conversion);
        if (read) {
            *open = 1;
            get(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, open);
        }
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    return read;
}

// The input method's mode, asked to the input method window of the app in
// front (its thread's, from the window that has the keyboard focus), and to
// TSF if that doesn't answer. What it answered is logged when it changes
// (#6460: an input method whose mode never shows, to see which step doesn't
// answer).
void AskInputMethod(const InputMethodJob& job) {
    const HWND foreground = GetForegroundWindow();
    const LANGID language = ForegroundLanguage(foreground);
    WORD mode = 0;
    if (HasInputModes(language)) {
        mode = 2;
        GUITHREADINFO gui{sizeof(gui)};
        const HWND focus =
            foreground &&
                    GetGUIThreadInfo(
                        GetWindowThreadProcessId(foreground, nullptr), &gui) &&
                    gui.hwndFocus
                ? gui.hwndFocus
                : foreground;
        DWORD_PTR open = 0;
        DWORD_PTR conversion = 0;
        const HWND ime = focus ? ImmGetDefaultIMEWnd(focus) : nullptr;
        const bool openAsked =
            ime && SendMessageTimeout(ime, WM_IME_CONTROL, kImcGetOpenStatus,
                                      0, SMTO_ABORTIFHUNG, 300, &open);
        const bool conversionAsked =
            ime &&
            SendMessageTimeout(ime, WM_IME_CONTROL, kImcGetConversionMode, 0,
                               SMTO_ABORTIFHUNG, 300, &conversion);
        DWORD tsfOpen = 0;
        DWORD tsfConversion = 0;
        const bool tsf = !(openAsked && conversionAsked) &&
                         ReadTsfInputMode(&tsfConversion, &tsfOpen);
        if (tsf ? tsfOpen && (tsfConversion & IME_CMODE_NATIVE)
                : openAsked && open && conversionAsked &&
                      (conversion & IME_CMODE_NATIVE)) {
            mode = 1;
        }
        static std::atomic<unsigned long long> logged{~0ULL};
        const unsigned long long seen =
            ((unsigned long long)language << 48) |
            ((unsigned long long)(ime != nullptr) << 40) |
            ((unsigned long long)openAsked << 39) |
            ((unsigned long long)conversionAsked << 38) |
            ((unsigned long long)tsf << 37) |
            ((unsigned long long)(tsfOpen & 1) << 36) |
            ((unsigned long long)(open & 0xF) << 32) |
            ((unsigned long long)(tsfConversion & 0xFFFF) << 16) |
            (conversion & 0xFFFF);
        if (logged.exchange(seen) != seen) {
            IslandLog(L"Input method: language %04X, its window %s, open %s "
                      L"%u, mode %s 0x%X, TSF %s 0x%X open %u",
                      language, ime ? L"found" : L"none",
                      openAsked ? L"answered" : L"not answered",
                      (unsigned)open,
                      conversionAsked ? L"answered" : L"not answered",
                      (unsigned)conversion, tsf ? L"read" : L"not read",
                      (unsigned)tsfConversion, (unsigned)tsfOpen);
        }
    }
    PostMessage(job.island, WM_APP_INPUT_METHOD, MAKEWPARAM(language, mode),
                (GetKeyboardLayoutList(0, nullptr) & 0xFFFF) |
                    kInputMethodAsked |
                    ((LPARAM)(job.id & kInputMethodIdMask)
                     << kInputMethodIdShift));
}

DWORD WINAPI InputMethodThreadProc(LPVOID parameter) {
    const auto& job = *(InputMethodJob*)parameter;
    const HANDLE waits[] = {job.stop, job.ask};
    while (WaitForMultipleObjects(2, waits, FALSE, INFINITE) ==
           WAIT_OBJECT_0 + 1) {
        AskInputMethod(job);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// USB drives (an option): Windows tells the windows when a drive comes or
// goes (WM_DEVICECHANGE). The drives are listed on a thread (a new drive
// can take a moment to answer): a notification of the island's own shows a
// new one (a click opens it), and while there are some, an icon on the pill
// lists them, to open one or eject it (as Windows' "Safely Remove Hardware"
// does, with the documented CM_Request_Device_Eject).

struct RemovableDrive {
    WCHAR letter;
    std::wstring label;
};
SRWLOCK g_drivesLock = SRWLOCK_INIT;
std::vector<RemovableDrive> g_drives;
// Tells the island the drives changed (see g_drives).
constexpr UINT WM_APP_DRIVES = WM_APP + 76;
// WM_APP_DRIVES's wParam when it comes from DrivesThreadProc (it's ending).
constexpr WPARAM kDrivesListed = 1;
// Tells the drives' list an eject is done (wParam: the letter; lParam: 1 if
// it can be removed, 0 if something still uses it).
constexpr UINT WM_APP_EJECTED = WM_APP + 77;

// The drives listed again; the ones in `arrived` (bits from A) also get a
// notification.
struct DriveJob {
    HWND notify;
    DWORD arrived;
    // In the answer's lParam (an older answer still queued isn't taken for
    // the current thread's).
    UINT id;
};

// A drive on the USB bus (an external disk shows as a fixed one).
bool IsUsbDrive(WCHAR letter) {
    WCHAR path[] = L"\\\\.\\?:";
    path[4] = letter;
    HANDLE device = CreateFile(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr, OPEN_EXISTING, 0, nullptr);
    if (device == INVALID_HANDLE_VALUE) {
        return false;
    }
    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    STORAGE_DEVICE_DESCRIPTOR descriptor{};
    DWORD size = 0;
    const bool usb =
        DeviceIoControl(device, IOCTL_STORAGE_QUERY_PROPERTY, &query,
                        sizeof(query), &descriptor, sizeof(descriptor), &size,
                        nullptr) &&
        descriptor.BusType == BusTypeUsb;
    CloseHandle(device);
    return usb;
}

// A drive's name as Explorer shows it: its label, or "USB drive", with its
// letter.
std::wstring DriveName(const RemovableDrive& drive) {
    return (drive.label.empty() ? std::wstring(Tr(L"USB drive", L"Unidade USB"))
                                : drive.label) +
           L" (" + drive.letter + L":)";
}

DWORD WINAPI DrivesThreadProc(LPVOID parameter) {
    std::unique_ptr<DriveJob> job((DriveJob*)parameter);
    std::vector<RemovableDrive> drives;
    std::vector<IslandToast> toasts;
    const DWORD present = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (!(present & (1u << i))) {
            continue;
        }
        const WCHAR letter = (WCHAR)(L'A' + i);
        const std::wstring root = std::wstring(1, letter) + L":\\";
        const UINT type = GetDriveType(root.c_str());
        WCHAR label[MAX_PATH + 1] = L"";
        // (A card reader without a card answers nothing: not listed.)
        if ((type != DRIVE_REMOVABLE &&
             !(type == DRIVE_FIXED && IsUsbDrive(letter))) ||
            !GetVolumeInformation(root.c_str(), label, ARRAYSIZE(label),
                                  nullptr, nullptr, nullptr, nullptr, 0)) {
            continue;
        }
        RemovableDrive drive{letter, label};
        if (job->arrived & (1u << i)) {
            IslandToast toast;
            toast.title = Tr(L"USB drive plugged in", L"Pendrive conectado");
            toast.text = DriveName(drive);
            toast.openPath = root;
            toasts.push_back(std::move(toast));
        }
        drives.push_back(std::move(drive));
    }
    AcquireSRWLockExclusive(&g_drivesLock);
    g_drives = std::move(drives);
    ReleaseSRWLockExclusive(&g_drivesLock);
    PostMessage(job->notify, WM_APP_DRIVES, kDrivesListed, job->id);
    if (!toasts.empty()) {
        AcquireSRWLockExclusive(&g_islandToastsLock);
        g_islandToasts.insert(g_islandToasts.end(),
                              std::make_move_iterator(toasts.begin()),
                              std::make_move_iterator(toasts.end()));
        ReleaseSRWLockExclusive(&g_islandToastsLock);
        PostMessage(job->notify, WM_APP_TOAST, 0, 0);
    }
    return 0;
}

// The disks' device interface (GUID_DEVINTERFACE_DISK).
constexpr GUID kGUID_DEVINTERFACE_DISK = {
    0x53f56307,
    0xb6bf,
    0x11d0,
    {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

// A drive's disk number (and type), or false.
bool DiskNumber(PCWSTR path, STORAGE_DEVICE_NUMBER* number) {
    HANDLE device = CreateFile(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr, OPEN_EXISTING, 0, nullptr);
    if (device == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD size = 0;
    const bool read =
        DeviceIoControl(device, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0,
                        number, sizeof(*number), &size, nullptr) != FALSE;
    CloseHandle(device);
    return read;
}

struct EjectJob {
    HWND notify;
    WCHAR letter;
};

// A card taken out of its reader: the volume locked (nothing open on it),
// dismounted, and its media ejected.
bool EjectMedia(PCWSTR volume) {
    HANDLE handle = CreateFile(volume, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                               OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD bytes = 0;
    PREVENT_MEDIA_REMOVAL allow{FALSE};
    const bool ejected =
        DeviceIoControl(handle, FSCTL_LOCK_VOLUME, nullptr, 0, nullptr, 0,
                        &bytes, nullptr) &&
        DeviceIoControl(handle, FSCTL_DISMOUNT_VOLUME, nullptr, 0, nullptr, 0,
                        &bytes, nullptr) &&
        DeviceIoControl(handle, IOCTL_STORAGE_MEDIA_REMOVAL, &allow,
                        sizeof(allow), nullptr, 0, &bytes, nullptr) &&
        DeviceIoControl(handle, IOCTL_STORAGE_EJECT_MEDIA, nullptr, 0, nullptr,
                        0, &bytes, nullptr);
    CloseHandle(handle);
    return ejected;
}

// A device's property as text (its first string, for a list).
std::wstring DevNodeText(DEVINST node, ULONG property) {
    WCHAR value[512] = L"";
    ULONG size = sizeof(value) - sizeof(WCHAR);
    if (CM_Get_DevNode_Registry_Property(node, property, nullptr, value, &size,
                                         0) != CR_SUCCESS) {
        return {};
    }
    return value;
}

// A USB hub (everything plugged into it would go with it): its compatible
// IDs name the hub class.
bool IsUsbHub(DEVINST node) {
    WCHAR ids[1024] = L"";
    ULONG size = sizeof(ids) - 2 * sizeof(WCHAR);
    if (CM_Get_DevNode_Registry_Property(node, CM_DRP_COMPATIBLEIDS, nullptr,
                                         ids, &size, 0) != CR_SUCCESS) {
        return false;
    }
    for (PCWSTR id = ids; *id; id += wcslen(id) + 1) {
        if (_wcsnicmp(id, L"USB\\Class_09", 12) == 0) {
            return true;
        }
    }
    return false;
}

// The drive's disk is found among the disks (by its number), and the device
// it's on asked to be ejected (a program with a file open on it says no).
DWORD WINAPI EjectThreadProc(LPVOID parameter) {
    std::unique_ptr<EjectJob> job((EjectJob*)parameter);
    WCHAR volume[] = L"\\\\.\\?:";
    volume[4] = job->letter;
    STORAGE_DEVICE_NUMBER wanted{};
    bool ejected = false;
    HDEVINFO disks = INVALID_HANDLE_VALUE;
    if (DiskNumber(volume, &wanted)) {
        disks = SetupDiGetClassDevs(&kGUID_DEVINTERFACE_DISK, nullptr, nullptr,
                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    }
    DEVINST disk = 0;
    if (disks != INVALID_HANDLE_VALUE) {
        SP_DEVICE_INTERFACE_DATA item{sizeof(item)};
        for (DWORD i = 0; !disk && SetupDiEnumDeviceInterfaces(
                                       disks, nullptr, &kGUID_DEVINTERFACE_DISK,
                                       i, &item);
             i++) {
            DWORD size = 0;
            SetupDiGetDeviceInterfaceDetail(disks, &item, nullptr, 0, &size,
                                            nullptr);
            if (size < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA)) {
                continue;
            }
            std::vector<BYTE> buffer(size);
            auto* detail = (SP_DEVICE_INTERFACE_DETAIL_DATA*)buffer.data();
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            SP_DEVINFO_DATA info{sizeof(info)};
            STORAGE_DEVICE_NUMBER number{};
            if (SetupDiGetDeviceInterfaceDetail(disks, &item, detail, size,
                                                nullptr, &info) &&
                DiskNumber(detail->DevicePath, &number) &&
                number.DeviceType == wanted.DeviceType &&
                number.DeviceNumber == wanted.DeviceNumber) {
                disk = info.DevInst;
            }
        }
        SetupDiDestroyDeviceInfoList(disks);
    }
    // The device Windows' "Safely remove" ejects: the first one up from the
    // disk that is removable (a USB drive), never as far as a hub. A card
    // in a reader has none (the reader stays): only its card is ejected,
    // like Explorer's "Eject" does. A USB disk with fixed media on a port
    // Windows doesn't call removable: its USB device, as before.
    DEVINST device = 0;
    for (DEVINST node = disk; node && !device && !IsUsbHub(node);) {
        ULONG capabilities = 0;
        ULONG size = sizeof(capabilities);
        if (CM_Get_DevNode_Registry_Property(node, CM_DRP_CAPABILITIES,
                                             nullptr, &capabilities, &size,
                                             0) == CR_SUCCESS &&
            (capabilities & CM_DEVCAP_REMOVABLE)) {
            device = node;
        } else if (CM_Get_Parent(&node, node, 0) != CR_SUCCESS) {
            node = 0;
        }
    }
    const WCHAR root[] = {job->letter, L':', L'\\', L'\0'};
    DEVINST parent = 0;
    if (!device && disk && GetDriveType(root) == DRIVE_FIXED &&
        CM_Get_Parent(&parent, disk, 0) == CR_SUCCESS && !IsUsbHub(parent) &&
        _wcsicmp(DevNodeText(parent, CM_DRP_ENUMERATOR_NAME).c_str(), L"USB") ==
            0) {
        device = parent;
    }
    // (Windows can say no for a moment while it lets go of the drive.)
    for (int attempt = 0; attempt < 3 && !ejected && (device || disk);
         attempt++) {
        if (device) {
            PNP_VETO_TYPE veto = PNP_VetoTypeUnknown;
            WCHAR vetoName[MAX_PATH] = L"";
            ejected = CM_Request_Device_Eject(device, &veto, vetoName,
                                              ARRAYSIZE(vetoName),
                                              0) == CR_SUCCESS &&
                      veto == PNP_VetoTypeUnknown;
        } else {
            ejected = EjectMedia(volume);
        }
        if (!ejected) {
            Sleep(300);
        }
    }
    PostMessage(job->notify, WM_APP_EJECTED, job->letter, ejected);
    return 0;
}

// The list opened from the drives' icon: each drive with "Open" and
// "Eject" (and how ejecting went).
constexpr float kDrivesWidth = 330;

