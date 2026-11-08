// The toasts newer than `lastId` (or only the one with the ID `onlyId`):
// the AppUserModelIDs of their apps, and the toasts themselves; moves
// `lastId` past them. Web pictures are downloaded only with `pictures`.
// Returns false if the database can't be read.
bool ReadNewToasts(Sqlite& sqlite,
                   const std::string& path,
                   long long& lastId,
                   std::vector<std::wstring>& apps,
                   std::vector<IslandToast>& toasts,
                   long long onlyId = 0,
                   bool pictures = true) {
    void* db = nullptr;
    if (sqlite.open_v2(path.c_str(), &db, Sqlite::kOpenReadOnly, nullptr) !=
        0) {
        if (db) {
            sqlite.close(db);
        }
        return false;
    }
    sqlite.busy_timeout(db, 500);
    void* statement = nullptr;
    const bool ok =
        sqlite.prepare_v2(db,
                          "SELECT n.Id, h.PrimaryId, n.Payload, "
                          "n.ArrivalTime FROM Notification n JOIN "
                          "NotificationHandler h ON h.RecordId = n.HandlerId "
                          "WHERE n.Id > ?1 AND (?2 = 0 OR n.Id = ?2) AND "
                          "n.Type = 'toast' ORDER BY n.Id",
                          -1, &statement, nullptr) == 0;
    if (ok) {
        sqlite.bind_int64(statement, 1, lastId);
        sqlite.bind_int64(statement, 2, onlyId);
        while (sqlite.step(statement) == Sqlite::kRow) {
            lastId = std::max(lastId, sqlite.column_int64(statement, 0));
            auto app = (PCWSTR)sqlite.column_text16(statement, 1);
            if (!app) {
                continue;
            }
            if (std::find(apps.begin(), apps.end(), app) == apps.end()) {
                apps.push_back(app);
            }
            IslandToast toast{app};
            toast.id = sqlite.column_int64(statement, 0);
            toast.arrival = sqlite.column_int64(statement, 3);
            const auto payload = (const char*)sqlite.column_blob(statement, 2);
            const auto texts =
                payload ? ToastTexts(payload, sqlite.column_bytes(statement, 2))
                        : std::vector<std::wstring>{};
            SplitToastTexts(texts, &toast.title, &toast.text, &toast.fullText);
            if (payload) {
                toast.reply = ParseToastReply(
                    app, payload, sqlite.column_bytes(statement, 2));
                ToastImage(app, payload, sqlite.column_bytes(statement, 2),
                           &toast.image, &toast.imageCircle);
                toast.sound =
                    ToastSound(payload, sqlite.column_bytes(statement, 2));
                toast.loop = ToastSoundLoops(payload,
                                             sqlite.column_bytes(statement, 2));
                toast.urgent =
                    IsUrgentToast(payload, sqlite.column_bytes(statement, 2));
                if (toast.image.starts_with(L"https://") &&
                    (!pictures ||
                     !DownloadPicture(toast.image, &toast.imageData))) {
                    toast.image.clear();
                }
            }
            if (!toast.title.empty() || !toast.text.empty()) {
                toasts.push_back(std::move(toast));
            }
        }
        sqlite.finalize(statement);
    }
    sqlite.close(db);
    return ok;
}

// The newest notification's ID, to start after it.
bool ReadLastToastId(Sqlite& sqlite, const std::string& path, long long& id) {
    void* db = nullptr;
    if (sqlite.open_v2(path.c_str(), &db, Sqlite::kOpenReadOnly, nullptr) !=
        0) {
        if (db) {
            sqlite.close(db);
        }
        return false;
    }
    sqlite.busy_timeout(db, 500);
    void* statement = nullptr;
    bool ok = sqlite.prepare_v2(db, "SELECT MAX(Id) FROM Notification", -1,
                                &statement, nullptr) == 0;
    if (ok) {
        id = sqlite.step(statement) == Sqlite::kRow
                 ? sqlite.column_int64(statement, 0)
                 : 0;
        sqlite.finalize(statement);
    }
    sqlite.close(db);
    return ok;
}

