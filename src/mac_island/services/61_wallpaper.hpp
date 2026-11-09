#pragma once
// The wallpaper switcher (an option): the pictures of a folder the user
// picks, as thumbnails (Windows' own, from its thumbnail cache) in a list
// opened from the pill. One chosen comes in with a transition, drawn by a
// window of the mod's just above the desktop (under the apps' windows), is
// set as Windows' wallpaper of that monitor (IDesktopWallpaper), and the
// window fades away. The mouse wheel on the item goes to the next or the
// previous picture. Nothing else is changed (the pictures stay where they
// are).
constexpr PCWSTR kWallpaperTypes[] = {L".jpg",  L".jpeg", L".jfif",
                                      L".png",  L".bmp",  L".webp"};
constexpr size_t kWallpaperMax = 300;
// The thumbnails read (lParam: a WallpaperJob), and a transition done.
constexpr UINT WM_APP_THUMBNAILS = WM_APP + 81;
constexpr UINT WM_APP_WALLPAPER_DONE = WM_APP + 82;
// The folder's pictures listed for the wheel (lParam: a WallpaperListJob).
constexpr UINT WM_APP_WALLPAPER_LIST = WM_APP + 84;
// The transitions (see islandWallpaperTransition).
enum class WallpaperTransition { Fade, Circle, Slide, Zoom, Count };

// The pictures in a folder, sorted as Explorer sorts them.
std::vector<std::wstring> ListWallpapers(const std::wstring& folder) {
    std::vector<std::wstring> paths;
    if (folder.empty()) {
        return paths;
    }
    const std::wstring base =
        folder.back() == L'\\' ? folder : folder + L"\\";
    WIN32_FIND_DATA data;
    HANDLE find = FindFirstFileEx((base + L"*").c_str(), FindExInfoBasic,
                                  &data, FindExSearchNameMatch, nullptr,
                                  FIND_FIRST_EX_LARGE_FETCH);
    if (find == INVALID_HANDLE_VALUE) {
        return paths;
    }
    do {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        PCWSTR extension = PathFindExtension(data.cFileName);
        for (PCWSTR type : kWallpaperTypes) {
            if (_wcsicmp(extension, type) == 0) {
                paths.push_back(base + data.cFileName);
                break;
            }
        }
    } while (paths.size() < kWallpaperMax && FindNextFile(find, &data));
    FindClose(find);
    std::sort(paths.begin(), paths.end(),
              [](const std::wstring& a, const std::wstring& b) {
                  return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
              });
    return paths;
}

// Pictures listed (when `paths` comes empty) and some of their thumbnails,
// read on a thread.
struct WallpaperJob {
    HWND panel;
    std::wstring folder;
    std::vector<std::wstring> paths;
    size_t first;
    size_t count;
    UINT generation;
    SIZE size;
    std::vector<HBITMAP> thumbnails;
};

// (Not taken by its window.)
void FreeWallpaperJob(WallpaperJob* job) {
    for (HBITMAP thumbnail : job->thumbnails) {
        if (thumbnail) {
            DeleteObject(thumbnail);
        }
    }
    delete job;
}

