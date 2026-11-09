#pragma once
std::wstring ProfileValueName(int id) {
    return L"profile" + std::to_wstring(id);
}

// "profiles": an id and a name per line (tab-separated).
void LoadProfiles() {
    g_profiles.clear();
    const std::wstring value = GetWholeStringValue(L"profiles", 1024);
    for (PCWSTR line = value.c_str();
         *line && g_profiles.size() < kMaxProfiles;) {
        PCWSTR end = wcschr(line, L'\n');
        const std::wstring text =
            end ? std::wstring(line, end) : std::wstring(line);
        const size_t tab = text.find(L'\t');
        if (tab != std::wstring::npos && tab + 1 < text.size()) {
            const int id = _wtoi(text.c_str());
            if (id > 0) {
                g_profiles.push_back({id, text.substr(tab + 1)});
            }
        }
        if (!end) {
            break;
        }
        line = end + 1;
    }
    g_activeProfile = Wh_GetIntValue(L"profileActive", -1);
    g_autoProfileRulesRead = false;
}

void SaveProfiles() {
    std::wstring value;
    for (const auto& profile : g_profiles) {
        value += std::to_wstring(profile.id) + L"\t" + profile.name + L"\n";
    }
    SetStoredString(L"profiles", value.c_str());
    Wh_SetIntValue(L"profileActive", g_activeProfile);
}

// "Profile 1", or the first number not taken.
std::wstring NewProfileName() {
    for (int number = 1;; number++) {
        const std::wstring name =
            Tr(L"Profile ", L"Perfil ") + std::to_wstring(number);
        if (std::none_of(g_profiles.begin(), g_profiles.end(),
                         [&](const IslandProfile& profile) {
                             return profile.name == name;
                         })) {
            return name;
        }
    }
}

// A profile's settings as text, a line each: "i" or "s" (a number or a
// text), the value's name, "=", and the value (a text with its tabs, line
// breaks and backslashes escaped).
std::wstring EscapeProfileText(const std::wstring& text) {
    std::wstring escaped;
    for (WCHAR c : text) {
        escaped += c == L'\\'  ? L"\\\\"
                   : c == L'\n' ? L"\\n"
                   : c == L'\t' ? L"\\t"
                                : std::wstring(1, c);
    }
    return escaped;
}

std::wstring UnescapeProfileText(std::wstring_view text) {
    std::wstring plain;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == L'\\' && i + 1 < text.size()) {
            i++;
            plain += text[i] == L'n'   ? L'\n'
                     : text[i] == L't' ? L'\t'
                                       : text[i];
        } else {
            plain += text[i];
        }
    }
    return plain;
}

// The texts a profile keeps, and the room to read each.
struct ProfileText {
    PCWSTR name;
    size_t room;
};
constexpr ProfileText kProfileTexts[] = {
    {L"islandOrder", 64},    {L"monitorPlaces", 2048}, {L"appPlaces", 16384},
    {L"controls", 1024},     {L"islandMonitor", 64},   {L"islandNotchSides", 64},
    {L"trayPinned", 8192},   {L"trayOrder", 8192},     {L"trayAnchors", 8192},
    {L"islandIconFolder", 1024}, {L"islandIconFiles", kIconFilesRoom},
};

