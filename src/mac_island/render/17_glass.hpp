#pragma once
// A glass's shape, in its window's coordinates: a rounded rectangle, or a
// notch (see NotchGeometry) when `shoulder` isn't negative.
struct GlassShape {
    D2D1_RECT_F rect{};
    float radius = 0;
    float shoulder = -1;
    bool edgeLeft = false;
    bool edgeRight = false;
    // The color over the blur, and how much of it all shows (fading in and
    // out).
    D2D1_COLOR_F tint{};
    float opacity = 1;
};

bool SameGlassShape(const GlassShape& a, const GlassShape& b) {
    auto same = [](const D2D1_COLOR_F& x, const D2D1_COLOR_F& y) {
        return x.r == y.r && x.g == y.g && x.b == y.b && x.a == y.a;
    };
    return a.rect.left == b.rect.left && a.rect.top == b.rect.top &&
           a.rect.right == b.rect.right && a.rect.bottom == b.rect.bottom &&
           a.radius == b.radius && a.shoulder == b.shoulder &&
           a.edgeLeft == b.edgeLeft && a.edgeRight == b.edgeRight &&
           same(a.tint, b.tint) &&
           a.opacity == b.opacity;
}

constexpr WCHAR kGlassClassName[] = L"WindhawkMacIslandDockGlass";

// Windows.UI.Composition's interop for a desktop window and for a Direct2D
// shape, not in the compiler's headers.
struct ICompositorDesktopInteropLocal : ::IUnknown {
    virtual HRESULT __stdcall CreateDesktopWindowTarget(HWND window,
                                                        BOOL isTopmost,
                                                        void** target) = 0;
    virtual HRESULT __stdcall EnsureOnThread(DWORD threadId) = 0;
};
template <>
inline constexpr winrt::guid
    winrt::impl::guid_v<ICompositorDesktopInteropLocal>{
        0x29E691FA,
        0x4567,
        0x4DCA,
        {0xB3, 0x19, 0xD0, 0xF2, 0x07, 0xEB, 0x68, 0x07}};
struct IGeometrySource2DInteropLocal : ::IUnknown {
    virtual HRESULT __stdcall GetGeometry(ID2D1Geometry** value) = 0;
    virtual HRESULT __stdcall TryGetGeometryUsingFactory(
        ID2D1Factory* factory,
        ID2D1Geometry** value) = 0;
};
template <>
inline constexpr winrt::guid
    winrt::impl::guid_v<IGeometrySource2DInteropLocal>{
        0x0657AF73,
        0x53FD,
        0x47CF,
        {0x84, 0xFF, 0xC8, 0x49, 0x2D, 0x2A, 0x80, 0xA3}};
// DWMWA_USE_HOSTBACKDROPBRUSH.
constexpr DWORD kDwmUseHostBackdropBrush = 17;

// A notch's shape (see NotchGeometry) for a composition clip.
struct GlassPath
    : winrt::implements<GlassPath,
                        winrt::Windows::Graphics::IGeometrySource2D,
                        IGeometrySource2DInteropLocal> {
    explicit GlassPath(ID2D1Geometry* geometry) {
        m_geometry.copy_from(geometry);
    }
    HRESULT __stdcall GetGeometry(ID2D1Geometry** value) noexcept override {
        m_geometry.copy_to(value);
        return S_OK;
    }
    HRESULT __stdcall TryGetGeometryUsingFactory(
        ID2D1Factory*,
        ID2D1Geometry** value) noexcept override {
        *value = nullptr;
        return E_NOTIMPL;
    }

   private:
    winrt::com_ptr<ID2D1Geometry> m_geometry;
};

// Windows' energy saver: the glass gives way to the plain color while it's
// on (Windows itself turns its transparency effects off then).
bool g_energySaver = false;
// How much of the background's color is over the glass, of the island's and
// of its panels' (times the backgrounds' opacity slider): enough to read
// their texts over any window.
constexpr float kLiquidTint = 0.5f;
constexpr float kLiquidPanelTint = 0.62f;
// The same with the light and shadow of macOS 27 (islandGlassLight): the
// glass's own light does part of the work of the color, so less of it.
constexpr float kLiquidTintLight = 0.38f;
constexpr float kLiquidPanelTintLight = 0.46f;
// The translucent glass (see islandGlassTranslucent): that much of the tint,
// and its colors that much livelier (as a saturation).
constexpr float kGlassTranslucentTint = 0.5f;
constexpr float kGlassTranslucentSaturation = 1.5f;
// The glass's blur slider at 100%: this much more blur (a Gaussian blur's
// standard deviation, in pixels) over Windows' own.
constexpr float kGlassBlurMost = 20;
// Windows refused its blur to a glass's window: tried again this much later.
constexpr double kGlassRetrySeconds = 5;
// The light (islandGlassLight): what passes through the glass a touch
// livelier and brighter (a saturation, a gain and a bias, as Siri Plus
// Launchpad's).
constexpr float kGlassLightSaturation = 1.12f;
constexpr float kGlassLightGain = 1.03f;
constexpr float kGlassLightBias = 0.015f;
// Its shadow (see GlassLayer::UpdateShadow): that many layers, all together
// that black, spreading that far out (px at 100% scaling, so it stays inside
// the windows' margin) and dropping that far down.
constexpr int kGlassShadowLayers = 6;
constexpr float kGlassShadowStrength = 0.11f;
constexpr float kGlassShadowSpread = 9.5f;
constexpr float kGlassShadowDrop = 2.5f;

