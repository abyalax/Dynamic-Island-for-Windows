#pragma once
// Every property the mod sets, with the value it had before, so that it can
// be put back.
struct StyledProperty {
    winrt::weak_ref<DependencyObject> element;
    DependencyProperty property{nullptr};
    // DependencyProperty::UnsetValue() if it had no local value.
    winrt::Windows::Foundation::IInspectable original{nullptr};
};
// The globals below hold XAML objects. They're released on the taskbar's
// thread when the mod unloads (see ApplySettingsFromTaskbarThread), and never
// destroyed at process exit, when XAML is already gone (Windhawk wiki: "Global
// objects and process shutdown").
[[clang::no_destroy]] std::vector<StyledProperty> g_styledProperties;

// Keeps the dock's width automatic, because the taskbar sets the frame's width
// itself.
struct WidthWatch {
    winrt::weak_ref<FrameworkElement> element;
    int64_t token;
};
[[clang::no_destroy]] std::vector<WidthWatch> g_widthWatches;

void SetStyled(DependencyObject const& element,
               DependencyProperty const& property,
               winrt::Windows::Foundation::IInspectable const& value) {
    const bool known =
        std::any_of(g_styledProperties.begin(), g_styledProperties.end(),
                    [&](const StyledProperty& entry) {
                        return entry.property == property &&
                               entry.element.get() == element;
                    });
    if (!known) {
        g_styledProperties.push_back({winrt::make_weak(element), property,
                                      element.ReadLocalValue(property)});
    }
    element.SetValue(property, value);
}

void ResetDockBehavior();

void ClearStyles() {
    ResetDockBehavior();

    for (const auto& watch : g_widthWatches) {
        if (auto element = watch.element.get()) {
            Safely(L"Releasing the taskbar width watch", [&] {
                element.UnregisterPropertyChangedCallback(
                    FrameworkElement::WidthProperty(), watch.token);
            });
        }
    }
    g_widthWatches.clear();

    // In reverse, in case a property was recorded twice.
    for (auto it = g_styledProperties.rbegin(); it != g_styledProperties.rend();
         ++it) {
        auto element = it->element.get();
        if (!element) {
            continue;
        }
        Safely(L"Restoring a taskbar property", [&] {
            if (it->original == DependencyProperty::UnsetValue()) {
                element.ClearValue(it->property);
            } else {
                element.SetValue(it->property, it->original);
            }
        });
    }
    g_styledProperties.clear();
}

bool IsLightTheme() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValue(HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\"
                    L"Personalize",
                    L"SystemUsesLightTheme", RRF_RT_REG_DWORD, nullptr, &value,
                    &size) != ERROR_SUCCESS) {
        return false;
    }
    return value != 0;
}

// The dock's tint, in the theme's color.
winrt::Windows::UI::Color DockTint() {
    return IsLightTheme() ? winrt::Windows::UI::Color{255, 255, 255, 255}
                          : winrt::Windows::UI::Color{255, 30, 30, 36};
}

// The dock's background (its shelf, and the tray's pill): the tint, as
// see-through as chosen; none without the background. (XAML's acrylic can't
// blur behind the taskbar: it drew its fallback color, and black once the
// taskbar had the focus.)
Media::Brush CreateDockBackground() {
    winrt::Windows::UI::Color tint = DockTint();
    tint.A = g_settings.dockBackground
                 ? (uint8_t)std::lround(255 * g_settings.backgroundOpacity)
                 : 0;
    return Media::SolidColorBrush(tint);
}

// Its thin edge: a faint white line (a stronger one on the light theme, like
// light caught by the glass); none without the background.
winrt::Windows::UI::Color DockEdgeColor() {
    if (!g_settings.dockBackground) {
        return {0, 255, 255, 255};
    }
    return IsLightTheme() ? winrt::Windows::UI::Color{0xB3, 255, 255, 255}
                          : winrt::Windows::UI::Color{0x21, 255, 255, 255};
}

Media::Brush CreateDockBorder() {
    return Media::SolidColorBrush(DockEdgeColor());
}

// Styles a Grid or StackPanel as a rounded pill, on the dock's background.
void StylePill(FrameworkElement const& element, Thickness margin) {
    const auto radius =
        CornerRadiusHelper::FromUniformRadius(g_settings.cornerRadius);
    const auto border = ThicknessHelper::FromUniformLength(1);

    SetStyled(element, Controls::Panel::BackgroundProperty(),
              CreateDockBackground());
    SetStyled(element, FrameworkElement::MarginProperty(),
              winrt::box_value(margin));

    if (element.try_as<Controls::Grid>()) {
        SetStyled(element, Controls::Grid::CornerRadiusProperty(),
                  winrt::box_value(radius));
        SetStyled(element, Controls::Grid::BorderThicknessProperty(),
                  winrt::box_value(border));
        SetStyled(element, Controls::Grid::BorderBrushProperty(),
                  CreateDockBorder());
    } else if (element.try_as<Controls::StackPanel>()) {
        SetStyled(element, Controls::StackPanel::CornerRadiusProperty(),
                  winrt::box_value(radius));
        SetStyled(element, Controls::StackPanel::BorderThicknessProperty(),
                  winrt::box_value(border));
        SetStyled(element, Controls::StackPanel::BorderBrushProperty(),
                  CreateDockBorder());
    }
}

// The dock's shelf: the rounded glass under its icons, on a canvas of the
// mod's own behind everything in the taskbar's XAML (see ApplyStyleUnscoped),
// moved and sized each frame as the icons grow (see CreateDockBackground).
struct DockShelf {
    Controls::Border border{nullptr};
    // Where it was last put, in its canvas's coordinates, and its corners.
    winrt::Windows::Foundation::Rect placed{};
    double radius = -1;
};

DockShelf MakeDockShelf(Controls::Canvas const& host) {
    DockShelf shelf;
    shelf.border = Controls::Border();
    shelf.border.Background(CreateDockBackground());
    shelf.border.BorderBrush(CreateDockBorder());
    shelf.border.BorderThickness(ThicknessHelper::FromUniformLength(1));
    shelf.border.Width(0);
    shelf.border.Height(0);
    if (!g_settings.dockBackground) {
        shelf.border.Visibility(Visibility::Collapsed);
    }
    host.Children().Append(shelf.border);
    return shelf;
}

// Puts the shelf at `rect` (its canvas's coordinates).
void PlaceDockShelf(DockShelf& shelf,
                    winrt::Windows::Foundation::Rect rect,
                    double radius) {
    if (!shelf.border ||
        (std::fabs(rect.X - shelf.placed.X) < 0.05f &&
         std::fabs(rect.Y - shelf.placed.Y) < 0.05f &&
         std::fabs(rect.Width - shelf.placed.Width) < 0.05f &&
         std::fabs(rect.Height - shelf.placed.Height) < 0.05f &&
         radius == shelf.radius)) {
        return;
    }
    shelf.placed = rect;
    Controls::Canvas::SetLeft(shelf.border, rect.X);
    Controls::Canvas::SetTop(shelf.border, rect.Y);
    shelf.border.Width(std::max(0.0f, rect.Width));
    shelf.border.Height(std::max(0.0f, rect.Height));
    if (radius != shelf.radius) {
        shelf.radius = radius;
        shelf.border.CornerRadius(CornerRadiusHelper::FromUniformRadius(radius));
    }
}

// The taskbar's background, under the dock's shelf (see DockShelf): nothing
// of its own shows.
void StyleDockBackground(Shapes::Rectangle const& fill) {
    SetStyled(fill, UIElement::VisibilityProperty(),
              winrt::box_value(Visibility::Visible));
    SetStyled(fill, Shapes::Shape::FillProperty(),
              Media::SolidColorBrush(winrt::Windows::UI::Colors::Transparent()));
    SetStyled(fill, Shapes::Shape::StrokeThicknessProperty(),
              winrt::box_value(0.0));
}

// The tray's pill's height, when it's shown beside the dock.
constexpr double kDockTrayHeight = 40;
// The element the dock's shelf is drawn on (see ApplyStyleUnscoped).
constexpr WCHAR kDockShelfHostName[] = L"WindhawkMacIslandDockShelf";

// Gives an element a height at the bottom of the taskbar.
void SetDockGeometry(FrameworkElement const& element,
                     double height,
                     double rightMargin) {
    SetStyled(element, FrameworkElement::HeightProperty(),
              winrt::box_value(height));
    SetStyled(element, FrameworkElement::VerticalAlignmentProperty(),
              winrt::box_value(VerticalAlignment::Bottom));
    SetStyled(element, FrameworkElement::MarginProperty(),
              winrt::box_value(ThicknessHelper::FromLengths(
                  0, 0, rightMargin, g_settings.bottomGap)));
}

void WatchFrameWidth(FrameworkElement const& frame) {
    const bool watched =
        std::any_of(g_widthWatches.begin(), g_widthWatches.end(),
                    [&](const WidthWatch& watch) {
                        return watch.element.get() == frame;
                    });
    if (watched) {
        return;
    }
    const int64_t token = frame.RegisterPropertyChangedCallback(
        FrameworkElement::WidthProperty(),
        [](DependencyObject const& sender, DependencyProperty const&) {
            if (g_unloading) {
                return;
            }
            Safely(L"Keeping the frame's width", [&] {
                auto element = sender.as<FrameworkElement>();
                if (!std::isnan(element.Width())) {
                    element.Width(std::numeric_limits<double>::quiet_NaN());
                }
            });
        });
    g_widthWatches.push_back({winrt::make_weak(frame), token});
}

