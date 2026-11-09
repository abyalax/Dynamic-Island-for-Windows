#pragma once
class DrivesPanel : public Panel {
   public:
    // The drives now (from the island). An ejected one that's gone stays a
    // few seconds, saying it can be removed (the list closes after it when
    // it was the last); the others forget how ejecting went.
    void SetDrives(std::vector<RemovableDrive> drives) {
        auto present = [&](WCHAR letter) {
            return std::any_of(
                drives.begin(), drives.end(),
                [&](const RemovableDrive& drive) {
                    return drive.letter == letter;
                });
        };
        std::erase_if(m_gone, [&](const RemovableDrive& drive) {
            return present(drive.letter);
        });
        for (const auto& drive : m_drives) {
            const EjectState* state = State(drive.letter);
            if (!present(drive.letter) && state &&
                *state == EjectState::Removable && IsOpen()) {
                m_gone.push_back(drive);
                SetTimer(m_hwnd, kGoneTimerId, kGoneMs, nullptr);
            }
        }
        m_drives = std::move(drives);
        std::erase_if(m_states, [&](const auto& state) {
            return !present(state.first);
        });
        if (IsOpen()) {
            Layout();
            Render();
        }
    }

   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_iconFormat = MakeFormat(IconFont(),
                                      DWRITE_FONT_WEIGHT_NORMAL, 15 * scale);
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           13 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_smallFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11 * scale, DWRITE_TEXT_ALIGNMENT_CENTER);
            // How ejecting went: on as many lines as it takes.
            m_stateFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            if (m_stateFormat) {
                m_stateFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
                m_stateFormat->SetParagraphAlignment(
                    DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            }
        }
        Layout();
    }

    void OnOpen() override {
        m_hover = -1;
        m_gone.clear();
        Layout();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

    // The ejected drives' rows go; the list closes when nothing is left.
    bool OnTimer(UINT_PTR id) override {
        if (id != kGoneTimerId || m_gone.empty()) {
            return false;
        }
        m_gone.clear();
        if (m_drives.empty()) {
            Close();
            // The island lets go of its icon (see Island::GroupWanted).
            PostMessage(IslandWindow(m_owner), WM_APP_DRIVES, 0, 0);
            return false;
        }
        Layout();
        return true;
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
        if (m_hover < 0) {
            return false;
        }
        m_hover = -1;
        return true;
    }

    bool OnMouseUp(POINT pt) override {
        const int hit = HitTest(pt);
        const int index = hit / kPartsPerDrive;
        if (hit < 0 || index >= (int)m_drives.size()) {
            return false;
        }
        const WCHAR letter = m_drives[index].letter;
        if (hit % kPartsPerDrive == kPartEject) {
            StartEject(letter);
            return true;
        }
        // The drive, or "Open": opened in Explorer.
        const std::wstring root = std::wstring(1, letter) + L":\\";
        Close();
        ShellExecute(nullptr, L"open", root.c_str(), nullptr, nullptr,
                     SW_SHOWNORMAL);
        return false;
    }

    bool OnAppMessage(UINT msg, WPARAM wParam, LPARAM) override;

    void OnDestroy() override {
        if (m_ejectThread) {
            WaitForSingleObject(m_ejectThread, INFINITE);
            CloseHandle(m_ejectThread);
            m_ejectThread = nullptr;
        }
    }

   private:
    // How long an ejected drive that's gone stays in the list.
    static constexpr UINT_PTR kGoneTimerId = 1;
    static constexpr UINT kGoneMs = 4000;
    static constexpr int kPartsPerDrive = 4;
    static constexpr int kPartRow = 0;
    static constexpr int kPartOpen = 1;
    static constexpr int kPartEject = 2;
    // How ejecting a drive went.
    enum class EjectState { Ejecting, Removable, InUse };

    void StartEject(WCHAR letter) {
        if (m_ejectThread) {
            if (WaitForSingleObject(m_ejectThread, 0) != WAIT_OBJECT_0) {
                return;
            }
            CloseHandle(m_ejectThread);
            m_ejectThread = nullptr;
        }
        auto* job = new EjectJob{m_hwnd, letter};
        m_ejectThread =
            CreateThread(nullptr, 0, EjectThreadProc, job, 0, nullptr);
        if (!m_ejectThread) {
            delete job;
            return;
        }
        SetState(letter, EjectState::Ejecting);
    }

    void SetState(WCHAR letter, EjectState state) {
        for (auto& entry : m_states) {
            if (entry.first == letter) {
                entry.second = state;
                return;
            }
        }
        m_states.push_back({letter, state});
    }

    const EjectState* State(WCHAR letter) const {
        for (const auto& entry : m_states) {
            if (entry.first == letter) {
                return &entry.second;
            }
        }
        return nullptr;
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(kDrivesWidth * s);
        const float padding = std::round(6 * s);
        const float row = std::round(44 * s);
        const float button = std::round(26 * s);
        const float buttonWidth = std::round(64 * s);
        const float buttonGap = std::round(6 * s);
        // With how ejecting went under the name, a row has room for two
        // lines of it.
        const float stateRow = std::round(58 * s);
        float y = padding;
        m_rows.clear();
        m_open.clear();
        m_eject.clear();
        m_goneRows.clear();
        for (size_t i = 0; i < m_drives.size(); i++) {
            const float height = State(m_drives[i].letter) ? stateRow : row;
            const D2D1_RECT_F rect{padding, y, width - padding, y + height};
            m_rows.push_back(rect);
            const float top = std::round((rect.top + rect.bottom - button) / 2);
            const float right = rect.right - std::round(8 * s);
            m_eject.push_back(
                {right - buttonWidth, top, right, top + button});
            m_open.push_back({right - buttonWidth * 2 - buttonGap, top,
                              right - buttonWidth - buttonGap, top + button});
            y += height;
        }
        for (size_t i = 0; i < m_gone.size(); i++) {
            m_goneRows.push_back({padding, y, width - padding, y + stateRow});
            y += stateRow;
        }
        if (m_drives.empty() && m_gone.empty()) {
            m_empty = {padding, y, width - padding, y + row};
            y += row;
        } else {
            m_empty = {};
        }
        y += padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    int HitTest(POINT pt) const {
        for (size_t i = 0; i < m_rows.size(); i++) {
            const int base = (int)i * kPartsPerDrive;
            if (PointInRect(pt, m_eject[i])) {
                return base + kPartEject;
            }
            if (PointInRect(pt, m_open[i])) {
                return base + kPartOpen;
            }
            if (PointInRect(pt, m_rows[i])) {
                return base + kPartRow;
            }
        }
        return -1;
    }

    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallFormat;
    winrt::com_ptr<IDWriteTextFormat> m_stateFormat;
    float m_preparedScale = 0;
    std::vector<RemovableDrive> m_drives;
    // Ejected and gone, still shown (see SetDrives).
    std::vector<RemovableDrive> m_gone;
    std::vector<std::pair<WCHAR, EjectState>> m_states;
    std::vector<D2D1_RECT_F> m_rows, m_open, m_eject, m_goneRows;
    D2D1_RECT_F m_empty{};
    HANDLE m_ejectThread = nullptr;
    int m_hover = -1;
};

bool DrivesPanel::OnAppMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg != WM_APP_EJECTED) {
        return false;
    }
    if (m_ejectThread) {
        WaitForSingleObject(m_ejectThread, INFINITE);
        CloseHandle(m_ejectThread);
        m_ejectThread = nullptr;
    }
    SetState((WCHAR)wParam,
             lParam ? EjectState::Removable : EjectState::InUse);
    Layout();
    return true;
}