// Everything the island's settings choose, the tray icons pinned and their
// places too (not its notifications, notes or tasks).
// `texts`: the texts kept (another monitor's profile), not the stored ones.
std::wstring CaptureProfile(
    const std::unordered_map<std::wstring, std::wstring>* texts = nullptr) {
    std::wstring data;
    auto number = [&](PCWSTR name, int value) {
        data += L"i" + std::wstring(name) + L"=" + std::to_wstring(value) +
                L"\n";
    };
    for (const auto& info : kIslandPrefs) {
        if (info.key) {
            number(info.key, GetIslandPref(info.pref));
        }
    }
    number(L"islandSize", g_settings.islandSize);
    number(L"islandGap", g_settings.islandGap);
    number(L"islandOpacity", g_islandOpacity);
    number(L"islandAutoHideDelay", g_settings.islandAutoHideDelay);
    number(L"islandNotchCurve", g_settings.islandNotchCurve);
    number(L"islandIconSize", g_settings.islandIconSize);
    number(L"islandLineOpacity", g_settings.islandLineOpacity);
    number(L"islandActivationWidth", g_settings.islandActivationWidth);
    number(L"islandPanelSize", g_settings.islandPanelSize);
    number(L"islandMediaTitleLimit", g_settings.islandMediaTitleLimit);
    number(L"islandPillHeight", g_settings.islandPillHeight);
    number(L"islandPillCorners", g_settings.islandPillCorners);
    number(L"islandPillEnds", g_settings.islandPillEnds);
    number(L"islandEdge", g_settings.islandEdge);
    number(L"islandGlassBlur", g_settings.islandGlassBlur);
    number(L"islandTrayColumns", g_settings.islandTrayColumns);
    number(L"islandVizSensitivity", g_settings.islandVizSensitivity);
    number(L"islandTextSize", g_settings.islandTextSize);
    for (int i = 0; i < kCustomColors; i++) {
        number(kCustomColorKeys[i], g_customColors[i]);
    }
    number(L"trayListIsland", g_trayListIcons);
    number(L"animationsIsland", EncodeAnimations(g_animationChoice));
    number(L"placeX", (int)std::lround(g_islandPlace.x * 1000));
    number(L"placeY", (int)std::lround(g_islandPlace.y * 1000));
    for (const auto& text : kProfileTexts) {
        std::wstring kept = GetWholeStringValue(text.name, text.room);
        if (texts) {
            if (const auto it = texts->find(text.name); it != texts->end()) {
                kept = it->second;
            }
        }
        data += L"s" + std::wstring(text.name) + L"=" +
                EscapeProfileText(kept) + L"\n";
    }
    return data;
}

void ParseProfileLines(PCWSTR data,
                       std::unordered_map<std::wstring, int>* numbers,
                       std::unordered_map<std::wstring, std::wstring>* texts);

// A profile's text, whole (it grows with the tray icons' places).
std::wstring ReadProfileText(int id) {
    return GetWholeStringValue(ProfileValueName(id).c_str(), 64 * 1024);
}

// What the current island shows (another monitor's look, or the main
// settings), as profile `id`'s new text: its own texts kept (they're the
// main island's on the others), with the pill's order it shows.
std::wstring CaptureShownProfile(int id) {
    const std::wstring value = ReadProfileText(id);
    std::unordered_map<std::wstring, int> numbers;
    std::unordered_map<std::wstring, std::wstring> texts;
    ParseProfileLines(value.data(), &numbers, &texts);
    if (!OwnLookShown()) {
        return CaptureProfile();
    }
    texts[L"islandOrder"] = EncodePillOrder(g_pillOrder);
    std::wstring sides;
    for (uint8_t side : g_notchSides) {
        sides += (WCHAR)(L'0' + side);
    }
    texts[L"islandNotchSides"] = sides;
    texts[L"trayPinned"] = EncodeTrayKeys(g_trayPinned);
    texts[L"trayOrder"] = EncodeTrayKeys(g_trayOrder);
    texts[L"trayAnchors"] = EncodeTrayAnchors(g_trayAnchors);
    texts[L"islandIconFolder"] = g_iconFolder;
    texts[L"islandIconFiles"] = g_iconFiles.text;
    return CaptureProfile(&texts);
}

int AddProfile(const std::wstring& name) {
    if (g_profiles.size() >= kMaxProfiles) {
        return -1;
    }
    // Another monitor's island: a copy of what it shows, used by it alone
    // from then on (the main island's profile in use stays).
    Island* monitor = g_island && !g_island->HasMainSettings()
                          ? g_island->SettingsIsland()
                          : nullptr;
    if (!monitor) {
        SaveActiveProfile();
    }
    // (Never one used before: a monitor unplugged when its profile was
    // deleted mustn't find another one under its id later.)
    int id = std::max(1, Wh_GetIntValue(L"profileNextId", 1));
    for (const auto& profile : g_profiles) {
        id = std::max(id, profile.id + 1);
    }
    Wh_SetIntValue(L"profileNextId", id + 1);
    SetStoredString(ProfileValueName(id).c_str(),
                    CaptureShownProfile(ChosenProfile()).c_str());
    g_profiles.push_back({id, name});
    if (monitor) {
        SaveProfiles();
        IslandScope scope(monitor);
        monitor->SetMonitorProfile(id);
        return id;
    }
    g_activeProfile = id;
    SaveProfiles();
    return id;
}

