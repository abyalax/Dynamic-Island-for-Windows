#!/usr/bin/env python3
"""Static codebase inventory for AI-assisted code review. Standard library only."""
from __future__ import annotations
import argparse, collections, csv, json, re, sys, os
from concurrent.futures import ProcessPoolExecutor, as_completed
from bisect import bisect_right
from pathlib import Path
from datetime import datetime

DEFAULT_EXCLUDED_DIRS = {
    '.git', '.hg', '.svn', '.vs', '.idea', '__pycache__', 'node_modules',
    'bin', 'obj', 'build', 'out', 'dist', 'release', 'debug', 'x64', 'x86',
    '.venv', 'venv', 'vendor', 'third_party', 'packages', 'coverage'
}
DEFAULT_EXTENSIONS = {
    '.cpp'
}
SOURCE_EXTENSIONS = {'.cpp'}
WIN_API_RE = re.compile(r'\b(?:[A-Z][A-Za-z0-9_]*(?:W|A)?|Co[A-Z][A-Za-z0-9_]*|SH[A-Z][A-Za-z0-9_]*)\s*\(')
CALLBACK_RE = re.compile(r'\b(?:WNDPROC|WINAPI|CALLBACK|NTAPI|APIENTRY|LRESULT|HRESULT|DWORD|BOOL|HANDLE|HHOOK|HOOKPROC|LPTHREAD_START_ROUTINE)\b')
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
FUNCTION_RE = re.compile(r'''(?mx)
 ^[ \t]*
 (?:(?:static|inline|virtual|constexpr|extern|friend|explicit|__forceinline|WINAPI|CALLBACK|APIENTRY|NTAPI|consteval|constinit)[ \t]+)*
 (?:[\w:\<\>\~\*&\[\], \t]+?)[ \t]+
 (?P<name>(?:[A-Za-z_~][\w~]*::)*[A-Za-z_~][\w~]*)[ \t]*
 \((?P<args>[^;{}]*?)\)[ \t]*
 (?:const[ \t]*)?(?:noexcept(?:\([^)]*\))?[ \t]*)?(?:override[ \t]*)?(?:final[ \t]*)?
 (?:->[ \t]*[\w:\<\>\*& \t]+[ \t]*)?
 \{
''')
WINAPI_PATTERNS = re.compile(r'\b(CreateWindowExW?|DestroyWindow|DefWindowProcW?|RegisterClassExW?|SetWindowLongPtrW?|GetWindowLongPtrW?|SetWindowsHookExW?|UnhookWindowsHookEx|CallNextHookEx|PostMessageW?|SendMessageW?|PeekMessageW?|GetMessageW?|DispatchMessageW?|CreateThread|_beginthreadex|CreateTimerQueueTimer|SetTimer|KillTimer|CoInitializeEx|CoCreateInstance|Shell_NotifyIconW?|DwmExtendFrameIntoClientArea|DwmSetWindowAttribute|SetWinEventHook|UnhookWinEvent|GetMonitorInfoW?|EnumDisplayMonitors|MonitorFromWindow|GetCursorPos|GetForegroundWindow|OpenProcess|ReadProcessMemory|WriteProcessMemory|SetWinEventHook|RegisterHotKey|UnregisterHotKey|CreateEventW?|SetEvent|WaitForSingleObject|WaitForMultipleObjects|CreateFileW?|DeviceIoControl|LoadLibraryW?|GetProcAddress|VirtualAlloc|VirtualProtect|SetThreadExecutionState|GetSystemMetrics|GetDpiForWindow|AdjustWindowRectExForDpi|SetProcessDpiAwarenessContext|GetDpiForMonitor|SHGetFileInfoW?|SHGetKnownFolderPath|I[A-Z][A-Za-z0-9_]+)\b')
LIFECYCLE_PATTERNS = re.compile(r'\b(?:DllMain|Wh_ModInit|Wh_ModUninit|Wh_ModSettingsChanged|Wh_ModAfterInit|Wh_ModBeforeUninit|WinMain|wWinMain|main|WndProc|WindowProc|OnInit|Initialize|Shutdown|Cleanup|Uninitialize|Start|Stop|WorkerThread|ThreadProc|TimerProc|HookProc|HookCallback|On[A-Z][A-Za-z0-9_]*)\b')
GLOBAL_PATTERN = re.compile(r'(?m)^\s*(?:(?:static|extern|const|constexpr|inline|volatile|thread_local|std::atomic<[^>]+>)\s+)*(?:[A-Za-z_][\w:<>,*&\s]*?)\s+([A-Za-z_][\w]*)\s*(?:=\s*[^;]+)?;\s*(?://.*)?$')

