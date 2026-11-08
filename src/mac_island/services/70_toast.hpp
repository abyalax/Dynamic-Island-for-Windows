bool IsUrgentToast(const char* xml, int size) {
    const std::string_view view{xml, (size_t)std::max(size, 0)};
    const size_t at = view.find("<toast");
    if (at == std::string_view::npos) {
        return false;
    }
    const size_t end = view.find('>', at);
    const std::wstring scenario = XmlAttribute(
        view.substr(at, end == std::string_view::npos ? std::string_view::npos
                                                      : end - at),
        "scenario");
    return scenario == L"incomingCall" || scenario == L"alarm" ||
           scenario == L"reminder" || scenario == L"urgent";
}

std::wstring ToastSound(const char* xml, int size) {
    const std::string_view view{xml, (size_t)std::max(size, 0)};
    const size_t at = view.find("<audio");
    if (at == std::string_view::npos) {
        return {};
    }
    const size_t end = view.find('>', at);
    const std::string_view tag = view.substr(
        at, end == std::string_view::npos ? std::string_view::npos : end - at);
    if (XmlAttribute(tag, "silent") == L"true") {
        return L"silent";
    }
    return XmlAttribute(tag, "src");
}

bool ToastSoundLoops(const char* xml, int size) {
    const std::string_view view{xml, (size_t)std::max(size, 0)};
    const size_t at = view.find("<audio");
    if (at == std::string_view::npos) {
        return false;
    }
    const size_t end = view.find('>', at);
    return XmlAttribute(view.substr(at, end == std::string_view::npos
                                            ? std::string_view::npos
                                            : end - at),
                        "loop") == L"true";
}

// The player for the apps' own sounds that aren't WAV files (WhatsApp's is
// M4A), on the island's thread; let go with the island (see Island::Destroy).
[[clang::no_destroy]] winrt::Windows::Media::Playback::MediaPlayer
    g_soundPlayer{nullptr};

bool PlaySoundFile(const std::wstring& path, bool loop) {
    const size_t dot = path.rfind(L'.');
    if (dot != std::wstring::npos &&
        _wcsicmp(path.c_str() + dot, L".wav") == 0) {
        return PlaySound(path.c_str(), nullptr,
                         SND_FILENAME | SND_ASYNC | SND_NODEFAULT |
                             (loop ? SND_LOOP : 0));
    }
    WCHAR url[2048];
    DWORD length = ARRAYSIZE(url);
    if (FAILED(UrlCreateFromPath(path.c_str(), url, &length, 0))) {
        return false;
    }
    try {
        using namespace winrt::Windows::Media;
        if (!g_soundPlayer) {
            // A sound effect: other sounds keep their volume (alerts make
            // Windows lower the media), and it isn't shown as media playing.
            g_soundPlayer = Playback::MediaPlayer();
            g_soundPlayer.AudioCategory(
                Playback::MediaPlayerAudioCategory::SoundEffects);
            g_soundPlayer.CommandManager().IsEnabled(false);
        }
        g_soundPlayer.Source(Core::MediaSource::CreateFromUri(
            winrt::Windows::Foundation::Uri(url)));
        g_soundPlayer.IsLoopingEnabled(loop);
        g_soundPlayer.Play();
        return true;
    } catch (winrt::hresult_error const& e) {
        IslandLog(L"Playing %s failed: %08X", path.c_str(), (unsigned)e.code());
        return false;
    }
}

void FreeSoundPlayer() {
    if (g_soundPlayer) {
        try {
            g_soundPlayer.Close();
        } catch (...) {
        }
        g_soundPlayer = nullptr;
    }
}

// Stops a sound that repeats (see PlayToastSound).
void StopToastSound() {
    PlaySound(nullptr, nullptr, 0);
    if (g_soundPlayer) {
        try {
            g_soundPlayer.Pause();
        } catch (...) {
        }
    }
}

// Plays a notification's sound (see IslandToast::sound): its app's own file,
// or the Windows sound it names (the usual one by default), as the sounds
// are set in Windows; over and over with `loop`, until StopToastSound.
void PlayToastSound(const std::wstring& appId,
                    const std::wstring& sound,
                    bool loop = false) {
    if (sound == L"silent") {
        return;
    }
    constexpr std::wstring_view kAppFile = L"ms-appx:///";
    if (sound.starts_with(kAppFile)) {
        std::wstring path = PackageFolder(appId);
        if (!path.empty()) {
            std::wstring file = sound.substr(kAppFile.size());
            std::replace(file.begin(), file.end(), L'/', L'\\');
            path += L"\\" + file;
            if (file.find(L"..") == std::wstring::npos &&
                GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES &&
                PlaySoundFile(path, loop)) {
                return;
            }
        }
    }
    constexpr std::wstring_view kWindowsSound = L"ms-winsoundevent:";
    const std::wstring alias = sound.starts_with(kWindowsSound)
                                   ? sound.substr(kWindowsSound.size())
                                   : L"Notification.Default";
    if (!PlaySound(alias.c_str(), nullptr,
                   SND_ALIAS | SND_ASYNC | SND_NODEFAULT |
                       (loop ? SND_LOOP : 0))) {
        PlaySound(L"Notification.Default", nullptr,
                  SND_ALIAS | SND_ASYNC | SND_NODEFAULT |
                      (loop ? SND_LOOP : 0));
    }
}

