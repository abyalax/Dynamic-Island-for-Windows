#pragma once
// The island's own options, chosen in it (the control center's settings,
// from the bubble of a right click on the island) rather than in Windhawk, and kept with Wh_SetIntValue.
enum class IslandPref {
    Bar,
    Theme,
    Tray,
    TrayOpen,
    Network,
    Volume,
    Battery,
    Bell,
    Media,
    MediaButtons,
    MediaTitle,
    Banner,
    BannerMinimized,
    HideWindowsBanners,
    WebPictures,
    ClockSeconds,
    ClockWeekday,
    ClockDate,
    ClockYear,
    FreeDrag,
    VerticalSides,
    ClockCenter,
    Power,
    OwnNotifications,
    PanelsFollow,
    ThemeButton,
    UsageCpu,
    UsageRam,
    UsageVram,
    Microphone,
    Holidays,
    Notes,
    Tasks,
    Shortcuts,
    Weather,
    Updates,
    Animations,
    FollowMouse,
    Profiles,
    AutoHide,
    AllMonitors,
    VolumePercent,
    BatteryPercent,
    Brightness,
    Clipboard,
    Frosted,
    DateOrder,
    MonthNumber,
    InputMethod,
    Privacy,
    UsbDrives,
    UsbNotify,
    ClockFormat,
    TrayStyle,
    WindowColor,
    Folders,
    Wallpaper,
    WallpaperTransition,
    Desktops,
    // The control center's buttons on the pill (see kPillActions).
    ActionNetwork,
    ActionBluetooth,
    ActionNightLight,
    ActionHotspot,
    ActionAirplane,
    ActionProject,
    ActionCast,
    ActionCaptions,
    ActionAccessibility,
    ActionScreenshot,
    ActionLock,
    ActionEmoji,
    ActionClipboard,
    ActionTaskManager,
    ActionCalculator,
    ActionExplorer,
    // Win+N and Win+A open the island's notifications and control center.
    ShortcutNotifications,
    ShortcutControl,
    // The Fixed style's notches (see islandNotches).
    Notches,
    NotchEach,
    GroupApps,
    // komorebi's button on the pill (see kPillActions) and what its buttons
    // take along.
    ActionKomorebi,
    KomorebiWhkd,
    KomorebiMasir,
    ScreenshotFolder,
    MediaWheel,
    MediaMarquee,
    // The icons of the apps with notifications on the pill.
    NotificationApps,
    // komorebi's layout on the pill, and the app in focus on the monitor.
    KomorebiLayout,
    FocusedApp,
    KomorebiHideEmpty,
    // Files dragged out of the folders panel: 0 asked, 1 copied, 2 moved
    // (chosen on its own row, see kChoiceFolderDrag).
    FolderDrag,
    LiquidGlass,
    // The names under the icons and buttons (see Island::ShowTrayTip).
    Tips,
    HoverHighlight,
    MinimizeButton,
    BarCorners,
    GlassTranslucent,
    Clock,
    ActionShowDesktop,
    MinimizeKey,
    NetSpeed,
    WorldClocks,
    Countdown,
    RecycleBin,
    ActionOnScreenKeyboard,
    StatusNotices,
    FoldApps,
    Timer,
    Visualizer,
    // What "Time until the next" counts down to, and the pill filling like
    // a loading bar (downloads, the battery charging; see Island::Fill).
    CountdownEvents,
    CountdownAlarms,
    CountdownTimer,
    CountdownPomodoro,
    DownloadFill,
    ChargeFill,
    // The fill's colors, chosen on rows of their own (see kChoiceFillColor).
    FillColor,
    FillOwnColor,
    FillCopyColor,
    FillCopyOwnColor,
    FillChargeColor,
    FillChargeOwnColor,
    // Chosen on its own row (see kChoiceFillStyle).
    FillStyle,
    // Song lyrics, their size and color (rows of their own, see
    // kChoiceLyricsSize), and the line in the title's place.
    Lyrics,
    LyricsSize,
    LyricsColor,
    LyricsOwnColor,
    LyricsInTitle,
    // The audio visualizer's color (see kChoiceVisualizerColor).
    VisualizerColor,
    VisualizerOwnColor,
    // From how much in it the Recycle Bin shows (kChoiceRecycleMinSize).
    RecycleMinSize,
    // The longest a song's line gets on the pill (kChoiceLyricsWidth).
    LyricsWidth,
    VisualizerSmooth,
    WhatsNewNotice,
    // 3.0 (new ones go here, at the end, like in kIslandPrefs).
    WeatherUnit,
    NotchOpen,
    IconPack,
    IconColors,
    CapsLock,
    HideWhen,
    LineFade,
    HideClock,
    PillNotches,
    BarClear,
    AiUsage,
    AiClaude,
    AiCodex,
    FocusedTitle,
    WeatherCards,
    CalendarBigDay,
    Commands,
    BarSide,
    BarInnerCorners,
    DndSilent,
    BannerSize,
    BannerWidth,
    BannerCorners,
    BannerPlace,
    DownloadStalled,
    FolderThumbnails,
    Shelf,
    // (Chosen on its own row, see kChoiceShelfLimit.)
    ShelfLimit,
    TimerLine,
    LineClick,
    ExternalBrightness,
    GlassLight,
    // The dragon's eyes, and their colors and shape (kChoiceEyeTheme,
    // kChoiceEyeShape).
    Eyes,
    EyeTheme,
    EyeShape,
};

struct IslandPrefInfo {
    IslandPref pref;
    PCWSTR key;
    WCHAR glyph;
    PCWSTR english;
    PCWSTR portuguese;
};

