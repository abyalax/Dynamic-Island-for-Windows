#pragma once
// The Recycle Bin's item clicked (islandRecycleBin): what's in it, opened in
// Explorer, or emptied with a second click on "Empty" (on a thread of its
// own, Windows' own emptying).
class RecycleBinPanel : public Panel {
   public:
    void SetContents(long long items, long long kilobytes) {
        m_items = items;
        m_kilobytes = kilobytes;
        if (IsOpen()) {
            Layout();
            Render();
        }
    }

   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_iconFormat = MakeFormat(IconFont(), DWRITE_FONT_WEIGHT_NORMAL,
                                      20 * scale);
            m_titleFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 13 * scale,
                           DWRITE_TEXT_ALIGNMENT_LEADING);
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_buttonFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11 * scale, DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        Layout();
    }

    void OnOpen() override {
        m_hover = kNone;
        m_asking = false;
        m_failed = false;
        m_doneAt = -10;
        Layout();
    }

    void OnClose() override {
        m_asking = false;
        KillTimer(m_hwnd, kAskTimerId);
        KillTimer(m_hwnd, kDoneTimerId);
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override {
        if (!m_iconFormat || !m_titleFormat || !m_textFormat ||
            !m_buttonFormat) {
            return;
        }
        const float s = m_scale;
        // The bin: rocking while it empties, hopping once when it's done.
        const double now = NowSeconds();
        const D2D1_POINT_2F middle{(m_icon.left + m_icon.right) / 2,
                                   (m_icon.top + m_icon.bottom) / 2};
        D2D1_MATRIX_3X2_F transform;
        target->GetTransform(&transform);
        const double done =
            (now - m_doneAt) / std::max(AnimationSeconds(0.6), 0.001);
        if (m_emptying && AnimationLevel() < 2) {
            const float angle =
                8 * (float)std::sin((now - m_emptyStartAt) * 14);
            target->SetTransform(
                D2D1::Matrix3x2F::Rotation(angle, middle) * transform);
        } else if (done >= 0 && done < 1) {
            const float hop =
                1 + 0.22f * (float)(std::sin(done * 3.14159265) * (1 - done));
            target->SetTransform(
                D2D1::Matrix3x2F::Scale(hop, hop, middle) * transform);
        }
        const WCHAR glyph = 0xE74D;
        brush->SetColor(Fg(0.95f));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(), m_icon, brush);
        target->SetTransform(transform);
        PCWSTR title = Tr(L"Recycle Bin", L"Lixeira");
        target->DrawText(title, (UINT32)wcslen(title), m_titleFormat.get(),
                         m_title, brush);
        // (Done: said for a few seconds.)
        const bool justDone = now - m_doneAt < 3;
        const std::wstring text =
            m_emptying  ? std::wstring(Tr(L"Emptying…", L"Esvaziando…"))
            : m_failed  ? std::wstring(Tr(L"Couldn't empty it",
                                          L"Não deu para esvaziar"))
            : justDone  ? std::wstring(Tr(L"\u2713 Emptied",
                                          L"\u2713 Esvaziada"))
            : m_items < 0 ? std::wstring()
                          : RecycleBinText(m_items, m_kilobytes);
        brush->SetColor(m_failed ? D2D1_COLOR_F(D2D1::ColorF(1, 0.45f, 0.3f, 1))
                                 : Fg(0.6f));
        target->DrawText(text.c_str(), (UINT32)text.size(), m_textFormat.get(),
                         m_text, brush);
        // Emptying: a short stripe going back and forth under the text.
        if (m_emptying) {
            const float thickness = std::round(3 * s);
            const D2D1_RECT_F track{m_text.left, m_text.bottom - thickness,
                                    m_text.right - std::round(8 * s),
                                    m_text.bottom};
            const float width = (track.right - track.left) * 0.3f;
            const float t =
                0.5f - 0.5f * (float)std::cos((now - m_emptyStartAt) * 3.2);
            const float left =
                track.left + (track.right - track.left - width) * t;
            brush->SetColor(Fg(0.12f));
            target->FillRoundedRectangle({track, thickness / 2, thickness / 2},
                                         brush);
            brush->SetColor(Accent(1));
            target->FillRoundedRectangle(
                {{left, track.top, left + width, track.bottom},
                 thickness / 2,
                 thickness / 2},
                brush);
        }
        // (`danger` from 0 to 1: it turns red, asking, and back.)
        auto button = [&](const D2D1_RECT_F& rect, int part, PCWSTR label,
                          PCWSTR asking, float danger, bool enabled) {
            const float lit =
                enabled ? Hover(HoverKey(6500, part), m_hover == part) : 0;
            const float radius = (rect.bottom - rect.top) / 2;
            brush->SetColor(Fg((0.1f + 0.08f * lit) * (enabled ? 1 : 0.5f)));
            GlassFill(target, {rect, radius, radius}, brush);
            if (danger > 0.01f) {
                brush->SetColor(D2D1::ColorF(0.9f, 0.25f, 0.2f,
                                             (0.85f + 0.15f * lit) * danger));
                target->FillRoundedRectangle({rect, radius, radius}, brush);
            }
            brush->SetColor(Fg((enabled ? 0.9f : 0.4f) * (1 - danger)));
            target->DrawText(label, (UINT32)wcslen(label),
                             m_buttonFormat.get(), rect, brush);
            if (asking && danger > 0.01f) {
                brush->SetColor(D2D1::ColorF(1, 1, 1, danger));
                target->DrawText(asking, (UINT32)wcslen(asking),
                                 m_buttonFormat.get(), rect, brush);
            }
        };
        button(m_open, kOpen, Tr(L"Open", L"Abrir"), nullptr, 0, true);
        const bool canEmpty = m_items > 0 && !m_emptying;
        button(m_empty, kEmpty, Tr(L"Empty the bin", L"Esvaziar"),
               Tr(L"Empty it? Click again", L"Esvaziar? Clique de novo"),
               Hover(HoverKey(6501, 0), m_asking), canEmpty);
    }

    // While it empties, and a moment after (the bin hopping).
    bool AnimateContents(double) override {
        return m_emptying ||
               NowSeconds() - m_doneAt < AnimationSeconds(0.6) + 0.05;
    }

    bool OnMouseMove(POINT pt) override {
        const int hover = HitTest(pt);
        if (hover == m_hover) {
            return false;
        }
        m_hover = hover;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hover == kNone) {
            return false;
        }
        m_hover = kNone;
        return true;
    }

    bool OnMouseUp(POINT pt) override {
        switch (HitTest(pt)) {
            case kOpen:
                Close();
                ShellExecute(nullptr, L"open", L"shell:RecycleBinFolder",
                             nullptr, nullptr, SW_SHOWNORMAL);
                return false;
            case kEmpty:
                if (m_items <= 0 || m_emptying) {
                    return false;
                }
                // The first click asks (a few seconds), the second empties.
                if (!m_asking) {
                    m_asking = true;
                    SetTimer(m_hwnd, kAskTimerId, kAskMs, nullptr);
                    return true;
                }
                m_asking = false;
                KillTimer(m_hwnd, kAskTimerId);
                StartEmptying();
                return true;
            default:
                return false;
        }
    }

    bool OnTimer(UINT_PTR id) override {
        if (id == kDoneTimerId) {
            PulseContents();
            return true;
        }
        if (id != kAskTimerId || !m_asking) {
            return false;
        }
        m_asking = false;
        return true;
    }

    bool OnAppMessage(UINT msg, WPARAM wParam, LPARAM) override {
        if (msg != WM_APP_RECYCLE_EMPTIED) {
            return false;
        }
        if (m_emptyThread) {
            WaitForSingleObject(m_emptyThread, INFINITE);
            CloseHandle(m_emptyThread);
            m_emptyThread = nullptr;
        }
        m_emptying = false;
        m_failed = !wParam;
        if (wParam) {
            m_doneAt = NowSeconds();
            // (Back to what's in it after saying it's done.)
            SetTimer(m_hwnd, kDoneTimerId, kDoneMs, nullptr);
        }
        // (The island counts again as Windows tells it the bin changed.)
        Layout();
        PulseContents();
        return true;
    }

    void OnDestroy() override {
        // (Windows' emptying of a full bin can take long: not waited for
        // forever as the mod goes; it only posts to this window.)
        if (m_emptyThread) {
            WaitForSingleObject(m_emptyThread, 5000);
            CloseHandle(m_emptyThread);
            m_emptyThread = nullptr;
        }
    }

   private:
    static constexpr int kNone = -1;
    static constexpr int kOpen = 0;
    static constexpr int kEmpty = 1;
    static constexpr UINT_PTR kAskTimerId = 1;
    static constexpr UINT kAskMs = 4000;
    static constexpr UINT_PTR kDoneTimerId = 2;
    static constexpr UINT kDoneMs = 3000;

    void StartEmptying() {
        if (m_emptyThread) {
            return;
        }
        m_emptyThread = CreateThread(nullptr, 0, RecycleEmptyProc, m_hwnd, 0,
                                     nullptr);
        m_emptying = m_emptyThread != nullptr;
        m_failed = !m_emptying;
        m_emptyStartAt = NowSeconds();
        PulseContents();
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(300 * s);
        const float padding = std::round(14 * s);
        const float icon = std::round(40 * s);
        m_icon = {padding, padding, padding + icon, padding + icon};
        m_title = {padding + icon + std::round(8 * s), padding,
                   width - padding, padding + std::round(20 * s)};
        m_text = {m_title.left, m_title.bottom, width - padding,
                  padding + icon};
        const float top = padding + icon + std::round(10 * s);
        const float height = std::round(28 * s);
        const float gap = std::round(8 * s);
        const float half = (width - padding * 2 - gap) / 2;
        m_open = {padding, top, padding + half, top + height};
        m_empty = {padding + half + gap, top, width - padding, top + height};
        m_size = {(LONG)width, (LONG)std::ceil(top + height + padding)};
    }

    int HitTest(POINT pt) const {
        if (PointInRect(pt, m_open)) {
            return kOpen;
        }
        if (PointInRect(pt, m_empty)) {
            return kEmpty;
        }
        return kNone;
    }

    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_titleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_buttonFormat;
    float m_preparedScale = 0;
    long long m_items = -1;
    long long m_kilobytes = 0;
    bool m_asking = false;
    bool m_emptying = false;
    bool m_failed = false;
    // When it started emptying, and when it was done (see Draw).
    double m_emptyStartAt = 0;
    double m_doneAt = -10;
    HANDLE m_emptyThread = nullptr;
    D2D1_RECT_F m_icon{}, m_title{}, m_text{}, m_open{}, m_empty{};
    int m_hover = kNone;
};

