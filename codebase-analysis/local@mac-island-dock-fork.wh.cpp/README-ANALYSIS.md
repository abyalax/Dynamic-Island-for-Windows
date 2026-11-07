# Codebase Analysis Report

- Root: `C:\Users\abya\c\Dynamic-Island-for-Windows`
- Generated: 2026-11-07T13:08:12+07:00
- Files analyzed: **1**
- Total physical lines: **109,420**
- Nonblank lines: **106,048**
- Source files (`.c/.cpp/.h` family): **1**
- Function definitions detected (heuristic): **2893**

> Static-analysis note: function, variable, API, and call-graph results are heuristic inventories, not compiler AST output. Validate ambiguous matches against source context.

## 1. Repository Structure

| Extension | Files |
| --------- | ----: |
| `.cpp`    |     1 |

### Largest files

| File                                    |   Lines | Size (bytes) |
| --------------------------------------- | ------: | -----------: |
| `src/local@mac-island-dock-fork.wh.cpp` | 109,420 |    5,099,470 |

## 2. Entry Points and Lifecycle Candidates

| Symbol                  | File                                    |   Line |
| ----------------------- | --------------------------------------- | -----: |
| `Wh_ModInit`            | `src/local@mac-island-dock-fork.wh.cpp` | 109230 |
| `Wh_ModAfterInit`       | `src/local@mac-island-dock-fork.wh.cpp` | 109320 |
| `Wh_ModBeforeUninit`    | `src/local@mac-island-dock-fork.wh.cpp` | 109388 |
| `Wh_ModSettingsChanged` | `src/local@mac-island-dock-fork.wh.cpp` | 109394 |
| `Wh_ModUninit`          | `src/local@mac-island-dock-fork.wh.cpp` | 109408 |

### Lifecycle-related symbol occurrences

| Symbol | Occurrences |
| ------ | ----------: |

## 3. Configurable Settings and Variable Candidates

- Windhawk setting candidates: **172**
- Global/declaration candidates: **0**

### Windhawk settings detected

