std::wstring TrayIconKey(const TrayIcon& icon) {
    std::wstring key;
    if (HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                     icon.processId)) {
        WCHAR path[MAX_PATH];
        DWORD length = ARRAYSIZE(path);
        if (QueryFullProcessImageName(process, 0, path, &length)) {
            key = path;
        }
        CloseHandle(process);
    }
    // (One it can't open: its picture, kept across a reload of the mod, is
    // its program's, see TakeBackTrayIcons.)
    if (key.empty()) {
        key = ProcessImagePathById(icon.processId);
    }
    for (auto& c : key) {
        c = towlower(c);
    }
    if (icon.hasGuid) {
        WCHAR guid[64];
        StringFromGUID2(icon.guid, guid, ARRAYSIZE(guid));
        key += L"|";
        key += guid;
    } else {
        key += L"|" + std::to_wstring(icon.id);
    }
    return key;
}

// On the taskbar's thread, as an app sends it (see TaskbarSubclassProc).
void SendTrayIcons();

// The island gets the tray icons a moment after they change (see
// TaskbarSubclassProc).
const UINT g_sendTrayIconsMessage =
    RegisterWindowMessage(L"WindhawkMacIslandDock_SendTrayIcons");
std::atomic<bool> g_trayIconsSendPending;

void OnTrayNotify(const COPYDATASTRUCT* copyData) {
    if (!copyData ||
        copyData->dwData != 1 ||
        copyData->cbData < offsetof(TrayNotifyData, nid) +
                               offsetof(NotifyIconData32, szTip)) {
        return;
    }
    const auto* data = (const TrayNotifyData*)copyData->lpData;
    if (data->signature != kTrayNotifySignature) {
        return;
    }
    NotifyIconData32 nid{};
    memcpy(&nid, &data->nid,
           std::min<size_t>(copyData->cbData - offsetof(TrayNotifyData, nid),
                            sizeof(nid)));
    nid.szTip[ARRAYSIZE(nid.szTip) - 1] = L'\0';
    const bool hasGuid = (nid.uFlags & NIF_GUID) &&
                         copyData->cbData >= offsetof(TrayNotifyData, nid) +
                                                 offsetof(NotifyIconData32,
                                                          hBalloonIcon);
    const HWND owner = (HWND)(LONG_PTR)(LONG)nid.hWnd;
    // Windows' own icons (volume, network, power, microphone...), which the
    // island shows its own way: {7820AE7x-23E3-4229-82C1-E41CB67D5B9C}.
    static const BYTE kSystemIconTail[] = {0x82, 0xC1, 0xE4, 0x1C,
                                           0xB6, 0x7D, 0x5B, 0x9C};
    if (hasGuid && (nid.guidItem.Data1 & 0xFFFFFFF0) == 0x7820AE70 &&
        nid.guidItem.Data2 == 0x23E3 && nid.guidItem.Data3 == 0x4229 &&
        memcmp(nid.guidItem.Data4, kSystemIconTail, 8) == 0) {
        return;
    }

    bool changed = false;
    HWND askAgain = nullptr;
    AcquireSRWLockExclusive(&g_trayIconsLock);
    auto it = std::find_if(g_trayIcons.begin(), g_trayIcons.end(),
                           [&](const TrayIcon& icon) {
                               return hasGuid ? icon.hasGuid &&
                                                    icon.guid == nid.guidItem
                                              : !icon.hasGuid &&
                                                    icon.owner == owner &&
                                                    icon.id == nid.uID;
                           });
    switch (data->message) {
        case NIM_ADD:
        case NIM_MODIFY: {
            if (it == g_trayIcons.end()) {
                // A change to an icon added before the mod saw it (an app
                // started before it, which didn't add its icons again when
                // asked: the request is a broadcast, which doesn't reach
                // message-only windows). With its picture, it's added; and
                // its window is asked once, directly, to add it again (with
                // its version, so clicks reach it the way it expects).
                if (data->message == NIM_MODIFY) {
                    if (!(nid.uFlags & NIF_ICON) || !nid.hIcon ||
                        !IsWindow(owner)) {
                        break;
                    }
                    if (std::find(g_trayOwnersAsked.begin(),
                                  g_trayOwnersAsked.end(),
                                  owner) == g_trayOwnersAsked.end()) {
                        g_trayOwnersAsked.push_back(owner);
                        askAgain = owner;
                    }
                }
                TrayIcon icon;
                icon.owner = owner;
                icon.id = nid.uID;
                icon.hasGuid = hasGuid;
                icon.guid = nid.guidItem;
                icon.version = kTrayVersionUnknown;
                GetWindowThreadProcessId(owner, &icon.processId);
                icon.key = TrayIconKey(icon);
                g_trayIcons.push_back(std::move(icon));
                it = g_trayIcons.end() - 1;
            }
            // An icon known by its GUID may come without its window
            // (H.NotifyIcon, used by WhatsApp, does that): the one it came
            // with stays.
            if (owner && IsWindow(owner)) {
                it->owner = owner;
                GetWindowThreadProcessId(owner, &it->processId);
            }
            if (nid.uFlags & NIF_MESSAGE) {
                it->callback = nid.uCallbackMessage;
            }
            if ((nid.uFlags & NIF_ICON) && nid.hIcon) {
                // The app may destroy its handle right after: a copy.
                if (HICON copy = CopyIcon((HICON)(LONG_PTR)(LONG)nid.hIcon)) {
                    it->icon = std::make_shared<OwnedIcon>(copy);
                    it->serial = ++g_trayIconSerial;
                }
            }
            if (nid.uFlags & NIF_TIP) {
                it->tip = nid.szTip;
            }
            if ((nid.uFlags & NIF_STATE) && (nid.dwStateMask & NIS_HIDDEN)) {
                it->hidden = (nid.dwState & NIS_HIDDEN) != 0;
            }
            changed = true;
            break;
        }
        case NIM_DELETE:
            if (it != g_trayIcons.end()) {
                g_trayIcons.erase(it);
                changed = true;
            }
            break;
        case NIM_SETVERSION:
            if (it != g_trayIcons.end()) {
                it->version = nid.uVersion;
                changed = true;
            }
            break;
    }
    // Icons whose window is gone (the app closed without removing them).
    if (std::erase_if(g_trayIcons, [](const TrayIcon& icon) {
            return !IsWindow(icon.owner);
        })) {
        changed = true;
    }
    std::erase_if(g_trayOwnersAsked, [](HWND window) {
        return !IsWindow(window);
    });
    ReleaseSRWLockExclusive(&g_trayIconsLock);
    if (askAgain) {
        static const UINT taskbarCreated =
            RegisterWindowMessage(L"TaskbarCreated");
        PostMessage(askAgain, taskbarCreated, 0, 0);
    }
    // Sent a moment later, once for all the changes until then: the app
    // that changed its icon is waiting for the taskbar meanwhile.
    if (changed && !g_trayIconsSendPending.exchange(true)) {
        if (HWND taskbar = FindWindow(L"Shell_TrayWnd", nullptr)) {
            PostMessage(taskbar, g_sendTrayIconsMessage, 0, 0);
        } else {
            g_trayIconsSendPending = false;
        }
    }
}

