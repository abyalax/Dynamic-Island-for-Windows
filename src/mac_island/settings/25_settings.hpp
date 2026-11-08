// Where a language button's flag goes: at `left` (its text's start), in the
// button's middle, on whole pixels so its edges stay sharp (at 150% a
// button starts half a pixel in).
D2D1_RECT_F ChoiceFlagRect(float left, const D2D1_RECT_F& button,
                           float scale) {
    const float width = std::round(18 * scale);
    const float height = std::round(12 * scale);
    const float x = std::round(left);
    const float y = std::round((button.top + button.bottom - height) / 2);
    return {x, y, x + width, y + height};
}

// A language's flag in `rect` (about 3:2), drawn in shapes (Windows has no
// emoji flags): simplified, like an icon, its corners rounded and a thin
// edge so a white one shows on a light background. English: the United
// States'.
void DrawFlag(ID2D1RenderTarget* target,
              ID2D1SolidColorBrush* brush,
              const D2D1_RECT_F& rect,
              TextLanguage language,
              float opacity) {
    winrt::com_ptr<ID2D1Factory> factory;
    target->GetFactory(factory.put());
    const float w = rect.right - rect.left;
    const float h = rect.bottom - rect.top;
    const float radius = std::max(1.0f, std::round(h / 6));
    winrt::com_ptr<ID2D1RoundedRectangleGeometry> shape;
    if (!factory || w < 2 || h < 2 ||
        FAILED(factory->CreateRoundedRectangleGeometry({rect, radius, radius},
                                                       shape.put()))) {
        return;
    }
    auto color = [&](int rgb) {
        brush->SetColor(RgbColor(rgb, opacity));
        return brush;
    };
    auto band = [&](float from, float to, int rgb, bool across = true) {
        const D2D1_RECT_F part =
            across ? D2D1_RECT_F{rect.left, rect.top + h * from, rect.right,
                                 rect.top + h * to}
                   : D2D1_RECT_F{rect.left + w * from, rect.top,
                                 rect.left + w * to, rect.bottom};
        target->FillRectangle(part, color(rgb));
    };
    // A five-pointed star around (x, y), r its points' reach.
    auto star = [&](float x, float y, float r, int rgb) {
        winrt::com_ptr<ID2D1PathGeometry> path;
        winrt::com_ptr<ID2D1GeometrySink> sink;
        if (FAILED(factory->CreatePathGeometry(path.put())) ||
            FAILED(path->Open(sink.put()))) {
            return;
        }
        for (int k = 0; k < 10; k++) {
            const float reach = k % 2 ? r * 0.4f : r;
            const float angle = -1.5708f + k * 0.62832f;
            const D2D1_POINT_2F point{x + reach * std::cos(angle),
                                      y + reach * std::sin(angle)};
            if (k == 0) {
                sink->BeginFigure(point, D2D1_FIGURE_BEGIN_FILLED);
            } else {
                sink->AddLine(point);
            }
        }
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        if (SUCCEEDED(sink->Close())) {
            target->FillGeometry(path.get(), color(rgb));
        }
    };
    winrt::com_ptr<ID2D1Layer> layer;
    target->CreateLayer(nullptr, layer.put());
    target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), shape.get()),
                      layer.get());
    const float cx = (rect.left + rect.right) / 2;
    const float cy = (rect.top + rect.bottom) / 2;
    switch (language) {
        case TextLanguage::English:
            band(0, 1, 0xFFFFFF);
            for (int k = 0; k < 13; k += 2) {
                band(k / 13.0f, (k + 1) / 13.0f, 0xB22234);
            }
            target->FillRectangle(
                {rect.left, rect.top, rect.left + w * 0.42f,
                 rect.top + h * 7 / 13},
                color(0x3C3B6E));
            break;
        case TextLanguage::Portuguese: {
            band(0, 1, 0x009C3B);
            winrt::com_ptr<ID2D1PathGeometry> path;
            winrt::com_ptr<ID2D1GeometrySink> sink;
            if (SUCCEEDED(factory->CreatePathGeometry(path.put())) &&
                SUCCEEDED(path->Open(sink.put()))) {
                sink->BeginFigure({rect.left + w * 0.1f, cy},
                                  D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine({cx, rect.top + h * 0.14f});
                sink->AddLine({rect.right - w * 0.1f, cy});
                sink->AddLine({cx, rect.bottom - h * 0.14f});
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                if (SUCCEEDED(sink->Close())) {
                    target->FillGeometry(path.get(), color(0xFFDF00));
                }
            }
            target->FillEllipse({{cx, cy}, h * 0.23f, h * 0.23f},
                                color(0x002776));
            break;
        }
        case TextLanguage::Chinese:
            band(0, 1, 0xEE1C25);
            star(rect.left + w * 0.18f, rect.top + h * 0.28f, h * 0.17f,
                 0xFFDE00);
            for (const auto& [x, y] : {std::pair{0.34f, 0.12f},
                                       std::pair{0.41f, 0.24f},
                                       std::pair{0.41f, 0.4f},
                                       std::pair{0.34f, 0.52f}}) {
                star(rect.left + w * x, rect.top + h * y, h * 0.06f, 0xFFDE00);
            }
            break;
        case TextLanguage::German:
            band(0, 1 / 3.0f, 0x000000);
            band(1 / 3.0f, 2 / 3.0f, 0xDD0000);
            band(2 / 3.0f, 1, 0xFFCE00);
            break;
        case TextLanguage::Spanish:
            band(0, 0.25f, 0xAA151B);
            band(0.25f, 0.75f, 0xF1BF00);
            band(0.75f, 1, 0xAA151B);
            break;
        case TextLanguage::Russian:
            band(0, 1 / 3.0f, 0xFFFFFF);
            band(1 / 3.0f, 2 / 3.0f, 0x0039A6);
            band(2 / 3.0f, 1, 0xD52B1E);
            break;
        case TextLanguage::Japanese:
            band(0, 1, 0xFFFFFF);
            target->FillEllipse({{cx, cy}, h * 0.3f, h * 0.3f},
                                color(0xBC002D));
            break;
        case TextLanguage::Vietnamese:
            band(0, 1, 0xDA251D);
            star(cx, cy, h * 0.3f, 0xFFFF00);
            break;
    }
    target->PopLayer();
    brush->SetColor(D2D1::ColorF(0, 0, 0, 0.18f * opacity));
    target->DrawRoundedRectangle(
        {{rect.left + 0.5f, rect.top + 0.5f, rect.right - 0.5f,
          rect.bottom - 0.5f},
         radius,
         radius},
        brush, 1);
}