DWORD WINAPI WallpaperThumbnailsProc(LPVOID parameter) {
    auto* job = (WallpaperJob*)parameter;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (job->paths.empty()) {
        job->paths = ListWallpapers(job->folder);
    }
    for (size_t i = job->first;
         i < job->paths.size() && i < job->first + job->count; i++) {
        HBITMAP thumbnail = nullptr;
        winrt::com_ptr<IShellItemImageFactory> factory;
        if (SUCCEEDED(SHCreateItemFromParsingName(
                job->paths[i].c_str(), nullptr, IID_PPV_ARGS(factory.put())))) {
            factory->GetImage(job->size, SIIGBF_BIGGERSIZEOK, &thumbnail);
        }
        job->thumbnails.push_back(thumbnail);
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    if (!PostMessage(job->panel, WM_APP_THUMBNAILS, 0, (LPARAM)job)) {
        FreeWallpaperJob(job);
    }
    return 0;
}

// The folder's pictures for the wheel on the pill, listed on a thread (a
// sleeping network drive can take long to answer); the step taken then.
struct WallpaperListJob {
    HWND island;
    std::wstring folder;
    int step;
    std::vector<std::wstring> paths;
};

DWORD WINAPI WallpaperListThreadProc(LPVOID parameter) {
    auto* job = (WallpaperListJob*)parameter;
    job->paths = ListWallpapers(job->folder);
    if (!PostMessage(job->island, WM_APP_WALLPAPER_LIST, 0, (LPARAM)job)) {
        delete job;
    }
    return 0;
}

// Windows showing a new wallpaper (see WallpaperChangeThreadProc): looked
// at this often, for this long at most; without a look (an older Windows),
// the transition's window goes after a moment.
constexpr DWORD kWallpaperSampleMs = 50;
constexpr double kWallpaperShownSeconds = 2.5;
constexpr DWORD kWallpaperShownFallbackMs = 400;

// A picture coming in as the wallpaper of a monitor (see above).
struct WallpaperChange {
    HWND island;
    std::wstring path;
    RECT monitor;
    WallpaperTransition transition;
    // Where a circle grows from (on the screen).
    POINT origin;
};

// The part of a picture scaled to `scaledWidth` x `scaledHeight` shown on a
// monitor of `size`, cut where it's bigger: in the middle, except for a
// filling picture's height, where Windows keeps a third of the cut part
// above and two thirds below (measured on Windows 11 for many sizes).
WICRect WallpaperCut(UINT scaledWidth,
                     UINT scaledHeight,
                     SIZE size,
                     DESKTOP_WALLPAPER_POSITION position) {
    const UINT shownWidth = std::min<UINT>(scaledWidth, size.cx);
    const UINT shownHeight = std::min<UINT>(scaledHeight, size.cy);
    const bool fills = position == DWPOS_FILL || position == DWPOS_SPAN;
    return {(INT)(scaledWidth - shownWidth) / 2,
            (INT)std::lround((scaledHeight - shownHeight) /
                             (fills ? 3.0 : 2.0)),
            (INT)shownWidth, (INT)shownHeight};
}

// The picture as Windows will show it on a monitor of `size`, by its
// wallpaper position: cut to fill it ("Fill"; "Span" too, set for one
// monitor as here), whole inside it ("Fit"), stretched, or at its own size
// in the middle ("Center", "Tile"); turned as the photo says (its EXIF
// orientation). `place` gets where it goes on the monitor.
winrt::com_ptr<IWICBitmapSource> LoadWallpaperPicture(
    IWICImagingFactory* wic,
    PCWSTR path,
    SIZE size,
    DESKTOP_WALLPAPER_POSITION position,
    D2D1_RECT_F* place) {
    winrt::com_ptr<IWICBitmapDecoder> decoder;
    winrt::com_ptr<IWICBitmapFrameDecode> frame;
    if (FAILED(wic->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
                                              WICDecodeMetadataCacheOnDemand,
                                              decoder.put())) ||
        FAILED(decoder->GetFrame(0, frame.put()))) {
        return nullptr;
    }
    winrt::com_ptr<IWICBitmapSource> source = frame.as<IWICBitmapSource>();
    // Turned as the camera says (JPEG's EXIF orientation).
    winrt::com_ptr<IWICMetadataQueryReader> metadata;
    PROPVARIANT value;
    PropVariantInit(&value);
    if (SUCCEEDED(frame->GetMetadataQueryReader(metadata.put())) &&
        SUCCEEDED(metadata->GetMetadataByName(L"/app1/ifd/{ushort=274}",
                                              &value)) &&
        value.vt == VT_UI2) {
        const WICBitmapTransformOptions turn =
            value.uiVal == 3   ? WICBitmapTransformRotate180
            : value.uiVal == 6 ? WICBitmapTransformRotate90
            : value.uiVal == 8 ? WICBitmapTransformRotate270
                               : WICBitmapTransformRotate0;
        winrt::com_ptr<IWICBitmapFlipRotator> rotator;
        if (turn != WICBitmapTransformRotate0 &&
            SUCCEEDED(wic->CreateBitmapFlipRotator(rotator.put())) &&
            SUCCEEDED(rotator->Initialize(source.get(), turn))) {
            source = rotator.as<IWICBitmapSource>();
        }
    }
    PropVariantClear(&value);
    UINT width = 0, height = 0;
    if (FAILED(source->GetSize(&width, &height)) || !width || !height) {
        return nullptr;
    }
    const double fill = std::max((double)size.cx / width,
                                 (double)size.cy / height);
    const double fit = std::min((double)size.cx / width,
                                (double)size.cy / height);
    UINT scaledWidth = width, scaledHeight = height;
    switch (position) {
        case DWPOS_FIT:
            scaledWidth = std::max<UINT>(1, (UINT)std::lround(width * fit));
            scaledHeight = std::max<UINT>(1, (UINT)std::lround(height * fit));
            break;
        case DWPOS_STRETCH:
            scaledWidth = size.cx;
            scaledHeight = size.cy;
            break;
        case DWPOS_CENTER:
        case DWPOS_TILE:
            break;
        default:
            scaledWidth =
                std::max<UINT>(size.cx, (UINT)std::ceil(width * fill));
            scaledHeight =
                std::max<UINT>(size.cy, (UINT)std::ceil(height * fill));
            break;
    }
    const WICRect crop =
        WallpaperCut(scaledWidth, scaledHeight, size, position);
    const UINT shownWidth = crop.Width;
    const UINT shownHeight = crop.Height;
    winrt::com_ptr<IWICBitmapScaler> scaler;
    winrt::com_ptr<IWICBitmapClipper> clipper;
    winrt::com_ptr<IWICFormatConverter> converter;
    if (FAILED(wic->CreateBitmapScaler(scaler.put())) ||
        FAILED(scaler->Initialize(source.get(), scaledWidth, scaledHeight,
                                  WICBitmapInterpolationModeFant)) ||
        FAILED(wic->CreateBitmapClipper(clipper.put())) ||
        FAILED(clipper->Initialize(scaler.get(), &crop)) ||
        FAILED(wic->CreateFormatConverter(converter.put())) ||
        FAILED(converter->Initialize(clipper.get(),
                                     GUID_WICPixelFormat32bppPBGRA,
                                     WICBitmapDitherTypeNone, nullptr, 0,
                                     WICBitmapPaletteTypeMedianCut))) {
        return nullptr;
    }
    const float left = (size.cx - (float)shownWidth) / 2;
    const float top = (size.cy - (float)shownHeight) / 2;
    *place = {left, top, left + shownWidth, top + shownHeight};
    return converter.as<IWICBitmapSource>();
}

