#pragma once
// A link or a path as typed or pasted (quotes and spaces around it gone,
// webcal:// read as https://, a Google Calendar page's link as its iCal
// address); nothing when it's neither an https link nor a file's path on a
// drive.
std::wstring AgendaSource(std::wstring text) {
    const size_t start = text.find_first_not_of(L" \t\"'");
    if (start == std::wstring::npos) {
        return {};
    }
    text = text.substr(start, text.find_last_not_of(L" \t\"'") - start + 1);
    if (_wcsnicmp(text.c_str(), L"webcal://", 9) == 0) {
        text = L"https://" + text.substr(9);
    }
    const bool link =
        _wcsnicmp(text.c_str(), L"https://", 8) == 0 && text.size() > 8;
    if (link) {
        text = L"https://" + text.substr(8);
        if (const std::wstring google = GooglePublicIcs(text);
            !google.empty()) {
            text = google;
        }
    }
    // (A drive's path: not a network share's, which would connect to the
    // computer named with the user's sign-in, nor a device's.)
    const bool path = text.size() > 3 && iswalpha(text[0]) && text[1] == L':' &&
                      (text[2] == L'\\' || text[2] == L'/');
    return (link || path) && text.size() <= kMaxAgendaSource ? text
                                                             : std::wstring();
}

// What an agenda is called before it's read: its link's host, or its
// file's name.
std::wstring AgendaShortName(const std::wstring& source) {
    if (source.starts_with(L"https://")) {
        const size_t end = source.find(L'/', 8);
        return source.substr(8, end == std::wstring::npos ? end : end - 8);
    }
    const size_t slash = source.find_last_of(L"\\/");
    return slash == std::wstring::npos ? source : source.substr(slash + 1);
}

// A color for a new agenda: the first one none has.
int NextAgendaColor(const std::vector<Agenda>& agendas) {
    for (const int color : kAgendaColors) {
        if (std::none_of(
                agendas.begin(), agendas.end(),
                [&](const Agenda& agenda) { return agenda.color == color; })) {
            return color;
        }
    }
    return kAgendaColors[agendas.size() % std::size(kAgendaColors)];
}

// (Under the lock, exclusive.)
void IndexAgendaDays() {
    g_agendaDays.clear();
    for (const auto& agenda : g_agendas) {
        for (const auto& event : agenda.events) {
            g_agendaDays.try_emplace(
                event.year * 10000 + event.month * 100 + event.day,
                agenda.color);
        }
    }
}

