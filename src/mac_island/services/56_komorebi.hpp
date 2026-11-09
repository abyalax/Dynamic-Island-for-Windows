#pragma once
// A workspace of a monitor: its name ("" without one) and whether it has
// windows.
struct KomorebiWorkspace {
    std::wstring name;
    bool windows = false;
    bool operator==(const KomorebiWorkspace&) const = default;
};

// A monitor komorebi manages, as the pill and the menu show it.
struct KomorebiMonitor {
    // Its place in komorebi's list (its commands take it), its screen as
    // komorebi tells it ("size": left and top, then the width and height),
    // and its device's name ("DISPLAY1").
    int index = 0;
    LONG left = 0;
    LONG top = 0;
    LONG right = 0;
    LONG bottom = 0;
    std::wstring device;
    // Its workspaces (up to 9), the one shown on it, and that workspace's
    // layout (komorebi's name: "BSP", "Grid"..., or "Custom"), tiling and
    // monocle.
    std::vector<KomorebiWorkspace> workspaces;
    int workspace = 0;
    std::wstring layout;
    bool tiling = true;
    bool monocle = false;
    bool operator==(const KomorebiMonitor&) const = default;
};

struct KomorebiState {
    // Subscribed (otherwise nothing of komorebi's shows).
    bool connected = false;
    bool paused = false;
    int focusedMonitor = 0;
    std::vector<KomorebiMonitor> monitors;
    // Its tool, for the menu's commands.
    std::wstring komorebic;
    bool operator==(const KomorebiState&) const = default;
};
SRWLOCK g_komorebiLock = SRWLOCK_INIT;
KomorebiState g_komorebiState;
// The islands' copy (their thread's), taken at each WM_APP_KOMOREBI.
KomorebiState g_komorebiShown;
// Wakes the thread at once (komorebi started from the island).
HANDLE g_komorebiKick = nullptr;

void KickKomorebiWatch() {
    if (g_komorebiKick) {
        SetEvent(g_komorebiKick);
    }
}

// The most of komorebi's state read (its windows' titles are in it too).
constexpr size_t kKomorebiMaxText = 16 * 1024 * 1024;
constexpr DWORD kKomorebiPipeBuffer = 64 * 1024;
// Subscribing again to the same komorebi after failing (it may not take
// commands yet as it starts): this many times, a second apart.
constexpr int kKomorebiTries = 3;

// A whole number from komorebi's state, within a sane range (it's another
// program's data).
int KomorebiInt(const Json& value, int otherwise = 0) {
    return value.type == Json::Type::Number && value.number > -1e6 &&
                   value.number < 1e6
               ? (int)value.number
               : otherwise;
}

// How many a ring of komorebi's state holds (a list, or {"elements": [...]}).
size_t KomorebiCount(const Json& ring) {
    return ring.type == Json::Type::Array ? ring.Size()
                                          : ring[L"elements"].Size();
}

// komorebi's state (its JSON) as the island keeps it; false if it isn't one.
bool ReadKomorebiState(const Json& state, KomorebiState* read) {
    const Json& monitors = state[L"monitors"];
    if (monitors.type != Json::Type::Object) {
        return false;
    }
    read->paused = state[L"is_paused"].type == Json::Type::Bool &&
                   state[L"is_paused"].boolean;
    read->focusedMonitor = KomorebiInt(monitors[L"focused"]);
    const Json& elements = monitors[L"elements"];
    for (size_t i = 0; i < elements.Size() && i < 16; i++) {
        const Json& monitor = elements[i];
        KomorebiMonitor entry;
        entry.index = (int)i;
        const Json& size = monitor[L"size"];
        entry.left = KomorebiInt(size[L"left"]);
        entry.top = KomorebiInt(size[L"top"]);
        entry.right = KomorebiInt(size[L"right"]);
        entry.bottom = KomorebiInt(size[L"bottom"]);
        if (monitor[L"name"].type == Json::Type::String) {
            entry.device = monitor[L"name"].string.substr(0, 64);
        }
        const Json& workspaces = monitor[L"workspaces"];
        for (size_t j = 0; j < workspaces[L"elements"].Size() && j < 9; j++) {
            const Json& one = workspaces[L"elements"][j];
            KomorebiWorkspace workspace;
            if (one[L"name"].type == Json::Type::String) {
                workspace.name = one[L"name"].string.substr(0, 32);
            }
            workspace.windows =
                KomorebiCount(one[L"containers"]) > 0 ||
                KomorebiCount(one[L"floating_windows"]) > 0 ||
                one[L"monocle_container"].type == Json::Type::Object ||
                one[L"maximized_window"].type == Json::Type::Object;
            entry.workspaces.push_back(std::move(workspace));
        }
        entry.workspace = std::max(0, KomorebiInt(workspaces[L"focused"]));
        const Json& workspace = workspaces[L"elements"][entry.workspace];
        // {"Default": "BSP"}, or {"Custom": ...}.
        const Json& layout = workspace[L"layout"];
        if (layout.type == Json::Type::Object) {
            const Json& known = layout[L"Default"];
            entry.layout = known.type == Json::Type::String
                               ? known.string.substr(0, 64)
                               : L"Custom";
        } else if (layout.type == Json::Type::String) {
            entry.layout = layout.string.substr(0, 64);
        }
        entry.tiling = workspace[L"tile"].type != Json::Type::Bool ||
                       workspace[L"tile"].boolean;
        entry.monocle =
            workspace[L"monocle_container"].type == Json::Type::Object;
        read->monitors.push_back(std::move(entry));
    }
    return true;
}

// A JSON text (UTF-8) from komorebi: its state, under "state" in what it
// writes into the pipe; false if it isn't one.
bool ReadKomorebiText(const char* text,
                      size_t size,
                      bool notification,
                      KomorebiState* state) {
    if (!size || size > kKomorebiMaxText) {
        return false;
    }
    const int length =
        MultiByteToWideChar(CP_UTF8, 0, text, (int)size, nullptr, 0);
    if (length <= 0) {
        return false;
    }
    std::wstring wide(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, (int)size, wide.data(), length);
    Json json;
    return JsonReader(wide).Read(&json) &&
           ReadKomorebiState(notification ? json[L"state"] : json, state);
}

