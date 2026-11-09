#pragma once
// The pill filling like a loading bar (options): an app's progress on its
// taskbar button (see PublishProgress; yellow paused, red for an error), or,
// on a laptop, the battery's charge while it charges; in the chosen color
// (FillColorRgb), moving as chosen (see DrawFill); a download that ends
// flashes with a check (done) or drains back (canceled). The main island's
// thread; every island shows it.
void Island::UpdateFill() {
    bool wanted = false;
    double target = 0;
    D2D1_COLOR_F color = RgbColor(
        FillColorRgb(g_downloadKind == kProgressKindCopy ? kFillCopies
                                                         : kFillDownloads),
        1);
    bool unknown = false;
    bool download = false;
    const double now = NowSeconds();
    const int state = TaskbarDownloadState(now);
    const int permille = g_downloadPermille;
    // (Checked again each second, see Tick.)
    if (g_settings.islandDownloadFill && state != 0 && now >= g_progressFrom) {
        wanted = true;
        download = true;
        unknown = state == kProgressIndeterminate;
        target = unknown ? 1.0 : permille / 1000.0;
        if (state == kProgressError) {
            color = D2D1::ColorF(1.0f, 0.27f, 0.23f);
        } else if (state == kProgressPaused) {
            color = D2D1::ColorF(1.0f, 0.8f, 0.0f);
        }
    } else if (g_settings.islandChargeFill) {
        SYSTEM_POWER_STATUS power{};
        // Charging, not only plugged in (a charge limit, like 80%, keeps it
        // plugged in under 100% without charging).
        if (GetSystemPowerStatus(&power) && !(power.BatteryFlag & 128) &&
            (power.BatteryFlag & 8) && power.BatteryLifePercent < 100) {
            wanted = true;
            target = power.BatteryLifePercent / 100.0;
            color = RgbColor(FillColorRgb(kFillCharge), 1);
        }
    }
    const int shownState = download ? state : 0;
    if (shownState != m_fillState) {
        m_fillState = shownState;
        m_fillStateAt = now;
    }
    // A download gone: done when it got to its end, canceled otherwise;
    // shown a moment more (see DrawFill), the charge's fill after it.
    if (download) {
        m_fillEnd = kFillEndNone;
    } else if (m_fillWanted && m_fillDownload && m_fillEnd == kFillEndNone) {
        m_fillEnd = m_fillTarget >= 0.97 && !m_fillUnknown ? kFillEndDone
                                                          : kFillEndCanceled;
        m_fillEndAt = now;
    }
    if (m_fillEnd != kFillEndNone) {
        const bool done = m_fillEnd == kFillEndDone;
        if (now - m_fillEndAt <
            AnimationSeconds(done ? kFillDoneSeconds : kFillCanceledSeconds)) {
            wanted = true;
            download = true;
            target = done ? 1.0 : 0.0;
            color = m_fillColor;
        } else {
            m_fillEnd = kFillEndNone;
        }
    }
    m_fillDownload = download;
    // Drawn again each frame while something in it moves.
    const bool moving =
        wanted &&
        (unknown || state == kProgressNormal || m_fillEnd != kFillEndNone ||
         m_fillState == kProgressPaused ||
         (m_fillState == kProgressError && now - m_fillStateAt < 0.7) ||
         g_settings.islandFillStyle != kFillStyleSimple);
    if (wanted != m_fillWanted || target != m_fillTarget ||
        moving != m_fillMoving || unknown != m_fillUnknown ||
        color.r != m_fillColor.r || color.g != m_fillColor.g ||
        color.b != m_fillColor.b) {
        m_fillWanted = wanted;
        m_fillTarget = target;
        m_fillMoving = moving;
        m_fillUnknown = unknown;
        m_fillColor = color;
        StartAnimating();
    }
}

// A notch along the screen, for the fill across them all (see
// NotchBarPlace): where it starts (on the screen), how wide, and which.
struct NotchBarPart {
    float left;
    float width;
    int notch;
};

// The notches as one bar, from the leftmost one's start to the rightmost
// one's end (see Island::DrawFill): how far along them `self` starts, and
// their lengths together.
std::pair<float, float> NotchBarPlace(const std::vector<NotchBarPart>& parts,
                                      size_t self) {
    float before = 0;
    float total = 0;
    for (const NotchBarPart& part : parts) {
        total += part.width;
        if (part.left < parts[self].left ||
            (part.left == parts[self].left && part.notch < parts[self].notch)) {
            before += part.width;
        }
    }
    return {before, total};
}

