#pragma once
// Their cards in the settings: how tall (DIPs), and how many side by side
// (two once each has room for its icon, name and switch).
constexpr float kLyricsAppCard = 46;
size_t LyricsAppsColumns(float width, float scale) {
    return width >= std::round(400 * scale) ? 2 : 1;
}

std::vector<LyricsApp>& LyricsApps() {
    if (!g_lyricsAppsRead) {
        g_lyricsAppsRead = true;
        g_lyricsApps.clear();
        // A line each: "1|<app ID>" (1: off, 0: on).
        const std::wstring text =
            GetUserStringValue(L"lyricsApps", kLyricsAppsRoom);
        size_t start = 0;
        while (start < text.size() && g_lyricsApps.size() < kLyricsAppsKept) {
            size_t end = text.find(L'\n', start);
            if (end == std::wstring::npos) {
                end = text.size();
            }
            const std::wstring_view line(text.data() + start, end - start);
            if (line.size() > 2 && line[1] == L'|') {
                g_lyricsApps.push_back(
                    {std::wstring(line.substr(2)), line[0] == L'1'});
            }
            start = end + 1;
        }
    }
    return g_lyricsApps;
}

void SaveLyricsApps() {
    std::wstring text;
    for (const auto& app : LyricsApps()) {
        text += (app.off ? L"1|" : L"0|") + app.appId + L"\n";
    }
    SetUserStringValue(L"lyricsApps", text.c_str());
}

bool LyricsAppOff(const std::wstring& appId) {
    for (const auto& app : LyricsApps()) {
        if (app.appId == appId) {
            return app.off;
        }
    }
    return false;
}

// An app seen playing: listed, with its lyrics on. A full list lets the
// oldest one that's on go (the ones turned off stay: they matter). True if
// it's new there.
bool NoteLyricsApp(const std::wstring& appId) {
    auto& apps = LyricsApps();
    if (appId.empty() || appId.size() > MAX_PATH ||
        std::any_of(apps.begin(), apps.end(),
                    [&](const LyricsApp& app) { return app.appId == appId; })) {
        return false;
    }
    if (apps.size() >= kLyricsAppsKept) {
        const auto on = std::find_if(apps.begin(), apps.end(),
                                     [](const LyricsApp& app) { return !app.off; });
        if (on == apps.end()) {
            return false;
        }
        apps.erase(on);
    }
    apps.push_back({appId, false});
    SaveLyricsApps();
    return true;
}

// An LRC text's lines ("[mm:ss.xx] words"; a line may have several times),
// in time order; the empty ones kept (a pause in the song).
std::vector<LyricLine> ParseLrc(const std::wstring& text) {
    std::vector<LyricLine> lines;
    size_t start = 0;
    while (start <= text.size()) {
        size_t end = text.find(L'\n', start);
        if (end == std::wstring::npos) {
            end = text.size();
        }
        std::wstring_view line(text.data() + start, end - start);
        std::vector<double> times;
        while (line.size() > 2 && line[0] == L'[') {
            const size_t close = line.find(L']');
            if (close == std::wstring_view::npos) {
                break;
            }
            int minutes = 0;
            double seconds = 0;
            const std::wstring stamp(line.substr(1, close - 1));
            // (Times that make sense only: no NaN to sort.)
            if (swscanf_s(stamp.c_str(), L"%d:%lf", &minutes, &seconds) == 2) {
                const double at = minutes * 60.0 + seconds;
                if (std::isfinite(at) && at >= 0 && at < 24 * 60 * 60) {
                    times.push_back(at);
                }
            }
            line.remove_prefix(close + 1);
        }
        std::wstring words(line);
        while (!words.empty() &&
               (words.back() == L'\r' || iswspace(words.back()))) {
            words.pop_back();
        }
        words.erase(words.begin(),
                    std::find_if_not(words.begin(), words.end(), [](WCHAR c) {
                        return iswspace(c) != 0;
                    }));
        words.resize(std::min(words.size(), kLyricsMaxLineLength));
        for (double at : times) {
            if (lines.size() < kLyricsMaxLines) {
                lines.push_back({at, words});
            }
        }
        start = end + 1;
    }
    std::stable_sort(lines.begin(), lines.end(),
                     [](const LyricLine& a, const LyricLine& b) {
                         return a.at < b.at;
                     });
    return lines;
}