// The tint over the glass as chosen (lighter when translucent).
float GlassTint(float tint) {
    return g_settings.islandGlassTranslucent ? tint * kGlassTranslucentTint
                                             : tint;
}

// How much of the color is over the glass of the island and of its panels
// (see kLiquidTint).
float LiquidTint() {
    return g_settings.islandGlassLight ? kLiquidTintLight : kLiquidTint;
}
float LiquidPanelTint() {
    return g_settings.islandGlassLight ? kLiquidPanelTintLight
                                       : kLiquidPanelTint;
}

// Windows.UI.Composition's effects take Direct2D's (more blur, a color
// matrix) through this interop, not in the compiler's headers.
struct IGraphicsEffectD2D1InteropLocal : ::IUnknown {
    virtual HRESULT __stdcall GetEffectId(GUID* id) = 0;
    virtual HRESULT __stdcall GetNamedPropertyMapping(LPCWSTR name,
                                                      UINT* index,
                                                      int* mapping) = 0;
    virtual HRESULT __stdcall GetPropertyCount(UINT* count) = 0;
    virtual HRESULT __stdcall GetProperty(UINT index, void** value) = 0;
    virtual HRESULT __stdcall GetSource(UINT index, void** source) = 0;
    virtual HRESULT __stdcall GetSourceCount(UINT* count) = 0;
};
template <>
inline constexpr winrt::guid
    winrt::impl::guid_v<IGraphicsEffectD2D1InteropLocal>{
        0x2FC57384,
        0xA068,
        0x44D7,
        {0xA3, 0x31, 0x30, 0x98, 0x2F, 0xCF, 0x71, 0x77}};
// CLSID_D2D1GaussianBlur and CLSID_D2D1ColorMatrix (d2d1effects.h).
constexpr GUID kD2DGaussianBlur{
    0x1FEB6D69,
    0x2FE6,
    0x4AC9,
    {0x8C, 0x58, 0x1D, 0x7F, 0x93, 0xE7, 0xA6, 0xA5}};
constexpr GUID kD2DColorMatrix{
    0x921F03D6,
    0x641C,
    0x47DF,
    {0x85, 0x2D, 0xB4, 0xBB, 0x61, 0x53, 0xAE, 0x11}};

