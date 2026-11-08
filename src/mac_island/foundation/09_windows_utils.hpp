// The windows the kind was worked out for, and it (see PublishProgress).
std::vector<HWND> g_progressKindWindows;
int g_progressKind = 0;
// Only browsers' windows (their next bit): they never tell a download paused
// or failed, so the island takes one standing still as paused (see
// islandDownloadStalled).
constexpr int kProgressFromBrowsers = 1 << 4;
bool g_progressBrowsers = false;
// TBPF_* (shobjidl's): none, going on without a value, normal, error,
// paused.
constexpr int kProgressIndeterminate = 1;
constexpr int kProgressNormal = 2;
constexpr int kProgressError = 4;
constexpr int kProgressPaused = 8;
// To the island (posted: nothing waits): wParam the state, lParam per mille.
constexpr UINT WM_APP_PROGRESS = WM_APP + 98;

bool IsAppWindow(HWND hWnd);

// Whether the windows with progress are Windows' file copies: every one
// Explorer's (its copy dialog, or the folder window the taskbar shows it on:
// its class isn't the dialog's). Another app's window without a taskbar
// button doesn't count: a state it never cleared.
bool ProgressFromCopies(const std::vector<std::pair<HWND, int>>& windows) {
    bool copies = false;
    for (const auto& window : windows) {
        DWORD processId = 0;
        GetWindowThreadProcessId(window.first, &processId);
        if (ProcessIsOneOf(processId, {L"explorer.exe"})) {
            copies = true;
        } else if (IsAppWindow(window.first)) {
            return false;
        }
    }
    return copies;
}

// Whether every window with progress is a browser's (Edge, Chrome, Firefox,
// Brave, Opera, Vivaldi).
bool ProgressFromBrowsers(const std::vector<std::pair<HWND, int>>& windows) {
    if (windows.empty()) {
        return false;
    }
    for (const auto& window : windows) {
        DWORD processId = 0;
        GetWindowThreadProcessId(window.first, &processId);
        if (!ProcessIsOneOf(processId,
                            {L"msedge.exe", L"chrome.exe", L"firefox.exe",
                             L"brave.exe", L"opera.exe", L"vivaldi.exe"})) {
            return false;
        }
    }
    return true;
}

// A bar's state by its look: the visual state its template is in
// (Determinate, Updating, Indeterminate, Error, Paused, IndeterminateError,
// IndeterminatePaused), normal when it can't be read.
int ProgressBarState(Controls::Primitives::RangeBase const& bar) {
    if (Media::VisualTreeHelper::GetChildrenCount(bar) < 1) {
        return kProgressNormal;
    }
    auto root =
        Media::VisualTreeHelper::GetChild(bar, 0).try_as<FrameworkElement>();
    if (!root) {
        return kProgressNormal;
    }
    for (const auto& group : VisualStateManager::GetVisualStateGroups(root)) {
        const auto state = group.CurrentState();
        if (group.Name() != L"CommonStates" || !state) {
            continue;
        }
        const std::wstring name = state.Name().c_str();
        return name.find(L"Error") != std::wstring::npos    ? kProgressError
               : name.find(L"Paused") != std::wstring::npos ? kProgressPaused
               : name.find(L"Indeterminate") != std::wstring::npos
                   ? kProgressIndeterminate
                   : kProgressNormal;
    }
    return kProgressNormal;
}

// The one furthest behind (an error first, then paused, normal and going
// on without a value), to the island when it changed (`force`: anyway,
// when it starts).
void PublishProgress(bool force) {
    if (!g_dockIsMain || g_unloading || !g_progressWanted) {
        return;
    }
    auto rank = [](int state) {
        return state == kProgressError    ? 4
               : state == kProgressPaused ? 3
               : state == kProgressNormal ? 2
               : state == kProgressIndeterminate ? 1
                                                 : 0;
    };
    int state = 0;
    int value = 1000;
    auto add = [&](int barState, int permille) {
        if (rank(barState) > rank(state)) {
            state = barState;
            value = permille;
        } else if (rank(barState) == rank(state)) {
            value = std::min(value, permille);
        }
    };
    std::erase_if(g_progressBars, [](const ProgressBarWatch& watch) {
        return !watch.bar.get();
    });
    for (auto& watch : g_progressBars) {
        auto bar = watch.bar.get();
        // (Shown only while its app has progress.)
        if (bar.Visibility() != Visibility::Visible || bar.Opacity() < 0.01 ||
            bar.ActualWidth() < 1) {
            continue;
        }
        // (Its template's state, once it has one.)
        if (!watch.stateChanged &&
            Media::VisualTreeHelper::GetChildrenCount(bar) > 0) {
            if (auto root = Media::VisualTreeHelper::GetChild(bar, 0)
                                .try_as<FrameworkElement>()) {
                for (const auto& group :
                     VisualStateManager::GetVisualStateGroups(root)) {
                    if (group.Name() == L"CommonStates") {
                        watch.stateChanged = group.CurrentStateChanged(
                            winrt::auto_revoke,
                            [](winrt::Windows::Foundation::IInspectable const&,
                               VisualStateChangedEventArgs const&) {
                                Safely(L"Following a progress bar", [] {
                                    PublishProgress(false);
                                });
                            });
                    }
                }
            }
        }
        const double range = bar.Maximum() - bar.Minimum();
        const int permille =
            range > 0 ? (int)std::clamp(std::lround((bar.Value() -
                                                     bar.Minimum()) *
                                                    1000 / range),
                                        0L, 1000L)
                      : 0;
        add(ProgressBarState(bar), permille);
    }
    // (Only what the taskbar's buttons show: a state told and never
    // cleared, or told by a hidden window, isn't a progress.)
    std::erase_if(g_windowProgress, [](const auto& window) {
        return !IsWindow(window.first) || window.second == 0;
    });
    if (state == 0) {
        value = 0;
    }
    // (Worked out again only when other windows tell one: it opens their
    // processes.)
    std::vector<HWND> windows;
    for (const auto& window : g_windowProgress) {
        windows.push_back(window.first);
    }
    if (windows != g_progressKindWindows) {
        g_progressKindWindows = std::move(windows);
        g_progressKind =
            ProgressFromCopies(g_windowProgress) ? kProgressKindCopy : 0;
        g_progressBrowsers = ProgressFromBrowsers(g_windowProgress);
    }
    const int kind =
        g_progressKind | (g_progressBrowsers ? kProgressFromBrowsers : 0);
    // A new one shows once it has lasted a few seconds (an app showing one
    // for a moment, starting or restoring itself, doesn't fill the pill);
    // Windows' file copies at once (see RecheckProgress).
    const double now = NowSeconds();
    if (state == 0) {
        g_progressSeenAt = 0;
    } else if (!g_progressSeenAt) {
        g_progressSeenAt = now;
    }
    if (state != 0 && kind != kProgressKindCopy &&
        now - g_progressSeenAt < kProgressSureSeconds) {
        state = 0;
        value = 0;
    }
    if (!force && state == g_progressSentState && kind == g_progressSentKind &&
        std::abs(value - g_progressSentValue) < 2) {
        return;
    }
    g_progressSentState = state;
    g_progressSentValue = value;
    g_progressSentKind = kind;
    if (HWND island = FindIslandWnd()) {
        PostMessage(island, WM_APP_PROGRESS, (WPARAM)(state | kind << 8),
                    (LPARAM)value);
    }
}

// A taskbar button's bar watched (its value, whether it shows), once: it
// may come only with the app's first progress, so looked for again then.
void WatchProgressBar(FrameworkElement const& button) {
    if (!g_progressWanted) {
        return;
    }
    auto element = FindByName(button, L"ProgressIndicator");
    auto bar = element ? element.try_as<Controls::Primitives::RangeBase>()
                       : nullptr;
    if (!bar || std::any_of(g_progressBars.begin(), g_progressBars.end(),
                            [&](const ProgressBarWatch& watch) {
                                return watch.bar.get() == bar;
                            })) {
        return;
    }
    ProgressBarWatch watch;
    watch.bar = winrt::make_weak(bar);
    watch.valueChanged = bar.ValueChanged(
        winrt::auto_revoke,
        [](winrt::Windows::Foundation::IInspectable const&,
           Controls::Primitives::RangeBaseValueChangedEventArgs const&) {
            Safely(L"Following a progress bar",
                   [] { PublishProgress(false); });
        });
    watch.visibilityToken = bar.RegisterPropertyChangedCallback(
        UIElement::VisibilityProperty(),
        [](DependencyObject const&, DependencyProperty const&) {
            Safely(L"Following a progress bar",
                   [] { PublishProgress(false); });
        });
    g_progressBars.push_back(std::move(watch));
}

// The bars let go (the dock reset, or the mod going).
void StopWatchingProgressBars() {
    for (auto& watch : g_progressBars) {
        if (auto bar = watch.bar.get()) {
            Safely(L"Releasing a progress bar watch", [&] {
                bar.UnregisterPropertyChangedCallback(
                    UIElement::VisibilityProperty(), watch.visibilityToken);
            });
        }
    }
    std::vector<ProgressBarWatch>().swap(g_progressBars);
}

// The buttons' bars looked for (a new one) and read.
void LookAtProgressBars() {
    for (const auto& weak : g_watchedButtons) {
        if (auto button = weak.get()) {
            WatchProgressBar(button);
        }
    }
    PublishProgress(false);
}

// A window's progress changed as the taskbar was told: its state kept; for
// a new window or another state, the buttons' bars looked for (a new one)
// and read. A value alone comes through its bar's own handler once one is
// shown: looked for then at most every kProgressLookSeconds; before (a new
// window's bar comes a moment after its first values), each time.
// RecheckProgress also looks each second.
constexpr double kProgressLookSeconds = 0.5;
double g_progressLookedAt = 0;

