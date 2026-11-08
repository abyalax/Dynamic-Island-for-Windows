// Where a button is drawn this frame, in the taskbar's coordinates, for
// UpdateDots.
struct IconPlacement {
    double buttonTop;   // Layout position, before the mod's transforms.
    double originY;     // Scaling origin, in the button's coordinates.
    double offsetY;     // The button's vertical move (alignment + bounce).
    double iconBottom;  // The resting icon's bottom edge.
    double dockBottom;  // The dock background's bottom edge.
};

// Keeps the icon's dots round, small, just below the icon, and still while
// the icon grows and bounces, like on macOS. The dots are inside the button
// that grows and bounces, so they're moved and scaled back.
void UpdateDots(DockIcon& icon, const IconPlacement& place) {
    auto button = icon.button.get();
    if (!button || g_unloading) {
        return;
    }
    // An open app without its dot yet: its line came after the button was
    // seen (a pinned app opened from the dock), made a dot now.
    if (icon.open && std::none_of(icon.dots.begin(), icon.dots.end(),
                                  [](const DockIcon::Dot& dot) {
                                      return dot.rectangle.get() != nullptr;
                                  })) {
        MakeDots(icon, button);
    }
    const double magnification =
        std::max(icon.magnification * icon.baseScale, 0.01);
    const double dotCenter =
        std::min(place.iconBottom + kDotGap + kDotSize / 2,
                 place.dockBottom - kDotSize / 2 - 1);

    for (auto& dot : icon.dots) {
        auto rectangle = dot.rectangle.get();
        if (!rectangle) {
            continue;
        }
        const double width = rectangle.ActualWidth();
        const double height = rectangle.ActualHeight();
        auto parent =
            Media::VisualTreeHelper::GetParent(rectangle).try_as<UIElement>();
        if (width <= 0 || height <= 0 || !parent) {
            continue;
        }

        // Rounded fully before scaling, so it's a circle after. Only set when
        // it changes: a new radius makes the taskbar lay itself out again,
        // which would render another frame, and so on.
        if (rectangle.RadiusX() != width / 2 ||
            rectangle.RadiusY() != height / 2) {
            SetStyled(rectangle, Shapes::Rectangle::RadiusXProperty(),
                      winrt::box_value(width / 2));
            SetStyled(rectangle, Shapes::Rectangle::RadiusYProperty(),
                      winrt::box_value(height / 2));
        }
        dot.scale.ScaleX(kDotSize / (width * magnification));
        dot.scale.ScaleY(kDotSize / (height * magnification));

        // The line's center, in the button's coordinates.
        const auto offset = rectangle.ActualOffset();
        const double center = parent.TransformToVisual(button)
                                  .TransformPoint({offset.x, offset.y})
                                  .Y +
                              height / 2;
        // A point `y` of the button is drawn at
        //   buttonTop + originY + (y - originY) * magnification + offsetY.
        // This is the move that puts the line's center at `dotCenter`.
        dot.move.Y((dotCenter - place.buttonTop - place.originY -
                    place.offsetY) /
                       magnification +
                   place.originY - center);
    }
}

void MakeDots(DockIcon& icon, FrameworkElement const& button) {
    std::vector<FrameworkElement> indicators;
    FindAllByName(button, L"RunningIndicator", indicators);

    // (Again later for new lines: the ones gone let go, the ones already
    // dots left as they are.)
    std::erase_if(icon.dots, [](const DockIcon::Dot& dot) {
        return !dot.rectangle.get();
    });
    for (const auto& element : indicators) {
        auto rectangle = element.try_as<Shapes::Rectangle>();
        if (!rectangle || std::any_of(icon.dots.begin(), icon.dots.end(),
                                      [&](const DockIcon::Dot& dot) {
                                          return dot.rectangle.get() ==
                                                 rectangle;
                                      })) {
            continue;
        }
        DockIcon::Dot dot;
        dot.rectangle = winrt::make_weak(rectangle);
        Media::TransformGroup transforms;
        transforms.Children().Append(dot.scale = Media::ScaleTransform());
        transforms.Children().Append(dot.move = Media::TranslateTransform());
        SetStyled(rectangle, UIElement::RenderTransformProperty(), transforms);
        SetStyled(rectangle, UIElement::RenderTransformOriginProperty(),
                  winrt::box_value(winrt::Windows::Foundation::Point{0.5f, 0.5f}));
        icon.dots.push_back(dot);

        // The line changes size with the app's state.
        g_dotRevokers.push_back(rectangle.SizeChanged(
            winrt::auto_revoke,
            [weakButton = winrt::make_weak(button)](
                winrt::Windows::Foundation::IInspectable const&,
                SizeChangedEventArgs const&) {
                if (weakButton.get()) {
                    StartRendering();
                }
            }));
    }
}

// Finds the dock's buttons and prepares the ones it hasn't seen yet. Called
// again whenever the number of buttons changes.
void RescanDockIcons() {
    auto frame = g_dockFrame.get();
    if (!frame) {
        return;
    }

    std::erase_if(g_dockIcons, [](const DockIcon& icon) {
        return !icon.button.get();
    });

    std::vector<FrameworkElement> buttons;
    FindAllByClassName(frame,
                       {L"Taskbar.TaskListButton",
                        L"Taskbar.ExperienceToggleButton",
                        L"Taskbar.SearchBoxButton",
                        L"Taskbar.AugmentedEntryPointButton"},
                       buttons);

    // A button that comes once the dock is set up is an app opening: it
    // grows in (not the ones there from the start).
    const bool opening = g_dockItemCount >= 0 && NowSeconds() - g_dockSince > 1;
    bool added = false;
    for (const auto& button : buttons) {
        if (FindDockIcon(button)) {
            continue;
        }

        // The whole button grows and bounces. The icon image inside it isn't
        // touched, because the taskbar animates it itself when it's pressed,
        // and replacing its transform breaks that.
        DockIcon icon;
        icon.button = winrt::make_weak(button);
        if (auto image = FindByName(button, L"Icon")) {
            icon.image = winrt::make_weak(image);
        }
        Media::TransformGroup transforms;
        transforms.Children().Append(icon.scale = Media::ScaleTransform());
        transforms.Children().Append(icon.lift = Media::TranslateTransform());
        if (AnimatesIcons()) {
            SetStyled(button, UIElement::RenderTransformProperty(), transforms);
        }
        // The scaling origin is set in pixels on the scale itself (see
        // OnRendering), so the relative origin stays at the corner.
        if (AnimatesIcons()) {
            SetStyled(button, UIElement::RenderTransformOriginProperty(),
                      winrt::box_value(winrt::Windows::Foundation::Point{0, 0}));
        }
        // (Its room in the row, set each frame.)
        if (AnimatesIcons()) {
            icon.clip = Media::RectangleGeometry();
            SetStyled(button, UIElement::ClipProperty(), icon.clip);
        }
        icon.app = winrt::get_class_name(button) == L"Taskbar.TaskListButton";
        if (opening && icon.app) {
            icon.appear = 0;
            added = true;
        }
        g_dockIcons.push_back(icon);
        DockIcon& newIcon = g_dockIcons.back();

        const bool watched =
            std::any_of(g_watchedButtons.begin(), g_watchedButtons.end(),
                        [&](const auto& weak) { return weak.get() == button; });
        if (!watched &&
            winrt::get_class_name(button) == L"Taskbar.TaskListButton") {
            g_watchedButtons.push_back(winrt::make_weak(button));
            WatchRunningState(button);
        }
        if (g_settings.dotIndicator && IsDock()) {
            MakeDots(newIcon, button);
        }

        // No highlight behind the icon on hover or for the active app, nor
        // the plate stacked behind it for an app with more windows, like on
        // macOS. Hidden rather than recolored, because the button's visual
        // states set their color.
        std::vector<FrameworkElement> highlights;
        if (AnimatesIcons()) {
            FindAllByName(button, L"BackgroundElement", highlights);
            FindAllByName(button, L"MultiWindowElement", highlights);
        }
        for (const auto& highlight : highlights) {
            SetStyled(highlight, UIElement::VisibilityProperty(),
                      winrt::box_value(Visibility::Collapsed));
        }
    }
    if (added && AnimatesIcons()) {
        StartRendering();
    }
}