// komorebi's layouts, in the menu's order: its name in the state, in its
// commands, and shown.
struct KomorebiLayoutInfo {
    PCWSTR id;
    PCWSTR command;
    PCWSTR english;
    PCWSTR portuguese;
};
constexpr KomorebiLayoutInfo kKomorebiLayouts[] = {
    {L"Columns", L"columns", L"Columns", L"Colunas"},
    {L"Rows", L"rows", L"Rows", L"Linhas"},
    {L"Grid", L"grid", L"Grid", L"Grade"},
    {L"BSP", L"bsp", L"BSP", L"BSP"},
    {L"VerticalStack", L"vertical-stack", L"Vertical stack",
     L"Pilha vertical"},
    {L"HorizontalStack", L"horizontal-stack", L"Horizontal stack",
     L"Pilha horizontal"},
    {L"UltrawideVerticalStack", L"ultrawide-vertical-stack",
     L"Ultrawide vertical stack", L"Pilha vertical ultrawide"},
    {L"RightMainVerticalStack", L"right-main-vertical-stack",
     L"Vertical stack, main on the right",
     L"Pilha vertical, principal à direita"},
    {L"Scrolling", L"scrolling", L"Scrolling", L"Rolagem"},
};
constexpr int kKomorebiLayoutCount = (int)ARRAYSIZE(kKomorebiLayouts);
// The menu's rows after the layouts (see KomorebiPanel).
constexpr int kKomorebiRowTiling = kKomorebiLayoutCount;
constexpr int kKomorebiRowMonocle = kKomorebiLayoutCount + 1;
constexpr int kKomorebiRowPause = kKomorebiLayoutCount + 2;
constexpr int kKomorebiRows = kKomorebiLayoutCount + 3;
// (A menu command's KomorebiJob::button.)
constexpr int kKomorebiMenuButton = 3;

// What the pill shows for a monitor: komorebi paused, then monocle, then
// tiling off, then the layout.
std::wstring KomorebiLayoutName(const KomorebiMonitor& monitor, bool paused) {
    if (paused) {
        return Tr(L"Paused", L"Pausado");
    }
    if (monitor.monocle) {
        return L"Monocle";
    }
    if (!monitor.tiling) {
        return Tr(L"Floating", L"Flutuante");
    }
    for (const auto& info : kKomorebiLayouts) {
        if (monitor.layout == info.id) {
            return Tr(info.english, info.portuguese);
        }
    }
    if (monitor.layout == L"Custom") {
        return Tr(L"Custom", L"Personalizado");
    }
    return monitor.layout;
}

// The icons of komorebi's menu rows (a layout's place in kKomorebiLayouts,
// then tiling, monocle and pause) and of tiling off (floating).
constexpr int kKomorebiIconFloating = kKomorebiRows;

// What the pill shows for a monitor, as an icon (see KomorebiLayoutName); -1
// for a layout without one (a custom one).
int KomorebiIconOf(const KomorebiMonitor& monitor, bool paused) {
    if (paused) {
        return kKomorebiRowPause;
    }
    if (monitor.monocle) {
        return kKomorebiRowMonocle;
    }
    if (!monitor.tiling) {
        return kKomorebiIconFloating;
    }
    for (int i = 0; i < kKomorebiLayoutCount; i++) {
        if (monitor.layout == kKomorebiLayouts[i].id) {
            return i;
        }
    }
    return -1;
}

// A komorebi icon (see KomorebiIconOf) around `middle`: a small screen split
// the way the layout places the windows (like yasb's), in the brush's color.
void DrawKomorebiIcon(ID2D1RenderTarget* target,
                      ID2D1SolidColorBrush* brush,
                      D2D1_POINT_2F middle,
                      float scale,
                      int icon) {
    const float w = std::round(16 * scale);
    const float h = std::round(12 * scale);
    const float stroke = std::max(1.0f, 1.2f * scale);
    const D2D1_RECT_F frame{std::round(middle.x - w / 2) + 0.5f,
                            std::round(middle.y - h / 2) + 0.5f,
                            std::round(middle.x - w / 2) + 0.5f + w,
                            std::round(middle.y - h / 2) + 0.5f + h};
    const float radius = 2 * scale;
    auto x = [&](float f) { return frame.left + (frame.right - frame.left) * f; };
    auto y = [&](float f) { return frame.top + (frame.bottom - frame.top) * f; };
    // A line across the frame at `at`, from `from` to `to` (fractions).
    auto vertical = [&](float at, float from = 0, float to = 1) {
        target->DrawLine({x(at), y(from)}, {x(at), y(to)}, brush, stroke);
    };
    auto horizontal = [&](float at, float from = 0, float to = 1) {
        target->DrawLine({x(from), y(at)}, {x(to), y(at)}, brush, stroke);
    };
    if (icon == kKomorebiRowPause) {
        const float bar = std::round(3 * scale);
        const float gap = std::round(3 * scale);
        const float top = middle.y - h / 2;
        target->FillRectangle({middle.x - gap / 2 - bar, top,
                               middle.x - gap / 2, top + h},
                              brush);
        target->FillRectangle({middle.x + gap / 2, top,
                               middle.x + gap / 2 + bar, top + h},
                              brush);
        return;
    }
    if (icon == kKomorebiIconFloating) {
        // Two windows over each other, not tiled.
        const D2D1_RECT_F back{frame.left, frame.top, x(0.7f), y(0.7f)};
        const D2D1_RECT_F front{x(0.3f), y(0.3f), frame.right, frame.bottom};
        target->DrawRoundedRectangle({back, radius, radius}, brush, stroke);
        target->FillRoundedRectangle({front, radius, radius}, brush);
        return;
    }
    // Scrolling: columns going past the screen's sides (no sides drawn).
    if (icon == 8) {
        horizontal(0);
        horizontal(1);
        vertical(0.18f);
        vertical(0.82f);
        return;
    }
    target->DrawRoundedRectangle({frame, radius, radius}, brush, stroke);
    switch (icon) {
        case 0:  // Columns.
            vertical(1 / 3.0f);
            vertical(2 / 3.0f);
            break;
        case 1:  // Rows.
            horizontal(1 / 3.0f);
            horizontal(2 / 3.0f);
            break;
        case 2:  // Grid.
            vertical(0.5f);
            horizontal(0.5f);
            break;
        case 3:  // BSP: each half split again.
            vertical(0.5f);
            horizontal(0.5f, 0.5f, 1);
            vertical(0.75f, 0.5f, 1);
            break;
        case 4:  // Vertical stack: the main one on the left.
            vertical(0.5f);
            horizontal(0.5f, 0.5f, 1);
            break;
        case 5:  // Horizontal stack: the main one on top.
            horizontal(0.5f);
            vertical(0.5f, 0.5f, 1);
            break;
        case 6:  // Ultrawide: the main one in the middle.
            vertical(0.25f);
            vertical(0.75f);
            horizontal(0.5f, 0.75f, 1);
            break;
        case 7:  // The main one on the right.
            vertical(0.5f);
            horizontal(0.5f, 0, 0.5f);
            break;
        case kKomorebiRowTiling: {
            // Tiled: the screen split, one tile filled.
            vertical(0.5f);
            horizontal(0.5f, 0.5f, 1);
            const float inset = stroke * 1.5f;
            target->FillRectangle({frame.left + inset, frame.top + inset,
                                   x(0.5f) - inset, frame.bottom - inset},
                                  brush);
            break;
        }
        case kKomorebiRowMonocle: {
            // One window filling the screen.
            const float inset = std::round(2.5f * scale);
            target->FillRoundedRectangle(
                {{frame.left + inset, frame.top + inset, frame.right - inset,
                  frame.bottom - inset},
                 radius / 2,
                 radius / 2},
                brush);
            break;
        }
        default:
            break;
    }
}

