#pragma once
// The dragon's eyes (islandEyes): two eyes on the pill that follow the mouse
// when it's near, blink, glance around on their own and get excited with a
// new notification. They're drawn in the units of a 50 x 50 box (the design's,
// design/ToothlessEye.html): an eye's shape in them (kEyeShapes), its pupil
// and look (EyesState), all scaled to the pill when drawn (see DrawEyes).
//
// They cost nothing while they rest: a timer (kEyesTimerId) only asks every
// kEyesWatchMs whether the mouse came near or something is due (a blink, a
// glance), and frames run only while something moves, and then at most
// 1 / kEyesFrameSeconds times a second (see Island::StepEyes and
// Island::OnEyesTimer). Not at all while the pill isn't seen, or with the
// animations off (the eyes stay open, looking ahead; see
// Island::UpdateEyesTimer).
constexpr UINT_PTR kEyesTimerId = 62;
constexpr UINT kEyesWatchMs = 80;
constexpr double kEyesFrameSeconds = 1.0 / 30;
// How far from the middle of the eyes the mouse is seen, in pixels at 100%.
constexpr float kEyesRadius = 280;
// How long a new notification keeps them excited, and how long the pupils
// stay as they were once what held them is over.
constexpr double kEyesExcitedSeconds = 1.6;
constexpr double kEyesRelaxSeconds = 2.0;
// How far a pupil goes from the middle, at most, in the units.
constexpr double kEyesMaxLook = 9;
// A pupil at rest (its width and height, in the units).
constexpr double kEyePupilWidth = 3.5;
constexpr double kEyePupilHeight = 14;
// An eye's unit as a part of the pill's height.
constexpr float kEyesUnitOfHeight = 0.0141f;

struct EyeTheme {
    // The iris, from its bright middle to its edge (0xRRGGBB).
    int stops[4];
    int socket;
    int pupil;
    int lid;
};

// (kChoiceEyeTheme's options, in this order.)
constexpr EyeTheme kEyeThemes[] = {
    {{0xB5FF36, 0x10B981, 0x047857, 0x022C22}, 0x020617, 0x020617, 0x05070A},
    {{0xD8FBFF, 0x38BDF8, 0x0369A1, 0x082F49}, 0x020617, 0x020617, 0x061827},
    {{0xFFF1A8, 0xF59E0B, 0xC2410C, 0x431407}, 0x1C1008, 0x160B05, 0x160B05},
    {{0xF5D0FE, 0xC084FC, 0x7E22CE, 0x2E1065}, 0x10051D, 0x090313, 0x090313},
    {{0xFECACA, 0xEF4444, 0x991B1B, 0x450A0A}, 0x180606, 0x100303, 0x100303},
    {{0xFFFFFF, 0xA5F3FC, 0x0891B2, 0x164E63}, 0x03151B, 0x020B10, 0x020B10},
};

struct EyeShape {
    // The eye's half width and half height, in the units.
    float rx;
    float ry;
    // A soft triangle, not an ellipse.
    bool triangle;
};

// (kChoiceEyeShape's options, in this order.)
constexpr EyeShape kEyeShapes[] = {
    {16, 22, false},
    {20, 20, false},
    {22, 14, false},
    {23, 12, false},
    {22, 19, true},
};

const EyeTheme& ChosenEyeTheme() {
    return kEyeThemes[std::clamp(g_settings.islandEyeTheme, 0,
                                 (int)std::size(kEyeThemes) - 1)];
}

const EyeShape& ChosenEyeShape() {
    return kEyeShapes[std::clamp(g_settings.islandEyeShape, 0,
                                 (int)std::size(kEyeShapes) - 1)];
}

// The eyes' room across the pill (the two eyes and the gap between them).
float EyesWidth(const EyeShape& shape, float height) {
    return std::round((4 * shape.rx + 10) * height * kEyesUnitOfHeight);
}

// What an eye's unit is in pixels in their room (what fits, across and
// along it: the pill upright has them the other way round).
float EyesUnit(const D2D1_RECT_F& rect, const EyeShape& shape) {
    const float width = rect.right - rect.left;
    const float height = rect.bottom - rect.top;
    return std::min(width / (4 * shape.rx + 10),
                    std::min(width, height) * kEyesUnitOfHeight);
}