// The picture a notification shows instead of its app's icon (its
// "appLogoOverride" image, like a contact's photo), as a file on this
// computer, or on the web when chosen (islandWebPictures), only for an app
// allowed to go online, like Windows does (see PackageGoesOnline).
bool ToastImage(const std::wstring& appId,
                const char* xml,
                int size,
                std::wstring* path,
                bool* circle) {
    const std::string_view view{xml, (size_t)std::max(size, 0)};
    std::wstring src;
    for (size_t at = view.find("<image"); at != std::string_view::npos;
         at = view.find("<image", at + 1)) {
        const size_t close = view.find('>', at);
        if (close == std::string_view::npos) {
            break;
        }
        const std::string_view tag = view.substr(at, close - at);
        if (XmlAttribute(tag, "placement") == L"appLogoOverride") {
            src = XmlAttribute(tag, "src");
            *circle = XmlAttribute(tag, "hint-crop") == L"circle";
            break;
        }
    }
    if (src.empty()) {
        return false;
    }
    auto startsWith = [&](std::wstring_view prefix) {
        return src.size() > prefix.size() &&
               _wcsnicmp(src.c_str(), prefix.data(), prefix.size()) == 0;
    };
    // A packaged app's own files (ms-appdata:///local/...).
    std::wstring packageFolder;
    PWSTR localAppData = nullptr;
    if (const size_t bang = appId.find(L'!');
        bang != std::wstring::npos &&
        SUCCEEDED(SHGetKnownFolderPath(kFOLDERID_LocalAppData, 0, nullptr,
                                       &localAppData))) {
        packageFolder = std::wstring(localAppData) + L"\\Packages\\" +
                        appId.substr(0, bang) + L"\\";
    }
    CoTaskMemFree(localAppData);
    std::wstring file;
    if (startsWith(L"file:///")) {
        file = src.substr(8);
    } else if (!packageFolder.empty() && startsWith(L"ms-appdata:///local/")) {
        file = packageFolder + L"LocalState\\" + src.substr(20);
    } else if (!packageFolder.empty() &&
               startsWith(L"ms-appdata:///roaming/")) {
        file = packageFolder + L"RoamingState\\" + src.substr(22);
    } else if (!packageFolder.empty() && startsWith(L"ms-appdata:///temp/")) {
        file = packageFolder + L"TempState\\" + src.substr(19);
    } else if (src.size() > 2 && src[1] == L':' && src[2] == L'\\') {
        file = src;
    } else if (startsWith(L"https://") && g_settings.islandWebPictures &&
               PackageGoesOnline(appId)) {
        // Downloaded (see DownloadPicture).
        *path = src;
        return true;
    } else {
        return false;
    }
    // "%20" and the like, which are UTF-8 bytes.
    std::string bytes;
    for (size_t i = 0; i < file.size(); i++) {
        if (file[i] == L'%' && i + 2 < file.size() && iswxdigit(file[i + 1]) &&
            iswxdigit(file[i + 2])) {
            bytes += (char)wcstol(file.substr(i + 1, 2).c_str(), nullptr, 16);
            i += 2;
        } else if (file[i] < 0x80) {
            bytes += file[i] == L'/' ? '\\' : (char)file[i];
        } else {
            char utf8[8];
            const int length =
                WideCharToMultiByte(CP_UTF8, 0, &file[i], 1, utf8,
                                    sizeof(utf8), nullptr, nullptr);
            bytes.append(utf8, std::max(length, 0));
        }
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, bytes.data(),
                                           (int)bytes.size(), nullptr, 0);
    std::wstring decoded(std::max(length, 0), L' ');
    MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(),
                        decoded.data(), length);
    const DWORD attributes = GetFileAttributes(decoded.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES ||
        (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }
    *path = std::move(decoded);
    return true;
}

// A notification's picture on the web, only when chosen in the island's
// settings (off by default): downloaded like Windows does to show it, from
// the address the app put in its notification. Only over https, small, not
// waiting long, and kept in memory only.
bool DownloadPicture(const std::wstring& url, std::vector<BYTE>* data) {
    if (!HttpsGet(url, data, 4 * 1024 * 1024)) {
        IslandLog(L"Notification picture not downloaded: error %u",
               GetLastError());
        return false;
    }
    return true;
}

// An https address's contents (up to `maxSize`), not waiting long (for
// each answer, `waitMs`); given up once `stop` is set, and at once when
// `cancel` is canceled. `answer`: the server's HTTP status (0 without one).
bool HttpsGet(const std::wstring& url,
              std::vector<BYTE>* data,
              size_t maxSize,
              DWORD waitMs,
              const std::atomic<bool>* stop,
              HttpsCancel* cancel,
              DWORD* answer) {
    if (answer) {
        *answer = 0;
    }
    if (stop && *stop) {
        return false;
    }
    URL_COMPONENTS parts{sizeof(parts)};
    parts.dwHostNameLength = (DWORD)-1;
    parts.dwUrlPathLength = (DWORD)-1;
    parts.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts) ||
        parts.nScheme != INTERNET_SCHEME_HTTPS || !parts.lpszHostName) {
        return false;
    }
    const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    // The path with its query, which follows it.
    const std::wstring path =
        parts.lpszUrlPath
            ? std::wstring(parts.lpszUrlPath,
                           parts.dwUrlPathLength + parts.dwExtraInfoLength)
            : std::wstring(L"/");
    bool ok = false;
    HINTERNET session =
        WinHttpOpen(L"Windhawk", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET connection =
        session ? WinHttpConnect(session, host.c_str(), parts.nPort, 0)
                : nullptr;
    HINTERNET request =
        connection
            ? WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr,
                                 WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 WINHTTP_FLAG_SECURE)
            : nullptr;
    if (request && cancel) {
        AcquireSRWLockExclusive(&cancel->lock);
        if (cancel->canceled) {
            WinHttpCloseHandle(std::exchange(request, nullptr));
        } else {
            cancel->request = request;
        }
        ReleaseSRWLockExclusive(&cancel->lock);
    }
    if (request) {
        WinHttpSetTimeouts(request, 2000, 2000, 2000, (int)waitMs);
        DWORD status = 0;
        DWORD statusSize = sizeof(status);
        if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(request, nullptr) &&
            WinHttpQueryHeaders(
                request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                WINHTTP_NO_HEADER_INDEX) &&
            (answer ? (*answer = status) : status) == 200) {
            // Whole, or nothing: a connection dropped halfway (the read
            // failing) isn't taken for the end.
            BYTE buffer[16 * 1024];
            for (;;) {
                DWORD read = 0;
                if ((stop && *stop) ||
                    !WinHttpReadData(request, buffer, sizeof(buffer), &read)) {
                    break;
                }
                if (read == 0) {
                    ok = true;
                    break;
                }
                if (data->size() + read > maxSize) {
                    break;
                }
                data->insert(data->end(), buffer, buffer + read);
            }
        }
        // (Unless canceled meanwhile: closed there already.)
        bool mine = true;
        if (cancel) {
            AcquireSRWLockExclusive(&cancel->lock);
            mine = cancel->request == request;
            if (mine) {
                cancel->request = nullptr;
            }
            ReleaseSRWLockExclusive(&cancel->lock);
        }
        if (mine) {
            WinHttpCloseHandle(request);
        }
    }
    if (connection) {
        WinHttpCloseHandle(connection);
    }
    if (session) {
        WinHttpCloseHandle(session);
    }
    if (!ok || data->empty()) {
        data->clear();
        return false;
    }
    return true;
}