// Height of the icon's bounce right now, in pixels (negative is up), for a
// bounce at most `maxHeight` high.
double BounceOffset(DockIcon& icon, double now, double maxHeight) {
    if (!icon.bounceStart) {
        return 0;
    }
    // Bounces are decorations (none from the "partial" level on), at the
    // chosen speed.
    if (AnimationLevel() >= 1) {
        icon.bounceStart = 0;
        return 0;
    }
    bool done;
    const double height = BounceSequence(
        AnimationDt(now - icon.bounceStart), icon.bounceGroups, &done);
    if (done) {
        icon.bounceStart = 0;
        return 0;
    }
    return -std::max(maxHeight, 0.0) * height;
}

// Cuts the transparent taskbar's window down to its apps (and a margin), and
// puts its shadow behind them. `left` and `right` are the apps' extent, in
// the taskbar's XAML coordinates. Only done when it changes (an app opened or
// closed): the shadow's size is part of the layout.
void PlaceShadow(double left, double right) {
    if (left == g_shadowLeft && right == g_shadowRight) {
        return;
    }
    Safely(L"Placing the shadow", [&] {
        auto fill = g_shadowFill.get().try_as<FrameworkElement>();
        if (!fill) {
            return;
        }
        double parentLeft = 0;
        if (auto parent =
                Media::VisualTreeHelper::GetParent(fill).try_as<UIElement>()) {
            parentLeft = parent.TransformToVisual(nullptr)
                             .TransformPoint({0, 0})
                             .X;
        }
        SetStyled(fill, FrameworkElement::HorizontalAlignmentProperty(),
                  winrt::box_value(HorizontalAlignment::Left));
        SetStyled(fill, FrameworkElement::MarginProperty(),
                  winrt::box_value(ThicknessHelper::FromLengths(
                      std::max(0.0, left - kShadowMargin - parentLeft), 0, 0,
                      0)));
        SetStyled(fill, FrameworkElement::WidthProperty(),
                  winrt::box_value(right - left + kShadowMargin * 2));
        g_shadowLeft = left;
        g_shadowRight = right;
    });
}

// `withShadow`: also puts the shadow behind the apps (XAML, so only from the
// taskbar's own drawing, not while Windows moves it). `own`: the part of the
// window it may show (see ClipSlideStep), or the one on its monitor.
void UpdateAppsArea(double left,
                    double right,
                    bool withShadow,
                    const RECT* own = nullptr) {
    if (!g_dockWnd) {
        return;
    }
    if (withShadow) {
        PlaceShadow(left, right);
    }
    const double scale = GetDpiForWindow(g_dockWnd) / 96.0;
    RECT window{};
    if (!GetWindowRect(g_dockWnd, &window)) {
        return;
    }
    RECT region{(LONG)std::floor((left - kAppsAreaMargin) * scale), 0,
                (LONG)std::ceil((right + kAppsAreaMargin) * scale),
                window.bottom - window.top};
    region.left = std::max(region.left, 0L);
    region.right = std::min(region.right, window.right - window.left);
    // Laid out again at a new scale (see g_dockScaleSettled): its apps'
    // places are stale for a moment, and Windows would take the mouse past
    // them for leaving and hide it again at once. Its whole width meanwhile
    // (nothing of it shows yet).
    if (!g_dockScaleSettled) {
        region.left = 0;
        region.right = window.right - window.left;
    }
    // Only what's on its own monitor (hidden, the rest is past its edge,
    // maybe over the next monitor: Windows' own clip, which this region
    // replaces, leaves only that part too).
    const RECT shown = own ? *own : TaskbarOwnPart(g_dockWnd);
    region.top = std::max(region.top, shown.top);
    region.bottom = std::min(region.bottom, shown.bottom);
    if (region.right <= region.left || region.bottom <= region.top) {
        return;
    }
    g_appsLeft = left;
    g_appsRight = right;
    // Already in place (Windows sometimes sets the taskbar's region itself,
    // which drops this one: then it's made again).
    RECT current{};
    const int kind = GetWindowRgnBox(g_dockWnd, &current);
    if (EqualRect(&region, &g_appsRegion) && kind != ERROR &&
        kind != NULLREGION && EqualRect(&current, &region)) {
        return;
    }
    // (Also when Windows dropped it, as its own moves do: set again.)
    HRGN handle = CreateRectRgnIndirect(&region);
    if (!handle) {
        return;
    }
    // The system owns the region once it's set.
    if (!SetWindowRgn(g_dockWnd, handle, RedrawsWithCut(g_dockWnd))) {
        DeleteObject(handle);
        return;
    }
    g_appsRegion = region;
    g_appsAreaMonitor = TaskbarMonitor(g_dockWnd);
    SetAppsArea(g_appsAreaMonitor, window.left + region.left,
                window.left + region.right);
}

// The shadow: an ellipse centered on the bottom edge, dark in its middle and
// fading out to nothing at its edges, so it has no corners.
void MakeShadow(UIElement const& element) {
    namespace Composition = winrt::Windows::UI::Composition;
    try {
        auto compositor =
            Hosting::ElementCompositionPreview::GetElementVisual(element)
                .Compositor();
        auto gradient = compositor.CreateRadialGradientBrush();
        gradient.EllipseCenter({0.5f, 1.0f});
        gradient.EllipseRadius({0.5f, 1.0f});
        const std::pair<float, uint8_t> kStops[] = {
            {0.0f, 0xC4}, {0.35f, 0x98}, {0.65f, 0x4C}, {0.85f, 0x1A},
            {1.0f, 0x00}};
        for (const auto& [offset, alpha] : kStops) {
            gradient.ColorStops().Append(compositor.CreateColorGradientStop(
                offset, winrt::Windows::UI::Color{alpha, 0, 0, 0}));
        }
        auto visual = compositor.CreateSpriteVisual();
        visual.Brush(gradient);
        // As big as the element, whatever its size.
        visual.RelativeSizeAdjustment({1.0f, 1.0f});
        Hosting::ElementCompositionPreview::SetElementChildVisual(element,
                                                                 visual);
        g_shadowVisual = visual;
    } catch (const winrt::hresult_error& e) {
        IslandLog(L"The shadow couldn't be made: %08X", (unsigned)e.code());
    }
}

void ResetAppsArea() {
    if (g_dockWnd && (g_appsRegion.right > g_appsRegion.left)) {
        SetWindowRgn(g_dockWnd, nullptr, TRUE);
    }
    g_appsRegion = {};
    g_appsLeft = g_appsRight = 0;
    g_shadowLeft = g_shadowRight = 0;
    SetAppsArea(g_appsAreaMonitor, 0, 0);
    g_appsAreaMonitor = nullptr;
}

bool OnRenderingUnsafe(double now, double dt);
void RaiseOverWindows(HWND hTaskbarWnd);

// The frame's own opacity back (see g_dockScaleSettled).
void RestoreDockOpacity(FrameworkElement const& frame) {
    if (!g_dockOpacityKept ||
        g_dockOpacityKept == DependencyProperty::UnsetValue()) {
        frame.ClearValue(UIElement::OpacityProperty());
    } else {
        frame.SetValue(UIElement::OpacityProperty(), g_dockOpacityKept);
    }
    g_dockOpacityKept = nullptr;
}

// A frame for every taskbar (the main one and the other monitors'): stops
// once none is moving anymore.
void OnRendering(winrt::Windows::Foundation::IInspectable const&,
                 winrt::Windows::Foundation::IInspectable const&) {
    const double now = NowSeconds();
    // Under a frame rate limit, some of the taskbar's frames are skipped.
    if (const int limit = g_frameRateLimit.load(std::memory_order_relaxed);
        limit && now - g_lastRenderTime < 1.0 / limit - 0.002) {
        return;
    }
    const double dt =
        AnimationDt(std::clamp(now - g_lastRenderTime, 0.0, 0.05));
    g_lastRenderTime = now;
    bool failed = true;
    bool busy = false;
    Safely(L"Drawing the icons", [&] {
        busy = OnRenderingUnsafe(now, dt);
        // (Taskbars gone with their monitor are let go, with their apps
        // area and their clip.)
        std::erase_if(g_otherDocks, [](const auto& state) {
            if (IsWindow(state->wnd)) {
                return false;
            }
            SetAppsArea(state->appsAreaMonitor, 0, 0);
            std::erase(g_ownClips, state->wnd);
            if (state->notificationTimer) {
                state->notificationTimer.Stop();
            }
            return true;
        });
        for (auto& state : g_otherDocks) {
            DockScope scope(state->wnd);
            busy = OnRenderingUnsafe(now, dt) || busy;
        }
        failed = false;
    });
    // Tried again on the next change, not every frame.
    if (failed || !busy) {
        g_renderingRevoker.revoke();
    }
}