const IslandPrefInfo kIslandPrefs[] = {
    {IslandPref::Bar, L"islandBar", 0xE7C4, L"Style", L"Estilo"},
    {IslandPref::Theme, L"islandTheme", 0xE790, L"Colors", L"Cores"},
    {IslandPref::Tray, L"islandTray", 0xE712, L"Other apps' tray icons",
     L"Ícones dos outros apps"},
    {IslandPref::TrayOpen, L"trayKeepOpen", 0xE718, L"Keep their list open",
     L"Manter a lista aberta"},
    {IslandPref::Network, L"islandNetwork", kGlyphControlCenter,
     L"Control center", L"Central de controle"},
    {IslandPref::Volume, L"islandVolume", 0xE767, L"Volume", L"Volume"},
    {IslandPref::Battery, L"islandBattery", 0xE83F, L"Battery", L"Bateria"},
    {IslandPref::Bell, L"islandBell", 0xEA8F, L"Notifications (bell)",
     L"Notificações (sino)"},
    {IslandPref::Media, L"islandMedia", 0xE8D6, L"What's playing",
     L"O que está tocando"},
    {IslandPref::MediaButtons, L"mediaButtons", 0xE768,
     L"Media buttons on the pill", L"Botões de mídia na pílula"},
    {IslandPref::MediaTitle, L"mediaTitle", 0xE93C,
     L"Cover and title on the pill", L"Capa e título na pílula"},
    {IslandPref::Banner, L"bannerEnabled", 0xE8BD, L"Banners in the pill",
     L"Avisos na pílula"},
    {IslandPref::BannerMinimized, L"bannerMinimized", 0xE921,
     L"Banners even with the pill minimized",
     L"Avisos mesmo com a pílula minimizada"},
    {IslandPref::HideWindowsBanners, L"hideWindowsBanners", 0xED1A,
     L"Hide the Windows banners", L"Esconder os avisos do Windows"},
    {IslandPref::WebPictures, L"bannerWebPictures", 0xE774,
     L"Download pictures from the web (goes online)",
     L"Baixar fotos da internet (acessa a internet)"},
    {IslandPref::ClockSeconds, L"clockSeconds", 0xE916, L"Seconds",
     L"Segundos"},
    {IslandPref::ClockWeekday, L"clockWeekday", 0xE787, L"Day of the week",
     L"Dia da semana"},
    {IslandPref::ClockDate, L"clockDate", 0xE8BF, L"Day and month",
     L"Dia e mês"},
    {IslandPref::ClockYear, L"clockYear", 0xE787, L"Year", L"Ano"},
    {IslandPref::FreeDrag, L"islandFreeDrag", 0xE7C2,
     L"Move the pill by dragging it", L"Mover a pílula arrastando"},
    {IslandPref::VerticalSides, L"islandVerticalSides", 0xE8B4,
     L"Upright on the left and right edges", L"Em pé nas laterais"},
    {IslandPref::ClockCenter, L"islandClockCenter", 0xE823,
     L"Clock in the middle (the other items move around it)",
     L"Relógio no meio (os outros itens se reorganizam em volta)"},
    {IslandPref::Power, L"islandPower", 0xE7E8,
     L"Power button (sleep, restart, shut down)",
     L"Botão de desligar (suspender, reiniciar, desligar)"},
    {IslandPref::OwnNotifications, L"islandOwnsNotifications", 0xE767,
     L"The island handles notifications and their sounds (Windows' stay quiet)",
     L"A ilha cuida das notificações e dos sons (as do Windows ficam quietas)"},
    {IslandPref::PanelsFollow, L"islandPanelsFollow", 0xE740,
     L"Panels as wide as the island", L"Painéis na largura da ilha"},
    {IslandPref::ThemeButton, L"islandThemeButton", 0xE7A1,
     L"Light and dark mode button", L"Botão de modo claro e escuro"},
    {IslandPref::UsageCpu, L"usageCpu", 0xE950, L"Processor use (CPU)",
     L"Uso do processador (CPU)"},
    {IslandPref::UsageRam, L"usageRam", 0xE964, L"Memory use (RAM)",
     L"Uso da memória (RAM)"},
    {IslandPref::UsageVram, L"usageVram", 0xEEA1, L"Video memory in use (VRAM)",
     L"Memória de vídeo em uso (VRAM)"},
    {IslandPref::Microphone, L"islandMicrophone", 0xE720,
     L"Microphone button (mute)", L"Botão do microfone (silenciar)"},
    {IslandPref::Holidays, L"calendarHolidays", 0xE787,
     L"Holidays in the calendar (goes online)",
     L"Feriados no calendário (acessa a internet)"},
    {IslandPref::Notes, L"islandNotes", 0xE70B, L"Notes button",
     L"Botão de anotações"},
    {IslandPref::Tasks, L"islandTasks", 0xE9D5, L"Tasks button",
     L"Botão de tarefas"},
    {IslandPref::Shortcuts, L"islandShortcuts", 0xE765,
     L"Keyboard shortcuts button", L"Botão de atalhos do teclado"},
    {IslandPref::Weather, L"islandWeather", 0xE753, L"Weather (goes online)",
     L"Clima (acessa a internet)"},
    {IslandPref::Updates, L"islandUpdates", 0xE895,
     L"Updates (winget and Windows Update)",
     L"Atualizações (winget e Windows Update)"},
    // Kept with the other animation settings (see SetAnimationChoice).
    {IslandPref::Animations, nullptr, 0xE945, L"Animations", L"Animações"},
    {IslandPref::FollowMouse, L"islandFollowMouse", 0xE962,
     L"On the monitor with the mouse", L"No monitor onde está o mouse"},
    {IslandPref::Profiles, L"islandProfiles", 0xE748,
     L"Profiles button (switch profiles)",
     L"Botão de perfis (trocar de perfil)"},
    {IslandPref::AutoHide, L"islandAutoHide", 0xE89F,
     L"Hide as its line until the mouse reaches it",
     L"Esconder na linha até o mouse chegar nela"},
    {IslandPref::AllMonitors, L"islandAllMonitors", 0xE7F4, L"On every monitor",
     L"Em todos os monitores"},
    {IslandPref::VolumePercent, L"islandVolumePercent", 0xE995,
     L"Volume percentage beside its icon",
     L"Porcentagem do volume ao lado do ícone"},
    {IslandPref::BatteryPercent, L"islandBatteryPercent", 0xE83F,
     L"Battery percentage beside its icon",
     L"Porcentagem da bateria ao lado do ícone"},
    {IslandPref::Brightness, L"islandBrightness", 0xE706,
     L"Screen brightness (the mouse wheel changes it)",
     L"Brilho da tela (a roda do mouse muda)"},
    {IslandPref::Clipboard, L"islandClipboard", 0xE77F,
     L"Show what's copied (a bubble under the pill)",
     L"Mostrar o que foi copiado (um balão sob a pílula)"},
    {IslandPref::Frosted, L"islandFrosted", 0xE771,
     L"Frosted glass on the bar and the Fixed style's strip",
     L"Vidro fosco na barra e na faixa do estilo Fixa"},
    // Chosen on its own row (see kChoiceDateOrder).
    {IslandPref::DateOrder, L"islandDateOrder", 0xE787, L"Date order",
     L"Ordem da data"},
    {IslandPref::MonthNumber, L"islandMonthNumber", 0xE8BF,
     L"Month as a number (9/10)", L"Mês em número (9/10)"},
    {IslandPref::InputMethod, L"islandInputMethod", 0xE774,
     L"Keyboard language (like Windows' input indicator)",
     L"Idioma do teclado (como o indicador do Windows)"},
    {IslandPref::Privacy, L"islandPrivacy", 0xEA18,
     L"Microphone, camera and location in use (and which app)",
     L"Microfone, câmera e localização em uso (e qual app)"},
    {IslandPref::UsbDrives, L"islandUsbDrives", 0xE88E,
     L"USB drives icon on the pill (open or eject them)",
     L"Ícone dos pendrives na pílula (abrir ou ejetar)"},
    {IslandPref::UsbNotify, L"islandUsbNotify", 0xE7E7,
     L"Notify when a USB drive is plugged in",
     L"Avisar quando um pendrive for conectado"},
    // Chosen on its own row (see kChoiceClockFormat).
    {IslandPref::ClockFormat, L"islandClockFormat", 0xE823, L"Time format",
     L"Formato da hora"},
    // Chosen on its own row (see kChoiceTrayStyle).
    {IslandPref::TrayStyle, L"islandTrayStyle", 0xE712,
     L"Icons that aren't pinned", L"Ícones não fixados"},
    {IslandPref::WindowColor, L"islandWindowColor", 0xE790,
     L"Colors following the app in front (its title bar or its icon)",
     L"Cores seguindo o app da frente (a barra de título ou o ícone)"},
    {IslandPref::Folders, L"islandFolders", 0xE8B7,
     L"Folders and places (your shortcuts)", L"Pastas e locais (seus atalhos)"},
    {IslandPref::Wallpaper, L"islandWallpaper", 0xE8B9,
     L"Wallpapers from a folder (with a transition)",
     L"Papéis de parede de uma pasta (com transição)"},
    // Chosen on its own row (see kChoiceWallpaper).
    {IslandPref::WallpaperTransition, L"islandWallpaperTransition", 0xE8B9,
     L"Transition", L"Transição"},
    {IslandPref::Desktops, L"islandDesktops", 0xE7C4,
     L"Virtual desktops (the mouse wheel switches them)",
     L"Áreas de trabalho virtuais (a roda do mouse troca)"},
    {IslandPref::ActionNetwork, L"pillNetwork", 0xE701, L"Wi-Fi / network",
     L"Wi-Fi / rede"},
    {IslandPref::ActionBluetooth, L"pillBluetooth", 0xE702, L"Bluetooth",
     L"Bluetooth"},
    {IslandPref::ActionNightLight, L"pillNightLight", 0xF08C, L"Night light",
     L"Luz noturna"},
    {IslandPref::ActionHotspot, L"pillHotspot", 0xE88A, L"Mobile hotspot",
     L"Hotspot móvel"},
    {IslandPref::ActionAirplane, L"pillAirplane", 0xE709, L"Airplane mode",
     L"Modo avião"},
    {IslandPref::ActionProject, L"pillProject", 0xEBC6, L"Project",
     L"Projetar"},
    {IslandPref::ActionCast, L"pillCast", 0xEC15, L"Cast", L"Transmitir"},
    {IslandPref::ActionCaptions, L"pillCaptions", 0xE7F0, L"Captions",
     L"Legendas"},
    {IslandPref::ActionAccessibility, L"pillAccessibility", 0xE776,
     L"Accessibility", L"Acessibilidade"},
    {IslandPref::ActionScreenshot, L"pillScreenshot", 0xE722, L"Screenshot",
     L"Captura de tela"},
    {IslandPref::ActionLock, L"pillLock", 0xE72E, L"Lock", L"Bloquear"},
    {IslandPref::ActionEmoji, L"pillEmoji", 0xE76E, L"Emoji", L"Emojis"},
    {IslandPref::ActionClipboard, L"pillClipboard", 0xF0E3, L"Clipboard",
     L"Área de transferência"},
    {IslandPref::ActionTaskManager, L"pillTaskManager", 0xE9D9, L"Task Manager",
     L"Gerenciador de Tarefas"},
    {IslandPref::ActionCalculator, L"pillCalculator", 0xE8EF, L"Calculator",
     L"Calculadora"},
    {IslandPref::ActionExplorer, L"pillExplorer", 0xEC50, L"File Explorer",
     L"Explorador de Arquivos"},
    {IslandPref::ShortcutNotifications, L"islandWinN", 0xE765,
     L"Win+N opens the island's notifications",
     L"Win+N abre as notificações da ilha"},
    {IslandPref::ShortcutControl, L"islandWinA", 0xE765,
     L"Win+A opens the island's control center",
     L"Win+A abre a central de controle da ilha"},
    // Chosen on its own row (see kChoiceNotches).
    {IslandPref::Notches, L"islandNotches", 0xECA5, L"Notches", L"Entalhes"},
    {IslandPref::NotchEach, L"islandNotchEach", 0xE921,
     L"Hide each notch on its own (not all together)",
     L"Esconder cada entalhe sozinho (não todos juntos)"},
    {IslandPref::GroupApps, L"islandGroupApps", 0xE71D,
     L"One icon per app (Phone Link's phone apps together)",
     L"Um ícone por app (os apps do celular do Vincular ao Celular juntos)"},
    {IslandPref::ActionKomorebi, L"pillKomorebi", kGlyphKomorebi,
     L"komorebi (start, stop, reload)",
     L"komorebi (iniciar, parar, recarregar)"},
    {IslandPref::KomorebiWhkd, L"komorebiWhkd", kGlyphKomorebi,
     L"komorebi: with whkd (its shortcuts)",
     L"komorebi: com o whkd (os atalhos dele)"},
    {IslandPref::KomorebiMasir, L"komorebiMasir", kGlyphKomorebi,
     L"komorebi: with masir (focus follows the mouse)",
     L"komorebi: com o masir (o foco segue o mouse)"},
    {IslandPref::ScreenshotFolder, L"islandScreenshotFolder", 0xE722,
     L"Screenshot notifications open the picture (in Explorer)",
     L"Avisos de captura abrem a imagem (no Explorador)"},
    {IslandPref::MediaWheel, L"islandMediaWheel", 0xE893,
     L"The wheel on what's playing changes the song (not the volume)",
     L"A roda no que está tocando troca a música (não o volume)"},
    {IslandPref::MediaMarquee, L"islandMediaMarquee", 0xE8AB,
     L"A long title keeps scrolling while it plays",
     L"Título longo rolando enquanto toca"},
    {IslandPref::NotificationApps, L"islandApps", 0xE7E7,
     L"Icons of the apps with notifications",
     L"Ícones dos apps com notificações"},
    {IslandPref::KomorebiLayout, L"islandKomorebiLayout", kGlyphKomorebiLayout,
     L"komorebi: its layout on the pill (a menu changes it)",
     L"komorebi: o layout na pílula (um menu troca)"},
    {IslandPref::FocusedApp, L"islandFocusedApp", kGlyphFocusedApp,
     L"The app in focus on this monitor (its icon and name)",
     L"O app em foco neste monitor (ícone e nome)"},
    {IslandPref::KomorebiHideEmpty, L"islandKomorebiHideEmpty",
     kGlyphKomorebiLayout, L"komorebi: hide the workspaces without windows",
     L"komorebi: esconder as áreas de trabalho sem janelas"},
    // Chosen on its own row (see kChoiceFolderDrag).
    {IslandPref::FolderDrag, L"islandFolderDrag", 0xE8C8, L"Files dragged out",
     L"Arquivos arrastados para fora"},
    {IslandPref::LiquidGlass, L"islandLiquidGlass", 0xE771,
     L"Glass: see-through and blurred, with a bright edge (Windows' blur: "
     L"needs its transparency effects)",
     L"Vidro: transparente e desfocado, com borda de luz (desfoque do "
     L"Windows: precisa dos efeitos de transparência dele)"},
    {IslandPref::Tips, L"islandTips", 0xE946,
     L"Tips under the icons and buttons (their names, an app's notifications)",
     L"Dicas embaixo dos ícones e botões (os nomes, as notificações de um "
     L"app)"},
    {IslandPref::HoverHighlight, L"islandHoverHighlight", 0xE7C9,
     L"Highlight behind the item under the mouse",
     L"Destaque atrás do item sob o mouse"},
    {IslandPref::MinimizeButton, L"islandMinimizeButton", 0xE70E,
     L"Minimize arrow at the end of the pill",
     L"Seta de minimizar no fim da pílula"},
    {IslandPref::BarCorners, L"islandBarCorners", 0xE7C4,
     L"Bar: items at the screen's corners (the clock in the middle)",
     L"Barra: itens nos cantos da tela (o relógio no meio)"},
    {IslandPref::GlassTranslucent, L"islandGlassTranslucent", 0xF4A5,
     L"Translucent glass: a lighter tint and livelier colors",
     L"Vidro translúcido: tinta mais leve e cores mais vivas"},
    {IslandPref::Clock, L"islandClock", 0xE823, L"Clock", L"Relógio"},
    {IslandPref::ActionShowDesktop, L"pillShowDesktop", 0xE7F4,
     L"Show desktop", L"Mostrar área de trabalho"},
    {IslandPref::MinimizeKey, L"islandMinimizeKey", 0xE765,
     L"Win+J minimizes the island to its line and brings it back",
     L"Win+J minimiza a ilha na linha e a traz de volta"},
    {IslandPref::NetSpeed, L"islandNetSpeed", 0xEC4A,
     L"Network speed (up and down)",
     L"Velocidade da rede (envio e recebimento)"},
    {IslandPref::WorldClocks, L"islandWorldClocks", 0xE909,
     L"World clocks (Windows' additional clocks)",
     L"Relógios mundiais (os relógios adicionais do Windows)"},
    {IslandPref::Countdown, L"islandCountdown", 0xE916,
     L"Time until the next", L"Tempo até o próximo"},
    {IslandPref::RecycleBin, L"islandRecycleBin", 0xE74D, L"Recycle Bin",
     L"Lixeira"},
    {IslandPref::ActionOnScreenKeyboard, L"pillOnScreenKeyboard", 0xE765,
     L"On-screen keyboard", L"Teclado virtual"},
    {IslandPref::StatusNotices, L"islandStatusNotices", 0xE7E7,
     L"Status notices: charger, low battery, Bluetooth audio, Do not disturb",
     L"Avisos de status: carregador, bateria fraca, áudio Bluetooth, Não "
     L"incomodar"},
    {IslandPref::FoldApps, L"islandFoldApps", 0xE76C,
     L"Fold the apps with notifications into an arrow (out when one comes, "
     L"and under the mouse)",
     L"Recolher os apps com notificações numa seta (saem quando chega uma e "
     L"com o mouse em cima)"},
    {IslandPref::Timer, L"islandTimer", 0xE916,
     L"Timer, stopwatch and Pomodoro", L"Timer, cronômetro e Pomodoro"},
    {IslandPref::Visualizer, L"islandVisualizer", 0xE8D6,
     L"Audio visualizer (bars moving with the sound)",
     L"Visualizador de áudio (barras que se mexem com o som)"},
    {IslandPref::CountdownEvents, L"islandCountdownEvents", 0xE787,
     L"Counts down to events",
     L"Conta até os eventos"},
    {IslandPref::CountdownAlarms, L"islandCountdownAlarms", 0xEA8F,
     L"Counts down to alarms",
     L"Conta até os alarmes"},
    {IslandPref::CountdownTimer, L"islandCountdownTimer", 0xE916,
     L"Counts down to the timer's end",
     L"Conta até o fim do timer"},
    {IslandPref::CountdownPomodoro, L"islandCountdownPomodoro", 0xE916,
     L"Counts down to Pomodoro's next part",
     L"Conta até a próxima parte do Pomodoro"},
    {IslandPref::DownloadFill, L"islandDownloadFill", 0xE896,
     L"The pill fills like a loading bar with downloads",
     L"A pílula enche como uma barra de carregamento com downloads"},
    {IslandPref::ChargeFill, L"islandChargeFill", 0xE83E,
     L"The pill fills with the battery's charge while charging",
     L"A pílula enche com a carga da bateria enquanto carrega"},
    // Chosen on its own row (see kChoiceFillColor).
    {IslandPref::FillColor, L"islandFillColor", 0xE896,
     L"Downloads and other apps", L"Downloads e outros apps"},
    {IslandPref::FillOwnColor, L"islandFillOwnColor", 0xE896,
     L"Downloads and other apps (own color)",
     L"Downloads e outros apps (cor própria)"},
    {IslandPref::FillCopyColor, L"islandFillCopyColor", 0xE8C8,
     L"File copies", L"Cópias de arquivos"},
    {IslandPref::FillCopyOwnColor, L"islandFillCopyOwnColor", 0xE8C8,
     L"File copies (own color)", L"Cópias de arquivos (cor própria)"},
    {IslandPref::FillChargeColor, L"islandFillChargeColor", 0xE83E,
     L"Battery charging", L"Carga da bateria"},
    {IslandPref::FillChargeOwnColor, L"islandFillChargeOwnColor", 0xE83E,
     L"Battery charging (own color)", L"Carga da bateria (cor própria)"},
    {IslandPref::FillStyle, L"islandFillStyle", 0xE9E9, L"Fill animation",
     L"Animação do preenchimento"},
    {IslandPref::Lyrics, L"islandLyrics", 0xEC4F,
     L"Song lyrics (goes online: lrclib.net)",
     L"Letras das músicas (acessa a internet: lrclib.net)"},
    {IslandPref::LyricsSize, L"islandLyricsSize", 0xE8E9, L"Lyrics size",
     L"Tamanho da letra"},
    {IslandPref::LyricsColor, L"islandLyricsColor", 0xE790, L"Lyrics color",
     L"Cor da letra"},
    {IslandPref::LyricsOwnColor, L"islandLyricsOwnColor", 0xE790,
     L"Lyrics color (own)", L"Cor da letra (própria)"},
    {IslandPref::LyricsInTitle, L"islandLyricsInTitle", 0xE8D6,
     L"Lyrics replace the song title", L"A letra substitui o nome da música"},
    {IslandPref::VisualizerColor, L"islandVisualizerColor", 0xE790,
     L"Visualizer color", L"Cor do visualizador"},
    {IslandPref::VisualizerOwnColor, L"islandVisualizerOwnColor", 0xE790,
     L"Visualizer color (own)", L"Cor do visualizador (própria)"},
    {IslandPref::RecycleMinSize, L"islandRecycleMinSize", 0xE74D,
     L"Recycle Bin appears", L"A lixeira aparece"},
    {IslandPref::LyricsWidth, L"islandLyricsWidth", 0xE8E9,
     L"Longest line on the pill", L"Comprimento da linha na pílula"},
    {IslandPref::VisualizerSmooth, L"islandVisualizerSmooth", 0xE8D6,
     L"Smooth bars (like the iPhone)", L"Barras suaves (como no iPhone)"},
    {IslandPref::WhatsNewNotice, L"islandWhatsNewNotice", 0xE789,
     L"Tell me what's new after an update",
     L"Avisar das novidades depois de atualizar"},
    // Chosen on their own rows (see kChoiceWeatherUnit, kChoiceNotchOpen,
    // kChoiceIconPack, kChoiceIconColors, kChoiceHideWhen and
    // kChoiceLineFade).
    {IslandPref::WeatherUnit, L"islandWeatherUnit", 0xE753,
     L"Temperature unit", L"Unidade de temperatura"},
    {IslandPref::NotchOpen, L"islandNotchOpen", 0xE8A7,
     L"Line that opens the island", L"Linha que abre a ilha"},
    {IslandPref::IconPack, L"islandIconPack", 0xE91B, L"Icon pack",
     L"Pacote de ícones"},
    {IslandPref::IconColors, L"islandIconColors", 0xE790, L"Icon colors",
     L"Cores dos ícones"},
    {IslandPref::CapsLock, L"islandCapsLock", 0xE765,
     L"Caps Lock bubble", L"Aviso do Caps Lock"},
    {IslandPref::HideWhen, L"islandHideWhen", 0xE89F, L"When it hides",
     L"Quando esconder"},
    {IslandPref::LineFade, L"islandLineFade", 0xE7B3, L"Fade the line after",
     L"Apagar a linha depois de"},
    {IslandPref::HideClock, L"islandHideClock", 0xE823,
     L"Only the clock when hidden", L"Só o relógio ao esconder"},
    // Chosen on the notches' row (see kChoiceNotches).
    {IslandPref::PillNotches, L"islandPillNotches", 0xECA5, L"Notches",
     L"Entalhes"},
    {IslandPref::BarClear, L"islandBarClear", 0xF4A5, L"See-through bar",
     L"Barra transparente"},
    {IslandPref::AiUsage, L"islandAiUsage", kGlyphAiUsage, L"AI usage",
     L"Uso de IA"},
    {IslandPref::AiClaude, L"islandAiClaude", kGlyphAiUsage,
     L"Claude Code (terminal)", L"Claude Code (terminal)"},
    {IslandPref::AiCodex, L"islandAiCodex", kGlyphAiUsage, L"Codex",
     L"Codex"},
    {IslandPref::FocusedTitle, L"islandFocusedTitle", kGlyphFocusedApp,
     L"With its window's title", L"Com o título da janela"},
    {IslandPref::WeatherCards, L"islandWeatherCards", 0xE753,
     L"Details as cards", L"Detalhes em cartões"},
    {IslandPref::CalendarBigDay, L"islandCalendarBigDay", 0xE787,
     L"Calendar with today big", L"Calendário com o dia grande"},
    {IslandPref::Commands, L"islandCommands", 0xE756,
     L"Your commands", L"Seus comandos"},
    // (Chosen on their own rows, see kChoiceBarSide and
    // kChoiceBarInnerCorners.)
    {IslandPref::BarSide, L"islandBarSide", 0xE8E4, L"The bar's side",
     L"Lado da barra"},
    {IslandPref::BarInnerCorners, L"islandBarInnerCorners", 0xE71A,
     L"The bar's inner corners", L"Cantos de dentro da barra"},
    {IslandPref::DndSilent, L"islandDndSilent", kGlyphMoon,
     L"Calls and alarms silent in Do not disturb", L"Chamadas e alarmes em silêncio no Não incomodar"},
    // (Chosen on their own rows, see kChoiceBannerSize.)
    {IslandPref::BannerSize, L"islandBannerSize", 0xE8E9, L"Banner size",
     L"Tamanho do aviso"},
    {IslandPref::BannerWidth, L"islandBannerWidth", 0xE8E9, L"Banner width",
     L"Largura do aviso"},
    {IslandPref::BannerCorners, L"islandBannerCorners", 0xE71A,
     L"Banner corners", L"Cantos do aviso"},
    {IslandPref::BannerPlace, L"islandBannerPlace", 0xE81E,
     L"Where banners show", L"Onde os avisos aparecem"},
    {IslandPref::DownloadStalled, L"islandDownloadStalled", 0xE896,
     L"A browser's download standing still shows paused", L"Download do navegador parado aparece pausado"},
    {IslandPref::FolderThumbnails, L"islandFolderThumbnails", 0xEB9F,
     L"Thumbnails of the files", L"Miniaturas dos arquivos"},
    {IslandPref::Shelf, L"islandShelf", 0xE7B8,
     L"Shelf for files", L"Prateleira de arquivos"},
    {IslandPref::ShelfLimit, L"islandShelfLimit", 0xE7B8, L"Shelf holds",
     L"A prateleira guarda"},
    {IslandPref::TimerLine, L"islandTimerLine", 0xE916,
     L"Timer on the minimized line", L"Timer na linha minimizada"},
    {IslandPref::LineClick, L"islandLineClick", 0xE962, L"Opens the line",
     L"Abre a linha"},
    {IslandPref::ExternalBrightness, L"islandExternalBrightness", 0xE706,
     L"Brightness of external monitors (DDC/CI)", L"Brilho de monitores externos (DDC/CI)"},
    {IslandPref::GlassLight, L"islandGlassLight", 0xE81E,
     L"Light and shadow like macOS 27", L"Luz e sombra como no macOS 27"},
    {IslandPref::Eyes, L"islandEyes", 0xE7B3,
     L"Dragon eyes that follow the mouse", L"Olhos de dragão que seguem o mouse"},
    {IslandPref::EyeTheme, L"islandEyeTheme", 0xE790, L"Eyes' colors",
     L"Cores dos olhos"},
    {IslandPref::EyeShape, L"islandEyeShape", 0xE7B3, L"Eyes' shape",
     L"Formato dos olhos"},
};