// Defined further down.
bool FindToastActivator(const std::wstring& appId, CLSID* clsid);

// The notification's own arguments, its text box and the button sending it.
// No reply box when the app can't be answered from here (like Phone Link,
// which has no notification activator).
ToastReply ParseToastReply(const std::wstring& appId,
                           const char* xml,
                           int size) {
    ToastReply reply;
    const std::string_view view{xml, (size_t)std::max(size, 0)};
    std::vector<std::string_view> actions;
    for (size_t at = view.find('<'); at != std::string_view::npos;
         at = view.find('<', at + 1)) {
        const size_t close = view.find('>', at);
        if (close == std::string_view::npos) {
            break;
        }
        const std::string_view tag = view.substr(at, close - at);
        auto is = [&](std::string_view name) {
            return tag.size() > name.size() + 1 &&
                   tag.substr(1, name.size()) == name &&
                   (isspace((unsigned char)tag[name.size() + 1]) ||
                    tag[name.size() + 1] == '/');
        };
        if (is("toast")) {
            reply.launch = XmlAttribute(tag, "launch");
            reply.launchType = XmlAttribute(tag, "activationType");
        } else if (is("input") && reply.inputId.empty() &&
                   XmlAttribute(tag, "type") == L"text") {
            reply.inputId = XmlAttribute(tag, "id");
            reply.placeholder = XmlAttribute(tag, "placeHolderContent");
        } else if (is("action")) {
            actions.push_back(tag);
        }
    }
    for (const auto& tag : actions) {
        if (!reply.inputId.empty() &&
            XmlAttribute(tag, "hint-inputId") == reply.inputId) {
            reply.arguments = XmlAttribute(tag, "arguments");
            reply.activationType = XmlAttribute(tag, "activationType");
            reply.label = XmlAttribute(tag, "content");
            break;
        }
    }
    // The other buttons, up to three (like Windows shows them); the app's
    // own ones are left out when it has no notification handler.
    bool activatorKnown = false;
    bool activator = false;
    const bool fromWindows = appId.starts_with(L"Windows.SystemToast.") ||
                             appId.starts_with(L"Microsoft.Windows.");
    for (const auto& tag : actions) {
        if (reply.actions.size() >= 3) {
            break;
        }
        if ((!reply.inputId.empty() &&
             XmlAttribute(tag, "hint-inputId") == reply.inputId) ||
            XmlAttribute(tag, "placement") == L"contextMenu") {
            continue;
        }
        ToastReply::Action action{XmlAttribute(tag, "content"),
                                  XmlAttribute(tag, "arguments"),
                                  XmlAttribute(tag, "activationType")};
        if (action.activationType.empty()) {
            action.activationType = L"foreground";
        }
        if (action.activationType == L"system") {
            // Windows' own: "Snooze" (the island shows it again a few minutes
            // later, see Island::SnoozeToast) and "Dismiss", named as Windows
            // names them.
            if (action.arguments == L"snooze") {
                if (action.label.empty()) {
                    action.label = Tr(L"Snooze", L"Tocar de novo");
                }
            } else if (action.arguments == L"dismiss") {
                if (action.label.empty()) {
                    action.label = Tr(L"Dismiss", L"Ignorar");
                }
            } else {
                reply.windowsButtons = reply.windowsButtons || fromWindows;
                continue;
            }
        } else if (action.activationType != L"protocol") {
            if (!activatorKnown) {
                activatorKnown = true;
                CLSID actionClsid;
                activator = FindToastActivator(appId, &actionClsid);
            }
            if (!activator) {
                reply.windowsButtons = reply.windowsButtons || fromWindows;
                continue;
            }
        }
        if (!action.label.empty()) {
            reply.actions.push_back(std::move(action));
        }
    }
    CLSID clsid;
    if (reply.CanReply() && reply.activationType != L"protocol" &&
        !FindToastActivator(appId, &clsid)) {
        reply.inputId.clear();
    }
    return reply;
}

// Notifications are answered like Windows does: through the app's
// notification activator (a COM class it registers for that), with the
// button's arguments and what was typed.
struct NotificationUserInputData {
    LPCWSTR Key;
    LPCWSTR Value;
};

struct INotificationActivationCallback : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE
    Activate(LPCWSTR appUserModelId,
             LPCWSTR invokedArgs,
             const NotificationUserInputData* data,
             ULONG count) = 0;
};

constexpr GUID kIID_INotificationActivationCallback = {
    0x53e31837,
    0x6600,
    0x4a81,
    {0x93, 0x95, 0x75, 0xcf, 0xfe, 0x74, 0x6f, 0x94}};

// The activator of a packaged app, from its manifest
// (ToastActivatorCLSID), or of another one, from its registration or its
// shortcut (see UnpackagedToastActivator); remembered.
// Looked up from the notification threads and the island's.
SRWLOCK g_toastActivatorsLock = SRWLOCK_INIT;

// A packaged app's folder (empty for other apps).
std::wstring PackageFolder(const std::wstring& appId) {
    const size_t bang = appId.find(L'!');
    if (bang == std::wstring::npos) {
        return {};
    }
    const std::wstring family = appId.substr(0, bang);
    UINT32 count = 0;
    UINT32 length = 0;
    if (GetPackagesByPackageFamily(family.c_str(), &count, nullptr, &length,
                                   nullptr) != ERROR_INSUFFICIENT_BUFFER ||
        !count) {
        return {};
    }
    std::vector<PWSTR> names(count);
    std::vector<WCHAR> buffer(length);
    UINT32 pathLength = 0;
    if (GetPackagesByPackageFamily(family.c_str(), &count, names.data(),
                                   &length, buffer.data()) != ERROR_SUCCESS ||
        GetPackagePathByFullName(names[0], &pathLength, nullptr) !=
            ERROR_INSUFFICIENT_BUFFER) {
        return {};
    }
    std::wstring path(pathLength, L'\0');
    if (GetPackagePathByFullName(names[0], &pathLength, path.data()) !=
        ERROR_SUCCESS) {
        return {};
    }
    path.resize(wcslen(path.c_str()));
    return path;
}

