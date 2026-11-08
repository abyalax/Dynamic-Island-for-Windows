class CalendarPanel : public Panel {
   protected:
    void Prepare(float scale) override;
    void OnOpen() override;
    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;
    bool OnMouseMove(POINT pt) override;
    bool OnMouseLeave() override;
    bool OnMouseUp(POINT pt) override;
    bool OnWheel(int delta) override;
    bool OnKey(WPARAM key) override;
    bool OnChar(WCHAR c) override;

   public:
    // Called every second while open, for the clock.
    void Refresh() {
        // Events gone elsewhere, or hidden otherwise (see g_eventsDropped):
        // the one being changed no longer, the rows laid out again.
        if (m_eventsDropped != g_eventsDropped) {
            m_eventsDropped = g_eventsDropped;
            if (m_adding && m_editingId &&
                std::none_of(g_events.begin(), g_events.end(),
                             [&](const CalendarEvent& event) {
                                 return event.id == m_editingId;
                             })) {
                StopEditing();
            }
            if (IsOpen()) {
                Layout();
            }
        }
        if (IsOpen()) {
            Render();
        }
    }
    // Opens on a day (a reminder's).
    void SelectDay(int year, int month, int day) {
        m_openOn = {year, month, day};
    }

   protected:
    bool AnimateContents(double dt) override;

   private:
    enum class Part {
        None,
        Previous,
        Next,
        Title,
        Day,          // A day of the month (value).
        DeleteEvent,  // An event's "x" (id).
        EventRow,     // An event, to edit it (id).
        Add,
        TitleField,
        // The value: 0 in the box (to type), +1/-1 on its arrows.
        Hour,
        Minute,
        AllDay,
        Remind,
        Save,
        Cancel,
        // The agendas' view: its button, an agenda's color and its "x" (the
        // agenda's index), the link's field and "Add", and today's colors
        // (0 to 2).
        Agendas,
        AgendaColor,
        AgendaRemove,
        LinkField,
        LinkAdd,
        TodayChoice,
        // When past events are hidden (the choice's index).
        DeleteChoice,
        DeletePast,
        // "Show N hidden" under the day's events.
        ShowHidden,
        // An upcoming event's or holiday's row (its day, yyyymmdd).
        Upcoming,
        // Today big at the left (islandCalendarBigDay): its alarms' and
        // timer's buttons, and its gear (the agendas' view).
        BigAlarms,
        BigTimer,
        BigGear,
    };
    struct Hit {
        Part part = Part::None;
        long long value = 0;
        bool operator==(const Hit& other) const {
            return part == other.part && value == other.value;
        }
    };
    struct DayOpen {
        int year = 0;
        int month = 0;
        int day = 0;
    };

    void ChangeMonth(int delta);
    void Layout();
    Hit HitTest(POINT pt) const;
    // Today big (islandCalendarBigDay, #6165, see LayoutBigDay): its left
    // column's width (0 without it), its buttons, the line before them, its
    // number's format.
    float m_side = 0;
    D2D1_RECT_F m_bigAlarms{}, m_bigTimer{}, m_bigGear{};
    float m_bigDivider = 0;
    // The chosen day's events on it (the agendas' and the user's own, by
    // their time; see LayoutBigDay), up to kBigDayEvents, how many more,
    // and their title's row.
    struct BigDayEvent {
        int minute;
        // The agenda's color, or -1 for the user's own (red, as in the
        // month).
        int color;
        std::wstring title;
    };
    std::vector<BigDayEvent> m_bigEvents;
    size_t m_bigEventsMore = 0;
    D2D1_RECT_F m_bigEventsHeader{};
    // (The upcoming events' and holidays' rows, as both views have them.)
    Hit UpcomingHit(POINT pt) const;
    void LayoutBigDay();
    winrt::com_ptr<IDWriteTextFormat> m_bigDayFormat;
    void DrawCalendar(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawBigDay(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    // The holidays of the year shown, downloaded if chosen and not known
    // yet (once a session; on a thread, see HolidayThreadProc).
    void RequestHolidays();
    bool OnAppMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;
    void OnDestroy() override;
    HANDLE m_holidayThread = nullptr;
    std::vector<int> m_holidaysAsked;
    // A download failed (offline): not asked again before then.
    ULONGLONG m_holidaysRetryAt = 0;
    // The chosen day's holidays' rows, over its events.
    std::vector<D2D1_RECT_F> m_holidayRows;
    // The next holidays, under it all.
    std::vector<Holiday> m_upcoming;
    D2D1_RECT_F m_upcomingHeader{};
    // A month's title and days, moved sideways by `offset` pixels.
    void DrawMonth(ID2D1RenderTarget* target,
                   ID2D1SolidColorBrush* brush,
                   int year,
                   int month,
                   float offset,
                   float opacity);
    void DrawUpcoming(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawField(ID2D1RenderTarget* target,
                   ID2D1SolidColorBrush* brush,
                   const D2D1_RECT_F& rect,
                   const std::wstring& text,
                   PCWSTR placeholder,
                   bool focused);
    std::vector<const CalendarEvent*> DayEvents(size_t* hidden = nullptr) const;
    void SaveNewEvent();
    void StartEditing(const CalendarEvent* event);
    void StopEditing();
    void TypeDigit(int digit);

    winrt::com_ptr<IDWriteTextFormat> m_titleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_dayFormat;
    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_clockFormat;
    winrt::com_ptr<IDWriteTextFormat> m_dateFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallFormat;
    winrt::com_ptr<IDWriteTypography> m_tabular;
    // Changing months: the one before slides out, the new one in (m_slide
    // from 1 to 0), in the direction of m_slideDirection.
    int m_fromYear = 0;
    int m_fromMonth = 0;
    double m_slide = 0;
    double m_slideVelocity = 0;
    int m_slideDirection = 0;
    float m_preparedScale = 0;
    D2D1_RECT_F m_previous{}, m_next{}, m_title{};
    Hit m_hover;
    int m_year = 0;
    int m_month = 0;
    // The day chosen (its events below the month).
    int m_selectedYear = 0;
    int m_selectedMonth = 0;
    int m_selectedDay = 0;
    DayOpen m_openOn;
    // Where things are.
    std::vector<std::pair<int, D2D1_RECT_F>> m_dayRects;
    std::vector<std::pair<long long, D2D1_RECT_F>> m_eventRows;
    D2D1_RECT_F m_eventsHeader{}, m_addButton{}, m_titleField{}, m_hourBox{},
        m_minuteBox{}, m_allDayButton{}, m_remindButton{}, m_saveButton{},
        m_cancelButton{}, m_status{};
    // A new event being written (or one being changed, m_editingId): its
    // title, its time (or the whole day), and the reminder.
    bool m_adding = false;
    long long m_editingId = 0;
    // g_eventsDropped when last seen (see Refresh).
    int m_eventsDropped = 0;
    // The chosen day's events hidden as past (see g_hidePastAfter): how
    // many, their "Show N hidden" row, and the day they're shown for once
    // that's clicked (year * 10000 + month * 100 + day).
    size_t m_hiddenCount = 0;
    D2D1_RECT_F m_hiddenRow{};
    int m_showHiddenFor = 0;
    int SelectedDayKey() const {
        return m_selectedYear * 10000 + m_selectedMonth * 100 + m_selectedDay;
    }
    // Where typing goes; in the hour and the minute, how many figures were
    // typed (two move on).
    enum class Field { Title, Hour, Minute } m_focus = Field::Title;
    int m_digits = 0;
    // The fields fading and sliding in (0 to 1).
    double m_form = 0;
    double m_formVelocity = 0;
    std::wstring m_newTitle;
    int m_newHour = 9;
    int m_newMinute = 0;
    bool m_newAllDay = false;
    bool m_newRemind = true;
    void StepTime(Part part, int steps);
    std::wstring m_statusText;

    // The agendas' view (instead of the month): the agendas, a link or a
    // path to add, and today's color.
    bool m_agendasView = false;
    D2D1_RECT_F m_agendasButton{}, m_agendaHint{}, m_linkField{}, m_linkAdd{},
        m_agendaStatus{}, m_todayHeader{};
    std::vector<D2D1_RECT_F> m_agendaRows;
    // The ones added in the calendar (the settings' ones are removed there).
    std::vector<bool> m_agendaRemovable;
    std::array<D2D1_RECT_F, 3> m_todayChips{};
    // Past events hidden after (see g_hidePastAfter): its header and its
    // choices, and the events' count with "Delete past events" (pressed
    // twice) next to it. The main island's calendar only.
    D2D1_RECT_F m_deleteHeader{};
    std::array<D2D1_RECT_F, 4> m_deleteChips{};
    D2D1_RECT_F m_countLine{};
    D2D1_RECT_F m_deletePastButton{};
    double m_deletePastArmedAt = -100;
    std::wstring m_newLink;
    std::wstring m_agendaStatusText;
    // The chosen day's agenda events (up to kEventsShown, and how many
    // more), and the next ones from now.
    std::vector<AgendaItem> m_dayAgenda;
    std::vector<D2D1_RECT_F> m_agendaEventRows;
    size_t m_agendaMore = 0;
    D2D1_RECT_F m_agendaMoreRow{};
    std::vector<AgendaItem> m_upcomingAgenda;
    D2D1_RECT_F m_upcomingAgendaHeader{};
    // Today's circle (see kTodayRed), and Windows' accent color (read as the
    // calendar opens).
    int m_todayMode = 0;
    int m_todayColor = kTodayRed;
    int m_windowsAccent = 0x0078D4;
    D2D1_COLOR_F TodayColor() const;
    void SetTodayMode(int mode);
    void LayoutAgendas(float y);
    void DrawAgendas(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void AddAgenda();
    void RemoveAgenda(size_t index);
    // Windows' color dialog (see ColorThreadProc), for an agenda (its
    // source) or for today (empty).
    HANDLE m_colorThread = nullptr;
    std::wstring m_colorSource;
    void StartColorDialog(const std::wstring& source, int color);
    bool OnColorDone(LPARAM lParam);
    void EndColorDialog();
};

// The clock on top of the calendar; the events' rows under it.
constexpr float kCalendarClockHeight = 74;
// Today big at the calendar's left (islandCalendarBigDay), and its buttons
// opening the timer's panel on a tab (see Island::OpenTimerTab).
constexpr float kCalendarSideWidth = 160;
// Its right column (the buttons and the upcoming holidays).
constexpr float kBigDayRight = 190;
void OpenTimerTabOf(Island* island, int tab);
constexpr float kEventRowHeight = 30;
constexpr int kEventsShown = 4;
constexpr WCHAR kGlyphEventDelete = 0xE711;
constexpr WCHAR kGlyphChevronDownCalendar = 0xE70D;
constexpr WCHAR kGlyphCalendarBack = 0xE72B;

void CalendarPanel::Prepare(float scale) {
    if (scale != m_preparedScale) {
        m_preparedScale = scale;
        m_clockFormat = MakeFormat(L"Segoe UI Variable Display",
                                   DWRITE_FONT_WEIGHT_SEMI_BOLD, 34 * scale,
                                   DWRITE_TEXT_ALIGNMENT_LEADING);
        m_dateFormat = MakeFormat(L"Segoe UI Variable Text",
                                  DWRITE_FONT_WEIGHT_NORMAL, 13 * scale,
                                  DWRITE_TEXT_ALIGNMENT_LEADING);
        if (!m_tabular &&
            SUCCEEDED(m_dwrite->CreateTypography(m_tabular.put()))) {
            m_tabular->AddFontFeature(
                {DWRITE_FONT_FEATURE_TAG_TABULAR_FIGURES, 1});
        }
        m_titleFormat = MakeFormat(L"Segoe UI Variable Display",
                                   DWRITE_FONT_WEIGHT_BOLD, 15 * scale,
                                   DWRITE_TEXT_ALIGNMENT_LEADING);
        m_dayFormat = MakeFormat(L"Segoe UI Variable Text",
                                 DWRITE_FONT_WEIGHT_SEMI_BOLD, 13 * scale);
        m_iconFormat = MakeFormat(IconFont(),
                                  DWRITE_FONT_WEIGHT_NORMAL, 12 * scale);
        m_textFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       12.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_smallFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       11.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_bigDayFormat = MakeFormat(L"Segoe UI Variable Display",
                                    DWRITE_FONT_WEIGHT_SEMI_BOLD, 64 * scale,
                                    DWRITE_TEXT_ALIGNMENT_CENTER);
    }
    LoadEvents();
    Layout();
}

void CalendarPanel::OnOpen() {
    SYSTEMTIME today;
    GetLocalTime(&today);
    m_selectedYear = m_year = today.wYear;
    m_selectedMonth = m_month = today.wMonth;
    m_selectedDay = today.wDay;
    // A reminder's day.
    if (m_openOn.year) {
        m_selectedYear = m_year = m_openOn.year;
        m_selectedMonth = m_month = m_openOn.month;
        m_selectedDay = m_openOn.day;
        m_openOn = {};
    }
    RequestHolidays();
    m_hover = {};
    m_slide = 0;
    m_slideVelocity = 0;
    m_adding = false;
    m_editingId = 0;
    m_form = 0;
    m_formVelocity = 0;
    m_statusText.clear();
    // The agendas (read again when due) and today's color.
    m_agendasView = false;
    m_newLink.clear();
    m_agendaStatusText.clear();
    m_todayMode = std::clamp(Wh_GetIntValue(L"calendarToday", 0), 0, 2);
    m_todayColor = Wh_GetIntValue(L"calendarTodayColor", kTodayRed) & 0xFFFFFF;
    m_windowsAccent = WindowsAccentColor();
    LoadAgendas();
    RequestAgendas(m_hwnd);
    Layout();
}

bool CalendarPanel::AnimateContents(double dt) {
    const bool slide =
        SpringTowards(m_slide, m_slideVelocity, 0, 0.45, dt, 0.85);
    const bool form = SpringTowards(m_form, m_formVelocity, m_adding ? 1 : 0,
                                    m_adding ? 0.4 : 0.25, dt,
                                    m_adding ? kBounceDamping : 1.0);
    // Writing an event or a link: the caret blinks a moment (at the lower
    // frame rate).
    const bool typing = (m_adding || m_agendasView) && CaretBlinking();
    m_idleMotion = typing && !slide && !form;
    return slide || form || typing;
}

// Writing an event: a new one (nullptr) for the chosen day, or one to change.
void CalendarPanel::StartEditing(const CalendarEvent* event) {
    m_adding = true;
    m_statusText.clear();
    m_focus = Field::Title;
    m_digits = 0;
    if (event) {
        m_editingId = event->id;
        m_newTitle = event->title;
        m_newRemind = event->remind;
        m_newAllDay = event->minute < 0;
        const int minute = event->minute < 0 ? 9 * 60 : event->minute;
        m_newHour = minute / 60;
        m_newMinute = minute % 60;
    } else {
        m_editingId = 0;
        m_newTitle.clear();
        m_newRemind = true;
        m_newAllDay = false;
        // The next full hour.
        SYSTEMTIME now;
        GetLocalTime(&now);
        m_newHour = (now.wHour + 1) % 24;
        m_newMinute = 0;
    }
    m_form = 0;
    m_formVelocity = 0;
    StartIslandAnimation();
    Layout();
}

void CalendarPanel::StopEditing() {
    m_adding = false;
    m_editingId = 0;
    m_statusText.clear();
    StartIslandAnimation();
    Layout();
}

// A figure typed in the hour or the minute: two make the value (the hour
// then moves on to the minute), like typing a time on a phone.
void CalendarPanel::TypeDigit(int digit) {
    m_newAllDay = false;
    if (m_focus == Field::Hour) {
        if (m_digits == 0) {
            m_newHour = digit;
            m_digits = 1;
            // No hour starts with 3 or more and has a second figure.
            if (digit > 2) {
                m_focus = Field::Minute;
                m_digits = 0;
            }
        } else {
            const int value = m_newHour * 10 + digit;
            m_newHour = value <= 23 ? value : digit;
            m_focus = Field::Minute;
            m_digits = 0;
        }
    } else if (m_focus == Field::Minute) {
        if (m_digits == 0) {
            m_newMinute = digit;
            m_digits = digit > 5 ? 0 : 1;
        } else {
            m_newMinute = std::min(m_newMinute * 10 + digit, 59);
            m_digits = 0;
        }
    }
}

void CalendarPanel::RequestHolidays() {
    if (!g_settings.calendarHolidays || !m_year) {
        return;
    }
    LoadHolidays();
    AcquireSRWLockShared(&g_holidaysLock);
    const std::wstring country = g_holidaysCountry;
    const bool known = std::find(g_holidayYears.begin(), g_holidayYears.end(),
                                 m_year) != g_holidayYears.end();
    ReleaseSRWLockShared(&g_holidaysLock);
    if (country.empty() || known || GetTickCount64() < m_holidaysRetryAt ||
        std::find(m_holidaysAsked.begin(), m_holidaysAsked.end(), m_year) !=
            m_holidaysAsked.end()) {
        return;
    }
    // One at a time.
    if (m_holidayThread) {
        if (WaitForSingleObject(m_holidayThread, 0) != WAIT_OBJECT_0) {
            return;
        }
        CloseHandle(m_holidayThread);
        m_holidayThread = nullptr;
    }
    m_holidaysAsked.push_back(m_year);
    auto* request = new HolidayRequest{m_hwnd, m_year, country};
    m_holidayThread =
        CreateThread(nullptr, 0, HolidayThreadProc, request, 0, nullptr);
    if (!m_holidayThread) {
        delete request;
    }
}

bool CalendarPanel::OnAppMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_APP_COLOR_DONE) {
        return OnColorDone(lParam);
    }
    // An agenda read; the last one: the ones added meanwhile are read too.
    if (msg == WM_APP_AGENDAS) {
        if (wParam) {
            RequestAgendas(m_hwnd);
        }
        Layout();
        StartIslandAnimation();
        return true;
    }
    if (msg != WM_APP_HOLIDAYS) {
        return false;
    }
    // Not downloaded (offline): asked again when it's shown, after 10
    // minutes.
    if (lParam) {
        std::erase(m_holidaysAsked, (int)wParam);
        m_holidaysRetryAt = GetTickCount64() + 10 * 60 * 1000;
        return true;
    }
    SaveHolidays();
    Layout();
    // The next year shown, maybe.
    RequestHolidays();
    return true;
}

void CalendarPanel::OnDestroy() {
    EndColorDialog();
    // The agendas being read (any calendar's): stopped at the next step (a
    // request gives up after a few seconds), and read again as the calendar
    // opens next time.
    if (g_agendaThread) {
        g_agendaStop = true;
        WaitForSingleObject(g_agendaThread, 20000);
        CloseHandle(g_agendaThread);
        g_agendaThread = nullptr;
        AcquireSRWLockExclusive(&g_agendasLock);
        for (auto& agenda : g_agendas) {
            agenda.reading = false;
        }
        ReleaseSRWLockExclusive(&g_agendasLock);
    }
    if (m_holidayThread) {
        // The download gives up after a few seconds.
        WaitForSingleObject(m_holidayThread, 15000);
        CloseHandle(m_holidayThread);
        m_holidayThread = nullptr;
    }
}

void CalendarPanel::ChangeMonth(int delta) {
    // The month shown slides out (from where it is, if still sliding).
    if (m_slide < 0.5) {
        m_fromYear = m_year;
        m_fromMonth = m_month;
    }
    m_slideDirection = delta > 0 ? 1 : -1;
    m_slide = 1;
    m_slideVelocity = 0;
    StartIslandAnimation();
    m_month += delta;
    while (m_month < 1) {
        m_month += 12;
        m_year--;
    }
    while (m_month > 12) {
        m_month -= 12;
        m_year++;
    }
    RequestHolidays();
    Layout();
}

std::vector<const CalendarEvent*> CalendarPanel::DayEvents(
    size_t* hidden) const {
    std::vector<const CalendarEvent*> events;
    // Past the chosen wait (see g_hidePastAfter): only once asked for;
    // `hidden` gets how many.
    const int days =
        m_showHiddenFor == SelectedDayKey() ? 0 : HidePastDays(g_hidePastAfter);
    const long long today = TodayMinutes();
    if (hidden) {
        *hidden = 0;
    }
    for (const auto& event : g_events) {
        if (event.year == m_selectedYear && event.month == m_selectedMonth &&
            event.day == m_selectedDay) {
            if (EventIsPast(event, today, days)) {
                if (hidden) {
                    (*hidden)++;
                }
                continue;
            }
            events.push_back(&event);
        }
    }
    std::stable_sort(events.begin(), events.end(),
                     [](const CalendarEvent* a, const CalendarEvent* b) {
                         return a->minute < b->minute;
                     });
    return events;
}

// Today big (islandCalendarBigDay, #6165), as one card: the chosen day at
// the left (its weekday, month, number, week and holiday), the month's
// grid in the middle (its title over it, its neighbors' days dimmed), and at
// the right the alarm's and timer's buttons and the agendas' gear, over the
// chosen day's events, the upcoming events and holidays. A day clicked is
// shown at the left; the wheel changes the month.
void CalendarPanel::LayoutBigDay() {
    const float scale = m_scale;
    const float padding = std::round(kPanelPadding * scale);
    const float cell = std::round(kCalendarCell * scale);
    const float gridLeft = m_side;
    // The month shown ("Novembro de 2026", DrawBigDay) over the weekdays;
    // clicking it goes back to today, as in the month without the card.
    const float titleHeight = std::round(24 * scale);
    m_title = {gridLeft + cell * 0.2f, padding, gridLeft + cell * 7,
               padding + titleHeight};
    const float gridTop = m_title.bottom + std::round(26 * scale);
    const float divider = gridLeft + cell * 7 + padding;
    const float right = divider + padding + std::round(kBigDayRight * scale);
    m_dayRects.clear();
    WCHAR firstDayText[4] = L"6";
    GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
                    firstDayText, ARRAYSIZE(firstDayText));
    const int firstDay = (_wtoi(firstDayText) + 1) % 7;
    const int lead =
        m_year ? (DayOfWeek(m_year, m_month, 1) - firstDay + 7) % 7 : 0;
    const int days = m_year ? DaysInMonth(m_year, m_month) : 0;
    for (int day = 1; day <= days; day++) {
        const int index = day - 1 + lead;
        m_dayRects.push_back(
            {day,
             {gridLeft + (index % 7) * cell, gridTop + (index / 7) * cell,
              gridLeft + (index % 7 + 1) * cell,
              gridTop + (index / 7 + 1) * cell}});
    }
    const float button = std::round(32 * scale);
    const float gap = std::round(8 * scale);
    const float left = divider + padding;
    m_bigAlarms = {left, padding, right, padding + button};
    m_bigTimer = {left, padding + button + gap, right,
                  padding + button * 2 + gap};
    m_bigGear = {left, m_bigTimer.bottom + gap, right,
                 m_bigTimer.bottom + gap + button};
    // The chosen day's events, the agendas' and the user's own, by their
    // time (whole-day ones first).
    constexpr size_t kBigDayEvents = 3;
    const float row = std::round(kEventRowHeight * scale);
    const float line = std::round(22 * scale);
    float y = m_bigGear.bottom + gap * 1.5f;
    SYSTEMTIME today;
    GetLocalTime(&today);
    const int year = m_selectedYear ? m_selectedYear : today.wYear;
    const int month = m_selectedYear ? m_selectedMonth : today.wMonth;
    const int dayOf = m_selectedYear ? m_selectedDay : today.wDay;
    m_bigEvents.clear();
    for (const auto& item : AgendaEventsOn(year, month, dayOf)) {
        m_bigEvents.push_back(
            {item.event.minute, item.color, item.event.title});
    }
    for (const CalendarEvent* event : DayEvents()) {
        m_bigEvents.push_back({event->minute, -1, event->title});
    }
    std::stable_sort(m_bigEvents.begin(), m_bigEvents.end(),
                     [](const BigDayEvent& a, const BigDayEvent& b) {
                         return a.minute < b.minute;
                     });
    m_bigEventsMore = m_bigEvents.size() > kBigDayEvents
                          ? m_bigEvents.size() - kBigDayEvents
                          : 0;
    if (m_bigEventsMore) {
        m_bigEvents.resize(kBigDayEvents);
    }
    m_bigEventsHeader = {};
    if (!m_bigEvents.empty()) {
        m_bigEventsHeader = {left, y, right, y + row};
        y += row + line * (float)(m_bigEvents.size() + (m_bigEventsMore ? 1 : 0));
        y += gap;
    }
    // Then the upcoming events and holidays (DrawUpcoming draws them,
    // UpcomingHit finds their rows).
    m_upcomingAgenda = UpcomingAgendaEvents(3);
    m_upcomingAgendaHeader = {};
    if (!m_upcomingAgenda.empty()) {
        m_upcomingAgendaHeader = {left, y, right, y + row};
        y += row + line * (float)m_upcomingAgenda.size() + gap;
    }
    m_upcoming = UpcomingHolidays(3);
    m_upcomingHeader = {};
    if (!m_upcoming.empty()) {
        m_upcomingHeader = {left, y, right, y + row};
        y += row + line * (float)m_upcoming.size();
    }
    m_bigDivider = divider;
    // (Taller than the month with a lot on the right.)
    m_size = {(LONG)std::ceil(right + padding),
              (LONG)std::ceil(std::max(gridTop + cell * 6, y) + padding)};
}

void CalendarPanel::Layout() {
    const float scale = m_scale;
    const float padding = std::round(kPanelPadding * scale);
    const float cell = std::round(kCalendarCell * scale);
    const float width = padding * 2 + cell * 7;
    const float top = padding + std::round(kCalendarClockHeight * scale);
    const float titleHeight = cell * 1.2f;
    const float gridTop = top + titleHeight + cell * 0.8f;
    // Today big (not in the agendas' view): a card of its own.
    m_side = g_settings.islandCalendarBigDay && !m_agendasView
                 ? std::round(kCalendarSideWidth * scale)
                 : 0;
    m_bigAlarms = m_bigTimer = m_bigGear = {};
    if (m_side > 0) {
        LayoutBigDay();
        return;
    }

    if (m_agendasView) {
        m_dayRects.clear();
        m_holidayRows.clear();
        m_eventRows.clear();
        m_dayAgenda.clear();
        m_agendaEventRows.clear();
        m_hiddenRow = {};
        m_upcoming.clear();
        m_upcomingAgenda.clear();
        m_agendaMore = 0;
        m_eventsHeader = m_addButton = m_titleField = m_hourBox = m_minuteBox =
            m_allDayButton = m_remindButton = m_saveButton = m_cancelButton =
                m_status = m_upcomingHeader = m_upcomingAgendaHeader =
                    m_agendaMoreRow = {};
        LayoutAgendas(top + titleHeight);
        return;
    }
    m_agendaRows.clear();
    m_agendaRemovable.clear();
    m_agendaHint = m_linkField = m_linkAdd = m_agendaStatus =
        m_todayHeader = m_deleteHeader = {};
    m_todayChips = {};
    m_deleteChips = {};

    // The days of the month shown, where they are.
    m_dayRects.clear();
    WCHAR firstDayText[4] = L"6";
    GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
                    firstDayText, ARRAYSIZE(firstDayText));
    const int firstDay = (_wtoi(firstDayText) + 1) % 7;
    const int lead = m_year ? (DayOfWeek(m_year, m_month, 1) - firstDay + 7) % 7
                            : 0;
    const int days = m_year ? DaysInMonth(m_year, m_month) : 0;
    for (int day = 1; day <= days; day++) {
        const int index = day - 1 + lead;
        m_dayRects.push_back(
            {day,
             {padding + (index % 7) * cell, gridTop + (index / 7) * cell,
              padding + (index % 7 + 1) * cell,
              gridTop + (index / 7 + 1) * cell}});
    }

    // The chosen day's holidays, agenda events and events, then adding one.
    float y = gridTop + cell * 6 + std::round(6 * scale);
    const float row = std::round(kEventRowHeight * scale);
    m_eventsHeader = {padding, y, width - padding, y + row};
    y += row;
    m_holidayRows.clear();
    for (size_t i = 0;
         i < HolidaysOn(m_selectedYear, m_selectedMonth, m_selectedDay).size() &&
         i < 2;
         i++) {
        m_holidayRows.push_back({padding, y, width - padding, y + row});
        y += row;
    }
    // (Up to kEventsShown agenda events; how many more under them.)
    m_dayAgenda =
        AgendaEventsOn(m_selectedYear, m_selectedMonth, m_selectedDay);
    m_agendaMore = m_dayAgenda.size() > (size_t)kEventsShown
                       ? m_dayAgenda.size() - kEventsShown
                       : 0;
    if (m_agendaMore) {
        m_dayAgenda.resize(kEventsShown);
    }
    m_agendaEventRows.clear();
    for (size_t i = 0; i < m_dayAgenda.size(); i++) {
        m_agendaEventRows.push_back({padding, y, width - padding, y + row});
        y += row;
    }
    m_agendaMoreRow = {};
    if (m_agendaMore) {
        m_agendaMoreRow = {padding, y, width - padding, y + row * 0.7f};
        y += row * 0.7f;
    }
    m_eventRows.clear();
    const auto events = DayEvents(&m_hiddenCount);
    for (size_t i = 0; i < events.size() && (int)i < kEventsShown; i++) {
        m_eventRows.push_back(
            {events[i]->id, {padding, y, width - padding, y + row}});
        y += row;
    }
    m_hiddenRow = {};
    if (m_hiddenCount) {
        m_hiddenRow = {padding, y, width - padding, y + row * 0.7f};
        y += row * 0.7f;
    }
    if (events.empty() && m_hiddenCount == 0 && m_holidayRows.empty() &&
        m_dayAgenda.empty()) {
        // "No events".
        y += row;
    }
    y += std::round(4 * scale);
    const float field = std::round(32 * scale);
    if (!m_adding) {
        m_addButton = {padding, y, width - padding, y + field};
        m_titleField = m_hourBox = m_minuteBox = m_allDayButton =
            m_remindButton = m_saveButton = m_cancelButton = m_status = {};
        y += field;
    } else {
        // The title; under it, the time and the reminder; then "Cancel" and
        // "Add" (or "Save").
        m_addButton = {};
        const float gap = std::round(6 * scale);
        m_titleField = {padding, y, width - padding, y + field};
        y += field + gap;
        // From the right: the bell, the minute, ":", the hour; "All day"
        // takes what's left.
        const float box = std::round(50 * scale);
        const float time = std::round(40 * scale);
        const float colon = std::round(12 * scale);
        m_remindButton = {width - padding - time, y, width - padding,
                          y + time};
        m_minuteBox = {m_remindButton.left - gap * 2 - box, y,
                       m_remindButton.left - gap * 2, y + time};
        m_hourBox = {m_minuteBox.left - colon - box, y,
                     m_minuteBox.left - colon, y + time};
        m_allDayButton = {padding, y, m_hourBox.left - gap * 2, y + time};
        y += time;
        // (A problem in red, or what the bell does: #6156.)
        m_status = {padding, y, width - padding, y + row * 0.8f};
        y += row * 0.8f;
        y += gap * 1.5f;
        const float buttonWidth =
            std::floor((width - padding * 2 - gap) / 2);
        m_cancelButton = {padding, y, padding + buttonWidth, y + field};
        m_saveButton = {width - padding - buttonWidth, y, width - padding,
                        y + field};
        y += field;
    }
    // The next agenda events and holidays, from today.
    m_upcomingAgenda = UpcomingAgendaEvents(4);
    m_upcomingAgendaHeader = {};
    if (!m_upcomingAgenda.empty()) {
        y += std::round(8 * scale);
        m_upcomingAgendaHeader = {padding, y, width - padding, y + row};
        y += row + m_upcomingAgenda.size() * std::round(22 * scale);
    }
    m_upcoming = UpcomingHolidays(4);
    m_upcomingHeader = {};
    if (!m_upcoming.empty()) {
        y += std::round(8 * scale);
        m_upcomingHeader = {padding, y, width - padding, y + row};
        y += row + m_upcoming.size() * std::round(22 * scale);
    }
    y += padding;
    m_size = {(LONG)width, (LONG)std::ceil(y)};
}