void Island::DrawFill(ID2D1RenderTarget* target,
                      ID2D1SolidColorBrush* brush,
                      const D2D1_RECT_F& rect,
                      float radius,
                      ID2D1Geometry* notch,
                      float opacity) {
    // (In the pill's own shape, a notch's too; the pill's made again only
    // when it changes, the layer Direct2D's own.)
    ID2D1Geometry* shape = notch;
    if (!shape) {
        const D2D1_ROUNDED_RECT rounded{rect, radius, radius};
        if (!m_fillShape ||
            memcmp(&rounded, &m_fillShapeRect, sizeof(rounded)) != 0) {
            m_fillShape = nullptr;
            if (FAILED(m_d2d->CreateRoundedRectangleGeometry(
                    rounded, m_fillShape.put()))) {
                return;
            }
            m_fillShapeRect = rounded;
        }
        shape = m_fillShape.get();
    }
    target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), shape),
                      nullptr);
    // Its length as drawn; the notches are one bar from the leftmost one's
    // start to the rightmost one's end: their lengths together, and how far
    // along them this one starts (in their expanded sizes).
    const float own =
        m_vertical ? rect.bottom - rect.top : rect.right - rect.left;
    float length = own;
    float before = 0;
    float total = own;
    if (m_notch >= 0 && !m_vertical && m_pill.right - m_pill.left >= 1) {
        length = m_pill.right - m_pill.left;
        std::vector<NotchBarPart> parts;
        size_t self = 0;
        for (const Island* other : NotchSet()) {
            if (other == this) {
                self = parts.size();
            }
            parts.push_back({other->m_position.x + other->m_pill.left,
                             other->m_pill.right - other->m_pill.left,
                             other->m_notch});
        }
        std::tie(before, total) = NotchBarPlace(parts, self);
    }
    // A notch's whole shape, its curves flaring out at the top (its
    // shoulders) beyond `rect`: a stretch reaching an end of it reaches
    // into them, so the fill follows its curves.
    D2D1_RECT_F bounds = rect;
    if (notch) {
        notch->GetBounds(nullptr, &bounds);
    }
    // A stretch of the whole bar (from its start), in this one.
    auto span = [&](float from, float to) {
        const float a = std::clamp((from - before) * own / length, 0.0f, own);
        const float b = std::clamp((to - before) * own / length, 0.0f, own);
        if (m_vertical) {
            return D2D1_RECT_F{rect.left, rect.top + a, rect.right, rect.top + b};
        }
        D2D1_RECT_F stretch{rect.left + a, rect.top, rect.left + b, rect.bottom};
        if (a <= 0.5f && b > a) {
            stretch.left = std::min(stretch.left, bounds.left);
        }
        if (b >= own - 0.5f && b > a) {
            stretch.right = std::max(stretch.right, bounds.right);
        }
        return stretch;
    };
    const float filled = total * (float)std::clamp(m_fillShown, 0.0, 1.0);
    const double now = NowSeconds();
    const float sc = m_scale;
    const int style = g_settings.islandFillStyle;
    // An error: a short shake as it comes.
    D2D1_MATRIX_3X2_F transform;
    target->GetTransform(&transform);
    if (m_fillState == kProgressError && now - m_fillStateAt < 0.6) {
        const double t = now - m_fillStateAt;
        const float shake = (float)(std::sin(t * 55) * (1 - t / 0.6)) *
                            std::round(4 * sc);
        target->SetTransform(D2D1::Matrix3x2F::Translation(shake, 0) *
                             transform);
    }
    // Paused: it breathes.
    const float breath =
        m_fillState == kProgressPaused && m_fillEnd == kFillEndNone
            ? 0.65f + 0.35f * (float)(0.5 + 0.5 * std::cos(now * 3.2))
            : 1.0f;
    // From its start: the left lying down, the top upright.
    const D2D1_RECT_F part = span(0, filled);
    const D2D1_COLOR_F color = m_fillColor;
    const D2D1_COLOR_F light{color.r + (1 - color.r) * 0.6f,
                             color.g + (1 - color.g) * 0.6f,
                             color.b + (1 - color.b) * 0.6f, 1};
    // Its front (where the filled part ends) in this one, lying down.
    const bool frontHere = !m_vertical && part.right > part.left + 0.5f &&
                           part.right < rect.right - 0.5f;
    const float middle = (rect.top + rect.bottom) / 2;
    const float tall = rect.bottom - rect.top;
    brush->SetColor({color.r, color.g, color.b, 0.35f * opacity * breath});
    winrt::com_ptr<ID2D1PathGeometry> wave;
    if (style == kFillStyleLiquid && frontHere &&
        SUCCEEDED(m_d2d->CreatePathGeometry(wave.put()))) {
        // A liquid's front: a wave going down it, moving.
        winrt::com_ptr<ID2D1GeometrySink> sink;
        if (SUCCEEDED(wave->Open(sink.put()))) {
            const float amplitude =
                std::min(std::round(3 * sc), (part.right - part.left) / 2);
            const int steps = 12;
            sink->BeginFigure({part.left, part.top},
                              D2D1_FIGURE_BEGIN_FILLED);
            for (int k = 0; k <= steps; k++) {
                const float y = part.top + tall * k / steps;
                const float x =
                    part.right - amplitude +
                    amplitude * (float)std::sin(k * 6.2832 / steps * 0.9 +
                                                now * 3.5);
                sink->AddLine({x, y});
            }
            sink->AddLine({part.left, part.bottom});
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            target->FillGeometry(wave.get(), brush);
        }
    } else {
        target->FillRectangle(part, brush);
    }
    // A solid line along its edge, like an app's progress under its taskbar
    // button: at the bottom lying down, on the side facing the screen
    // upright.
    const float thickness = std::max(2.0f, std::round(3 * sc));
    auto line = [&](const D2D1_RECT_F& stretch) {
        return !m_vertical ? D2D1_RECT_F{stretch.left, rect.bottom - thickness,
                                         stretch.right, rect.bottom}
               : g_islandPlace.x < 0.5
                   ? D2D1_RECT_F{rect.right - thickness, stretch.top,
                                 rect.right, stretch.bottom}
                   : D2D1_RECT_F{rect.left, stretch.top, rect.left + thickness,
                                 stretch.bottom};
    };
    brush->SetColor({color.r, color.g, color.b, 0.95f * opacity * breath});
    if (!m_fillUnknown) {
        target->FillRectangle(line(part), brush);
    }
    // Liquid: small bubbles rising in what's filled.
    if (style == kFillStyleLiquid && !m_vertical &&
        part.right - part.left > std::round(12 * sc)) {
        for (int k = 0; k < 6; k++) {
            const double seed = k * 0.618;
            const double period = 2.2 + k * 0.37;
            const double phase = std::fmod(now / period + seed, 1.0);
            const float x =
                part.left + (float)std::fmod(seed * 7.3 + 0.13, 1.0) *
                                (part.right - part.left);
            const float y = rect.bottom - tall * (float)phase;
            const float radius =
                (1.2f + (float)std::fmod(seed * 3.1, 1.0) * 1.3f) * sc;
            brush->SetColor(
                {light.r, light.g, light.b,
                 0.45f * (1 - (float)phase) * opacity * breath});
            target->FillEllipse({{x, y}, radius, radius}, brush);
        }
    }
    // Glow: a light at its front, particles left behind it.
    if (style == kFillStyleGlow && frontHere) {
        // (Its brush made again only for another color; moved, sized and
        // dimmed each frame. Kept for the island's own drawing target.)
        const bool own = target == m_canvas.target.get();
        winrt::com_ptr<ID2D1RadialGradientBrush> made;
        ID2D1RadialGradientBrush* glow = own ? m_fillGlow.get() : nullptr;
        if (!glow || memcmp(&light, &m_fillGlowColor, sizeof(light)) != 0) {
            const D2D1_GRADIENT_STOP stops[] = {
                {0, {light.r, light.g, light.b, 0.6f}},
                {1, {light.r, light.g, light.b, 0}}};
            winrt::com_ptr<ID2D1GradientStopCollection> collection;
            if (SUCCEEDED(target->CreateGradientStopCollection(
                    stops, 2, collection.put())) &&
                SUCCEEDED(target->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties({0, 0}, {0, 0}, 1, 1),
                    collection.get(), made.put()))) {
                glow = made.get();
                if (own) {
                    m_fillGlow = made;
                    m_fillGlowColor = light;
                }
            } else {
                glow = nullptr;
            }
        }
        if (glow) {
            glow->SetCenter({part.right, middle});
            glow->SetRadiusX(tall * 0.9f);
            glow->SetRadiusY(tall * 0.7f);
            glow->SetOpacity(opacity * breath);
            target->FillEllipse({{part.right, middle}, tall * 0.9f, tall * 0.7f},
                                glow);
        }
        for (int k = 0; k < 7; k++) {
            const double phase = std::fmod(now / 1.1 + k * 0.143, 1.0);
            const float x = part.right - (float)phase * tall * 2.2f;
            if (x <= part.left) {
                continue;
            }
            const float y =
                middle + (float)std::sin(k * 2.1 + now * 2) * tall * 0.25f;
            brush->SetColor({light.r, light.g, light.b,
                             0.6f * (1 - (float)phase) * opacity * breath});
            target->FillEllipse({{x, y}, 1.4f * sc, 1.4f * sc}, brush);
        }
    }
    // A light band of its color sliding along what's filled (along all the
    // notches as one), every 1.6 seconds: with the simple look, or how far
    // isn't known (the line goes with it then).
    if (m_fillEnd == kFillEndNone &&
        (m_fillUnknown ||
         (style == kFillStyleSimple && m_fillState == kProgressNormal))) {
        const float phase = (float)std::fmod(now / 1.6, 1.0);
        const float band = std::max(filled * 0.3f, std::round(24 * sc));
        const float at = -band + (filled + band) * phase;
        const D2D1_RECT_F stretch =
            span(std::max(at, 0.0f), std::min(at + band, filled));
        if (m_fillUnknown) {
            brush->SetColor({color.r, color.g, color.b, 0.95f * opacity});
            target->FillRectangle(line(stretch), brush);
        }
        brush->SetColor({light.r, light.g, light.b, 0.3f * opacity});
        target->FillRectangle(stretch, brush);
    }
    // Done: a flash over it all, and a check drawn in its middle (the
    // middle of all the notches as one).
    if (m_fillEnd == kFillEndDone) {
        const double t = now - m_fillEndAt;
        if (t < 0.45) {
            brush->SetColor({light.r, light.g, light.b,
                             0.55f * (float)(1 - t / 0.45) * opacity});
            target->FillRectangle(bounds, brush);
        }
        const D2D1_RECT_F mid = span(total / 2, total / 2);
        const bool hereMiddle = !m_vertical
                                    ? mid.left > rect.left && mid.left < rect.right
                                    : true;
        if (hereMiddle) {
            const float cx = !m_vertical ? mid.left : (rect.left + rect.right) / 2;
            const float cy = middle;
            const float drawn = (float)std::clamp(t / 0.35, 0.0, 1.0);
            const float fade =
                t < 0.8 ? 1.0f : (float)std::max(0.0, 1 - (t - 0.8) / 0.5);
            const D2D1_POINT_2F a{cx - 6 * sc, cy};
            const D2D1_POINT_2F b{cx - 2 * sc, cy + 4 * sc};
            const D2D1_POINT_2F c{cx + 6 * sc, cy - 5 * sc};
            const float first = std::min(drawn * 2, 1.0f);
            const float second = std::max(drawn * 2 - 1, 0.0f);
            brush->SetColor(D2D1::ColorF(1, 1, 1, fade * opacity));
            const float width = std::max(2.0f, std::round(2.5f * sc));
            target->DrawLine(a, {a.x + (b.x - a.x) * first,
                                 a.y + (b.y - a.y) * first},
                             brush, width);
            if (second > 0) {
                target->DrawLine(b, {b.x + (c.x - b.x) * second,
                                     b.y + (c.y - b.y) * second},
                                 brush, width);
            }
        }
    }
    target->SetTransform(transform);
    target->PopLayer();
}