// One effect over the glass's blur (as the taskbar styler's WindhawkBlur,
// from TranslucentTB): more blur ("BlurAmount", which can change on the
// brush), or a color matrix (livelier colors). Its properties: the blur's
// standard deviation, balanced optimization and hard borders (no faded
// edge); the matrix, premultiplied alpha and no clamping.
struct GlassEffect
    : winrt::implements<
          GlassEffect,
          winrt::Windows::Graphics::Effects::IGraphicsEffect,
          winrt::Windows::Graphics::Effects::IGraphicsEffectSource,
          IGraphicsEffectD2D1InteropLocal> {
    bool blurs = true;
    float blur = 0;
    float matrix[20] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
                        1, 0, 0, 0, 0, 1, 0, 0, 0, 0};
    winrt::Windows::Graphics::Effects::IGraphicsEffectSource input{nullptr};

    winrt::hstring Name() { return m_name; }
    void Name(winrt::hstring const& name) { m_name = name; }

    HRESULT __stdcall GetEffectId(GUID* id) noexcept override {
        if (!id) {
            return E_INVALIDARG;
        }
        *id = blurs ? kD2DGaussianBlur : kD2DColorMatrix;
        return S_OK;
    }
    HRESULT __stdcall GetNamedPropertyMapping(LPCWSTR name,
                                              UINT* index,
                                              int* mapping) noexcept override {
        if (!name || !index || !mapping) {
            return E_INVALIDARG;
        }
        if (blurs && wcscmp(name, L"BlurAmount") == 0) {
            *index = 0;
            // GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT.
            *mapping = 1;
            return S_OK;
        }
        return E_INVALIDARG;
    }
    HRESULT __stdcall GetPropertyCount(UINT* count) noexcept override {
        if (!count) {
            return E_INVALIDARG;
        }
        *count = 3;
        return S_OK;
    }
    HRESULT __stdcall GetProperty(UINT index, void** value) noexcept override {
        if (!value) {
            return E_INVALIDARG;
        }
        try {
            namespace Foundation = winrt::Windows::Foundation;
            winrt::Windows::Foundation::IInspectable property{nullptr};
            if (index == 0) {
                property = blurs ? Foundation::PropertyValue::CreateSingle(blur)
                                 : Foundation::PropertyValue::CreateSingleArray(
                                       winrt::array_view<const float>(
                                           matrix, matrix + 20));
            } else if (index == 1) {
                // D2D1_GAUSSIANBLUR_OPTIMIZATION_BALANCED, or
                // D2D1_COLORMATRIX_ALPHA_MODE_PREMULTIPLIED.
                property = Foundation::PropertyValue::CreateUInt32(1);
            } else if (index == 2) {
                // D2D1_BORDER_MODE_HARD, or no clamping.
                property =
                    blurs ? Foundation::PropertyValue::CreateUInt32(1)
                          : Foundation::PropertyValue::CreateBoolean(false);
            } else {
                return E_BOUNDS;
            }
            *value = winrt::detach_abi(
                property.as<winrt::Windows::Foundation::IPropertyValue>());
            return S_OK;
        } catch (...) {
            return winrt::to_hresult();
        }
    }
    HRESULT __stdcall GetSource(UINT index, void** source) noexcept override {
        if (!source) {
            return E_INVALIDARG;
        }
        if (index != 0 || !input) {
            return E_BOUNDS;
        }
        *source = nullptr;
        winrt::copy_to_abi(input, *source);
        return S_OK;
    }
    HRESULT __stdcall GetSourceCount(UINT* count) noexcept override {
        if (!count) {
            return E_INVALIDARG;
        }
        *count = 1;
        return S_OK;
    }

   private:
    winrt::hstring m_name;
};

// Windows' transparency effects (defined with the island's colors): Windows'
// blur needs them.
extern bool g_transparencyEffects;

// Windows' blur for the glass, seen by screenshots and recordings (see
// GlassLayer::ShowBlurred): the island's thread's compositor, with the
// dispatcher queue it needs, the blur of what's behind (one brush for every
// glass), and a factory for the notches' shapes.
struct GlassComposition {
    winrt::Windows::System::DispatcherQueueController queue{nullptr};
    winrt::Windows::UI::Composition::Compositor compositor{nullptr};
    winrt::Windows::UI::Composition::CompositionBackdropBrush backdrop{nullptr};
    winrt::com_ptr<ID2D1Factory> factory;
};
[[clang::no_destroy]] std::optional<GlassComposition> g_glassComposition;
// It couldn't be made: not tried again until the island's thread starts
// again.
bool g_glassCompositionFailed = false;
// Each glass's window is made again once this changes (see RenewAllGlass):
// one made as Windows starts may never get Windows' blur (measured at
// sign-in: the island's glass stayed black, while the panels' glass made
// later showed the blur).
unsigned g_glassGeneration = 0;

// Whether the Liquid Glass shows now (the island's own look, see
// IslandScope): Windows' blur only with its transparency effects on.
bool LiquidGlassOn() {
    return g_settings.islandLiquidGlass && !g_energySaver &&
           g_transparencyEffects && !g_glassCompositionFailed;
}

// Light and shadow like macOS 27 (islandGlassLight, only over the Liquid
// Glass; idea by Thiago Silva): the glass gets a sheen, a beveled edge, light
// along its bottom and a soft shadow (see GlassLayer), and the panels' cards
// (the light fills in the texts' color) look like glass too (see GlassFill).
bool GlassLightOn() {
    return g_settings.islandGlassLight && LiquidGlassOn();
}

// How round the control center's tiles, buttons and sections are: up to
// 22px with the light (a pill or a circle when smaller), as macOS 27's.
float ControlRadius(const D2D1_RECT_F& rect, float scale) {
    if (!GlassLightOn()) {
        return std::round(14 * scale);
    }
    return std::min(
        std::round(22 * scale),
        std::min(rect.right - rect.left, rect.bottom - rect.top) / 2);
}

