// The timer's panel (from its item), like the macOS Clock app: tabs for the
// timer, the stopwatch, Pomodoro and the alarms (the lit one sliding, the
// page coming in from its side); a countdown's length chosen on wheels
// (hours, minutes and seconds: the mouse wheel, a drag up or down, or typed
// after a click), then a ring going round while it runs, its time inside,
// with round buttons to cancel and to pause or go on; the stopwatch with
// hundredths; the alarms as a list (each switched on or off, its sound, a
// bin), a new one's time on wheels too.
class TimerPanel : public Panel {
   public:
    // Every second while open (see Island::Tick).
    void Refresh() {
        if (IsOpen() && m_tab != kTabAlarms) {
            Render();
        }
    }
    // Told when it changed from elsewhere (it ended, see Island::Tick).
    void Changed() {
        if (IsOpen()) {
            Layout();
            Render();
        }
    }
    // On a mode's tab (its time clicked on the pill).
    void ShowTab(int tab) {
        m_activity = false;
        if (tab >= 0 && tab < kTabCount) {
            SetTab(tab, !IsOpen());
        }
    }
    // A countdown or the stopwatch going, clicked on the pill: like the
    // iPhone's Live Activities, every one going (the soonest end first, the
    // stopwatch last), each with its time, Pause and Cancel, and the next
    // alarm of the day with its switch; "Show all": the whole panel, on the
    // clicked one's tab. The whole panel once none is going.
    void ShowActivity(TimerMode mode) {
        ShowTab(mode == TimerMode::Stopwatch  ? kTabStopwatch
                : mode == TimerMode::Pomodoro ? kTabPomodoro
                                              : kTabTimer);
        m_activity = AnyTimerActive();
        if (IsOpen()) {
            Changed();
        }
    }

   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_timeFormat = MakeFormat(L"Segoe UI Variable Display",
                                      DWRITE_FONT_WEIGHT_SEMI_BOLD, 40 * scale);
            m_ringFormat = MakeFormat(L"Segoe UI Variable Display",
                                      DWRITE_FONT_WEIGHT_SEMI_BOLD, 30 * scale);
            m_wheelFormat = MakeFormat(L"Segoe UI Variable Display",
                                       DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                       24 * scale);
            m_wheelSideFormat =
                MakeFormat(L"Segoe UI Variable Display",
                           DWRITE_FONT_WEIGHT_NORMAL, 16 * scale);
            m_alarmFormat = MakeFormat(L"Segoe UI Variable Display",
                                       DWRITE_FONT_WEIGHT_NORMAL, 28 * scale,
                                       DWRITE_TEXT_ALIGNMENT_LEADING);
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11.5f * scale);
            m_leftTextFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_buttonFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 12 * scale);
            m_roundButtonFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 14 * scale);
            m_iconFormat =
                MakeFormat(IconFont(), DWRITE_FONT_WEIGHT_NORMAL, 13 * scale);
            m_bigIconFormat =
                MakeFormat(IconFont(), DWRITE_FONT_WEIGHT_NORMAL, 20 * scale);
            if (!m_tabular &&
                SUCCEEDED(m_dwrite->CreateTypography(m_tabular.put()))) {
                m_tabular->AddFontFeature(
                    {DWRITE_FONT_FEATURE_TAG_TABULAR_FIGURES, 1});
            }
        }
        Layout();
    }

    void OnOpen() override {
        m_hover = -1;
        m_typing = -1;
        m_dragWheel = -1;
        m_adding = false;
        m_naming = false;
        LoadTimerLengths();
        Layout();
        m_tabKnob = m_tab;
        m_tabKnobVelocity = 0;
        m_pageIn = 1;
        m_pageInVelocity = 0;
    }

    void OnClose() override {
        EndTyping(true);
        SaveWheelsTurned();
    }
    void OnDestroy() override { SaveWheelsTurned(); }

    bool AnimateContents(double dt) override {
        bool moving = SpringTowards(m_tabKnob, m_tabKnobVelocity, m_tab, 0.3,
                                    dt, 0.85);
        moving |= SpringTowards(m_pageIn, m_pageInVelocity, 1, 0.32, dt);
        for (size_t i = 0; i < m_wheelOffset.size(); i++) {
            moving |= SpringTowards(m_wheelOffset[i], m_wheelVelocity[i], 0,
                                    0.22, dt, 0.9);
        }
        // A countdown's ring and the stopwatch's hundredths go on smoothly
        // while they run (at a lower frame rate: see m_idleMotion).
        const bool ticking =
            m_activity ? std::any_of(m_activityModes.begin(),
                                     m_activityModes.end(),
                                     [](TimerMode mode) {
                                         return TimerClockOf(mode).running;
                                     })
                       : m_tab != kTabAlarms &&
                             TimerClockOf(TabMode(m_tab)).running;
        // (An alarm's name being typed: its caret blinks.)
        const bool blinking = m_naming && m_adding && CaretBlinking();
        m_idleMotion = (ticking || blinking) && !moving;
        return moving || ticking || blinking;
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override {
        if (!m_timeFormat || !m_ringFormat || !m_wheelFormat ||
            !m_wheelSideFormat || !m_alarmFormat || !m_textFormat ||
            !m_leftTextFormat || !m_buttonFormat || !m_roundButtonFormat ||
            !m_iconFormat || !m_bigIconFormat) {
            return;
        }
        if (m_activity) {
            DrawActivity(target, brush);
            return;
        }
        const float s = m_scale;
        DrawTabs(target, brush);
        // The page, coming in from its side.
        const float in = (float)std::clamp(m_pageIn, 0.0, 1.0);
        winrt::com_ptr<ID2D1Layer> layer;
        D2D1_MATRIX_3X2_F base;
        target->GetTransform(&base);
        if (in < 0.999f) {
            target->CreateLayer(nullptr, layer.put());
            target->PushLayer(
                D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                      D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                      D2D1::IdentityMatrix(), in),
                layer.get());
            target->SetTransform(
                D2D1::Matrix3x2F::Translation(
                    m_pageDirection * (1 - in) * std::round(24 * s), 0) *
                *D2D1::Matrix3x2F::ReinterpretBaseType(&base));
        }
        if (m_tab == kTabAlarms) {
            DrawAlarms(target, brush);
        } else {
            DrawClock(target, brush);
        }
        if (layer) {
            target->SetTransform(base);
            target->PopLayer();
        }
    }

    bool OnMouseMove(POINT pt) override {
        // Dragging a wheel: a step for each row's height up or down.
        if (m_dragWheel >= 0 && (GetKeyState(VK_LBUTTON) & 0x8000)) {
            const float row = WheelRow();
            const int steps =
                (int)std::lround((m_dragY - pt.y) / std::max(row, 1.0f));
            if (std::abs(pt.y - m_dragY) > std::round(4 * m_scale)) {
                m_dragMoved = true;
            }
            const int wanted = m_dragValue + steps;
            const int now = WheelValue(m_dragWheel);
            if (WrapWheel(m_dragWheel, wanted) != now) {
                StepWheel(m_dragWheel, WrapWheel(m_dragWheel, wanted) - now,
                          false);
            }
            return true;
        }
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

    bool OnMouseDown(POINT pt) override {
        const int hit = HitTest(pt);
        if (hit >= kWheel && hit < kWheel + kWheelCount) {
            m_dragWheel = hit - kWheel;
            m_dragY = pt.y;
            m_dragValue = WheelValue(m_dragWheel);
            m_dragMoved = false;
            SetCapture(m_hwnd);
        }
        return false;
    }

    void OnCaptureLost() override { m_dragWheel = -1; }

    bool OnMouseUp(POINT pt) override {
        // A wheel dragged: kept; clicked: its figures typed.
        if (m_dragWheel >= 0) {
            const int wheel = m_dragWheel;
            m_dragWheel = -1;
            ReleaseCapture();
            if (m_dragMoved) {
                SaveWheels();
            } else {
                EndTyping(true);
                m_typing = wheel;
                m_typed.clear();
            }
            Changed();
            return true;
        }
        int hit = HitTest(pt);
        // (The name typed in until a click elsewhere.)
        if (hit != kNameBox && m_naming) {
            m_naming = false;
            Changed();
        }
        if (hit < 0) {
            EndTyping(true);
            return true;
        }
        EndTyping(true);
        auto& alarms = Alarms();
        if (hit < kTabCount) {
            SetTab(hit, false);
            return true;
        }
        if (hit == kShowAll) {
            m_activity = false;
            m_pageIn = 0;
            m_pageInVelocity = 0;
            Changed();
            return true;
        }
        // A row of the Live Activities: its own one.
        const int row = hit >= kRowSecondary && hit < kRowSecondary + kTimerModes
                            ? hit - kRowSecondary
                        : hit >= kRowPrimary && hit < kRowPrimary + kTimerModes
                            ? hit - kRowPrimary
                            : -1;
        const TimerMode mode = row >= 0 && row < (int)m_activityModes.size()
                                   ? m_activityModes[row]
                                   : TabMode(m_tab);
        TimerClock& clock = TimerClockOf(mode);
        if (row >= 0) {
            hit = hit >= kRowSecondary ? kSecondary : kPrimary;
        }
        if (hit == kNextAlarm) {
            if (m_activityAlarm >= 0 && m_activityAlarm < (int)alarms.size()) {
                alarms[m_activityAlarm].on = !alarms[m_activityAlarm].on;
                SaveAlarms();
            }
            hit = -1;
        }
        // (The last one going canceled from the Live Activities: they
        // close, like the iPhone's.)
        bool canceled = false;
        switch (hit) {
            case kPrimary:
                if (clock.running) {
                    clock.Pause();
                } else if (TimerLength(mode) > 0 || mode == TimerMode::Stopwatch) {
                    clock.Start();
                }
                break;
            case kSecondary:
                clock.Reset();
                if (mode == TimerMode::Pomodoro) {
                    g_timer.onBreak = false;
                    g_timer.rounds = 0;
                }
                canceled = m_activity && !AnyTimerActive();
                break;
            case kAddAlarm:
                m_adding = true;
                m_editing = -1;
                m_newMinute = 7 * 60;
                m_newSound = true;
                m_newDays = kAlarmEveryDay;
                m_newName.clear();
                m_nameEdit.Reset();
                PulseContents();
                break;
            case kNameBox:
                m_naming = true;
                g_lastTyped = NowSeconds();
                EditTextClick(m_newName, m_nameEdit, pt);
                break;
            case kAmPm:
                m_newMinute = (m_newMinute + 12 * 60) % (24 * 60);
                break;
            case kSound:
                m_newSound = !m_newSound;
                break;
            case kSave:
                // (One edited: changed in its place; it stays on or off.)
                if (m_editing >= 0 && m_editing < (int)alarms.size()) {
                    alarms[m_editing].minute = m_newMinute;
                    alarms[m_editing].sound = m_newSound;
                    alarms[m_editing].days = m_newDays;
                    alarms[m_editing].name = AlarmNameTyped();
                } else if (alarms.size() < kMaxAlarms) {
                    alarms.push_back({m_newMinute, true, m_newSound, m_newDays,
                                      AlarmNameTyped()});
                }
                m_savedAlarm = m_newMinute * 2 + (m_newSound ? 1 : 0);
                std::sort(alarms.begin(), alarms.end(),
                          [](const Alarm& a, const Alarm& b) {
                              return a.minute < b.minute;
                          });
                SaveAlarms();
                m_adding = false;
                m_editing = -1;
                PulseContents();
                break;
            case kCancelAdd:
                m_adding = false;
                m_editing = -1;
                m_naming = false;
                PulseContents();
                break;
            default:
                if (hit >= kPreset && hit < kPreset + kPresetCount) {
                    g_timer.seconds = kPresets[hit - kPreset] * 60;
                    SaveTimerLengths();
                } else if (hit >= kDayToggle && hit < kDayToggle + 7) {
                    m_newDays ^= 1 << ((FirstWeekday() + hit - kDayToggle) % 7);
                } else if (hit >= kAlarmRow) {
                    const size_t index =
                        (size_t)((hit - kAlarmRow) / kPartsPerAlarm);
                    const int part = (hit - kAlarmRow) % kPartsPerAlarm;
                    if (index < alarms.size()) {
                        if (part == kAlarmOn) {
                            alarms[index].on = !alarms[index].on;
                        } else if (part == kAlarmEdit) {
                            // Its time and sound on the wheels, as when
                            // added.
                            m_adding = true;
                            m_editing = (int)index;
                            m_newMinute = alarms[index].minute;
                            m_newSound = alarms[index].sound;
                            m_newDays = alarms[index].days;
                            m_newName = alarms[index].name;
                            m_nameEdit.Reset();
                            m_nameEdit.caret = m_newName.size();
                            PulseContents();
                        } else if (part == kAlarmSound) {
                            alarms[index].sound = !alarms[index].sound;
                        } else if (part == kAlarmDelete) {
                            alarms.erase(alarms.begin() + index);
                            PulseContents();
                        }
                        SaveAlarms();
                    }
                }
                break;
        }
        Changed();
        StartIslandAnimation();
        // The pill shows it (or its icon again).
        PostMessage(IslandWindow(m_owner), WM_APP_TIMER_CHANGED, 0, 0);
        if (canceled) {
            Close();
        }
        return true;
    }

    // Over a wheel: a step a notch.
    bool OnWheel(int delta) override {
        POINT cursor{};
        GetCursorPos(&cursor);
        RECT window{};
        GetWindowRect(m_hwnd, &window);
        const POINT pt{cursor.x - window.left - m_contentOffset.x,
                       cursor.y - window.top - m_contentOffset.y};
        const int hit = HitTest(pt);
        if (hit < kWheel || hit >= kWheel + kWheelCount) {
            return false;
        }
        m_wheelSpin += delta;
        const int steps = m_wheelSpin / WHEEL_DELTA;
        m_wheelSpin %= WHEEL_DELTA;
        if (steps) {
            EndTyping(true);
            StepWheel(hit - kWheel, -steps, true);
        }
        return true;
    }

    // Typing a wheel's figures (after a click on it): its value as typed,
    // Enter or Tab keeps it (Tab goes on to the next), Escape doesn't; the
    // arrows step it.
    bool OnKey(WPARAM key) override {
        if (m_naming) {
            if (key == VK_RETURN || key == VK_ESCAPE || key == VK_TAB) {
                m_naming = false;
                Changed();
                return true;
            }
            return EditTextKey(m_newName, m_nameEdit, key);
        }
        if (m_typing < 0) {
            return false;
        }
        if (key == VK_UP || key == VK_DOWN) {
            const int wheel = m_typing;
            EndTyping(true);
            m_typing = wheel;
            StepWheel(wheel, key == VK_UP ? 1 : -1, true);
            return true;
        }
        if (key == VK_RETURN || key == VK_TAB) {
            const int wheel = m_typing;
            EndTyping(true);
            if (key == VK_TAB) {
                const int next = NextWheel(wheel);
                if (next >= 0) {
                    m_typing = next;
                    m_typed.clear();
                }
            }
            Changed();
            return true;
        }
        if (key == VK_ESCAPE) {
            EndTyping(false);
            Changed();
            return true;
        }
        if (key == VK_BACK && !m_typed.empty()) {
            m_typed.pop_back();
            return true;
        }
        return false;
    }

    bool OnChar(WCHAR c) override {
        if (m_naming) {
            // (One line: Enter and Esc end it, see OnKey.)
            return c != L'\r' && c != L'\n' && c != 0x1B && c != L'\t' &&
                   EditTextChar(m_newName, m_nameEdit, c, kMaxAlarmName, m_hwnd);
        }
        if (m_typing < 0 || c < L'0' || c > L'9') {
            return false;
        }
        if (m_typed.size() >= 2) {
            m_typed.clear();
        }
        m_typed += c;
        // Two figures: on to the next wheel, like a time field.
        if (m_typed.size() == 2) {
            const int wheel = m_typing;
            EndTyping(true);
            const int next = NextWheel(wheel);
            if (next >= 0) {
                m_typing = next;
                m_typed.clear();
            }
            Changed();
        }
        return true;
    }

   private:
    static constexpr int kTabTimer = 0;
    static constexpr int kTabStopwatch = 1;
    static constexpr int kTabPomodoro = 2;
    static constexpr int kTabAlarms = 3;
    static constexpr int kTabCount = 4;
    // The parts: the tabs (0-3), the round buttons, the presets, the alarms'
    // buttons, the wheels, and each alarm's switch, sound and bin.
    static constexpr int kPrimary = 4;
    static constexpr int kSecondary = 5;
    static constexpr int kPreset = 6;
    static constexpr int kPresetCount = 4;
    static constexpr int kPresets[kPresetCount] = {1, 5, 10, 25};
    static constexpr int kAddAlarm = kPreset + kPresetCount;
    static constexpr int kAmPm = kAddAlarm + 1;
    static constexpr int kSound = kAddAlarm + 2;
    static constexpr int kSave = kAddAlarm + 3;
    static constexpr int kCancelAdd = kAddAlarm + 4;
    // A new alarm's name box and its seven days (in the week's order).
    static constexpr int kNameBox = kCancelAdd + 1;
    static constexpr int kDayToggle = kNameBox + 1;
    // The wheels: the timer's hours, minutes, seconds; Pomodoro's focus and
    // break (the same three each); a new alarm's hour and minute.
    static constexpr int kWheel = kDayToggle + 7;
    static constexpr int kWheelCount = 11;
    static constexpr int kWheelTimer = 0;
    static constexpr int kWheelFocus = 3;
    static constexpr int kWheelBreak = 6;
    static constexpr int kWheelAlarm = 9;
    static constexpr int kAlarmRow = kWheel + kWheelCount;
    static constexpr int kPartsPerAlarm = 4;
    static constexpr int kAlarmOn = 0;
    static constexpr int kAlarmSound = 1;
    static constexpr int kAlarmDelete = 2;
    static constexpr int kAlarmEdit = 3;
    // The Live Activities (see ShowActivity): each one going's pause and
    // cancel, the next alarm's switch, and "Show all".
    static constexpr int kRowPrimary = kAlarmRow + kPartsPerAlarm * (int)kMaxAlarms;
    static constexpr int kRowSecondary = kRowPrimary + kTimerModes;
    static constexpr int kNextAlarm = kRowSecondary + kTimerModes;
    static constexpr int kShowAll = kNextAlarm + 1;
    static constexpr int kPartCount = kShowAll + 1;

    static TimerMode TabMode(int tab) {
        return tab == kTabStopwatch  ? TimerMode::Stopwatch
               : tab == kTabPomodoro ? TimerMode::Pomodoro
                                     : TimerMode::Timer;
    }

    void SetTab(int tab, bool instant) {
        if (tab == m_tab) {
            return;
        }
        m_pageDirection = tab > m_tab ? 1.0f : -1.0f;
        m_tab = tab;
        m_adding = false;
        m_naming = false;
        EndTyping(true);
        if (instant) {
            m_tabKnob = tab;
            m_tabKnobVelocity = 0;
        } else {
            m_pageIn = 0;
            m_pageInVelocity = 0;
        }
        Changed();
        StartIslandAnimation();
    }

    // ---- The wheels.
    float WheelRow() const { return std::round(30 * m_scale); }

    int WheelMax(int wheel) const {
        const int part = wheel < kWheelAlarm ? wheel % 3 : wheel - kWheelAlarm;
        return part == 0 ? 23 : 59;
    }

    int WrapWheel(int wheel, int value) const {
        const int count = WheelMax(wheel) + 1;
        return ((value % count) + count) % count;
    }

    // The seconds (or the alarm's minute of the day) a wheel is part of.
    int& WheelTotal(int wheel) {
        LoadTimerLengths();
        return wheel < kWheelFocus   ? g_timer.seconds
               : wheel < kWheelBreak ? g_timer.focusSeconds
               : wheel < kWheelAlarm ? g_timer.breakSeconds
                                     : m_newMinute;
    }

    int WheelValue(int wheel) {
        const int total = WheelTotal(wheel);
        if (wheel >= kWheelAlarm) {
            return wheel == kWheelAlarm ? total / 60 : total % 60;
        }
        const int part = wheel % 3;
        return part == 0 ? total / 3600 : part == 1 ? total / 60 % 60
                                                    : total % 60;
    }

    void SetWheelValue(int wheel, int value) {
        value = std::clamp(value, 0, WheelMax(wheel));
        int& total = WheelTotal(wheel);
        if (wheel >= kWheelAlarm) {
            total = wheel == kWheelAlarm ? value * 60 + total % 60
                                         : total / 60 * 60 + value;
            return;
        }
        int hours = total / 3600;
        int minutes = total / 60 % 60;
        int seconds = total % 60;
        (wheel % 3 == 0 ? hours : wheel % 3 == 1 ? minutes : seconds) = value;
        // (A countdown lasts a second at least.)
        total = std::max(1, hours * 3600 + minutes * 60 + seconds);
    }

    // Turned by `steps` (up: more), rolling there; kept when `save`.
    void StepWheel(int wheel, int steps, bool save) {
        if (!steps) {
            return;
        }
        SetWheelValue(wheel, WrapWheel(wheel, WheelValue(wheel) + steps));
        m_wheelOffset[wheel] = std::clamp(m_wheelOffset[wheel] + steps, -2.0,
                                          2.0);
        // (Kept as the panel closes: not a storage write per notch.)
        if (save) {
            m_wheelsTurned = true;
        }
        Changed();
        StartIslandAnimation();
    }

    void SaveWheels() {
        SaveTimerLengths();
        m_wheelsTurned = false;
    }

    // Turned with the wheel or the arrows (see StepWheel): kept now.
    void SaveWheelsTurned() {
        if (m_wheelsTurned) {
            SaveWheels();
        }
    }
    bool m_wheelsTurned = false;

    // The wheel after this one on its page (for typing), or -1.
    int NextWheel(int wheel) const {
        if (wheel == kWheelAlarm) {
            return kWheelAlarm + 1;
        }
        if (wheel < kWheelAlarm && wheel % 3 < 2) {
            return wheel + 1;
        }
        if (wheel == kWheelFocus + 2) {
            return kWheelBreak;
        }
        return -1;
    }

    void EndTyping(bool keep) {
        if (m_typing >= 0 && keep && !m_typed.empty()) {
            const int wheel = m_typing;
            int value = _wtoi(m_typed.c_str());
            // (An alarm's hour typed as the clock shows it, with 12 hours.)
            if (wheel == kWheelAlarm && ClockShows12Hours()) {
                const bool pm = m_newMinute >= 12 * 60;
                value = value % 12 + (pm ? 12 : 0);
            }
            SetWheelValue(wheel, value);
            SaveWheels();
        }
        m_typing = -1;
        m_typed.clear();
    }

    // A wheel's figures as shown: two of them (an alarm's hour as the clock
    // shows it).
    std::wstring WheelText(int wheel, int value) const {
        if (wheel == kWheelAlarm && ClockShows12Hours()) {
            value = (value + 11) % 12 + 1;
            return std::to_wstring(value);
        }
        WCHAR text[8];
        swprintf_s(text, L"%02d", value);
        return text;
    }

    // ---- Drawing.
    void DrawTabs(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
        const float s = m_scale;
        const D2D1_RECT_F& track = m_tabsRect;
        const float radius = (track.bottom - track.top) / 2;
        brush->SetColor(Fg(0.08f));
        GlassFill(target, {track, radius, radius}, brush);
        const float segment = (track.right - track.left) / kTabCount;
        const float knob = (float)std::clamp(m_tabKnob, -0.05, kTabCount - 0.95);
        const float inset = std::round(2 * s);
        const D2D1_RECT_F lit{track.left + segment * knob + inset,
                              track.top + inset,
                              track.left + segment * (knob + 1) - inset,
                              track.bottom - inset};
        const float litRadius = (lit.bottom - lit.top) / 2;
        brush->SetColor(Fg(0.2f));
        GlassFill(target, {lit, litRadius, litRadius}, brush);
        static constexpr PCWSTR kNames[kTabCount][2] = {
            {L"Timer", L"Timer"},
            {L"Stopwatch", L"Cronômetro"},
            {L"Pomodoro", L"Pomodoro"},
            {L"Alarms", L"Alarmes"}};
        for (int i = 0; i < kTabCount; i++) {
            PCWSTR name = Tr(kNames[i][0], kNames[i][1]);
            const float hover = Hover(HoverKey(6600, i), m_hover == i);
            brush->SetColor(Fg(i == m_tab ? 0.95f : 0.6f + 0.2f * hover));
            target->DrawText(name, (UINT32)wcslen(name), m_buttonFormat.get(),
                             m_parts[i], brush);
        }
    }

    // A round button like iOS's Clock (Cancel / Start / Pause): a tinted
    // disc inside a thin ring of the same tint, a gap between them, its
    // label in its color (made smaller when it wouldn't fit).
    void DrawRoundButton(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush,
                         int part,
                         PCWSTR label,
                         D2D1_COLOR_F color,
                         bool enabled = true,
                         IDWriteTextFormat* format = nullptr) {
        const D2D1_RECT_F& rect = m_parts[part];
        if (rect.right <= rect.left) {
            return;
        }
        const float s = m_scale;
        const float hover =
            enabled ? Hover(HoverKey(6600, part), m_hover == part) : 0.0f;
        const D2D1_POINT_2F center{(rect.left + rect.right) / 2,
                                   (rect.top + rect.bottom) / 2};
        const float radius = (rect.right - rect.left) / 2;
        const float ring = std::max(1.0f, std::round(1.5f * s));
        const float inner = radius - ring - std::round(2.5f * s);
        const float alpha = enabled ? 1.0f : 0.4f;
        brush->SetColor({color.r, color.g, color.b,
                         (0.3f + 0.12f * hover) * alpha});
        target->FillEllipse({center, inner, inner}, brush);
        target->DrawEllipse({center, radius - ring / 2, radius - ring / 2},
                            brush, ring);
        brush->SetColor({color.r, color.g, color.b, alpha});
        winrt::com_ptr<IDWriteTextLayout> layout;
        const float room = inner * 2 - std::round(10 * s);
        if (FAILED(m_dwrite->CreateTextLayout(
                label, (UINT32)wcslen(label),
                format ? format : m_roundButtonFormat.get(), room,
                rect.bottom - rect.top, layout.put()))) {
            return;
        }
        layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        DWRITE_TEXT_METRICS metrics{};
        if (SUCCEEDED(layout->GetMetrics(&metrics)) &&
            metrics.widthIncludingTrailingWhitespace > room) {
            layout->SetFontSize(
                std::max(9 * s, 14 * s * room /
                                    metrics.widthIncludingTrailingWhitespace),
                {0, (UINT32)wcslen(label)});
        }
        target->DrawTextLayout({center.x - room / 2, rect.top}, layout.get(),
                               brush);
    }

    // A pill-shaped button (lit: in the accent color).
    void DrawPillButton(ID2D1RenderTarget* target,
                        ID2D1SolidColorBrush* brush,
                        int part,
                        PCWSTR label,
                        bool accent,
                        IDWriteTextFormat* format = nullptr) {
        const D2D1_RECT_F& rect = m_parts[part];
        if (rect.right <= rect.left) {
            return;
        }
        const float hover = Hover(HoverKey(6600, part), m_hover == part);
        const float radius = (rect.bottom - rect.top) / 2;
        brush->SetColor(accent ? AccentLit(0.9f, hover)
                               : Fg(0.08f + 0.08f * hover));
        GlassFill(target, {rect, radius, radius}, brush);
        brush->SetColor(accent ? AccentInk(1) : Fg(0.85f));
        target->DrawText(label, (UINT32)wcslen(label),
                         format ? format : m_buttonFormat.get(), rect, brush);
    }

    // Text with figures of equal width, in the middle of `rect`.
    void DrawFigures(ID2D1RenderTarget* target,
                     ID2D1SolidColorBrush* brush,
                     const std::wstring& text,
                     IDWriteTextFormat* format,
                     const D2D1_RECT_F& rect) {
        winrt::com_ptr<IDWriteTextLayout> layout;
        if (FAILED(m_dwrite->CreateTextLayout(
                text.c_str(), (UINT32)text.size(), format,
                rect.right - rect.left, rect.bottom - rect.top,
                layout.put()))) {
            return;
        }
        if (m_tabular) {
            layout->SetTypography(m_tabular.get(), {0, (UINT32)text.size()});
        }
        target->DrawTextLayout({rect.left, rect.top}, layout.get(), brush);
    }

    // A wheel: its value in the middle, the ones before and after faded over
    // and under it, rolling when it turns; ringed while typed.
    void DrawWheel(ID2D1RenderTarget* target,
                   ID2D1SolidColorBrush* brush,
                   int wheel) {
        const D2D1_RECT_F& rect = m_parts[kWheel + wheel];
        if (rect.right <= rect.left) {
            return;
        }
        const float s = m_scale;
        const float row = WheelRow();
        const float middle = (rect.top + rect.bottom) / 2;
        const float hover =
            Hover(HoverKey(6600, kWheel + wheel), m_hover == kWheel + wheel);
        const float radius = std::round(8 * s);
        // The chosen row's band.
        const D2D1_RECT_F band{rect.left, middle - row / 2, rect.right,
                               middle + row / 2};
        brush->SetColor(Fg(0.08f + 0.05f * hover));
        GlassFill(target, {band, radius, radius}, brush);
        if (m_typing == wheel) {
            brush->SetColor(Accent(1));
            target->DrawRoundedRectangle(
                {{band.left + 0.75f, band.top + 0.75f, band.right - 0.75f,
                  band.bottom - 0.75f},
                 radius,
                 radius},
                brush, 1.5f);
        }
        target->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        const int value = WheelValue(wheel);
        const float offset = (float)m_wheelOffset[wheel];
        for (int i = -2; i <= 2; i++) {
            const float y = middle + (i + offset) * row;
            const float distance = std::fabs(i + offset);
            if (distance > 2.2f) {
                continue;
            }
            std::wstring text;
            if (i == 0 && m_typing == wheel && !m_typed.empty()) {
                text = m_typed;
            } else {
                text = WheelText(wheel, WrapWheel(wheel, value + i));
            }
            const float alpha = std::clamp(1.0f - distance * 0.45f, 0.0f, 1.0f);
            brush->SetColor(Fg(0.95f * alpha));
            DrawFigures(target, brush, text,
                        distance < 0.5f ? m_wheelFormat.get()
                                        : m_wheelSideFormat.get(),
                        {rect.left, y - row / 2, rect.right, y + row / 2});
        }
        target->PopAxisAlignedClip();
    }

    // Three wheels for hours, minutes and seconds (`first`), their names
    // under them.
    void DrawLength(ID2D1RenderTarget* target,
                    ID2D1SolidColorBrush* brush,
                    int first) {
        static constexpr PCWSTR kUnits[3][2] = {
            {L"hours", L"horas"}, {L"min", L"min"}, {L"sec", L"seg"}};
        for (int i = 0; i < 3; i++) {
            DrawWheel(target, brush, first + i);
            const D2D1_RECT_F& rect = m_parts[kWheel + first + i];
            PCWSTR unit = Tr(kUnits[i][0], kUnits[i][1]);
            brush->SetColor(Fg(0.5f));
            target->DrawText(unit, (UINT32)wcslen(unit), m_textFormat.get(),
                             {rect.left, rect.bottom, rect.right,
                              rect.bottom + std::round(16 * m_scale)},
                             brush);
        }
    }

    // A countdown's ring: what's left of it, going round (in `ring`, the
    // tab's own by default).
    void DrawRing(ID2D1RenderTarget* target,
                  ID2D1SolidColorBrush* brush,
                  float left,
                  const D2D1_RECT_F* place = nullptr,
                  int modeIndex = -1) {
        const float s = m_scale;
        const D2D1_RECT_F& ring = place ? *place : m_ring;
        const TimerMode mode =
            modeIndex >= 0 ? (TimerMode)modeIndex : TabMode(m_tab);
        const D2D1_POINT_2F center{(ring.left + ring.right) / 2,
                                   (ring.top + ring.bottom) / 2};
        const float width = place ? std::round(4 * s) : std::round(6 * s);
        const float radius = (ring.right - ring.left) / 2 - width / 2;
        brush->SetColor(Fg(0.1f));
        target->DrawEllipse({center, radius, radius}, brush, width);
        winrt::com_ptr<ID2D1Factory> factory;
        target->GetFactory(factory.put());
        if (!m_roundCaps && factory) {
            factory->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND,
                                            D2D1_CAP_STYLE_ROUND,
                                            D2D1_CAP_STYLE_ROUND),
                nullptr, 0, m_roundCaps.put());
        }
        const float fraction = std::clamp(left, 0.0f, 1.0f);
        brush->SetColor(mode == TimerMode::Pomodoro && g_timer.onBreak
                            ? D2D1::ColorF(0.19f, 0.82f, 0.35f)
                            : D2D1::ColorF(1.0f, 0.62f, 0.04f));
        if (fraction >= 0.999f) {
            target->DrawEllipse({center, radius, radius}, brush, width);
            return;
        }
        winrt::com_ptr<ID2D1PathGeometry> path;
        winrt::com_ptr<ID2D1GeometrySink> sink;
        if (fraction <= 0.001f || !factory ||
            FAILED(factory->CreatePathGeometry(path.put())) ||
            FAILED(path->Open(sink.put()))) {
            return;
        }
        constexpr float kPi = 3.14159265f;
        const float end = -kPi / 2 + fraction * 2 * kPi;
        sink->BeginFigure({center.x, center.y - radius},
                          D2D1_FIGURE_BEGIN_HOLLOW);
        sink->AddArc({{center.x + radius * std::cos(end),
                       center.y + radius * std::sin(end)},
                      {radius, radius},
                      0,
                      D2D1_SWEEP_DIRECTION_CLOCKWISE,
                      fraction > 0.5f ? D2D1_ARC_SIZE_LARGE
                                      : D2D1_ARC_SIZE_SMALL});
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        if (SUCCEEDED(sink->Close())) {
            target->DrawGeometry(path.get(), brush, width, m_roundCaps.get());
        }
    }

    // The timer, the stopwatch or Pomodoro: its wheels before it starts, its
    // ring (or the stopwatch's big time) while it goes, and its buttons.
    void DrawClock(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
        const TimerMode mode = TabMode(m_tab);
        const TimerClock& clock = TimerClockOf(mode);
        if (mode == TimerMode::Stopwatch) {
            brush->SetColor(Fg(clock.running ? 0.95f : 0.75f));
            DrawFigures(target, brush, TimerText(mode, true),
                        m_timeFormat.get(), m_bigTime);
        } else if (!clock.Active()) {
            if (mode == TimerMode::Timer) {
                DrawLength(target, brush, kWheelTimer);
                for (int i = 0; i < kPresetCount; i++) {
                    const std::wstring label =
                        std::to_wstring(kPresets[i]) + L" min";
                    DrawPillButton(target, brush, kPreset + i, label.c_str(),
                                   g_timer.seconds == kPresets[i] * 60);
                }
            } else {
                brush->SetColor(Fg(0.6f));
                PCWSTR focus = Tr(L"Focus", L"Foco");
                PCWSTR rest = Tr(L"Break", L"Pausa");
                target->DrawText(focus, (UINT32)wcslen(focus),
                                 m_textFormat.get(), m_focusLabel, brush);
                target->DrawText(rest, (UINT32)wcslen(rest), m_textFormat.get(),
                                 m_breakLabel, brush);
                DrawLength(target, brush, kWheelFocus);
                DrawLength(target, brush, kWheelBreak);
            }
        } else {
            const int length = TimerLength(mode);
            DrawRing(target, brush,
                     length > 0 ? (float)(TimerLeft(mode) / length) : 0.0f);
            brush->SetColor(Fg(clock.running ? 0.95f : 0.7f));
            const float s = m_scale;
            const D2D1_RECT_F time{m_ring.left, m_ring.top, m_ring.right,
                                   (m_ring.top + m_ring.bottom) / 2 +
                                       std::round(16 * s)};
            DrawFigures(target, brush, TimerText(mode), m_ringFormat.get(),
                        time);
            std::wstring caption = TimerModeName(mode, g_timer.onBreak);
            if (mode == TimerMode::Pomodoro && g_timer.rounds > 0) {
                caption += L" · " + std::to_wstring(g_timer.rounds) +
                           L" ✓";
            }
            brush->SetColor(Fg(0.55f));
            target->DrawText(caption.c_str(), (UINT32)caption.size(),
                             m_textFormat.get(),
                             {m_ring.left, time.bottom, m_ring.right,
                              time.bottom + std::round(18 * s)},
                             brush);
        }
        const D2D1_COLOR_F gray = Fg(0.9f);
        const D2D1_COLOR_F green = D2D1::ColorF(0.19f, 0.82f, 0.35f);
        const D2D1_COLOR_F orange = D2D1::ColorF(1.0f, 0.62f, 0.04f);
        DrawRoundButton(target, brush, kSecondary,
                        mode == TimerMode::Stopwatch ? Tr(L"Reset", L"Zerar")
                                                     : Tr(L"Cancel", L"Cancelar"),
                        gray, clock.Active());
        DrawRoundButton(target, brush, kPrimary,
                        clock.running   ? Tr(L"Pause", L"Pausar")
                        : clock.Active() ? Tr(L"Resume", L"Continuar")
                                         : Tr(L"Start", L"Iniciar"),
                        clock.running ? orange : green);
    }

    // The Live Activities: each one going in its row (its ring, or the
    // stopwatch's glyph; its name over its time in orange, green on a
    // break; its round buttons), the next alarm with its switch, and "Show
    // all" under them.
    void DrawActivity(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
        const float s = m_scale;
        for (size_t i = 0; i < m_activityModes.size(); i++) {
            const TimerMode mode = m_activityModes[i];
            const TimerClock& clock = TimerClockOf(mode);
            const D2D1_RECT_F& ring = m_activityRings[i];
            const D2D1_COLOR_F orange =
                mode == TimerMode::Pomodoro && g_timer.onBreak
                    ? D2D1::ColorF(0.19f, 0.82f, 0.35f)
                    : D2D1::ColorF(1.0f, 0.62f, 0.04f);
            if (mode == TimerMode::Stopwatch) {
                const WCHAR glyph = 0xE916;
                brush->SetColor(orange);
                DrawGlyph(target, &glyph, 1, m_bigIconFormat.get(), ring,
                          brush);
            } else {
                const int length = TimerLength(mode);
                DrawRing(target, brush,
                         length > 0 ? (float)(TimerLeft(mode) / length) : 0.0f,
                         &ring, (int)mode);
            }
            const float textLeft = ring.right + std::round(12 * s);
            const float textRight =
                m_parts[kRowSecondary + (int)i].left - std::round(8 * s);
            brush->SetColor(Fg(0.55f));
            const std::wstring name = TimerModeName(mode, g_timer.onBreak);
            target->DrawText(name.c_str(), (UINT32)name.size(),
                             m_leftTextFormat.get(),
                             {textLeft, ring.top - std::round(4 * s), textRight,
                              ring.top + std::round(14 * s)},
                             brush);
            brush->SetColor({orange.r, orange.g, orange.b,
                             clock.running ? 1.0f : 0.6f});
            m_ringFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            DrawFigures(target, brush, TimerText(mode), m_ringFormat.get(),
                        {textLeft, ring.top + std::round(12 * s), textRight,
                         ring.bottom + std::round(8 * s)});
            m_ringFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            const WCHAR pause[2] = {clock.running ? kGlyphPause : kGlyphPlay, 0};
            const WCHAR cancel[2] = {kGlyphClose, 0};
            DrawRoundButton(target, brush, kRowSecondary + (int)i, cancel,
                            Fg(0.9f), true, m_bigIconFormat.get());
            DrawRoundButton(target, brush, kRowPrimary + (int)i, pause, orange,
                            true, m_bigIconFormat.get());
        }
        const auto& alarms = Alarms();
        if (m_activityAlarm >= 0 && m_activityAlarm < (int)alarms.size()) {
            const Alarm& alarm = alarms[m_activityAlarm];
            const D2D1_RECT_F& row = m_activityAlarmRow;
            const D2D1_RECT_F icon{row.left, row.top, row.left + std::round(44 * s),
                                   row.bottom};
            const WCHAR glyph = 0xE823;
            brush->SetColor(D2D1::ColorF(1.0f, 0.62f, 0.04f, alarm.on ? 1 : 0.5f));
            DrawGlyph(target, &glyph, 1, m_bigIconFormat.get(), icon, brush);
            SYSTEMTIME now;
            GetLocalTime(&now);
            const int until = AlarmMinutesUntil(alarm, now);
            const std::wstring text = AlarmTimeText(alarm.minute) + L" · " +
                                      UntilText(until);
            const float textLeft = icon.right + std::round(12 * s);
            brush->SetColor(Fg(0.55f));
            PCWSTR title =
                alarm.name.empty() ? Tr(L"Alarm", L"Alarme") : alarm.name.c_str();
            target->DrawText(title, (UINT32)wcslen(title), m_leftTextFormat.get(),
                             {textLeft, row.top, m_parts[kNextAlarm].left,
                              row.top + std::round(18 * s)},
                             brush);
            brush->SetColor(Fg(alarm.on ? 0.95f : 0.5f));
            target->DrawText(text.c_str(), (UINT32)text.size(),
                             m_leftTextFormat.get(),
                             {textLeft, row.top + std::round(18 * s),
                              m_parts[kNextAlarm].left - std::round(8 * s),
                              row.bottom},
                             brush);
            DrawSwitch(target, brush, m_parts[kNextAlarm],
                       Hover(HoverKey(6703, alarm.minute), alarm.on,
                             alarm.on ? 1 : 0));
        }
        DrawPillButton(target, brush, kShowAll, Tr(L"Show all", L"Mostrar tudo"),
                       false);
    }

    // The alarms: each one's time, when it rings, its switch, its sound and
    // its bin; or a new one's time on wheels.
    void DrawAlarms(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
        const float s = m_scale;
        if (m_adding) {
            DrawNameBox(target, brush);
            DrawWheel(target, brush, kWheelAlarm);
            DrawWheel(target, brush, kWheelAlarm + 1);
            // Its days, each a circle with its initial (lit: it rings then).
            const int first = FirstWeekday();
            for (int i = 0; i < 7; i++) {
                const int day = (first + i) % 7;
                const bool on = m_newDays >> day & 1;
                const D2D1_RECT_F& rect = m_parts[kDayToggle + i];
                const float lit =
                    Hover(HoverKey(6704, i), m_hover == kDayToggle + i);
                const float radius = (rect.bottom - rect.top) / 2;
                brush->SetColor(on ? Accent(1.0f - 0.15f * lit)
                                   : Fg(0.08f + 0.07f * lit));
                GlassFillEllipse(
                    target,
                    {{(rect.left + rect.right) / 2, rect.top + radius},
                     radius,
                     radius},
                    brush);
                const std::wstring name = WeekdayName(day, true);
                brush->SetColor(on ? AccentInk(1) : Fg(0.8f));
                target->DrawText(name.c_str(), (UINT32)name.size(),
                                 m_buttonFormat.get(), rect, brush,
                                 D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
            const std::wstring days = AlarmDaysText(m_newDays);
            brush->SetColor(Fg(0.55f));
            target->DrawText(days.c_str(), (UINT32)days.size(),
                             m_textFormat.get(), m_daysLabel, brush,
                             D2D1_DRAW_TEXT_OPTIONS_CLIP);
            brush->SetColor(Fg(0.8f));
            target->DrawText(L":", 1, m_wheelFormat.get(), m_colon, brush);
            if (ClockShows12Hours()) {
                std::wstring marker;
                AlarmTimeText(m_newMinute, &marker);
                DrawPillButton(target, brush, kAmPm,
                               marker.empty() ? L"AM" : marker.c_str(), false);
            }
            const WCHAR bell[2] = {m_newSound ? (WCHAR)0xEA8F : (WCHAR)0xE7ED,
                                   0};
            DrawPillButton(target, brush, kSound, bell, m_newSound,
                           m_iconFormat.get());
            DrawPillButton(target, brush, kCancelAdd, Tr(L"Cancel", L"Cancelar"),
                           false);
            DrawPillButton(target, brush, kSave, Tr(L"Save", L"Salvar"), true);
            return;
        }
        const auto& alarms = Alarms();
        for (size_t i = 0; i < alarms.size() && i < kMaxAlarms; i++) {
            const Alarm& alarm = alarms[i];
            const int base = kAlarmRow + (int)i * kPartsPerAlarm;
            const D2D1_RECT_F& row = m_alarmRows[i];
            // (A new one: faded and a little lower, coming in.)
            const long long key = alarm.minute * 2 + (alarm.sound ? 1 : 0);
            const float appear =
                Hover(HoverKey(6702, key), true, key == m_savedAlarm ? 0 : 1);
            winrt::com_ptr<ID2D1Layer> layer;
            const bool coming = appear < 0.99f;
            if (coming) {
                target->CreateLayer(nullptr, layer.put());
                target->PushLayer(
                    D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                          D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                          D2D1::Matrix3x2F::Translation(
                                              0, (1 - appear) * 10 * s),
                                          appear),
                    layer.get());
            }
            std::wstring marker;
            const std::wstring time = AlarmTimeText(alarm.minute, &marker);
            const float alpha = alarm.on ? 0.95f : 0.45f;
            brush->SetColor(Fg(alpha));
            target->DrawText(time.c_str(), (UINT32)time.size(),
                             m_alarmFormat.get(),
                             {row.left, row.top, row.right,
                              row.top + std::round(36 * s)},
                             brush);
            // "Wake up · Weekdays", or "AM · Every day", "· no sound".
            std::wstring when = AlarmDaysText(alarm.days);
            if (!marker.empty()) {
                when = marker + L" · " + when;
            }
            if (!alarm.name.empty()) {
                when = alarm.name + L" · " + when;
            }
            if (!alarm.sound) {
                when += Tr(L" · no sound", L" · sem som");
            }
            brush->SetColor(Fg(0.5f * (alarm.on ? 1.0f : 0.7f)));
            m_leftTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            target->DrawText(when.c_str(), (UINT32)when.size(),
                             m_leftTextFormat.get(),
                             {row.left, row.top + std::round(34 * s), row.right,
                              row.bottom},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            m_leftTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
            // Its sound, its bin, and its switch.
            const WCHAR bell[2] = {alarm.sound ? (WCHAR)0xEA8F : (WCHAR)0xE7ED,
                                   0};
            const WCHAR bin[2] = {(WCHAR)0xE74D, 0};
            const WCHAR pencil[2] = {(WCHAR)0xE70F, 0};
            DrawIconButton(target, brush, base + kAlarmEdit, pencil);
            DrawIconButton(target, brush, base + kAlarmSound, bell);
            DrawIconButton(target, brush, base + kAlarmDelete, bin);
            DrawSwitch(target, brush, m_parts[base + kAlarmOn],
                       Hover(HoverKey(6701, key), alarm.on, alarm.on ? 1 : 0));
            // (A line between them.)
            if (i + 1 < alarms.size()) {
                brush->SetColor(Fg(0.08f));
                target->FillRectangle(
                    {row.left, row.bottom - 0.5f, row.right, row.bottom + 0.5f},
                    brush);
            }
            if (coming) {
                target->PopLayer();
            }
        }
        if (alarms.empty()) {
            PCWSTR none = Tr(L"No alarms yet", L"Nenhum alarme ainda");
            brush->SetColor(Fg(0.5f));
            target->DrawText(none, (UINT32)wcslen(none), m_textFormat.get(),
                             m_empty, brush);
        }
        if (alarms.size() < kMaxAlarms) {
            DrawPillButton(target, brush, kAddAlarm,
                           Tr(L"+ Add alarm", L"+ Adicionar alarme"), false);
        }
    }

    // A new alarm's name: one line, its hint while empty, the caret while
    // typed in (like the panels' boxes, see EditingPanel::DrawTextBox).
    void DrawNameBox(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
        const float s = m_scale;
        const D2D1_RECT_F& box = m_parts[kNameBox];
        const float radius = std::round(9 * s);
        brush->SetColor(Fg(0.07f));
        GlassFill(target, {box, radius, radius}, brush);
        brush->SetColor(m_naming ? Accent(0.9f) : Fg(0.12f));
        target->DrawRoundedRectangle(
            {{box.left + 0.5f, box.top + 0.5f, box.right - 0.5f,
              box.bottom - 0.5f},
             radius,
             radius},
            brush, m_naming ? 1.5f : 1.0f);
        const float line = std::round(18 * s);
        const float inner = std::round(10 * s);
        const float padTop = std::round((box.bottom - box.top - line) / 2);
        const D2D1_RECT_F textRect{box.left + inner, box.top + padTop,
                                   box.right - inner, box.top + padTop + line};
        float caretX = textRect.left;
        float caretTop = textRect.top;
        float caretHeight = line;
        if (m_newName.empty()) {
            PCWSTR hint = Tr(L"Name (optional)", L"Nome (opcional)");
            brush->SetColor(Fg(0.4f));
            target->DrawText(hint, (UINT32)wcslen(hint), m_leftTextFormat.get(),
                             textRect, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            m_nameEdit.layout = nullptr;
            m_nameEdit.origin = {textRect.left, textRect.top};
        } else {
            m_leftTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            const WrappedField wrapped =
                LayOutEdit(m_dwrite, m_leftTextFormat.get(), m_newName,
                           m_nameEdit, textRect.right - textRect.left, line,
                           line);
            m_leftTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
            if (wrapped.layout) {
                target->PushAxisAlignedClip(textRect,
                                            D2D1_ANTIALIAS_MODE_ALIASED);
                DrawTextSelection(target, brush, wrapped.layout.get(),
                                  {textRect.left, textRect.top - wrapped.scroll},
                                  m_nameEdit);
                brush->SetColor(Fg(0.95f));
                target->DrawTextLayout(
                    {textRect.left, textRect.top - wrapped.scroll},
                    wrapped.layout.get(), brush,
                    D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
                target->PopAxisAlignedClip();
                caretX = textRect.left + wrapped.caretX;
                caretTop = textRect.top - wrapped.scroll + wrapped.caretY;
                caretHeight = wrapped.caretHeight;
                m_nameEdit.origin = {textRect.left,
                                     textRect.top - wrapped.scroll};
            }
        }
        if (m_naming && caretX <= textRect.right + 1) {
            brush->SetColor(NowSeconds() - g_lastTyped < 5 ? CaretColor()
                                                           : Accent(1));
            target->FillRectangle({caretX + 1, caretTop + 1, caretX + 2.5f,
                                   caretTop + caretHeight - 1},
                                  brush);
        }
    }

    // The name typed, without spaces around it. A line break or a tab (a
    // pasted text can have them) is a space: the alarms are kept a line
    // each, with a comma between their parts (see SaveAlarms).
    std::wstring AlarmNameTyped() const {
        std::wstring name = m_newName;
        std::replace_if(
            name.begin(), name.end(), [](WCHAR c) { return c < 0x20; }, L' ');
        const size_t from = name.find_first_not_of(L' ');
        if (from == std::wstring::npos) {
            return {};
        }
        return name.substr(from, name.find_last_not_of(L' ') - from + 1);
    }

    void DrawIconButton(ID2D1RenderTarget* target,
                        ID2D1SolidColorBrush* brush,
                        int part,
                        const WCHAR* glyph) {
        const D2D1_RECT_F& rect = m_parts[part];
        const float hover = Hover(HoverKey(6600, part), m_hover == part);
        if (hover > 0.01f) {
            const float radius = (rect.bottom - rect.top) / 2;
            brush->SetColor(Fg(0.1f * hover));
            GlassFill(target, {rect, radius, radius}, brush);
        }
        brush->SetColor(Fg(0.6f + 0.3f * hover));
        DrawGlyph(target, glyph, 1, m_iconFormat.get(), rect, brush);
    }

    // An on/off switch like the settings' (green when on, like macOS's).
    // (`on` from 0 to 1: it slides and changes color as it switches.)
    void DrawSwitch(ID2D1RenderTarget* target,
                    ID2D1SolidColorBrush* brush,
                    const D2D1_RECT_F& rect,
                    float on) {
        const float radius = (rect.bottom - rect.top) / 2;
        const D2D1_COLOR_F off = Fg(0.22f);
        brush->SetColor({off.r + (0.19f - off.r) * on,
                         off.g + (0.82f - off.g) * on,
                         off.b + (0.35f - off.b) * on,
                         off.a + (1 - off.a) * on});
        GlassFill(target, {rect, radius, radius}, brush);
        const float knob = radius - std::round(2 * m_scale);
        brush->SetColor(KnobColor(on));
        const float x = rect.left + radius + (rect.right - rect.left - radius * 2) * on;
        target->FillEllipse({{x, rect.top + radius}, knob, knob}, brush);
    }

    // ---- Layout.
    void Layout() {
        const float s = m_scale;
        const float width = std::round(320 * s);
        const float padding = std::round(14 * s);
        const float gap = std::round(6 * s);
        for (auto& part : m_parts) {
            part = {};
        }
        for (auto& row : m_alarmRows) {
            row = {};
        }
        m_ring = m_bigTime = m_empty = m_colon = m_focusLabel = m_breakLabel =
            m_daysLabel = {};
        // The Live Activities (gone with the last one going: the whole
        // panel). The countdowns, the soonest end first, then the stopwatch.
        m_activityModes.clear();
        m_activityAlarm = -1;
        m_activityAlarmRow = {};
        for (TimerMode mode : {TimerMode::Timer, TimerMode::Pomodoro}) {
            if (TimerActive(mode)) {
                m_activityModes.push_back(mode);
            }
        }
        std::sort(m_activityModes.begin(), m_activityModes.end(),
                  [](TimerMode a, TimerMode b) {
                      return TimerLeft(a) < TimerLeft(b);
                  });
        if (TimerActive(TimerMode::Stopwatch)) {
            m_activityModes.push_back(TimerMode::Stopwatch);
        }
        m_activity = m_activity && !m_activityModes.empty();
        if (m_activity) {
            const float wide = std::round(340 * s);
            const float ring = std::round(44 * s);
            const float round = std::round(44 * s);
            const float rowHeight = std::round(64 * s);
            float y = padding;
            for (size_t i = 0; i < m_activityModes.size(); i++) {
                const float middle = y + rowHeight / 2;
                m_activityRings[i] = {padding, middle - ring / 2,
                                      padding + ring, middle + ring / 2};
                m_parts[kRowPrimary + (int)i] = {wide - padding - round,
                                                 middle - round / 2,
                                                 wide - padding,
                                                 middle + round / 2};
                m_parts[kRowSecondary + (int)i] = {
                    wide - padding - round * 2 - gap, middle - round / 2,
                    wide - padding - round - gap, middle + round / 2};
                y += rowHeight;
            }
            // The next alarm that's on, within a day.
            SYSTEMTIME now;
            GetLocalTime(&now);
            int soonest = 24 * 60 + 1;
            const auto& alarms = Alarms();
            for (size_t i = 0; i < alarms.size(); i++) {
                const int until = AlarmMinutesUntil(alarms[i], now);
                if (alarms[i].on && until < soonest) {
                    soonest = until;
                    m_activityAlarm = (int)i;
                }
            }
            if (m_activityAlarm >= 0) {
                const float alarmRow = std::round(48 * s);
                m_activityAlarmRow = {padding, y, wide - padding, y + alarmRow};
                const float switchWidth = std::round(40 * s);
                const float switchHeight = std::round(22 * s);
                const float middle = y + alarmRow / 2;
                m_parts[kNextAlarm] = {wide - padding - switchWidth,
                                       middle - switchHeight / 2,
                                       wide - padding,
                                       middle + switchHeight / 2};
                y += alarmRow;
            }
            y += std::round(8 * s);
            m_parts[kShowAll] = {padding, y, padding + std::round(120 * s),
                                 y + std::round(28 * s)};
            y = m_parts[kShowAll].bottom + padding;
            m_size = {(LONG)wide, (LONG)std::ceil(y)};
            return;
        }
        float y = padding;
        m_tabsRect = {padding, y, width - padding, y + std::round(30 * s)};
        const float segment = (m_tabsRect.right - m_tabsRect.left) / kTabCount;
        for (int i = 0; i < kTabCount; i++) {
            m_parts[i] = {m_tabsRect.left + segment * i, m_tabsRect.top,
                          m_tabsRect.left + segment * (i + 1),
                          m_tabsRect.bottom};
        }
        y = m_tabsRect.bottom + std::round(16 * s);
        const float row = WheelRow();
        const float wheelWidth = std::round(62 * s);
        const float wheelHeight = row * 3;
        // Three wheels side by side in the middle, `y` their top.
        auto placeWheels = [&](int first, float top) {
            const float all = wheelWidth * 3 + gap * 2;
            float x = std::round((width - all) / 2);
            for (int i = 0; i < 3; i++) {
                m_parts[kWheel + first + i] = {x, top, x + wheelWidth,
                                               top + wheelHeight};
                x += wheelWidth + gap;
            }
        };
        if (m_tab == kTabAlarms) {
            if (m_adding) {
                // Its name over the wheels, its days under them.
                m_parts[kNameBox] = {padding, y, width - padding,
                                     y + std::round(32 * s)};
                y += std::round(32 * s) + std::round(12 * s);
                const float all = wheelWidth * 2 + std::round(20 * s);
                float x = std::round((width - all) / 2);
                m_parts[kWheel + kWheelAlarm] = {x, y, x + wheelWidth,
                                                 y + wheelHeight};
                m_colon = {x + wheelWidth, y, x + wheelWidth + std::round(20 * s),
                           y + wheelHeight};
                x += wheelWidth + std::round(20 * s);
                m_parts[kWheel + kWheelAlarm + 1] = {x, y, x + wheelWidth,
                                                     y + wheelHeight};
                y += wheelHeight + std::round(14 * s);
                const float circle = std::round(32 * s);
                const float spread =
                    (width - padding * 2 - circle * 7) / 6;
                for (int i = 0; i < 7; i++) {
                    const float left =
                        std::round(padding + i * (circle + spread));
                    m_parts[kDayToggle + i] = {left, y, left + circle,
                                               y + circle};
                }
                y += circle + std::round(4 * s);
                m_daysLabel = {padding, y, width - padding,
                               y + std::round(18 * s)};
                y += std::round(18 * s) + std::round(10 * s);
                const float chip = std::round(30 * s);
                float left = padding;
                if (ClockShows12Hours()) {
                    m_parts[kAmPm] = {left, y, left + std::round(64 * s),
                                      y + chip};
                    left += std::round(64 * s) + gap;
                }
                m_parts[kSound] = {left, y, left + std::round(44 * s), y + chip};
                const float button = std::round(84 * s);
                m_parts[kSave] = {width - padding - button, y, width - padding,
                                  y + chip};
                m_parts[kCancelAdd] = {m_parts[kSave].left - gap - button, y,
                                       m_parts[kSave].left - gap, y + chip};
                y += chip + padding;
            } else {
                const auto& alarms = Alarms();
                const float alarmRow = std::round(58 * s);
                const float icon = std::round(28 * s);
                const float switchWidth = std::round(40 * s);
                const float switchHeight = std::round(22 * s);
                for (size_t i = 0; i < alarms.size() && i < kMaxAlarms; i++) {
                    const int base = kAlarmRow + (int)i * kPartsPerAlarm;
                    const float middle = y + alarmRow / 2;
                    float right = width - padding;
                    m_parts[base + kAlarmOn] = {
                        right - switchWidth, middle - switchHeight / 2, right,
                        middle + switchHeight / 2};
                    right -= switchWidth + gap * 2;
                    m_parts[base + kAlarmDelete] = {right - icon,
                                                    middle - icon / 2, right,
                                                    middle + icon / 2};
                    right -= icon + gap;
                    m_parts[base + kAlarmSound] = {right - icon,
                                                   middle - icon / 2, right,
                                                   middle + icon / 2};
                    right -= icon + gap;
                    m_parts[base + kAlarmEdit] = {right - icon,
                                                  middle - icon / 2, right,
                                                  middle + icon / 2};
                    m_alarmRows[i] = {padding, y, right - gap, y + alarmRow};
                    y += alarmRow;
                }
                if (alarms.empty()) {
                    m_empty = {padding, y, width - padding,
                               y + std::round(40 * s)};
                    y = m_empty.bottom;
                }
                if (alarms.size() < kMaxAlarms) {
                    y += std::round(8 * s);
                    m_parts[kAddAlarm] = {padding, y, width - padding,
                                          y + std::round(32 * s)};
                    y = m_parts[kAddAlarm].bottom;
                }
                y += padding;
            }
            m_size = {(LONG)width, (LONG)std::ceil(y)};
            return;
        }
        const TimerMode mode = TabMode(m_tab);
        const bool active = TimerActive(mode);
        if (mode == TimerMode::Stopwatch) {
            m_bigTime = {padding, y, width - padding, y + std::round(70 * s)};
            y = m_bigTime.bottom + std::round(14 * s);
        } else if (!active && mode == TimerMode::Timer) {
            placeWheels(kWheelTimer, y);
            y += wheelHeight + std::round(22 * s);
            const float chip = std::round(28 * s);
            const float presetWidth =
                (width - padding * 2 - gap * (kPresetCount - 1)) / kPresetCount;
            for (int i = 0; i < kPresetCount; i++) {
                const float left = padding + i * (presetWidth + gap);
                m_parts[kPreset + i] = {left, y, left + presetWidth, y + chip};
            }
            y += chip + std::round(14 * s);
        } else if (!active) {
            const float label = std::round(18 * s);
            m_focusLabel = {padding, y, width - padding, y + label};
            y += label;
            placeWheels(kWheelFocus, y);
            y += wheelHeight + std::round(22 * s);
            m_breakLabel = {padding, y, width - padding, y + label};
            y += label;
            placeWheels(kWheelBreak, y);
            y += wheelHeight + std::round(22 * s);
        } else {
            const float ring = std::round(170 * s);
            m_ring = {std::round((width - ring) / 2), y,
                      std::round((width + ring) / 2), y + ring};
            y = m_ring.bottom + std::round(14 * s);
        }
        // The round buttons, at the sides; with the ring, at its lower
        // corners, like iOS's Clock (clear of its curve there).
        const float round = std::round(76 * s);
        float top = y;
        if (m_ring.right > m_ring.left) {
            top = m_ring.bottom - std::round(round * 0.4f);
        }
        m_parts[kSecondary] = {padding, top, padding + round, top + round};
        m_parts[kPrimary] = {width - padding - round, top, width - padding,
                             top + round};
        y = std::max(y, top + round + std::round(4 * s)) + padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    int HitTest(POINT pt) const {
        for (int i = 0; i < kPartCount; i++) {
            if (m_parts[i].right > m_parts[i].left &&
                PointInRect(pt, m_parts[i])) {
                return i;
            }
        }
        return -1;
    }

    winrt::com_ptr<IDWriteTextFormat> m_timeFormat;
    winrt::com_ptr<IDWriteTextFormat> m_ringFormat;
    winrt::com_ptr<IDWriteTextFormat> m_wheelFormat;
    winrt::com_ptr<IDWriteTextFormat> m_wheelSideFormat;
    winrt::com_ptr<IDWriteTextFormat> m_alarmFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_leftTextFormat;
    winrt::com_ptr<IDWriteTextFormat> m_buttonFormat;
    winrt::com_ptr<IDWriteTextFormat> m_roundButtonFormat;
    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_bigIconFormat;
    winrt::com_ptr<IDWriteTypography> m_tabular;
    winrt::com_ptr<ID2D1StrokeStyle> m_roundCaps;
    float m_preparedScale = 0;
    D2D1_RECT_F m_parts[kPartCount]{};
    D2D1_RECT_F m_alarmRows[kMaxAlarms]{};
    // The alarm just saved (its minute * 2 + its sound): it comes in.
    long long m_savedAlarm = -1;
    D2D1_RECT_F m_tabsRect{}, m_ring{}, m_bigTime{}, m_empty{}, m_colon{},
        m_focusLabel{}, m_breakLabel{}, m_daysLabel{};
    // Shown as the Live Activities (see ShowActivity): the ones going in
    // their order, their rings, and the next alarm (-1 none) and its row.
    bool m_activity = false;
    std::vector<TimerMode> m_activityModes;
    D2D1_RECT_F m_activityRings[kTimerModes]{};
    int m_activityAlarm = -1;
    D2D1_RECT_F m_activityAlarmRow{};
    // The alarm being edited on the wheels (-1: a new one).
    int m_editing = -1;
    // Its days and its name (typed in while m_naming).
    int m_newDays = kAlarmEveryDay;
    std::wstring m_newName;
    TextEdit m_nameEdit;
    bool m_naming = false;
    // The tab shown, the lit one sliding to it, and its page coming in
    // (from the right going to a later tab).
    int m_tab = kTabTimer;
    double m_tabKnob = 0;
    double m_tabKnobVelocity = 0;
    double m_pageIn = 1;
    double m_pageInVelocity = 0;
    float m_pageDirection = 1;
    // The wheels: each one's roll (rows, back to 0), the one being typed and
    // what's typed, the one dragged (from where, its value then, moved), and
    // the mouse wheel's turns not a step yet.
    std::array<double, kWheelCount> m_wheelOffset{};
    std::array<double, kWheelCount> m_wheelVelocity{};
    int m_typing = -1;
    std::wstring m_typed;
    int m_dragWheel = -1;
    LONG m_dragY = 0;
    int m_dragValue = 0;
    bool m_dragMoved = false;
    int m_wheelSpin = 0;
    // A new alarm being made: its time and its sound.
    bool m_adding = false;
    int m_newMinute = 7 * 60;
    bool m_newSound = true;
    int m_hover = -1;
};