////////////////////////////////////////////////////////////////////////////////
// Dock behavior: magnification, bouncing and dots (taskbar thread).

struct DockIcon {
    // The button grows and bounces as a whole.
    winrt::weak_ref<FrameworkElement> button;
    // The icon image inside it, used to line the icon up with the dock.
    winrt::weak_ref<FrameworkElement> image;
    Media::ScaleTransform scale{nullptr};
    Media::TranslateTransform lift{nullptr};
    // Eased towards the size the mouse position asks for, times the dock's
    // icon size over the taskbar's (dockIconSize; 1 on the transparent
    // taskbar).
    double magnification = 1;
    double baseScale = 1;
    // Where it takes the mouse: its own room in the row, so a magnified
    // icon's neighbors don't take its clicks (see OnRenderingUnsafe).
    Media::RectangleGeometry clip{nullptr};
    winrt::Windows::Foundation::Rect clipRect{};
    // An app's button (not Start or the search), and its picture: the app's
    // own icon as big as it's shown magnified, in place of the taskbar's
    // small one, which is put back when the mod lets go (see UseBigIcon).
    bool app = false;
    Media::ImageSource bigSource{nullptr};
    Media::ImageSource ownSource{nullptr};
    std::wstring ownAppId;
    // The size of the picture in bigSource (see BigIconPixels), and the app
    // it's for.
    int bigPixels = 0;
    std::wstring bigAppId;
    // The app's overlay icon (like Discord's red counter) drawn from the
    // app's own picture, and the taskbar's in its place before (see
    // UseBigOverlay).
    winrt::weak_ref<Controls::Image> overlayImage;
    Media::ImageSource overlayBig{nullptr};
    Media::ImageSource overlayOwn{nullptr};
    std::wstring overlayAppId;
    // An overlay that's a picture (like a browser profile's photo, 16
    // pixels): kept at its own size as the icon grows, sharp, rather than
    // magnified and blurred; a count drawn big grows with it (see
    // OnRenderingUnsafe).
    Media::ScaleTransform overlayKeep{nullptr};
    bool overlayCounted = false;
    bool overlayLooked = false;
    // Looked for in vain this many times (see kOverlayMostLooks).
    int overlayLooks = 0;
    // An app opened: its button grows in (0 to 1, a bit more while
    // bouncing).
    double appear = 1;
    double appearVelocity = 0;
    // Moved by the taskbar (an app opened or closed next to it, or the dock
    // centered again): it slides from where it was (its layout middle last
    // time, NAN before the first frame).
    double slide = 0;
    double slideVelocity = 0;
    double lastCenter = NAN;
    // Bouncing: when it started (0 when not bouncing) and how many groups of
    // bounces it has (see BounceSequence).
    double bounceStart = 0;
    int bounceGroups = 0;
    // Whether the app is open, and whether it's asking for attention, to
    // notice when that changes.
    bool open = false;
    bool attention = false;
    // The app's notification badge (the count on the icon), once it exists
    // (see CheckBadge): the one watched, its text and whether it was shown
    // when last checked, for which app (buttons are reused for other apps),
    // and when it last counted as a notification.
    winrt::weak_ref<FrameworkElement> badge;
    std::wstring badgeText;
    bool badgeShown = false;
    std::wstring badgeAppId;
    double lastNotified = 0;
    // The small parts drawn over the icon's corners, sticking out of it (the
    // badge, an app's own overlay icon like Discord's red one): their highest
    // top and rightmost edge, in the button's coordinates (0 for none). See
    // MeasureDockExtras.
    double extrasTop = 0;
    double extrasRight = 0;
    // The running indicators turned into dots (see UpdateDots).
    struct Dot {
        winrt::weak_ref<Shapes::Rectangle> rectangle;
        Media::ScaleTransform scale{nullptr};
        Media::TranslateTransform move{nullptr};
    };
    std::vector<Dot> dots;
};

[[clang::no_destroy]] std::vector<DockIcon> g_dockIcons;
void UseBigIcon(DockIcon& icon, FrameworkElement const& button, int pixels);
void UseBigOverlay(DockIcon& icon,
                   FrameworkElement const& button,
                   double factor);
void RestoreDockIcon(DockIcon& icon);
HWND g_dockWnd;
winrt::weak_ref<FrameworkElement> g_dockFrame;
winrt::weak_ref<FrameworkElement> g_dockRepeater;
// The taskbar's background, which the dock's shelf is drawn on (see
// DockShelf).
winrt::weak_ref<FrameworkElement> g_dockBackground;
[[clang::no_destroy]] DockShelf g_dockShelf;
// The shelf at rest, in the window's pixels (see ClipDockWindow), and the
// last frame's, in its XAML coordinates (where the mouse makes it grow).
RECT g_dockShelfRest{};
winrt::Windows::Foundation::Rect g_dockShelfNow{};
// Whether the dock's room above the taskbar takes the mouse (the mouse on the
// dock, or its icons moving): see ClipDockWindow.
bool g_dockRoomOpen;
// The transparent taskbar's shadow, and how strong it's shown.
winrt::weak_ref<UIElement> g_shadowFill;
double g_shadowStrength = 0.6;
// The transparent taskbar only takes the mouse around its apps: its window is
// cut down to them (a window region), so clicks elsewhere reach the windows
// behind it, and the bottom edge brings it up only there. In the window's
// pixels, and on the screen for the bottom edge (see EdgeThreadProc).
RECT g_appsRegion{};
// The apps' extent it was made for (XAML coordinates), to make it again.
double g_appsLeft = 0;
double g_appsRight = 0;
// The monitor it was cut on (see g_appsAreas).
HMONITOR g_appsAreaMonitor;
// The span the shadow was last put behind.
double g_shadowLeft = 0;
double g_shadowRight = 0;
// Room around the apps that still counts as theirs, and the shadow's (which
// fades out before its edges).
constexpr double kAppsAreaMargin = 34;
constexpr double kShadowMargin = 30;
// The shadow's visual, drawn on its element.
[[clang::no_destroy]] winrt::Windows::UI::Composition::SpriteVisual
    g_shadowVisual{nullptr};
int g_dockItemCount = -1;
double g_dockSince;
double g_lastRenderTime;
// The dock as a box (dock.maxWidth): how far its row is scrolled (DIPs from
// its start), how far it can be (0: it all fits), and an app's room in it
// (a wheel notch scrolls one). Its wheel handler, kept to remove it again.
double g_dockScroll;
double g_dockScrollMax;
double g_dockScrollStep = 40;
[[clang::no_destroy]] winrt::Windows::Foundation::IInspectable
    g_wheelHandler{nullptr};
// When the taskbar's scale last changed (WM_DPICHANGED: hidden past its edge
// over a monitor with another scaling, then shown again): the icons take
// their new places at once for a moment, instead of sliding there.
double g_dockScaleChangedAt;
constexpr double kDockScaleSettleSeconds = 1.0;
// The taskbar's buttons are moved by its repeater's animator, one by one:
// when its scale changes, that's every button at once, out of order (Windows'
// own taskbar does it too). Meanwhile the animator is taken out: the value
// it had (DependencyProperty::UnsetValue() if none of its own), put back a
// moment later (see kAnimatorTimerId).
[[clang::no_destroy]] winrt::Windows::Foundation::IInspectable
    g_animatorKept{nullptr};
bool g_animatorPaused;
// The animator also leaves an implicit animation on each button's visual,
// which moves it on every layout change by itself: taken out meanwhile too,
// and put back on the buttons still there.
struct KeptImplicitAnimations {
    winrt::weak_ref<UIElement> element;
    winrt::Windows::UI::Composition::ImplicitAnimationCollection animations{
        nullptr};
};
[[clang::no_destroy]] std::vector<KeptImplicitAnimations> g_implicitKept;
// Until the taskbar is laid out again at its new scale (its buttons move
// once, a moment after WM_DPICHANGED), and while its scale isn't its own
// monitor's (hidden, mostly over a monitor with another scaling), its icons
// are drawn at the wrong size and places: its frame is see-through
// meanwhile (it still takes the mouse), its own opacity kept.
bool g_dockScaleSettled = true;
bool g_dockScaleMoved;
bool g_dockContentHidden;
[[clang::no_destroy]] winrt::Windows::Foundation::IInspectable
    g_dockOpacityKept{nullptr};
// (Windows lays it out again in about 0.15 s.)
constexpr double kDockScaleHideSeconds = 0.3;
// (Not one of the taskbar's own timers.)
constexpr UINT_PTR kAnimatorTimerId = 0x4D49;
constexpr UINT_PTR kDisplayRestyleTimerId = 0x4D4A;
constexpr UINT kAnimatorPauseMs = 1000;
// The repeater's Animator property, found by its name (the repeater is
// Microsoft.UI.Xaml's, which has no header here) as a style's setter, like
// the taskbar stylers do; null if it couldn't be.
[[clang::no_destroy]] DependencyProperty g_animatorProperty{nullptr};
bool g_animatorPropertyTried;