// The cards' light, as gradient brushes of a drawing target (a brush can't be
// used on another one): the sheen and the edge, each for dark and for light
// glass, made the first time a card needs them. Their strength is the
// brush's opacity and their places are set for each card, so the card
// itself makes nothing. The targets of the last few windows are kept (the
// target itself too, so the address of one gone is never taken for a new
// one's); everything is released with the glass (see ReleaseGlass).
struct GlassCardTarget {
    winrt::com_ptr<ID2D1RenderTarget> target;
    winrt::com_ptr<ID2D1LinearGradientBrush> sheen[2];
    winrt::com_ptr<ID2D1LinearGradientBrush> edge[2];
};
constexpr size_t kGlassCardTargets = 8;
[[clang::no_destroy]] std::optional<std::vector<GlassCardTarget>>
    g_glassCardTargets;

// The cards' light of this target (the last one used first).
GlassCardTarget& GlassCardsFor(ID2D1RenderTarget* target) {
    if (!g_glassCardTargets) {
        g_glassCardTargets.emplace();
    }
    auto& all = *g_glassCardTargets;
    const auto it =
        std::find_if(all.begin(), all.end(), [&](const GlassCardTarget& cards) {
            return cards.target.get() == target;
        });
    if (it == all.end()) {
        if (all.size() >= kGlassCardTargets) {
            all.pop_back();
        }
        GlassCardTarget made;
        made.target.copy_from(target);
        all.insert(all.begin(), std::move(made));
    } else if (it != all.begin()) {
        std::rotate(all.begin(), it, it + 1);
    }
    return all.front();
}

// How much of its light the brightest card gets (see GlassCardLight).
constexpr float kGlassCardMost = 1.4f;

// One of those brushes, at that strength (nullptr if it can't be made):
// white light, or a cool gray edge on light glass.
winrt::com_ptr<ID2D1LinearGradientBrush>
MakeGlassCardBrush(ID2D1RenderTarget* target, bool edge, bool light) {
    static constexpr float kOffsets[2][4] = {{0, 0.5f, 1, 0},
                                             {0, 0.3f, 0.6f, 1}};
    static constexpr float kAlphas[2][2][4] = {
        {{0.11f, 0.035f, 0, 0}, {0.30f, 0.10f, 0, 0}},
        {{0.45f, 0.12f, 0.06f, 0.22f}, {0.55f, 0.12f, 0.06f, 0.30f}}};
    const D2D1_COLOR_F ink = edge && light ? D2D1::ColorF(0.45f, 0.47f, 0.52f)
                                           : D2D1::ColorF(1, 1, 1);
    const UINT32 count = edge ? 4 : 3;
    D2D1_GRADIENT_STOP stops[4];
    for (UINT32 i = 0; i < count; i++) {
        stops[i] = {
            kOffsets[edge][i],
            {ink.r, ink.g, ink.b, kAlphas[edge][light][i] * kGlassCardMost}};
    }
    winrt::com_ptr<ID2D1GradientStopCollection> collection;
    winrt::com_ptr<ID2D1LinearGradientBrush> brush;
    if (FAILED(target->CreateGradientStopCollection(stops, count,
                                                    collection.put())) ||
        FAILED(target->CreateLinearGradientBrush({}, collection.get(),
                                                 brush.put()))) {
        return nullptr;
    }
    return brush;
}

// The card's fill, if it's one to make glass of: its opacity, or 0. Cards are
// the light fills of the panels (the texts' color at 3 to 32%), big enough
// for the light to show; the accent, the backgrounds and small dots are
// drawn as they were.
float GlassCardAmount(ID2D1SolidColorBrush* brush, float width, float height) {
    if (width < 16 || height < 16) {
        return 0;
    }
    const D2D1_COLOR_F c = brush->GetColor();
    const float a = c.a * brush->GetOpacity();
    const float off = std::fabs(c.r - g_theme.foreground[0]) +
                      std::fabs(c.g - g_theme.foreground[1]) +
                      std::fabs(c.b - g_theme.foreground[2]);
    return a >= 0.03f && a <= 0.32f && off < 0.08f ? a : 0;
}