| Setting              | Default                                                                                           | File                                    | Line |
| -------------------- | ------------------------------------------------------------------------------------------------- | --------------------------------------- | ---- |
| `taskbarMode`        | `windows`                                                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 1531 |
| `dock`               | `"Dock: compact and floating, see-through, like on macOS"`                                        | `src/local@mac-island-dock-fork.wh.cpp` | 1549 |
| `windowsTransparent` | `"Windows taskbar, transparent: only the apps, with a soft shadow, hiding itself"`                | `src/local@mac-island-dock-fork.wh.cpp` | 1550 |
| `windowsApps`        | `"Windows taskbar, only the apps (the island shows the rest)"`                                    | `src/local@mac-island-dock-fork.wh.cpp` | 1551 |
| `windows`            | `"Windows taskbar, unchanged (to use other taskbar mods)"`                                        | `src/local@mac-island-dock-fork.wh.cpp` | 1552 |
| `dock`               | `"Dock: compacta e flutuante, transparente, como no macOS"`                                       | `src/local@mac-island-dock-fork.wh.cpp` | 1554 |
| `windowsTransparent` | `"Barra do Windows transparente: só os apps, com uma sombra suave, se escondendo sozinha"`        | `src/local@mac-island-dock-fork.wh.cpp` | 1555 |
| `windowsApps`        | `"Barra do Windows só com os apps (a ilha mostra o resto)"`                                       | `src/local@mac-island-dock-fork.wh.cpp` | 1556 |
| `windows`            | `"Barra do Windows sem mudanças (para usar outros mods de barra)"`                                | `src/local@mac-island-dock-fork.wh.cpp` | 1557 |
| `dock`               | `"程序坞：紧凑、悬浮、半透明，像 macOS 一样"`                                                     | `src/local@mac-island-dock-fork.wh.cpp` | 1559 |
| `windowsTransparent` | `"Windows 任务栏，透明：只显示应用，带柔和阴影，会自动隐藏"`                                      | `src/local@mac-island-dock-fork.wh.cpp` | 1560 |
| `windowsApps`        | `"Windows 任务栏，只显示应用（其余由灵动岛显示）"`                                                | `src/local@mac-island-dock-fork.wh.cpp` | 1561 |
| `windows`            | `"Windows 任务栏，不做更改（可与其他任务栏 mod 一起使用）"`                                       | `src/local@mac-island-dock-fork.wh.cpp` | 1562 |
| `dock`               | `"Dock: kompakt und schwebend, durchsichtig, wie unter macOS"`                                    | `src/local@mac-island-dock-fork.wh.cpp` | 1564 |
| `windowsTransparent` | `"Windows-Taskleiste, transparent: nur die Apps, mit weichem Schatten, blendet sich selbst aus"`  | `src/local@mac-island-dock-fork.wh.cpp` | 1565 |
| `windowsApps`        | `"Windows-Taskleiste, nur die Apps (die Insel zeigt den Rest)"`                                   | `src/local@mac-island-dock-fork.wh.cpp` | 1566 |
| `windows`            | `"Windows-Taskleiste, unverändert (für die Nutzung anderer Taskleisten-Mods)"`                    | `src/local@mac-island-dock-fork.wh.cpp` | 1567 |
| `dock`               | `"Dock: compacto y flotante, transparente, como en macOS"`                                        | `src/local@mac-island-dock-fork.wh.cpp` | 1569 |
| `windowsTransparent` | `"Barra de tareas de Windows, transparente: solo las apps, con una sombra suave, se oculta sola"` | `src/local@mac-island-dock-fork.wh.cpp` | 1570 |
| `windowsApps`        | `"Barra de tareas de Windows, solo las apps (la isla muestra el resto)"`                          | `src/local@mac-island-dock-fork.wh.cpp` | 1571 |
| `windows`            | `"Barra de tareas de Windows, sin cambios (para usar otros mods de la barra de tareas)"`          | `src/local@mac-island-dock-fork.wh.cpp` | 1572 |
| `dock`               | `"Док: компактный и плавающий, прозрачный, как в macOS"`                                          | `src/local@mac-island-dock-fork.wh.cpp` | 1574 |
| `windowsTransparent` | `"Панель задач Windows, прозрачная: только приложения, с мягкой тенью, скрывается сама"`          | `src/local@mac-island-dock-fork.wh.cpp` | 1575 |
| `windowsApps`        | `"Панель задач Windows, только приложения (остальное показывает остров)"`                         | `src/local@mac-island-dock-fork.wh.cpp` | 1576 |
| `windows`            | `"Панель задач Windows, без изменений (для других модов панели задач)"`                           | `src/local@mac-island-dock-fork.wh.cpp` | 1577 |
| `dock`               | `"Dock: コンパクトで浮かぶ透明なバー（macOS のように）"`                                          | `src/local@mac-island-dock-fork.wh.cpp` | 1579 |
| `windowsTransparent` | `"Windows タスクバー、透明: アプリのみ、やわらかい影付きで自動的に隠れる"`                        | `src/local@mac-island-dock-fork.wh.cpp` | 1580 |
| `windowsApps`        | `"Windows タスクバー、アプリのみ（その他はアイランドが表示）"`                                    | `src/local@mac-island-dock-fork.wh.cpp` | 1581 |
| `windows`            | `"Windows タスクバー、変更なし（他のタスクバー Mod 用）"`                                         | `src/local@mac-island-dock-fork.wh.cpp` | 1582 |
| `dock`               | `"Dock: nhỏ gọn, nổi và trong suốt giống macOS"`                                                  | `src/local@mac-island-dock-fork.wh.cpp` | 1584 |
| `windowsTransparent` | `"Taskbar Windows trong suốt: chỉ hiện icon ứng dụng, có đổ bóng nhẹ và tự ẩn"`                   | `src/local@mac-island-dock-fork.wh.cpp` | 1585 |
| `windowsApps`        | `"Taskbar Windows chỉ hiện ứng dụng (đồng hồ và khay hệ thống chuyển lên đảo)"`                   | `src/local@mac-island-dock-fork.wh.cpp` | 1586 |
| `windows`            | `"Taskbar Windows mặc định, giữ nguyên (để dùng chung với các mod taskbar khác)"`                 | `src/local@mac-island-dock-fork.wh.cpp` | 1587 |
| `windowsHideTray`    | `false`                                                                                           | `src/local@mac-island-dock-fork.wh.cpp` | 1588 |
| `showIsland`         | `true`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 1605 |
| `calendarIcs`        | `[""]`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 1622 |
| `hiding`             | ``                                                                                                | `src/local@mac-island-dock-fork.wh.cpp` | 1639 |
| `autoHide`           | `false`                                                                                           | `src/local@mac-island-dock-fork.wh.cpp` | 1640 |
| `hideWhen`           | `mouseAway`                                                                                       | `src/local@mac-island-dock-fork.wh.cpp` | 1657 |
| `mouseAway`          | `Mouse away`                                                                                      | `src/local@mac-island-dock-fork.wh.cpp` | 1675 |
| `maximized`          | `Window maximized`                                                                                | `src/local@mac-island-dock-fork.wh.cpp` | 1676 |
| `windowOver`         | `Window over it`                                                                                  | `src/local@mac-island-dock-fork.wh.cpp` | 1677 |
| `mouseAway`          | `Mouse longe`                                                                                     | `src/local@mac-island-dock-fork.wh.cpp` | 1679 |
| `maximized`          | `Janela maximizada`                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 1680 |
| `windowOver`         | `Janela por cima`                                                                                 | `src/local@mac-island-dock-fork.wh.cpp` | 1681 |
| `mouseAway`          | `鼠标离开`                                                                                        | `src/local@mac-island-dock-fork.wh.cpp` | 1683 |
| `maximized`          | `窗口最大化`                                                                                      | `src/local@mac-island-dock-fork.wh.cpp` | 1684 |
| `windowOver`         | `窗口挡住它`                                                                                      | `src/local@mac-island-dock-fork.wh.cpp` | 1685 |
| `mouseAway`          | `Maus weg`                                                                                        | `src/local@mac-island-dock-fork.wh.cpp` | 1687 |
| `maximized`          | `Fenster maximiert`                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 1688 |
| `windowOver`         | `Fenster darüber`                                                                                 | `src/local@mac-island-dock-fork.wh.cpp` | 1689 |
| `mouseAway`          | `Ratón lejos`                                                                                     | `src/local@mac-island-dock-fork.wh.cpp` | 1691 |
| `maximized`          | `Ventana maximizada`                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 1692 |
| `windowOver`         | `Ventana encima`                                                                                  | `src/local@mac-island-dock-fork.wh.cpp` | 1693 |
| `mouseAway`          | `Мышь далеко`                                                                                     | `src/local@mac-island-dock-fork.wh.cpp` | 1695 |
| `maximized`          | `Окно развёрнуто`                                                                                 | `src/local@mac-island-dock-fork.wh.cpp` | 1696 |
| `windowOver`         | `Окно над ней`                                                                                    | `src/local@mac-island-dock-fork.wh.cpp` | 1697 |
| `mouseAway`          | `マウスが離れたとき`                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 1699 |
| `maximized`          | `ウィンドウの最大化`                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 1700 |
| `windowOver`         | `ウィンドウが重なったとき`                                                                        | `src/local@mac-island-dock-fork.wh.cpp` | 1701 |
| `mouseAway`          | `Chuột rời đi`                                                                                    | `src/local@mac-island-dock-fork.wh.cpp` | 1703 |
| `maximized`          | `Cửa sổ phóng to`                                                                                 | `src/local@mac-island-dock-fork.wh.cpp` | 1704 |
| `windowOver`         | `Cửa sổ đè lên`                                                                                   | `src/local@mac-island-dock-fork.wh.cpp` | 1705 |
| `showDelay`          | `0`                                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 1706 |
| `revealHeight`       | `0`                                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 1723 |
| `hideDelay`          | `400`                                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 1740 |
| `showDuration`       | `260`                                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 1757 |
| `hideDuration`       | `220`                                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 1774 |
| `fullscreenGuard`    | `true`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 1791 |
| `icons`              | ``                                                                                                | `src/local@mac-island-dock-fork.wh.cpp` | 1824 |
| `magnification`      | `170`                                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 1825 |
| `magnificationRange` | `150`                                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 1842 |
| `onlyApps`           | `false`                                                                                           | `src/local@mac-island-dock-fork.wh.cpp` | 1859 |
| `bounceOnLaunch`     | `true`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 1876 |
| `keepOpenOnLaunch`   | `true`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 1893 |
| `keepOpenDuration`   | `1500`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 1910 |
| `dock`               | ``                                                                                                | `src/local@mac-island-dock-fork.wh.cpp` | 1943 |
| `iconSize`           | `32`                                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 1944 |
| `iconGap`            | `8`                                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 1961 |
| `bottomGap`          | `2`                                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 1978 |
| `cornerRadius`       | `22`                                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 1995 |
| `maxWidth`           | `0`                                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 2012 |
| `background`         | `true`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2029 |
| `backgroundOpacity`  | `50`                                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 2046 |
| `dotIndicator`       | `true`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2063 |
| `attentionMode`      | `icon`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2080 |
| `icon`               | `Only the app's icon peeks out, bouncing`                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 2098 |
| `dock`               | `The whole dock shows up`                                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 2099 |
| `none`               | `Nothing`                                                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 2100 |
| `icon`               | `Só o ícone do app aparece, pulando`                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 2102 |
| `dock`               | `A dock inteira aparece`                                                                          | `src/local@mac-island-dock-fork.wh.cpp` | 2103 |
| `none`               | `Nada`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2104 |
| `icon`               | `只有应用图标探出并跳动`                                                                          | `src/local@mac-island-dock-fork.wh.cpp` | 2106 |
| `dock`               | `整个程序坞显示出来`                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 2107 |
| `none`               | `什么都不做`                                                                                      | `src/local@mac-island-dock-fork.wh.cpp` | 2108 |
| `icon`               | `Nur das App-Symbol schaut hüpfend hervor`                                                        | `src/local@mac-island-dock-fork.wh.cpp` | 2110 |
| `dock`               | `Das ganze Dock erscheint`                                                                        | `src/local@mac-island-dock-fork.wh.cpp` | 2111 |
| `none`               | `Nichts`                                                                                          | `src/local@mac-island-dock-fork.wh.cpp` | 2112 |
| `icon`               | `Solo asoma el icono de la app, rebotando`                                                        | `src/local@mac-island-dock-fork.wh.cpp` | 2114 |
| `dock`               | `Aparece todo el dock`                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2115 |
| `none`               | `Nada`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2116 |
| `icon`               | `Выглядывает только значок приложения, подпрыгивая`                                               | `src/local@mac-island-dock-fork.wh.cpp` | 2118 |
| `dock`               | `Появляется весь док`                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2119 |
| `none`               | `Ничего`                                                                                          | `src/local@mac-island-dock-fork.wh.cpp` | 2120 |
| `icon`               | `アプリのアイコンだけがバウンスしながらのぞく`                                                    | `src/local@mac-island-dock-fork.wh.cpp` | 2122 |
| `dock`               | `Dock 全体が表示される`                                                                           | `src/local@mac-island-dock-fork.wh.cpp` | 2123 |
| `none`               | `何もしない`                                                                                      | `src/local@mac-island-dock-fork.wh.cpp` | 2124 |
| `icon`               | `Chỉ nhô riêng icon của ứng dụng đó lên và nảy nhẹ`                                               | `src/local@mac-island-dock-fork.wh.cpp` | 2126 |
| `dock`               | `Trượt cả thanh Dock lên`                                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 2127 |
| `none`               | `Không làm gì`                                                                                    | `src/local@mac-island-dock-fork.wh.cpp` | 2128 |
| `hideTray`           | `false`                                                                                           | `src/local@mac-island-dock-fork.wh.cpp` | 2129 |
| `animations`         | ``                                                                                                | `src/local@mac-island-dock-fork.wh.cpp` | 2162 |
| `enabled`            | `true`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2163 |
| `level`              | `all`                                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2180 |
| `all`                | `All`                                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2198 |
| `partial`            | `"Partial: no bounces, glows, color blends, counting numbers nor scrolling text"`                 | `src/local@mac-island-dock-fork.wh.cpp` | 2199 |
| `none`               | `None`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2200 |
| `all`                | `Todas`                                                                                           | `src/local@mac-island-dock-fork.wh.cpp` | 2202 |
| `partial`            | `"Parciais: sem quiques, brilhos, mistura de cores, números contando nem texto rolando"`          | `src/local@mac-island-dock-fork.wh.cpp` | 2203 |
| `none`               | `Nenhuma`                                                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 2204 |
| `all`                | `全部`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2206 |
| `partial`            | `"部分：没有弹跳、发光、颜色渐变、数字滚动和文字滚动"`                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2207 |
| `none`               | `无`                                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 2208 |
| `all`                | `Alle`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2210 |
| `partial`            | `"Teilweise: kein Hüpfen, Leuchten, Farbübergänge, Zählzahlen oder Lauftext"`                     | `src/local@mac-island-dock-fork.wh.cpp` | 2211 |
| `none`               | `Keine`                                                                                           | `src/local@mac-island-dock-fork.wh.cpp` | 2212 |
| `all`                | `Todas`                                                                                           | `src/local@mac-island-dock-fork.wh.cpp` | 2214 |
| `partial`            | `"Parcial: sin rebotes, brillos, mezclas de color, números que cuentan ni texto que se desplaza"` | `src/local@mac-island-dock-fork.wh.cpp` | 2215 |
| `none`               | `Ninguna`                                                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 2216 |
| `all`                | `Все`                                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2218 |
| `partial`            | `"Частично: без подпрыгиваний, свечения, смешения цветов, счёта чисел и прокрутки текста"`        | `src/local@mac-island-dock-fork.wh.cpp` | 2219 |
| `none`               | `Нет`                                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2220 |
| `all`                | `すべて`                                                                                          | `src/local@mac-island-dock-fork.wh.cpp` | 2222 |
| `partial`            | `"一部: バウンス、グロー、色のブレンド、数字のカウント、テキストのスクロールなし"`                | `src/local@mac-island-dock-fork.wh.cpp` | 2223 |
| `none`               | `なし`                                                                                            | `src/local@mac-island-dock-fork.wh.cpp` | 2224 |
| `all`                | `Đầy đủ`                                                                                          | `src/local@mac-island-dock-fork.wh.cpp` | 2226 |
| `partial`            | `"Cơ bản: tắt hiệu ứng nảy, phát sáng, chuyển màu, nhảy số và chạy chữ"`                          | `src/local@mac-island-dock-fork.wh.cpp` | 2227 |
| `none`               | `Tắt hết`                                                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 2228 |
| `speed`              | `125`                                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2229 |
| `frameRate`          | `screen`                                                                                          | `src/local@mac-island-dock-fork.wh.cpp` | 2246 |
| `screen`             | `The screen's rate`                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 2264 |
| `fps90`              | `About 90 a second`                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 2265 |
| `fps60`              | `About 60 a second`                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 2266 |
| `fps30`              | `About 30 a second`                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 2267 |
| `screen`             | `A taxa da tela`                                                                                  | `src/local@mac-island-dock-fork.wh.cpp` | 2269 |
| `fps90`              | `Cerca de 90 por segundo`                                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 2270 |
| `fps60`              | `Cerca de 60 por segundo`                                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 2271 |
| `fps30`              | `Cerca de 30 por segundo`                                                                         | `src/local@mac-island-dock-fork.wh.cpp` | 2272 |
| `screen`             | `屏幕刷新率`                                                                                      | `src/local@mac-island-dock-fork.wh.cpp` | 2274 |
| `fps90`              | `约每秒 90 帧`                                                                                    | `src/local@mac-island-dock-fork.wh.cpp` | 2275 |
| `fps60`              | `约每秒 60 帧`                                                                                    | `src/local@mac-island-dock-fork.wh.cpp` | 2276 |
| `fps30`              | `约每秒 30 帧`                                                                                    | `src/local@mac-island-dock-fork.wh.cpp` | 2277 |
| `screen`             | `Die Rate des Bildschirms`                                                                        | `src/local@mac-island-dock-fork.wh.cpp` | 2279 |
| `fps90`              | `Etwa 90 pro Sekunde`                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2280 |
| `fps60`              | `Etwa 60 pro Sekunde`                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2281 |
| `fps30`              | `Etwa 30 pro Sekunde`                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2282 |
| `screen`             | `La tasa de la pantalla`                                                                          | `src/local@mac-island-dock-fork.wh.cpp` | 2284 |
| `fps90`              | `Unos 90 por segundo`                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2285 |
| `fps60`              | `Unos 60 por segundo`                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2286 |
| `fps30`              | `Unos 30 por segundo`                                                                             | `src/local@mac-island-dock-fork.wh.cpp` | 2287 |
| `screen`             | `Частота экрана`                                                                                  | `src/local@mac-island-dock-fork.wh.cpp` | 2289 |
| `fps90`              | `Около 90 в секунду`                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 2290 |
| `fps60`              | `Около 60 в секунду`                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 2291 |
| `fps30`              | `Около 30 в секунду`                                                                              | `src/local@mac-island-dock-fork.wh.cpp` | 2292 |
| `screen`             | `画面のリフレッシュ レート`                                                                       | `src/local@mac-island-dock-fork.wh.cpp` | 2294 |
| `fps90`              | `約 90 フレーム/秒`                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 2295 |
| `fps60`              | `約 60 フレーム/秒`                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 2296 |
| `fps30`              | `約 30 フレーム/秒`                                                                               | `src/local@mac-island-dock-fork.wh.cpp` | 2297 |
| `screen`             | `Theo tần số quét màn hình`                                                                       | `src/local@mac-island-dock-fork.wh.cpp` | 2299 |
| `fps90`              | `Khoảng 90 FPS`                                                                                   | `src/local@mac-island-dock-fork.wh.cpp` | 2300 |
| `fps60`              | `Khoảng 60 FPS`                                                                                   | `src/local@mac-island-dock-fork.wh.cpp` | 2301 |
| `fps30`              | `Khoảng 30 FPS`                                                                                   | `src/local@mac-island-dock-fork.wh.cpp` | 2302 |

