// The bitmaps' size step for an icon drawn `wanted` pixels big.
int IconPixels(float wanted) {
    for (int size : kIconPixelSizes) {
        if (size >= wanted) {
            return size;
        }
    }
    return kIconPixelSizes[std::size(kIconPixelSizes) - 1];
}

// The bitmaps made for `target` (see IconTargetBitmaps).
IconTargetBitmaps& IconTargetOf(ID2D1RenderTarget* target) {
    auto kept = std::find_if(g_iconTargets.begin(), g_iconTargets.end(),
                             [&](const IconTargetBitmaps& entry) {
                                 return entry.target.get() == target;
                             });
    if (kept == g_iconTargets.end()) {
        // The targets only kept here (their windows gone) go first, then the
        // least used one when there are too many.
        std::erase_if(g_iconTargets, [](const IconTargetBitmaps& entry) {
            entry.target->AddRef();
            return entry.target->Release() == 1;
        });
        if (g_iconTargets.size() >= kIconTargetsMost) {
            g_iconTargets.erase(std::min_element(
                g_iconTargets.begin(), g_iconTargets.end(),
                [](const IconTargetBitmaps& a, const IconTargetBitmaps& b) {
                    return a.used < b.used;
                }));
        }
        g_iconTargets.push_back({});
        kept = std::prev(g_iconTargets.end());
        kept->target.copy_from(target);
    }
    kept->used = ++g_iconUses;
    return *kept;
}

// A bitmap `pixels` big to draw on (at 96 DPI: its DIPs are its pixels);
// nullptr if it can't be.
winrt::com_ptr<ID2D1BitmapRenderTarget> IconCanvas(ID2D1RenderTarget* target,
                                                   int pixels) {
    const D2D1_SIZE_F size{(float)pixels, (float)pixels};
    const D2D1_SIZE_U pixelSize{(UINT32)pixels, (UINT32)pixels};
    winrt::com_ptr<ID2D1BitmapRenderTarget> canvas;
    if (FAILED(target->CreateCompatibleRenderTarget(
            &size, &pixelSize, nullptr,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, canvas.put()))) {
        return nullptr;
    }
    return canvas;
}

// A source's picture as a bitmap of `target`'s, at least `wanted` pixels
// big (or as big as it is); nullptr if it can't be.
ID2D1Bitmap* IconBitmap(ID2D1RenderTarget* target,
                        UINT serial,
                        const IconPicture& picture,
                        float wanted) {
    const int pixels = IconPixels(wanted);
    IconTargetBitmaps* kept = &IconTargetOf(target);
    for (const auto& made : kept->bitmaps) {
        if (made.serial == serial && made.glyph == picture.glyph &&
            made.pixels == pixels) {
            return made.bitmap.get();
        }
    }
    // (The ones of pictures read before go.)
    std::erase_if(kept->bitmaps, [&](const IconTargetBitmap& made) {
        return made.glyph == picture.glyph && made.serial != serial;
    });
    if (kept->bitmaps.size() >= kIconBitmapsMost) {
        kept->bitmaps.clear();
    }
    winrt::com_ptr<ID2D1Bitmap> bitmap;
    if (!picture.pixels.empty()) {
        // Made smaller to fit, its proportions kept (one already smaller,
        // as it is).
        const float fit =
            std::min(1.0f, (float)pixels / (float)std::max(picture.width,
                                                           picture.height));
        winrt::com_ptr<IWICBitmap> memory;
        winrt::com_ptr<IWICBitmapScaler> scaler;
        if (fit >= 1) {
            target->CreateBitmap(
                D2D1::SizeU(picture.width, picture.height),
                picture.pixels.data(), picture.width * 4,
                D2D1::BitmapProperties(D2D1::PixelFormat(
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    D2D1_ALPHA_MODE_PREMULTIPLIED)),
                bitmap.put());
        } else if ((g_iconWic ||
                    SUCCEEDED(CoCreateInstance(
                        CLSID_WICImagingFactory, nullptr,
                        CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(g_iconWic.put())))) &&
                   SUCCEEDED(g_iconWic->CreateBitmapFromMemory(
                       picture.width, picture.height,
                       GUID_WICPixelFormat32bppPBGRA, picture.width * 4,
                       (UINT)picture.pixels.size(),
                       const_cast<BYTE*>(picture.pixels.data()),
                       memory.put())) &&
                   SUCCEEDED(g_iconWic->CreateBitmapScaler(scaler.put())) &&
                   SUCCEEDED(scaler->Initialize(
                       memory.get(),
                       std::max(1u, (UINT)std::lround(picture.width * fit)),
                       std::max(1u, (UINT)std::lround(picture.height * fit)),
                       WICBitmapInterpolationModeFant))) {
            target->CreateBitmapFromWicBitmap(scaler.get(), nullptr,
                                              bitmap.put());
        }
    }
    kept->bitmaps.push_back({serial, picture.glyph, pixels, bitmap});
    return kept->bitmaps.back().bitmap.get();
}