// The groups of items on the pill, which can be put in another order in the
// island's settings (the minimize button always ends it).
enum class PillGroup {
    Apps,
    Tray,
    Network,
    Volume,
    Battery,
    Clock,
    Bell,
    Media,
    Power,
    Usage,
    Theme,
    Microphone,
    Notes,
    Tasks,
    Shortcuts,
    Weather,
    Updates,
    Profiles,
    Brightness,
    InputMethod,
    Privacy,
    Drives,
    Desktops,
    // The control center's buttons chosen for the pill, each on its own
    // (see kPillActions): Wi-Fi here, the others after the wallpaper.
    ActionNetwork,
    Folders,
    Wallpaper,
    ActionBluetooth,
    ActionNightLight,
    ActionHotspot,
    ActionAirplane,
    ActionProject,
    ActionCast,
    ActionCaptions,
    ActionAccessibility,
    ActionScreenshot,
    ActionLock,
    ActionEmoji,
    ActionClipboard,
    ActionTaskManager,
    ActionCalculator,
    ActionExplorer,
    ActionKomorebi,
    // komorebi's layout on this monitor, and the app in focus on it.
    KomorebiLayout,
    FocusedApp,
    // (A control center button added later: after the others.)
    ActionShowDesktop,
    // (Items added in 2.1: after the others.)
    NetSpeed,
    WorldClocks,
    Countdown,
    RecycleBin,
    ActionOnScreenKeyboard,
    Timer,
    Visualizer,
    // What's playing's cover and title, and its buttons, each placed on its
    // own (they were part of Media; old orders put them right after it, see
    // DecodePillOrder).
    MediaTitle,
    MediaButtons,
    // The line of the song sung now (see Island::m_lyricsLine).
    Lyrics,
    // (Items added in 3.0: after the others.)
    AiUsage,
    Commands,
    Shelf,
    // (The dragon's eyes: after the others.)
    Eyes,
    Count
};
constexpr int kPillGroups = (int)PillGroup::Count;