// A menu row's command for a monitor (its shown workspace), without moving
// the focus; monocle takes the window in focus, so this monitor's first
// when another one has it.
std::wstring KomorebiMenuCommand(const std::wstring& komorebic,
                                 const KomorebiMonitor& monitor,
                                 int focusedMonitor,
                                 int row) {
    const std::wstring tool = L"\"" + komorebic + L"\"";
    const std::wstring place = std::to_wstring(monitor.index) + L" " +
                               std::to_wstring(monitor.workspace);
    if (row >= 0 && row < kKomorebiLayoutCount) {
        return tool + L" workspace-layout " + place + L" " +
               kKomorebiLayouts[row].command;
    }
    switch (row) {
        case kKomorebiRowTiling:
            return tool + L" workspace-tiling " + place +
                   (monitor.tiling ? L" disable" : L" enable");
        case kKomorebiRowMonocle:
            return (focusedMonitor == monitor.index
                        ? std::wstring()
                        : tool + L" focus-monitor " +
                              std::to_wstring(monitor.index) + L" && ") +
                   tool + L" toggle-monocle";
        case kKomorebiRowPause:
            return tool + L" toggle-pause";
        default:
            return {};
    }
}

// A workspace of a monitor shown (its dot on the pill clicked).
std::wstring KomorebiWorkspaceCommand(const std::wstring& komorebic,
                                      const KomorebiMonitor& monitor,
                                      int workspace) {
    return L"\"" + komorebic + L"\" focus-monitor-workspace " +
           std::to_wstring(monitor.index) + L" " + std::to_wstring(workspace);
}

// A workspace's name, as its dot's tip says it.
std::wstring KomorebiWorkspaceName(const KomorebiMonitor& monitor,
                                   int workspace) {
    if (workspace >= 0 && workspace < (int)monitor.workspaces.size() &&
        !monitor.workspaces[workspace].name.empty()) {
        return monitor.workspaces[workspace].name;
    }
    return std::wstring(Tr(L"Workspace", L"Área de trabalho")) + L" " +
           std::to_wstring(workspace + 1);
}

// The monitor komorebi knows as this screen: by its place and size (as
// komorebi gives it, or as a rectangle), else by its device's name
// ("DISPLAY1" in "\\.\DISPLAY1").
const KomorebiMonitor* FindKomorebiMonitor(const KomorebiState& state,
                                           const RECT& rect,
                                           PCWSTR device) {
    for (const auto& monitor : state.monitors) {
        if (monitor.left == rect.left && monitor.top == rect.top &&
            (monitor.right == rect.right - rect.left ||
             monitor.right == rect.right) &&
            (monitor.bottom == rect.bottom - rect.top ||
             monitor.bottom == rect.bottom)) {
            return &monitor;
        }
    }
    const size_t length = wcslen(device);
    for (const auto& monitor : state.monitors) {
        const size_t name = monitor.device.size();
        if (name && name <= length &&
            _wcsicmp(device + length - name, monitor.device.c_str()) == 0) {
            return &monitor;
        }
    }
    return nullptr;
}

// The monitor komorebi manages on an island's screen (none while komorebi
// isn't subscribed to).
const KomorebiMonitor* KomorebiMonitorOf(HMONITOR screen) {
    if (!g_komorebiShown.connected || !screen) {
        return nullptr;
    }
    MONITORINFOEX info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfo(screen, &info)) {
        return nullptr;
    }
    return FindKomorebiMonitor(g_komorebiShown, info.rcMonitor, info.szDevice);
}

struct KomorebiWatchJob {
    HWND notify;
    // Its own copies (closed by the thread).
    HANDLE stop;
    HANDLE kick;
};