Variable candidates are exported to `variable-candidates.csv`; inspect each declaration before classifying it as global/configuration/state.

## 4. Function Inventory

Function definitions detected: **2893**

### Largest functions by estimated body span

| Function                             | File                                    | Lines | Incoming calls (heuristic) |
| ------------------------------------ | --------------------------------------- | ----: | -------------------------: |
| `Island::Render`                     | `src/local@mac-island-dock-fork.wh.cpp` |  1863 |                         68 |
| `Island::Layout`                     | `src/local@mac-island-dock-fork.wh.cpp` |  1617 |                        152 |
| `Island::HandleMessage`              | `src/local@mac-island-dock-fork.wh.cpp` |  1230 |                          3 |
| `ControlPanel::DrawSettingEntry`     | `src/local@mac-island-dock-fork.wh.cpp` |  1126 |                          1 |
| `ControlPanel::OnMouseUp`            | `src/local@mac-island-dock-fork.wh.cpp` |  1018 |                          1 |
| `ControlPanel::SettingDescription`   | `src/local@mac-island-dock-fork.wh.cpp` |   793 |                          2 |
| `FoldersPanel::Draw`                 | `src/local@mac-island-dock-fork.wh.cpp` |   702 |                          3 |
| `ControlPanel::Layout`               | `src/local@mac-island-dock-fork.wh.cpp` |   677 |                        152 |
| `OnRenderingUnsafe`                  | `src/local@mac-island-dock-fork.wh.cpp` |   584 |                          1 |
| `SetIslandPref`                      | `src/local@mac-island-dock-fork.wh.cpp` |   561 |                         15 |
| `ControlPanel::HitTest`              | `src/local@mac-island-dock-fork.wh.cpp` |   505 |                         62 |
| `SetIslandPrefValue`                 | `src/local@mac-island-dock-fork.wh.cpp` |   471 |                          3 |
| `Island::AnimationFrame`             | `src/local@mac-island-dock-fork.wh.cpp` |   464 |                          1 |
| `CalendarPanel::DrawCalendar`        | `src/local@mac-island-dock-fork.wh.cpp` |   459 |                          1 |
| `NotificationPanel::Draw`            | `src/local@mac-island-dock-fork.wh.cpp` |   454 |                          3 |
| `Island::OnClick`                    | `src/local@mac-island-dock-fork.wh.cpp` |   423 |                          2 |
| `LoadSettings`                       | `src/local@mac-island-dock-fork.wh.cpp` |   421 |                          6 |
| `ControlPanel::DrawControl`          | `src/local@mac-island-dock-fork.wh.cpp` |   375 |                          2 |
| `ControlPanel::LayoutSettings`       | `src/local@mac-island-dock-fork.wh.cpp` |   356 |                          1 |
| `ControlPanel::PlaceSettingParts`    | `src/local@mac-island-dock-fork.wh.cpp` |   344 |                          1 |
| `MediaThreadProc`                    | `src/local@mac-island-dock-fork.wh.cpp` |   333 |                          0 |
| `Island::DrawBannerContents`         | `src/local@mac-island-dock-fork.wh.cpp` |   333 |                          2 |
| `TaskbarSubclassProc`                | `src/local@mac-island-dock-fork.wh.cpp` |   322 |                          0 |
| `FoldersPanel::OnMouseUp`            | `src/local@mac-island-dock-fork.wh.cpp` |   316 |                          1 |
| `GetIslandPref`                      | `src/local@mac-island-dock-fork.wh.cpp` |   316 |                         25 |
| `ControlPanel::SettingName`          | `src/local@mac-island-dock-fork.wh.cpp` |   311 |                          4 |
| `WeatherPanel::Draw`                 | `src/local@mac-island-dock-fork.wh.cpp` |   283 |                          3 |
| `ControlPanel::OnAppMessage`         | `src/local@mac-island-dock-fork.wh.cpp` |   279 |                          1 |
| `CalendarPanel::DrawBigDay`          | `src/local@mac-island-dock-fork.wh.cpp` |   273 |                          1 |
| `Panel::HandleMessage`               | `src/local@mac-island-dock-fork.wh.cpp` |   268 |                          3 |
| `MediaPanel::Draw`                   | `src/local@mac-island-dock-fork.wh.cpp` |   262 |                          3 |
| `ControlPanel::SettingText`          | `src/local@mac-island-dock-fork.wh.cpp` |   261 |                          1 |
| `Island::DrawFill`                   | `src/local@mac-island-dock-fork.wh.cpp` |   261 |                          1 |
| `Island::ShowToasts`                 | `src/local@mac-island-dock-fork.wh.cpp` |   257 |                          1 |
| `ControlPanel::DisabledReason`       | `src/local@mac-island-dock-fork.wh.cpp` |   248 |                          2 |
| `ControlPanel::LayoutSettingsWindow` | `src/local@mac-island-dock-fork.wh.cpp` |   243 |                          1 |
| `TasksPanel::Draw`                   | `src/local@mac-island-dock-fork.wh.cpp` |   241 |                          3 |
| `WallpaperChangeThreadProc`          | `src/local@mac-island-dock-fork.wh.cpp` |   235 |                          0 |
| `ParseIcs`                           | `src/local@mac-island-dock-fork.wh.cpp` |   234 |                          1 |
| `Layout`                             | `src/local@mac-island-dock-fork.wh.cpp` |   234 |                        152 |
| `CalendarPanel::DrawAgendas`         | `src/local@mac-island-dock-fork.wh.cpp` |   233 |                          1 |
| `GlassLayer::UpdateBlurredShape`     | `src/local@mac-island-dock-fork.wh.cpp` |   229 |                          1 |
| `ControlPanel::DrawAppPlaces`        | `src/local@mac-island-dock-fork.wh.cpp` |   225 |                          1 |
| `ChoiceOption`                       | `src/local@mac-island-dock-fork.wh.cpp` |   222 |                          6 |
| `Island::Create`                     | `src/local@mac-island-dock-fork.wh.cpp` |   213 |                          4 |
| `CommandsPanel::Draw`                | `src/local@mac-island-dock-fork.wh.cpp` |   208 |                          3 |
| `UpdatesPanel::Draw`                 | `src/local@mac-island-dock-fork.wh.cpp` |   205 |                          3 |
| `Island::ItemTip`                    | `src/local@mac-island-dock-fork.wh.cpp` |   200 |                          1 |
| `Island::Destroy`                    | `src/local@mac-island-dock-fork.wh.cpp` |   199 |                          2 |
| `HookTaskbarDllSymbols`              | `src/local@mac-island-dock-fork.wh.cpp` |   193 |                          1 |