// To the island, in its own process: each icon's data, and its icon handle
// (icons can be used from any process of the session; the island makes its
// own copy while handling the message, while this one is kept alive).
//
// Nothing is locked while sending: an app's tray icon (OnTrayNotify, sent to
// this thread) takes the lock, and may be handled right after.
void SendTrayIcons() {
    HWND island = FindIslandWnd();
    if (!island) {
        return;
    }
    IpcWriter writer;
    std::vector<std::shared_ptr<OwnedIcon>> keptAlive;
    AcquireSRWLockShared(&g_trayIconsLock);
    writer.Put<UINT32>((UINT32)g_trayIcons.size());
    for (const auto& icon : g_trayIcons) {
        writer.Put<UINT64>((UINT64)(ULONG_PTR)icon.owner);
        writer.Put<UINT32>(icon.id);
        writer.Put<GUID>(icon.guid);
        writer.Put<BYTE>(icon.hasGuid);
        writer.Put<UINT32>(icon.callback);
        writer.Put<UINT32>(icon.version);
        writer.Put<UINT64>(
            (UINT64)(ULONG_PTR)(icon.icon ? icon.icon->icon : nullptr));
        writer.Put<UINT32>(icon.serial);
        writer.Put<BYTE>(icon.hidden);
        writer.Put<UINT32>(icon.processId);
        writer.PutString(icon.tip);
        writer.PutString(icon.key);
        keptAlive.push_back(icon.icon);
    }
    ReleaseSRWLockShared(&g_trayIconsLock);
    SendIpc(island, OwnTaskbarWnd(), kIpcTrayIcons,
            writer.bytes.data(), (DWORD)writer.bytes.size());
}