void DrivesPanel::Draw(ID2D1RenderTarget* target,
                       ID2D1SolidColorBrush* brush) {
    if (!m_iconFormat || !m_textFormat || !m_smallFormat || !m_stateFormat) {
        return;
    }
    const float s = m_scale;
    const float radius = std::round(9 * s);
    const float icon = std::round(36 * s);
    const D2D1::ColorF removable(0.2f, 0.78f, 0.35f, 1);
    // A name on the row's first line, and how ejecting went under it (in
    // its color).
    auto nameAndState = [&](const D2D1_RECT_F& row, float right,
                            const std::wstring& name, PCWSTR state,
                            const D2D1_COLOR_F& color) {
        const float nameBottom = row.top + std::round(28 * s);
        brush->SetColor(Fg(0.95f));
        target->DrawText(name.c_str(), (UINT32)name.size(), m_textFormat.get(),
                         {row.left + icon, state ? row.top + std::round(4 * s)
                                                 : row.top,
                          right, state ? nameBottom : row.bottom},
                         brush);
        if (state) {
            brush->SetColor(color);
            target->DrawText(state, (UINT32)wcslen(state),
                             m_stateFormat.get(),
                             {row.left + icon, nameBottom, right,
                              row.bottom - std::round(2 * s)},
                             brush);
        }
    };
    for (size_t i = 0; i < m_drives.size() && i < m_rows.size(); i++) {
        const RemovableDrive& drive = m_drives[i];
        const D2D1_RECT_F& row = m_rows[i];
        const int base = (int)i * kPartsPerDrive;
        const float lit = Hover(HoverKey(6200, base),
                                m_hover >= base &&
                                    m_hover < base + kPartsPerDrive);
        if (lit > 0.01f) {
            brush->SetColor(Fg(0.09f * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        const WCHAR glyph = 0xE88E;
        brush->SetColor(Fg(0.95f));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                  {row.left, row.top, row.left + icon, row.bottom},
                  brush);
        // Its name, and under it how ejecting went (the name in white, the
        // state in its color).
        const EjectState* state = State(drive.letter);
        const float textRight = m_open[i].left - std::round(6 * s);
        PCWSTR text =
            !state ? nullptr
            : *state == EjectState::Ejecting
                ? Tr(L"Ejecting…", L"Ejetando…")
            : *state == EjectState::Removable
                ? Tr(L"Safe to remove", L"Pode remover com segurança")
                : Tr(L"In use: close what's open on it",
                     L"Em uso: feche o que está aberto nele");
        nameAndState(row, textRight, DriveName(drive), text,
                     !state ? Fg(0.6f)
                     : *state == EjectState::Removable
                         ? D2D1_COLOR_F(removable)
                     : *state == EjectState::InUse
                         ? D2D1_COLOR_F(D2D1::ColorF(1.0f, 0.45f, 0.3f, 1))
                         : Fg(0.6f));
        auto button = [&](const D2D1_RECT_F& rect, int part, PCWSTR label,
                          bool accent) {
            const float buttonLit =
                Hover(HoverKey(6201, base + part), m_hover == base + part);
            const float buttonRadius = (rect.bottom - rect.top) / 2;
            brush->SetColor(accent ? AccentLit(0.9f, buttonLit)
                                   : Fg(0.1f + 0.08f * buttonLit));
            GlassFill(target, {rect, buttonRadius, buttonRadius}, brush);
            brush->SetColor(accent ? AccentInk(1) : Fg(0.9f));
            target->DrawText(label, (UINT32)wcslen(label), m_smallFormat.get(),
                             rect, brush);
        };
        button(m_open[i], kPartOpen, Tr(L"Open", L"Abrir"), false);
        button(m_eject[i], kPartEject, Tr(L"Eject", L"Ejetar"), true);
    }
    for (size_t i = 0; i < m_gone.size() && i < m_goneRows.size(); i++) {
        const D2D1_RECT_F& row = m_goneRows[i];
        const WCHAR glyph = 0xE88E;
        brush->SetColor(Fg(0.95f));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                  {row.left, row.top, row.left + icon, row.bottom},
                  brush);
        nameAndState(row, row.right - std::round(8 * s), DriveName(m_gone[i]),
                     Tr(L"Safe to remove", L"Pode remover com segurança"),
                     removable);
    }
    if (m_drives.empty() && m_gone.empty()) {
        PCWSTR none = Tr(L"No USB drives", L"Nenhum pendrive");
        brush->SetColor(Fg(0.5f));
        m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        target->DrawText(none, (UINT32)wcslen(none), m_smallFormat.get(),
                         {m_empty.left + std::round(12 * s), m_empty.top,
                          m_empty.right, m_empty.bottom},
                         brush);
        m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    }
}

// A tray icon's name: its tip's first line, or its program's name for one
// that sends none.
std::wstring TrayIconName(const TrayIcon& icon) {
    std::wstring text = icon.tip;
    if (text.find_first_not_of(L" \t\r\n") == std::wstring::npos) {
        text.clear();
        const std::wstring program = icon.key.substr(0, icon.key.find(L'|'));
        if (!program.empty()) {
            text = GetProgramName(program.c_str());
        }
    }
    return text;
}

// ---- Items added in 2.1: the network's speed, world clocks, the next
// event, the Recycle Bin.

// The bytes the physical adapters (a cable, Wi-Fi, a phone's) got and sent
// so far; false when the table can't be read.
bool ReadNetworkBytes(ULONG64* in, ULONG64* out) {
    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR || !table) {
        return false;
    }
    *in = *out = 0;
    for (ULONG i = 0; i < table->NumEntries; i++) {
        const MIB_IF_ROW2& row = table->Table[i];
        if (!row.InterfaceAndOperStatusFlags.HardwareInterface ||
            row.InterfaceAndOperStatusFlags.FilterInterface ||
            row.OperStatus != IfOperStatusUp) {
            continue;
        }
        *in += row.InOctets;
        *out += row.OutOctets;
    }
    FreeMibTable(table);
    return true;
}