// A packaged app's AppxManifest.xml (empty for other apps).
std::string ReadAppxManifest(const std::wstring& appId) {
    std::string xml;
    std::wstring path = PackageFolder(appId);
    if (path.empty()) {
        return xml;
    }
    path += L"\\AppxManifest.xml";
    HANDLE file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                             nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return xml;
    }
    const DWORD size = GetFileSize(file, nullptr);
    xml.resize(size < 4 * 1024 * 1024 ? size : 0);
    DWORD read = 0;
    ReadFile(file, xml.data(), (DWORD)xml.size(), &read, nullptr);
    xml.resize(read);
    CloseHandle(file);
    return xml;
}

// Whether a packaged app may go online (it declares the "internetClient"
// capability): Windows only shows its notifications' pictures from the web
// then. Remembered.
SRWLOCK g_onlineAppsLock = SRWLOCK_INIT;

bool PackageGoesOnline(const std::wstring& appId) {
    static std::vector<std::pair<std::wstring, bool>> cache;
    AcquireSRWLockShared(&g_onlineAppsLock);
    for (const auto& [id, online] : cache) {
        if (id == appId) {
            ReleaseSRWLockShared(&g_onlineAppsLock);
            return online;
        }
    }
    ReleaseSRWLockShared(&g_onlineAppsLock);
    const std::string xml = ReadAppxManifest(appId);
    const bool online =
        xml.find("Name=\"internetClient\"") != std::string::npos ||
        xml.find("Name=\"internetClientServer\"") != std::string::npos;
    AcquireSRWLockExclusive(&g_onlineAppsLock);
    cache.push_back({appId, online});
    ReleaseSRWLockExclusive(&g_onlineAppsLock);
    return online;
}

// PKEY_AppUserModel_ToastActivatorCLSID: the notification activator an
// unpackaged app puts on its Start-menu shortcut.
constexpr PROPERTYKEY kPKEY_AppUserModel_ToastActivatorCLSID = {
    kPKEY_AppUserModel_ID.fmtid, 26};
// FOLDERID_Programs and FOLDERID_CommonPrograms: the Start menu's programs.
constexpr GUID kFOLDERID_Programs = {
    0xa77f5d77,
    0x2e2b,
    0x44c3,
    {0xa6, 0xa2, 0xab, 0xa6, 0x01, 0x05, 0x4a, 0x51}};
constexpr GUID kFOLDERID_CommonPrograms = {
    0x0139d44e,
    0x6afe,
    0x49f2,
    {0x86, 0x90, 0x3d, 0xaf, 0xca, 0xe6, 0xff, 0xb8}};

// The shortcuts of the Start menu that name an app and its notification
// activator ({AppUserModelID, CLSID}), read on a thread of its own (not on
// the island's); again, when an app isn't there, after this long.
// Under g_toastActivatorsLock.
std::vector<std::pair<std::wstring, std::wstring>> g_toastShortcuts;
ULONGLONG g_toastShortcutsRead = 0;
bool g_toastShortcutsReading = false;
HANDLE g_toastShortcutsThread = nullptr;
constexpr ULONGLONG kToastShortcutsRefresh = 5 * 60 * 1000;
// What one read looks at, at most: entries of the folders, and how deep.
constexpr int kToastShortcutsMaxEntries = 5000;
constexpr int kToastShortcutsMaxDepth = 4;

// The shortcuts under a folder with an AppUserModelID and a notification
// activator. `budget` counts down the entries still to look at.
void ReadToastShortcuts(
    const std::wstring& folder,
    int depth,
    int* budget,
    std::vector<std::pair<std::wstring, std::wstring>>* found) {
    WIN32_FIND_DATA data;
    HANDLE find = FindFirstFileEx((folder + L"\\*").c_str(), FindExInfoBasic,
                                  &data, FindExSearchNameMatch, nullptr, 0);
    if (find == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if (--*budget < 0) {
            break;
        }
        if (wcscmp(data.cFileName, L".") == 0 ||
            wcscmp(data.cFileName, L"..") == 0) {
            continue;
        }
        const std::wstring path = folder + L"\\" + data.cFileName;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (depth < kToastShortcutsMaxDepth &&
                !(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                ReadToastShortcuts(path, depth + 1, budget, found);
            }
            continue;
        }
        if (_wcsicmp(PathFindExtension(data.cFileName), L".lnk") != 0) {
            continue;
        }
        winrt::com_ptr<IPropertyStore> store;
        if (FAILED(SHGetPropertyStoreFromParsingName(
                path.c_str(), nullptr, GPS_DEFAULT,
                IID_PPV_ARGS(store.put())))) {
            continue;
        }
        PROPVARIANT id;
        PROPVARIANT activator;
        PropVariantInit(&id);
        PropVariantInit(&activator);
        if (SUCCEEDED(store->GetValue(kPKEY_AppUserModel_ID, &id)) &&
            id.vt == VT_LPWSTR && id.pwszVal && id.pwszVal[0] &&
            SUCCEEDED(store->GetValue(kPKEY_AppUserModel_ToastActivatorCLSID,
                                      &activator)) &&
            activator.vt == VT_CLSID && activator.puuid) {
            WCHAR text[64];
            if (StringFromGUID2(*activator.puuid, text, ARRAYSIZE(text))) {
                found->push_back({id.pwszVal, text});
            }
        }
        PropVariantClear(&id);
        PropVariantClear(&activator);
    } while (FindNextFile(find, &data));
    FindClose(find);
}

