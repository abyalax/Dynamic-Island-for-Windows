# Codebase Analysis Report

- Root: `C:\Users\abya\c\Dynamic-Island-for-Windows`
- Generated: 2026-10-25T12:11:14+07:00
- Files analyzed: **1**
- Total physical lines: **23,693**
- Nonblank lines: **21,230**
- Source files (`.c/.cpp/.h` family): **1**
- Function definitions detected (heuristic): **534**

> Static-analysis note: function, variable, API, and call-graph results are heuristic inventories, not compiler AST output. Validate ambiguous matches against source context.

## 1. Repository Structure

| Extension | Files |
| --------- | ----: |
| `.cpp`    |     1 |

### Largest files

| File                                          |  Lines | Size (bytes) |
| --------------------------------------------- | -----: | -----------: |
| `src/dynamic-island-for-windows-local.wh.cpp` | 23,693 |    1,007,261 |

## 2. Entry Points and Lifecycle Candidates

| Symbol                  | File                                          |  Line |
| ----------------------- | --------------------------------------------- | ----: |
| `Wh_ModInit`            | `src/dynamic-island-for-windows-local.wh.cpp` | 23558 |
| `Wh_ModAfterInit`       | `src/dynamic-island-for-windows-local.wh.cpp` | 23631 |
| `Wh_ModSettingsChanged` | `src/dynamic-island-for-windows-local.wh.cpp` | 23678 |
| `Wh_ModUninit`          | `src/dynamic-island-for-windows-local.wh.cpp` | 23686 |

### Lifecycle-related symbol occurrences

| Symbol                   | Occurrences |
| ------------------------ | ----------: |
| `Initialize`             |           6 |
| `WndProc`                |           2 |
| `Start`                  |           2 |
| `Stop`                   |           2 |
| `Shutdown`               |           2 |
| `Wh_ModInit`             |           1 |
| `Wh_ModAfterInit`        |           1 |
| `Wh_ModSettingsChanged`  |           1 |
| `Wh_ModUninit`           |           1 |
| `OnNotify`               |           1 |
| `OnDefaultDeviceChanged` |           1 |
| `OnDeviceStateChanged`   |           1 |
| `OnDeviceAdded`          |           1 |
| `OnDeviceRemoved`        |           1 |
| `OnPropertyValueChanged` |           1 |

## 3. Configurable Settings and Variable Candidates

- Windhawk setting candidates: **183**
- Global/declaration candidates: **4026**

### Windhawk settings detected

