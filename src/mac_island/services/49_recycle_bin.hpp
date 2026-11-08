// Whether its item shows on the pill: on, with enough in it (see
// kRecycleMinMegabytes; not counted yet, only "always").
bool RecycleBinShown() {
    const int at = std::clamp(g_settings.islandRecycleMinSize, 0,
                              (int)std::size(kRecycleMinMegabytes) - 1);
    return g_settings.islandRecycleBin &&
           g_recycleKilobytes >= kRecycleMinMegabytes[at] * 1024LL;
}

DWORD WINAPI RecycleCountProc(LPVOID parameter) {
    SHQUERYRBINFO info{sizeof(info)};
    const bool counted = SUCCEEDED(SHQueryRecycleBin(nullptr, &info));
    const WPARAM items =
        counted ? (WPARAM)std::min<long long>(info.i64NumItems, INT_MAX)
                : (WPARAM)-1;
    const LPARAM kilobytes =
        counted ? (LPARAM)std::min<long long>(info.i64Size / 1024, INT_MAX)
                : (LPARAM)-1;
    PostMessage((HWND)parameter, WM_APP_RECYCLE_BIN, items, kilobytes);
    return 0;
}

DWORD WINAPI RecycleEmptyProc(LPVOID parameter) {
    // (Windows' own emptying, its sound too, without its question: the
    // panel asked already.)
    const HRESULT result = SHEmptyRecycleBin(
        nullptr, nullptr, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI);
    // (Already empty: nothing to do, not a failure.)
    PostMessage((HWND)parameter, WM_APP_RECYCLE_EMPTIED,
                SUCCEEDED(result) || result == E_UNEXPECTED, 0);
    return 0;
}

// "12 items · 1.2 GB", or "Nothing in it".
std::wstring RecycleBinText(long long items, long long kilobytes) {
    if (items <= 0) {
        return Tr(L"Nothing in it", L"Vazia");
    }
    WCHAR count[32];
    if (items == 1) {
        wcscpy_s(count, Tr(L"1 item", L"1 item"));
    } else {
        swprintf_s(count, Tr(L"%d items", L"%d itens"), (int)items);
    }
    WCHAR size[32] = L"";
    StrFormatByteSizeW(kilobytes * 1024, size, ARRAYSIZE(size));
    return std::wstring(count) + L" · " + size;
}

// ---- The timer, the stopwatch and Pomodoro (islandTimer): each its own, all
// three can run at once; for the whole island (the main island's thread),
// kept while the island restarts. The timer's length and Pomodoro's focus and
// break (hours, minutes and seconds) are kept in "timerLength",
// "pomodoroFocus" and "pomodoroBreak".
enum class TimerMode { Timer, Stopwatch, Pomodoro };
constexpr int kTimerModes = 3;

// One of them: running since when, and the time counted before (paused).
struct TimerClock {
    bool running = false;
    double startedAt = 0;
    double counted = 0;
    double Counted() const {
        return counted + (running ? NowSeconds() - startedAt : 0);
    }
    bool Active() const { return running || counted > 0; }
    void Start() {
        if (!running) {
            running = true;
            startedAt = NowSeconds();
        }
    }
    void Pause() {
        if (running) {
            counted = Counted();
            running = false;
        }
    }
    void Reset() {
        running = false;
        counted = 0;
    }
};

struct TimerState {
    TimerClock clocks[kTimerModes];
    // The timer's length, and Pomodoro's focus and break, in seconds (read
    // once, see LoadTimerLengths).
    int seconds = 5 * 60;
    int focusSeconds = 25 * 60;
    int breakSeconds = 5 * 60;
    bool lengthsLoaded = false;
    // Pomodoro: on a break, and the rounds of focus done.
    bool onBreak = false;
    int rounds = 0;
};
TimerState g_timer;
constexpr int kTimerLongest = 24 * 3600 - 1;

void LoadTimerLengths() {
    if (g_timer.lengthsLoaded) {
        return;
    }
    g_timer.lengthsLoaded = true;
    // (The account's own, see UserValueName: another account's island keeps
    // its own; each starts from the lengths all used before.)
    g_timer.seconds = std::clamp(
        GetUserIntValue(L"timerLength", 5 * 60, true), 1, kTimerLongest);
    g_timer.focusSeconds = std::clamp(
        GetUserIntValue(L"pomodoroFocus", 25 * 60, true), 1, kTimerLongest);
    g_timer.breakSeconds = std::clamp(
        GetUserIntValue(L"pomodoroBreak", 5 * 60, true), 1, kTimerLongest);
}

void SaveTimerLengths() {
    SetUserIntValue(L"timerLength", g_timer.seconds);
    SetUserIntValue(L"pomodoroFocus", g_timer.focusSeconds);
    SetUserIntValue(L"pomodoroBreak", g_timer.breakSeconds);
}

TimerClock& TimerClockOf(TimerMode mode) {
    return g_timer.clocks[(int)mode];
}

