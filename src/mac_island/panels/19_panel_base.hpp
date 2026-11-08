class Panel {
   public:
    // The island it belongs to.
    Island* Owner() const { return m_owner; }
    HWND Window() const { return m_hwnd; }
    virtual ~Panel() = default;
    bool Create(ID2D1Factory* factory, IDWriteFactory* dwrite);
    void Destroy();
    // `source` is the part of the island it grows out of, on the screen, and
    // `anchor` the middle of the panel's top (of its bottom when it opens
    // up). `fromPill`: it grows out of the pill to under it (over it, on the
    // lower half of the screen), under the item it opened from (otherwise it
    // drops out of an item of the bar). `behind`: under the island's window,
    // so it comes out from behind it (always, out of the pill).
    void Open(POINT anchor,
              float scale,
              D2D1_RECT_F source,
              bool fromPill,
              bool behind = false);
    // Open, it glides to another place (another item's view, see
    // Island::MovePanel); it closes back into `source` then.
    void MoveTo(POINT anchor, D2D1_RECT_F source);
    void Close();
    bool IsOpen() const { return m_target > 0; }
    // Out of the pill and on the screen (open, or still closing): the pill
    // keeps its width under it (see Island::Layout).
    bool ShownFromPill() const {
        return m_fromPill && !m_windowed &&
               (m_target > 0 || m_open > 0 || m_openX > 0);
    }
    // Which way it opens out of the island: down, up (on the lower half of
    // the screen), right or left (upright on the left or right edge).
    enum class Direction { Down, Up, Right, Left };
    void SetDirection(Direction direction) { m_direction = direction; }
    // Keeps it above the island, which is put back on top now and then.
    void Raise() {
        if (m_hwnd && (m_open > 0 || m_openX > 0)) {
            SetWindowPos(m_hwnd, ZOrderPlace(), 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            m_liquid.FollowOwner();
        }
    }
    // Over everything, or, out from under a wider pill, right under its
    // island (so it comes out from behind it and goes back behind it).
    HWND ZOrderPlace() const {
        HWND island = IslandWindow(m_owner);
        return m_underIsland && island ? island : HWND_TOPMOST;
    }
    // Being put under the island as it opens: drawn see-through meanwhile.
    bool m_placing = false;
    bool Animate(double dt);
    void Render();
    // Gets ready to open without a hitch: drawn once, off the screen, so its
    // fonts and pictures are made beforehand (for one slow to draw the first
    // time).
    void WarmUp(float scale);
    // Laid out again at another scale while open (another panel size).
    void Rescale(float scale);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    // A window of its own instead (the settings' window, see
    // ControlPanel::LayoutSettingsWindow): a normal one the user moves and
    // sizes, over Windows 11's Mica, drawn at its whole size; it shows and
    // hides at once, and stays when another window takes the focus.
    bool CreateWindowed(ID2D1Factory* factory, IDWriteFactory* dwrite);
    // Shown with `title` (in front, if it is already), in the middle of the
    // island's monitor the first time.
    // (`size`: the panels' chosen size, islandPanelSize / 100; its scale is
    // that times its own monitor's.)
    void OpenWindowed(float size, PCWSTR title);
    // What it draws on: its window's own target once made (see
    // RenderWindowed), or the layered canvas'. Pictures are made on it.
    ID2D1RenderTarget* DrawingTarget() const {
        return m_windowed ? (ID2D1RenderTarget*)m_windowTarget.get()
                          : m_canvas.target.get();
    }
    // Its window's target just made (pictures made before it can't be drawn
    // on it).
    virtual void OnWindowTargetMade() {}

   protected:
    // (A window of its own:) its size changed, or it's in front again
    // (what it shows may have changed elsewhere meanwhile).
    virtual void OnResized() {}
    virtual void OnActivated() {}
    bool m_windowed = false;
    // Placed once (then where the user leaves it), over Mica (Windows 11
    // 22H2 on), its title bar dark (see RenderWindowed), and its DPI.
    bool m_windowPlaced = false;
    bool m_mica = false;
    int m_windowDark = -1;
    UINT m_windowDpi = 96;
    float m_windowSize = 1;
    // Its icons (the mod's logo, big and small), freed with it.
    HICON m_windowIcons[2]{};
    // Its own title bar, like Windows 11's apps: Windows' caption taken off
    // (see WM_NCCALCSIZE), the Mica across the whole window, its icon and
    // name drawn over it (dimmed while another window is in front), and
    // Windows' own caption buttons (DwmDefWindowProc, its snap layouts with
    // them). The contents under it.
    bool m_windowActive = true;
    winrt::com_ptr<IDWriteTextFormat> m_captionFormat;
    UINT m_captionDpi = 0;
    // The logo as drawn for its size (the window target's: made again with
    // it).
    winrt::com_ptr<ID2D1Bitmap> m_captionLogo;
    LONG CaptionHeight() const;
    void DrawCaption(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    // Where its contents are drawn in the window: under its title bar.
    POINT ContentOffset() const {
        POINT offset = WindowContentOffset();
        offset.y += CaptionHeight();
        return offset;
    }
    // Drawn on the GPU straight onto it (its see-through parts show the
    // Mica), made with its first frame.
    ID2D1Factory* m_factory = nullptr;
    winrt::com_ptr<ID2D1HwndRenderTarget> m_windowTarget;
    void RenderWindowed();
    // Where its contents are drawn in it (a view smaller than the window, in
    // its middle).
    virtual POINT WindowContentOffset() const { return {}; }
    // Sets up the fonts and the size for this scale.
    virtual void Prepare(float scale) = 0;
    // Called when opening, before the first frame.
    virtual void OnOpen() {}
    virtual void Draw(ID2D1RenderTarget* target,
                      ID2D1SolidColorBrush* brush) = 0;
    // A panel with animated controls batches input into its next frame.
    virtual bool DeferInputDraw() const { return false; }
    // These return true to draw a new frame.
    virtual bool OnMouseMove(POINT) { return false; }
    virtual bool OnMouseLeave() { return false; }
    virtual bool OnMouseDown(POINT) { return false; }
    virtual bool OnMouseUp(POINT) { return false; }
    // The right button (down and up) and the middle one (up).
    virtual bool OnOtherButton(UINT, POINT) { return false; }
    virtual bool OnWheel(int) { return false; }
    // Escape closes the panel unless this returns true.
    virtual bool OnKey(WPARAM) { return false; }
    // A key with Alt (or F10): true if taken (otherwise Windows' own use).
    virtual bool OnSysKey(WPARAM) { return false; }
    // A key let go (with Alt too).
    virtual bool OnKeyUp(WPARAM) { return false; }
    virtual bool OnChar(WCHAR) { return false; }
    // Messages from WM_APP up, posted to the panel's window.
    virtual bool OnAppMessage(UINT, WPARAM, LPARAM) { return false; }
    // Before the window goes away.
    virtual void OnDestroy() {}
    // Starting to close.
    virtual void OnClose() {}
    // Whether it stays open when another window takes the focus (closed
    // otherwise, like a menu).
    virtual bool StaysWhenDeactivated() const { return false; }
    // The mouse held by the window taken away (it closed, or another window
    // took it).
    virtual void OnCaptureLost() {}
    // A timer set on the panel's window (other than the screenshot's); it
    // runs once.
    virtual bool OnTimer(UINT_PTR) { return false; }
    // Animates the contents while open; returns true while it moves.
    virtual bool AnimateContents(double) { return false; }
    // The most the panel can be tall: from where it opens to the far edge of
    // the screen's work area (the whole height, opening sideways). Known
    // from Prepare on.
    float MaxHeight() const;
    // A scrolled list: its contents are drawn between these two calls, at
    // the list's own coordinates (0 at its top), clipped to `viewport` and
    // fading out at an edge with more to see beyond it.
    // (`opacity`: the whole list's, fading in.)
    void BeginScrollArea(ID2D1RenderTarget* target,
                         const D2D1_RECT_F& viewport,
                         const SmoothScroll& scroll,
                         float opacity = 1);
    void EndScrollArea(ID2D1RenderTarget* target);
    // The thin bar at the list's right, full while it scrolls, and always
    // a little (`rest` of it, with a faint track) so the list shows it has
    // more; wider (`wide`, 0 to 1) with the mouse on it.
    void DrawScrollBar(ID2D1RenderTarget* target,
                       ID2D1SolidColorBrush* brush,
                       const D2D1_RECT_F& viewport,
                       const SmoothScroll& scroll,
                       float rest = 0.6f,
                       float wide = 0);
    // Where a list's bar takes the mouse (a strip at its right, `width`
    // wide: narrower beside the settings' tabs, which go up to it), and its
    // thumb's top and height.
    D2D1_RECT_F ScrollBarStrip(const D2D1_RECT_F& viewport,
                               float width = 14) const {
        return {viewport.right - std::round(width * m_scale), viewport.top,
                viewport.right, viewport.bottom};
    }
    void ScrollThumb(const D2D1_RECT_F& viewport,
                     const SmoothScroll& scroll,
                     float* top,
                     float* height) const;
    // A dialog of its own is shown (the panel stays open behind it).
    bool m_dialogShown = false;
    // Its contents changed a lot (another state, new data): they dip and
    // come back (0 to 1), as the panel stretches to its new size.
    void PulseContents() {
        m_contentsIn = 0;
        m_contentsInVelocity = 0;
        StartIslandAnimation();
    }
    double m_contentsIn = 1;
    double m_contentsInVelocity = 0;
    // Set by AnimateContents when only something that never stops moves
    // (scrolling text): drawn at a lower frame rate then, which
    // looks the same and costs much less.
    bool m_idleMotion = false;
    // Moving on the last frame, and the frames asked for when last drawn
    // (see Animate).
    bool m_moved = false;
    unsigned long long m_drawnRequest = 0;
    // Texts too long for their place scroll only under the mouse (see
    // ScrollsHere), so a panel left open draws nothing: the ones drawn last
    // time (where they are in the panel), the one under the mouse, and since
    // when.
    std::vector<D2D1_RECT_F> m_scrollSpots;
    int m_scrollSpot = -1;
    double m_scrollSince = 0;
    // A text too long for `rect` (drawn with the target's transform): true
    // while the mouse is over it (it scrolls from m_scrollSince).
    bool ScrollsHere(ID2D1RenderTarget* target, const D2D1_RECT_F& rect) {
        D2D1_MATRIX_3X2_F transform;
        target->GetTransform(&transform);
        m_scrollSpots.push_back(
            {rect.left * transform._11 + transform._31 - m_contentOffset.x,
             rect.top * transform._22 + transform._32 - m_contentOffset.y,
             rect.right * transform._11 + transform._31 - m_contentOffset.x,
             rect.bottom * transform._22 + transform._32 - m_contentOffset.y});
        return m_scrollSpot == (int)m_scrollSpots.size() - 1;
    }
    // The mouse moved in the panel (or left it: nullptr): true when it went
    // onto a long text or off one (drawn again).
    bool TrackScrollSpot(const POINT* pt) {
        int spot = -1;
        for (size_t i = 0; pt && i < m_scrollSpots.size(); i++) {
            if (PointInRect(*pt, m_scrollSpots[i])) {
                spot = (int)i;
                break;
            }
        }
        if (spot == m_scrollSpot) {
            return false;
        }
        m_scrollSpot = spot;
        m_scrollSince = NowSeconds();
        return true;
    }
    // What's under the mouse, lighting up smoothly (see HoverFades).
    HoverFades m_hoverFades;
    float Hover(const D2D1_RECT_F& rect, bool on) {
        return m_hoverFades.Get(rect, on);
    }
    float Hover(unsigned long long key, bool on, double start = 0) {
        return m_hoverFades.Get(key, on, start);
    }
    double m_lastIdleFrame = 0;

    winrt::com_ptr<IDWriteTextFormat> MakeFormat(
        PCWSTR family,
        DWRITE_FONT_WEIGHT weight,
        float size,
        DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_CENTER);

    Island* m_owner = nullptr;
    HWND m_hwnd = nullptr;
    IDWriteFactory* m_dwrite = nullptr;
    LayeredCanvas m_canvas;
    // Its Liquid Glass (islandLiquidGlass, see GlassLayer).
    GlassLayer m_liquid;
    float m_scale = 0;
    // The panel's place on the screen when open, and its size.
    POINT m_position{};
    SIZE m_size{};
    bool m_tracking = false;
    // The island part it grows out of. The window covers both, and the panel
    // is drawn at `m_contentOffset` in it.
    D2D1_RECT_F m_source{};
    POINT m_contentOffset{};

    // Opening: 0 (closed) to 1 (open), animated downwards (m_open) and
    // sideways (m_openX); downwards starts a moment later (with animations).
    double m_open = 0;
    double m_velocity = 0;
    double m_openX = 0;
    double m_velocityX = 0;
    double m_heightDelay = 0;
    double m_target = 0;
    // Out of the pill, to under it (see CenterWanted).
    bool m_fromPill = false;
    // Coming out from behind the island (see Open).
    bool m_underIsland = false;
    Direction m_direction = Direction::Down;
    // Out of the pill with panels following the island's width: at least as
    // wide as it.
    float m_minWidth = 0;
    POINT m_anchor{};
    RECT m_monitorRect{};
    RECT m_workRect{};
    // What BeginScrollArea pushed, for EndScrollArea.
    std::vector<std::pair<winrt::com_ptr<ID2D1Layer>, D2D1_MATRIX_3X2_F>>
        m_scrollAreas;
    // The size shown, following m_size when it changes (another view), so the
    // panel stretches to it.
    double m_shownWidth = 0;
    double m_shownHeight = 0;
    double m_widthVelocity = 0;
    double m_heightVelocity = 0;
    // Its middle, gliding to another place (another item, see MoveTo).
    double m_shownCenter = 0;
    double m_centerVelocity = 0;
    float CenterWanted() const;
};

LRESULT CALLBACK PanelWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                              LPARAM lParam) {
    if (auto* panel = (Panel*)GetWindowLongPtr(hWnd, GWLP_USERDATA)) {
        IslandScope scope(panel->Owner());
        return panel->HandleMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool Panel::Create(ID2D1Factory* factory, IDWriteFactory* dwrite) {
    m_dwrite = dwrite;
    // Takes the focus when open, so clicking anywhere else closes it.
    m_hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                            kPanelClassName, L"", WS_POPUP, 0, 0, 0, 0, nullptr,
                            nullptr, g_module, nullptr);
    if (!m_hwnd) {
        return false;
    }
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
    m_owner = g_island;
    return m_canvas.Create(factory);
}

bool Panel::CreateWindowed(ID2D1Factory* factory, IDWriteFactory* dwrite) {
    m_dwrite = dwrite;
    m_factory = factory;
    m_windowed = true;
    m_hwnd = CreateWindowEx(WS_EX_APPWINDOW, kPanelClassName, L"",
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr,
                            g_module, nullptr);
    if (!m_hwnd) {
        return false;
    }
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
    m_owner = g_island;
    m_windowDpi = GetDpiForWindow(m_hwnd);
    // Mica behind all of it (the frame stretched over the whole window),
    // showing where it's drawn see-through; without it (Windows 10, older
    // Windows 11), drawn on the island's own color.
    const MARGINS all{-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(m_hwnd, &all);
    const int mica = DWMSBT_MAINWINDOW;
    m_mica = SUCCEEDED(DwmSetWindowAttribute(
        m_hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &mica, sizeof(mica)));
    // Its title bar its own (no color of Windows' caption over it), its
    // frame measured again without Windows' caption (see WM_NCCALCSIZE).
    const COLORREF noCaption = DWMWA_COLOR_NONE;
    DwmSetWindowAttribute(m_hwnd, DWMWA_CAPTION_COLOR, &noCaption,
                          sizeof(noCaption));
    SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOACTIVATE);
    // Its icons: the mod's logo, big and small, at its monitor's scale.
    const UINT dpi = GetDpiForWindow(m_hwnd);
    for (int i = 0; i < 2; i++) {
        const int size =
            GetSystemMetricsForDpi(i == 0 ? SM_CXICON : SM_CXSMICON, dpi);
        if (HICON icon = MakeLogoIcon(factory, size)) {
            m_windowIcons[i] = icon;
            SendMessage(m_hwnd, WM_SETICON, i == 0 ? ICON_BIG : ICON_SMALL,
                        (LPARAM)icon);
        }
    }
    return true;
}

LONG Panel::CaptionHeight() const {
    return m_windowed ? MulDiv(kCaptionHeight, m_windowDpi, 96) : 0;
}

void Panel::DrawCaption(ID2D1RenderTarget* target,
                        ID2D1SolidColorBrush* brush) {
    const float scale = m_windowDpi / 96.0f;
    if (!m_captionFormat || m_captionDpi != m_windowDpi) {
        m_captionDpi = m_windowDpi;
        m_captionFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       12 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
    }
    if (!m_captionFormat) {
        return;
    }
    const float height = (float)CaptionHeight();
    const float dim = m_windowActive ? 1.0f : 0.5f;
    const float left = std::round(16 * scale);
    const float icon = std::round(16 * scale);
    const float iconTop = std::round((height - icon) / 2);
    if (!m_captionLogo || m_captionLogo->GetSize().width != icon) {
        m_captionLogo = nullptr;
        const D2D1_SIZE_F size{icon, icon};
        const D2D1_SIZE_U pixels{(UINT32)icon, (UINT32)icon};
        winrt::com_ptr<ID2D1BitmapRenderTarget> canvas;
        if (SUCCEEDED(target->CreateCompatibleRenderTarget(
                &size, &pixels, nullptr,
                D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, canvas.put()))) {
            canvas->BeginDraw();
            canvas->Clear(D2D1::ColorF(0, 0, 0, 0));
            DrawModLogo(canvas.get(), {0, 0, icon, icon});
            if (SUCCEEDED(canvas->EndDraw())) {
                canvas->GetBitmap(m_captionLogo.put());
            }
        }
    }
    if (m_captionLogo) {
        target->DrawBitmap(m_captionLogo.get(),
                           {left, iconTop, left + icon, iconTop + icon},
                           m_windowActive ? 1.0f : 0.6f);
    }
    WCHAR title[128];
    const int length = GetWindowText(m_hwnd, title, ARRAYSIZE(title));
    // (Up to Windows' caption buttons.)
    RECT client{};
    GetClientRect(m_hwnd, &client);
    brush->SetColor(Fg(0.85f * dim));
    target->DrawText(title, (UINT32)std::max(length, 0),
                     m_captionFormat.get(),
                     {left + icon + std::round(12 * scale), 0,
                      (float)client.right - std::round(160 * scale), height},
                     brush);
}

void Panel::OpenWindowed(float size, PCWSTR title) {
    if (!m_hwnd) {
        return;
    }
    SetWindowText(m_hwnd, title);
    m_windowSize = size;
    // Like Windows' Settings, a little wider for the island's preview and
    // taller so most tabs need no scrolling: about 1440 x 980, up to most of
    // the work area.
    if (!m_windowPlaced) {
        m_windowPlaced = true;
        const HMONITOR here = MonitorFromWindow(IslandWindow(m_owner),
                                                MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitor{sizeof(monitor)};
        GetMonitorInfo(here, &monitor);
        UINT dpiX = 96;
        UINT dpiY = 96;
        if (FAILED(GetDpiForMonitor(here, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
            dpiX = 96;
        }
        const float scale = dpiX / 96.0f * size;
        const RECT& work = monitor.rcWork;
        const int width = std::min((int)std::round(1440 * scale),
                                   (int)((work.right - work.left) * 0.92f));
        const int height = std::min((int)std::round(980 * scale),
                                    (int)((work.bottom - work.top) * 0.94f));
        const int left = work.left + (work.right - work.left - width) / 2;
        const int top = work.top + (work.bottom - work.top - height) / 2;
        // Onto that monitor first (made on the default one, it gets that
        // monitor's scale there, WM_DPICHANGED resizing it), then its size,
        // already in that scale: not scaled twice.
        SetWindowPos(m_hwnd, nullptr, left, top, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(m_hwnd, nullptr, left, top, width, height,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        m_windowDpi = GetDpiForWindow(m_hwnd);
    }
    // Its own monitor's scale (where it was put back after a restart too),
    // never one carried over.
    m_windowDpi = GetDpiForWindow(m_hwnd);
    m_scale = m_windowDpi / 96.0f * m_windowSize;
    MONITORINFO monitor{sizeof(monitor)};
    GetMonitorInfo(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST),
                   &monitor);
    m_monitorRect = monitor.rcMonitor;
    m_workRect = monitor.rcWork;
    m_scrollSpot = -1;
    Prepare(m_scale);
    OnOpen();
    m_target = m_open = m_openX = 1;
    m_contentsIn = 1;
    ShowWindow(m_hwnd, IsIconic(m_hwnd) ? SW_RESTORE : SW_SHOW);
    SetForegroundWindow(m_hwnd);
    Render();
    StartIslandAnimation();
}

void Panel::Destroy() {
    OnDestroy();
    m_captionLogo = nullptr;
    m_windowTarget = nullptr;
    m_liquid.Destroy();
    if (m_hwnd) {
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    for (HICON& icon : m_windowIcons) {
        if (icon) {
            DestroyIcon(icon);
            icon = nullptr;
        }
    }
    m_canvas.Destroy();
}

winrt::com_ptr<IDWriteTextFormat> Panel::MakeFormat(
    PCWSTR family,
    DWRITE_FONT_WEIGHT weight,
    float size,
    DWRITE_TEXT_ALIGNMENT alignment) {
    winrt::com_ptr<IDWriteTextFormat> format;
    if (SUCCEEDED(m_dwrite->CreateTextFormat(
            family, nullptr, weight, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, size, L"", format.put()))) {
        format->SetTextAlignment(alignment);
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        // Too long texts end with "...".
        const DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER,
                                       0, 0};
        winrt::com_ptr<IDWriteInlineObject> ellipsis;
        m_dwrite->CreateEllipsisTrimmingSign(format.get(), ellipsis.put());
        format->SetTrimming(&trimming, ellipsis.get());
    }
    return format;
}

void Panel::Open(POINT anchor,
                 float scale,
                 D2D1_RECT_F source,
                 bool fromPill,
                 bool behind) {
    if (!m_hwnd) {
        return;
    }
    m_scrollSpot = -1;
    m_source = source;
    m_fromPill = fromPill;
    // Out of the pill, it grows out of the pill's body from behind the
    // island (its window right under the island's), and closes back behind
    // it; stretched to the pill's width, if chosen.
    m_underIsland = behind || fromPill;
    m_minWidth = fromPill && g_settings.islandPanelsFollow
                     ? source.right - source.left
                     : 0;
    m_scale = scale;
    // Where it opens, first: its layout depends on the room there.
    m_anchor = anchor;
    MONITORINFO monitor{sizeof(monitor)};
    GetMonitorInfo(MonitorFromPoint(anchor, MONITOR_DEFAULTTONEAREST),
                   &monitor);
    m_monitorRect = monitor.rcMonitor;
    m_workRect = monitor.rcWork;
    // (Clicks go to it again; see Close.)
    SetWindowLongPtr(m_hwnd, GWL_EXSTYLE,
                     GetWindowLongPtr(m_hwnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
    Prepare(scale);
    OnOpen();

    // Closed (not still closing): starts from the island part, at its size.
    if (m_open <= 0.01 && m_openX <= 0.01) {
        m_open = m_openX = 0;
        m_velocity = m_velocityX = 0;
        m_shownCenter = CenterWanted();
        m_centerVelocity = 0;
        m_shownWidth = std::max((float)m_size.cx, m_minWidth);
        m_shownHeight = m_size.cy;
        m_widthVelocity = m_heightVelocity = 0;
        m_heightDelay = m_fromPill ? kPanelHeightDelay : 0;
    }

    m_target = 1;
    // Under the island: shown see-through until it's back under it (coming
    // to the front puts it over the island for a moment, which showed as the
    // pill blinking empty).
    m_placing = m_underIsland;
    Render();
    SetWindowPos(m_hwnd, ZOrderPlace(), 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(m_hwnd);
    if (m_underIsland) {
        SetWindowPos(m_hwnd, ZOrderPlace(), 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        m_placing = false;
        Render();
    }
    StartIslandAnimation();
}

void Panel::MoveTo(POINT anchor, D2D1_RECT_F source) {
    if (!m_hwnd || m_target <= 0) {
        return;
    }
    m_anchor = anchor;
    m_source = source;
    if (m_fromPill && g_settings.islandPanelsFollow) {
        m_minWidth = source.right - source.left;
    }
    // (The springs take it there, see Render.)
    StartIslandAnimation();
}

void Panel::Rescale(float scale) {
    if (!m_hwnd || m_target <= 0 || scale == m_scale) {
        return;
    }
    m_scale = scale;
    // (The springs take it to its new size, see Render.)
    Prepare(scale);
    StartIslandAnimation();
}

void Panel::Close() {
    // (A window of its own: hidden at once, kept for the next time.)
    if (m_windowed) {
        if (m_target > 0) {
            m_target = m_open = m_openX = 0;
            OnClose();
            ShowWindow(m_hwnd, SW_HIDE);
        }
        if (GetCapture() == m_hwnd) {
            ReleaseCapture();
        }
        return;
    }
    if (m_target > 0) {
        m_target = 0;
        OnClose();
        // Back behind the island (clicks in it may have brought it over).
        if (m_underIsland) {
            Raise();
        }
        // Clicks go through it while it closes (to the island or what's
        // under it), never to what it showed.
        SetWindowLongPtr(m_hwnd, GWL_EXSTYLE,
                         GetWindowLongPtr(m_hwnd, GWL_EXSTYLE) |
                             WS_EX_TRANSPARENT);
        StartIslandAnimation();
    }
    if (GetCapture() == m_hwnd) {
        ReleaseCapture();
    }
}

// Frames of motion that never stops (see m_idleMotion): 30 a second.
constexpr double kIdleFrameSeconds = 1.0 / 30;

