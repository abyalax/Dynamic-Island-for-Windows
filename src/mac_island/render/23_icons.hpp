// Every icon of the font the island draws (see IconFont), as the settings'
// Icons page lists them: its name for a picture of one's own (a file in the
// icon pack's folder, see g_iconFolder), its group on the page, its picture
// in each online pack (kOnlineIconPacks; none: that pack hasn't one, the
// font's stays) and its emoji (none: the font's).
enum class IconGroup { Status, Media, Controls, Pill, Buttons, Other, Count };
struct IconInfo {
    WCHAR glyph;
    PCWSTR name;
    IconGroup group;
    PCWSTR ionicon;
    PCWSTR lucide;
    PCWSTR fluent;
    PCWSTR tabler;
    PCWSTR phosphor;
    PCWSTR material;
    PCWSTR bootstrap;
    PCWSTR iconoir;
    PCWSTR heroicon;
    PCWSTR emoji;
};
constexpr IconInfo kIcons[] = {
    {0xE701, L"wifi", IconGroup::Status, L"wifi-outline", L"wifi", L"wifi_1",
     L"wifi", L"wifi-high", L"wifi", L"wifi", L"wifi", L"wifi", L"\U0001F4F6"},
    {0xE872, L"wifi-1", IconGroup::Status, L"wifi-outline", L"wifi-low",
     L"wifi_4", L"wifi-0", L"wifi-none", L"wifi_1_bar", L"wifi-1", L"wifi",
     L"wifi", L"\U0001F4F6"},
    {0xE873, L"wifi-2", IconGroup::Status, L"wifi-outline", L"wifi-high",
     L"wifi_3", L"wifi-1", L"wifi-low", L"wifi_2_bar", L"wifi-2", L"wifi",
     L"wifi", L"\U0001F4F6"},
    {0xE874, L"wifi-3", IconGroup::Status, L"wifi-outline", L"wifi", L"wifi_2",
     L"wifi-2", L"wifi-medium", L"wifi", L"wifi", L"wifi", L"wifi",
     L"\U0001F4F6"},
    {0xEB60, L"wifi-warning-1", IconGroup::Status, L"wifi-outline", L"wifi-off",
     L"wifi_warning", L"wifi-off", L"wifi-slash", L"wifi_off", L"wifi-off",
     L"wifi-warning", L"signal-slash", L"\U0001F4F6"},
    {0xEB61, L"wifi-warning-2", IconGroup::Status, L"wifi-outline", L"wifi-off",
     L"wifi_warning", L"wifi-off", L"wifi-slash", L"wifi_off", L"wifi-off",
     L"wifi-warning", L"signal-slash", L"\U0001F4F6"},
    {0xEB62, L"wifi-warning-3", IconGroup::Status, L"wifi-outline", L"wifi-off",
     L"wifi_warning", L"wifi-off", L"wifi-slash", L"wifi_off", L"wifi-off",
     L"wifi-warning", L"signal-slash", L"\U0001F4F6"},
    {0xEB63, L"wifi-warning", IconGroup::Status, L"wifi-outline", L"wifi-off",
     L"wifi_warning", L"wifi-off", L"wifi-slash", L"wifi_off", L"wifi-off",
     L"wifi-warning", L"signal-slash", L"\U0001F4F6"},
    {0xE839, L"ethernet", IconGroup::Status, L"git-network-outline",
     L"ethernet-port", L"plug_connected", L"network", L"network", L"lan",
     L"ethernet", L"network", L"signal", L"\U0001F310"},
    {0xEB56, L"ethernet-warning", IconGroup::Status, L"git-network-outline",
     L"ethernet-port", L"plug_disconnected", L"network-off", L"network-slash",
     L"lan", L"ethernet", L"network", L"signal-slash", L"\U0001F310"},
    {0xF384, L"no-internet", IconGroup::Status, L"cloud-offline-outline",
     L"globe-lock", L"globe_off", L"globe-off", L"globe-x", L"cloud_off",
     L"cloud-slash", L"wifi-off", L"signal-slash", L"\U0001F6AB"},
    {0xE995, L"volume", IconGroup::Status, L"volume-high-outline", L"volume-2",
     L"speaker_2", L"volume", L"speaker-high", L"volume_up", L"volume-up",
     L"sound-high", L"speaker-wave", L"\U0001F50A"},
    {0xE992, L"volume-0", IconGroup::Status, L"volume-off-outline", L"volume",
     L"speaker_0", L"volume-4", L"speaker-none", L"volume_mute", L"volume-off",
     L"sound-min", L"speaker-wave", L"\U0001F508"},
    {0xE993, L"volume-1", IconGroup::Status, L"volume-low-outline", L"volume-1",
     L"speaker_1", L"volume-2", L"speaker-low", L"volume_down", L"volume-down",
     L"sound-low", L"speaker-wave", L"\U0001F509"},
    {0xE994, L"volume-2", IconGroup::Status, L"volume-medium-outline",
     L"volume-2", L"speaker_2", L"volume-2", L"speaker-low", L"volume_down",
     L"volume-down", L"sound-low", L"speaker-wave", L"\U0001F509"},
    {0xE74F, L"mute", IconGroup::Status, L"volume-mute-outline", L"volume-x",
     L"speaker_mute", L"volume-3", L"speaker-x", L"volume_off", L"volume-mute",
     L"sound-off", L"speaker-x-mark", L"\U0001F507"},
    {0xE767, L"speaker", IconGroup::Status, L"volume-high-outline", L"speaker",
     L"speaker_2", L"device-speaker", L"speaker-hifi", L"speaker", L"speaker",
     L"sound-high", L"speaker-wave", L"\U0001F508"},
    {0xE720, L"microphone", IconGroup::Status, L"mic-outline", L"mic", L"mic",
     L"microphone", L"microphone", L"mic", L"mic", L"microphone", L"microphone",
     L"\U0001F3A4"},
    {0xF781, L"microphone-off", IconGroup::Status, L"mic-off-outline",
     L"mic-off", L"mic_off", L"microphone-off", L"microphone-slash", L"mic_off",
     L"mic-mute", L"microphone-mute", nullptr, L"\U0001F507"},
    {0xE83F, L"battery", IconGroup::Status, L"battery-full-outline",
     L"battery-full", L"battery_10", L"battery-4", L"battery-full",
     L"battery_full", L"battery-full", L"battery-full", L"battery-100",
     L"\U0001F50B"},
    {0xE850, L"battery-0", IconGroup::Status, L"battery-dead-outline",
     L"battery", L"battery_0", L"battery", L"battery-empty", L"battery_0_bar",
     L"battery", L"battery-empty", L"battery-0", L"\U0001FAAB"},
    {0xE851, L"battery-1", IconGroup::Status, L"battery-dead-outline",
     L"battery-low", L"battery_1", L"battery-1", L"battery-low",
     L"battery_1_bar", L"battery-low", L"battery-25", L"battery-0",
     L"\U0001FAAB"},
    {0xE852, L"battery-2", IconGroup::Status, L"battery-dead-outline",
     L"battery-low", L"battery_2", L"battery-1", L"battery-low",
     L"battery_1_bar", L"battery-low", L"battery-25", L"battery-0",
     L"\U0001FAAB"},
    {0xE853, L"battery-3", IconGroup::Status, L"battery-half-outline",
     L"battery-low", L"battery_3", L"battery-2", L"battery-low",
     L"battery_2_bar", L"battery-low", L"battery-25", L"battery-50",
     L"\U0001F50B"},
    {0xE854, L"battery-4", IconGroup::Status, L"battery-half-outline",
     L"battery-medium", L"battery_4", L"battery-2", L"battery-medium",
     L"battery_3_bar", L"battery-half", L"battery-50", L"battery-50",
     L"\U0001F50B"},
    {0xE855, L"battery-5", IconGroup::Status, L"battery-half-outline",
     L"battery-medium", L"battery_5", L"battery-3", L"battery-medium",
     L"battery_4_bar", L"battery-half", L"battery-50", L"battery-50",
     L"\U0001F50B"},
    {0xE856, L"battery-6", IconGroup::Status, L"battery-half-outline",
     L"battery-medium", L"battery_6", L"battery-3", L"battery-high",
     L"battery_4_bar", L"battery-half", L"battery-75", L"battery-50",
     L"\U0001F50B"},
    {0xE857, L"battery-7", IconGroup::Status, L"battery-full-outline",
     L"battery-full", L"battery_7", L"battery-4", L"battery-high",
     L"battery_5_bar", L"battery-full", L"battery-75", L"battery-100",
     L"\U0001F50B"},
    {0xE858, L"battery-8", IconGroup::Status, L"battery-full-outline",
     L"battery-full", L"battery_8", L"battery-4", L"battery-full",
     L"battery_6_bar", L"battery-full", L"battery-full", L"battery-100",
     L"\U0001F50B"},
    {0xE859, L"battery-9", IconGroup::Status, L"battery-full-outline",
     L"battery-full", L"battery_9", L"battery-4", L"battery-full",
     L"battery_full", L"battery-full", L"battery-full", L"battery-100",
     L"\U0001F50B"},
    {0xEA93, L"battery-charging", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE85A, L"battery-charging-0", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE85B, L"battery-charging-1", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE85C, L"battery-charging-2", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE85D, L"battery-charging-3", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE85E, L"battery-charging-4", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE85F, L"battery-charging-5", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE860, L"battery-charging-6", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE861, L"battery-charging-7", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE862, L"battery-charging-8", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE863, L"battery-charging-9", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xE83E, L"battery-charging-full", IconGroup::Status,
     L"battery-charging-outline", L"battery-charging", L"battery_charge",
     L"battery-charging", L"battery-charging", L"battery_charging_full",
     L"battery-charging", L"battery-charging", L"bolt", L"\U0001F50C"},
    {0xEA8F, L"bell", IconGroup::Status, L"notifications-outline", L"bell",
     L"alert", L"bell", L"bell", L"notifications", L"bell", L"bell", L"bell",
     L"\U0001F514"},
    {0xE708, L"moon", IconGroup::Status, L"moon-outline", L"moon",
     L"weather_moon", L"moon", L"moon", L"bedtime", L"moon", L"half-moon",
     L"moon", L"\U0001F319"},
    {0xE7ED, L"ringer-silent", IconGroup::Status, L"notifications-off-outline",
     L"bell-off", L"alert_off", L"bell-off", L"bell-slash",
     L"notifications_off", L"bell-slash", L"bell-off", L"bell-slash",
     L"\U0001F515"},
    {0xE823, L"clock", IconGroup::Status, L"time-outline", L"clock", L"clock",
     L"clock", L"clock", L"schedule", L"clock", L"clock", L"clock",
     L"\U0001F552"},
    {0xE787, L"calendar", IconGroup::Status, L"calendar-outline", L"calendar",
     L"calendar", L"calendar", L"calendar", L"calendar_today", L"calendar",
     L"calendar", L"calendar", L"\U0001F4C5"},
    {0xE8BF, L"calendar-day", IconGroup::Status, L"calendar-number-outline",
     L"calendar-days", L"calendar_day", L"calendar-event", L"calendar-dots",
     L"calendar_today", L"calendar-day", L"calendar", L"calendar-days",
     L"\U0001F4C6"},
    {0xE753, L"weather", IconGroup::Status, L"partly-sunny-outline",
     L"cloud-sun", L"weather_partly_cloudy_day", L"cloud", L"cloud-sun",
     L"partly_cloudy_day", L"cloud-sun", L"cloud-sunny", L"cloud", L"\u26C5"},
    {0xE8D6, L"music", IconGroup::Media, L"musical-notes-outline", L"music",
     L"music_note_2", L"music", L"music-notes", L"music_note_2", L"music-note",
     L"music-note", L"musical-note", L"\U0001F3B5"},
    {0xE93C, L"music-album", IconGroup::Media, L"disc-outline", L"disc-3",
     L"album", L"disc", L"disc", L"album", L"disc", L"album", nullptr,
     L"\U0001F4BF"},
    {0xE768, L"play", IconGroup::Media, L"play-outline", L"play", L"play",
     L"player-play", L"play", L"play_arrow", L"play", L"play", L"play",
     L"\u25B6"},
    {0xE769, L"pause", IconGroup::Media, L"pause-outline", L"pause", L"pause",
     L"player-pause", L"pause", L"pause", L"pause", L"pause", L"pause",
     L"\u23F8"},
    {0xE71A, L"stop", IconGroup::Media, L"stop-outline", L"square", L"stop",
     L"player-stop", L"stop", L"stop", L"stop", L"square", L"stop", L"\u23F9"},
    {0xE892, L"previous-track", IconGroup::Media, L"play-skip-back-outline",
     L"skip-back", L"previous", L"player-track-prev", L"skip-back",
     L"skip_previous", L"skip-backward", L"skip-prev", L"backward", L"\u23EE"},
    {0xE893, L"next-track", IconGroup::Media, L"play-skip-forward-outline",
     L"skip-forward", L"next", L"player-track-next", L"skip-forward",
     L"skip_next", L"skip-forward", L"skip-next", L"forward", L"\u23ED"},
    {0xE8B1, L"shuffle", IconGroup::Media, L"shuffle-outline", L"shuffle",
     L"arrow_shuffle", L"arrows-shuffle", L"shuffle", L"shuffle", L"shuffle",
     L"shuffle", L"arrows-right-left", L"\U0001F500"},
    {0xEB9F, L"cover", IconGroup::Media, L"image-outline", L"image", L"image",
     L"photo", L"image", L"photo", L"image", L"media-image", L"photo",
     L"\U0001F5BC"},
    {0xEC4F, L"lyrics", IconGroup::Media, L"mic-circle-outline", L"mic-vocal",
     L"text_quote", L"microphone-2", L"microphone-stage", L"lyrics", L"quote",
     L"quote", L"microphone", L"\U0001F3A4"},
    {0xE714, L"video", IconGroup::Media, L"videocam-outline", L"video",
     L"video", L"video", L"video", L"videocam", L"camera-video",
     L"video-camera", L"video-camera", L"\U0001F3AC"},
    {0xE9E9, L"control-center", IconGroup::Controls, L"options-outline",
     L"sliders-horizontal", L"options", L"adjustments-horizontal",
     L"sliders-horizontal", L"sliders", L"sliders", L"control-slider",
     L"adjustments-horizontal", L"\U0001F39B"},
    {0xE702, L"bluetooth", IconGroup::Controls, L"bluetooth-outline",
     L"bluetooth", L"bluetooth", L"bluetooth", L"bluetooth", L"bluetooth",
     L"bluetooth", L"bluetooth", nullptr, nullptr},
    {0xF08C, L"night-light", IconGroup::Controls, L"moon-outline", L"sun-moon",
     L"weather_moon", L"sun-moon", L"moon-stars", L"moon_stars", L"moon-stars",
     L"half-moon", L"moon", L"\U0001F319"},
    {0xE88A, L"hotspot", IconGroup::Controls, L"radio-outline", L"radio-tower",
     L"cellular_data_1", L"antenna-bars-5", L"broadcast", L"wifi_tethering",
     L"broadcast", L"antenna", L"radio", L"\U0001F4E1"},
    {0xE709, L"airplane", IconGroup::Controls, L"airplane-outline", L"plane",
     L"airplane", L"plane", L"airplane", L"flight", L"airplane", L"airplane",
     L"paper-airplane", L"\u2708"},
    {0xE706, L"brightness", IconGroup::Controls, L"sunny-outline", L"sun",
     L"weather_sunny", L"sun", L"sun", L"brightness_5", L"sun", L"sun-light",
     L"sun", L"\U0001F506"},
    {0xEBC6, L"project", IconGroup::Controls, L"tv-outline", L"monitor-up",
     L"cast", L"screen-share", L"monitor", L"screen_share", L"cast", L"tv",
     L"tv", L"\U0001F4FD"},
    {0xEC15, L"cast", IconGroup::Controls, L"tv-outline", L"cast", L"cast",
     L"cast", L"screencast", L"cast", L"cast", L"airplay", L"tv",
     L"\U0001F4FA"},
    {0xE7F0, L"captions", IconGroup::Controls, L"chatbox-outline", L"captions",
     L"closed_caption", L"badge-cc", L"closed-captioning", L"closed_caption",
     L"badge-cc", nullptr, nullptr, L"\U0001F4AC"},
    {0xE776, L"accessibility", IconGroup::Controls, L"accessibility-outline",
     L"accessibility", L"accessibility", L"accessible", L"person-arms-spread",
     L"accessibility", L"universal-access", L"accessibility", nullptr,
     L"\u267F"},
    {0xE71E, L"magnifier", IconGroup::Controls, L"search-outline", L"zoom-in",
     L"zoom_in", L"zoom-in", L"magnifying-glass-plus", L"zoom_in", L"zoom-in",
     L"zoom-in", L"magnifying-glass-plus", L"\U0001F50D"},
    {0xEBC5, L"narrator", IconGroup::Controls, L"ear-outline", L"ear",
     L"person_voice", L"ear", L"ear", L"hearing", L"ear", L"sound-high",
     L"speaker-wave", L"\U0001F442"},
    {0xE790, L"color-filters", IconGroup::Controls, L"color-filter-outline",
     L"palette", L"color", L"color-filter", L"palette", L"palette", L"palette",
     L"color-filter", L"swatch", L"\U0001F3A8"},
    {0xE793, L"dark-mode", IconGroup::Controls, L"contrast-outline", L"moon",
     L"weather_moon", L"moon", L"moon", L"dark_mode", L"moon", L"half-moon",
     L"moon", L"\U0001F313"},
    {0xE7A1, L"light-and-dark", IconGroup::Controls, L"invert-mode-outline",
     L"sun-moon", L"dark_theme", L"sun-moon", L"circle-half", L"contrast",
     L"circle-half", L"half-moon", nullptr, L"\U0001F313"},
    {0xE722, L"screenshot", IconGroup::Controls, L"crop-outline", L"scissors",
     L"screenshot", L"screenshot", L"scissors", L"crop", L"scissors",
     L"screenshot", L"scissors", L"\u2702"},
    {0xE72E, L"lock", IconGroup::Controls, L"lock-closed-outline", L"lock",
     L"lock_closed", L"lock", L"lock", L"lock", L"lock", L"lock",
     L"lock-closed", L"\U0001F512"},
    {0xE76E, L"emoji", IconGroup::Controls, L"happy-outline", L"smile",
     L"emoji", L"mood-smile", L"smiley", L"mood", L"emoji-smile", L"emoji",
     L"face-smile", L"\U0001F600"},
    {0xF0E3, L"clipboard", IconGroup::Controls, L"clipboard-outline",
     L"clipboard", L"clipboard", L"clipboard", L"clipboard", L"content_paste",
     L"clipboard", L"clipboard-check", L"clipboard", L"\U0001F4CB"},
    {0xE9D9, L"task-manager", IconGroup::Controls, L"pulse-outline",
     L"activity", L"pulse", L"activity", L"pulse", L"monitoring", L"activity",
     L"activity", L"chart-bar", L"\U0001F4C8"},
    {0xE8EF, L"calculator", IconGroup::Controls, L"calculator-outline",
     L"calculator", L"calculator", L"calculator", L"calculator", L"calculate",
     L"calculator", L"calculator", L"calculator", L"\U0001F9EE"},
    {0xEC50, L"explorer", IconGroup::Controls, L"folder-outline", L"folder",
     L"folder", L"folder", L"folder", L"folder", L"folder", L"folder",
     L"folder", L"\U0001F4C1"},
    {0xE7F4, L"desktop", IconGroup::Controls, L"desktop-outline", L"monitor",
     L"desktop", L"device-desktop", L"monitor", L"monitor", L"display",
     L"computer", L"computer-desktop", L"\U0001F5A5"},
    {0xE765, L"keyboard", IconGroup::Controls, L"keypad-outline", L"keyboard",
     L"keyboard", L"keyboard", L"keyboard", L"keyboard", L"keyboard", nullptr,
     nullptr, L"\u2328"},
    {0xE962, L"mouse", IconGroup::Controls, L"navigate-outline", L"mouse",
     nullptr, L"mouse", L"mouse", L"mouse", L"mouse", L"pc-mouse",
     L"cursor-arrow-rays", L"\U0001F5B1"},
    {0xE7F6, L"headphones", IconGroup::Controls, L"headset-outline",
     L"headphones", L"headphones", L"headphones", L"headphones", L"headphones",
     L"headphones", L"headset", nullptr, L"\U0001F3A7"},
    {0xE7F5, L"speakers", IconGroup::Controls, L"volume-high-outline",
     L"speaker", L"speaker_2", L"device-speaker", L"speaker-hifi", L"speaker",
     L"speaker", L"sound-high", L"speaker-wave", L"\U0001F508"},
    {0xE7E8, L"power", IconGroup::Controls, L"power-outline", L"power",
     L"power", L"power", L"power", L"power", L"power", nullptr, L"power",
     L"\U0001F50C"},
    {0xEC48, L"power-mode", IconGroup::Controls, L"speedometer-outline",
     L"gauge", L"gauge", L"gauge", L"gauge", L"speed", L"speedometer",
     L"dashboard-speed", nullptr, L"\u26A1"},
    {0xE945, L"lightning-bolt", IconGroup::Controls, L"flash-outline", L"zap",
     L"flash", L"bolt", L"lightning", L"bolt", L"lightning", L"flash", L"bolt",
     L"\u26A1"},
    {0xE71D, L"apps", IconGroup::Pill, L"apps-outline", L"layout-grid", L"apps",
     L"layout-grid", L"grid-four", L"apps", L"grid", L"view-grid",
     L"squares-2x2", L"\U0001F4F1"},
    {0xE712, L"more", IconGroup::Pill, L"ellipsis-horizontal-outline",
     L"ellipsis", L"more_horizontal", L"dots", L"dots-three", L"more_horiz",
     L"three-dots", L"more-horiz", L"ellipsis-horizontal", nullptr},
    {0xE70B, L"notes", IconGroup::Pill, L"document-text-outline",
     L"notebook-pen", L"note", L"note", L"note", L"notes", L"journal-text",
     L"notes", L"pencil-square", L"\U0001F4DD"},
    {0xE9D5, L"tasks", IconGroup::Pill, L"checkbox-outline", L"list-checks",
     L"task_list_ltr", L"checklist", L"list-checks", L"checklist",
     L"list-check", L"task-list", L"clipboard-document-check", L"\u2705"},
    {0xE895, L"updates", IconGroup::Pill, L"cloud-download-outline",
     L"refresh-cw", L"arrow_sync", L"refresh", L"arrows-clockwise", L"refresh",
     L"arrow-repeat", L"refresh", L"arrow-path", L"\U0001F504"},
    {0xE748, L"profiles", IconGroup::Pill, L"people-outline", L"users",
     L"people", L"users", L"users", L"group", L"people", L"group", L"users",
     L"\U0001F465"},
    {0xE774, L"keyboard-language", IconGroup::Pill, L"language-outline",
     L"languages", L"translate", L"language", L"translate", L"language",
     L"translate", L"language", L"language", L"\U0001F310"},
    {0xEA18, L"privacy", IconGroup::Pill, L"shield-checkmark-outline",
     L"shield", L"shield", L"shield", L"shield", L"shield", L"shield",
     L"shield", L"shield-check", L"\U0001F6E1"},
    {0xE88E, L"drives", IconGroup::Pill, L"server-outline", L"usb", L"usb",
     L"usb", L"usb", L"usb", L"usb", L"usb", L"server", L"\U0001F4BE"},
    {0xE7C4, L"desktops", IconGroup::Pill, L"albums-outline", L"layers",
     L"stack", L"stack", L"stack", L"layers", L"layers", L"copy",
     L"rectangle-stack", L"\U0001F5C2"},
    {0xE8B7, L"folders", IconGroup::Pill, L"folder-open-outline",
     L"folder-open", L"folder_open", L"folder-open", L"folder-open",
     L"folder_open", L"folder2-open", L"folder", L"folder-open", L"\U0001F4C2"},
    {0xE8B9, L"wallpaper", IconGroup::Pill, L"image-outline", L"image",
     L"wallpaper", L"wallpaper", L"image", L"wallpaper", L"image",
     L"media-image", L"photo", L"\U0001F5BC"},
    {0xEC4A, L"network-speed", IconGroup::Pill, L"speedometer-outline",
     L"gauge", L"gauge", L"gauge", L"gauge", L"speed", L"speedometer",
     L"dashboard-speed", nullptr, L"\U0001F4F6"},
    {0xE909, L"world-clocks", IconGroup::Pill, L"globe-outline", L"globe",
     L"globe", L"globe", L"globe", L"globe", L"globe", L"globe", L"globe-alt",
     L"\U0001F30D"},
    {0xE916, L"timer", IconGroup::Pill, L"timer-outline", L"timer", L"timer",
     L"hourglass", L"timer", L"timer", L"hourglass", L"timer", L"clock",
     L"\u23F1"},
    {0xE74D, L"recycle-bin", IconGroup::Pill, L"trash-outline", L"trash-2",
     L"delete", L"trash", L"trash", L"delete", L"trash", L"trash", L"trash",
     L"\U0001F5D1"},
    {0xECAA, L"focused-app", IconGroup::Pill, L"browsers-outline",
     L"app-window", L"window", L"app-window", L"app-window", L"window",
     L"window", L"app-window", L"window", L"\U0001FA9F"},
    {0xE9F5, L"komorebi", IconGroup::Pill, L"grid-outline", L"layout-dashboard",
     L"grid", L"layout-dashboard", L"layout", L"dashboard", L"grid",
     L"dashboard", L"squares-2x2", nullptr},
    {0xE80A, L"komorebi-layout", IconGroup::Pill, L"grid-outline",
     L"layout-grid", L"grid", L"layout-grid", L"grid-four", L"view_quilt",
     L"grid", L"view-grid", L"squares-2x2", nullptr},
    {0xF246, L"widgets", IconGroup::Pill, L"apps-outline", L"layout-panel-top",
     L"apps", L"layout-dashboard", L"squares-four", L"widgets", L"grid-1x2",
     L"dashboard", L"squares-plus", nullptr},
    {0xEEA1, L"cpu", IconGroup::Pill, L"hardware-chip-outline", L"cpu",
     L"developer_board", L"cpu", L"cpu", L"memory", L"cpu", L"cpu", L"cpu-chip",
     nullptr},
    {0xE7E7, L"notification", IconGroup::Pill, L"notifications-outline",
     L"bell-dot", L"alert_badge", L"bell-ringing", L"bell-ringing",
     L"notification_important", L"bell", L"bell-notification", L"bell-alert",
     L"\U0001F514"},
    {0xEA86, L"puzzle", IconGroup::Pill, L"extension-puzzle-outline", L"puzzle",
     L"puzzle_piece", L"puzzle", L"puzzle-piece", L"extension", L"puzzle",
     L"puzzle", L"puzzle-piece", L"\U0001F9E9"},
    {0xE713, L"settings", IconGroup::Buttons, L"settings-outline", L"settings",
     L"settings", L"settings", L"gear", L"settings", L"gear", L"settings",
     L"cog-6-tooth", L"\u2699"},
    {0xE718, L"pin", IconGroup::Buttons, L"pin-outline", L"pin", L"pin", L"pin",
     L"push-pin", L"pin", L"pin", L"pin", nullptr, L"\U0001F4CC"},
    {0xE70E, L"chevron-up", IconGroup::Buttons, L"chevron-up-outline",
     L"chevron-up", L"chevron_up", L"chevron-up", L"caret-up",
     L"keyboard_arrow_up", L"chevron-up", L"nav-arrow-up", L"chevron-up",
     nullptr},
    {0xE70D, L"chevron-down", IconGroup::Buttons, L"chevron-down-outline",
     L"chevron-down", L"chevron_down", L"chevron-down", L"caret-down",
     L"keyboard_arrow_down", L"chevron-down", L"nav-arrow-down",
     L"chevron-down", nullptr},
    {0xE76B, L"chevron-left", IconGroup::Buttons, L"chevron-back-outline",
     L"chevron-left", L"chevron_left", L"chevron-left", L"caret-left",
     L"chevron_left", L"chevron-left", L"nav-arrow-left", L"chevron-left",
     nullptr},
    {0xE76C, L"chevron-right", IconGroup::Buttons, L"chevron-forward-outline",
     L"chevron-right", L"chevron_right", L"chevron-right", L"caret-right",
     L"chevron_right", L"chevron-right", L"nav-arrow-right", L"chevron-right",
     nullptr},
    {0xE74A, L"up", IconGroup::Buttons, L"arrow-up-outline", L"arrow-up",
     L"arrow_up", L"arrow-up", L"arrow-up", L"arrow_upward", L"arrow-up",
     L"arrow-up", L"arrow-up", L"\u2B06"},
    {0xE74B, L"down", IconGroup::Buttons, L"arrow-down-outline", L"arrow-down",
     L"arrow_down", L"arrow-down", L"arrow-down", L"arrow_downward",
     L"arrow-down", L"arrow-down", L"arrow-down", L"\u2B07"},
    {0xE72B, L"back", IconGroup::Buttons, L"arrow-back-outline", L"arrow-left",
     L"arrow_left", L"arrow-left", L"arrow-left", L"arrow_left", L"arrow-left",
     L"arrow-left", L"arrow-left", L"\u2B05"},
    {0xE711, L"close", IconGroup::Buttons, L"close-outline", L"x", L"dismiss",
     L"x", L"x", L"close", L"x", L"x", L"x-mark", L"\u274C"},
    {0xE89F, L"close-pane", IconGroup::Buttons, L"close-circle-outline",
     L"panel-right-close", L"panel_right_contract",
     L"layout-sidebar-right-collapse", L"x-circle", L"right_panel_close",
     L"x-circle", L"sidebar-collapse", L"x-circle", nullptr},
    {0xE894, L"clear", IconGroup::Buttons, L"close-outline", L"x", L"dismiss",
     L"x", L"x", L"close", L"x", L"x", L"x-mark", L"\u274C"},
    {0xE73E, L"check", IconGroup::Buttons, L"checkmark-outline", L"check",
     L"check", L"check", L"check", L"check", L"check", L"check", L"check",
     L"\u2714"},
    {0xE710, L"add", IconGroup::Buttons, L"add-outline", L"plus", L"add",
     L"plus", L"plus", L"add", L"plus", L"plus", L"plus", L"\u2795"},
    {0xE738, L"remove", IconGroup::Buttons, L"remove-outline", L"minus",
     L"subtract", L"minus", L"minus", L"remove", L"subtract", L"minus",
     L"minus", L"\u2796"},
    {0xE70F, L"edit", IconGroup::Buttons, L"create-outline", L"pencil", L"edit",
     L"pencil", L"pencil", L"edit", L"pencil", L"edit", L"pencil", L"\u270F"},
    {0xE8C8, L"copy", IconGroup::Buttons, L"copy-outline", L"copy", L"copy",
     L"copy", L"copy", L"content_copy", L"copy", L"copy", L"document-duplicate",
     nullptr},
    {0xE77F, L"paste", IconGroup::Buttons, L"clipboard-outline",
     L"clipboard-paste", L"clipboard_paste", L"clipboard-copy",
     L"clipboard-text", L"content_paste", L"clipboard", L"paste-clipboard",
     L"clipboard", L"\U0001F4CB"},
    {0xE724, L"send", IconGroup::Buttons, L"send-outline", L"send", L"send",
     L"send", L"paper-plane-right", L"send", L"send", L"send",
     L"paper-airplane", L"\U0001F4E8"},
    {0xE72C, L"reload", IconGroup::Buttons, L"reload-outline", L"rotate-cw",
     L"arrow_clockwise", L"refresh", L"arrow-clockwise", L"refresh",
     L"arrow-clockwise", L"refresh", L"arrow-path", L"\U0001F504"},
    {0xE7A7, L"undo", IconGroup::Buttons, L"arrow-undo-outline", L"undo-2",
     L"arrow_undo", L"arrow-back-up", L"arrow-u-up-left", L"undo",
     L"arrow-counterclockwise", L"undo", L"arrow-uturn-left", L"\u21A9"},
    {0xE7B3, L"show", IconGroup::Buttons, L"eye-outline", L"eye", L"eye",
     L"eye", L"eye", L"visibility", L"eye", L"eye", L"eye", L"\U0001F441"},
    {0xED1A, L"hide", IconGroup::Buttons, L"eye-off-outline", L"eye-off",
     L"eye_off", L"eye-off", L"eye-slash", L"visibility_off", L"eye-slash",
     L"eye-closed", L"eye-slash", nullptr},
    {0xE721, L"search", IconGroup::Buttons, L"search-outline", L"search",
     L"search", L"search", L"magnifying-glass", L"search", L"search", L"search",
     L"magnifying-glass", L"\U0001F50E"},
    {0xE71C, L"filter", IconGroup::Buttons, L"filter-outline", L"filter",
     L"filter", L"filter", L"funnel", L"filter", L"filter", L"filter",
     L"funnel", nullptr},
    {0xE8CB, L"sort", IconGroup::Buttons, L"swap-vertical-outline",
     L"arrow-down-up", L"arrow_sort", L"arrows-sort", L"sort-ascending",
     L"sort", L"arrow-down-up", L"sort", L"arrows-up-down", nullptr},
    {0xE71B, L"link", IconGroup::Buttons, L"link-outline", L"link", L"link",
     L"link", L"link", L"link", L"link", L"link", L"link", L"\U0001F517"},
    {0xE8A7, L"open-in-new-window", IconGroup::Buttons, L"open-outline",
     L"external-link", L"open", L"external-link", L"arrow-square-out",
     L"open_in_new", L"box-arrow-up-right", L"open-new-window",
     L"arrow-top-right-on-square", nullptr},
    {0xE740, L"full-screen", IconGroup::Buttons, L"expand-outline", L"maximize",
     L"maximize", L"maximize", L"corners-out", L"maximize",
     L"arrows-fullscreen", L"maximize", L"arrows-pointing-out", nullptr},
    {0xE921, L"minimize", IconGroup::Buttons, L"remove-outline", L"minus",
     L"subtract", L"minus", L"minus", L"remove", L"subtract", L"minus",
     L"minus", nullptr},
    {0xE734, L"favorite-star", IconGroup::Buttons, L"star-outline", L"star",
     L"star", L"star", L"star", L"star", L"star", L"star", L"star", L"\u2B50"},
    {0xE735, L"favorite-star-fill", IconGroup::Buttons, L"star", L"star",
     L"star", L"star", L"star", L"star", L"star", L"star", L"star", L"\u2B50"},
    {0xEB51, L"heart", IconGroup::Buttons, L"heart-outline", L"heart", L"heart",
     L"heart", L"heart", L"favorite", L"heart", L"heart", L"heart", L"\u2764"},
    {0xE7C1, L"flag", IconGroup::Buttons, L"flag-outline", L"flag", L"flag",
     L"flag", L"flag", L"flag", L"flag", L"triangle-flag", L"flag",
     L"\U0001F6A9"},
    {0xE7BA, L"warning", IconGroup::Buttons, L"warning-outline",
     L"triangle-alert", L"warning", L"alert-triangle", L"warning", L"warning",
     L"exclamation-triangle", L"warning-triangle", L"exclamation-triangle",
     L"\u26A0"},
    {0xE946, L"info", IconGroup::Buttons, L"information-circle-outline",
     L"info", L"info", L"info-circle", L"info", L"info", L"info",
     L"info-circle", L"information-circle", L"\u2139"},
    {0xE896, L"download", IconGroup::Buttons, L"download-outline", L"download",
     L"arrow_download", L"download", L"download", L"download", L"download",
     L"download", L"arrow-down-tray", L"\U0001F4E5"},
    {0xE838, L"folder-open", IconGroup::Buttons, L"folder-open-outline",
     L"folder-open", L"folder_open", L"folder-open", L"folder-open",
     L"folder_open", L"folder2-open", L"folder", L"folder-open", L"\U0001F4C2"},
    {0xE8F4, L"new-folder", IconGroup::Buttons, L"folder-outline",
     L"folder-plus", L"folder_add", L"folder-plus", L"folder-plus",
     L"create_new_folder", L"folder-plus", L"folder-plus", L"folder-plus",
     L"\U0001F4C1"},
    {0xE8A5, L"document", IconGroup::Buttons, L"document-outline", L"file",
     L"document", L"file", L"file", L"description", L"file", L"page",
     L"document", L"\U0001F4C4"},
    {0xE7C3, L"page", IconGroup::Buttons, L"document-outline", L"file-text",
     L"document_text", L"file-text", L"file-text", L"description", L"file-text",
     L"page", L"document-text", L"\U0001F4C4"},
    {0xE91B, L"photo", IconGroup::Buttons, L"image-outline", L"image", L"image",
     L"photo", L"image", L"photo", L"image", L"media-image", L"photo",
     L"\U0001F5BC"},
    {0xE8F1, L"library", IconGroup::Buttons, L"library-outline", L"library",
     L"library", L"library", L"books", L"local_library", L"book", L"book-stack",
     L"book-open", L"\U0001F4DA"},
    {0xE715, L"mail", IconGroup::Buttons, L"mail-outline", L"mail", L"mail",
     L"mail", L"envelope", L"mail", L"envelope", L"mail", L"envelope",
     L"\U0001F4E7"},
    {0xE8BD, L"message", IconGroup::Buttons, L"chatbubble-outline",
     L"message-square", L"chat", L"message", L"chat", L"chat", L"chat",
     L"message", L"chat-bubble-left", L"\U0001F4AC"},
    {0xE716, L"people", IconGroup::Buttons, L"people-outline", L"users",
     L"people", L"users", L"users", L"group", L"people", L"group", L"users",
     L"\U0001F465"},
    {0xE77B, L"contact", IconGroup::Buttons, L"person-outline", L"user",
     L"person", L"user", L"user", L"person", L"person", L"user", L"user",
     L"\U0001F464"},
    {0xE80F, L"home", IconGroup::Buttons, L"home-outline", L"house", L"home",
     L"home", L"house", L"house", L"house", L"home", L"home", L"\U0001F3E0"},
    {0xE81C, L"history", IconGroup::Buttons, L"time-outline", L"history",
     L"history", L"history", L"clock-counter-clockwise", L"history",
     L"clock-history", L"clock-rotate-right", L"clock", L"\U0001F552"},
    {0xE707, L"map-pin", IconGroup::Buttons, L"location-outline", L"map-pin",
     L"location", L"map-pin", L"map-pin", L"location_on", L"geo-alt",
     L"map-pin", L"map-pin", L"\U0001F4CD"},
    {0xE81D, L"location", IconGroup::Buttons, L"navigate-outline", L"locate",
     L"my_location", L"current-location", L"crosshair", L"my_location",
     L"crosshair", L"compass", L"map-pin", L"\U0001F4CD"},
    {0xE81E, L"map-layers", IconGroup::Buttons, L"layers-outline", L"layers",
     L"stack", L"stack", L"stack", L"layers", L"layers", L"map",
     L"rectangle-stack", L"\U0001F5FA"},
    {0xE8EC, L"tag", IconGroup::Buttons, L"pricetag-outline", L"tag", L"tag",
     L"tag", L"tag", L"tag", L"tag", L"label", L"tag", L"\U0001F3F7"},
    {0xE719, L"shop", IconGroup::Buttons, L"bag-handle-outline",
     L"shopping-bag", L"shopping_bag", L"shopping-bag", L"shopping-bag",
     L"shopping_bag", L"bag", L"shopping-bag", L"shopping-bag", L"\U0001F6CD"},
    {0xE7FC, L"game", IconGroup::Buttons, L"game-controller-outline",
     L"gamepad-2", L"games", L"device-gamepad-2", L"game-controller",
     L"sports_esports", L"controller", L"gamepad", nullptr, L"\U0001F3AE"},
    {0xE825, L"bank", IconGroup::Buttons, L"card-outline", L"landmark",
     L"building_bank", L"building-bank", L"bank", L"account_balance", L"bank",
     L"bank", L"building-library", L"\U0001F3E6"},
    {0xE821, L"work", IconGroup::Buttons, L"briefcase-outline", L"briefcase",
     L"briefcase", L"briefcase", L"briefcase", L"work", L"briefcase",
     L"suitcase", L"briefcase", L"\U0001F4BC"},
    {0xE7BE, L"education", IconGroup::Buttons, L"school-outline",
     L"graduation-cap", L"hat_graduation", L"school", L"graduation-cap",
     L"school", L"mortarboard", L"graduation-cap", L"academic-cap",
     L"\U0001F393"},
    {0xE789, L"megaphone", IconGroup::Buttons, L"megaphone-outline",
     L"megaphone", L"megaphone", L"speakerphone", L"megaphone", L"campaign",
     L"megaphone", L"megaphone", L"megaphone", L"\U0001F4E3"},
    {0xE7C2, L"move", IconGroup::Buttons, L"move-outline", L"move",
     L"arrow_move", L"arrows-move", L"arrows-out-cardinal", L"move",
     L"arrows-move", L"drag", L"arrows-pointing-out", nullptr},
    {0xEBE8, L"bug", IconGroup::Buttons, L"bug-outline", L"bug", L"bug", L"bug",
     L"bug", L"bug_report", L"bug", L"bug", L"bug-ant", L"\U0001F41B"},
    {0xE943, L"code", IconGroup::Buttons, L"code-slash-outline", L"code",
     L"code", L"code", L"code", L"code", L"code", L"code", L"code-bracket",
     nullptr},
    {0xEDA2, L"hard-drive", IconGroup::Buttons, L"server-outline",
     L"hard-drive", L"hard_drive", L"database", L"hard-drive", L"hard_drive",
     L"hdd", L"hard-drive", L"server", L"\U0001F4BD"},
    {0xE7F8, L"laptop", IconGroup::Buttons, L"laptop-outline", L"laptop",
     L"laptop", L"device-laptop", L"laptop", L"laptop_mac", L"laptop",
     L"laptop", L"computer-desktop", L"\U0001F4BB"},
    {0xF3B1, L"sign-out", IconGroup::Buttons, L"log-out-outline", L"log-out",
     L"sign_out", L"logout", L"sign-out", L"logout", L"box-arrow-right",
     L"log-out", L"arrow-right-on-rectangle", nullptr},
    {0xE770, L"system", IconGroup::Other, L"desktop-outline", L"monitor-cog",
     L"desktop", L"device-desktop-cog", L"monitor", L"settings_applications",
     L"display", L"computer", L"computer-desktop", L"\U0001F5A5"},
    {0xE771, L"personalize", IconGroup::Other, L"brush-outline", L"paintbrush",
     L"paint_brush", L"brush", L"paint-brush", L"brush", L"brush", L"palette",
     L"paint-brush", L"\U0001F58C"},
    {0xE772, L"devices", IconGroup::Other, L"phone-portrait-outline",
     L"smartphone", L"phone", L"device-mobile", L"device-mobile", L"mobile",
     L"phone", L"phone", L"phone", L"\U0001F4F1"},
    {0xE777, L"update-restore", IconGroup::Other, L"refresh-outline",
     L"history", L"history", L"history", L"clock-counter-clockwise", L"history",
     L"clock-history", L"refresh", L"arrow-uturn-left", L"\U0001F504"},
    {0xE74C, L"oem", IconGroup::Other, L"construct-outline", L"wrench",
     L"wrench", L"tool", L"wrench", L"build", L"wrench", L"wrench", L"wrench",
     L"\U0001F527"},
    {0xE75B, L"dock", IconGroup::Other, L"browsers-outline", L"panel-bottom",
     L"dock", L"layout-bottombar", L"browser", L"bottom_panel_open",
     L"window-dock", L"mac-dock", nullptr, nullptr},
    {0xE7C9, L"touch-pointer", IconGroup::Other, L"hand-left-outline",
     L"pointer", L"hand_left", L"pointer", L"cursor-click", L"touch_app",
     L"hand-index", L"one-finger-select-hand-gesture", L"cursor-arrow-ripple",
     L"\U0001F446"},
    {0xE8A3, L"zoom-in", IconGroup::Other, L"add-circle-outline", L"zoom-in",
     L"zoom_in", L"zoom-in", L"magnifying-glass-plus", L"zoom_in", L"zoom-in",
     L"zoom-in", L"magnifying-glass-plus", L"\U0001F50D"},
    {0xE8A9, L"view-all", IconGroup::Other, L"grid-outline", L"layout-grid",
     L"apps", L"layout-grid", L"grid-four", L"apps", L"grid", L"view-grid",
     L"squares-2x2", nullptr},
    {0xE8AB, L"switch", IconGroup::Other, L"swap-horizontal-outline",
     L"arrow-left-right", L"arrow_swap", L"arrow-left-right",
     L"arrows-left-right", L"swap_horiz", L"arrow-left-right",
     L"arrow-separate", L"arrows-right-left", L"\U0001F500"},
    {0xE8B4, L"orientation", IconGroup::Other, L"phone-landscape-outline",
     L"rotate-ccw-square", L"arrow_rotate_counterclockwise",
     L"device-mobile-rotated", L"device-rotate", L"screen_rotation_alt",
     L"phone-landscape", L"rotate-camera-left", nullptr, nullptr},
    {0xE8D2, L"font", IconGroup::Other, L"text-outline", L"type", L"text_font",
     L"typography", L"text-aa", L"text_fields", L"type", L"type", nullptr,
     L"\U0001F524"},
    {0xE8E9, L"font-size", IconGroup::Other, L"text-outline", L"a-large-small",
     L"text_font_size", L"text-size", L"text-aa", L"text_increase", L"type",
     L"text-size", nullptr, L"\U0001F520"},
    {0xE8FD, L"bulleted-list", IconGroup::Other, L"list-outline", L"list",
     L"list", L"list", L"list", L"list", L"list", L"list", L"list-bullet",
     nullptr},
    {0xE90C, L"dock-left", IconGroup::Other, L"browsers-outline", L"panel-left",
     L"panel_left", L"layout-sidebar", L"sidebar", L"left_panel_open",
     L"layout-sidebar", L"layout-left", nullptr, nullptr},
    {0xE950, L"component", IconGroup::Other, L"cube-outline", L"component",
     L"puzzle_piece", L"cube", L"puzzle-piece", L"box", L"box", L"component",
     L"puzzle-piece", L"\U0001F9F1"},
    {0xE964, L"smartcard", IconGroup::Other, L"card-outline", L"credit-card",
     L"payment", L"credit-card", L"credit-card", L"credit_card", L"credit-card",
     L"credit-card", L"credit-card", L"\U0001F4B3"},
    {0xE9A6, L"fit-page", IconGroup::Other, L"expand-outline", L"scan", L"scan",
     L"scan", L"scan", L"scan", L"aspect-ratio", L"maximize",
     L"arrows-pointing-out", nullptr},
    {0xE9F9, L"report-document", IconGroup::Other, L"document-text-outline",
     L"file-chart-column", L"document_data", L"file-chart", L"chart-bar",
     L"report", L"file-earmark-bar-graph", L"stats-report", L"chart-bar",
     L"\U0001F4CA"},
    {0xEA3A, L"circle-ring", IconGroup::Other, L"ellipse-outline", L"circle",
     L"circle", L"circle", L"circle", L"circle", L"circle", L"circle", nullptr,
     L"\u2B55"},
    {0xEC7A, L"developer-tools", IconGroup::Other, L"hammer-outline", L"hammer",
     L"toolbox", L"hammer", L"hammer", L"construction", L"hammer", L"hammer",
     L"wrench", L"\U0001F6E0"},
    {0xECA5, L"tiles", IconGroup::Other, L"apps-outline", L"layout-grid",
     L"apps", L"layout-grid", L"grid-four", L"apps", L"grid", L"view-grid",
     L"squares-2x2", nullptr},
    {0xEDA8, L"brush-size", IconGroup::Other, L"brush-outline", L"brush",
     L"paint_brush", L"brush", L"paint-brush", L"brush", L"brush", nullptr,
     L"paint-brush", L"\U0001F58C"},
    {0xF000, L"knowledge-article", IconGroup::Other, L"book-outline",
     L"book-open", L"book_open", L"book", L"book-open", L"book", L"book",
     L"book", L"book-open", L"\U0001F4D6"},
    {0xF0E2, L"grid-view", IconGroup::Other, L"grid-outline", L"grid-3x3",
     L"grid", L"grid-3x3", L"grid-nine", L"grid_3x3", L"grid-3x3",
     L"dots-grid-3x3", L"squares-2x2", nullptr},
    {0xF4A5, L"effect-size", IconGroup::Other, L"sparkles-outline", L"sparkles",
     L"sparkle", L"sparkles", L"sparkle", L"stars", L"stars", L"magic-wand",
     L"sparkles", L"\u2728"},
};