// What the eyes see in a step.
struct EyesView {
    // The mouse is within the radius.
    bool nearby = false;
    // How far it is from the middle of the eyes (in pixels at 100%), and how
    // close of the radius (0 at its edge, 1 on them).
    double distance = 0;
    double closeness = 0;
    // From each eye to the mouse, in pixels.
    double dx[2]{};
    double dy[2]{};
    // A notification came now.
    bool excite = false;
};

// The eyes' state: where each pupil looks, how wide and tall they are, the
// lid of a blink, the wobble of excitement, and when the next blink and
// glance are due. All of it moves with SpringTowards, so the animations'
// level and speed apply (see Step).
struct EyesState {
    double pupilX[2]{};
    double pupilY[2]{};
    double velocityX[2]{};
    double velocityY[2]{};
    double width = kEyePupilWidth;
    double widthVelocity = 0;
    double height = kEyePupilHeight;
    double heightVelocity = 0;
    double targetWidth = kEyePupilWidth;
    double targetHeight = kEyePupilHeight;
    // How closed the lid is: 0 open, 1 closed.
    double lid = 0;
    // The wobble's strength (see Wobble).
    double bounce = 0;
    double bounceVelocity = 0;
    bool blinking = false;
    double blinkTime = 0;
    double blinkAt = 0;
    double glanceAt = 0;
    // A glance going on: until when, where to, and how much less the other
    // eye goes.
    double glanceUntil = 0;
    double glanceX = 0;
    double glanceY = 0;
    double glanceSkew = 1;
    // When the pupils go back to rest, once what held them is over.
    double relaxAt = 0;
    double excitedFrom = 0;
    double excitedUntil = 0;
    bool wasExcited = false;
    bool wasNear = false;
    // The blink and glance times planned (they are at the first step).
    bool planned = false;
    uint64_t seed = 0x9E3779B97F4A7C15ull;

    // Back to rest, the plans forgotten (the eyes shown again start anew).
    void Reset() {
        const uint64_t kept = seed;
        *this = EyesState();
        seed = kept;
    }

    double Random() {
        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        return (double)((seed * 0x2545F4914F6CDD1Dull) >> 11) /
               9007199254740992.0;
    }

    void Plan(double now) {
        seed ^= (uint64_t)(now * 1000000.0) | 1;
        blinkAt = now + 1.8 + Random() * 3.5;
        glanceAt = now + 3.0 + Random() * 4.0;
        planned = true;
    }

    bool WasNear() const { return wasNear; }

    // Whether a step has something to do (a blink, a glance, the pupils
    // going back to rest), so the watch timer wakes the frames for it.
    bool Due(double now) const {
        return !planned || now >= blinkAt || now >= glanceAt ||
               (relaxAt > 0 && now >= relaxAt) ||
               (glanceUntil > 0 && now >= glanceUntil) ||
               (wasExcited && now >= excitedUntil);
    }

    // A blink now (a click on them).
    void Blink() {
        if (!blinking && AnimationLevel() < 2) {
            blinking = true;
            blinkTime = 0;
        }
    }

    // The lid in a blink's time: two blinks, each closing, holding and
    // opening (the second a little quicker). -1 once it's over.
    static double BlinkLid(double t) {
        struct Stage {
            double close;
            double hold;
            double open;
        };
        constexpr Stage first{0.08, 0.05, 0.12};
        constexpr Stage second{0.07, 0.05, 0.10};
        constexpr double gap = 0.17;
        auto one = [](const Stage& stage, double u) {
            if (u < stage.close) {
                return EaseInOutCubic(u / stage.close);
            }
            u -= stage.close;
            if (u < stage.hold) {
                return 1.0;
            }
            u -= stage.hold;
            if (u < stage.open) {
                const double rest = 1 - u / stage.open;
                return rest * rest * rest;
            }
            return -1.0;
        };
        const double firstEnd = first.close + first.hold + first.open;
        if (t < firstEnd) {
            return one(first, t);
        }
        t -= firstEnd;
        if (t < gap) {
            return 0;
        }
        return one(second, t - gap);
    }