// What's on the monitor's screen, shrunk to a few pixels (a window excluded
// from captures isn't in it): to see when Windows shows a new wallpaper.
constexpr int kScreenSampleWidth = 48;
constexpr int kScreenSampleHeight = 27;

std::vector<DWORD> SampleScreen(const RECT& monitor) {
    std::vector<DWORD> pixels;
    HDC screen = GetDC(nullptr);
    HDC dc = screen ? CreateCompatibleDC(screen) : nullptr;
    BITMAPINFO info{};
    info.bmiHeader = {sizeof(BITMAPINFOHEADER), kScreenSampleWidth,
                      -kScreenSampleHeight, 1, 32, BI_RGB};
    void* bits = nullptr;
    HBITMAP bitmap = dc ? CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits,
                                           nullptr, 0)
                        : nullptr;
    if (bitmap) {
        HGDIOBJ old = SelectObject(dc, bitmap);
        SetStretchBltMode(dc, HALFTONE);
        if (StretchBlt(dc, 0, 0, kScreenSampleWidth, kScreenSampleHeight,
                       screen, monitor.left, monitor.top,
                       monitor.right - monitor.left,
                       monitor.bottom - monitor.top, SRCCOPY)) {
            GdiFlush();
            pixels.assign((const DWORD*)bits,
                          (const DWORD*)bits +
                              kScreenSampleWidth * kScreenSampleHeight);
        }
        SelectObject(dc, old);
        DeleteObject(bitmap);
    }
    if (dc) {
        DeleteDC(dc);
    }
    if (screen) {
        ReleaseDC(nullptr, screen);
    }
    return pixels;
}

// Whether two pixels differ by more than `threshold` in a color.
bool PixelDiffers(DWORD a, DWORD b, int threshold) {
    for (int shift : {0, 8, 16}) {
        if (std::abs((int)((a >> shift) & 0xFF) - (int)((b >> shift) & 0xFF)) >
            threshold) {
            return true;
        }
    }
    return false;
}

// How many of two samples' pixels differ by more than `threshold` in a
// color.
int SampleChanges(const std::vector<DWORD>& a,
                  const std::vector<DWORD>& b,
                  int threshold) {
    int changes = 0;
    for (size_t i = 0; i < a.size() && i < b.size(); i++) {
        changes += PixelDiffers(a[i], b[i], threshold);
    }
    return changes;
}

// Whether the screen (`now`) shows the new picture as the transition's
// window showed it (`wanted`), at the pixels where it differs from the old
// wallpaper (`places`): all of them but a few (a video or a moving window
// there). In between, Windows is still bringing it in (its own fade and
// zoom).
bool SampleShows(const std::vector<DWORD>& now,
                 const std::vector<DWORD>& wanted,
                 const std::vector<size_t>& places) {
    size_t off = 0;
    for (size_t i : places) {
        if (i >= now.size() || i >= wanted.size() ||
            PixelDiffers(now[i], wanted[i], 24)) {
            off++;
        }
    }
    return off * 10 <= places.size();
}

// Windows' wallpaper of the monitor (all of them if it isn't found).
void SetMonitorWallpaper(const RECT& monitor, PCWSTR path) {
    winrt::com_ptr<IDesktopWallpaper> wallpaper;
    if (FAILED(CoCreateInstance(CLSID_DesktopWallpaper, nullptr,
                                CLSCTX_ALL, IID_PPV_ARGS(wallpaper.put())))) {
        return;
    }
    UINT count = 0;
    wallpaper->GetMonitorDevicePathCount(&count);
    for (UINT i = 0; i < count; i++) {
        PWSTR id = nullptr;
        RECT rect{};
        if (SUCCEEDED(wallpaper->GetMonitorDevicePathAt(i, &id)) &&
            SUCCEEDED(wallpaper->GetMonitorRECT(id, &rect)) &&
            EqualRect(&rect, &monitor)) {
            wallpaper->SetWallpaper(id, path);
            CoTaskMemFree(id);
            return;
        }
        CoTaskMemFree(id);
    }
    // (Not matched: only with a single monitor, it's that one; otherwise
    // the others' wallpapers stay as they are.)
    if (count == 1) {
        wallpaper->SetWallpaper(nullptr, path);
    }
}