// A command line run hidden and waited for (up to `timeout`, or until the
// stop): its exit code, or -1.
DWORD RunKomorebiHidden(const std::wstring& line, HANDLE stop, DWORD timeout) {
    PROCESS_INFORMATION process{};
    if (!StartKomorebiCommand(line, false, &process)) {
        return (DWORD)-1;
    }
    DWORD exitCode = (DWORD)-1;
    const HANDLE waits[] = {process.hProcess, stop};
    if (WaitForMultipleObjects(ARRAYSIZE(waits), waits, FALSE, timeout) !=
            WAIT_OBJECT_0 ||
        !GetExitCodeProcess(process.hProcess, &exitCode)) {
        exitCode = (DWORD)-1;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return exitCode;
}

// Kept for the islands; told to them only when it changed.
void PublishKomorebiState(HWND notify, KomorebiState state) {
    AcquireSRWLockExclusive(&g_komorebiLock);
    const bool changed = !(state == g_komorebiState);
    g_komorebiState = std::move(state);
    ReleaseSRWLockExclusive(&g_komorebiLock);
    if (changed) {
        PostMessage(notify, WM_APP_KOMOREBI, 0, 0);
    }
}

// One subscription: the island's pipe (komorebi connects to it and writes
// its state at every change), komorebi told about it and its state read now
// (into a file of the mod's storage, deleted after), then what komorebi
// writes until it goes (it stopped) or the thread is told to stop. False if
// komorebi never connected.
bool KomorebiSession(const KomorebiWatchJob& job,
                     const std::wstring& komorebic,
                     int session) {
    const std::wstring name = L"mac-island-dock-" +
                              std::to_wstring(GetCurrentProcessId()) + L"-" +
                              std::to_wstring(session);
    HANDLE pipe = CreateNamedPipe(
        (L"\\\\.\\pipe\\" + name).c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
            FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT |
            PIPE_REJECT_REMOTE_CLIENTS,
        1, kKomorebiPipeBuffer, kKomorebiPipeBuffer, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }
    OVERLAPPED overlapped{};
    overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!overlapped.hEvent) {
        CloseHandle(pipe);
        return false;
    }
    // The pipe's operation done (true), or not by `timeout` or the stop: then
    // cancelled and waited for, so nothing writes into `overlapped` after.
    auto finish = [&](DWORD timeout) {
        const HANDLE waits[] = {overlapped.hEvent, job.stop};
        if (WaitForMultipleObjects(ARRAYSIZE(waits), waits, FALSE, timeout) ==
            WAIT_OBJECT_0) {
            return true;
        }
        DWORD bytes = 0;
        CancelIoEx(pipe, &overlapped);
        GetOverlappedResult(pipe, &overlapped, &bytes, TRUE);
        return false;
    };
    bool connected = ConnectNamedPipe(pipe, &overlapped) != FALSE;
    const DWORD error = connected ? ERROR_SUCCESS : GetLastError();
    connected = connected || error == ERROR_PIPE_CONNECTED;
    const bool pending = error == ERROR_IO_PENDING;
    WCHAR folder[MAX_PATH];
    WCHAR output[MAX_PATH] = L"";
    if ((connected || pending) &&
        Wh_GetModStoragePath(folder, ARRAYSIZE(folder))) {
        CreateDirectory(folder, nullptr);
        if (!GetTempFileName(folder, L"kms", 0, output)) {
            output[0] = L'\0';
        }
    }
    if (connected || pending) {
        const std::wstring tool = L"\"" + komorebic + L"\"";
        const DWORD exitCode = RunKomorebiHidden(
            tool + L" subscribe-pipe " + name +
                (output[0] ? L" && " + tool + L" state > \"" + output + L"\""
                           : std::wstring()),
            job.stop, 10000);
        if (pending) {
            DWORD bytes = 0;
            connected =
                exitCode == 0 && finish(5000) &&
                GetOverlappedResult(pipe, &overlapped, &bytes, FALSE);
            if (exitCode != 0) {
                CancelIoEx(pipe, &overlapped);
                GetOverlappedResult(pipe, &overlapped, &bytes, TRUE);
            }
        }
        connected = connected && exitCode == 0;
    }
    // Its state now (then each change).
    if (connected && output[0]) {
        HANDLE file = CreateFile(output, GENERIC_READ, FILE_SHARE_READ,
                                 nullptr, OPEN_EXISTING, 0, nullptr);
        LARGE_INTEGER size{};
        if (file != INVALID_HANDLE_VALUE && GetFileSizeEx(file, &size) &&
            size.QuadPart > 0 && (size_t)size.QuadPart <= kKomorebiMaxText) {
            std::string text((size_t)size.QuadPart, '\0');
            DWORD read = 0;
            KomorebiState state;
            if (ReadFile(file, text.data(), (DWORD)text.size(), &read,
                         nullptr) &&
                ReadKomorebiText(text.data(), read, false, &state)) {
                state.connected = true;
                state.komorebic = komorebic;
                PublishKomorebiState(job.notify, std::move(state));
            }
        }
        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
        }
    }
    if (output[0]) {
        DeleteFile(output);
    }
    // Each message is a change (in pieces when longer than the buffer).
    std::string message;
    std::vector<char> buffer(kKomorebiPipeBuffer);
    bool tooLong = false;
    while (connected) {
        DWORD bytes = 0;
        ResetEvent(overlapped.hEvent);
        if (!ReadFile(pipe, buffer.data(), (DWORD)buffer.size(), nullptr,
                      &overlapped)) {
            const DWORD readError = GetLastError();
            if (readError == ERROR_IO_PENDING) {
                if (!finish(INFINITE)) {
                    break;
                }
            } else if (readError != ERROR_MORE_DATA) {
                break;
            }
        }
        const bool whole =
            GetOverlappedResult(pipe, &overlapped, &bytes, FALSE) != FALSE;
        if (!whole && GetLastError() != ERROR_MORE_DATA) {
            break;
        }
        if (message.size() + bytes > kKomorebiMaxText) {
            tooLong = true;
        } else {
            message.append(buffer.data(), bytes);
        }
        if (whole) {
            KomorebiState state;
            if (!tooLong && ReadKomorebiText(message.data(), message.size(),
                                             true, &state)) {
                state.connected = true;
                state.komorebic = komorebic;
                PublishKomorebiState(job.notify, std::move(state));
            }
            message.clear();
            tooLong = false;
        }
    }
    CloseHandle(overlapped.hEvent);
    CloseHandle(pipe);
    // (Gone: nothing of komorebi's shows until it's back.)
    if (connected && WaitForSingleObject(job.stop, 0) != WAIT_OBJECT_0) {
        PublishKomorebiState(job.notify, {});
    }
    return connected;
}