// The upcoming events' and holidays' rows (see DrawUpcoming).
CalendarPanel::Hit CalendarPanel::UpcomingHit(POINT pt) const {
    const float line = std::round(22 * m_scale);
    for (const bool agenda : {true, false}) {
        const D2D1_RECT_F& header =
            agenda ? m_upcomingAgendaHeader : m_upcomingHeader;
        const size_t count =
            agenda ? m_upcomingAgenda.size() : m_upcoming.size();
        if (header.bottom <= header.top || pt.x < header.left ||
            pt.x >= header.right || pt.y < header.bottom) {
            continue;
        }
        const size_t i = (size_t)((pt.y - header.bottom) / line);
        if (i >= count) {
            continue;
        }
        if (agenda) {
            const AgendaEvent& event = m_upcomingAgenda[i].event;
            return {Part::Upcoming,
                    event.year * 10000LL + event.month * 100 + event.day};
        }
        const Holiday& holiday = m_upcoming[i];
        return {Part::Upcoming,
                holiday.year * 10000LL + holiday.month * 100 + holiday.day};
    }
    return {};
}

CalendarPanel::Hit CalendarPanel::HitTest(POINT pt) const {
    // Today big: its buttons, the upcoming holidays and the days.
    if (m_side > 0) {
        if (PointInRect(pt, m_bigAlarms)) {
            return {Part::BigAlarms};
        }
        if (PointInRect(pt, m_bigTimer)) {
            return {Part::BigTimer};
        }
        if (PointInRect(pt, m_bigGear)) {
            return {Part::BigGear};
        }
        if (const Hit upcoming = UpcomingHit(pt);
            upcoming.part != Part::None) {
            return upcoming;
        }
        if (PointInRect(pt, m_title)) {
            return {Part::Title};
        }
        for (const auto& [day, rect] : m_dayRects) {
            if (PointInRect(pt, rect)) {
                return {Part::Day, day};
            }
        }
        return {};
    }
    if (PointInRect(pt, m_agendasButton)) {
        return {Part::Agendas};
    }
    if (m_agendasView) {
        // An agenda's color at its start, its "x" at its end.
        for (size_t i = 0; i < m_agendaRows.size(); i++) {
            const D2D1_RECT_F& row = m_agendaRows[i];
            if (!PointInRect(pt, row)) {
                continue;
            }
            if (pt.x < row.left + std::round(28 * m_scale)) {
                return {Part::AgendaColor, (long long)i};
            }
            if (pt.x >= row.right - std::round(28 * m_scale) &&
                i < m_agendaRemovable.size() && m_agendaRemovable[i]) {
                return {Part::AgendaRemove, (long long)i};
            }
            return {};
        }
        if (PointInRect(pt, m_linkField)) {
            return {Part::LinkField};
        }
        if (PointInRect(pt, m_linkAdd)) {
            return {Part::LinkAdd};
        }
        for (size_t i = 0; i < m_todayChips.size(); i++) {
            if (PointInRect(pt, m_todayChips[i])) {
                return {Part::TodayChoice, (long long)i};
            }
        }
        for (size_t i = 0; i < m_deleteChips.size(); i++) {
            if (PointInRect(pt, m_deleteChips[i])) {
                return {Part::DeleteChoice, (long long)i};
            }
        }
        if (PointInRect(pt, m_deletePastButton)) {
            return {Part::DeletePast};
        }
        return {};
    }
    if (PointInRect(pt, m_hiddenRow)) {
        return {Part::ShowHidden};
    }
    if (const Hit upcoming = UpcomingHit(pt); upcoming.part != Part::None) {
        return upcoming;
    }
    if (PointInRect(pt, m_previous)) {
        return {Part::Previous};
    }
    if (PointInRect(pt, m_next)) {
        return {Part::Next};
    }
    if (PointInRect(pt, m_title)) {
        return {Part::Title};
    }
    for (const auto& [day, rect] : m_dayRects) {
        if (PointInRect(pt, rect)) {
            return {Part::Day, day};
        }
    }
    for (const auto& [id, rect] : m_eventRows) {
        // The "x" at the end of the row; the rest of it edits the event.
        if (PointInRect(pt, rect)) {
            if (pt.x >= rect.right - std::round(28 * m_scale)) {
                return {Part::DeleteEvent, id};
            }
            return {Part::EventRow, id};
        }
    }
    if (PointInRect(pt, m_addButton)) {
        return {Part::Add};
    }
    if (PointInRect(pt, m_titleField)) {
        return {Part::TitleField};
    }
    // The hour and the minute: their arrows (at the right end, up adds,
    // down takes away), or the box itself, to type.
    for (Part part : {Part::Hour, Part::Minute}) {
        const D2D1_RECT_F& box = part == Part::Hour ? m_hourBox : m_minuteBox;
        if (!PointInRect(pt, box)) {
            continue;
        }
        if (pt.x >= box.right - std::round(16 * m_scale)) {
            return {part, pt.y < (box.top + box.bottom) / 2 ? 1 : -1};
        }
        return {part, 0};
    }
    if (PointInRect(pt, m_allDayButton)) {
        return {Part::AllDay};
    }
    if (PointInRect(pt, m_remindButton)) {
        return {Part::Remind};
    }
    if (PointInRect(pt, m_saveButton)) {
        return {Part::Save};
    }
    if (PointInRect(pt, m_cancelButton)) {
        return {Part::Cancel};
    }
    return {};
}