DWORD WINAPI ToastShortcutsThreadProc(LPVOID) {
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    std::vector<std::pair<std::wstring, std::wstring>> found;
    int budget = kToastShortcutsMaxEntries;
    for (const GUID& known : {kFOLDERID_Programs, kFOLDERID_CommonPrograms}) {
        const std::wstring folder = KnownFolderPath(known);
        if (!folder.empty()) {
            ReadToastShortcuts(folder, 0, &budget, &found);
        }
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    AcquireSRWLockExclusive(&g_toastActivatorsLock);
    g_toastShortcuts = std::move(found);
    g_toastShortcutsReading = false;
    ReleaseSRWLockExclusive(&g_toastActivatorsLock);
    return 0;
}

// Starts reading the Start menu's shortcuts, once and then again at most
// every few minutes (never once the mod is unloading).
void StartToastShortcutsRead() {
    AcquireSRWLockExclusive(&g_toastActivatorsLock);
    const ULONGLONG now = GetTickCount64();
    if (!g_unloading && !g_toastShortcutsReading &&
        (!g_toastShortcutsRead ||
         now - g_toastShortcutsRead > kToastShortcutsRefresh)) {
        if (g_toastShortcutsThread) {
            CloseHandle(g_toastShortcutsThread);
        }
        g_toastShortcutsRead = now;
        g_toastShortcutsThread = CreateThread(
            nullptr, 0, ToastShortcutsThreadProc, nullptr, 0, nullptr);
        g_toastShortcutsReading = g_toastShortcutsThread != nullptr;
    }
    ReleaseSRWLockExclusive(&g_toastActivatorsLock);
}

// The activator of an app that isn't packaged, as a CLSID string: its
// registration (CustomActivator), or its Start-menu shortcut's. Never waits
// for the shortcuts: what's read so far (see StartToastShortcutsRead).
std::wstring UnpackagedToastActivator(const std::wstring& appId) {
    const std::wstring registered =
        RegisteredAppValue(appId, L"CustomActivator");
    CLSID clsid;
    if (!registered.empty() &&
        SUCCEEDED(CLSIDFromString(registered.c_str(), &clsid))) {
        return registered;
    }
    std::wstring found;
    AcquireSRWLockShared(&g_toastActivatorsLock);
    for (const auto& [id, value] : g_toastShortcuts) {
        if (_wcsicmp(id.c_str(), appId.c_str()) == 0) {
            found = value;
            break;
        }
    }
    ReleaseSRWLockShared(&g_toastActivatorsLock);
    return found;
}

bool FindToastActivator(const std::wstring& appId, CLSID* clsid) {
    static std::vector<std::pair<std::wstring, std::wstring>> cache;
    std::wstring found;
    bool cached = false;
    AcquireSRWLockShared(&g_toastActivatorsLock);
    for (const auto& [id, value] : cache) {
        if (id == appId) {
            found = value;
            cached = true;
        }
    }
    ReleaseSRWLockShared(&g_toastActivatorsLock);
    if (!cached) {
        const std::string xml = ReadAppxManifest(appId);
        const size_t at = xml.find("ToastActivatorCLSID=\"");
        if (at != std::string::npos) {
            const size_t start = at + 21;
            const size_t end = xml.find('"', start);
            if (end != std::string::npos && end - start < 40) {
                found = L"{" +
                        std::wstring(xml.begin() + start, xml.begin() + end) +
                        L"}";
            }
        }
        // An app that isn't packaged: not remembered until it's found (its
        // shortcut may not be read yet, or be added later).
        if (found.empty() && xml.empty()) {
            found = UnpackagedToastActivator(appId);
            if (found.empty()) {
                StartToastShortcutsRead();
                return false;
            }
        }
        AcquireSRWLockExclusive(&g_toastActivatorsLock);
        cache.push_back({appId, found});
        ReleaseSRWLockExclusive(&g_toastActivatorsLock);
    }
    return !found.empty() && SUCCEEDED(CLSIDFromString(found.c_str(), clsid));
}

struct ToastActivation {
    CLSID clsid;
    std::wstring appId;
    std::wstring arguments;
    std::wstring inputId;
    std::wstring text;
};

// The app's activator is out of process and may have to start: called on a
// thread of its own.
DWORD WINAPI ToastActivationThreadProc(LPVOID parameter) {
    std::unique_ptr<ToastActivation> activation((ToastActivation*)parameter);
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    winrt::com_ptr<INotificationActivationCallback> callback;
    HRESULT result = CoCreateInstance(
        activation->clsid, nullptr, CLSCTX_LOCAL_SERVER,
        kIID_INotificationActivationCallback, callback.put_void());
    if (SUCCEEDED(result)) {
        const NotificationUserInputData data{activation->inputId.c_str(),
                                             activation->text.c_str()};
        result = callback->Activate(activation->appId.c_str(),
                                    activation->arguments.c_str(),
                                    activation->inputId.empty() ? nullptr
                                                                : &data,
                                    activation->inputId.empty() ? 0 : 1);
    }
    IslandLog(L"Notification activation of \"%s\": %08X",
        activation->appId.c_str(), (unsigned)result);
    callback = nullptr;
    if (SUCCEEDED(comResult)) {
        CoUninitialize();
    }
    return 0;
}

std::vector<HANDLE> g_toastActivationThreads;

void WaitForToastActivations() {
    for (HANDLE thread : g_toastActivationThreads) {
        WaitForSingleObject(thread, 10000);
        CloseHandle(thread);
    }
    g_toastActivationThreads.clear();
    WaitForToastShortcutsRead();
}

// The Start menu's shortcuts' reader, if one runs, waited for (it ends by
// itself: a few thousand entries at most).
void WaitForToastShortcutsRead() {
    AcquireSRWLockExclusive(&g_toastActivatorsLock);
    HANDLE shortcuts = std::exchange(g_toastShortcutsThread, nullptr);
    ReleaseSRWLockExclusive(&g_toastActivatorsLock);
    if (shortcuts) {
        WaitForSingleObject(shortcuts, 10000);
        CloseHandle(shortcuts);
    }
}

// Presses a notification's button (or the notification itself): the app
// gets its arguments, and what was typed for `inputId`. Returns false if the
// app can't be reached this way.
bool ActivateToast(const std::wstring& appId,
                   const std::wstring& arguments,
                   const std::wstring& activationType,
                   const std::wstring& inputId = {},
                   const std::wstring& text = {}) {
    // The app (or the page it opens) can come to the front.
    AllowSetForegroundWindow(ASFW_ANY);
    if (activationType == L"protocol") {
        // Only a URI with a scheme (like "https:" or an app's own), never a
        // file or a path.
        const size_t colon = arguments.find(L':');
        const bool uri =
            colon != std::wstring::npos && colon >= 2 &&
            std::all_of(arguments.begin(), arguments.begin() + colon,
                        [](WCHAR c) {
                            return iswalnum(c) || c == L'+' || c == L'-' ||
                                   c == L'.';
                        }) &&
            _wcsnicmp(arguments.c_str(), L"file:", 5) != 0;
        return uri && (INT_PTR)ShellExecute(nullptr, L"open",
                                            arguments.c_str(), nullptr,
                                            nullptr, SW_SHOWNORMAL) > 32;
    }
    CLSID clsid;
    if (!FindToastActivator(appId, &clsid)) {
        return false;
    }
    std::erase_if(g_toastActivationThreads, [](HANDLE thread) {
        if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0) {
            CloseHandle(thread);
            return true;
        }
        return false;
    });
    auto* activation =
        new ToastActivation{clsid, appId, arguments, inputId, text};
    HANDLE thread = CreateThread(nullptr, 0, ToastActivationThreadProc,
                                 activation, 0, nullptr);
    if (!thread) {
        delete activation;
        return false;
    }
    g_toastActivationThreads.push_back(thread);
    return true;
}