Full index: `functions.json` and `functions.csv`.

## 5. Windows API and Internal-System Interaction

- API symbol occurrences matched: **2698**

| API / symbol                        | Occurrences |
| ----------------------------------- | ----------: |
| `ID2D1RenderTarget`                 |         244 |
| `ID2D1SolidColorBrush`              |         204 |
| `IDWriteTextFormat`                 |         185 |
| `KillTimer`                         |         108 |
| `WaitForSingleObject`               |         107 |
| `ID2D1Bitmap`                       |         102 |
| `PostMessage`                       |          93 |
| `SetTimer`                          |          75 |
| `IDWriteTextLayout`                 |          69 |
| `CreateThread`                      |          61 |
| `INFINITE`                          |          59 |
| `IID_PPV_ARGS`                      |          56 |
| `GetMonitorInfo`                    |          48 |
| `GetProcAddress`                    |          47 |
| `CoCreateInstance`                  |          41 |
| `INVALID_HANDLE_VALUE`              |          41 |
| `SetEvent`                          |          40 |
| `CreateEvent`                       |          39 |
| `GetCursorPos`                      |          36 |
| `ID2D1Factory`                      |          31 |
| `IInspectable`                      |          30 |
| `IWICImagingFactory`                |          29 |
| `IFACEMETHODIMP`                    |          29 |
| `CoInitializeEx`                    |          28 |
| `ID2D1Layer`                        |          28 |
| `GetForegroundWindow`               |          26 |
| `IUnknown`                          |          26 |
| `PeekMessage`                       |          25 |
| `DefWindowProc`                     |          25 |
| `INVALID_FILE_ATTRIBUTES`           |          22 |
| `GetDpiForWindow`                   |          20 |
| `GetWindowLongPtr`                  |          19 |
| `WaitForMultipleObjects`            |          19 |
| `IDWriteFactory`                    |          19 |
| `INPUT`                             |          17 |
| `OpenProcess`                       |          16 |
| `ID2D1PathGeometry`                 |          16 |
| `INT_PTR`                           |          16 |
| `IDataObject`                       |          16 |
| `DestroyWindow`                     |          15 |
| `IWICFormatConverter`               |          15 |
| `IShellItem`                        |          14 |
| `IMMDevice`                         |          14 |
| `ID2D1Brush`                        |          13 |
| `IAudioEndpointVolume`              |          13 |
| `CreateWindowEx`                    |          12 |
| `IWICBitmap`                        |          12 |
| `CreateFile`                        |          12 |
| `IMMDeviceEnumerator`               |          12 |
| `ID2D1GeometrySink`                 |          11 |
| `GetSystemMetrics`                  |          10 |
| `ID2D1LinearGradientBrush`          |          10 |
| `MonitorFromWindow`                 |           9 |
| `SetWinEventHook`                   |           9 |
| `ID2D1Geometry`                     |           9 |
| `SetWindowLongPtr`                  |           9 |
| `IWICBitmapSource`                  |           9 |
| `IUIAutomationElement`              |           9 |
| `UnhookWinEvent`                    |           8 |
| `CallNextHookEx`                    |           8 |
| `INPUT_KEYBOARD`                    |           8 |
| `IFACEMETHODIMP_`                   |           8 |
| `GetDpiForMonitor`                  |           7 |
| `DispatchMessage`                   |           7 |
| `IWICBitmapFrameDecode`             |           7 |
| `IAudioMeterInformation`            |           7 |
| `ID2D1RoundedRectangleGeometry`     |           7 |
| `ID2D1BitmapRenderTarget`           |           7 |
| `INT`                               |           7 |
| `IPropertyStore`                    |           6 |
| `ICONINFO`                          |           6 |
| `IWICBitmapDecoder`                 |           6 |
| `ID2D1GradientStopCollection`       |           6 |
| `IMMDeviceCollection`               |           6 |
| `IAudioSessionControl`              |           6 |
| `DeviceIoControl`                   |           6 |
| `IWICBitmapScaler`                  |           5 |
| `IAudioSessionManager2`             |           5 |
| `IMMNotificationClient`             |           5 |
| `IDropTarget`                       |           5 |
| `IUIAutomationElementArray`         |           5 |
| `GetMessage`                        |           4 |
| `IShellItemImageFactory`            |           4 |
| `EnumDisplayMonitors`               |           4 |
| `IGraphicsEffect`                   |           4 |
| `DwmSetWindowAttribute`             |           4 |
| `IDWriteInlineObject`               |           4 |
| `IDWriteTypography`                 |           4 |
| `ISimpleAudioVolume`                |           4 |
| `IAudioSessionEnumerator`           |           4 |
| `IAudioSessionControl2`             |           4 |
| `IID_IUnknown`                      |           4 |
| `IMFSensorActivityReport`           |           4 |
| `IMFSensorActivitiesReportCallback` |           4 |
| `IAppResolver8`                     |           3 |
| `IDispatch`                         |           3 |
| `ID2D1BitmapBrush`                  |           3 |
| `IWbemServices`                     |           3 |
| `IWbemClassObject`                  |           3 |
| `ID2D1DCRenderTarget`               |           3 |
| `ICompositorDesktopInteropLocal`    |           3 |
| `IGeometrySource2DInteropLocal`     |           3 |
| `IGraphicsEffectD2D1InteropLocal`   |           3 |
| `IGraphicsEffectSource`             |           3 |
| `IStream`                           |           3 |
| `IAudioSessionNotification`         |           3 |
| `IAudioSessionEvents`               |           3 |
| `IMFSensorProcessActivity`          |           3 |
| `IMFSensorActivitiesReport`         |           3 |
| `IMFSensorActivityMonitor`          |           3 |
| `SHGetKnownFolderPath`              |           3 |
| `ID2D1RadialGradientBrush`          |           3 |
| `IF_TYPE_ETHERNET_CSMACD`           |           3 |
| `INetworkListManagerEvents`         |           3 |
| `IUIAutomation`                     |           3 |
| `IPersistFile`                      |           2 |
| `ID2D1DeviceContext`                |           2 |
| `SetWindowsHookEx`                  |           2 |
| `SendMessage`                       |           2 |
| `UnhookWindowsHookEx`               |           2 |
| `IQuietHoursSettings`               |           2 |
| `ID2D1HwndRenderTarget`             |           2 |
| `ICON_BIG`                          |           2 |
| `IWICStream`                        |           2 |
| `IPolicyConfig`                     |           2 |
| `IRadioManager`                     |           2 |
| `IKsControlProperty`                |           2 |
| `IDeviceTopology`                   |           2 |
| `IConnector`                        |           2 |
| `IDXGIFactory1`                     |           2 |
| `IFileOpenDialog`                   |           2 |
| `IME_CMODE_NATIVE`                  |           2 |
| `INT_MAX`                           |           2 |
| `SHGetFileInfo`                     |           2 |
| `IFileDialog`                       |           2 |
| `IDesktopWallpaper`                 |           2 |
| `INotificationActivationCallback`   |           2 |
| `IAudioEndpointVolumeCallback`      |           2 |
| `IF_TYPE_IEEE80211`                 |           2 |
| `IUIAutomationCondition`            |           2 |
| `IMAGE_DOS_HEADER`                  |           2 |
| `IMAGE_NT_HEADERS`                  |           2 |
| `IShellLinkW`                       |           1 |
| `IShellWindows`                     |           1 |
| `IShellBrowser`                     |           1 |
| `IShellView`                        |           1 |
| `IServiceProvider`                  |           1 |
| `IShellFolderViewDual`              |           1 |
| `IShellDispatch2`                   |           1 |
| `IDC_HAND`                          |           1 |