void OnWindowProgress(HWND hWnd, int state) {
    if (g_unloading) {
        return;
    }
    // (Windows gone, or whose progress ended, let go here too: with no
    // island filling, PublishProgress doesn't run.)
    std::erase_if(g_windowProgress, [](const auto& window) {
        return !IsWindow(window.first) || window.second == 0;
    });
    bool changed = false;
    if (state >= 0) {
        auto it = std::find_if(
            g_windowProgress.begin(), g_windowProgress.end(),
            [&](const auto& window) { return window.first == hWnd; });
        if (it != g_windowProgress.end()) {
            changed = it->second != state;
            it->second = state;
        } else if (state != 0) {
            g_windowProgress.push_back({hWnd, state});
            changed = true;
        }
    } else if (std::none_of(
                   g_windowProgress.begin(), g_windowProgress.end(),
                   [&](const auto& window) { return window.first == hWnd; })) {
        // (A value set alone means it's going normally.)
        g_windowProgress.push_back({hWnd, kProgressNormal});
        changed = true;
    }
    // (Kept either way: an island turning its fill on sees what's going.)
    const double now = NowSeconds();
    if (g_progressWanted &&
        (changed || g_progressSentState <= 0 ||
         now - g_progressLookedAt >= kProgressLookSeconds)) {
        g_progressLookedAt = now;
        LookAtProgressBars();
    }
}

// An island fills with the apps' progress, or none does anymore (see
// kIpcProgressWanted): the bars watched and read from now, or let go.
void SetProgressWanted(bool wanted) {
    if (wanted == g_progressWanted) {
        return;
    }
    g_progressWanted = wanted;
    if (wanted) {
        LookAtProgressBars();
        PublishProgress(true);
    } else {
        StopWatchingProgressBars();
        g_progressSentState = -1;
    }
}

// Each second while a window has progress, one is waiting to be told, or
// the island shows one (see g_badgeTimer): the bars read again. A new
// window's button, and its bar, may come or show only after the taskbar's
// call; and the taskbar may hide a finished one without telling (no
// change of its bar's visibility), so it's never left filling the pill.
void RecheckProgress() {
    if (g_progressWanted &&
        (!g_windowProgress.empty() || g_progressSeenAt > 0 ||
         g_progressSentState > 0)) {
        LookAtProgressBars();
    }
}

using CTaskBand__OnSetProgressState_t = void(WINAPI*)(void* pThis,
                                                      HWND hWnd,
                                                      int state);
CTaskBand__OnSetProgressState_t CTaskBand__OnSetProgressState_Original;
void WINAPI CTaskBand__OnSetProgressState_Hook(void* pThis,
                                               HWND hWnd,
                                               int state) {
    CTaskBand__OnSetProgressState_Original(pThis, hWnd, state);
    Safely(L"Following the progress", [&] { OnWindowProgress(hWnd, state); });
}

using CTaskBand__OnSetProgressValue_t = void(WINAPI*)(void* pThis,
                                                      HWND hWnd,
                                                      DWORD value);
CTaskBand__OnSetProgressValue_t CTaskBand__OnSetProgressValue_Original;
void WINAPI CTaskBand__OnSetProgressValue_Hook(void* pThis,
                                               HWND hWnd,
                                               DWORD value) {
    CTaskBand__OnSetProgressValue_Original(pThis, hWnd, value);
    Safely(L"Following the progress", [&] { OnWindowProgress(hWnd, -1); });
}

// (Before the taskbar takes it.)
using CTaskBand__HandleOverlayIcon_t = void(WINAPI*)(void* pThis,
                                                     HWND hWnd,
                                                     HICON icon);
CTaskBand__HandleOverlayIcon_t CTaskBand__HandleOverlayIcon_Original;
void WINAPI CTaskBand__HandleOverlayIcon_Hook(void* pThis,
                                              HWND hWnd,
                                              HICON icon) {
    KeepOverlayIcon(hWnd, icon);
    CTaskBand__HandleOverlayIcon_Original(pThis, hWnd, icon);
}

// An app's overlay icon's text set (it may come before the picture). Not
// while unloading (see KeepOverlayIcon).
void KeepOverlayText(HWND hWnd, PCWSTR text) {
    if (g_unloading) {
        return;
    }
    AcquireSRWLockExclusive(&g_overlayIconsLock);
    auto it = std::find_if(
        g_overlayIcons.begin(), g_overlayIcons.end(),
        [&](const OverlayIcon& overlay) { return overlay.wnd == hWnd; });
    if (it == g_overlayIcons.end()) {
        g_overlayIcons.push_back({hWnd});
        it = g_overlayIcons.end() - 1;
    }
    const std::wstring description = text ? text : L"";
    if (it->description != description) {
        it->description = description;
        it->serial = ++g_overlaySerial;
    }
    ReleaseSRWLockExclusive(&g_overlayIconsLock);
}

// An app's overlay icon's text: the taskbar hands it to its item (see
// CTaskItem_SetOverlayIconDescription_Hook) while this runs, for this
// window. Taskbar thread only.
HWND g_overlayTextWnd;
using CTaskBand__HandleOverlayIconDescription_t =
    void(WINAPI*)(void* pThis, HWND hWnd, void* text);
CTaskBand__HandleOverlayIconDescription_t
    CTaskBand__HandleOverlayIconDescription_Original;
void WINAPI CTaskBand__HandleOverlayIconDescription_Hook(void* pThis,
                                                         HWND hWnd,
                                                         void* text) {
    g_overlayTextWnd = hWnd;
    CTaskBand__HandleOverlayIconDescription_Original(pThis, hWnd, text);
    g_overlayTextWnd = nullptr;
}

using CTaskItem_SetOverlayIconDescription_t = HRESULT(WINAPI*)(void* pThis,
                                                               PCWSTR text);
CTaskItem_SetOverlayIconDescription_t
    CTaskItem_SetOverlayIconDescription_Original;
HRESULT WINAPI CTaskItem_SetOverlayIconDescription_Hook(void* pThis,
                                                        PCWSTR text) {
    if (HWND hWnd = g_overlayTextWnd) {
        KeepOverlayText(hWnd, text);
    }
    return CTaskItem_SetOverlayIconDescription_Original(pThis, text);
}

// The apps' overlay icons set before the mod started (like Discord's counter
// from before the mod was turned on or updated), read from the taskbar's
// items as if they came now: otherwise they'd stay as the taskbar draws them
// (see UseBigOverlay) until they change. Through the task band's
// ITaskListManager (found like its ITaskListWndSite, see
// GetTaskbarXamlRoot); only the taskbar's own items are read (a window's
// or a Store app's, by their tables).
void* CTaskBand_ITaskListManager_vftable;
void* CWindowTaskItem_ITaskItem_vftable;
void* CImmersiveTaskItem_ITaskItem_vftable;
using CTaskBand_GetTaskFromWindow_t = HRESULT(WINAPI*)(void* pThis,
                                                       HWND hWnd,
                                                       IUnknown** group,
                                                       IUnknown** item);
CTaskBand_GetTaskFromWindow_t CTaskBand_GetTaskFromWindow_Original;
using CTaskItem_GetOverlayIcon_t = HICON(WINAPI*)(void* pThis);
CTaskItem_GetOverlayIcon_t CTaskItem_GetOverlayIcon_Original;
using CTaskItem_GetOverlayIconDescription_t = PCWSTR(WINAPI*)(void* pThis);
CTaskItem_GetOverlayIconDescription_t
    CTaskItem_GetOverlayIconDescription_Original;

BOOL CALLBACK ReadOverlayIconProc(HWND hWnd, LPARAM lParam) {
    if (!IsWindowVisible(hWnd) && !IsIconic(hWnd)) {
        return TRUE;
    }
    AcquireSRWLockShared(&g_overlayIconsLock);
    const bool known = std::any_of(
        g_overlayIcons.begin(), g_overlayIcons.end(),
        [&](const OverlayIcon& overlay) { return overlay.wnd == hWnd; });
    ReleaseSRWLockShared(&g_overlayIconsLock);
    IUnknown* group = nullptr;
    IUnknown* item = nullptr;
    if (known ||
        FAILED(CTaskBand_GetTaskFromWindow_Original((void*)lParam, hWnd, &group,
                                                    &item)) ||
        !item) {
        if (group) {
            group->Release();
        }
        return TRUE;
    }
    const void* table = *(void**)item;
    if (table && (table == CWindowTaskItem_ITaskItem_vftable ||
                  table == CImmersiveTaskItem_ITaskItem_vftable)) {
        if (HICON icon = CTaskItem_GetOverlayIcon_Original(item)) {
            KeepOverlayText(hWnd,
                            CTaskItem_GetOverlayIconDescription_Original(item));
            KeepOverlayIcon(hWnd, icon);
        }
    }
    item->Release();
    if (group) {
        group->Release();
    }
    return TRUE;
}

void WINAPI ReadOverlayIconsProc(void*) {
    if (!CTaskBand_ITaskListManager_vftable ||
        (!CWindowTaskItem_ITaskItem_vftable &&
         !CImmersiveTaskItem_ITaskItem_vftable) ||
        !CTaskBand_GetTaskFromWindow_Original ||
        !CTaskItem_GetOverlayIcon_Original ||
        !CTaskItem_GetOverlayIconDescription_Original) {
        return;
    }
    HWND hTaskbarWnd = FindCurrentProcessTaskbarWnd();
    HWND hTaskSwWnd =
        hTaskbarWnd ? (HWND)GetProp(hTaskbarWnd, L"TaskbandHWND") : nullptr;
    void* manager =
        hTaskSwWnd ? (void*)GetWindowLongPtr(hTaskSwWnd, 0) : nullptr;
    for (int i = 0;
         manager && *(void**)manager != CTaskBand_ITaskListManager_vftable;
         i++) {
        manager = i == 20 ? nullptr : (void**)manager + 1;
    }
    if (manager) {
        EnumWindows(ReadOverlayIconProc, (LPARAM)manager);
        StartRendering();
    }
}

void ReadOverlayIcons() {
    if (HWND hTaskbarWnd = FindCurrentProcessTaskbarWnd()) {
        RunFromWindowThread(hTaskbarWnd, ReadOverlayIconsProc, nullptr);
    }
}