struct PillGroupInfo {
    WCHAR glyph;
    PCWSTR english;
    PCWSTR portuguese;
};

const PillGroupInfo kPillGroupInfo[kPillGroups] = {
    {0xE71D, L"Apps with notifications", L"Apps com notificações"},
    {0xE712, L"Other apps' tray icons", L"Ícones dos outros apps"},
    {kGlyphControlCenter, L"Control center", L"Central de controle"},
    {0xE767, L"Volume", L"Volume"},
    {0xE83F, L"Battery", L"Bateria"},
    {0xE823, L"Clock", L"Relógio"},
    {0xEA8F, L"Notifications (bell)", L"Notificações (sino)"},
    {0xE8D6, L"What's playing", L"O que está tocando"},
    {0xE7E8, L"Power button", L"Botão de desligar"},
    {0xE9D9, L"CPU, RAM and VRAM use", L"Uso de CPU, RAM e VRAM"},
    {0xE7A1, L"Light and dark mode", L"Modo claro e escuro"},
    {0xE720, L"Microphone", L"Microfone"},
    {0xE70B, L"Notes", L"Anotações"},
    {0xE9D5, L"Tasks", L"Tarefas"},
    {0xE765, L"Keyboard shortcuts", L"Atalhos do teclado"},
    {0xE753, L"Weather", L"Clima"},
    {0xE895, L"Updates", L"Atualizações"},
    {0xE748, L"Profiles", L"Perfis"},
    {0xE706, L"Screen brightness", L"Brilho da tela"},
    {0xE774, L"Keyboard language", L"Idioma do teclado"},
    {0xEA18, L"Microphone, camera and location in use",
     L"Microfone, câmera e localização em uso"},
    {0xE88E, L"USB drives", L"Pendrives"},
    {0xE7C4, L"Virtual desktops", L"Áreas de trabalho virtuais"},
    {0xE701, L"Wi-Fi / network", L"Wi-Fi / rede"},
    {0xE8B7, L"Folders and places", L"Pastas e locais"},
    {0xE8B9, L"Wallpaper", L"Papel de parede"},
    {0xE702, L"Bluetooth", L"Bluetooth"},
    {0xF08C, L"Night light", L"Luz noturna"},
    {0xE88A, L"Mobile hotspot", L"Hotspot móvel"},
    {0xE709, L"Airplane mode", L"Modo avião"},
    {0xEBC6, L"Project", L"Projetar"},
    {0xEC15, L"Cast", L"Transmitir"},
    {0xE7F0, L"Captions", L"Legendas"},
    {0xE776, L"Accessibility", L"Acessibilidade"},
    {0xE722, L"Screenshot", L"Captura de tela"},
    {0xE72E, L"Lock", L"Bloquear"},
    {0xE76E, L"Emoji", L"Emojis"},
    {0xF0E3, L"Clipboard", L"Área de transferência"},
    {0xE9D9, L"Task Manager", L"Gerenciador de Tarefas"},
    {0xE8EF, L"Calculator", L"Calculadora"},
    {0xEC50, L"File Explorer", L"Explorador de Arquivos"},
    {kGlyphKomorebi, L"komorebi", L"komorebi"},
    {kGlyphKomorebiLayout, L"komorebi's layout", L"Layout do komorebi"},
    {kGlyphFocusedApp, L"App in focus", L"App em foco"},
    {0xE7F4, L"Show desktop", L"Mostrar área de trabalho"},
    {0xEC4A, L"Network speed", L"Velocidade da rede"},
    {0xE909, L"World clocks", L"Relógios mundiais"},
    {0xE916, L"Next event", L"Próximo evento"},
    {0xE74D, L"Recycle Bin", L"Lixeira"},
    {0xE765, L"On-screen keyboard", L"Teclado virtual"},
    {0xE916, L"Timer, stopwatch and Pomodoro", L"Timer, cronômetro e Pomodoro"},
    {0xE8D6, L"Audio visualizer", L"Visualizador de áudio"},
    {0xEB9F, L"Cover and title", L"Capa e título"},
    {0xE768, L"Media buttons", L"Botões de mídia"},
    {0xEC4F, L"Song lyrics", L"Letra da música"},
    {kGlyphAiUsage, L"AI usage", L"Uso de IA"},
    {0xE756, L"Your commands", L"Seus comandos"},
    {0xE7B8, L"Shelf", L"Prateleira"},
    {0xE7B3, L"Dragon eyes", L"Olhos de dragão"},
};