// One taskbar's frame; whether it's still moving.
bool OnRenderingUnsafe(double now, double dt) {
    auto frame = g_dockFrame.get();
    if (!frame || g_unloading || !AnimatesIcons()) {
        if (frame && g_dockContentHidden) {
            RestoreDockOpacity(frame);
            g_dockContentHidden = false;
        }
        g_dockScaleSettled = true;
        return false;
    }

    // The mouse, in the taskbar's XAML coordinates, and the XAML's size: its
    // own layout's, pixels per DIP from its width against the window's (its
    // scale can lag behind the window's as it moves over a monitor with
    // another scale, and its RasterizationScale can be the monitor's below,
    // where the hidden taskbar mostly was).
    auto xamlRoot = frame.XamlRoot();
    RECT client{};
    GetClientRect(g_dockWnd, &client);
    auto content =
        xamlRoot ? xamlRoot.Content().try_as<FrameworkElement>() : nullptr;
    double scale = GetDpiForWindow(g_dockWnd) / 96.0;
    if (content && content.ActualWidth() > 0 && client.right > 0) {
        scale = client.right / content.ActualWidth();
    } else if (xamlRoot) {
        scale = xamlRoot.RasterizationScale();
    }
    POINT pt;
    double mouseX = 0, mouseY = -1;
    if (GetCursorPos(&pt) && ScreenToClient(g_dockWnd, &pt)) {
        mouseX = pt.x / scale;
        mouseY = pt.y / scale;
    }
    const double clientWidth = content && content.ActualWidth() > 0
                                   ? content.ActualWidth()
                                   : client.right / scale;
    const double clientHeight = content && content.ActualHeight() > 0
                                    ? content.ActualHeight()
                                    : client.bottom / scale;

    // The dock: its measures (within the taskbar's own height when it doesn't
    // hide itself, see GrownTaskbar), and the line its icons stand on, above
    // their dots. The transparent taskbar's icons rest on its middle line.
    const bool dock = IsDock();
    const GrownTaskbar* grown = FindGrownTaskbar(g_dockWnd);
    const DockMetrics metrics = GetDockMetrics(
        g_taskbarAutoHides ? 0
                           : clientHeight - (grown ? grown->extraDips : 0));
    const double shelfBottom = clientHeight - metrics.bottomGap;
    const double restBottom = shelfBottom - metrics.padBottom - metrics.dotRoom;
    const double middle = clientHeight / 2;

    struct Placed {
        DockIcon* icon;
        FrameworkElement button{nullptr};
        double center;
        double buttonTop;
        double buttonWidth;
        // The icon image, in the button's coordinates.
        double iconTop;
        double iconHeight;
        // The highest part drawn with the icon: its top, or the top of the
        // notification badge on it; and the badge's right edge (0 without
        // one). In the button's coordinates.
        double contentTop;
        double badgeRight;
        // Its size at rest (the dock's icon size over the taskbar's), how
        // wide it's drawn then (its icon, or its whole button with its
        // name), its room in the row (the gap after it included), and where
        // its middle is then.
        double base = 1;
        double width = 0;
        double slot = 0;
        double restCenter = 0;
        // How much wider than at rest it's drawn now.
        double grow = 0;
    };
    std::vector<Placed> placed;
    placed.reserve(g_dockIcons.size());
    double left = std::numeric_limits<double>::max();
    double right = std::numeric_limits<double>::lowest();
    for (auto& icon : g_dockIcons) {
        auto button = icon.button.get();
        if (!button || button.ActualWidth() <= 0) {
            continue;
        }
        auto parent =
            Media::VisualTreeHelper::GetParent(button).try_as<UIElement>();
        if (!parent) {
            continue;
        }
        // The layout position, without the mod's own transforms.
        const auto offset = button.ActualOffset();
        const auto origin = parent.TransformToVisual(nullptr).TransformPoint(
            {offset.x, offset.y});
        const double width = button.ActualWidth();
        const double height = button.ActualHeight();
        // Windows keeps some buttons far off the window (like a hidden
        // Snipping Tool one): not among the apps.
        if (origin.X + width < 0 || origin.X > clientWidth ||
            button.Visibility() != Visibility::Visible) {
            continue;
        }

        // Buttons without an icon image (like Start) use their middle part.
        double iconTop = height * 0.2;
        double iconHeight = height * 0.6;
        // The layout position, not the drawn one: the taskbar's own press
        // animation shrinks the image, which mustn't move the icon around.
        if (auto image = icon.image.get(); image && image.ActualHeight() > 0) {
            if (auto imageParent = Media::VisualTreeHelper::GetParent(image)
                                       .try_as<UIElement>()) {
                const auto imageOffset = image.ActualOffset();
                iconTop = imageParent.TransformToVisual(button)
                              .TransformPoint({imageOffset.x, imageOffset.y})
                              .Y;
                iconHeight = image.ActualHeight();
            }
        }
        if (iconHeight <= 0) {
            continue;
        }

        left = std::min(left, (double)origin.X);
        right = std::max(right, origin.X + width);
        // The badge sits on the icon's top corner and grows with it.
        double contentTop = iconTop;
        double badgeRight = 0;
        // (Where it's drawn: the taskbar moves it with its own transform.)
        if (auto badge = icon.badge.get(); badge && IsBadgeShown(icon)) {
            const auto bounds = badge.TransformToVisual(button).TransformBounds(
                {0, 0, (float)badge.ActualWidth(),
                 (float)badge.ActualHeight()});
            contentTop = std::min(contentTop, (double)bounds.Y);
            badgeRight = bounds.X + bounds.Width;
        }
        // (And the other small parts over its corners, measured every second:
        // an app's overlay icon, the badges of the other monitors' docks.)
        if (icon.extrasRight > 0) {
            contentTop = std::min(contentTop, icon.extrasTop);
            badgeRight = std::max(badgeRight, icon.extrasRight);
        }

        Placed item{&icon,      button,     origin.X + width / 2,
                    origin.Y,   width,      iconTop,
                    iconHeight, contentTop, badgeRight};
        placed.push_back(item);
    }

    // At rest, the dock's icons are side by side at its icon size, centered
    // where the taskbar centers its buttons; the transparent taskbar's stay
    // where the taskbar puts them.
    std::sort(placed.begin(), placed.end(),
              [](const Placed& a, const Placed& b) { return a.center < b.center; });
    double rowWidth = 0;
    for (auto& item : placed) {
        // (With their names shown, "Combine taskbar buttons" off, buttons are
        // wide: drawn whole.)
        // (Those keep the taskbar's size.)
        const bool wide = item.buttonWidth > item.iconHeight * 3;
        item.base = dock && !wide
                        ? std::max(0.5, metrics.icon / item.iconHeight)
                        : 1.0;
        item.width = wide ? item.buttonWidth : item.iconHeight * item.base;
        item.slot = dock ? item.width + metrics.gap : item.buttonWidth;
        rowWidth += item.slot;
    }
    // (A box, with more apps than fit in the widest the dock gets: its ends,
    // where the row is cut; far off otherwise.)
    constexpr double kNoBox = 1e9;
    double boxLeft = -kNoBox;
    double boxRight = kNoBox;
    g_dockScrollMax = 0;
    if (dock && !placed.empty()) {
        rowWidth -= placed.back().slot - placed.back().width;
        const auto bounds = frame.TransformToVisual(nullptr).TransformBounds(
            {0, 0, (float)frame.ActualWidth(), (float)frame.ActualHeight()});
        const double widest =
            g_settings.dockMaxWidth > 0
                ? bounds.Width * g_settings.dockMaxWidth / 100.0 -
                      metrics.padSide * 2
                : 0;
        double shown = rowWidth;
        if (widest > metrics.icon && rowWidth > widest) {
            shown = widest;
            g_dockScrollMax = rowWidth - widest;
            boxLeft = bounds.X + bounds.Width / 2 - shown / 2;
            boxRight = boxLeft + shown;
        }
        g_dockScrollStep = metrics.icon + metrics.gap;
        g_dockScroll = std::clamp(g_dockScroll, 0.0, g_dockScrollMax);
        double x = bounds.X + bounds.Width / 2 - shown / 2 - g_dockScroll;
        for (auto& item : placed) {
            item.restCenter = x + item.width / 2;
            x += item.slot;
        }
    } else {
        for (auto& item : placed) {
            item.restCenter = item.center;
        }
    }
    // The dock's shelf at rest, around its icons.
    double restLeft = placed.empty() ? 0
                                     : placed.front().restCenter -
                                           placed.front().width / 2 -
                                           metrics.padSide;
    double restRight = placed.empty() ? 0
                                      : placed.back().restCenter +
                                            placed.back().width / 2 +
                                            metrics.padSide;
    // (In a box, its ends.)
    if (boxLeft > -kNoBox) {
        restLeft = boxLeft - metrics.padSide;
        restRight = boxRight + metrics.padSide;
    }
    const double restTop = restBottom - metrics.icon - metrics.padTop;

    // After a scale change: settled one frame after its buttons moved (see
    // g_dockScaleSettled), or after a while if they never do.
    if (!g_dockScaleSettled) {
        bool moved = false;
        for (const auto& item : placed) {
            if (!std::isnan(item.icon->lastCenter) &&
                std::fabs(item.restCenter - item.icon->lastCenter) > 0.5) {
                moved = true;
            }
        }
        if ((g_dockScaleMoved && !moved) ||
            now - g_dockScaleChangedAt > kDockScaleHideSeconds) {
            g_dockScaleSettled = true;
        }
        g_dockScaleMoved = g_dockScaleMoved || moved;
    }
    bool ownScale = true;
    if (const HMONITOR monitor = TaskbarMonitor(g_dockWnd)) {
        UINT dpiX = 0;
        UINT dpiY = 0;
        MONITORINFO info{sizeof(info)};
        RECT window{};
        const bool inside = GetMonitorInfo(monitor, &info) &&
                            GetWindowRect(g_dockWnd, &window) &&
                            window.top >= info.rcMonitor.top &&
                            window.bottom <= info.rcMonitor.bottom;
        if (!inside && SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI,
                                                  &dpiX, &dpiY))) {
            ownScale = dpiX == GetDpiForWindow(g_dockWnd);
        }
    }
    const bool hideContent = !g_dockScaleSettled || !ownScale;
    if (hideContent != g_dockContentHidden) {
        g_dockContentHidden = hideContent;
        if (hideContent) {
            g_dockOpacityKept =
                frame.ReadLocalValue(UIElement::OpacityProperty());
            frame.Opacity(0);
        } else {
            RestoreDockOpacity(frame);
        }
        // (The shelf with the icons: it's not in their frame.)
        if (auto host = g_dockBackground.get();
            host && host.Name() == kDockShelfHostName) {
            host.Opacity(hideContent ? 0 : 1);
        }
    }

    // The mouse on the dock: on its shelf, as big as it grew last frame (its
    // magnified icons included); on the transparent taskbar, over its apps.
    bool hovering = false;
    if (!placed.empty() && mouseY <= clientHeight + 2) {
        if (dock) {
            const double shelfLeft = std::min(restLeft, (double)g_dockShelfNow.X);
            const double shelfRight = std::max(
                restRight, (double)(g_dockShelfNow.X + g_dockShelfNow.Width));
            const double shelfTop = g_dockShelfNow.Height > 0
                                        ? std::min(restTop,
                                                   (double)g_dockShelfNow.Y)
                                        : restTop;
            hovering = mouseX >= shelfLeft - 2 && mouseX <= shelfRight + 2 &&
                       mouseY >= shelfTop - 2;
        } else {
            hovering = mouseY >= 0 && mouseX >= left - 16 &&
                       mouseX <= right + 16;
        }
    }
    const double follow = AnimationLevel() >= 2
                              ? 1.0
                              : 1 - std::exp(-dt * kMagnificationFollow);

    bool busy = hovering || !g_dockScaleSettled;
    // The icons still moving (growing, sliding, bouncing).
    bool moving = false;

    // The transparent taskbar's shadow: stronger over light windows, and
    // with the mouse on the taskbar.
    if (auto fill = g_shadowFill.get()) {
        const bool over = mouseY >= 0 && mouseY <= clientHeight + 2 &&
                          mouseX >= 0 && mouseX <= clientWidth;
        const double target =
            g_dockContentHidden
                ? 0.0
                : std::min(1.0, 0.66 + 0.28 * g_backdropLight +
                                    (over ? 0.12 : 0.0));
        if (g_dockContentHidden) {
            g_shadowStrength = 0;
        }
        g_shadowStrength += (target - g_shadowStrength) * (1 - std::exp(-dt * 8));
        if (std::fabs(target - g_shadowStrength) < 0.003) {
            g_shadowStrength = target;
        } else {
            busy = true;
        }
        if (std::fabs(fill.Opacity() - g_shadowStrength) > 0.002) {
            fill.Opacity(g_shadowStrength);
        }
        if (over) {
            busy = true;
        }
    }

    // A wave around the mouse, measured from where the icons rest (not from
    // where they're pushed to, which would chase the mouse): the icon under
    // it grows the most. Each icon grows from its bottom edge, and its
    // neighbors make room for it, so it never covers them.
    double rowGrow = 0;
    for (auto& item : placed) {
        DockIcon& icon = *item.icon;
        icon.baseScale = item.base;
        // The icon's bottom edge, which stays put.
        const double iconBottom =
            dock ? restBottom : middle + item.iconHeight / 2;
        // Never taller than the space up to the top of the window, badge
        // included. `reach` is how far the content goes above the bottom.
        const double reach =
            (item.iconTop + item.iconHeight - item.contentTop) * item.base;
        double maxScale = std::max(
            1.0, std::min(g_settings.magnification, (iconBottom - 1) / reach));
        // Wide buttons grow with their text: only a few pixels.
        if (item.buttonWidth > item.iconHeight * 3) {
            maxScale = std::min(maxScale, 1 + 12 / item.buttonWidth);
        }
        // (As big as it's drawn: last frame's size, see BigAppIcon.)
        if (icon.app) {
            UseBigIcon(icon, item.button,
                       (int)std::ceil(item.iconHeight * item.base *
                                      std::max(1.0, icon.magnification) *
                                      scale));
            UseBigOverlay(icon, item.button,
                          item.base * std::max(1.0, icon.magnification) *
                              scale);
        }

        double target = 1;
        if (hovering && maxScale > 1 && item.restCenter >= boxLeft &&
            item.restCenter <= boxRight) {
            const double distance = std::fabs(mouseX - item.restCenter) /
                                    g_settings.magnificationRange;
            if (distance < 1) {
                const double bell = 0.5 + 0.5 * std::cos(3.14159265 * distance);
                target = 1 + (maxScale - 1) * bell;
            }
        }
        icon.magnification += (target - icon.magnification) * follow;
        if (std::fabs(target - icon.magnification) < 0.002) {
            icon.magnification = target;
        } else {
            moving = true;
        }
        item.grow = item.width * (icon.magnification - 1);
        rowGrow += item.grow;
    }

    double before = 0;
    double drawnLeft = std::numeric_limits<double>::max();
    double drawnRight = std::numeric_limits<double>::lowest();
    double tallest = metrics.icon;
    const double edgeLeft = boxLeft - rowGrow / 2;
    const double edgeRight = boxRight + rowGrow / 2;
    for (auto& item : placed) {
        DockIcon& icon = *item.icon;
        const double base = item.base;
        const double iconBottom =
            dock ? restBottom : middle + item.iconHeight / 2;
        // Moves the icon's bottom edge to where it rests.
        const double align =
            iconBottom - (item.buttonTop + item.iconTop + item.iconHeight);
        const double reach =
            (item.iconTop + item.iconHeight - item.contentTop) * base;

        // Grows upwards from the icon's bottom edge, around the button's
        // middle, like on macOS.
        const double originY = item.iconTop + item.iconHeight;
        // Growing in when its app opened.
        if (SpringTowards(icon.appear, icon.appearVelocity, 1, 0.42, dt,
                          0.72)) {
            moving = true;
        }
        const double appear = std::max(icon.appear, 0.0);
        const double drawnScale = icon.magnification * base * appear;
        icon.scale.CenterX(item.buttonWidth / 2);
        icon.scale.CenterY(originY);
        icon.scale.ScaleX(drawnScale);
        icon.scale.ScaleY(drawnScale);
        if (icon.overlayKeep) {
            const double keep =
                icon.overlayCounted || drawnScale <= 0.01 ? 1 : 1 / drawnScale;
            if (std::fabs(icon.overlayKeep.ScaleX() - keep) > 0.001) {
                icon.overlayKeep.ScaleX(keep);
                icon.overlayKeep.ScaleY(keep);
            }
        }

        // Moved (an app opened or closed next to it, or the taskbar's
        // buttons centered again): it slides there from where it was (not
        // while an icon is dragged, which the taskbar moves itself, nor just
        // after its scale changed: see g_dockScaleChangedAt).
        if (!std::isnan(icon.lastCenter) &&
            std::fabs(item.restCenter - icon.lastCenter) > 0.5 &&
            !(GetAsyncKeyState(VK_LBUTTON) & 0x8000) &&
            now - g_dockScaleChangedAt > kDockScaleSettleSeconds) {
            icon.slide += icon.lastCenter - item.restCenter;
        }
        icon.lastCenter = item.restCenter;
        if (SpringTowards(icon.slide, icon.slideVelocity, 0, 0.36, dt)) {
            moving = true;
        }
        // Its middle now: pushed aside by the icons growing before it, the
        // whole row widening on both sides.
        const double center =
            item.restCenter + before + item.grow / 2 - rowGrow / 2 + icon.slide;
        before += item.grow;
        icon.lift.X(center - item.center);

        // The bounce uses what's left above the (possibly magnified) icon.
        const double bounceRoom = iconBottom - 1 - reach * icon.magnification;
        const double bounceHeight =
            dock ? std::min(item.width * kDockBounce, bounceRoom)
                 : std::min({item.iconHeight * 0.6, 14.0, bounceRoom});
        const double bounce = BounceOffset(icon, now, bounceHeight);
        icon.lift.Y(align + bounce);
        if (icon.bounceStart) {
            moving = true;
        }

        UpdateDots(icon, {item.buttonTop, originY, align + bounce, iconBottom,
                          dock ? shelfBottom : clientHeight});

        // It takes the mouse only in its own room in the row (its neighbors
        // take theirs), its badge included: in the button's coordinates,
        // before the scale.
        if (icon.clip && drawnScale > 0.05) {
            const double half =
                (item.slot + item.grow) / 2 / drawnScale;
            const double middleX = item.buttonWidth / 2;
            // (A little more for the badge's antialiased edge.)
            // (In a box, cut at its ends.)
            const double clipLeft = std::max(
                middleX - half, middleX + (edgeLeft - center) / drawnScale);
            const double clipRight = std::min(
                std::max(middleX + half,
                         item.badgeRight > 0
                             ? item.badgeRight + 2 / drawnScale
                             : 0.0),
                middleX + (edgeRight - center) / drawnScale);
            const winrt::Windows::Foundation::Rect rect{
                (float)clipLeft, -2000.0f,
                (float)std::max(clipRight - clipLeft, 0.0), 4000.0f};
            if (std::fabs(rect.X - icon.clipRect.X) > 0.05f ||
                std::fabs(rect.Width - icon.clipRect.Width) > 0.05f) {
                icon.clipRect = rect;
                icon.clip.Rect(rect);
            }
        }

        const double drawnHalf = item.width * icon.magnification * appear / 2;
        drawnLeft = std::max(std::min(drawnLeft, center - drawnHalf), edgeLeft);
        drawnRight =
            std::min(std::max(drawnRight, center + drawnHalf), edgeRight);
        tallest = std::max(tallest, item.iconHeight * base *
                                        icon.magnification * appear);
    }
    busy = busy || moving;

    // The transparent taskbar takes the mouse around its apps, magnified
    // ones included (in steps, not set again every frame), and puts its
    // shadow behind them.
    if (IsTransparentTaskbar() && !placed.empty()) {
        PlaceShadow(left, right);
        UpdateAppsArea(std::min(left, std::floor(drawnLeft / 8) * 8),
                       std::max(right, std::ceil(drawnRight / 8) * 8), false);
    }

    if (dock) {
        // The shelf around the icons as they're drawn, as high as the
        // tallest one.
        const winrt::Windows::Foundation::Rect shelf =
            placed.empty()
                ? winrt::Windows::Foundation::Rect{}
                : winrt::Windows::Foundation::Rect{
                      (float)(drawnLeft - metrics.padSide),
                      (float)(restBottom - tallest - metrics.padTop),
                      (float)(drawnRight - drawnLeft + metrics.padSide * 2),
                      (float)(shelfBottom -
                              (restBottom - tallest - metrics.padTop))};
        g_dockShelfNow = shelf;
        if (auto background = g_dockBackground.get()) {
            const auto origin =
                background.TransformToVisual(nullptr).TransformPoint({0, 0});
            PlaceDockShelf(
                g_dockShelf,
                {shelf.X - origin.X, shelf.Y - origin.Y, shelf.Width,
                 shelf.Height},
                std::min(g_settings.cornerRadius, (double)shelf.Height / 2));
        }

        // The window takes the mouse only on the taskbar's own height and
        // the shelf at rest, and on its whole room while the mouse is on the
        // dock or its icons move (see ClipDockWindow).
        // (The apps' badges on the icons' top corners stick out of it.)
        RECT rest{};
        if (!placed.empty()) {
            double top = restTop;
            double right = restRight;
            for (const auto& item : placed) {
                if (item.badgeRight > 0) {
                    top = std::min(top, restBottom - (item.iconTop +
                                                      item.iconHeight -
                                                      item.contentTop) *
                                                         item.base);
                    right = std::max(
                        right, item.restCenter + (item.badgeRight -
                                                  item.buttonWidth / 2) *
                                                     item.base);
                }
            }
            // (In a box, nothing past its end.)
            if (boxLeft > -kNoBox) {
                right = std::min(right, restRight);
            }
            rest = {(LONG)std::floor(restLeft * scale) - 1,
                    (LONG)std::floor(top * scale) - 2,
                    (LONG)std::ceil(right * scale) + 2, client.bottom};
        }
        const bool open = hovering || moving;
        if (open != g_dockRoomOpen || !EqualRect(&rest, &g_dockShelfRest)) {
            g_dockRoomOpen = open;
            g_dockShelfRest = rest;
            ClipDockWindow(g_dockWnd);
        }
        // Where the shelf's top is on the screen with the dock shown (it may
        // be on its way up), for the apps' thumbnails (see PlaceThumbnails).
        RECT window{};
        MONITORINFO shownOn{sizeof(shownOn)};
        if (GrownTaskbar* room = FindGrownTaskbar(g_dockWnd);
            room && GetWindowRect(g_dockWnd, &window) &&
            GetMonitorInfo(TaskbarMonitor(g_dockWnd), &shownOn)) {
            room->shelfTop =
                shownOn.rcMonitor.bottom - (window.bottom - window.top) +
                (LONG)std::floor(std::min((double)shelf.Y, restTop) * scale);
        }
    }

    // The mouse left the dock with the taskbar holding the focus (a click on
    // the open app's icon minimized it): the taskbar's hide timer hands the
    // focus on, so it hides (see HandFocusBack).
    if (hovering) {
        if (dock && g_dockHovered != g_dockWnd) {
            RaiseOverWindows(g_dockWnd);
        }
        g_dockHovered = g_dockWnd;
        // (Windows shows them in its window as it is: placed beforehand.)
        if (dock) {
            WatchThumbnails();
            PlaceThumbnailWindows();
        }
    } else if (g_dockHovered == g_dockWnd) {
        g_dockHovered = nullptr;
        PlaceThumbnailWindows();
        if (g_taskbarAutoHides && GetForegroundWindow() == g_dockWnd) {
            ScheduleTaskbarHide(g_dockWnd,
                                (UINT)std::max(g_settings.hideDelay, 10));
        }
    }

    return busy;
}