const IconInfo* FindIcon(WCHAR glyph) {
    for (const auto& icon : kIcons) {
        if (icon.glyph == glyph) {
            return &icon;
        }
    }
    return nullptr;
}

// The icon packs (islandIconPack), in the order the settings list them: the
// font's (Windows 11's; Windows 10's where it has the icon), Segoe UI
// Emoji's, the online ones (downloaded once, see kOnlineIconPacks), or a
// folder of the user's (g_iconFolder). Pictures chosen for single icons
// (g_iconFiles) go over any of them; an icon a pack hasn't is the font's.
enum IconPackId {
    kIconPackWindows,
    kIconPackWindows10,
    kIconPackEmoji,
    kIconPackFluent,
    kIconPackIonicons,
    kIconPackLucide,
    kIconPackTabler,
    kIconPackPhosphor,
    kIconPackMaterial,
    kIconPackBootstrap,
    kIconPackIconoir,
    kIconPackHeroicons,
    kIconPackFolder,
    kIconPacks
};
// The online packs (all free to use: MIT, or Apache 2.0 for Google's): where
// their pictures are (one version, on jsDelivr's CDN; each file its name in
// the pack then `ending`), the folder they're kept in (in the mod's
// storage), their name for each icon, and who makes them and their license
// (as the settings say). Their pictures are only shapes: drawn in the
// icons' colors.
struct OnlineIconPack {
    int pack;
    PCWSTR folder;
    PCWSTR url;
    PCWSTR ending;
    PCWSTR IconInfo::*name;
    PCWSTR by;
    PCWSTR license;
};
constexpr OnlineIconPack kOnlineIconPacks[] = {
    {kIconPackFluent, L"fluent-1.1.343",
     L"https://cdn.jsdelivr.net/npm/@fluentui/svg-icons@1.1.343/icons/",
     L"_24_regular.svg", &IconInfo::fluent, L"Microsoft", L"MIT"},
    {kIconPackIonicons, L"ionicons-7.4.0",
     L"https://cdn.jsdelivr.net/npm/ionicons@7.4.0/dist/svg/", L".svg",
     &IconInfo::ionicon, L"Ionic", L"MIT"},
    {kIconPackLucide, L"lucide-1.40.0",
     L"https://cdn.jsdelivr.net/npm/lucide-static@1.40.0/icons/", L".svg",
     &IconInfo::lucide, L"Lucide", L"ISC"},
    {kIconPackTabler, L"tabler-3.48.0",
     L"https://cdn.jsdelivr.net/npm/@tabler/icons@3.48.0/icons/outline/",
     L".svg", &IconInfo::tabler, L"Tabler", L"MIT"},
    {kIconPackPhosphor, L"phosphor-2.1.1",
     L"https://cdn.jsdelivr.net/npm/@phosphor-icons/core@2.1.1/assets/"
     L"regular/",
     L".svg", &IconInfo::phosphor, L"Phosphor", L"MIT"},
    {kIconPackMaterial, L"material-symbols-0.47.6",
     L"https://cdn.jsdelivr.net/npm/@material-symbols/svg-400@0.47.6/"
     L"outlined/",
     L".svg", &IconInfo::material, L"Google", L"Apache 2.0"},
    {kIconPackBootstrap, L"bootstrap-icons-1.13.1",
     L"https://cdn.jsdelivr.net/npm/bootstrap-icons@1.13.1/icons/", L".svg",
     &IconInfo::bootstrap, L"Bootstrap", L"MIT"},
    {kIconPackIconoir, L"iconoir-7.12.1",
     L"https://cdn.jsdelivr.net/npm/iconoir@7.12.1/icons/regular/", L".svg",
     &IconInfo::iconoir, L"Iconoir", L"MIT"},
    {kIconPackHeroicons, L"heroicons-2.2.0",
     L"https://cdn.jsdelivr.net/npm/heroicons@2.2.0/24/outline/", L".svg",
     &IconInfo::heroicon, L"Tailwind Labs", L"MIT"},
};