// A line shows this much before its time (seconds): it takes a moment to
// come in, and comes with the voice rather than after it.
constexpr double kLyricsLead = 0.25;

// The line sung at `at` (seconds): its index, -1 before the first.
int LyricLineAt(const SongLyrics& lyrics, double at) {
    if (!lyrics.synced) {
        return -1;
    }
    int index = -1;
    for (size_t i = 0; i < lyrics.lines.size(); i++) {
        if (lyrics.lines[i].at > at + 0.05) {
            break;
        }
        index = (int)i;
    }
    return index;
}

// A song's lyrics as kept (none yet: false).
bool CopyLyrics(const std::wstring& key, SongLyrics* lyrics) {
    AcquireSRWLockShared(&g_lyricsLock);
    bool found = false;
    for (const auto& kept : g_lyricsKept) {
        // (Not reached a while ago: looked for again.)
        if (kept.key == key &&
            !(kept.failed &&
              NowSeconds() - kept.failedAt > kLyricsRetrySeconds)) {
            *lyrics = kept;
            found = true;
        }
    }
    ReleaseSRWLockShared(&g_lyricsLock);
    return found;
}

// Set as the island goes: a search under way gives up (see HttpsGet), its
// request closed at once (g_lyricsCancel).
std::atomic<bool> g_lyricsStop{false};
HttpsCancel g_lyricsCancel;

struct LyricsRequest {
    HWND notify;
    std::wstring key;
    std::wstring title;
    std::wstring artist;
    double duration;
};

DWORD WINAPI LyricsThreadProc(LPVOID parameter) {
    std::unique_ptr<LyricsRequest> request((LyricsRequest*)parameter);
    SongLyrics lyrics;
    lyrics.key = request->key;
    auto take = [&](const Json& song) {
        if (song.type != Json::Type::Object) {
            return false;
        }
        const Json& instrumental = song[L"instrumental"];
        lyrics.instrumental =
            instrumental.type == Json::Type::Bool && instrumental.boolean;
        lyrics.lines = ParseLrc(song[L"syncedLyrics"].string);
        lyrics.synced = !lyrics.lines.empty();
        if (!lyrics.synced) {
            // Only the words: a line each, without times.
            lyrics.lines.clear();
            const std::wstring& plain = song[L"plainLyrics"].string;
            size_t start = 0;
            while (start < plain.size()) {
                size_t end = plain.find(L'\n', start);
                if (end == std::wstring::npos) {
                    end = plain.size();
                }
                std::wstring words = plain.substr(start, end - start);
                if (!words.empty() && words.back() == L'\r') {
                    words.pop_back();
                }
                words.resize(std::min(words.size(), kLyricsMaxLineLength));
                if (lyrics.lines.size() < kLyricsMaxLines) {
                    lyrics.lines.push_back({-1, words});
                }
                start = end + 1;
            }
        }
        lyrics.found = !lyrics.lines.empty() || lyrics.instrumental;
        return lyrics.found;
    };
    const std::wstring query = L"artist_name=" + UrlEncode(request->artist) +
                               L"&track_name=" + UrlEncode(request->title);
    Json song;
    if (request->duration > 0 &&
        HttpsGetJson(L"https://lrclib.net/api/get?" + query + L"&duration=" +
                         std::to_wstring(std::lround(request->duration)),
                     &song, &g_lyricsStop, &g_lyricsCancel)) {
        take(song);
    }
    // Not found that way (another length, a live version): the closest
    // match, one with times first. (Its answer not had: not reached, unless
    // the first one found them.)
    Json results;
    if (!lyrics.found && !g_lyricsStop) {
        if (HttpsGetJson(L"https://lrclib.net/api/search?" + query, &results,
                         &g_lyricsStop, &g_lyricsCancel)) {
            for (int pass = 0; pass < 2 && !lyrics.found; pass++) {
                for (size_t i = 0; i < results.Size() && !lyrics.found; i++) {
                    if (pass == 0 &&
                        results[i][L"syncedLyrics"].string.empty()) {
                        continue;
                    }
                    // (Another song's never: none rather than wrong ones.)
                    if (!LyricsResultMatches(results[i], request->title,
                                             request->artist,
                                             request->duration)) {
                        continue;
                    }
                    take(results[i]);
                }
            }
        } else {
            lyrics.failed = true;
            lyrics.failedAt = NowSeconds();
        }
    }
    // (The island going: nothing to tell.)
    if (g_lyricsStop) {
        return 0;
    }
    AcquireSRWLockExclusive(&g_lyricsLock);
    std::erase_if(g_lyricsKept, [&](const SongLyrics& kept) {
        return kept.key == lyrics.key;
    });
    g_lyricsKept.push_back(std::move(lyrics));
    if (g_lyricsKept.size() > kLyricsKept) {
        g_lyricsKept.erase(g_lyricsKept.begin());
    }
    ReleaseSRWLockExclusive(&g_lyricsLock);
    PostMessage(request->notify, WM_APP_LYRICS, 0, 0);
    return 0;
}

