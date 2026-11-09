#!/usr/bin/env python3
"""Plan the modules of the mac-island mod from its top-level units.

Order is never changed: a module is a contiguous range of the original file,
so including the modules in order rebuilds the original text exactly.
Usage: plan_modules.py <source.cpp> <units.json> <plan.json>
"""
import json
import re
import sys
import collections

# (domain, directory, regex over the unit's name). First match wins.
FAMILIES = [
    ("i18n_tables", "i18n", r"^k(Chinese|German|Spanish|Russian|French|Japanese|Korean|Italian|Turkish|Arabic|Polish|Dutch|Ukrainian|Vietnamese|Indonesian)Texts$|^TextLanguage$"),
    ("release_notes", "i18n", r"ReleaseNotes|kReleaseNotes"),
    ("text_helpers", "i18n", r"^(IslandTextLanguage|DateLocale|MonthDayPicture|TableText|Tr)$"),
    ("komorebi", "services", r"Komorebi"),
    ("weather", "services", r"Weather|^Located$|LocateMe|HttpsGetJson|UrlEncode|UsesImperial|UsesFahrenheit|WeatherPanel"),
    ("lyrics", "services", r"Lyric"),
    ("calendar", "services", r"Calendar|Agenda|^Ics|Holiday|DaysInMonth|DayOfWeek|IsoWeek|YearMonthTitle|LoadEvents|SaveEvents"),
    ("eyes", "render", r"Eye|Eyes|SoftTriangle"),
    ("bluetooth", "services", r"Bluetooth"),
    ("brightness", "services", r"Brightness|NightLight|SdrWhite|AdvancedColorInfo"),
    ("wifi", "services", r"Wlan|Wifi|Network"),
    ("usage", "services", r"^Usage|Pdh|Kmt|GpuName|CountCores|GetUsage|UsagePanel"),
    ("ai_usage", "services", r"^Ai[A-Z]|ClaudeUsage|AiStorage"),
    ("privacy", "services", r"Privacy|Microphone|Camera|IMFSensor|ConsentInUse|AppsUsing"),
    ("drives", "services", r"RemovableDrive|DriveJob|EjectJob|DrivesPanel|UsbDrive"),
    ("recycle_bin", "services", r"RecycleBin"),
    ("wallpaper", "services", r"Wallpaper"),
    ("folders", "services", r"Folder|ShelfPanel|^Shelf"),
    ("desktops", "services", r"VirtualDesktops|DesktopsJob|DesktopsPanel"),
    ("commands", "services", r"UserCommand|CommandsPanel|CommandOpen|RunUserCommand|PickCommand|CommandGlyph|CommandIconPath|SaveCommands|^Commands$"),
    ("shortcuts", "services", r"Shortcut"),
    ("timer_tasks", "panels", r"TimerPanel|TasksPanel|NotesPanel|AlarmDue"),
    ("media", "services", r"^Media|AudioOutput|MixerApp|Mixer|Volume"),
    ("power", "services", r"^Power|Shutdown|FastStartup|RunPowerAction"),
    ("profiles", "settings", r"Profile"),
    ("toast", "services", r"Toast|Notification|INotificationActivation|Banner"),
    ("windows_banner", "services", r"WindowsBanner"),
    ("input_method", "services", r"InputMethod|KeyboardPanel|KeyboardEntry|Ime"),
    ("island_theme", "foundation", r"Theme|CustomColor|StyledProperty|RgbColor|^Fg|^Bg|AnimationChoice|AttentionMode|UsesWindowColor|LoadCustomColors"),
    ("ipc_json", "foundation", r"^Ipc|^Json|Sqlite|Ipc(Reader|Writer)"),
    ("logging", "foundation", r"KeptLog|IslandLog|Log"),
    ("tray", "taskbar", r"^Tray|NotifyIcon|TrayListPanel"),
    ("taskbar", "taskbar", r"Thumbnail|Taskbar|^Edge|CTask|HideTimerScope|WindowInTheWay|DockShelf|DockIcon|OverlayIcon|BigIcon|AttentionRequest|RecordAttention|RunAttention|ProgressBarWatch|AppWindowSearch|FindDockIcon|FindAllByClassName|RunFromWindowThread|IconPlacement"),
    ("glass", "render", r"Glass|Compositor|GraphicsEffect|GeometrySource|LayeredCanvas|WrappedField"),
    ("icons", "render", r"^Icon|IconPack|IconSource|IconPicture|IconTarget|IconFiles|OnlineIconPack|DrawInstagram|ReleaseIconPictures|SaveWindowsIcons|ReadIconPicture"),
    ("text_edit", "render", r"TextEdit|EditDrag|EditText|LayOutEdit|DrawTextSelection|KeepOneLine|DrawLineEdit|WrappedField"),
    ("settings", "settings", r"Setting|Choice|IslandPref|PillGroup|PillAction|AutoProfile|Setup|SettingsWindow|IslandSlider|ControlId|ControlKind|ControlInfo"),
    ("windows_utils", "foundation", r"Wait|Frame|Subclass|Window|Process|Path|String|Pump|Thread"),
]