// A value of an app's registration as a notification sender
// (Software\Classes\AppUserModelId\<ID>, the user's then the machine's, as
// Windows' notification center reads it), with environment variables and
// "@file,-id" or "@{...}" resources resolved. `registered` (optional) tells
// whether the app has one.
std::wstring RegisteredAppValue(const std::wstring& appId,
                                PCWSTR name,
                                bool* registered = nullptr) {
    if (registered) {
        *registered = false;
    }
    if (appId.empty() || appId.size() > 256 ||
        appId.find(L'\\') != std::wstring::npos) {
        return {};
    }
    const std::wstring key = L"Software\\Classes\\AppUserModelId\\" + appId;
    for (HKEY root : {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE}) {
        HKEY app = nullptr;
        if (RegOpenKeyEx(root, key.c_str(), 0, KEY_QUERY_VALUE, &app) !=
            ERROR_SUCCESS) {
            continue;
        }
        if (registered) {
            *registered = true;
        }
        WCHAR value[1024];
        DWORD size = sizeof(value);
        const bool read =
            RegGetValue(app, nullptr, name,
                        RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, value,
                        &size) == ERROR_SUCCESS;
        RegCloseKey(app);
        if (!read || !value[0]) {
            continue;
        }
        if (value[0] != L'@') {
            return value;
        }
        WCHAR resolved[1024];
        if (SUCCEEDED(SHLoadIndirectString(value, resolved,
                                           ARRAYSIZE(resolved), nullptr))) {
            return resolved;
        }
    }
    return {};
}

// A full path on one of this computer's drives (not a share, nor a drive
// letter mapped to one: no connection to another computer).
bool IsLocalDrivePath(const std::wstring& path) {
    if (path.size() < 4 || !iswalpha(path[0]) || path[1] != L':' ||
        path[2] != L'\\') {
        return false;
    }
    const WCHAR root[] = {path[0], L':', L'\\', 0};
    return GetDriveType(root) != DRIVE_REMOTE;
}

// The picture a notification sender registered (IconUri: a file, or a
// resource of a package or of Windows), as a file; Windows' default picture
// for its own notifications for one registered without any. Only a file on
// a drive (never a share: no connection to another computer). Empty if the
// app isn't registered.
std::wstring RegisteredAppPicture(const std::wstring& appId) {
    bool registered = false;
    std::wstring uri = RegisteredAppValue(appId, L"IconUri", &registered);
    if (!registered) {
        return {};
    }
    if (uri.empty()) {
        uri =
            L"ms-resource://Windows.UI.ShellCommon/Files/Images/"
            L"DefaultSystemNotification.png";
    }
    // (Windows' resources, by the same name as in a package's.)
    if (uri.starts_with(L"ms-resource://")) {
        WCHAR resolved[1024];
        const std::wstring resource = L"@{Windows?" + uri + L"}";
        uri = SUCCEEDED(SHLoadIndirectString(resource.c_str(), resolved,
                                             ARRAYSIZE(resolved), nullptr))
                  ? resolved
                  : L"";
    }
    if (!IsLocalDrivePath(uri)) {
        return {};
    }
    return uri;
}

// A picture file as a 32-bit bitmap with premultiplied alpha, fit in a
// square of `size` pixels (like an icon).
HBITMAP LoadPictureBitmap(PCWSTR path, int size) {
    winrt::com_ptr<IWICImagingFactory> wic;
    winrt::com_ptr<IWICBitmapDecoder> decoder;
    winrt::com_ptr<IWICBitmapFrameDecode> frame;
    UINT width = 0, height = 0;
    if (size <= 0 ||
        FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wic.put()))) ||
        FAILED(wic->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
                                              WICDecodeMetadataCacheOnDemand,
                                              decoder.put())) ||
        FAILED(decoder->GetFrame(0, frame.put())) ||
        FAILED(frame->GetSize(&width, &height)) || !width || !height) {
        return nullptr;
    }
    const double fit = std::min((double)size / width, (double)size / height);
    const UINT shownWidth = std::max<UINT>(1, (UINT)std::lround(width * fit));
    const UINT shownHeight = std::max<UINT>(1, (UINT)std::lround(height * fit));
    winrt::com_ptr<IWICBitmapScaler> scaler;
    winrt::com_ptr<IWICFormatConverter> converter;
    if (FAILED(wic->CreateBitmapScaler(scaler.put())) ||
        FAILED(scaler->Initialize(frame.get(), shownWidth, shownHeight,
                                  WICBitmapInterpolationModeFant)) ||
        FAILED(wic->CreateFormatConverter(converter.put())) ||
        FAILED(converter->Initialize(scaler.get(),
                                     GUID_WICPixelFormat32bppPBGRA,
                                     WICBitmapDitherTypeNone, nullptr, 0,
                                     WICBitmapPaletteTypeMedianCut))) {
        return nullptr;
    }
    BITMAPINFO header{};
    header.bmiHeader = {sizeof(BITMAPINFOHEADER), size, -size, 1, 32, BI_RGB};
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &header, DIB_RGB_COLORS, &bits,
                                      nullptr, 0);
    if (!bitmap) {
        return nullptr;
    }
    // (Transparent around it; in the middle.)
    ZeroMemory(bits, (size_t)size * size * 4);
    const UINT stride = (UINT)size * 4;
    BYTE* start = (BYTE*)bits + ((size - shownHeight) / 2) * stride +
                  ((size - shownWidth) / 2) * 4;
    if (FAILED(converter->CopyPixels(
            nullptr, stride, (shownHeight - 1) * stride + shownWidth * 4,
            start))) {
        DeleteObject(bitmap);
        return nullptr;
    }
    return bitmap;
}

// An icon as a 32-bit bitmap with premultiplied alpha (nullptr without
// colors of its own). An old one without an alpha channel (fewer colors, or
// 32 bits all at 0) shows where its mask says, as Windows draws it.
HBITMAP IconBitmap(HICON icon) {
    ICONINFO info{};
    if (!GetIconInfo(icon, &info)) {
        return nullptr;
    }
    HBITMAP result = nullptr;
    BITMAP color{};
    if (info.hbmColor && GetObject(info.hbmColor, sizeof(color), &color) &&
        color.bmWidth > 0 && color.bmHeight > 0) {
        const int width = color.bmWidth;
        const int height = std::abs(color.bmHeight);
        BITMAPINFO header{};
        header.bmiHeader = {sizeof(BITMAPINFOHEADER), width, -height, 1, 32,
                            BI_RGB};
        void* bits = nullptr;
        HDC screen = GetDC(nullptr);
        result = CreateDIBSection(screen, &header, DIB_RGB_COLORS, &bits,
                                  nullptr, 0);
        if (result && bits &&
            GetDIBits(screen, info.hbmColor, 0, height, bits, &header,
                      DIB_RGB_COLORS) == height) {
            // (Icons keep their colors as they are: premultiplied here.)
            auto* pixels = static_cast<uint8_t*>(bits);
            const size_t count = (size_t)width * height * 4;
            bool alpha = false;
            for (size_t i = 3; i < count && !alpha; i += 4) {
                alpha = pixels[i] != 0;
            }
            if (!alpha) {
                // (The mask's white is what doesn't show.)
                std::vector<uint8_t> mask(count);
                const bool masked =
                    info.hbmMask &&
                    GetDIBits(screen, info.hbmMask, 0, height, mask.data(),
                              &header, DIB_RGB_COLORS) == height;
                for (size_t i = 0; i < count; i += 4) {
                    pixels[i + 3] = masked && mask[i] ? 0 : 255;
                }
            }
            for (size_t i = 0; i < count; i += 4) {
                for (int c = 0; c < 3; c++) {
                    pixels[i + c] =
                        (uint8_t)(pixels[i + c] * pixels[i + 3] / 255);
                }
            }
        } else if (result) {
            DeleteObject(result);
            result = nullptr;
        }
        ReleaseDC(nullptr, screen);
    }
    if (info.hbmColor) {
        DeleteObject(info.hbmColor);
    }
    if (info.hbmMask) {
        DeleteObject(info.hbmMask);
    }
    return result;
}

BOOL CALLBACK FindAppIdWindowProc(HWND hWnd, LPARAM lParam) {
    auto* search = reinterpret_cast<std::pair<const std::wstring*, HWND>*>(
        lParam);
    if (IsWindowVisible(hWnd) &&
        _wcsicmp(GetWindowAppId(hWnd).c_str(), search->first->c_str()) == 0) {
        search->second = hWnd;
        return FALSE;
    }
    return TRUE;
}

// The program an app's window gives the taskbar to start it again
// (PKEY_AppUserModel_RelaunchCommand), when it's a file that's there. The
// command may be a path with spaces and no quotes, like Steam's
// "C:\Program Files (x86)\Steam\steam.exe": the whole of it first.
std::wstring WindowRelaunchProgram(HWND window) {
    auto isProgram = [](const std::wstring& path) {
        const DWORD attributes =
            IsLocalDrivePath(path) ? GetFileAttributes(path.c_str())
                                   : INVALID_FILE_ATTRIBUTES;
        return attributes != INVALID_FILE_ATTRIBUTES &&
               !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    };
    const std::wstring command = GetWindowAppText(window, 2);
    if (command.empty() || isProgram(command)) {
        return command;
    }
    std::wstring program;
    int count = 0;
    if (LPWSTR* parts = CommandLineToArgvW(command.c_str(), &count)) {
        if (count > 0 && isProgram(parts[0])) {
            program = parts[0];
        }
        LocalFree(parts);
    }
    return program;
}

// The icon an app's window gives the taskbar for its own ID (its
// PKEY_AppUserModel_RelaunchIconResource, "path,index"), like the profile's
// picture of a browser's second profile, which has no Start menu entry; or,
// without one (like Steam's window), its relaunch program's or its process
// file's, as big as asked (the taskbar's own is small).
HBITMAP WindowRelaunchIconBitmap(const std::wstring& appId, int size) {
    std::pair<const std::wstring*, HWND> search{&appId, nullptr};
    EnumWindows(FindAppIdWindowProc, reinterpret_cast<LPARAM>(&search));
    if (!search.second) {
        return nullptr;
    }
    const std::wstring resource = GetWindowAppText(search.second, 3);
    if (const size_t comma = resource.rfind(L',');
        comma != std::wstring::npos && comma != 0) {
        WCHAR path[MAX_PATH];
        HICON icon = nullptr;
        if (ExpandEnvironmentStrings(resource.substr(0, comma).c_str(), path,
                                     ARRAYSIZE(path)) &&
            IsLocalDrivePath(path) &&
            PrivateExtractIcons(path, _wtoi(resource.c_str() + comma + 1),
                                size, size, &icon, nullptr, 1, 0) == 1 &&
            icon) {
            HBITMAP bitmap = IconBitmap(icon);
            DestroyIcon(icon);
            return bitmap;
        }
    }
    std::wstring program = WindowRelaunchProgram(search.second);
    // (Nor a program: its process's file. Not a packaged app's: its window
    // may be the frame of another process.)
    DWORD processId = 0;
    if (program.empty() && appId.find(L'!') == std::wstring::npos &&
        GetWindowThreadProcessId(search.second, &processId)) {
        if (HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                         FALSE, processId)) {
            WCHAR path[MAX_PATH];
            DWORD length = ARRAYSIZE(path);
            if (QueryFullProcessImageName(process, 0, path, &length)) {
                program = path;
            }
            CloseHandle(process);
        }
    }
    winrt::com_ptr<IShellItemImageFactory> factory;
    HBITMAP bitmap = nullptr;
    if (program.empty() ||
        FAILED(SHCreateItemFromParsingName(program.c_str(), nullptr,
                                           IID_PPV_ARGS(factory.put()))) ||
        FAILED(factory->GetImage({size, size},
                                 SIIGBF_ICONONLY | SIIGBF_BIGGERSIZEOK,
                                 &bitmap))) {
        return nullptr;
    }
    return bitmap;
}

