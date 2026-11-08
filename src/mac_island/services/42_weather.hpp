enum class Located { Found, NotAllowed, NotFound };

// Where the computer is, from Windows' own location (asked to the user by
// Windows: "Let desktop apps access your location"), rounded to about a
// kilometer before it goes anywhere. Only Windows saying no is "not allowed":
// a position that isn't there yet (the service starting at sign-in, no
// network) is "not found", to be tried again. The weather's thread (it waits
// up to 10 s, never the island's, and gives up when the island stops).
Located LocateMe(double* latitude, double* longitude) {
    namespace Geolocation = winrt::Windows::Devices::Geolocation;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    Located result = Located::NotFound;
    try {
        const auto access = Geolocation::Geolocator::RequestAccessAsync();
        if (WaitForWindows(access)) {
            const auto status = access.GetResults();
            if (status == Geolocation::GeolocationAccessStatus::Denied) {
                IslandLog(L"Location not allowed in Windows' settings");
                result = Located::NotAllowed;
            } else if (status == Geolocation::GeolocationAccessStatus::Allowed) {
                Geolocation::Geolocator locator;
                locator.DesiredAccuracy(Geolocation::PositionAccuracy::Default);
                const auto asked = locator.GetGeopositionAsync(
                    std::chrono::minutes(30), std::chrono::seconds(10));
                if (WaitForWindows(asked)) {
                    const auto position =
                        asked.GetResults().Coordinate().Point().Position();
                    *latitude = std::round(position.Latitude * 100) / 100;
                    *longitude = std::round(position.Longitude * 100) / 100;
                    result = Located::Found;
                }
            } else {
                IslandLog(L"Location: Windows hasn't answered yet");
            }
        }
    } catch (const winrt::hresult_error& error) {
        IslandLog(L"Location not found: %08X", (unsigned)error.code());
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    return result;
}

DWORD WINAPI WeatherThreadProc(LPVOID parameter) {
    std::unique_ptr<WeatherRequest> request((WeatherRequest*)parameter);
    WeatherPlace place = request->place;
    if (request->locate) {
        const Located located = LocateMe(&place.latitude, &place.longitude);
        if (g_weatherStop) {
            return 0;
        }
        if (located == Located::NotAllowed) {
            // Back to the city typed (see OnWeather's 3), unless "Use my
            // location" was clicked again meanwhile.
            if (g_weatherLocateClicks == request->clicks) {
                Wh_SetIntValue(L"weatherMyLocation", 0);
            }
            PostMessage(request->notify, WM_APP_WEATHER, 3, 0);
            return 0;
        }
        if (located == Located::NotFound) {
            // Kept chosen: asked again later (see OnWeather's 4); after a
            // few misses, the city typed (in `place`) meanwhile.
            if (++g_weatherLocateMisses < kWeatherLocateMisses ||
                place.name.empty()) {
                PostMessage(request->notify, WM_APP_WEATHER, 4, 0);
                return 0;
            }
            IslandLog(L"Location not found again: the city typed meanwhile");
        } else {
            g_weatherLocateMisses = 0;
            place.name = Tr(L"My location", L"Minha localização");
        }
    } else if (!request->search.empty()) {
        Json found;
        // (In the island's language, so cities are found by their names
        // there.)
        static constexpr PCWSTR kCodes[] = {L"en", L"pt", L"zh", L"de",
                                            L"es", L"ru", L"ja", L"vi"};
        const std::wstring language = kCodes[(int)IslandTextLanguage()];
        if (!HttpsGetJson(
                L"https://geocoding-api.open-meteo.com/v1/search?count=1&"
                L"format=json&language=" +
                    language + L"&name=" + UrlEncode(request->search),
                &found)) {
            PostMessage(request->notify, WM_APP_WEATHER, 2, 0);
            return 0;
        }
        const Json& city = found[L"results"][(size_t)0];
        if (city.type != Json::Type::Object) {
            PostMessage(request->notify, WM_APP_WEATHER, 1, 0);
            return 0;
        }
        place.name = city[L"name"].string;
        if (!city[L"admin1"].string.empty() &&
            city[L"admin1"].string != place.name) {
            place.name += L", " + city[L"admin1"].string;
        }
        place.latitude = city[L"latitude"].Number();
        place.longitude = city[L"longitude"].Number();
        SaveWeatherPlace(place);
    }
    const bool imperial = UsesImperialUnits();
    WCHAR where[96];
    swprintf_s(where, L"latitude=%.4f&longitude=%.4f", place.latitude,
               place.longitude);
    Json forecast;
    if (!HttpsGetJson(
            std::wstring(L"https://api.open-meteo.com/v1/forecast?") + where +
                L"&current=temperature_2m,apparent_temperature,relative_"
                L"humidity_2m,weather_code,cloud_cover,pressure_msl,wind_speed_"
                L"10m,is_day&hourly=temperature_2m,weather_code,wind_speed_10m,"
                L"is_day&daily=weather_code,temperature_2m_max,temperature_2m_"
                L"min,sunrise,sunset,uv_index_max,precipitation_probability_max"
                L"&timezone=auto&forecast_days=7" +
                (imperial ? L"&wind_speed_unit=mph" : L""),
            &forecast) ||
        forecast[L"current"].type != Json::Type::Object) {
        PostMessage(request->notify, WM_APP_WEATHER, 2, 0);
        return 0;
    }
    WeatherData data;
    data.valid = true;
    data.imperial = imperial;
    data.city = place.name;
    const Json& now = forecast[L"current"];
    data.temperature = now[L"temperature_2m"].Number();
    data.feelsLike = now[L"apparent_temperature"].Number();
    data.humidity = (int)now[L"relative_humidity_2m"].Number();
    data.code = (int)now[L"weather_code"].Number();
    data.day = now[L"is_day"].Number(1) != 0;
    data.clouds = (int)now[L"cloud_cover"].Number();
    data.pressure = now[L"pressure_msl"].Number();
    data.wind = now[L"wind_speed_10m"].Number();
    data.now = now[L"time"].string;
    const Json& hourly = forecast[L"hourly"];
    for (size_t i = 0; i < hourly[L"time"].Size(); i++) {
        WeatherHour hour;
        hour.time = hourly[L"time"][i].string;
        hour.temperature = hourly[L"temperature_2m"][i].Number();
        hour.code = (int)hourly[L"weather_code"][i].Number();
        hour.day = hourly[L"is_day"][i].Number(1) != 0;
        hour.wind = hourly[L"wind_speed_10m"][i].Number();
        data.hours.push_back(std::move(hour));
    }
    const Json& daily = forecast[L"daily"];
    for (size_t i = 0; i < daily[L"time"].Size(); i++) {
        WeatherDay day;
        day.date = daily[L"time"][i].string;
        day.code = (int)daily[L"weather_code"][i].Number();
        day.high = daily[L"temperature_2m_max"][i].Number();
        day.low = daily[L"temperature_2m_min"][i].Number();
        data.days.push_back(std::move(day));
    }
    if (daily[L"time"].Size() > 0) {
        data.sunrise = daily[L"sunrise"][(size_t)0].string;
        data.sunset = daily[L"sunset"][(size_t)0].string;
        data.uv = daily[L"uv_index_max"][(size_t)0].Number(-1);
        data.rainChance =
            (int)daily[L"precipitation_probability_max"][(size_t)0].Number(-1);
    }
    data.fetchedAt = NowSeconds();
    AcquireSRWLockExclusive(&g_weatherLock);
    g_weather = std::move(data);
    ReleaseSRWLockExclusive(&g_weatherLock);
    PostMessage(request->notify, WM_APP_WEATHER, 0, 0);
    return 0;
}

WeatherData GetWeather() {
    AcquireSRWLockShared(&g_weatherLock);
    WeatherData data = g_weather;
    ReleaseSRWLockShared(&g_weatherLock);
    // The city's time now: the one fetched, moved on by the time since, so
    // that after midnight (before the next fetch) the day that ended isn't
    // "today" anymore, and its hours are gone.
    SYSTEMTIME time{};
    FILETIME file;
    if (data.valid && data.fetchedAt > 0 &&
        swscanf_s(data.now.c_str(), L"%hu-%hu-%huT%hu:%hu", &time.wYear,
                  &time.wMonth, &time.wDay, &time.wHour,
                  &time.wMinute) == 5 &&
        SystemTimeToFileTime(&time, &file)) {
        ULARGE_INTEGER value{};
        value.LowPart = file.dwLowDateTime;
        value.HighPart = file.dwHighDateTime;
        value.QuadPart += (ULONGLONG)(
            std::max(0.0, NowSeconds() - data.fetchedAt) * 10000000);
        file.dwLowDateTime = value.LowPart;
        file.dwHighDateTime = value.HighPart;
        if (FileTimeToSystemTime(&file, &time)) {
            WCHAR text[32];
            swprintf_s(text, L"%04u-%02u-%02uT%02u:%02u", time.wYear,
                       time.wMonth, time.wDay, time.wHour, time.wMinute);
            data.now = text;
            const std::wstring today = data.now.substr(0, 10);
            std::erase_if(data.days, [&](const WeatherDay& day) {
                return day.date < today;
            });
        }
    }
    return data;
}

// A weather code (WMO) as an emoji, and in words.
std::wstring WeatherEmoji(int code, bool day) {
    switch (code) {
        case 0:
            return day ? L"☀️" : L"\U0001F319";
        case 1:
            return day ? L"\U0001F324️" : L"\U0001F319";
        case 2:
            return L"⛅";
        // Fog as a cloud too: Windows 11's fog emoji is a plain light square,
        // which doesn't read as anything (#6226).
        case 3:
        case 45:
        case 48:
            return L"☁️";
        case 51:
        case 53:
        case 55:
        case 80:
        case 81:
        case 82:
            return L"\U0001F326️";
        case 71:
        case 73:
        case 75:
        case 85:
        case 86:
            return L"\U0001F328️";
        case 77:
            return L"❄️";
        case 95:
        case 96:
        case 99:
            return L"⛈️";
        default:
            return L"\U0001F327️";
    }
}

PCWSTR WeatherWords(int code) {
    switch (code) {
        case 0:
            return Tr(L"Clear", L"Céu limpo");
        case 1:
            return Tr(L"Mostly clear", L"Poucas nuvens");
        case 2:
            return Tr(L"Partly cloudy", L"Parcialmente nublado");
        case 3:
            return Tr(L"Overcast", L"Nublado");
        case 45:
        case 48:
            return Tr(L"Fog", L"Neblina");
        case 51:
        case 53:
        case 55:
            return Tr(L"Drizzle", L"Garoa");
        case 56:
        case 57:
            return Tr(L"Freezing drizzle", L"Garoa congelante");
        case 61:
            return Tr(L"Light rain", L"Chuva fraca");
        case 63:
            return Tr(L"Rain", L"Chuva");
        case 65:
            return Tr(L"Heavy rain", L"Chuva forte");
        case 66:
        case 67:
            return Tr(L"Freezing rain", L"Chuva congelante");
        case 71:
        case 73:
        case 75:
        case 77:
            return Tr(L"Snow", L"Neve");
        case 80:
        case 81:
        case 82:
            return Tr(L"Showers", L"Pancadas de chuva");
        case 85:
        case 86:
            return Tr(L"Snow showers", L"Pancadas de neve");
        case 95:
            return Tr(L"Thunderstorm", L"Trovoadas");
        case 96:
        case 99:
            return Tr(L"Thunderstorm with hail", L"Trovoadas com granizo");
        default:
            return L"";
    }
}

// "19°", from °C (in °F with UsesFahrenheit).
std::wstring Degrees(double celsius) {
    const double shown = UsesFahrenheit() ? celsius * 9 / 5 + 32 : celsius;
    return std::to_wstring((int)std::lround(shown)) + L"°";
}

constexpr float kWeatherWidth = 620;

////////////////////////////////////////////////////////////////////////////////
// Song lyrics (an option, off by default, since it goes online): from the
// free LRCLIB service (lrclib.net, no key), for what's playing (its title,
// artist and length), synced to its time when the service has them so; on a
// thread of its own, one song at a time, the last few kept. Shown on the
// pill (the line sung now, see PillGroup::Lyrics) and in their panel (all of
// them, the line now lit, a line clicked goes there).

struct LyricLine {
    // When it's sung (seconds; -1 without times).
    double at = -1;
    std::wstring text;
};
struct SongLyrics {
    // The song (see LyricsKey).
    std::wstring key;
    bool found = false;
    // With their times (else only the words).
    bool synced = false;
    bool instrumental = false;
    std::vector<LyricLine> lines;
    // lrclib.net couldn't be reached (offline, an error): not "no lyrics",
    // looked for again after a while (kLyricsRetrySeconds), from when.
    bool failed = false;
    double failedAt = 0;
    // Not looked for (see LyricsSkipFor): why, or kLyricsSkipNone.
    int skip = 0;
};
constexpr double kLyricsRetrySeconds = 60;
// The most lines a song's lyrics keep, and the longest line (an answer
// from the internet: a song's are far fewer and shorter).
constexpr size_t kLyricsMaxLines = 2000;
constexpr size_t kLyricsMaxLineLength = 500;
SRWLOCK g_lyricsLock = SRWLOCK_INIT;
// The last songs' (looked for once each).
std::vector<SongLyrics> g_lyricsKept;
constexpr size_t kLyricsKept = 12;
// Tells the islands a song's lyrics came (or that there are none).
constexpr UINT WM_APP_LYRICS = WM_APP + 99;

std::wstring LyricsKey(const MediaState& media) {
    return media.title + L"\n" + media.artist;
}

// Lyrics are looked for only for a song, so only a song's title and artist
// ever go to lrclib.net: with an artist, neither of them the app's own name
// (a WhatsApp voice message is "WhatsApp" without an artist; a browser tab,
// its page), as long as a song when its length is told (kLyricsMinSeconds
// to kLyricsMaxSeconds), not a video (as the app says); and not from an app
// whose lyrics are turned off in the settings (see LyricsApps). Otherwise
// why, shown in their panel.
enum LyricsSkip { kLyricsSkipNone, kLyricsSkipNotSong, kLyricsSkipApp };
constexpr double kLyricsMinSeconds = 45;
constexpr double kLyricsMaxSeconds = 20 * 60;

// Only the letters and digits, in lowercase: names compared as they're read
// ("AC/DC" and "ac dc" alike; any script, see IsCharAlphaNumeric).
std::wstring LyricsFolded(std::wstring_view text) {
    std::wstring folded;
    for (WCHAR c : text) {
        if (IsCharAlphaNumeric(c)) {
            folded += c;
        }
    }
    if (!folded.empty()) {
        CharLowerBuff(folded.data(), (DWORD)folded.size());
    }
    return folded;
}

// The artist as asked for: Apple Music (the Windows app, Cider, its web
// page) says "Artist — Album".
std::wstring LyricsArtist(std::wstring_view artist) {
    return std::wstring(artist.substr(0, artist.find(L" — ")));
}

int LyricsSkipFor(const MediaState& media,
                  const std::wstring& appName,
                  bool appOff) {
    if (appOff) {
        return kLyricsSkipApp;
    }
    const std::wstring title = LyricsFolded(media.title);
    const std::wstring artist = LyricsFolded(LyricsArtist(media.artist));
    const std::wstring app = LyricsFolded(appName);
    // (A length not told by the app, 0, doesn't count: some music players
    // and web pages don't tell it.)
    if (media.video || title.empty() || artist.empty() || title == artist ||
        (!app.empty() && (title == app || artist == app)) ||
        (media.duration > 0 && (media.duration < kLyricsMinSeconds ||
                                media.duration > kLyricsMaxSeconds))) {
        return kLyricsSkipNotSong;
    }
    return kLyricsSkipNone;
}

// A title without its extras ("Song (Remastered)", "Song - Live"), folded.
std::wstring LyricsBareTitle(std::wstring_view title) {
    size_t cut = title.size();
    for (PCWSTR mark : {L" - ", L" (", L" [", L" feat."}) {
        const size_t at = title.find(mark);
        if (at != std::wstring_view::npos && at > 0) {
            cut = std::min(cut, at);
        }
    }
    return LyricsFolded(title.substr(0, cut));
}

// Whether a song lrclib.net's search found is the one asked for (it answers
// with whatever is close: a voice message once got a song's lyrics): the
// same title (with or without its extras), one of the artists ("A, B",
// "A & B", "A feat. B"), and the same length within 10 s when both are
// known.
bool LyricsResultMatches(const Json& song,
                         const std::wstring& title,
                         const std::wstring& artist,
                         double duration) {
    const std::wstring& foundTitle = song[L"trackName"].string;
    const std::wstring bare = LyricsBareTitle(title);
    const bool sameTitle =
        !LyricsFolded(title).empty() &&
        (LyricsFolded(foundTitle) == LyricsFolded(title) ||
         (!bare.empty() && LyricsBareTitle(foundTitle) == bare));
    const std::wstring found = LyricsFolded(song[L"artistName"].string);
    bool sameArtist = !found.empty() && found == LyricsFolded(artist);
    std::wstring parts = artist;
    if (!parts.empty()) {
        CharLowerBuff(parts.data(), (DWORD)parts.size());
    }
    for (PCWSTR mark : {L" feat. ", L" feat ", L" ft. ", L" & ", L" x ",
                        L" and ", L",", L";", L"/"}) {
        for (size_t at = parts.find(mark); at != std::wstring::npos;
             at = parts.find(mark, at + 1)) {
            parts.replace(at, wcslen(mark), L"\n");
        }
    }
    size_t start = 0;
    while (!sameArtist && !found.empty() && start <= parts.size()) {
        size_t end = parts.find(L'\n', start);
        if (end == std::wstring::npos) {
            end = parts.size();
        }
        const std::wstring part =
            LyricsFolded(std::wstring_view(parts).substr(start, end - start));
        // (A short name only whole: "a" is in every name.)
        sameArtist = !part.empty() &&
                     (part == found ||
                      (std::min(part.size(), found.size()) >= 3 &&
                       (found.find(part) != std::wstring::npos ||
                        part.find(found) != std::wstring::npos)));
        start = end + 1;
    }
    const double foundDuration = song[L"duration"].Number();
    const bool sameLength = duration <= 0 || foundDuration <= 0 ||
                            std::fabs(foundDuration - duration) <= 10;
    return sameTitle && sameArtist && sameLength;
}

// The apps seen playing, each with its lyrics on or off (off: never looked
// for, so nothing of its media is sent; see SettingKind::LyricsApps). This
// account's (see UserValueName); read once, and again after a backup is
// restored (g_lyricsAppsRead). On the island's thread.
struct LyricsApp {
    std::wstring appId;
    bool off = false;
};
std::vector<LyricsApp> g_lyricsApps;
bool g_lyricsAppsRead = false;
constexpr size_t kLyricsAppsKept = 12;
// The room to read them (12 app IDs, a path at most each).
constexpr size_t kLyricsAppsRoom = 8192;
// The Icons tab's cells (see SettingKind::Icons): how tall and how far
// apart (DIPs), how wide at least, and the icon in them bigger than in a
// row; how many side by side; each group's name, icon and size.
constexpr float kIconCellHeight = 72;
constexpr float kIconCellGap = 6;
constexpr float kIconCellLeast = 92;
constexpr float kIconCellZoom = 1.5f;
size_t IconCellColumns(float width, float scale) {
    return std::max<size_t>(
        1, (size_t)((width - std::round(8 * scale) +
                     std::round(kIconCellGap * scale)) /
                    std::round((kIconCellLeast + kIconCellGap) * scale)));
}

PCWSTR IconGroupName(int group) {
    switch ((IconGroup)group) {
        case IconGroup::Status:
            return Tr(L"Status", L"Status");
        case IconGroup::Media:
            return Tr(L"What's playing", L"O que está tocando");
        case IconGroup::Controls:
            return Tr(L"Control center", L"Central de controle");
        case IconGroup::Pill:
            return Tr(L"Pill items", L"Itens da pílula");
        case IconGroup::Buttons:
            return Tr(L"Panels and buttons", L"Painéis e botões");
        default:
            return Tr(L"Settings", L"Configurações");
    }
}

WCHAR IconGroupGlyph(int group) {
    switch ((IconGroup)group) {
        case IconGroup::Status:
            return 0xE701;
        case IconGroup::Media:
            return kGlyphMusic;
        case IconGroup::Controls:
            return kGlyphControlCenter;
        case IconGroup::Pill:
            return 0xE71D;
        case IconGroup::Buttons:
            return 0xE8A9;
        default:
            return kGlyphSettings;
    }
}

size_t IconGroupSize(int group) {
    return (size_t)std::count_if(
        std::begin(kIcons), std::end(kIcons),
        [&](const IconInfo& icon) { return (int)icon.group == group; });
}

bool HasIconFile(WCHAR glyph) {
    return std::any_of(g_iconFiles.files.begin(), g_iconFiles.files.end(),
                       [&](const auto& file) { return file.first == glyph; });
}

// A picture or a color of its own (the Icons tab's cell ringed, its "x"
// back to the pack's).
bool HasOwnIconLook(WCHAR glyph) {
    return HasIconFile(glyph) ||
           std::any_of(g_iconFiles.colors.begin(), g_iconFiles.colors.end(),
                       [&](const auto& own) { return own.first == glyph; });
}