// Where the font would draw an icon of `size` in `rect` (by `format`'s
// alignments): its square's top left corner.
D2D1_POINT_2F IconCorner(IDWriteTextFormat* format,
                         const D2D1_RECT_F& rect,
                         float size) {
    const DWRITE_TEXT_ALIGNMENT across = format->GetTextAlignment();
    const DWRITE_PARAGRAPH_ALIGNMENT down = format->GetParagraphAlignment();
    return {across == DWRITE_TEXT_ALIGNMENT_LEADING ? rect.left
            : across == DWRITE_TEXT_ALIGNMENT_TRAILING
                ? rect.right - size
                : (rect.left + rect.right - size) / 2,
            down == DWRITE_PARAGRAPH_ALIGNMENT_NEAR ? rect.top
            : down == DWRITE_PARAGRAPH_ALIGNMENT_FAR
                ? rect.bottom - size
                : (rect.top + rect.bottom - size) / 2};
}

// How much `target` draws bigger (an icon's pixels as big as it's drawn).
float TargetZoom(ID2D1RenderTarget* target) {
    D2D1_MATRIX_3X2_F transform;
    target->GetTransform(&transform);
    return std::sqrt(std::fabs(transform._11 * transform._22 -
                               transform._12 * transform._21));
}

// A picture in its own colors drawn as see-through as `brush` would draw.
void DrawIconOwnColors(ID2D1RenderTarget* target,
                       ID2D1Bitmap* bitmap,
                       const D2D1_RECT_F& box,
                       ID2D1Brush* brush) {
    float opacity = brush->GetOpacity();
    winrt::com_ptr<ID2D1SolidColorBrush> solid;
    if (SUCCEEDED(brush->QueryInterface(IID_PPV_ARGS(solid.put())))) {
        opacity *= solid->GetColor().a;
    }
    DrawBitmapSmooth(target, bitmap, box, opacity);
}

// A source's picture for `glyph` drawn where the font would draw it: only
// its shape, in the brush's color like the font's (`tint`), or with its own
// colors. False without one (the source asked for if it isn't yet).
bool DrawIconPicture(ID2D1RenderTarget* target,
                     IconSourceKind kind,
                     std::wstring_view what,
                     WCHAR glyph,
                     IDWriteTextFormat* format,
                     const D2D1_RECT_F& rect,
                     ID2D1Brush* brush,
                     bool tint) {
    AcquireSRWLockShared(&g_iconLock);
    const auto source = std::find_if(
        g_iconSources.begin(), g_iconSources.end(),
        [&](const IconSource& own) {
            return own.kind == kind && own.what == what;
        });
    if (source == g_iconSources.end()) {
        ReleaseSRWLockShared(&g_iconLock);
        RequestIconSource(kind, what);
        return false;
    }
    const auto picture = std::find_if(
        source->pictures.begin(), source->pictures.end(),
        [&](const IconPicture& own) { return own.glyph == glyph; });
    bool drawn = false;
    if (picture != source->pictures.end()) {
        const float size = format->GetFontSize();
        if (ID2D1Bitmap* bitmap = IconBitmap(target, source->serial, *picture,
                                             size * TargetZoom(target))) {
            const D2D1_POINT_2F corner = IconCorner(format, rect, size);
            const D2D1_SIZE_U pixels = bitmap->GetPixelSize();
            const float fit =
                size / (float)std::max(pixels.width, pixels.height);
            const float width = pixels.width * fit;
            const float height = pixels.height * fit;
            const D2D1_RECT_F box{corner.x + (size - width) / 2,
                                  corner.y + (size - height) / 2,
                                  corner.x + (size + width) / 2,
                                  corner.y + (size + height) / 2};
            if (tint) {
                // (Its opacity as the mask, which Direct2D only draws
                // aliased.)
                const D2D1_ANTIALIAS_MODE mode = target->GetAntialiasMode();
                target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
                target->FillOpacityMask(bitmap, brush,
                                        D2D1_OPACITY_MASK_CONTENT_GRAPHICS,
                                        &box);
                target->SetAntialiasMode(mode);
            } else {
                DrawIconOwnColors(target, bitmap, box, brush);
            }
            drawn = true;
        }
    }
    ReleaseSRWLockShared(&g_iconLock);
    return drawn;
}

// Another font's format (kept, see g_iconFormats); nullptr if it can't be.
IDWriteTextFormat* IconFontFormatFor(PCWSTR family,
                                     float size,
                                     DWRITE_TEXT_ALIGNMENT across,
                                     DWRITE_PARAGRAPH_ALIGNMENT down) {
    if (!g_iconDwrite &&
        FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                   __uuidof(IDWriteFactory),
                                   (IUnknown**)g_iconDwrite.put()))) {
        return nullptr;
    }
    auto made = std::find_if(
        g_iconFormats.begin(), g_iconFormats.end(),
        [&](const IconFontFormat& kept) {
            return kept.family == family && kept.size == size &&
                   kept.across == across && kept.down == down;
        });
    if (made == g_iconFormats.end()) {
        if (g_iconFormats.size() >= kIconFormatsMost) {
            g_iconFormats.clear();
        }
        winrt::com_ptr<IDWriteTextFormat> own;
        if (FAILED(g_iconDwrite->CreateTextFormat(
                family, nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size,
                L"", own.put()))) {
            return nullptr;
        }
        own->SetTextAlignment(across);
        own->SetParagraphAlignment(down);
        own->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        g_iconFormats.push_back({family, size, across, down, own});
        made = std::prev(g_iconFormats.end());
    }
    return made->format.get();
}

