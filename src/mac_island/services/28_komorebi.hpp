// komorebi's process in this session, or 0 if it doesn't run.
DWORD KomorebiProcessId() {
    return SessionProcessId(L"komorebi.exe");
}

bool KomorebiRunning() {
    return KomorebiProcessId() != 0;
}

// Its version: from komorebi.exe beside its tool (scoop's shim says where
// that is), or from its installer's entry in Windows' apps; empty if
// neither says.
std::wstring KomorebiVersion(std::wstring komorebic) {
    if (komorebic.ends_with(L"\\shims\\komorebic.exe")) {
        const std::wstring shim =
            komorebic.substr(0, komorebic.size() - 3) + L"shim";
        if (FILE* file = _wfopen(shim.c_str(), L"rt, ccs=UTF-8")) {
            WCHAR line[MAX_PATH + 16];
            while (fgetws(line, ARRAYSIZE(line), file)) {
                std::wstring text = line;
                if (text.starts_with(L"path = ")) {
                    text = text.substr(7);
                    std::erase(text, L'"');
                    std::erase(text, L'\n');
                    komorebic = text;
                    break;
                }
            }
            fclose(file);
        }
    }
    const std::wstring exe =
        komorebic.substr(0, komorebic.find_last_of(L'\\') + 1) +
        L"komorebi.exe";
    DWORD handle = 0;
    if (const DWORD size = GetFileVersionInfoSize(exe.c_str(), &handle)) {
        std::vector<BYTE> data(size);
        VS_FIXEDFILEINFO* info = nullptr;
        UINT length = 0;
        if (GetFileVersionInfo(exe.c_str(), 0, size, data.data()) &&
            VerQueryValue(data.data(), L"\\", (void**)&info, &length) &&
            info && length >= sizeof(*info) && info->dwFileVersionMS) {
            return std::to_wstring(HIWORD(info->dwFileVersionMS)) + L"." +
                   std::to_wstring(LOWORD(info->dwFileVersionMS)) + L"." +
                   std::to_wstring(HIWORD(info->dwFileVersionLS));
        }
    }
    for (HKEY root : {HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER}) {
        HKEY apps = nullptr;
        if (RegOpenKeyEx(root,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                         L"Uninstall",
                         0, KEY_READ | KEY_WOW64_64KEY,
                         &apps) != ERROR_SUCCESS) {
            continue;
        }
        std::wstring version;
        WCHAR name[256];
        for (DWORD i = 0; version.empty(); i++) {
            DWORD nameLength = ARRAYSIZE(name);
            if (RegEnumKeyEx(apps, i, name, &nameLength, nullptr, nullptr,
                             nullptr, nullptr) != ERROR_SUCCESS) {
                break;
            }
            WCHAR display[64] = L"";
            DWORD displaySize = sizeof(display);
            if (RegGetValue(apps, name, L"DisplayName", RRF_RT_REG_SZ, nullptr,
                            display, &displaySize) != ERROR_SUCCESS ||
                _wcsicmp(display, L"komorebi") != 0) {
                continue;
            }
            WCHAR value[64] = L"";
            DWORD valueSize = sizeof(value);
            if (RegGetValue(apps, name, L"DisplayVersion", RRF_RT_REG_SZ,
                            nullptr, value, &valueSize) == ERROR_SUCCESS) {
                version = value;
            }
        }
        RegCloseKey(apps);
        if (!version.empty()) {
            return version;
        }
    }
    return {};
}

// The commands for a button (0 start, 1 stop, 2 reload), with what it
// takes along (whkd, masir).
std::wstring KomorebiCommands(const std::wstring& komorebic, int button) {
    std::wstring flags;
    if (g_settings.komorebiWhkd) {
        flags += L" --whkd";
    }
    if (g_settings.komorebiMasir) {
        flags += L" --masir";
    }
    const std::wstring tool = L"\"" + komorebic + L"\"";
    const std::wstring start = tool + L" start" + flags;
    const std::wstring stop = tool + L" stop" + flags;
    return button == 0 ? start : button == 1 ? stop : stop + L" && " + start;
}