// Each second while the visualizer is on: some sound on the output (its
// peak), kept a few seconds after it goes quiet (between tracks), so the
// bars come with any app's sound.
void Island::CheckAudioPlaying() {
    if (!g_settings.islandVisualizer) {
        m_audioPlaying = false;
        return;
    }
    if (!m_vizMeter && m_devices) {
        winrt::com_ptr<IMMDevice> device;
        if (SUCCEEDED(m_devices->GetDefaultAudioEndpoint(eRender, eMultimedia,
                                                         device.put()))) {
            device->Activate(kIID_IAudioMeterInformation, CLSCTX_ALL, nullptr,
                             m_vizMeter.put_void());
        }
    }
    float peak = 0;
    if (m_vizMeter && FAILED(m_vizMeter->GetPeakValue(&peak))) {
        m_vizMeter = nullptr;
    }
    const double now = NowSeconds();
    if (peak > kVizSilentPeak) {
        m_audioHeardAt = now;
    }
    const bool playing = now - m_audioHeardAt < 3;
    if (playing != m_audioPlaying) {
        m_audioPlaying = playing;
        Relayout();
    }
    // The playing app's sessions, looked for here rather than in a frame:
    // for another app, and every few seconds while none of them is heard
    // (a session that came since, like a browser's for another tab).
    if (m_vizRunning &&
        (m_mediaShown.appId != m_vizApp ||
         (now >= m_vizLookAt && now - m_vizSessionHeardAt > 1))) {
        m_vizApp = m_mediaShown.appId;
        m_vizLookAt = now + 3;
        m_vizSessionMeters = FindSessionMeters(m_vizApp);
    }
}