| Setting                      | Default                                      | File                                          | Line |
| ---------------------------- | -------------------------------------------- | --------------------------------------------- | ---: |
| `Appearance`                 | ``                                           | `src/dynamic-island-for-windows-local.wh.cpp` |  158 |
| `Position`                   | `top-center`                                 | `src/dynamic-island-for-windows-local.wh.cpp` |  159 |
| `top-center`                 | `Top Center`                                 | `src/dynamic-island-for-windows-local.wh.cpp` |  163 |
| `top-left`                   | `Top Left`                                   | `src/dynamic-island-for-windows-local.wh.cpp` |  164 |
| `top-right`                  | `Top Right`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  165 |
| `bottom-center`              | `Bottom Center`                              | `src/dynamic-island-for-windows-local.wh.cpp` |  166 |
| `bottom-left`                | `Bottom Left`                                | `src/dynamic-island-for-windows-local.wh.cpp` |  167 |
| `bottom-right`               | `Bottom Right`                               | `src/dynamic-island-for-windows-local.wh.cpp` |  168 |
| `TargetMonitor`              | `primary`                                    | `src/dynamic-island-for-windows-local.wh.cpp` |  169 |
| `OffsetX`                    | `0`                                          | `src/dynamic-island-for-windows-local.wh.cpp` |  180 |
| `OffsetY`                    | `0`                                          | `src/dynamic-island-for-windows-local.wh.cpp` |  183 |
| `SeparateExpandedOffsetY`    | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  186 |
| `OffsetYExpanded`            | `0`                                          | `src/dynamic-island-for-windows-local.wh.cpp` |  189 |
| `BorderMergedMode`           | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  192 |
| `ShapeStyle`                 | `default`                                    | `src/dynamic-island-for-windows-local.wh.cpp` |  195 |
| `default`                    | `Default (Apple Pill)`                       | `src/dynamic-island-for-windows-local.wh.cpp` |  199 |
| `w11`                        | `Windows 11 (Rounded Box)`                   | `src/dynamic-island-for-windows-local.wh.cpp` |  200 |
| `notch`                      | `macOS Notch (Top Edge Flush)`               | `src/dynamic-island-for-windows-local.wh.cpp` |  201 |
| `ProgressStyle`              | `slim`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  202 |
| `slim`                       | `Slim (classic bar)`                         | `src/dynamic-island-for-windows-local.wh.cpp` |  206 |
| `wavy`                       | `Wavy`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  207 |
| `squiggle`                   | `Squiggle (slower, subtler wave)`            | `src/dynamic-island-for-windows-local.wh.cpp` |  208 |
| `bar`                        | `Bar (thick bar with a line thumb)`          | `src/dynamic-island-for-windows-local.wh.cpp` |  209 |
| `SpectrumStyle`              | `bars`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  210 |
| `bars`                       | `Classic Bars`                               | `src/dynamic-island-for-windows-local.wh.cpp` |  214 |
| `led`                        | `Peak Matrix`                                | `src/dynamic-island-for-windows-local.wh.cpp` |  215 |
| `plasma`                     | `Plasma Thread`                              | `src/dynamic-island-for-windows-local.wh.cpp` |  216 |
| `orb`                        | `Pulse Orb`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  217 |
| `SizeScale`                  | `'1.0'`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  218 |
| `AutoDpiScale`               | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  229 |
| `Behavior`                   | ``                                           | `src/dynamic-island-for-windows-local.wh.cpp` |  233 |
| `AlwaysOnTop`                | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  234 |
| `ExpandOnHover`              | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  237 |
| `AutoHideIdleSeconds`        | `'0'`                                        | `src/dynamic-island-for-windows-local.wh.cpp` |  240 |
| `AutoHideFullscreen`         | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  250 |
| `UnhideOnHover`              | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  253 |
| `Animations`                 | ``                                           | `src/dynamic-island-for-windows-local.wh.cpp` |  257 |
| `TargetFPS`                  | `auto`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  258 |
| `auto`                       | `Auto (Match Monitor Refresh Rate)`          | `src/dynamic-island-for-windows-local.wh.cpp` |  262 |
| `AnimationStyle`             | `default`                                    | `src/dynamic-island-for-windows-local.wh.cpp` |  271 |
| `smooth`                     | `Smooth (No bounciness / Critically damped)` | `src/dynamic-island-for-windows-local.wh.cpp` |  275 |
| `default`                    | `Default (Balanced Apple-like spring)`       | `src/dynamic-island-for-windows-local.wh.cpp` |  276 |
| `bouncy`                     | `Bouncy (Dynamic elastic spring)`            | `src/dynamic-island-for-windows-local.wh.cpp` |  277 |
| `snappy`                     | `Snappy (High stiffness, quick settle)`      | `src/dynamic-island-for-windows-local.wh.cpp` |  278 |
| `AnimationSpeed`             | `normal`                                     | `src/dynamic-island-for-windows-local.wh.cpp` |  279 |
| `very-slow`                  | `Very Slow (0.5x)`                           | `src/dynamic-island-for-windows-local.wh.cpp` |  283 |
| `slow`                       | `Slow (0.75x)`                               | `src/dynamic-island-for-windows-local.wh.cpp` |  284 |
| `normal`                     | `Normal (1.0x)`                              | `src/dynamic-island-for-windows-local.wh.cpp` |  285 |
| `fast`                       | `Fast (1.35x)`                               | `src/dynamic-island-for-windows-local.wh.cpp` |  286 |
| `very-fast`                  | `Very Fast (1.65x)`                          | `src/dynamic-island-for-windows-local.wh.cpp` |  287 |
| `ultra-fast`                 | `Ultra Fast (2.0x)`                          | `src/dynamic-island-for-windows-local.wh.cpp` |  288 |
| `ExpandedMediaTransitions`   | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  289 |
| `PillCoverSpin`              | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  293 |
| `Themes`                     | ``                                           | `src/dynamic-island-for-windows-local.wh.cpp` |  297 |
| `ThemePreset`                | `obsidian`                                   | `src/dynamic-island-for-windows-local.wh.cpp` |  298 |
| `obsidian`                   | `Obsidian - true black (Default)`            | `src/dynamic-island-for-windows-local.wh.cpp` |  302 |
| `graphite`                   | `Graphite - neutral Windows 11 dark`         | `src/dynamic-island-for-windows-local.wh.cpp` |  303 |
| `slate`                      | `Slate - cool blue-grey`                     | `src/dynamic-island-for-windows-local.wh.cpp` |  304 |
| `nord`                       | `Nord - the Nord palette`                    | `src/dynamic-island-for-windows-local.wh.cpp` |  305 |
| `evergreen`                  | `Evergreen - deep green`                     | `src/dynamic-island-for-windows-local.wh.cpp` |  306 |
| `espresso`                   | `Espresso - warm brown`                      | `src/dynamic-island-for-windows-local.wh.cpp` |  307 |
| `plum`                       | `Plum - muted mauve`                         | `src/dynamic-island-for-windows-local.wh.cpp` |  308 |
| `porcelain`                  | `Porcelain - light theme`                    | `src/dynamic-island-for-windows-local.wh.cpp` |  309 |
| `custom`                     | `Custom Colors (Use Hex Below)`              | `src/dynamic-island-for-windows-local.wh.cpp` |  310 |
| `PillOpacity`                | `96`                                         | `src/dynamic-island-for-windows-local.wh.cpp` |  311 |
| `TintIntensity`              | `72`                                         | `src/dynamic-island-for-windows-local.wh.cpp` |  314 |
| `BackdropMaterial`           | `none`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  317 |
| `none`                       | `Off (solid background)`                     | `src/dynamic-island-for-windows-local.wh.cpp` |  321 |
| `blur`                       | `Blur`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  322 |
| `acrylic`                    | `Acrylic (frosted)`                          | `src/dynamic-island-for-windows-local.wh.cpp` |  323 |
| `BackdropFillOpacity`        | `45`                                         | `src/dynamic-island-for-windows-local.wh.cpp` |  324 |
| `BackdropTint`               | `55`                                         | `src/dynamic-island-for-windows-local.wh.cpp` |  327 |
| `MaterialDepth`              | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  330 |
| `DropShadow`                 | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  333 |
| `AccentBloom`                | `100`                                        | `src/dynamic-island-for-windows-local.wh.cpp` |  336 |
| `TextScale`                  | `100`                                        | `src/dynamic-island-for-windows-local.wh.cpp` |  339 |
| `AccentColorMode`            | `auto`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  342 |
| `auto`                       | `Auto, from album art`                       | `src/dynamic-island-for-windows-local.wh.cpp` |  346 |
| `system`                     | `System (Device Accent)`                     | `src/dynamic-island-for-windows-local.wh.cpp` |  347 |
| `custom`                     | `Custom hex`                                 | `src/dynamic-island-for-windows-local.wh.cpp` |  348 |
| `CustomAccentHex`            | `"#4cc9f0"`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  349 |
| `CalendarAccent`             | `red`                                        | `src/dynamic-island-for-windows-local.wh.cpp` |  352 |
| `red`                        | `Default (Island accent)`                    | `src/dynamic-island-for-windows-local.wh.cpp` |  356 |
| `system`                     | `System (Device Accent)`                     | `src/dynamic-island-for-windows-local.wh.cpp` |  357 |
| `ClockAccentGlow`            | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  358 |
| `FontFamily`                 | `""`                                         | `src/dynamic-island-for-windows-local.wh.cpp` |  361 |
| `ContourBorderMode`          | `default`                                    | `src/dynamic-island-for-windows-local.wh.cpp` |  364 |
| `default`                    | `Default`                                    | `src/dynamic-island-for-windows-local.wh.cpp` |  368 |
| `auto`                       | `Auto (From album art)`                      | `src/dynamic-island-for-windows-local.wh.cpp` |  369 |
| `borderless`                 | `Borderless`                                 | `src/dynamic-island-for-windows-local.wh.cpp` |  370 |
| `ContourBorderHex`           | `"#1E1E22"`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  371 |
| `PillBgColor`                | `"#08080A"`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  374 |
| `TextPrimaryColor`           | `"#FFFFFF"`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  377 |
| `TextSecondaryColor`         | `"#9B9BA5"`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  380 |
| `Indicators`                 | ``                                           | `src/dynamic-island-for-windows-local.wh.cpp` |  384 |
| `PrivacyDots`                | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  385 |
| `PrivacyDotsMic`             | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  388 |
| `PrivacyDotsMicHex`          | `"#FF9500"`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  391 |
| `PrivacyDotsCam`             | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  394 |
| `PrivacyDotsCamHex`          | `"#34C759"`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  397 |
| `Modules`                    | ``                                           | `src/dynamic-island-for-windows-local.wh.cpp` |  401 |
| `Media`                      | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  402 |
| `MediaAutoExpand`            | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  405 |
| `MediaPillClock`             | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  408 |
| `Volume`                     | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  411 |
| `Brightness`                 | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  414 |
| `CapsLock`                   | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  417 |
| `Battery`                    | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  420 |
| `BluetoothIndicator`         | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  423 |
| `BluetoothShowBattery`       | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  426 |
| `Progress`                   | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  429 |
| `TimerModule`                | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  432 |
| `Clipboard`                  | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  435 |
| `StatusCountdownProgress`    | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  438 |
| `DoNotDisturbIndicator`      | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  441 |
| `NotificationRespectDnD`     | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  444 |
| `HardwareMonitorModule`      | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  447 |
| `GameOverlay`                | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  450 |
| `ShowMetricText`             | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  453 |
| `Weather`                    | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  456 |
| `WeatherCity`                | `""`                                         | `src/dynamic-island-for-windows-local.wh.cpp` |  459 |
| `WeatherFahrenheit`          | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  462 |
| `MediaTabLyrics`             | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  465 |
| `Lyrics`                     | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  468 |
| `LyricsVisualizer`           | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  471 |
| `KaraokeLyrics`              | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  474 |
| `KaraokeLetterGlow`          | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  477 |
| `LyricsOffsetMs`             | `0`                                          | `src/dynamic-island-for-windows-local.wh.cpp` |  480 |
| `LyricsOffsetBluetoothMs`    | `0`                                          | `src/dynamic-island-for-windows-local.wh.cpp` |  483 |
| `LyricsNeighborLinesVisible` | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  486 |
| `LyricsCache`                | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  489 |
| `LyricsCacheMaxMB`           | `'25'`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  492 |
| `Language`                   | `auto`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  500 |
| `auto`                       | `Auto (Follow Windows)`                      | `src/dynamic-island-for-windows-local.wh.cpp` |  504 |
| `en`                         | `English`                                    | `src/dynamic-island-for-windows-local.wh.cpp` |  505 |
| `fr`                         | `Français (French)`                          | `src/dynamic-island-for-windows-local.wh.cpp` |  506 |
| `es`                         | `Español (Spanish)`                          | `src/dynamic-island-for-windows-local.wh.cpp` |  507 |
| `de`                         | `Deutsch (German)`                           | `src/dynamic-island-for-windows-local.wh.cpp` |  508 |
| `pt`                         | `Português (Portuguese)`                     | `src/dynamic-island-for-windows-local.wh.cpp` |  509 |
| `it`                         | `Italiano (Italian)`                         | `src/dynamic-island-for-windows-local.wh.cpp` |  510 |
| `ru`                         | `Русский (Russian)`                          | `src/dynamic-island-for-windows-local.wh.cpp` |  511 |
| `tr`                         | `Türkçe (Turkish)`                           | `src/dynamic-island-for-windows-local.wh.cpp` |  512 |
| `hi`                         | `हिन्दी (Hindi)`                                | `src/dynamic-island-for-windows-local.wh.cpp` |  513 |
| `zh`                         | `简体中文 (Simplified Chinese)`              | `src/dynamic-island-for-windows-local.wh.cpp` |  514 |
| `ja`                         | `日本語 (Japanese)`                          | `src/dynamic-island-for-windows-local.wh.cpp` |  515 |
| `ko`                         | `한국어 (Korean)`                            | `src/dynamic-island-for-windows-local.wh.cpp` |  516 |
| `ClockFormat`                | `system`                                     | `src/dynamic-island-for-windows-local.wh.cpp` |  517 |
| `system`                     | `Follow Windows locale`                      | `src/dynamic-island-for-windows-local.wh.cpp` |  521 |
| `ShowSeconds`                | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  524 |
| `DateFormat`                 | `""`                                         | `src/dynamic-island-for-windows-local.wh.cpp` |  527 |
| `DateFirst`                  | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  530 |
| `FirstDayOfWeek`             | `auto`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  533 |
| `auto`                       | `Auto (Follow Windows)`                      | `src/dynamic-island-for-windows-local.wh.cpp` |  537 |
| `monday`                     | `Monday`                                     | `src/dynamic-island-for-windows-local.wh.cpp` |  538 |
| `sunday`                     | `Sunday`                                     | `src/dynamic-island-for-windows-local.wh.cpp` |  539 |
| `QuickLookup`                | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  540 |
| `FileTrayModule`             | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  543 |
| `FileTrayMaxItems`           | `10`                                         | `src/dynamic-island-for-windows-local.wh.cpp` |  546 |
| `FileTrayPersist`            | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  549 |
| `MediaExpandBlocklist`       | `""`                                         | `src/dynamic-island-for-windows-local.wh.cpp` |  552 |
| `GameOverlayShowFps`         | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  555 |
| `GameOverlayShowCpu`         | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  558 |
| `GameOverlayShowGpu`         | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  561 |
| `GameOverlayShowRam`         | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  564 |
| `GameOverlayShowDisk`        | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  567 |
| `GameOverlayCompact`         | `false`                                      | `src/dynamic-island-for-windows-local.wh.cpp` |  570 |
| `localLrcFolders`            | `""`                                         | `src/dynamic-island-for-windows-local.wh.cpp` |  574 |
| `Shortcuts`                  | ``                                           | `src/dynamic-island-for-windows-local.wh.cpp` |  577 |
| `HideShowHotkeyEnabled`      | `true`                                       | `src/dynamic-island-for-windows-local.wh.cpp` |  578 |
| `HideShowModifiers`          | `ctrl_alt`                                   | `src/dynamic-island-for-windows-local.wh.cpp` |  581 |
| `ctrl_alt`                   | `Ctrl + Alt`                                 | `src/dynamic-island-for-windows-local.wh.cpp` |  585 |
| `ctrl_shift`                 | `Ctrl + Shift`                               | `src/dynamic-island-for-windows-local.wh.cpp` |  586 |
| `alt_shift`                  | `Alt + Shift`                                | `src/dynamic-island-for-windows-local.wh.cpp` |  587 |
| `win_alt`                    | `Win + Alt`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  588 |
| `ctrl_alt_shift`             | `Ctrl + Alt + Shift`                         | `src/dynamic-island-for-windows-local.wh.cpp` |  589 |
| `HideShowKey`                | `"D"`                                        | `src/dynamic-island-for-windows-local.wh.cpp` |  590 |
| `LookupModifiers`            | `ctrl_alt`                                   | `src/dynamic-island-for-windows-local.wh.cpp` |  593 |
| `ctrl_alt`                   | `Ctrl + Alt`                                 | `src/dynamic-island-for-windows-local.wh.cpp` |  597 |
| `ctrl_shift`                 | `Ctrl + Shift`                               | `src/dynamic-island-for-windows-local.wh.cpp` |  598 |
| `alt_shift`                  | `Alt + Shift`                                | `src/dynamic-island-for-windows-local.wh.cpp` |  599 |
| `win_alt`                    | `Win + Alt`                                  | `src/dynamic-island-for-windows-local.wh.cpp` |  600 |
| `ctrl_alt_shift`             | `Ctrl + Alt + Shift`                         | `src/dynamic-island-for-windows-local.wh.cpp` |  601 |
| `LookupKey`                  | `"Space"`                                    | `src/dynamic-island-for-windows-local.wh.cpp` |  602 |