// The calendar's agendas: the ones added in it (value "agendas": a line
// each, "u", its color and its source, tab-separated), then the settings'
// ones (whose colors "s" lines keep). Read as the calendar opens: what's
// already read stays.
void LoadAgendas() {
    const std::wstring value = GetWholeStringValue(L"agendas", 32 * 1024);
    std::vector<Agenda> wanted;
    std::vector<std::pair<std::wstring, int>> settingsColors;
    auto known = [&](const std::wstring& source) {
        return std::any_of(
            wanted.begin(), wanted.end(),
            [&](const Agenda& agenda) { return agenda.source == source; });
    };
    for (PCWSTR line = value.data(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        const std::wstring text =
            end ? std::wstring(line, end) : std::wstring(line);
        line = end ? end + 1 : line + wcslen(line);
        const size_t tab = text.find(L'\t');
        const size_t second =
            tab == std::wstring::npos ? tab : text.find(L'\t', tab + 1);
        if (second == std::wstring::npos) {
            continue;
        }
        const int color =
            (int)wcstoul(text.substr(tab + 1, second - tab - 1).c_str(),
                         nullptr, 16) &
            0xFFFFFF;
        const std::wstring source = AgendaSource(text.substr(second + 1));
        if (source.empty()) {
            continue;
        }
        if (text[0] == L's') {
            settingsColors.push_back({source, color});
        } else if (!known(source) && wanted.size() < kMaxAgendas) {
            Agenda agenda;
            agenda.source = source;
            agenda.color = color;
            wanted.push_back(std::move(agenda));
        }
    }
    for (const auto& source : g_calendarIcs) {
        if (known(source) || wanted.size() >= kMaxAgendas) {
            continue;
        }
        Agenda agenda;
        agenda.source = source;
        agenda.fromSettings = true;
        agenda.color = -1;
        for (const auto& [kept, color] : settingsColors) {
            if (kept == source) {
                agenda.color = color;
            }
        }
        if (agenda.color < 0) {
            agenda.color = NextAgendaColor(wanted);
        }
        wanted.push_back(std::move(agenda));
    }
    AcquireSRWLockExclusive(&g_agendasLock);
    for (auto& agenda : wanted) {
        for (auto& old : g_agendas) {
            if (old.source == agenda.source) {
                agenda.name = std::move(old.name);
                agenda.events = std::move(old.events);
                agenda.read = old.read;
                agenda.failed = old.failed;
                agenda.notIcs = old.notIcs;
                agenda.reading = old.reading;
                agenda.readAt = old.readAt;
                break;
            }
        }
    }
    g_agendas = std::move(wanted);
    IndexAgendaDays();
    ReleaseSRWLockExclusive(&g_agendasLock);
}

void SaveAgendas() {
    std::wstring value;
    AcquireSRWLockShared(&g_agendasLock);
    for (const auto& agenda : g_agendas) {
        WCHAR color[8];
        swprintf_s(color, L"%06X", agenda.color & 0xFFFFFF);
        value += std::wstring(agenda.fromSettings ? L"s\t" : L"u\t") + color +
                 L"\t" + agenda.source + L"\n";
    }
    ReleaseSRWLockShared(&g_agendasLock);
    SetStoredString(L"agendas", value.c_str());
}

// The color of a day with agenda events, if it has some.
bool AgendaDayColor(int year, int month, int day, int* color) {
    AcquireSRWLockShared(&g_agendasLock);
    const auto it = g_agendaDays.find(year * 10000 + month * 100 + day);
    const bool found = it != g_agendaDays.end();
    if (found) {
        *color = it->second;
    }
    ReleaseSRWLockShared(&g_agendasLock);
    return found;
}

// A day's agenda events: the whole-day ones, then by time.
std::vector<AgendaItem> AgendaEventsOn(int year, int month, int day) {
    std::vector<AgendaItem> items;
    AcquireSRWLockShared(&g_agendasLock);
    for (const auto& agenda : g_agendas) {
        for (const auto& event : agenda.events) {
            if (event.year == year && event.month == month &&
                event.day == day) {
                items.push_back({event, agenda.color});
            }
        }
    }
    ReleaseSRWLockShared(&g_agendasLock);
    std::stable_sort(items.begin(), items.end(),
                     [](const AgendaItem& a, const AgendaItem& b) {
                         return a.event.minute < b.event.minute;
                     });
    return items;
}

// The next `count` agenda events from now (today's whole-day ones too); the
// same one in two agendas counts once.
std::vector<AgendaItem> UpcomingAgendaEvents(size_t count) {
    SYSTEMTIME now;
    GetLocalTime(&now);
    const int today = now.wYear * 10000 + now.wMonth * 100 + now.wDay;
    const int minute = now.wHour * 60 + now.wMinute;
    auto day = [](const AgendaEvent& event) {
        return event.year * 10000 + event.month * 100 + event.day;
    };
    struct Candidate {
        const AgendaEvent* event;
        int color;
    };
    auto earlier = [&](const Candidate& a, const Candidate& b) {
        const int dayA = day(*a.event);
        const int dayB = day(*b.event);
        if (dayA != dayB) {
            return dayA < dayB;
        }
        if (a.event->minute != b.event->minute) {
            return a.event->minute < b.event->minute;
        }
        return a.event->title < b.event->title;
    };
    std::vector<AgendaItem> upcoming;
    AcquireSRWLockShared(&g_agendasLock);
    std::vector<Candidate> candidates;
    for (const auto& agenda : g_agendas) {
        for (const auto& event : agenda.events) {
            const int on = day(event);
            if (on > today ||
                (on == today && (event.minute < 0 || event.minute >= minute))) {
                candidates.push_back({&event, agenda.color});
            }
        }
    }
    const size_t kept = std::min(candidates.size(), count * 3);
    std::partial_sort(candidates.begin(), candidates.begin() + kept,
                      candidates.end(), earlier);
    for (size_t i = 0; i < kept && upcoming.size() < count; i++) {
        if (i && !earlier(candidates[i - 1], candidates[i])) {
            continue;
        }
        upcoming.push_back({*candidates[i].event, candidates[i].color});
    }
    ReleaseSRWLockShared(&g_agendasLock);
    return upcoming;
}

// A day as a number, to count days with (days since 1601; 0 for none).
int DayNumber(int year, int month, int day) {
    SYSTEMTIME time{(WORD)year, (WORD)month, 0, (WORD)day};
    FILETIME file;
    if (year < 1601 || year > 30000 || !SystemTimeToFileTime(&time, &file)) {
        return 0;
    }
    return (int)((((ULONGLONG)file.dwHighDateTime << 32) | file.dwLowDateTime) /
                 (24ULL * 60 * 60 * 10000000));
}

void DayFromNumber(int number, int* year, int* month, int* day) {
    const ULONGLONG ticks = (ULONGLONG)number * 24 * 60 * 60 * 10000000;
    const FILETIME file{(DWORD)ticks, (DWORD)(ticks >> 32)};
    SYSTEMTIME time{};
    FileTimeToSystemTime(&file, &time);
    *year = time.wYear;
    *month = time.wMonth;
    *day = time.wDay;
}

// 0 is Sunday (1601 started on a Monday).
int NumberWeekday(int number) {
    return (number + 1) % 7;
}

// A day number and a minute in UTC as local ones.
void UtcToLocal(int* number, int* minute) {
    int year;
    int month;
    int day;
    DayFromNumber(*number, &year, &month, &day);
    SYSTEMTIME utc{(WORD)year, (WORD)month,          0,
                   (WORD)day,  (WORD)(*minute / 60), (WORD)(*minute % 60)};
    SYSTEMTIME local;
    if (SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) {
        *number = DayNumber(local.wYear, local.wMonth, local.wDay);
        *minute = local.wHour * 60 + local.wMinute;
    }
}

// An iCalendar date ("20261012") or time ("20261012T143000", in UTC with a
// "Z" at its end), as a day number and minute (-1 for a date): a time in
// UTC turned into a local one, or kept in UTC when `utc` is given (and set:
// a rule's days are counted in UTC then).
bool ParseIcsTime(const std::wstring& value,
                  int* number,
                  int* minute,
                  bool* utc = nullptr) {
    int year = 0;
    int month = 0;
    int day = 0;
    if (value.size() < 8 ||
        swscanf_s(value.c_str(), L"%4d%2d%2d", &year, &month, &day) != 3) {
        return false;
    }
    *minute = -1;
    if (value.size() >= 13 && value[8] == L'T') {
        int hour = 0;
        int minutes = 0;
        if (swscanf_s(value.c_str() + 9, L"%2d%2d", &hour, &minutes) != 2 ||
            hour < 0 || hour > 23 || minutes < 0 || minutes > 59) {
            return false;
        }
        *minute = hour * 60 + minutes;
    }
    *number = DayNumber(year, month, day);
    if (!*number) {
        return false;
    }
    if (*minute >= 0 && value.size() >= 16 && towupper(value[15]) == L'Z') {
        if (utc) {
            *utc = true;
        } else {
            UtcToLocal(number, minute);
        }
    }
    return true;
}

// A text with its escapes ("\," "\;" "\n"), on one line.
std::wstring UnescapeIcsText(const std::wstring& text) {
    std::wstring result;
    for (size_t i = 0; i < text.size() && result.size() < 200; i++) {
        if (text[i] == L'\\' && i + 1 < text.size()) {
            const WCHAR next = text[++i];
            result += next == L'n' || next == L'N' ? L' ' : next;
        } else if (text[i] >= 0x20) {
            result += text[i];
        }
    }
    return result;
}

// A repeating event's rule (RRULE): what the island understands of it.
struct IcsRule {
    // 1 daily, 2 weekly, 3 monthly, 4 yearly.
    int frequency = 0;
    int interval = 1;
    int count = 0;
    // The last day (a day number), or 0.
    int until = 0;
    // BYDAY: which one in the month (0 for each one, from the end when
    // negative) and the weekday (0 for Sunday).
    std::vector<std::pair<int, int>> days;
    std::vector<int> monthDays;
    std::vector<int> months;
    int position = 0;
};

// (`utc`: the event's days are UTC ones, see ParseIcsTime. Its lists are
// bounded and without repeats: a file from the web can say anything.)
bool ParseIcsRule(const std::wstring& text, IcsRule* rule, bool utc = false) {
    for (size_t at = 0; at < text.size();) {
        size_t end = text.find(L';', at);
        if (end == std::wstring::npos) {
            end = text.size();
        }
        const std::wstring part = text.substr(at, end - at);
        at = end + 1;
        const size_t equals = part.find(L'=');
        if (equals == std::wstring::npos) {
            continue;
        }
        std::wstring key = part.substr(0, equals);
        for (auto& c : key) {
            c = towupper(c);
        }
        const std::wstring value = part.substr(equals + 1);
        std::vector<std::wstring> items;
        constexpr size_t kMaxRuleItems = 400;
        for (size_t from = 0;
             from <= value.size() && items.size() < kMaxRuleItems;) {
            size_t comma = value.find(L',', from);
            if (comma == std::wstring::npos) {
                comma = value.size();
            }
            items.push_back(value.substr(from, comma - from));
            from = comma + 1;
        }
        if (key == L"FREQ") {
            static constexpr PCWSTR kFrequencies[] = {L"DAILY", L"WEEKLY",
                                                      L"MONTHLY", L"YEARLY"};
            for (int i = 0; i < 4; i++) {
                if (_wcsicmp(value.c_str(), kFrequencies[i]) == 0) {
                    rule->frequency = i + 1;
                }
            }
        } else if (key == L"INTERVAL") {
            rule->interval = std::clamp(_wtoi(value.c_str()), 1, 1000);
        } else if (key == L"COUNT") {
            rule->count = std::clamp(_wtoi(value.c_str()), 0, 100000);
        } else if (key == L"UNTIL") {
            int minute;
            bool inUtc = false;
            if (!ParseIcsTime(value, &rule->until, &minute,
                              utc ? &inUtc : nullptr)) {
                rule->until = 0;
            }
        } else if (key == L"BYDAY") {
            static constexpr PCWSTR kWeekdays[] = {L"SU", L"MO", L"TU", L"WE",
                                                   L"TH", L"FR", L"SA"};
            for (const auto& item : items) {
                if (item.size() < 2) {
                    continue;
                }
                const std::wstring name = item.substr(item.size() - 2);
                // (Which one in a month or a year: at most the 53rd.)
                const int nth =
                    item.size() > 2
                        ? _wtoi(item.substr(0, item.size() - 2).c_str())
                        : 0;
                if (nth < -53 || nth > 53) {
                    continue;
                }
                for (int weekday = 0; weekday < 7; weekday++) {
                    const std::pair<int, int> entry{nth, weekday};
                    if (_wcsicmp(name.c_str(), kWeekdays[weekday]) == 0 &&
                        rule->days.size() < 35 &&
                        std::find(rule->days.begin(), rule->days.end(),
                                  entry) == rule->days.end()) {
                        rule->days.push_back(entry);
                    }
                }
            }
        } else if (key == L"BYMONTHDAY") {
            for (const auto& item : items) {
                const int day = _wtoi(item.c_str());
                if (day && day >= -31 && day <= 31 &&
                    std::find(rule->monthDays.begin(), rule->monthDays.end(),
                              day) == rule->monthDays.end()) {
                    rule->monthDays.push_back(day);
                }
            }
        } else if (key == L"BYMONTH") {
            for (const auto& item : items) {
                const int month = _wtoi(item.c_str());
                if (month >= 1 && month <= 12 &&
                    std::find(rule->months.begin(), rule->months.end(),
                              month) == rule->months.end()) {
                    rule->months.push_back(month);
                }
            }
        } else if (key == L"BYSETPOS") {
            rule->position = std::clamp(_wtoi(value.c_str()), -366, 366);
        }
    }
    return rule->frequency != 0;
}

// The days a monthly or yearly rule picks in a month (day numbers, sorted):
// its days of the month, its weekdays (each one, or the nth one), or the
// event's own day.
std::vector<int> IcsMonthDays(const IcsRule& rule,
                              int year,
                              int month,
                              int ownDay) {
    std::vector<int> days;
    const int first = DayNumber(year, month, 1);
    if (!first) {
        return days;
    }
    const int count = DaysInMonth(year, month);
    if (!rule.monthDays.empty()) {
        for (const int day : rule.monthDays) {
            const int one = day > 0 ? day : count + day + 1;
            if (one >= 1 && one <= count) {
                days.push_back(first + one - 1);
            }
        }
    } else if (!rule.days.empty()) {
        for (const auto& [nth, weekday] : rule.days) {
            std::vector<int> all;
            for (int day = first + (weekday - NumberWeekday(first) + 7) % 7;
                 day < first + count; day += 7) {
                all.push_back(day);
            }
            if (nth == 0) {
                days.insert(days.end(), all.begin(), all.end());
            } else if (nth > 0 && nth <= (int)all.size()) {
                days.push_back(all[nth - 1]);
            } else if (nth < 0 && -nth <= (int)all.size()) {
                days.push_back(all[(int)all.size() + nth]);
            }
        }
    } else if (ownDay <= count) {
        days.push_back(first + ownDay - 1);
    }
    std::sort(days.begin(), days.end());
    days.erase(std::unique(days.begin(), days.end()), days.end());
    if (rule.position && !days.empty()) {
        const int index = rule.position > 0 ? rule.position - 1
                                            : (int)days.size() + rule.position;
        const int one =
            index >= 0 && index < (int)days.size() ? days[index] : 0;
        days.clear();
        if (one) {
            days.push_back(one);
        }
    }
    return days;
}

// An event as read from the file.
struct IcsEvent {
    std::wstring uid;
    std::wstring title;
    std::wstring rule;
    // Its first day (a day number), its minute (-1 for the whole day), and
    // how many days it lasts.
    int start = 0;
    int minute = -1;
    int span = 1;
    int end = 0;
    int endMinute = -1;
    int durationDays = 0;
    // Days taken out of its rule (EXDATE, or moved).
    std::vector<int> excluded;
    // An occurrence of another event, moved (RECURRENCE-ID): the day it
    // was on.
    int moved = 0;
    bool cancelled = false;
    // Its start is in UTC: its days are UTC ones (its rule's days fall on
    // them), turned into local ones only as they're shown.
    bool utc = false;
    // Its times as written, read once the whole event is (its start says
    // whether the others stay in UTC).
    std::wstring startText;
    std::wstring endText;
    std::wstring movedText;
    std::vector<std::wstring> excludedTexts;
};

// The days an event starts on (day numbers), up to `last` (from `first`,
// counting the days it lasts): its own, or its rule's, without the ones
// taken out.
std::vector<int> IcsEventDays(const IcsEvent& event, int first, int last) {
    std::vector<int> days;
    const int from = first - event.span + 1;
    IcsRule rule;
    if (event.moved || event.rule.empty() ||
        !ParseIcsRule(event.rule, &rule, event.utc)) {
        if (event.start >= from && event.start <= last) {
            days.push_back(event.start);
        }
        return days;
    }
    int taken = 0;
    // False once past its end (its last day, its count, or `last`).
    auto take = [&](int day) {
        if (day < event.start) {
            return true;
        }
        if ((rule.until && day > rule.until) || day > last) {
            return false;
        }
        taken++;
        if (day >= from &&
            std::find(event.excluded.begin(), event.excluded.end(), day) ==
                event.excluded.end()) {
            days.push_back(day);
        }
        return !rule.count || taken < rule.count;
    };
    auto past = [&](int day) {
        return day > last || (rule.until && day > rule.until);
    };
    auto inMonths = [&](int day) {
        int year;
        int month;
        int date;
        DayFromNumber(day, &year, &month, &date);
        return rule.months.empty() ||
               std::find(rule.months.begin(), rule.months.end(), month) !=
                   rule.months.end();
    };
    int year;
    int month;
    int day;
    DayFromNumber(event.start, &year, &month, &day);
    int fromYear;
    int fromMonth;
    int fromDay;
    DayFromNumber(std::max(from, event.start), &fromYear, &fromMonth, &fromDay);
    const int step = rule.interval;
    // Without a count, the periods long before `first` are skipped.
    const bool skip = !rule.count;
    constexpr int kMaxPeriods = 5000;
    if (rule.frequency == 1) {
        int k = skip ? std::max(0, (from - event.start) / step - 1) : 0;
        for (int n = 0; n < kMaxPeriods; n++, k++) {
            const int one = event.start + k * step;
            if (past(one)) {
                break;
            }
            const bool weekday =
                rule.days.empty() ||
                std::any_of(rule.days.begin(), rule.days.end(),
                            [&](const std::pair<int, int>& entry) {
                                return entry.second == NumberWeekday(one);
                            });
            if (weekday && inMonths(one) && !take(one)) {
                break;
            }
        }
    } else if (rule.frequency == 2) {
        // Weeks from Monday.
        const int base = event.start - (NumberWeekday(event.start) + 6) % 7;
        std::vector<int> offsets;
        for (const auto& entry : rule.days) {
            offsets.push_back((entry.second + 6) % 7);
        }
        if (offsets.empty()) {
            offsets.push_back((NumberWeekday(event.start) + 6) % 7);
        }
        std::sort(offsets.begin(), offsets.end());
        offsets.erase(std::unique(offsets.begin(), offsets.end()),
                      offsets.end());
        int k = skip ? std::max(0, (from - base) / (7 * step) - 1) : 0;
        bool going = true;
        for (int n = 0; n < kMaxPeriods && going; n++, k++) {
            const int week = base + k * 7 * step;
            if (past(week)) {
                break;
            }
            for (const int offset : offsets) {
                if (inMonths(week + offset) && !take(week + offset)) {
                    going = false;
                    break;
                }
            }
        }
    } else if (rule.frequency == 3) {
        const int startIndex = year * 12 + month - 1;
        int k =
            skip ? std::max(
                       0,
                       (fromYear * 12 + fromMonth - 1 - startIndex) / step - 1)
                 : 0;
        bool going = true;
        for (int n = 0; n < kMaxPeriods && going; n++, k++) {
            const int index = startIndex + k * step;
            const int y = index / 12;
            const int m = index % 12 + 1;
            const int firstDay = DayNumber(y, m, 1);
            if (!firstDay || past(firstDay)) {
                break;
            }
            if (!rule.months.empty() &&
                std::find(rule.months.begin(), rule.months.end(), m) ==
                    rule.months.end()) {
                continue;
            }
            for (const int one : IcsMonthDays(rule, y, m, day)) {
                if (!take(one)) {
                    going = false;
                    break;
                }
            }
        }
    } else if (rule.frequency == 4) {
        std::vector<int> months = rule.months;
        if (months.empty()) {
            months.push_back(month);
        }
        std::sort(months.begin(), months.end());
        int k = skip ? std::max(0, (fromYear - year) / step - 1) : 0;
        bool going = true;
        for (int n = 0; n < kMaxPeriods && going; n++, k++) {
            const int y = year + k * step;
            const int firstDay = DayNumber(y, 1, 1);
            if (!firstDay || past(firstDay)) {
                break;
            }
            for (const int m : months) {
                for (const int one : IcsMonthDays(rule, y, m, day)) {
                    if (!take(one)) {
                        going = false;
                        break;
                    }
                }
                if (!going) {
                    break;
                }
            }
        }
    }
    return days;
}

// The events of an .ics file's text between `first` and `last` (day
// numbers; up to kMaxAgendaEvents, the nearest to today), and the
// calendar's name. Nothing once g_agendaStop is set (the calendar going
// away). Its lines, lists and occurrences are bounded: a file from the web
// can say anything.
std::vector<AgendaEvent> ParseIcs(const std::wstring& text,
                                  int first,
                                  int last,
                                  std::wstring* name) {
    // A long line goes on in the next ones, after a space or a tab (up to a
    // length no calendar needs).
    constexpr size_t kMaxIcsLine = 16 * 1024;
    std::vector<std::wstring> lines;
    for (size_t at = 0; at < text.size();) {
        size_t end = text.find(L'\n', at);
        if (end == std::wstring::npos) {
            end = text.size();
        }
        std::wstring line = text.substr(at, std::min(end - at, kMaxIcsLine));
        at = end + 1;
        if (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }
        if (!line.empty() && (line[0] == L' ' || line[0] == L'\t') &&
            !lines.empty()) {
            if (lines.back().size() < kMaxIcsLine) {
                lines.back() +=
                    line.substr(1, kMaxIcsLine - lines.back().size());
            }
        } else {
            lines.push_back(std::move(line));
        }
    }
    constexpr size_t kMaxIcsExcluded = 366;
    std::vector<IcsEvent> events;
    IcsEvent event;
    bool inEvent = false;
    // Inside an event's own parts (its alarms).
    int nested = 0;
    size_t read = 0;
    for (const auto& line : lines) {
        if (++read % 4096 == 0 && g_agendaStop) {
            return {};
        }
        // "NAME;PARAMETERS:VALUE" (a colon in quotes is a parameter's).
        size_t colon = std::wstring::npos;
        bool quoted = false;
        for (size_t i = 0; i < line.size(); i++) {
            if (line[i] == L'"') {
                quoted = !quoted;
            } else if (line[i] == L':' && !quoted) {
                colon = i;
                break;
            }
        }
        if (colon == std::wstring::npos) {
            continue;
        }
        std::wstring key = line.substr(0, std::min(colon, line.find(L';')));
        for (auto& c : key) {
            c = towupper(c);
        }
        const std::wstring value = line.substr(colon + 1);
        if (key == L"BEGIN") {
            if (inEvent) {
                nested++;
            } else if (_wcsicmp(value.c_str(), L"VEVENT") == 0) {
                inEvent = true;
                event = {};
            }
            continue;
        }
        if (key == L"END") {
            if (nested) {
                nested--;
            } else if (inEvent && _wcsicmp(value.c_str(), L"VEVENT") == 0) {
                inEvent = false;
                // Its times: in UTC when its start is, otherwise local.
                bool utc = false;
                if (!ParseIcsTime(event.startText, &event.start, &event.minute,
                                  &utc)) {
                    event.start = 0;
                }
                event.utc = utc;
                bool other = false;
                bool* keep = utc ? &other : nullptr;
                if (!ParseIcsTime(event.endText, &event.end, &event.endMinute,
                                  keep)) {
                    event.end = 0;
                }
                int minute;
                if (!ParseIcsTime(event.movedText, &event.moved, &minute,
                                  keep)) {
                    event.moved = 0;
                }
                for (const auto& one : event.excludedTexts) {
                    int day;
                    if (ParseIcsTime(one, &day, &minute, keep)) {
                        event.excluded.push_back(day);
                    }
                }
                event.excludedTexts.clear();
                if (event.minute < 0) {
                    event.span = event.end > event.start
                                     ? event.end - event.start
                                 : event.durationDays > 0 ? event.durationDays
                                                          : 1;
                } else if (event.end > event.start) {
                    // On its next days too only when it lasts a day or more.
                    const int days = event.end - event.start;
                    const bool lasting =
                        days >= 2 || event.endMinute >= event.minute;
                    event.span =
                        lasting ? days + (event.endMinute > 0 ? 1 : 0) : 1;
                }
                event.span = std::clamp(event.span, 1, 60);
                if (event.start) {
                    events.push_back(std::move(event));
                }
            }
            continue;
        }
        if (!inEvent) {
            if (key == L"X-WR-CALNAME" && name->empty()) {
                *name = UnescapeIcsText(value).substr(0, 60);
            }
            continue;
        }
        if (nested) {
            continue;
        }
        if (key == L"SUMMARY") {
            event.title = UnescapeIcsText(value);
        } else if (key == L"UID") {
            event.uid = value;
        } else if (key == L"DTSTART") {
            event.startText = value;
        } else if (key == L"DTEND") {
            event.endText = value;
        } else if (key == L"DURATION") {
            // "P2D" or "P1W" (whole days).
            const size_t p = value.find(L'P');
            if (p != std::wstring::npos) {
                wchar_t* after = nullptr;
                const long number = wcstol(value.c_str() + p + 1, &after, 10);
                if (after && (*after == L'D' || *after == L'W')) {
                    event.durationDays = (int)std::clamp(number, 0L, 60L) *
                                         (*after == L'W' ? 7 : 1);
                }
            }
        } else if (key == L"RRULE") {
            event.rule = value;
        } else if (key == L"EXDATE") {
            for (size_t from = 0;
                 from < value.size() &&
                 event.excludedTexts.size() < kMaxIcsExcluded;) {
                size_t comma = value.find(L',', from);
                if (comma == std::wstring::npos) {
                    comma = value.size();
                }
                event.excludedTexts.push_back(value.substr(from, comma - from));
                from = comma + 1;
            }
        } else if (key == L"RECURRENCE-ID") {
            event.movedText = value;
        } else if (key == L"STATUS") {
            event.cancelled = _wcsicmp(value.c_str(), L"CANCELLED") == 0;
        }
    }
    // A moved occurrence (or a cancelled one) takes its event's place on
    // the day it was on.
    std::unordered_map<std::wstring, std::vector<size_t>> byUid;
    for (size_t i = 0; i < events.size(); i++) {
        if (!events[i].moved && !events[i].uid.empty()) {
            byUid[events[i].uid].push_back(i);
        }
    }
    for (const auto& moved : events) {
        if (!moved.moved) {
            continue;
        }
        if (const auto it = byUid.find(moved.uid); it != byUid.end()) {
            for (const size_t i : it->second) {
                events[i].excluded.push_back(moved.moved);
            }
        }
    }
    // Every occurrence (up to many more than are kept), on its local day,
    // then the ones nearest to today kept.
    struct Occurrence {
        int day;
        int minute;
        size_t event;
    };
    constexpr size_t kMaxOccurrences = 10 * kMaxAgendaEvents;
    std::vector<Occurrence> occurrences;
    for (size_t e = 0;
         e < events.size() && occurrences.size() < kMaxOccurrences; e++) {
        if (g_agendaStop) {
            return {};
        }
        const IcsEvent& one = events[e];
        if (one.cancelled) {
            continue;
        }
        // (In UTC: a day more on each side, its local days may be those.)
        for (const int start : IcsEventDays(one, one.utc ? first - 1 : first,
                                            one.utc ? last + 1 : last)) {
            int day = start;
            int minute = one.minute;
            if (one.utc) {
                UtcToLocal(&day, &minute);
            }
            for (int i = 0;
                 i < one.span && occurrences.size() < kMaxOccurrences; i++) {
                if (day + i >= first && day + i <= last) {
                    occurrences.push_back({day + i, i == 0 ? minute : -1, e});
                }
            }
        }
    }
    if (occurrences.size() > kMaxAgendaEvents) {
        SYSTEMTIME now;
        GetLocalTime(&now);
        const int today = DayNumber(now.wYear, now.wMonth, now.wDay);
        std::nth_element(
            occurrences.begin(), occurrences.begin() + kMaxAgendaEvents,
            occurrences.end(), [&](const Occurrence& a, const Occurrence& b) {
                return std::abs(a.day - today) < std::abs(b.day - today);
            });
        occurrences.resize(kMaxAgendaEvents);
    }
    std::vector<AgendaEvent> found;
    found.reserve(occurrences.size());
    for (const auto& one : occurrences) {
        AgendaEvent item;
        DayFromNumber(one.day, &item.year, &item.month, &item.day);
        item.minute = one.minute;
        item.title = events[one.event].title;
        found.push_back(std::move(item));
    }
    return found;
}

struct AgendaRequest {
    HWND notify = nullptr;
    std::vector<std::wstring> sources;
};

// Reads the agendas asked for, one at a time: the calendar is told as each
// comes.
DWORD WINAPI AgendaThreadProc(LPVOID parameter) {
    std::unique_ptr<AgendaRequest> request((AgendaRequest*)parameter);
    SYSTEMTIME today;
    GetLocalTime(&today);
    const int first = DayNumber(today.wYear - 1, 1, 1);
    const int last = DayNumber(today.wYear + 1, 12, 31);
    for (const auto& source : request->sources) {
        if (g_agendaStop) {
            break;
        }
        std::wstring text;
        bool ok;
        // (Why it failed, for the log.)
        DWORD answer = 0;
        const ULONGLONG askedAt = GetTickCount64();
        if (source.starts_with(L"https://")) {
            std::vector<BYTE> data;
            ok = HttpsGet(source, &data, kMaxAgendaBytes, 10000, &g_agendaStop,
                          nullptr, &answer);
            const int length =
                ok ? MultiByteToWideChar(CP_UTF8, 0, (const char*)data.data(),
                                         (int)data.size(), nullptr, 0)
                   : 0;
            if (length > 0) {
                text.resize(length);
                MultiByteToWideChar(CP_UTF8, 0, (const char*)data.data(),
                                    (int)data.size(), text.data(), length);
            }
        } else {
            ok = ReadTextFile(source.c_str(), kMaxAgendaBytes, &text);
        }
        const bool fetched = ok;
        ok = ok && text.find(L"BEGIN:VCALENDAR") != std::wstring::npos;
        if (!ok && !g_agendaStop) {
            // Only its host: the address can be a secret one.
            std::wstring where = L"a file";
            if (source.starts_with(L"https://")) {
                where = source.substr(8, source.find(L'/', 8) - 8);
            }
            std::wstring start = text.substr(0, 40);
            std::replace_if(
                start.begin(), start.end(), [](WCHAR c) { return c < 0x20; },
                L' ');
            if (fetched) {
                IslandLog(L"Agenda from %s isn't a calendar: %u characters, "
                          L"it starts with \"%s\"",
                          where.c_str(), (unsigned)text.size(), start.c_str());
            } else if (source.starts_with(L"https://")) {
                // (No answer within the wait, an error, or a whole answer
                // cut off or over kMaxAgendaBytes.)
                IslandLog(L"Agenda from %s not read: %s %u after %.1f s",
                          where.c_str(),
                          !answer         ? L"no answer, status"
                          : answer == 200 ? L"cut off or too big, HTTP"
                                          : L"HTTP",
                          (unsigned)answer,
                          (GetTickCount64() - askedAt) / 1000.0);
            } else {
                IslandLog(L"Agenda from a file not read");
            }
        }
        std::wstring name;
        std::vector<AgendaEvent> events;
        if (ok) {
            events = ParseIcs(text, first, last, &name);
        }
        // (Stopped: nothing kept, nobody told.)
        if (g_agendaStop) {
            break;
        }
        AcquireSRWLockExclusive(&g_agendasLock);
        for (auto& agenda : g_agendas) {
            if (agenda.source != source) {
                continue;
            }
            agenda.reading = false;
            agenda.readAt = GetTickCount64();
            agenda.failed = !ok;
            agenda.notIcs = fetched && !ok;
            if (ok) {
                agenda.read = true;
                agenda.name = std::move(name);
                agenda.events = std::move(events);
            }
            break;
        }
        IndexAgendaDays();
        ReleaseSRWLockExclusive(&g_agendasLock);
        if (&source != &request->sources.back()) {
            PostMessage(request->notify, WM_APP_AGENDAS, 0, 0);
        }
    }
    g_agendaBusy = false;
    if (!g_agendaStop) {
        PostMessage(request->notify, WM_APP_AGENDAS, 1, 0);
    }
    return 0;
}

// Reads the agendas not read yet, or read 15 minutes ago (10, after
// failing), on a thread (one at a time); `notify` is told as each comes.
void RequestAgendas(HWND notify) {
    if (g_agendaBusy) {
        return;
    }
    if (g_agendaThread) {
        CloseHandle(g_agendaThread);
        g_agendaThread = nullptr;
    }
    const ULONGLONG now = GetTickCount64();
    auto request = std::make_unique<AgendaRequest>();
    request->notify = notify;
    AcquireSRWLockExclusive(&g_agendasLock);
    for (auto& agenda : g_agendas) {
        const ULONGLONG wait = (agenda.failed ? 10 : 15) * 60 * 1000;
        if (!agenda.readAt || now - agenda.readAt >= wait) {
            agenda.reading = true;
            request->sources.push_back(agenda.source);
        }
    }
    ReleaseSRWLockExclusive(&g_agendasLock);
    if (request->sources.empty()) {
        return;
    }
    g_agendaBusy = true;
    g_agendaStop = false;
    g_agendaThread =
        CreateThread(nullptr, 0, AgendaThreadProc, request.get(), 0, nullptr);
    if (g_agendaThread) {
        request.release();
        return;
    }
    g_agendaBusy = false;
    AcquireSRWLockExclusive(&g_agendasLock);
    for (auto& agenda : g_agendas) {
        agenda.reading = false;
    }
    ReleaseSRWLockExclusive(&g_agendasLock);
}

bool HasEvents(int year, int month, int day) {
    return std::any_of(g_events.begin(), g_events.end(),
                       [&](const CalendarEvent& event) {
                           return event.year == year && event.month == month &&
                                  event.day == day;
                       });
}

// A local time as minutes since 1601, to compare.
long long LocalMinutes(int year, int month, int day, int minute) {
    SYSTEMTIME time{(WORD)year, (WORD)month, 0, (WORD)day,
                    (WORD)(minute / 60), (WORD)(minute % 60)};
    FILETIME file;
    if (!SystemTimeToFileTime(&time, &file)) {
        return 0;
    }
    return (((long long)file.dwHighDateTime << 32) | file.dwLowDateTime) /
           (60LL * 10000000);
}

// The reminders due now (marked as done). Missed ones from long ago are
// marked done without reminding.
std::vector<CalendarEvent> TakeDueReminders() {
    LoadEvents();
    SYSTEMTIME now;
    GetLocalTime(&now);
    const long long current = LocalMinutes(now.wYear, now.wMonth, now.wDay,
                                           now.wHour * 60 + now.wMinute);
    std::vector<CalendarEvent> due;
    bool changed = false;
    for (auto& event : g_events) {
        if (!event.remind || event.fired) {
            continue;
        }
        const long long at = LocalMinutes(
            event.year, event.month, event.day,
            event.minute >= 0 ? event.minute : kAllDayReminderMinute);
        if (current < at) {
            continue;
        }
        event.fired = true;
        changed = true;
        // (Logged without its title, to see why one wasn't seen: #6156.)
        if (current - at < 12 * 60) {
            due.push_back(event);
            IslandLog(L"Reminder due %lld min ago", current - at);
        } else {
            IslandLog(L"Reminder missed by %lld min: not shown", current - at);
        }
    }
    if (changed) {
        SaveEvents();
    }
    return due;
}

// Past events hidden from their day's list a while after their day
// (calendarHidePastAfter: 0 never, 1 after a day, 2 a week, 3 thirty days),
// one choice for every island (the events are the same for all), made in the
// main island's calendar. Still marked on their day, and shown when asked
// for (see CalendarPanel::DayEvents).
int g_hidePastAfter = 0;

// The days a calendarHidePastAfter choice waits (0: never hidden).
int HidePastDays(int choice) {
    return choice == 1 ? 1 : choice == 2 ? 7 : choice == 3 ? 30 : 0;
}

// Whether an event's day is at least `days` days before `today` (in
// LocalMinutes, at midnight).
bool EventIsPast(const CalendarEvent& event, long long today, int days) {
    const long long day = LocalMinutes(event.year, event.month, event.day, 0);
    return days > 0 && day && today - day >= days * 24LL * 60;
}

// Today in LocalMinutes, at midnight.
long long TodayMinutes() {
    SYSTEMTIME now;
    GetLocalTime(&now);
    return LocalMinutes(now.wYear, now.wMonth, now.wDay, 0);
}

// The events before today (see DeletePastEvents).
size_t PastEventCount() {
    const long long today = TodayMinutes();
    return std::count_if(g_events.begin(), g_events.end(),
                         [&](const CalendarEvent& event) {
                             return EventIsPast(event, today, 1);
                         });
}

// Changed when the events or their hiding change elsewhere (see
// SetHidePastAfter, DeletePastEvents): an open calendar lays itself out
// again.
int g_eventsDropped = 0;

void SetHidePastAfter(int choice) {
    g_hidePastAfter = std::clamp(choice, 0, 3);
    Wh_SetIntValue(L"calendarHidePastAfter", g_hidePastAfter);
    g_eventsDropped++;
}

// Every event before today deleted (the calendar's "Delete past events").
void DeletePastEvents() {
    const long long today = TodayMinutes();
    if (std::erase_if(g_events, [&](const CalendarEvent& event) {
            return EventIsPast(event, today, 1);
        })) {
        SaveEvents();
        g_eventsDropped++;
    }
}

// Whether the island shows the main island's settings (see
// Island::HasMainSettings); defined further down.
bool IslandHasMainSettings(const Island* island);

// A time as the clock's format says (see clockFormat), everywhere the island
// shows one. With 12 hours, AM or PM (the regional format's words, or these)
// goes in `marker` when it's drawn apart (smaller, on the pill), otherwise
// after the time.
std::wstring FormatClockTime(const SYSTEMTIME& time,
                             bool seconds,
                             std::wstring* marker = nullptr) {
    WCHAR text[64] = L"";
    const int format = g_settings.clockFormat;
    if (format == 0) {
        GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, seconds ? 0 : TIME_NOSECONDS,
                        &time, nullptr, text, ARRAYSIZE(text));
        return text;
    }
    PCWSTR picture = format == 1 ? (seconds ? L"HH:mm:ss" : L"HH:mm")
                                 : (seconds ? L"h:mm:ss" : L"h:mm");
    GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &time, picture, text,
                    ARRAYSIZE(text));
    std::wstring result = text;
    if (format == 2) {
        WCHAR designator[16] = L"";
        GetTimeFormatEx(DateLocale(), 0, &time, L"tt", designator,
                        ARRAYSIZE(designator));
        const std::wstring words =
            designator[0] ? designator : time.wHour < 12 ? L"AM" : L"PM";
        if (marker) {
            *marker = words;
        } else {
            result += L" " + words;
        }
    }
    return result;
}