// The light of glass over a card in `rect`, `amount` as opaque: a sheen over
// its upper part and its beveled edge, catching the light at the top left.
template <typename FillShape, typename DrawEdge>
void GlassCardLight(ID2D1RenderTarget* target,
                    const D2D1_RECT_F& rect,
                    float amount,
                    FillShape fillShape,
                    DrawEdge drawEdge) {
    const float w = rect.right - rect.left, h = rect.bottom - rect.top;
    // Brighter cards (under the mouse, chosen) catch a little more light;
    // small ones (the island's buttons) less of it (the brushes' opacity).
    const float k = std::clamp(amount / 0.09f, 0.6f, kGlassCardMost) /
                    kGlassCardMost *
                    std::clamp((std::min(w, h) - 16) / 48.0f, 0.35f, 1.0f);
    const bool light = g_theme.light > 0.5f;
    GlassCardTarget& cards = GlassCardsFor(target);
    if (!cards.sheen[light]) {
        cards.sheen[light] = MakeGlassCardBrush(target, false, light);
    }
    if (ID2D1LinearGradientBrush* sheen = cards.sheen[light].get()) {
        sheen->SetStartPoint({0, rect.top});
        sheen->SetEndPoint({0, rect.top + std::min(h * 0.5f, 28.0f)});
        sheen->SetOpacity(k);
        fillShape(sheen);
    }
    if (!cards.edge[light]) {
        cards.edge[light] = MakeGlassCardBrush(target, true, light);
    }
    if (ID2D1LinearGradientBrush* edge = cards.edge[light].get()) {
        const float side = std::min(w, h);
        edge->SetStartPoint({rect.left, rect.top});
        edge->SetEndPoint({rect.left + side * 1.4f, rect.top + side});
        edge->SetOpacity(k);
        drawEdge(edge);
    }
}

// The mod's logo (the site's, its favicon.svg): the island's pill at the top
// of a night desktop, a dock with four apps under the mountains, on a square
// with round corners; drawn at any size into the square `box`.
void DrawModLogo(ID2D1RenderTarget* target,
                 const D2D1_RECT_F& box,
                 float opacity = 1) {
    winrt::com_ptr<ID2D1Factory> factory;
    target->GetFactory(factory.put());
    D2D1_MATRIX_3X2_F was;
    target->GetTransform(&was);
    // (Drawn in its own 512 x 512.)
    const float k = (box.right - box.left) / 512;
    target->SetTransform(D2D1::Matrix3x2F::Scale(k, k) *
                         D2D1::Matrix3x2F::Translation(box.left, box.top) *
                         was);
    auto color = [&](UINT32 rgb, float alpha = 1) {
        return D2D1::ColorF(rgb, alpha * opacity);
    };
    auto gradient = [&](std::initializer_list<D2D1_GRADIENT_STOP> stops,
                        float top, float bottom) {
        const std::vector<D2D1_GRADIENT_STOP> list(stops);
        winrt::com_ptr<ID2D1GradientStopCollection> collection;
        winrt::com_ptr<ID2D1LinearGradientBrush> brush;
        if (SUCCEEDED(target->CreateGradientStopCollection(
                list.data(), (UINT32)list.size(), collection.put()))) {
            target->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties({0, top}, {0, bottom}),
                collection.get(), brush.put());
        }
        return brush;
    };
    // (Its round corners: everything cut to them.)
    winrt::com_ptr<ID2D1RoundedRectangleGeometry> corners;
    winrt::com_ptr<ID2D1Layer> layer;
    if (factory &&
        SUCCEEDED(factory->CreateRoundedRectangleGeometry(
            {{0, 0, 512, 512}, 112, 112}, corners.put())) &&
        SUCCEEDED(target->CreateLayer(nullptr, layer.put()))) {
        target->PushLayer(
            D2D1::LayerParameters(D2D1::InfiniteRect(), corners.get()),
            layer.get());
    } else {
        layer = nullptr;
    }
    if (auto sky = gradient({{0, color(0x1F2C44)},
                             {0.62f, color(0x0F1624)},
                             {1, color(0x07090D)}},
                            0, 512)) {
        target->FillRectangle({0, 0, 512, 512}, sky.get());
    }
    // The mountains.
    winrt::com_ptr<ID2D1PathGeometry> ridge;
    winrt::com_ptr<ID2D1GeometrySink> sink;
    if (factory && SUCCEEDED(factory->CreatePathGeometry(ridge.put())) &&
        SUCCEEDED(ridge->Open(sink.put()))) {
        static constexpr D2D1_POINT_2F kRidge[] = {
            {92, 262},  {150, 292}, {236, 220}, {318, 278},
            {386, 238}, {512, 300}, {512, 512}, {0, 512}};
        sink->BeginFigure({0, 318}, D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLines(kRidge, ARRAYSIZE(kRidge));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        if (SUCCEEDED(sink->Close())) {
            if (auto fill = gradient({{0, color(0x2B3A55, 0.9f)},
                                      {1, color(0x0D131F, 0.9f)}},
                                     220, 512)) {
                target->FillGeometry(ridge.get(), fill.get());
            }
        }
    }
    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(target->CreateSolidColorBrush(color(0), brush.put()))) {
        // The island: its pill, a line of text and the accent's dot.
        const D2D1_ROUNDED_RECT pill{{96, 80, 416, 168}, 44, 44};
        target->FillRoundedRectangle(pill, brush.get());
        brush->SetColor(color(0xFFFFFF, 0.22f));
        target->DrawRoundedRectangle(pill, brush.get(), 4);
        brush->SetColor(color(0xFFFFFF));
        target->FillRoundedRectangle({{152, 114, 268, 134}, 10, 10},
                                     brush.get());
        brush->SetColor(color(0x4CC2FF));
        target->FillEllipse({{356, 124}, 18, 18}, brush.get());
        // The dock and its four apps.
        const D2D1_ROUNDED_RECT dock{{84, 350, 428, 446}, 32, 32};
        brush->SetColor(color(0xFFFFFF, 0.16f));
        target->FillRoundedRectangle(dock, brush.get());
        brush->SetColor(color(0xFFFFFF, 0.26f));
        target->DrawRoundedRectangle(dock, brush.get(), 3);
        static constexpr UINT32 kApps[] = {0xFF8A3D, 0x4CC2FF, 0xFFD166,
                                           0xB892FF};
        for (int i = 0; i < 4; i++) {
            const float x = 114.0f + 76 * i;
            brush->SetColor(color(kApps[i]));
            target->FillRoundedRectangle({{x, 368, x + 60, 428}, 16, 16},
                                         brush.get());
        }
    }
    if (layer) {
        target->PopLayer();
    }
    target->SetTransform(was);
}