DWORD WINAPI WallpaperChangeThreadProc(LPVOID parameter) {
    std::unique_ptr<WallpaperChange> change((WallpaperChange*)parameter);
    // (In the monitor's real pixels, like the island.)
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // How Windows places its wallpaper, and the color around a smaller one.
    DESKTOP_WALLPAPER_POSITION wallpaperPosition = DWPOS_FILL;
    COLORREF backgroundColor = 0;
    {
        winrt::com_ptr<IDesktopWallpaper> wallpaper;
        if (SUCCEEDED(CoCreateInstance(CLSID_DesktopWallpaper, nullptr,
                                       CLSCTX_ALL,
                                       IID_PPV_ARGS(wallpaper.put())))) {
            wallpaper->GetPosition(&wallpaperPosition);
            wallpaper->GetBackgroundColor(&backgroundColor);
        }
    }
    D2D1_RECT_F pictureRect{};
    const SIZE size{change->monitor.right - change->monitor.left,
                    change->monitor.bottom - change->monitor.top};
    const double length = AnimationSeconds(0.7);
    winrt::com_ptr<IWICImagingFactory> wic;
    winrt::com_ptr<ID2D1Factory> factory;
    winrt::com_ptr<IWICBitmapSource> picture;
    LayeredCanvas canvas;
    canvas.tellsLostDevice = false;
    HWND window = nullptr;
    if (length > 0 &&
        SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(wic.put()))) &&
        SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                    factory.put())) &&
        canvas.Create(factory.get()) &&
        (picture = LoadWallpaperPicture(wic.get(), change->path.c_str(),
                                        size, wallpaperPosition,
                                        &pictureRect))) {
        // It doesn't take the mouse, and stays under the apps' windows,
        // just above the desktop.
        window = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT |
                                    WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                L"STATIC", nullptr, WS_POPUP, 0, 0, 0, 0,
                                nullptr, nullptr, nullptr, nullptr);
    }
    winrt::com_ptr<ID2D1Bitmap> bitmap;
    if (window && canvas.Begin(size)) {
        canvas.target->CreateBitmapFromWicBitmap(picture.get(), nullptr,
                                                 bitmap.put());
        canvas.Finish();
    }
    const POINT position{change->monitor.left, change->monitor.top};
    // Just above the desktop: under the first window shown above it (an
    // app's, or a taskbar, which isn't always on top while the desktop is in
    // front: measured, the picture covered the second monitor's taskbar), not
    // under a hidden one; the top of the windows not on top when none is.
    auto placeOverDesktop = [window]() {
        HWND progman = FindWindow(L"Progman", nullptr);
        HWND above = progman ? GetWindow(progman, GW_HWNDPREV) : nullptr;
        for (int i = 0; above && i < 1000 &&
                        (above == window || !IsWindowVisible(above));
             i++) {
            above = GetWindow(above, GW_HWNDPREV);
        }
        const bool onTop =
            above && (GetWindowLongPtr(above, GWL_EXSTYLE) & WS_EX_TOPMOST);
        SetWindowPos(window, !progman ? HWND_BOTTOM : above && !onTop ? above
                                                                      : HWND_TOP,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    };
    if (window && bitmap) {
        // Not a full-screen app to the taskbar (it covers the whole monitor):
        // measured, a taskbar left showing the desktop stopped being on top
        // during the transition, under it (explorer's "NonRudeHWND" mark).
        SetProp(window, L"NonRudeHWND", (HANDLE)TRUE);
        placeOverDesktop();
        const float w = (float)size.cx;
        const float h = (float)size.cy;
        const D2D1_POINT_2F origin{
            (float)(change->origin.x - change->monitor.left),
            (float)(change->origin.y - change->monitor.top)};
        const float reach = std::hypot(std::max(origin.x, w - origin.x),
                                       std::max(origin.y, h - origin.y));
        FrameTimer timer;
        const double start = NowSeconds();
        bool shown = false;
        for (;;) {
            const double t =
                std::clamp((NowSeconds() - start) / length, 0.0, 1.0);
            const float k = (float)(t < 0.5 ? 4 * t * t * t
                                            : 1 - std::pow(-2 * t + 2, 3) / 2);
            if (!canvas.Begin(size)) {
                break;
            }
            ID2D1DCRenderTarget* target = canvas.target.get();
            const D2D1_RECT_F full{0, 0, w, h};
            // The background color around a smaller picture, then it (`x`
            // to the right).
            winrt::com_ptr<ID2D1SolidColorBrush> fillBrush;
            target->CreateSolidColorBrush(
                D2D1::ColorF(GetRValue(backgroundColor) / 255.0f,
                             GetGValue(backgroundColor) / 255.0f,
                             GetBValue(backgroundColor) / 255.0f, 1),
                fillBrush.put());
            auto drawPicture = [&](float x, float opacity) {
                if (fillBrush) {
                    fillBrush->SetOpacity(opacity);
                    target->FillRectangle({x, 0, x + w, h}, fillBrush.get());
                }
                DrawBitmapSmooth(target, bitmap.get(),
                                 {pictureRect.left + x, pictureRect.top,
                                  pictureRect.right + x, pictureRect.bottom},
                                 opacity);
            };
            switch (change->transition) {
                case WallpaperTransition::Circle: {
                    winrt::com_ptr<ID2D1EllipseGeometry> circle;
                    factory->CreateEllipseGeometry(
                        D2D1::Ellipse(origin, reach * k, reach * k),
                        circle.put());
                    winrt::com_ptr<ID2D1Layer> layer;
                    if (circle &&
                        SUCCEEDED(target->CreateLayer(nullptr, layer.put()))) {
                        target->PushLayer(
                            D2D1::LayerParameters(full, circle.get()),
                            layer.get());
                        drawPicture(0, 1);
                        target->PopLayer();
                    }
                    break;
                }
                case WallpaperTransition::Slide: {
                    drawPicture((1 - k) * w, 0.4f + 0.6f * k);
                    break;
                }
                case WallpaperTransition::Zoom: {
                    target->SetTransform(D2D1::Matrix3x2F::Scale(
                        1.12f - 0.12f * k, 1.12f - 0.12f * k,
                        {w / 2, h / 2}));
                    drawPicture(0, k);
                    target->SetTransform(D2D1::Matrix3x2F::Identity());
                    break;
                }
                default:
                    drawPicture(0, k);
                    break;
            }
            canvas.End(window, position, 1);
            if (!shown) {
                ShowWindow(window, SW_SHOWNOACTIVATE);
                placeOverDesktop();
                shown = true;
            }
            if (t >= 1) {
                break;
            }
            WaitForAnimationFrame(timer);
        }
    }
    // The screen with the new picture shown, then what's under it: the old
    // wallpaper. (Left out of captures while it waits for the new one,
    // screen recordings too, from the next composition on.)
    std::vector<DWORD> wanted;
    if (window) {
        DwmFlush();
        wanted = SampleScreen(change->monitor);
    }
    const bool sampled =
        !wanted.empty() &&
        SetWindowDisplayAffinity(window, WDA_EXCLUDEFROMCAPTURE) != FALSE;
    if (sampled) {
        DwmFlush();
    }
    const std::vector<DWORD> before =
        sampled ? SampleScreen(change->monitor) : std::vector<DWORD>{};
    SetMonitorWallpaper(change->monitor, change->path.c_str());
    if (window) {
        // It stays until Windows shows the new one under it as it does (its
        // own fade and zoom done: the desktop's pixels look like it and
        // hold still), not over the old one; then it fades away. With
        // nothing of the desktop showing (or the same picture), it goes at
        // once; after a while at most, and at once when the mod is
        // unloading.
        const int pixels = kScreenSampleWidth * kScreenSampleHeight;
        std::vector<size_t> places;
        if (before.size() == wanted.size()) {
            for (size_t i = 0; i < before.size(); i++) {
                if (PixelDiffers(before[i], wanted[i], 24)) {
                    places.push_back(i);
                }
            }
        }
        const double since = NowSeconds();
        std::vector<DWORD> last = before;
        int still = 0;
        while (!places.empty() && !g_unloading &&
               NowSeconds() - since < kWallpaperShownSeconds && still < 2) {
            Sleep(kWallpaperSampleMs);
            std::vector<DWORD> now = SampleScreen(change->monitor);
            if (now.size() != before.size()) {
                break;
            }
            // (A fade moves a few levels a look: "still" is almost none.)
            still = SampleShows(now, wanted, places) &&
                            SampleChanges(now, last, 4) <= pixels / 50
                        ? still + 1
                        : 0;
            last = std::move(now);
        }
        if (before.empty()) {
            Sleep(kWallpaperShownFallbackMs);
        }
        if (sampled) {
            SetWindowDisplayAffinity(window, WDA_NONE);
        }
        for (int step = 8; step >= 0; step--) {
            BLENDFUNCTION blend{AC_SRC_OVER, 0, (BYTE)(step * 255 / 8),
                                AC_SRC_ALPHA};
            UpdateLayeredWindow(window, nullptr, nullptr, nullptr, nullptr,
                                nullptr, 0, &blend, ULW_ALPHA);
            Sleep(25);
        }
        RemoveProp(window, L"NonRudeHWND");
        DestroyWindow(window);
    }
    bitmap = nullptr;
    canvas.Destroy();
    canvas.target = nullptr;
    factory = nullptr;
    picture = nullptr;
    wic = nullptr;
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    PostMessage(change->island, WM_APP_WALLPAPER_DONE, 0, 0);
    return 0;
}