// A mouse message to a tray icon's app, as from the taskbar (its menu or
// window can come to the front; with the island not elevated even from
// another process of the app: Steam draws its menu in steamwebhelper.exe).
// In the form its version takes, or the newer one (`newer`). With the
// island elevated (Windhawk run as administrator), only to an app that
// isn't: the icons come from explorer's messages, which UIPI lets a lower
// process send too (see AllowExplorerMessages), and nothing may reach an
// elevated window through the island.
void PostTrayIconMessage(const TrayIcon& icon,
                         UINT message,
                         bool newer = false) {
    static const bool elevated = ProcessElevated(GetCurrentProcessId());
    DWORD owner = 0;
    if (!GetWindowThreadProcessId(icon.owner, &owner) ||
        (elevated && ProcessElevated(owner))) {
        return;
    }
    AllowSetForegroundWindow(elevated ? owner : ASFW_ANY);
    POINT pt;
    GetCursorPos(&pt);
    if (newer || TrayIconNewer(icon)) {
        PostMessage(icon.owner, icon.callback, MAKEWPARAM(pt.x, pt.y),
                    MAKELPARAM(message, icon.id));
    } else {
        PostMessage(icon.owner, icon.callback, icon.id, message);
    }
}

// After a button is let go, what the taskbar sends then: a click's
// NIN_SELECT (version 4), the menu's WM_CONTEXTMENU (from version 3 on; some
// apps, like Qt's, open their menu only then). An icon whose version isn't
// known gets them in the newer form too, which older apps don't use.
void PostTrayIconRelease(const TrayIcon& icon, UINT released) {
    const bool unknown = icon.version == kTrayVersionUnknown;
    if (released == WM_LBUTTONUP && (unknown || TrayIconNewer(icon))) {
        PostTrayIconMessage(icon, NIN_SELECT, unknown);
    }
    if (released == WM_RBUTTONUP && (unknown || icon.version >= 3)) {
        PostTrayIconMessage(icon, WM_CONTEXTMENU, unknown);
    }
}

bool ProcessIsMediaApp(DWORD processId, const std::wstring& appId);

// An app hidden in the tray (like many chat apps, with no window to bring
// up): its tray icon is clicked, as the user would. False without one. The
// ID can also be the app's program's name (like QQ's notifications, "QQ";
// see ProcessIsMediaApp).
bool OpenFromTrayIcon(const std::wstring& appId) {
    AcquireSRWLockShared(&g_trayIconsLock);
    const std::vector<TrayIcon> icons = g_trayIcons;
    ReleaseSRWLockShared(&g_trayIconsLock);
    for (const auto& icon : icons) {
        DWORD processId = 0;
        if (icon.callback && IsWindow(icon.owner) &&
            (WindowMatchesApp(icon.owner, appId) ||
             (GetWindowThreadProcessId(icon.owner, &processId) &&
              ProcessIsMediaApp(processId, appId)))) {
            PostTrayIconMessage(icon, WM_LBUTTONDOWN);
            PostTrayIconMessage(icon, WM_LBUTTONUP);
            PostTrayIconRelease(icon, WM_LBUTTONUP);
            return true;
        }
    }
    return false;
}

// The Snipping Tool's (Win+Shift+S): its notification says a screenshot
// was taken.
bool IsSnippingToolApp(const std::wstring& appId) {
    return StrStrI(appId.c_str(), L"ScreenSketch") ||
           StrStrI(appId.c_str(), L"ScreenClipping") ||
           StrStrI(appId.c_str(), L"SnippingTool");
}