DependencyProperty RepeaterAnimatorProperty() {
    if (!g_animatorPropertyTried) {
        g_animatorPropertyTried = true;
        try {
            auto resources =
                Markup::XamlReader::Load(
                    LR"(<ResourceDictionary
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:muxc="using:Microsoft.UI.Xaml.Controls">
    <Style x:Key="repeater" TargetType="muxc:ItemsRepeater">
        <Setter Property="Animator" Value="{x:Null}"/>
    </Style>
</ResourceDictionary>)")
                    .as<ResourceDictionary>();
            auto style =
                resources.Lookup(winrt::box_value(L"repeater")).as<Style>();
            g_animatorProperty =
                style.Setters().GetAt(0).as<Setter>().Property();
        } catch (const winrt::hresult_error& e) {
            IslandLog(L"The taskbar's animator isn't reachable: %08X",
                   (unsigned)e.code());
        }
    }
    return g_animatorProperty;
}

void PauseRepeaterAnimations() {
    auto repeater = g_dockRepeater.get();
    if (g_animatorPaused || !repeater ||
        winrt::get_class_name(repeater) !=
            L"Microsoft.UI.Xaml.Controls.ItemsRepeater") {
        return;
    }
    if (auto property = RepeaterAnimatorProperty()) {
        g_animatorKept = repeater.ReadLocalValue(property);
        repeater.SetValue(property, nullptr);
        g_animatorPaused = true;
    }
    const int count = Media::VisualTreeHelper::GetChildrenCount(repeater);
    for (int i = 0; i < count; i++) {
        auto child = Media::VisualTreeHelper::GetChild(repeater, i)
                         .try_as<UIElement>();
        if (!child) {
            continue;
        }
        auto visual =
            Hosting::ElementCompositionPreview::GetElementVisual(child);
        if (auto animations = visual.ImplicitAnimations()) {
            g_implicitKept.push_back({winrt::make_weak(child), animations});
            visual.ImplicitAnimations(nullptr);
            g_animatorPaused = true;
        }
    }
}

void ResumeRepeaterAnimations() {
    if (!g_animatorPaused) {
        return;
    }
    // (Each on its own: a retired element failing mustn't keep the rest, or
    // the kept animations, after the unload.)
    auto repeater = g_dockRepeater.get();
    if (repeater && g_animatorProperty && g_animatorKept) {
        Safely(L"Restoring the dock's animator", [&] {
            if (g_animatorKept == DependencyProperty::UnsetValue()) {
                repeater.ClearValue(g_animatorProperty);
            } else {
                repeater.SetValue(g_animatorProperty, g_animatorKept);
            }
        });
    }
    for (const auto& kept : g_implicitKept) {
        if (auto element = kept.element.get()) {
            Safely(L"Restoring an icon's animations", [&] {
                Hosting::ElementCompositionPreview::GetElementVisual(element)
                    .ImplicitAnimations(kept.animations);
            });
        }
    }
    std::vector<KeptImplicitAnimations>().swap(g_implicitKept);
    g_animatorPaused = false;
    g_animatorKept = nullptr;
}

// The boxed handler, kept to remove it again.
[[clang::no_destroy]] winrt::Windows::Foundation::IInspectable
    g_pointerHandler{nullptr};
[[clang::no_destroy]] winrt::Windows::Foundation::IInspectable
    g_pressedHandler{nullptr};
winrt::weak_ref<FrameworkElement> g_pointerTarget;
[[clang::no_destroy]] FrameworkElement::LayoutUpdated_revoker g_layoutRevoker;
[[clang::no_destroy]] Media::CompositionTarget::Rendering_revoker
    g_renderingRevoker;
[[clang::no_destroy]] DispatcherTimer g_badgeTimer{nullptr};
// The whole dock shown for a notification (see ShowDockForNotification).
[[clang::no_destroy]] DispatcherTimer g_notificationTimer{nullptr};
// The dock shown for a notification (see ShowDockForNotification), and where
// it goes back to: each taskbar's own (see DockState).
bool g_shownForNotification;
RECT g_notificationHiddenRect;
[[clang::no_destroy]] std::vector<
    VisualStateGroup::CurrentStateChanged_revoker>
    g_stateRevokers;
[[clang::no_destroy]] std::vector<FrameworkElement::SizeChanged_revoker>
    g_dotRevokers;
// Weak references: safe to destroy at exit.
std::vector<winrt::weak_ref<FrameworkElement>> g_watchedButtons;

// Property callbacks on the notification badges, to remove them again.
struct BadgeWatch {
    winrt::weak_ref<DependencyObject> element;
    DependencyProperty property{nullptr};
    int64_t token;
};
[[clang::no_destroy]] std::vector<BadgeWatch> g_badgeWatches;
double g_lastBadgeScan;

// Whether the dock state in the globals above is the main taskbar's.
bool g_dockIsMain = true;

// The other monitors' taskbars have the same behavior as the main one
// (magnification, sliding and growing icons, dots, the transparent taskbar's
// cut and shadow), with a state of their own each, kept here. All taskbars
// are on one thread: the code above works on the globals, and a DockScope
// puts another taskbar's state in them while that taskbar is handled (its
// XAML events, its frames, its slides). The badges and the icon peeking out
// for attention are the main taskbar's only (the same apps are on each).
struct DockState {
    std::vector<DockIcon> icons;
    HWND wnd = nullptr;
    winrt::weak_ref<FrameworkElement> frame;
    winrt::weak_ref<FrameworkElement> repeater;
    winrt::weak_ref<FrameworkElement> background;
    DockShelf shelf;
    RECT shelfRest{};
    winrt::Windows::Foundation::Rect shelfNow{};
    bool roomOpen = false;
    winrt::weak_ref<UIElement> shadowFill;
    double shadowStrength = 0.6;
    winrt::Windows::UI::Composition::SpriteVisual shadowVisual{nullptr};
    RECT appsRegion{};
    double appsLeft = 0;
    double appsRight = 0;
    HMONITOR appsAreaMonitor = nullptr;
    double shadowLeft = 0;
    double shadowRight = 0;
    int itemCount = -1;
    double since = 0;
    double scaleChangedAt = 0;
    winrt::Windows::Foundation::IInspectable animatorKept{nullptr};
    bool animatorPaused = false;
    std::vector<KeptImplicitAnimations> implicitKept;
    bool scaleSettled = true;
    bool scaleMoved = false;
    bool contentHidden = false;
    winrt::Windows::Foundation::IInspectable opacityKept{nullptr};
    winrt::Windows::Foundation::IInspectable pointerHandler{nullptr};
    winrt::Windows::Foundation::IInspectable pressedHandler{nullptr};
    winrt::Windows::Foundation::IInspectable wheelHandler{nullptr};
    double scroll = 0;
    double scrollMax = 0;
    winrt::weak_ref<FrameworkElement> pointerTarget;
    FrameworkElement::LayoutUpdated_revoker layoutRevoker;
    bool isMain = false;
    bool shownForNotification = false;
    RECT notificationHiddenRect{};
    DispatcherTimer notificationTimer{nullptr};
};

[[clang::no_destroy]] std::vector<std::unique_ptr<DockState>> g_otherDocks;

void SwapDockState(DockState& state) {
    std::swap(g_dockIcons, state.icons);
    std::swap(g_dockWnd, state.wnd);
    std::swap(g_dockFrame, state.frame);
    std::swap(g_dockRepeater, state.repeater);
    std::swap(g_dockBackground, state.background);
    std::swap(g_dockShelf, state.shelf);
    std::swap(g_dockShelfRest, state.shelfRest);
    std::swap(g_dockShelfNow, state.shelfNow);
    std::swap(g_dockRoomOpen, state.roomOpen);
    std::swap(g_shadowFill, state.shadowFill);
    std::swap(g_shadowStrength, state.shadowStrength);
    std::swap(g_shadowVisual, state.shadowVisual);
    std::swap(g_appsRegion, state.appsRegion);
    std::swap(g_appsLeft, state.appsLeft);
    std::swap(g_appsRight, state.appsRight);
    std::swap(g_appsAreaMonitor, state.appsAreaMonitor);
    std::swap(g_shadowLeft, state.shadowLeft);
    std::swap(g_shadowRight, state.shadowRight);
    std::swap(g_dockItemCount, state.itemCount);
    std::swap(g_dockSince, state.since);
    std::swap(g_dockScaleChangedAt, state.scaleChangedAt);
    std::swap(g_animatorKept, state.animatorKept);
    std::swap(g_animatorPaused, state.animatorPaused);
    std::swap(g_implicitKept, state.implicitKept);
    std::swap(g_dockScaleSettled, state.scaleSettled);
    std::swap(g_dockScaleMoved, state.scaleMoved);
    std::swap(g_dockContentHidden, state.contentHidden);
    std::swap(g_dockOpacityKept, state.opacityKept);
    std::swap(g_pointerHandler, state.pointerHandler);
    std::swap(g_pressedHandler, state.pressedHandler);
    std::swap(g_wheelHandler, state.wheelHandler);
    std::swap(g_dockScroll, state.scroll);
    std::swap(g_dockScrollMax, state.scrollMax);
    std::swap(g_pointerTarget, state.pointerTarget);
    std::swap(g_layoutRevoker, state.layoutRevoker);
    std::swap(g_dockIsMain, state.isMain);
    std::swap(g_shownForNotification, state.shownForNotification);
    std::swap(g_notificationHiddenRect, state.notificationHiddenRect);
    std::swap(g_notificationTimer, state.notificationTimer);
}

DockState* FindOtherDock(HWND wnd) {
    for (auto& state : g_otherDocks) {
        if (state->wnd == wnd) {
            return state.get();
        }
    }
    return nullptr;
}