// FOLDERID_UserPinned: the shortcuts pinned to the taskbar are in its
// "TaskBar" folder.
constexpr GUID kFOLDERID_UserPinned = {
    0x9e3995ab,
    0x1f9c,
    0x4f13,
    {0xb8, 0x27, 0x48, 0xb2, 0x4b, 0x6c, 0x71, 0x74}};
// CLSID_ShellLink: reads a shortcut's file.
constexpr GUID kCLSID_ShellLink = {
    0x00021401,
    0x0000,
    0x0000,
    {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

std::wstring KnownFolderPath(const GUID& folder);

// The icon given to an app's shortcut pinned to the taskbar (like one changed
// in its properties), which the taskbar shows in place of the app's own, as
// big as asked; nullptr when it has none of its own (its program's first
// icon) or isn't pinned. The app is the shortcut's as the taskbar works it
// out (see TaskbarAppIdOfWindow).
HBITMAP PinnedShortcutIconBitmap(const std::wstring& appId, int size) {
    const std::wstring folder = KnownFolderPath(kFOLDERID_UserPinned);
    winrt::com_ptr<IAppResolver8> resolver;
    if (appId.empty() || folder.empty() ||
        FAILED(CoCreateInstance(kCLSID_AppResolver, nullptr,
                                CLSCTX_INPROC_SERVER, kIID_AppResolver8,
                                resolver.put_void()))) {
        return nullptr;
    }
    const std::wstring pinned = folder + L"\\TaskBar\\";
    WIN32_FIND_DATA data;
    HANDLE find =
        FindFirstFileEx((pinned + L"*.lnk").c_str(), FindExInfoBasic, &data,
                        FindExSearchNameMatch, nullptr, 0);
    if (find == INVALID_HANDLE_VALUE) {
        return nullptr;
    }
    HBITMAP bitmap = nullptr;
    do {
        const std::wstring path = pinned + data.cFileName;
        winrt::com_ptr<IShellItem> item;
        PWSTR id = nullptr;
        const bool same =
            SUCCEEDED(SHCreateItemFromParsingName(path.c_str(), nullptr,
                                                  IID_PPV_ARGS(item.put()))) &&
            SUCCEEDED(resolver->GetAppIDForShortcut(item.get(), &id)) && id &&
            _wcsicmp(id, appId.c_str()) == 0;
        CoTaskMemFree(id);
        if (!same) {
            continue;
        }
        winrt::com_ptr<IShellLinkW> link;
        winrt::com_ptr<IPersistFile> file;
        WCHAR icon[MAX_PATH] = L"";
        WCHAR program[MAX_PATH] = L"";
        WCHAR iconPath[MAX_PATH];
        int index = 0;
        if (SUCCEEDED(CoCreateInstance(kCLSID_ShellLink, nullptr,
                                       CLSCTX_INPROC_SERVER,
                                       IID_PPV_ARGS(link.put()))) &&
            (file = link.try_as<IPersistFile>()) &&
            SUCCEEDED(file->Load(path.c_str(), STGM_READ)) &&
            SUCCEEDED(link->GetIconLocation(icon, ARRAYSIZE(icon), &index)) &&
            icon[0] &&
            ExpandEnvironmentStrings(icon, iconPath, ARRAYSIZE(iconPath)) &&
            IsLocalDrivePath(iconPath)) {
            // (Its program's own first icon: the app's, as without it.)
            const bool own =
                index == 0 &&
                SUCCEEDED(link->GetPath(program, ARRAYSIZE(program), nullptr,
                                        0)) &&
                _wcsicmp(program, iconPath) == 0;
            HICON picture = nullptr;
            if (!own &&
                PrivateExtractIcons(iconPath, index, size, size, &picture,
                                    nullptr, 1, 0) == 1 &&
                picture) {
                bitmap = IconBitmap(picture);
                DestroyIcon(picture);
            }
        }
        break;
    } while (FindNextFile(find, &data));
    FindClose(find);
    return bitmap;
}

DWORD SessionProcessId(PCWSTR exe);
std::wstring ProcessImagePath(DWORD processId);

// An app known only by its program's file name (a media app without an ID of
// its own, like "Spotify.exe" or "vlc.exe"): that program's path while it
// runs in this session, or empty.
std::wstring RunningProgramPath(const std::wstring& appId) {
    if (appId.size() < 5 || appId.find(L'\\') != std::wstring::npos ||
        _wcsicmp(appId.c_str() + appId.size() - 4, L".exe") != 0) {
        return {};
    }
    const DWORD processId = SessionProcessId(appId.c_str());
    return processId ? ProcessImagePath(processId) : std::wstring();
}

// The app's icon as a 32-bit bitmap with premultiplied alpha, from the shell:
// by the app's AppUserModelID if it has one, by its program file otherwise.
HBITMAP LoadAppIconBitmap(HWND hWnd, std::wstring appId, int size) {
    winrt::com_ptr<IShellItemImageFactory> factory;

    if (appId.empty()) {
        appId = GetWindowAppId(hWnd);
    }
    if (!appId.empty()) {
        SHCreateItemInKnownFolder(kFOLDERID_AppsFolder, 0, appId.c_str(),
                                  IID_PPV_ARGS(factory.put()));
    }
    // Notifications of a packaged app may come under an ID of their own (like
    // Phone Link's, one for each phone app): its package's app then.
    if (const size_t bang = appId.find(L'!');
        !factory && bang != std::wstring::npos) {
        const std::wstring packageApp = appId.substr(0, bang) + L"!App";
        if (packageApp != appId) {
            SHCreateItemInKnownFolder(kFOLDERID_AppsFolder, 0,
                                      packageApp.c_str(),
                                      IID_PPV_ARGS(factory.put()));
        }
    }

    // An app known by its program file (a window without an ID of its own,
    // like Discord's): that file's icon (on a drive only, never a network
    // path).
    if (!factory && IsLocalDrivePath(appId)) {
        SHCreateItemFromParsingName(appId.c_str(), nullptr,
                                    IID_PPV_ARGS(factory.put()));
    }
    // (Or only by its file's name: the one running.)
    if (!factory) {
        if (const std::wstring path = RunningProgramPath(appId);
            IsLocalDrivePath(path)) {
            SHCreateItemFromParsingName(path.c_str(), nullptr,
                                        IID_PPV_ARGS(factory.put()));
        }
    }
    if (!factory && hWnd) {
        DWORD processId = 0;
        GetWindowThreadProcessId(hWnd, &processId);
        HANDLE process =
            OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (process) {
            WCHAR path[MAX_PATH];
            DWORD length = ARRAYSIZE(path);
            if (QueryFullProcessImageName(process, 0, path, &length)) {
                SHCreateItemFromParsingName(path, nullptr,
                                            IID_PPV_ARGS(factory.put()));
            }
            CloseHandle(process);
        }
    }

    // (No Start menu entry: the icon its windows give the taskbar.)
    if (!factory && !appId.empty()) {
        if (HBITMAP icon = WindowRelaunchIconBitmap(appId, size)) {
            return icon;
        }
    }
    HBITMAP bitmap = nullptr;
    if (factory &&
        FAILED(factory->GetImage({size, size},
                                 SIIGBF_ICONONLY | SIIGBF_BIGGERSIZEOK,
                                 &bitmap))) {
        bitmap = nullptr;
    }
    // A sender with no Start menu entry (Windows' own, like Windows
    // Security's, or an app registered only for its notifications): the
    // picture it registered.
    if (!factory && !appId.empty()) {
        const std::wstring picture = RegisteredAppPicture(appId);
        if (!picture.empty()) {
            bitmap = LoadPictureBitmap(picture.c_str(), size);
        }
    }
    return bitmap;
}

// The app a notification is shown under on the pill: its sender, or (see
// islandGroupApps) the package's app when the sender is one of its own IDs
// without a Start menu entry of its own (like Phone Link's, one for each
// phone app: they'd all be the same icon), so its notifications are one icon
// with their counts together. Asked once for each ID (the island's thread).
std::unordered_map<std::wstring, std::wstring> g_notificationAppKeys;

std::wstring NotificationAppKey(const std::wstring& appId) {
    const size_t bang = appId.find(L'!');
    if (!g_settings.islandGroupApps || bang == std::wstring::npos) {
        return appId;
    }
    const std::wstring packageApp = appId.substr(0, bang) + L"!App";
    if (_wcsicmp(packageApp.c_str(), appId.c_str()) == 0) {
        return appId;
    }
    if (const auto it = g_notificationAppKeys.find(appId);
        it != g_notificationAppKeys.end()) {
        return it->second;
    }
    winrt::com_ptr<IShellItem> own;
    winrt::com_ptr<IShellItem> package;
    const bool grouped =
        FAILED(SHCreateItemInKnownFolder(kFOLDERID_AppsFolder, 0, appId.c_str(),
                                         IID_PPV_ARGS(own.put()))) &&
        SUCCEEDED(SHCreateItemInKnownFolder(kFOLDERID_AppsFolder, 0,
                                            packageApp.c_str(),
                                            IID_PPV_ARGS(package.put())));
    const std::wstring key = grouped ? packageApp : appId;
    g_notificationAppKeys[appId] = key;
    return key;
}

bool SameNotificationApp(const std::wstring& appId, const std::wstring& key) {
    return _wcsicmp(appId.c_str(), key.c_str()) == 0 ||
           _wcsicmp(NotificationAppKey(appId).c_str(), key.c_str()) == 0;
}

// The icons given to the island (see kIpcAppIcon), by app and size. Only used
// on the taskbar's thread.
std::vector<std::pair<std::wstring, HICON>> g_islandAppIcons;

HICON AppIconForIsland(const COPYDATASTRUCT* copyData) {
    IpcReader reader(copyData);
    int size = 0;
    std::wstring appId;
    if (!reader.Get(&size) || !reader.GetString(&appId) || appId.empty() ||
        size <= 0 || size > 256) {
        return nullptr;
    }
    const std::wstring key = std::to_wstring(size) + L"|" + appId;
    for (const auto& [id, icon] : g_islandAppIcons) {
        if (id == key) {
            return icon;
        }
    }
    HICON icon = nullptr;
    if (HBITMAP bitmap = LoadAppIconBitmap(nullptr, appId, size)) {
        // An icon's colors aren't premultiplied by their alpha.
        BITMAP info{};
        GetObject(bitmap, sizeof(info), &info);
        const int width = info.bmWidth;
        const int height = std::abs(info.bmHeight);
        BITMAPINFO header{};
        header.bmiHeader = {sizeof(BITMAPINFOHEADER), width, -height, 1, 32,
                            BI_RGB};
        void* bits = nullptr;
        HDC screen = GetDC(nullptr);
        HBITMAP color = width > 0 && height > 0
                            ? CreateDIBSection(screen, &header, DIB_RGB_COLORS,
                                               &bits, nullptr, 0)
                            : nullptr;
        if (color &&
            GetDIBits(screen, bitmap, 0, height, bits, &header,
                      DIB_RGB_COLORS) == height) {
            auto* pixel = (BYTE*)bits;
            for (int i = 0; i < width * height; i++, pixel += 4) {
                if (const BYTE alpha = pixel[3]; alpha && alpha < 255) {
                    for (int c = 0; c < 3; c++) {
                        pixel[c] = (BYTE)std::min(255, pixel[c] * 255 / alpha);
                    }
                }
            }
            HBITMAP mask = CreateBitmap(width, height, 1, 1, nullptr);
            ICONINFO iconInfo{TRUE, 0, 0, mask, color};
            icon = CreateIconIndirect(&iconInfo);
            if (mask) {
                DeleteObject(mask);
            }
        }
        if (color) {
            DeleteObject(color);
        }
        ReleaseDC(nullptr, screen);
        DeleteObject(bitmap);
    }
    // Remembered even when missing, so it isn't looked up again; the oldest
    // go past a limit (the island has copied them already).
    constexpr size_t kMaxIslandAppIcons = 128;
    if (g_islandAppIcons.size() >= kMaxIslandAppIcons) {
        for (size_t i = 0; i < kMaxIslandAppIcons / 2; i++) {
            if (g_islandAppIcons[i].second) {
                DestroyIcon(g_islandAppIcons[i].second);
            }
        }
        g_islandAppIcons.erase(g_islandAppIcons.begin(),
                               g_islandAppIcons.begin() +
                                   kMaxIslandAppIcons / 2);
    }
    g_islandAppIcons.push_back({key, icon});
    return icon;
}

void FreeIslandAppIcons() {
    for (const auto& [id, icon] : g_islandAppIcons) {
        if (icon) {
            DestroyIcon(icon);
        }
    }
    g_islandAppIcons.clear();
}

// A window brought back from minimized (see ActivateApp), checked a few
// times after: some (consoles) stay minimized at first.
thread_local HWND t_restoreWindow;
thread_local int t_restoreTries;
thread_local UINT_PTR t_restoreTimer;

void CALLBACK RestoreCheckProc(HWND, UINT, UINT_PTR, DWORD) {
    const HWND hWnd = t_restoreWindow;
    if (!IsWindow(hWnd) || !IsIconic(hWnd) || ++t_restoreTries > 3) {
        KillTimer(nullptr, t_restoreTimer);
        t_restoreTimer = 0;
        t_restoreWindow = nullptr;
        return;
    }
    // Asked the way its own title bar does, then directly (not when it
    // doesn't answer: that would wait for it).
    if (t_restoreTries == 1) {
        PostMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    } else if (!IsHungAppWindow(hWnd)) {
        ShowWindow(hWnd, SW_RESTORE);
    }
}

// Brings a window to the front: first the right to (the island or the
// taskbar was just clicked), then restored if minimized. Restored first, a
// console window could end up active but still minimized.
void ActivateApp(HWND hWnd) {
    if (!IsWindow(hWnd)) {
        return;
    }
    HWND popup = GetLastActivePopup(hWnd);
    SetForegroundWindow(popup && IsWindowVisible(popup) ? popup : hWnd);
    if (IsIconic(hWnd)) {
        ShowWindowAsync(hWnd, SW_RESTORE);
        t_restoreWindow = hWnd;
        t_restoreTries = 0;
        if (t_restoreTimer) {
            KillTimer(nullptr, t_restoreTimer);
        }
        t_restoreTimer = SetTimer(nullptr, 0, 150, RestoreCheckProc);
    }
}

// Whether a window belongs to the app with this AppUserModelID. Apps without
// one of their own are told apart by their program file, which the taskbar
// then uses as their ID.
bool WindowMatchesApp(HWND hWnd, const std::wstring& appId) {
    const std::wstring id = GetWindowAppId(hWnd);
    if (!id.empty()) {
        return _wcsicmp(id.c_str(), appId.c_str()) == 0;
    }
    // (An app that sets its ID on its process, like Discord: as the taskbar
    // works it out.)
    if (appId.find(L'\\') == std::wstring::npos) {
        return _wcsicmp(TaskbarAppIdOfWindow(hWnd).c_str(), appId.c_str()) ==
               0;
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(hWnd, &processId);
    HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process) {
        return false;
    }
    WCHAR path[MAX_PATH];
    DWORD length = ARRAYSIZE(path);
    const bool matches =
        QueryFullProcessImageName(process, 0, path, &length) &&
        _wcsicmp(path, appId.c_str()) == 0;
    CloseHandle(process);
    return matches;
}

// Looking for an app's window.
struct AppWindowSearch {
    const std::wstring* appId;
    HWND found;
};

BOOL CALLBACK FindAppWindowProc(HWND hWnd, LPARAM lParam) {
    auto* context = reinterpret_cast<AppWindowSearch*>(lParam);
    if (!IsWindowVisible(hWnd) || GetWindow(hWnd, GW_OWNER) ||
        (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)) {
        return TRUE;
    }
    if (WindowMatchesApp(hWnd, *context->appId)) {
        context->found = hWnd;
        return FALSE;
    }
    return TRUE;
}

// Whether a process runs elevated (also when it can't be told).
bool ProcessElevated(DWORD processId) {
    HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    HANDLE token = nullptr;
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    const bool known =
        process && OpenProcessToken(process, TOKEN_QUERY, &token) &&
        GetTokenInformation(token, TokenElevation, &elevation,
                            sizeof(elevation), &size);
    if (token) {
        CloseHandle(token);
    }
    if (process) {
        CloseHandle(process);
    }
    return !known || elevation.TokenIsElevated;
}

// CLSID_ShellWindows and SID_STopLevelBrowser.
constexpr GUID kCLSID_ShellWindows = {
    0x9ba05972,
    0xf6a8,
    0x11cf,
    {0xa4, 0x42, 0x00, 0xa0, 0xc9, 0x0a, 0x8f, 0x39}};
constexpr GUID kSID_STopLevelBrowser = {
    0x4c96be40,
    0x915c,
    0x11cf,
    {0x99, 0xd3, 0x00, 0xaa, 0x00, 0x4a, 0xe8, 0x37}};

// A program, file or app (shell:AppsFolder\...) started by Explorer's own
// shell, as the user and unelevated: the desktop's view, its shell's
// IShellDispatch2::ShellExecute (see "How can I launch an unelevated
// process from my elevated process", The Old New Thing). On a thread with
// COM. False if it can't be.
bool ShellExecuteAsUser(const std::wstring& target) {
    winrt::com_ptr<IShellWindows> windows;
    winrt::com_ptr<IDispatch> desktop;
    VARIANT none;
    VariantInit(&none);
    long found = 0;
    if (FAILED(CoCreateInstance(kCLSID_ShellWindows, nullptr, CLSCTX_ALL,
                                IID_PPV_ARGS(windows.put()))) ||
        windows->FindWindowSW(&none, &none, SWC_DESKTOP, &found,
                              SWFO_NEEDDISPATCH, desktop.put()) != S_OK ||
        !desktop) {
        return false;
    }
    winrt::com_ptr<IShellBrowser> browser;
    winrt::com_ptr<IShellView> view;
    winrt::com_ptr<IDispatch> background;
    winrt::com_ptr<IDispatch> application;
    const auto provider = desktop.try_as<IServiceProvider>();
    if (!provider ||
        FAILED(provider->QueryService(kSID_STopLevelBrowser,
                                      IID_PPV_ARGS(browser.put()))) ||
        FAILED(browser->QueryActiveShellView(view.put())) ||
        FAILED(view->GetItemObject(SVGIO_BACKGROUND,
                                   IID_PPV_ARGS(background.put())))) {
        return false;
    }
    const auto folderView = background.try_as<IShellFolderViewDual>();
    if (!folderView || FAILED(folderView->get_Application(application.put()))) {
        return false;
    }
    const auto shell = application.try_as<IShellDispatch2>();
    if (!shell) {
        return false;
    }
    BSTR file = SysAllocString(target.c_str());
    VARIANT show;
    VariantInit(&show);
    show.vt = VT_I4;
    show.lVal = SW_SHOWNORMAL;
    const HRESULT hr =
        file ? shell->ShellExecute(file, none, none, none, show)
             : E_OUTOFMEMORY;
    SysFreeString(file);
    return SUCCEEDED(hr);
}

// Brings an app to the front by its AppUserModelID: its window if it has one
// (restored if minimized), otherwise it's started, or brought back from the
// tray, through the shell. With Windhawk run as administrator, the island
// never starts it itself: the ID comes from explorer.exe's message, which a
// lower process can send too, and what the island starts would run
// elevated; Explorer starts it, as the user (see ShellExecuteAsUser).
void OpenApp(const std::wstring& appId) {
    if (appId.empty()) {
        return;
    }
    AppWindowSearch context{&appId, nullptr};
    EnumWindows(
        FindAppWindowProc,
        (LPARAM)&context);
    if (context.found) {
        ActivateApp(context.found);
        return;
    }
    const std::wstring target = appId.find(L'\\') != std::wstring::npos
                                    ? appId
                                    : L"shell:AppsFolder\\" + appId;
    static const bool elevated = ProcessElevated(GetCurrentProcessId());
    if (elevated) {
        if (!ShellExecuteAsUser(target)) {
            IslandLog(L"An app couldn't be started through Explorer");
        }
        return;
    }
    ShellExecute(nullptr, L"open", target.c_str(), nullptr, nullptr,
                 SW_SHOWNORMAL);
}

LRESULT CALLBACK AttentionWndProc(HWND hWnd,
                                  UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam) {
    switch (uMsg) {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_LBUTTONDOWN:
            g_attentionClicked = true;
            return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Whether the app is the one in front, so it doesn't need the icon.
bool IsAppInFront(const AttentionRequest& request) {
    HWND foreground = GetForegroundWindow();
    if (request.app) {
        return foreground == request.app;
    }
    return !request.appId.empty() &&
           GetWindowAppId(foreground) == request.appId;
}

// Shows the app's icon peeking out and bouncing, then hides it. Returns false
// if the thread should quit.
bool RunAttentionIcon(HWND window, const AttentionRequest& request) {
    static std::wstring lastKey;
    static double lastEnd;

    const HWND app = request.app;
    const std::wstring key =
        app ? L"#" + std::to_wstring((ULONG_PTR)app) : request.appId;
    if (g_settings.attentionMode != AttentionMode::Icon ||
        (app && !IsWindow(app)) || (!app && request.appId.empty()) ||
        IsAppInFront(request) ||
        (key == lastKey && NowSeconds() - lastEnd < kAttentionRepeatDelay)) {
        return true;
    }

    HWND taskbar = FindCurrentProcessTaskbarWnd();
    MONITORINFO monitor{sizeof(monitor)};
    if (!taskbar || !GetMonitorInfo(TaskbarMonitor(taskbar), &monitor)) {
        return true;
    }
    // With the dock on screen, the icon bounces in it instead. Full-screen
    // apps aren't disturbed.
    if (!IsTaskbarHidden(taskbar, monitor.rcMonitor) ||
        (g_settings.fullscreenGuard && IsFullscreenAppOnMonitor(taskbar))) {
        return true;
    }

    // Above the app's button, measured by the taskbar when the app started
    // asking for attention. That can come a little after this.
    const double asked = NowSeconds();
    int centerX = (monitor.rcMonitor.left + monitor.rcMonitor.right) / 2;
    for (int i = 0; i < 30; i++) {
        if (g_attentionButtonTime >= asked - 1.0) {
            centerX = g_attentionButtonX;
            break;
        }
        Sleep(10);
    }

    const UINT dpi = GetDpiForWindow(taskbar);
    const int iconSize = MulDiv(kAttentionIconSize, dpi, 96);
    const int bounceHeight = MulDiv(kAttentionBounceHeight, dpi, 96);
    const int restGap = MulDiv(kAttentionRestGap, dpi, 96);

    HBITMAP icon = LoadAppIconBitmap(app, request.appId, iconSize);
    if (!icon) {
        return true;
    }
    BITMAP iconInfo{};
    GetObject(icon, sizeof(iconInfo), &iconInfo);

    const SIZE size{iconSize, iconSize + bounceHeight + restGap};
    const POINT position{centerX - size.cx / 2,
                         monitor.rcMonitor.bottom - size.cy};
    // The icon's top edge, in the window: hidden below it, and resting.
    const double hiddenTop = size.cy;
    const double restTop = size.cy - restGap - iconSize;

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = size.cx;
    info.bmiHeader.biHeight = -size.cy;  // Top-down.
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    HDC canvas = CreateCompatibleDC(screen);
    HDC iconDc = CreateCompatibleDC(screen);
    HBITMAP canvasBitmap =
        CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, screen);
    if (!canvas || !iconDc || !canvasBitmap) {
        if (canvasBitmap) {
            DeleteObject(canvasBitmap);
        }
        if (canvas) {
            DeleteDC(canvas);
        }
        if (iconDc) {
            DeleteDC(iconDc);
        }
        DeleteObject(icon);
        return true;
    }
    HGDIOBJ oldCanvas = SelectObject(canvas, canvasBitmap);
    HGDIOBJ oldIcon = SelectObject(iconDc, icon);

    const BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    g_attentionClicked = false;
    bool keepRunning = true;
    bool shown = false;
    FrameTimer frames;
    const double start = NowSeconds();
    // When it starts going back, once the bounces are over or it's not needed
    // anymore.
    double sinkStart = 0;
    // The app in front is only looked up when the foreground window changes.
    HWND lastForeground = nullptr;
    bool appInFront = false;

    for (;;) {
        const double now = NowSeconds();
        if (HWND foreground = GetForegroundWindow();
            foreground != lastForeground) {
            lastForeground = foreground;
            appInFront = IsAppInFront(request);
        }
        if (!sinkStart &&
            (g_attentionClicked || appInFront ||
             !IsTaskbarHidden(taskbar, monitor.rcMonitor) || g_unloading)) {
            if (g_attentionClicked) {
                if (app) {
                    ActivateApp(app);
                } else {
                    OpenApp(request.appId);
                }
            }
            sinkStart = now;
        }

        double top;
        const double elapsed = now - start;
        if (!sinkStart && elapsed < kAttentionRiseTime) {
            top = hiddenTop + (restTop - hiddenTop) *
                                  SpringOut(elapsed / kAttentionRiseTime);
        } else if (!sinkStart) {
            bool done;
            const double height =
                BounceSequence(elapsed - kAttentionRiseTime,
                               kAttentionBounceGroups, &done);
            top = restTop - bounceHeight * height;
            if (done) {
                sinkStart = now;
            }
        } else {
            const double t = std::min((now - sinkStart) / kAttentionSinkTime, 1.0);
            top = restTop + (hiddenTop - restTop) * EaseInOutCubic(t);
            if (t >= 1) {
                break;
            }
        }

        ZeroMemory(bits, size.cx * size.cy * 4);
        AlphaBlend(canvas, 0, (int)std::lround(top), iconSize, iconSize, iconDc,
                   0, 0, iconInfo.bmWidth, std::abs(iconInfo.bmHeight), blend);
        POINT source{0, 0};
        UpdateLayeredWindow(window, nullptr, (POINT*)&position, (SIZE*)&size,
                            canvas, &source, 0, (BLENDFUNCTION*)&blend,
                            ULW_ALPHA);
        if (!shown) {
            SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                             SWP_SHOWWINDOW);
            shown = true;
        }

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                keepRunning = false;
                break;
            }
            // Another app asking meanwhile is skipped.
            if (msg.message == WM_APP_ATTENTION && !msg.hwnd) {
                delete (AttentionRequest*)msg.lParam;
                continue;
            }
            DispatchMessage(&msg);
        }
        if (!keepRunning) {
            break;
        }
        WaitForAnimationFrame(frames);
    }

    ShowWindow(window, SW_HIDE);
    SelectObject(iconDc, oldIcon);
    SelectObject(canvas, oldCanvas);
    DeleteObject(canvasBitmap);
    DeleteDC(iconDc);
    DeleteDC(canvas);
    DeleteObject(icon);

    lastKey = key;
    lastEnd = NowSeconds();
    return keepRunning;
}

