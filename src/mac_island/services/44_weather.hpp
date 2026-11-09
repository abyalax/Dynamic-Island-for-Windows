#pragma once
class WeatherPanel : public EditingPanel {
   public:
    // New weather came (`problem`: 1 city not found, 2 no answer, 3 location
    // not allowed, 4 location not found yet).
    void OnWeather(int problem) {
        m_searching = false;
        if (problem == 1) {
            m_status = Tr(L"City not found", L"Cidade não encontrada");
        } else if (problem == 2) {
            m_status = Tr(L"The weather service didn't answer",
                          L"O serviço do clima não respondeu");
        } else if (problem == 3) {
            m_status = Tr(L"Location off or not allowed in Windows",
                          L"Localização desligada ou não permitida no Windows");
        } else if (problem == 4) {
            m_status = Tr(L"Your location wasn't found yet, trying again",
                          L"Sua localização ainda não foi achada, tentando de novo");
        } else {
            m_status.clear();
            m_choosing = false;
        }
        if (IsOpen()) {
            m_data = GetWeather();
            PulseContents();
            Layout();
            Render();
        }
    }
    // Asks the island for the weather of a city typed, or of where the
    // computer is ("Use my location").
    std::function<void(const std::wstring&)> search;
    std::function<void()> locate;

   protected:
    void Prepare(float scale) override {
        PrepareFormats(scale);
        if (scale != m_weatherScale) {
            m_weatherScale = scale;
            m_bigFormat = MakeFormat(L"Segoe UI Variable Display",
                                     DWRITE_FONT_WEIGHT_SEMI_BOLD, 26 * scale);
            m_centerFormat = MakeFormat(L"Segoe UI Variable Text",
                                        DWRITE_FONT_WEIGHT_NORMAL, 12 * scale);
            m_centerBoldFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 13 * scale);
            m_emojiFormat = MakeFormat(L"Segoe UI Emoji",
                                       DWRITE_FONT_WEIGHT_NORMAL, 26 * scale);
            m_smallEmojiFormat = MakeFormat(
                L"Segoe UI Emoji", DWRITE_FONT_WEIGHT_NORMAL, 17 * scale);
            m_emojiBitmaps.clear();
            m_hoursBitmap = nullptr;
            m_hoursKey.clear();
        }
        m_data = GetWeather();
        Layout();
    }

    void OnOpen() override {
        m_data = GetWeather();
        m_hover = {};
        m_day = 0;
        m_hourScroll = m_hourScrollTarget = m_hourScrollVelocity = 0;
        m_choosing = !m_data.valid && !m_searching;
        m_cityEdit.caret = m_city.size();
        Layout();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

    bool OnMouseMove(POINT pt) override {
        int day = -1;
        const Part hover = HitTest(pt, &day);
        if (hover == m_hover && day == m_hoverDay) {
            return false;
        }
        m_hover = hover;
        m_hoverDay = day;
        return true;
    }
    bool OnWheel(int delta) override {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(m_hwnd, &pt);
        pt.x -= m_contentOffset.x;
        pt.y -= m_contentOffset.y;
        if (!PointInRect(pt, m_hoursRect)) {
            return false;
        }
        const double most =
            std::max(0.0, (double)DayHours().size() - kHoursShown);
        m_hourScrollTarget = std::clamp(
            m_hourScrollTarget - (double)delta / WHEEL_DELTA * 2, 0.0, most);
        StartIslandAnimation();
        return true;
    }
    bool OnMouseLeave() override {
        m_hover = Part::None;
        return true;
    }
    bool OnMouseDown(POINT pt) override {
        if (m_choosing && PointInRect(pt, m_cityBox)) {
            EditTextClick(m_city, m_cityEdit, pt);
            return true;
        }
        return false;
    }
    bool OnMouseUp(POINT pt) override {
        int day = -1;
        switch (HitTest(pt, &day)) {
            case Part::Day:
                // Its hours below, from its start (now, for today).
                if (day != m_day) {
                    m_day = day;
                    m_hourScroll = m_hourScrollTarget = 0;
                    m_hourScrollVelocity = 0;
                    m_daySwitch = 1;
                    StartIslandAnimation();
                }
                return true;
            case Part::Change:
                m_choosing = !m_choosing || !m_data.valid;
                m_status.clear();
                g_lastTyped = NowSeconds();
                PulseContents();
                Layout();
                return true;
            case Part::Search:
                Search();
                return true;
            case Part::Locate:
                if (!m_searching && locate) {
                    m_searching = true;
                    m_status = Tr(L"Finding your location…",
                                  L"Procurando sua localização…");
                    locate();
                    Layout();
                }
                return true;
            default:
                return false;
        }
    }
    bool OnKey(WPARAM key) override {
        if (!m_choosing) {
            return false;
        }
        if (key == VK_ESCAPE && m_data.valid) {
            m_choosing = false;
            Layout();
            return true;
        }
        return EditTextKey(m_city, m_cityEdit, key);
    }
    bool OnChar(WCHAR c) override {
        if (!m_choosing) {
            return false;
        }
        if (c == L'\r' || c == 0x0A) {
            Search();
            return true;
        }
        if (c == 0x1B) {
            return false;
        }
        return c != L'\n' && EditTextChar(m_city, m_cityEdit, c, 80, m_hwnd);
    }
    bool AnimateContents(double dt) override {
        const bool scrolling =
            SpringTowards(m_hourScroll, m_hourScrollVelocity,
                          m_hourScrollTarget, 0.32, dt);
        // Another day's hours fade in.
        const bool switching = m_daySwitch > 0;
        if (switching) {
            m_daySwitch = std::max(0.0, m_daySwitch - dt / 0.3);
        }
        return AnimateTyping(m_choosing, scrolling || switching);
    }

   private:
    enum class Part { None, Change, Search, Locate, Day };
    // How many hours show at once.
    static constexpr int kHoursShown = 8;

    // The chosen day's hours (today's from now).
    std::vector<const WeatherHour*> DayHours() const {
        std::vector<const WeatherHour*> hours;
        if (m_day < 0 || m_day >= (int)m_data.days.size()) {
            return hours;
        }
        const std::wstring& date = m_data.days[m_day].date;
        const std::wstring nowHour = m_data.now.substr(0, 13);
        for (const auto& hour : m_data.hours) {
            if (hour.time.compare(0, date.size(), date) != 0) {
                continue;
            }
            if (m_day == 0 && !nowHour.empty() &&
                hour.time.substr(0, 13) < nowHour) {
                continue;
            }
            hours.push_back(&hour);
        }
        return hours;
    }

    void Search() {
        if (m_city.find_first_not_of(L' ') == std::wstring::npos ||
            m_searching || !search) {
            return;
        }
        m_searching = true;
        m_status = Tr(L"Looking…", L"Procurando…");
        search(m_city);
        Layout();
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(kWeatherWidth * s);
        const float padding = std::round(14 * s);
        const float gap = std::round(8 * s);
        float y = padding;
        m_change = {padding, y, padding + std::round(32 * s),
                    y + std::round(32 * s)};
        if (m_choosing || !m_data.valid) {
            y += std::round(40 * s);
            m_cityBox = {padding, y, width - padding - std::round(110 * s),
                         y + std::round(36 * s)};
            m_search = {m_cityBox.right + gap, y, width - padding,
                        y + std::round(36 * s)};
            y += std::round(36 * s) + gap;
            // "Use my location" under them (Windows' own location, off
            // until clicked).
            m_locate = {padding, y, width - padding, y + std::round(30 * s)};
            y += std::round(30 * s) + gap;
            m_statusRect = {padding, y, width - padding, y + std::round(22 * s)};
            y += std::round(22 * s);
        } else {
            m_cityBox = m_search = m_locate = m_statusRect = {};
        }
        if (m_data.valid) {
            m_title = {padding, y, width - padding, y + std::round(40 * s)};
            y += std::round(40 * s);
            m_sun = {padding, y, width - padding, y + std::round(24 * s)};
            y += std::round(26 * s);
            // (As cards, islandWeatherCards: two rows of four.)
            const float details = g_settings.islandWeatherCards
                                      ? std::round(110 * s)
                                      : std::round(36 * s);
            m_details = {padding, y, width - padding, y + details};
            y += details + gap;
            // The week: a card a day.
            m_dayCards.clear();
            const size_t days = std::min<size_t>(m_data.days.size(), 7);
            const float cardGap = std::round(6 * s);
            const float card =
                days ? (width - padding * 2 - cardGap * (days - 1)) / days : 0;
            for (size_t i = 0; i < days; i++) {
                const float left = padding + (card + cardGap) * i;
                m_dayCards.push_back({std::round(left), y,
                                      std::round(left + card),
                                      y + std::round(118 * s)});
            }
            y += std::round(118 * s) + gap * 2;
            // The next hours.
            m_hoursRect = {padding, y, width - padding, y + std::round(122 * s)};
            y += std::round(122 * s) + gap;
            m_credit = {padding, y, width - padding, y + std::round(18 * s)};
            y += std::round(18 * s);
        }
        y += padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    Part HitTest(POINT pt, int* day) const {
        if (m_data.valid && PointInRect(pt, m_change)) {
            return Part::Change;
        }
        if (PointInRect(pt, m_search)) {
            return Part::Search;
        }
        if (PointInRect(pt, m_locate)) {
            return Part::Locate;
        }
        for (size_t i = 0; i < m_dayCards.size(); i++) {
            if (PointInRect(pt, m_dayCards[i])) {
                *day = (int)i;
                return Part::Day;
            }
        }
        return Part::None;
    }

    // Color emojis are slow to draw (every frame of a scroll drew dozens):
    // each one is drawn once into a bitmap, then the bitmap is drawn.
    void DrawEmoji(ID2D1RenderTarget* target,
                   ID2D1SolidColorBrush* brush,
                   const std::wstring& emoji,
                   IDWriteTextFormat* format,
                   const D2D1_RECT_F& rect) {
        const float width = std::ceil(rect.right - rect.left);
        const float height = std::ceil(rect.bottom - rect.top);
        if (width < 1 || height < 1) {
            return;
        }
        const std::wstring key =
            emoji + L"|" + std::to_wstring((uintptr_t)format) + L"|" +
            std::to_wstring((int)width) + L"x" + std::to_wstring((int)height) +
            L"|" + std::to_wstring((int)(g_theme.light * 10));
        ID2D1Bitmap* bitmap = nullptr;
        for (const auto& cached : m_emojiBitmaps) {
            if (cached.key == key) {
                bitmap = cached.bitmap.get();
            }
        }
        if (!bitmap) {
            winrt::com_ptr<ID2D1BitmapRenderTarget> emojiTarget;
            winrt::com_ptr<ID2D1Bitmap> made;
            if (SUCCEEDED(target->CreateCompatibleRenderTarget(
                    {width, height}, emojiTarget.put()))) {
                emojiTarget->SetTextAntialiasMode(
                    target->GetTextAntialiasMode());
                emojiTarget->BeginDraw();
                emojiTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
                brush->SetColor(Fg(1));
                emojiTarget->DrawText(
                    emoji.c_str(), (UINT32)emoji.size(), format,
                    {0, 0, width, height}, brush,
                    D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
                if (SUCCEEDED(emojiTarget->EndDraw())) {
                    emojiTarget->GetBitmap(made.put());
                }
            }
            if (!made) {
                brush->SetColor(Fg(1));
                target->DrawText(emoji.c_str(), (UINT32)emoji.size(), format,
                                 rect, brush,
                                 D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
                return;
            }
            if (m_emojiBitmaps.size() >= 64) {
                m_emojiBitmaps.clear();
            }
            bitmap = made.get();
            m_emojiBitmaps.push_back({key, std::move(made)});
        }
        const float left = std::round(rect.left);
        const float top = std::round(rect.top);
        DrawBitmapSmooth(target, bitmap,
                         {left, top, left + width, top + height});
    }

    // The chosen day's hours as one long strip (drawn once per day, size
    // and colors; scrolling only moves it).
    void DrawHoursStrip(ID2D1RenderTarget* target,
                        ID2D1SolidColorBrush* brush,
                        const std::vector<const WeatherHour*>& dayHours,
                        float column,
                        float stripHeight);

    struct EmojiBitmap {
        std::wstring key;
        winrt::com_ptr<ID2D1Bitmap> bitmap;
    };
    std::vector<EmojiBitmap> m_emojiBitmaps;
    winrt::com_ptr<ID2D1Bitmap> m_hoursBitmap;
    std::wstring m_hoursKey;

    WeatherData m_data;
    // The day chosen, and where its hours are scrolled (in hours).
    int m_day = 0;
    int m_hoverDay = -1;
    double m_hourScroll = 0;
    double m_hourScrollTarget = 0;
    double m_hourScrollVelocity = 0;
    double m_daySwitch = 0;
    std::wstring m_city;
    TextEdit m_cityEdit;
    bool m_choosing = false;
    bool m_searching = false;
    std::wstring m_status;
    Part m_hover = Part::None;
    float m_weatherScale = 0;
    winrt::com_ptr<IDWriteTextFormat> m_bigFormat;
    winrt::com_ptr<IDWriteTextFormat> m_centerFormat;
    winrt::com_ptr<IDWriteTextFormat> m_centerBoldFormat;
    winrt::com_ptr<IDWriteTextFormat> m_emojiFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallEmojiFormat;
    D2D1_RECT_F m_change{}, m_cityBox{}, m_search{}, m_locate{}, m_statusRect{},
        m_title{}, m_sun{}, m_details{}, m_hoursRect{}, m_credit{};
    std::vector<D2D1_RECT_F> m_dayCards;
};

void WeatherPanel::Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
    if (!Ready() || !m_bigFormat || !m_centerFormat || !m_centerBoldFormat ||
        !m_emojiFormat || !m_smallEmojiFormat) {
        return;
    }
    const float s = m_scale;
    const WeatherData& d = m_data;
    // The city's button (changes it).
    if (d.valid) {
        const float lit = Hover(m_change, m_hover == Part::Change);
        const float radius = std::round(8 * s);
        brush->SetColor(Fg(0.06f + 0.1f * lit));
        GlassFill(target, {m_change, radius, radius}, brush);
        const WCHAR pin = 0xE707;
        brush->SetColor(Fg(0.8f));
        DrawGlyph(target, &pin, 1, m_iconFormat.get(), m_change, brush);
    }
    if (m_choosing || !d.valid) {
        DrawTextBox(target, brush, m_cityBox, m_city, m_cityEdit,
                    Tr(L"Your city (like Campinas)",
                       L"Sua cidade (como Campinas)"),
                    true);
        DrawButton(target, brush, m_search, Tr(L"Search", L"Buscar"), true,
                   m_hover == Part::Search, m_searching ? 0.5f : 1.0f);
        DrawButton(target, brush, m_locate,
                   Tr(L"Use my location", L"Usar minha localização"), false,
                   m_hover == Part::Locate, m_searching ? 0.5f : 1.0f);
        brush->SetColor(Fg(0.55f));
        target->DrawText(m_status.c_str(), (UINT32)m_status.size(),
                         m_smallFormat.get(), m_statusRect, brush);
    }
    if (!d.valid) {
        return;
    }
    // "Campinas 19°".
    const std::wstring title = d.city + L"  " + Degrees(d.temperature) +
                               (UsesFahrenheit() ? L"F" : L"C");
    brush->SetColor(Fg(0.95f));
    target->DrawText(title.c_str(), (UINT32)title.size(), m_bigFormat.get(),
                     m_title, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    // Sunrise and sunset.
    auto clock = [](const std::wstring& time) {
        const size_t t = time.find(L'T');
        return t == std::wstring::npos ? std::wstring() : time.substr(t + 1, 5);
    };
    const std::wstring sun = L"☀️ " + clock(d.sunrise) +
                             L"     \U0001F319 " + clock(d.sunset);
    brush->SetColor(D2D1::ColorF(1.0f, 0.82f, 0.35f, 1));
    target->DrawText(sun.c_str(), (UINT32)sun.size(), m_centerBoldFormat.get(),
                     m_sun, brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    // The details as cards (islandWeatherCards, #6129): a name over its
    // value, two rows of four.
    if (g_settings.islandWeatherCards) {
        WCHAR value[64];
        std::vector<std::pair<std::wstring, std::wstring>> cards;
        cards.push_back({Tr(L"Feels like", L"Sensação de"),
                         Degrees(d.feelsLike)});
        swprintf_s(value, L"%d%%", d.humidity);
        cards.push_back({Tr(L"Humidity", L"Umidade"), value});
        swprintf_s(value, d.imperial ? L"%.0f mph" : L"%.0f km/h", d.wind);
        cards.push_back({Tr(L"Wind", L"Vento"), value});
        if (d.uv >= 0) {
            swprintf_s(value, L"%.1f", d.uv);
        } else {
            wcscpy_s(value, L"–");
        }
        cards.push_back({Tr(L"UV index", L"Índice UV"), value});
        if (d.rainChance >= 0) {
            swprintf_s(value, L"%d%%", d.rainChance);
        } else {
            wcscpy_s(value, L"–");
        }
        cards.push_back({Tr(L"Rain chance", L"Chance de chuva"), value});
        swprintf_s(value, L"%.0f hPa", d.pressure);
        cards.push_back({Tr(L"Pressure", L"Pressão"), value});
        swprintf_s(value, L"%d%%", d.clouds);
        cards.push_back({Tr(L"Clouds", L"Nuvens"), value});
        cards.push_back({Tr(L"Sky", L"Céu"), WeatherWords(d.code)});
        const float gap = std::round(6 * s);
        const float cardWidth =
            (m_details.right - m_details.left - gap * 3) / 4;
        const float cardHeight = (m_details.bottom - m_details.top - gap) / 2;
        const float radius = std::round(12 * s);
        const float inner = std::round(8 * s);
        for (size_t i = 0; i < cards.size(); i++) {
            const float left =
                std::round(m_details.left + (cardWidth + gap) * (i % 4));
            const float top =
                std::round(m_details.top + (cardHeight + gap) * (i / 4));
            const D2D1_RECT_F card{left, top, std::round(left + cardWidth),
                                   std::round(top + cardHeight)};
            brush->SetColor(Fg(0.06f));
            GlassFill(target, {card, radius, radius}, brush);
            brush->SetColor(Fg(0.55f));
            target->DrawText(cards[i].first.c_str(),
                             (UINT32)cards[i].first.size(),
                             m_smallFormat.get(),
                             {card.left + inner, card.top + inner,
                              card.right - inner,
                              card.top + inner + std::round(16 * s)},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            brush->SetColor(Fg(0.95f));
            target->DrawText(cards[i].second.c_str(),
                             (UINT32)cards[i].second.size(),
                             m_boldFormat.get(),
                             {card.left + inner, card.bottom - inner -
                                                     std::round(20 * s),
                              card.right - inner, card.bottom - inner},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    } else {
        // The details, on two lines.
        WCHAR line1[256], line2[256];
        swprintf_s(line1, L"%ls %ls  •  %ls  •  %ls %d%%  •  %ls %.0f hPa",
                   Tr(L"Feels like", L"Sensação de"),
                   Degrees(d.feelsLike).c_str(), WeatherWords(d.code),
                   Tr(L"Humidity", L"Umidade"), d.humidity,
                   Tr(L"Pressure", L"Pressão"), d.pressure);
        std::wstring second = std::wstring(Tr(L"Clouds", L"Nuvens")) + L" " +
                              std::to_wstring(d.clouds) + L"%";
        if (d.rainChance >= 0) {
            second += std::wstring(L"  •  ") +
                      Tr(L"Rain chance", L"Chance de chuva") + L" " +
                      std::to_wstring(d.rainChance) + L"%";
        }
        if (d.uv >= 0) {
            swprintf_s(line2, L"  •  %ls %.1f", Tr(L"UV index", L"Índice UV"),
                       d.uv);
            second += line2;
        }
        brush->SetColor(Fg(0.6f));
        target->DrawText(line1, (UINT32)wcslen(line1), m_centerFormat.get(),
                         {m_details.left, m_details.top, m_details.right,
                          m_details.top + std::round(18 * s)},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        target->DrawText(second.c_str(), (UINT32)second.size(),
                         m_centerFormat.get(),
                         {m_details.left, m_details.top + std::round(18 * s),
                          m_details.right, m_details.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    // The week.
    for (size_t i = 0; i < m_dayCards.size() && i < d.days.size(); i++) {
        const D2D1_RECT_F& card = m_dayCards[i];
        const WeatherDay& day = d.days[i];
        const float radius = std::round(12 * s);
        const bool chosen = (int)i == m_day;
        const float lit = Hover(card, m_hover == Part::Day &&
                                          m_hoverDay == (int)i);
        brush->SetColor(Fg((chosen ? 0.14f : 0.06f) + 0.05f * lit));
        GlassFill(target, {card, radius, radius}, brush);
        if (chosen) {
            brush->SetColor(Accent(0.7f));
            target->DrawRoundedRectangle(
                {{card.left + 0.75f, card.top + 0.75f, card.right - 0.75f,
                  card.bottom - 0.75f},
                 radius,
                 radius},
                brush, 1.5f);
        }
        std::wstring name;
        if (i == 0) {
            name = Tr(L"Today", L"Hoje");
        } else if (i == 1) {
            name = Tr(L"Tomorrow", L"Amanhã");
        } else {
            SYSTEMTIME date{};
            if (swscanf_s(day.date.c_str(), L"%hu-%hu-%hu", &date.wYear,
                          &date.wMonth, &date.wDay) == 3) {
                WCHAR text[32] = L"";
                GetDateFormatEx(DateLocale(), 0, &date, L"dddd", text,
                                ARRAYSIZE(text), nullptr);
                name = text;
                // "segunda-feira": "segunda".
                const size_t dash = name.find(L'-');
                if (dash != std::wstring::npos) {
                    name = name.substr(0, dash);
                }
                if (!name.empty()) {
                    name[0] = towupper(name[0]);
                }
            }
        }
        brush->SetColor(Fg(0.75f));
        target->DrawText(name.c_str(), (UINT32)name.size(), m_centerFormat.get(),
                         {card.left, card.top + std::round(8 * s), card.right,
                          card.top + std::round(26 * s)},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        DrawEmoji(target, brush, WeatherEmoji(day.code, true),
                  m_emojiFormat.get(),
                  {card.left, card.top + std::round(28 * s), card.right,
                   card.top + std::round(66 * s)});
        const std::wstring high = Degrees(day.high);
        const std::wstring low = Degrees(day.low);
        brush->SetColor(Fg(0.95f));
        target->DrawText(high.c_str(), (UINT32)high.size(),
                         m_centerBoldFormat.get(),
                         {card.left, card.top + std::round(70 * s), card.right,
                          card.top + std::round(90 * s)},
                         brush);
        brush->SetColor(Fg(0.5f));
        target->DrawText(low.c_str(), (UINT32)low.size(), m_centerFormat.get(),
                         {card.left, card.top + std::round(92 * s), card.right,
                          card.top + std::round(110 * s)},
                         brush);
    }

    // The chosen day's hours (eight at a time, the wheel scrolls them):
    // their temperature as a line, their sky, wind and time, from a strip
    // drawn once (see DrawHoursStrip).
    const auto dayHours = DayHours();
    const size_t hours = dayHours.size();
    if (hours >= 2) {
        const D2D1_RECT_F& area = m_hoursRect;
        const float radius = std::round(12 * s);
        brush->SetColor(Fg(0.05f));
        GlassFill(target, {area, radius, radius}, brush);
        const float column =
            (area.right - area.left) / std::min<size_t>(hours, kHoursShown);
        const float scrolled = (float)m_hourScroll * column;
        const float stripWidth = std::ceil(column * hours);
        const float stripHeight = std::ceil(area.bottom - area.top);
        WCHAR key[160];
        swprintf_s(key, L"%d|%.3f|%zu|%.2f|%.2f|%.3f,%.3f,%.3f|%.2f|%ls",
                   m_day, m_data.fetchedAt, hours, column, s,
                   g_theme.foreground[0], g_theme.foreground[1],
                   g_theme.foreground[2], g_theme.light, m_data.now.c_str());
        if (!m_hoursBitmap || m_hoursKey != key) {
            m_hoursBitmap = nullptr;
            m_hoursKey.clear();
            winrt::com_ptr<ID2D1BitmapRenderTarget> stripTarget;
            if (SUCCEEDED(target->CreateCompatibleRenderTarget(
                    {stripWidth, stripHeight}, stripTarget.put()))) {
                stripTarget->SetTextAntialiasMode(
                    target->GetTextAntialiasMode());
                stripTarget->BeginDraw();
                stripTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
                DrawHoursStrip(stripTarget.get(), brush, dayHours, column,
                               stripHeight);
                if (SUCCEEDED(stripTarget->EndDraw()) &&
                    SUCCEEDED(stripTarget->GetBitmap(m_hoursBitmap.put()))) {
                    m_hoursKey = key;
                }
            }
        }
        winrt::com_ptr<ID2D1Layer> hoursLayer;
        target->CreateLayer(nullptr, hoursLayer.put());
        target->PushLayer(
            D2D1::LayerParameters(area, nullptr,
                                  D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                  D2D1::IdentityMatrix(),
                                  1 - 0.8f * (float)m_daySwitch),
            hoursLayer.get());
        if (m_hoursBitmap) {
            const float left = std::round(area.left - scrolled);
            DrawBitmapSmooth(
                target, m_hoursBitmap.get(),
                {left, area.top, left + stripWidth, area.top + stripHeight});
        }
        // How far along the day it's scrolled.
        if (hours > (size_t)kHoursShown) {
            const float track = area.right - area.left - radius * 2;
            const float thumb = track * kHoursShown / hours;
            const float x = area.left + radius +
                            (track - thumb) * (float)(m_hourScroll /
                                                      (hours - kHoursShown));
            brush->SetColor(Fg(0.25f));
            target->FillRoundedRectangle(
                {{x, area.bottom - std::round(5 * s), x + thumb,
                  area.bottom - std::round(2 * s)},
                 1.5f * s,
                 1.5f * s},
                brush);
        }
        target->PopLayer();
    }
    // Its source (as its terms ask).
    PCWSTR credit = Tr(L"Weather data by Open-Meteo.com",
                       L"Dados do clima: Open-Meteo.com");
    brush->SetColor(Fg(0.3f));
    target->DrawText(credit, (UINT32)wcslen(credit), m_centerFormat.get(),
                     m_credit, brush);
}

void WeatherPanel::DrawHoursStrip(
    ID2D1RenderTarget* target,
    ID2D1SolidColorBrush* brush,
    const std::vector<const WeatherHour*>& dayHours,
    float column,
    float stripHeight) {
    const float s = m_scale;
    const WeatherData& d = m_data;
    const size_t hours = dayHours.size();
    auto clock = [](const std::wstring& time) {
        const size_t t = time.find(L'T');
        return t == std::wstring::npos ? std::wstring() : time.substr(t + 1, 5);
    };
    double lowest = dayHours[0]->temperature;
    double highest = lowest;
    for (size_t i = 0; i < hours; i++) {
        lowest = std::min(lowest, dayHours[i]->temperature);
        highest = std::max(highest, dayHours[i]->temperature);
    }
    const float lineTop = std::round(28 * s);
    const float lineBottom = std::round(46 * s);
    auto pointOf = [&](size_t i) {
        const double range = std::max(highest - lowest, 1.0);
        const float y = lineBottom -
                        (lineBottom - lineTop) *
                            (float)((dayHours[i]->temperature - lowest) / range);
        return D2D1::Point2F(column * (i + 0.5f), y);
    };
    winrt::com_ptr<ID2D1Factory> factory;
    target->GetFactory(factory.put());
    winrt::com_ptr<ID2D1PathGeometry> path;
    winrt::com_ptr<ID2D1GeometrySink> sink;
    if (SUCCEEDED(factory->CreatePathGeometry(path.put())) &&
        SUCCEEDED(path->Open(sink.put()))) {
        sink->BeginFigure(pointOf(0), D2D1_FIGURE_BEGIN_HOLLOW);
        for (size_t i = 1; i < hours; i++) {
            sink->AddLine(pointOf(i));
        }
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        sink->Close();
        brush->SetColor(D2D1::ColorF(1.0f, 0.78f, 0.3f, 0.9f));
        target->DrawGeometry(path.get(), brush, std::max(1.0f, 1.5f * s));
    }
    for (size_t i = 0; i < hours; i++) {
        const WeatherHour& hour = *dayHours[i];
        const float left = column * i;
        const D2D1_POINT_2F point = pointOf(i);
        const std::wstring temperature = Degrees(hour.temperature);
        brush->SetColor(Fg(0.9f));
        target->DrawText(temperature.c_str(), (UINT32)temperature.size(),
                         m_centerFormat.get(),
                         {left, point.y - std::round(22 * s), left + column,
                          point.y - std::round(4 * s)},
                         brush);
        brush->SetColor(D2D1::ColorF(1.0f, 0.78f, 0.3f, 1));
        target->FillEllipse({point, 2.5f * s, 2.5f * s}, brush);
        DrawEmoji(target, brush, WeatherEmoji(hour.code, hour.day),
                  m_smallEmojiFormat.get(),
                  {left, std::round(52 * s), left + column, std::round(78 * s)});
        WCHAR wind[32];
        swprintf_s(wind, L"%.0f %ls", hour.wind, d.imperial ? L"mph" : L"km/h");
        brush->SetColor(Fg(0.6f));
        target->DrawText(wind, (UINT32)wcslen(wind), m_centerFormat.get(),
                         {left, std::round(80 * s), left + column,
                          std::round(97 * s)},
                         brush);
        const std::wstring when = i == 0 && m_day == 0
                                      ? std::wstring(Tr(L"Now", L"Agora"))
                                      : clock(hour.time);
        brush->SetColor(Fg(0.85f));
        target->DrawText(when.c_str(), (UINT32)when.size(),
                         m_centerBoldFormat.get(),
                         {left, std::round(98 * s), left + column,
                          std::min(stripHeight, std::round(116 * s))},
                         brush);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Updates, when chosen in the island's settings (off by default): the apps
// with a newer version in winget (Windows' package manager, its own "winget
// upgrade" list of its community source, run hidden; it only lists them and
// accepts nothing for the user), updated in a terminal where winget shows
// what it does and asks what it needs: one app, the ones ticked, or all.
// And the Windows updates waiting (the Windows Update Agent's documented COM
// interface, read only and offline: the result of Windows' own last scan;
// they're installed in Windows Update, which the panel opens). Looked for 20
// seconds after the island starts (unless they were a moment ago) and every
// 6 hours while the item is on the pill (or when its panel opens, if it's
// old), on a thread of their own.

struct AppUpdate {
    std::wstring name;
    std::wstring id;
    std::wstring version;
    std::wstring available;
    // 0: waiting, 4: opened in a terminal, 5: updated in the terminal but
    // still listed with the same version (an app that finishes its update
    // when it closes, like VS Code while open).
    int state = 0;
    // Ticked, to update with the others ticked.
    bool ticked = false;
    // Ignored (see g_ignoredUpdates): listed last, not counted.
    bool ignored = false;
};
struct UpdatesData {
    bool checking = false;
    bool checked = false;
    bool wingetMissing = false;
    // winget ran but couldn't list them (it says why in a terminal).
    bool wingetFailed = false;
    std::vector<AppUpdate> apps;
    std::vector<std::wstring> windows;
    double checkedAt = 0;
};
SRWLOCK g_updatesLock = SRWLOCK_INIT;
UpdatesData g_updates;
// Apps whose updates are ignored (their winget IDs, value "updatesIgnored"):
// like ones winget can't update, they'd keep the count on the pill forever.
// Under g_updatesLock.
std::vector<std::wstring> g_ignoredUpdates;
// Tells the island the updates changed.
constexpr UINT WM_APP_UPDATES = WM_APP + 68;
// Windhawk's settings changed (see WhTool_ModSettingsChanged).
constexpr UINT WM_APP_RELOAD_SETTINGS = WM_APP + 70;