// A picture of the folder chosen (see Island::ChangeWallpaper).
void ChooseWallpaper(Island* island,
                     const std::vector<std::wstring>& paths,
                     int index);

// The list opened from the wallpaper's item: the folder (and a button to
// choose another), and its pictures' thumbnails, a page at a time (the
// wheel turns the pages).
class WallpaperPanel : public Panel {
   protected:
    void Prepare(float scale) override {
        if (!m_wic) {
            CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                             CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_wic.put()));
        }
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           12.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_buttonFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 12 * scale);
        }
        Layout();
    }

    void OnOpen() override {
        m_hover = -1;
        const std::wstring folder =
            GetWholeStringValue(L"wallpaperFolder", MAX_PATH);
        if (folder != m_folder || m_paths.empty()) {
            m_folder = folder;
            m_paths.clear();
            m_page = 0;
            m_thumbnails.clear();
            Read(true);
        }
        Layout();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

    bool OnMouseMove(POINT pt) override {
        const int hover = HitTest(pt);
        if (hover == m_hover) {
            return false;
        }
        m_hover = hover;
        return true;
    }

    bool OnMouseLeave() override {
        m_hover = -1;
        return true;
    }

    bool OnMouseUp(POINT pt) override;

    // Another page.
    bool OnWheel(int delta) override {
        const int pages = PageCount();
        const int page = std::clamp(m_page + (delta < 0 ? 1 : -1), 0,
                                    std::max(0, pages - 1));
        if (page == m_page) {
            return false;
        }
        m_page = page;
        m_hover = -1;
        Read(false);
        Layout();
        return true;
    }

    bool OnAppMessage(UINT msg, WPARAM, LPARAM lParam) override;

    void OnDestroy() override {
        // (A folder dialog still open is closed.) What the threads posted to
        // this window, about to go, is freed here.
        for (HANDLE& thread : m_threads) {
            EndFileDialogThread(&thread);
        }
        m_threads.clear();
        MSG msg;
        while (m_hwnd && PeekMessage(&msg, m_hwnd, WM_APP_FOLDER_PICKED,
                                     WM_APP_THUMBNAILS, PM_REMOVE)) {
            if (msg.message == WM_APP_THUMBNAILS) {
                FreeWallpaperJob((WallpaperJob*)msg.lParam);
            } else {
                delete (FolderPickJob*)msg.lParam;
            }
        }
    }

   private:
    static constexpr int kColumns = 4;
    static constexpr int kRows = 3;
    static constexpr int kPageSize = kColumns * kRows;
    static constexpr int kHitFolder = 1000;
    // The transitions' chips (then the transition's index).
    static constexpr int kHitTransition = 2000;

    int PageCount() const {
        return (int)((m_paths.size() + kPageSize - 1) / kPageSize);
    }

    // The page's thumbnails (and the pictures, the first time) on a thread.
    void Read(bool list) {
        if (m_folder.empty()) {
            return;
        }
        m_generation++;
        const size_t first = (size_t)m_page * kPageSize;
        bool missing = list;
        for (size_t i = first; i < first + kPageSize && i < m_paths.size();
             i++) {
            missing = missing || !Tried(m_paths[i]);
        }
        if (!missing) {
            return;
        }
        m_loading = true;
        const float s = m_scale;
        auto* job = new WallpaperJob{
            m_hwnd,       m_folder,
            list ? std::vector<std::wstring>{} : m_paths,
            first,        (size_t)kPageSize,
            m_generation, {(LONG)(150 * s), (LONG)(90 * s)},
            {}};
        std::erase_if(m_threads, [](HANDLE thread) {
            if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0) {
                CloseHandle(thread);
                return true;
            }
            return false;
        });
        if (HANDLE thread = CreateThread(nullptr, 0, WallpaperThumbnailsProc,
                                         job, 0, nullptr)) {
            m_threads.push_back(thread);
        } else {
            delete job;
            m_loading = false;
        }
    }

    ID2D1Bitmap* Thumbnail(const std::wstring& path) const {
        for (const auto& [key, bitmap] : m_thumbnails) {
            if (key == path) {
                return bitmap.get();
            }
        }
        return nullptr;
    }

    // Read already (a picture whose thumbnail failed keeps an empty entry:
    // it isn't read again each time its page shows).
    bool Tried(const std::wstring& path) const {
        return std::any_of(m_thumbnails.begin(), m_thumbnails.end(),
                           [&](const auto& entry) {
                               return entry.first == path;
                           });
    }

    void Layout() {
        const float s = m_scale;
        const float padding = std::round(10 * s);
        const float cell = std::round(124 * s);
        const float cellHeight = std::round(74 * s);
        const float gap = std::round(8 * s);
        const float width =
            padding * 2 + kColumns * cell + (kColumns - 1) * gap;
        float y = padding;
        m_folderButton = {width - padding - std::round(96 * s), y,
                          width - padding, y + std::round(28 * s)};
        m_header = {padding, y, m_folderButton.left - gap,
                    y + std::round(28 * s)};
        y += std::round(28 * s) + gap;
        // How a picture comes in, under them.
        m_transitions.clear();
        const float chipHeight = std::round(26 * s);
        const int transitions = (int)WallpaperTransition::Count;
        const float chipWidth = std::floor(
            (width - padding * 2 - gap * (transitions - 1)) / transitions);
        for (int i = 0; i < transitions; i++) {
            const float x = padding + (chipWidth + gap) * i;
            m_transitions.push_back({x, y, x + chipWidth, y + chipHeight});
        }
        y += chipHeight + gap;
        m_cells.clear();
        const size_t first = (size_t)m_page * kPageSize;
        const size_t shown =
            m_paths.size() > first
                ? std::min<size_t>(kPageSize, m_paths.size() - first)
                : 0;
        const int rows = std::max(1, (int)((shown + kColumns - 1) / kColumns));
        for (size_t i = 0; i < shown; i++) {
            const float x = padding + (i % kColumns) * (cell + gap);
            const float top = y + (i / kColumns) * (cellHeight + gap);
            m_cells.push_back({x, top, x + cell, top + cellHeight});
        }
        y += rows * (cellHeight + gap);
        m_footer = {padding, y - gap, width - padding,
                    y - gap + std::round(22 * s)};
        m_size = {(LONG)width, (LONG)std::ceil(m_footer.bottom + padding / 2)};
    }

    int HitTest(POINT pt) const {
        if (PointInRect(pt, m_folderButton)) {
            return kHitFolder;
        }
        for (size_t i = 0; i < m_transitions.size(); i++) {
            if (PointInRect(pt, m_transitions[i])) {
                return kHitTransition + (int)i;
            }
        }
        for (size_t i = 0; i < m_cells.size(); i++) {
            if (PointInRect(pt, m_cells[i])) {
                return (int)i;
            }
        }
        return -1;
    }

    winrt::com_ptr<IWICImagingFactory> m_wic;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_buttonFormat;
    float m_preparedScale = 0;
    std::wstring m_folder;
    std::vector<std::wstring> m_paths;
    std::vector<std::pair<std::wstring, winrt::com_ptr<ID2D1Bitmap>>>
        m_thumbnails;
    int m_page = 0;
    UINT m_generation = 0;
    bool m_loading = false;
    std::vector<HANDLE> m_threads;
    D2D1_RECT_F m_header{}, m_folderButton{}, m_footer{};
    std::vector<D2D1_RECT_F> m_cells, m_transitions;
    int m_hover = -1;
};