// Whether this is the 32-bit Windhawk process on 64-bit Windows: it sees
// SysWOW64 as System32, Program Files (x86) as %ProgramFiles% and the
// registry's 32-bit view.
bool IsWow64() {
    static const bool wow64 = [] {
        BOOL wow64 = FALSE;
        return IsWow64Process(GetCurrentProcess(), &wow64) && wow64;
    }();
    return wow64;
}

// A path the user gave with its environment variables, as a 64-bit program
// sees it: the 64-bit Program Files, and a file in System32 through
// Sysnative (the 32-bit process's way to the real one; a folder stays as it
// is: the 64-bit Explorer that shows it doesn't know that name). Empty if it
// can't be expanded.
std::wstring ExpandedAsNative(std::wstring text) {
    WCHAR windows[MAX_PATH];
    const bool wow64 = IsWow64() && GetWindowsDirectory(windows, MAX_PATH);
    if (wow64) {
        for (const auto& [from, to] :
             {std::pair<std::wstring_view, std::wstring_view>{
                  L"%programfiles%", L"%ProgramW6432%"},
              {L"%commonprogramfiles%", L"%CommonProgramW6432%"}}) {
            for (size_t at = 0;
                 (at = std::wstring_view(text).find(L'%', at)) !=
                 std::wstring::npos;
                 at++) {
                if (_wcsnicmp(text.c_str() + at, from.data(), from.size()) ==
                    0) {
                    text.replace(at, from.size(), to);
                    at += to.size() - 1;
                }
            }
        }
    }
    const DWORD size = ExpandEnvironmentStrings(text.c_str(), nullptr, 0);
    std::wstring expanded(size, L'\0');
    if (!size || ExpandEnvironmentStrings(text.c_str(), expanded.data(),
                                          size) != size) {
        return {};
    }
    expanded.resize(size - 1);
    if (wow64) {
        const std::wstring system = std::wstring(windows) + L"\\System32";
        if (_wcsnicmp(expanded.c_str(), system.c_str(), system.size()) == 0 &&
            (expanded.size() == system.size() ||
             expanded[system.size()] == L'\\')) {
            const std::wstring native = std::wstring(windows) +
                                        L"\\Sysnative" +
                                        expanded.substr(system.size());
            const DWORD attributes = GetFileAttributes(native.c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES &&
                !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                expanded = native;
            }
        }
    }
    return expanded;
}

// Runs a cmd line (what follows "cmd /c"), with the user's current
// environment (komorebic looks for whkd and masir on PATH): without a
// window, or in its own console window on the screen. It inherits nothing.
// `process` (if given) gets its handles. The 64-bit cmd.exe, also from the
// 32-bit Windhawk process: the 64-bit System32's tools found (wsl,
// pnputil).
bool StartKomorebiCommand(const std::wstring& line,
                          bool onScreen,
                          PROCESS_INFORMATION* process) {
    WCHAR cmd[MAX_PATH];
    const UINT length = GetWindowsDirectory(cmd, MAX_PATH);
    if (!length || length + 21 > MAX_PATH) {
        return false;
    }
    wcscat_s(cmd,
             IsWow64() ? L"\\Sysnative\\cmd.exe" : L"\\System32\\cmd.exe");
    std::wstring command = L"cmd.exe /c \"" + line + L"\"";
    void* environment = FreshEnvironment();
    STARTUPINFO startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = onScreen ? SW_SHOWNORMAL : SW_HIDE;
    PROCESS_INFORMATION started{};
    const bool ok =
        CreateProcess(cmd, command.data(), nullptr, nullptr, FALSE,
                      (onScreen ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW) |
                          (environment ? CREATE_UNICODE_ENVIRONMENT : 0),
                      environment, nullptr, &startup, &started);
    if (environment) {
        DestroyEnvironmentBlock(environment);
    }
    if (!ok) {
        IslandLog(L"komorebi's command didn't start: %u", GetLastError());
        return false;
    }
    if (process) {
        *process = started;
    } else {
        CloseHandle(started.hThread);
        CloseHandle(started.hProcess);
    }
    return true;
}