// Taskbar buttons are recycled: a saved native picture belongs to an app,
// not just to the Image object which happens to still be alive.
bool SameDockIconApp(const std::wstring& saved, const std::wstring& current) {
    return !saved.empty() && !current.empty() &&
           _wcsicmp(saved.c_str(), current.c_str()) == 0;
}

// The app's own icon, as big as its button shows it now (the taskbar's
// picture is decoded at its small size and blurs magnified; a much bigger one
// shrunk to the icon at rest looks jagged), in steps of 16 pixels so a few
// sizes are made. Kept for each app and size (nullptr if it has none the
// shell knows). Taskbar thread only.
constexpr int kBigIconStep = 16;

int BigIconPixels(int pixels) {
    return std::clamp((pixels + kBigIconStep - 1) / kBigIconStep * kBigIconStep,
                      kBigIconStep, 256);
}

// The apps' pictures by app and size, as XAML pictures (made at their first
// use, freed whole while unloading, see FreeXamlGlobals). Taskbar thread
// only.
using DockPictures = std::vector<std::pair<std::wstring, Media::ImageSource>>;
[[clang::no_destroy]] std::optional<DockPictures> g_bigIcons;

// A 32-bit picture for XAML (nullptr if it can't be read).
Media::ImageSource PictureFromBitmap(HBITMAP bitmap) {
    BITMAP info{};
    GetObject(bitmap, sizeof(info), &info);
    const int width = info.bmWidth;
    const int height = std::abs(info.bmHeight);
    if (width <= 0 || height <= 0) {
        return nullptr;
    }
    Media::ImageSource source{nullptr};
    Media::Imaging::WriteableBitmap picture(width, height);
    BITMAPINFO header{};
    header.bmiHeader = {sizeof(BITMAPINFOHEADER), width, -height, 1, 32,
                        BI_RGB};
    HDC screen = GetDC(nullptr);
    uint8_t* pixels = picture.PixelBuffer().data();
    if (GetDIBits(screen, bitmap, 0, height, pixels, &header,
                  DIB_RGB_COLORS) == height) {
        // The picture takes its colors premultiplied by their alpha; the
        // shell's come as they are for most apps (a color brighter than its
        // alpha says so). Without any alpha, it's all solid.
        const size_t count = (size_t)width * height * 4;
        bool alpha = false;
        for (size_t i = 3; i < count && !alpha; i += 4) {
            alpha = pixels[i] != 0;
        }
        for (size_t i = 3; !alpha && i < count; i += 4) {
            pixels[i] = 255;
        }
        bool straight = false;
        for (size_t i = 0; i < count && !straight; i += 4) {
            straight = std::max({pixels[i], pixels[i + 1], pixels[i + 2]}) >
                       pixels[i + 3];
        }
        for (size_t i = 0; straight && i < count; i += 4) {
            for (int c = 0; c < 3; c++) {
                pixels[i + c] = (uint8_t)(pixels[i + c] * pixels[i + 3] / 255);
            }
        }
        picture.Invalidate();
        source = picture;
    }
    ReleaseDC(nullptr, screen);
    return source;
}