// Whether any of the processor, memory and video memory use is shown (the
// usage group has a switch for each).
bool AnyUsageShown() {
    return g_settings.usageCpu || g_settings.usageRam || g_settings.usageVram;
}

int GetIslandPref(IslandPref pref);
int PillGroupPref(PillGroup group);

// Whether a group is on the pill (for all apps): its option on, or always.
bool PillGroupOn(PillGroup group) {
    const int pref = PillGroupPref(group);
    if (pref >= 0) {
        return GetIslandPref((IslandPref)pref) != 0;
    }
    return group != PillGroup::Usage || AnyUsageShown();
}

// The control center's buttons that can go on the pill (each its own
// option, kept as a bit of pillActions in this order, and its own group of
// the pill, ordered with the others; the control center's own order is
// another).
struct PillAction {
    IslandPref pref;
    ControlId id;
    PillGroup group;
};
constexpr PillAction kPillActions[] = {
    {IslandPref::ActionNetwork, ControlId::Network, PillGroup::ActionNetwork},
    {IslandPref::ActionBluetooth, ControlId::Bluetooth, PillGroup::ActionBluetooth},
    {IslandPref::ActionNightLight, ControlId::NightLight, PillGroup::ActionNightLight},
    {IslandPref::ActionHotspot, ControlId::Hotspot, PillGroup::ActionHotspot},
    {IslandPref::ActionAirplane, ControlId::Airplane, PillGroup::ActionAirplane},
    {IslandPref::ActionProject, ControlId::Project, PillGroup::ActionProject},
    {IslandPref::ActionCast, ControlId::Cast, PillGroup::ActionCast},
    {IslandPref::ActionCaptions, ControlId::Captions, PillGroup::ActionCaptions},
    {IslandPref::ActionAccessibility, ControlId::Accessibility, PillGroup::ActionAccessibility},
    {IslandPref::ActionScreenshot, ControlId::Screenshot, PillGroup::ActionScreenshot},
    {IslandPref::ActionLock, ControlId::Lock, PillGroup::ActionLock},
    {IslandPref::ActionEmoji, ControlId::Emoji, PillGroup::ActionEmoji},
    {IslandPref::ActionClipboard, ControlId::Clipboard, PillGroup::ActionClipboard},
    {IslandPref::ActionTaskManager, ControlId::TaskManager, PillGroup::ActionTaskManager},
    {IslandPref::ActionCalculator, ControlId::Calculator, PillGroup::ActionCalculator},
    {IslandPref::ActionExplorer, ControlId::Explorer, PillGroup::ActionExplorer},
    {IslandPref::ActionKomorebi, ControlId::Komorebi, PillGroup::ActionKomorebi},
    {IslandPref::ActionShowDesktop, ControlId::ShowDesktop, PillGroup::ActionShowDesktop},
    {IslandPref::ActionOnScreenKeyboard, ControlId::OnScreenKeyboard, PillGroup::ActionOnScreenKeyboard},
};

