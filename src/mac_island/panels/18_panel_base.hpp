void GlassLayer::Show(HWND owner,
                      POINT position,
                      SIZE size,
                      const std::vector<GlassShape>& shapes) {
    if (!owner || size.cx <= 0 || size.cy <= 0 || shapes.empty()) {
        Hide();
        return;
    }
    try {
        ShowBlurred(owner, position, size, shapes);
    } catch (winrt::hresult_error const& e) {
        IslandLog(L"Liquid Glass: Windows' blur failed: %08X", (unsigned)e.code());
        g_glassCompositionFailed = true;
        Hide();
        DestroyWindowParts();
    }
}

void GlassLayer::Place(HWND owner, POINT position, SIZE size, bool moved) {
    if (moved || !IsWindowVisible(m_hwnd) ||
        GetWindow(owner, GW_HWNDNEXT) != m_hwnd) {
        SetWindowPos(m_hwnd, owner, position.x, position.y, size.cx, size.cy,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

// Windows' blur: the blur of what's behind (Windows' own, the one of its
// acrylic), cut to each shape, under the tint, with the light over its upper
// half and the bright edge. Composition visuals, which Windows draws: nothing
// is drawn here again unless a shape changes, and screenshots and recordings
// see it.
void GlassLayer::ShowBlurred(HWND owner,
                             POINT position,
                             SIZE size,
                             const std::vector<GlassShape>& shapes) {
    GlassComposition* parts = GlassCompositionParts();
    if (!parts) {
        Hide();
        return;
    }
    if (m_hwnd && m_generation != g_glassGeneration) {
        DestroyWindowParts();
    }
    if (!m_hwnd && NowSeconds() - m_refusedAt < kGlassRetrySeconds) {
        Hide();
        return;
    }
    if (!m_hwnd) {
        // The mouse goes through it.
        m_hwnd = CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED |
                                    WS_EX_TRANSPARENT | WS_EX_TOPMOST |
                                    WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                kGlassClassName, L"", WS_POPUP, 0, 0, 0, 0,
                                nullptr, nullptr, g_module, nullptr);
        if (!m_hwnd) {
            Hide();
            return;
        }
        SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
        // (A window of an app that isn't from the Store can show Windows'
        // blur of what's behind it only with this.)
        const BOOL hostBackdrop = TRUE;
        const HRESULT hr =
            DwmSetWindowAttribute(m_hwnd, kDwmUseHostBackdropBrush,
                                  &hostBackdrop, sizeof(hostBackdrop));
        if (FAILED(hr)) {
            IslandLog(L"Liquid Glass: Windows refused its blur: %08X",
                   (unsigned)hr);
            m_refusedAt = NowSeconds();
            DestroyWindowParts();
            Hide();
            return;
        }
        m_generation = g_glassGeneration;
    }
    if (!m_desktopTarget) {
        auto interop = parts->compositor.as<ICompositorDesktopInteropLocal>();
        winrt::check_hresult(interop->CreateDesktopWindowTarget(
            m_hwnd, TRUE, winrt::put_abi(m_desktopTarget)));
        m_root = parts->compositor.CreateContainerVisual();
        m_desktopTarget.Root(m_root);
    }
    // (Made again when the light is turned on or off.)
    if (m_light != g_settings.islandGlassLight) {
        ResetLook();
        m_light = g_settings.islandGlassLight;
    }
    if (m_light) {
        if (const UINT dpi = GetDpiForWindow(owner)) {
            m_scale = dpi / 96.0f;
        }
    }
    m_owner = owner;
    const bool moved = !m_visible || position.x != m_position.x ||
                       position.y != m_position.y || size.cx != m_size.cx ||
                       size.cy != m_size.cy;
    m_position = position;
    m_size = size;
    while (m_blurred.size() > shapes.size()) {
        m_root.Children().Remove(m_blurred.back().container);
        m_blurred.pop_back();
    }
    m_blurred.resize(shapes.size());
    for (size_t i = 0; i < shapes.size(); i++) {
        UpdateBlurredShape(i, shapes[i]);
    }
    if (m_light) {
        UpdateShadow(shapes);
    }
    m_shapes = shapes;
    m_visible = true;
    m_drawn = true;
    Place(owner, position, size, moved);
}

winrt::Windows::UI::Composition::CompositionBrush GlassLayer::Backdrop(
    GlassComposition& parts) {
    namespace Composition = winrt::Windows::UI::Composition;
    const float blur = kGlassBlurMost * g_settings.islandGlassBlur / 100.0f;
    const bool translucent = g_settings.islandGlassTranslucent;
    const bool light = g_settings.islandGlassLight;
    if ((blur <= 0 && !translucent && !light) || m_effectFailed) {
        return parts.backdrop;
    }
    try {
        if (!m_effect || m_effectTranslucent != translucent ||
            m_effectLight != light) {
            // Blurred further (its amount changes on the brush), then, when
            // translucent or with the light, livelier: a saturation's matrix
            // (Rec. 709 luma).
            auto blurred = winrt::make_self<GlassEffect>();
            blurred->Name(L"Blur");
            blurred->input =
                Composition::CompositionEffectSourceParameter(L"backdrop");
            winrt::Windows::Graphics::Effects::IGraphicsEffect top =
                blurred
                    .as<winrt::Windows::Graphics::Effects::IGraphicsEffect>();
            if (translucent || light) {
                auto lively = winrt::make_self<GlassEffect>();
                lively->Name(L"Lively");
                lively->blurs = false;
                const float s = translucent ? kGlassTranslucentSaturation
                                            : kGlassLightSaturation;
                const float gain = translucent ? 1.0f : kGlassLightGain;
                const float luma[3] = {0.2126f * (1 - s), 0.7152f * (1 - s),
                                       0.0722f * (1 - s)};
                for (int row = 0; row < 3; row++) {
                    for (int column = 0; column < 3; column++) {
                        lively->matrix[row * 4 + column] =
                            (luma[row] + (row == column ? s : 0)) * gain;
                    }
                }
                if (!translucent) {
                    for (int column = 0; column < 3; column++) {
                        lively->matrix[16 + column] = kGlassLightBias;
                    }
                }
                lively->input = blurred.as<
                    winrt::Windows::Graphics::Effects::IGraphicsEffectSource>();
                top = lively.as<
                    winrt::Windows::Graphics::Effects::IGraphicsEffect>();
            }
            const auto factory = parts.compositor.CreateEffectFactory(
                top, winrt::single_threaded_vector<winrt::hstring>(
                         {L"Blur.BlurAmount"}));
            m_effect = factory.CreateBrush();
            m_effect.SetSourceParameter(L"backdrop", parts.backdrop);
            m_effectTranslucent = translucent;
            m_effectLight = light;
            m_effectBlur = -1;
        }
        if (blur != m_effectBlur) {
            m_effect.Properties().InsertScalar(L"Blur.BlurAmount", blur);
            m_effectBlur = blur;
        }
        return m_effect;
    } catch (winrt::hresult_error const& e) {
        IslandLog(L"Liquid Glass: the blur's effect failed: %08X",
               (unsigned)e.code());
        m_effect = nullptr;
        m_effectFailed = true;
        return parts.backdrop;
    }
}

void GlassLayer::UpdateBlurredShape(size_t index, const GlassShape& shape) {
    namespace Composition = winrt::Windows::UI::Composition;
    GlassComposition& parts = *g_glassComposition;
    const Composition::Compositor& compositor = parts.compositor;
    BlurredShape& v = m_blurred[index];
    const winrt::Windows::Foundation::Numerics::float2 whole{(float)m_size.cx,
                                                             (float)m_size.cy};
    auto color = [](const D2D1_COLOR_F& c) {
        return winrt::Windows::UI::Color{
            (uint8_t)std::lround(std::clamp(c.a, 0.0f, 1.0f) * 255),
            (uint8_t)std::lround(std::clamp(c.r, 0.0f, 1.0f) * 255),
            (uint8_t)std::lround(std::clamp(c.g, 0.0f, 1.0f) * 255),
            (uint8_t)std::lround(std::clamp(c.b, 0.0f, 1.0f) * 255)};
    };
    if (!v.container) {
        v.container = compositor.CreateContainerVisual();
        v.backdrop = compositor.CreateSpriteVisual();
        v.tintBrush = compositor.CreateColorBrush();
        v.tint = compositor.CreateSpriteVisual();
        v.tint.Brush(v.tintBrush);
        v.sheenBrush = compositor.CreateLinearGradientBrush();
        v.sheenBrush.MappingMode(Composition::CompositionMappingMode::Absolute);
        v.sheenBrush.ColorStops().Append(compositor.CreateColorGradientStop(
            0, winrt::Windows::UI::Color{(uint8_t)(m_light ? 0x2E : 0x1A), 255,
                                         255, 255}));
        if (m_light) {
            v.sheenBrush.ColorStops().Append(compositor.CreateColorGradientStop(
                0.45f, winrt::Windows::UI::Color{0x0D, 255, 255, 255}));
        }
        v.sheenBrush.ColorStops().Append(compositor.CreateColorGradientStop(
            1, winrt::Windows::UI::Color{0, 255, 255, 255}));
        v.sheen = compositor.CreateSpriteVisual();
        v.sheen.Brush(v.sheenBrush);
        // The bright edge: strongest at the top, faint on the sides, a bit
        // again at the bottom (with the light, from the top left, as a
        // bevel).
        v.rimBrush = compositor.CreateLinearGradientBrush();
        v.rimBrush.MappingMode(Composition::CompositionMappingMode::Absolute);
        if (m_light) {
            for (const auto& [offset, alpha] :
                 {std::pair{0.0f, 0xCC}, std::pair{0.28f, 0x33},
                  std::pair{0.55f, 0x1A}, std::pair{0.82f, 0x38},
                  std::pair{1.0f, 0x80}}) {
                v.rimBrush.ColorStops().Append(
                    compositor.CreateColorGradientStop(
                        offset, winrt::Windows::UI::Color{(uint8_t)alpha, 255,
                                                          255, 255}));
            }
        } else {
            for (const auto& [offset, alpha] :
                 {std::pair{0.0f, 0x80}, std::pair{0.35f, 0x1F},
                  std::pair{0.7f, 0x0F}, std::pair{1.0f, 0x48}}) {
                v.rimBrush.ColorStops().Append(
                    compositor.CreateColorGradientStop(
                        offset, winrt::Windows::UI::Color{(uint8_t)alpha, 255,
                                                          255, 255}));
            }
        }
        v.rimShape = compositor.CreateSpriteShape();
        v.rimShape.StrokeBrush(v.rimBrush);
        v.rimShape.StrokeThickness(1);
        v.rim = compositor.CreateShapeVisual();
        v.rim.Shapes().Append(v.rimShape);
        std::vector<Composition::Visual> visuals{
            v.backdrop.as<Composition::Visual>(),
            v.tint.as<Composition::Visual>(),
            v.sheen.as<Composition::Visual>()};
        if (m_light) {
            // Light bouncing back along the bottom, a little cool.
            v.bounceBrush = compositor.CreateLinearGradientBrush();
            v.bounceBrush.MappingMode(
                Composition::CompositionMappingMode::Absolute);
            v.bounceBrush.ColorStops().Append(
                compositor.CreateColorGradientStop(
                    0, winrt::Windows::UI::Color{0, 217, 224, 255}));
            v.bounceBrush.ColorStops().Append(
                compositor.CreateColorGradientStop(
                    1, winrt::Windows::UI::Color{0x17, 217, 224, 255}));
            v.bounce = compositor.CreateSpriteVisual();
            v.bounce.Brush(v.bounceBrush);
            // The glass's thickness: a faint bright band inside its edge.
            v.bandShape = compositor.CreateSpriteShape();
            v.bandShape.StrokeBrush(compositor.CreateColorBrush(
                winrt::Windows::UI::Color{0x17, 255, 255, 255}));
            v.band = compositor.CreateShapeVisual();
            v.band.Shapes().Append(v.bandShape);
            // The soft edge of small glass (the island's, a chip's): light
            // at the top and bottom, darker in between.
            struct SoftStop {
                float offset;
                int alpha;
                int gray;
            };
            static constexpr SoftStop kSoft[] = {
                {0.0f, 0x70, 212}, {0.1f, 0x4F, 64},  {0.25f, 0x61, 64},
                {0.5f, 0x70, 33},  {0.75f, 0x8F, 64}, {0.9f, 0x8F, 64},
                {1.0f, 0x70, 194}};
            v.softRimBrush = compositor.CreateLinearGradientBrush();
            v.softRimBrush.MappingMode(
                Composition::CompositionMappingMode::Absolute);
            for (const SoftStop& stop : kSoft) {
                v.softRimBrush.ColorStops().Append(
                    compositor.CreateColorGradientStop(
                        stop.offset,
                        winrt::Windows::UI::Color{
                            (uint8_t)stop.alpha, (uint8_t)stop.gray,
                            (uint8_t)stop.gray, (uint8_t)stop.gray}));
            }
            visuals.push_back(v.bounce.as<Composition::Visual>());
            visuals.push_back(v.band.as<Composition::Visual>());
        }
        visuals.push_back(v.rim.as<Composition::Visual>());
        for (const Composition::Visual& visual : visuals) {
            v.container.Children().InsertAtTop(visual);
        }
        m_root.Children().InsertAtTop(v.container);
    }
    for (const Composition::SpriteVisual& sprite :
         {v.backdrop, v.tint, v.sheen}) {
        sprite.Size(whole);
    }
    v.rim.Size(whole);
    if (m_light) {
        v.bounce.Size(whole);
        v.band.Size(whole);
    }
    v.container.Opacity(std::clamp(shape.opacity, 0.0f, 1.0f));
    v.rim.Opacity(EdgeAmount());
    if (const Composition::CompositionBrush backdrop = Backdrop(parts);
        backdrop != v.backdropBrush) {
        v.backdrop.Brush(backdrop);
        v.backdropBrush = backdrop;
    }
    v.tintBrush.Color(color(shape.tint));
    if (v.hasShape && SameGlassShape(shape, v.made)) {
        return;
    }
    // Its place changed: cut again.
    const D2D1_RECT_F& rect = shape.rect;
    Composition::CompositionGeometry fill{nullptr};
    Composition::CompositionGeometry edge{nullptr};
    if (shape.shoulder >= 0) {
        const auto body =
            NotchGeometry(parts.factory.get(), rect, shape.radius,
                          shape.shoulder, shape.edgeLeft, shape.edgeRight, true);
        const auto outline = NotchGeometry(parts.factory.get(), rect,
                                           shape.radius, shape.shoulder,
                                           shape.edgeLeft, shape.edgeRight,
                                           false);
        if (!body || !outline) {
            return;
        }
        fill = compositor.CreatePathGeometry(
            Composition::CompositionPath(winrt::make<GlassPath>(body.get())));
        edge = compositor.CreatePathGeometry(
            Composition::CompositionPath(winrt::make<GlassPath>(outline.get())));
    } else {
        auto rounded = compositor.CreateRoundedRectangleGeometry();
        rounded.Offset({rect.left, rect.top});
        rounded.Size({rect.right - rect.left, rect.bottom - rect.top});
        rounded.CornerRadius({shape.radius, shape.radius});
        fill = rounded;
        const float inset = std::max(0.0f, shape.radius - 0.5f);
        auto line = compositor.CreateRoundedRectangleGeometry();
        line.Offset({rect.left + 0.5f, rect.top + 0.5f});
        line.Size({std::max(0.0f, rect.right - rect.left - 1),
                   std::max(0.0f, rect.bottom - rect.top - 1)});
        line.CornerRadius({inset, inset});
        edge = line;
    }
    for (const Composition::SpriteVisual& sprite :
         {v.backdrop, v.tint, v.sheen}) {
        sprite.Clip(compositor.CreateGeometricClip(fill));
    }
    v.rimShape.Geometry(edge);
    if (m_light) {
        const float s = m_scale;
        const float width = rect.right - rect.left;
        const float height = rect.bottom - rect.top;
        v.bounce.Clip(compositor.CreateGeometricClip(fill));
        // The sheen over the upper part, the bounce along the bottom.
        v.sheenBrush.StartPoint({0, rect.top});
        v.sheenBrush.EndPoint(
            {0, rect.top + std::max(1.0f, std::min(height * 0.42f, 34 * s))});
        const float bounce = std::max(1.0f, std::min(height * 0.30f, 22 * s));
        v.bounceBrush.StartPoint({0, rect.bottom - bounce});
        v.bounceBrush.EndPoint({0, rect.bottom});
        // The bevel, from the top left corner.
        const float side = std::max(1.0f, std::min(width, height));
        v.rimBrush.StartPoint({rect.left, rect.top});
        v.rimBrush.EndPoint({rect.left + side * 1.5f, rect.top + side});
        // The light in scale with the glass: small glass (the island, a
        // chip) gets the soft edge, less sheen and no bounce or band; the
        // full optics from the size of a panel up.
        const float minSide = std::min(width, height);
        const float big =
            std::clamp((minSide - 40 * s) / (160 * s), 0.0f, 1.0f);
        const bool small = minSide < 120 * s;
        v.softRimBrush.StartPoint({0, rect.top});
        v.softRimBrush.EndPoint({0, rect.bottom});
        v.rimShape.StrokeBrush(small ? v.softRimBrush : v.rimBrush);
        v.rimShape.StrokeThickness(std::max(1.0f, (small ? 1.0f : 1.1f) * s));
        v.sheen.Opacity(0.35f + 0.65f * big);
        v.bounce.Opacity(big);
        // The thickness band, 3.5px wide, 1.2px inside the edge (round shapes
        // big enough; a notch has none).
        v.band.Opacity(0);
        if (shape.shoulder < 0 && big > 0.3f) {
            const float bandWidth = 3.5f * s;
            const float in = 1.2f * s + bandWidth / 2;
            if (width > in * 2 && height > in * 2) {
                auto line = compositor.CreateRoundedRectangleGeometry();
                line.Offset({rect.left + in, rect.top + in});
                line.Size({width - in * 2, height - in * 2});
                const float r = std::max(0.0f, shape.radius - in);
                line.CornerRadius({r, r});
                v.bandShape.Geometry(line);
                v.bandShape.StrokeThickness(bandWidth);
                v.band.Opacity(1);
            }
        }
    } else {
        v.sheenBrush.StartPoint({0, rect.top});
        v.sheenBrush.EndPoint({0, (rect.top + rect.bottom) / 2});
        v.rimBrush.StartPoint({0, rect.top});
        v.rimBrush.EndPoint({0, rect.bottom});
    }
    v.made = shape;
    v.hasShape = true;
}

// The shadow, as Siri Plus Launchpad's: layers of each shape growing out and
// a little down, each fainter than the one under it, so the glass sits over
// the wallpaper. They're under every shape's glass (which Windows' blur
// fills, so they show only outside it), and in scale: the smaller the glass,
// the closer and lighter its shadow. Rounded rectangles that are only moved
// (no geometry is made again), and not even that for a change smaller than a
// pixel (the shapes' colors changing, a fade): the shadow doesn't follow a
// pill every frame of its animation with new work.
void GlassLayer::UpdateShadow(const std::vector<GlassShape>& shapes) {
    namespace Composition = winrt::Windows::UI::Composition;
    const Composition::Compositor& compositor = g_glassComposition->compositor;
    if (!m_shadow) {
        m_shadow = compositor.CreateShapeVisual();
        m_root.Children().InsertAtBottom(m_shadow);
    }
    m_shadow.Size({(float)m_size.cx, (float)m_size.cy});
    const size_t count = shapes.size() * kGlassShadowLayers;
    if (m_shadowLayers.size() != count) {
        m_shadow.Shapes().Clear();
        m_shadowLayers.clear();
        m_shadowShapes.assign(shapes.size(), GlassShape{});
        for (size_t i = 0; i < count; i++) {
            ShadowLayer layer;
            layer.geometry = compositor.CreateRoundedRectangleGeometry();
            layer.brush = compositor.CreateColorBrush();
            const auto sprite = compositor.CreateSpriteShape(layer.geometry);
            sprite.FillBrush(layer.brush);
            m_shadow.Shapes().Append(sprite);
            m_shadowLayers.push_back(std::move(layer));
        }
    }
    auto within = [](float a, float b) { return std::fabs(a - b) < 1; };
    for (size_t i = 0; i < shapes.size(); i++) {
        const GlassShape& shape = shapes[i];
        const GlassShape& was = m_shadowShapes[i];
        if (within(shape.rect.left, was.rect.left) &&
            within(shape.rect.top, was.rect.top) &&
            within(shape.rect.right, was.rect.right) &&
            within(shape.rect.bottom, was.rect.bottom) &&
            within(shape.radius, was.radius) &&
            std::fabs(shape.opacity - was.opacity) < 0.04f &&
            (shape.opacity > 0.01f) == (was.opacity > 0.01f)) {
            continue;
        }
        m_shadowShapes[i] = shape;
        const D2D1_RECT_F& rect = shape.rect;
        const float width = rect.right - rect.left;
        const float height = rect.bottom - rect.top;
        const float s = m_scale;
        // Full from about a panel's size, about half on the island.
        const float size =
            std::clamp(std::min(width, height) / (140 * s), 0.0f, 1.0f);
        const float spread = (4 + (kGlassShadowSpread - 4) * size) * s;
        const float drop = (1.5f + (kGlassShadowDrop - 1.5f) * size) * s;
        const float strength = kGlassShadowStrength * (0.35f + 0.65f * size) *
                               std::clamp(shape.opacity, 0.0f, 1.0f);
        for (int layer = 1; layer <= kGlassShadowLayers; layer++) {
            ShadowLayer& piece =
                m_shadowLayers[i * kGlassShadowLayers + layer - 1];
            const float grown = spread * layer / kGlassShadowLayers;
            piece.geometry.Offset({rect.left - grown, rect.top - grown + drop});
            piece.geometry.Size({width + grown * 2, height + grown * 2});
            piece.geometry.CornerRadius(
                {shape.radius + grown, shape.radius + grown});
            // (Each layer lighter further out, to a soft falloff with no
            // visible edge; the weights add up to 1.)
            const float weight =
                2.0f * (kGlassShadowLayers + 1 - layer) /
                (kGlassShadowLayers * (kGlassShadowLayers + 1));
            piece.brush.Color(winrt::Windows::UI::Color{
                (uint8_t)std::lround(255 * strength * weight), 0, 0, 0});
        }
    }
}

// Everything made for the shapes goes, to be made again with or without the
// light.
void GlassLayer::ResetLook() {
    if (m_root) {
        m_root.Children().RemoveAll();
    }
    m_blurred.clear();
    m_shadow = nullptr;
    m_shadowLayers.clear();
    m_shadowShapes.clear();
}

void GlassLayer::DestroyWindowParts() {
    m_blurred.clear();
    m_shadow = nullptr;
    m_shadowLayers.clear();
    m_shadowShapes.clear();
    m_effect = nullptr;
    m_effectFailed = false;
    m_root = nullptr;
    m_desktopTarget = nullptr;
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void GlassLayer::FollowOwner() {
    if (m_visible && m_hwnd && m_owner &&
        GetWindow(m_owner, GW_HWNDNEXT) != m_hwnd) {
        SetWindowPos(m_hwnd, m_owner, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void GlassLayer::Hide() {
    if (m_hwnd && IsWindowVisible(m_hwnd)) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
    m_owner = nullptr;
    m_visible = false;
    m_drawn = false;
    m_shapes.clear();
}

void GlassLayer::Destroy() {
    Hide();
    DestroyWindowParts();
}

class Panel;
// An island's window (none without one).
HWND IslandWindow(const Island* island);

// A list taller than the room it has in a panel, scrolled with the mouse
// wheel: it glides to where the wheel sends it (a spring, so a notch never
// jumps), and a thin bar shows while it moves (see Panel::DrawScrollBar).
struct SmoothScroll {
    double offset = 0;
    double velocity = 0;
    double target = 0;
    // The room shown, and the list's whole height.
    float viewport = 0;
    float content = 0;
    // The bar's opacity (0 to 1), and when the wheel last turned.
    double bar = 0;
    double barVelocity = 0;
    double lastWheel = -10;

    float Max() const { return std::max(0.0f, content - viewport); }
    float Offset() const { return (float)offset; }
    void Reset() {
        offset = velocity = target = 0;
        bar = barVelocity = 0;
        lastWheel = -10;
    }
    // A new layout: the list may be shorter now.
    void SetSizes(float viewportHeight, float contentHeight) {
        viewport = viewportHeight;
        content = contentHeight;
        target = std::clamp(target, 0.0, (double)Max());
    }
    // Returns true if it's going somewhere else.
    bool Wheel(int delta, float step) {
        if (Max() <= 0) {
            return false;
        }
        lastWheel = NowSeconds();
        StartIslandAnimation();
        const double to = std::clamp(
            target - (double)delta / WHEEL_DELTA * step, 0.0, (double)Max());
        if (to == target) {
            return false;
        }
        target = to;
        return true;
    }
    // Brings a part of the list (from `top` to `bottom`) into view.
    void ShowRange(float top, float bottom) {
        if (top < target) {
            target = top;
        } else if (bottom > target + viewport) {
            target = bottom - viewport;
        }
        target = std::clamp(target, 0.0, (double)Max());
        StartIslandAnimation();
    }
    // Returns true while it moves (or its bar is still to fade away).
    bool Animate(double dt) {
        target = std::clamp(target, 0.0, (double)Max());
        const bool moving = SpringTowards(offset, velocity, target, 0.32, dt);
        const bool recent = NowSeconds() - lastWheel < 0.8;
        const bool fading = SpringTowards(
            bar, barVelocity, (moving || recent) && Max() > 0 ? 1 : 0, 0.3, dt);
        return moving || fading || recent;
    }
};

// Things under the mouse light up smoothly (and back): each is known by a
// key (or its place), with how lit it is (0 to 1), following the mouse with a
// spring. Asked while drawing; the animation moves them.
struct HoverFades {
    struct Entry {
        unsigned long long key;
        double amount;
        double velocity;
        bool on;
        int unseen;
    };
    std::vector<Entry> entries;

    // (`start`: where a new one starts, like a switch shown as it is.)
    float Get(unsigned long long key, bool on, double start = 0) {
        for (auto& entry : entries) {
            if (entry.key == key) {
                entry.unseen = 0;
                if (entry.on != on) {
                    entry.on = on;
                    StartIslandAnimation();
                }
                return (float)std::clamp(entry.amount, 0.0, 1.0);
            }
        }
        entries.push_back({key, start, 0, on, 0});
        if (on != (start >= 1)) {
            StartIslandAnimation();
        }
        return (float)start;
    }
    // Known by its place (to the pixel).
    float Get(const D2D1_RECT_F& rect, bool on) {
        const auto round = [](float v) {
            return (unsigned long long)(long long)std::lround(v) & 0xFFFF;
        };
        return Get((round(rect.left) << 48) | (round(rect.top) << 32) |
                       (round(rect.right) << 16) | round(rect.bottom),
                   on);
    }
    // Returns true while one moves.
    bool Animate(double dt) {
        bool moving = false;
        for (auto& entry : entries) {
            // Not drawn for a long while (another view): it goes out.
            if (++entry.unseen > 600) {
                entry.on = false;
            }
            if (SpringDecoration(entry.amount, entry.velocity,
                                 entry.on ? 1 : 0, 0.2, dt)) {
                moving = true;
            }
        }
        std::erase_if(entries, [](const Entry& entry) {
            return !entry.on && entry.amount <= 0;
        });
        return moving;
    }
    // How lit each one is, for a window that draws again only when what it
    // shows changes.
    std::wstring Signature() const {
        std::wstring signature;
        for (const auto& entry : entries) {
            signature += std::to_wstring(entry.key % 100000) + L":" +
                         std::to_wstring((int)(entry.amount * 60)) + L",";
        }
        return signature;
    }
};

// A key for HoverFades from two numbers.
unsigned long long HoverKey(int a, long long b) {
    return ((unsigned long long)(unsigned)a << 40) ^ (unsigned long long)b;
}

////////////////////////////////////////////////////////////////////////////////
// Panels: windows that open out of the island and close when clicking anywhere
// else. Out of the pill, the pill itself stretches into the panel, like the
// iPhone's Dynamic Island: sideways first, then down, its contents shrinking
// away as the panel's fade in. On the bar, the panel drops out of the clicked
// item.