////////////////////////////////////////////////////////////////////////////////
// The AI coding agents' usage limits (islandAiUsage, #6636): how much of
// Claude Code's and Codex's limits is used and when each starts over, read
// from what they keep on this PC: never their accounts, logins or tokens,
// and nothing goes online. Codex writes them in its sessions' logs. Claude
// Code gives them to its status line's command, which saves them in the
// mod's storage (ClaudeUsageFile) for the island, with a line the user adds
// to their own status line script (see ClaudeUsageLine and the README): the
// island never changes Claude Code's settings.

enum class AiAgent { Claude, Codex, Count };
constexpr int kAiAgents = (int)AiAgent::Count;

// A limit: how much of it is used (0 to 100; -1 not known), when it starts
// over (Unix seconds; 0 not known) and how long it lasts (minutes).
struct AiLimit {
    double used = -1;
    long long resetsAt = 0;
    int minutes = 0;
};

// An agent's limits as last read: its session's (5 hours) and its week's,
// and when they were written (Unix seconds; 0: nothing found).
struct AiUsage {
    AiLimit session;
    AiLimit week;
    long long writtenAt = 0;
};

// Read while an island shows them, on the thread watching their files (see
// AiUsageThreadProc), a moment after the first change (an answer writes
// many times) and never sooner than a few seconds after the last read
// (Codex writes its log all the time while it works), and given to the
// main island; the islands laid out again each minute (the time left goes
// down).
AiUsage g_aiUsage[kAiAgents];
constexpr UINT kAiUsageReadMs = 1000;
constexpr UINT kAiUsageGapMs = 5000;
constexpr UINT kAiUsageMinuteMs = 60 * 1000;
// Used from this much (in percent): yellow, then red.
constexpr double kAiUsageWarn = 70;
constexpr double kAiUsageHigh = 90;