// In the island's process: the icons explorer sent, each with the island's
// own copy of its icon (kept while it doesn't change).
void ReceiveTrayIcons(const COPYDATASTRUCT* copyData) {
    IpcReader reader(copyData);
    UINT32 count = 0;
    if (!reader.Get(&count)) {
        return;
    }
    std::vector<TrayIcon> icons;
    for (UINT32 i = 0; i < count && i < 512; i++) {
        TrayIcon icon;
        UINT64 owner = 0, handle = 0;
        BYTE hasGuid = 0, hidden = 0;
        if (!reader.Get(&owner) || !reader.Get(&icon.id) ||
            !reader.Get(&icon.guid) || !reader.Get(&hasGuid) ||
            !reader.Get(&icon.callback) || !reader.Get(&icon.version) ||
            !reader.Get(&handle) || !reader.Get(&icon.serial) ||
            !reader.Get(&hidden) || !reader.Get(&icon.processId) ||
            !reader.GetString(&icon.tip) || !reader.GetString(&icon.key)) {
            return;
        }
        icon.owner = (HWND)(ULONG_PTR)owner;
        icon.hasGuid = hasGuid != 0;
        icon.hidden = hidden != 0;
        icons.push_back(std::move(icon));
        // The copy made before, if it's the same icon.
        TrayIcon& added = icons.back();
        AcquireSRWLockShared(&g_trayIconsLock);
        for (const auto& old : g_trayIcons) {
            if (old.key == added.key && old.serial == added.serial) {
                added.icon = old.icon;
            }
        }
        ReleaseSRWLockShared(&g_trayIconsLock);
        if (!added.icon && handle) {
            if (HICON copy = CopyIcon((HICON)(ULONG_PTR)handle)) {
                added.icon = std::make_shared<OwnedIcon>(copy);
            }
        }
    }
    AcquireSRWLockExclusive(&g_trayIconsLock);
    g_trayIcons = std::move(icons);
    g_trayIconSerial++;
    ReleaseSRWLockExclusive(&g_trayIconsLock);
}

// Where each icon is on the island (or its arrow, when it's in the closed
// list), on the screen, as the island last drew them.
struct TrayIconPlace {
    HWND owner;
    UINT id;
    GUID guid;
    bool hasGuid;
    RECT rect;
};
SRWLOCK g_trayPlacesLock = SRWLOCK_INIT;
std::vector<TrayIconPlace> g_trayPlaces;

// An app asking where its icon is (Shell_NotifyIconGetRect): the request
// that comes to the taskbar window. Returns the answer, or 0 to leave it to
// the taskbar.
#pragma pack(push, 4)
struct NotifyIconIdentifier32 {
    DWORD signature;
    // 1: the top left corner, 2: the size.
    DWORD request;
    DWORD cbSize;
    DWORD hWndHigh;
    DWORD hWndLow;
    UINT uID;
    GUID guidItem;
};
#pragma pack(pop)