// The logo as an icon `size` pixels wide (the settings window's, in the
// taskbar and Alt+Tab), or null.
HICON MakeLogoIcon(ID2D1Factory* factory, int size) {
    BITMAPINFO info{};
    info.bmiHeader = {sizeof(BITMAPINFOHEADER), size, -size, 1, 32, BI_RGB};
    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP color =
        CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, screen);
    HICON icon = nullptr;
    winrt::com_ptr<ID2D1DCRenderTarget> target;
    const auto properties = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                          D2D1_ALPHA_MODE_PREMULTIPLIED),
        96, 96);
    if (dc && color && bits && factory &&
        SUCCEEDED(factory->CreateDCRenderTarget(&properties, target.put()))) {
        const HGDIOBJ old = SelectObject(dc, color);
        const RECT all{0, 0, size, size};
        if (SUCCEEDED(target->BindDC(dc, &all))) {
            target->BeginDraw();
            target->Clear(D2D1::ColorF(0, 0, 0, 0));
            DrawModLogo(target.get(), {0, 0, (float)size, (float)size});
            if (SUCCEEDED(target->EndDraw())) {
                GdiFlush();
                // (An icon's colors aren't premultiplied.)
                auto* pixels = static_cast<uint8_t*>(bits);
                for (int i = 0; i < size * size * 4; i += 4) {
                    const int alpha = pixels[i + 3];
                    for (int c = 0; alpha && alpha < 255 && c < 3; c++) {
                        pixels[i + c] =
                            (uint8_t)std::min(255, pixels[i + c] * 255 / alpha);
                    }
                }
                const std::vector<BYTE> zeros((size_t)((size + 15) / 16) * 2 *
                                              size);
                HBITMAP mask = CreateBitmap(size, size, 1, 1, zeros.data());
                ICONINFO parts{TRUE, 0, 0, mask, color};
                if (mask) {
                    icon = CreateIconIndirect(&parts);
                    DeleteObject(mask);
                }
            }
        }
        SelectObject(dc, old);
    }
    if (color) {
        DeleteObject(color);
    }
    if (dc) {
        DeleteDC(dc);
    }
    return icon;
}

// A card's fill (FillRoundedRectangle) with the light of glass over it when
// there's the light; the same drawing without it.
void GlassFill(ID2D1RenderTarget* target,
               const D2D1_ROUNDED_RECT& shape,
               ID2D1SolidColorBrush* brush) {
    target->FillRoundedRectangle(shape, brush);
    if (!GlassLightOn()) {
        return;
    }
    const D2D1_RECT_F& r = shape.rect;
    const float amount =
        GlassCardAmount(brush, r.right - r.left, r.bottom - r.top);
    if (amount <= 0) {
        return;
    }
    GlassCardLight(
        target, r, amount,
        [&](ID2D1Brush* b) { target->FillRoundedRectangle(shape, b); },
        [&](ID2D1Brush* b) {
            const D2D1_ROUNDED_RECT line{
                {r.left + 0.5f, r.top + 0.5f, r.right - 0.5f, r.bottom - 0.5f},
                std::max(0.0f, shape.radiusX - 0.5f),
                std::max(0.0f, shape.radiusY - 0.5f)};
            target->DrawRoundedRectangle(line, b, 1);
        });
}