// Another taskbar's state in the globals for as long as it lives (nothing
// for the main taskbar, or inside another scope).
class DockScope {
   public:
    explicit DockScope(HWND wnd) {
        if (!s_active && wnd && wnd != g_dockWnd) {
            m_state = FindOtherDock(wnd);
            if (m_state) {
                SwapDockState(*m_state);
                s_active = true;
            }
        }
    }
    ~DockScope() {
        if (m_state) {
            SwapDockState(*m_state);
            s_active = false;
        }
    }
    DockScope(const DockScope&) = delete;
    DockScope& operator=(const DockScope&) = delete;

   private:
    DockState* m_state = nullptr;
    static inline bool s_active = false;
};

// The taskbars cut by ClipToOwnMonitor (their regions go when the mod does).
[[clang::no_destroy]] std::vector<HWND> g_ownClips;

// Where each transparent taskbar takes the mouse at the bottom edge (its
// apps, on the screen), by monitor, for the edge thread.
struct AppsArea {
    HMONITOR monitor;
    int left;
    int right;
};
SRWLOCK g_appsAreasLock = SRWLOCK_INIT;
std::vector<AppsArea> g_appsAreas;

// (An empty span forgets it.)
void SetAppsArea(HMONITOR monitor, int left, int right) {
    AcquireSRWLockExclusive(&g_appsAreasLock);
    std::erase_if(g_appsAreas, [&](const AppsArea& area) {
        return area.monitor == monitor;
    });
    if (monitor && right > left) {
        g_appsAreas.push_back({monitor, left, right});
    }
    ReleaseSRWLockExclusive(&g_appsAreasLock);
}

bool GetAppsArea(HMONITOR monitor, int* left, int* right) {
    AcquireSRWLockShared(&g_appsAreasLock);
    bool found = false;
    for (const auto& area : g_appsAreas) {
        if (area.monitor == monitor) {
            *left = area.left;
            *right = area.right;
            found = true;
        }
    }
    ReleaseSRWLockShared(&g_appsAreasLock);
    return found;
}

// Defined further down.
bool IsTaskbarHidden(HWND hTaskbarWnd, const RECT& monitorRect);
HBITMAP LoadAppIconBitmap(HWND hWnd, std::wstring appId, int size);
HBITMAP PinnedShortcutIconBitmap(const std::wstring& appId, int size);

// An app's icon kept by the island or a panel (see LoadAppIcon). One explorer
// didn't give (none, or the island's own try: a blank square for some
// packaged apps while explorer starts again) is looked up again later.
struct KeptAppIcon {
    std::wstring appId;
    winrt::com_ptr<ID2D1Bitmap> bitmap;
    double retryAt = 0;
    // Looked up in vain this many times in a row.
    int misses = 0;
};
constexpr double kAppIconRetrySeconds = 30;
// (None found, like just after the island or explorer started again: asked
// again soon, not after a gray square for half a minute; then later each
// time, up to kMissingIconLongestSeconds: an app whose icon never comes
// isn't asked of explorer every few seconds forever.)
constexpr double kMissingIconRetrySeconds = 3;
constexpr double kMissingIconLongestSeconds = 120;

// When an icon kept is looked up again (see KeptAppIcon): never explorer's,
// after a while the island's own, a missing one sooner (`misses` before).
double AppIconRetryAt(bool fromExplorer, bool found, int misses, double now) {
    if (fromExplorer) {
        return 0;
    }
    if (found) {
        return now + kAppIconRetrySeconds;
    }
    return now + std::min(kMissingIconRetrySeconds * (1 << std::min(misses, 6)),
                          kMissingIconLongestSeconds);
}