const OnlineIconPack* FindOnlineIconPack(int pack) {
    for (const auto& online : kOnlineIconPacks) {
        if (online.pack == pack) {
            return &online;
        }
    }
    return nullptr;
}

// The icon pack's folder (empty: none chosen yet), and the pictures and
// colors chosen for single icons (as stored, "E701|path" and "E701|#RRGGBB"
// lines, and read), per profile.
std::wstring g_iconFolder;
struct IconFiles {
    std::wstring text;
    std::vector<std::pair<WCHAR, std::wstring>> files;
    // 0xRRGGBB.
    std::vector<std::pair<WCHAR, int>> colors;
};
IconFiles g_iconFiles;
constexpr size_t kIconFilesRoom = 16 * 1024;

// The stored list read: an icon's code (one of kIcons) and a path on this
// computer's drives or a color each line; anything else, a second picture
// or color for an icon, or more than there are icons, left out.
IconFiles DecodeIconFiles(const std::wstring& text) {
    IconFiles read{text};
    size_t start = 0;
    while (start < text.size() &&
           read.files.size() + read.colors.size() < 2 * std::size(kIcons)) {
        size_t end = text.find(L'\n', start);
        if (end == std::wstring::npos) {
            end = text.size();
        }
        const std::wstring_view line(text.data() + start, end - start);
        start = end + 1;
        if (line.size() < 6 || line[4] != L'|' ||
            !std::all_of(line.begin(), line.begin() + 4,
                         [](WCHAR c) { return iswxdigit(c) != 0; })) {
            continue;
        }
        const WCHAR glyph = (WCHAR)wcstoul(
            std::wstring(line.substr(0, 4)).c_str(), nullptr, 16);
        std::wstring path(line.substr(5));
        if (path.size() == 7 && path[0] == L'#' &&
            std::all_of(path.begin() + 1, path.end(),
                        [](WCHAR c) { return iswxdigit(c) != 0; })) {
            if (FindIcon(glyph) &&
                std::none_of(read.colors.begin(), read.colors.end(),
                             [&](const auto& color) {
                                 return color.first == glyph;
                             })) {
                read.colors.push_back(
                    {glyph, (int)wcstoul(path.c_str() + 1, nullptr, 16)});
            }
            continue;
        }
        if (FindIcon(glyph) && IsLocalDrivePath(path) &&
            std::none_of(read.files.begin(), read.files.end(),
                         [&](const auto& file) {
                             return file.first == glyph;
                         })) {
            read.files.push_back({glyph, std::move(path)});
        }
    }
    return read;
}