int MediaCoverRgb();

// A visualizer color's 0xRRGGBB (the rainbow: its first bar's; the cover's:
// the icons' without one).
int VisualizerChoiceRgb(int choice) {
    if (choice == kVisualizerCover) {
        if (const int cover = MediaCoverRgb(); cover >= 0) {
            return cover;
        }
        choice = 0;
    }
    switch (choice) {
        case 1:
            return WindowsAccentColor();
        case kVisualizerRainbow:
            return RainbowRgb(0);
        case 3:
            return g_settings.islandVisualizerOwnColor;
        default: {
            const D2D1_COLOR_F icon = FgIcon(1);
            return (int)std::lround(icon.r * 255) << 16 |
                   (int)std::lround(icon.g * 255) << 8 |
                   (int)std::lround(icon.b * 255);
        }
    }
}

// How the fill moves (islandFillStyle).
constexpr int kFillStyleSimple = 0;
constexpr int kFillStyleLiquid = 1;
constexpr int kFillStyleGlow = 2;
// How a download's fill ended: not (going on, or none), done (it got to its
// end), canceled; and how long each shows.
constexpr int kFillEndNone = 0;
constexpr int kFillEndDone = 1;
constexpr int kFillEndCanceled = 2;
constexpr double kFillDoneSeconds = 1.3;
constexpr double kFillCanceledSeconds = 0.9;

// A base64 text's bytes (the URL-safe kind too; it ends at its padding).
std::string DecodeBase64(const std::wstring& text) {
    std::string bytes;
    int value = 0;
    int bits = 0;
    for (const WCHAR c : text) {
        int digit;
        if (c >= L'A' && c <= L'Z') {
            digit = c - L'A';
        } else if (c >= L'a' && c <= L'z') {
            digit = c - L'a' + 26;
        } else if (c >= L'0' && c <= L'9') {
            digit = c - L'0' + 52;
        } else if (c == L'+' || c == L'-') {
            digit = 62;
        } else if (c == L'/' || c == L'_') {
            digit = 63;
        } else {
            break;
        }
        value = ((value << 6) | digit) & 0xFFFFFF;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            bytes += (char)((value >> bits) & 0xFF);
        }
    }
    return bytes;
}

// A Google Calendar page's link (sharing a calendar, "?cid=" its ID in
// base64, or showing it, "embed?src=" its ID) as the calendar's public iCal
// address (which works only while it's public; any calendar has a secret
// one, in its settings); nothing for other links.
std::wstring GooglePublicIcs(const std::wstring& link) {
    if (!link.starts_with(L"https://calendar.google.com/") ||
        link.find(L"/ical/") != std::wstring::npos) {
        return {};
    }
    auto parameter = [&](PCWSTR name) {
        for (const WCHAR before : {L'?', L'&'}) {
            const std::wstring key = std::wstring(1, before) + name + L"=";
            const size_t at = link.find(key);
            if (at != std::wstring::npos) {
                const size_t start = at + key.size();
                return link.substr(start,
                                   link.find_first_of(L"&#", start) - start);
            }
        }
        return std::wstring();
    };
    std::wstring id;
    if (const std::wstring cid = parameter(L"cid"); !cid.empty()) {
        const std::string bytes = DecodeBase64(cid);
        id.assign(bytes.begin(), bytes.end());
    } else {
        id = parameter(L"src");
    }
    std::wstring escaped;
    for (const WCHAR c : id) {
        if (c == L'@') {
            escaped += L"%40";
        } else if (iswalnum(c) || c == L'.' || c == L'-' || c == L'_' ||
                   c == L'%') {
            escaped += c;
        } else {
            return {};
        }
    }
    return escaped.empty() ? std::wstring()
                           : L"https://calendar.google.com/calendar/ical/" +
                                 escaped + L"/public/basic.ics";
}