DWORD WINAPI AttentionThreadProc(LPVOID parameter) {
    // Create the message queue before the creator continues.
    MSG msg;
    PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    SetEvent((HANDLE)parameter);

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const HRESULT comResult =
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASS windowClass{};
    windowClass.lpfnWndProc = AttentionWndProc;
    windowClass.hInstance = g_module;
    windowClass.hCursor = LoadCursor(nullptr, IDC_HAND);
    windowClass.lpszClassName = kAttentionClassName;
    RegisterClass(&windowClass);

    HWND window = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        kAttentionClassName, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
        g_module, nullptr);
    if (!window) {
        IslandLog(L"CreateWindowEx failed: %u", GetLastError());
    }

    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_APP_ATTENTION && !msg.hwnd) {
            std::unique_ptr<AttentionRequest> request{
                (AttentionRequest*)msg.lParam};
            if (window && !RunAttentionIcon(window, *request)) {
                break;
            }
            continue;
        }
        DispatchMessage(&msg);
    }

    // Requests that came in after the end.
    while (PeekMessage(&msg, nullptr, WM_APP_ATTENTION, WM_APP_ATTENTION,
                       PM_REMOVE)) {
        if (!msg.hwnd) {
            delete (AttentionRequest*)msg.lParam;
        }
    }

    if (window) {
        DestroyWindow(window);
    }
    UnregisterClass(kAttentionClassName, g_module);
    if (SUCCEEDED(comResult)) {
        CoUninitialize();
    }
    return 0;
}