// The button a group is (its index in kPillActions), or -1.
int PillActionOfGroup(PillGroup group) {
    for (size_t i = 0; i < ARRAYSIZE(kPillActions); i++) {
        if (kPillActions[i].group == group) {
            return (int)i;
        }
    }
    return -1;
}

int PillActionIndex(IslandPref pref) {
    for (size_t i = 0; i < ARRAYSIZE(kPillActions); i++) {
        if (kPillActions[i].pref == pref) {
            return (int)i;
        }
    }
    return -1;
}

// A pill action's option set in pillActions.
void SetPillActionBit(IslandPref pref, bool on) {
    const int index = PillActionIndex(pref);
    if (index >= 0) {
        g_settings.pillActions = on ? g_settings.pillActions | (1u << index)
                                    : g_settings.pillActions & ~(1u << index);
    }
}

// The groups that go online (weather, updates) show only with their own
// option on: an app's own icons can hide them, never turn them on.
bool OnlineGroupAllowed(PillGroup group) {
    switch (group) {
        case PillGroup::Weather:
            return g_settings.islandWeather;
        case PillGroup::Updates:
            return g_settings.islandUpdates;
        default:
            return true;
    }
}

// The settings' profiles: the island's options saved under a name (for work,
// for games, one with everything on...), applied again with a click. Each is
// kept in a value of its own ("profile" and its id), listed in "profiles";
// "profileActive" is the one applied last (see ApplyProfile).
struct IslandProfile {
    int id;
    std::wstring name;
};
constexpr size_t kMaxProfiles = 10;
constexpr size_t kMaxProfileName = 32;
std::vector<IslandProfile> g_profiles;
int g_activeProfile = -1;
// While a profile is applied, its options don't each redraw the island (it
// does once at the end).
bool g_applyingProfile = false;
void LoadProfiles();
std::wstring NewProfileName();
// Each returns -1 or false when it can't (no room, no such profile).
int AddProfile(const std::wstring& name);
void UpdateProfile(int id);
void SaveActiveProfile();
void RenameProfile(int id, const std::wstring& name);
void DeleteProfile(int id);
bool ApplyProfile(int id);
// Whether the settings are no longer the ones saved in a profile.
bool ProfileChanged(int id);
// The profile the current island shows: the one in use, or another
// monitor's own (-1: the main island's settings).
int ChosenProfile();
// Everything the user made and chose in the island, as text for a file
// (see kBackupValues), and put back from one (false if it isn't one).
std::wstring CaptureBackup();
bool RestoreBackup(const std::wstring& text);
std::wstring LogsText();
void OpenWindhawk();
// The island's settings, opened from another panel.
void OpenIslandSettingsFromPanel();