// Bytes a second, short: "0K", "120K", "4.5M", "12M", "1.2G".
std::wstring FormatRate(double bytes) {
    WCHAR text[16];
    if (bytes >= 1e9) {
        swprintf_s(text, L"%.1fG", bytes / 1e9);
    } else if (bytes >= 1e7) {
        swprintf_s(text, L"%.0fM", bytes / 1e6);
    } else if (bytes >= 1e6) {
        swprintf_s(text, L"%.1fM", bytes / 1e6);
    } else {
        swprintf_s(text, L"%.0fK", bytes / 1e3);
    }
    return text;
}

// The network's speed now (the island's thread; measured once a second by
// the main island while one shows it, see Island::OnNetSpeedTimer).
struct NetSpeed {
    ULONG64 in = 0;
    ULONG64 out = 0;
    ULONGLONG at = 0;
    std::wstring down;
    std::wstring up;
};
NetSpeed g_netSpeed;

// One of Windows' additional clocks (Settings > Time & language > Date &
// time > Additional clocks): its name, its time and how many days it is
// ahead of here (-1, 0, 1).
struct WorldClock {
    std::wstring name;
    std::wstring time;
    int days = 0;
    bool operator==(const WorldClock&) const = default;
};

// The time zones found by their key name (looked up once each: Windows'
// list of them is long).
std::vector<std::pair<std::wstring, DYNAMIC_TIME_ZONE_INFORMATION>>
    g_worldClockZones;