// `text` drawn with another font, in `format`'s place and size (times
// `scale`): Windows 10's icons, an emoji in the brush's color.
void DrawIconWithFont(ID2D1RenderTarget* target,
                      PCWSTR family,
                      PCWSTR text,
                      IDWriteTextFormat* format,
                      float scale,
                      const D2D1_RECT_F& rect,
                      ID2D1Brush* brush) {
    if (IDWriteTextFormat* own = IconFontFormatFor(
            family, format->GetFontSize() * scale, format->GetTextAlignment(),
            format->GetParagraphAlignment())) {
        target->DrawText(text, (UINT32)wcslen(text), own, rect, brush);
    }
}

// An emoji in its own colors drawn where the font would draw `glyph`'s
// icon. Drawn once in a bitmap (each size step, see IconBitmap): a color
// emoji is many layers, too slow to draw with every frame.
void DrawColorEmoji(ID2D1RenderTarget* target,
                    WCHAR glyph,
                    PCWSTR emoji,
                    IDWriteTextFormat* format,
                    const D2D1_RECT_F& rect,
                    ID2D1Brush* brush) {
    // (Kept as the pictures of source 0, which no source is.)
    constexpr UINT kEmojiSerial = 0;
    const float size = format->GetFontSize();
    const int pixels = IconPixels(size * TargetZoom(target));
    IconTargetBitmaps& kept = IconTargetOf(target);
    auto made = std::find_if(kept.bitmaps.begin(), kept.bitmaps.end(),
                             [&](const IconTargetBitmap& own) {
                                 return own.serial == kEmojiSerial &&
                                        own.glyph == glyph &&
                                        own.pixels == pixels;
                             });
    if (made == kept.bitmaps.end()) {
        if (kept.bitmaps.size() >= kIconBitmapsMost) {
            kept.bitmaps.clear();
        }
        winrt::com_ptr<ID2D1Bitmap> bitmap;
        IDWriteTextFormat* own = IconFontFormatFor(
            kEmojiFont, pixels * kEmojiScale, DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        winrt::com_ptr<ID2D1BitmapRenderTarget> canvas;
        winrt::com_ptr<ID2D1SolidColorBrush> ink;
        if (own) {
            canvas = IconCanvas(target, pixels);
        }
        if (canvas && SUCCEEDED(canvas->CreateSolidColorBrush(
                          D2D1::ColorF(D2D1::ColorF::White), ink.put()))) {
            // (Gray edges: ClearType's colored ones need a background.)
            canvas->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
            canvas->BeginDraw();
            canvas->Clear(D2D1::ColorF(0, 0, 0, 0));
            canvas->DrawText(emoji, (UINT32)wcslen(emoji), own,
                             {0, 0, (float)pixels, (float)pixels}, ink.get(),
                             D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
            if (SUCCEEDED(canvas->EndDraw())) {
                canvas->GetBitmap(bitmap.put());
            }
        }
        kept.bitmaps.push_back({kEmojiSerial, glyph, pixels, bitmap});
        made = std::prev(kept.bitmaps.end());
    }
    if (made->bitmap) {
        const D2D1_POINT_2F corner = IconCorner(format, rect, size);
        DrawIconOwnColors(target, made->bitmap.get(),
                          {corner.x, corner.y, corner.x + size,
                           corner.y + size},
                          brush);
    } else {
        // (No bitmap: in the brush's color.)
        DrawIconWithFont(target, kEmojiFont, emoji, format, kEmojiScale, rect,
                         brush);
    }
}

// Whether Windows 10's icon font has `glyph` (its newer icons are only in
// Windows 11's), asked once each.
bool Windows10FontHas(WCHAR glyph) {
    for (const auto& [kept, has] : g_windows10Glyphs) {
        if (kept == glyph) {
            return has;
        }
    }
    bool has = false;
    winrt::com_ptr<IDWriteFontCollection> fonts;
    UINT32 index = 0;
    BOOL exists = FALSE;
    winrt::com_ptr<IDWriteFontFamily> family;
    winrt::com_ptr<IDWriteFont> font;
    if ((g_iconDwrite ||
         SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                       __uuidof(IDWriteFactory),
                                       (IUnknown**)g_iconDwrite.put()))) &&
        SUCCEEDED(g_iconDwrite->GetSystemFontCollection(fonts.put())) &&
        SUCCEEDED(fonts->FindFamilyName(kWindows10IconFont, &index,
                                        &exists)) &&
        exists && SUCCEEDED(fonts->GetFontFamily(index, family.put())) &&
        SUCCEEDED(family->GetFirstMatchingFont(
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, font.put()))) {
        font->HasCharacter(glyph, &exists);
        has = exists;
    }
    g_windows10Glyphs.push_back({glyph, has});
    return has;
}