// The battery charge's fill moves (its wave, its glow) a frame every
// kChargeFillMs, while shown (not minimized or hidden); a download's every
// frame (see AnimationFrame).
void Island::UpdateFillTimer() {
    const bool running = m_fillMoving && !m_fillDownload &&
                         m_fillAmount > 0.01 && !m_minimized && !m_away;
    if (running == m_chargeFillRunning) {
        return;
    }
    m_chargeFillRunning = running;
    if (running) {
        SetTimer(m_hwnd, kChargeFillTimerId, kChargeFillMs, nullptr);
    } else {
        KillTimer(m_hwnd, kChargeFillTimerId);
    }
}

// The dragon's eyes (islandEyes, see EyesState). The frames run only while
// something moves (StepEyes says so, see AnimationFrame); this timer, every
// kEyesWatchMs, only looks whether the mouse came near or a blink or a glance
// is due, and wakes them for it. It runs only while the eyes are on the pill
// and seen (not minimized, nor away over a full-screen app, nor with the
// animations off), and it's gone with them.
const IslandSlot* Island::EyesSlot() const {
    for (const auto& slot : m_slots) {
        if (slot.item == IslandItem::Eyes) {
            return &slot;
        }
    }
    return nullptr;
}

// What excites them: a notification waiting unseen, or a banner.
bool Island::EyesAlerted() const {
    return m_unseen || m_bannerAmount > 0.5;
}