DWORD WINAPI KomorebiWatchThreadProc(LPVOID parameter) {
    std::unique_ptr<KomorebiWatchJob> job((KomorebiWatchJob*)parameter);
    std::wstring local;
    PWSTR path = nullptr;
    if (SUCCEEDED(
            SHGetKnownFolderPath(kFOLDERID_LocalAppData, 0, nullptr, &path))) {
        local = path;
    }
    CoTaskMemFree(path);
    const std::wstring data = local + L"\\komorebi";
    // (State files left by an earlier run that didn't end.)
    WCHAR storage[MAX_PATH];
    if (Wh_GetModStoragePath(storage, ARRAYSIZE(storage))) {
        const std::wstring base = std::wstring(storage) + L"\\";
        WIN32_FIND_DATA found;
        HANDLE find = FindFirstFile((base + L"kms*.tmp").c_str(), &found);
        if (find != INVALID_HANDLE_VALUE) {
            do {
                DeleteFile((base + found.cFileName).c_str());
            } while (FindNextFile(find, &found));
            FindClose(find);
        }
    }
    int sessions = 0;
    DWORD failedProcess = 0;
    int failures = 0;
    while (WaitForSingleObject(job->stop, 0) == WAIT_TIMEOUT) {
        // Given up on this komorebi for now (it didn't take the subscription,
        // like one still starting at sign-in): tried again every 30 seconds,
        // at once when it ends or the island starts it (not woken by its
        // folder, where it keeps writing its log).
        if (failures >= kKomorebiTries) {
            // (Gone: looked at again at once. One that can't be opened, like
            // komorebi run as administrator, is waited for all the same.)
            HANDLE running = OpenProcess(SYNCHRONIZE, FALSE, failedProcess);
            const bool gone =
                !running && GetLastError() == ERROR_INVALID_PARAMETER;
            DWORD woken = WAIT_TIMEOUT;
            if (!gone) {
                const HANDLE waits[] = {job->stop, job->kick, running};
                woken = WaitForMultipleObjects(running ? 3 : 2, waits, FALSE,
                                               30000);
            }
            if (running) {
                CloseHandle(running);
            }
            if (woken != WAIT_OBJECT_0) {
                failures = 0;
                failedProcess = 0;
            }
            continue;
        }
        // Watched before looking, so komorebi starting in between isn't
        // missed: its folder (or, before it has one, where it'll be made).
        const DWORD attributes = GetFileAttributes(data.c_str());
        const bool hasData = attributes != INVALID_FILE_ATTRIBUTES &&
                             (attributes & FILE_ATTRIBUTE_DIRECTORY);
        HANDLE change =
            local.empty()
                ? INVALID_HANDLE_VALUE
                : FindFirstChangeNotification(
                      (hasData ? data : local).c_str(), FALSE,
                      hasData ? FILE_NOTIFY_CHANGE_FILE_NAME |
                                    FILE_NOTIFY_CHANGE_LAST_WRITE
                              : FILE_NOTIFY_CHANGE_DIR_NAME);
        const DWORD process = KomorebiProcessId();
        if (process != failedProcess) {
            failures = 0;
        }
        const std::wstring komorebic =
            process ? FindKomorebic() : std::wstring();
        if (process) {
            if (change != INVALID_HANDLE_VALUE) {
                FindCloseChangeNotification(change);
            }
            // (Running without its komorebic found: as given up.)
            if (komorebic.empty()) {
                failedProcess = process;
                failures = kKomorebiTries;
                continue;
            }
            // (A subscription komorebi drops at once counts as failed: no
            // command run over and over.)
            const ULONGLONG start = GetTickCount64();
            if (KomorebiSession(*job, komorebic, ++sessions) &&
                GetTickCount64() - start > 5000) {
                failures = 0;
            } else {
                failedProcess = process;
                failures++;
                const HANDLE waits[] = {job->stop, job->kick};
                WaitForMultipleObjects(ARRAYSIZE(waits), waits, FALSE, 1000);
            }
            continue;
        }
        // Until komorebi starts (or the island starts it).
        HANDLE waits[3] = {job->stop, job->kick, change};
        const DWORD count = change != INVALID_HANDLE_VALUE ? 3 : 2;
        if (WaitForMultipleObjects(count, waits, FALSE, INFINITE) ==
            WAIT_OBJECT_0 + 1) {
            failures = 0;
            failedProcess = 0;
        }
        if (change != INVALID_HANDLE_VALUE) {
            FindCloseChangeNotification(change);
        }
    }
    CloseHandle(job->stop);
    CloseHandle(job->kick);
    return 0;
}

// The menu of komorebi's layout item: the layouts (the one shown marked),
// then tiling, monocle and pause (marked when on), for the island's monitor.
// A click runs its command and closes the menu; the pill shows the result
// when komorebi tells it.
constexpr float kKomorebiMenuWidth = 280;

class KomorebiPanel : public Panel {
   public:
    // The screen of the island it opens from (its commands go there).
    void SetScreen(HMONITOR screen) { m_screen = screen; }
    // A command for komorebi, hidden (not while another one runs).
    void RunCommand(const std::wstring& command) {
        if (!m_thread && !command.empty()) {
            StartKomorebiJob(command, kKomorebiMenuButton, m_hwnd, &m_thread,
                             &m_stop);
        }
    }
    // komorebi's state changed: drawn again, or closed when komorebi no
    // longer manages this monitor.
    void Refresh() {
        if (!IsOpen()) {
            return;
        }
        if (!KomorebiMonitorOf(m_screen)) {
            Close();
            return;
        }
        StartIslandAnimation();
    }

   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_iconFormat = MakeFormat(IconFont(),
                                      DWRITE_FONT_WEIGHT_NORMAL, 14 * scale);
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           13 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            if (m_textFormat) {
                m_textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER,
                                         0, 0};
                m_textFormat->SetTrimming(&trimming, nullptr);
            }
        }
        Layout();
    }

    void OnOpen() override {
        m_hover = -1;
        Layout();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

    bool OnMouseMove(POINT pt) override {
        const int hover = HitTest(pt);
        if (hover == m_hover) {
            return false;
        }
        m_hover = hover;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hover < 0) {
            return false;
        }
        m_hover = -1;
        return true;
    }

    bool OnMouseUp(POINT pt) override {
        const int hit = HitTest(pt);
        const KomorebiMonitor* monitor = KomorebiMonitorOf(m_screen);
        // (One command at a time.)
        if (hit < 0 || !monitor || m_thread) {
            return false;
        }
        const std::wstring command =
            KomorebiMenuCommand(g_komorebiShown.komorebic, *monitor,
                                g_komorebiShown.focusedMonitor, hit);
        Close();
        RunCommand(command);
        return false;
    }

    // Its command ended (a failure is shown in a terminal).
    bool OnAppMessage(UINT msg, WPARAM, LPARAM) override {
        if (msg == WM_APP_KOMOREBI_DONE) {
            EndKomorebiJob(&m_thread, &m_stop);
        }
        return false;
    }

    void OnDestroy() override { EndKomorebiJob(&m_thread, &m_stop); }

   private:
    void Layout() {
        const float s = m_scale;
        const float width = std::round(kKomorebiMenuWidth * s);
        const float padding = std::round(6 * s);
        const float row = std::round(34 * s);
        const float separator = std::round(9 * s);
        float y = padding;
        m_rows.clear();
        for (int i = 0; i < kKomorebiRows; i++) {
            if (i == kKomorebiRowTiling) {
                m_separatorY = y + separator / 2;
                y += separator;
            }
            m_rows.push_back({padding, y, width - padding, y + row});
            y += row;
        }
        y += padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    int HitTest(POINT pt) const {
        for (size_t i = 0; i < m_rows.size(); i++) {
            if (PointInRect(pt, m_rows[i])) {
                return (int)i;
            }
        }
        return -1;
    }

    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    float m_preparedScale = 0;
    std::vector<D2D1_RECT_F> m_rows;
    float m_separatorY = 0;
    int m_hover = -1;
    HMONITOR m_screen = nullptr;
    // The command running (see StartKomorebiJob).
    HANDLE m_thread = nullptr;
    HANDLE m_stop = nullptr;
};