# Minimum chunk size (chars) before we bother splitting a file for parallel processing.
MIN_CHUNK_SIZE = 150_000


def read_text(path: Path) -> str:
    for enc in ('utf-8-sig', 'utf-8', 'cp1252', 'latin-1'):
        try: return path.read_text(encoding=enc)
        except UnicodeDecodeError: pass
    return path.read_text(encoding='utf-8', errors='replace')


def mask_comments_and_strings(text: str) -> str:
    """Replace comments and string/char contents with spaces, preserving newlines and offsets."""
    out = list(text); i = 0; n = len(text); state = 'code'
    while i < n:
        c = text[i]; nxt = text[i+1] if i+1 < n else ''
        if state == 'code':
            if c == '/' and nxt == '/': out[i] = out[i+1] = ' '; i += 2; state = 'line'; continue
            if c == '/' and nxt == '*': out[i] = out[i+1] = ' '; i += 2; state = 'block'; continue
            if c == '"': out[i] = ' '; i += 1; state = 'string'; continue
            if c == "'": out[i] = ' '; i += 1; state = 'char'; continue
            if c == 'R' and nxt == '"':
                m = re.match(r'R"([^ ()\\\t\r\n]{0,16})\(', text[i:])
                if m:
                    token = m.group(0); endtoken = ')' + m.group(1) + '"'
                    for j in range(i, min(i+len(token), n)): out[j] = ' '
                    i += len(token); end = text.find(endtoken, i)
                    if end < 0: end = n - len(endtoken)
                    while i < min(end+len(endtoken), n):
                        if text[i] != '\n': out[i] = ' '
                        i += 1
                    continue
            i += 1; continue
        if state == 'line':
            if c == '\n': state = 'code'
            else: out[i] = ' '
            i += 1; continue
        if state == 'block':
            if c == '*' and nxt == '/': out[i] = out[i+1] = ' '; i += 2; state = 'code'; continue
            if c != '\n': out[i] = ' '
            i += 1; continue
        if state in ('string', 'char'):
            if c == '\\':
                if c != '\n': out[i] = ' '
                if i+1 < n and text[i+1] != '\n': out[i+1] = ' '
                i += 2; continue
            if (state == 'string' and c == '"') or (state == 'char' and c == "'"):
                out[i] = ' '; i += 1; state = 'code'; continue
            if c != '\n': out[i] = ' '
            i += 1
    return ''.join(out)


def line_of(text: str, offset: int) -> int:
    return text.count('\n', 0, offset) + 1


def find_chunk_boundaries(masked: str, n_chunks: int):
    """Split a masked source string into n_chunks at newline positions where brace
    depth is 0, so each chunk can be function-extracted independently. Falls back
    to a single chunk if the file is too small or has no safe split points."""
    length = len(masked)
    if n_chunks <= 1 or length < MIN_CHUNK_SIZE:
        return [(0, length)]
    target = length // n_chunks
    depth = 0
    cuts = [0]
    next_target = target
    for i, ch in enumerate(masked):
        if ch == '{': depth += 1
        elif ch == '}': depth -= 1
        elif ch == '\n' and depth <= 0 and i >= next_target:
            cuts.append(i + 1)
            next_target = i + 1 + target
    cuts.append(length)
    cuts = sorted(set(cuts))
    return list(zip(cuts[:-1], cuts[1:]))


