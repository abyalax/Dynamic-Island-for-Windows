#pragma once
void LoadSettings();
// How often updates are looked for (updatesCheckEvery, chosen in their
// panel, for every island: they're looked for once for all): every 1, 3, 6,
// 12 or 24 hours.
constexpr int kUpdatesEveryHours[] = {1, 3, 6, 12, 24};
int g_updatesEvery = 0;

double UpdatesCheckSeconds() {
    return kUpdatesEveryHours[std::clamp(
               g_updatesEvery, 0, (int)std::size(kUpdatesEveryHours) - 1)] *
           60.0 * 60;
}

UpdatesData GetUpdates() {
    AcquireSRWLockShared(&g_updatesLock);
    UpdatesData data = g_updates;
    for (auto& app : data.apps) {
        app.ignored = std::find(g_ignoredUpdates.begin(),
                                g_ignoredUpdates.end(),
                                app.id) != g_ignoredUpdates.end();
    }
    ReleaseSRWLockShared(&g_updatesLock);
    // The ignored ones last.
    std::stable_partition(data.apps.begin(), data.apps.end(),
                          [](const AppUpdate& app) { return !app.ignored; });
    return data;
}

void LoadIgnoredUpdates() {
    const std::wstring value = GetWholeStringValue(L"updatesIgnored", 4096);
    std::vector<std::wstring> ids;
    for (PCWSTR line = value.c_str(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        std::wstring id = end ? std::wstring(line, end) : std::wstring(line);
        if (!id.empty() && ids.size() < 64) {
            ids.push_back(std::move(id));
        }
        if (!end) {
            break;
        }
        line = end + 1;
    }
    AcquireSRWLockExclusive(&g_updatesLock);
    g_ignoredUpdates = std::move(ids);
    ReleaseSRWLockExclusive(&g_updatesLock);
}

// Ignores an app's updates, or shows them again; kept.
void SetUpdateIgnored(const std::wstring& id, bool ignored) {
    AcquireSRWLockExclusive(&g_updatesLock);
    std::erase(g_ignoredUpdates, id);
    if (ignored) {
        g_ignoredUpdates.push_back(id);
        if (g_ignoredUpdates.size() > 64) {
            g_ignoredUpdates.erase(g_ignoredUpdates.begin());
        }
        // Not among the ticked ones anymore.
        for (auto& app : g_updates.apps) {
            if (app.id == id) {
                app.ticked = false;
            }
        }
    }
    std::wstring value;
    for (const auto& ignoredId : g_ignoredUpdates) {
        value += ignoredId + L"\n";
    }
    ReleaseSRWLockExclusive(&g_updatesLock);
    SetStoredString(L"updatesIgnored", value.c_str());
}

// A package ID as winget writes them: words joined by dots, nothing a
// command line would read otherwise.
bool IsPackageId(const std::wstring& id) {
    return !id.empty() && id.size() < 128 &&
           std::all_of(id.begin(), id.end(), [](WCHAR c) {
               return (c < 0x80 && iswalnum(c)) || c == L'.' || c == L'-' ||
                      c == L'_' || c == L'+';
           });
}

std::wstring DecodeUtf8(const std::string& bytes) {
    std::wstring text;
    const int length = MultiByteToWideChar(CP_UTF8, 0, bytes.data(),
                                           (int)bytes.size(), nullptr, 0);
    text.resize(std::max(length, 0));
    MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(),
                        text.data(), length);
    return text;
}

