#pragma once
// Returns true while it's still moving.
bool Panel::Animate(double dt) {
    if (!m_hwnd) {
        return false;
    }
    // A window of its own: only its contents move.
    if (m_windowed) {
        if (m_target <= 0) {
            return false;
        }
        const bool hoverFading = m_hoverFades.Animate(dt);
        const bool contents = AnimateContents(dt);
        const bool moving = hoverFading || contents || g_themeChanging;
        if (m_idleMotion && !hoverFading && !g_themeChanging) {
            const double now = NowSeconds();
            if (now - m_lastIdleFrame < kIdleFrameSeconds) {
                return true;
            }
            m_lastIdleFrame = now;
        }
        if (moving || m_moved || m_drawnRequest != g_frameRequests) {
            m_drawnRequest = g_frameRequests;
            Render();
        }
        m_moved = moving;
        return moving;
    }
    if (m_open <= 0 && m_openX <= 0 && m_target <= 0) {
        // (Opened and closed again before a frame: its window, shown with
        // its first frame, goes too.)
        if (IsWindowVisible(m_hwnd)) {
            ShowWindow(m_hwnd, SW_HIDE);
        }
        m_liquid.Hide();
        return false;
    }
    bool moving;
    if (m_target > 0) {
        const bool sideways =
            SpringTowards(m_openX, m_velocityX, 1, kPanelWidthSeconds * 0.9,
                          dt, kPanelOpenDamping);
        bool downwards = true;
        // (Without animations, down in the same frame as sideways: waiting
        // showed it as the pill stretched to the sides for a few frames.)
        if (m_heightDelay > 0 && AnimationLevel() < 2) {
            m_heightDelay -= dt;
        } else {
            m_heightDelay = 0;
            downwards = SpringTowards(m_open, m_velocity, 1,
                                      kPanelHeightSeconds * 0.9, dt,
                                      kPanelOpenDamping);
        }
        const bool hoverFading = m_hoverFades.Animate(dt);
        const bool pulsing = SpringDecoration(
            m_contentsIn, m_contentsInVelocity, 1, 0.4, dt);
        const bool contents = AnimateContents(dt) || hoverFading ||
                              pulsing || g_themeChanging;
        // Another view, another size: it settles there without swinging.
        const bool width = SpringTowards(
            m_shownWidth, m_widthVelocity,
            std::max((float)m_size.cx, m_minWidth), kOpenSeconds, dt);
        const bool height = SpringTowards(m_shownHeight, m_heightVelocity,
                                          m_size.cy, kOpenSeconds, dt);
        const bool center = SpringTowards(m_shownCenter, m_centerVelocity,
                                          CenterWanted(), kOpenSeconds, dt);
        moving = sideways || downwards || width || height || center || contents;
        if (m_idleMotion && !sideways && !downwards && !width && !height &&
            !center && !g_themeChanging) {
            const double now = NowSeconds();
            if (now - m_lastIdleFrame < kIdleFrameSeconds) {
                return true;
            }
            m_lastIdleFrame = now;
        }
    } else {
        m_idleMotion = false;
        m_heightDelay = 0;
        // Up first, then in from the sides once its contents have faded
        // (see Render): one wider than the pill keeps its width while they
        // show, rather than its sides being cut off.
        const bool downwards = SpringTowards(m_open, m_velocity, 0,
                                             kPanelCloseSeconds, dt);
        bool sideways = false;
        if (m_open <= kPanelContentsFrom) {
            sideways = SpringTowards(m_openX, m_velocityX, 0,
                                     kPanelCloseSeconds, dt);
        } else {
            m_velocityX = 0;
        }
        moving = sideways || downwards;
    }
    if (!moving && m_open <= 0 && m_openX <= 0) {
        ShowWindow(m_hwnd, SW_HIDE);
        m_liquid.Hide();
        if (m_fromPill) {
            StartPanelAnimation(m_owner);
        }
        return false;
    }
    // Drawn while something moves, once more as it stops, and after frames
    // were asked for (something changed): not on every frame of the island's
    // own animations (a count, a graph), which would draw an open panel
    // again and again with nothing new.
    if (moving || m_moved || m_drawnRequest != g_frameRequests) {
        m_drawnRequest = g_frameRequests;
        Render();
    }
    m_moved = moving;
    return moving;
}