// The color chosen for them (islandLyricsColor): the text's, Windows'
// accent, or their own.
D2D1_COLOR_F LyricsColor(float alpha) {
    switch (g_settings.islandLyricsColor) {
        case 1:
            return RgbColor(WindowsAccentColor(), alpha);
        case 2:
            return RgbColor(g_settings.islandLyricsOwnColor, alpha);
        default:
            return Fg(alpha);
    }
}

// The song's lyrics, all of them: the line sung now lit and big, kept in
// the middle as the song goes (the wheel looks around for a moment), a line
// clicked goes there in the song.
class LyricsPanel : public Panel {
   public:
    void SetSong(const MediaState& media,
                 const SongLyrics& lyrics,
                 bool searching) {
        const bool other = lyrics.key != m_lyrics.key ||
                           lyrics.lines.size() != m_lyrics.lines.size();
        m_media = media;
        m_lyrics = lyrics;
        m_searching = searching;
        // (Other lines, another song's or found now: the old ones' places
        // go, so a click while it closes finds none; laid out again below,
        // or when it opens.)
        if (other) {
            m_userScrollAt = 0;
            m_current = -2;
            m_hover = -1;
            m_lineRects.clear();
        }
        if (IsOpen()) {
            Layout();
            // (Paused, played, moved in the song: the next line again.)
            ArmLineTimer();
            StartIslandAnimation();
        }
    }

   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_titleFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 13 * scale,
                           DWRITE_TEXT_ALIGNMENT_LEADING);
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_lineFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           15 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_nowFormat = MakeFormat(L"Segoe UI Variable Display",
                                     DWRITE_FONT_WEIGHT_SEMI_BOLD, 19 * scale,
                                     DWRITE_TEXT_ALIGNMENT_LEADING);
            for (auto* format : {m_lineFormat.get(), m_nowFormat.get()}) {
                if (format) {
                    format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
                    format->SetParagraphAlignment(
                        DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
                }
            }
        }
        Layout();
    }

    void OnOpen() override {
        m_hover = -1;
        m_userScrollAt = 0;
        m_current = -2;
        Layout();
        FollowLine();
        m_scroll = m_scrollTarget;
        m_scrollVelocity = 0;
        ArmLineTimer();
    }

    void OnClose() override {
        KillTimer(m_hwnd, kLineTimerId);
        KillTimer(m_hwnd, kBackTimerId);
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override {
        if (!m_titleFormat || !m_textFormat || !m_lineFormat || !m_nowFormat) {
            return;
        }
        const float s = m_scale;
        brush->SetColor(Fg(0.95f));
        target->DrawText(m_media.title.c_str(), (UINT32)m_media.title.size(),
                         m_titleFormat.get(), m_title, brush);
        brush->SetColor(Fg(0.6f));
        target->DrawText(m_media.artist.c_str(),
                         (UINT32)m_media.artist.size(), m_textFormat.get(),
                         m_artist, brush);
        // (Nothing to list: why.)
        PCWSTR empty = m_searching ? Tr(L"Looking for the lyrics…",
                                        L"Procurando a letra…")
                       : m_lyrics.skip == kLyricsSkipApp
                           ? Tr(L"Lyrics are off for this app (in the "
                                L"settings, What's playing)",
                                L"As letras estão desligadas para este app "
                                L"(nas configurações, O que está tocando)")
                       : m_lyrics.skip == kLyricsSkipNotSong
                           ? Tr(L"Lyrics are looked for only for songs (with "
                                L"an artist, 45 s to 20 min, not videos)",
                                L"As letras só são buscadas para músicas (com "
                                L"artista, de 45 s a 20 min, sem vídeos)")
                       : m_lyrics.failed
                           ? Tr(L"Couldn't reach lrclib.net",
                                L"Não deu para acessar o lrclib.net")
                       : m_lyrics.instrumental && m_lyrics.lines.empty()
                           ? Tr(L"Instrumental", L"Instrumental")
                       : m_lyrics.lines.empty()
                           ? Tr(L"No lyrics found for this song",
                                L"Nenhuma letra encontrada para esta música")
                           : nullptr;
        if (empty) {
            brush->SetColor(Fg(0.55f));
            target->DrawText(empty, (UINT32)wcslen(empty), m_textFormat.get(),
                             m_list, brush);
        }
        target->PushAxisAlignedClip(m_list, D2D1_ANTIALIAS_MODE_ALIASED);
        for (size_t i = 0; i < m_lineRects.size(); i++) {
            const float top = m_lineRects[i].top - (float)m_scroll;
            const float bottom = m_lineRects[i].bottom - (float)m_scroll;
            if (bottom < m_list.top || top > m_list.bottom) {
                continue;
            }
            const bool now = (int)i == m_current;
            const float lit =
                Hover(HoverKey(6900, (long long)i), (int)i == m_hover);
            const float sung = m_lyrics.synced && (int)i < m_current ? 0.4f
                                                                    : 0.65f;
            if (lit > 0.01f && m_lyrics.synced) {
                brush->SetColor(Fg(0.07f * lit));
                const float radius = std::round(8 * s);
                GlassFill(target,
                          {{m_list.left, top - std::round(3 * s), m_list.right,
                            bottom + std::round(3 * s)},
                           radius,
                           radius},
                          brush);
            }
            brush->SetColor(now ? LyricsColor(1) : Fg(sung + 0.2f * lit));
            // (Laid out once, see Layout: each frame only drawn.)
            IDWriteTextLayout* layout =
                now ? m_nowLayout.get()
                    : i < m_lineLayouts.size() ? m_lineLayouts[i].get()
                                               : nullptr;
            if (layout) {
                target->DrawTextLayout({m_list.left + std::round(8 * s), top},
                                       layout, brush);
            }
        }
        target->PopAxisAlignedClip();
        PCWSTR credit = Tr(L"Lyrics from lrclib.net", L"Letra do lrclib.net");
        brush->SetColor(Fg(0.4f));
        target->DrawText(credit, (UINT32)wcslen(credit), m_textFormat.get(),
                         m_credit, brush);
    }

    bool OnMouseMove(POINT pt) override {
        const int hover = HitLine(pt);
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

    // A line: the song goes there (and the lines follow it again).
    bool OnMouseUp(POINT pt) override {
        const int line = HitLine(pt);
        if (line < 0 || !m_lyrics.synced || !m_media.canSeek) {
            return false;
        }
        SendMediaCommand(MediaCommand::Seek, m_lyrics.lines[line].at + 0.05);
        m_media.position = m_lyrics.lines[line].at + 0.05;
        m_media.updatedAt = NowSeconds();
        m_userScrollAt = 0;
        ArmLineTimer();
        StartIslandAnimation();
        return true;
    }

    bool OnWheel(int delta) override {
        m_scrollTarget =
            std::clamp(m_scrollTarget - delta / 120.0 * 60 * m_scale, 0.0,
                       m_scrollMost);
        m_userScrollAt = NowSeconds();
        SetTimer(m_hwnd, kBackTimerId, kBackMs, nullptr);
        StartIslandAnimation();
        return false;
    }

    // When the next line comes (or a second, for the song moved), and a
    // moment after the wheel: the frames slide the list there.
    bool OnTimer(UINT_PTR id) override {
        if (id == kLineTimerId) {
            ArmLineTimer();
        } else if (id != kBackTimerId) {
            return false;
        }
        StartIslandAnimation();
        return false;
    }

    // Each frame while it moves (the panels' animation, at the screen's
    // pace): the line sung now, kept in the middle (not for a few seconds
    // after the wheel), sliding there.
    bool AnimateContents(double dt) override {
        FollowLine();
        return SpringTowards(m_scroll, m_scrollVelocity, m_scrollTarget, 0.45,
                             dt);
    }

   private:
    static constexpr UINT_PTR kLineTimerId = 1;
    static constexpr UINT_PTR kBackTimerId = 2;
    static constexpr UINT kBackMs = 3050;

    // The line sung now (laid out bigger), and the list's place to keep it
    // in the middle.
    void FollowLine() {
        const int current =
            LyricLineAt(m_lyrics, m_media.Now() + kLyricsLead);
        if (current != m_current) {
            m_current = current;
            Layout();
        }
        if (NowSeconds() - m_userScrollAt > 3 && m_current >= 0 &&
            m_current < (int)m_lineRects.size()) {
            const D2D1_RECT_F& line = m_lineRects[m_current];
            m_scrollTarget = std::clamp(
                (double)(line.top + line.bottom) / 2 -
                    (m_list.top + m_list.bottom) / 2,
                0.0, m_scrollMost);
        }
    }

    // A timer for the next line (at most a second away, for a song moved
    // meanwhile); none while it's paused.
    void ArmLineTimer() {
        KillTimer(m_hwnd, kLineTimerId);
        if (!m_media.playing || !m_lyrics.synced) {
            return;
        }
        const double now = m_media.Now() + kLyricsLead;
        const int current = LyricLineAt(m_lyrics, now);
        const double next = current + 1 < (int)m_lyrics.lines.size()
                                ? m_lyrics.lines[current + 1].at - now
                                : 1;
        SetTimer(m_hwnd, kLineTimerId,
                 (UINT)std::clamp(next * 1000 + 15, 15.0, 1000.0), nullptr);
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(400 * s);
        const float padding = std::round(16 * s);
        m_title = {padding, padding, width - padding, padding + std::round(20 * s)};
        m_artist = {padding, m_title.bottom, width - padding,
                    m_title.bottom + std::round(18 * s)};
        m_list = {padding, m_artist.bottom + std::round(12 * s),
                  width - padding,
                  m_artist.bottom + std::round(12 * s) + std::round(330 * s)};
        m_credit = {padding, m_list.bottom + std::round(8 * s), width - padding,
                    m_list.bottom + std::round(26 * s)};
        // Each line as tall as its words wrap (the one sung now bigger), in
        // the list's own coordinates from its top. Their layouts are kept
        // (made again for another song or size; the one sung now's for
        // another line): the panel only draws them, each frame.
        const float lineWidth = m_list.right - m_list.left - std::round(16 * s);
        if (m_layoutsKey != m_lyrics.key || m_layoutsScale != s ||
            m_lineLayouts.size() != m_lyrics.lines.size()) {
            m_layoutsKey = m_lyrics.key;
            m_layoutsScale = s;
            m_lineLayouts.assign(m_lyrics.lines.size(), nullptr);
            m_lineHeights.assign(m_lyrics.lines.size(), 0);
            m_nowLayout = nullptr;
            m_nowLayoutLine = -1;
        }
        auto lay = [&](size_t i, IDWriteTextFormat* format,
                       winrt::com_ptr<IDWriteTextLayout>& layout,
                       float least) {
            const std::wstring& text = m_lyrics.lines[i].text;
            const std::wstring shown = text.empty() ? L"♪" : text;
            float height = least;
            DWRITE_TEXT_METRICS metrics{};
            if (format &&
                SUCCEEDED(m_dwrite->CreateTextLayout(
                    shown.c_str(), (UINT32)shown.size(), format, lineWidth,
                    1000, layout.put())) &&
                SUCCEEDED(layout->GetMetrics(&metrics)) && !text.empty()) {
                height = std::max(height, std::ceil(metrics.height));
            }
            return height;
        };
        m_lineRects.clear();
        float y = m_list.top + (m_list.bottom - m_list.top) / 3;
        for (size_t i = 0; i < m_lyrics.lines.size(); i++) {
            const bool now = (int)i == m_current;
            if (!m_lineLayouts[i]) {
                m_lineHeights[i] = lay(i, m_lineFormat.get(), m_lineLayouts[i],
                                       std::round(21 * s));
            }
            float height = m_lineHeights[i];
            if (now) {
                if (m_nowLayoutLine != (int)i) {
                    m_nowLayout = nullptr;
                    m_nowLayoutLine = (int)i;
                    m_nowHeight = lay(i, m_nowFormat.get(), m_nowLayout,
                                      std::round(26 * s));
                }
                height = m_nowHeight;
            }
            m_lineRects.push_back({m_list.left, y, m_list.right, y + height});
            y += height + std::round(10 * s);
        }
        m_scrollMost = std::max(
            0.0, (double)y + (m_list.bottom - m_list.top) / 3 - m_list.bottom);
        m_scrollTarget = std::clamp(m_scrollTarget, 0.0, m_scrollMost);
        m_size = {(LONG)width, (LONG)std::ceil(m_credit.bottom + padding)};
    }

    int HitLine(POINT pt) const {
        if (!PointInRect(pt, m_list)) {
            return -1;
        }
        for (size_t i = 0; i < m_lineRects.size(); i++) {
            if (pt.y >= m_lineRects[i].top - m_scroll &&
                pt.y < m_lineRects[i].bottom - m_scroll) {
                return (int)i;
            }
        }
        return -1;
    }

    winrt::com_ptr<IDWriteTextFormat> m_titleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_lineFormat;
    winrt::com_ptr<IDWriteTextFormat> m_nowFormat;
    float m_preparedScale = 0;
    MediaState m_media;
    SongLyrics m_lyrics;
    bool m_searching = false;
    int m_current = -2;
    int m_hover = -1;
    std::vector<D2D1_RECT_F> m_lineRects;
    // The lines laid out (see Layout): for which song and size, each line's
    // and its height, and the one sung now's, bigger.
    std::wstring m_layoutsKey;
    float m_layoutsScale = 0;
    std::vector<winrt::com_ptr<IDWriteTextLayout>> m_lineLayouts;
    std::vector<float> m_lineHeights;
    winrt::com_ptr<IDWriteTextLayout> m_nowLayout;
    int m_nowLayoutLine = -1;
    float m_nowHeight = 0;
    double m_scroll = 0;
    double m_scrollVelocity = 0;
    double m_scrollTarget = 0;
    double m_scrollMost = 0;
    double m_userScrollAt = 0;
    D2D1_RECT_F m_title{}, m_artist{}, m_list{}, m_credit{};
};