// winget (the App Installer's alias), run hidden with these arguments: its
// output (UTF-8) and exit code. Gives up (and ends it) after `timeoutMs`, or
// when `stop` is set. It inherits only its output pipe.
bool RunWinget(const std::wstring& arguments,
               DWORD timeoutMs,
               HANDLE stop,
               std::wstring* output,
               DWORD* exitCode) {
    WCHAR path[MAX_PATH];
    if (!ExpandEnvironmentStrings(
            L"%LOCALAPPDATA%\\Microsoft\\WindowsApps\\winget.exe", path,
            MAX_PATH) ||
        GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &security, 0)) {
        return false;
    }
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
    SIZE_T listSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &listSize);
    std::vector<BYTE> list(listSize);
    auto* attributes = (LPPROC_THREAD_ATTRIBUTE_LIST)list.data();
    const bool listed =
        listSize && InitializeProcThreadAttributeList(attributes, 1, 0,
                                                      &listSize) &&
        UpdateProcThreadAttribute(attributes, 0,
                                  PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &writePipe,
                                  sizeof(writePipe), nullptr, nullptr);
    STARTUPINFOEX startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.StartupInfo.wShowWindow = SW_HIDE;
    startup.StartupInfo.hStdOutput = writePipe;
    startup.StartupInfo.hStdError = writePipe;
    startup.StartupInfo.hStdInput = nullptr;
    startup.lpAttributeList = attributes;
    std::wstring command = L"\"" + std::wstring(path) + L"\" " + arguments;
    PROCESS_INFORMATION process{};
    const BOOL started =
        listed && CreateProcess(path, command.data(), nullptr, nullptr, TRUE,
                                CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT,
                                nullptr, nullptr, &startup.StartupInfo,
                                &process);
    if (listed) {
        DeleteProcThreadAttributeList(attributes);
    }
    CloseHandle(writePipe);
    if (!started) {
        CloseHandle(readPipe);
        return false;
    }
    std::string bytes;
    const ULONGLONG until = GetTickCount64() + timeoutMs;
    bool finished = false;
    for (;;) {
        DWORD available = 0;
        while (PeekNamedPipe(readPipe, nullptr, 0, nullptr, &available,
                             nullptr) &&
               available) {
            char buffer[4096];
            DWORD read = 0;
            if (!ReadFile(readPipe, buffer,
                          std::min<DWORD>(available, sizeof(buffer)), &read,
                          nullptr) ||
                !read) {
                break;
            }
            if (bytes.size() < 1024 * 1024) {
                bytes.append(buffer, read);
            }
        }
        if (finished) {
            break;
        }
        const HANDLE handles[] = {process.hProcess, stop};
        const DWORD woke =
            WaitForMultipleObjects(stop ? 2 : 1, handles, FALSE, 100);
        if (woke == WAIT_OBJECT_0) {
            // Once more for what's left in the pipe.
            finished = true;
            continue;
        }
        if (woke != WAIT_TIMEOUT || GetTickCount64() > until) {
            TerminateProcess(process.hProcess, 1);
            break;
        }
    }
    DWORD code = 1;
    if (finished) {
        GetExitCodeProcess(process.hProcess, &code);
    }
    CloseHandle(process.hProcess);
    CloseHandle(process.hThread);
    CloseHandle(readPipe);
    if (exitCode) {
        *exitCode = code;
    }
    if (output) {
        *output = DecodeUtf8(bytes);
    }
    return finished;
}

// The first table of "winget upgrade": its header's words give where each
// column starts (name, ID, version, available, source; in any language).
std::vector<AppUpdate> ParseWingetUpgrades(const std::wstring& output) {
    std::vector<std::wstring> lines;
    for (size_t at = 0; at < output.size();) {
        size_t end = output.find(L'\n', at);
        if (end == std::wstring::npos) {
            end = output.size();
        }
        std::wstring line = output.substr(at, end - at);
        // Progress drawn over itself: only what's after the last return.
        while (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }
        const size_t carriage = line.rfind(L'\r');
        if (carriage != std::wstring::npos) {
            line = line.substr(carriage + 1);
        }
        lines.push_back(line);
        at = end + 1;
    }
    std::vector<AppUpdate> apps;
    for (size_t i = 1; i < lines.size(); i++) {
        const std::wstring& rule = lines[i];
        if (rule.size() < 20 ||
            rule.find_first_not_of(L'-') != std::wstring::npos) {
            continue;
        }
        const std::wstring& header = lines[i - 1];
        std::vector<size_t> starts;
        for (size_t c = 0; c < header.size(); c++) {
            if (header[c] != L' ' && (c == 0 || header[c - 1] == L' ')) {
                starts.push_back(c);
            }
        }
        if (starts.size() < 4) {
            return apps;
        }
        auto column = [&](const std::wstring& line, size_t index) {
            if (index >= starts.size() || starts[index] >= line.size()) {
                return std::wstring();
            }
            const size_t end = index + 1 < starts.size()
                                   ? std::min(starts[index + 1], line.size())
                                   : line.size();
            std::wstring text = line.substr(starts[index], end - starts[index]);
            while (!text.empty() && text.back() == L' ') {
                text.pop_back();
            }
            return text;
        };
        for (size_t j = i + 1; j < lines.size(); j++) {
            const std::wstring& line = lines[j];
            // The table ends at its count ("10 upgrades available.").
            if (line.empty() || line.size() < starts[3] ||
                line.find_first_not_of(L' ') == std::wstring::npos) {
                break;
            }
            AppUpdate app{column(line, 0), column(line, 1), column(line, 2),
                          column(line, 3)};
            if (app.available.empty()) {
                break;
            }
            // (An ID winget cut short, "Some.Long.Id…", can't be updated by
            // it: only that row is skipped.)
            if (!IsPackageId(app.id)) {
                continue;
            }
            apps.push_back(std::move(app));
        }
        break;
    }
    return apps;
}