// The icon drawn by the user's choice (see IconPackId): a picture chosen
// for it, the pack's picture or emoji, or Windows 10's; false for the
// font's (Windows 11's), as before. On the islands' thread only (the dock's
// icons in explorer stay the font's).
bool DrawChosenIcon(ID2D1RenderTarget* target,
                    WCHAR glyph,
                    IDWriteTextFormat* format,
                    const D2D1_RECT_F& rect,
                    ID2D1Brush* brush) {
    const int pack = g_settings.islandIconPack;
    if ((pack == kIconPackWindows && g_iconFiles.files.empty()) ||
        GetCurrentThreadId() != g_islandThreadId) {
        return false;
    }
    // (Only the mod's icons: not a letter or a figure drawn alone.)
    const IconInfo* icon = FindIcon(glyph);
    if (!icon) {
        return false;
    }
    const bool ownColors = g_settings.islandIconColors == 1;
    if (std::any_of(g_iconFiles.files.begin(), g_iconFiles.files.end(),
                    [&](const auto& file) { return file.first == glyph; }) &&
        DrawIconPicture(target, IconSourceKind::Files, g_iconFiles.text,
                        glyph, format, rect, brush, !ownColors)) {
        return true;
    }
    switch (pack) {
        case kIconPackWindows10: {
            if (wcscmp(IconFont(), kWindows10IconFont) == 0 ||
                !Windows10FontHas(glyph)) {
                return false;
            }
            const WCHAR text[] = {glyph, 0};
            DrawIconWithFont(target, kWindows10IconFont, text, format, 1,
                             rect, brush);
            return true;
        }
        case kIconPackEmoji:
            if (!icon->emoji) {
                return false;
            }
            if (ownColors) {
                DrawColorEmoji(target, glyph, icon->emoji, format, rect,
                               brush);
            } else {
                DrawIconWithFont(target, kEmojiFont, icon->emoji, format,
                                 kEmojiScale, rect, brush);
            }
            return true;
        case kIconPackFolder:
            return !g_iconFolder.empty() &&
                   DrawIconPicture(target, IconSourceKind::Folder,
                                   g_iconFolder, glyph, format, rect, brush,
                                   !ownColors);
        default:
            if (const OnlineIconPack* online = FindOnlineIconPack(pack)) {
                return DrawIconPicture(target, IconSourceKind::Online,
                                       online->folder, glyph, format, rect,
                                       brush, true);
            }
            return false;
    }
}

// An icon of the font drawn (like DrawText), as the user chose (see
// DrawChosenIcon).
void DrawGlyph(ID2D1RenderTarget* target,
               const WCHAR* text,
               UINT32 length,
               IDWriteTextFormat* format,
               const D2D1_RECT_F& rect,
               ID2D1Brush* brush,
               D2D1_DRAW_TEXT_OPTIONS options = D2D1_DRAW_TEXT_OPTIONS_NONE) {
    // (In its own color, if one was chosen for it, as see-through as the
    // brush would draw it.)
    winrt::com_ptr<ID2D1SolidColorBrush> solid;
    D2D1_COLOR_F before{};
    if (const int color = length == 1 ? IconColorOf(text[0]) : -1;
        color >= 0 &&
        SUCCEEDED(brush->QueryInterface(IID_PPV_ARGS(solid.put())))) {
        before = solid->GetColor();
        solid->SetColor(D2D1::ColorF(((color >> 16) & 0xFF) / 255.0f,
                                     ((color >> 8) & 0xFF) / 255.0f,
                                     (color & 0xFF) / 255.0f, before.a));
    }
    if (length != 1 || !DrawChosenIcon(target, text[0], format, rect, brush)) {
        target->DrawText(text, length, format, rect, brush, options);
    }
    if (solid) {
        solid->SetColor(before);
    }
}

// Instagram's logo in lines (a rounded square, a lens, a dot), the size of an
// icon of the font, around its middle.
void DrawInstagramLogo(ID2D1RenderTarget* target,
                       ID2D1Brush* brush,
                       D2D1_POINT_2F middle,
                       float size) {
    const float half = size / 2;
    const float stroke = std::max(1.0f, size / 12);
    const float inset = stroke / 2;
    target->DrawRoundedRectangle(
        {{middle.x - half + inset, middle.y - half + inset,
          middle.x + half - inset, middle.y + half - inset},
         size * 0.28f,
         size * 0.28f},
        brush, stroke);
    target->DrawEllipse({middle, size * 0.22f, size * 0.22f}, brush, stroke);
    const float dot = stroke * 0.8f;
    target->FillEllipse(
        {{middle.x + size * 0.25f, middle.y - size * 0.25f}, dot, dot}, brush);
}