// How long the timer or Pomodoro's part lasts (0 for the stopwatch).
int TimerLength(TimerMode mode) {
    LoadTimerLengths();
    switch (mode) {
        case TimerMode::Timer:
            return g_timer.seconds;
        case TimerMode::Pomodoro:
            return g_timer.onBreak ? g_timer.breakSeconds
                                   : g_timer.focusSeconds;
        default:
            return 0;
    }
}

double TimerCounted(TimerMode mode) {
    return TimerClockOf(mode).Counted();
}

// Whether it shows as running (or paused) rather than ready to start.
bool TimerActive(TimerMode mode) {
    return TimerClockOf(mode).Active();
}

bool AnyTimerActive() {
    return TimerActive(TimerMode::Timer) ||
           TimerActive(TimerMode::Stopwatch) ||
           TimerActive(TimerMode::Pomodoro);
}

// The time a countdown has left (-1 for the stopwatch).
double TimerLeft(TimerMode mode) {
    const int length = TimerLength(mode);
    return length ? std::max(0.0, length - TimerCounted(mode)) : -1;
}

// "4:59" or "1:02:03" (with hundredths, "1:02.34").
std::wstring DurationText(double seconds, bool hundredths) {
    const int whole = (int)std::max(0.0, seconds);
    WCHAR text[48];
    if (whole >= 3600) {
        swprintf_s(text, L"%d:%02d:%02d", whole / 3600, whole / 60 % 60,
                   whole % 60);
    } else {
        swprintf_s(text, L"%d:%02d", whole / 60, whole % 60);
    }
    std::wstring result = text;
    if (hundredths) {
        swprintf_s(text, L".%02d",
                   std::clamp((int)((seconds - whole) * 100), 0, 99));
        result += text;
    }
    return result;
}

// What it shows: the time left (the stopwatch: the time counted).
std::wstring TimerText(TimerMode mode, bool hundredths = false) {
    const double left = TimerLeft(mode);
    return left >= 0 ? DurationText(std::ceil(left), false)
                     : DurationText(TimerCounted(mode), hundredths);
}

PCWSTR TimerModeName(TimerMode mode, bool onBreak) {
    switch (mode) {
        case TimerMode::Stopwatch:
            return Tr(L"Stopwatch", L"Cronômetro");
        case TimerMode::Pomodoro:
            return onBreak ? Tr(L"Break", L"Pausa") : Tr(L"Focus", L"Foco");
        default:
            return Tr(L"Timer", L"Timer");
    }
}

// The ones the pill shows (see PillGroup::Timer): the countdown that ends
// first, with any ending in the same second; the stopwatch when it's the
// only one going.
std::vector<TimerMode> PillTimers() {
    std::vector<TimerMode> shown;
    double first = 1e18;
    for (TimerMode mode : {TimerMode::Timer, TimerMode::Pomodoro}) {
        if (TimerActive(mode)) {
            first = std::min(first, std::ceil(TimerLeft(mode)));
        }
    }
    for (TimerMode mode : {TimerMode::Timer, TimerMode::Pomodoro}) {
        if (TimerActive(mode) && std::ceil(TimerLeft(mode)) == first) {
            shown.push_back(mode);
        }
    }
    if (shown.empty() && TimerActive(TimerMode::Stopwatch)) {
        shown.push_back(TimerMode::Stopwatch);
    }
    return shown;
}

// Alarms (the timer panel's last tab, #6156): a time on the days chosen,
// each on or off, with or without a sound (the notice always shows), with
// an optional name, kept in "alarms" as "minute,on,sound,days,name" a line
// (before 3.0 "minute,on,sound": every day). The island's thread.
// (Its days: bit 0 Sunday to bit 6 Saturday, as SYSTEMTIME's wDayOfWeek;
// none = once, turned off after it rings.)
constexpr int kAlarmEveryDay = 0x7F;
constexpr int kAlarmWeekdays = 0x3E;
constexpr int kAlarmWeekends = 0x41;
constexpr size_t kMaxAlarmName = 32;
struct Alarm {
    int minute = 7 * 60;
    bool on = true;
    bool sound = true;
    int days = kAlarmEveryDay;
    std::wstring name;
};
constexpr size_t kMaxAlarms = 8;

// (The account's own, see UserValueName: another account's island doesn't
// ring them. Read once, and again after a backup is restored.)
std::vector<Alarm> g_alarms;
bool g_alarmsRead = false;

std::vector<Alarm>& Alarms() {
    auto& alarms = g_alarms;
    if (!g_alarmsRead) {
        g_alarmsRead = true;
        alarms.clear();
        // (Room for kMaxAlarms with names of kMaxAlarmName: one read.)
        const std::wstring text = GetWholeUserStringValue(L"alarms", 512);
        size_t start = 0;
        while (start < text.size() && alarms.size() < kMaxAlarms) {
            size_t end = text.find(L'\n', start);
            if (end == std::wstring::npos) {
                end = text.size();
            }
            // Its numbers (the days missing: every day), then its name.
            const std::wstring line = text.substr(start, end - start);
            long values[4] = {0, 0, 0, kAlarmEveryDay};
            int count = 0;
            PCWSTR at = line.c_str();
            for (; count < 4; count++) {
                if (count && *at++ != L',') {
                    break;
                }
                PWSTR next = nullptr;
                values[count] = wcstol(at, &next, 10);
                if (next == at) {
                    break;
                }
                at = next;
            }
            if (count >= 3 && values[0] >= 0 && values[0] < 24 * 60) {
                Alarm alarm{(int)values[0], values[1] != 0, values[2] != 0,
                            (int)values[3] & kAlarmEveryDay};
                if (count == 4 && *at == L',') {
                    alarm.name = std::wstring(at + 1).substr(0, kMaxAlarmName);
                }
                alarms.push_back(std::move(alarm));
            }
            start = end + 1;
        }
    }
    return alarms;
}

