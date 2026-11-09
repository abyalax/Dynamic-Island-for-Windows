# Codebase Analysis Report

- Root: \`/home/claude/Dynamic-Island-for-Windows\`
- Generated: 2026-11-08T22:49:02+00:00
- Files analyzed: **1**
- Total physical lines: **114,200**
- Nonblank lines: **110,793**
- Source files (\`.c/.cpp/.h\` family): **1**
- Function definitions detected (heuristic): **2919**

> Static-analysis note: function, variable, API, and call-graph results are heuristic inventories, not compiler AST output. Validate ambiguous matches against source context.

## 1. Repository Structure

| Extension | Files |
|---|---:|
| \`.cpp\` | 1 |

### Largest files

| File | Lines | Size (bytes) |
|---|---:|---:|
| \`output/local@mac-island-dock-fork.wh.cpp\` | 114,200 | 5,155,778 |

## 2. Entry Points and Lifecycle Candidates

| Symbol | File | Line |
|---|---|---:|
| \`Wh_ModInit\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 111157 |
| \`Wh_ModAfterInit\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 111247 |
| \`Wh_ModBeforeUninit\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 111315 |
| \`Wh_ModSettingsChanged\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 111321 |
| \`Wh_ModUninit\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 111335 |

### Lifecycle-related symbol occurrences

| Symbol | Occurrences |
|---|---:|
| \`main\` | 170 |
| \`OnMouseUp\` | 35 |
| \`OnOpen\` | 32 |
| \`OnMouseMove\` | 31 |
| \`OnMouseLeave\` | 31 |
| \`Initialize\` | 24 |
| \`OnWheel\` | 21 |
| \`OnAppMessage\` | 19 |
| \`Stop\` | 18 |
| \`OnDestroy\` | 18 |
| \`OnPreferencesChanged\` | 18 |
| \`OnMouseDown\` | 17 |
| \`OnKey\` | 16 |
| \`OnChar\` | 15 |
| \`OnSearchChanged\` | 12 |
| \`OnVolumeChanged\` | 12 |
| \`OnClose\` | 10 |
| \`OnTimer\` | 10 |
| \`OnPlacesChanged\` | 10 |
| \`Start\` | 9 |
| \`OnCaptureLost\` | 9 |
| \`OnScreenKeyboard\` | 8 |
| \`OnWindowsBanner\` | 8 |
| \`OnDoNotDisturbChanged\` | 7 |
| \`OnOrderChanged\` | 6 |
| \`OnMouseButton\` | 6 |
| \`OnIconsRead\` | 5 |
| \`OnWeather\` | 5 |
| \`OnNetworkChanged\` | 5 |
| \`OnProfileApplied\` | 5 |
| \`OnClick\` | 5 |
| \`OnMonitorChoiceChanged\` | 5 |
| \`OnRenderingUnsafe\` | 4 |
| \`OnDrawingDeviceLost\` | 4 |
| \`OnOtherButton\` | 4 |
| \`OnWifiJoinEnded\` | 4 |
| \`OnDeviceLost\` | 4 |
| \`OnNetSpeedTimer\` | 4 |
| \`OnForegroundChanged\` | 4 |
| \`OnClipboardChanged\` | 4 |
| \`OnTrayArranged\` | 4 |
| \`OnUpdates\` | 4 |
| \`OnUsagePanelChanged\` | 4 |
| \`OnRendering\` | 3 |
| \`OnAppNotified\` | 3 |
| \`OnAppFlashed\` | 3 |
| \`OnWindowProgress\` | 3 |
| \`OnWindowTargetMade\` | 3 |
| \`OnResized\` | 3 |
| \`OnActivated\` | 3 |
| \`OnSysKey\` | 3 |
| \`OnKeyUp\` | 3 |
| \`OnColorDone\` | 3 |
| \`OnShortcutRecorded\` | 3 |
| \`OnLookColorDone\` | 3 |
| \`OnRecycleCounted\` | 3 |
| \`OnAiUsageRead\` | 3 |
| \`OnAiUsageMinute\` | 3 |
| \`OnNotificationsSeen\` | 3 |
| \`OnToastsChanged\` | 3 |
| \`OnInputMethod\` | 3 |
| \`OnPrivacy\` | 3 |
| \`OnDeviceChange\` | 3 |
| \`OnBannerChar\` | 3 |
| \`OnWindowsBannerSeen\` | 3 |
| \`OnWindowsBannersGone\` | 3 |
| \`OnWindowsBannersRead\` | 3 |
| \`OnTrayAppWindow\` | 3 |
| \`OnInternetBack\` | 3 |
| \`OnUsage\` | 3 |
| \`OnEyesTimer\` | 3 |
| \`OnBannerRestoreTimer\` | 3 |
| \`OnOwnNoticeAction\` | 3 |
| \`OnRunningStateChanged\` | 2 |
| \`OnDockPressed\` | 2 |
| \`OnTrayNotify\` | 2 |
| \`OnTrayIconRectRequest\` | 2 |
| \`ThreadProc\` | 2 |
| \`OnDeviceStateChanged\` | 2 |
| \`OnDeviceAdded\` | 2 |

## 3. Configurable Settings and Variable Candidates

- Windhawk setting candidates: **172**
- Global/declaration candidates: **12666**

### Windhawk settings detected

| Setting | Default | File | Line |
|---|---|---|---:|
| \`taskbarMode\` | \`windows\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1534 |
| \`dock\` | \`"Dock: compact and floating, see-through, like on macOS"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1552 |
| \`windowsTransparent\` | \`"Windows taskbar, transparent: only the apps, with a soft shadow, hiding itself"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1553 |
| \`windowsApps\` | \`"Windows taskbar, only the apps (the island shows the rest)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1554 |
| \`windows\` | \`"Windows taskbar, unchanged (to use other taskbar mods)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1555 |
| \`dock\` | \`"Dock: compacta e flutuante, transparente, como no macOS"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1557 |
| \`windowsTransparent\` | \`"Barra do Windows transparente: só os apps, com uma sombra suave, se escondendo sozinha"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1558 |
| \`windowsApps\` | \`"Barra do Windows só com os apps (a ilha mostra o resto)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1559 |
| \`windows\` | \`"Barra do Windows sem mudanças (para usar outros mods de barra)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1560 |
| \`dock\` | \`"程序坞：紧凑、悬浮、半透明，像 macOS 一样"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1562 |
| \`windowsTransparent\` | \`"Windows 任务栏，透明：只显示应用，带柔和阴影，会自动隐藏"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1563 |
| \`windowsApps\` | \`"Windows 任务栏，只显示应用（其余由灵动岛显示）"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1564 |
| \`windows\` | \`"Windows 任务栏，不做更改（可与其他任务栏 mod 一起使用）"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1565 |
| \`dock\` | \`"Dock: kompakt und schwebend, durchsichtig, wie unter macOS"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1567 |
| \`windowsTransparent\` | \`"Windows-Taskleiste, transparent: nur die Apps, mit weichem Schatten, blendet sich selbst aus"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1568 |
| \`windowsApps\` | \`"Windows-Taskleiste, nur die Apps (die Insel zeigt den Rest)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1569 |
| \`windows\` | \`"Windows-Taskleiste, unverändert (für die Nutzung anderer Taskleisten-Mods)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1570 |
| \`dock\` | \`"Dock: compacto y flotante, transparente, como en macOS"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1572 |
| \`windowsTransparent\` | \`"Barra de tareas de Windows, transparente: solo las apps, con una sombra suave, se oculta sola"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1573 |
| \`windowsApps\` | \`"Barra de tareas de Windows, solo las apps (la isla muestra el resto)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1574 |
| \`windows\` | \`"Barra de tareas de Windows, sin cambios (para usar otros mods de la barra de tareas)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1575 |
| \`dock\` | \`"Док: компактный и плавающий, прозрачный, как в macOS"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1577 |
| \`windowsTransparent\` | \`"Панель задач Windows, прозрачная: только приложения, с мягкой тенью, скрывается сама"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1578 |
| \`windowsApps\` | \`"Панель задач Windows, только приложения (остальное показывает остров)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1579 |
| \`windows\` | \`"Панель задач Windows, без изменений (для других модов панели задач)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1580 |
| \`dock\` | \`"Dock: コンパクトで浮かぶ透明なバー（macOS のように）"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1582 |
| \`windowsTransparent\` | \`"Windows タスクバー、透明: アプリのみ、やわらかい影付きで自動的に隠れる"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1583 |
| \`windowsApps\` | \`"Windows タスクバー、アプリのみ（その他はアイランドが表示）"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1584 |
| \`windows\` | \`"Windows タスクバー、変更なし（他のタスクバー Mod 用）"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1585 |
| \`dock\` | \`"Dock: nhỏ gọn, nổi và trong suốt giống macOS"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1587 |
| \`windowsTransparent\` | \`"Taskbar Windows trong suốt: chỉ hiện icon ứng dụng, có đổ bóng nhẹ và tự ẩn"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1588 |
| \`windowsApps\` | \`"Taskbar Windows chỉ hiện ứng dụng (đồng hồ và khay hệ thống chuyển lên đảo)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1589 |
| \`windows\` | \`"Taskbar Windows mặc định, giữ nguyên (để dùng chung với các mod taskbar khác)"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1590 |
| \`windowsHideTray\` | \`false\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1591 |
| \`showIsland\` | \`true\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1608 |
| \`calendarIcs\` | \`[""]\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1625 |
| \`hiding\` | \`\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1642 |
| \`autoHide\` | \`false\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1643 |
| \`hideWhen\` | \`mouseAway\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1660 |
| \`mouseAway\` | \`Mouse away\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1678 |
| \`maximized\` | \`Window maximized\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1679 |
| \`windowOver\` | \`Window over it\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1680 |
| \`mouseAway\` | \`Mouse longe\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1682 |
| \`maximized\` | \`Janela maximizada\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1683 |
| \`windowOver\` | \`Janela por cima\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1684 |
| \`mouseAway\` | \`鼠标离开\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1686 |
| \`maximized\` | \`窗口最大化\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1687 |
| \`windowOver\` | \`窗口挡住它\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1688 |
| \`mouseAway\` | \`Maus weg\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1690 |
| \`maximized\` | \`Fenster maximiert\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1691 |
| \`windowOver\` | \`Fenster darüber\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1692 |
| \`mouseAway\` | \`Ratón lejos\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1694 |
| \`maximized\` | \`Ventana maximizada\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1695 |
| \`windowOver\` | \`Ventana encima\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1696 |
| \`mouseAway\` | \`Мышь далеко\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1698 |
| \`maximized\` | \`Окно развёрнуто\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1699 |
| \`windowOver\` | \`Окно над ней\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1700 |
| \`mouseAway\` | \`マウスが離れたとき\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1702 |
| \`maximized\` | \`ウィンドウの最大化\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1703 |
| \`windowOver\` | \`ウィンドウが重なったとき\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1704 |
| \`mouseAway\` | \`Chuột rời đi\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1706 |
| \`maximized\` | \`Cửa sổ phóng to\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1707 |
| \`windowOver\` | \`Cửa sổ đè lên\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1708 |
| \`showDelay\` | \`0\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1709 |
| \`revealHeight\` | \`0\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1726 |
| \`hideDelay\` | \`400\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1743 |
| \`showDuration\` | \`260\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1760 |
| \`hideDuration\` | \`220\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1777 |
| \`fullscreenGuard\` | \`true\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1794 |
| \`icons\` | \`\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1827 |
| \`magnification\` | \`170\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1828 |
| \`magnificationRange\` | \`150\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1845 |
| \`onlyApps\` | \`false\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1862 |
| \`bounceOnLaunch\` | \`true\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1879 |
| \`keepOpenOnLaunch\` | \`true\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1896 |
| \`keepOpenDuration\` | \`1500\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1913 |
| \`dock\` | \`\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1946 |
| \`iconSize\` | \`32\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1947 |
| \`iconGap\` | \`8\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1964 |
| \`bottomGap\` | \`2\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1981 |
| \`cornerRadius\` | \`22\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1998 |
| \`maxWidth\` | \`0\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2015 |
| \`background\` | \`true\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2032 |
| \`backgroundOpacity\` | \`50\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2049 |
| \`dotIndicator\` | \`true\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2066 |
| \`attentionMode\` | \`icon\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2083 |
| \`icon\` | \`Only the app's icon peeks out, bouncing\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2101 |
| \`dock\` | \`The whole dock shows up\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2102 |
| \`none\` | \`Nothing\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2103 |
| \`icon\` | \`Só o ícone do app aparece, pulando\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2105 |
| \`dock\` | \`A dock inteira aparece\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2106 |
| \`none\` | \`Nada\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2107 |
| \`icon\` | \`只有应用图标探出并跳动\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2109 |
| \`dock\` | \`整个程序坞显示出来\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2110 |
| \`none\` | \`什么都不做\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2111 |
| \`icon\` | \`Nur das App-Symbol schaut hüpfend hervor\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2113 |
| \`dock\` | \`Das ganze Dock erscheint\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2114 |
| \`none\` | \`Nichts\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2115 |
| \`icon\` | \`Solo asoma el icono de la app, rebotando\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2117 |
| \`dock\` | \`Aparece todo el dock\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2118 |
| \`none\` | \`Nada\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2119 |
| \`icon\` | \`Выглядывает только значок приложения, подпрыгивая\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2121 |
| \`dock\` | \`Появляется весь док\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2122 |
| \`none\` | \`Ничего\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2123 |
| \`icon\` | \`アプリのアイコンだけがバウンスしながらのぞく\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2125 |
| \`dock\` | \`Dock 全体が表示される\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2126 |
| \`none\` | \`何もしない\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2127 |
| \`icon\` | \`Chỉ nhô riêng icon của ứng dụng đó lên và nảy nhẹ\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2129 |
| \`dock\` | \`Trượt cả thanh Dock lên\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2130 |
| \`none\` | \`Không làm gì\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2131 |
| \`hideTray\` | \`false\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2132 |
| \`animations\` | \`\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2165 |
| \`enabled\` | \`true\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2166 |
| \`level\` | \`all\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2183 |
| \`all\` | \`All\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2201 |
| \`partial\` | \`"Partial: no bounces, glows, color blends, counting numbers nor scrolling text"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2202 |
| \`none\` | \`None\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2203 |
| \`all\` | \`Todas\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2205 |
| \`partial\` | \`"Parciais: sem quiques, brilhos, mistura de cores, números contando nem texto rolando"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2206 |
| \`none\` | \`Nenhuma\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2207 |
| \`all\` | \`全部\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2209 |
| \`partial\` | \`"部分：没有弹跳、发光、颜色渐变、数字滚动和文字滚动"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2210 |
| \`none\` | \`无\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2211 |
| \`all\` | \`Alle\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2213 |
| \`partial\` | \`"Teilweise: kein Hüpfen, Leuchten, Farbübergänge, Zählzahlen oder Lauftext"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2214 |
| \`none\` | \`Keine\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2215 |
| \`all\` | \`Todas\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2217 |
| \`partial\` | \`"Parcial: sin rebotes, brillos, mezclas de color, números que cuentan ni texto que se desplaza"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2218 |
| \`none\` | \`Ninguna\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2219 |
| \`all\` | \`Все\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2221 |
| \`partial\` | \`"Частично: без подпрыгиваний, свечения, смешения цветов, счёта чисел и прокрутки текста"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2222 |
| \`none\` | \`Нет\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2223 |
| \`all\` | \`すべて\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2225 |
| \`partial\` | \`"一部: バウンス、グロー、色のブレンド、数字のカウント、テキストのスクロールなし"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2226 |
| \`none\` | \`なし\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2227 |
| \`all\` | \`Đầy đủ\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2229 |
| \`partial\` | \`"Cơ bản: tắt hiệu ứng nảy, phát sáng, chuyển màu, nhảy số và chạy chữ"\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2230 |
| \`none\` | \`Tắt hết\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2231 |
| \`speed\` | \`125\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2232 |
| \`frameRate\` | \`screen\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2249 |
| \`screen\` | \`The screen's rate\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2267 |
| \`fps90\` | \`About 90 a second\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2268 |
| \`fps60\` | \`About 60 a second\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2269 |
| \`fps30\` | \`About 30 a second\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2270 |
| \`screen\` | \`A taxa da tela\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2272 |
| \`fps90\` | \`Cerca de 90 por segundo\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2273 |
| \`fps60\` | \`Cerca de 60 por segundo\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2274 |
| \`fps30\` | \`Cerca de 30 por segundo\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2275 |
| \`screen\` | \`屏幕刷新率\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2277 |
| \`fps90\` | \`约每秒 90 帧\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2278 |
| \`fps60\` | \`约每秒 60 帧\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2279 |
| \`fps30\` | \`约每秒 30 帧\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2280 |
| \`screen\` | \`Die Rate des Bildschirms\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2282 |
| \`fps90\` | \`Etwa 90 pro Sekunde\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2283 |
| \`fps60\` | \`Etwa 60 pro Sekunde\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2284 |
| \`fps30\` | \`Etwa 30 pro Sekunde\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2285 |
| \`screen\` | \`La tasa de la pantalla\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2287 |
| \`fps90\` | \`Unos 90 por segundo\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2288 |
| \`fps60\` | \`Unos 60 por segundo\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2289 |
| \`fps30\` | \`Unos 30 por segundo\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2290 |
| \`screen\` | \`Частота экрана\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2292 |
| \`fps90\` | \`Около 90 в секунду\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2293 |
| \`fps60\` | \`Около 60 в секунду\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2294 |
| \`fps30\` | \`Около 30 в секунду\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2295 |
| \`screen\` | \`画面のリフレッシュ レート\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2297 |
| \`fps90\` | \`約 90 フレーム/秒\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2298 |
| \`fps60\` | \`約 60 フレーム/秒\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2299 |
| \`fps30\` | \`約 30 フレーム/秒\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2300 |
| \`screen\` | \`Theo tần số quét màn hình\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2302 |
| \`fps90\` | \`Khoảng 90 FPS\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2303 |
| \`fps60\` | \`Khoảng 60 FPS\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2304 |
| \`fps30\` | \`Khoảng 30 FPS\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 2305 |

Variable candidates are exported to \`variable-candidates.csv\`; inspect each declaration before classifying it as global/configuration/state.

## 4. Function Inventory

Function definitions detected: **2919**

### Largest functions by estimated body span

| Function | File | Lines | Incoming calls (heuristic) |
|---|---|---:|---:|
| \`Island::Render\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1875 | 68 |
| \`Island::Layout\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1627 | 152 |
| \`Island::HandleMessage\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1234 | 3 |
| \`ControlPanel::DrawSettingEntry\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1129 | 1 |
| \`ControlPanel::OnMouseUp\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 1022 | 1 |
| \`ControlPanel::SettingDescription\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 804 | 2 |
| \`FoldersPanel::Draw\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 702 | 3 |
| \`ControlPanel::Layout\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 677 | 152 |
| \`OnRenderingUnsafe\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 584 | 1 |
| \`SetIslandPref\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 572 | 15 |
| \`ControlPanel::HitTest\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 505 | 62 |
| \`SetIslandPrefValue\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 482 | 3 |
| \`Island::AnimationFrame\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 468 | 1 |
| \`CalendarPanel::DrawCalendar\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 459 | 1 |
| \`NotificationPanel::Draw\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 454 | 3 |
| \`LoadSettings\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 428 | 6 |
| \`Island::OnClick\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 426 | 2 |
| \`ControlPanel::DrawControl\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 375 | 2 |
| \`ControlPanel::LayoutSettings\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 359 | 1 |
| \`ControlPanel::PlaceSettingParts\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 344 | 1 |
| \`MediaThreadProc\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 333 | 0 |
| \`Island::DrawBannerContents\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 333 | 2 |
| \`TaskbarSubclassProc\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 322 | 0 |
| \`GetIslandPref\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 322 | 25 |
| \`FoldersPanel::OnMouseUp\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 316 | 1 |
| \`ControlPanel::SettingName\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 315 | 4 |
| \`WeatherPanel::Draw\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 283 | 3 |
| \`ControlPanel::OnAppMessage\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 279 | 1 |
| \`CalendarPanel::DrawBigDay\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 273 | 1 |
| \`Panel::HandleMessage\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 268 | 3 |
| \`ControlPanel::SettingText\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 265 | 1 |
| \`MediaPanel::Draw\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 262 | 3 |
| \`Island::DrawFill\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 261 | 1 |
| \`Island::ShowToasts\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 257 | 1 |
| \`ControlPanel::DisabledReason\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 251 | 2 |
| \`ControlPanel::LayoutSettingsWindow\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 243 | 1 |
| \`TasksPanel::Draw\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 241 | 3 |
| \`ChoiceOption\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 235 | 6 |
| \`WallpaperChangeThreadProc\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 235 | 0 |
| \`ParseIcs\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 234 | 1 |
| \`Layout\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 234 | 152 |
| \`CalendarPanel::DrawAgendas\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 233 | 1 |
| \`GlassLayer::UpdateBlurredShape\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 229 | 1 |
| \`ControlPanel::DrawAppPlaces\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 225 | 1 |
| \`Island::Create\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 213 | 4 |
| \`CommandsPanel::Draw\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 208 | 3 |
| \`UpdatesPanel::Draw\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 205 | 3 |
| \`Island::ItemTip\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 204 | 1 |
| \`Island::Destroy\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 200 | 2 |
| \`HookTaskbarDllSymbols\` | \`output/local@mac-island-dock-fork.wh.cpp\` | 193 | 1 |

Full index: \`functions.json\` and \`functions.csv\`.

## 5. Windows API and Internal-System Interaction

- API symbol occurrences matched: **2710**

| API / symbol | Occurrences |
|---|---:|
| \`ID2D1RenderTarget\` | 245 |
| \`ID2D1SolidColorBrush\` | 205 |
| \`IDWriteTextFormat\` | 185 |
| \`KillTimer\` | 110 |
| \`WaitForSingleObject\` | 107 |
| \`ID2D1Bitmap\` | 102 |
| \`PostMessage\` | 93 |
| \`SetTimer\` | 76 |
| \`IDWriteTextLayout\` | 69 |
| \`CreateThread\` | 61 |
| \`INFINITE\` | 59 |
| \`IID_PPV_ARGS\` | 56 |
| \`GetMonitorInfo\` | 48 |
| \`GetProcAddress\` | 47 |
| \`CoCreateInstance\` | 41 |
| \`INVALID_HANDLE_VALUE\` | 41 |
| \`SetEvent\` | 40 |
| \`CreateEvent\` | 39 |
| \`GetCursorPos\` | 38 |
| \`ID2D1Factory\` | 33 |
| \`IInspectable\` | 30 |
| \`IWICImagingFactory\` | 29 |
| \`IFACEMETHODIMP\` | 29 |
| \`CoInitializeEx\` | 28 |
| \`ID2D1Layer\` | 28 |
| \`IUnknown\` | 26 |
| \`GetForegroundWindow\` | 26 |
| \`PeekMessage\` | 25 |
| \`DefWindowProc\` | 25 |
| \`INVALID_FILE_ATTRIBUTES\` | 22 |
| \`GetDpiForWindow\` | 20 |
| \`GetWindowLongPtr\` | 19 |
| \`IDWriteFactory\` | 19 |
| \`WaitForMultipleObjects\` | 19 |
| \`ID2D1PathGeometry\` | 18 |
| \`INPUT\` | 17 |
| \`OpenProcess\` | 16 |
| \`INT_PTR\` | 16 |
| \`IDataObject\` | 16 |
| \`DestroyWindow\` | 15 |
| \`IWICFormatConverter\` | 15 |
| \`IShellItem\` | 14 |
| \`IMMDevice\` | 14 |
| \`ID2D1Brush\` | 13 |
| \`IAudioEndpointVolume\` | 13 |
| \`ID2D1GeometrySink\` | 12 |
| \`IWICBitmap\` | 12 |
| \`CreateWindowEx\` | 12 |
| \`CreateFile\` | 12 |
| \`IMMDeviceEnumerator\` | 12 |
| \`GetSystemMetrics\` | 10 |
| \`ID2D1LinearGradientBrush\` | 10 |
| \`MonitorFromWindow\` | 9 |
| \`SetWinEventHook\` | 9 |
| \`ID2D1Geometry\` | 9 |
| \`SetWindowLongPtr\` | 9 |
| \`IWICBitmapSource\` | 9 |
| \`IUIAutomationElement\` | 9 |
| \`CallNextHookEx\` | 8 |
| \`UnhookWinEvent\` | 8 |
| \`INPUT_KEYBOARD\` | 8 |
| \`IFACEMETHODIMP_\` | 8 |
| \`GetDpiForMonitor\` | 7 |
| \`DispatchMessage\` | 7 |
| \`IWICBitmapFrameDecode\` | 7 |
| \`IAudioMeterInformation\` | 7 |
| \`ID2D1RoundedRectangleGeometry\` | 7 |
| \`ID2D1BitmapRenderTarget\` | 7 |
| \`INT\` | 7 |
| \`ICONINFO\` | 6 |
| \`IPropertyStore\` | 6 |
| \`IWICBitmapDecoder\` | 6 |
| \`ID2D1GradientStopCollection\` | 6 |
| \`IMMDeviceCollection\` | 6 |
| \`IAudioSessionControl\` | 6 |
| \`DeviceIoControl\` | 6 |
| \`IWICBitmapScaler\` | 5 |
| \`IAudioSessionManager2\` | 5 |
| \`IMMNotificationClient\` | 5 |
| \`IDropTarget\` | 5 |
| \`IUIAutomationElementArray\` | 5 |
| \`GetMessage\` | 4 |
| \`IShellItemImageFactory\` | 4 |
| \`EnumDisplayMonitors\` | 4 |
| \`IGraphicsEffect\` | 4 |
| \`DwmSetWindowAttribute\` | 4 |
| \`IDWriteInlineObject\` | 4 |
| \`IDWriteTypography\` | 4 |
| \`ISimpleAudioVolume\` | 4 |
| \`IAudioSessionEnumerator\` | 4 |
| \`IAudioSessionControl2\` | 4 |
| \`IID_IUnknown\` | 4 |
| \`IMFSensorActivityReport\` | 4 |
| \`IMFSensorActivitiesReportCallback\` | 4 |
| \`ID2D1BitmapBrush\` | 3 |
| \`IAppResolver8\` | 3 |
| \`IDispatch\` | 3 |
| \`IWbemServices\` | 3 |
| \`IWbemClassObject\` | 3 |
| \`ID2D1DCRenderTarget\` | 3 |
| \`ICompositorDesktopInteropLocal\` | 3 |
| \`IGeometrySource2DInteropLocal\` | 3 |
| \`IGraphicsEffectD2D1InteropLocal\` | 3 |
| \`IGraphicsEffectSource\` | 3 |
| \`IStream\` | 3 |
| \`IAudioSessionNotification\` | 3 |
| \`IAudioSessionEvents\` | 3 |
| \`IMFSensorProcessActivity\` | 3 |
| \`IMFSensorActivitiesReport\` | 3 |
| \`IMFSensorActivityMonitor\` | 3 |
| \`SHGetKnownFolderPath\` | 3 |
| \`ID2D1RadialGradientBrush\` | 3 |
| \`IF_TYPE_ETHERNET_CSMACD\` | 3 |
| \`INetworkListManagerEvents\` | 3 |
| \`IUIAutomation\` | 3 |
| \`ID2D1DeviceContext\` | 2 |
| \`SetWindowsHookEx\` | 2 |
| \`SendMessage\` | 2 |
| \`UnhookWindowsHookEx\` | 2 |
| \`IPersistFile\` | 2 |
| \`IQuietHoursSettings\` | 2 |
| \`ID2D1HwndRenderTarget\` | 2 |
| \`ICON_BIG\` | 2 |
| \`IWICStream\` | 2 |
| \`IPolicyConfig\` | 2 |
| \`IRadioManager\` | 2 |
| \`IKsControlProperty\` | 2 |
| \`IDeviceTopology\` | 2 |
| \`IConnector\` | 2 |
| \`IDXGIFactory1\` | 2 |
| \`IFileOpenDialog\` | 2 |
| \`IME_CMODE_NATIVE\` | 2 |
| \`INT_MAX\` | 2 |
| \`SHGetFileInfo\` | 2 |
| \`IFileDialog\` | 2 |
| \`IDesktopWallpaper\` | 2 |
| \`INotificationActivationCallback\` | 2 |
| \`IAudioEndpointVolumeCallback\` | 2 |
| \`IF_TYPE_IEEE80211\` | 2 |
| \`IUIAutomationCondition\` | 2 |
| \`IMAGE_DOS_HEADER\` | 2 |
| \`IMAGE_NT_HEADERS\` | 2 |
| \`INT_MIN\` | 1 |
| \`ID2D1BitmapBrush1\` | 1 |
| \`IShellLinkW\` | 1 |
| \`IShellWindows\` | 1 |
| \`IShellBrowser\` | 1 |
| \`IShellView\` | 1 |
| \`IServiceProvider\` | 1 |
| \`IShellFolderViewDual\` | 1 |

Every occurrence with file and line is in \`windows-api-usage.csv\`. Matching is pattern-based; it does not prove that every call is a direct Windows API call.

## 6. Include and Dependency Inventory

| Include | Occurrences |
|---|---:|
| \`windhawk_utils.h\` | 1 |
| \`appmodel.h\` | 1 |
| \`aclapi.h\` | 1 |
| \`audiopolicy.h\` | 1 |
| \`bluetoothapis.h\` | 1 |
| \`cfgmgr32.h\` | 1 |
| \`commdlg.h\` | 1 |
| \`d2d1.h\` | 1 |
| \`d2d1_1.h\` | 1 |
| \`d2d1_3.h\` | 1 |
| \`dbt.h\` | 1 |
| \`devicetopology.h\` | 1 |
| \`dwmapi.h\` | 1 |
| \`dwrite.h\` | 1 |
| \`endpointvolume.h\` | 1 |
| \`exdisp.h\` | 1 |
| \`highlevelmonitorconfigurationapi.h\` | 1 |
| \`imm.h\` | 1 |
| \`mmdeviceapi.h\` | 1 |
| \`mmsystem.h\` | 1 |
| \`msctf.h\` | 1 |
| \`netlistmgr.h\` | 1 |
| \`winsock2.h\` | 1 |
| \`ws2ipdef.h\` | 1 |
| \`iphlpapi.h\` | 1 |
| \`dxgi.h\` | 1 |
| \`pdh.h\` | 1 |
| \`psapi.h\` | 1 |
| \`physicalmonitorenumerationapi.h\` | 1 |
| \`propsys.h\` | 1 |
| \`wbemidl.h\` | 1 |
| \`wincodec.h\` | 1 |
| \`winhttp.h\` | 1 |
| \`winioctl.h\` | 1 |
| \`wlanapi.h\` | 1 |
| \`setupapi.h\` | 1 |
| \`shellapi.h\` | 1 |
| \`sddl.h\` | 1 |
| \`shellscalingapi.h\` | 1 |
| \`shldisp.h\` | 1 |
| \`shlobj.h\` | 1 |
| \`shlwapi.h\` | 1 |
| \`shobjidl.h\` | 1 |
| \`tlhelp32.h\` | 1 |
| \`userenv.h\` | 1 |
| \`uiautomation.h\` | 1 |
| \`wuapi.h\` | 1 |
| \`dispatcherqueue.h\` | 1 |
| \`algorithm\` | 1 |
| \`array\` | 1 |
| \`atomic\` | 1 |
| \`chrono\` | 1 |
| \`climits\` | 1 |
| \`cmath\` | 1 |
| \`cstdio\` | 1 |
| \`cwctype\` | 1 |
| \`functional\` | 1 |
| \`limits\` | 1 |
| \`memory\` | 1 |
| \`optional\` | 1 |
| \`string\` | 1 |
| \`string_view\` | 1 |
| \`unordered_map\` | 1 |
| \`utility\` | 1 |
| \`vector\` | 1 |
| \`winrt/Windows.Devices.Geolocation.h\` | 1 |
| \`winrt/Windows.Devices.Radios.h\` | 1 |
| \`winrt/Windows.Foundation.Collections.h\` | 1 |
| \`winrt/Windows.Foundation.h\` | 1 |
| \`winrt/Windows.Graphics.Effects.h\` | 1 |
| \`winrt/Windows.Graphics.h\` | 1 |
| \`winrt/Windows.System.h\` | 1 |
| \`winrt/Windows.UI.Composition.Desktop.h\` | 1 |
| \`winrt/Windows.Media.Control.h\` | 1 |
| \`winrt/Windows.Media.Core.h\` | 1 |
| \`winrt/Windows.UI.Notifications.h\` | 1 |
| \`winrt/Windows.Media.Playback.h\` | 1 |
| \`winrt/Windows.Networking.Connectivity.h\` | 1 |
| \`winrt/Windows.Networking.NetworkOperators.h\` | 1 |
| \`winrt/Windows.Storage.Streams.h\` | 1 |
| \`winrt/Windows.UI.Composition.h\` | 1 |
| \`winrt/Windows.UI.Input.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Automation.Peers.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Automation.Provider.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Automation.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Controls.Primitives.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Controls.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Data.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Hosting.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Input.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Markup.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Media.Imaging.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Media.h\` | 1 |
| \`winrt/Windows.UI.Xaml.Shapes.h\` | 1 |
| \`winrt/Windows.UI.Xaml.h\` | 1 |
| \`winrt/Windows.UI.h\` | 1 |

Full include list: \`includes.csv\`. Local/external classification is approximate.

## 7. Heuristic Internal Call Graph

Use \`functions.json\` and the \`calls_defined_in_repo\` field to inspect likely internal call relationships. Name-based matching can overcount overloaded functions or miss indirect calls, callbacks, function pointers, macros, and virtual dispatch.

## 8. Recommended AI-Agent Reading Order

1. Read this report and \`repository-files.csv\` to understand the repository shape.
2. Read the mod metadata and settings block, then trace the primary initialization and teardown functions.
3. Follow the entry-point call graph into window creation, event/message dispatch, hooks, timers, worker threads, and cleanup.
4. For a feature task, use \`functions.csv\` to locate relevant functions and inspect only those functions plus their callers/callees.
5. Use \`windows-api-usage.csv\` to find system-boundary code and \`includes.csv\` to identify dependencies.
6. Before edits, ask the agent to state the exact files/functions it intends to change and the expected lifecycle impact.

## Output Files

- \`README-ANALYSIS.md\` — this report
- \`repository-files.csv\` — file inventory and line counts
- \`functions.csv\` / \`functions.json\` — function index and heuristic call relationships
- \`windhawk-settings.csv\` — detected Windhawk settings
- \`variable-candidates.csv\` — global/declaration candidates
- \`windows-api-usage.csv\` — matched Windows/system API occurrences
- \`lifecycle-symbols.csv\` — lifecycle/event-related symbol occurrences
- \`includes.csv\` — include/dependency inventory
- \`analysis-summary.json\` — machine-readable summary