// The same for a circle (FillEllipse).
void GlassFillEllipse(ID2D1RenderTarget* target,
                      const D2D1_ELLIPSE& shape,
                      ID2D1SolidColorBrush* brush) {
    target->FillEllipse(shape, brush);
    if (!GlassLightOn()) {
        return;
    }
    const D2D1_RECT_F r{
        shape.point.x - shape.radiusX, shape.point.y - shape.radiusY,
        shape.point.x + shape.radiusX, shape.point.y + shape.radiusY};
    const float amount =
        GlassCardAmount(brush, r.right - r.left, r.bottom - r.top);
    if (amount <= 0) {
        return;
    }
    GlassCardLight(
        target, r, amount,
        [&](ID2D1Brush* b) { target->FillEllipse(shape, b); },
        [&](ID2D1Brush* b) {
            target->DrawEllipse(
                {shape.point, std::max(0.0f, shape.radiusX - 0.5f),
                 std::max(0.0f, shape.radiusY - 0.5f)},
                b, 1);
        });
}

// A glass under one of the island's windows (its owner): a window of its own,
// which the mouse goes through, right under it.
class GlassLayer {
   public:
    GlassLayer() = default;
    GlassLayer(const GlassLayer&) = delete;
    GlassLayer& operator=(const GlassLayer&) = delete;
    // Shows it under `owner` (on the screen at `position`, `size` big) with
    // these shapes, in the owner's coordinates; made again if any changed.
    void Show(HWND owner,
              POINT position,
              SIZE size,
              const std::vector<GlassShape>& shapes);
    void Hide();
    void Destroy();
    // Drawn on the screen with what's behind: the owner leaves its own
    // background out then.
    bool Shown() const { return m_visible && m_drawn; }
    // Right under its owner again (it was put on top of the others).
    void FollowOwner();

   private:
    void ShowBlurred(HWND owner,
                     POINT position,
                     SIZE size,
                     const std::vector<GlassShape>& shapes);
    void UpdateBlurredShape(size_t index, const GlassShape& shape);
    // With the light (islandGlassLight): the shadow around the shapes, and
    // all the visuals made again when the light is turned on or off.
    void UpdateShadow(const std::vector<GlassShape>& shapes);
    void ResetLook();
    // The blur of what's behind as chosen: Windows' own, or (more blur or
    // translucent or with the light) an effect over it, made again when it
    // changes.
    winrt::Windows::UI::Composition::CompositionBrush Backdrop(
        GlassComposition& parts);
    // Its window and everything drawn on it gone.
    void DestroyWindowParts();
    void Place(HWND owner, POINT position, SIZE size, bool moved);

    HWND m_hwnd = nullptr;
    HWND m_owner = nullptr;
    // The window's composition tree, and each shape's visuals (the blur, the
    // tint, the light over its upper half, the bright edge), with the shape
    // they were made for.
    winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget
        m_desktopTarget{nullptr};
    winrt::Windows::UI::Composition::ContainerVisual m_root{nullptr};
    struct BlurredShape {
        winrt::Windows::UI::Composition::ContainerVisual container{nullptr};
        winrt::Windows::UI::Composition::SpriteVisual backdrop{nullptr};
        winrt::Windows::UI::Composition::SpriteVisual tint{nullptr};
        winrt::Windows::UI::Composition::SpriteVisual sheen{nullptr};
        winrt::Windows::UI::Composition::ShapeVisual rim{nullptr};
        // With the light: light bouncing back along its bottom, and the
        // glass's thickness just inside its edge.
        winrt::Windows::UI::Composition::SpriteVisual bounce{nullptr};
        winrt::Windows::UI::Composition::CompositionLinearGradientBrush
            bounceBrush{nullptr};
        winrt::Windows::UI::Composition::ShapeVisual band{nullptr};
        winrt::Windows::UI::Composition::CompositionSpriteShape bandShape{
            nullptr};
        winrt::Windows::UI::Composition::CompositionColorBrush tintBrush{
            nullptr};
        winrt::Windows::UI::Composition::CompositionLinearGradientBrush
            sheenBrush{nullptr};
        winrt::Windows::UI::Composition::CompositionLinearGradientBrush
            rimBrush{nullptr};
        // With the light: the soft edge of small glass.
        winrt::Windows::UI::Composition::CompositionLinearGradientBrush
            softRimBrush{nullptr};
        winrt::Windows::UI::Composition::CompositionSpriteShape rimShape{
            nullptr};
        // The blur it shows (see Backdrop).
        winrt::Windows::UI::Composition::CompositionBrush backdropBrush{
            nullptr};
        GlassShape made;
        bool hasShape = false;
    };
    std::vector<BlurredShape> m_blurred;
    // The effect over Windows' blur (see Backdrop), the blur it adds, and
    // whether it's the translucent one or with the light; it couldn't be
    // made (Windows' own then, until the window is made again).
    winrt::Windows::UI::Composition::CompositionEffectBrush m_effect{nullptr};
    float m_effectBlur = -1;
    bool m_effectTranslucent = false;
    bool m_effectLight = false;
    bool m_effectFailed = false;
    // Whether the visuals were made with the light (islandGlassLight), and
    // its owner's scaling (the light is in scale with the glass).
    bool m_light = false;
    float m_scale = 1;
    // The shadow, under every shape: a few layers of each shape's size
    // growing and fading out, and the shapes they were made for.
    struct ShadowLayer {
        winrt::Windows::UI::Composition::CompositionRoundedRectangleGeometry
            geometry{nullptr};
        winrt::Windows::UI::Composition::CompositionColorBrush brush{nullptr};
    };
    winrt::Windows::UI::Composition::ShapeVisual m_shadow{nullptr};
    std::vector<ShadowLayer> m_shadowLayers;
    std::vector<GlassShape> m_shadowShapes;
    POINT m_position{};
    SIZE m_size{};
    std::vector<GlassShape> m_shapes;
    bool m_visible = false;
    bool m_drawn = false;
    // The g_glassGeneration its window was made for.
    unsigned m_generation = 0;
    // When Windows last refused its blur to the window (see ShowBlurred):
    // tried again a while later, the plain color meanwhile.
    double m_refusedAt = -100;
};