bool AiAgentOn(int agent) {
    return agent == (int)AiAgent::Claude ? g_settings.islandAiClaude
                                         : g_settings.islandAiCodex;
}

PCWSTR AiAgentName(int agent) {
    return agent == (int)AiAgent::Claude ? L"Claude Code" : L"Codex";
}

// A time in Unix seconds.
long long UnixSeconds(const FILETIME& time) {
    return (long long)(((((ULONGLONG)time.dwHighDateTime << 32) |
                         time.dwLowDateTime) -
                        116444736000000000ULL) /
                       10000000);
}

long long UnixNow() {
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    return UnixSeconds(now);
}

// A path with its environment variables (empty if one isn't set).
std::wstring ExpandedPath(PCWSTR path) {
    WCHAR expanded[MAX_PATH];
    const DWORD length = ExpandEnvironmentStrings(path, expanded, MAX_PATH);
    if (length == 0 || length > MAX_PATH || wcschr(expanded, L'%')) {
        return {};
    }
    return expanded;
}

// A file in the mod's storage (its folder made if it isn't there; empty
// without one): what Claude Code's status line saves for the island. The
// account's own (the storage is the computer's, see UserValueName): another
// account's Claude Code doesn't show here.
std::wstring AiStorageFile(PCWSTR name) {
    WCHAR storage[MAX_PATH];
    if (!Wh_GetModStoragePath(storage, ARRAYSIZE(storage))) {
        return {};
    }
    CreateDirectory(storage, nullptr);
    return std::wstring(storage) + L"\\" + UserValueName(name) + L".json";
}