// On the taskbar thread: the apps' buttons count it as news.
void WINAPI NotifyToastApps(void* parameter) {
    const auto& apps = *(const std::vector<std::wstring>*)parameter;
    if (g_unloading) {
        return;
    }
    for (const auto& app : apps) {
        for (auto& icon : g_dockIcons) {
            auto button = icon.button.get();
            if (button && _wcsicmp(GetButtonAppId(button).c_str(),
                                   app.c_str()) == 0) {
                OnAppNotified(button);
                break;
            }
        }
    }
}

// The notification database's path, in UTF-8 for SQLite.
std::string GetNotificationDatabasePath() {
    WCHAR widePath[MAX_PATH];
    std::string path;
    if (ExpandEnvironmentStrings(
            L"%LOCALAPPDATA%\\Microsoft\\Windows\\Notifications\\"
            L"wpndatabase.db",
            widePath, ARRAYSIZE(widePath))) {
        const int size = WideCharToMultiByte(CP_UTF8, 0, widePath, -1,
                                             nullptr, 0, nullptr, nullptr);
        if (size > 0) {
            path.resize(size);
            WideCharToMultiByte(CP_UTF8, 0, widePath, -1, path.data(), size,
                                nullptr, nullptr);
            path.resize(size - 1);
        }
    }
    return path;
}

// One notification still in the database (by its ID), without its web
// picture.
bool ReadToastById(long long id, IslandToast* toast) {
    Sqlite sqlite;
    const std::string path = GetNotificationDatabasePath();
    if (id <= 0 || path.empty() || !sqlite.Load()) {
        return false;
    }
    long long lastId = id - 1;
    std::vector<std::wstring> apps;
    std::vector<IslandToast> toasts;
    ReadNewToasts(sqlite, path, lastId, apps, toasts, id, false);
    sqlite.Unload();
    if (toasts.empty()) {
        return false;
    }
    *toast = std::move(toasts[0]);
    return true;
}

// New notifications are being read (the pill can show them).
std::atomic<bool> g_toastWatchReady;