// The islands' thread ending: the icons' thread stopped (a download under
// way closed at once) and waited for, then the pictures and what was made
// of them let go.
void ReleaseIconPictures() {
    if (g_iconThread) {
        g_iconStop = true;
        SetEvent(g_iconStopEvent);
        CancelHttps(&g_iconCancel);
        WaitForSingleObject(g_iconThread, INFINITE);
        CloseHandle(g_iconThread);
        g_iconThread = nullptr;
    }
    for (HANDLE* event : {&g_iconStopEvent, &g_iconKick}) {
        if (*event) {
            CloseHandle(*event);
            *event = nullptr;
        }
    }
    AcquireSRWLockExclusive(&g_iconLock);
    g_iconSources.clear();
    ReleaseSRWLockExclusive(&g_iconLock);
    g_iconTargets.clear();
    g_iconFormats.clear();
    g_windows10Glyphs.clear();
    g_iconWic = nullptr;
    g_iconDwrite = nullptr;
}

// The font's icons saved in `folder` as PNG pictures named after them
// (kIcons), black on transparent: a start for a pack of one's own (the
// files already there are left as they are). How many were saved, or -1
// when the folder couldn't be written.
int SaveWindowsIcons(const std::wstring& folder) {
    constexpr UINT kPixels = 64;
    winrt::com_ptr<IWICImagingFactory> wic;
    winrt::com_ptr<ID2D1Factory> d2d;
    winrt::com_ptr<IDWriteFactory> dwrite;
    winrt::com_ptr<IWICBitmap> bitmap;
    winrt::com_ptr<ID2D1RenderTarget> target;
    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    winrt::com_ptr<IDWriteTextFormat> format;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(wic.put()))) ||
        FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                 d2d.put())) ||
        FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                   __uuidof(IDWriteFactory),
                                   (IUnknown**)dwrite.put())) ||
        FAILED(wic->CreateBitmap(kPixels, kPixels,
                                 GUID_WICPixelFormat32bppPBGRA,
                                 WICBitmapCacheOnLoad, bitmap.put())) ||
        FAILED(d2d->CreateWicBitmapRenderTarget(
            bitmap.get(),
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                  D2D1_ALPHA_MODE_PREMULTIPLIED),
                96, 96),
            target.put())) ||
        FAILED(target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0),
                                             brush.put())) ||
        FAILED(dwrite->CreateTextFormat(
            IconFont(), nullptr, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            (float)kPixels, L"", format.put()))) {
        return -1;
    }
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    int saved = 0;
    bool failed = false;
    for (const auto& icon : kIcons) {
        const std::wstring path = folder + L"\\" + icon.name + L".png";
        if (GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            continue;
        }
        target->BeginDraw();
        target->Clear(D2D1::ColorF(0, 0, 0, 0));
        target->DrawText(&icon.glyph, 1, format.get(),
                         D2D1::RectF(0, 0, kPixels, kPixels), brush.get());
        winrt::com_ptr<IWICFormatConverter> converter;
        winrt::com_ptr<IWICStream> stream;
        winrt::com_ptr<IWICBitmapEncoder> encoder;
        winrt::com_ptr<IWICBitmapFrameEncode> frame;
        WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
        if (FAILED(target->EndDraw()) ||
            FAILED(wic->CreateFormatConverter(converter.put())) ||
            FAILED(converter->Initialize(
                bitmap.get(), GUID_WICPixelFormat32bppBGRA,
                WICBitmapDitherTypeNone, nullptr, 0,
                WICBitmapPaletteTypeMedianCut)) ||
            FAILED(wic->CreateStream(stream.put())) ||
            FAILED(stream->InitializeFromFilename(path.c_str(),
                                                  GENERIC_WRITE)) ||
            FAILED(wic->CreateEncoder(GUID_ContainerFormatPng, nullptr,
                                      encoder.put())) ||
            FAILED(encoder->Initialize(stream.get(),
                                       WICBitmapEncoderNoCache)) ||
            FAILED(encoder->CreateNewFrame(frame.put(), nullptr)) ||
            FAILED(frame->Initialize(nullptr)) ||
            FAILED(frame->SetSize(kPixels, kPixels)) ||
            FAILED(frame->SetPixelFormat(&pixelFormat)) ||
            FAILED(frame->WriteSource(converter.get(), nullptr)) ||
            FAILED(frame->Commit()) || FAILED(encoder->Commit())) {
            failed = true;
            continue;
        }
        saved++;
    }
    return failed && !saved ? -1 : saved;
}

struct HolidayRequest {
    HWND notify;
    int year;
    std::wstring country;
};