void CalendarPanel::DrawField(ID2D1RenderTarget* target,
                              ID2D1SolidColorBrush* brush,
                              const D2D1_RECT_F& rect,
                              const std::wstring& text,
                              PCWSTR placeholder,
                              bool focused) {
    const float s = m_scale;
    const float radius = std::round(8 * s);
    brush->SetColor(Fg(focused ? 0.14f : 0.09f));
    GlassFill(target, {rect, radius, radius}, brush);
    if (focused) {
        brush->SetColor(Accent(1));
        target->DrawRoundedRectangle(
            {{rect.left + 0.75f, rect.top + 0.75f, rect.right - 0.75f,
              rect.bottom - 0.75f},
             radius,
             radius},
            brush, 1.5f);
    }
    const D2D1_RECT_F textRect{rect.left + std::round(10 * s), rect.top,
                               rect.right - std::round(8 * s), rect.bottom};
    const bool empty = text.empty();
    const std::wstring shown = empty ? std::wstring(placeholder) : text;
    float caretX = textRect.left;
    winrt::com_ptr<IDWriteTextLayout> layout;
    if (SUCCEEDED(m_dwrite->CreateTextLayout(
            shown.c_str(), (UINT32)shown.size(), m_textFormat.get(), 10000,
            textRect.bottom - textRect.top, layout.put()))) {
        DWRITE_TEXT_METRICS metrics;
        layout->GetMetrics(&metrics);
        const float overflow =
            empty ? 0
                  : std::max(0.0f, metrics.widthIncludingTrailingWhitespace -
                                       (textRect.right - textRect.left));
        target->PushAxisAlignedClip(textRect, D2D1_ANTIALIAS_MODE_ALIASED);
        brush->SetColor(Fg(empty ? 0.4f : 0.95f));
        target->DrawTextLayout({textRect.left - overflow, textRect.top},
                               layout.get(), brush);
        target->PopAxisAlignedClip();
        if (!empty) {
            caretX = textRect.left + metrics.widthIncludingTrailingWhitespace -
                     overflow;
        }
    }
    if (focused) {
        const float middle = (rect.top + rect.bottom) / 2;
        brush->SetColor(CaretColor());
        target->FillRectangle({caretX + 1, middle - std::round(8 * s),
                               caretX + 2.5f, middle + std::round(8 * s)},
                              brush);
    }
}

// With today big at the left (islandCalendarBigDay): that column, and the
// calendar beside it as it's drawn without it.
void CalendarPanel::Draw(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush) {
    if (m_side <= 0) {
        DrawCalendar(target, brush);
        return;
    }
    DrawBigDay(target, brush);
}

// A day's ISO 8601 week (weeks start on Monday; the first has the year's
// first Thursday), as calendars number them.
int IsoWeek(int year, int month, int day) {
    int ordinal = day;
    for (int m = 1; m < month; m++) {
        ordinal += DaysInMonth(year, m);
    }
    // Monday 1 to Sunday 7.
    const int weekday = (DayOfWeek(year, month, day) + 6) % 7 + 1;
    const int week = (ordinal - weekday + 10) / 7;
    auto weeks = [](int y) {
        auto p = [](int x) { return (x + x / 4 - x / 100 + x / 400) % 7; };
        return p(y) == 4 || p(y - 1) == 3 ? 53 : 52;
    };
    if (week < 1) {
        return weeks(year - 1);
    }
    return week > weeks(year) ? 1 : week;
}

// A month's title in the dates' language ("Novembro de 2026").
std::wstring YearMonthTitle(int year, int month) {
    SYSTEMTIME first{(WORD)year, (WORD)month, 0, 1};
    WCHAR title[64] = L"";
    GetDateFormatEx(DateLocale(), DATE_YEARMONTH, &first, nullptr, title,
                    ARRAYSIZE(title), nullptr);
    if (title[0]) {
        title[0] = towupper(title[0]);
    }
    return title;
}