// The picture the Snipping Tool just saved in the Screenshots folder (the
// newest, from the last two minutes), selected in Explorer; false without
// one (saving off, or another folder chosen in it: its notification opens
// it as usual, like other ones of it, such as a recording's).
bool ShowNewestScreenshot() {
    // FOLDERID_Screenshots.
    constexpr GUID kScreenshots = {
        0xb7bede81,
        0xdf94,
        0x4682,
        {0xa7, 0xd8, 0x57, 0xa5, 0x26, 0x20, 0xb8, 0x6f}};
    const std::wstring folder = KnownFolderPath(kScreenshots);
    WCHAR windows[MAX_PATH];
    if (folder.empty() || !GetWindowsDirectory(windows, ARRAYSIZE(windows))) {
        return false;
    }
    std::wstring newest;
    FILETIME newestTime{};
    WIN32_FIND_DATA data;
    HANDLE find = FindFirstFileEx((folder + L"\\*").c_str(), FindExInfoBasic,
                                  &data, FindExSearchNameMatch, nullptr, 0);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            // (Pictures only: not desktop.ini, Thumbs.db or folders.)
            PCWSTR extension = PathFindExtension(data.cFileName);
            const bool picture = _wcsicmp(extension, L".png") == 0 ||
                                 _wcsicmp(extension, L".jpg") == 0 ||
                                 _wcsicmp(extension, L".jpeg") == 0 ||
                                 _wcsicmp(extension, L".gif") == 0;
            if (picture &&
                !(data.dwFileAttributes &
                  (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN |
                   FILE_ATTRIBUTE_SYSTEM)) &&
                CompareFileTime(&data.ftLastWriteTime, &newestTime) > 0) {
                newest = data.cFileName;
                newestTime = data.ftLastWriteTime;
            }
        } while (FindNextFile(find, &data));
        FindClose(find);
    }
    FILETIME nowTime;
    GetSystemTimeAsFileTime(&nowTime);
    const ULONGLONG now =
        ((ULONGLONG)nowTime.dwHighDateTime << 32) | nowTime.dwLowDateTime;
    const ULONGLONG saved =
        ((ULONGLONG)newestTime.dwHighDateTime << 32) | newestTime.dwLowDateTime;
    constexpr ULONGLONG kJustSaved = 120ULL * 10000000;
    if (newest.empty() || now < saved || now - saved > kJustSaved) {
        return false;
    }
    const std::wstring explorer = std::wstring(windows) + L"\\explorer.exe";
    const std::wstring arguments =
        L"/select,\"" + folder + L"\\" + newest + L"\"";
    return (INT_PTR)ShellExecute(nullptr, L"open", explorer.c_str(),
                                 arguments.c_str(), nullptr,
                                 SW_SHOWNORMAL) > 32;
}

// Opens a notification: the place in the app it points to (a chat, through
// the app's notification handler), the app's window, or the app itself when
// the shell knows it by its ID; the Snipping Tool's, the screenshot when
// chosen (islandScreenshotFolder). Returns false when there's nothing to open
// (like a script's notification, whose ID is only a path): the island shows
// the list instead.
bool OpenNotification(const std::wstring& appId, const ToastReply& reply) {
    if (appId.empty()) {
        return false;
    }
    if (g_settings.islandScreenshotFolder && IsSnippingToolApp(appId) &&
        ShowNewestScreenshot()) {
        return true;
    }
    // Its buttons are Windows' (see ToastReply::windowsButtons): Windows'
    // notification center has them (the key pressed passes the island's
    // own Win+N, see the keyboard hook).
    if (reply.windowsButtons) {
        PressWindowsKey('N');
        return true;
    }
    if (!reply.launch.empty() &&
        ActivateToast(
            appId, reply.launch,
            reply.launchType.empty() ? L"foreground" : reply.launchType)) {
        return true;
    }
    AppWindowSearch context{&appId, nullptr};
    EnumWindows(FindAppWindowProc, (LPARAM)&context);
    if (context.found) {
        ActivateApp(context.found);
        return true;
    }
    if (OpenFromTrayIcon(appId)) {
        return true;
    }
    if (appId.find(L'\\') != std::wstring::npos || appId[0] == L'{') {
        return false;
    }
    const std::wstring target = L"shell:AppsFolder\\" + appId;
    return (INT_PTR)ShellExecute(nullptr, L"open", target.c_str(), nullptr,
                                 nullptr, SW_SHOWNORMAL) > 32;
}

std::vector<std::wstring> ToastTexts(const char* xml, int size) {
    std::vector<std::wstring> texts;
    const std::string_view view{xml, (size_t)std::max(size, 0)};
    size_t at = 0;
    while ((at = view.find("<text", at)) != std::string_view::npos) {
        const size_t open = at + 5;
        at = open;
        if (open >= view.size() || (view[open] != '>' && view[open] != ' ')) {
            continue;
        }
        const size_t close = view.find('>', open);
        if (close == std::string_view::npos) {
            break;
        }
        const std::string_view tag = view.substr(open, close - open);
        if (!tag.empty() && tag.back() == '/') {
            continue;
        }
        // The app's name the toast sometimes adds isn't part of the message.
        if (tag.find("placement=\"attribution\"") != std::string_view::npos) {
            continue;
        }
        const size_t end = view.find("</text>", close);
        if (end == std::string_view::npos) {
            break;
        }
        const std::string_view raw = view.substr(close + 1, end - close - 1);
        at = end + 7;

        std::wstring wide = DecodeXml(raw);
        // Its own lines kept (see SplitToastTexts); tabs become spaces.
        std::erase(wide, L'\r');
        for (auto& c : wide) {
            if (c == L'\t') {
                c = L' ';
            }
        }
        if (!wide.empty()) {
            texts.push_back(std::move(wide));
        }
    }
    return texts;
}

// "now", "5 min", the time today, "yesterday", or the date, for a
// notification's arrival time (a FILETIME, in UTC).
std::wstring NotificationTimeText(long long arrival) {
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    const long long current =
        ((long long)now.dwHighDateTime << 32) | now.dwLowDateTime;
    const long long minutes = (current - arrival) / (60LL * 10000000);
    if (minutes < 1) {
        return Tr(L"now", L"agora");
    }
    if (minutes < 60) {
        return std::to_wstring(minutes) + L" min";
    }
    const ULARGE_INTEGER value{.QuadPart = (ULONGLONG)arrival};
    const FILETIME utc{value.LowPart, value.HighPart};
    FILETIME localFile;
    SYSTEMTIME local;
    SYSTEMTIME today;
    if (!FileTimeToLocalFileTime(&utc, &localFile) ||
        !FileTimeToSystemTime(&localFile, &local)) {
        return {};
    }
    GetLocalTime(&today);
    WCHAR text[64] = L"";
    if (local.wYear == today.wYear && local.wMonth == today.wMonth &&
        local.wDay == today.wDay) {
        return FormatClockTime(local, false);
    }
    if (minutes < 48 * 60) {
        return Tr(L"yesterday", L"ontem");
    }
    GetDateFormatEx(DateLocale(), 0, &local, MonthDayPicture(true).c_str(),
                    text, ARRAYSIZE(text), nullptr);
    return text;
}