DWORD WINAPI ToastThreadProc(LPVOID) {
    Sqlite sqlite;
    const std::string path = GetNotificationDatabasePath();
    long long lastId = 0;
    if (path.empty() || !sqlite.Load() ||
        !ReadLastToastId(sqlite, path, lastId)) {
        IslandLog(L"The notification database can't be read");
        sqlite.Unload();
        return 0;
    }
    IslandLog(L"Watching new notifications after %lld", lastId);
    g_toastWatchReady = true;
    // The apps' notification activators in the Start menu, before the first
    // click on a notification needs them.
    StartToastShortcutsRead();
    // The sender of the apps with new notifications (see kNotifierClassName).
    WNDCLASS notifierClass{};
    notifierClass.lpfnWndProc = DefWindowProc;
    notifierClass.hInstance = g_module;
    notifierClass.lpszClassName = kNotifierClassName;
    RegisterClass(&notifierClass);
    HWND notifier = CreateWindowEx(0, kNotifierClassName, nullptr, 0, 0, 0, 0,
                                   0, HWND_MESSAGE, nullptr, g_module, nullptr);

    const HANDLE events[] = {g_toastStopEvent, g_toastChangedEvent};
    while (WaitForMultipleObjects(ARRAYSIZE(events), events, FALSE,
                                  INFINITE) == WAIT_OBJECT_0 + 1) {
        // Written right after the state changes, or a little later (some
        // apps' notifications, like WhatsApp's, land after a moment):
        // looked for a few times, until one is there.
        std::vector<std::wstring> apps;
        std::vector<IslandToast> toasts;
        bool stopping = false;
        for (DWORD wait : {150, 350, 700, 1500}) {
            if (WaitForSingleObject(g_toastStopEvent, wait) == WAIT_OBJECT_0) {
                stopping = true;
                break;
            }
            // (Pictures only for an island to show them.)
            if (ReadNewToasts(sqlite, path, lastId, apps, toasts, 0,
                              g_islandWnd != nullptr) &&
                !apps.empty()) {
                break;
            }
        }
        if (stopping) {
            break;
        }
        // Some may have gone too (read elsewhere, or a call that ended).
        if (HWND island = g_islandWnd) {
            PostMessage(island, WM_APP_TOASTS_CHANGED, 0, 0);
        }
        if (apps.empty()) {
            continue;
        }
        // The island shows the latest one, and keeps them all as unread
        // (only while it runs: without it, nothing is kept; the latest few
        // while it's starting or busy).
        if (HWND island = g_islandWnd; island && !toasts.empty()) {
            // (Without their downloaded pictures.)
            constexpr size_t kMaxKeptToasts = 100;
            AcquireSRWLockExclusive(&g_keptToastsLock);
            for (const auto& toast : toasts) {
                g_keptToasts.push_back({toast, NowSeconds()});
                g_keptToasts.back().toast.imageData.clear();
            }
            if (g_keptToasts.size() > kMaxKeptToasts) {
                g_keptToasts.erase(g_keptToasts.begin(),
                                   g_keptToasts.end() - kMaxKeptToasts);
            }
            ReleaseSRWLockExclusive(&g_keptToastsLock);
            constexpr size_t kMaxQueuedToasts = 50;
            AcquireSRWLockExclusive(&g_islandToastsLock);
            g_islandToasts.insert(g_islandToasts.end(),
                                  std::make_move_iterator(toasts.begin()),
                                  std::make_move_iterator(toasts.end()));
            if (g_islandToasts.size() > kMaxQueuedToasts) {
                g_islandToasts.erase(g_islandToasts.begin(),
                                     g_islandToasts.end() - kMaxQueuedToasts);
            }
            ReleaseSRWLockExclusive(&g_islandToastsLock);
            PostMessage(island, WM_APP_TOAST, 0, 0);
        }
        // Explorer makes their icons bounce.
        SendLines(FindTaskbarWnd(), notifier, kIpcNotifiedApps, apps);
    }
    if (notifier) {
        DestroyWindow(notifier);
    }
    UnregisterClass(kNotifierClassName, g_module);
    sqlite.Unload();
    return 0;
}

void StartToastWatch() {
    if (g_toastThread) {
        return;
    }
    HMODULE ntdll = GetModuleHandle(L"ntdll.dll");
    using Subscribe_t = LONG(NTAPI*)(void**, ULONGLONG, ULONG, void*, void*,
                                     const void*, ULONG, ULONG);
    using Query_t = LONG(NTAPI*)(const ULONGLONG*, const void*, const void*,
                                 ULONG*, void*, ULONG*);
    auto subscribe = (Subscribe_t)GetProcAddress(
        ntdll, "RtlSubscribeWnfStateChangeNotification");
    auto query = (Query_t)GetProcAddress(ntdll, "NtQueryWnfStateData");
    if (!subscribe || !query) {
        return;
    }
    g_toastStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    g_toastChangedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!g_toastStopEvent || !g_toastChangedEvent) {
        StopToastWatch();
        return;
    }
    g_toastThread =
        CreateThread(nullptr, 0, ToastThreadProc, nullptr, 0, nullptr);
    if (!g_toastThread) {
        StopToastWatch();
        return;
    }
    // Told of changes after the current one only.
    ULONG changeStamp = 0;
    BYTE buffer[64];
    ULONG size = sizeof(buffer);
    query(&kWnfNotificationsChanged, nullptr, nullptr, &changeStamp, buffer,
          &size);
    if (subscribe(&g_toastSubscription, kWnfNotificationsChanged, changeStamp,
                  (void*)ToastStateCallback, nullptr, nullptr, 0, 0) < 0) {
        g_toastSubscription = nullptr;
        IslandLog(L"Can't watch new notifications");
        StopToastWatch();
    }
}