bool WallpaperPanel::OnAppMessage(UINT msg, WPARAM, LPARAM lParam) {
    if (msg == WM_APP_FOLDER_PICKED) {
        std::unique_ptr<FolderPickJob> job((FolderPickJob*)lParam);
        m_dialogShown = false;
        if (!job->path.empty()) {
            SetStoredString(L"wallpaperFolder", job->path.c_str());
            m_folder = job->path;
            m_paths.clear();
            m_thumbnails.clear();
            m_page = 0;
            Read(true);
            Layout();
        }
        return true;
    }
    if (msg != WM_APP_THUMBNAILS) {
        return false;
    }
    std::unique_ptr<WallpaperJob> job((WallpaperJob*)lParam);
    const bool current = job->generation == m_generation;
    if (current) {
        m_loading = false;
        if (m_paths.empty()) {
            m_paths = job->paths;
        }
    }
    for (size_t i = 0; i < job->thumbnails.size(); i++) {
        HBITMAP thumbnail = job->thumbnails[i];
        const std::wstring& path = job->paths[job->first + i];
        // (Once per picture: one read again, or by an older page's job.)
        if (!current || Tried(path)) {
            if (thumbnail) {
                DeleteObject(thumbnail);
            }
            continue;
        }
        if (!thumbnail) {
            m_thumbnails.push_back({path, nullptr});
            continue;
        }
        winrt::com_ptr<IWICBitmap> wicBitmap;
        winrt::com_ptr<IWICFormatConverter> converter;
        winrt::com_ptr<ID2D1Bitmap> bitmap;
        if (m_wic && m_canvas.target &&
            SUCCEEDED(m_wic->CreateBitmapFromHBITMAP(
                thumbnail, nullptr, WICBitmapIgnoreAlpha, wicBitmap.put())) &&
            SUCCEEDED(m_wic->CreateFormatConverter(converter.put())) &&
            SUCCEEDED(converter->Initialize(
                wicBitmap.get(), GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0,
                WICBitmapPaletteTypeMedianCut)) &&
            SUCCEEDED(m_canvas.target->CreateBitmapFromWicBitmap(
                converter.get(), nullptr, bitmap.put()))) {
            m_thumbnails.push_back({path, bitmap});
        } else {
            m_thumbnails.push_back({path, nullptr});
        }
        DeleteObject(thumbnail);
    }
    if (current) {
        Layout();
    }
    return current;
}