// What the eyes see now: the mouse, from each of them.
EyesView Island::EyesLook(const IslandSlot& slot) const {
    EyesView view;
    POINT cursor{};
    if (!GetCursorPos(&cursor) || !ScreenToClient(m_hwnd, &cursor)) {
        return view;
    }
    const EyeShape& shape = ChosenEyeShape();
    const float unit = EyesUnit(slot.rect, shape);
    const float middleX = (slot.rect.left + slot.rect.right) / 2;
    const float middleY = (slot.rect.top + slot.rect.bottom) / 2;
    const double away = std::hypot((double)cursor.x - middleX,
                                   (double)cursor.y - middleY);
    const double radius = kEyesRadius * (double)m_scale;
    if (away > radius) {
        return view;
    }
    view.nearby = true;
    view.distance = away / m_scale;
    view.closeness = std::clamp(1 - away / radius, 0.0, 1.0);
    for (int e = 0; e < 2; e++) {
        const float x =
            middleX + (e == 0 ? -1.0f : 1.0f) * (shape.rx + 4) * unit;
        view.dx[e] = cursor.x - x;
        view.dy[e] = cursor.y - middleY;
    }
    return view;
}

void Island::UpdateEyesTimer() {
    const bool shown = m_hwnd && !m_away && !m_minimized && !m_barHidden &&
                       AnimationLevel() < 2 && EyesSlot();
    if (shown == m_eyesRunning) {
        return;
    }
    m_eyesRunning = shown;
    // (Shown again, they start anew: open, looking ahead, nothing waiting to
    // excite them but what comes now.)
    m_eyes.Reset();
    if (shown) {
        m_eyesNotified = EyesAlerted();
        m_eyesMoving = true;
        m_eyesSince = kEyesFrameSeconds;
        SetTimer(m_hwnd, kEyesTimerId, kEyesWatchMs, nullptr);
        StartAnimating();
    } else {
        m_eyesMoving = false;
        if (m_hwnd) {
            KillTimer(m_hwnd, kEyesTimerId);
        }
    }
}