Variable candidates are exported to `variable-candidates.csv`; inspect each declaration before classifying it as global/configuration/state.

## 4. Function Inventory

Function definitions detected: **534**

### Largest functions by estimated body span

| Function                       | File                                          | Lines | Incoming calls (heuristic) |
| ------------------------------ | --------------------------------------------- | ----: | -------------------------: |
| `RenderThreadProc`             | `src/dynamic-island-for-windows-local.wh.cpp` |   985 |                          0 |
| `OverlayWndProc`               | `src/dynamic-island-for-windows-local.wh.cpp` |   855 |                          0 |
| `LoadSettings`                 | `src/dynamic-island-for-windows-local.wh.cpp` |   605 |                          3 |
| `DrawMedia`                    | `src/dynamic-island-for-windows-local.wh.cpp` |   498 |                          1 |
| `DrawLyricsDashboard`          | `src/dynamic-island-for-windows-local.wh.cpp` |   380 |                          1 |
| `ShowContextMenu`              | `src/dynamic-island-for-windows-local.wh.cpp` |   342 |                          1 |
| `MediaThreadProc`              | `src/dynamic-island-for-windows-local.wh.cpp` |   316 |                          0 |
| `DrawImmersiveLyricsView`      | `src/dynamic-island-for-windows-local.wh.cpp` |   228 |                          1 |
| `NotificationThreadProc`       | `src/dynamic-island-for-windows-local.wh.cpp` |   218 |                          0 |
| `WeatherThreadProc`            | `src/dynamic-island-for-windows-local.wh.cpp` |   202 |                          0 |
| `DecodeImageBytesToPixels`     | `src/dynamic-island-for-windows-local.wh.cpp` |   200 |                          2 |
| `DrawIdleDashboard`            | `src/dynamic-island-for-windows-local.wh.cpp` |   190 |                          1 |
| `EnsureTextFormats`            | `src/dynamic-island-for-windows-local.wh.cpp` |   170 |                          4 |
| `DrawCalendarDashboard`        | `src/dynamic-island-for-windows-local.wh.cpp` |   157 |                          3 |
| `DrawWeatherDashboard`         | `src/dynamic-island-for-windows-local.wh.cpp` |   157 |                          3 |
| `Render`                       | `src/dynamic-island-for-windows-local.wh.cpp` |   153 |                          1 |
| `DrawSourceDock`               | `src/dynamic-island-for-windows-local.wh.cpp` |   152 |                          1 |
| `DrawMediaSideLyrics`          | `src/dynamic-island-for-windows-local.wh.cpp` |   151 |                          1 |
| `DrawCollapsedLyrics`          | `src/dynamic-island-for-windows-local.wh.cpp` |   141 |                          1 |
| `DrawFileTrayDashboard`        | `src/dynamic-island-for-windows-local.wh.cpp` |   137 |                          3 |
| `LyricsThreadProc`             | `src/dynamic-island-for-windows-local.wh.cpp` |   133 |                          0 |
| `DrawTimeDashboard`            | `src/dynamic-island-for-windows-local.wh.cpp` |   132 |                          2 |
| `AudioThreadProc`              | `src/dynamic-island-for-windows-local.wh.cpp` |   131 |                          0 |
| `DrawTabTransition`            | `src/dynamic-island-for-windows-local.wh.cpp` |   129 |                          2 |
| `DrawKaraokeLetterPop`         | `src/dynamic-island-for-windows-local.wh.cpp` |   126 |                          1 |
| `CaptureShellNotification`     | `src/dynamic-island-for-windows-local.wh.cpp` |   125 |                          1 |
| `DrawStyledProgress`           | `src/dynamic-island-for-windows-local.wh.cpp` |   118 |                          1 |
| `DrawAlbumArtDrop`             | `src/dynamic-island-for-windows-local.wh.cpp` |   118 |                          1 |
| `Sync`                         | `src/dynamic-island-for-windows-local.wh.cpp` |   114 |                          1 |
| `DrawHardwareMonitorDashboard` | `src/dynamic-island-for-windows-local.wh.cpp` |   113 |                          3 |
| `DrawMetricGlyph`              | `src/dynamic-island-for-windows-local.wh.cpp` |   112 |                          4 |
| `LoadFileTray`                 | `src/dynamic-island-for-windows-local.wh.cpp` |   111 |                          1 |
| `CollectMediaSources`          | `src/dynamic-island-for-windows-local.wh.cpp` |   107 |                          1 |
| `ParseWiktionaryEnglish`       | `src/dynamic-island-for-windows-local.wh.cpp` |   107 |                          1 |
| `DrawPill`                     | `src/dynamic-island-for-windows-local.wh.cpp` |   101 |                          1 |
| `ParseEntry`                   | `src/dynamic-island-for-windows-local.wh.cpp` |    91 |                          1 |
| `HandleLookupKeyDown`          | `src/dynamic-island-for-windows-local.wh.cpp` |    91 |                          1 |
| `ReadClipboardImagePixels`     | `src/dynamic-island-for-windows-local.wh.cpp` |    89 |                          1 |
| `DrawTitleTransition`          | `src/dynamic-island-for-windows-local.wh.cpp` |    88 |                          1 |
| `DrawSpectrumOrb`              | `src/dynamic-island-for-windows-local.wh.cpp` |    88 |                          1 |
| `std::thread`                  | `src/dynamic-island-for-windows-local.wh.cpp` |    84 |                          5 |
| `DrawGameOverlay`              | `src/dynamic-island-for-windows-local.wh.cpp` |    84 |                          1 |
| `DrawLookupRows`               | `src/dynamic-island-for-windows-local.wh.cpp` |    84 |                          1 |
| `UpdateMediaButtonAnimations`  | `src/dynamic-island-for-windows-local.wh.cpp` |    82 |                          1 |
| `DrawMediaButton`              | `src/dynamic-island-for-windows-local.wh.cpp` |    81 |                          1 |
| `UpdateSystemSnapshot`         | `src/dynamic-island-for-windows-local.wh.cpp` |    80 |                          1 |
| `DrawSpectrumLed`              | `src/dynamic-island-for-windows-local.wh.cpp` |    80 |                          1 |
| `DrawBluetoothDevice`          | `src/dynamic-island-for-windows-local.wh.cpp` |    80 |                          1 |
| `IconToPixels`                 | `src/dynamic-island-for-windows-local.wh.cpp` |    79 |                          1 |
| `UpdateSourceSwap`             | `src/dynamic-island-for-windows-local.wh.cpp` |    79 |                          1 |