// Drawn once while closed, off any window: its first opening doesn't wait
// for its resources (fonts, layouts) to be made.
void Panel::WarmUp(float scale) {
    if (!m_hwnd || m_open > 0 || m_target > 0) {
        return;
    }
    m_scale = scale;
    Prepare(scale);
    if (!m_canvas.Begin(m_size)) {
        return;
    }
    auto* target = m_canvas.target.get();
    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(target->CreateSolidColorBrush(Bg(1), brush.put()))) {
        Draw(target, brush.get());
    }
    // (Never shown: the window stays as it is.)
    m_canvas.Finish();
}

// Where its middle goes: over its anchor.
float Panel::CenterWanted() const {
    // Out of the pill: under the item it opened from, but not past the
    // pill's ends (in its middle once it's wider than the pill).
    if (m_fromPill) {
        const float half = std::max((float)m_size.cx, m_minWidth) / 2.0f;
        const float from = m_source.left + half;
        const float to = m_source.right - half;
        return from <= to ? std::clamp((float)m_anchor.x, from, to)
                          : (m_source.left + m_source.right) / 2;
    }
    return (float)m_anchor.x;
}

// The island part stretches into the panel: a black shape going from the
// island part's rect (from behind the pill, out of it) to the panel's,
// sideways and downwards on their own springs, then the panel's contents
// fade in, sliding down a little. Closing goes back the same way.
void Panel::Render() {
    if (!m_hwnd) {
        return;
    }
    if (m_windowed) {
        RenderWindowed();
        return;
    }
    // Another view, another size (or place): it stretches to it.
    if (m_target > 0 &&
        (std::fabs(m_shownWidth - std::max((float)m_size.cx, m_minWidth)) >
             0.5 ||
         std::fabs(m_shownHeight - m_size.cy) > 0.5 ||
         std::fabs(m_shownCenter - CenterWanted()) > 0.5)) {
        StartIslandAnimation();
    }
    const float width = (float)std::max(m_shownWidth, 1.0);
    const float height = (float)std::max(m_shownHeight, 1.0);
    // Centered under (or over) the anchor, or beside it and centered on it,
    // inside the monitor.
    const bool sideways =
        m_direction == Direction::Right || m_direction == Direction::Left;
    const float left =
        sideways ? (m_direction == Direction::Right ? (float)m_anchor.x
                                                    : (float)m_anchor.x - width)
                 : std::clamp((float)m_shownCenter - width / 2,
                              (float)m_monitorRect.left + 8,
                              std::max((float)m_monitorRect.left + 8,
                                       (float)m_monitorRect.right - width - 8));
    const float panelTop =
        sideways ? std::clamp((float)m_anchor.y - height / 2,
                              (float)m_monitorRect.top + 8,
                              std::max((float)m_monitorRect.top + 8,
                                       (float)m_monitorRect.bottom - height - 8))
        : m_direction == Direction::Up ? (float)m_anchor.y - height
                                       : (float)m_anchor.y;
    const D2D1_RECT_F panel{std::round(left), panelTop,
                            std::round(left) + width, panelTop + height};
    m_position = {(LONG)panel.left, (LONG)panel.top};

    // The window covers the island part and the panel, and the bounce.
    const float margin = std::round(12 * m_scale);
    const float sideMargin =
        margin + 0.08f * std::max(std::fabs(panel.left - m_source.left),
                                  std::fabs(panel.right - m_source.right));
    const float bottomMargin =
        margin + 0.08f * std::fabs(panel.bottom - m_source.bottom);
    const float topMargin =
        m_direction != Direction::Down
            ? margin + 0.08f * std::fabs(panel.top - m_source.top)
            : 2;
    POINT windowPosition{
        (LONG)std::floor(std::min(panel.left, m_source.left) - sideMargin),
        (LONG)std::floor(std::min(panel.top, m_source.top) - topMargin)};
    SIZE windowSize{
        (LONG)std::ceil(std::max(panel.right, m_source.right) + sideMargin -
                        windowPosition.x),
        (LONG)std::ceil(std::max(panel.bottom, m_source.bottom) +
                        bottomMargin - windowPosition.y)};
    // Inside its monitor (the bounce past an edge isn't seen there anyway),
    // not over the next one; and never as tall or as wide as it: an app in
    // front from edge to edge is a full-screen one to Wallpaper Engine and
    // the like, which stop for it (measured: a tall list panel did).
    if (m_monitorRect.right > m_monitorRect.left) {
        const LONG right = std::min(windowPosition.x + windowSize.cx,
                                    m_monitorRect.right);
        const LONG bottom = std::min(windowPosition.y + windowSize.cy,
                                     m_monitorRect.bottom);
        windowPosition.x = std::max(windowPosition.x, m_monitorRect.left);
        windowPosition.y = std::max(windowPosition.y, m_monitorRect.top);
        windowSize.cx = std::min(right - windowPosition.x,
                                 m_monitorRect.right - m_monitorRect.left - 1);
        windowSize.cy = std::min(bottom - windowPosition.y,
                                 m_monitorRect.bottom - m_monitorRect.top - 1);
    }
    if (!m_canvas.Begin(windowSize)) {
        return;
    }
    auto* target = m_canvas.target.get();

    auto local = [&](const D2D1_RECT_F& r) {
        return D2D1_RECT_F{r.left - windowPosition.x, r.top - windowPosition.y,
                           r.right - windowPosition.x,
                           r.bottom - windowPosition.y};
    };
    const D2D1_RECT_F from = local(m_source);
    const D2D1_RECT_F to = local(panel);

    const float tx = (float)std::clamp(m_openX, -0.05, 1.08);
    const float ty = (float)std::clamp(m_open, -0.05, 1.08);
    auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    const D2D1_RECT_F shape{lerp(from.left, to.left, tx),
                            lerp(from.top, to.top, ty),
                            lerp(from.right, to.right, tx),
                            lerp(from.bottom, to.bottom, ty)};
    // How far down it has stretched, which leads the contents.
    const float progress = std::clamp(ty, 0.0f, 1.0f);
    const float fromRadius = (from.bottom - from.top) / 2 * PanelCornerShare();
    const float toRadius =
        (m_fromPill ? kIslandPanelRadius : kPanelRadius) * m_scale *
        PanelCornerShare();
    const float radius = std::clamp(
        fromRadius + (toRadius - fromRadius) * progress, 0.0f,
        std::max(0.0f, std::min(shape.right - shape.left,
                                shape.bottom - shape.top) /
                           2));

    winrt::com_ptr<ID2D1Factory> factory;
    target->GetFactory(factory.put());
    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    // Over the Liquid Glass (once it shows), only taking the mouse: the
    // glass has the color and the bright edge.
    const bool liquid = LiquidGlassOn();
    const bool overLiquid = liquid && m_liquid.Shown();
    target->CreateSolidColorBrush(
        overLiquid ? D2D1::ColorF(0, 0, 0, 1.0f / 255) : Bg(1), brush.put());
    target->FillRoundedRectangle({shape, radius, radius}, brush.get());
    if (!overLiquid) {
        // Its edge (half a pixel in).
        brush->SetColor(
            Fg(0.10f * EdgeAmount() *
               (m_fromPill ? 1.0f : std::clamp(progress * 2, 0.0f, 1.0f))));
        const float edgeRadius = std::max(0.0f, radius - 0.5f);
        target->DrawRoundedRectangle(
            {{shape.left + 0.5f, shape.top + 0.5f, shape.right - 0.5f,
              shape.bottom - 0.5f},
             edgeRadius,
             edgeRadius},
            brush.get(), 1);
    }

    // Clipped to the rounded shape.
    winrt::com_ptr<ID2D1RoundedRectangleGeometry> clip;
    factory->CreateRoundedRectangleGeometry({shape, radius, radius},
                                            clip.put());

    // The panel's contents (laid out at m_size), centered in the shape as
    // it changes size, fading in and sliding down into place.
    const float contents =
        std::clamp((progress - kPanelContentsFrom) / (1 - kPanelContentsFrom),
                   0.0f, 1.0f) *
        (0.25f + 0.75f * (float)std::clamp(m_contentsIn, 0.0, 1.0));
    const float contentLeft =
        std::round((to.left + to.right) / 2 - m_size.cx / 2.0f);
    const float contentTop = to.top;
    m_contentOffset = {(LONG)contentLeft, (LONG)contentTop};
    // (Found again while drawing, see ScrollsHere.)
    m_scrollSpots.clear();
    if (contents > 0.01f && clip) {
        winrt::com_ptr<ID2D1Layer> layer;
        target->CreateLayer(nullptr, layer.put());
        target->PushLayer(
            D2D1::LayerParameters(D2D1::InfiniteRect(), clip.get(),
                                  D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                  D2D1::IdentityMatrix(), contents),
            layer.get());
        target->SetTransform(D2D1::Matrix3x2F::Translation(
            contentLeft,
            contentTop + (m_direction == Direction::Up     ? 1.0f
                          : m_direction == Direction::Down ? -1.0f
                                                           : 0.0f) *
                             (1 - progress) * 10 * m_scale));
        Draw(target, brush.get());
        target->SetTransform(D2D1::Matrix3x2F::Identity());
        target->PopLayer();
    }

    m_canvas.End(m_hwnd, windowPosition, m_placing ? 0.0f : 1.0f);
    if (liquid) {
        GlassShape glass;
        glass.rect = shape;
        glass.radius = radius;
        glass.tint = Bg(GlassTint(LiquidPanelTint()));
        glass.opacity = m_placing ? 0.0f : 1.0f;
        m_liquid.Show(m_hwnd, windowPosition, windowSize, {glass});
        // (Gone: drawn again with its own background, as the island.)
        if (overLiquid && !m_liquid.Shown()) {
            Render();
            return;
        }
    } else {
        m_liquid.Hide();
    }
}