    void StartGlance(double now, const EyeShape& shape) {
        const double angle = Random() * 6.283185307179586;
        const double reach = std::min(
            kEyesMaxLook, 0.6 * std::min<double>(shape.rx, shape.ry));
        const double span = std::min(reach, 4.5 + Random() * 3.5);
        glanceX = std::cos(angle) * span;
        glanceY = std::sin(angle) * span * 0.8;
        glanceSkew = 0.94 + Random() * 0.12;
        glanceUntil = now + 0.9 + Random() * 0.8;
        targetWidth = 8;
        targetHeight = 12;
        relaxAt = 0;
    }

    // A spring's tail takes long to die out, far past what shows (an eighth
    // of a unit, see Signature): close enough and slow enough, it's there.
    static bool Settled(bool moving,
                        double& value,
                        double& velocity,
                        double target) {
        if (moving && std::fabs(target - value) < 0.05 &&
            std::fabs(velocity) < 0.5) {
            value = target;
            velocity = 0;
            return false;
        }
        return moving;
    }

    // One step of `dt` seconds (already the animations' speed) at `now`.
    // Returns true while something moves: the frames go on until it
    // doesn't (and the watch timer wakes them for what comes next).
    bool Step(double now, double dt, const EyesView& view,
              const EyeShape& shape) {
        const int level = AnimationLevel();
        const bool still = level >= 2;
        if (!planned) {
            Plan(now);
        }
        bool excited = false;
        if (still) {
            // (Open, looking ahead.)
            blinking = false;
            glanceUntil = 0;
            relaxAt = 0;
            excitedUntil = 0;
            wasExcited = false;
            wasNear = false;
            targetWidth = kEyePupilWidth;
            targetHeight = kEyePupilHeight;
        } else {
            if (view.excite) {
                excitedFrom = now;
                excitedUntil = now + kEyesExcitedSeconds;
                glanceUntil = 0;
                relaxAt = 0;
            }
            excited = now < excitedUntil;
            if (excited) {
                wasExcited = true;
                targetWidth = 12;
                targetHeight = 16;
            } else if (wasExcited) {
                wasExcited = false;
                targetWidth = kEyePupilWidth;
                targetHeight = kEyePupilHeight;
            }
            if (view.nearby && !excited) {
                // Looking at the mouse: the closer, the wider the pupils.
                wasNear = true;
                relaxAt = 0;
                glanceUntil = 0;
                targetWidth = kEyePupilWidth + view.closeness * 4.5;
                targetHeight = kEyePupilHeight - view.closeness * 2;
            } else if (wasNear) {
                wasNear = false;
                relaxAt = now + kEyesRelaxSeconds;
            }
            if (glanceUntil > 0 && now >= glanceUntil) {
                glanceUntil = 0;
                relaxAt = now + kEyesRelaxSeconds;
            }
            if (relaxAt > 0 && now >= relaxAt) {
                relaxAt = 0;
                if (!excited && !view.nearby && glanceUntil == 0) {
                    targetWidth = kEyePupilWidth;
                    targetHeight = kEyePupilHeight;
                }
            }
            // A glance now and then, on its own, if nothing holds the eyes.
            if (now >= glanceAt) {
                glanceAt = now + 5.5 + Random() * 5.5;
                if (!excited && !view.nearby && glanceUntil == 0 &&
                    Random() < 0.78) {
                    StartGlance(now, shape);
                }
            }
            if (!blinking && now >= blinkAt) {
                blinkAt = now + 1.8 + Random() * 3.5;
                if (!excited) {
                    blinking = true;
                    blinkTime = 0;
                }
            }
        }

        // Where each pupil goes.
        double goX[2]{};
        double goY[2]{};
        if (!still && !excited) {
            if (view.nearby) {
                const double reach =
                    std::min(view.distance / 16.0,
                             std::min(kEyesMaxLook,
                                      0.6 * std::min<double>(shape.rx,
                                                             shape.ry)));
                for (int e = 0; e < 2; e++) {
                    const double length = std::hypot(view.dx[e], view.dy[e]);
                    if (length > 0.001) {
                        goX[e] = view.dx[e] / length * reach;
                        goY[e] = view.dy[e] / length * reach;
                    }
                }
            } else if (glanceUntil > 0) {
                goX[0] = glanceX;
                goY[0] = glanceY;
                goX[1] = glanceX * glanceSkew;
                goY[1] = glanceY * glanceSkew;
            }
        }
        bool moving = false;
        for (int e = 0; e < 2; e++) {
            if (Settled(SpringTowards(pupilX[e], velocityX[e], goX[e], 0.25, dt),
                        pupilX[e], velocityX[e], goX[e])) {
                moving = true;
            }
            if (Settled(SpringTowards(pupilY[e], velocityY[e], goY[e], 0.25, dt),
                        pupilY[e], velocityY[e], goY[e])) {
                moving = true;
            }
        }
        if (Settled(SpringTowards(width, widthVelocity, targetWidth, 0.3, dt),
                    width, widthVelocity, targetWidth)) {
            moving = true;
        }
        if (Settled(SpringTowards(height, heightVelocity, targetHeight, 0.3, dt),
                    height, heightVelocity, targetHeight)) {
            moving = true;
        }

        // The blink.
        if (blinking) {
            blinkTime += dt;
            lid = BlinkLid(blinkTime);
            if (lid < 0) {
                lid = 0;
                blinking = false;
            } else {
                moving = true;
            }
        } else {
            lid = 0;
        }

        // The wobble of excitement: a decoration, so not from the "partial"
        // level on (the eyes only open their pupils wide).
        const bool bubbly = excited && level == 0;
        if (Settled(SpringTowards(bounce, bounceVelocity, bubbly ? 1 : 0, 0.2,
                                  dt),
                    bounce, bounceVelocity, bubbly ? 1 : 0)) {
            moving = true;
        }
        if (bubbly) {
            moving = true;
        }
        return moving;
    }