void NotificationPanel::Prepare(float scale) {
    if (!m_wic) {
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(m_wic.put()));
    }
    if (scale != m_preparedScale) {
        m_preparedScale = scale;
        m_bitmaps.clear();
        m_cardLines.clear();
        m_headerFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_SEMI_BOLD,
                       14 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_appFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       11.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_timeFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       11.5f * scale, DWRITE_TEXT_ALIGNMENT_TRAILING);
        m_titleFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_SEMI_BOLD,
                       13 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_textFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       12.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_linkFormat = MakeFormat(L"Segoe UI Variable Text",
                                  DWRITE_FONT_WEIGHT_NORMAL, 12 * scale);
        m_iconFormat = MakeFormat(IconFont(),
                                  DWRITE_FONT_WEIGHT_NORMAL, 14 * scale);
    }
    Load();
    Layout();
}

void NotificationPanel::ClearApp(const std::wstring& appId) {
    LoadCleared();
    if (!m_sqliteLoaded) {
        m_sqliteLoaded = true;
        if (!m_sqlite.Load()) {
            m_sqlite.Unload();
        }
    }
    const std::string path = GetNotificationDatabasePath();
    void* db = nullptr;
    if (!m_sqlite.module || !m_sqlite.open_v2 || path.empty() ||
        m_sqlite.open_v2(path.c_str(), &db, Sqlite::kOpenReadOnly, nullptr) !=
            0) {
        if (db) {
            m_sqlite.close(db);
        }
        return;
    }
    m_sqlite.busy_timeout(db, 500);
    void* statement = nullptr;
    bool changed = false;
    if (m_sqlite.prepare_v2(
            db,
            "SELECT n.Id, h.PrimaryId FROM Notification n JOIN "
            "NotificationHandler h ON h.RecordId = n.HandlerId WHERE n.Type = "
            "'toast' AND n.ArrivalTime > ?1",
            -1, &statement, nullptr) == 0) {
        m_sqlite.bind_int64(statement, 1, m_clearedUntil);
        while (m_sqlite.step(statement) == Sqlite::kRow) {
            const long long id = m_sqlite.column_int64(statement, 0);
            const auto app = (PCWSTR)m_sqlite.column_text16(statement, 1);
            if (app && SameNotificationApp(app, appId) &&
                std::find(m_dismissed.begin(), m_dismissed.end(), id) ==
                    m_dismissed.end()) {
                m_dismissed.push_back(id);
                changed = true;
            }
        }
        m_sqlite.finalize(statement);
    }
    m_sqlite.close(db);
    // (The kept ones too, see MergeKeptToasts.)
    AcquireSRWLockShared(&g_keptToastsLock);
    for (const auto& kept : g_keptToasts) {
        if (SameNotificationApp(kept.toast.appId, appId) &&
            std::find(m_dismissed.begin(), m_dismissed.end(), kept.toast.id) ==
                m_dismissed.end()) {
            m_dismissed.push_back(kept.toast.id);
            changed = true;
        }
    }
    ReleaseSRWLockShared(&g_keptToastsLock);
    if (changed) {
        SaveCleared();
        Refresh();
    }
}

bool NotificationPanel::ReadCenter(PCSTR query,
                                   const std::function<void(void*)>& row) {
    if (!m_sqliteLoaded) {
        m_sqliteLoaded = true;
        if (!m_sqlite.Load()) {
            m_sqlite.Unload();
        }
    }
    const std::string path = GetNotificationDatabasePath();
    void* db = nullptr;
    if (!m_sqlite.module || !m_sqlite.open_v2 || path.empty() ||
        m_sqlite.open_v2(path.c_str(), &db, Sqlite::kOpenReadOnly, nullptr) !=
            0) {
        if (db) {
            m_sqlite.close(db);
        }
        return false;
    }
    m_sqlite.busy_timeout(db, 500);
    void* statement = nullptr;
    bool read = false;
    if (m_sqlite.prepare_v2(db, query, -1, &statement, nullptr) == 0) {
        while (m_sqlite.step(statement) == Sqlite::kRow) {
            row(statement);
        }
        m_sqlite.finalize(statement);
        read = true;
    }
    m_sqlite.close(db);
    return read;
}

bool NotificationPanel::AppsInCenter(std::vector<std::wstring>* apps) {
    apps->clear();
    return ReadCenter(
        "SELECT DISTINCT h.PrimaryId FROM Notification n JOIN "
        "NotificationHandler h ON h.RecordId = n.HandlerId WHERE n.Type = "
        "'toast'",
        [&](void* statement) {
            if (auto app = (PCWSTR)m_sqlite.column_text16(statement, 0)) {
                apps->push_back(app);
                // (And the app it's shown under, see NotificationAppKey.)
                if (std::wstring key = NotificationAppKey(app); key != app) {
                    apps->push_back(std::move(key));
                }
            }
        });
}

// WhatsApp puts a chat's messages in one notification, saying how many
// ("3 unread messages", "3 mensagens não lidas", "3条未读消息"; its badge
// says 1): the number on a line with one of these words, or 1.
int UnreadInToastTexts(const std::vector<std::wstring>& texts) {
    static constexpr PCWSTR kUnreadWords[] = {
        L"unread",     L"não lida",  L"no leído", L"未读",
        L"未讀",       L"未読",      L"non lu",   L"ungelesen",
        L"non lett",   L"непрочитан", L"okunmamış", L"belum dibaca",
        L"읽지 않은"};
    for (const auto& text : texts) {
        size_t start = 0;
        while (start < text.size()) {
            size_t end = text.find(L'\n', start);
            if (end == std::wstring::npos) {
                end = text.size();
            }
            std::wstring line = text.substr(start, end - start);
            CharLowerBuff(line.data(), (DWORD)line.size());
            const size_t digit = line.find_first_of(L"0123456789");
            if (digit != std::wstring::npos &&
                std::any_of(std::begin(kUnreadWords), std::end(kUnreadWords),
                            [&](PCWSTR word) {
                                return line.find(word) != std::wstring::npos;
                            })) {
                if (const int count = _wtoi(line.c_str() + digit); count > 1) {
                    return std::min(count, 999);
                }
            }
            start = end + 1;
        }
    }
    return 1;
}