// Profiles that turn on by themselves (#6166): one with an app in front
// (back to the one in use before when another app comes, unless it stays,
// see CheckAutoProfile), or one with a style chosen. Kept for every
// profile, in "autoProfiles": the profile's id, a kind ("a" an app's ID,
// "s" a style as IslandPref::Bar, "k" staying after its app) and its value,
// tab-separated, a line each. An app or a style turns on one profile at
// most.
struct AutoProfileRule {
    int id;
    WCHAR kind;
    std::wstring value;
};
constexpr size_t kMaxAutoProfileRules = 40;
std::vector<AutoProfileRule> g_autoProfileRules;
// (Read again after the profiles are, see LoadProfiles.)
bool g_autoProfileRulesRead = false;

const std::vector<AutoProfileRule>& AutoProfileRules() {
    if (g_autoProfileRulesRead) {
        return g_autoProfileRules;
    }
    g_autoProfileRulesRead = true;
    g_autoProfileRules.clear();
    const std::wstring value = GetWholeStringValue(L"autoProfiles", 8192);
    for (PCWSTR line = value.c_str(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        const std::wstring text =
            end ? std::wstring(line, end) : std::wstring(line);
        const size_t tab = text.find(L'\t');
        if (tab != std::wstring::npos && tab + 3 < text.size() &&
            text[tab + 2] == L'\t' &&
            g_autoProfileRules.size() < kMaxAutoProfileRules) {
            g_autoProfileRules.push_back(
                {_wtoi(text.c_str()), text[tab + 1], text.substr(tab + 3)});
        }
        if (!end) {
            break;
        }
        line = end + 1;
    }
    return g_autoProfileRules;
}

bool ProfileExists(int id) {
    return std::any_of(
        g_profiles.begin(), g_profiles.end(),
        [id](const IslandProfile& profile) { return profile.id == id; });
}

bool AutoProfileHas(int id, WCHAR kind, const std::wstring& value) {
    return std::any_of(AutoProfileRules().begin(), AutoProfileRules().end(),
                       [&](const AutoProfileRule& rule) {
                           return rule.id == id && rule.kind == kind &&
                                  _wcsicmp(rule.value.c_str(),
                                           value.c_str()) == 0;
                       });
}

// The profile an app (kind "a") or a style ("s") turns on, or -1.
int AutoProfileFor(WCHAR kind, const std::wstring& value) {
    for (const auto& rule : AutoProfileRules()) {
        if (rule.kind == kind &&
            _wcsicmp(rule.value.c_str(), value.c_str()) == 0 &&
            ProfileExists(rule.id)) {
            return rule.id;
        }
    }
    return -1;
}

// The apps turning profile `id` on.
std::vector<std::wstring> AutoProfileApps(int id) {
    std::vector<std::wstring> apps;
    for (const auto& rule : AutoProfileRules()) {
        if (rule.id == id && rule.kind == L'a') {
            apps.push_back(rule.value);
        }
    }
    return apps;
}

// A rule on or off (an app or a style taken from another profile; the
// deleted profiles' rules go).
void SetAutoProfileRule(int id, WCHAR kind, const std::wstring& value,
                        bool on) {
    std::vector<AutoProfileRule> rules = AutoProfileRules();
    std::erase_if(rules, [&](const AutoProfileRule& rule) {
        return !ProfileExists(rule.id) ||
               (rule.kind == kind &&
                _wcsicmp(rule.value.c_str(), value.c_str()) == 0 &&
                (kind != L'k' || rule.id == id));
    });
    if (on && rules.size() < kMaxAutoProfileRules) {
        rules.push_back({id, kind, value});
    }
    std::wstring text;
    for (const auto& rule : rules) {
        text += std::to_wstring(rule.id) + L"\t" + rule.kind + L"\t" +
                rule.value + L"\n";
    }
    SetStoredString(L"autoProfiles", text.c_str());
    g_autoProfileRules = std::move(rules);
}

// A profile applied by hand: the app's one no longer goes back.
void ForgetAutoProfile();

// The option that shows a group, or -1 for the ones always there.
int PillGroupPref(PillGroup group) {
    switch (group) {
        case PillGroup::Apps:
            return (int)IslandPref::NotificationApps;
        case PillGroup::KomorebiLayout:
            return (int)IslandPref::KomorebiLayout;
        case PillGroup::FocusedApp:
            return (int)IslandPref::FocusedApp;
        case PillGroup::Tray:
            return (int)IslandPref::Tray;
        case PillGroup::Network:
            return (int)IslandPref::Network;
        case PillGroup::Volume:
            return (int)IslandPref::Volume;
        case PillGroup::Battery:
            return (int)IslandPref::Battery;
        case PillGroup::Bell:
            return (int)IslandPref::Bell;
        case PillGroup::Media:
            return (int)IslandPref::Media;
        case PillGroup::Power:
            return (int)IslandPref::Power;
        case PillGroup::Theme:
            return (int)IslandPref::ThemeButton;
        case PillGroup::Microphone:
            return (int)IslandPref::Microphone;
        case PillGroup::Notes:
            return (int)IslandPref::Notes;
        case PillGroup::Tasks:
            return (int)IslandPref::Tasks;
        case PillGroup::Shortcuts:
            return (int)IslandPref::Shortcuts;
        case PillGroup::Weather:
            return (int)IslandPref::Weather;
        case PillGroup::Updates:
            return (int)IslandPref::Updates;
        case PillGroup::Profiles:
            return (int)IslandPref::Profiles;
        case PillGroup::Brightness:
            return (int)IslandPref::Brightness;
        case PillGroup::InputMethod:
            return (int)IslandPref::InputMethod;
        case PillGroup::Privacy:
            return (int)IslandPref::Privacy;
        case PillGroup::Drives:
            return (int)IslandPref::UsbDrives;
        case PillGroup::Desktops:
            return (int)IslandPref::Desktops;
        case PillGroup::Folders:
            return (int)IslandPref::Folders;
        case PillGroup::Wallpaper:
            return (int)IslandPref::Wallpaper;
        case PillGroup::NetSpeed:
            return (int)IslandPref::NetSpeed;
        case PillGroup::WorldClocks:
            return (int)IslandPref::WorldClocks;
        case PillGroup::Countdown:
            return (int)IslandPref::Countdown;
        case PillGroup::RecycleBin:
            return (int)IslandPref::RecycleBin;
        case PillGroup::Timer:
            return (int)IslandPref::Timer;
        case PillGroup::Visualizer:
            return (int)IslandPref::Visualizer;
        case PillGroup::Lyrics:
            return (int)IslandPref::Lyrics;
        case PillGroup::MediaTitle:
            return (int)IslandPref::MediaTitle;
        case PillGroup::MediaButtons:
            return (int)IslandPref::MediaButtons;
        case PillGroup::AiUsage:
            return (int)IslandPref::AiUsage;
        case PillGroup::Commands:
            return (int)IslandPref::Commands;
        case PillGroup::Shelf:
            return (int)IslandPref::Shelf;
        case PillGroup::Eyes:
            return (int)IslandPref::Eyes;
        default: {
            const int action = PillActionOfGroup(group);
            return action >= 0 ? (int)kPillActions[action].pref : -1;
        }
    }
}

// The order chosen (the island's thread, or while it's stopped), kept as
// text (see EncodePillOrder).
std::vector<PillGroup> g_pillOrder;

// An order of the pill's groups as text: a character each, from '0' (kept
// for all apps in "islandOrder", and for an app in its place).
std::wstring EncodePillOrder(const std::vector<PillGroup>& order) {
    std::wstring text;
    for (PillGroup group : order) {
        text += (WCHAR)(L'0' + (int)group);
    }
    return text;
}

// The group a missing one goes after (see DecodePillOrder): the one before
// it in the enum, with the control center's buttons together, and the
// media's parts after the note (they were in it).
int GroupBefore(PillGroup group, int index) {
    if (group == PillGroup::MediaTitle) {
        return (int)PillGroup::Media;
    }
    if (group == PillGroup::MediaButtons) {
        return (int)PillGroup::MediaTitle;
    }
    if (group == PillGroup::Lyrics) {
        return (int)PillGroup::MediaButtons;
    }
    if (group == PillGroup::AiUsage) {
        return (int)PillGroup::Usage;
    }
    if (group == PillGroup::Shelf) {
        return (int)PillGroup::Folders;
    }
    const int action = PillActionOfGroup(group);
    return action > 0 ? (int)kPillActions[action - 1].group : index - 1;
}

// The groups read back (each once), and the ones missing (all of them the
// first time, or a new one) after the group that comes before them in the
// usual order (see GroupBefore).
std::vector<PillGroup> DecodePillOrder(std::wstring_view text) {
    std::vector<PillGroup> order;
    for (WCHAR c : text) {
        if (c < L'0' || c >= L'0' + kPillGroups) {
            continue;
        }
        const auto group = (PillGroup)(c - L'0');
        if (std::find(order.begin(), order.end(), group) == order.end()) {
            order.push_back(group);
        }
    }
    for (int i = 0; i < kPillGroups; i++) {
        const auto group = (PillGroup)i;
        if (std::find(order.begin(), order.end(), group) != order.end()) {
            continue;
        }
        auto at = order.begin();
        const int before = GroupBefore(group, i);
        if (auto it = std::find(order.begin(), order.end(), (PillGroup)before);
            it != order.end() && before != i - 1) {
            at = it + 1;
        } else {
            for (int j = before; j >= 0; j--) {
                it = std::find(order.begin(), order.end(), (PillGroup)j);
                if (it != order.end()) {
                    at = it + 1;
                    break;
                }
            }
        }
        order.insert(at, group);
    }
    return order;
}

void LoadPillOrder() {
    g_pillOrder =
        DecodePillOrder(GetWholeStringValue(L"islandOrder", kPillGroups + 1));
}

void SavePillOrder() {
    StoreProfileString(L"islandOrder", EncodePillOrder(g_pillOrder).c_str());
}

// The other apps' tray icons as arranged on the island, each look its own
// like the pill's order (the main settings and the islands following them,
// a profile, another monitor's profile): the ones pinned on the pill, the
// order they were put in (the others after them, in Windows' order) and,
// for pinned ones put elsewhere on the pill than with the tray, the item
// they're beside (see TrayAnchor).
std::vector<std::wstring> g_trayPinned;
std::vector<std::wstring> g_trayOrder;
std::vector<std::pair<std::wstring, int>> g_trayAnchors;
// (Remembered for icons not there now too, up to this many.)
constexpr size_t kTrayOrderMax = 64;