Every occurrence with file and line is in `windows-api-usage.csv`. Matching is pattern-based; it does not prove that every call is a direct Windows API call.

## 6. Include and Dependency Inventory

| Include                                       | Occurrences |
| --------------------------------------------- | ----------: |
| `windhawk_utils.h`                            |           1 |
| `appmodel.h`                                  |           1 |
| `aclapi.h`                                    |           1 |
| `audiopolicy.h`                               |           1 |
| `bluetoothapis.h`                             |           1 |
| `cfgmgr32.h`                                  |           1 |
| `commdlg.h`                                   |           1 |
| `d2d1.h`                                      |           1 |
| `d2d1_1.h`                                    |           1 |
| `d2d1_3.h`                                    |           1 |
| `dbt.h`                                       |           1 |
| `devicetopology.h`                            |           1 |
| `dwmapi.h`                                    |           1 |
| `dwrite.h`                                    |           1 |
| `endpointvolume.h`                            |           1 |
| `exdisp.h`                                    |           1 |
| `highlevelmonitorconfigurationapi.h`          |           1 |
| `imm.h`                                       |           1 |
| `mmdeviceapi.h`                               |           1 |
| `mmsystem.h`                                  |           1 |
| `msctf.h`                                     |           1 |
| `netlistmgr.h`                                |           1 |
| `winsock2.h`                                  |           1 |
| `ws2ipdef.h`                                  |           1 |
| `iphlpapi.h`                                  |           1 |
| `dxgi.h`                                      |           1 |
| `pdh.h`                                       |           1 |
| `psapi.h`                                     |           1 |
| `physicalmonitorenumerationapi.h`             |           1 |
| `propsys.h`                                   |           1 |
| `wbemidl.h`                                   |           1 |
| `wincodec.h`                                  |           1 |
| `winhttp.h`                                   |           1 |
| `winioctl.h`                                  |           1 |
| `wlanapi.h`                                   |           1 |
| `setupapi.h`                                  |           1 |
| `shellapi.h`                                  |           1 |
| `sddl.h`                                      |           1 |
| `shellscalingapi.h`                           |           1 |
| `shldisp.h`                                   |           1 |
| `shlobj.h`                                    |           1 |
| `shlwapi.h`                                   |           1 |
| `shobjidl.h`                                  |           1 |
| `tlhelp32.h`                                  |           1 |
| `userenv.h`                                   |           1 |
| `uiautomation.h`                              |           1 |
| `wuapi.h`                                     |           1 |
| `dispatcherqueue.h`                           |           1 |
| `algorithm`                                   |           1 |
| `array`                                       |           1 |
| `atomic`                                      |           1 |
| `chrono`                                      |           1 |
| `climits`                                     |           1 |
| `cmath`                                       |           1 |
| `cstdio`                                      |           1 |
| `cwctype`                                     |           1 |
| `functional`                                  |           1 |
| `limits`                                      |           1 |
| `memory`                                      |           1 |
| `optional`                                    |           1 |
| `string`                                      |           1 |
| `string_view`                                 |           1 |
| `unordered_map`                               |           1 |
| `utility`                                     |           1 |
| `vector`                                      |           1 |
| `winrt/Windows.Devices.Geolocation.h`         |           1 |
| `winrt/Windows.Devices.Radios.h`              |           1 |
| `winrt/Windows.Foundation.Collections.h`      |           1 |
| `winrt/Windows.Foundation.h`                  |           1 |
| `winrt/Windows.Graphics.Effects.h`            |           1 |
| `winrt/Windows.Graphics.h`                    |           1 |
| `winrt/Windows.System.h`                      |           1 |
| `winrt/Windows.UI.Composition.Desktop.h`      |           1 |
| `winrt/Windows.Media.Control.h`               |           1 |
| `winrt/Windows.Media.Core.h`                  |           1 |
| `winrt/Windows.UI.Notifications.h`            |           1 |
| `winrt/Windows.Media.Playback.h`              |           1 |
| `winrt/Windows.Networking.Connectivity.h`     |           1 |
| `winrt/Windows.Networking.NetworkOperators.h` |           1 |
| `winrt/Windows.Storage.Streams.h`             |           1 |
| `winrt/Windows.UI.Composition.h`              |           1 |
| `winrt/Windows.UI.Input.h`                    |           1 |
| `winrt/Windows.UI.Xaml.Automation.Peers.h`    |           1 |
| `winrt/Windows.UI.Xaml.Automation.Provider.h` |           1 |
| `winrt/Windows.UI.Xaml.Automation.h`          |           1 |
| `winrt/Windows.UI.Xaml.Controls.Primitives.h` |           1 |
| `winrt/Windows.UI.Xaml.Controls.h`            |           1 |
| `winrt/Windows.UI.Xaml.Data.h`                |           1 |
| `winrt/Windows.UI.Xaml.Hosting.h`             |           1 |
| `winrt/Windows.UI.Xaml.Input.h`               |           1 |
| `winrt/Windows.UI.Xaml.Markup.h`              |           1 |
| `winrt/Windows.UI.Xaml.Media.Imaging.h`       |           1 |
| `winrt/Windows.UI.Xaml.Media.h`               |           1 |
| `winrt/Windows.UI.Xaml.Shapes.h`              |           1 |
| `winrt/Windows.UI.Xaml.h`                     |           1 |
| `winrt/Windows.UI.h`                          |           1 |