void ShowSavedProfile(int id, Island* from);

// Another monitor's island: what it shows now, saved in profile `id`, which
// it uses from then on (and every island with it shows).
void SaveShownInProfile(int id) {
    if (!g_island || g_island->HasMainSettings() ||
        std::none_of(g_profiles.begin(), g_profiles.end(),
                     [id](const IslandProfile& profile) {
                         return profile.id == id;
                     })) {
        return;
    }
    Island* monitor = g_island->SettingsIsland();
    g_island->SaveOwnProfile();
    SetStoredString(ProfileValueName(id).c_str(),
                    CaptureShownProfile(id).c_str());
    {
        IslandScope scope(monitor);
        monitor->SetMonitorProfile(id);
    }
    ShowSavedProfile(id, monitor);
}

// The profile in use follows the settings: changed (see StoreProfileInt), it's
// saved a moment later, once for a batch of changes (a slider dragged), on
// the main island's window; not while a profile is being applied.
void SaveActiveProfileSoon() {
    if (g_applyingProfile || g_activeProfile < 0) {
        return;
    }
    if (Island* main = MainIsland()) {
        main->SaveProfileSoon();
    }
}

// Now (before another profile is applied, added or exported), if its
// settings changed since.
void SaveActiveProfile() {
    Island* main = MainIsland();
    if (main) {
        main->CancelProfileSave();
    }
    // (The main island's settings, whichever island is the current one.)
    IslandScope scope(main);
    if (g_activeProfile >= 0 && !g_applyingProfile &&
        ProfileChanged(g_activeProfile)) {
        UpdateProfile(g_activeProfile);
    }
}

// The main island's settings saved in profile `id`, now the one in use there
// (and shown by every other island with it).
void UpdateProfile(int id) {
    Island* main = MainIsland();
    IslandScope scope(main);
    for (const auto& profile : g_profiles) {
        if (profile.id == id) {
            SetStoredString(ProfileValueName(id).c_str(),
                            CaptureProfile().c_str());
            g_activeProfile = id;
            SaveProfiles();
            ShowSavedProfile(id, main);
            return;
        }
    }
}

// Without tabs and line breaks (they separate the list), trimmed; an empty
// name keeps the one it had.
void RenameProfile(int id, const std::wstring& name) {
    std::wstring clean;
    for (WCHAR c : name) {
        clean += c < 0x20 ? L' ' : c;
    }
    clean.erase(0, clean.find_first_not_of(L' '));
    clean.erase(clean.find_last_not_of(L' ') + 1);
    if (clean.size() > kMaxProfileName) {
        clean.resize(kMaxProfileName);
    }
    if (clean.empty()) {
        return;
    }
    for (auto& profile : g_profiles) {
        if (profile.id == id && profile.name != clean) {
            profile.name = clean;
            SaveProfiles();
            return;
        }
    }
}

void DeleteProfile(int id) {
    const size_t before = g_profiles.size();
    std::erase_if(g_profiles, [id](const IslandProfile& profile) {
        return profile.id == id;
    });
    if (g_profiles.size() == before) {
        return;
    }
    Wh_DeleteValue(ProfileValueName(id).c_str());
    if (g_activeProfile == id) {
        g_activeProfile = -1;
    }
    SaveProfiles();
    // The monitors that used it: the main island's settings again.
    ForEachIsland([id](Island& island) {
        if (!island.IsNotchSibling() && !island.IsMainIsland() &&
            island.MonitorProfile() == id) {
            island.SetMonitorProfile(-1);
        }
    });
}

