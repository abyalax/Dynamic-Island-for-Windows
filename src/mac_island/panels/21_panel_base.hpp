LRESULT Panel::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam,
                             LPARAM lParam) {
    // (A window of its own: Windows' caption buttons, over its title bar.)
    if (LRESULT result = 0;
        m_windowed && DwmDefWindowProc(hWnd, msg, wParam, lParam, &result)) {
        return result;
    }
    const POINT pt{(short)LOWORD(lParam) - m_contentOffset.x,
                   (short)HIWORD(lParam) - m_contentOffset.y};
    bool redraw = false;
    switch (msg) {
        case WM_ACTIVATE:
            // Clicked somewhere else (not a dialog of its own).
            if (LOWORD(wParam) == WA_INACTIVE && !m_dialogShown &&
                !StaysWhenDeactivated()) {
                Close();
            }
            if (m_windowed) {
                m_windowActive = LOWORD(wParam) != WA_INACTIVE;
                redraw = true;
            }
            if (m_windowed && LOWORD(wParam) != WA_INACTIVE && m_target > 0) {
                OnActivated();
            }
            break;

        // A window of its own: Alt or F10 alone doesn't go into Windows'
        // menu mode (it has no menu bar: a modal loop that stopped every
        // island's frames, its first click taken by the menu); Alt+Space
        // still opens its system menu.
        case WM_SYSCOMMAND:
            if (m_windowed && (wParam & 0xFFF0) == SC_KEYMENU && lParam == 0) {
                return 0;
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);

        // Moved, sized or its system menu open (Windows' own modal loops):
        // the islands' frames on a timer meanwhile.
        case WM_ENTERSIZEMOVE:
        case WM_ENTERMENULOOP:
            if (m_windowed) {
                SetTimer(hWnd, kPanelModalFramesTimerId, kPanelModalFramesMs,
                         nullptr);
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);
        case WM_EXITSIZEMOVE:
        case WM_EXITMENULOOP:
            if (m_windowed) {
                KillTimer(hWnd, kPanelModalFramesTimerId);
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);

        // A window of its own: Windows' caption taken off (its borders kept,
        // inside the screen when maximized), its own title bar there.
        case WM_NCCALCSIZE:
            if (!m_windowed || !wParam) {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
            {
                auto* sizes = (NCCALCSIZE_PARAMS*)lParam;
                const LONG top = sizes->rgrc[0].top;
                const LRESULT result =
                    DefWindowProc(hWnd, msg, wParam, lParam);
                sizes->rgrc[0].top =
                    top + (IsZoomed(hWnd) ? WindowFrameThickness(hWnd) : 0);
                return result;
            }

        case WM_NCHITTEST:
            if (!m_windowed) {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
            {
                const LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
                if (hit != HTCLIENT) {
                    return hit;
                }
                POINT at{(short)LOWORD(lParam), (short)HIWORD(lParam)};
                ScreenToClient(hWnd, &at);
                // Its top border (taken off with the caption) still resizes
                // it; the rest of the title bar moves it.
                const int frame = WindowFrameThickness(hWnd);
                if (!IsZoomed(hWnd) && at.y < frame) {
                    RECT client{};
                    GetClientRect(hWnd, &client);
                    return at.x < frame * 2                  ? HTTOPLEFT
                           : at.x >= client.right - frame * 2 ? HTTOPRIGHT
                                                              : HTTOP;
                }
                return at.y < CaptionHeight() ? HTCAPTION : HTCLIENT;
            }

        case WM_KEYDOWN:
            g_lastTyped = NowSeconds();
            redraw = OnKey(wParam);
            // (A window of its own closes with its "x", or Alt+F4.)
            if (wParam == VK_ESCAPE && !redraw && !m_windowed) {
                Close();
            }
            break;

        // A window of its own: sized, painted, moved to another DPI.
        case WM_SIZE:
            if (!m_windowed) {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
            if (m_target > 0 && wParam != SIZE_MINIMIZED) {
                OnResized();
                redraw = true;
            }
            break;

        case WM_GETMINMAXINFO:
            if (m_windowed) {
                auto* info = (MINMAXINFO*)lParam;
                info->ptMinTrackSize = {MulDiv(420, m_windowDpi, 96),
                                        MulDiv(460, m_windowDpi, 96)};
                return 0;
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);

        case WM_PAINT:
            if (!m_windowed) {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
            {
                PAINTSTRUCT paint;
                BeginPaint(hWnd, &paint);
                EndPaint(hWnd, &paint);
            }
            redraw = true;
            break;

        case WM_ERASEBKGND:
            if (m_windowed) {
                return 1;
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);

        case WM_DPICHANGED:
            if (!m_windowed) {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
            {
                const UINT dpi = HIWORD(wParam);
                m_scale = dpi / 96.0f * m_windowSize;
                m_windowDpi = dpi;
                const RECT* suggested = (const RECT*)lParam;
                SetWindowPos(hWnd, nullptr, suggested->left, suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
                if (m_target > 0) {
                    Prepare(m_scale);
                    redraw = true;
                }
            }
            break;

        // Alt+F4 (the panel has the focus): closed like Esc, never
        // destroyed (it's opened again with the same window).
        case WM_CLOSE:
            Close();
            return 0;

        case WM_CHAR:
            g_lastTyped = NowSeconds();
            redraw = OnChar((WCHAR)wParam);
            break;

        case WM_SYSKEYDOWN:
            if (!OnSysKey(wParam)) {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
            g_lastTyped = NowSeconds();
            redraw = true;
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (!OnKeyUp(wParam)) {
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
            redraw = true;
            break;

        case WM_TIMER:
            // (Repeating, unlike the panels' own timers.)
            if (wParam == kPanelModalFramesTimerId) {
                RunIslandFrames();
                return 0;
            }
            KillTimer(hWnd, wParam);
            if (wParam == kPanelScreenshotTimerId) {
                redraw = OnAppMessage(WM_APP + 64, 0, 0);
            } else {
                redraw = OnTimer(wParam);
            }
            break;

        case WM_MOUSEMOVE:
            if (!m_tracking) {
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hWnd, 0};
                m_tracking = TrackMouseEvent(&track);
            }
            redraw = OnMouseMove(pt);
            if ((wParam & MK_LBUTTON) && EditTextDrag(hWnd, pt)) {
                redraw = true;
            }
            if (TrackScrollSpot(&pt)) {
                redraw = true;
            }
            break;

        case WM_MOUSELEAVE:
            m_tracking = false;
            redraw = OnMouseLeave();
            if (TrackScrollSpot(nullptr)) {
                redraw = true;
            }
            break;

        case WM_LBUTTONDOWN:
            g_editDrag.edit = nullptr;
            redraw = OnMouseDown(pt);
            StartEditDrag(hWnd, pt);
            break;

        case WM_LBUTTONUP:
            if (EndEditDrag(hWnd)) {
                redraw = true;
                break;
            }
            redraw = OnMouseUp(pt);
            break;

        case WM_CAPTURECHANGED:
            if ((HWND)lParam != hWnd) {
                EndEditDrag(hWnd);
                OnCaptureLost();
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);

        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            redraw = OnOtherButton(msg, pt);
            break;

        case WM_MOUSEWHEEL:
            redraw = OnWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            break;

        default:
            if (msg >= WM_APP && msg <= 0xBFFF) {
                redraw = OnAppMessage(msg, wParam, lParam);
                break;
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    if (redraw && m_open > 0) {
        if (DeferInputDraw()) {
            m_moved = true;
            StartPanelAnimation(m_owner);
        } else {
            Render();
        }
    }
    return 0;
}

float Panel::MaxHeight() const {
    const float margin = std::round(12 * m_scale);
    // (A window of its own: its height, under where the view starts.)
    if (m_windowed) {
        RECT client{};
        GetClientRect(m_hwnd, &client);
        return std::max(
            (float)(client.bottom - ContentOffset().y) - margin * 2,
            std::round(240 * m_scale));
    }
    float room;
    switch (m_direction) {
        case Direction::Down:
            room = (float)(m_workRect.bottom - m_anchor.y);
            break;
        case Direction::Up:
            room = (float)(m_anchor.y - m_workRect.top);
            break;
        default:
            room = (float)(m_workRect.bottom - m_workRect.top);
            break;
    }
    return std::max(room - margin * 2, std::round(240 * m_scale));
}

void Panel::BeginScrollArea(ID2D1RenderTarget* target,
                            const D2D1_RECT_F& viewport,
                            const SmoothScroll& scroll,
                            float opacity) {
    D2D1_MATRIX_3X2_F transform;
    target->GetTransform(&transform);
    // Faded at the edges with more beyond them: an opacity mask going from
    // transparent to opaque over a short distance.
    const float height = std::max(viewport.bottom - viewport.top, 1.0f);
    const float fade = std::min(std::round(16 * m_scale) / height, 0.45f);
    const float offset = scroll.Offset();
    const float top = offset > 0.5f ? 0.0f : 1.0f;
    const float bottom = offset < scroll.Max() - 0.5f ? 0.0f : 1.0f;
    const D2D1_GRADIENT_STOP stops[] = {
        {0, D2D1::ColorF(1, 1, 1, top)},
        {fade, D2D1::ColorF(1, 1, 1, 1)},
        {1 - fade, D2D1::ColorF(1, 1, 1, 1)},
        {1, D2D1::ColorF(1, 1, 1, bottom)},
    };
    winrt::com_ptr<ID2D1GradientStopCollection> collection;
    winrt::com_ptr<ID2D1LinearGradientBrush> mask;
    if (SUCCEEDED(target->CreateGradientStopCollection(stops, ARRAYSIZE(stops),
                                                       collection.put()))) {
        target->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties({0, viewport.top},
                                                {0, viewport.bottom}),
            collection.get(), mask.put());
    }
    winrt::com_ptr<ID2D1Layer> layer;
    target->CreateLayer(nullptr, layer.put());
    target->PushLayer(D2D1::LayerParameters(
                          viewport, nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                          D2D1::IdentityMatrix(), opacity, mask.get()),
                      layer.get());
    m_scrollAreas.push_back({layer, transform});
    target->SetTransform(
        D2D1::Matrix3x2F::Translation(0, std::round(viewport.top - offset)) *
        *D2D1::Matrix3x2F::ReinterpretBaseType(&transform));
}

void Panel::EndScrollArea(ID2D1RenderTarget* target) {
    if (m_scrollAreas.empty()) {
        return;
    }
    target->SetTransform(m_scrollAreas.back().second);
    target->PopLayer();
    m_scrollAreas.pop_back();
}

void Panel::ScrollThumb(const D2D1_RECT_F& viewport,
                        const SmoothScroll& scroll,
                        float* top,
                        float* height) const {
    const float s = m_scale;
    const float room = viewport.bottom - viewport.top;
    const float inset = std::round(3 * s);
    const float max = scroll.Max();
    *height = scroll.content > 0
                  ? std::max(std::round(28 * s),
                             (room - inset * 2) * room / scroll.content)
                  : room - inset * 2;
    const float travel = room - inset * 2 - *height;
    *top = viewport.top + inset +
           (max > 0 ? travel * std::clamp(scroll.Offset() / max, 0.0f, 1.0f)
                    : 0);
}

void Panel::DrawScrollBar(ID2D1RenderTarget* target,
                          ID2D1SolidColorBrush* brush,
                          const D2D1_RECT_F& viewport,
                          const SmoothScroll& scroll,
                          float rest,
                          float wide) {
    const float max = scroll.Max();
    const float shown = std::max(
        {(float)std::clamp(scroll.bar, 0.0, 1.0), rest, wide});
    if (max <= 0 || shown < 0.01f || scroll.content <= 0) {
        return;
    }
    const float s = m_scale;
    const float inset = std::round(3 * s);
    float top = 0;
    float thumb = 0;
    ScrollThumb(viewport, scroll, &top, &thumb);
    const float width = std::round((4 + 4 * wide) * s);
    const float right = viewport.right - std::round(3 * s);
    if (rest > 0 || wide > 0) {
        brush->SetColor(Fg(0.1f * rest + 0.06f * wide));
        target->FillRoundedRectangle(
            {{right - width, viewport.top + inset, right,
              viewport.bottom - inset},
             width / 2,
             width / 2},
            brush);
    }
    brush->SetColor(Fg(0.5f * shown + 0.25f * wide));
    target->FillRoundedRectangle(
        {{right - width, top, right, top + thumb}, width / 2, width / 2},
        brush);
}

////////////////////////////////////////////////////////////////////////////////
// The calendar, like the one in the macOS notification center: the month,
// today marked, arrows (or the mouse wheel) to change the month, and the
// agendas (.ics) in a view of their own.