// A step of the eyes every frame they're awake (see AnimationFrame), at most
// kEyesFrameSeconds apart: true while something moves, so the frames go on.
bool Island::StepEyes(double dt) {
    m_eyesSince += dt;
    if (m_eyesSince < kEyesFrameSeconds) {
        return m_eyesMoving;
    }
    const double elapsed = std::min(m_eyesSince, 0.1);
    m_eyesSince = 0;
    const IslandSlot* slot = EyesSlot();
    if (!slot) {
        m_eyesMoving = false;
        return false;
    }
    EyesView view = EyesLook(*slot);
    const bool alerted = EyesAlerted();
    view.excite = alerted && !m_eyesNotified;
    m_eyesNotified = alerted;
    m_eyesMoving =
        m_eyes.Step(NowSeconds(), elapsed, view, ChosenEyeShape());
    return m_eyesMoving;
}

void Island::OnEyesTimer() {
    const IslandSlot* slot = m_eyesRunning ? EyesSlot() : nullptr;
    if (!slot) {
        return;
    }
    if (m_eyesMoving) {
        // (The frames look after them; woken again if they somehow stopped.)
        if (!Animating()) {
            StartAnimating();
        }
        return;
    }
    bool wake = m_eyes.Due(NowSeconds()) || EyesAlerted() != m_eyesNotified;
    POINT cursor{};
    if (!wake && GetCursorPos(&cursor) &&
        (cursor.x != m_eyesCursor.x || cursor.y != m_eyesCursor.y)) {
        // The mouse moved: only matters near them (or just away from them,
        // the pupils going back to the middle).
        m_eyesCursor = cursor;
        wake = m_eyes.WasNear() || EyesLook(*slot).nearby;
    }
    if (wake) {
        m_eyesMoving = true;
        m_eyesSince = kEyesFrameSeconds;
        StartAnimating();
    }
}

// A click on them: a blink.
void Island::BlinkEyes() {
    if (!m_eyesRunning) {
        return;
    }
    m_eyes.Blink();
    m_eyesMoving = true;
    m_eyesSince = kEyesFrameSeconds;
    StartAnimating();
}

// Its bars moving each frame (see AnimationFrame) while they're on the pill
// (something playing) and it's seen (not minimized, nor away over a
// full-screen app): no event tells the sound's level, so only then.
void Island::UpdateVisualizerTimer() {
    bool shown = false;
    if (!m_away && !m_minimized) {
        for (const auto& slot : m_slots) {
            shown = shown || slot.item == IslandItem::Visualizer;
        }
    }
    if (shown == m_vizRunning) {
        return;
    }
    m_vizRunning = shown;
    if (shown) {
        m_vizClock = 0;
        StartAnimating();
    } else {
        m_vizSessionMeters.clear();
        m_vizApp.clear();
        m_vizLevels = {};
        m_vizTargets = {};
        m_vizKicks = {};
        m_vizFast = m_vizSlow = 0;
    }
}