Full index: `functions.json` and `functions.csv`.

## 5. Windows API and Internal-System Interaction

- API symbol occurrences matched: **569**

| API / symbol                    | Occurrences |
| ------------------------------- | ----------: |
| `ID2D1SolidColorBrush`          |          94 |
| `IDWriteTextFormat`             |          47 |
| `WaitForSingleObject`           |          38 |
| `ID2D1Layer`                    |          21 |
| `PostMessageW`                  |          17 |
| `ID2D1PathGeometry`             |          17 |
| `ID2D1Bitmap`                   |          17 |
| `IDWriteTextLayout`             |          13 |
| `ID2D1Brush`                    |          13 |
| `GetProcAddress`                |          12 |
| `INVALID_HANDLE_VALUE`          |          12 |
| `ID2D1GeometrySink`             |          11 |
| `ID2D1Geometry`                 |          10 |
| `ID2D1StrokeStyle`              |           9 |
| `CreateThread`                  |           9 |
| `IID_PPV_ARGS`                  |           7 |
| `CreateFileW`                   |           7 |
| `ID2D1LinearGradientBrush`      |           7 |
| `IDataObject`                   |           7 |
| `GetForegroundWindow`           |           6 |
| `CoCreateInstance`              |           6 |
| `IS_HIGH_SURROGATE`             |           6 |
| `SetEvent`                      |           5 |
| `ID2D1RadialGradientBrush`      |           5 |
| `ID2D1GradientStopCollection`   |           5 |
| `ID2D1RoundedRectangleGeometry` |           5 |
| `IDropSource`                   |           5 |
| `ID_HIDE_SHOW_HOTKEY`           |           4 |
| `ID_LOOKUP_HOTKEY`              |           4 |
| `GetCursorPos`                  |           4 |
| `SHGetFileInfoW`                |           4 |
| `IMMDevice`                     |           4 |
| `IUnknown`                      |           4 |
| `IAudioEndpointVolume`          |           4 |
| `IDC_HAND`                      |           4 |
| `INVALID_FILE_ATTRIBUTES`       |           4 |
| `UnhookWindowsHookEx`           |           4 |
| `IDropTarget`                   |           4 |
| `GetMonitorInfoW`               |           3 |
| `DestroyWindow`                 |           3 |
| `CoInitializeEx`                |           3 |
| `IMMDeviceEnumerator`           |           3 |
| `IAudioEndpointVolumeCallback`  |           3 |
| `IMMNotificationClient`         |           3 |
| `GetWindowLongPtrW`             |           3 |
| `SetWindowLongPtrW`             |           3 |
| `PeekMessageW`                  |           3 |
| `DispatchMessageW`              |           3 |
| `CreateEventW`                  |           3 |
| `IMAGE_DOS_HEADER`              |           2 |
| `IMAGE_NT_HEADERS`              |           2 |
| `MonitorFromWindow`             |           2 |
| `OpenProcess`                   |           2 |
| `DefWindowProcW`                |           2 |
| `RegisterClassExW`              |           2 |
| `CreateWindowExW`               |           2 |
| `UnregisterHotKey`              |           2 |
| `RegisterHotKey`                |           2 |
| `IWICImagingFactory`            |           2 |
| `IWICBitmapDecoder`             |           2 |
| `IWICBitmapFrameDecode`         |           2 |
| `IWICFormatConverter`           |           2 |
| `IDI_APPLICATION`               |           2 |
| `WaitForMultipleObjects`        |           2 |
| `INTERNET_DEFAULT_HTTPS_PORT`   |           2 |
| `INTERNET_DEFAULT_HTTP_PORT`    |           2 |
| `IAudioClient`                  |           2 |
| `IUIAutomation`                 |           2 |
| `IUIAutomation2`                |           2 |
| `IUIAutomationElement`          |           2 |
| `INPUT`                         |           2 |
| `IS_LOW_SURROGATE`              |           2 |
| `IDC_ARROW`                     |           2 |
| `ID2D1Factory`                  |           2 |
| `IDWriteFactory`                |           2 |
| `CallNextHookEx`                |           2 |
| `SetWindowsHookExW`             |           2 |
| `GetSystemMetrics`              |           2 |
| `INFINITE`                      |           2 |
| `EnumDisplayMonitors`           |           1 |
| `IStream`                       |           1 |
| `ICON_BIG`                      |           1 |
| `ICON_SMALL`                    |           1 |
| `IRandomAccessStreamReference`  |           1 |
| `IInspectable`                  |           1 |
| `IWICBitmapSource`              |           1 |
| `IWICBitmapScaler`              |           1 |
| `IAudioCaptureClient`           |           1 |
| `IPropertyStore`                |           1 |
| `DeviceIoControl`               |           1 |
| `IUIAutomationCondition`        |           1 |
| `IUIAutomationElementArray`     |           1 |
| `IDC_IBEAM`                     |           1 |
| `INT_PTR`                       |           1 |
| `IDWriteFontCollection`         |           1 |
| `ID2D1TransformedGeometry`      |           1 |
| `ID2D1EllipseGeometry`          |           1 |
| `IDWriteInlineObject`           |           1 |
| `ID2D1DCRenderTarget`           |           1 |
| `IShellFolder`                  |           1 |