// Reads a profile's lines (see CaptureProfile) into its numbers and texts;
// other lines are skipped.
void ParseProfileLines(PCWSTR data,
                       std::unordered_map<std::wstring, int>* numbers,
                       std::unordered_map<std::wstring, std::wstring>* texts) {
    for (PCWSTR line = data; *line;) {
        PCWSTR end = wcschr(line, L'\n');
        const std::wstring_view text =
            end ? std::wstring_view(line, end - line) : std::wstring_view(line);
        const size_t equals = text.find(L'=');
        if (text.size() > 1 && equals != std::wstring_view::npos) {
            const std::wstring name(text.substr(1, equals - 1));
            const std::wstring_view value = text.substr(equals + 1);
            if (text[0] == L'i') {
                (*numbers)[name] = _wtoi(std::wstring(value).c_str());
            } else if (text[0] == L's') {
                (*texts)[name] = UnescapeProfileText(value);
            }
        }
        if (!end) {
            break;
        }
        line = end + 1;
    }
}

// (Only what the profile keeps: options added since it was saved don't
// count.)
bool ProfileChanged(int id) {
    if (std::none_of(g_profiles.begin(), g_profiles.end(),
                     [id](const IslandProfile& profile) {
                         return profile.id == id;
                     })) {
        return false;
    }
    const std::wstring value = ReadProfileText(id);
    std::unordered_map<std::wstring, int> savedNumbers, numbers;
    std::unordered_map<std::wstring, std::wstring> savedTexts, texts;
    ParseProfileLines(value.data(), &savedNumbers, &savedTexts);
    ParseProfileLines(CaptureProfile().c_str(), &numbers, &texts);
    for (const auto& [name, number] : savedNumbers) {
        if (const auto it = numbers.find(name);
            it != numbers.end() && it->second != number) {
            return true;
        }
    }
    for (const auto& [name, text] : savedTexts) {
        if (const auto it = texts.find(name);
            it != texts.end() && it->second != text) {
            return true;
        }
    }
    return false;
}