void StartAttentionThread() {
    if (g_attentionThread || !OwnsTaskbar()) {
        return;
    }
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      (LPCWSTR)&AttentionWndProc, &g_module);
    HANDLE started = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!started) {
        return;
    }
    g_attentionThread = CreateThread(nullptr, 0, AttentionThreadProc, started,
                                     0, &g_attentionThreadId);
    if (g_attentionThread) {
        WaitForSingleObject(started, INFINITE);
    } else {
        g_attentionThreadId = 0;
    }
    CloseHandle(started);
}

void StopAttentionThread() {
    if (!g_attentionThread) {
        return;
    }
    PostThreadMessage(g_attentionThreadId, WM_QUIT, 0, 0);
    WaitForSingleObject(g_attentionThread, INFINITE);
    CloseHandle(g_attentionThread);
    g_attentionThread = nullptr;
    g_attentionThreadId = 0;
}

////////////////////////////////////////////////////////////////////////////////
// The island at the top (its own thread).
//
// The clock, network, volume, battery and notifications at the top of the
// screen: a black pill at the top center (which can be minimized to a thin
// line), or a full-width bar like the macOS menu bar, which reserves its strip
// so maximized windows start below it. The taskbar is at the bottom, so the
// island is a window of its own, drawn with Direct2D. Clicking the clock opens
// a calendar under it; clicking the network, volume or battery opens a
// control center, like on macOS.

HANDLE g_islandThread;
DWORD g_islandThreadId;