LRESULT OnTrayIconRectRequest(const COPYDATASTRUCT* copyData) {
    if (!g_settings.showIsland || !copyData ||
        copyData->dwData != 3 ||
        copyData->cbData < sizeof(NotifyIconIdentifier32)) {
        return 0;
    }
    const auto* request = (const NotifyIconIdentifier32*)copyData->lpData;
    if (request->signature != kTrayNotifySignature ||
        (request->request != 1 && request->request != 2)) {
        return 0;
    }
    static const GUID kNoGuid{};
    const bool byGuid = request->guidItem != kNoGuid;
    LRESULT result = 0;
    AcquireSRWLockShared(&g_trayPlacesLock);
    for (const auto& place : g_trayPlaces) {
        const DWORD owner = (DWORD)(ULONG_PTR)place.owner;
        const bool matches =
            byGuid ? place.hasGuid && place.guid == request->guidItem
                   : (owner == request->hWndLow || owner == request->hWndHigh) &&
                         place.id == request->uID;
        if (matches) {
            const RECT& r = place.rect;
            result = request->request == 1
                         ? MAKELONG((short)r.left, (short)r.top)
                         : MAKELONG((short)(r.right - r.left),
                                    (short)(r.bottom - r.top));
            break;
        }
    }
    ReleaseSRWLockShared(&g_trayPlacesLock);
    return result;
}

// Some apps never add their icons again when asked (Spotify ignores
// TaskbarCreated) and rarely change them, so an icon seen before the mod was
// reloaded (its settings saved, turned off and on, Windhawk updated) would be
// lost while the app runs. The list is kept in the mod's storage when the
// explorer part unloads, and taken back when it loads: the icons whose
// window is still there, in the same process. Their picture is the
// program's own icon until the app changes it.
// The kept list's room (characters): short tooltips, and the icons that
// don't fit are left out.
constexpr size_t kTrayIconsKeptRoom = 16384;
constexpr size_t kTrayIconsKeptTip = 64;

void KeepTrayIcons() {
    if (!OwnsTaskbar()) {
        return;
    }
    std::wstring value;
    AcquireSRWLockShared(&g_trayIconsLock);
    for (const auto& icon : g_trayIcons) {
        WCHAR guid[64] = L"";
        StringFromGUID2(icon.guid, guid, ARRAYSIZE(guid));
        std::wstring tip = icon.tip.substr(0, kTrayIconsKeptTip);
        std::replace_if(
            tip.begin(), tip.end(),
            [](WCHAR c) { return c == L'\n' || c == L'\r' || c == L'\t'; },
            L' ');
        const std::wstring line =
            std::to_wstring((UINT64)(ULONG_PTR)icon.owner) + L"\t" +
            std::to_wstring(icon.id) + L"\t" + guid + L"\t" +
            std::to_wstring(icon.hasGuid) + L"\t" +
            std::to_wstring(icon.callback) + L"\t" +
            std::to_wstring(icon.version) + L"\t" +
            std::to_wstring(icon.hidden) + L"\t" +
            std::to_wstring(icon.processId) + L"\t" + tip + L"\n";
        if (value.size() + line.size() >= kTrayIconsKeptRoom) {
            break;
        }
        value += line;
    }
    ReleaseSRWLockShared(&g_trayIconsLock);
    SetUserStringValue(L"trayIconsKept", value.c_str());
}