void StopToastWatch() {
    g_toastWatchReady = false;
    if (g_toastSubscription) {
        using Unsubscribe_t = LONG(NTAPI*)(void*);
        if (auto unsubscribe = (Unsubscribe_t)GetProcAddress(
                GetModuleHandle(L"ntdll.dll"),
                "RtlUnsubscribeWnfStateChangeNotification")) {
            unsubscribe(g_toastSubscription);
        }
        g_toastSubscription = nullptr;
    }
    if (g_toastThread) {
        SetEvent(g_toastStopEvent);
        WaitForSingleObject(g_toastThread, INFINITE);
        CloseHandle(g_toastThread);
        g_toastThread = nullptr;
    }
    // (The shortcuts' reader it may have started.)
    WaitForToastShortcutsRead();
    for (HANDLE* event : {&g_toastStopEvent, &g_toastChangedEvent}) {
        if (*event) {
            CloseHandle(*event);
            *event = nullptr;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// The attention icon (its own thread).
//
// When an app asks for attention while the dock is hidden, only its icon peeks
// out from the bottom edge, bounces, and goes away again. The dock's buttons
// are off screen then, so the icon is drawn in a small window of its own.

HANDLE g_attentionThread;
DWORD g_attentionThreadId;
std::atomic<bool> g_attentionClicked;

constexpr UINT WM_APP_ATTENTION = WM_APP + 30;  // lParam: the app's window.
constexpr WCHAR kAttentionClassName[] = L"WindhawkMacDockAttentionIcon";

// Sizes in pixels at 100% scaling: the icon, how high it bounces, and how far
// above the bottom edge it rests.
constexpr int kAttentionIconSize = 48;
constexpr int kAttentionBounceHeight = 22;
constexpr int kAttentionRestGap = 10;
// How long it takes to come out and to go back, in seconds.
constexpr double kAttentionRiseTime = 0.28;
constexpr double kAttentionSinkTime = 0.24;
// The same app doesn't bounce again this soon after, in seconds.
constexpr double kAttentionRepeatDelay = 4;

constexpr GUID kFOLDERID_AppsFolder = {
    0x1e87508d,
    0x89c2,
    0x42f0,
    {0x8a, 0x7e, 0x64, 0x5a, 0x0f, 0x50, 0xca, 0x58}};
constexpr GUID kFOLDERID_LocalAppData = {
    0xf1b32785,
    0x6fba,
    0x4fcf,
    {0x9d, 0x55, 0x7b, 0x8e, 0x7f, 0x15, 0x70, 0x91}};
// The shell's handler giving an item's data, for dragging it (BHID_DataObject).
constexpr GUID kBHID_DataObject = {
    0xb8c0bd9f,
    0xed24,
    0x455c,
    {0x83, 0xe6, 0xd5, 0x39, 0x0c, 0x4f, 0xe8, 0xc4}};
// The shell's handler of an item's own objects (BHID_SFUIObject): a folder's
// drop target, the one Explorer uses for files dropped on it.
constexpr GUID kBHID_SFUIObject = {
    0x3981e225,
    0xf559,
    0x11d3,
    {0x8e, 0x3a, 0x00, 0xc0, 0x4f, 0x68, 0x37, 0xd5}};
// Windows' picture of the files dragged, kept over a window they're dragged
// over (CLSID_DragDropHelper).
constexpr GUID kCLSID_DragDropHelper = {
    0x4657278a,
    0x411b,
    0x11d2,
    {0x83, 0x9a, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0xd0}};
constexpr PROPERTYKEY kPKEY_AppUserModel_ID = {
    {0x9f4c2855,
     0x9f79,
     0x4b39,
     {0xa8, 0xd0, 0xe1, 0xd4, 0x2d, 0xe1, 0xd5, 0xf3}},
    5};

// What RunAttentionIcon shows: the app's window (a flashing button), or its
// AppUserModelID (a badge, see OnAppNotified).
struct AttentionRequest {
    HWND app;
    std::wstring appId;
};

void PostAttentionRequest(HWND app, std::wstring appId) {
    if (!g_attentionThreadId) {
        return;
    }
    auto* request = new AttentionRequest{app, std::move(appId)};
    if (!PostThreadMessage(g_attentionThreadId, WM_APP_ATTENTION, 0,
                           (LPARAM)request)) {
        delete request;
    }
}

void OnAppFlashed(HWND hWnd) {
    if (IsDock() && g_settings.attentionMode == AttentionMode::Icon) {
        PostAttentionRequest(hWnd, {});
    }
}

std::wstring GetWindowAppId(HWND hWnd) {
    std::wstring appId;
    winrt::com_ptr<IPropertyStore> store;
    if (hWnd && SUCCEEDED(SHGetPropertyStoreForWindow(
                    hWnd, IID_PPV_ARGS(store.put())))) {
        PROPVARIANT value;
        PropVariantInit(&value);
        if (SUCCEEDED(store->GetValue(kPKEY_AppUserModel_ID, &value)) &&
            value.vt == VT_LPWSTR && value.pwszVal) {
            appId = value.pwszVal;
        }
        PropVariantClear(&value);
    }
    // Packaged apps (installed from the Store, like WhatsApp) have theirs on
    // their process, not on their windows.
    if (appId.empty() && hWnd) {
        DWORD processId = 0;
        GetWindowThreadProcessId(hWnd, &processId);
        if (HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                         FALSE, processId)) {
            // An AppUserModelID has at most 130 characters.
            WCHAR id[256] = L"";
            UINT32 length = ARRAYSIZE(id);
            if (GetApplicationUserModelId(process, &length, id) ==
                ERROR_SUCCESS) {
                appId = id;
            }
            CloseHandle(process);
        }
    }
    return appId;
}

// A text a window gives about its app for the taskbar
// (PKEY_AppUserModel_*: 2 the command that starts it again, 4 its name), or
// empty.
std::wstring GetWindowAppText(HWND hWnd, DWORD id) {
    std::wstring text;
    winrt::com_ptr<IPropertyStore> store;
    if (hWnd && SUCCEEDED(SHGetPropertyStoreForWindow(
                    hWnd, IID_PPV_ARGS(store.put())))) {
        PROPERTYKEY key = kPKEY_AppUserModel_ID;
        key.pid = id;
        PROPVARIANT value;
        PropVariantInit(&value);
        if (SUCCEEDED(store->GetValue(key, &value)) && value.vt == VT_LPWSTR &&
            value.pwszVal) {
            text = value.pwszVal;
        }
        PropVariantClear(&value);
    }
    return text;
}

// An app to remember things for (its place): its ID, or its program's path
// when it has none.
std::wstring WindowAppKey(HWND hWnd) {
    std::wstring key = GetWindowAppId(hWnd);
    if (!key.empty() || !hWnd) {
        return key;
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(hWnd, &processId);
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

// CLSID_StartMenuCacheAndAppResolver and IApplicationResolver (Windows 8
// and later), as the taskbar uses them: the AppUserModelID it gives a window
// or a shortcut.
constexpr GUID kCLSID_AppResolver = {
    0x660b90c8,
    0x73a9,
    0x4b58,
    {0x8c, 0xae, 0x35, 0x5b, 0x7f, 0x55, 0x34, 0x1b}};
constexpr GUID kIID_AppResolver8 = {
    0xde25675a,
    0x72de,
    0x44b4,
    {0x93, 0x73, 0x05, 0x17, 0x04, 0x50, 0xc1, 0x40}};
struct IAppResolver8 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetAppIDForShortcut(IShellItem* item,
                                                          PWSTR* appId) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAppIDForShortcutObject() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAppIDForWindow(HWND hWnd,
                                                        PWSTR* appId,
                                                        void* unknown1,
                                                        void* unknown2,
                                                        void* unknown3) = 0;
};

// The AppUserModelID the taskbar gives a window's button (its automation ID,
// see GetButtonAppId), as the shell's app resolver works it out: also for an
// app that sets it on its process rather than on its windows (like
// Discord). Its window's own ID if the resolver can't be had.
std::wstring TaskbarAppIdOfWindow(HWND hWnd) {
    std::wstring appId;
    winrt::com_ptr<IAppResolver8> resolver;
    if (SUCCEEDED(CoCreateInstance(kCLSID_AppResolver, nullptr,
                                   CLSCTX_INPROC_SERVER, kIID_AppResolver8,
                                   resolver.put_void()))) {
        PWSTR id = nullptr;
        if (SUCCEEDED(resolver->GetAppIDForWindow(hWnd, &id, nullptr, nullptr,
                                                  nullptr)) &&
            id) {
            appId = id;
        }
        CoTaskMemFree(id);
    }
    return appId.empty() ? GetWindowAppId(hWnd) : appId;
}

// An app's overlay icon set or cleared (see OverlayIcon): its picture copied
// (the taskbar keeps its own). Not while unloading: the copies are freed then
// (see FreeXamlGlobals), and the hooks stay until the unload ends.
void KeepOverlayIcon(HWND hWnd, HICON icon) {
    if (g_unloading) {
        return;
    }
    HICON copy = icon ? CopyIcon(icon) : nullptr;
    int size = 0;
    ICONINFO info{};
    if (copy && GetIconInfo(copy, &info)) {
        BITMAP bitmap{};
        if (GetObject(info.hbmColor ? info.hbmColor : info.hbmMask,
                      sizeof(bitmap), &bitmap)) {
            size = bitmap.bmWidth;
        }
        if (info.hbmColor) {
            DeleteObject(info.hbmColor);
        }
        if (info.hbmMask) {
            DeleteObject(info.hbmMask);
        }
    }
    const std::wstring appId =
        copy ? TaskbarAppIdOfWindow(hWnd) : std::wstring();
    AcquireSRWLockExclusive(&g_overlayIconsLock);
    auto it = std::find_if(
        g_overlayIcons.begin(), g_overlayIcons.end(),
        [&](const OverlayIcon& overlay) { return overlay.wnd == hWnd; });
    if (it != g_overlayIcons.end()) {
        DestroyIcon(it->icon);
        it->icon = nullptr;
    }
    if (!copy) {
        if (it != g_overlayIcons.end()) {
            g_overlayIcons.erase(it);
        }
    } else {
        if (it == g_overlayIcons.end()) {
            g_overlayIcons.push_back({hWnd});
            it = g_overlayIcons.end() - 1;
        }
        it->appId = appId;
        it->icon = copy;
        it->size = size;
        it->serial = ++g_overlaySerial;
    }
    ReleaseSRWLockExclusive(&g_overlayIconsLock);
}

// The apps' progress on their taskbar buttons (ITaskbarList3::
// SetProgressState and SetProgressValue: downloads, copies, installs), for
// the island's pill filling like a loading bar (islandDownloadFill), read
// from the bars the taskbar draws under the buttons (ProgressIndicator):
// their value as shown (whatever scale the app used) and their state by
// their look (see ProgressBarState), the one furthest behind told to the
// island (see PublishProgress). The taskbar's calls for a window's progress
// (CTaskBand::_OnSetProgressState and _OnSetProgressValue) say when to look
// again, and their state stands in when no bar is found at all: their value
// isn't on the bars' scale (it stayed 0 through a whole download). Taskbar
// thread only.
struct ProgressBarWatch {
    winrt::weak_ref<Controls::Primitives::RangeBase> bar;
    Controls::Primitives::RangeBase::ValueChanged_revoker valueChanged;
    VisualStateGroup::CurrentStateChanged_revoker stateChanged;
    int64_t visibilityToken = 0;
};
[[clang::no_destroy]] std::vector<ProgressBarWatch> g_progressBars;
// Each window's state as the taskbar was told (see above).
std::vector<std::pair<HWND, int>> g_windowProgress;
// When the progress the bars show now started (0 none): it's told only
// once it has lasted kProgressSureSeconds (see PublishProgress).
double g_progressSeenAt = 0;
constexpr double kProgressSureSeconds = 5;
// Whether an island fills with the apps' progress (kIpcProgressWanted): the
// bars are watched and read only then.
bool g_progressWanted = false;
// What the island was told last (its state, and per mille).
int g_progressSentState = -1;
int g_progressSentValue = -1;
// What it is, told with the state (its next byte): only Windows' file
// copies (its copy dialog's windows), or anything else (a download, an
// install).
constexpr int kProgressKindCopy = 1;
int g_progressSentKind = -1;