DWORD WINAPI HolidayThreadProc(LPVOID parameter) {
    std::unique_ptr<HolidayRequest> request((HolidayRequest*)parameter);
    std::vector<BYTE> data;
    const std::wstring url = L"https://date.nager.at/api/v3/PublicHolidays/" +
                             std::to_wstring(request->year) + L"/" +
                             request->country;
    if (!HttpsGet(url, &data, 256 * 1024)) {
        PostMessage(request->notify, WM_APP_HOLIDAYS, request->year, 1);
        return 0;
    }
    std::wstring json;
    const int length = MultiByteToWideChar(CP_UTF8, 0, (const char*)data.data(),
                                           (int)data.size(), nullptr, 0);
    if (length > 0) {
        json.resize(length);
        MultiByteToWideChar(CP_UTF8, 0, (const char*)data.data(),
                            (int)data.size(), json.data(), length);
    }
    std::vector<Holiday> holidays = ParseHolidays(json);
    AcquireSRWLockExclusive(&g_holidaysLock);
    if (g_holidaysCountry == request->country &&
        std::find(g_holidayYears.begin(), g_holidayYears.end(),
                  request->year) == g_holidayYears.end()) {
        g_holidayYears.push_back(request->year);
        for (auto& holiday : holidays) {
            if (holiday.year == request->year) {
                g_holidays.push_back(std::move(holiday));
            }
        }
    }
    ReleaseSRWLockExclusive(&g_holidaysLock);
    PostMessage(request->notify, WM_APP_HOLIDAYS, request->year, 0);
    return 0;
}

// Agendas: .ics calendars (iCalendar, like Google Calendar's "Secret address
// in iCal format"), added in the calendar (value "agendas") or in the mod's
// settings (calendarIcs). Their events show on their days in each agenda's
// color, read only (no reminders: their own apps remind). They're read on a
// thread of their own as the calendar opens (again after 15 minutes, or 10
// after failing) and kept in memory only; repeating events are expanded
// from the start of last year to the end of the next one. Times in UTC turn
// into local ones; times in a time zone of their own are taken as local.
struct AgendaEvent {
    int year = 0;
    int month = 0;
    int day = 0;
    // Minutes into the day, or -1 for the whole day.
    int minute = -1;
    std::wstring title;
};

struct Agenda {
    // An https link (webcal:// read as https://) or a file's path on a drive.
    std::wstring source;
    // 0xRRGGBB.
    int color = 0;
    // In the mod's settings: removed there, not in the calendar.
    bool fromSettings = false;
    // Its own name (X-WR-CALNAME), once read.
    std::wstring name;
    std::vector<AgendaEvent> events;
    bool read = false;
    bool failed = false;
    // Read, but not an .ics (a web page's link).
    bool notIcs = false;
    bool reading = false;
    ULONGLONG readAt = 0;
};

// An agenda's event, with the agenda's color.
struct AgendaItem {
    AgendaEvent event;
    int color = 0;
};

// Touched under the lock (read on a thread of their own).
SRWLOCK g_agendasLock = SRWLOCK_INIT;
std::vector<Agenda> g_agendas;
// The days with events (year * 10000 + month * 100 + day) and their color:
// the first agenda's with an event that day.
std::unordered_map<int, int> g_agendaDays;
// The links and paths in the mod's settings (read with them).
std::vector<std::wstring> g_calendarIcs;
// Reading them (one thread at a time), and told to stop (the calendar
// going away).
HANDLE g_agendaThread;
std::atomic<bool> g_agendaBusy{false};
std::atomic<bool> g_agendaStop{false};
constexpr size_t kMaxAgendas = 12;
constexpr size_t kMaxAgendaSource = 1024;
constexpr size_t kMaxAgendaEvents = 20000;
constexpr size_t kMaxAgendaBytes = 16 * 1024 * 1024;
// New agendas' colors, in turn (macOS's calendar colors, without the
// holidays' green).
constexpr int kAgendaColors[] = {0x0A84FF, 0xFF9F0A, 0xBF5AF2, 0xFF375F,
                                 0x64D2FF, 0xFFD60A, 0xAC8E68};
// Today's circle (value calendarToday): red like on macOS (0), Windows'
// accent color (1), or one chosen (2, calendarTodayColor).
constexpr int kTodayRed = 0xFF453A;
// An agenda was read (wParam 1: the last one asked for).
constexpr UINT WM_APP_AGENDAS = WM_APP + 87;
// The color dialog closed (see ColorThreadProc).
constexpr UINT WM_APP_COLOR_DONE = WM_APP + 72;

D2D1_COLOR_F RgbColor(int rgb, float alpha) {
    float parts[3];
    CustomColorParts(rgb, parts);
    return {parts[0], parts[1], parts[2], alpha};
}