void TakeBackTrayIcons() {
    if (!OwnsTaskbar()) {
        return;
    }
    const std::wstring kept =
        GetUserStringValue(L"trayIconsKept", kTrayIconsKeptRoom);
    if (kept.empty()) {
        return;
    }
    Wh_DeleteValue(UserValueName(L"trayIconsKept").c_str());
    std::vector<TrayIcon> icons;
    std::wstring_view rest = kept;
    while (!rest.empty()) {
        const size_t end = rest.find(L'\n');
        std::wstring_view line = rest.substr(0, end);
        rest = end == std::wstring_view::npos ? std::wstring_view{}
                                              : rest.substr(end + 1);
        std::vector<std::wstring> fields;
        size_t start = 0;
        while (fields.size() < 8) {
            const size_t tab = line.find(L'\t', start);
            if (tab == std::wstring_view::npos) {
                break;
            }
            fields.emplace_back(line.substr(start, tab - start));
            start = tab + 1;
        }
        if (fields.size() < 8) {
            continue;
        }
        TrayIcon icon;
        icon.owner =
            (HWND)(ULONG_PTR)_wcstoui64(fields[0].c_str(), nullptr, 10);
        icon.id = wcstoul(fields[1].c_str(), nullptr, 10);
        icon.hasGuid = fields[3] == L"1";
        if (icon.hasGuid &&
            FAILED(CLSIDFromString(fields[2].c_str(), &icon.guid))) {
            continue;
        }
        icon.callback = wcstoul(fields[4].c_str(), nullptr, 10);
        icon.version = wcstoul(fields[5].c_str(), nullptr, 10);
        // (An older mod kept none as 0: not known then.)
        if (!icon.version) {
            icon.version = kTrayVersionUnknown;
        }
        icon.hidden = fields[6] == L"1";
        icon.processId = wcstoul(fields[7].c_str(), nullptr, 10);
        icon.tip = std::wstring(line.substr(start));
        DWORD processId = 0;
        if (!IsWindow(icon.owner) ||
            !GetWindowThreadProcessId(icon.owner, &processId) ||
            processId != icon.processId) {
            continue;
        }
        icon.key = TrayIconKey(icon);
        const std::wstring program = icon.key.substr(0, icon.key.find(L'|'));
        HICON picture = nullptr;
        if (!program.empty() &&
            ExtractIconEx(program.c_str(), 0, &picture, nullptr, 1) > 0 &&
            picture) {
            icon.icon = std::make_shared<OwnedIcon>(picture);
        }
        icons.push_back(std::move(icon));
    }
    if (icons.empty()) {
        return;
    }
    AcquireSRWLockExclusive(&g_trayIconsLock);
    for (auto& icon : icons) {
        const bool known = std::any_of(
            g_trayIcons.begin(), g_trayIcons.end(), [&](const TrayIcon& other) {
                return icon.hasGuid ? other.hasGuid && other.guid == icon.guid
                                    : !other.hasGuid &&
                                          other.owner == icon.owner &&
                                          other.id == icon.id;
            });
        if (!known) {
            icon.serial = ++g_trayIconSerial;
            g_trayIcons.push_back(std::move(icon));
        }
    }
    ReleaseSRWLockExclusive(&g_trayIconsLock);
}

// Asks the apps to add their icons again (the message Windows sends when the
// taskbar restarts), at most once while Explorer runs: the icons are kept
// from then on, for the island to ask for them (kIpcHello). Only once the
// taskbar receives them (subclassed), and only for an island that shows them.
std::atomic<bool> g_trayIconsRequested;
// Whether a taskbar is subclassed yet (see SubclassTaskbars), defined further
// down.
bool AnyTaskbarSubclassed();

void RequestTrayIcons() {
    TakeBackTrayIcons();
    if (g_trayIconsRequested || !g_settings.showIsland ||
        !g_settings.islandTray || !OwnsTaskbar() ||
        !AnyTaskbarSubclassed()) {
        return;
    }
    g_trayIconsRequested = true;
    static const UINT taskbarCreated = RegisterWindowMessage(L"TaskbarCreated");
    PostMessage(HWND_BROADCAST, taskbarCreated, 0, 0);
}

// Tells the taskbar's thread to send the island everything (see
// TaskbarSubclassProc).
const UINT g_sendToIslandMessage =
    RegisterWindowMessage(L"WindhawkMacIslandDock_SendToIsland");