// The playing app's own sound when it's heard (its loudest audio session's
// meter: the output's mixes in every app, and a quiet track under a loud
// browser barely moved the bars), else the output's (the app paused or
// quiet while another plays, or its session not found). Each bar moves on
// its own, as high as the sound is loud (its level: up fast, down slowly),
// resting low and still when it's quiet: with the beat (the level jumping
// over its average gives each bar another speed and kicks some of them up),
// or smoothly like the iPhone's (islandVisualizerSmooth: two waves of its
// own, the middle ones higher). The level follows the loudest of the last
// seconds (a quiet song moves them as much as a loud one), times the
// sensitivity chosen (islandVizSensitivity). The sound is read at a steady
// pace (kVizStepSeconds); the bars follow it as drawn, at most
// kVizFrameSeconds apart (a wide window drawn on a fast screen costs).
void Island::StepVisualizer(double dt) {
    m_vizClock += dt;
    // (A frame late: one reading, not all the ones missed.)
    if (m_vizClock >= kVizStepSeconds) {
        m_vizClock = std::fmod(m_vizClock, kVizStepSeconds);
        ReadVisualizerSound();
    }
    m_vizSinceShown += dt;
    if (m_vizSinceShown < kVizFrameSeconds) {
        return;
    }
    // (The rates as they were for a reading's time, for this frame's.)
    const double steps = m_vizSinceShown / kVizStepSeconds;
    m_vizSinceShown = 0;
    const bool smooth = g_settings.islandVisualizerSmooth;
    const float up = 1 - (float)std::pow(1 - (smooth ? 0.35 : 0.55), steps);
    const float down = 1 - (float)std::pow(1 - (smooth ? 0.18 : 0.25), steps);
    for (size_t i = 0; i < m_vizLevels.size(); i++) {
        float& level = m_vizLevels[i];
        const float target = m_vizTargets[i];
        level += (target - level) * (target > level ? up : down);
    }
}

void Island::ReadVisualizerSound() {
    const double now = NowSeconds();
    // (Another app's: its sessions are looked for on the next second.)
    if (m_mediaShown.appId != m_vizApp) {
        m_vizSessionMeters.clear();
    }
    float peak = 0;
    std::erase_if(m_vizSessionMeters, [&](const auto& meter) {
        float level = 0;
        if (FAILED(meter->GetPeakValue(&level))) {
            return true;
        }
        peak = std::max(peak, level);
        return false;
    });
    if (peak > kVizSilentPeak) {
        m_vizSessionHeardAt = now;
    } else if (m_vizMeter && FAILED(m_vizMeter->GetPeakValue(&peak))) {
        // (Made again on the next second, see CheckAudioPlaying.)
        m_vizMeter = nullptr;
        peak = 0;
    }
    // (The loudest falls back by half in about 4 seconds.)
    m_vizLoudest = std::max({peak, m_vizLoudest * 0.995f, 0.05f});
    const float gain =
        g_settings.islandVizSensitivity / 100.0f / m_vizLoudest;
    const float sound = std::clamp(peak * gain, 0.0f, 1.2f);
    if (g_settings.islandVisualizerSmooth) {
        m_vizFast += (sound - m_vizFast) * (sound > m_vizFast ? 0.5f : 0.12f);
        for (size_t i = 0; i < kVizBars; i++) {
            // (Not wrapped: the second wave isn't a whole turn of the first.)
            m_vizPhases[i] += kVizStepSeconds * kVizSpeeds[i];
            const float wave =
                0.5f + 0.3f * (float)std::sin(m_vizPhases[i]) +
                0.2f * (float)std::sin(m_vizPhases[i] * 1.618 + (double)i);
            m_vizTargets[i] = std::clamp(
                m_vizFast * kVizShape[i] * (0.35f + 0.65f * wave), 0.0f, 1.0f);
        }
        return;
    }
    m_vizFast += (sound - m_vizFast) * (sound > m_vizFast ? 0.65f : 0.2f);
    m_vizSlow += (m_vizFast - m_vizSlow) * 0.04f;
    const bool beat =
        m_vizFast > m_vizSlow * 1.25f + 0.05f && now - m_vizBeatAt > 0.18;
    if (beat) {
        m_vizBeatAt = now;
        m_vizBeats++;
    }
    for (size_t i = 0; i < kVizBars; i++) {
        // (A number of its own each beat, 0 to 1: the same for the same
        // beat.)
        auto noise = [&](int salt) {
            const double n =
                std::sin((m_vizBeats * 7 + (int)i * 3 + salt) * 12.9898) *
                43758.5453;
            return (float)(n - std::floor(n));
        };
        if (beat) {
            m_vizSpeeds[i] = 5 + 9 * noise(1);
            if (noise(2) > 0.4f) {
                m_vizKicks[i] = 0.35f + 0.45f * noise(3);
            }
        }
        m_vizPhases[i] =
            std::fmod(m_vizPhases[i] + kVizStepSeconds * m_vizSpeeds[i],
                      2 * 3.14159265);
        m_vizKicks[i] *= 0.82f;
        const float wave = 0.5f + 0.5f * (float)std::sin(m_vizPhases[i]);
        m_vizTargets[i] = std::clamp(
            m_vizFast * (0.18f + 0.82f * wave + m_vizKicks[i]), 0.0f, 1.0f);
    }
}