    // The wobble at `now`: how much the eyes lift (in the units) and widen
    // and flatten (1: not).
    void Wobble(double now, float* lift, float* wide, float* tall) const {
        *lift = 0;
        *wide = 1;
        *tall = 1;
        if (bounce <= 0.001) {
            return;
        }
        static constexpr double kLift[] = {0, -6, 0, -3, 0};
        static constexpr double kWide[] = {1, 1.15, 1, 0.95, 1};
        static constexpr double kTall[] = {1, 0.9, 1, 1.05, 1};
        const double phase =
            std::fmod(std::max(0.0, now - excitedFrom), 0.35) / 0.35;
        const int part = std::min(3, (int)(phase * 4));
        const double t = phase * 4 - part;
        const double ease = t * t * (3 - 2 * t);
        *lift = (float)(bounce * (kLift[part] +
                                  (kLift[part + 1] - kLift[part]) * ease));
        *wide = (float)(1 + bounce * (kWide[part] - 1 +
                                      (kWide[part + 1] - kWide[part]) * ease));
        *tall = (float)(1 + bounce * (kTall[part] - 1 +
                                      (kTall[part + 1] - kTall[part]) * ease));
    }

    // What is drawn, as a key: nothing is drawn again while it's the same.
    std::wstring Signature(double now) const {
        float lift, wide, tall;
        Wobble(now, &lift, &wide, &tall);
        std::wstring key = L"y";
        for (int e = 0; e < 2; e++) {
            key += std::to_wstring((int)std::lround(pupilX[e] * 8)) + L"," +
                   std::to_wstring((int)std::lround(pupilY[e] * 8)) + L",";
        }
        key += std::to_wstring((int)std::lround(width * 8)) + L"," +
               std::to_wstring((int)std::lround(height * 8)) + L"," +
               std::to_wstring((int)std::lround(lid * 50)) + L"," +
               std::to_wstring((int)std::lround(lift * 8)) + L"," +
               std::to_wstring((int)std::lround(wide * 50)) + L"," +
               std::to_wstring((int)std::lround(tall * 50)) + L"," +
               std::to_wstring(g_settings.islandEyeTheme) + L"," +
               std::to_wstring(g_settings.islandEyeShape) + L"|";
        return key;
    }
};
// The next line of a song's lyrics (see Island::StepLyrics).
constexpr UINT_PTR kLyricsTimerId = 54;
// What's new's notice, a moment after the island starts (see
// Island::StartWhatsNewNotice); on a first install, the setup assistant
// instead.
constexpr UINT_PTR kWhatsNewTimerId = 55;
constexpr UINT kWhatsNewDelayMs = 10000;
// The battery charge's fill moving (see Island::UpdateFillTimer): a frame
// this often, not every one (it changes over minutes).
constexpr UINT_PTR kChargeFillTimerId = 56;
constexpr UINT kChargeFillMs = 66;
// An alarm's sound, over and over until its Stop, at most this long (like a
// call's, see kUrgentSoundMaxMs).
constexpr UINT_PTR kAlarmSoundStopTimerId = 57;
// The window in front wasn't an app's (the taskbar, one not shown yet):
// looked at once more a moment later, for the places per app (#6692).
constexpr UINT_PTR kPlaceRecheckTimerId = 60;
constexpr UINT kPlaceRecheckMs = 400;
// The app in front looked at for its profile once it stayed a moment (see
// CheckAutoProfile).
constexpr UINT_PTR kAutoProfileTimerId = 61;
constexpr UINT kAutoProfileWaitMs = 700;
constexpr UINT kGlassRenewMs[] = {15000, 30000, 75000};
constexpr UINT kUrgentSoundMaxMs = 60000;
constexpr UINT kPinDelayMs = 2000;
constexpr double kPinGrowSeconds = 0.22;
// What's playing stays on the pill at least this long, and this long after it
// stops (seconds).
constexpr double kMediaMinShown = 2.0;
// The title on the pill: a short one gets its artist next to it (so the
// island doesn't shrink to a sliver), and a long one is cut (with "...")
// past the width chosen (see islandMediaTitleLimit).
constexpr float kMediaTitleShortWidth = 90;
constexpr float kMediaTitleMinWidth = 60;
constexpr double kMediaLinger = 1.5;
// The wheel on what's playing changing the song: at most once in this long
// (a touchpad sends many small steps).
constexpr double kMediaWheelSeconds = 0.4;
// The cloaking and uncloaking events (not in older headers).
constexpr DWORD kEventObjectCloaked = 0x8017;
constexpr DWORD kEventObjectUncloaked = 0x8018;