const UINT g_requestTrayIconsMessage =
    RegisterWindowMessage(L"WindhawkMacIslandDock_RequestTrayIcons");

// Where the island drew the tray icons, sent by it.
void ReceiveTrayPlaces(const COPYDATASTRUCT* copyData) {
    IpcReader reader(copyData);
    UINT32 count = 0;
    if (!reader.Get(&count)) {
        return;
    }
    std::vector<TrayIconPlace> places;
    for (UINT32 i = 0; i < count && i < 512; i++) {
        UINT64 owner = 0;
        TrayIconPlace place{};
        BYTE hasGuid = 0;
        if (!reader.Get(&owner) || !reader.Get(&place.id) ||
            !reader.Get(&place.guid) || !reader.Get(&hasGuid) ||
            !reader.Get(&place.rect)) {
            return;
        }
        place.owner = (HWND)(ULONG_PTR)owner;
        place.hasGuid = hasGuid != 0;
        places.push_back(place);
    }
    AcquireSRWLockExclusive(&g_trayPlacesLock);
    g_trayPlaces = std::move(places);
    ReleaseSRWLockExclusive(&g_trayPlacesLock);
}

void WINAPI NotifyToastApps(void* parameter);

// Defined further down.
HICON AppIconForIsland(const COPYDATASTRUCT* copyData);
void WatchTaskbarPlace();
void StopWatchingTaskbarPlace();
void PublishProgress(bool force);

void WatchIslandProcess(HWND island);
void StopWatchingIslandProcess();

// When an app's window closes, Windows gives the focus back to the window
// that had it before: often the taskbar, clicked to open that app. A taskbar
// that hides itself stays up while it has the focus, so it would stay until
// the user clicked elsewhere. Then the focus goes to the next window, like
// Alt+Esc (or the desktop), and the taskbar hides as usual.
std::atomic<double> g_appClosedAt{0};

bool IsWindowToFocus(HWND hWnd) {
    if (!IsWindowVisible(hWnd) || IsIconic(hWnd) || IsShellWindow(hWnd)) {
        return false;
    }
    const LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    if ((exStyle &
         (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT)) ||
        (GetWindow(hWnd, GW_OWNER) && !(exStyle & WS_EX_APPWINDOW))) {
        return false;
    }
    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked,
                                        sizeof(cloaked))) &&
        cloaked) {
        return false;
    }
    RECT rect;
    return GetWindowRect(hWnd, &rect) && rect.right > rect.left &&
           rect.bottom > rect.top;
}

// On the taskbar's hide timer, right after an app closed. Returns whether
// the focus was moved.
// (How long after a click on it the taskbar's focus counts as the click's.)
constexpr double kFocusClickSeconds = 600;

bool HandFocusBack(HWND hWnd) {
    const double now = NowSeconds();
    if (g_unloading || !SlidesItself() || IsKeptShown(hWnd)) {
        return false;
    }
    // Also when the taskbar has the focus from a click on it (on the open
    // app's icon, which minimized it): Windows keeps a taskbar that hides
    // itself up while it has the focus (not when it got it from the
    // keyboard, to be used with it).
    const HWND foreground = GetForegroundWindow();
    const bool clicked = g_taskbarAutoHides && foreground == hWnd &&
                         now - g_dockPressedAt < kFocusClickSeconds;
    if ((now - g_appClosedAt > 8 && !clicked) ||
        (foreground && foreground != hWnd) || UserIsReachingForTaskbar(hWnd)) {
        return false;
    }
    g_appClosedAt = 0;
    HWND next = nullptr;
    for (HWND window = GetTopWindow(nullptr); window;
         window = GetWindow(window, GW_HWNDNEXT)) {
        if (IsWindowToFocus(window)) {
            next = window;
            break;
        }
    }
    if (!next) {
        next = FindWindow(L"Progman", nullptr);
    }
    return next && SetForegroundWindow(next);
}