Full include list: `includes.csv`. Local/external classification is approximate.

## 7. Heuristic Internal Call Graph

Use `functions.json` and the `calls_defined_in_repo` field to inspect likely internal call relationships. Name-based matching can overcount overloaded functions or miss indirect calls, callbacks, function pointers, macros, and virtual dispatch.

## 8. Recommended AI-Agent Reading Order

1. Read this report and `repository-files.csv` to understand the repository shape.
2. Read the mod metadata and settings block, then trace the primary initialization and teardown functions.
3. Follow the entry-point call graph into window creation, event/message dispatch, hooks, timers, worker threads, and cleanup.
4. For a feature task, use `functions.csv` to locate relevant functions and inspect only those functions plus their callers/callees.
5. Use `windows-api-usage.csv` to find system-boundary code and `includes.csv` to identify dependencies.
6. Before edits, ask the agent to state the exact files/functions it intends to change and the expected lifecycle impact.

## Output Files

- `README-ANALYSIS.md` — this report
- `repository-files.csv` — file inventory and line counts
- `functions.csv` / `functions.json` — function index and heuristic call relationships
- `windhawk-settings.csv` — detected Windhawk settings
- `variable-candidates.csv` — global/declaration candidates
- `windows-api-usage.csv` — matched Windows/system API occurrences
- `lifecycle-symbols.csv` — lifecycle/event-related symbol occurrences
- `includes.csv` — include/dependency inventory
- `analysis-summary.json` — machine-readable summary