std::wstring EncodeIconFiles(
    const std::vector<std::pair<WCHAR, std::wstring>>& files,
    const std::vector<std::pair<WCHAR, int>>& colors) {
    std::wstring text;
    for (const auto& [glyph, path] : files) {
        WCHAR code[8];
        swprintf_s(code, L"%04X|", glyph);
        text += code + path + L"\n";
    }
    for (const auto& [glyph, color] : colors) {
        WCHAR line[16];
        swprintf_s(line, L"%04X|#%06X\n", glyph, color & 0xFFFFFF);
        text += line;
    }
    return text;
}

// The color chosen for an icon (0xRRGGBB; -1: none, the island's). On the
// islands' thread only (the dock's icons in explorer stay as they are).
int IconColorOf(WCHAR glyph) {
    if (g_iconFiles.colors.empty() ||
        GetCurrentThreadId() != g_islandThreadId) {
        return -1;
    }
    for (const auto& [own, color] : g_iconFiles.colors) {
        if (own == glyph) {
            return color;
        }
    }
    return -1;
}

// The icon a picture's file name stands for in the user's folder: an icon's
// name (any case) or its code (0: none).
WCHAR IconGlyphFromName(std::wstring_view stem) {
    if (stem.size() == 4 &&
        std::all_of(stem.begin(), stem.end(),
                    [](WCHAR c) { return iswxdigit(c) != 0; })) {
        const WCHAR glyph =
            (WCHAR)wcstoul(std::wstring(stem).c_str(), nullptr, 16);
        return FindIcon(glyph) ? glyph : 0;
    }
    for (const auto& icon : kIcons) {
        if (stem.size() == wcslen(icon.name) &&
            _wcsnicmp(stem.data(), icon.name, stem.size()) == 0) {
            return icon.glyph;
        }
    }
    return 0;
}