def extract_functions(text: str, masked: str, rel: str):
    """Heuristic function extraction with one-pass brace matching."""
    found = []
    line_starts = [0]
    line_starts.extend(i + 1 for i, ch in enumerate(text) if ch == '\n')

    brace_stack = []
    brace_end = {}
    for pos, ch in enumerate(masked):
        if ch == '{':
            brace_stack.append(pos)
        elif ch == '}' and brace_stack:
            brace_end[brace_stack.pop()] = pos + 1

    for m in FUNCTION_RE.finditer(masked):
        name = m.group('name')
        if name in {'if', 'for', 'while', 'switch', 'catch'}:
            continue
        brace = m.end() - 1
        end = brace_end.get(brace)
        if end is None:
            continue
        header_start = max(masked.rfind('\n', 0, m.start() - 1) + 1, 0)
        header = ' '.join(text[header_start:brace].strip().split())
        body_masked = masked[brace + 1:end - 1]
        calls = sorted(set(re.findall(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(', body_masked)))
        found.append({
            'name': name, 'qualified_name': name, 'file': rel,
            'line_start': bisect_right(line_starts, m.start()),
            'line_end': bisect_right(line_starts, end),
            'signature': header[:500], 'calls': calls,
            'body_lines': max(1, bisect_right(line_starts, end) - bisect_right(line_starts, brace))
        })
    return found


def extract_windhawk_settings(text: str, rel: str):
    results = []
    m = re.search(r'//\s*==WindhawkModSettings==\s*/\*(.*?)\*/', text, re.S)
    if not m: return results
    body = m.group(1)
    for idx, line in enumerate(body.splitlines(), 1):
        mm = re.match(r'^\s*(?:-\s*)?([A-Za-z_$][\w$-]*)\s*:\s*(.*?)\s*$', line)
        if mm and not mm.group(1).startswith('$') and mm.group(1) not in {'name','description','options'}:
            results.append({'setting': mm.group(1), 'default': mm.group(2), 'file': rel,
                            'line': line_of(text, m.start(1)) + idx - 1})
    return results


def process_chunk(job):
    """Runs in a worker process: does every CPU-heavy scan for one chunk of one file
    in a single call, so chunk text is only pickled across the process boundary once."""
    text_chunk, masked_chunk, rel, line_offset, is_first_chunk = job

    functions = extract_functions(text_chunk, masked_chunk, rel)
    for f in functions:
        f['line_start'] += line_offset
        f['line_end'] += line_offset

    winapi_rows = [
        {'symbol': m.group(0), 'file': rel, 'line': line_of(text_chunk, m.start()) + line_offset}
        for m in WINAPI_PATTERNS.finditer(masked_chunk)
    ]
    lifecycle_rows = [
        {'symbol': m.group(0), 'file': rel, 'line': line_of(text_chunk, m.start()) + line_offset,
         'context': ' '.join(text_chunk[max(0, m.start()-100):m.end()+140].split())[:260]}
        for m in LIFECYCLE_PATTERNS.finditer(masked_chunk)
    ]
    variable_rows = []
    for m in GLOBAL_PATTERN.finditer(masked_chunk):
        name = m.group(1)
        if name in {'return','if','while','switch','case','typedef','using','include','define'}: continue
        if '::' in m.group(0) and not any(x in m.group(0) for x in ('static ', 'extern ', 'constexpr ', 'const ')): continue
        variable_rows.append({'name': name, 'file': rel, 'line': line_of(text_chunk, m.start()) + line_offset,
                              'declaration': ' '.join(text_chunk[m.start():m.end()].strip().split())[:240]})
    include_rows = []
    settings_rows = []
    if is_first_chunk:
        for inc in INCLUDE_RE.findall(text_chunk):
            include_rows.append({'file': rel, 'include': inc,
                                  'kind': 'system' if inc.startswith(('windows.h','winuser.h','unknwn.h','dwmapi.h','shellapi.h','shlobj.h','processthreadsapi.h','commctrl.h','d2d1.h','dwrite.h','wrl/')) else 'local_or_external'})
        settings_rows = extract_windhawk_settings(text_chunk, rel)

    return functions, winapi_rows, lifecycle_rows, variable_rows, include_rows, settings_rows


def main():
    ap = argparse.ArgumentParser(description='Inventory a codebase and generate AI-friendly architecture reports.')
    ap.add_argument('root', nargs='?', default='.', help='Repository root (default: current directory)')
    ap.add_argument('-o', '--output', default='codebase-analysis', help='Output directory, relative to root unless absolute')
    ap.add_argument('--all-files', action='store_true', help='Include files with extensions outside the default list')
    ap.add_argument('--max-file-size-mb', type=float, default=5.0, help='Skip text files larger than this size (default: 5 MB)')
    ap.add_argument('--workers', type=int, default=(os.cpu_count() or 1), help='Worker processes (default: all logical CPU cores)')
    args = ap.parse_args()
    if args.workers < 1:
        print('ERROR: --workers must be at least 1.', file=sys.stderr); return 2
    root = Path(args.root).expanduser().resolve()
    if not root.is_dir():
        print(f'ERROR: Repository root does not exist: {root}', file=sys.stderr); return 2
    output = Path(args.output).expanduser()
    if not output.is_absolute(): output = root / output
    output = output.resolve()
    excluded = set(DEFAULT_EXCLUDED_DIRS)
    files = []
    for p in root.rglob('*'):
        if not p.is_file(): continue
        try: rel = p.relative_to(root)
        except ValueError: continue
        if output == p or output in p.parents: continue
        if any(part.lower() in excluded for part in rel.parts[:-1]): continue
        if p.suffix.lower() not in DEFAULT_EXTENSIONS and not args.all_files: continue
        try:
            if p.stat().st_size > args.max_file_size_mb * 1024 * 1024: continue
        except OSError: continue
        files.append((p, rel.as_posix()))
    files.sort(key=lambda x: x[1].lower())
    output.mkdir(parents=True, exist_ok=True)
    print(f'Scanning {len(files)} files with up to {max(1, args.workers)} worker process(es)...', flush=True)

    file_rows = []
    extension_counts = collections.Counter(); dir_counts = collections.Counter()
    total_lines = 0; blank_lines = 0

    # Build chunk jobs up front across ALL source files, so one giant file is split
    # into several chunks and still uses every worker, same as many small files would.
    jobs = []
    for p, rel in files:
        try: text = read_text(p)
        except Exception as e:
            file_rows.append({'file': rel, 'error': str(e)}); continue
        lines = text.splitlines(); loc = sum(1 for x in lines if x.strip()); blank = sum(1 for x in lines if not x.strip())
        ext = p.suffix.lower() or '[no extension]'; extension_counts[ext] += 1
        parent = str(Path(rel).parent); dir_counts[parent] += 1
        total_lines += len(lines); blank_lines += blank
        is_source = p.suffix.lower() in SOURCE_EXTENSIONS
        file_rows.append({'file': rel, 'extension': ext, 'bytes': p.stat().st_size, 'lines_total': len(lines),
                           'lines_nonblank': loc, 'lines_blank': blank, 'is_source': is_source})
        if not is_source:
            continue
        masked = mask_comments_and_strings(text)
        n_chunks = max(1, min(args.workers, len(masked) // MIN_CHUNK_SIZE)) if args.workers > 1 else 1
        boundaries = find_chunk_boundaries(masked, n_chunks)
        for idx, (start, end) in enumerate(boundaries):
            line_offset = text.count('\n', 0, start)
            jobs.append((text[start:end], masked[start:end], rel, line_offset, idx == 0))

    all_functions = []; winapi_rows = []; lifecycle_rows = []; variable_rows = []
    include_rows = []; all_settings = []

    if jobs:
        workers = max(1, min(args.workers, len(jobs)))
        if workers == 1:
            for job in jobs:
                f, w, l, v, inc, s = process_chunk(job)
                all_functions += f; winapi_rows += w; lifecycle_rows += l
                variable_rows += v; include_rows += inc; all_settings += s
        else:
            print(f'Processing {len(jobs)} chunk(s) across {len(files)} file(s) using {workers} worker process(es)...', flush=True)
            with ProcessPoolExecutor(max_workers=workers) as pool:
                futures = [pool.submit(process_chunk, job) for job in jobs]
                completed = 0
                for future in as_completed(futures):
                    f, w, l, v, inc, s = future.result()
                    all_functions += f; winapi_rows += w; lifecycle_rows += l
                    variable_rows += v; include_rows += inc; all_settings += s
                    completed += 1
                    if completed == len(futures) or completed % 5 == 0:
                        print(f'Chunk processing: {completed}/{len(futures)}', flush=True)

    # Heuristic internal call graph: map unqualified function names to definitions.
    name_to_functions = collections.defaultdict(list)
    for f in all_functions: name_to_functions[f['name'].split('::')[-1]].append(f)
    for f in all_functions:
        f['calls_defined_in_repo'] = sorted({c for c in f['calls'] if c in name_to_functions and c != f['name'].split('::')[-1]})
    incoming = collections.Counter(c for f in all_functions for c in f['calls_defined_in_repo'])
    for f in all_functions: f['incoming_call_count_heuristic'] = incoming[f['name'].split('::')[-1]]

    api_counts = collections.Counter(x['symbol'] for x in winapi_rows)
    lifecycle_counts = collections.Counter(x['symbol'] for x in lifecycle_rows)
    includes_counts = collections.Counter(x['include'] for x in include_rows)
    largest = sorted(file_rows, key=lambda x: x.get('lines_total', 0), reverse=True)[:30]
    largest_functions = sorted(all_functions, key=lambda x: x['body_lines'], reverse=True)[:50]
    roots = [x for x in all_functions if x['name'].split('::')[-1] in {'Wh_ModInit','Wh_ModUninit','Wh_ModSettingsChanged','Wh_ModAfterInit','Wh_ModBeforeUninit','DllMain','WinMain','wWinMain','main'}]

    report = []
    report += ['# Codebase Analysis Report','',f'- Root: `{root}`',f'- Generated: {datetime.now().astimezone().isoformat(timespec="seconds")}',f'- Files analyzed: **{len(file_rows)}**',f'- Total physical lines: **{total_lines:,}**',f'- Nonblank lines: **{total_lines-blank_lines:,}**',f'- Source files (`.c/.cpp/.h` family): **{sum(1 for x in file_rows if x.get("is_source"))}**',f'- Function definitions detected (heuristic): **{len(all_functions)}**','', '> Static-analysis note: function, variable, API, and call-graph results are heuristic inventories, not compiler AST output. Validate ambiguous matches against source context.','']
    report += ['## 1. Repository Structure','', '| Extension | Files |','|---|---:|'] + [f'| `{k}` | {v} |' for k,v in extension_counts.most_common()] + ['', '### Largest files','', '| File | Lines | Size (bytes) |','|---|---:|---:|'] + [f'| `{x["file"]}` | {x.get("lines_total",0):,} | {x.get("bytes",0):,} |' for x in largest] + ['']
    report += ['## 2. Entry Points and Lifecycle Candidates','']
    if roots:
        report += ['| Symbol | File | Line |','|---|---|---:|'] + [f'| `{x["name"]}` | `{x["file"]}` | {x["line_start"]} |' for x in roots]
    else: report += ['No common entry-point name was detected. Inspect `lifecycle-symbols.csv`.']
    report += ['', '### Lifecycle-related symbol occurrences','', '| Symbol | Occurrences |','|---|---:|'] + [f'| `{k}` | {v} |' for k,v in lifecycle_counts.most_common(80)] + ['']
    report += ['## 3. Configurable Settings and Variable Candidates','', f'- Windhawk setting candidates: **{len(all_settings)}**', f'- Global/declaration candidates: **{len(variable_rows)}**', '', '### Windhawk settings detected','']
    if all_settings: report += ['| Setting | Default | File | Line |','|---|---|---|---:|'] + [f'| `{x["setting"]}` | `{x["default"]}` | `{x["file"]}` | {x["line"]} |' for x in all_settings[:300]]
    else: report += ['No settings block was detected by the current heuristic.']
    report += ['', 'Variable candidates are exported to `variable-candidates.csv`; inspect each declaration before classifying it as global/configuration/state.','']
    report += ['## 4. Function Inventory','', f'Function definitions detected: **{len(all_functions)}**', '', '### Largest functions by estimated body span','', '| Function | File | Lines | Incoming calls (heuristic) |','|---|---|---:|---:|'] + [f'| `{x["name"]}` | `{x["file"]}` | {x["body_lines"]} | {x["incoming_call_count_heuristic"]} |' for x in largest_functions] + ['', 'Full index: `functions.json` and `functions.csv`.','']
    report += ['## 5. Windows API and Internal-System Interaction','', f'- API symbol occurrences matched: **{len(winapi_rows)}**', '', '| API / symbol | Occurrences |','|---|---:|'] + [f'| `{k}` | {v} |' for k,v in api_counts.most_common(150)] + ['', 'Every occurrence with file and line is in `windows-api-usage.csv`. Matching is pattern-based; it does not prove that every call is a direct Windows API call.','']
    report += ['## 6. Include and Dependency Inventory','', '| Include | Occurrences |','|---|---:|'] + [f'| `{k}` | {v} |' for k,v in includes_counts.most_common(150)] + ['', 'Full include list: `includes.csv`. Local/external classification is approximate.','']
    report += ['## 7. Heuristic Internal Call Graph','', 'Use `functions.json` and the `calls_defined_in_repo` field to inspect likely internal call relationships. Name-based matching can overcount overloaded functions or miss indirect calls, callbacks, function pointers, macros, and virtual dispatch.','']
    report += ['## 8. Recommended AI-Agent Reading Order','', '1. Read this report and `repository-files.csv` to understand the repository shape.', '2. Read the mod metadata and settings block, then trace the primary initialization and teardown functions.', '3. Follow the entry-point call graph into window creation, event/message dispatch, hooks, timers, worker threads, and cleanup.', '4. For a feature task, use `functions.csv` to locate relevant functions and inspect only those functions plus their callers/callees.', '5. Use `windows-api-usage.csv` to find system-boundary code and `includes.csv` to identify dependencies.', '6. Before edits, ask the agent to state the exact files/functions it intends to change and the expected lifecycle impact.', '', '## Output Files','', '- `README-ANALYSIS.md` — this report', '- `repository-files.csv` — file inventory and line counts', '- `functions.csv` / `functions.json` — function index and heuristic call relationships', '- `windhawk-settings.csv` — detected Windhawk settings', '- `variable-candidates.csv` — global/declaration candidates', '- `windows-api-usage.csv` — matched Windows/system API occurrences', '- `lifecycle-symbols.csv` — lifecycle/event-related symbol occurrences', '- `includes.csv` — include/dependency inventory', '- `analysis-summary.json` — machine-readable summary', '']
    (output/'README-ANALYSIS.md').write_text('\n'.join(report), encoding='utf-8')

    def write_csv(name, rows):
        if not rows: (output/name).write_text('', encoding='utf-8'); return
        fields = []
        for row in rows:
            for k in row:
                if k not in fields: fields.append(k)
        with (output/name).open('w', newline='', encoding='utf-8-sig') as f:
            w = csv.DictWriter(f, fieldnames=fields, extrasaction='ignore'); w.writeheader(); w.writerows(rows)

    write_csv('repository-files.csv', file_rows)
    write_csv('functions.csv', all_functions)
    write_csv('windhawk-settings.csv', all_settings)
    write_csv('variable-candidates.csv', variable_rows)
    write_csv('windows-api-usage.csv', winapi_rows)
    write_csv('lifecycle-symbols.csv', lifecycle_rows)
    write_csv('includes.csv', include_rows)
    (output/'functions.json').write_text(json.dumps(all_functions, indent=2, ensure_ascii=False), encoding='utf-8')
    summary = {
        'root': str(root), 'output': str(output), 'generated_at': datetime.now().astimezone().isoformat(timespec='seconds'),
        'files_analyzed': len(file_rows), 'total_physical_lines': total_lines, 'nonblank_lines': total_lines-blank_lines,
        'source_files': sum(1 for x in file_rows if x.get('is_source')), 'functions_detected_heuristically': len(all_functions),
        'windhawk_settings_detected': len(all_settings), 'variable_candidates': len(variable_rows),
        'windows_api_occurrences': len(winapi_rows), 'includes': len(include_rows),
        'top_api_symbols': api_counts.most_common(30), 'top_includes': includes_counts.most_common(30),
        'limitations': ['Regex-based static analysis, not a compiler AST.','Indirect calls, macros, templates, lambdas, overloaded methods, and callbacks may be missed or misclassified.','API-name matches require manual verification.']
    }
    (output/'analysis-summary.json').write_text(json.dumps(summary, indent=2, ensure_ascii=False), encoding='utf-8')
    print('Codebase analysis completed.')
    print(f'Repository: {root}')
    print(f'Files analyzed: {len(file_rows)}')
    print(f'Physical lines: {total_lines:,}')
    print(f'Function definitions detected (heuristic): {len(all_functions)}')
    print(f'Windhawk settings detected: {len(all_settings)}')
    print(f'Windows API occurrences matched: {len(winapi_rows)}')
    print(f'Reports: {output}')
    print(f'Start here: {output / "README-ANALYSIS.md"}')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())