bool WallpaperPanel::OnMouseUp(POINT pt) {
    const int hit = HitTest(pt);
    if (hit == kHitFolder) {
        if (m_dialogShown) {
            return false;
        }
        auto* job = new FolderPickJob{
            m_hwnd, Tr(L"The folder with your wallpapers",
                       L"A pasta com seus papéis de parede"),
            0, {}};
        m_dialogShown = true;
        if (HANDLE thread = CreateThread(nullptr, 0, FolderPickThreadProc, job,
                                         0, nullptr)) {
            m_threads.push_back(thread);
        } else {
            m_dialogShown = false;
            delete job;
        }
        return false;
    }
    if (hit >= kHitTransition) {
        SetIslandPref(IslandPref::WallpaperTransition, hit - kHitTransition);
        return true;
    }
    const size_t index = (size_t)m_page * kPageSize + hit;
    if (hit < 0 || index >= m_paths.size()) {
        return false;
    }
    Close();
    ChooseWallpaper(m_owner, m_paths, (int)index);
    return false;
}

void WallpaperPanel::Draw(ID2D1RenderTarget* target,
                          ID2D1SolidColorBrush* brush) {
    if (!m_textFormat || !m_buttonFormat) {
        return;
    }
    const float s = m_scale;
    const float radius = std::round(8 * s);
    // The folder's name, and the button.
    std::wstring folder =
        m_folder.empty()
            ? std::wstring(Tr(L"Choose the folder with your pictures",
                              L"Escolha a pasta com suas imagens"))
            : std::wstring(PathFindFileName(m_folder.c_str()));
    brush->SetColor(Fg(0.92f));
    target->DrawText(folder.c_str(), (UINT32)folder.size(), m_textFormat.get(),
                     m_header, brush);
    const float lit = Hover(HoverKey(6600, 0), m_hover == kHitFolder);
    const float buttonRadius = (m_folderButton.bottom - m_folderButton.top) / 2;
    brush->SetColor(AccentLit(0.9f, lit));
    target->FillRoundedRectangle({m_folderButton, buttonRadius, buttonRadius},
                                 brush);
    PCWSTR button = Tr(L"Folder", L"Pasta");
    brush->SetColor(AccentInk(1));
    target->DrawText(button, (UINT32)wcslen(button), m_buttonFormat.get(),
                     m_folderButton, brush);
    // The transitions, the chosen one lit.
    for (size_t i = 0; i < m_transitions.size(); i++) {
        const D2D1_RECT_F& chip = m_transitions[i];
        const bool chosen = (int)i == g_settings.wallpaperTransition;
        const float over =
            Hover(HoverKey(6602, (int)i), m_hover == kHitTransition + (int)i);
        const float chipRadius = (chip.bottom - chip.top) / 2;
        brush->SetColor(chosen ? AccentLit(0.9f, over)
                               : Fg(0.08f + 0.06f * over));
        GlassFill(target, {chip, chipRadius, chipRadius}, brush);
        const std::wstring name = ChoiceOption(kChoiceWallpaper, (int)i);
        brush->SetColor(chosen ? AccentInk(1) : Fg(0.85f));
        target->DrawText(name.c_str(), (UINT32)name.size(),
                         m_buttonFormat.get(), chip, brush);
    }
    const size_t first = (size_t)m_page * kPageSize;
    for (size_t i = 0; i < m_cells.size(); i++) {
        const D2D1_RECT_F& cell = m_cells[i];
        const float over = Hover(HoverKey(6601, (int)i), m_hover == (int)i);
        // Under the mouse, it grows a little.
        const float grow = 3 * s * over;
        const D2D1_RECT_F rect{cell.left - grow, cell.top - grow,
                               cell.right + grow, cell.bottom + grow};
        brush->SetColor(Fg(0.08f));
        GlassFill(target, {rect, radius, radius}, brush);
        if (ID2D1Bitmap* bitmap = Thumbnail(m_paths[first + i])) {
            // Cut to fill the cell, rounded.
            const D2D1_SIZE_F size = bitmap->GetSize();
            const float scale =
                std::max((rect.right - rect.left) / size.width,
                         (rect.bottom - rect.top) / size.height);
            const float w = size.width * scale;
            const float h = size.height * scale;
            const float cx = (rect.left + rect.right) / 2;
            const float cy = (rect.top + rect.bottom) / 2;
            winrt::com_ptr<ID2D1RoundedRectangleGeometry> clip;
            winrt::com_ptr<ID2D1Factory> factory;
            target->GetFactory(factory.put());
            winrt::com_ptr<ID2D1Layer> layer;
            if (factory &&
                SUCCEEDED(factory->CreateRoundedRectangleGeometry(
                    {rect, radius, radius}, clip.put())) &&
                SUCCEEDED(target->CreateLayer(nullptr, layer.put()))) {
                target->PushLayer(D2D1::LayerParameters(rect, clip.get()),
                                  layer.get());
                DrawBitmapSmooth(
                    target, bitmap,
                    {cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2});
                target->PopLayer();
            }
        }
        if (over > 0.01f) {
            brush->SetColor(Fg(0.6f * over));
            target->DrawRoundedRectangle({rect, radius, radius}, brush,
                                         1.5f * s);
        }
    }
    // The page (with more than one), or why there's nothing.
    std::wstring footer;
    if (m_loading && m_paths.empty()) {
        footer = Tr(L"Reading…", L"Lendo…");
    } else if (!m_folder.empty() && m_paths.empty()) {
        footer = Tr(L"No pictures in this folder",
                    L"Nenhuma imagem nesta pasta");
    } else if (PageCount() > 1) {
        footer = std::to_wstring(m_page + 1) + L" / " +
                 std::to_wstring(PageCount()) + L"   " +
                 Tr(L"(the wheel turns the pages)",
                    L"(a roda do mouse vira as páginas)");
    }
    if (!footer.empty()) {
        brush->SetColor(Fg(0.5f));
        target->DrawText(footer.c_str(), (UINT32)footer.size(),
                         m_textFormat.get(), m_footer, brush);
    }
}