// The pictures of a pack, read on the icons' own thread (see
// IconThreadProc): a folder's (the user's, read again when it changes; an
// online pack's, downloaded into the mod's storage first), or the pictures
// chosen for single icons. The islands' thread draws them (see
// DrawIconPicture), and only asks: no file is read while drawing.
enum class IconSourceKind { Folder, Online, Files };
struct IconPicture {
    WCHAR glyph;
    // Premultiplied BGRA, kIconPictureMost square at most (an SVG drawn
    // that big).
    UINT width = 0;
    UINT height = 0;
    std::vector<BYTE> pixels;
};
enum class IconSourceState { Reading, Downloading, Ready, Failed, NotLocal };
struct IconSource {
    IconSourceKind kind;
    // The folder, the online pack's folder name, or the files' list.
    std::wstring what;
    IconSourceState state = IconSourceState::Reading;
    // Told apart from the pictures before (see IconBitmap).
    UINT serial = 0;
    std::vector<IconPicture> pictures;
    // Downloading: how many are there of how many.
    int done = 0;
    int total = 0;
    // To be read (again): asked, its folder changed, or tried again.
    bool again = true;
};
constexpr size_t kIconSourcesMost = 8;
constexpr size_t kIconEntriesMost = 4096;
constexpr ULONGLONG kIconFileMost = 4 * 1024 * 1024;
constexpr size_t kIconSvgMost = 64 * 1024;
constexpr UINT kIconPictureMost = 128;
// (A folder changing: read once the changes stop for this long.)
constexpr DWORD kIconChangeSettleMs = 400;
// (At most this many waits, about a minute, for a folder written all the
// time: saving Windows' icons there takes several seconds.)
constexpr int kIconChangeSettleTimes = 150;
// Told to the islands' thread: pictures read (see OnIconsRead).
constexpr UINT WM_APP_ICONS = WM_APP + 100;
// Every island drawn again with them (defined further down).
void OnIconsRead();
// Pictures read, told to the main island's window: a message to its thread
// is lost while a modal loop runs it (a menu, a move, a drag); to the
// thread before it has a window.
void TellIconsRead() {
    if (const HWND island = g_islandWnd;
        island && PostMessage(island, WM_APP_ICONS, 0, 0)) {
        return;
    }
    PostThreadMessage(g_islandThreadId, WM_APP_ICONS, 0, 0);
}