// A button's commands, on a thread of their own (which owns this, and its
// own copy of the stop event). What they print goes to a file in the mod's
// storage folder (not a pipe: komorebi, started by them, may keep their
// output open, and a pipe closed on it could fail its writes), shown in a
// terminal only if they fail (they're never run a second time: a reload
// would stop komorebi again), and deleted, or at the next run if still open.
struct KomorebiJob {
    std::wstring commands;
    int button;
    HWND notify;
    HANDLE stop;
};

// Tells the panel how it went (wParam the button, lParam 1 if it failed).
constexpr UINT WM_APP_KOMOREBI_DONE = WM_APP + 85;

// How long a command may take (komorebi starting takes a few seconds).
constexpr DWORD kKomorebiCommandMs = 60000;

DWORD WINAPI KomorebiThreadProc(LPVOID parameter) {
    std::unique_ptr<KomorebiJob> job((KomorebiJob*)parameter);
    WCHAR folder[MAX_PATH];
    WCHAR output[MAX_PATH] = L"";
    if (Wh_GetModStoragePath(folder, ARRAYSIZE(folder))) {
        CreateDirectory(folder, nullptr);
        // (Ones left by a command still running at an earlier wait's end.)
        WIN32_FIND_DATA data;
        const std::wstring base = std::wstring(folder) + L"\\";
        HANDLE find = FindFirstFile((base + L"kmb*.tmp").c_str(), &data);
        if (find != INVALID_HANDLE_VALUE) {
            do {
                DeleteFile((base + data.cFileName).c_str());
            } while (FindNextFile(find, &data));
            FindClose(find);
        }
        if (!GetTempFileName(folder, L"kmb", 0, output)) {
            output[0] = L'\0';
        }
    }
    PROCESS_INFORMATION process{};
    bool started = false;
    bool ended = false;
    DWORD exitCode = 0;
    if (output[0] &&
        StartKomorebiCommand(L"(" + job->commands + L") > \"" + output +
                                 L"\" 2>&1",
                             false, &process)) {
        started = true;
        const HANDLE waits[] = {process.hProcess, job->stop};
        ended = WaitForMultipleObjects(ARRAYSIZE(waits), waits, FALSE,
                                       kKomorebiCommandMs) == WAIT_OBJECT_0 &&
                GetExitCodeProcess(process.hProcess, &exitCode);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
    const bool stopping = WaitForSingleObject(job->stop, 0) == WAIT_OBJECT_0;
    const bool failed = !started || !ended || exitCode != 0;
    if (!stopping && !started) {
        // (Nothing ran: it runs once, on the screen, where it waits for a
        // key if it fails.)
        StartKomorebiCommand(job->commands + L" || pause", true, nullptr);
    } else if (!stopping && ended && exitCode != 0) {
        // What it printed, in a terminal on the screen (which deletes it).
        const std::wstring file = L"\"" + std::wstring(output) + L"\"";
        if (!StartKomorebiCommand(
                L"type " + file + L" & del " + file + L" & pause", true,
                nullptr)) {
            DeleteFile(output);
        }
    } else if (output[0]) {
        // (One still running at the wait's end keeps its file open.)
        DeleteFile(output);
    }
    if (!stopping && job->notify) {
        PostMessage(job->notify, WM_APP_KOMOREBI_DONE, job->button, failed);
    }
    CloseHandle(job->stop);
    return 0;
}

// A job's commands on a thread of their own, which gets its own copy of the
// stop event (`*stop` is the caller's, `*thread` the thread); false if it
// couldn't start.
bool StartKomorebiJob(const std::wstring& commands,
                      int button,
                      HWND notify,
                      HANDLE* thread,
                      HANDLE* stop) {
    HANDLE event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    HANDLE copy = nullptr;
    if (!event || !DuplicateHandle(GetCurrentProcess(), event,
                                   GetCurrentProcess(), &copy, 0, FALSE,
                                   DUPLICATE_SAME_ACCESS)) {
        if (event) {
            CloseHandle(event);
        }
        return false;
    }
    auto* job = new KomorebiJob{commands, button, notify, copy};
    *thread = CreateThread(nullptr, 0, KomorebiThreadProc, job, 0, nullptr);
    if (!*thread) {
        CloseHandle(copy);
        CloseHandle(event);
        delete job;
        return false;
    }
    *stop = event;
    return true;
}

// Its thread told to stop and waited for (a while: it owns its job, so it
// can be left to end by itself). The command itself goes on.
void EndKomorebiJob(HANDLE* thread, HANDLE* stop) {
    if (*stop) {
        SetEvent(*stop);
    }
    if (*thread) {
        WaitForSingleObject(*thread, 5000);
        CloseHandle(*thread);
        *thread = nullptr;
    }
    if (*stop) {
        CloseHandle(*stop);
        *stop = nullptr;
    }
}

// The last app window in front brought back there, when one of the island's
// own windows is (defined further down).
void FrontLastApp();

// One of Windows' 64-bit tools (the magnifier and the on-screen keyboard,
// which run with UI access; Task Manager) started through Explorer: the
// 32-bit Windhawk process sees SysWOW64 as System32, which doesn't have
// them, and Windows can't start them from the Sysnative path (a 64-bit
// service starts them). False if Explorer couldn't be started (whether the
// tool then started isn't known here, see StartMagnifier).
bool StartWithUiAccess(PCWSTR program) {
    WCHAR windows[MAX_PATH];
    if (!GetWindowsDirectory(windows, ARRAYSIZE(windows))) {
        return false;
    }
    const std::wstring explorer = std::wstring(windows) + L"\\explorer.exe";
    // (Quoted: a Windows folder with a space in its path.)
    const std::wstring path =
        L"\"" + std::wstring(windows) + L"\\System32\\" + program + L"\"";
    return (INT_PTR)ShellExecute(nullptr, L"open", explorer.c_str(),
                                 path.c_str(), nullptr, SW_SHOWNORMAL) > 32;
}

// Task Manager, started itself (Ctrl+Shift+Esc pressed for it is dropped
// with an elevated window in front, and mixes with keys held), or with
// those keys.
void OpenTaskManager() {
    if (!StartWithUiAccess(L"Taskmgr.exe")) {
        PressKeys({VK_CONTROL, VK_SHIFT, VK_ESCAPE});
    }
}

// The controls that are a shortcut (from the control center or the pill).
void RunControlShortcut(ControlId id) {
    switch (id) {
        case ControlId::Cast:
            // Casting looks for wireless displays: the Windows panel.
            PressKeys({VK_LWIN, 'K'});
            break;
        case ControlId::Captions:
            // Live captions switch on and off with their shortcut.
            PressKeys({VK_LWIN, VK_CONTROL, 'L'});
            break;
        case ControlId::Lock:
            LockWorkStation();
            break;
        // Both open for the window in front and close as it loses the focus:
        // from the control center (in front while it goes away), the app
        // that was in front before it, first.
        case ControlId::Emoji:
            FrontLastApp();
            PressKeys({VK_LWIN, VK_OEM_PERIOD});
            break;
        case ControlId::Clipboard:
            FrontLastApp();
            PressKeys({VK_LWIN, 'V'});
            break;
        case ControlId::TaskManager:
            OpenTaskManager();
            break;
        case ControlId::Calculator:
            ShellExecute(nullptr, L"open", L"calc.exe", nullptr, nullptr,
                         SW_SHOWNORMAL);
            break;
        case ControlId::Explorer:
            ShellExecute(nullptr, L"open", L"explorer.exe", nullptr, nullptr,
                         SW_SHOWNORMAL);
            break;
        case ControlId::ShowDesktop:
            // Windows' own (Win+D): pressed again, the windows come back.
            PressKeys({VK_LWIN, 'D'});
            break;
        case ControlId::OnScreenKeyboard:
            StartWithUiAccess(L"osk.exe");
            break;
        default:
            break;
    }
}

// The pill's item a control center button opens like (see
// Island::OpenFromControlCenter), or None for the control center's own.
IslandItem ControlItem(ControlId id) {
    switch (id) {
        case ControlId::Notifications:
            return IslandItem::Bell;
        case ControlId::Calendar:
            return IslandItem::Clock;
        case ControlId::Media:
            return IslandItem::Media;
        case ControlId::Power:
            return IslandItem::Power;
        case ControlId::Usage:
            return IslandItem::UsageCpu;
        case ControlId::Profiles:
            return IslandItem::Profiles;
        case ControlId::KeyboardLanguage:
            return IslandItem::InputMethod;
        case ControlId::Privacy:
            return IslandItem::PrivacyMicrophone;
        case ControlId::Drives:
            return IslandItem::Drives;
        case ControlId::Desktops:
            return IslandItem::Desktops;
        case ControlId::Folders:
            return IslandItem::Folders;
        case ControlId::Wallpaper:
            return IslandItem::Wallpaper;
        case ControlId::KomorebiLayout:
            return IslandItem::KomorebiLayout;
        case ControlId::WorldClocks:
            return IslandItem::WorldClock;
        case ControlId::NextEvent:
            return IslandItem::Countdown;
        case ControlId::RecycleBin:
            return IslandItem::RecycleBin;
        case ControlId::Timer:
        case ControlId::Stopwatch:
        case ControlId::Pomodoro:
        case ControlId::Alarms:
            return IslandItem::Timer;
        case ControlId::TrayIcons:
            return IslandItem::Overflow;
        default:
            return IslandItem::None;
    }
}

const ControlInfo& GetControlInfo(ControlId id) {
    for (const auto& info : kControls) {
        if (info.id == id) {
            return info;
        }
    }
    return kControls[0];
}

// The shortcut that minimizes the islands (islandMinimizeShortcut): its key,
// with its modifiers in the next byte; Win+J unless other keys were recorded
// (see ControlPanel::StartShortcutRecording).
constexpr int kShortcutWin = 1;
constexpr int kShortcutCtrl = 2;
constexpr int kShortcutAlt = 4;
constexpr int kShortcutShift = 8;
constexpr int kDefaultMinimizeShortcut = kShortcutWin << 8 | 'J';
std::atomic<int> g_minimizeShortcut{kDefaultMinimizeShortcut};
// Recording new keys for it: the next key pressed with Win, Ctrl or Alt goes
// to this window (as WM_APP_SHORTCUT_RECORDED, its combination; -1: Escape
// cancelled it; a key without them, its combination too, to say it needs
// one) instead of the app in front (see ShortcutKeyProc).
constexpr UINT WM_APP_SHORTCUT_RECORDED = WM_APP + 96;
std::atomic<bool> g_shortcutRecording;
std::atomic<HWND> g_shortcutRecorder;
// The low-level keyboard hook on while a shortcut is chosen or one is being
// recorded (see Island::UpdateShortcutKeys).
void UpdateShortcutHook();
std::wstring ShortcutText(int combo, bool spaced = true);
std::wstring SetMinimizeShortcut(int combo);

// The setup assistant, a tab of the settings' window: its steps (see
// ControlPanel::SetupLines), and its buttons under them (see
// ControlPanel::PressSetupButton).
enum SetupStep {
    kSetupWelcome,
    kSetupStyle,
    kSetupPill,
    kSetupHiding,
    kSetupLook,
    kSetupDone,
    kSetupSteps
};
enum SetupButton { kSetupCancel, kSetupBack, kSetupNext, kSetupButtons };
// Where it is: its step, the profile in use and every option's value
// (by IslandPref) as its tab showed, put back by its Cancel (empty while its
// tab isn't shown).
struct SetupState {
    int step = kSetupWelcome;
    int profile = -1;
    std::vector<int> from;
};

// The settings' window as it was when the islands started again (another
// style, the notches, a monitor...: they're all made again, the window too,
// with their thread): opened again where it was, on the same tab and place
// in it, by the island of the same monitor (see ReopenSettingsWindow).
struct SettingsWindowState {
    double at = -100;
    // The island's monitor (empty: the main island).
    std::wstring device;
    WINDOWPLACEMENT placement{sizeof(WINDOWPLACEMENT)};
    // Where it was on the screen (the placement's is against the main
    // monitor's work area, which the island's own strip changes with the
    // style: back a few pixels off).
    RECT rect{};
    int tab = 0;
    std::wstring search;
    double scroll = 0;
    double navScroll = 0;
    SetupState setup;
    // "Restore what changed" on an option's card (see ApplyNeeds): its
    // entry, and each change made (the option, from, to).
    int restoreEntry = -1;
    std::vector<std::array<int, 3>> restore;
};
// (One for each monitor's window open when the islands started again.)
std::vector<SettingsWindowState> g_settingsWindowStates;