// A rounded inner corner where a bar meets the screen, in the brush's
// color: the square of `radius` at `corner` (going `along` the bar's edge and
// `out` into the screen) without the quarter circle facing away from it.
void FillInnerCorner(ID2D1RenderTarget* target,
                     ID2D1Brush* brush,
                     D2D1_POINT_2F corner,
                     D2D1_POINT_2F along,
                     D2D1_POINT_2F out,
                     float radius) {
    winrt::com_ptr<ID2D1Factory> factory;
    target->GetFactory(factory.put());
    winrt::com_ptr<ID2D1PathGeometry> path;
    winrt::com_ptr<ID2D1GeometrySink> sink;
    if (!factory || FAILED(factory->CreatePathGeometry(path.put())) ||
        FAILED(path->Open(sink.put()))) {
        return;
    }
    const D2D1_POINT_2F a{corner.x + along.x * radius,
                          corner.y + along.y * radius};
    const D2D1_POINT_2F b{corner.x + out.x * radius,
                          corner.y + out.y * radius};
    // (The quarter turn from `a` to `b` around the square's far corner, the
    // way that passes by `corner`.)
    const float turn = (-out.x) * (-along.y) - (-out.y) * (-along.x);
    sink->BeginFigure(corner, D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(a);
    sink->AddArc({b,
                  {radius, radius},
                  0,
                  turn > 0 ? D2D1_SWEEP_DIRECTION_CLOCKWISE
                           : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                  D2D1_ARC_SIZE_SMALL});
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    if (SUCCEEDED(sink->Close())) {
        target->FillGeometry(path.get(), brush);
    }
}

// The smooth way of drawing a picture smaller (see DrawBitmapSmooth) is
// Direct2D's only effect here, and the only drawing that needs a picture of
// its own on the way: some graphics drivers can't make it
// (D2DERR_INTERMEDIATE_TOO_LARGE, #6638: whole frames lost, the island left
// empty). Then pictures are made smaller the usual way from there on.
constexpr HRESULT kIntermediateTooLarge = (HRESULT)0x88990027;
std::atomic<bool> g_plainScaling{false};
// True if `hr` (a drawing's end) just turned the smooth way off.
bool PlainScalingAfter(HRESULT hr) {
    return hr == kIntermediateTooLarge && !g_plainScaling.exchange(true);
}

// A picture drawn smaller than it is (app icons, covers, photos): smoothly,
// with every pixel counted (Direct2D's usual way skips some, and small icons
// look jagged); as big or bigger, the usual way.
void DrawBitmapSmooth(ID2D1RenderTarget* target,
                      ID2D1Bitmap* bitmap,
                      const D2D1_RECT_F& rect,
                      float opacity = 1) {
    const D2D1_SIZE_F size = bitmap->GetSize();
    winrt::com_ptr<ID2D1DeviceContext> context;
    if ((rect.right - rect.left < size.width - 0.5f ||
         rect.bottom - rect.top < size.height - 0.5f) &&
        !g_plainScaling &&
        SUCCEEDED(target->QueryInterface(IID_PPV_ARGS(context.put())))) {
        context->DrawBitmap(bitmap, &rect, opacity,
                            D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC, nullptr,
                            nullptr);
        return;
    }
    target->DrawBitmap(bitmap, rect, opacity);
}

// A brush painting a picture (scaled by its transform) the same smooth way.
winrt::com_ptr<ID2D1BitmapBrush> SmoothBitmapBrush(ID2D1RenderTarget* target,
                                                   ID2D1Bitmap* bitmap) {
    winrt::com_ptr<ID2D1BitmapBrush> brush;
    winrt::com_ptr<ID2D1DeviceContext> context;
    winrt::com_ptr<ID2D1BitmapBrush1> smooth;
    const D2D1_BITMAP_BRUSH_PROPERTIES1 properties{
        D2D1_EXTEND_MODE_CLAMP, D2D1_EXTEND_MODE_CLAMP,
        D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC};
    if (!g_plainScaling &&
        SUCCEEDED(target->QueryInterface(IID_PPV_ARGS(context.put()))) &&
        SUCCEEDED(context->CreateBitmapBrush(bitmap, &properties, nullptr,
                                             smooth.put()))) {
        brush.copy_from(smooth.get());
    } else {
        target->CreateBitmapBrush(bitmap, brush.put());
    }
    return brush;
}
std::wstring GetWindowAppId(HWND hWnd);
std::wstring WindowAppKey(HWND hWnd);
void PostAttentionRequest(HWND app, std::wstring appId);
void ScheduleTaskbarHide(HWND hTaskbarWnd, UINT delayMs);
void SlideTo(HWND hWnd, const RECT& target, int durationMs, bool show);

// Whether the taskbar itself has the dock shown: it calls SlideWindow to
// show and hide it (see TrayUI_SlideWindow_Hook).
bool g_trayShown;

// How long the dock waits at most for a clicked app to open, in seconds.
constexpr double kLaunchWaitSeconds = 6;
// The groups an app clicked in the dock bounces for while it opens, as long
// as that wait (see OnDockPressed).
constexpr int kLaunchWaitBounceGroups =
    (int)(kLaunchWaitSeconds / kBounceGroupLength) + 1;

// Keeps the dock up for `seconds` from now, replacing any earlier wait. The
// taskbar asks PermitAutoHide before hiding (see
// CTaskListWnd_PermitAutoHide_Hook), and its hide timer is set to try again
// right after.
void KeepDockShown(double seconds) {
    if (!g_dockWnd) {
        return;
    }
    g_keepShownUntil = NowSeconds() + seconds;
    g_keepShownWnd = g_dockWnd;
    ScheduleTaskbarHide(g_dockWnd, (UINT)(seconds * 1000) + 100);
}

// How fast the magnification follows the mouse (higher is snappier).
constexpr double kMagnificationFollow = 18;
// Diameter of the open-app dot, and its distance below the icon, in pixels.
constexpr double kDotSize = 4;
constexpr double kDotGap = 3;

// The dock's measures around its icons, like the dock on the mod's site
// (46-pixel icons, 6 pixels apart, the shelf 8 pixels above them, 10 beside
// and 9 under the dots): the margins grow and shrink with the icons.
struct DockMetrics {
    double icon;
    double gap;
    double padTop;
    double padSide;
    double padBottom;
    double bottomGap;
    // The open apps' dots, under the icons.
    double dotRoom;
    double Height() const { return padTop + icon + dotRoom + padBottom; }
};

// How high an icon bounces, for its size at rest.
constexpr double kDockBounce = 0.55;

// Whether the taskbar hides itself (read on the taskbar's thread as it's
// styled): otherwise the dock at rest fits in its own height.
std::atomic<bool> g_taskbarAutoHides{true};

// The dock's measures for the chosen icon size and space between the icons,
// within `room` (the taskbar's own height, in DIPs) if it isn't 0: all
// smaller but the dots.
DockMetrics GetDockMetrics(double room) {
    const double icon = g_settings.dockIconSize;
    const double f = icon / 46;
    DockMetrics m{icon,         g_settings.dockIconGap, 8 * f, 10 * f, 9 * f,
                  g_settings.bottomGap, kDotGap + kDotSize};
    const double whole = m.bottomGap + m.Height();
    if (room > 0 && whole > room) {
        const double k =
            std::max(0.3, (room - m.dotRoom) / (whole - m.dotRoom));
        for (double* part : {&m.icon, &m.gap, &m.padTop, &m.padSide,
                             &m.padBottom, &m.bottomGap}) {
            *part *= k;
        }
    }
    // The end icons' top corners clear the shelf's round corners (small
    // icons make it almost a pill), with as much room as above them.
    const double radius = std::min(g_settings.cornerRadius, m.Height() / 2);
    if (m.padTop < radius) {
        const double curve = radius - m.padTop;
        const double inset =
            radius - std::sqrt(std::max(0.0, radius * radius - curve * curve));
        m.padSide = std::max(m.padSide, inset + m.padTop);
    }
    return m;
}

// The dock's room above the taskbar: the dock is taller than the taskbar (its
// icons magnified and bouncing, and the dock itself around big icons), so its
// window is made taller upwards (see GrowTaskbarWindow), shown or hidden
// (hidden, the room goes down past the screen's edge with the rest), so a
// slide never lays it out again. The taskbar keeps its own height for what it
// reserves on the screen, and the room takes no clicks while the dock rests
// (see ClipDockWindow). Taskbar thread only.
struct GrownTaskbar {
    HWND wnd;
    // The taskbar's own height and the room added, in DIPs.
    double baseDips;
    double extraDips;
    // The room was added upwards (shown) or downwards (hidden), and how
    // much higher it is hidden (see HiddenLift), in pixels.
    bool up;
    int lift = 0;
    // The top of the dock's shelf on the screen, as last drawn (0 before).
    LONG shelfTop = 0;
};
std::vector<GrownTaskbar> g_grownTaskbars;

GrownTaskbar* FindGrownTaskbar(HWND wnd) {
    for (auto& grown : g_grownTaskbars) {
        if (grown.wnd == wnd) {
            return &grown;
        }
    }
    return nullptr;
}

// The room the dock needs above the taskbar's own height, in DIPs (none but
// for the dock).
double DockRoomDips(double baseDips) {
    if (!IsDock() || !g_taskbarAtBottom || g_unloading) {
        return 0;
    }
    const DockMetrics m = GetDockMetrics(g_taskbarAutoHides ? 0 : baseDips);
    const double tallest = m.icon * (g_settings.magnification + kDockBounce);
    // (A few pixels more above the bounce.)
    return std::max(0.0, std::ceil(m.bottomGap + m.padBottom + m.dotRoom +
                                   tallest + 4 - baseDips));
}

// The mouse is on this dock (the shelf, or its magnified icons): the
// taskbar doesn't hide meanwhile (see CTaskListWnd_PermitAutoHide_Hook).
HWND g_dockHovered;
// The taskbar the mod's own slide is moving right now (see SlideTo), and
// whether it goes away: each frame is cut before and after its move there,
// so the taskbar's handlers leave that to the slide (they'd cut it again and
// again, and paint its whole window black for every frame of a hide, which
// shows nothing new). Taskbar thread only (the taskbars all live on it).
HWND g_slidingTaskbar;
bool g_slidingHides;
// A show Windows makes at once (no animation) with the user not reaching for
// the dock: its window goes up cut to nothing, and the dock shows only if it's
// still up this long after (see TrayUI_SlideWindow_Hook). Styling it again,
// or turning its auto-hide on (see UpdateAutoHide), Windows shows the taskbar
// so and hides it again right after, over and over for a few seconds
// (measured: up to 15 times in 4 seconds, each hide 2 to 93 ms after its
// show), and each one popped up and slid away: none of that shows now.
constexpr UINT kQuietShowDelayMs = 150;
// (Not one of the taskbar's own timers.)
constexpr UINT_PTR kQuietShowTimerId = 0x4D4C;
// The dock shown so that still waits, or null. Taskbar thread only.
HWND g_quietShownWnd;
// Whether cutting a taskbar's window (SetWindowRgn) redraws it: not while the
// slide moves it, which would redraw it with every frame (the slide's end
// draws it once).
bool RedrawsWithCut(HWND taskbar) {
    return taskbar != g_slidingTaskbar;
}
// The taskbar whose hide timer is being handled, so what the taskbar asks
// meanwhile (see CTaskListWnd_PermitAutoHide_Hook) is about that one and not
// the main one's. Taskbar thread only.
HWND g_hideTimerTaskbar;
// When a taskbar was last clicked (see HandFocusBack), and right-clicked
// (see JumpListOpen), and which one that was.
double g_dockPressedAt;
double g_dockRightPressedAt;
HWND g_dockRightPressedWnd;

// Cuts the dock's window to what it shows (defined further down).
void ClipDockWindow(HWND hWnd, const RECT* next = nullptr);

// The taskbar's thumbnails of an app's windows (shown with the mouse on its
// button): the taskbar puts them above its window, whose top is the dock's
// room (see GrownTaskbar), far above the icons, and the mouse couldn't get
// to them without leaving the dock; they're moved down to right above the
// shelf. Classic ones have a window of their own; Windows 11's draw in a XAML
// window as big as the monitor, above the taskbar's top, which Windows keeps
// shown (empty) and shows again without placing it: that window is moved down
// by as much as the shelf's top is below the taskbar's while the mouse is on
// the dock or right above it (on its thumbnails), and put back otherwise as
// it shows or moves, when the mouse leaves the dock and when the dock hides
// (other parts of Windows may show in it).
std::vector<HWND> g_thumbnailWindows;

// Whether `at` is the monitor's rectangle moved down.
bool MovedDownMonitor(const RECT& at, const RECT& monitor) {
    return at.top > monitor.top && at.left == monitor.left &&
           at.right == monitor.right &&
           at.bottom - at.top == monitor.bottom - monitor.top;
}

// Whether the mouse is on the dock `room` (whose window is at `taskbar`), or
// above it where its thumbnails show.
bool MouseOnDockOrThumbnails(const GrownTaskbar& room, const RECT& taskbar) {
    if (g_dockHovered == room.wnd) {
        return true;
    }
    POINT pt{};
    return GetCursorPos(&pt) && pt.x >= taskbar.left && pt.x < taskbar.right &&
           pt.y < room.shelfTop &&
           pt.y >= room.shelfTop - MulDiv(420, GetDpiForWindow(room.wnd), 96);
}

// Where the thumbnails' window `at` goes on the screen (unchanged if it isn't
// above a dock).
RECT PlaceThumbnails(const RECT& at) {
    RECT placed = at;
    // (Empty: nothing shows in it yet.)
    if (at.right <= at.left || at.bottom <= at.top) {
        return placed;
    }
    const HMONITOR monitor = MonitorFromRect(&at, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{sizeof(info)};
    if (!GetMonitorInfo(monitor, &info)) {
        return placed;
    }
    const bool monitorSized = EqualRect(&at, &info.rcMonitor) ||
                              MovedDownMonitor(at, info.rcMonitor);
    for (const auto& room : g_grownTaskbars) {
        RECT taskbar{};
        if (!room.shelfTop || !GetWindowRect(room.wnd, &taskbar) ||
            TaskbarMonitor(room.wnd) != monitor) {
            continue;
        }
        // Its window's top with it shown (it may be on its way up), where
        // Windows puts the thumbnails above (see GrownTaskbar::shelfTop).
        const LONG shownTop =
            info.rcMonitor.bottom - (taskbar.bottom - taskbar.top);
        if (monitorSized) {
            placed = info.rcMonitor;
            if (room.shelfTop > shownTop &&
                MouseOnDockOrThumbnails(room, taskbar)) {
                OffsetRect(&placed, 0, room.shelfTop - shownTop);
            }
        } else if (const LONG wanted = room.shelfTop -
                                       MulDiv(8, GetDpiForWindow(room.wnd), 96);
                   at.bottom <= shownTop + 2 && at.bottom < wanted) {
            // (Only the ones put above the taskbar's window.)
            OffsetRect(&placed, 0, wanted - at.bottom);
        }
        break;
    }
    return placed;
}

LRESULT CALLBACK ThumbnailSubclassProc(HWND hWnd,
                                       UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam,
                                       DWORD_PTR) {
    if (uMsg == WM_WINDOWPOSCHANGING && lParam && !g_unloading && IsDock()) {
        auto* pos = (WINDOWPOS*)lParam;
        // (Also shown again where it was, without a move.)
        if (!(pos->flags & SWP_NOMOVE) || (pos->flags & SWP_SHOWWINDOW)) {
            RECT now{};
            GetWindowRect(hWnd, &now);
            const POINT to = (pos->flags & SWP_NOMOVE)
                                 ? POINT{now.left, now.top}
                                 : POINT{pos->x, pos->y};
            const int height =
                (pos->flags & SWP_NOSIZE) ? now.bottom - now.top : pos->cy;
            const int width =
                (pos->flags & SWP_NOSIZE) ? now.right - now.left : pos->cx;
            const RECT placed =
                PlaceThumbnails({to.x, to.y, to.x + width, to.y + height});
            if (placed.top != to.y) {
                pos->x = to.x;
                pos->y = placed.top;
                pos->flags &= ~SWP_NOMOVE;
            }
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// Places the watched thumbnails' windows again (see PlaceThumbnails).
void PlaceThumbnailWindows() {
    for (HWND thumbnails : g_thumbnailWindows) {
        RECT at{};
        if (IsWindowVisible(thumbnails) && GetWindowRect(thumbnails, &at)) {
            const RECT placed = PlaceThumbnails(at);
            if (placed.top != at.top) {
                SetWindowPos(thumbnails, nullptr, placed.left, placed.top, 0, 0,
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
    }
}

// Finds the thumbnails' windows (made the first time they show: the classic
// one, and the XAML ones of newer Windows 11 builds) and watches them. Taskbar
// thread only.
void WatchThumbnails() {
    // (Not the "_WASDK" XAML window: the desktop's context menu shows in
    // it. Only the ones of this thread, the taskbar's, which shows the
    // thumbnails: Alt+Tab, Task View and the desktops' switcher show in
    // windows of the same class on a thread of their own, and
    // PlaceThumbnails reads the taskbar thread's g_grownTaskbars.)
    for (PCWSTR className :
         {L"TaskListThumbnailWnd", L"XamlExplorerHostIslandWindow"}) {
        HWND thumbnails = nullptr;
        while ((thumbnails = FindWindowEx(nullptr, thumbnails, className,
                                          nullptr))) {
            DWORD processId = 0;
            const DWORD threadId =
                GetWindowThreadProcessId(thumbnails, &processId);
            if (processId != GetCurrentProcessId() ||
                threadId != GetCurrentThreadId() ||
                std::find(g_thumbnailWindows.begin(), g_thumbnailWindows.end(),
                          thumbnails) != g_thumbnailWindows.end()) {
                continue;
            }
            if (!WindhawkUtils::SetWindowSubclassFromAnyThread(
                    thumbnails, ThumbnailSubclassProc, 0)) {
                continue;
            }
            g_thumbnailWindows.push_back(thumbnails);
        }
    }
}

void UnwatchThumbnails() {
    for (HWND thumbnails : g_thumbnailWindows) {
        // (A monitor-sized one moved down goes back in place.)
        RECT at{};
        MONITORINFO info{sizeof(info)};
        if (GetWindowRect(thumbnails, &at) &&
            GetMonitorInfo(MonitorFromRect(&at, MONITOR_DEFAULTTONEAREST),
                           &info) &&
            MovedDownMonitor(at, info.rcMonitor)) {
            SetWindowPos(thumbnails, nullptr, info.rcMonitor.left,
                         info.rcMonitor.top, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    for (HWND thumbnails : g_thumbnailWindows) {
        if (IsWindow(thumbnails)) {
            WindhawkUtils::RemoveWindowSubclassFromAnyThread(
                thumbnails, ThumbnailSubclassProc);
        }
    }
    g_thumbnailWindows.clear();
}

void OnRendering(winrt::Windows::Foundation::IInspectable const&,
                 winrt::Windows::Foundation::IInspectable const&);

void StartRendering() {
    if (!g_renderingRevoker && !g_unloading) {
        g_lastRenderTime = NowSeconds();
        Safely(L"Starting to draw", [] {
            g_renderingRevoker = Media::CompositionTarget::Rendering(
                winrt::auto_revoke, OnRendering);
        });
    }
}

DockIcon* FindDockIcon(FrameworkElement const& button) {
    for (auto& icon : g_dockIcons) {
        if (icon.button.get() == button) {
            return &icon;
        }
    }
    return nullptr;
}

// Collects every element of the given classes under `root`, without looking
// inside the ones found.
void FindAllByClassName(FrameworkElement const& root,
                        std::initializer_list<PCWSTR> classNames,
                        std::vector<FrameworkElement>& found,
                        int depth = 0) {
    if (depth > 14) {
        return;
    }
    const int count = Media::VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < count; i++) {
        auto child = Media::VisualTreeHelper::GetChild(root, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            continue;
        }
        const auto className = winrt::get_class_name(child);
        if (std::any_of(classNames.begin(), classNames.end(),
                        [&](PCWSTR name) { return className == name; })) {
            found.push_back(child);
        } else {
            FindAllByClassName(child, classNames, found, depth + 1);
        }
    }
}

// Remembers where an app's button is on the screen, for the icon that peeks
// out while the dock is hidden (see RunAttentionIcon).
void RecordAttentionButton(FrameworkElement const& button) {
    auto parent =
        Media::VisualTreeHelper::GetParent(button).try_as<UIElement>();
    if (!parent || !g_dockWnd || !g_dockIsMain) {
        return;
    }
    const auto offset = button.ActualOffset();
    const double center =
        parent.TransformToVisual(nullptr)
            .TransformPoint({offset.x, offset.y})
            .X +
        button.ActualWidth() / 2;
    POINT pt{MulDiv((int)std::lround(center), GetDpiForWindow(g_dockWnd), 96),
             0};
    ClientToScreen(g_dockWnd, &pt);
    g_attentionButtonX = pt.x;
    g_attentionButtonTime = NowSeconds();
}

void MakeDots(DockIcon& icon, FrameworkElement const& button);

// The running state comes from the button's "RunningIndicatorStates" visual
// states: NoRunningIndicator (closed), Active/InactiveRunningIndicator (open)
// and RequestingAttention... (asking for attention).
void OnRunningStateChanged(winrt::weak_ref<FrameworkElement> weakButton,
                           std::wstring_view oldState,
                           std::wstring_view newState) {
    auto button = weakButton.get();
    DockIcon* icon = button ? FindDockIcon(button) : nullptr;
    if (!icon) {
        return;
    }

    const bool wasClosed = oldState == L"NoRunningIndicator" ||
                           // A new button for an app that wasn't pinned.
                           (oldState.empty() && NowSeconds() - g_dockSince > 5);
    const bool attention =
        newState.find(L"RequestingAttention") != std::wstring_view::npos;
    const bool open = !newState.empty() && newState != L"NoRunningIndicator";

    if (attention && !icon->attention) {
        RecordAttentionButton(button);
        // (The dock bounces; the transparent taskbar doesn't, see
        // OnAppFlashed.)
        if (IsDock()) {
            icon->bounceStart = NowSeconds();
            icon->bounceGroups = kAttentionBounceGroups;
            StartRendering();
        }
    }
    icon->attention = attention;

    icon->open = open;
    // Its line, made by Windows only now for a pinned app: a dot too (see
    // UpdateDots).
    if (open) {
        MakeDots(*icon, button);
        StartRendering();
    }

    if (wasClosed && open && !attention) {
        if (g_settings.bounceOnLaunch) {
            if (icon->bounceStart &&
                icon->bounceGroups == kLaunchWaitBounceGroups) {
                // Bouncing since it was clicked (see OnDockPressed): it
                // opened, so the bounce in progress is the last one.
                icon->bounceGroups = std::min(
                    icon->bounceGroups,
                    (int)(AnimationDt(NowSeconds() - icon->bounceStart) /
                          kBounceGroupLength) +
                        1);
            } else {
                icon->bounceStart = NowSeconds();
                icon->bounceGroups = kLaunchBounceGroups;
            }
            StartRendering();
        }
        // Only if it was clicked in the dock (see OnDockPressed), so the
        // dock is up.
        if (g_settings.keepOpenOnLaunch && IsKeptShown(g_dockWnd)) {
            KeepDockShown(g_settings.keepOpenDuration / 1000.0);
        }
    }
}

// A click on a closed app's button: the dock waits for the app to open, and
// its icon bounces meanwhile (the bounce that shows once its window is there
// is easy to miss, over the new window and with the dock about to hide).
void OnDockPressed(Input::PointerRoutedEventArgs const& args) {
    if (!AnimatesIcons()) {
        return;
    }
    if (!g_settings.keepOpenOnLaunch && !g_settings.bounceOnLaunch) {
        return;
    }
    auto element = args.OriginalSource().try_as<DependencyObject>();
    while (element) {
        auto frameworkElement = element.try_as<FrameworkElement>();
        if (frameworkElement && winrt::get_class_name(frameworkElement) ==
                                    L"Taskbar.TaskListButton") {
            DockIcon* icon = FindDockIcon(frameworkElement);
            if (icon && !icon->open) {
                if (g_settings.keepOpenOnLaunch) {
                    KeepDockShown(kLaunchWaitSeconds);
                }
                if (g_settings.bounceOnLaunch &&
                    args.GetCurrentPoint(nullptr)
                        .Properties()
                        .IsLeftButtonPressed()) {
                    icon->bounceStart = NowSeconds();
                    icon->bounceGroups = kLaunchWaitBounceGroups;
                    StartRendering();
                }
            }
            return;
        }
        element = Media::VisualTreeHelper::GetParent(element);
    }
}

// How long the whole dock stays up for a notification, when that's the
// setting, in milliseconds.
constexpr UINT kNotificationShowMs = 3500;

// The app a taskbar button belongs to: its AppUserModelID, which the button
// carries as its automation ID (with an "Appid: " prefix on some builds).
std::wstring GetButtonAppId(FrameworkElement const& button) {
    std::wstring id{Automation::AutomationProperties::GetAutomationId(button)};
    constexpr std::wstring_view kPrefix = L"Appid: ";
    if (id.starts_with(kPrefix)) {
        id.erase(0, kPrefix.size());
    }
    return id;
}

// Shows the whole dock for a moment, for a notification. The taskbar only
// shows itself for the mouse at the edge, so the mod slides it in itself, and
// back out after a while unless the mouse is on it. If the taskbar shows
// itself meanwhile (the mouse went there), it takes over.
void ShowDockForNotification() {
    if (g_shownForNotification || g_trayShown || !g_dockWnd) {
        return;
    }
    RECT rect;
    MONITORINFO monitor{sizeof(monitor)};
    if (!GetWindowRect(g_dockWnd, &rect) ||
        !GetMonitorInfo(TaskbarMonitor(g_dockWnd), &monitor)) {
        return;
    }
    g_notificationHiddenRect = rect;
    RECT shown = rect;
    OffsetRect(&shown, 0, monitor.rcMonitor.bottom - rect.bottom);
    g_shownForNotification = true;
    SlideTo(g_dockWnd, shown, AnimationMs(g_settings.showDuration), true);

    if (g_notificationTimer) {
        g_notificationTimer.Stop();
    }
    g_notificationTimer = DispatcherTimer();
    g_notificationTimer.Interval(std::chrono::milliseconds(kNotificationShowMs));
    // (The tick comes outside any dock's scope: this dock's state then.)
    g_notificationTimer.Tick(
        [wnd = g_dockWnd](winrt::Windows::Foundation::IInspectable const& sender,
                          winrt::Windows::Foundation::IInspectable const&) {
            DockScope scope(wnd);
            auto timer = sender.as<DispatcherTimer>();
            // (A dock already let go: its window isn't the one in the
            // globals, so nothing of another dock is touched.)
            if (g_dockWnd != wnd) {
                timer.Stop();
                return;
            }
            if (!g_shownForNotification || g_trayShown || g_unloading) {
                timer.Stop();
                g_shownForNotification = false;
                return;
            }
            // Stays while the mouse is on it; asked again on the next tick.
            POINT pt;
            RECT rect;
            if (GetCursorPos(&pt) && GetWindowRect(g_dockWnd, &rect) &&
                PtInRect(&rect, pt)) {
                return;
            }
            timer.Stop();
            g_shownForNotification = false;
            SlideTo(g_dockWnd, g_notificationHiddenRect,
                    AnimationMs(g_settings.hideDuration), false);
        });
    g_notificationTimer.Start();
}

// An app has something new: a new count on its badge, or its button started
// flashing. The icon bounces in the dock if it's on screen. Otherwise, it
// depends on the setting: only the icon peeks out, the whole dock shows up,
// or nothing happens.
void OnAppNotified(FrameworkElement const& button) {
    DockIcon* icon = FindDockIcon(button);
    const double now = NowSeconds();
    if (!icon || !g_dockWnd || now - icon->lastNotified < 1.0) {
        return;
    }
    icon->lastNotified = now;

    const std::wstring appId = GetButtonAppId(button);
    // On the Windows taskbar (and the transparent one), the button shows it
    // itself; only the dock bounces.
    if (!AnimatesIcons() || !IsDock()) {
        return;
    }
    // Not for the app the user is looking at.
    if (!appId.empty() && GetWindowAppId(GetForegroundWindow()) == appId) {
        return;
    }
    RecordAttentionButton(button);

    MONITORINFO monitor{sizeof(monitor)};
    GetMonitorInfo(TaskbarMonitor(g_dockWnd), &monitor);
    const bool hidden = IsTaskbarHidden(g_dockWnd, monitor.rcMonitor);
    if (hidden && g_settings.attentionMode == AttentionMode::Icon) {
        PostAttentionRequest(nullptr, appId);
        return;
    }
    if (hidden && g_settings.attentionMode == AttentionMode::None) {
        return;
    }
    icon->bounceStart = now;
    icon->bounceGroups = kAttentionBounceGroups;
    StartRendering();
    if (hidden) {
        ShowDockForNotification();
    }
}

bool IsBadgeShown(const DockIcon& icon) {
    auto badge = icon.badge.get();
    return badge && badge.Visibility() == Visibility::Visible &&
           badge.Opacity() > 0;
}

// The apps with a notification badge right now, by AppUserModelID, for the
// island (see Island::UpdateApps). Written on the taskbar thread. In explorer,
// each line is the ID, a tab and the badge's count (0 without a number); the
// island keeps the IDs here, and their counts in g_badgeCounts.
SRWLOCK g_badgeAppsLock = SRWLOCK_INIT;
std::vector<std::wstring> g_badgeApps;
std::vector<int> g_badgeCounts;
// The apps' overlay icons (ITaskbarList3::SetOverlayIcon, like Discord's red
// counter, which isn't a badge), as the taskbar gets them (see
// CTaskBand__HandleOverlayIcon_Hook): the app's own picture (a copy; the
// taskbar draws it at 16 pixels, blurry magnified) and its text (where an
// app says how many, like "3 unread"). They count as the app's badge (see
// PublishBadgeApps) and are drawn sharp in the dock (see UseBigOverlay).
struct OverlayIcon {
    HWND wnd;
    std::wstring appId;
    HICON icon = nullptr;
    // Its picture's width, in pixels.
    int size = 0;
    // Changed with every new picture (see UseBigOverlay).
    unsigned serial = 0;
    std::wstring description;
};
SRWLOCK g_overlayIconsLock = SRWLOCK_INIT;
std::vector<OverlayIcon> g_overlayIcons;
unsigned g_overlaySerial = 0;

// Whether an overlay's text has a count in it (like "0 notifications"):
// apps also show their status with one (presence, "recording"), without.
bool OverlayHasCount(const std::wstring& description) {
    return description.find_first_of(L"0123456789") != std::wstring::npos;
}

// The count an overlay's text gives (its first number, "99+" as 99), or 0.
int OverlayCount(const std::wstring& description) {
    const size_t digit = description.find_first_of(L"0123456789");
    return digit == std::wstring::npos ? 0
                                       : _wtoi(description.c_str() + digit);
}

// The count an app's overlay icon shows. Discord's text ("N notifications",
// in its own language) says the count before the newest one (its first
// notification comes with "0 notifications", its picture showing 1): one
// more.
int OverlayShownCount(const OverlayIcon& overlay) {
    const int count = OverlayCount(overlay.description);
    const bool discord = overlay.icon &&
                         StrStrI(overlay.appId.c_str(), L"Discord") &&
                         OverlayHasCount(overlay.description);
    return discord ? count + 1 : count;
}

// The island's window, told when they change.
std::atomic<HWND> g_islandWnd;
// The apps' progress as explorer last told it (see PublishProgress; the
// island's thread).
int g_downloadState = 0;
int g_downloadPermille = 0;
// (kProgressKindCopy: a file copy.)
int g_downloadKind = 0;
// Only browsers have it (kProgressFromBrowsers), and since when its value
// hasn't moved: standing still this long, it shows as paused
// (islandDownloadStalled; Chromium's bar moves 1% at a time, so a big slow
// download stands still for a while).
bool g_downloadBrowsers = false;
double g_downloadMovedAt = 0;
constexpr double kDownloadStalledSeconds = 30;
// Right after Windows starts, apps restoring themselves tell the taskbar a
// progress they don't have: none fills the pill until then (NowSeconds()).
constexpr double kBootProgressSeconds = 30;
constexpr ULONGLONG kBootUptimeMs = 5 * 60 * 1000;
double g_progressFrom = 0;
// The message that tells it.
constexpr UINT WM_APP_BADGES = WM_APP + 51;

// To the island, in its own process.
void SendBadgeApps() {
    HWND island = FindIslandWnd();
    if (!island) {
        return;
    }
    AcquireSRWLockShared(&g_badgeAppsLock);
    const std::vector<std::wstring> apps = g_badgeApps;
    ReleaseSRWLockShared(&g_badgeAppsLock);
    SendLines(island, OwnTaskbarWnd(), kIpcBadgeApps, apps);
}

void PublishBadgeApps() {
    if (!g_dockIsMain) {
        return;
    }
    std::vector<std::wstring> ids;
    std::vector<std::wstring> apps;
    for (const auto& icon : g_dockIcons) {
        auto button = icon.button.get();
        if (!button || !icon.badgeShown) {
            continue;
        }
        // Apps' buttons, and the Widgets button (not Start, search or task
        // view, which aren't apps either).
        std::wstring appId = GetButtonAppId(button);
        if (winrt::get_class_name(button) != L"Taskbar.TaskListButton" &&
            appId != kWidgetsBadgeId) {
            continue;
        }
        if (!appId.empty() &&
            std::find(ids.begin(), ids.end(), appId) == ids.end()) {
            apps.push_back(appId + L"\t" +
                           std::to_wstring(
                               std::max(0, _wtoi(icon.badgeText.c_str()))));
            ids.push_back(std::move(appId));
        }
    }
    // The apps' overlay icons too (their windows still open), with the count
    // their text gives: only the ones with a count (not a status).
    AcquireSRWLockExclusive(&g_overlayIconsLock);
    std::erase_if(g_overlayIcons, [](const OverlayIcon& overlay) {
        if (IsWindow(overlay.wnd)) {
            return false;
        }
        DestroyIcon(overlay.icon);
        return true;
    });
    for (const auto& overlay : g_overlayIcons) {
        if (overlay.icon && !overlay.appId.empty() &&
            OverlayHasCount(overlay.description) &&
            std::find(ids.begin(), ids.end(), overlay.appId) == ids.end()) {
            apps.push_back(overlay.appId + L"\t" +
                           std::to_wstring(OverlayShownCount(overlay)));
            ids.push_back(overlay.appId);
        }
    }
    ReleaseSRWLockExclusive(&g_overlayIconsLock);
    AcquireSRWLockExclusive(&g_badgeAppsLock);
    const bool changed = apps != g_badgeApps;
    if (changed) {
        g_badgeApps = std::move(apps);
    }
    ReleaseSRWLockExclusive(&g_badgeAppsLock);
    if (changed) {
        SendBadgeApps();
    }
}

// The badge (Taskbar.Badge#BadgeControl, with TextBlock#BadgeText inside) is
// only created with an app's first notification.
FrameworkElement FindBadge(FrameworkElement const& button) {
    return FindDescendant(button, [](FrameworkElement const& e) {
        return std::wstring_view{winrt::get_class_name(e)}.find(L"Badge") !=
                   std::wstring_view::npos ||
               std::wstring_view{e.Name()}.find(L"Badge") !=
                   std::wstring_view::npos;
    });
}

Controls::TextBlock FindBadgeText(FrameworkElement const& badge) {
    if (auto found = FindDescendant(badge, [](FrameworkElement const& e) {
            return (bool)e.try_as<Controls::TextBlock>();
        })) {
        return found.as<Controls::TextBlock>();
    }
    return nullptr;
}

void CheckBadgeOf(winrt::weak_ref<FrameworkElement> const& weakButton);

// Asks to be told when the badge's text, visibility or opacity change.
void WatchBadge(DockIcon& icon,
                FrameworkElement const& button,
                FrameworkElement const& badge,
                Controls::TextBlock const& text) {
    icon.badge = winrt::make_weak(badge);
    const auto weakButton = winrt::make_weak(button);
    auto watch = [&](DependencyObject const& element,
                     DependencyProperty const& property) {
        g_badgeWatches.push_back(
            {winrt::make_weak(element), property,
             element.RegisterPropertyChangedCallback(
                 property, [weakButton](DependencyObject const&,
                                        DependencyProperty const&) {
                     Safely(L"Checking a badge",
                            [&] { CheckBadgeOf(weakButton); });
                 })});
    };
    watch(text, Controls::TextBlock::TextProperty());
    watch(badge, UIElement::VisibilityProperty());
    watch(badge, UIElement::OpacityProperty());
}

// Compares the badge with how it was last seen. News: it appeared with a
// count, or the count changed (but not to a smaller number: fewer unread
// items isn't news; "99+" reads as 99). Nothing is news for a moment after the
// mod starts (the badges already there), or when the button now shows another
// app.
void CheckBadge(DockIcon& icon) {
    auto button = icon.button.get();
    if (!button) {
        return;
    }
    auto badge = FindBadge(button);
    Controls::TextBlock text = badge ? FindBadgeText(badge) : nullptr;
    if (badge && text && badge != icon.badge.get()) {
        WatchBadge(icon, button, badge, text);
    }

    const std::wstring appId = GetButtonAppId(button);
    const std::wstring newText = text ? std::wstring{text.Text()} : L"";
    const bool shown = text && !newText.empty() &&
                       badge.Visibility() == Visibility::Visible &&
                       badge.Opacity() > 0;

    bool news = false;
    if (appId == icon.badgeAppId && shown) {
        if (!icon.badgeShown) {
            news = true;
        } else if (newText != icon.badgeText) {
            const int count = _wtoi(newText.c_str());
            const int oldCount = _wtoi(icon.badgeText.c_str());
            news = !(count > 0 && oldCount > 0 && count < oldCount);
        }
    }
    icon.badgeAppId = appId;
    icon.badgeText = newText;
    icon.badgeShown = shown;
    if (news && NowSeconds() - g_dockSince > 5) {
        OnAppNotified(button);
    }
}

void CheckBadgeOf(winrt::weak_ref<FrameworkElement> const& weakButton) {
    auto button = weakButton.get();
    if (DockIcon* icon = button ? FindDockIcon(button) : nullptr) {
        CheckBadge(*icon);
        PublishBadgeApps();
    }
}

// The badges are created, replaced, faded in and out by the taskbar, so they're
// checked again as it changes, and every second.
void WatchBadges() {
    for (auto& icon : g_dockIcons) {
        CheckBadge(icon);
    }
}

// Widens `top` and `right` with the parts under `element` smaller than
// `limit` (the icon's size) that show, in `button`'s coordinates.
void FindIconExtras(FrameworkElement const& button,
                    FrameworkElement const& element,
                    double limit,
                    double& top,
                    double& right,
                    int depth = 0) {
    const int count = Media::VisualTreeHelper::GetChildrenCount(element);
    for (int i = 0; i < count; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (!child || child.Visibility() != Visibility::Visible ||
            child.Opacity() <= 0) {
            continue;
        }
        const double width = child.ActualWidth();
        const double height = child.ActualHeight();
        if (width > 0 && height > 0 && width < limit && height < limit) {
            const auto bounds = child.TransformToVisual(button).TransformBounds(
                {0, 0, (float)width, (float)height});
            top = top ? std::min(top, (double)bounds.Y) : bounds.Y;
            right = std::max(right, (double)(bounds.X + bounds.Width));
        } else if (depth < 8) {
            FindIconExtras(button, child, limit, top, right, depth + 1);
        }
    }
}

// Measures each dock's icons' extras (see DockIcon::extrasTop), every second
// (they come and go): the dock is cut around them (see OnRenderingUnsafe).
// Every dock's (its badges are only followed on the main one) and also hidden
// (ready as it shows), every icon (an overlay icon set before the mod started
// isn't known, see g_overlayIcons).
void MeasureDockExtras() {
    const auto measure = [] {
        bool changed = false;
        for (auto& icon : g_dockIcons) {
            auto button = icon.button.get();
            auto image = icon.image.get();
            if (!button || !image || image.ActualHeight() <= 0) {
                continue;
            }
            double top = 0;
            double right = 0;
            FindIconExtras(button, button, image.ActualHeight() * 0.95, top,
                           right);
            if (top != icon.extrasTop || right != icon.extrasRight) {
                icon.extrasTop = top;
                icon.extrasRight = right;
                changed = true;
            }
        }
        if (changed) {
            StartRendering();
        }
    };
    measure();
    for (auto& state : g_otherDocks) {
        DockScope scope(state->wnd);
        measure();
    }
}

void WatchBadgesAndPublish() {
    if (!g_dockIsMain) {
        return;
    }
    WatchBadges();
    PublishBadgeApps();
}

// (Defined further down.)
void WatchProgressBar(FrameworkElement const& button);
void StopWatchingProgressBars();
void RecheckProgress();
void SetProgressWanted(bool wanted);

void WatchRunningState(FrameworkElement const& button) {
    WatchProgressBar(button);
    auto panel = FindByName(button, L"IconPanel");
    if (!panel) {
        return;
    }
    for (const auto& group : VisualStateManager::GetVisualStateGroups(panel)) {
        if (group.Name() != L"RunningIndicatorStates") {
            continue;
        }
        // (Its state now too: one already asking for attention, like an app
        // flashing before the taskbar's style changed, still lets the
        // transparent taskbar hide, see CTaskListWnd_PermitAutoHide_Hook.)
        if (DockIcon* icon = FindDockIcon(button)) {
            const auto state = group.CurrentState();
            const std::wstring name = state ? state.Name().c_str() : L"";
            icon->open = state && name != L"NoRunningIndicator";
            icon->attention =
                name.find(L"RequestingAttention") != std::wstring::npos;
        }
        g_stateRevokers.push_back(group.CurrentStateChanged(
            winrt::auto_revoke,
            [weakButton = winrt::make_weak(button), taskbar = g_dockWnd](
                winrt::Windows::Foundation::IInspectable const&,
                VisualStateChangedEventArgs const& args) {
                DockScope scope(taskbar);
                Safely(L"Following an app's state", [&] {
                    const auto oldState = args.OldState()
                                              ? args.OldState().Name()
                                              : winrt::hstring{};
                    const auto newState = args.NewState()
                                              ? args.NewState().Name()
                                              : winrt::hstring{};
                    OnRunningStateChanged(weakButton, oldState, newState);
                });
            }));
    }
}

void FindAllByName(FrameworkElement const& root,
                   PCWSTR name,
                   std::vector<FrameworkElement>& found,
                   int depth = 0) {
    if (depth > 14) {
        return;
    }
    const int count = Media::VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < count; i++) {
        auto child = Media::VisualTreeHelper::GetChild(root, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            continue;
        }
        if (child.Name() == name) {
            found.push_back(child);
        }
        FindAllByName(child, name, found, depth + 1);
    }
}