void KomorebiPanel::Draw(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush) {
    if (!m_iconFormat || !m_textFormat) {
        return;
    }
    const KomorebiMonitor* monitor = KomorebiMonitorOf(m_screen);
    const float s = m_scale;
    const float radius = std::round(9 * s);
    const float icon = std::round(34 * s);
    // Each row: its icon, its name, and a check at the end when on.
    auto drawRow = [&](int row, PCWSTR name, bool checked) {
        if (row >= (int)m_rows.size()) {
            return;
        }
        const D2D1_RECT_F& rect = m_rows[row];
        const float lit = Hover(HoverKey(6200, row), m_hover == row);
        if (lit > 0.01f) {
            brush->SetColor(Fg(0.09f * lit));
            GlassFill(target, {rect, radius, radius}, brush);
        }
        brush->SetColor(checked ? Accent(1) : Fg(0.85f));
        DrawKomorebiIcon(target, brush,
                         {rect.left + icon / 2, (rect.top + rect.bottom) / 2},
                         s, row);
        const float checkWidth = std::round(28 * s);
        if (checked) {
            const WCHAR check = 0xE73E;
            brush->SetColor(Accent(1));
            DrawGlyph(target, &check, 1, m_iconFormat.get(),
                      {rect.right - checkWidth, rect.top, rect.right,
                       rect.bottom},
                      brush);
        }
        brush->SetColor(Fg(0.95f));
        target->DrawText(name, (UINT32)wcslen(name), m_textFormat.get(),
                         {rect.left + icon, rect.top,
                          rect.right - checkWidth, rect.bottom},
                         brush);
    };
    for (int i = 0; i < kKomorebiLayoutCount; i++) {
        drawRow(i,
                Tr(kKomorebiLayouts[i].english, kKomorebiLayouts[i].portuguese),
                monitor && monitor->layout == kKomorebiLayouts[i].id);
    }
    brush->SetColor(Fg(0.10f));
    target->FillRectangle({std::round(14 * s), m_separatorY,
                           (float)m_size.cx - std::round(14 * s),
                           m_separatorY + std::max(1.0f, std::round(s))},
                          brush);
    drawRow(kKomorebiRowTiling, Tr(L"Tiling", L"Tiling (organizar as janelas)"),
            monitor && monitor->tiling);
    drawRow(kKomorebiRowMonocle,
            Tr(L"Monocle (the window in focus fills the screen)",
               L"Monocle (a janela em foco ocupa a tela)"),
            monitor && monitor->monocle);
    drawRow(kKomorebiRowPause,
            Tr(L"Paused (every monitor)", L"Pausado (todos os monitores)"),
            g_komorebiShown.paused);
}

// An item's width on the pill following its name's (see Island::Layout):
// shown (growing or shrinking), wanted, and the name measured last; and the
// name with its icon (an app's key, or "" for the item's glyph) shown, the
// one before sliding up and out as it comes up from below (0 to 1).
struct ItemWidth {
    double shown = -1;
    double velocity = 0;
    float target = 0;
    std::wstring measuredKey;
    float measured = 0;
    std::wstring name;
    std::wstring icon;
    std::wstring nameBefore;
    std::wstring iconBefore;
    double swap = 0;
    double swapVelocity = 0;
};

// Whether a window is hidden by Windows (another virtual desktop, a
// suspended app).
bool IsCloakedWindow(HWND window) {
    BOOL cloaked = FALSE;
    return SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked,
                                           sizeof(cloaked))) &&
           cloaked;
}

// The desktop's windows (its icons' window behind the apps).
bool IsDesktopWindow(HWND window) {
    WCHAR className[16] = L"";
    GetClassName(window, className, ARRAYSIZE(className));
    return _wcsicmp(className, L"Progman") == 0 ||
           _wcsicmp(className, L"WorkerW") == 0;
}

HWND FindTopAppWindow(HMONITOR screen);

// Virtual desktops (an option): Windows keeps them in the registry (their
// IDs in order, the current one's, and the names given to them), watched
// for changes on a thread (RegNotifyChangeKeyValue, no polling). Switching
// is Windows' own shortcut (Ctrl+Win+Left or Right), pressed as many times as
// it takes; nothing undocumented is called.
constexpr WCHAR kVirtualDesktopsKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktops";

struct VirtualDesktops {
    int current = -1;
    // Each one's name ("" for one without a name of its own).
    std::vector<std::wstring> names;
    bool operator==(const VirtualDesktops&) const = default;
};
SRWLOCK g_desktopsLock = SRWLOCK_INIT;
VirtualDesktops g_desktops;
// Tells the island they changed (see g_desktops).
constexpr UINT WM_APP_DESKTOPS = WM_APP + 78;

