// ==WindhawkMod==
// @id              mac-island-dock-fork
// @name            Mac Island & Dock - Fork
// @name:pt-BR      Ilha e Dock estilo Mac - Fork
// @name:vi-VN      Mac Island & Dock (Dynamic Island & Dock kiểu Mac) - Fork
// @description     An island at the top of the screen like the macOS menu bar and the iPhone's Dynamic Island (clock, control center, notifications, media, tray icons), and a macOS-like dock instead of the taskbar (or the Windows taskbar, transparent or as it is)
// @description:pt-BR Uma ilha no topo da tela como a barra de menus do macOS e a Dynamic Island do iPhone (relógio, central de controle, notificações, mídia, ícones da bandeja) e uma dock como a do macOS no lugar da barra de tarefas (ou a barra do Windows, transparente ou como ela é)
// @description:vi-VN Thanh đảo ở cạnh trên màn hình kết hợp phong cách menu bar macOS và Dynamic Island của iPhone (đồng hồ, trung tâm điều khiển, thông báo, trình phát nhạc, khay hệ thống), đi kèm thanh Dock kiểu macOS thay cho taskbar (hoặc làm trong suốt taskbar Windows)
// @version         3.0.0
// @author          caliberda
// @github          https://github.com/cesarkali
// @homepage        https://island.caliberda.com.br
// @donateUrl       https://github.com/sponsors/cesarkali
// @include         windhawk.exe
// @include         explorer.exe
// @compilerOptions -DWIN32_LEAN_AND_MEAN -lole32 -loleaut32 -lruntimeobject -lshell32 -ldwmapi -lcomctl32 -lgdi32 -lmsimg32 -ld2d1 -ldwrite -lwlanapi -luuid -lversion -lbthprops -ldxva2 -lshlwapi -lwinhttp -lwinmm -liphlpapi -lshcore -lcomdlg32 -limm32 -lsetupapi -lcfgmgr32 -luserenv
// @license         GPL-3.0
// ==/WindhawkMod==

// The code that reaches the taskbar's XAML tree is based on "Taskbar tray
// auto-hide (show on hover)" by m417z, published under the GNU General Public
// License v3.0, so this mod uses the same license.
//
// If you reuse this mod's code, or it inspired your work, credit to "Mac
// Island & Dock by caliberda" with a link to it would be appreciated (and to
// m417z's mod for the taskbar XAML code). This is a request, not a license
// condition.

#include "mac_island/meta/readme.hpp"

#include "mac_island/meta/settings.hpp"

#include "mac_island/foundation/00_prelude.hpp"
#include "mac_island/foundation/01_island_theme.hpp"
#include "mac_island/taskbar/02_taskbar.hpp"
#include "mac_island/taskbar/03_taskbar.hpp"
#include "mac_island/taskbar/04_taskbar.hpp"
#include "mac_island/taskbar/05_taskbar.hpp"
#include "mac_island/taskbar/06_tray.hpp"
#include "mac_island/taskbar/07_taskbar.hpp"
#include "mac_island/services/08_toast.hpp"
#include "mac_island/foundation/09_windows_utils.hpp"
#include "mac_island/i18n/10_i18n_tables.hpp"
#include "mac_island/i18n/11_i18n_tables.hpp"
#include "mac_island/i18n/12_release_notes.hpp"
#include "mac_island/services/13_brightness.hpp"
#include "mac_island/foundation/14_windows_utils.hpp"
#include "mac_island/render/15_eyes.hpp"
#include "mac_island/render/16_text_edit.hpp"
#include "mac_island/render/17_glass.hpp"
#include "mac_island/panels/18_panel_base.hpp"
#include "mac_island/panels/19_panel_base.hpp"
#include "mac_island/panels/20_panel_base.hpp"
#include "mac_island/panels/21_panel_base.hpp"
#include "mac_island/services/22_calendar.hpp"
#include "mac_island/render/23_icons.hpp"
#include "mac_island/render/24_icons.hpp"
#include "mac_island/settings/25_settings.hpp"
#include "mac_island/services/26_calendar.hpp"
#include "mac_island/services/27_calendar.hpp"
#include "mac_island/services/28_komorebi.hpp"
#include "mac_island/settings/29_settings.hpp"
#include "mac_island/settings/30_settings_and_tray.hpp"
#include "mac_island/settings/31_settings.hpp"
#include "mac_island/settings/32_settings.hpp"
#include "mac_island/panels/33_control_panel.hpp"
#include "mac_island/panels/34_notification_panel.hpp"
#include "mac_island/services/35_media.hpp"
#include "mac_island/services/36_power.hpp"
#include "mac_island/services/37_usage.hpp"
#include "mac_island/services/38_shortcuts.hpp"
#include "mac_island/panels/39_timer_tasks.hpp"
#include "mac_island/services/40_commands.hpp"
#include "mac_island/services/41_shortcuts.hpp"
#include "mac_island/services/42_weather.hpp"
#include "mac_island/services/43_lyrics.hpp"
#include "mac_island/services/44_weather.hpp"
#include "mac_island/settings/45_settings.hpp"
#include "mac_island/foundation/46_windows_utils.hpp"
#include "mac_island/services/47_privacy.hpp"
#include "mac_island/services/48_drives.hpp"
#include "mac_island/services/49_recycle_bin.hpp"
#include "mac_island/panels/50_timer_tasks.hpp"
#include "mac_island/services/51_recycle_bin.hpp"
#include "mac_island/services/52_ai_usage.hpp"
#include "mac_island/services/53_usage.hpp"
#include "mac_island/taskbar/54_tray.hpp"
#include "mac_island/services/55_input_method.hpp"
#include "mac_island/services/56_komorebi.hpp"
#include "mac_island/services/57_folders.hpp"
#include "mac_island/services/58_folders_and_taskbar.hpp"
#include "mac_island/services/59_folders.hpp"
#include "mac_island/services/60_folders.hpp"
#include "mac_island/services/61_wallpaper.hpp"
#include "mac_island/island/62_island.hpp"
#include "mac_island/panels/63_control_panel.hpp"
#include "mac_island/panels/64_control_panel.hpp"
#include "mac_island/panels/65_control_panel.hpp"
#include "mac_island/panels/66_control_panel.hpp"
#include "mac_island/panels/67_control_panel.hpp"
#include "mac_island/panels/68_control_panel.hpp"
#include "mac_island/panels/69_control_panel.hpp"
#include "mac_island/services/70_toast.hpp"
#include "mac_island/panels/71_notification_panel.hpp"
#include "mac_island/panels/72_notification_panel.hpp"
#include "mac_island/panels/73_media_panel.hpp"
#include "mac_island/settings/74_settings.hpp"
#include "mac_island/settings/75_settings.hpp"
#include "mac_island/settings/76_profiles.hpp"
#include "mac_island/settings/77_settings.hpp"
#include "mac_island/island/78_island.hpp"
#include "mac_island/island/79_island.hpp"
#include "mac_island/island/80_island.hpp"
#include "mac_island/island/81_island_and_shortcuts.hpp"
#include "mac_island/island/82_island.hpp"
#include "mac_island/island/83_island.hpp"
#include "mac_island/island/84_island.hpp"
#include "mac_island/island/85_island.hpp"
#include "mac_island/island/86_island.hpp"
#include "mac_island/island/87_island.hpp"
#include "mac_island/island/88_island.hpp"
#include "mac_island/island/89_island.hpp"
#include "mac_island/island/90_island.hpp"
#include "mac_island/foundation/91_windows_utils_and_taskbar.hpp"
#include "mac_island/settings/92_settings.hpp"