# Panels and the island get their own domain by class prefix.
MEMBER_DOMAINS = [
    (r"^Island$|^Island::", "island", "island"),
    (r"^ControlPanel(::|$)", "control_panel", "panels"),
    (r"^NotificationPanel(::|$)", "notification_panel", "panels"),
    (r"^MediaPanel(::|$)", "media_panel", "panels"),
    (r"^CalendarPanel(::|$)", "calendar", "services"),
    (r"^FoldersPanel(::|$)", "folders", "services"),
    (r"^TrayListPanel(::|$)", "tray", "taskbar"),
    (r"^(Panel|GlassLayer)(::|$)", "panel_base", "panels"),
]


def label(name, kind):
    for pat, domain, directory in MEMBER_DOMAINS:
        if re.search(pat, name):
            return domain, directory
    for domain, directory, pat in FAMILIES:
        if re.search(pat, name):
            return domain, directory
    return None


def plan(units, total_lines, first_code_line, min_lines=250, max_lines=3600):
    labelled = []
    prev = None
    for u in units:
        lab = label(u["name"], u["kind"])
        if lab is None:
            lab = prev if prev else ("foundation_core", "foundation")
        else:
            prev = lab
        u["_lab"] = lab
        labelled.append((u, lab))
    # runs of one domain
    runs = []
    for u, lab in labelled:
        if runs and runs[-1]["domain"] == lab[0]:
            runs[-1]["units"].append(u)
        else:
            runs.append({"domain": lab[0], "dir": lab[1], "units": [u]})
    # merge tiny runs into the previous one (or the next for the first)
    def size(r):
        return r["units"][-1]["end"] - r["units"][0]["start"] + 1
    changed = True
    while changed:
        changed = False
        for i, r in enumerate(runs):
            if size(r) < min_lines and len(runs) > 1:
                j = i - 1 if i > 0 else i + 1
                tgt = runs[j]
                if j < i:
                    tgt["units"].extend(r["units"])
                else:
                    r["units"].extend(tgt["units"])
                    tgt = r
                    runs[j] = tgt
                del runs[i if j < i else i]
                changed = True
                break
    # split long runs
    modules = []
    for r in runs:
        total = size(r)
        parts = max(1, round(total / max_lines + 0.49))
        if total <= max_lines:
            parts = 1
        target = total / parts
        chunk, chunk_start = [], r["units"][0]["start"]
        count = 0
        for idx, u in enumerate(r["units"]):
            chunk.append(u)
            used = u["end"] - chunk_start + 1
            if used >= target and count < parts - 1 and idx < len(r["units"]) - 1:
                modules.append({"domain": r["domain"], "dir": r["dir"], "units": chunk})
                chunk, chunk_start = [], r["units"][idx + 1]["start"]
                count += 1
        if chunk:
            modules.append({"domain": r["domain"], "dir": r["dir"], "units": chunk})
    return modules


if __name__ == "__main__":
    src, units_json, out = sys.argv[1:4]
    units = json.load(open(units_json))
    lines = open(src, encoding="utf-8").read().split("\n")
    mods = plan(units, len(lines), units[0]["start"])
    def dominant(m):
        hist = collections.Counter()
        dirs = {}
        for u in m["units"]:
            dom, d = u["_lab"]
            hist[dom] += u["end"] - u["start"] + 1
            dirs[dom] = d
        total = sum(hist.values())
        top = hist.most_common(2)
        name = top[0][0]
        if len(top) > 1 and top[1][1] >= 0.30 * total:
            name = top[0][0] + "_and_" + top[1][0]
        return name, dirs[top[0][0]]
    named = []
    for m in mods:
        name, d = dominant(m)
        named.append((name, d, m))
    seen = collections.Counter((d, n) for n, d, _ in named)
    idx = collections.Counter()
    res = []
    for name, d, m in named:
        idx[(d, name)] += 1
        suffix = "" if seen[(d, name)] == 1 else "_%d" % idx[(d, name)]
        res.append({
            "file": "%s/%s%s.hpp" % (d, name, suffix),
            "domain": name,
            "start": m["units"][0]["start"],
            "end": m["units"][-1]["end"],
            "units": len(m["units"]),
        })
    json.dump(res, open(out, "w"), indent=1)
    for r in res:
        print("%-40s %6d-%-6d %5d lines %4d units" % (r["file"], r["start"], r["end"], r["end"] - r["start"] + 1, r["units"]))
    print(len(res), "modules")