// With 12 hours, the clock's AM or PM (its last word) smaller, like the
// iPhone's.
void SmallClockMarker(IDWriteTextLayout* layout, const std::wstring& text) {
    const size_t space = text.rfind(L' ');
    if (g_settings.clockFormat != 2 || space == std::wstring::npos ||
        space + 1 >= text.size()) {
        return;
    }
    const DWRITE_TEXT_RANGE range{(UINT32)space + 1,
                                  (UINT32)(text.size() - space - 1)};
    FLOAT size = 0;
    layout->GetFontSize(0, &size);
    layout->SetFontSize(size * 0.68f, range);
}

// "14:30", or "All day".
std::wstring EventTimeText(int minute) {
    if (minute < 0) {
        return Tr(L"All day", L"Dia todo");
    }
    SYSTEMTIME time{2000, 1, 0, 1, (WORD)(minute / 60), (WORD)(minute % 60)};
    return FormatClockTime(time, false);
}

// What's on the clipboard added to a text being typed (on one line, up to
// `limit` characters).
void PasteText(HWND hwnd, std::wstring* text, size_t limit) {
    if (!OpenClipboard(hwnd)) {
        return;
    }
    if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
        if (auto pasted = (PCWSTR)GlobalLock(data)) {
            for (PCWSTR p = pasted; *p && text->size() < limit; p++) {
                if (*p >= 0x20 && *p != L'\t') {
                    *text += *p;
                }
            }
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
}