// The apps' overlay icons as XAML pictures, by app, the picture they come
// from (its serial, see OverlayIcon) and their size (like g_bigIcons).
// Taskbar thread only.
[[clang::no_destroy]] std::optional<DockPictures> g_overlaySources;

// A red bubble with `count` (a smaller dot in its corner for 0), `pixels`
// wide, for an app's overlay icon the app draws small (see UseBigOverlay).
Media::ImageSource CountBadgePicture(int count, int pixels) {
    winrt::com_ptr<IWICImagingFactory> wic;
    winrt::com_ptr<ID2D1Factory> d2d;
    winrt::com_ptr<IDWriteFactory> dwrite;
    winrt::com_ptr<IWICBitmap> bitmap;
    winrt::com_ptr<ID2D1RenderTarget> target;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(wic.put()))) ||
        FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                 d2d.put())) ||
        FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                   __uuidof(IDWriteFactory),
                                   (IUnknown**)dwrite.put())) ||
        FAILED(wic->CreateBitmap(pixels, pixels, GUID_WICPixelFormat32bppPBGRA,
                                 WICBitmapCacheOnLoad, bitmap.put())) ||
        FAILED(d2d->CreateWicBitmapRenderTarget(
            bitmap.get(),
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                  D2D1_ALPHA_MODE_PREMULTIPLIED)),
            target.put()))) {
        return nullptr;
    }
    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    if (FAILED(target->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.25f, 0.26f),
                                             brush.put()))) {
        return nullptr;
    }
    const float size = (float)pixels;
    const float radius = count > 0 ? size / 2 : size * 0.3f;
    target->BeginDraw();
    target->Clear(D2D1::ColorF(0, 0, 0, 0));
    target->FillEllipse({{size - radius, size - radius}, radius, radius},
                        brush.get());
    if (count > 0) {
        const std::wstring number =
            count > 99 ? L"99+" : std::to_wstring(count);
        winrt::com_ptr<IDWriteTextFormat> format;
        if (SUCCEEDED(dwrite->CreateTextFormat(
                L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD,
                DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                size * (number.size() > 1 ? 0.48f : 0.66f), L"",
                format.put()))) {
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            brush->SetColor(D2D1::ColorF(1, 1, 1));
            target->DrawText(number.c_str(), (UINT32)number.size(),
                             format.get(), D2D1::RectF(0, 0, size, size),
                             brush.get());
        }
    }
    if (FAILED(target->EndDraw())) {
        return nullptr;
    }
    Media::Imaging::WriteableBitmap picture(pixels, pixels);
    if (FAILED(bitmap->CopyPixels(nullptr, (UINT)pixels * 4,
                                  (UINT)(pixels * pixels * 4),
                                  picture.PixelBuffer().data()))) {
        return nullptr;
    }
    picture.Invalidate();
    return picture;
}