VirtualDesktops ReadVirtualDesktops() {
    VirtualDesktops desktops;
    BYTE ids[64 * sizeof(GUID)];
    DWORD size = sizeof(ids);
    if (RegGetValue(HKEY_CURRENT_USER, kVirtualDesktopsKey,
                    L"VirtualDesktopIDs", RRF_RT_REG_BINARY, nullptr, ids,
                    &size) != ERROR_SUCCESS) {
        return desktops;
    }
    GUID current{};
    DWORD currentSize = sizeof(current);
    const bool hasCurrent =
        RegGetValue(HKEY_CURRENT_USER, kVirtualDesktopsKey,
                    L"CurrentVirtualDesktop", RRF_RT_REG_BINARY, nullptr,
                    &current, &currentSize) == ERROR_SUCCESS &&
        currentSize == sizeof(current);
    for (DWORD at = 0; at + sizeof(GUID) <= size; at += sizeof(GUID)) {
        GUID id;
        memcpy(&id, ids + at, sizeof(id));
        if (hasCurrent && IsEqualGUID(id, current)) {
            desktops.current = (int)desktops.names.size();
        }
        WCHAR text[64];
        StringFromGUID2(id, text, ARRAYSIZE(text));
        const std::wstring key =
            std::wstring(kVirtualDesktopsKey) + L"\\Desktops\\" + text;
        WCHAR name[256] = L"";
        DWORD nameSize = sizeof(name);
        RegGetValue(HKEY_CURRENT_USER, key.c_str(), L"Name", RRF_RT_REG_SZ,
                    nullptr, name, &nameSize);
        desktops.names.push_back(name);
    }
    // One desktop only: Windows may not list it.
    if (desktops.names.empty()) {
        desktops.names.push_back({});
        desktops.current = 0;
    }
    return desktops;
}

// A desktop's name: its own, or "Desktop 2".
std::wstring DesktopName(const VirtualDesktops& desktops, int index) {
    if (index < 0 || index >= (int)desktops.names.size()) {
        return {};
    }
    if (!desktops.names[index].empty()) {
        return desktops.names[index];
    }
    return std::wstring(Tr(L"Desktop", L"Área de trabalho")) + L" " +
           std::to_wstring(index + 1);
}

struct DesktopsJob {
    HWND notify;
    HANDLE stop;
};

DWORD WINAPI DesktopsThreadProc(LPVOID parameter) {
    std::unique_ptr<DesktopsJob> job((DesktopsJob*)parameter);
    auto tell = [&] {
        VirtualDesktops desktops = ReadVirtualDesktops();
        AcquireSRWLockExclusive(&g_desktopsLock);
        const bool different = !(desktops == g_desktops);
        g_desktops = std::move(desktops);
        ReleaseSRWLockExclusive(&g_desktopsLock);
        if (different) {
            PostMessage(job->notify, WM_APP_DESKTOPS, 0, 0);
        }
    };
    HANDLE changed = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!changed) {
        return 0;
    }
    // Windows makes its key once a desktop is added: until then it's looked
    // for again every few seconds (the one desktop shown meanwhile).
    HKEY key = nullptr;
    while (RegOpenKeyEx(HKEY_CURRENT_USER, kVirtualDesktopsKey, 0,
                        KEY_READ | KEY_NOTIFY, &key) != ERROR_SUCCESS) {
        key = nullptr;
        tell();
        if (WaitForSingleObject(job->stop, 5000) != WAIT_TIMEOUT) {
            CloseHandle(changed);
            return 0;
        }
    }
    for (;;) {
        // (Asked before reading, so no change is missed.)
        RegNotifyChangeKeyValue(
            key, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
            changed, TRUE);
        tell();
        const HANDLE events[] = {job->stop, changed};
        if (WaitForMultipleObjects(ARRAYSIZE(events), events, FALSE,
                                   INFINITE) != WAIT_OBJECT_0 + 1) {
            break;
        }
    }
    RegCloseKey(key);
    CloseHandle(changed);
    return 0;
}

// Presses Ctrl+Win+Left or Right (Windows' shortcut for the next desktop)
// `times` times.
void StepVirtualDesktop(int times) {
    std::vector<INPUT> inputs;
    const WORD arrow = times > 0 ? VK_RIGHT : VK_LEFT;
    for (int i = std::abs(times); i > 0; i--) {
        for (WORD key : {(WORD)VK_CONTROL, (WORD)VK_LWIN, arrow}) {
            INPUT input{INPUT_KEYBOARD};
            input.ki.wVk = key;
            input.ki.dwFlags = key == arrow ? KEYEVENTF_EXTENDEDKEY : 0;
            inputs.push_back(input);
        }
        for (WORD key : {arrow, (WORD)VK_LWIN, (WORD)VK_CONTROL}) {
            INPUT input{INPUT_KEYBOARD};
            input.ki.wVk = key;
            input.ki.dwFlags =
                KEYEVENTF_KEYUP | (key == arrow ? KEYEVENTF_EXTENDEDKEY : 0);
            inputs.push_back(input);
        }
    }
    if (!inputs.empty()) {
        SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
    }
}

// After closing another desktop than the current one (see DesktopsPanel):
// the desktop to go back to, once Windows has closed it (this many left),
// until when it's waited for, on the island's thread; -1 none.
int g_desktopReturnTo = -1;
size_t g_desktopReturnCount = 0;
double g_desktopReturnUntil = 0;
// The desktop to close (Windows closes the current one): closed once
// Windows says it's there (going there takes a step per desktop, and a
// close pressed meanwhile closed the wrong one), until the same time; -1
// none.
int g_desktopCloseAt = -1;

// Goes to a desktop from the current one.
void SwitchVirtualDesktop(int index) {
    AcquireSRWLockShared(&g_desktopsLock);
    const int current = g_desktops.current;
    const int count = (int)g_desktops.names.size();
    ReleaseSRWLockShared(&g_desktopsLock);
    if (current >= 0 && index >= 0 && index < count && index != current) {
        StepVirtualDesktop(index - current);
    }
}