constexpr UINT_PTR kIslandTimerId = 1;
// The clock is checked this often; the rest every few ticks.
// The island wakes up on each new second, for the clock; the network,
// volume and "Do not disturb" are checked every few seconds (and right away
// when changed from the island).
// Network, volume, battery and "Do not disturb" come as events (see
// Island::StartStatusWatch); in case one is missed, they're read again this
// often (in clock ticks, one a second).
constexpr int kIslandStatusTicks = 60;
// Every couple of seconds, the island and its open panel are put back on top
// (other always-on-top windows can cover them), and an open panel's contents
// refreshed.
constexpr int kIslandRaiseTicks = 2;
// What changed, posted to the island (wParam).
constexpr UINT WM_APP_STATUS = WM_APP + 55;
enum StatusChange : WPARAM {
    kStatusVolume = 1,
    kStatusAudioDevice,
    kStatusNetwork,
    kStatusBattery,
    kStatusDoNotDisturb,
    // Another default microphone.
    kStatusMicrophoneDevice,
    // An audio device on or off (a Bluetooth one connecting).
    kStatusAudioDevices,
    // A media-panel write: the callback already supplies its output level.
    kStatusMediaVolume,
};

// The bar's messages from the shell, as a registered app bar.
constexpr UINT WM_APP_APPBAR = WM_APP + 50;

// Sizes in pixels at 100% scaling.
constexpr float kPillHeight = 30;
// The pill's height chosen (see islandPillHeight), as a factor.
float PillHeightScale();
// How much the edges show (see islandEdge), 0 to 1.
float EdgeAmount();
// Upright on a side, the pill is this wide.
constexpr float kVerticalPillWidth = 46;
constexpr float kPillTopGap = 6;
// A notch (see islandNotches): its width with nothing in it, and the room
// kept between notches (each takes at most its share of the screen; the
// wheel slides along the rest).
constexpr float kNotchMinWidth = 56;
// Between two notches' curves at the most (they grow with their items until
// they almost touch, see Island::NotchBodyMax).
constexpr float kNotchTouchGap = 8;