// Puts settings captured by CaptureProfile back: each option the way it's
// changed in the island's settings (with what each does when changed), then
// the order and the places read again.
bool ApplyProfileSettings(
    const std::unordered_map<std::wstring, int>& numbers,
    const std::unordered_map<std::wstring, std::wstring>& texts) {
    if (numbers.empty()) {
        return false;
    }
    g_applyingProfile = true;
    // The custom colors first: the theme (maybe them) comes after.
    for (int i = 0; i < kCustomColors; i++) {
        if (const auto it = numbers.find(kCustomColorKeys[i]);
            it != numbers.end() && it->second != g_customColors[i]) {
            g_customColors[i] = it->second & 0xFFFFFF;
            Wh_SetIntValue(kCustomColorKeys[i], g_customColors[i]);
            ApplyThemeIndex(true);
        }
    }
    // (Not the profiles' button: the one switching profiles stays.)
    for (const auto& info : kIslandPrefs) {
        if (!info.key || info.pref == IslandPref::Profiles) {
            continue;
        }
        if (const auto it = numbers.find(info.key);
            it != numbers.end() && GetIslandPref(info.pref) != it->second) {
            SetIslandPref(info.pref, it->second);
        }
    }
    if (const auto it = numbers.find(L"islandSize"); it != numbers.end()) {
        SetIslandSize(it->second, true);
    }
    if (const auto it = numbers.find(L"islandGap"); it != numbers.end()) {
        SetIslandGap(it->second, true);
    }
    if (const auto it = numbers.find(L"islandOpacity"); it != numbers.end()) {
        SetIslandOpacity(it->second, true);
    }
    if (const auto it = numbers.find(L"islandAutoHideDelay");
        it != numbers.end()) {
        SetAutoHideDelay(it->second, true);
    }
    if (const auto it = numbers.find(L"islandNotchCurve");
        it != numbers.end()) {
        SetNotchCurve(it->second, true);
    }
    if (const auto it = numbers.find(L"islandIconSize"); it != numbers.end()) {
        SetIconSize(it->second, true);
    }
    if (const auto it = numbers.find(L"islandLineOpacity");
        it != numbers.end()) {
        SetLineOpacity(it->second, true);
    }
    if (const auto it = numbers.find(L"islandActivationWidth");
        it != numbers.end()) {
        SetActivationWidth(it->second, true);
    }
    if (const auto it = numbers.find(L"islandPanelSize");
        it != numbers.end()) {
        SetPanelSize(it->second, true);
    }
    if (const auto it = numbers.find(L"islandMediaTitleLimit");
        it != numbers.end()) {
        SetMediaTitleLimit(it->second, true);
    }
    for (int slider = 12; slider <= 19; slider++) {
        if (const auto it = numbers.find(
                slider == 12   ? L"islandPillHeight"
                : slider == 13 ? L"islandPillCorners"
                : slider == 14 ? L"islandPillEnds"
                : slider == 15 ? L"islandEdge"
                : slider == 16 ? L"islandGlassBlur"
                : slider == 17 ? L"islandTrayColumns"
                : slider == 18 ? L"islandVizSensitivity"
                               : L"islandTextSize");
            it != numbers.end()) {
            SetPillLook(slider, it->second, true);
        }
    }
    // (A backup made before the texts' own size: they grew with the icons.)
    if (const auto it = numbers.find(L"islandIconSize");
        it != numbers.end() && !numbers.contains(L"islandTextSize")) {
        SetPillLook(19, it->second, true);
    }
    if (const auto it = numbers.find(L"trayListIsland"); it != numbers.end()) {
        SetTrayListIcons(it->second, true);
    }
    if (const auto it = numbers.find(L"animationsIsland");
        it != numbers.end() &&
        it->second != EncodeAnimations(g_animationChoice)) {
        SetAnimationChoice(DecodeAnimations(it->second), true);
    }
    for (PCWSTR name : {L"placeX", L"placeY"}) {
        if (const auto it = numbers.find(name); it != numbers.end()) {
            Wh_SetIntValue(name, std::clamp(it->second, 0, 1000));
        }
    }
    for (const auto& text : kProfileTexts) {
        if (const auto it = texts.find(text.name); it != texts.end()) {
            SetStoredString(text.name, it->second.c_str());
        }
    }
    g_applyingProfile = false;
    LoadPillOrder();
    LoadNotchSides();
    LoadTrayArrangement();
    LoadPlaces();
    g_iconFolder = GetWholeStringValue(L"islandIconFolder", 1024);
    g_iconFiles = DecodeIconFiles(
        GetWholeStringValue(L"islandIconFiles", kIconFilesRoom));
    g_iconLook++;
    if (const auto it = texts.find(L"islandMonitor"); it != texts.end()) {
        g_islandMonitor = it->second;
    }
    return true;
}

// Profile `id`'s values put in the main settings.
bool LoadProfileSettings(int id) {
    const std::wstring value = ReadProfileText(id);
    std::unordered_map<std::wstring, int> numbers;
    std::unordered_map<std::wstring, std::wstring> texts;
    ParseProfileLines(value.data(), &numbers, &texts);
    return ApplyProfileSettings(numbers, texts);
}

bool ApplyProfile(int id) {
    if (std::none_of(g_profiles.begin(), g_profiles.end(),
                     [id](const IslandProfile& profile) {
                         return profile.id == id;
                     })) {
        return false;
    }
    if (g_island && !g_island->HasMainSettings()) {
        g_island->SetMonitorProfile(id);
        return true;
    }
    // (The one in use keeps its last changes.)
    SaveActiveProfile();
    if (!LoadProfileSettings(id)) {
        return false;
    }
    g_activeProfile = id;
    SaveProfiles();
    if (g_island) {
        ForEachIsland([&](Island& island) { island.OnProfileApplied(); });
    }
    return true;
}

// Profile `id` just saved from island `from`: every other island with it
// shows it again, the main island's settings too if it's the one in use
// there.
void ShowSavedProfile(int id, Island* from) {
    // (Not the ones already gone, as the islands stop.)
    for (Island* island : std::vector<Island*>(g_islands)) {
        if (island != from && island->Window() && !island->IsMainIsland() &&
            island->MonitorProfile() == id) {
            IslandScope scope(island);
            island->ReloadLook();
        }
    }
    Island* main = MainIsland();
    if (main && main->Window() && from != main && g_activeProfile == id) {
        IslandScope scope(main);
        main->CancelProfileSave();
        if (LoadProfileSettings(id)) {
            ForEachIsland([](Island& island) { island.OnProfileApplied(); });
        }
    }
}