// The list opened from the desktops' dots: each desktop (the current one
// checked; a click goes there), a new one and the task view.
class DesktopsPanel : public Panel {
   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           13 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_iconFormat = MakeFormat(IconFont(),
                                      DWRITE_FONT_WEIGHT_NORMAL, 14 * scale);
        }
        Load();
        Layout();
    }

    void OnOpen() override {
        m_hover = -1;
        Load();
        Layout();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

    bool OnMouseMove(POINT pt) override {
        const int hover = HitTest(pt);
        const bool remove =
            hover >= 0 && hover < (int)m_desktops.names.size() && CanRemove() &&
            PointInRect(pt, RemoveRect(m_rows[hover]));
        if (hover == m_hover && remove == m_removeHover) {
            return false;
        }
        m_hover = hover;
        m_removeHover = remove;
        return true;
    }

    bool OnMouseLeave() override {
        m_hover = -1;
        return true;
    }

    bool OnMouseUp(POINT pt) override {
        const int hit = HitTest(pt);
        if (hit < 0) {
            return false;
        }
        const int count = (int)m_desktops.names.size();
        // Its "x": that desktop closed (Windows moves its windows to the
        // one beside it), going to it first; then, once Windows has closed
        // it, back to the one the user was on (see WM_APP_DESKTOPS).
        const bool remove = hit < count && CanRemove() &&
                            PointInRect(pt, RemoveRect(m_rows[hit]));
        Close();
        if (remove) {
            const int current = m_desktops.current;
            if (current >= 0 && current != hit) {
                g_desktopReturnTo = current > hit ? current - 1 : current;
                g_desktopReturnCount = (size_t)count - 1;
                g_desktopReturnUntil = NowSeconds() + 5;
                g_desktopCloseAt = hit;
                SwitchVirtualDesktop(hit);
            } else {
                PressKeys({VK_CONTROL, VK_LWIN, VK_F4});
            }
        } else if (hit < count) {
            SwitchVirtualDesktop(hit);
        } else if (hit == count) {
            PressKeys({VK_CONTROL, VK_LWIN, 'D'});
        } else {
            PressKeys({VK_LWIN, VK_TAB});
        }
        return false;
    }

    // Changed while open (switched with the keyboard, renamed).
    bool OnAppMessage(UINT msg, WPARAM, LPARAM) override {
        if (msg != WM_APP_DESKTOPS) {
            return false;
        }
        Load();
        Layout();
        return true;
    }

   private:
    void Load() {
        AcquireSRWLockShared(&g_desktopsLock);
        m_desktops = g_desktops;
        ReleaseSRWLockShared(&g_desktopsLock);
    }

    void Layout() {
        const float s = m_scale;
        const float padding = std::round(8 * s);
        const float width = std::round(260 * s);
        const float row = std::round(36 * s);
        float y = padding;
        m_rows.clear();
        // The desktops, then a line, "New desktop" and "Task view".
        const size_t rows = m_desktops.names.size() + 2;
        for (size_t i = 0; i < rows; i++) {
            if (i == m_desktops.names.size()) {
                y += std::round(7 * s);
            }
            m_rows.push_back({padding, y, width - padding, y + row});
            y += row;
        }
        m_size = {(LONG)width, (LONG)std::ceil(y + padding)};
    }

    int HitTest(POINT pt) const {
        for (size_t i = 0; i < m_rows.size(); i++) {
            if (PointInRect(pt, m_rows[i])) {
                return (int)i;
            }
        }
        return -1;
    }

    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    float m_preparedScale = 0;
    VirtualDesktops m_desktops;
    std::vector<D2D1_RECT_F> m_rows;
    int m_hover = -1;
    bool m_removeHover = false;

    // Not the last one; not while the current one isn't known (Windows'
    // shortcut closes the current one, after going to the one clicked).
    bool CanRemove() const {
        return m_desktops.names.size() > 1 && m_desktops.current >= 0;
    }

    D2D1_RECT_F RemoveRect(const D2D1_RECT_F& row) const {
        return {row.right - std::round(30 * m_scale), row.top, row.right,
                row.bottom};
    }
};

void DesktopsPanel::Draw(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush) {
    if (!m_textFormat || !m_iconFormat) {
        return;
    }
    const float s = m_scale;
    const float radius = std::round(8 * s);
    const float icon = std::round(36 * s);
    const int count = (int)m_desktops.names.size();
    for (int i = 0; i < (int)m_rows.size(); i++) {
        const D2D1_RECT_F& row = m_rows[i];
        const bool current = i == m_desktops.current;
        const float lit = Hover(HoverKey(6500, i), m_hover == i);
        if (current || lit > 0.01f) {
            brush->SetColor(current ? Accent(0.18f + 0.08f * lit)
                                    : Fg(0.09f * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        const WCHAR glyph = i < count        ? (WCHAR)0xE7F4
                            : i == count     ? (WCHAR)0xE710
                                             : (WCHAR)0xE7C4;
        brush->SetColor(current ? Accent(1) : Fg(0.8f));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                  {row.left, row.top, row.left + icon, row.bottom},
                  brush);
        const std::wstring text =
            i < count    ? DesktopName(m_desktops, i)
            : i == count ? std::wstring(Tr(L"New desktop",
                                           L"Nova área de trabalho"))
                         : std::wstring(Tr(L"Task view",
                                           L"Visão de tarefas"));
        brush->SetColor(Fg(0.92f));
        target->DrawText(text.c_str(), (UINT32)text.size(), m_textFormat.get(),
                         {row.left + icon, row.top,
                          row.right - std::round(32 * s), row.bottom},
                         brush);
        // Under the mouse, an "x" to close it (not the last one).
        if (m_hover == i && i < count && CanRemove()) {
            const WCHAR close = 0xE711;
            brush->SetColor(m_removeHover ? D2D1::ColorF(1, 0.36f, 0.3f, 1)
                                          : Fg(0.55f));
            DrawGlyph(target, &close, 1, m_iconFormat.get(), RemoveRect(row),
                      brush);
        }
    }
    // The line before "New desktop".
    if (count < (int)m_rows.size()) {
        const float lineY = m_rows[count].top - std::round(4 * s);
        brush->SetColor(Fg(0.12f));
        target->FillRectangle({m_rows[count].left + std::round(12 * s), lineY,
                               m_rows[count].right - std::round(12 * s),
                               lineY + 1},
                              brush);
    }
}