std::wstring ClaudeUsageFile() {
    return AiStorageFile(L"claude-status");
}

// The line that saves it, for the user's status line script: in Git Bash
// (where Claude Code runs it when it's there) one that read its input with
// input=$(cat), else in PowerShell one that read it into $stdin (see the
// README).
std::wstring ClaudeUsageLine(bool bash) {
    std::wstring path = ClaudeUsageFile();
    if (bash) {
        std::replace(path.begin(), path.end(), L'\\', L'/');
        for (size_t at = path.find(L'\''); at != std::wstring::npos;
             at = path.find(L'\'', at + 4)) {
            path.replace(at, 1, L"'\\''");
        }
        return L"printf '%s' \"$input\" > '" + path + L"'";
    }
    for (size_t at = path.find(L'\''); at != std::wstring::npos;
         at = path.find(L'\'', at + 2)) {
        path.insert(at, 1, L'\'');
    }
    return L"$stdin | Set-Content -Encoding utf8 -LiteralPath '" + path + L"'";
}

// That file made before the status line first writes it, with only this
// account allowed (the storage's folder lets the PC's other accounts read
// it, and what Claude Code gives its status line has the session's folder
// and log); the line's `>` and Set-Content keep the ACL of a file that's
// there. One made before gets the same ACL. False if it can't be (logged).
bool PrepareClaudeUsageFile() {
    std::wstring path = ClaudeUsageFile();
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (path.empty() || UserSid().empty() ||
        !ConvertStringSecurityDescriptorToSecurityDescriptor(
            (L"D:P(A;;FA;;;" + UserSid() + L")").c_str(), SDDL_REVISION_1,
            &descriptor, nullptr)) {
        IslandLog(L"AI usage: Claude Code's file couldn't be made");
        return false;
    }
    SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
    HANDLE file = CreateFile(
        path.c_str(), GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, &attributes,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    bool ok = file != INVALID_HANDLE_VALUE;
    if (ok) {
        CloseHandle(file);
    } else if (GetLastError() == ERROR_FILE_EXISTS) {
        PACL dacl = nullptr;
        BOOL present = FALSE;
        BOOL defaulted = FALSE;
        ok = GetSecurityDescriptorDacl(descriptor, &present, &dacl,
                                       &defaulted) &&
             SetNamedSecurityInfo(path.data(), SE_FILE_OBJECT,
                                  DACL_SECURITY_INFORMATION |
                                      PROTECTED_DACL_SECURITY_INFORMATION,
                                  nullptr, nullptr, dacl,
                                  nullptr) == ERROR_SUCCESS;
    }
    LocalFree(descriptor);
    if (!ok) {
        IslandLog(L"AI usage: Claude Code's file couldn't be made: %u",
                  (unsigned)GetLastError());
    }
    return ok;
}

// A file's last `limit` bytes as text (UTF-8, or UTF-16 with its mark),
// opened beside the app writing it, and when it was last written (Unix
// seconds). False if it can't be read.
bool ReadFileEnd(const std::wstring& path,
                 size_t limit,
                 std::wstring* text,
                 long long* writtenAt) {
    HANDLE file = CreateFile(
        path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    LARGE_INTEGER size{};
    FILETIME written{};
    std::string bytes;
    bool ok = GetFileSizeEx(file, &size) &&
              GetFileTime(file, nullptr, nullptr, &written);
    if (ok) {
        LARGE_INTEGER start{};
        start.QuadPart =
            std::max<LONGLONG>(0, size.QuadPart - (LONGLONG)limit);
        bytes.resize((size_t)(size.QuadPart - start.QuadPart));
        DWORD read = 0;
        ok = SetFilePointerEx(file, start, nullptr, FILE_BEGIN) &&
             (bytes.empty() || ReadFile(file, bytes.data(),
                                        (DWORD)bytes.size(), &read, nullptr));
        bytes.resize(read);
    }
    CloseHandle(file);
    if (!ok) {
        return false;
    }
    *writtenAt = UnixSeconds(written);
    text->clear();
    if (bytes.size() >= 2 && (BYTE)bytes[0] == 0xFF && (BYTE)bytes[1] == 0xFE) {
        text->assign((const WCHAR*)(bytes.data() + 2), (bytes.size() - 2) / 2);
        return true;
    }
    if (bytes.size() >= 3 && (BYTE)bytes[0] == 0xEF) {
        bytes.erase(0, 3);
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, bytes.data(),
                                           (int)bytes.size(), nullptr, 0);
    if (length > 0) {
        text->resize(length);
        MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(),
                            text->data(), length);
    }
    return true;
}

size_t SkipJsonSpaces(std::wstring_view text, size_t at) {
    while (at < text.size() && iswspace(text[at])) {
        at++;
    }
    return at;
}

// Where the JSON value starting at `start` ends; npos if it's cut.
size_t JsonValueEnd(std::wstring_view text, size_t start) {
    if (start >= text.size()) {
        return std::wstring_view::npos;
    }
    if (text[start] != L'"' && text[start] != L'{' && text[start] != L'[') {
        size_t end = start;
        while (end < text.size() && !iswspace(text[end]) &&
               !wcschr(L",}]", text[end])) {
            end++;
        }
        return end;
    }
    int depth = 0;
    bool quoted = false;
    for (size_t i = start; i < text.size(); i++) {
        const WCHAR c = text[i];
        if (quoted) {
            if (c == L'\\') {
                i++;
            } else if (c == L'"') {
                quoted = false;
                if (depth == 0) {
                    return i + 1;
                }
            }
        } else if (c == L'"') {
            quoted = true;
        } else if (c == L'{' || c == L'[') {
            depth++;
        } else if ((c == L'}' || c == L']') && --depth == 0) {
            return i + 1;
        }
    }
    return std::wstring_view::npos;
}

// Git Bash, which Claude Code runs its status line in when it's there
// (found as Claude Code finds it: CLAUDE_CODE_GIT_BASH_PATH, or beside
// git); without it, PowerShell runs it. (The 64-bit Program Files too: the
// island may be a 32-bit process.)
bool GitBashFound() {
    auto exists = [](const std::wstring& path) {
        return !path.empty() &&
               GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
    };
    if (const std::wstring set = ExpandedPath(L"%CLAUDE_CODE_GIT_BASH_PATH%");
        !set.empty()) {
        return exists(set);
    }
    WCHAR git[MAX_PATH];
    if (SearchPath(nullptr, L"git.exe", nullptr, MAX_PATH, git, nullptr)) {
        // (...\Git\cmd\git.exe or ...\Git\bin\git.exe.)
        std::wstring root = git;
        for (int up = 0; up < 2; up++) {
            const size_t slash = root.rfind(L'\\');
            root.resize(slash == std::wstring::npos ? 0 : slash);
        }
        if (exists(root + L"\\bin\\bash.exe")) {
            return true;
        }
    }
    return exists(ExpandedPath(L"%ProgramW6432%\\Git\\bin\\bash.exe")) ||
           exists(ExpandedPath(L"%ProgramFiles%\\Git\\bin\\bash.exe"));
}

// The folders' or Codex logs' (rollout-*.jsonl) entries of a folder: the
// folders by name (Codex's years, months and days, as numbers), the logs by
// when they were written, newest first, with when each was written.
std::vector<std::pair<std::wstring, long long>> NewestEntries(
    const std::wstring& folder,
    bool folders) {
    std::vector<std::pair<std::wstring, long long>> entries;
    WIN32_FIND_DATA data;
    HANDLE find = FindFirstFileEx(
        (folder + (folders ? L"\\*" : L"\\rollout-*.jsonl")).c_str(),
        FindExInfoBasic, &data, FindExSearchNameMatch, nullptr, 0);
    if (find == INVALID_HANDLE_VALUE) {
        return entries;
    }
    do {
        const bool isFolder = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        if (isFolder == folders && data.cFileName[0] != L'.') {
            entries.push_back(
                {data.cFileName, UnixSeconds(data.ftLastWriteTime)});
        }
    } while (FindNextFile(find, &data));
    FindClose(find);
    std::sort(entries.begin(), entries.end(),
              [folders](const auto& a, const auto& b) {
                  return folders ? a.first > b.first : a.second > b.second;
              });
    return entries;
}

// Codex's sessions' logs' folder (under CODEX_HOME, or ~/.codex; empty
// without either).
std::wstring CodexSessionsFolder() {
    std::wstring home = ExpandedPath(L"%CODEX_HOME%");
    if (home.empty()) {
        home = ExpandedPath(L"%USERPROFILE%\\.codex");
    }
    return home.empty() ? home : home + L"\\sessions";
}

// Its newest logs: the ones of its last days (a session keeps writing in
// the day it started), newest first.
std::vector<std::wstring> NewestCodexLogs() {
    std::vector<std::pair<std::wstring, long long>> logs;
    if (const std::wstring root = CodexSessionsFolder(); !root.empty()) {
        int days = 0;
        for (const auto& [year, yearWritten] : NewestEntries(root, true)) {
            const std::wstring yearFolder = root + L"\\" + year;
            for (const auto& [month, monthWritten] :
                 NewestEntries(yearFolder, true)) {
                const std::wstring monthFolder = yearFolder + L"\\" + month;
                for (const auto& [day, dayWritten] :
                     NewestEntries(monthFolder, true)) {
                    const std::wstring dayFolder = monthFolder + L"\\" + day;
                    for (const auto& [name, written] :
                         NewestEntries(dayFolder, false)) {
                        logs.push_back({dayFolder + L"\\" + name, written});
                    }
                    if (++days == 3) {
                        break;
                    }
                }
                if (days == 3) {
                    break;
                }
            }
            if (days == 3) {
                break;
            }
        }
    }
    std::stable_sort(logs.begin(), logs.end(),
                     [](const auto& a, const auto& b) {
                         return a.second > b.second;
                     });
    std::vector<std::wstring> paths;
    for (const auto& [path, written] : logs) {
        paths.push_back(path);
    }
    return paths;
}

// One of the windows Claude Code's status line gets ("five_hour" or
// "seven_day": "used_percentage" and "resets_at").
AiLimit ClaudeLimit(const Json& window, int minutes) {
    AiLimit limit;
    if (window.type == Json::Type::Object) {
        limit.used =
            std::clamp(window[L"used_percentage"].Number(-1), -1.0, 100.0);
        limit.resetsAt = (long long)window[L"resets_at"].Number(0);
        limit.minutes = minutes;
    }
    return limit;
}