// The parts for Windows' blur, made the first time (nullptr if they can't
// be).
GlassComposition* GlassCompositionParts() {
    if (g_glassComposition) {
        return &*g_glassComposition;
    }
    if (g_glassCompositionFailed) {
        return nullptr;
    }
    GlassComposition parts;
    using CreateController_t =
        HRESULT(WINAPI*)(DispatcherQueueOptions, void**);
    static const HMODULE messaging = LoadLibraryEx(
        L"CoreMessaging.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    auto createController =
        messaging ? (CreateController_t)GetProcAddress(
                        messaging, "CreateDispatcherQueueController")
                  : nullptr;
    const DispatcherQueueOptions options{sizeof(options),
                                         DQTYPE_THREAD_CURRENT,
                                         DQTAT_COM_STA};
    HRESULT hr = E_NOTIMPL;
    try {
        if (!createController ||
            FAILED(hr = createController(options,
                                         winrt::put_abi(parts.queue))) ||
            FAILED(hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                          __uuidof(ID2D1Factory), nullptr,
                                          parts.factory.put_void()))) {
            IslandLog(L"Liquid Glass: Windows' blur failed: %08X", (unsigned)hr);
            g_glassCompositionFailed = true;
            return nullptr;
        }
        parts.compositor = winrt::Windows::UI::Composition::Compositor();
        parts.backdrop = parts.compositor.CreateHostBackdropBrush();
    } catch (winrt::hresult_error const& e) {
        IslandLog(L"Liquid Glass: Windows' blur failed: %08X", (unsigned)e.code());
        g_glassCompositionFailed = true;
        return nullptr;
    }
    g_glassComposition.emplace(std::move(parts));
    return &*g_glassComposition;
}

// With the island's thread, after every glass is gone.
void ReleaseGlass() {
    // (The cards' light: the targets it keeps go with the island's windows.)
    g_glassCardTargets.reset();
    if (g_glassComposition) {
        auto queue = std::move(g_glassComposition->queue);
        g_glassComposition.reset();
        // The queue ends with the messages it still has (pumped here, a
        // short while at the most).
        if (queue) {
            try {
                const auto shutdown = queue.ShutdownQueueAsync();
                for (int i = 0;
                     i < 100 && shutdown.Status() ==
                                    winrt::Windows::Foundation::AsyncStatus::
                                        Started;
                     i++) {
                    MsgWaitForMultipleObjects(0, nullptr, FALSE, 10,
                                              QS_ALLINPUT);
                    MSG msg;
                    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                        DispatchMessage(&msg);
                    }
                }
            } catch (winrt::hresult_error const& e) {
                IslandLog(L"Liquid Glass: ending the queue failed: %08X",
                       (unsigned)e.code());
            }
        }
    }
    g_glassCompositionFailed = false;
}

LRESULT CALLBACK GlassWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                              LPARAM lParam) {
    if (msg == WM_NCHITTEST) {
        return HTTRANSPARENT;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