SRWLOCK g_iconLock = SRWLOCK_INIT;
std::vector<IconSource> g_iconSources;
// Counts every new picture read, and every change of what the icons look
// like (see IconsGeneration): pictures drawn of them (the control center's
// tiles) are drawn again.
std::atomic<UINT> g_iconSerial{0};
// Counts every change of the online packs' files (downloaded, deleted): the
// settings look at which are there again (see IconPackOnDisk).
std::atomic<UINT> g_iconDiskChanges{0};
UINT g_iconLook = 0;
// The icons' thread (started by the islands' thread with a first pack, and
// ended with it, see ReleaseIconPictures): what stops it, a request under
// way closed at once, and what tells it to look at the sources.
HANDLE g_iconThread = nullptr;
HANDLE g_iconStopEvent = nullptr;
HANDLE g_iconKick = nullptr;
std::atomic<bool> g_iconStop{false};
HttpsCancel g_iconCancel;

UINT IconsGeneration() {
    return g_iconLook + g_iconSerial;
}

// An SVG picture's text (of the size kept), not a web page or anything
// else.
bool LooksLikeSvg(const std::string& text) {
    const size_t svg = text.find("<svg");
    return text.size() <= kIconSvgMost && svg != std::string::npos &&
           svg < 1024 && text.find("</svg>", svg) != std::string::npos;
}

// An SVG's own width and height taken out of its text (Lucide's say 24,
// which would draw it that big, cut or small): it then fills the size it's
// drawn at, its viewBox kept (one made of them when it has none). In the
// text, as Direct2D's SVG elements' attributes can't be set safely with
// this compiler's headers (their order differs from Windows').
void FillSvgSize(std::string* svg) {
    const size_t start = svg->find("<svg");
    const size_t end = start == std::string::npos ? start
                                                  : svg->find('>', start);
    if (end == std::string::npos) {
        return;
    }
    std::string tag = svg->substr(start, end - start);
    double sizes[2] = {0, 0};
    const char* names[2] = {"width=", "height="};
    for (int i = 0; i < 2; i++) {
        for (size_t at = tag.find(names[i]); at != std::string::npos;
             at = tag.find(names[i], at + 1)) {
            const size_t quote = at + strlen(names[i]);
            // (Not stroke-width or another name ending the same.)
            if (at == 0 || !isspace((unsigned char)tag[at - 1]) ||
                quote >= tag.size() ||
                (tag[quote] != '"' && tag[quote] != '\'')) {
                continue;
            }
            const size_t closing = tag.find(tag[quote], quote + 1);
            if (closing == std::string::npos) {
                return;
            }
            const std::string value =
                tag.substr(quote + 1, closing - quote - 1);
            char* rest = nullptr;
            const double number = strtod(value.c_str(), &rest);
            if (rest && (*rest == 0 || strcmp(rest, "px") == 0)) {
                sizes[i] = number;
            }
            tag.erase(at - 1, closing + 1 - (at - 1));
            break;
        }
    }
    if (tag.find("viewBox") == std::string::npos && sizes[0] > 0 &&
        sizes[1] > 0) {
        tag.insert(4, " viewBox=\"0 0 " + std::to_string(sizes[0]) + " " +
                          std::to_string(sizes[1]) + "\"");
    }
    svg->replace(start, end - start, tag);
}

// Whether a picture's pixels show anything (one all see-through would leave
// its icon empty: the font's is drawn instead).
bool IconPictureShows(const IconPicture& picture) {
    for (size_t i = 3; i < picture.pixels.size(); i += 4) {
        if (picture.pixels[i]) {
            return true;
        }
    }
    return false;
}

// An SVG's text drawn kIconPictureMost square (see FillSvgSize), here on
// the icons' thread rather than as it's drawn.
bool DrawSvgPicture(IWICImagingFactory* wic,
                    ID2D1Factory* d2d,
                    std::string svg,
                    IconPicture* picture) {
    FillSvgSize(&svg);
    constexpr UINT kPixels = kIconPictureMost;
    winrt::com_ptr<IWICBitmap> bitmap;
    winrt::com_ptr<ID2D1RenderTarget> target;
    winrt::com_ptr<ID2D1DeviceContext5> context;
    winrt::com_ptr<IStream> stream;
    winrt::com_ptr<ID2D1SvgDocument> document;
    stream.attach(
        SHCreateMemStream((const BYTE*)svg.data(), (UINT)svg.size()));
    if (!stream || !d2d ||
        FAILED(wic->CreateBitmap(kPixels, kPixels,
                                 GUID_WICPixelFormat32bppPBGRA,
                                 WICBitmapCacheOnLoad, bitmap.put())) ||
        FAILED(d2d->CreateWicBitmapRenderTarget(
            bitmap.get(),
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                  D2D1_ALPHA_MODE_PREMULTIPLIED),
                96, 96),
            target.put())) ||
        FAILED(target->QueryInterface(IID_PPV_ARGS(context.put()))) ||
        FAILED(context->CreateSvgDocument(
            stream.get(), D2D1::SizeF((float)kPixels, (float)kPixels),
            document.put()))) {
        return false;
    }
    target->BeginDraw();
    target->Clear(D2D1::ColorF(0, 0, 0, 0));
    context->DrawSvgDocument(document.get());
    if (FAILED(target->EndDraw())) {
        return false;
    }
    picture->width = kPixels;
    picture->height = kPixels;
    picture->pixels.resize((size_t)kPixels * kPixels * 4);
    return SUCCEEDED(bitmap->CopyPixels(nullptr, kPixels * 4,
                                        (UINT)picture->pixels.size(),
                                        picture->pixels.data()));
}

// A picture read: an SVG drawn, or the biggest of a picture's frames (an ICO
// file has several sizes), made kIconPictureMost square at most; false
// without one, or with one that shows nothing.
bool ReadIconPicture(IWICImagingFactory* wic,
                     ID2D1Factory* d2d,
                     const std::wstring& path,
                     IconPicture* picture) {
    if (_wcsicmp(PathFindExtension(path.c_str()), L".svg") == 0) {
        std::string svg;
        const HANDLE file = CreateFile(
            path.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, 0, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            return false;
        }
        LARGE_INTEGER size{};
        DWORD read = 0;
        bool done = GetFileSizeEx(file, &size) && size.QuadPart > 0 &&
                    (ULONGLONG)size.QuadPart <= kIconSvgMost;
        if (done) {
            svg.resize((size_t)size.QuadPart);
            done = ReadFile(file, svg.data(), (DWORD)svg.size(), &read,
                            nullptr) &&
                   read == svg.size();
        }
        CloseHandle(file);
        return done && LooksLikeSvg(svg) &&
               DrawSvgPicture(wic, d2d, std::move(svg), picture) &&
               IconPictureShows(*picture);
    }
    winrt::com_ptr<IWICBitmapDecoder> decoder;
    UINT frames = 0;
    if (FAILED(wic->CreateDecoderFromFilename(
            path.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, decoder.put())) ||
        FAILED(decoder->GetFrameCount(&frames))) {
        return false;
    }
    winrt::com_ptr<IWICBitmapFrameDecode> biggest;
    UINT width = 0;
    UINT height = 0;
    for (UINT i = 0; i < frames && i < 16; i++) {
        winrt::com_ptr<IWICBitmapFrameDecode> frame;
        UINT frameWidth = 0;
        UINT frameHeight = 0;
        if (SUCCEEDED(decoder->GetFrame(i, frame.put())) &&
            SUCCEEDED(frame->GetSize(&frameWidth, &frameHeight)) &&
            frameWidth > width && frameHeight > 0) {
            biggest = frame;
            width = frameWidth;
            height = frameHeight;
        }
    }
    winrt::com_ptr<IWICFormatConverter> converter;
    if (!biggest || FAILED(wic->CreateFormatConverter(converter.put())) ||
        FAILED(converter->Initialize(
            biggest.get(), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0,
            WICBitmapPaletteTypeMedianCut))) {
        return false;
    }
    winrt::com_ptr<IWICBitmapSource> source;
    source.copy_from(converter.get());
    const float fit = std::min(
        1.0f, (float)kIconPictureMost / (float)std::max(width, height));
    if (fit < 1) {
        winrt::com_ptr<IWICBitmapScaler> scaler;
        const UINT fitWidth = std::max(1u, (UINT)std::lround(width * fit));
        const UINT fitHeight = std::max(1u, (UINT)std::lround(height * fit));
        if (FAILED(wic->CreateBitmapScaler(scaler.put())) ||
            FAILED(scaler->Initialize(converter.get(), fitWidth, fitHeight,
                                      WICBitmapInterpolationModeFant))) {
            return false;
        }
        source.copy_from(scaler.get());
        width = fitWidth;
        height = fitHeight;
    }
    picture->width = width;
    picture->height = height;
    picture->pixels.resize((size_t)width * height * 4);
    return SUCCEEDED(source->CopyPixels(nullptr, width * 4,
                                        (UINT)picture->pixels.size(),
                                        picture->pixels.data())) &&
           IconPictureShows(*picture);
}