const DYNAMIC_TIME_ZONE_INFORMATION* FindTimeZone(const std::wstring& key) {
    for (const auto& [name, zone] : g_worldClockZones) {
        if (_wcsicmp(name.c_str(), key.c_str()) == 0) {
            return &zone;
        }
    }
    DYNAMIC_TIME_ZONE_INFORMATION zone{};
    for (DWORD i = 0; EnumDynamicTimeZoneInformation(i, &zone) == ERROR_SUCCESS;
         i++) {
        if (_wcsicmp(zone.TimeZoneKeyName, key.c_str()) == 0) {
            g_worldClockZones.push_back({key, zone});
            return &g_worldClockZones.back().second;
        }
    }
    return nullptr;
}

// Windows keeps them under this key: watched for changes on a thread
// (RegNotifyChangeKeyValue, like the desktops), so a clock added or turned
// off in Windows' settings shows at once, not at the next minute.
constexpr WCHAR kTimeDateKey[] = L"Control Panel\\TimeDate";
constexpr UINT WM_APP_WORLD_CLOCKS = WM_APP + 97;

struct WorldClocksJob {
    HWND notify;
    HANDLE stop;
};

DWORD WINAPI WorldClocksThreadProc(LPVOID parameter) {
    std::unique_ptr<WorldClocksJob> job((WorldClocksJob*)parameter);
    HKEY key = nullptr;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, kTimeDateKey, 0, KEY_NOTIFY, &key) !=
        ERROR_SUCCESS) {
        return 0;
    }
    HANDLE changed = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!changed) {
        RegCloseKey(key);
        return 0;
    }
    for (;;) {
        if (RegNotifyChangeKeyValue(
                key, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                changed, TRUE) != ERROR_SUCCESS) {
            break;
        }
        const HANDLE events[] = {job->stop, changed};
        if (WaitForMultipleObjects(ARRAYSIZE(events), events, FALSE,
                                   INFINITE) != WAIT_OBJECT_0 + 1) {
            break;
        }
        PostMessage(job->notify, WM_APP_WORLD_CLOCKS, 0, 0);
    }
    RegCloseKey(key);
    CloseHandle(changed);
    return 0;
}