// A notch's shape: `body` hanging from the screen's top edge (at body.top),
// round corners under it, and between each side and the edge a curve the
// other way (`shoulder` wide along the edge); at the screen's side (`edgeLeft`,
// `edgeRight`), a curve down that side instead. `filled`: the whole shape;
// otherwise its outline away from the screen's edges (for a line along it).
winrt::com_ptr<ID2D1PathGeometry> NotchGeometry(ID2D1Factory* factory,
                                                const D2D1_RECT_F& body,
                                                float radius,
                                                float shoulder,
                                                bool edgeLeft,
                                                bool edgeRight,
                                                bool filled) {
    winrt::com_ptr<ID2D1PathGeometry> path;
    winrt::com_ptr<ID2D1GeometrySink> sink;
    if (!factory || FAILED(factory->CreatePathGeometry(path.put())) ||
        FAILED(path->Open(sink.put()))) {
        return nullptr;
    }
    const float left = body.left;
    const float right = body.right;
    const float top = body.top;
    const float bottom = body.bottom;
    const float r =
        std::clamp(radius, 0.0f,
                   std::max(0.0f, std::min((right - left) / 2, bottom - top)));
    // The curves beside it: `s` along the edge, `d` across (wider than deep
    // when big: the side is only so tall; at a corner, across its bottom).
    const float s = std::max(0.0f, shoulder);
    const float d = std::min(s, std::max(0.0f, bottom - top - r));
    const float across = std::min(s, std::max(0.0f, (right - left) / 3));
    auto curve = [&](D2D1_POINT_2F to, float width, float height,
                     bool clockwise) {
        if (width > 0.01f && height > 0.01f) {
            sink->AddArc({to,
                          {width, height},
                          0,
                          clockwise ? D2D1_SWEEP_DIRECTION_CLOCKWISE
                                    : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                          D2D1_ARC_SIZE_SMALL});
        } else {
            sink->AddLine(to);
        }
    };
    const D2D1_POINT_2F start = edgeLeft ? D2D1_POINT_2F{left, bottom + s}
                                         : D2D1_POINT_2F{left - s, top};
    sink->BeginFigure(start, filled ? D2D1_FIGURE_BEGIN_FILLED
                                    : D2D1_FIGURE_BEGIN_HOLLOW);
    if (edgeLeft) {
        curve({left + across, bottom}, across, s, true);
    } else {
        curve({left, top + d}, s, d, true);
        sink->AddLine({left, bottom - r});
        curve({left + r, bottom}, r, r, false);
    }
    if (edgeRight) {
        sink->AddLine({right - across, bottom});
        curve({right, bottom + s}, across, s, true);
    } else {
        sink->AddLine({right - r, bottom});
        curve({right, bottom - r}, r, r, false);
        sink->AddLine({right, top + d});
        curve({right + s, top}, s, d, true);
    }
    // (Along the screen's edges back to the start.)
    if (filled) {
        if (edgeRight) {
            sink->AddLine({right, top});
        }
        sink->AddLine(edgeLeft ? D2D1_POINT_2F{left, top} : start);
    }
    sink->EndFigure(filled ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
    if (FAILED(sink->Close())) {
        return nullptr;
    }
    return path;
}
constexpr float kBarHeight = 28;
constexpr float kIslandPadding = 12;
constexpr float kIslandSpacing = 6;
// Every icon on the pill the same size: the symbols (the media and control
// center buttons, the arrows too) and the pictures (apps, tray icons, the app
// in focus, what's playing's cover), at 100% of the icons' own size (see
// islandIconSize; their room grows with them). The small one is for the
// banner's own buttons.
constexpr float kIslandIconSlot = 26;
constexpr float kIslandIconSize = 15;
constexpr float kIslandPictureSize = 15;
constexpr float kIslandSmallIconSize = 11;
constexpr float kIslandTextSize = 13;
// The minimized pill: a thin line against its edge, a bit bigger under the
// mouse.
constexpr float kLineWidth = 72;
constexpr float kLineHoverWidth = 96;
constexpr float kLineHeight = 5;
// Under the mouse, the line grows this much taller.
constexpr float kLineHoverGrowth = 3;
// The panels under the island.
constexpr float kPanelGap = 8;
constexpr float kNotificationsWidth = 360;
constexpr float kNotificationHeight = 74;
constexpr int kMaxNotifications = 50;
constexpr float kSwitchRowHeight = 50;
constexpr float kPanelPadding = 16;
constexpr float kPanelRadius = 16;
constexpr float kCalendarCell = 36;
constexpr float kControlWidth = 340;
constexpr float kControlPadding = 12;
constexpr float kControlGap = 10;
constexpr float kTileHeight = 62;
constexpr float kSoundHeight = 92;
constexpr float kDisplayHeight = 72;
// With more monitors: each one's name and slider.
constexpr float kBrightnessRowHeight = 52;
constexpr float kShortcutHeight = 64;
constexpr float kBatteryHeight = 50;
constexpr float kPowerModeHeight = 76;
constexpr float kFooterHeight = 30;

// The font the island's icons come from: Segoe Fluent Icons (Windows 11), or
// on Windows 10, which doesn't have it, Segoe MDL2 Assets (the same icons at
// the same places, but for a few newer ones).
PCWSTR IconFont() {
    static const bool fluent = [] {
        WCHAR file[MAX_PATH];
        DWORD size = sizeof(file);
        return RegGetValue(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\"
                           L"Fonts",
                           L"Segoe Fluent Icons (TrueType)", RRF_RT_REG_SZ,
                           nullptr, file, &size) == ERROR_SUCCESS;
    }();
    return fluent ? L"Segoe Fluent Icons" : L"Segoe MDL2 Assets";
}

// Icons from the Segoe Fluent Icons font (see IconFont).
constexpr WCHAR kGlyphWifi[] = {0xE872, 0xE873, 0xE874, 0xE701};
constexpr WCHAR kGlyphEthernet = 0xE839;
// The control center's item (sliders, like macOS's).
constexpr WCHAR kGlyphControlCenter = 0xE9E9;
constexpr WCHAR kGlyphNoInternet = 0xF384;
// Connected without internet: Wi-Fi's bars with a warning sign, or the cable
// with one.
constexpr WCHAR kGlyphWifiWarning[] = {0xEB60, 0xEB61, 0xEB62, 0xEB63};
constexpr WCHAR kGlyphEthernetWarning = 0xEB56;
// Windows checks for the internet this long after connecting (shown as
// connecting); a Wi-Fi joining for longer than kNetworkConnectingMax counts
// as not connected.
constexpr double kNetworkCheckSeconds = 25;
constexpr double kNetworkConnectingMax = 60;
constexpr WCHAR kGlyphMute = 0xE74F;
constexpr WCHAR kGlyphVolume[] = {0xE992, 0xE993, 0xE994, 0xE995};
constexpr WCHAR kGlyphBell = 0xEA8F;
constexpr WCHAR kGlyphMoon = 0xE708;
constexpr WCHAR kGlyphCollapse = 0xE70E;
constexpr WCHAR kGlyphCollapseDown = 0xE70D;
constexpr WCHAR kGlyphPrevious = 0xE76B;
constexpr WCHAR kGlyphNext = 0xE76C;
constexpr WCHAR kGlyphBluetooth = 0xE702;
constexpr WCHAR kGlyphSettings = 0xE713;
constexpr WCHAR kGlyphPin = 0xE718;
constexpr WCHAR kGlyphMusic = 0xE8D6;
constexpr WCHAR kGlyphCalendar = 0xE787;
constexpr WCHAR kGlyphPlay = 0xE768;
constexpr WCHAR kGlyphStop = 0xE71A;
constexpr WCHAR kGlyphReload = 0xE72C;
// komorebi's start and stop (its gears), and its layout (a grid).
constexpr WCHAR kGlyphKomorebi = 0xE9F5;
constexpr WCHAR kGlyphKomorebiLayout = 0xE80A;
// The app in focus (see IslandPref::FocusedApp), and the desktop in focus.
constexpr WCHAR kGlyphFocusedApp = 0xECAA;
constexpr WCHAR kGlyphDesktop = 0xE7F4;
constexpr WCHAR kGlyphPause = 0xE769;
constexpr WCHAR kGlyphPreviousTrack = 0xE892;
constexpr WCHAR kGlyphNextTrack = 0xE893;
constexpr WCHAR kGlyphSpeaker = 0xE767;
constexpr WCHAR kGlyphSend = 0xE724;
constexpr WCHAR kGlyphPower = 0xE7E8;
constexpr WCHAR kGlyphProfiles = 0xE748;
// The AI agents' usage (a robot).
constexpr WCHAR kGlyphAiUsage = 0xE99A;
constexpr WCHAR kGlyphWidgets = 0xF246;

// What's at each spot of the island.
enum class IslandItem {
    None,
    Overflow,
    Network,
    Volume,
    Battery,
    Clock,
    Bell,
    Minimize,
    Line,
    // An app with a notification badge (the slot's text is its ID).
    App,
    // What's playing, and its buttons on the pill.
    Media,
    MediaPrevious,
    MediaPlayPause,
    MediaNext,
    // A new notification shown in the pill, its reply box and "Open", and
    // a call's, an alarm's or a reminder's buttons (the slot's text is the
    // button's index).
    Banner,
    BannerReply,
    BannerOpen,
    BannerClose,
    BannerMinimize,
    BannerAction,
    // Another app's tray icon (the slot's text is its key), and its pin.
    TrayIcon,
    TrayPin,
    // A faint line between the open list and the pinned icons.
    TraySeparator,
    // Sleep, restart, shut down (see PowerPanel).
    Power,
    // Windows' light or dark mode, switched.
    ThemeSwitch,
    // The microphone, muted or not.
    Microphone,
    // The user's notes, tasks and keyboard shortcuts.
    Notes,
    Tasks,
    Shortcuts,
    // The weather.
    Weather,
    // Updates waiting.
    Updates,
    // The settings' profiles, to switch (see ProfilesPanel).
    Profiles,
    // How much the processor, the memory and the video memory are used.
    UsageCpu,
    UsageRam,
    UsageVram,
    // The bubble with a gear under the island (a right click on it): the
    // island's settings.
    SettingsBubble,
    // The screen's brightness (the mouse wheel changes it).
    Brightness,
    // The keyboard's input method ("中", "英", "PT").
    InputMethod,
    // The microphone, the camera and the location in use by an app.
    PrivacyMicrophone,
    PrivacyCamera,
    PrivacyLocation,
    // USB drives plugged in (see DrivesPanel).
    Drives,
    // A control center button on the pill (see IslandSlot::action).
    QuickAction,
    // The virtual desktops: a dot each, the current one lit (see
    // DesktopsPanel).
    Desktops,
    // Folders and places (see FoldersPanel), and the wallpapers (see
    // WallpaperPanel).
    Folders,
    Wallpaper,
    // komorebi's layout on the island's monitor (see KomorebiPanel), and
    // the app in focus on it (the slot's text is the name shown).
    KomorebiLayout,
    FocusedApp,
    // A komorebi workspace's dot (the slot's text is its number, with "*"
    // for the one shown).
    KomorebiWorkspace,
    // The arrow the apps with notifications fold into (islandFoldApps).
    AppsFold,
    // The network's speed, another city's time and the time until the next
    // event (two short lines each, see kStackedFont), and the Recycle Bin
    // (see RecycleBinPanel).
    NetSpeed,
    WorldClock,
    Countdown,
    RecycleBin,
    // The timer, the stopwatch or Pomodoro (its time while it runs, see
    // TimerPanel).
    Timer,
    // Bars moving with the sound (see kVisualizerFont).
    Visualizer,
    // The line of the song sung now (see kLyricsFont).
    Lyrics,
    // The AI agents' usage (each one's app icon and its use, see
    // AiUsagePanel).
    AiUsage,
    // The user's commands: their panel's icon, and each one chosen as a
    // button (its index in Commands() as the slot's action, see
    // CommandsPanel).
    Commands,
    // The shelf's files (its icon and how many, see ShelfPanel).
    Shelf,
    // The dragon's eyes (see kEyesFont and Island::StepEyes).
    Eyes,
};

bool IsPrivacyItem(IslandItem item) {
    return item == IslandItem::PrivacyMicrophone ||
           item == IslandItem::PrivacyCamera ||
           item == IslandItem::PrivacyLocation;
}

// The items showing a name under the mouse (see Island::ItemTip): the
// icons and what's playing (not the clock, the apps' and tray icons, which
// have their own).
bool HasItemTip(IslandItem item) {
    switch (item) {
        case IslandItem::Media:
        case IslandItem::Overflow:
        case IslandItem::Network:
        case IslandItem::Volume:
        case IslandItem::Battery:
        case IslandItem::Bell:
        case IslandItem::Power:
        case IslandItem::ThemeSwitch:
        case IslandItem::Microphone:
        case IslandItem::Notes:
        case IslandItem::Tasks:
        case IslandItem::Shortcuts:
        case IslandItem::Weather:
        case IslandItem::Updates:
        case IslandItem::Profiles:
        case IslandItem::Brightness:
        case IslandItem::InputMethod:
        case IslandItem::Drives:
        case IslandItem::QuickAction:
        case IslandItem::Desktops:
        case IslandItem::Folders:
        case IslandItem::Wallpaper:
        case IslandItem::KomorebiLayout:
        case IslandItem::FocusedApp:
        case IslandItem::KomorebiWorkspace:
        case IslandItem::AppsFold:
        case IslandItem::NetSpeed:
        case IslandItem::WorldClock:
        case IslandItem::Countdown:
        case IslandItem::RecycleBin:
        case IslandItem::Timer:
        case IslandItem::Visualizer:
        case IslandItem::Lyrics:
        case IslandItem::AiUsage:
        case IslandItem::Commands:
        case IslandItem::Shelf:
        case IslandItem::Eyes:
            return true;
        default:
            return IsPrivacyItem(item);
    }
}

struct IslandSlot {
    IslandItem item;
    D2D1_RECT_F rect;
    std::wstring text;
    // 0: text, 1: icon, 3: the app's icon (see App), 4: a tray icon (see
    // TrayIcon).
    int font;
    // How far an app's icon has grown in (see Island::AppEntry).
    float scale = 1;
    // Its group (a PillGroup), or -1 (the minimize button).
    int group = -1;
    // A tray icon in the open list, which may be cut at its edges (see
    // Island::m_trayScroll). The list's own rect is a slot of its own
    // (kTrayListFont), not drawn.
    bool inList = false;
    // Its opacity (its group fading to another place, see
    // Island::m_groupSwap).
    float alpha = 1;
    // A control center button's index in kPillActions (see QuickAction).
    int action = -1;
};

// What tells a slot from the others of its kind (an app's or a tray icon's
// key, a control center button's index).
// (A timer's: its mode, see PillGroup::Timer; a pinned command's: its
// index, see PillGroup::Commands.)
std::wstring SlotKey(const IslandSlot& slot) {
    return slot.item == IslandItem::QuickAction ||
                   ((slot.item == IslandItem::Timer ||
                     slot.item == IslandItem::Commands) &&
                    slot.action >= 0)
               ? std::to_wstring(slot.action)
               : slot.text;
}
constexpr int kTrayListFont = 20;
// The virtual desktops' dots (see IslandItem::Desktops).
constexpr int kDesktopsFont = 21;
// An icon and a name (komorebi's layout, the app in focus), the name up to
// this wide.
constexpr int kNamedItemFont = 22;
// komorebi's workspace dot (lit for the one shown).
constexpr int kWorkspaceDotFont = 23;
// Two short lines: a small dim label over a value (see Island::StackedWidth).
constexpr int kStackedFont = 24;
// A song's line (see PillGroup::Lyrics): drawn in its own size and color,
// the next one coming up as the one before goes.
constexpr int kLyricsFont = 26;
// The visualizer's bars (see Island::m_vizLevels).
constexpr int kVisualizerFont = 25;
// An app's or a file's icon without a notification's dot (the slot's text is
// the app's ID or the file: an AI agent's app, see Island::AiAgentApp; a
// pinned command's, see UserCommand::iconPath).
constexpr int kAppIconFont = 27;
// The dragon's eyes (see EyesState): drawn by DrawEyes.
constexpr int kEyesFont = 28;
constexpr float kItemNameMaxWidth = 140;

constexpr GUID kCLSID_MMDeviceEnumerator = {
    0xbcde0395,
    0xe52f,
    0x467c,
    {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e}};
constexpr GUID kIID_IMMDeviceEnumerator = {
    0xa95664d2,
    0x9614,
    0x4f35,
    {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}};
// IAudioMeterInformation (the visualizer's peak meter) is only declared by
// name in this compiler's endpointvolume.h: its methods are here, skipped once
// the header has them (like mutealert.wh.cpp).
#ifndef __IAudioMeterInformation_INTERFACE_DEFINED__
#define __IAudioMeterInformation_INTERFACE_DEFINED__
MIDL_INTERFACE("c02216f6-8c67-4b5b-9d00-d008e73e0064")
IAudioMeterInformation : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetPeakValue(float* peak) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMeteringChannelCount(
        UINT* channelCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetChannelsPeakValues(
        UINT32 channelCount, float* peakValues) = 0;
    virtual HRESULT STDMETHODCALLTYPE QueryHardwareSupport(
        DWORD* hardwareSupportMask) = 0;
};
#endif
constexpr GUID kIID_IAudioMeterInformation = {
    0xc02216f6,
    0x8c67,
    0x4b5b,
    {0x9d, 0x00, 0xd0, 0x08, 0xe7, 0x3e, 0x00, 0x64}};
constexpr GUID kIID_IAudioEndpointVolume = {
    0x5cdf2c82,
    0x841e,
    0x4546,
    {0x97, 0x22, 0x0c, 0xf7, 0x40, 0x78, 0x22, 0x9a}};
constexpr GUID kCLSID_NetworkListManager = {
    0xdcb00c01,
    0x570f,
    0x4a9b,
    {0x8d, 0x69, 0x19, 0x9f, 0xdb, 0xa5, 0x72, 0x3b}};
constexpr GUID kIID_INetworkListManager = {
    0xdcb00000,
    0x570f,
    0x4a9b,
    {0x8d, 0x69, 0x19, 0x9f, 0xdb, 0xa5, 0x72, 0x3b}};
constexpr PROPERTYKEY kPKEY_Device_FriendlyName = {
    {0xa45c254e,
     0xdf1c,
     0x4efd,
     {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}},
    14};

