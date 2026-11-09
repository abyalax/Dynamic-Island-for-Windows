#pragma once
int DaysInMonth(int year, int month) {
    static const int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    return month == 2 && leap ? 29 : kDays[month - 1];
}

// 0 is Sunday.
int DayOfWeek(int year, int month, int day) {
    SYSTEMTIME time{(WORD)year, (WORD)month, 0, (WORD)day};
    FILETIME file;
    SystemTimeToFileTime(&time, &file);
    FileTimeToSystemTime(&file, &time);
    return time.wDayOfWeek;
}

// Events and reminders, kept by the mod (no accounts): a day, an optional
// time, a title, and whether to be reminded (in the pill, with a sound).
struct CalendarEvent {
    long long id = 0;
    int year = 0;
    int month = 0;
    int day = 0;
    // Minutes into the day, or -1 for the whole day.
    int minute = -1;
    std::wstring title;
    bool remind = true;
    bool fired = false;
};

std::vector<CalendarEvent> g_events;
bool g_eventsLoaded;
constexpr size_t kMaxEvents = 200;
// Whole-day events remind at this hour.
constexpr int kAllDayReminderMinute = 9 * 60;

void LoadEvents() {
    if (g_eventsLoaded) {
        return;
    }
    g_eventsLoaded = true;
    // (Read again after a backup is imported: what was there goes.)
    g_events.clear();
    const std::wstring value = GetWholeStringValue(L"events", 64 * 1024);
    // One event per line: id, year, month, day, minute, remind, fired, title
    // (tab-separated).
    for (PCWSTR line = value.data(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        const std::wstring text =
            end ? std::wstring(line, end) : std::wstring(line);
        std::vector<std::wstring> fields;
        size_t at = 0;
        for (int i = 0; i < 7; i++) {
            const size_t tab = text.find(L'\t', at);
            if (tab == std::wstring::npos) {
                break;
            }
            fields.push_back(text.substr(at, tab - at));
            at = tab + 1;
        }
        if (fields.size() == 7) {
            CalendarEvent event;
            event.id = _wtoi64(fields[0].c_str());
            event.year = _wtoi(fields[1].c_str());
            event.month = _wtoi(fields[2].c_str());
            event.day = _wtoi(fields[3].c_str());
            event.minute = _wtoi(fields[4].c_str());
            event.remind = fields[5] == L"1";
            event.fired = fields[6] == L"1";
            event.title = text.substr(at);
            if (event.month >= 1 && event.month <= 12 && event.day >= 1 &&
                event.day <= 31) {
                g_events.push_back(std::move(event));
            }
        }
        if (!end) {
            break;
        }
        line = end + 1;
    }
}

void SaveEvents() {
    std::wstring value;
    for (const auto& event : g_events) {
        value += std::to_wstring(event.id) + L"\t" +
                 std::to_wstring(event.year) + L"\t" +
                 std::to_wstring(event.month) + L"\t" +
                 std::to_wstring(event.day) + L"\t" +
                 std::to_wstring(event.minute) + L"\t" +
                 (event.remind ? L"1" : L"0") + L"\t" +
                 (event.fired ? L"1" : L"0") + L"\t" + event.title + L"\n";
    }
    SetStoredString(L"events", value.c_str());
}

// Public holidays, in the calendar when chosen in the island's settings (off
// by default, since it goes online): from the free Nager.Date service
// (date.nager.at, over 200 countries), for the country of Windows' region,
// a year at a time, downloaded once and kept (value "holidays"). The lists
// are touched under the lock: they're filled on a thread of their own.
struct Holiday {
    int year;
    int month;
    int day;
    std::wstring name;
};
SRWLOCK g_holidaysLock = SRWLOCK_INIT;
std::vector<Holiday> g_holidays;
// Their country, and the years known (some with none).
std::wstring g_holidaysCountry;
std::vector<int> g_holidayYears;
bool g_holidaysLoaded;
// Tells the calendar a year came (wParam: the year).
constexpr UINT WM_APP_HOLIDAYS = WM_APP + 66;

// The country of Windows' region (two letters), or nothing.
std::wstring HolidayCountry() {
    using GetGeoName_t = int(WINAPI*)(LPWSTR, int);
    auto getGeoName = (GetGeoName_t)GetProcAddress(
        GetModuleHandle(L"kernel32.dll"), "GetUserDefaultGeoName");
    WCHAR geo[16] = L"";
    if (!getGeoName || getGeoName(geo, ARRAYSIZE(geo)) != 3 ||
        !iswalpha(geo[0]) || !iswalpha(geo[1])) {
        return {};
    }
    return geo;
}

// From the value: the country on the first line, the years known on the
// second, then a holiday per line (year, month, day, name; tab-separated).
// Another country (the region changed): nothing.
void LoadHolidays() {
    if (g_holidaysLoaded) {
        return;
    }
    g_holidaysLoaded = true;
    const std::wstring value = GetWholeStringValue(L"holidays", 32 * 1024);
    std::vector<std::wstring> lines;
    for (PCWSTR line = value.data(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        lines.push_back(end ? std::wstring(line, end) : std::wstring(line));
        line = end ? end + 1 : line + wcslen(line);
    }
    const std::wstring country = HolidayCountry();
    AcquireSRWLockExclusive(&g_holidaysLock);
    g_holidaysCountry = country;
    if (lines.size() >= 2 && lines[0] == country && !country.empty()) {
        for (PCWSTR p = lines[1].c_str(); *p;) {
            g_holidayYears.push_back(_wtoi(p));
            PCWSTR comma = wcschr(p, L',');
            p = comma ? comma + 1 : p + wcslen(p);
        }
        for (size_t i = 2; i < lines.size(); i++) {
            Holiday holiday{};
            int at = 0;
            if (swscanf_s(lines[i].c_str(), L"%d\t%d\t%d\t%n", &holiday.year,
                          &holiday.month, &holiday.day, &at) == 3 &&
                at > 0) {
                holiday.name = lines[i].substr(at);
                g_holidays.push_back(std::move(holiday));
            }
        }
    }
    ReleaseSRWLockExclusive(&g_holidaysLock);
}

void SaveHolidays() {
    AcquireSRWLockShared(&g_holidaysLock);
    std::wstring value = g_holidaysCountry + L"\n";
    for (size_t i = 0; i < g_holidayYears.size(); i++) {
        value += (i ? L"," : L"") + std::to_wstring(g_holidayYears[i]);
    }
    value += L"\n";
    for (const auto& holiday : g_holidays) {
        value += std::to_wstring(holiday.year) + L"\t" +
                 std::to_wstring(holiday.month) + L"\t" +
                 std::to_wstring(holiday.day) + L"\t" + holiday.name + L"\n";
    }
    ReleaseSRWLockShared(&g_holidaysLock);
    SetStoredString(L"holidays", value.c_str());
}

// The holidays of a day (when shown).
std::vector<std::wstring> HolidaysOn(int year, int month, int day) {
    std::vector<std::wstring> names;
    if (!g_settings.calendarHolidays) {
        return names;
    }
    AcquireSRWLockShared(&g_holidaysLock);
    for (const auto& holiday : g_holidays) {
        if (holiday.year == year && holiday.month == month &&
            holiday.day == day &&
            std::find(names.begin(), names.end(), holiday.name) ==
                names.end()) {
            names.push_back(holiday.name);
        }
    }
    ReleaseSRWLockShared(&g_holidaysLock);
    return names;
}

// The next `count` holidays from today (when shown).
std::vector<Holiday> UpcomingHolidays(size_t count) {
    std::vector<Holiday> upcoming;
    if (!g_settings.calendarHolidays) {
        return upcoming;
    }
    SYSTEMTIME today;
    GetLocalTime(&today);
    const int now = today.wYear * 10000 + today.wMonth * 100 + today.wDay;
    AcquireSRWLockShared(&g_holidaysLock);
    for (const auto& holiday : g_holidays) {
        if (holiday.year * 10000 + holiday.month * 100 + holiday.day >= now) {
            upcoming.push_back(holiday);
        }
    }
    ReleaseSRWLockShared(&g_holidaysLock);
    std::sort(upcoming.begin(), upcoming.end(),
              [](const Holiday& a, const Holiday& b) {
                  return std::tie(a.year, a.month, a.day) <
                         std::tie(b.year, b.month, b.day);
              });
    // A day with two names counts once.
    upcoming.erase(std::unique(upcoming.begin(), upcoming.end(),
                               [](const Holiday& a, const Holiday& b) {
                                   return a.year == b.year &&
                                          a.month == b.month && a.day == b.day;
                               }),
                   upcoming.end());
    if (upcoming.size() > count) {
        upcoming.resize(count);
    }
    return upcoming;
}

// The holidays in the service's answer (a list of objects with "date":
// "YYYY-MM-DD" and "localName", the name in the country's language).
std::vector<Holiday> ParseHolidays(const std::wstring& json) {
    std::vector<Holiday> holidays;
    size_t at = 0;
    while ((at = json.find(L"\"date\":\"", at)) != std::wstring::npos) {
        at += 8;
        Holiday holiday{};
        if (swscanf_s(json.c_str() + at, L"%d-%d-%d", &holiday.year,
                      &holiday.month, &holiday.day) != 3) {
            continue;
        }
        const size_t nameAt = json.find(L"\"localName\":\"", at);
        const size_t nextDate = json.find(L"\"date\":\"", at);
        if (nameAt == std::wstring::npos ||
            (nextDate != std::wstring::npos && nameAt > nextDate)) {
            continue;
        }
        // Only the country's own (not a state's or a region's).
        const size_t regional = json.find(L"\"global\":false", at);
        if (regional != std::wstring::npos &&
            (nextDate == std::wstring::npos || regional < nextDate)) {
            continue;
        }
        // The string, with its escapes.
        for (size_t i = nameAt + 13; i < json.size() && json[i] != L'"'; i++) {
            WCHAR c = json[i];
            if (c == L'\\' && i + 1 < json.size()) {
                const WCHAR next = json[++i];
                if (next == L'u' && i + 4 < json.size()) {
                    c = (WCHAR)wcstoul(json.substr(i + 1, 4).c_str(), nullptr,
                                       16);
                    i += 4;
                } else {
                    c = next == L'n' || next == L't' ? L' ' : next;
                }
            }
            holiday.name += c;
        }
        if (holiday.month >= 1 && holiday.month <= 12 && holiday.day >= 1 &&
            holiday.day <= 31 && !holiday.name.empty() &&
            holiday.name.size() < 100) {
            holidays.push_back(std::move(holiday));
        }
    }
    return holidays;
}

// A request under way that another thread can end at once (see
// CancelHttps): its handle, closed there, makes WinHTTP's waiting call give
// up; and once canceled, no other one starts.
struct HttpsCancel {
    SRWLOCK lock = SRWLOCK_INIT;
    HINTERNET request = nullptr;
    bool canceled = false;
};

void CancelHttps(HttpsCancel* cancel) {
    AcquireSRWLockExclusive(&cancel->lock);
    cancel->canceled = true;
    if (cancel->request) {
        WinHttpCloseHandle(std::exchange(cancel->request, nullptr));
    }
    ReleaseSRWLockExclusive(&cancel->lock);
}

bool HttpsGet(const std::wstring& url,
              std::vector<BYTE>* data,
              size_t maxSize,
              DWORD waitMs = 3000,
              const std::atomic<bool>* stop = nullptr,
              HttpsCancel* cancel = nullptr,
              DWORD* answer = nullptr);
bool ReadTextFile(PCWSTR path, size_t limit, std::wstring* text);

