// Claude Code's limits as its status line got them last ("rate_limits";
// only with a Pro or Max plan). False without the file, or while it's being
// written.
bool ReadClaudeUsage(AiUsage* usage) {
    const std::wstring path = ClaudeUsageFile();
    std::wstring text;
    long long writtenAt = 0;
    Json status;
    if (path.empty() || !ReadFileEnd(path, 256 * 1024, &text, &writtenAt) ||
        !JsonReader(text).Read(&status)) {
        return false;
    }
    const Json& limits = status[L"rate_limits"];
    usage->session = ClaudeLimit(limits[L"five_hour"], 5 * 60);
    usage->week = ClaudeLimit(limits[L"seven_day"], 7 * 24 * 60);
    usage->writtenAt = writtenAt;
    return true;
}

// The last limits in a Codex log's end (its token counts' "rate_limits":
// "primary" and "secondary", each "used_percent", "window_minutes" and
// "resets_at"). False if there are none.
bool ReadCodexLimits(const std::wstring& path, AiUsage* usage) {
    std::wstring text;
    long long writtenAt = 0;
    if (!ReadFileEnd(path, 256 * 1024, &text, &writtenAt)) {
        return false;
    }
    constexpr std::wstring_view kKey = L"\"rate_limits\":";
    for (size_t at = text.rfind(kKey); at != std::wstring::npos;
         at = at ? text.rfind(kKey, at - 1) : std::wstring::npos) {
        const size_t start = SkipJsonSpaces(text, at + kKey.size());
        const size_t end = start < text.size() && text[start] == L'{'
                               ? JsonValueEnd(text, start)
                               : std::wstring::npos;
        Json limits;
        if (end == std::wstring::npos ||
            !JsonReader(text.substr(start, end - start)).Read(&limits)) {
            continue;
        }
        AiUsage read;
        read.writtenAt = writtenAt;
        for (PCWSTR name : {L"primary", L"secondary"}) {
            const Json& window = limits[name];
            if (window.type != Json::Type::Object) {
                continue;
            }
            AiLimit limit;
            limit.used =
                std::clamp(window[L"used_percent"].Number(-1), -1.0, 100.0);
            limit.minutes = (int)window[L"window_minutes"].Number(0);
            limit.resetsAt = (long long)window[L"resets_at"].Number(0);
            // (Older versions: how long until then, from the log's time.)
            if (!limit.resetsAt &&
                window[L"resets_in_seconds"].type == Json::Type::Number) {
                limit.resetsAt =
                    writtenAt + (long long)window[L"resets_in_seconds"].number;
            }
            // (Shorter than a day: the session's.)
            const bool session = limit.minutes > 0
                                     ? limit.minutes < 24 * 60
                                     : wcscmp(name, L"primary") == 0;
            (session ? read.session : read.week) = limit;
        }
        if (read.session.used >= 0 || read.week.used >= 0) {
            *usage = read;
            return true;
        }
    }
    return false;
}

bool ReadCodexUsage(AiUsage* usage) {
    int tried = 0;
    for (const std::wstring& log : NewestCodexLogs()) {
        if (ReadCodexLimits(log, usage)) {
            return true;
        }
        if (++tried == 4) {
            break;
        }
    }
    return false;
}

// How much of a limit is used now: nothing once it started over (-1 not
// known).
double AiUsedNow(const AiLimit& limit, long long now) {
    if (limit.used < 0) {
        return -1;
    }
    return limit.resetsAt > 0 && now >= limit.resetsAt ? 0 : limit.used;
}

// The limit that stops the agent first: the most used now (the session's
// when they're even).
const AiLimit& AiTightLimit(const AiUsage& usage, long long now) {
    return AiUsedNow(usage.week, now) > AiUsedNow(usage.session, now)
               ? usage.week
               : usage.session;
}

bool AiUsageKnown(const AiUsage& usage) {
    return usage.session.used >= 0 || usage.week.used >= 0;
}

// Its color as it fills (-1 the text's).
int AiUsageRgb(double used) {
    return used >= kAiUsageHigh   ? 0xFF453A
           : used >= kAiUsageWarn ? 0xFFD60A
                                  : -1;
}

// How long until a time ("45 min", "2h10", "3d 4h"); empty when it's past
// or not known.
std::wstring AiTimeLeft(long long resetsAt, long long now) {
    if (resetsAt <= now) {
        return {};
    }
    const long long minutes = (resetsAt - now + 59) / 60;
    WCHAR text[32];
    if (minutes < 60) {
        swprintf_s(text, L"%lld min", minutes);
    } else if (minutes < 24 * 60) {
        swprintf_s(text, L"%lldh%02lld", minutes / 60, minutes % 60);
    } else {
        swprintf_s(text, L"%lldd %lldh", minutes / (24 * 60),
                   minutes % (24 * 60) / 60);
    }
    return text;
}

// The pill's value: the tighter limit's use, and how long until it starts
// over.
std::wstring AiPillValue(const AiUsage& usage, long long now) {
    const AiLimit& limit = AiTightLimit(usage, now);
    const double used = AiUsedNow(limit, now);
    if (used < 0) {
        return L"–";
    }
    WCHAR percent[16];
    swprintf_s(percent, L"%.0f%%", used);
    const std::wstring left =
        used > 0 ? AiTimeLeft(limit.resetsAt, now) : std::wstring();
    return left.empty() ? std::wstring(percent)
                        : std::wstring(percent) + L" · " + left;
}