// The app's overlay icon drawn sharp, also magnified: as a red bubble with
// the count its text gives (like Discord's "3 notifications"; its own
// picture is only as big as its window's DPI makes it), as big as it's
// shown (`factor` turns its size into pixels), or from the app's own
// picture when its text has no count and it's bigger than the taskbar's
// (16 pixels). The taskbar's back when the app has none.
void UseBigOverlay(DockIcon& icon,
                   FrameworkElement const& button,
                   double factor) {
    // (Every frame: nothing to do while no app has one.)
    if (!icon.overlayBig) {
        AcquireSRWLockShared(&g_overlayIconsLock);
        const bool none = g_overlayIcons.empty();
        ReleaseSRWLockShared(&g_overlayIconsLock);
        if (none) {
            return;
        }
    }
    const std::wstring appId = GetButtonAppId(button);
    // (No ID for a moment, being laid out again: left as it is.)
    if (appId.empty()) {
        return;
    }
    bool has = false;
    unsigned serial = 0;
    int size = 0;
    int count = 0;
    bool counted = false;
    bool listed = false;
    AcquireSRWLockShared(&g_overlayIconsLock);
    for (const auto& overlay : g_overlayIcons) {
        if (overlay.appId == appId && overlay.icon) {
            listed = true;
            counted = OverlayHasCount(overlay.description);
            has = overlay.size > 16 || counted;
            serial = overlay.serial;
            size = overlay.size;
            count = OverlayShownCount(overlay);
            break;
        }
    }
    ReleaseSRWLockShared(&g_overlayIconsLock);
    icon.overlayCounted = counted;
    // (Kept at its size from when it's found, here or below.)
    auto keepSize = [&](Controls::Image const& found) {
        icon.overlayImage = winrt::make_weak(found);
        icon.overlayKeep = Media::ScaleTransform();
        SetStyled(found, UIElement::RenderTransformProperty(), icon.overlayKeep);
        SetStyled(found, UIElement::RenderTransformOriginProperty(),
                  winrt::box_value(winrt::Windows::Foundation::Point{0.5f, 0.5f}));
    };
    // (Its image found once it has one: the taskbar makes it then, maybe a
    // moment later, so it's looked for again a few times.)
    constexpr int kOverlayMostLooks = 10;
    if (listed && !icon.overlayLooked &&
        icon.overlayLooks < kOverlayMostLooks) {
        if (auto found =
                FindByName(button, L"OverlayIcon").try_as<Controls::Image>()) {
            icon.overlayLooked = true;
            keepSize(found);
        } else {
            icon.overlayLooks++;
        }
    }
    auto image = icon.overlayImage.get();
    if (!has) {
        if (image && icon.overlayBig && image.Source() == icon.overlayBig) {
            image.Source(SameDockIconApp(icon.overlayAppId, appId)
                             ? icon.overlayOwn : nullptr);
        }
        icon.overlayOwn = nullptr;
        icon.overlayBig = nullptr;
        icon.overlayAppId.clear();
        return;
    }
    if (!icon.overlayAppId.empty() &&
        !SameDockIconApp(icon.overlayAppId, appId)) {
        icon.overlayOwn = nullptr;
    }
    if (!image) {
        image = FindByName(button, L"OverlayIcon").try_as<Controls::Image>();
        if (!image) {
            return;
        }
        keepSize(image);
    }
    const bool own = size > 16 && !counted;
    const int pixels =
        own ? size
            : BigIconPixels((int)std::ceil(image.ActualWidth() * factor));
    const std::wstring key = appId + L"|" + std::to_wstring(serial) + L"|" +
                             std::to_wstring(own ? 0 : pixels);
    Media::ImageSource big{nullptr};
    bool made = false;
    auto& sources =
        g_overlaySources ? *g_overlaySources : g_overlaySources.emplace();
    for (const auto& [id, source] : sources) {
        if (id == key) {
            big = source;
            made = true;
        }
    }
    if (!made) {
        if (own) {
            HICON copy = nullptr;
            AcquireSRWLockShared(&g_overlayIconsLock);
            for (const auto& overlay : g_overlayIcons) {
                if (overlay.appId == appId && overlay.serial == serial &&
                    overlay.icon) {
                    copy = CopyIcon(overlay.icon);
                }
            }
            ReleaseSRWLockShared(&g_overlayIconsLock);
            ICONINFO info{};
            if (copy && GetIconInfo(copy, &info)) {
                if (info.hbmColor) {
                    big = PictureFromBitmap(info.hbmColor);
                    DeleteObject(info.hbmColor);
                }
                if (info.hbmMask) {
                    DeleteObject(info.hbmMask);
                }
            }
            if (copy) {
                DestroyIcon(copy);
            }
        } else {
            big = CountBadgePicture(count, pixels);
        }
        // (A few sizes of a few apps' kept.)
        constexpr size_t kMaxOverlaySources = 16;
        if (sources.size() >= kMaxOverlaySources) {
            sources.erase(sources.begin(),
                          sources.begin() + kMaxOverlaySources / 2);
        }
        sources.push_back({key, big});
    }
    const auto source = image.Source();
    if (!big || !source || source == big ||
        image.ReadLocalValue(Controls::Image::SourceProperty())
            .try_as<Data::BindingExpression>()) {
        return;
    }
    // (At the size it has: a bigger picture mustn't make it bigger.)
    if (std::isnan(image.Width()) || std::isnan(image.Height())) {
        SetStyled(image, FrameworkElement::WidthProperty(),
                  winrt::box_value(image.ActualWidth()));
        SetStyled(image, FrameworkElement::HeightProperty(),
                  winrt::box_value(image.ActualHeight()));
    }
    if (source != icon.overlayBig) {
        icon.overlayOwn = source;
    }
    icon.overlayAppId = appId;
    icon.overlayBig = big;
    image.Source(big);
}