// A soft triangle for an eye's body (see kEyeShapes): the design's path, its
// point up, its sides round and its bottom a little flat, in `rx` and `ry`
// around (`cx`, `cy`).
winrt::com_ptr<ID2D1PathGeometry> SoftTriangleGeometry(ID2D1Factory* factory,
                                                       float cx,
                                                       float cy,
                                                       float rx,
                                                       float ry) {
    winrt::com_ptr<ID2D1PathGeometry> path;
    winrt::com_ptr<ID2D1GeometrySink> sink;
    if (!factory || FAILED(factory->CreatePathGeometry(path.put())) ||
        FAILED(path->Open(sink.put()))) {
        return nullptr;
    }
    auto at = [&](float x, float y) {
        return D2D1_POINT_2F{cx + x * rx, cy + y * ry};
    };
    static constexpr float kCurves[4][6] = {
        {0.30f, -1.02f, 0.72f, -0.48f, 0.91f, 0.30f},
        {1.04f, 0.76f, 0.60f, 1.02f, 0.0f, 0.94f},
        {-0.60f, 1.02f, -1.04f, 0.76f, -0.91f, 0.30f},
        {-0.72f, -0.48f, -0.30f, -1.02f, 0.0f, -1.0f}};
    sink->BeginFigure(at(0, -1), D2D1_FIGURE_BEGIN_FILLED);
    for (const auto& curve : kCurves) {
        const D2D1_BEZIER_SEGMENT segment{at(curve[0], curve[1]),
                                          at(curve[2], curve[3]),
                                          at(curve[4], curve[5])};
        sink->AddBezier(&segment);
    }
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    if (FAILED(sink->Close())) {
        return nullptr;
    }
    return path;
}

// An iris' color at `offset` of its gradient (0 its bright middle, 1 its
// edge), 0xRRGGBB: its four colors (the design's stops, at 0, 45, 85 and
// 100%) blended.
int EyeIrisColor(const EyeTheme& theme, float offset) {
    static constexpr float kAt[4] = {0, 0.45f, 0.85f, 1};
    int part = 0;
    while (part < 2 && offset > kAt[part + 1]) {
        part++;
    }
    const float t =
        std::clamp((offset - kAt[part]) / (kAt[part + 1] - kAt[part]), 0.0f,
                   1.0f);
    int rgb = 0;
    for (int shift = 16; shift >= 0; shift -= 8) {
        const int from = (theme.stops[part] >> shift) & 0xFF;
        const int to = (theme.stops[part + 1] >> shift) & 0xFF;
        rgb |= (int)std::lround(from + (to - from) * t) << shift;
    }
    return rgb;
}

// The eyes drawn in `rect`: for each, its rim, its iris (rings from the dark
// edge to the bright middle, which sits a little up and to the left, like
// the design's gradient, without a gradient to make and keep), the pupil
// looking where it does, two glints and the lid of a blink. `opacity` is
// the pill's content's; `now` tells the wobble of excitement.
void DrawEyes(ID2D1RenderTarget* target,
              ID2D1Factory* factory,
              ID2D1SolidColorBrush* brush,
              const D2D1_RECT_F& rect,
              const EyesState& eyes,
              const EyeTheme& theme,
              const EyeShape& shape,
              float opacity,
              double now) {
    const float unit = EyesUnit(rect, shape);
    if (opacity <= 0.01f || unit < 0.05f) {
        return;
    }
    float lift = 0;
    float wide = 1;
    float tall = 1;
    eyes.Wobble(now, &lift, &wide, &tall);
    const float middleX = (rect.left + rect.right) / 2;
    const float cy = (rect.top + rect.bottom) / 2 + lift * unit;
    // An eye's body: an ellipse, or the soft triangle.
    auto body = [&](float x, float y, float rx, float ry) {
        if (shape.triangle) {
            if (auto path = SoftTriangleGeometry(factory, x, y, rx, ry)) {
                target->FillGeometry(path.get(), brush);
            }
        } else {
            target->FillEllipse({{x, y}, rx, ry}, brush);
        }
    };
    const float rx = shape.rx * unit * wide;
    const float ry = shape.ry * unit * tall;
    constexpr int kRings = 6;
    for (int e = 0; e < 2; e++) {
        const float cx =
            middleX + (e == 0 ? -1.0f : 1.0f) * (shape.rx + 4) * unit * wide;
        brush->SetColor(RgbColor(theme.socket, opacity));
        body(cx, cy, rx + unit, ry + unit);
        for (int ring = 0; ring < kRings; ring++) {
            const float size = 1.0f - (float)ring / kRings;
            brush->SetColor(
                RgbColor(EyeIrisColor(theme, size * 0.9f), opacity));
            body(cx - (1 - size) * 0.30f * rx, cy - (1 - size) * 0.35f * ry,
                 rx * size, ry * size);
        }
        // The pupil stays inside the iris, whatever its size.
        const float roomX =
            std::max(0.0f, shape.rx - 1 - (float)eyes.width);
        const float roomY =
            std::max(0.0f, shape.ry - 1 - (float)eyes.height);
        const float px =
            cx + std::clamp((float)eyes.pupilX[e], -roomX, roomX) * unit * wide;
        const float py =
            cy + std::clamp((float)eyes.pupilY[e], -roomY, roomY) * unit * tall;
        brush->SetColor(RgbColor(theme.pupil, opacity));
        target->FillEllipse({{px, py},
                             (float)eyes.width * unit * wide,
                             (float)eyes.height * unit * tall},
                            brush);
        // (Still: the design's glints drift, but a drift never ends.)
        brush->SetColor({1, 1, 1, 0.95f * opacity});
        target->FillEllipse({{px - 5 * unit * wide, py - 8 * unit * tall},
                             3.5f * unit * wide,
                             3.5f * unit * tall},
                            brush);
        brush->SetColor({1, 1, 1, 0.75f * opacity});
        target->FillEllipse({{px + 3 * unit * wide, py + 5 * unit * tall},
                             1.8f * unit * wide,
                             1.8f * unit * tall},
                            brush);
        // The lid closing from the top.
        if (eyes.lid > 0.001) {
            const float pad = shape.triangle ? 1.8f : 1.5f;
            const float full = (shape.ry + pad) * unit * tall;
            const float shown = full * (float)eyes.lid;
            brush->SetColor(RgbColor(theme.lid, opacity));
            body(cx, cy - full + shown, (shape.rx + pad) * unit * wide, shown);
        }
    }
}