// A folder's pictures for icons (its first entries only; one per icon):
// named after an icon or by its code (the user's), or by the online pack's
// names (each of its pictures may stand for several icons).
std::vector<IconPicture> ReadIconFolder(IWICImagingFactory* wic,
                                        ID2D1Factory* d2d,
                                        const std::wstring& folder,
                                        const OnlineIconPack* online) {
    std::vector<IconPicture> pictures;
    WIN32_FIND_DATA data;
    const HANDLE find = FindFirstFile((folder + L"\\*").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        return pictures;
    }
    size_t entries = 0;
    do {
        const PCWSTR dot = wcsrchr(data.cFileName, L'.');
        // (Not an online pack's mark of a picture it doesn't have.)
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !dot ||
            _wcsicmp(dot, L".none") == 0 ||
            (((ULONGLONG)data.nFileSizeHigh << 32) | data.nFileSizeLow) >
                kIconFileMost) {
            continue;
        }
        const std::wstring_view stem(data.cFileName,
                                     (size_t)(dot - data.cFileName));
        std::vector<WCHAR> glyphs;
        if (online) {
            for (const auto& icon : kIcons) {
                if (icon.*online->name && stem == icon.*online->name) {
                    glyphs.push_back(icon.glyph);
                }
            }
        } else if (const WCHAR glyph = IconGlyphFromName(stem)) {
            glyphs.push_back(glyph);
        }
        std::erase_if(glyphs, [&](WCHAR glyph) {
            return std::any_of(pictures.begin(), pictures.end(),
                               [&](const IconPicture& picture) {
                                   return picture.glyph == glyph;
                               });
        });
        IconPicture picture{};
        if (glyphs.empty() ||
            !ReadIconPicture(wic, d2d, folder + L"\\" + data.cFileName,
                             &picture)) {
            continue;
        }
        for (WCHAR glyph : glyphs) {
            picture.glyph = glyph;
            pictures.push_back(picture);
        }
    } while (!g_iconStop && ++entries < kIconEntriesMost &&
             FindNextFile(find, &data));
    FindClose(find);
    return pictures;
}

// The online pack's pictures not downloaded yet (its picture for each
// icon, once each), into its folder, the source's count told as it goes;
// false if one couldn't be (the ones before are kept: the next time, only
// the rest). One the pack doesn't have (the server says 404) is skipped,
// and remembered (an empty <name>.none beside them) so it's never asked for
// again: that icon keeps the island's own. Only the pictures' addresses are
// asked for: nothing about the user is sent.
bool DownloadIconPack(const OnlineIconPack& pack, const std::wstring& folder) {
    std::vector<std::wstring> names;
    for (const auto& icon : kIcons) {
        if (!(icon.*pack.name)) {
            continue;
        }
        const std::wstring name = icon.*pack.name;
        auto missing = [&](PCWSTR ending) {
            const std::wstring path = folder + L"\\" + name + ending;
            return GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES;
        };
        if (std::find(names.begin(), names.end(), name) == names.end() &&
            missing(L".svg") && missing(L".none")) {
            names.push_back(name);
        }
    }
    auto tell = [&](int done, IconSourceState state) {
        AcquireSRWLockExclusive(&g_iconLock);
        for (auto& source : g_iconSources) {
            if (source.kind == IconSourceKind::Online &&
                source.what == pack.folder) {
                source.state = state;
                source.done = done;
                source.total = (int)names.size();
            }
        }
        ReleaseSRWLockExclusive(&g_iconLock);
        g_iconDiskChanges++;
        TellIconsRead();
    };
    if (names.empty()) {
        return true;
    }
    tell(0, IconSourceState::Downloading);
    for (size_t i = 0; i < names.size(); i++) {
        std::vector<BYTE> data;
        DWORD answer = 0;
        if (!HttpsGet(pack.url + names[i] + pack.ending, &data, kIconSvgMost,
                      5000, &g_iconStop, &g_iconCancel, &answer)) {
            if (answer != 404 || g_iconStop) {
                return false;
            }
            IslandLog(L"Icon pack %s: no %s", pack.folder, names[i].c_str());
            const HANDLE none = CreateFile(
                (folder + L"\\" + names[i] + L".none").c_str(), GENERIC_WRITE,
                0, nullptr, CREATE_ALWAYS, 0, nullptr);
            if (none != INVALID_HANDLE_VALUE) {
                CloseHandle(none);
            }
            continue;
        }
        if (!LooksLikeSvg(std::string(data.begin(), data.end()))) {
            return false;
        }
        // (Whole, or not there: written beside it, then put in its place.)
        const std::wstring path = folder + L"\\" + names[i] + L".svg";
        const std::wstring part = path + L".part";
        const HANDLE file = CreateFile(part.c_str(), GENERIC_WRITE, 0,
                                       nullptr, CREATE_ALWAYS, 0, nullptr);
        DWORD written = 0;
        const bool done =
            file != INVALID_HANDLE_VALUE &&
            WriteFile(file, data.data(), (DWORD)data.size(), &written,
                      nullptr) &&
            written == data.size();
        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
        }
        if (!done ||
            !MoveFileEx(part.c_str(), path.c_str(),
                        MOVEFILE_REPLACE_EXISTING)) {
            DeleteFile(part.c_str());
            return false;
        }
        // (The settings say how far it is, now and then.)
        if (i % 8 == 7) {
            tell((int)i + 1, IconSourceState::Downloading);
        }
    }
    return true;
}

// The folder in the mod's storage an online pack is kept in (empty if
// there's no storage).
std::wstring OnlineIconPackPath(const OnlineIconPack& pack) {
    WCHAR storage[MAX_PATH];
    if (!Wh_GetModStoragePath(storage, ARRAYSIZE(storage))) {
        return {};
    }
    return std::wstring(storage) + L"\\icon-packs\\" + pack.folder;
}

// That folder, made if it isn't there (empty if it can't be).
std::wstring OnlineIconPackFolder(const OnlineIconPack& pack) {
    const std::wstring folder = OnlineIconPackPath(pack);
    if (folder.empty()) {
        return {};
    }
    const std::wstring packs = folder.substr(0, folder.rfind(L'\\'));
    CreateDirectory(packs.substr(0, packs.rfind(L'\\')).c_str(), nullptr);
    CreateDirectory(packs.c_str(), nullptr);
    CreateDirectory(folder.c_str(), nullptr);
    const DWORD attributes = GetFileAttributes(folder.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
                   (attributes & FILE_ATTRIBUTE_DIRECTORY)
               ? folder
               : std::wstring();
}

// Whether an online pack has pictures in the mod's storage (downloaded, or
// partly): a look at its folder, not done while drawing.
bool IconPackOnDisk(int pack) {
    const OnlineIconPack* online = FindOnlineIconPack(pack);
    const std::wstring folder =
        online ? OnlineIconPackPath(*online) : std::wstring();
    if (folder.empty()) {
        return false;
    }
    WIN32_FIND_DATA data;
    const HANDLE find = FindFirstFile((folder + L"\\*.svg").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        return false;
    }
    FindClose(find);
    return true;
}

// The online packs' folders to delete, taken by the icons' thread (with
// g_iconLock).
std::vector<std::wstring> g_iconDeletes;

// A pack's folder deleted: its pictures (and any half-written one), then
// it.
void DeleteIconPackFolder(const std::wstring& folder) {
    WIN32_FIND_DATA data;
    const HANDLE find = FindFirstFile((folder + L"\\*").c_str(), &data);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                DeleteFile((folder + L"\\" + data.cFileName).c_str());
            }
        } while (FindNextFile(find, &data));
        FindClose(find);
    }
    RemoveDirectory(folder.c_str());
}