// The big icons are read from the shell on a thread of their own (it can
// take longer than a frame, more for a packaged app): asked for by
// BigAppIcon, which takes them once read; the dock draws again then (see
// TaskbarSubclassProc). Each one asked for once: 0 waiting, 1 being read, 2
// read (`bitmap` nullptr if the shell has none).
struct BigIconJob {
    std::wstring key;
    std::wstring appId;
    int pixels = 0;
    int state = 0;
    HBITMAP bitmap = nullptr;
};
SRWLOCK g_bigIconJobsLock = SRWLOCK_INIT;
std::vector<BigIconJob> g_bigIconJobs;
// (Made by the taskbar's thread under the lock, stopped by the unloading
// one, see StopBigIcons.)
HANDLE g_bigIconThread;
HANDLE g_bigIconWake;
HANDLE g_bigIconStop;
const UINT g_bigIconsReadMessage =
    RegisterWindowMessage(L"WindhawkMacIslandDock_BigIconsRead");

DWORD WINAPI BigIconThreadProc(LPVOID) {
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const HANDLE events[] = {g_bigIconStop, g_bigIconWake};
    while (WaitForMultipleObjects(2, events, FALSE, INFINITE) ==
           WAIT_OBJECT_0 + 1) {
        for (;;) {
            std::wstring key;
            std::wstring appId;
            int pixels = 0;
            AcquireSRWLockExclusive(&g_bigIconJobsLock);
            for (auto& job : g_bigIconJobs) {
                if (job.state == 0) {
                    job.state = 1;
                    key = job.key;
                    appId = job.appId;
                    pixels = job.pixels;
                    break;
                }
            }
            ReleaseSRWLockExclusive(&g_bigIconJobsLock);
            if (key.empty() ||
                WaitForSingleObject(g_bigIconStop, 0) == WAIT_OBJECT_0) {
                break;
            }
            // (A pinned shortcut's icon of its own first: the taskbar shows
            // that one.)
            HBITMAP bitmap = PinnedShortcutIconBitmap(appId, pixels);
            if (!bitmap) {
                bitmap = LoadAppIconBitmap(nullptr, appId, pixels);
            }
            AcquireSRWLockExclusive(&g_bigIconJobsLock);
            auto it = std::find_if(
                g_bigIconJobs.begin(), g_bigIconJobs.end(),
                [&](const BigIconJob& job) { return job.key == key; });
            if (it != g_bigIconJobs.end()) {
                it->bitmap = std::exchange(bitmap, nullptr);
                it->state = 2;
            }
            ReleaseSRWLockExclusive(&g_bigIconJobsLock);
            if (bitmap) {
                DeleteObject(bitmap);
            }
            if (HWND taskbar = FindCurrentProcessTaskbarWnd()) {
                PostMessage(taskbar, g_bigIconsReadMessage, 0, 0);
            }
        }
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    return 0;
}

// Asks for an app's big icon at `pixels` (see BigIconJob); its picture if
// it's been read (true; nullptr for none), taken out of the jobs.
bool TakeBigIcon(const std::wstring& key,
                 const std::wstring& appId,
                 int pixels,
                 HBITMAP* bitmap) {
    *bitmap = nullptr;
    bool read = false;
    bool asked = false;
    AcquireSRWLockExclusive(&g_bigIconJobsLock);
    auto it =
        std::find_if(g_bigIconJobs.begin(), g_bigIconJobs.end(),
                     [&](const BigIconJob& job) { return job.key == key; });
    if (it != g_bigIconJobs.end()) {
        if (it->state == 2) {
            *bitmap = it->bitmap;
            g_bigIconJobs.erase(it);
            read = true;
        }
    } else if (!g_unloading) {
        // (Its thread made the first time, never once unloading.)
        if (!g_bigIconThread) {
            g_bigIconWake = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            g_bigIconStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            if (g_bigIconWake && g_bigIconStop) {
                g_bigIconThread = CreateThread(nullptr, 0, BigIconThreadProc,
                                               nullptr, 0, nullptr);
            }
        }
        if (g_bigIconThread) {
            g_bigIconJobs.push_back({key, appId, pixels});
            asked = true;
        }
    }
    const HANDLE wake = g_bigIconWake;
    ReleaseSRWLockExclusive(&g_bigIconJobsLock);
    if (asked) {
        SetEvent(wake);
    }
    return read;
}

// Its thread stopped and what it read let go (unloading).
void StopBigIcons() {
    AcquireSRWLockExclusive(&g_bigIconJobsLock);
    const HANDLE thread = std::exchange(g_bigIconThread, nullptr);
    ReleaseSRWLockExclusive(&g_bigIconJobsLock);
    if (thread) {
        SetEvent(g_bigIconStop);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
    AcquireSRWLockExclusive(&g_bigIconJobsLock);
    for (const auto& job : g_bigIconJobs) {
        if (job.bitmap) {
            DeleteObject(job.bitmap);
        }
    }
    std::vector<BigIconJob>().swap(g_bigIconJobs);
    for (HANDLE* event : {&g_bigIconWake, &g_bigIconStop}) {
        if (*event) {
            CloseHandle(*event);
            *event = nullptr;
        }
    }
    ReleaseSRWLockExclusive(&g_bigIconJobsLock);
}

Media::ImageSource BigAppIcon(const std::wstring& appId, int pixels) {
    pixels = BigIconPixels(pixels);
    const std::wstring key = std::to_wstring(pixels) + L"|" + appId;
    auto& icons = g_bigIcons ? *g_bigIcons : g_bigIcons.emplace();
    for (const auto& [id, source] : icons) {
        if (id == key) {
            return source;
        }
    }
    // (Not read yet: the taskbar's own picture meanwhile.)
    HBITMAP bitmap = nullptr;
    if (!TakeBigIcon(key, appId, pixels, &bitmap)) {
        return nullptr;
    }
    Media::ImageSource source{nullptr};
    if (bitmap) {
        source = PictureFromBitmap(bitmap);
        DeleteObject(bitmap);
    }
    // The oldest go past a limit (the icons showing keep theirs).
    constexpr size_t kMaxBigIcons = 64;
    if (icons.size() >= kMaxBigIcons) {
        icons.erase(icons.begin(), icons.begin() + kMaxBigIcons / 2);
    }
    icons.push_back({key, source});
    return source;
}

// Shows the app's big icon on its button, unless it already does. The
// taskbar sets its picture again when the app's icon changes (or the button
// is reused for another app): the new one is replaced in turn. Not when the
// picture is bound to the taskbar's data (replacing it would drop that).
void UseBigIcon(DockIcon& icon, FrameworkElement const& button, int pixels) {
    auto image = icon.image.get().try_as<Controls::Image>();
    if (!image) {
        return;
    }
    const auto source = image.Source();
    const bool shownBig = icon.bigSource && source == icon.bigSource;
    // (The button reused for another app may keep the big picture of the one
    // before: the taskbar doesn't always set its own again. Measured at
    // sign-in: Discord's stayed on Windhawk's button.)
    const std::wstring appId = GetButtonAppId(button);
    const bool sameApp = appId == icon.bigAppId;
    // (The taskbar's own picture kept is the other app's: none to put back.
    // Not for a button without an ID for a moment.)
    if (!appId.empty() && !icon.bigAppId.empty() && !sameApp) {
        icon.ownSource = nullptr;
        icon.ownAppId.clear();
    }
    if (!source ||
        (shownBig && sameApp && icon.bigPixels == BigIconPixels(pixels))) {
        return;
    }
    if (image.ReadLocalValue(Controls::Image::SourceProperty())
            .try_as<Data::BindingExpression>()) {
        return;
    }
    auto big = appId.empty() ? nullptr : BigAppIcon(appId, pixels);
    if (!big) {
        return;
    }
    // (At the size it has: a bigger picture mustn't make it bigger.)
    if (std::isnan(image.Width()) || std::isnan(image.Height())) {
        SetStyled(image, FrameworkElement::WidthProperty(),
                  winrt::box_value(image.ActualWidth()));
        SetStyled(image, FrameworkElement::HeightProperty(),
                  winrt::box_value(image.ActualHeight()));
    }
    // (Another size of it: the taskbar's own is still the one kept.)
    if (!shownBig) {
        icon.ownSource = source;
        icon.ownAppId = appId;
    }
    icon.bigSource = big;
    icon.bigPixels = BigIconPixels(pixels);
    icon.bigAppId = appId;
    image.Source(big);
}

// The taskbar's own picture back.
void RestoreDockIcon(DockIcon& icon) {
    // (Put back unless the button shows another app now; one without an ID
    // at the moment is taken as the same.)
    auto button = icon.button.get();
    const std::wstring appId = button ? GetButtonAppId(button) : std::wstring();
    const auto sameApp = [&](const std::wstring& saved) {
        return appId.empty() || SameDockIconApp(saved, appId);
    };
    if (auto image = icon.image.get().try_as<Controls::Image>();
        image && icon.ownSource && image.Source() == icon.bigSource &&
        sameApp(icon.ownAppId)) {
        image.Source(icon.ownSource);
    }
    icon.ownSource = nullptr;
    icon.ownAppId.clear();
    icon.bigSource = nullptr;
    icon.bigPixels = 0;
    icon.bigAppId.clear();
    if (auto image = icon.overlayImage.get();
        image && icon.overlayBig && image.Source() == icon.overlayBig) {
        image.Source(sameApp(icon.overlayAppId) ? icon.overlayOwn : nullptr);
    }
    icon.overlayOwn = nullptr;
    icon.overlayBig = nullptr;
    icon.overlayAppId.clear();
}

void CheckDockItems() {
    auto repeater = g_dockRepeater.get();
    if (!repeater) {
        return;
    }
    const int count = Media::VisualTreeHelper::GetChildrenCount(repeater);
    if (count != g_dockItemCount) {
        g_dockItemCount = count;
        RescanDockIcons();
        WatchBadgesAndPublish();
    }
}

void SetupDockBehavior(HWND hWnd,
                       FrameworkElement const& frame,
                       FrameworkElement const& background) {
    g_dockWnd = hWnd;
    g_dockFrame = winrt::make_weak(frame);
    g_dockBackground = winrt::make_weak(background);
    auto repeater = FindByName(frame, L"TaskbarFrameRepeater");
    g_dockRepeater = winrt::make_weak(repeater ? repeater : frame);
    g_dockItemCount = -1;
    g_dockSince = NowSeconds();

    // Pointer events reach the frame even when a button handles them.
    g_pointerHandler = winrt::box_value(Input::PointerEventHandler(
        [](winrt::Windows::Foundation::IInspectable const&,
           Input::PointerRoutedEventArgs const&) { StartRendering(); }));
    frame.AddHandler(UIElement::PointerMovedEvent(), g_pointerHandler, true);
    g_pressedHandler = winrt::box_value(Input::PointerEventHandler(
        [hWnd](winrt::Windows::Foundation::IInspectable const&,
               Input::PointerRoutedEventArgs const& args) {
            g_dockPressedAt = NowSeconds();
            if (args.GetCurrentPoint(nullptr)
                    .Properties()
                    .IsRightButtonPressed()) {
                g_dockRightPressedAt = g_dockPressedAt;
                g_dockRightPressedWnd = hWnd;
            }
            DockScope scope(hWnd);
            Safely(L"A click on the taskbar", [&] { OnDockPressed(args); });
        }));
    frame.AddHandler(UIElement::PointerPressedEvent(), g_pressedHandler, true);
    // The wheel scrolls the dock's box (only when it has more apps than
    // fit: otherwise the taskbar keeps it).
    g_dockScroll = 0;
    g_dockScrollMax = 0;
    g_wheelHandler = winrt::box_value(Input::PointerEventHandler(
        [hWnd](winrt::Windows::Foundation::IInspectable const&,
               Input::PointerRoutedEventArgs const& args) {
            DockScope scope(hWnd);
            Safely(L"The wheel on the taskbar", [&] {
                if (g_dockScrollMax <= 0 || !IsDock()) {
                    return;
                }
                const int delta = args.GetCurrentPoint(nullptr)
                                      .Properties()
                                      .MouseWheelDelta();
                g_dockScroll = std::clamp(
                    g_dockScroll - delta / 120.0 * g_dockScrollStep, 0.0,
                    g_dockScrollMax);
                args.Handled(true);
                StartRendering();
            });
        }));
    frame.AddHandler(UIElement::PointerWheelChangedEvent(), g_wheelHandler,
                     true);
    g_pointerTarget = winrt::make_weak(frame);

    // New and closed apps change the number of buttons.
    g_layoutRevoker = frame.LayoutUpdated(
        winrt::auto_revoke,
        [hWnd](winrt::Windows::Foundation::IInspectable const&,
               winrt::Windows::Foundation::IInspectable const&) {
            DockScope scope(hWnd);
            Safely(L"Following the layout", [] {
                CheckDockItems();
                // Positions may have changed: line the icons up again.
                // Moving them doesn't cause another layout pass.
                StartRendering();
                // Looking for new badges, at most twice a second.
                if (NowSeconds() - g_lastBadgeScan > 0.5) {
                    g_lastBadgeScan = NowSeconds();
                    WatchBadgesAndPublish();
                }
            });
        });
    CheckDockItems();
    StartRendering();
    if (!g_dockIsMain) {
        return;
    }

    // Badges fade in and out, so they're also checked every second.
    g_badgeTimer = DispatcherTimer();
    g_badgeTimer.Interval(std::chrono::seconds(1));
    g_badgeTimer.Tick([](winrt::Windows::Foundation::IInspectable const&,
                         winrt::Windows::Foundation::IInspectable const&) {
        if (!g_unloading) {
            Safely(L"Watching the badges", [] { WatchBadgesAndPublish(); });
            Safely(L"Measuring the icons' extras",
                   [] { MeasureDockExtras(); });
            Safely(L"Following the progress", [] { RecheckProgress(); });
        }
    });
    g_badgeTimer.Start();
}