// Windows' accent color (Settings, Personalization, Colors), 0xRRGGBB.
int ReadWindowsAccentColor() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValue(HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                    L"Accent",
                    L"AccentColorMenu", RRF_RT_REG_DWORD, nullptr, &value,
                    &size) == ERROR_SUCCESS) {
        // 0xAABBGGRR.
        return (int)(((value & 0xFF) << 16) | (value & 0xFF00) |
                     ((value >> 16) & 0xFF));
    }
    DWORD color = 0;
    BOOL opaque = FALSE;
    return SUCCEEDED(DwmGetColorizationColor(&color, &opaque))
               ? (int)(color & 0xFFFFFF)
               : 0x0078D4;
}

// Filled before any island is created, refreshed on Windows' color events.
int g_windowsAccentColor = 0x0078D4;
int WindowsAccentColor() { return g_windowsAccentColor; }

// What the pill fills with (see Island::UpdateFill), each with its colors:
// downloads and other apps, file copies, the battery charging.
constexpr int kFillDownloads = 0;
constexpr int kFillCopies = 1;
constexpr int kFillCharge = 2;

// A kind's colors (islandFillColor and the others): green, blue, Windows'
// accent or its own, 0xRRGGBB; and the one chosen.
int FillChoiceRgb(int kind, int choice) {
    switch (choice) {
        case 1:
            return 0x0A84FF;
        case 2:
            return WindowsAccentColor();
        case 3:
            return kind == kFillCopies   ? g_settings.islandFillCopyOwnColor
                   : kind == kFillCharge ? g_settings.islandFillChargeOwnColor
                                         : g_settings.islandFillOwnColor;
        default:
            return 0x30D158;
    }
}

int FillColorRgb(int kind) {
    return FillChoiceRgb(kind, kind == kFillCopies ? g_settings.islandFillCopyColor
                               : kind == kFillCharge
                                   ? g_settings.islandFillChargeColor
                                   : g_settings.islandFillColor);
}

// The audio visualizer's colors (islandVisualizerColor): the icons',
// Windows' accent, a rainbow (each bar its own hue, going around slowly),
// its own, or the playing song's cover's (like the iPhone's).
constexpr int kVisualizerRainbow = 2;
constexpr int kVisualizerCover = 4;
// (How fast the rainbow goes around: turns a second.)
constexpr double kRainbowTurns = 0.12;

// The longest a song's line gets on the pill (DIPs, islandLyricsWidth):
// longer, it ends with "…" (all of it in the panel), so the pill doesn't
// overflow. The first follows each line (as wide as its words, up to the
// longest); the others keep the widest line's width while lines go on.
constexpr int kLyricsMaxWidths[] = {360, 180, 260, 360};
constexpr int kLyricsWidthFollows = 0;

// From how much in it (MB) the Recycle Bin's item shows
// (islandRecycleMinSize): always, or only once it weighs that much (it comes
// and goes by itself, see RecycleBinShown).
constexpr int kRecycleMinMegabytes[] = {0, 100, 500, 1024, 5120};

// A hue (0 to 1 around the color wheel, any number: it wraps) at full
// color, 0xRRGGBB.
int RainbowRgb(double hue) {
    const double h = (hue - std::floor(hue)) * 6;
    const int sector = std::min((int)h, 5);
    const double f = h - sector;
    const double rise = f;
    const double fall = 1 - f;
    double r = 1, g = 0, b = 0;
    switch (sector) {
        case 0:
            r = 1, g = rise, b = 0;
            break;
        case 1:
            r = fall, g = 1, b = 0;
            break;
        case 2:
            r = 0, g = 1, b = rise;
            break;
        case 3:
            r = 0, g = fall, b = 1;
            break;
        case 4:
            r = rise, g = 0, b = 1;
            break;
        default:
            r = 1, g = 0, b = fall;
            break;
    }
    auto byte = [](double v) { return (int)std::lround(v * 255); };
    return byte(r) << 16 | byte(g) << 8 | byte(b);
}