// Reads the sources the islands ask for (see RequestIconSource), each one
// once, and the user's folders again when they change (watched), until the
// islands' thread ends (see ReleaseIconPictures).
DWORD WINAPI IconThreadProc(LPVOID) {
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    winrt::com_ptr<IWICImagingFactory> wic;
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARGS(wic.put()));
    // (The SVGs drawn with it, see DrawSvgPicture.)
    winrt::com_ptr<ID2D1Factory> d2d;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d.put());
    // The user's folders watched (only the ones asked for).
    std::vector<std::pair<std::wstring, HANDLE>> watches;
    while (!g_iconStop) {
        struct Job {
            IconSourceKind kind;
            std::wstring what;
        };
        std::vector<Job> jobs;
        std::vector<std::wstring> watched;
        AcquireSRWLockExclusive(&g_iconLock);
        // (A pack deleted: its pictures gone, downloaded again if chosen.)
        std::vector<std::wstring> deletes;
        deletes.swap(g_iconDeletes);
        std::erase_if(g_iconSources, [&](const IconSource& source) {
            return source.kind == IconSourceKind::Online &&
                   std::any_of(deletes.begin(), deletes.end(),
                               [&](const std::wstring& folder) {
                                   return folder.ends_with(L"\\" +
                                                           source.what);
                               });
        });
        for (auto& source : g_iconSources) {
            if (source.kind == IconSourceKind::Folder &&
                IsLocalDrivePath(source.what)) {
                watched.push_back(source.what);
            }
            if (source.again) {
                source.again = false;
                jobs.push_back({source.kind, source.what});
            }
        }
        ReleaseSRWLockExclusive(&g_iconLock);
        for (const auto& folder : deletes) {
            DeleteIconPackFolder(folder);
        }
        if (!deletes.empty()) {
            g_iconDiskChanges++;
            TellIconsRead();
        }
        std::erase_if(watches, [&](const auto& watch) {
            if (std::find(watched.begin(), watched.end(), watch.first) !=
                watched.end()) {
                return false;
            }
            FindCloseChangeNotification(watch.second);
            return true;
        });
        for (const auto& folder : watched) {
            if (std::none_of(watches.begin(), watches.end(),
                             [&](const auto& watch) {
                                 return watch.first == folder;
                             })) {
                const HANDLE change = FindFirstChangeNotification(
                    folder.c_str(), FALSE,
                    FILE_NOTIFY_CHANGE_FILE_NAME |
                        FILE_NOTIFY_CHANGE_LAST_WRITE |
                        FILE_NOTIFY_CHANGE_SIZE);
                if (change != INVALID_HANDLE_VALUE) {
                    watches.push_back({folder, change});
                }
            }
        }
        for (const Job& job : jobs) {
            if (g_iconStop) {
                break;
            }
            IconSourceState state = IconSourceState::Ready;
            std::vector<IconPicture> pictures;
            if (!wic) {
                state = IconSourceState::Failed;
            } else if (job.kind == IconSourceKind::Folder) {
                if (IsLocalDrivePath(job.what)) {
                    pictures = ReadIconFolder(wic.get(), d2d.get(), job.what,
                                              nullptr);
                } else {
                    state = IconSourceState::NotLocal;
                }
            } else if (job.kind == IconSourceKind::Online) {
                const OnlineIconPack* pack = nullptr;
                for (const auto& online : kOnlineIconPacks) {
                    if (job.what == online.folder) {
                        pack = &online;
                    }
                }
                const std::wstring folder =
                    pack ? OnlineIconPackFolder(*pack) : std::wstring();
                if (folder.empty() || !DownloadIconPack(*pack, folder)) {
                    state = IconSourceState::Failed;
                }
                g_iconDiskChanges++;
                if (!folder.empty()) {
                    pictures =
                        ReadIconFolder(wic.get(), d2d.get(), folder, pack);
                }
            } else {
                for (const auto& [glyph, path] :
                     DecodeIconFiles(job.what).files) {
                    IconPicture picture{glyph};
                    if (g_iconStop) {
                        break;
                    }
                    if (ReadIconPicture(wic.get(), d2d.get(), path,
                                        &picture)) {
                        pictures.push_back(std::move(picture));
                    }
                }
            }
            if (g_iconStop) {
                break;
            }
            AcquireSRWLockExclusive(&g_iconLock);
            for (auto& source : g_iconSources) {
                if (source.kind == job.kind && source.what == job.what) {
                    source.state = state;
                    source.pictures = std::move(pictures);
                    source.serial = ++g_iconSerial;
                    break;
                }
            }
            ReleaseSRWLockExclusive(&g_iconLock);
            TellIconsRead();
        }
        // Then a request, a change in a watched folder, or the end.
        std::vector<HANDLE> handles{g_iconStopEvent, g_iconKick};
        for (const auto& watch : watches) {
            handles.push_back(watch.second);
        }
        const DWORD woken = WaitForMultipleObjects(
            (DWORD)handles.size(), handles.data(), FALSE, INFINITE);
        if (woken == WAIT_OBJECT_0 || woken == WAIT_FAILED) {
            break;
        }
        if (woken >= WAIT_OBJECT_0 + 2 &&
            woken < WAIT_OBJECT_0 + handles.size()) {
            // (Read once the files are written: the changes settled. A
            // watch that can't go on, its folder gone, is dropped: made
            // again above once the folder is back.)
            const size_t index = woken - WAIT_OBJECT_0 - 2;
            const std::wstring folder = watches[index].first;
            const HANDLE change = watches[index].second;
            bool watching = FindNextChangeNotification(change);
            for (int times = 0;
                 watching && times < kIconChangeSettleTimes &&
                 WaitForSingleObject(g_iconStopEvent, 0) == WAIT_TIMEOUT &&
                 WaitForSingleObject(change, kIconChangeSettleMs) ==
                     WAIT_OBJECT_0;
                 times++) {
                watching = FindNextChangeNotification(change);
            }
            if (!watching) {
                FindCloseChangeNotification(change);
                watches.erase(watches.begin() + index);
            }
            AcquireSRWLockExclusive(&g_iconLock);
            for (auto& source : g_iconSources) {
                if (source.kind == IconSourceKind::Folder &&
                    source.what == folder) {
                    source.again = true;
                }
            }
            ReleaseSRWLockExclusive(&g_iconLock);
        }
    }
    for (const auto& watch : watches) {
        FindCloseChangeNotification(watch.second);
    }
    wic = nullptr;
    d2d = nullptr;
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    return 0;
}

void KickIconThread();

// A source the islands draw from, asked for (read on the icons' thread,
// started now if it isn't running); `again`: read it again (an online pack
// that couldn't be downloaded, tried again). The islands' thread only.
void RequestIconSource(IconSourceKind kind,
                       std::wstring_view what,
                       bool again = false) {
    AcquireSRWLockExclusive(&g_iconLock);
    auto it = std::find_if(g_iconSources.begin(), g_iconSources.end(),
                           [&](const IconSource& source) {
                               return source.kind == kind &&
                                      source.what == what;
                           });
    const bool asked = it == g_iconSources.end() || again;
    if (it == g_iconSources.end()) {
        if (g_iconSources.size() >= kIconSourcesMost) {
            g_iconSources.erase(g_iconSources.begin());
        }
        g_iconSources.push_back({kind, std::wstring(what)});
    } else if (again) {
        it->again = true;
        it->state = IconSourceState::Reading;
    }
    ReleaseSRWLockExclusive(&g_iconLock);
    if (asked) {
        KickIconThread();
    }
}

// The icons' thread told to look at what's asked (started first if it
// isn't running).
void KickIconThread() {
    if (!g_iconThread) {
        g_iconStop = false;
        AcquireSRWLockExclusive(&g_iconCancel.lock);
        g_iconCancel.canceled = false;
        ReleaseSRWLockExclusive(&g_iconCancel.lock);
        if (!g_iconStopEvent) {
            g_iconStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        }
        if (!g_iconKick) {
            g_iconKick = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        }
        if (g_iconStopEvent && g_iconKick) {
            g_iconThread =
                CreateThread(nullptr, 0, IconThreadProc, nullptr, 0, nullptr);
        }
        if (!g_iconThread) {
            IslandLog(L"The icons' thread couldn't start: %u",
                      GetLastError());
            return;
        }
    }
    SetEvent(g_iconKick);
}

// An online pack's pictures deleted from the mod's storage (on the icons'
// thread; chosen again, it's downloaded again).
void DeleteIconPack(int pack) {
    const OnlineIconPack* online = FindOnlineIconPack(pack);
    const std::wstring folder =
        online ? OnlineIconPackPath(*online) : std::wstring();
    if (folder.empty()) {
        return;
    }
    AcquireSRWLockExclusive(&g_iconLock);
    g_iconDeletes.push_back(folder);
    ReleaseSRWLockExclusive(&g_iconLock);
    KickIconThread();
}

// An online pack's download under way: how far (0 to 1), or -1 when it
// isn't downloading.
float IconPackProgress(int pack) {
    const OnlineIconPack* online = FindOnlineIconPack(pack);
    if (!online) {
        return -1;
    }
    float progress = -1;
    AcquireSRWLockShared(&g_iconLock);
    for (const auto& source : g_iconSources) {
        if (source.kind == IconSourceKind::Online &&
            source.what == online->folder &&
            source.state == IconSourceState::Downloading) {
            progress = source.total > 0
                           ? std::clamp((float)source.done / source.total,
                                        0.0f, 1.0f)
                           : 0.0f;
        }
    }
    ReleaseSRWLockShared(&g_iconLock);
    return progress;
}

// Whether any online pack is downloading (the settings' list moving).
bool IconPacksDownloading() {
    return std::any_of(std::begin(kOnlineIconPacks), std::end(kOnlineIconPacks),
                       [](const OnlineIconPack& online) {
                           return IconPackProgress(online.pack) >= 0;
                       });
}

// The source the current island's pack draws from (none: the font's, or
// the emoji's): its kind and what it is.
bool CurrentIconSource(IconSourceKind* kind, std::wstring* what) {
    if (g_settings.islandIconPack == kIconPackFolder &&
        !g_iconFolder.empty()) {
        *kind = IconSourceKind::Folder;
        *what = g_iconFolder;
        return true;
    }
    if (const OnlineIconPack* online =
            FindOnlineIconPack(g_settings.islandIconPack)) {
        *kind = IconSourceKind::Online;
        *what = online->folder;
        return true;
    }
    return false;
}

// What the icon pack's row says about it (empty: nothing): an online one
// downloading (how far) or not downloaded, a folder not on this computer or
// without any icon's picture.
std::wstring IconPackStatus() {
    IconSourceKind kind;
    std::wstring what;
    if (!CurrentIconSource(&kind, &what)) {
        return {};
    }
    std::wstring status;
    AcquireSRWLockShared(&g_iconLock);
    for (const auto& source : g_iconSources) {
        if (source.kind != kind || source.what != what) {
            continue;
        }
        if (source.state == IconSourceState::Downloading) {
            status = std::wstring(Tr(L"Downloading the icons",
                                     L"Baixando os ícones")) +
                     L" · " + std::to_wstring(source.done) + L"/" +
                     std::to_wstring(source.total);
        } else if (source.state == IconSourceState::Failed) {
            status = Tr(L"Couldn't download them: click it again to try",
                        L"Não deu para baixar: clique de novo para tentar");
        } else if (source.state == IconSourceState::NotLocal) {
            status = Tr(L"Only a folder on this computer",
                        L"Só uma pasta deste computador");
        } else if (source.state == IconSourceState::Ready &&
                   source.pictures.empty()) {
            status = Tr(L"No icon's picture in that folder",
                        L"Nenhuma imagem de ícone nessa pasta");
        }
    }
    ReleaseSRWLockShared(&g_iconLock);
    return status;
}

// Drawn by the islands' thread: for each drawing target (one only kept
// here is let go), the bitmaps made of the sources' pictures (by size
// steps: an icon growing in an animation doesn't make one each frame); the
// fonts' formats (Windows 10's icons, the emojis), and which icons Windows
// 10's font has.
struct IconTargetBitmap {
    UINT serial;
    WCHAR glyph;
    int pixels;
    winrt::com_ptr<ID2D1Bitmap> bitmap;
};
struct IconTargetBitmaps {
    winrt::com_ptr<ID2D1RenderTarget> target;
    UINT64 used = 0;
    std::vector<IconTargetBitmap> bitmaps;
};
std::vector<IconTargetBitmaps> g_iconTargets;
UINT64 g_iconUses = 0;
winrt::com_ptr<IWICImagingFactory> g_iconWic;
winrt::com_ptr<IDWriteFactory> g_iconDwrite;
struct IconFontFormat {
    PCWSTR family;
    float size;
    DWRITE_TEXT_ALIGNMENT across;
    DWRITE_PARAGRAPH_ALIGNMENT down;
    winrt::com_ptr<IDWriteTextFormat> format;
};
std::vector<IconFontFormat> g_iconFormats;
std::vector<std::pair<WCHAR, bool>> g_windows10Glyphs;
constexpr size_t kIconTargetsMost = 32;
constexpr size_t kIconBitmapsMost = 256;
constexpr size_t kIconFormatsMost = 64;
constexpr int kIconPixelSizes[] = {16, 20, 24, 32, 40, 48, 64, 96, 128};
constexpr WCHAR kWindows10IconFont[] = L"Segoe MDL2 Assets";
constexpr WCHAR kEmojiFont[] = L"Segoe UI Emoji";
// The emojis fill their whole box, the icons not quite.
constexpr float kEmojiScale = 0.86f;