// What a backup keeps besides the settings (CaptureProfile) and the
// profiles: the user's own notes, tasks, shortcuts and events, and a few
// choices made on the pill. Not passing states (unread apps, notifications
// seen, what the island turned on).
// A NEW VALUE STORED FOR THE USER GOES HERE (or in CaptureProfile), so it's
// backed up too.
struct BackupValue {
    PCWSTR name;
    // The room to read it, or 0 for a number (with its default).
    size_t room;
    int number;
    // Kept for each account (see UserValueName); in the file by its plain
    // name, so a backup goes to another account or computer.
    bool user = false;
};
constexpr BackupValue kBackupValues[] = {
    {L"notes", 512 * 1024},     {L"tasks", 256 * 1024},
    {L"shortcuts", 256 * 1024}, {L"events", 64 * 1024},
    {L"weatherPlace", 1024},    {L"weatherMyLocation", 0, 0},
    {L"trayPinned", 8192},
    {L"updatesIgnored", 4096},  {L"profiles", 1024},
    {L"monitorProfiles", 2048}, {L"folderPins", 16 * 1024, 0, true},
    {L"wallpaperFolder", 1024}, {L"agendas", 32 * 1024},
    {L"trayOrder", 8192},       {L"trayAnchors", 8192},
    {L"profileActive", 0, -1},  {L"islandMinimized", 0, 0},
    {L"islandMinimizedNotch0", 0, 0}, {L"islandMinimizedNotch1", 0, 0},
    {L"islandMinimizedNotch2", 0, 0},
    {L"islandQuiet", 0, 0},     {L"notificationContent", 0, 1},
    {L"calendarToday", 0, 0},   {L"calendarTodayColor", 0, kTodayRed},
    {L"notifyScreens", 2048},   {L"notifyMainIsland", 0, 1},
    {L"folderLooks", 16 * 1024, 0, true}, {L"folderOrder", 0, 0, true},
    {L"islandMinimizeShortcut", 0, kDefaultMinimizeShortcut},
    {L"timerLength", 0, 5 * 60, true},
    {L"pomodoroFocus", 0, 25 * 60, true},
    {L"pomodoroBreak", 0, 5 * 60, true},
    {L"alarms", 4096, 0, true},
    {L"lyricsApps", kLyricsAppsRoom, 0, true},
    {L"commands", 1024 * 1024, 0, true},
    {L"autoProfiles", 8192},
};
constexpr WCHAR kBackupHeader[] = L"Mac Island & Dock backup";

// The settings' Logs page: the mod's version, Windows' build, the monitors,
// the island's settings (as a profile keeps them) and the last lines the
// island logged; copied or saved to a file the user picks.
std::wstring LogsText() {
    std::wstring text = L"Mac Island & Dock " WH_MOD_VERSION L"\r\n";
    WCHAR build[32] = L"";
    DWORD size = sizeof(build);
    DWORD update = 0;
    DWORD updateSize = sizeof(update);
    RegGetValue(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                L"CurrentBuild", RRF_RT_REG_SZ, nullptr, build, &size);
    RegGetValue(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"UBR",
                RRF_RT_REG_DWORD, nullptr, &update, &updateSize);
    text += L"Windows build " + std::wstring(build) + L"." +
            std::to_wstring(update) + L"\r\n";
    text += L"Monitors " + std::to_wstring(MonitorCount()) + L", islands " +
            std::to_wstring(OtherIslands().size() + 1) + L"\r\n\r\n";
    text += L"Settings\r\n";
    const std::wstring settings = CaptureProfile();
    for (WCHAR c : settings) {
        if (c == L'\n') {
            text += L'\r';
        }
        text += c;
    }
    text += L"\r\nLast lines logged\r\n" + KeptLogText();
    return text;
}