Every occurrence with file and line is in `windows-api-usage.csv`. Matching is pattern-based; it does not prove that every call is a direct Windows API call.

## 6. Include and Dependency Inventory

| Include                                                     | Occurrences |
| ----------------------------------------------------------- | ----------: |
| `mutex`                                                     |           2 |
| `audioclient.h`                                             |           1 |
| `audiopolicy.h`                                             |           1 |
| `d2d1.h`                                                    |           1 |
| `dbt.h`                                                     |           1 |
| `devpropdef.h`                                              |           1 |
| `dwmapi.h`                                                  |           1 |
| `dwrite.h`                                                  |           1 |
| `endpointvolume.h`                                          |           1 |
| `filesystem`                                                |           1 |
| `fstream`                                                   |           1 |
| `mmdeviceapi.h`                                             |           1 |
| `mmreg.h`                                                   |           1 |
| `mmsystem.h`                                                |           1 |
| `objbase.h`                                                 |           1 |
| `ole2.h`                                                    |           1 |
| `pdh.h`                                                     |           1 |
| `pdhmsg.h`                                                  |           1 |
| `sddl.h`                                                    |           1 |
| `setupapi.h`                                                |           1 |
| `shcore.h`                                                  |           1 |
| `shellapi.h`                                                |           1 |
| `shlobj.h`                                                  |           1 |
| `uiautomation.h`                                            |           1 |
| `unknwn.h`                                                  |           1 |
| `wincodec.h`                                                |           1 |
| `windows.h`                                                 |           1 |
| `windowsx.h`                                                |           1 |
| `winhttp.h`                                                 |           1 |
| `wrl/client.h`                                              |           1 |
| `algorithm`                                                 |           1 |
| `array`                                                     |           1 |
| `atomic`                                                    |           1 |
| `chrono`                                                    |           1 |
| `cmath`                                                     |           1 |
| `cstdint`                                                   |           1 |
| `cstring`                                                   |           1 |
| `cwchar`                                                    |           1 |
| `memory`                                                    |           1 |
| `optional`                                                  |           1 |
| `set`                                                       |           1 |
| `string`                                                    |           1 |
| `string_view`                                               |           1 |
| `thread`                                                    |           1 |
| `unordered_map`                                             |           1 |
| `utility`                                                   |           1 |
| `vector`                                                    |           1 |
| `winrt/Windows.Foundation.Collections.h`                    |           1 |
| `winrt/Windows.Foundation.h`                                |           1 |
| `winrt/Windows.Media.Control.h`                             |           1 |
| `winrt/Windows.Storage.Streams.h`                           |           1 |
| `winrt/base.h`                                              |           1 |
| `winrt/Windows.ApplicationModel.h`                          |           1 |
| `winrt/Windows.UI.Notifications.Management.h`               |           1 |
| `winrt/Windows.UI.Notifications.h`                          |           1 |
| `winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h` |           1 |
| `winrt/Windows.Devices.Bluetooth.h`                         |           1 |
| `winrt/Windows.Devices.Enumeration.h`                       |           1 |

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