// The card (see LayoutBigDay): the chosen day at the left, the month in the
// middle, a line, then the buttons and the upcoming holidays.
void CalendarPanel::DrawBigDay(ID2D1RenderTarget* target,
                               ID2D1SolidColorBrush* brush) {
    if (!m_bigDayFormat || !m_dateFormat || !m_textFormat || !m_dayFormat ||
        !m_titleFormat || !m_smallFormat) {
        return;
    }
    const float scale = m_scale;
    const float padding = std::round(kPanelPadding * scale);
    const float cell = std::round(kCalendarCell * scale);
    const float height = (float)m_size.cy;
    SYSTEMTIME today;
    GetLocalTime(&today);
    SYSTEMTIME chosen{(WORD)m_selectedYear, (WORD)m_selectedMonth, 0,
                      (WORD)m_selectedDay};
    if (!m_selectedYear) {
        chosen = today;
    }
    // The left column: its weekday (bold), its month, its number, its week
    // and its holiday.
    {
        const float left = padding;
        const float right = m_side - padding;
        auto line = [&](const std::wstring& text, IDWriteTextFormat* format,
                        float top, float bottom, D2D1_COLOR_F color) {
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            brush->SetColor(color);
            target->DrawText(text.c_str(), (UINT32)text.size(), format,
                             {left, top, right, bottom}, brush,
                             D2D1_DRAW_TEXT_OPTIONS_CLIP);
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        };
        auto date = [&](PCWSTR picture) {
            WCHAR text[64] = L"";
            GetDateFormatEx(DateLocale(), 0, &chosen, picture, text,
                            ARRAYSIZE(text), nullptr);
            std::wstring result = text;
            if (!result.empty()) {
                result[0] = towupper(result[0]);
            }
            return result;
        };
        float y = padding + std::round(14 * scale);
        line(date(L"dddd"), m_titleFormat.get(), y, y + std::round(22 * scale),
             Fg(0.95f));
        y += std::round(22 * scale);
        line(date(L"MMMM"), m_dateFormat.get(), y, y + std::round(20 * scale),
             Fg(0.75f));
        y += std::round(24 * scale);
        brush->SetColor(Fg(0.97f));
        const std::wstring number = std::to_wstring(chosen.wDay);
        target->DrawText(number.c_str(), (UINT32)number.size(),
                         m_bigDayFormat.get(),
                         {left, y, right, y + std::round(84 * scale)}, brush,
                         D2D1_DRAW_TEXT_OPTIONS_CLIP);
        const auto holidays =
            HolidaysOn(chosen.wYear, chosen.wMonth, chosen.wDay);
        const float bottom = height - padding - std::round(8 * scale);
        const float weekTop =
            bottom - std::round((holidays.empty() ? 18 : 36) * scale);
        line(std::wstring(Tr(L"Week", L"Semana")) + L" " +
                 std::to_wstring(
                     IsoWeek(chosen.wYear, chosen.wMonth, chosen.wDay)),
             m_smallFormat.get(), weekTop, weekTop + std::round(18 * scale),
             Fg(0.6f));
        if (!holidays.empty()) {
            line(holidays.front(), m_smallFormat.get(),
                 weekTop + std::round(18 * scale), bottom, Fg(0.8f));
        }
    }
    // The month: its title (so a month paged to is told), the weekdays'
    // short names, then six weeks (the neighbors' days dimmed), today in its
    // circle, the chosen day in a ring, holidays and agenda days in their
    // colors, a dot under days with events.
    WCHAR firstDayText[4] = L"6";
    GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
                    firstDayText, ARRAYSIZE(firstDayText));
    const int firstDay = (_wtoi(firstDayText) + 1) % 7;
    const float gridLeft = m_side;
    const float headerTop = m_title.bottom;
    const float gridTop = headerTop + std::round(26 * scale);
    if (m_year) {
        const std::wstring monthTitle = YearMonthTitle(m_year, m_month);
        brush->SetColor(Fg(0.95f));
        target->DrawText(monthTitle.c_str(), (UINT32)monthTitle.size(),
                         m_titleFormat.get(), m_title, brush,
                         D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    for (int column = 0; column < 7; column++) {
        const int weekday = (firstDay + column) % 7;
        WCHAR name[16] = L"";
        GetLocaleInfoEx(DateLocale(),
                        LOCALE_SABBREVDAYNAME1 + (weekday + 6) % 7, name,
                        ARRAYSIZE(name));
        brush->SetColor(Fg(0.55f));
        target->DrawText(name, (UINT32)wcslen(name), m_dayFormat.get(),
                         {gridLeft + column * cell, headerTop,
                          gridLeft + (column + 1) * cell, gridTop},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    const int year = m_year;
    const int month = m_month;
    if (year) {
        D2D1_COLOR_F todayFill = TodayColor();
        const float todayInk =
            Luminance(todayFill.r, todayFill.g, todayFill.b) > 0.62f ? 0.08f
                                                                       : 1.0f;
        const int lead = (DayOfWeek(year, month, 1) - firstDay + 7) % 7;
        const int days = DaysInMonth(year, month);
        const int previousDays = DaysInMonth(month == 1 ? year - 1 : year,
                                             month == 1 ? 12 : month - 1);
        for (int index = 0; index < 42; index++) {
            int day = index - lead + 1;
            bool inMonth = true;
            if (day < 1) {
                day += previousDays;
                inMonth = false;
            } else if (day > days) {
                day -= days;
                inMonth = false;
            }
            const D2D1_RECT_F rect{gridLeft + (index % 7) * cell,
                                   gridTop + (index / 7) * cell,
                                   gridLeft + (index % 7 + 1) * cell,
                                   gridTop + (index / 7 + 1) * cell};
            const D2D1_POINT_2F center{(rect.left + rect.right) / 2,
                                       (rect.top + rect.bottom) / 2};
            const float circle = cell * 0.4f;
            const bool isToday = inMonth && day == today.wDay &&
                                 month == today.wMonth && year == today.wYear;
            const bool selected = inMonth && day == chosen.wDay &&
                                  month == chosen.wMonth &&
                                  year == chosen.wYear;
            const bool hover = inMonth && m_hover.part == Part::Day &&
                               m_hover.value == day;
            if (isToday) {
                brush->SetColor(todayFill);
                target->FillEllipse({center, circle, circle}, brush);
            } else if (selected) {
                brush->SetColor(Accent(0.85f));
                target->FillEllipse({center, circle, circle}, brush);
            } else if (const float lit =
                           Hover(HoverKey(5002, year * 10000 + month * 100 +
                                                    index),
                                 hover);
                       lit > 0.01f) {
                brush->SetColor(Fg(0.1f * lit));
                GlassFillEllipse(target, {center, circle, circle}, brush);
            }
            if (selected && isToday) {
                brush->SetColor(Fg(0.6f));
                target->DrawEllipse({center, circle + 2 * scale,
                                     circle + 2 * scale},
                                    brush, 1.5f);
            }
            int agendaColor = 0;
            const bool agendaDay =
                inMonth && !isToday && !selected &&
                AgendaDayColor(year, month, day, &agendaColor);
            const bool holiday = inMonth && !isToday && !selected &&
                                 !agendaDay &&
                                 !HolidaysOn(year, month, day).empty();
            brush->SetColor(
                isToday     ? D2D1::ColorF(todayInk, todayInk, todayInk, 1)
                : selected  ? AccentInk(1)
                : agendaDay ? RgbColor(agendaColor, 1)
                : holiday   ? D2D1::ColorF(1.0f, 0.36f, 0.36f, 1)
                            : Fg(inMonth ? 0.9f : 0.3f));
            const std::wstring text = std::to_wstring(day);
            target->DrawText(text.c_str(), (UINT32)text.size(),
                             m_dayFormat.get(), rect, brush);
            if (inMonth && HasEvents(year, month, day)) {
                const float dot = 1.8f * scale;
                brush->SetColor(D2D1::ColorF(1.0f, 0.27f, 0.23f, 1));
                target->FillEllipse(
                    {{center.x, center.y + circle - 3 * scale}, dot, dot},
                    brush);
            }
        }
    }
    // The line between the month and the buttons.
    brush->SetColor(Fg(0.12f));
    target->FillRectangle({m_bigDivider, padding, m_bigDivider + 1,
                           height - padding},
                          brush);
    // The buttons (the gear's glyph before its name).
    for (const auto& [rect, part, name, glyph] :
         {std::tuple{m_bigAlarms, Part::BigAlarms,
                     Tr(L"Set an alarm", L"Criar alarme"), WCHAR(0)},
          std::tuple{m_bigTimer, Part::BigTimer,
                     Tr(L"Set a timer", L"Criar timer"), WCHAR(0)},
          std::tuple{m_bigGear, Part::BigGear, Tr(L"Agendas", L"Agendas"),
                     kGlyphSettings}}) {
        const float lit = Hover(rect, m_hover.part == part);
        const float radius = std::round(8 * scale);
        brush->SetColor(Fg(0.1f + 0.08f * lit));
        GlassFill(target, {rect, radius, radius}, brush);
        brush->SetColor(Fg(0.9f));
        D2D1_RECT_F label = rect;
        if (glyph && m_iconFormat) {
            const float inset = std::round(4 * scale);
            const D2D1_RECT_F icon{rect.left + inset, rect.top,
                                   rect.left + inset + std::round(28 * scale),
                                   rect.bottom};
            DrawGlyph(target, &glyph, 1, m_iconFormat.get(), icon, brush);
            label.left = icon.right;
            label.right -= inset;
        }
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        target->DrawText(name, (UINT32)wcslen(name), m_textFormat.get(), label,
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    }
    // The chosen day's events: a line over them, their title ("Events,
    // October 27"), then each one's color bar, time and title, and how many
    // more.
    if (const D2D1_RECT_F& header = m_bigEventsHeader;
        header.bottom > header.top) {
        brush->SetColor(Fg(0.1f));
        target->FillRectangle(
            {header.left, header.top, header.right, header.top + 1}, brush);
        WCHAR day[64] = L"";
        GetDateFormatEx(DateLocale(), 0, &chosen,
                        MonthDayPicture(false).c_str(), day, ARRAYSIZE(day),
                        nullptr);
        const std::wstring title =
            std::wstring(Tr(L"Events, ", L"Eventos, ")) + day;
        brush->SetColor(Fg(0.6f));
        target->DrawText(title.c_str(), (UINT32)title.size(),
                         m_smallFormat.get(), header, brush,
                         D2D1_DRAW_TEXT_OPTIONS_CLIP);
        const float line = std::round(22 * scale);
        float y = header.bottom;
        for (const auto& event : m_bigEvents) {
            brush->SetColor(event.color >= 0
                                ? RgbColor(event.color, 0.9f)
                                : D2D1::ColorF(1.0f, 0.27f, 0.23f, 0.9f));
            target->FillRoundedRectangle(
                {{header.left, y + std::round(4 * scale),
                  header.left + std::round(3 * scale),
                  y + line - std::round(4 * scale)},
                 1.5f,
                 1.5f},
                brush);
            const float timeWidth = std::round(58 * scale);
            const std::wstring time = EventTimeText(event.minute);
            brush->SetColor(Fg(0.55f));
            target->DrawText(time.c_str(), (UINT32)time.size(),
                             m_smallFormat.get(),
                             {header.left + std::round(9 * scale), y,
                              header.left + timeWidth, y + line},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            const std::wstring name = event.title.empty()
                                          ? std::wstring(Tr(L"Event", L"Evento"))
                                          : event.title;
            brush->SetColor(Fg(0.95f));
            target->DrawText(name.c_str(), (UINT32)name.size(),
                             m_smallFormat.get(),
                             {header.left + timeWidth, y, header.right,
                              y + line},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            y += line;
        }
        if (m_bigEventsMore) {
            const std::wstring more = L"+" + std::to_wstring(m_bigEventsMore) +
                                      L" " + Tr(L"more", L"mais");
            brush->SetColor(Fg(0.5f));
            target->DrawText(more.c_str(), (UINT32)more.size(),
                             m_smallFormat.get(),
                             {header.left + std::round(9 * scale), y,
                              header.right, y + line},
                             brush);
        }
    }
    DrawUpcoming(target, brush);
}

void CalendarPanel::DrawCalendar(ID2D1RenderTarget* target,
                                 ID2D1SolidColorBrush* brush) {
    if (!m_titleFormat || !m_dayFormat || !m_iconFormat || !m_textFormat ||
        !m_smallFormat) {
        return;
    }
    const float scale = m_scale;
    // (Beside today big, when it's there: see Draw.)
    const float width = (float)m_size.cx - m_side;
    const float padding = std::round(kPanelPadding * scale);
    const float cell = std::round(kCalendarCell * scale);

    // The time, big, and the date ("sexta-feira, 3 de outubro").
    SYSTEMTIME now;
    GetLocalTime(&now);
    const float clockHeight = std::round(kCalendarClockHeight * scale);
    if (m_clockFormat && m_dateFormat) {
        std::wstring marker;
        std::wstring time = FormatClockTime(now, true, &marker);
        if (!marker.empty()) {
            time += L" " + marker;
        }
        const D2D1_RECT_F timeRect{padding + cell * 0.2f, padding,
                                   width - padding,
                                   padding + clockHeight * 0.62f};
        brush->SetColor(Fg(0.97f));
        winrt::com_ptr<IDWriteTextLayout> layout;
        if (SUCCEEDED(m_dwrite->CreateTextLayout(
                time.c_str(), (UINT32)time.size(), m_clockFormat.get(),
                timeRect.right - timeRect.left,
                timeRect.bottom - timeRect.top, layout.put()))) {
            if (m_tabular) {
                layout->SetTypography(m_tabular.get(),
                                      {0, (UINT32)time.size()});
            }
            SmallClockMarker(layout.get(), time);
            target->DrawTextLayout({timeRect.left, timeRect.top}, layout.get(),
                                   brush);
        }
        WCHAR date[96] = L"";
        GetDateFormatEx(DateLocale(), DATE_LONGDATE, &now, nullptr, date,
                        ARRAYSIZE(date), nullptr);
        if (date[0]) {
            date[0] = towupper(date[0]);
        }
        brush->SetColor(Fg(0.6f));
        target->DrawText(date, (UINT32)wcslen(date), m_dateFormat.get(),
                         {timeRect.left, timeRect.bottom, width - padding,
                          padding + clockHeight - std::round(8 * scale)},
                         brush);
        // A line under it.
        brush->SetColor(Fg(0.1f));
        target->FillRectangle({padding, padding + clockHeight - 1,
                               width - padding, padding + clockHeight},
                              brush);
    }
    const float top = padding + clockHeight;

    // The agendas' button and the arrows; the title slides with the days.
    const float titleHeight = cell * 1.2f;
    // (In the agendas' view, its button goes back, at the end.)
    const float buttonRight =
        m_agendasView ? width - padding : width - padding - cell * 2;
    m_title = {padding + cell * 0.2f, top, buttonRight - cell,
               top + titleHeight};
    m_agendasButton = {buttonRight - cell, top, buttonRight, top + titleHeight};
    m_previous = m_next = {};
    if (!m_agendasView) {
        m_previous = {width - padding - cell * 2, top, width - padding - cell,
                      top + titleHeight};
        m_next = {width - padding - cell, top, width - padding,
                  top + titleHeight};
    }
    for (Part button : {Part::Agendas, Part::Previous, Part::Next}) {
        const D2D1_RECT_F& rect = button == Part::Agendas    ? m_agendasButton
                                  : button == Part::Previous ? m_previous
                                                             : m_next;
        if (rect.right <= rect.left) {
            continue;
        }
        if (const float lit = Hover(rect, m_hover.part == button);
            lit > 0.01f) {
            const float inset = cell * 0.18f;
            const D2D1_RECT_F back{rect.left + inset, rect.top + inset,
                                   rect.right - inset, rect.bottom - inset};
            brush->SetColor(Fg(0.16f * lit));
            GlassFill(target,
                      {back, (back.bottom - back.top) / 2,
                       (back.bottom - back.top) / 2},
                      brush);
        }
        const WCHAR glyph =
            button == Part::Agendas
                ? (m_agendasView ? kGlyphCalendarBack : kGlyphSettings)
            : button == Part::Previous ? kGlyphPrevious
                                       : kGlyphNext;
        brush->SetColor(Fg(0.9f));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(), rect, brush);
    }
    if (m_agendasView) {
        PCWSTR title = Tr(L"Agendas", L"Agendas");
        brush->SetColor(Fg(0.95f));
        target->DrawText(title, (UINT32)wcslen(title), m_titleFormat.get(),
                         m_title, brush);
        DrawAgendas(target, brush);
        return;
    }

    // Weekday initials, starting on the locale's first day of the week.
    WCHAR firstDayText[4] = L"6";
    GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
                    firstDayText, ARRAYSIZE(firstDayText));
    // LOCALE_IFIRSTDAYOFWEEK: 0 is Monday. Here, 0 is Sunday.
    const int firstDay = (_wtoi(firstDayText) + 1) % 7;
    const float headerTop = top + titleHeight;
    for (int column = 0; column < 7; column++) {
        const int weekday = (firstDay + column) % 7;
        // LOCALE_SSHORTESTDAYNAME1 is Monday.
        WCHAR name[16] = L"";
        GetLocaleInfoEx(DateLocale(),
                        LOCALE_SSHORTESTDAYNAME1 + (weekday + 6) % 7, name,
                        ARRAYSIZE(name));
        const D2D1_RECT_F rect{padding + column * cell, headerTop,
                               padding + (column + 1) * cell,
                               headerTop + cell * 0.8f};
        brush->SetColor(Fg(0.5f));
        target->DrawText(name, (UINT32)wcslen(name), m_dayFormat.get(), rect,
                         brush);
    }

    // The months: the new one slides in, the one before out, both fading,
    // clipped to the calendar.
    const float slide = (float)std::clamp(m_slide, 0.0, 1.0);
    target->PushAxisAlignedClip(
        {padding, top, width - padding, m_eventsHeader.top},
        D2D1_ANTIALIAS_MODE_ALIASED);
    if (slide > 0.001f && m_fromMonth) {
        DrawMonth(target, brush, m_fromYear, m_fromMonth,
                  -m_slideDirection * (1 - slide) * width * 0.6f, slide);
    }
    DrawMonth(target, brush, m_year, m_month,
              m_slideDirection * slide * width * 0.6f, 1 - slide);
    target->PopAxisAlignedClip();

    // The chosen day's events.
    SYSTEMTIME chosen{(WORD)m_selectedYear, (WORD)m_selectedMonth, 0,
                      (WORD)m_selectedDay};
    WCHAR day[64] = L"";
    GetDateFormatEx(DateLocale(), 0, &chosen, MonthDayPicture(false).c_str(),
                    day, ARRAYSIZE(day), nullptr);
    brush->SetColor(Fg(0.1f));
    target->FillRectangle({padding, m_eventsHeader.top, width - padding,
                           m_eventsHeader.top + 1},
                          brush);
    const std::wstring header =
        std::wstring(Tr(L"Events, ", L"Eventos, ")) + day;
    brush->SetColor(Fg(0.6f));
    target->DrawText(header.c_str(), (UINT32)header.size(),
                     m_smallFormat.get(), m_eventsHeader, brush);
    // Holidays: a green bar and the name.
    const auto holidays =
        HolidaysOn(m_selectedYear, m_selectedMonth, m_selectedDay);
    for (size_t i = 0; i < m_holidayRows.size() && i < holidays.size(); i++) {
        const D2D1_RECT_F& row = m_holidayRows[i];
        brush->SetColor(D2D1::ColorF(0.2f, 0.78f, 0.35f, 0.9f));
        target->FillRoundedRectangle(
            {{row.left, row.top + std::round(6 * scale),
              row.left + std::round(3 * scale),
              row.bottom - std::round(6 * scale)},
             1.5f,
             1.5f},
            brush);
        brush->SetColor(Fg(0.95f));
        target->DrawText(holidays[i].c_str(), (UINT32)holidays[i].size(),
                         m_textFormat.get(),
                         {row.left + std::round(10 * scale), row.top,
                          row.right, row.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    // The agendas' events: a bar in the agenda's color, the time, the title
    // (read only).
    for (size_t i = 0; i < m_agendaEventRows.size() && i < m_dayAgenda.size();
         i++) {
        const AgendaItem& item = m_dayAgenda[i];
        const D2D1_RECT_F& row = m_agendaEventRows[i];
        brush->SetColor(RgbColor(item.color, 0.9f));
        target->FillRoundedRectangle(
            {{row.left, row.top + std::round(6 * scale),
              row.left + std::round(3 * scale),
              row.bottom - std::round(6 * scale)},
             1.5f,
             1.5f},
            brush);
        const float timeWidth = std::round(64 * scale);
        const std::wstring time = EventTimeText(item.event.minute);
        brush->SetColor(Fg(0.55f));
        target->DrawText(time.c_str(), (UINT32)time.size(), m_smallFormat.get(),
                         {row.left + std::round(10 * scale), row.top,
                          row.left + timeWidth, row.bottom},
                         brush);
        const std::wstring title = item.event.title.empty()
                                       ? std::wstring(Tr(L"Event", L"Evento"))
                                       : item.event.title;
        brush->SetColor(Fg(0.95f));
        target->DrawText(title.c_str(), (UINT32)title.size(),
                         m_textFormat.get(),
                         {row.left + timeWidth, row.top, row.right, row.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    if (m_agendaMore) {
        const std::wstring more =
            L"+" + std::to_wstring(m_agendaMore) + L" " + Tr(L"more", L"mais");
        brush->SetColor(Fg(0.5f));
        target->DrawText(
            more.c_str(), (UINT32)more.size(), m_smallFormat.get(),
            {m_agendaMoreRow.left + std::round(10 * scale), m_agendaMoreRow.top,
             m_agendaMoreRow.right, m_agendaMoreRow.bottom},
            brush);
    }
    const auto events = DayEvents();
    if (m_hiddenRow.bottom > m_hiddenRow.top) {
        WCHAR more[64];
        swprintf_s(more, Tr(L"Show %zu hidden", L"Mostrar %zu ocultos"),
                   m_hiddenCount);
        brush->SetColor(Fg(0.45f + 0.4f * Hover(m_hiddenRow,
                                                m_hover.part ==
                                                    Part::ShowHidden)));
        target->DrawText(
            more, (UINT32)wcslen(more), m_smallFormat.get(),
            {m_hiddenRow.left + std::round(10 * scale), m_hiddenRow.top,
             m_hiddenRow.right, m_hiddenRow.bottom},
            brush);
    }
    if (events.empty() && m_hiddenCount == 0 && holidays.empty() &&
        m_dayAgenda.empty()) {
        PCWSTR none = Tr(L"No events", L"Nenhum evento");
        brush->SetColor(Fg(0.4f));
        target->DrawText(none, (UINT32)wcslen(none), m_textFormat.get(),
                         {padding, m_eventsHeader.bottom, width - padding,
                          m_eventsHeader.bottom +
                              std::round(kEventRowHeight * scale)},
                         brush);
    }
    for (size_t i = 0; i < m_eventRows.size() && i < events.size(); i++) {
        const CalendarEvent& event = *events[i];
        const D2D1_RECT_F& row = m_eventRows[i].second;
        const bool hover = (m_hover.part == Part::DeleteEvent) &&
                           m_hover.value == event.id;
        // The row under the mouse (or being edited) lights up: a click edits
        // it.
        const bool editing = m_adding && m_editingId == event.id;
        const float rowLit =
            Hover(row, m_hover.value == event.id &&
                           (m_hover.part == Part::EventRow ||
                            m_hover.part == Part::DeleteEvent));
        if (rowLit > 0.01f || editing) {
            const float rowRadius = std::round(7 * scale);
            brush->SetColor(Fg(editing ? 0.12f : 0.07f * rowLit));
            GlassFill(target, {row, rowRadius, rowRadius}, brush);
        }
        // A colored bar, the time, the title, the bell, the "x".
        brush->SetColor(D2D1::ColorF(1.0f, 0.27f, 0.23f, 0.9f));
        target->FillRoundedRectangle(
            {{row.left, row.top + std::round(6 * scale),
              row.left + std::round(3 * scale),
              row.bottom - std::round(6 * scale)},
             1.5f,
             1.5f},
            brush);
        const float timeWidth = std::round(64 * scale);
        const std::wstring time = EventTimeText(event.minute);
        brush->SetColor(Fg(0.55f));
        target->DrawText(time.c_str(), (UINT32)time.size(),
                         m_smallFormat.get(),
                         {row.left + std::round(10 * scale), row.top,
                          row.left + timeWidth, row.bottom},
                         brush);
        brush->SetColor(Fg(0.95f));
        target->DrawText(event.title.c_str(), (UINT32)event.title.size(),
                         m_textFormat.get(),
                         {row.left + timeWidth, row.top,
                          row.right - std::round(48 * scale), row.bottom},
                         brush);
        if (event.remind) {
            brush->SetColor(Fg(event.fired ? 0.3f : 0.6f));
            DrawGlyph(target, &kGlyphBell, 1, m_iconFormat.get(),
                      {row.right - std::round(48 * scale), row.top,
                       row.right - std::round(28 * scale), row.bottom},
                      brush);
        }
        brush->SetColor(Fg(0.4f + 0.55f * Hover(HoverKey(5000, event.id), hover)));
        DrawGlyph(target, &kGlyphEventDelete, 1, m_iconFormat.get(),
                  {row.right - std::round(28 * scale), row.top,
                   row.right, row.bottom},
                  brush);
    }

    DrawUpcoming(target, brush);

    // "+ New event", or its fields.
    if (!m_adding) {
        const float radius = std::round(8 * scale);
        brush->SetColor(
            Fg(0.09f + 0.07f * Hover(m_addButton, m_hover.part == Part::Add)));
        GlassFill(target, {m_addButton, radius, radius}, brush);
        PCWSTR add = Tr(L"+  New event", L"+  Novo evento");
        brush->SetColor(Fg(0.85f));
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        target->DrawText(add, (UINT32)wcslen(add), m_textFormat.get(),
                         m_addButton, brush);
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    } else {
        // The fields fade in, sliding down a little.
        const float form = (float)std::clamp(m_form, 0.0, 1.0);
        winrt::com_ptr<ID2D1Layer> formLayer;
        if (form < 0.999f) {
            target->CreateLayer(nullptr, formLayer.put());
        }
        if (formLayer) {
            target->PushLayer(
                D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                      D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                      D2D1::IdentityMatrix(), form),
                formLayer.get());
        }
        // On top of the panel's own placement.
        D2D1_MATRIX_3X2_F base;
        target->GetTransform(&base);
        target->SetTransform(
            D2D1::Matrix3x2F::Translation(
                0, -std::round(8 * scale) *
                       (1 - (float)std::clamp(m_form, 0.0, 1.2))) *
            *D2D1::Matrix3x2F::ReinterpretBaseType(&base));
        DrawField(target, brush, m_titleField, m_newTitle,
                  Tr(L"Title", L"Título"), m_focus == Field::Title);
        // "All day", blue when chosen.
        const float chipRadius =
            (m_allDayButton.bottom - m_allDayButton.top) / 2;
        brush->SetColor(m_newAllDay ? Accent(1)
                                    : Fg(0.1f + 0.08f * Hover(m_allDayButton,
                                                              m_hover.part ==
                                                                  Part::AllDay)));
        GlassFill(target, {m_allDayButton, chipRadius, chipRadius}, brush);
        PCWSTR allDay = Tr(L"All day", L"Dia todo");
        brush->SetColor(m_newAllDay ? AccentInk(1) : Fg(0.9f));
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        target->DrawText(allDay, (UINT32)wcslen(allDay), m_textFormat.get(),
                         m_allDayButton, brush);
        // The hour and the minute: click to type them (blue outline), or
        // use the arrows at their right end (or the wheel, or ↑ ↓).
        const float timeOpacity = m_newAllDay ? 0.35f : 1.0f;
        for (Part part : {Part::Hour, Part::Minute}) {
            const D2D1_RECT_F& box = part == Part::Hour ? m_hourBox : m_minuteBox;
            const bool focused =
                !m_newAllDay && m_focus == (part == Part::Hour ? Field::Hour
                                                               : Field::Minute);
            const float radius = std::round(8 * scale);
            brush->SetColor(Fg(
                (0.1f + 0.06f * Hover(box, m_hover.part == part || focused)) *
                timeOpacity));
            GlassFill(target, {box, radius, radius}, brush);
            if (focused) {
                brush->SetColor(Accent(1));
                target->DrawRoundedRectangle(
                    {{box.left + 0.75f, box.top + 0.75f, box.right - 0.75f,
                      box.bottom - 0.75f},
                     radius,
                     radius},
                    brush, 1.5f);
            }
            WCHAR value[8];
            swprintf_s(value, L"%02d",
                       part == Part::Hour ? m_newHour : m_newMinute);
            // Half typed: only the figure typed so far.
            if (focused && m_digits == 1) {
                swprintf_s(value, L"%d_",
                           part == Part::Hour ? m_newHour : m_newMinute);
            }
            const float arrow = std::round(14 * scale);
            brush->SetColor(Fg(0.95f * timeOpacity));
            target->DrawText(value, (UINT32)wcslen(value), m_dayFormat.get(),
                             {box.left, box.top, box.right - arrow / 2,
                              box.bottom},
                             brush);
            if ((m_hover.part == part || focused) && !m_newAllDay) {
                const float half = (box.bottom - box.top) / 2;
                const bool onArrows = m_hover.part == part;
                brush->SetColor(
                    Fg(onArrows && m_hover.value > 0 ? 0.95f : 0.4f));
                DrawGlyph(target, &kGlyphCollapse, 1, m_iconFormat.get(),
                          {box.right - arrow, box.top + 2,
                           box.right - 2, box.top + half},
                          brush);
                brush->SetColor(
                    Fg(onArrows && m_hover.value < 0 ? 0.95f : 0.4f));
                DrawGlyph(target, &kGlyphChevronDownCalendar, 1,
                          m_iconFormat.get(),
                          {box.right - arrow, box.top + half,
                           box.right - 2, box.bottom - 2},
                          brush);
            }
        }
        brush->SetColor(Fg(0.7f * timeOpacity));
        target->DrawText(L":", 1, m_dayFormat.get(),
                         {m_hourBox.right, m_hourBox.top, m_minuteBox.left,
                          m_hourBox.bottom},
                         brush);
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        // The reminder's bell: blue when on.
        const float radius = (m_remindButton.bottom - m_remindButton.top) / 2;
        brush->SetColor(m_newRemind ? Accent(1)
                                    : Fg(0.12f));
        GlassFill(target, {m_remindButton, radius, radius}, brush);
        brush->SetColor(Fg(0.95f));
        DrawGlyph(target, &kGlyphBell, 1, m_iconFormat.get(), m_remindButton,
                  brush);
        const float saveRadius = (m_saveButton.bottom - m_saveButton.top) / 2;
        brush->SetColor(Accent(1.0f - 0.15f * Hover(m_saveButton,
                                                          m_hover.part ==
                                                              Part::Save)));
        target->FillRoundedRectangle({m_saveButton, saveRadius, saveRadius},
                                     brush);
        PCWSTR save = m_editingId ? Tr(L"Save", L"Salvar")
                                  : Tr(L"Add", L"Adicionar");
        brush->SetColor(AccentInk(1));
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        target->DrawText(save, (UINT32)wcslen(save), m_textFormat.get(),
                         m_saveButton, brush);
        // "Cancel", gray.
        const float cancelRadius =
            (m_cancelButton.bottom - m_cancelButton.top) / 2;
        brush->SetColor(Fg(
            0.1f + 0.08f * Hover(m_cancelButton, m_hover.part == Part::Cancel)));
        GlassFill(target, {m_cancelButton, cancelRadius, cancelRadius}, brush);
        PCWSTR cancel = Tr(L"Cancel", L"Cancelar");
        brush->SetColor(Fg(0.9f));
        target->DrawText(cancel, (UINT32)wcslen(cancel), m_textFormat.get(),
                         m_cancelButton, brush);
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        if (!m_statusText.empty()) {
            brush->SetColor(D2D1::ColorF(1.0f, 0.55f, 0.5f, 1));
            target->DrawText(m_statusText.c_str(),
                             (UINT32)m_statusText.size(), m_smallFormat.get(),
                             m_status, brush);
        } else {
            PCWSTR bell =
                m_newRemind
                    ? Tr(L"In the pill with a sound, even in Do not disturb",
                         L"Na pílula com som, também no Não incomodar")
                    : Tr(L"No reminder (the bell turns it on)",
                         L"Sem lembrete (o sino liga)");
            brush->SetColor(Fg(0.5f));
            target->DrawText(bell, (UINT32)wcslen(bell), m_smallFormat.get(),
                             m_status, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
        target->SetTransform(base);
        if (formLayer) {
            target->PopLayer();
        }
    }
}

// The agendas' view: each agenda (its color, a click changes it; its name
// and how it's doing; its "x"), the link or path to add, and today's color.
void CalendarPanel::LayoutAgendas(float y) {
    const float scale = m_scale;
    const float padding = std::round(kPanelPadding * scale);
    // (Wider than the month, for the agendas' names.)
    const float width = std::round(
        (padding * 2 + std::round(kCalendarCell * scale) * 7) * 1.25f);
    const float row = std::round(40 * scale);
    const float gap = std::round(6 * scale);
    y += gap;
    m_agendaRows.clear();
    m_agendaRemovable.clear();
    AcquireSRWLockShared(&g_agendasLock);
    for (const auto& agenda : g_agendas) {
        m_agendaRows.push_back({padding, y, width - padding, y + row});
        m_agendaRemovable.push_back(!agenda.fromSettings);
        y += row;
    }
    const bool none = g_agendas.empty();
    ReleaseSRWLockShared(&g_agendasLock);
    // Without any: what to paste.
    m_agendaHint = {};
    if (none) {
        m_agendaHint = {padding, y, width - padding,
                        y + std::round(70 * scale)};
        y = m_agendaHint.bottom;
    }
    y += gap;
    const float field = std::round(32 * scale);
    const float add = std::round(84 * scale);
    m_linkField = {padding, y, width - padding - add - gap, y + field};
    m_linkAdd = {width - padding - add, y, width - padding, y + field};
    y += field;
    const float status = std::round(kEventRowHeight * scale * 0.8f);
    m_agendaStatus = {padding, y, width - padding, y + status};
    if (!m_agendaStatusText.empty()) {
        y += status;
    }
    y += std::round(12 * scale);
    m_todayHeader = {padding, y, width - padding,
                     y + std::round(kEventRowHeight * scale)};
    y = m_todayHeader.bottom;
    const float chip = std::round(30 * scale);
    const float chipWidth = std::floor((width - padding * 2 - gap * 2) / 3);
    for (size_t i = 0; i < m_todayChips.size(); i++) {
        const float left = padding + i * (chipWidth + gap);
        m_todayChips[i] = {left, y, left + chipWidth, y + chip};
    }
    y += chip;
    m_deleteHeader = m_countLine = m_deletePastButton = {};
    m_deleteChips = {};
    if (IslandHasMainSettings(m_owner)) {
        y += std::round(12 * scale);
        m_deleteHeader = {padding, y, width - padding,
                          y + std::round(kEventRowHeight * scale)};
        y = m_deleteHeader.bottom;
        const float deleteWidth =
            std::floor((width - padding * 2 - gap * 3) / 4);
        for (size_t i = 0; i < m_deleteChips.size(); i++) {
            const float left = padding + i * (deleteWidth + gap);
            m_deleteChips[i] = {left, y, left + deleteWidth, y + chip};
        }
        y += chip + gap;
        const float button = std::round(170 * scale);
        m_countLine = {padding, y, width - padding - button - gap, y + chip};
        m_deletePastButton = {width - padding - button, y, width - padding,
                              y + chip};
        y += chip;
    }
    y += padding;
    m_size = {(LONG)width, (LONG)std::ceil(y)};
}

void CalendarPanel::DrawAgendas(ID2D1RenderTarget* target,
                                ID2D1SolidColorBrush* brush) {
    const float scale = m_scale;
    AcquireSRWLockShared(&g_agendasLock);
    for (size_t i = 0; i < m_agendaRows.size() && i < g_agendas.size(); i++) {
        const Agenda& agenda = g_agendas[i];
        const D2D1_RECT_F& row = m_agendaRows[i];
        const float middle = (row.top + row.bottom) / 2;
        const float dot = std::round(6 * scale);
        const D2D1_POINT_2F center{row.left + std::round(10 * scale), middle};
        brush->SetColor(RgbColor(agenda.color, 1));
        target->FillEllipse({center, dot, dot}, brush);
        if (const float lit = Hover(HoverKey(5002, (long long)i),
                                    m_hover.part == Part::AgendaColor &&
                                        m_hover.value == (long long)i);
            lit > 0.01f) {
            brush->SetColor(Fg(0.5f * lit));
            target->DrawEllipse({center, dot + std::round(3 * scale),
                                 dot + std::round(3 * scale)},
                                brush, 1.5f);
        }
        const float textLeft = row.left + std::round(28 * scale);
        const float textRight = row.right - std::round(32 * scale);
        const std::wstring name =
            agenda.name.empty() ? AgendaShortName(agenda.source) : agenda.name;
        brush->SetColor(Fg(0.95f));
        target->DrawText(name.c_str(), (UINT32)name.size(), m_textFormat.get(),
                         {textLeft, row.top + std::round(3 * scale), textRight,
                          middle + std::round(2 * scale)},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        std::wstring state =
            agenda.reading && !agenda.read
                ? std::wstring(Tr(L"Reading…", L"Lendo…"))
            : agenda.notIcs
                ? std::wstring(
                      Tr(L"Not an .ics link (Google: Secret address in iCal "
                         L"format)",
                         L"Não é um link .ics (Google: Endereço secreto no "
                         L"formato iCal)"))
            : agenda.failed
                ? std::wstring(Tr(L"Couldn't read it", L"Não foi possível ler"))
            : agenda.read ? std::to_wstring(agenda.events.size()) +
                                Tr(L" events", L" eventos")
                          : std::wstring();
        if (agenda.fromSettings) {
            state += (state.empty() ? L"" : L" · ") +
                     std::wstring(Tr(L"from Windhawk", L"do Windhawk"));
        }
        brush->SetColor(agenda.failed ? D2D1::ColorF(1.0f, 0.55f, 0.5f, 1)
                                      : Fg(0.5f));
        target->DrawText(
            state.c_str(), (UINT32)state.size(), m_smallFormat.get(),
            {textLeft, middle, textRight, row.bottom - std::round(3 * scale)},
            brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        if (i < m_agendaRemovable.size() && m_agendaRemovable[i]) {
            brush->SetColor(
                Fg(0.4f + 0.55f * Hover(HoverKey(5003, (long long)i),
                                        m_hover.part == Part::AgendaRemove &&
                                            m_hover.value == (long long)i)));
            DrawGlyph(target, &kGlyphEventDelete, 1, m_iconFormat.get(),
                      {row.right - std::round(28 * scale), row.top,
                       row.right, row.bottom},
                      brush);
        }
    }
    ReleaseSRWLockShared(&g_agendasLock);
    if (m_agendaHint.bottom > m_agendaHint.top) {
        const std::wstring hint =
            std::wstring(Tr(L"No agendas yet.", L"Nenhuma agenda ainda.")) +
            L" " +
            Tr(L"Paste an .ics link (Google Calendar: Settings, your calendar, "
               L"Secret address in iCal format) or a file's path.",
               L"Cole um link .ics (Google Agenda: Configurações, a sua "
               L"agenda, Endereço secreto no formato iCal) ou o caminho "
               L"de um arquivo.");
        winrt::com_ptr<IDWriteTextLayout> layout;
        if (SUCCEEDED(m_dwrite->CreateTextLayout(
                hint.c_str(), (UINT32)hint.size(), m_smallFormat.get(),
                m_agendaHint.right - m_agendaHint.left,
                m_agendaHint.bottom - m_agendaHint.top, layout.put()))) {
            layout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
            layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            brush->SetColor(Fg(0.55f));
            target->DrawTextLayout({m_agendaHint.left, m_agendaHint.top},
                                   layout.get(), brush);
        }
    }
    DrawField(target, brush, m_linkField, m_newLink,
              Tr(L".ics link or file path", L"Link ou caminho do .ics"), true);
    const float addRadius = (m_linkAdd.bottom - m_linkAdd.top) / 2;
    brush->SetColor(
        Accent(1.0f - 0.15f * Hover(m_linkAdd, m_hover.part == Part::LinkAdd)));
    target->FillRoundedRectangle({m_linkAdd, addRadius, addRadius}, brush);
    PCWSTR add = Tr(L"Add", L"Adicionar");
    brush->SetColor(AccentInk(1));
    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    target->DrawText(add, (UINT32)wcslen(add), m_textFormat.get(), m_linkAdd,
                     brush);
    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    if (!m_agendaStatusText.empty()) {
        brush->SetColor(D2D1::ColorF(1.0f, 0.55f, 0.5f, 1));
        target->DrawText(m_agendaStatusText.c_str(),
                         (UINT32)m_agendaStatusText.size(), m_smallFormat.get(),
                         m_agendaStatus, brush);
    }
    // Today's color: red, Windows' accent color, or one chosen (a swatch on
    // each; the one in use outlined).
    brush->SetColor(Fg(0.1f));
    target->FillRectangle({m_todayHeader.left, m_todayHeader.top,
                           m_todayHeader.right, m_todayHeader.top + 1},
                          brush);
    PCWSTR header = Tr(L"Today's color", L"Cor de hoje");
    brush->SetColor(Fg(0.6f));
    target->DrawText(header, (UINT32)wcslen(header), m_smallFormat.get(),
                     m_todayHeader, brush);
    const int colors[] = {kTodayRed, m_windowsAccent, m_todayColor};
    const PCWSTR names[] = {Tr(L"Red", L"Vermelho"), Tr(L"Windows", L"Windows"),
                            Tr(L"Choose", L"Escolher")};
    for (size_t i = 0; i < m_todayChips.size(); i++) {
        const D2D1_RECT_F& chip = m_todayChips[i];
        const bool chosen = m_todayMode == (int)i;
        const float radius = (chip.bottom - chip.top) / 2;
        brush->SetColor(Fg(
            chosen ? 0.18f
                   : 0.08f + 0.08f * Hover(chip,
                                           m_hover.part == Part::TodayChoice &&
                                               m_hover.value == (long long)i)));
        GlassFill(target, {chip, radius, radius}, brush);
        if (chosen) {
            brush->SetColor(Fg(0.7f));
            target->DrawRoundedRectangle(
                {{chip.left + 0.75f, chip.top + 0.75f, chip.right - 0.75f,
                  chip.bottom - 0.75f},
                 radius,
                 radius},
                brush, 1.5f);
        }
        const float dot = std::round(5 * scale);
        brush->SetColor(RgbColor(colors[i], 1));
        target->FillEllipse(
            {{chip.left + std::round(13 * scale), (chip.top + chip.bottom) / 2},
             dot,
             dot},
            brush);
        brush->SetColor(Fg(0.9f));
        target->DrawText(names[i], (UINT32)wcslen(names[i]),
                         m_smallFormat.get(),
                         {chip.left + std::round(22 * scale), chip.top,
                          chip.right - std::round(4 * scale), chip.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    // Past events hidden after: never, a day, a week or thirty days (the one
    // in use outlined); the events' count, and "Delete past events".
    if (m_deleteHeader.bottom <= m_deleteHeader.top) {
        return;
    }
    brush->SetColor(Fg(0.1f));
    target->FillRectangle({m_deleteHeader.left, m_deleteHeader.top,
                           m_deleteHeader.right, m_deleteHeader.top + 1},
                          brush);
    PCWSTR deleteHeader =
        Tr(L"Hide past events after", L"Ocultar eventos passados depois de");
    brush->SetColor(Fg(0.6f));
    target->DrawText(deleteHeader, (UINT32)wcslen(deleteHeader),
                     m_smallFormat.get(), m_deleteHeader, brush,
                     D2D1_DRAW_TEXT_OPTIONS_CLIP);
    const PCWSTR waits[] = {Tr(L"Never", L"Nunca"), Tr(L"1 day", L"1 dia"),
                            Tr(L"7 days", L"7 dias"),
                            Tr(L"30 days", L"30 dias")};
    m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    for (size_t i = 0; i < m_deleteChips.size(); i++) {
        const D2D1_RECT_F& chip = m_deleteChips[i];
        const bool chosen = g_hidePastAfter == (int)i;
        const float radius = (chip.bottom - chip.top) / 2;
        brush->SetColor(Fg(
            chosen ? 0.18f
                   : 0.08f + 0.08f * Hover(chip,
                                           m_hover.part == Part::DeleteChoice &&
                                               m_hover.value == (long long)i)));
        GlassFill(target, {chip, radius, radius}, brush);
        if (chosen) {
            brush->SetColor(Fg(0.7f));
            target->DrawRoundedRectangle(
                {{chip.left + 0.75f, chip.top + 0.75f, chip.right - 0.75f,
                  chip.bottom - 0.75f},
                 radius,
                 radius},
                brush, 1.5f);
        }
        brush->SetColor(Fg(0.9f));
        target->DrawText(waits[i], (UINT32)wcslen(waits[i]),
                         m_smallFormat.get(), chip, brush,
                         D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    // "Delete 12 past events?" while it waits for the second press.
    const size_t past = PastEventCount();
    const bool armed = NowSeconds() - m_deletePastArmedAt < 4 && past > 0;
    {
        const D2D1_RECT_F& button = m_deletePastButton;
        const float radius = (button.bottom - button.top) / 2;
        const float opacity = past > 0 ? 1.0f : 0.4f;
        brush->SetColor(
            armed ? D2D1::ColorF(1.0f, 0.27f, 0.23f, 0.9f)
                  : Fg((0.08f + 0.08f * Hover(button, m_hover.part ==
                                                          Part::DeletePast)) *
                       opacity));
        GlassFill(target, {button, radius, radius}, brush);
        WCHAR label[64];
        if (armed) {
            swprintf_s(label, Tr(L"Delete %zu past events?",
                                 L"Apagar %zu eventos passados?"),
                       past);
        } else {
            wcscpy_s(label, Tr(L"Delete past events", L"Apagar eventos passados"));
        }
        brush->SetColor(armed ? D2D1::ColorF(1, 1, 1, 1) : Fg(0.9f * opacity));
        target->DrawText(label, (UINT32)wcslen(label), m_smallFormat.get(),
                         button, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    // "180 of 200 events", in orange once it's almost full.
    WCHAR count[96];
    swprintf_s(count, Tr(L"%zu of %zu events", L"%zu de %zu eventos"),
               g_events.size(), kMaxEvents);
    std::wstring countText = count;
    const bool almostFull = g_events.size() + 20 >= kMaxEvents;
    if (almostFull) {
        countText += Tr(L" · almost full", L" · quase cheio");
    }
    brush->SetColor(almostFull ? D2D1::ColorF(1.0f, 0.62f, 0.04f, 1)
                               : Fg(0.5f));
    target->DrawText(countText.c_str(), (UINT32)countText.size(),
                     m_smallFormat.get(), m_countLine, brush,
                     D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

// A link or a path typed (or pasted) added, and read.
void CalendarPanel::AddAgenda() {
    const std::wstring source = AgendaSource(m_newLink);
    m_agendaStatusText.clear();
    if (source.empty()) {
        m_agendaStatusText = Tr(L"Only https links or file paths",
                                L"Só links https ou caminhos de arquivos");
    } else {
        AcquireSRWLockExclusive(&g_agendasLock);
        const bool known = std::any_of(
            g_agendas.begin(), g_agendas.end(),
            [&](const Agenda& agenda) { return agenda.source == source; });
        const bool full = g_agendas.size() >= kMaxAgendas;
        if (!known && !full) {
            Agenda agenda;
            agenda.source = source;
            agenda.color = NextAgendaColor(g_agendas);
            // (Before the settings' ones, as they're read.)
            g_agendas.insert(std::find_if(g_agendas.begin(), g_agendas.end(),
                                          [](const Agenda& one) {
                                              return one.fromSettings;
                                          }),
                             std::move(agenda));
        }
        ReleaseSRWLockExclusive(&g_agendasLock);
        if (known) {
            m_agendaStatusText = Tr(L"Already added", L"Já adicionada");
        } else if (full) {
            m_agendaStatusText = Tr(L"Too many agendas", L"Agendas demais");
        } else {
            m_newLink.clear();
            SaveAgendas();
            RequestAgendas(m_hwnd);
        }
    }
    Layout();
    StartIslandAnimation();
}

void CalendarPanel::RemoveAgenda(size_t index) {
    AcquireSRWLockExclusive(&g_agendasLock);
    if (index < g_agendas.size() && !g_agendas[index].fromSettings) {
        g_agendas.erase(g_agendas.begin() + index);
        IndexAgendaDays();
    }
    ReleaseSRWLockExclusive(&g_agendasLock);
    SaveAgendas();
    Layout();
    StartIslandAnimation();
}

D2D1_COLOR_F CalendarPanel::TodayColor() const {
    return m_todayMode == 1   ? RgbColor(m_windowsAccent, 1)
           : m_todayMode == 2 ? RgbColor(m_todayColor, 1)
                              : D2D1::ColorF(1.0f, 0.27f, 0.23f, 1);
}

void CalendarPanel::SetTodayMode(int mode) {
    m_todayMode = std::clamp(mode, 0, 2);
    if (m_todayMode == 1) {
        m_windowsAccent = WindowsAccentColor();
    }
    Wh_SetIntValue(L"calendarToday", m_todayMode);
}

void CalendarPanel::DrawUpcoming(ID2D1RenderTarget* target,
                                 ID2D1SolidColorBrush* brush) {
    const float scale = m_scale;
    const float line = std::round(22 * scale);
    SYSTEMTIME now;
    GetLocalTime(&now);
    // A list: a line over it, its title, then a row each, the date (in the
    // agenda's color, or the holidays' green; with the year when it isn't
    // this one, like a yearly event already past this year) and what it is.
    struct Row {
        SYSTEMTIME date;
        int color;
        std::wstring text;
    };
    auto list = [&](const D2D1_RECT_F& header, PCWSTR title,
                    const std::vector<Row>& rows) {
        if (rows.empty()) {
            return;
        }
        brush->SetColor(Fg(0.1f));
        target->FillRectangle(
            {header.left, header.top, header.right, header.top + 1}, brush);
        brush->SetColor(Fg(0.6f));
        target->DrawText(title, (UINT32)wcslen(title), m_smallFormat.get(),
                         header, brush);
        bool otherYear = false;
        for (const auto& row : rows) {
            otherYear = otherYear || row.date.wYear != now.wYear;
        }
        // The dates, and their column as wide as the widest needs (longer
        // in some languages, like "11月20日").
        const std::wstring monthDay = MonthDayPicture(true);
        std::vector<std::wstring> dates;
        float dateWidth = std::round((otherYear ? 80 : 46) * scale);
        for (const auto& row : rows) {
            WCHAR date[32] = L"";
            const std::wstring picture =
                monthDay + (row.date.wYear != now.wYear ? L" yyyy" : L"");
            GetDateFormatEx(DateLocale(), 0, &row.date, picture.c_str(), date,
                            ARRAYSIZE(date), nullptr);
            dates.push_back(date);
            winrt::com_ptr<IDWriteTextLayout> layout;
            if (SUCCEEDED(m_dwrite->CreateTextLayout(date, (UINT32)wcslen(date),
                                                     m_smallFormat.get(), 1000,
                                                     line, layout.put()))) {
                DWRITE_TEXT_METRICS metrics{};
                layout->GetMetrics(&metrics);
                dateWidth =
                    std::max(dateWidth, std::ceil(metrics.width + 8 * scale));
            }
        }
        float y = header.bottom;
        for (size_t i = 0; i < rows.size(); i++) {
            const Row& row = rows[i];
            const std::wstring& date = dates[i];
            // Under the mouse: lit (a click shows its day, see HitTest).
            if (m_hover.part == Part::Upcoming &&
                m_hover.value == row.date.wYear * 10000LL +
                                     row.date.wMonth * 100 + row.date.wDay) {
                const float radius = std::round(6 * scale);
                brush->SetColor(Fg(0.08f));
                GlassFill(target,
                          {{header.left - std::round(4 * scale), y,
                            header.right + std::round(4 * scale), y + line},
                           radius,
                           radius},
                          brush);
            }
            brush->SetColor(RgbColor(row.color, 1));
            target->DrawText(
                date.c_str(), (UINT32)date.size(), m_smallFormat.get(),
                {header.left, y, header.left + dateWidth, y + line}, brush);
            brush->SetColor(Fg(0.85f));
            target->DrawText(
                row.text.c_str(), (UINT32)row.text.size(), m_smallFormat.get(),
                {header.left + dateWidth, y, header.right, y + line}, brush,
                D2D1_DRAW_TEXT_OPTIONS_CLIP);
            y += line;
        }
    };
    std::vector<Row> events;
    for (const auto& item : m_upcomingAgenda) {
        const AgendaEvent& event = item.event;
        const std::wstring title = event.title.empty()
                                       ? std::wstring(Tr(L"Event", L"Evento"))
                                       : event.title;
        events.push_back(
            {{(WORD)event.year, (WORD)event.month, 0, (WORD)event.day},
             item.color,
             event.minute >= 0 ? EventTimeText(event.minute) + L" · " + title
                               : title});
    }
    list(m_upcomingAgendaHeader, Tr(L"Upcoming events", L"Próximos eventos"),
         events);
    std::vector<Row> holidays;
    for (const auto& holiday : m_upcoming) {
        holidays.push_back(
            {{(WORD)holiday.year, (WORD)holiday.month, 0, (WORD)holiday.day},
             0x33C759,
             holiday.name});
    }
    list(m_upcomingHeader, Tr(L"Upcoming holidays", L"Próximos feriados"),
         holidays);
}

void CalendarPanel::DrawMonth(ID2D1RenderTarget* target,
                              ID2D1SolidColorBrush* brush,
                              int year,
                              int month,
                              float offset,
                              float opacity) {
    const float scale = m_scale;
    const float padding = std::round(kPanelPadding * scale);
    const float cell = std::round(kCalendarCell * scale);
    const float top = padding + std::round(kCalendarClockHeight * scale);
    const float titleHeight = cell * 1.2f;

    // Title ("Outubro de 2026").
    const std::wstring title = YearMonthTitle(year, month);
    brush->SetColor(Fg(0.95f * opacity));
    target->DrawText(title.c_str(), (UINT32)title.size(), m_titleFormat.get(),
                     {m_title.left + offset, m_title.top, m_title.right + offset,
                      m_title.bottom},
                     brush);

    WCHAR firstDayText[4] = L"6";
    GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
                    firstDayText, ARRAYSIZE(firstDayText));
    const int firstDay = (_wtoi(firstDayText) + 1) % 7;

    // Six weeks of days: today in a circle (red like on macOS, or the color
    // chosen), the chosen day in a ring, a dot under days with events.
    SYSTEMTIME today;
    GetLocalTime(&today);
    D2D1_COLOR_F todayFill = TodayColor();
    // White on it, or dark on a light color.
    const float todayInk =
        Luminance(todayFill.r, todayFill.g, todayFill.b) > 0.62f ? 0.08f : 1.0f;
    todayFill.a = opacity;
    const int lead = (DayOfWeek(year, month, 1) - firstDay + 7) % 7;
    const int days = DaysInMonth(year, month);
    const int previousMonth = month == 1 ? 12 : month - 1;
    const int previousDays =
        DaysInMonth(month == 1 ? year - 1 : year, previousMonth);
    const float gridTop = top + titleHeight + cell * 0.8f;
    for (int index = 0; index < 42; index++) {
        int day = index - lead + 1;
        bool inMonth = true;
        if (day < 1) {
            day += previousDays;
            inMonth = false;
        } else if (day > days) {
            day -= days;
            inMonth = false;
        }
        const D2D1_RECT_F rect{padding + (index % 7) * cell + offset,
                               gridTop + (index / 7) * cell,
                               padding + (index % 7 + 1) * cell + offset,
                               gridTop + (index / 7 + 1) * cell};
        const D2D1_POINT_2F center{(rect.left + rect.right) / 2,
                                   (rect.top + rect.bottom) / 2};
        const float circle = cell * 0.4f;
        const bool isToday = inMonth && day == today.wDay &&
                             month == today.wMonth && year == today.wYear;
        const bool selected = inMonth && day == m_selectedDay &&
                              month == m_selectedMonth &&
                              year == m_selectedYear;
        const bool hover = inMonth && m_hover.part == Part::Day &&
                           m_hover.value == day && year == m_year &&
                           month == m_month;
        if (isToday) {
            brush->SetColor(todayFill);
            target->FillEllipse({center, circle, circle}, brush);
        } else if (const float lit = Hover(
                       HoverKey(5001, year * 10000 + month * 100 + index),
                       hover);
                   lit > 0.01f) {
            brush->SetColor(Fg(0.1f * opacity * lit));
            GlassFillEllipse(target, {center, circle, circle}, brush);
        }
        if (selected && !isToday) {
            brush->SetColor(Fg(0.6f * opacity));
            target->DrawEllipse({center, circle, circle}, brush, 1.5f);
        }
        const std::wstring text = std::to_wstring(day);
        // On today's circle, its ink; a day with agenda events in its
        // agenda's color, a holiday green; the theme's color elsewhere.
        int agendaColor = 0;
        const bool agendaDay = inMonth && !isToday &&
                               AgendaDayColor(year, month, day, &agendaColor);
        const bool holiday = inMonth && !isToday && !agendaDay &&
                             !HolidaysOn(year, month, day).empty();
        brush->SetColor(
            isToday     ? D2D1::ColorF(todayInk, todayInk, todayInk, opacity)
            : agendaDay ? RgbColor(agendaColor, opacity)
            : holiday   ? D2D1::ColorF(0.2f, 0.78f, 0.35f, opacity)
                        : Fg((inMonth ? 0.9f : 0.3f) * opacity));
        target->DrawText(text.c_str(), (UINT32)text.size(), m_dayFormat.get(),
                         rect, brush);
        if (inMonth && HasEvents(year, month, day)) {
            const float dot = 1.8f * scale;
            brush->SetColor(
                isToday ? D2D1::ColorF(todayInk, todayInk, todayInk, opacity)
                        : D2D1::ColorF(1.0f, 0.27f, 0.23f, opacity));
            target->FillEllipse(
                {{center.x, center.y + circle - 3 * scale}, dot, dot}, brush);
        }
    }
}

bool CalendarPanel::OnMouseMove(POINT pt) {
    const Hit hover = HitTest(pt);
    if (hover == m_hover) {
        return false;
    }
    m_hover = hover;
    return true;
}

bool CalendarPanel::OnMouseLeave() {
    if (m_hover.part == Part::None) {
        return false;
    }
    m_hover = {};
    return true;
}

void CalendarPanel::SaveNewEvent() {
    const int minute = m_newAllDay ? -1 : m_newHour * 60 + m_newMinute;
    auto edited = std::find_if(
        g_events.begin(), g_events.end(),
        [&](const CalendarEvent& e) { return m_editingId && e.id == m_editingId; });
    if (edited == g_events.end() && g_events.size() >= kMaxEvents) {
        m_statusText = Tr(L"Calendar full: delete past events in the gear",
                          L"Calendário cheio: apague os eventos passados na "
                          L"engrenagem");
        Layout();
        return;
    }
    CalendarEvent event;
    if (edited != g_events.end()) {
        event = *edited;
    } else {
        FILETIME now;
        GetSystemTimeAsFileTime(&now);
        event.id = ((long long)now.dwHighDateTime << 32) | now.dwLowDateTime;
        event.year = m_selectedYear;
        event.month = m_selectedMonth;
        event.day = m_selectedDay;
    }
    event.minute = minute;
    event.title = m_newTitle.empty()
                      ? std::wstring(Tr(L"Event", L"Evento"))
                      : m_newTitle;
    event.remind = m_newRemind;
    // A reminder for a time already gone isn't due; moved later, it is again.
    SYSTEMTIME local;
    GetLocalTime(&local);
    event.fired =
        LocalMinutes(event.year, event.month, event.day,
                     minute >= 0 ? minute : kAllDayReminderMinute) <=
        LocalMinutes(local.wYear, local.wMonth, local.wDay,
                     local.wHour * 60 + local.wMinute);
    if (edited != g_events.end()) {
        *edited = std::move(event);
    } else {
        g_events.push_back(std::move(event));
    }
    SaveEvents();
    StopEditing();
}

bool CalendarPanel::OnMouseUp(POINT pt) {
    const Hit hit = HitTest(pt);
    switch (hit.part) {
        case Part::BigAlarms:
        case Part::BigTimer:
            // (3: the alarms' tab, 0: the timer's.)
            OpenTimerTabOf(Owner(), hit.part == Part::BigAlarms ? 3 : 0);
            return false;

        case Part::Previous:
            ChangeMonth(-1);
            return true;
        case Part::Next:
            ChangeMonth(1);
            return true;
        case Part::Title: {
            // The title goes back to this month (and today).
            SYSTEMTIME today;
            GetLocalTime(&today);
            const int delta =
                (today.wYear - m_year) * 12 + (today.wMonth - m_month);
            if (delta) {
                ChangeMonth(delta);
            }
            m_selectedYear = today.wYear;
            m_selectedMonth = today.wMonth;
            m_selectedDay = today.wDay;
            Layout();
            return true;
        }
        case Part::Day:
            m_selectedYear = m_year;
            m_selectedMonth = m_month;
            m_selectedDay = (int)hit.value;
            Layout();
            return true;
        case Part::Upcoming: {
            // Its month shown, its day chosen (with its events under it).
            const int year = (int)(hit.value / 10000);
            const int month = (int)(hit.value / 100 % 100);
            const int delta = (year - m_year) * 12 + (month - m_month);
            if (delta) {
                ChangeMonth(delta);
            }
            m_selectedYear = year;
            m_selectedMonth = month;
            m_selectedDay = (int)(hit.value % 100);
            Layout();
            return true;
        }
        case Part::DeleteEvent:
            if (m_adding && m_editingId == hit.value) {
                StopEditing();
            }
            std::erase_if(g_events, [&](const CalendarEvent& event) {
                return event.id == hit.value;
            });
            SaveEvents();
            Layout();
            return true;
        case Part::EventRow:
            for (const auto& event : g_events) {
                if (event.id == hit.value) {
                    StartEditing(&event);
                    break;
                }
            }
            return true;
        case Part::Add:
            StartEditing(nullptr);
            return true;
        case Part::TitleField:
            m_focus = Field::Title;
            return true;
        case Part::Hour:
        case Part::Minute:
            if (hit.value) {
                StepTime(hit.part, (int)hit.value);
            } else {
                m_newAllDay = false;
            }
            m_focus = hit.part == Part::Hour ? Field::Hour : Field::Minute;
            m_digits = 0;
            return true;
        case Part::Cancel:
            StopEditing();
            return true;
        case Part::AllDay:
            m_newAllDay = !m_newAllDay;
            return true;
        case Part::Remind:
            m_newRemind = !m_newRemind;
            return true;
        case Part::Save:
            SaveNewEvent();
            return true;
        // (Today big's gear: the same view, back to the card from there.)
        case Part::BigGear:
        case Part::Agendas:
            if (m_adding) {
                StopEditing();
            }
            m_agendasView = !m_agendasView;
            m_agendaStatusText.clear();
            g_lastTyped = NowSeconds();
            Layout();
            StartIslandAnimation();
            return true;
        case Part::AgendaColor: {
            std::wstring source;
            int color = 0;
            AcquireSRWLockShared(&g_agendasLock);
            if ((size_t)hit.value < g_agendas.size()) {
                source = g_agendas[hit.value].source;
                color = g_agendas[hit.value].color;
            }
            ReleaseSRWLockShared(&g_agendasLock);
            if (!source.empty()) {
                StartColorDialog(source, color);
            }
            return true;
        }
        case Part::AgendaRemove:
            RemoveAgenda((size_t)hit.value);
            return true;
        case Part::LinkField:
            g_lastTyped = NowSeconds();
            StartIslandAnimation();
            return true;
        case Part::LinkAdd:
            AddAgenda();
            return true;
        case Part::TodayChoice:
            SetTodayMode((int)hit.value);
            if (hit.value == 2) {
                StartColorDialog(L"", m_todayColor);
            }
            return true;
        case Part::DeleteChoice:
            SetHidePastAfter((int)hit.value);
            m_eventsDropped = g_eventsDropped;
            Layout();
            return true;
        case Part::DeletePast:
            // (Pressed again within a few seconds: deleted.)
            if (PastEventCount() == 0) {
                return true;
            }
            if (NowSeconds() - m_deletePastArmedAt >= 4) {
                m_deletePastArmedAt = NowSeconds();
                Render();
                return true;
            }
            m_deletePastArmedAt = -100;
            DeletePastEvents();
            m_eventsDropped = g_eventsDropped;
            Layout();
            return true;
        case Part::ShowHidden:
            m_showHiddenFor = SelectedDayKey();
            Layout();
            return true;
        case Part::None:
            break;
    }
    return false;
}

// By one, around the clock.
void CalendarPanel::StepTime(Part part, int steps) {
    m_digits = 0;
    if (m_newAllDay) {
        m_newAllDay = false;
        return;
    }
    if (part == Part::Hour) {
        m_newHour = ((m_newHour + steps) % 24 + 24) % 24;
    } else {
        m_newMinute = ((m_newMinute + steps) % 60 + 60) % 60;
    }
}

bool CalendarPanel::OnWheel(int delta) {
    if (m_agendasView) {
        return false;
    }
    // Over the hour or the minute: changes it.
    if (m_adding &&
        (m_hover.part == Part::Hour || m_hover.part == Part::Minute)) {
        StepTime(m_hover.part, delta > 0 ? 1 : -1);
        return true;
    }
    ChangeMonth(delta > 0 ? -1 : 1);
    return true;
}

bool CalendarPanel::OnKey(WPARAM key) {
    // Escape goes back from the agendas to the month.
    if (m_agendasView) {
        if (key != VK_ESCAPE) {
            return false;
        }
        m_agendasView = false;
        m_agendaStatusText.clear();
        Layout();
        StartIslandAnimation();
        return true;
    }
    if (m_adding) {
        // Escape leaves the event; the arrows change the hour or the minute.
        if (key == VK_ESCAPE) {
            StopEditing();
            return true;
        }
        if ((key == VK_UP || key == VK_DOWN) && m_focus != Field::Title) {
            StepTime(m_focus == Field::Hour ? Part::Hour : Part::Minute,
                     key == VK_UP ? 1 : -1);
            return true;
        }
        return false;
    }
    if (key == VK_LEFT || key == VK_PRIOR) {
        ChangeMonth(-1);
        return true;
    }
    if (key == VK_RIGHT || key == VK_NEXT) {
        ChangeMonth(1);
        return true;
    }
    return false;
}

// Typing the event's title, or its time; Tab goes to the next field, Enter
// adds it. In the agendas' view, a link or a path.
bool CalendarPanel::OnChar(WCHAR c) {
    if (m_agendasView) {
        std::wstring& text = m_newLink;
        if (c == L'\r') {
            AddAgenda();
            return true;
        }
        if (c == L'\b') {
            if (!text.empty()) {
                text.pop_back();
            }
        } else if (c == 0x16) {
            PasteText(m_hwnd, &text, kMaxAgendaSource);
        } else if (c >= 0x20 && text.size() < kMaxAgendaSource) {
            text += c;
        } else {
            return false;
        }
        if (!m_agendaStatusText.empty()) {
            m_agendaStatusText.clear();
            Layout();
        }
        return true;
    }
    if (!m_adding) {
        return false;
    }
    if (c == L'\t') {
        const bool back = GetKeyState(VK_SHIFT) < 0;
        m_focus = m_focus == Field::Title  ? (back ? Field::Minute : Field::Hour)
                  : m_focus == Field::Hour ? (back ? Field::Title : Field::Minute)
                                           : (back ? Field::Hour : Field::Title);
        m_digits = 0;
        return true;
    }
    if (m_focus != Field::Title && c != L'\r') {
        if (c >= L'0' && c <= L'9') {
            TypeDigit(c - L'0');
        } else if (c == L'\b') {
            // Back to the hour from an empty minute.
            if (m_focus == Field::Minute && m_digits == 0 && m_newMinute == 0) {
                m_focus = Field::Hour;
            } else if (m_focus == Field::Hour) {
                m_newHour /= 10;
            } else {
                m_newMinute /= 10;
            }
            m_digits = 0;
        } else if (c == L':' || c == L'h' || c == L'H') {
            m_focus = Field::Minute;
            m_digits = 0;
        } else {
            return false;
        }
        return true;
    }
    std::wstring& text = m_newTitle;
    if (c == L'\r') {
        SaveNewEvent();
    } else if (c == L'\b') {
        if (!text.empty()) {
            text.pop_back();
        }
    } else if (c == 0x16) {
        PasteText(m_hwnd, &text, 120);
    } else if (c >= 0x20 && text.size() < 120) {
        text += c;
    } else {
        return false;
    }
    m_statusText.clear();
    Layout();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// The control center, like on macOS: tiles for the network, Bluetooth, "Do
// not disturb" and the settings, the volume as a big slider, and the battery.
// The sound has two more screens, like the Windows quick settings: the output
// device, and the volume of each app.

// Windows' own interface for the default audio device. It isn't documented,
// but it has been stable since Windows 7, and the volume tools use it.
struct IPolicyConfig : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR, void**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR, INT, void**) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR, void*, void*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR, INT, PINT64,
                                                          PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR, void*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR, void*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR,
                                                       const PROPERTYKEY&,
                                                       PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR,
                                                       const PROPERTYKEY&,
                                                       PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR deviceId,
                                                         ERole role) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR, INT) = 0;
};

constexpr GUID kCLSID_PolicyConfig = {
    0x870af99c,
    0x171d,
    0x4f9e,
    {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}};
constexpr GUID kIID_IPolicyConfig = {
    0xf8679f50,
    0x850a,
    0x41cf,
    {0x9c, 0x72, 0x43, 0x0f, 0x29, 0x02, 0x90, 0xc8}};
constexpr GUID kIID_IAudioSessionManager2 = {
    0x77aa99a0,
    0x1bd6,
    0x484f,
    {0x8b, 0xc7, 0x2c, 0x65, 0x4c, 0x9a, 0x9b, 0x6f}};
constexpr PROPERTYKEY kPKEY_AudioEndpoint_FormFactor = {
    {0x1da5d803,
     0xd492,
     0x4edd,
     {0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e}},
    0};

constexpr WCHAR kGlyphHeadphones = 0xE7F6;
constexpr WCHAR kGlyphSpeakers = 0xE7F5;
constexpr WCHAR kGlyphMonitor = 0xE7F4;
constexpr WCHAR kGlyphCheck = 0xE73E;
constexpr WCHAR kGlyphBack = 0xE72B;

constexpr WCHAR kGlyphLock = 0xE72E;
constexpr WCHAR kGlyphEdit = 0xE70F;
constexpr WCHAR kGlyphClose = 0xE711;
constexpr WCHAR kGlyphBrightness = 0xE706;
constexpr WCHAR kGlyphNightLight = 0xF08C;
constexpr WCHAR kGlyphProject = 0xEBC6;
constexpr WCHAR kGlyphHotspot = 0xE88A;
constexpr WCHAR kGlyphAirplane = 0xE709;
constexpr WCHAR kGlyphMagnifier = 0xE71E;
constexpr WCHAR kGlyphNarrator = 0xEBC5;
constexpr WCHAR kGlyphColorFilters = 0xE790;
constexpr WCHAR kGlyphShow = 0xE7B3;
constexpr WCHAR kGlyphHide = 0xED1A;
constexpr WCHAR kGlyphKeyboard = 0xE765;
constexpr WCHAR kGlyphMouse = 0xE962;

// The controls the control center can show: big tiles (two per row), small
// buttons (four per row) and full-width sections. Which ones, and their
// order, are chosen in it ("Edit").
enum class ControlId {
    Network,
    Bluetooth,
    DoNotDisturb,
    NightLight,
    Hotspot,
    Airplane,
    Project,
    Cast,
    Captions,
    Accessibility,
    DarkMode,
    Screenshot,
    Lock,
    Emoji,
    Clipboard,
    TaskManager,
    Calculator,
    Explorer,
    Display,
    Sound,
    Battery,
    PowerMode,
    Microphone,
    Notes,
    Tasks,
    Shortcuts,
    Weather,
    Updates,
    Komorebi,
    ShowDesktop,
    OnScreenKeyboard,
    // The pill's items that open something, as buttons too (added in 2.1,
    // not shown until added; see ControlItem).
    Notifications,
    Calendar,
    Media,
    Power,
    Usage,
    Profiles,
    KeyboardLanguage,
    Privacy,
    Drives,
    Desktops,
    Folders,
    Wallpaper,
    KomorebiLayout,
    WorldClocks,
    NextEvent,
    RecycleBin,
    Timer,
    TrayIcons,
    // The timer panel's other tabs, like the iPhone's Control Center's
    // (added in 2.1 too, not shown until added).
    Stopwatch,
    Pomodoro,
    Alarms,
};

enum class ControlKind { Tile, Button, Section };

struct ControlInfo {
    ControlId id;
    ControlKind kind;
    WCHAR glyph;
    PCWSTR english;
    PCWSTR portuguese;
    // Remembered by this name.
    PCWSTR key;
    bool shownByDefault;
};

const ControlInfo kControls[] = {
    {ControlId::Network, ControlKind::Tile, 0xE701, L"Wi-Fi / network",
     L"Wi-Fi / rede", L"network", true},
    {ControlId::Bluetooth, ControlKind::Tile, 0xE702, L"Bluetooth",
     L"Bluetooth", L"bluetooth", true},
    {ControlId::DoNotDisturb, ControlKind::Tile, 0xE708, L"Do not disturb",
     L"Não incomodar", L"dnd", true},
    {ControlId::NightLight, ControlKind::Tile, 0xF08C, L"Night light",
     L"Luz noturna", L"nightLight", true},
    {ControlId::Hotspot, ControlKind::Tile, 0xE88A, L"Mobile hotspot",
     L"Hotspot móvel", L"hotspot", false},
    {ControlId::Airplane, ControlKind::Tile, 0xE709, L"Airplane mode",
     L"Modo avião", L"airplane", false},
    {ControlId::Project, ControlKind::Button, 0xEBC6, L"Project", L"Projetar",
     L"project", true},
    {ControlId::Cast, ControlKind::Button, 0xEC15, L"Cast", L"Transmitir",
     L"cast", true},
    {ControlId::Captions, ControlKind::Button, 0xE7F0, L"Captions",
     L"Legendas", L"captions", true},
    {ControlId::Accessibility, ControlKind::Button, 0xE776, L"Accessibility",
     L"Acessibilidade", L"accessibility", true},
    {ControlId::DarkMode, ControlKind::Button, 0xE793, L"Dark mode",
     L"Modo escuro", L"darkMode", false},
    {ControlId::Screenshot, ControlKind::Button, 0xE722, L"Screenshot",
     L"Captura de tela", L"screenshot", false},
    {ControlId::Lock, ControlKind::Button, 0xE72E, L"Lock", L"Bloquear",
     L"lock", false},
    {ControlId::Emoji, ControlKind::Button, 0xE76E, L"Emoji", L"Emojis",
     L"emoji", false},
    {ControlId::Clipboard, ControlKind::Button, 0xF0E3, L"Clipboard",
     L"Área de transferência", L"clipboard", false},
    {ControlId::TaskManager, ControlKind::Button, 0xE9D9, L"Task Manager",
     L"Gerenciador de Tarefas", L"taskManager", false},
    {ControlId::Calculator, ControlKind::Button, 0xE8EF, L"Calculator",
     L"Calculadora", L"calculator", false},
    {ControlId::Explorer, ControlKind::Button, 0xEC50, L"File Explorer",
     L"Explorador de Arquivos", L"explorer", false},
    {ControlId::ShowDesktop, ControlKind::Button, 0xE7F4, L"Show desktop",
     L"Mostrar área de trabalho", L"showDesktop", false},
    {ControlId::OnScreenKeyboard, ControlKind::Button, 0xE765,
     L"On-screen keyboard", L"Teclado virtual", L"onScreenKeyboard", false},
    {ControlId::Microphone, ControlKind::Button, 0xE720, L"Microphone",
     L"Microfone", L"microphone", false},
    {ControlId::Notes, ControlKind::Button, 0xE70B, L"Notes", L"Anotações",
     L"notes", false},
    {ControlId::Tasks, ControlKind::Button, 0xE9D5, L"Tasks", L"Tarefas",
     L"tasks", false},
    {ControlId::Shortcuts, ControlKind::Button, 0xE765, L"Shortcuts",
     L"Atalhos", L"shortcuts", false},
    {ControlId::Weather, ControlKind::Button, 0xE753, L"Weather", L"Clima",
     L"weather", false},
    {ControlId::Updates, ControlKind::Button, 0xE895, L"Updates",
     L"Atualizações", L"updates", false},
    {ControlId::Notifications, ControlKind::Button, 0xEA8F, L"Notifications",
     L"Notificações", L"notifications", false},
    {ControlId::Calendar, ControlKind::Button, 0xE787, L"Calendar",
     L"Calendário", L"calendar", false},
    {ControlId::Media, ControlKind::Button, 0xE8D6, L"What's playing",
     L"O que está tocando", L"media", false},
    {ControlId::Power, ControlKind::Button, 0xE7E8, L"Power button",
     L"Botão de desligar", L"power", false},
    {ControlId::Usage, ControlKind::Button, 0xE9D9, L"CPU, RAM and VRAM use",
     L"Uso de CPU, RAM e VRAM", L"usage", false},
    {ControlId::Profiles, ControlKind::Button, 0xE748, L"Profiles", L"Perfis",
     L"profiles", false},
    {ControlId::KeyboardLanguage, ControlKind::Button, 0xE774,
     L"Keyboard language", L"Idioma do teclado", L"keyboardLanguage", false},
    {ControlId::Privacy, ControlKind::Button, 0xEA18, L"Privacy",
     L"Privacidade", L"privacy", false},
    {ControlId::Drives, ControlKind::Button, 0xE88E, L"USB drives",
     L"Pendrives", L"drives", false},
    {ControlId::Desktops, ControlKind::Button, 0xE7C4, L"Virtual desktops",
     L"Áreas de trabalho virtuais", L"desktops", false},
    {ControlId::Folders, ControlKind::Button, 0xE8B7, L"Folders and places",
     L"Pastas e locais", L"folders", false},
    {ControlId::Wallpaper, ControlKind::Button, 0xE8B9, L"Wallpaper",
     L"Papel de parede", L"wallpaper", false},
    {ControlId::KomorebiLayout, ControlKind::Button, kGlyphKomorebiLayout,
     L"komorebi's layout", L"Layout do komorebi", L"komorebiLayout", false},
    {ControlId::WorldClocks, ControlKind::Button, 0xE909, L"World clocks",
     L"Relógios mundiais", L"worldClocks", false},
    {ControlId::NextEvent, ControlKind::Button, 0xE916, L"Next event",
     L"Próximo evento", L"nextEvent", false},
    {ControlId::RecycleBin, ControlKind::Button, 0xE74D, L"Recycle Bin",
     L"Lixeira", L"recycleBin", false},
    {ControlId::Timer, ControlKind::Button, 0xE916, L"Timer", L"Timer",
     L"timer", false},
    {ControlId::TrayIcons, ControlKind::Button, 0xE712,
     L"Other apps' tray icons", L"Ícones dos outros apps", L"trayIcons",
     false},
    {ControlId::Stopwatch, ControlKind::Button, 0xE916, L"Stopwatch",
     L"Cronômetro", L"stopwatch", false},
    {ControlId::Pomodoro, ControlKind::Button, 0xE916, L"Pomodoro",
     L"Pomodoro", L"pomodoro", false},
    {ControlId::Alarms, ControlKind::Button, 0xE823, L"Alarms", L"Alarmes",
     L"alarms", false},
    {ControlId::Display, ControlKind::Section, 0xE706, L"Display brightness",
     L"Brilho da tela", L"display", true},
    {ControlId::Sound, ControlKind::Section, 0xE767, L"Sound", L"Som",
     L"sound", true},
    {ControlId::Battery, ControlKind::Section, 0xE83F, L"Battery",
     L"Bateria", L"battery", true},
    {ControlId::PowerMode, ControlKind::Section, 0xEC48, L"Power",
     L"Energia", L"powermode", true},
    {ControlId::Komorebi, ControlKind::Section, kGlyphKomorebi, L"komorebi",
     L"komorebi", L"komorebi", false},
};

// komorebi (a tiling window manager, when installed): its control center
// section and its button on the pill start, stop or reload it, like yasb's
// control, with its own command line tool, only on a click: without a
// window (like yasb and the Windows 11 Start Menu Buttons mod), and again
// in a terminal on the screen when it fails, to show why. Nothing is
// installed or accepted for the user, and the mod never ends komorebi by
// itself.

// The user's environment as Windows has it now, not this process's (from
// when it started: its PATH lacks what was installed since, like komorebi
// and whkd installed with Windhawk running). Freed with
// DestroyEnvironmentBlock; null if it can't be made.
void* FreshEnvironment() {
    HANDLE token = nullptr;
    void* block = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE,
                         &token)) {
        if (!CreateEnvironmentBlock(&block, token, FALSE)) {
            block = nullptr;
        }
        CloseHandle(token);
    }
    return block;
}

// That environment's PATH (empty if it can't be read).
std::wstring FreshPath() {
    std::wstring path;
    if (void* block = FreshEnvironment()) {
        for (auto entry = (PCWSTR)block; *entry; entry += wcslen(entry) + 1) {
            if (_wcsnicmp(entry, L"Path=", 5) == 0) {
                path = entry + 5;
                break;
            }
        }
        DestroyEnvironmentBlock(block);
    }
    return path;
}

// A tool of komorebi's (komorebic.exe, whkd.exe), on PATH or where its
// installer (`folder` under Program Files) or scoop put it; empty if it
// isn't installed.
std::wstring FindKomorebiTool(PCWSTR file, PCWSTR folder) {
    WCHAR path[MAX_PATH];
    const std::wstring fresh = FreshPath();
    const DWORD found = SearchPath(fresh.empty() ? nullptr : fresh.c_str(),
                                   file, nullptr, MAX_PATH, path, nullptr);
    if (found > 0 && found < MAX_PATH) {
        return path;
    }
    // (%ProgramW6432%: the island's process is 32-bit.)
    for (const std::wstring& place :
         {L"%ProgramW6432%\\" + std::wstring(folder) + L"\\bin\\" + file,
          L"%USERPROFILE%\\scoop\\shims\\" + std::wstring(file)}) {
        if (ExpandEnvironmentStrings(place.c_str(), path, MAX_PATH) &&
            GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }
    return {};
}

std::wstring FindKomorebic() {
    return FindKomorebiTool(L"komorebic.exe", L"komorebi");
}

// Whether komorebi and whkd are installed, for the settings (-1 until
// looked for, see LookForKomorebi).
int g_komorebiFound = -1;
int g_whkdFound = -1;

void LookForKomorebi() {
    g_komorebiFound = !FindKomorebic().empty();
    g_whkdFound = !FindKomorebiTool(L"whkd.exe", L"whkd").empty();
}

// A program's process in this session (its file's name), or 0 if it doesn't
// run.
DWORD SessionProcessId(PCWSTR exe) {
    DWORD session = 0;
    ProcessIdToSessionId(GetCurrentProcessId(), &session);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    DWORD found = 0;
    PROCESSENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    for (BOOL more = Process32First(snapshot, &entry); more && !found;
         more = Process32Next(snapshot, &entry)) {
        DWORD its = 0;
        if (_wcsicmp(entry.szExeFile, exe) == 0 &&
            ProcessIdToSessionId(entry.th32ProcessID, &its) &&
            its == session) {
            found = entry.th32ProcessID;
        }
    }
    CloseHandle(snapshot);
    return found;
}