std::vector<WorldClock> ReadWorldClocks() {
    std::vector<WorldClock> clocks;
    SYSTEMTIME utc;
    SYSTEMTIME here;
    GetSystemTime(&utc);
    GetLocalTime(&here);
    for (PCWSTR key : {L"Control Panel\\TimeDate\\AdditionalClocks\\1",
                       L"Control Panel\\TimeDate\\AdditionalClocks\\2"}) {
        DWORD enabled = 0;
        DWORD size = sizeof(enabled);
        if (RegGetValue(HKEY_CURRENT_USER, key, L"Enable", RRF_RT_REG_DWORD,
                        nullptr, &enabled, &size) != ERROR_SUCCESS ||
            !enabled) {
            continue;
        }
        WCHAR name[64] = L"";
        WCHAR zoneName[128] = L"";
        DWORD nameSize = sizeof(name);
        DWORD zoneSize = sizeof(zoneName);
        RegGetValue(HKEY_CURRENT_USER, key, L"DisplayName", RRF_RT_REG_SZ,
                    nullptr, name, &nameSize);
        if (RegGetValue(HKEY_CURRENT_USER, key, L"TzRegKeyName",
                        RRF_RT_REG_SZ, nullptr, zoneName,
                        &zoneSize) != ERROR_SUCCESS) {
            continue;
        }
        const DYNAMIC_TIME_ZONE_INFORMATION* zone = FindTimeZone(zoneName);
        SYSTEMTIME there;
        if (!zone || !SystemTimeToTzSpecificLocalTimeEx(zone, &utc, &there)) {
            continue;
        }
        WorldClock clock;
        clock.name = name[0] ? name : zoneName;
        clock.time = FormatClockTime(there, false);
        clock.days = std::clamp(DayNumber(there.wYear, there.wMonth,
                                          there.wDay) -
                                    DayNumber(here.wYear, here.wMonth,
                                              here.wDay),
                                -1, 1);
        clocks.push_back(std::move(clock));
    }
    return clocks;
}

// The next event with a time (yours or an agenda's) in the next 12 hours:
// its title and how long until it starts, or false without one.
bool NextTimedEvent(std::wstring* title, int* minutes, int* year, int* month,
                    int* day) {
    SYSTEMTIME now;
    GetLocalTime(&now);
    const int today = DayNumber(now.wYear, now.wMonth, now.wDay);
    const int nowMinute = now.wHour * 60 + now.wMinute;
    int best = 12 * 60 + 1;
    auto consider = [&](int y, int m, int d, int minute,
                        const std::wstring& name) {
        if (minute < 0) {
            return;
        }
        const int until =
            (DayNumber(y, m, d) - today) * 24 * 60 + minute - nowMinute;
        if (until >= 0 && until < best) {
            best = until;
            *title = name.empty() ? std::wstring(Tr(L"Event", L"Evento")) : name;
            *year = y;
            *month = m;
            *day = d;
        }
    };
    for (const auto& event : g_events) {
        consider(event.year, event.month, event.day, event.minute, event.title);
    }
    for (const auto& item : UpcomingAgendaEvents(8)) {
        consider(item.event.year, item.event.month, item.event.day,
                 item.event.minute, item.event.title);
    }
    if (best > 12 * 60) {
        return false;
    }
    *minutes = best;
    return true;
}

// "in 25 min", "in 2 h 5 min", "starting now".
std::wstring UntilText(int minutes) {
    WCHAR text[48];
    if (minutes <= 0) {
        return Tr(L"starting now", L"começando agora");
    }
    if (minutes < 60) {
        swprintf_s(text, Tr(L"in %d min", L"em %d min"), minutes);
    } else if (minutes % 60 == 0) {
        swprintf_s(text, Tr(L"in %d h", L"em %d h"), minutes / 60);
    } else {
        swprintf_s(text, Tr(L"in %d h %d min", L"em %d h %d min"),
                   minutes / 60, minutes % 60);
    }
    return text;
}

// The Recycle Bin: changes in it told to the main island (see
// Island::UpdateRecycleWatch), which counts what's in it on a thread
// (WM_APP_RECYCLE_BIN: wParam the items, lParam the size in KB, or -1 each
// when it couldn't); emptying it told to its panel (wParam: whether it
// worked).
constexpr UINT WM_APP_RECYCLE_CHANGED = WM_APP + 92;
constexpr UINT WM_APP_RECYCLE_BIN = WM_APP + 93;
constexpr UINT WM_APP_RECYCLE_EMPTIED = WM_APP + 94;
// The timer changed in its panel: the pill shows it.
constexpr UINT WM_APP_TIMER_CHANGED = WM_APP + 95;
// What's in it, as last counted (-1 not yet; the island's thread).
long long g_recycleItems = -1;
long long g_recycleKilobytes = 0;