// When a limit starts over, as the clock shows a time (with the day when
// it isn't today).
std::wstring AiResetTime(long long resetsAt) {
    ULARGE_INTEGER value;
    value.QuadPart = (ULONGLONG)resetsAt * 10000000 + 116444736000000000ULL;
    const FILETIME utc{value.LowPart, value.HighPart};
    SYSTEMTIME utcTime, local, today;
    if (resetsAt <= 0 || !FileTimeToSystemTime(&utc, &utcTime) ||
        !SystemTimeToTzSpecificLocalTime(nullptr, &utcTime, &local)) {
        return {};
    }
    GetLocalTime(&today);
    std::wstring text = FormatClockTime(local, false);
    if (local.wYear != today.wYear || local.wMonth != today.wMonth ||
        local.wDay != today.wDay) {
        WCHAR day[32] = L"";
        GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &local, L"ddd", day,
                        ARRAYSIZE(day), nullptr);
        text = std::wstring(day) + L" " + text;
    }
    return text;
}

// How long ago the limits were written.
std::wstring AiUpdatedText(long long writtenAt, long long now) {
    const long long minutes = std::max(0LL, (now - writtenAt) / 60);
    WCHAR text[64];
    if (minutes < 1) {
        return Tr(L"Updated now", L"Atualizado agora");
    }
    if (minutes < 60) {
        swprintf_s(text, Tr(L"Updated %lld min ago", L"Atualizado há %lld min"),
                   minutes);
    } else if (minutes < 48 * 60) {
        swprintf_s(text, Tr(L"Updated %lld h ago", L"Atualizado há %lld h"),
                   minutes / 60);
    } else {
        swprintf_s(text,
                   Tr(L"Updated %lld days ago", L"Atualizado há %lld dias"),
                   minutes / (24 * 60));
    }
    return text;
}

// The agents' limits read (lParam: an AiUsageRead the island deletes, see
// AiUsageThreadProc).
constexpr UINT WM_APP_AI_USAGE = WM_APP + 101;

// Both agents' limits as the watching thread read them. Claude Code's only
// when its file was read whole, or isn't there: while its status line
// writes it, the ones read before are kept.
struct AiUsageRead {
    bool claudeRead = false;
    AiUsage claude;
    AiUsage codex;
};

// The folders their limits are written in, watched on a thread
// (FindFirstChangeNotification, like the folders' panel): the mod's storage
// (Claude Code's status line's file) and Codex's sessions (all of its days'
// folders). Both read there, once at the start and a moment after the
// first change of a burst (see kAiUsageGapMs), and posted to the island
// (the files' reading never on its thread). A folder not there yet is left
// out (see Island::OnAiUsageMinute).
struct AiUsageJob {
    HWND notify;
    HANDLE stop;
    std::wstring folders[kAiAgents];
};

DWORD WINAPI AiUsageThreadProc(LPVOID parameter) {
    std::unique_ptr<AiUsageJob> job((AiUsageJob*)parameter);
    HANDLE waits[1 + kAiAgents] = {job->stop};
    DWORD count = 1;
    for (int agent = 0; agent < kAiAgents; agent++) {
        if (job->folders[agent].empty()) {
            continue;
        }
        const HANDLE change = FindFirstChangeNotification(
            job->folders[agent].c_str(), agent == (int)AiAgent::Codex,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
        if (change != INVALID_HANDLE_VALUE) {
            waits[count++] = change;
        }
    }
    ULONGLONG readAt = GetTickCount64();
    ULONGLONG readBefore = 0;
    for (;;) {
        DWORD timeout = INFINITE;
        if (readAt) {
            const ULONGLONG now = GetTickCount64();
            timeout = now >= readAt ? 0 : (DWORD)(readAt - now);
        }
        const DWORD woken =
            WaitForMultipleObjects(count, waits, FALSE, timeout);
        if (woken == WAIT_TIMEOUT) {
            readAt = 0;
            readBefore = GetTickCount64();
            auto read = std::make_unique<AiUsageRead>();
            read->claudeRead =
                ReadClaudeUsage(&read->claude) ||
                GetFileAttributes(ClaudeUsageFile().c_str()) ==
                    INVALID_FILE_ATTRIBUTES;
            ReadCodexUsage(&read->codex);
            // (Not once it's being stopped.)
            if (WaitForSingleObject(job->stop, 0) != WAIT_TIMEOUT) {
                break;
            }
            if (PostMessage(job->notify, WM_APP_AI_USAGE, 0,
                            (LPARAM)read.get())) {
                read.release();
            }
            continue;
        }
        const DWORD k = woken - WAIT_OBJECT_0;
        if (woken == WAIT_FAILED || k == 0 || k >= count) {
            break;
        }
        if (!readAt) {
            readAt = std::max(GetTickCount64() + kAiUsageReadMs,
                              readBefore + kAiUsageGapMs);
        }
        if (!FindNextChangeNotification(waits[k])) {
            break;
        }
    }
    for (DWORD k = 1; k < count; k++) {
        FindCloseChangeNotification(waits[k]);
    }
    return 0;
}