// A window of its own: drawn at its size, over Mica (a veil of the island's
// color on it, so its texts read as on the island) or on the island's color,
// on the GPU (without the copy a layered window's frame takes, which made
// a big window's frames slow).
void Panel::RenderWindowed() {
    RECT client{};
    GetClientRect(m_hwnd, &client);
    if (client.right <= 0 || client.bottom <= 0 || IsIconic(m_hwnd)) {
        return;
    }
    const D2D1_SIZE_U size{(UINT32)client.right, (UINT32)client.bottom};
    if (!m_windowTarget) {
        // (Pixels, like the panels: 96 DPI. Not waiting for the screen: the
        // island's frames are paced already.)
        if (!m_factory ||
            FAILED(m_factory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(
                    D2D1_RENDER_TARGET_TYPE_DEFAULT,
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                      D2D1_ALPHA_MODE_PREMULTIPLIED),
                    96, 96),
                D2D1::HwndRenderTargetProperties(
                    m_hwnd, size, D2D1_PRESENT_OPTIONS_IMMEDIATELY),
                m_windowTarget.put()))) {
            return;
        }
        // (ClearType needs an opaque background.)
        m_windowTarget->SetTextAntialiasMode(
            D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        m_captionLogo = nullptr;
        OnWindowTargetMade();
    } else if (const D2D1_SIZE_U now = m_windowTarget->GetPixelSize();
               now.width != size.width || now.height != size.height) {
        m_windowTarget->Resize(size);
    }
    ID2D1HwndRenderTarget* target = m_windowTarget.get();
    target->BeginDraw();
    target->SetTransform(D2D1::Matrix3x2F::Identity());
    target->Clear(D2D1::ColorF(0, 0, 0, 0));
    // Its title bar (and Mica) dark or light like the island's colors.
    const D2D1_COLOR_F back = Bg(1);
    const int dark = back.r * 0.3f + back.g * 0.59f + back.b * 0.11f < 0.5f;
    if (dark != m_windowDark) {
        m_windowDark = dark;
        const BOOL on = dark;
        DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &on,
                              sizeof(on));
    }
    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(target->CreateSolidColorBrush(Bg(m_mica ? 0.55f : 1),
                                                brush.put()))) {
        target->FillRectangle(
            {0, 0, (float)client.right, (float)client.bottom}, brush.get());
        m_contentOffset = ContentOffset();
        // (Found again while drawing, see ScrollsHere.)
        m_scrollSpots.clear();
        target->SetTransform(D2D1::Matrix3x2F::Translation(
            (float)m_contentOffset.x, (float)m_contentOffset.y));
        // (Clipped under the title bar: a list scrolled up goes under it.)
        target->PushAxisAlignedClip(
            {-(float)m_contentOffset.x, 0,
             (float)(client.right - m_contentOffset.x),
             (float)(client.bottom - m_contentOffset.y)},
            D2D1_ANTIALIAS_MODE_ALIASED);
        Draw(target, brush.get());
        target->PopAxisAlignedClip();
        target->SetTransform(D2D1::Matrix3x2F::Identity());
        DrawCaption(target, brush.get());
    }
    const HRESULT hr = target->EndDraw();
    if (hr == (HRESULT)D2DERR_RECREATE_TARGET) {
        // (Its pictures were made for this target: the island starts again,
        // as when its own drawing device is lost.)
        m_windowTarget = nullptr;
        OnDrawingDeviceLost();
    }
}

// The frame Windows keeps around a window (its resizing borders).
int WindowFrameThickness(HWND hWnd) {
    const UINT dpi = GetDpiForWindow(hWnd);
    return GetSystemMetricsForDpi(SM_CYFRAME, dpi) +
           GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