bool PointInRect(POINT pt, const D2D1_RECT_F& rect) {
    return pt.x >= rect.left && pt.x < rect.right && pt.y >= rect.top &&
           pt.y < rect.bottom;
}

// Text on lines `lineHeight` apart, wrapping at `width` (long words too),
// from the top; past `height`, the last line ends with "..." (the format's
// trimming).
winrt::com_ptr<IDWriteTextLayout> WrappedLayout(IDWriteFactory* dwrite,
                                                IDWriteTextFormat* format,
                                                const std::wstring& text,
                                                float width,
                                                float height,
                                                float lineHeight) {
    winrt::com_ptr<IDWriteTextLayout> layout;
    if (!dwrite || !format ||
        FAILED(dwrite->CreateTextLayout(text.c_str(), (UINT32)text.size(),
                                        format, std::max(width, 1.0f),
                                        std::max(height, 1.0f),
                                        layout.put()))) {
        return nullptr;
    }
    layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    layout->SetWordWrapping(DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);
    layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    layout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, lineHeight,
                           lineHeight * 0.8f);
    return layout;
}

// How many lines that text takes (at least one).
int WrappedLineCount(IDWriteFactory* dwrite,
                     IDWriteTextFormat* format,
                     const std::wstring& text,
                     float width,
                     float lineHeight) {
    DWRITE_TEXT_METRICS metrics{};
    auto layout =
        WrappedLayout(dwrite, format, text, width, 100000, lineHeight);
    if (!layout || FAILED(layout->GetMetrics(&metrics))) {
        return 1;
    }
    return std::max(1, (int)metrics.lineCount);
}

// The text boxes' caret: blue, steady while typing (or just clicked), then
// blinking softly, fading in and out, for a few seconds (kCaretBlinkSeconds:
// four blinks), then steady again, so nothing is drawn while nothing happens.
double g_lastTyped;
constexpr double kCaretBlinkSeconds = 5;

bool CaretBlinking() {
    return NowSeconds() - g_lastTyped < kCaretBlinkSeconds;
}

D2D1_COLOR_F CaretColor(float opacity = 1) {
    const double since = NowSeconds() - g_lastTyped;
    const double shown =
        since < 0.6 || since >= kCaretBlinkSeconds
            ? 1.0
            : 0.5 + 0.5 * std::cos((since - 0.6) * 2 * 3.14159265 / 1.1);
    return Accent((float)shown * opacity);
}