// The meters of the audio sessions of the app playing (by its program's
// name in the media app's ID: "Spotify.exe", "Chrome", a packaged app's
// "...!Spotify"), on the default output: all of them (a browser has one for
// each tab that made a sound, most of them quiet); none if it has none
// there.
std::vector<winrt::com_ptr<IAudioMeterInformation>> Island::FindSessionMeters(
    const std::wstring& appId) {
    std::vector<winrt::com_ptr<IAudioMeterInformation>> meters;
    if (appId.empty() || !m_devices) {
        return meters;
    }
    std::wstring wanted = appId;
    std::transform(wanted.begin(), wanted.end(), wanted.begin(), ::towlower);
    winrt::com_ptr<IMMDevice> device;
    winrt::com_ptr<IAudioSessionManager2> manager;
    winrt::com_ptr<IAudioSessionEnumerator> sessions;
    if (FAILED(m_devices->GetDefaultAudioEndpoint(eRender, eMultimedia,
                                                  device.put())) ||
        FAILED(device->Activate(kIID_IAudioSessionManager2, CLSCTX_ALL,
                                nullptr, manager.put_void())) ||
        FAILED(manager->GetSessionEnumerator(sessions.put()))) {
        return meters;
    }
    int count = 0;
    sessions->GetCount(&count);
    for (int i = 0; i < count; i++) {
        winrt::com_ptr<IAudioSessionControl> control;
        AudioSessionState state = AudioSessionStateExpired;
        if (FAILED(sessions->GetSession(i, control.put())) ||
            FAILED(control->GetState(&state)) ||
            state == AudioSessionStateExpired) {
            continue;
        }
        auto control2 = control.try_as<IAudioSessionControl2>();
        DWORD processId = 0;
        if (!control2 || control2->IsSystemSoundsSession() == S_OK ||
            FAILED(control2->GetProcessId(&processId)) || !processId) {
            continue;
        }
        // (Its file's name, without ".exe".)
        const std::wstring path = ProcessImagePath(processId);
        const size_t slash = path.find_last_of(L'\\');
        std::wstring program =
            slash == std::wstring::npos ? path : path.substr(slash + 1);
        std::transform(program.begin(), program.end(), program.begin(),
                       ::towlower);
        if (program.size() > 4 && program.ends_with(L".exe")) {
            program.resize(program.size() - 4);
        }
        if (program.empty() || wanted.find(program) == std::wstring::npos) {
            continue;
        }
        winrt::com_ptr<IAudioMeterInformation> meter;
        if (SUCCEEDED(control->QueryInterface(kIID_IAudioMeterInformation,
                                              meter.put_void()))) {
            meters.push_back(std::move(meter));
        }
    }
    return meters;
}

// The last minute the alarms were looked at (days * 24 * 60 + the minute,
// local time), kept while the island restarts: every minute since then is
// looked at, so one skipped (the island starting again across it, a busy
// moment, the clock going forward) still rings; a minute again (the clock
// going back) doesn't ring twice. -1 before the first look, which only
// notes the minute (an island starting at 7:00 doesn't ring 7:00 again).
long long g_alarmsMinute = -1;
// The most minutes looked at once (after a sleep: not every alarm of the
// night; an hour, so the clock going forward still rings its own).
constexpr long long kAlarmsMostMinutes = 90;

// Whether an alarm falls in the minutes `from` to `to` (keys like
// g_alarmsMinute's), on one of its days (`today` and `weekday`: the day of
// `to`; no days: any).
bool AlarmDueOn(const Alarm& alarm,
                long long from,
                long long to,
                long long today,
                int weekday) {
    for (long long key = from; key <= to; key++) {
        if (alarm.minute != (int)(key % (24 * 60))) {
            continue;
        }
        const int day =
            (int)(((weekday + key / (24 * 60) - today) % 7 + 7) % 7);
        if (!alarm.days || (alarm.days >> day & 1)) {
            return true;
        }
    }
    return false;
}