// Windhawk's own window (where the dock's options are), from the folder
// the island's process runs from.
void OpenWindhawk() {
    WCHAR path[MAX_PATH];
    const DWORD length = GetModuleFileName(nullptr, path, ARRAYSIZE(path));
    if (!length || length >= ARRAYSIZE(path)) {
        return;
    }
    if (PWSTR name = wcsrchr(path, L'\\')) {
        *(name + 1) = 0;
    }
    if (wcscat_s(path, L"windhawk.exe") == 0) {
        ShellExecute(nullptr, L"open", path, nullptr, nullptr, SW_SHOWNORMAL);
    }
}

std::wstring CaptureBackup() {
    std::wstring data = std::wstring(kBackupHeader) + L"\n" + CaptureProfile();
    for (const auto& value : kBackupValues) {
        if (!value.room) {
            const int number = value.user
                                   ? GetUserIntValue(value.name, value.number,
                                                     true)
                                   : Wh_GetIntValue(value.name, value.number);
            data += L"i" + std::wstring(value.name) + L"=" +
                    std::to_wstring(number) + L"\n";
            continue;
        }
        data += L"s" + std::wstring(value.name) + L"=" +
                EscapeProfileText(
                    value.user ? GetWholeUserStringValue(value.name, value.room)
                               : GetWholeStringValue(value.name, value.room)) +
                L"\n";
    }
    for (const auto& profile : g_profiles) {
        data += L"s" + ProfileValueName(profile.id) + L"=" +
                EscapeProfileText(ReadProfileText(profile.id).c_str()) + L"\n";
    }
    return data;
}

bool RestoreBackup(const std::wstring& text) {
    if (!text.starts_with(kBackupHeader)) {
        return false;
    }
    std::unordered_map<std::wstring, int> numbers;
    std::unordered_map<std::wstring, std::wstring> texts;
    ParseProfileLines(text.c_str(), &numbers, &texts);
    // (A save of the profile in use still waiting would write the imported
    // settings over it as the island restarts.)
    if (Island* main = MainIsland()) {
        main->CancelProfileSave();
    }
    // The settings first (they're what a profile keeps).
    if (!ApplyProfileSettings(numbers, texts)) {
        return false;
    }
    for (const auto& value : kBackupValues) {
        if (!value.room) {
            if (const auto it = numbers.find(value.name); it != numbers.end()) {
                if (value.user) {
                    SetUserIntValue(value.name, it->second);
                } else {
                    Wh_SetIntValue(value.name, it->second);
                }
            }
        } else if (const auto it = texts.find(value.name); it != texts.end()) {
            if (value.user) {
                SetUserStringValue(value.name, it->second.c_str());
                // (Its own now, even empty: not seeded again.)
                Wh_SetIntValue(UserSeededName(value.name).c_str(), 1);
            } else {
                SetStoredString(value.name, it->second.c_str());
            }
        }
    }
    // The profiles here go; the backup's come (a backup without its list
    // of profiles keeps these).
    if (texts.contains(L"profiles")) {
        for (const auto& profile : g_profiles) {
            Wh_DeleteValue(ProfileValueName(profile.id).c_str());
        }
        LoadProfiles();
        for (const auto& profile : g_profiles) {
            const std::wstring name = ProfileValueName(profile.id);
            if (const auto it = texts.find(name); it != texts.end()) {
                SetStoredString(name.c_str(), it->second.c_str());
            }
        }
    }
    // Notes, tasks, the calendar's events, the alarms, the timer's lengths,
    // the lyrics' apps: read again as the island starts again.
    g_eventsLoaded = false;
    g_alarmsRead = false;
    g_timer.lengthsLoaded = false;
    g_lyricsAppsRead = false;
    LoadIgnoredUpdates();
    RestartIslandSoon();
    return true;
}