void SaveAlarms() {
    std::wstring text;
    for (const auto& alarm : Alarms()) {
        text += std::to_wstring(alarm.minute) + L"," +
                (alarm.on ? L"1" : L"0") + L"," + (alarm.sound ? L"1" : L"0") +
                L"," + std::to_wstring(alarm.days) + L"," + alarm.name + L"\n";
    }
    SetUserStringValue(L"alarms", text.c_str());
}

// The week's first day in Windows' settings (0 Sunday, as wDayOfWeek) and
// the weekdays' names (0 Sunday): short ("Mon") and shortest ("M"), in the
// island's language. Asked on every draw of the alarms tab, so they're kept
// until the island's language changes or Windows' settings do (see
// ForgetWeekdays).
struct WeekdayTexts {
    int language = -1;
    int first = 0;
    std::wstring names[2][7];
};
WeekdayTexts g_weekdays;

void ForgetWeekdays() {
    g_weekdays.language = -1;
}

const WeekdayTexts& Weekdays() {
    if (g_weekdays.language == g_textLanguage) {
        return g_weekdays;
    }
    g_weekdays.language = g_textLanguage;
    WCHAR text[4] = L"6";
    GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, text,
                    ARRAYSIZE(text));
    // (LOCALE_IFIRSTDAYOFWEEK: 0 is Monday.)
    g_weekdays.first = (_wtoi(text) + 1) % 7;
    for (int shortest = 0; shortest < 2; shortest++) {
        for (int day = 0; day < 7; day++) {
            WCHAR name[16] = L"";
            // (LOCALE_S...DAYNAME1 is Monday.)
            GetLocaleInfoEx(DateLocale(),
                            (shortest ? LOCALE_SSHORTESTDAYNAME1
                                      : LOCALE_SABBREVDAYNAME1) +
                                (day + 6) % 7,
                            name, ARRAYSIZE(name));
            g_weekdays.names[shortest][day] = name;
        }
    }
    return g_weekdays;
}

int FirstWeekday() {
    return Weekdays().first;
}

std::wstring WeekdayName(int day, bool shortest) {
    return Weekdays().names[shortest][day];
}

// When an alarm rings: "Every day", "Once", "Weekdays", "Weekends", or its
// days in the week's order ("Mon, Wed, Fri").
std::wstring AlarmDaysText(int days) {
    if (days == kAlarmEveryDay) {
        return Tr(L"Every day", L"Todos os dias");
    }
    if (!days) {
        return Tr(L"Once", L"Uma vez");
    }
    if (days == kAlarmWeekdays) {
        return Tr(L"Weekdays", L"Dias úteis");
    }
    if (days == kAlarmWeekends) {
        return Tr(L"Weekends", L"Fins de semana");
    }
    std::wstring text;
    const int first = FirstWeekday();
    for (int i = 0; i < 7; i++) {
        const int day = (first + i) % 7;
        if (days >> day & 1) {
            text += (text.empty() ? L"" : L", ") + WeekdayName(day, false);
        }
    }
    return text;
}

// The minutes until an alarm rings next, by its days (from 1: its minute
// now has rung already).
int AlarmMinutesUntil(const Alarm& alarm, const SYSTEMTIME& now) {
    const int nowMinute = now.wHour * 60 + now.wMinute;
    const int first = (alarm.minute - nowMinute + 24 * 60 - 1) % (24 * 60) + 1;
    for (int week = 0; week < 7; week++) {
        const int until = first + week * 24 * 60;
        const int day = (now.wDayOfWeek + (nowMinute + until) / (24 * 60)) % 7;
        if (!alarm.days || (alarm.days >> day & 1)) {
            return until;
        }
    }
    return first;
}

// An alarm's time as the clock shows times (24 or 12 hours); with 12, its AM
// or PM in `marker` when given.
std::wstring AlarmTimeText(int minute, std::wstring* marker = nullptr) {
    SYSTEMTIME time{};
    time.wYear = 2000;
    time.wMonth = 1;
    time.wDay = 1;
    time.wHour = (WORD)(minute / 60);
    time.wMinute = (WORD)(minute % 60);
    return FormatClockTime(time, false, marker);
}

// Whether the clock shows 12 hours (an alarm's hours then go 1 to 12, with
// AM and PM).
bool ClockShows12Hours() {
    return AlarmTimeText(13 * 60).find(L"13") == std::wstring::npos;
}