// What a monitor's own profile changes (another island than the main one,
// see islandAllMonitors): the island's settings, its colors, its order. It's
// only put in memory while that island is the current one (see
// IslandScope): the main island's settings stay as they are, in memory and
// kept.
struct IslandLook {
    decltype(g_settings) settings;
    int themeIndex = 0;
    IslandTheme theme;
    IslandTheme themeTarget;
    IslandTheme themeFrom;
    double themeChangeStart = 0;
    bool mediaButtons = false;
    bool mediaTitle = false;
    bool banner = true;
    bool bannerMinimized = false;
    int opacity = 100;
    int trayList = 12;
    std::vector<PillGroup> order;
    std::array<int, kCustomColors> colors{};
    std::array<uint8_t, kPillGroups> notchSides{};
    std::vector<std::wstring> trayPinned;
    std::vector<std::wstring> trayOrder;
    std::vector<std::pair<std::wstring, int>> trayAnchors;
    std::wstring iconFolder;
    IconFiles iconFiles;
};
IslandLook CaptureLook();
void PutLook(const IslandLook& look);
IslandLook LookFromProfile(int id);
// The main island's look, kept while another one's is in memory.
IslandLook g_mainLook;
int LoadMonitorProfile(const std::wstring& device);
void SaveMonitorProfile(const std::wstring& device, int id);

// The banner's window at a point of the screen (see RenderPlacedBanner).
constexpr WCHAR kBannerClassName[] = L"WindhawkMacIslandDockBanner";