// The island's settings back to their defaults (#6164: "Reset" beside the
// backup, asked twice): every value a profile keeps (see CaptureProfile)
// deleted, so each reads its default again, with no profile in use. The
// notes, tasks, events, agendas, profiles, alarms and notifications stay.
void ResetIslandSettings() {
    if (Island* main = MainIsland()) {
        main->CancelProfileSave();
    }
    for (const auto& info : kIslandPrefs) {
        if (info.key) {
            Wh_DeleteValue(info.key);
        }
    }
    for (PCWSTR name :
         {L"islandSize", L"islandGap", L"islandOpacity",
          L"islandAutoHideDelay", L"islandNotchCurve", L"islandIconSize",
          L"islandLineOpacity", L"islandActivationWidth", L"islandPanelSize",
          L"islandMediaTitleLimit", L"islandPillHeight", L"islandPillCorners",
          L"islandPillEnds", L"islandEdge", L"islandGlassBlur",
          L"islandTrayColumns", L"islandVizSensitivity", L"islandTextSize",
          L"trayListIsland", L"animationsIsland", L"placeX", L"placeY",
          L"bannerEnabled", L"bannerMinimized"}) {
        Wh_DeleteValue(name);
    }
    for (int i = 0; i < kCustomColors; i++) {
        Wh_DeleteValue(kCustomColorKeys[i]);
    }
    for (const auto& text : kProfileTexts) {
        Wh_DeleteValue(text.name);
    }
    g_activeProfile = -1;
    SaveProfiles();
    LoadSettings();
    LoadPillOrder();
    RestartIslandSoon();
}

IslandLook CaptureLook() {
    IslandLook look{g_settings,
                    g_themeIndex,
                    g_theme,
                    g_themeTarget,
                    g_themeFrom,
                    g_themeChangeStart,
                    g_mediaButtonsInPill,
                    g_mediaTitleInPill,
                    g_bannerEnabled,
                    g_bannerWhenMinimized,
                    g_islandOpacity,
                    g_trayListIcons,
                    g_pillOrder,
                    g_customColors,
                    {}};
    std::copy(std::begin(g_notchSides), std::end(g_notchSides),
              look.notchSides.begin());
    look.trayPinned = g_trayPinned;
    look.trayOrder = g_trayOrder;
    look.trayAnchors = g_trayAnchors;
    look.iconFolder = g_iconFolder;
    look.iconFiles = g_iconFiles;
    return look;
}

void PutLook(const IslandLook& look) {
    g_settings = look.settings;
    g_themeIndex = look.themeIndex;
    g_theme = look.theme;
    g_themeTarget = look.themeTarget;
    g_themeFrom = look.themeFrom;
    g_themeChangeStart = look.themeChangeStart;
    g_mediaButtonsInPill = look.mediaButtons;
    g_mediaTitleInPill = look.mediaTitle;
    g_bannerEnabled = look.banner;
    g_bannerWhenMinimized = look.bannerMinimized;
    g_islandOpacity = look.opacity;
    g_trayListIcons = look.trayList;
    g_pillOrder = look.order;
    g_customColors = look.colors;
    std::copy(look.notchSides.begin(), look.notchSides.end(), g_notchSides);
    g_trayPinned = look.trayPinned;
    g_trayOrder = look.trayOrder;
    g_trayAnchors = look.trayAnchors;
    g_iconFolder = look.iconFolder;
    g_iconFiles = look.iconFiles;
}

// A look made again (from its profile) keeps the colors shown before it:
// still blending into the same ones, or blending from them into its own
// (made again, it would show them at once). Returns whether they blend.
bool KeepThemeBlend(const IslandLook& before, IslandLook& look) {
    auto same = [](const IslandTheme& a, const IslandTheme& b) {
        return std::equal(a.background, a.background + 3, b.background) &&
               std::equal(a.foreground, a.foreground + 3, b.foreground) &&
               std::equal(a.icon, a.icon + 3, b.icon) &&
               a.backgroundAlpha == b.backgroundAlpha && a.light == b.light;
    };
    if (before.themeChangeStart && same(before.themeTarget, look.themeTarget)) {
        look.theme = before.theme;
        look.themeFrom = before.themeFrom;
        look.themeChangeStart = before.themeChangeStart;
        return true;
    }
    if (same(before.theme, look.themeTarget)) {
        return false;
    }
    look.theme = look.themeFrom = before.theme;
    look.themeChangeStart = NowSeconds();
    return true;
}

