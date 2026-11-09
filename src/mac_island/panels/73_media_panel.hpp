#pragma once
// A text too long for its place scrolls to its end and back while the
// mouse is over it (see MarqueeShift; otherwise its start shows); one that
// fits, or without animations, is drawn as it is.
void MediaPanel::DrawScrolling(ID2D1RenderTarget* target,
                               ID2D1SolidColorBrush* brush,
                               const std::wstring& text,
                               IDWriteTextFormat* format,
                               const D2D1_RECT_F& rect) {
    if (text.empty() || !format) {
        return;
    }
    const float width = rect.right - rect.left;
    const float height = rect.bottom - rect.top;
    const auto cached = std::find_if(m_texts.begin(), m_texts.end(),
        [&](const ScrollingText& item) {
            return item.text == text && item.format == format &&
                   std::fabs(item.height - height) < 0.01f;
        });
    if (cached == m_texts.end() || !cached->layout) {
        return;
    }
    auto* layout = cached->layout.get();
    if (AnimationLevel() != 0 || cached->width <= width) {
        target->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
        target->DrawTextLayout({rect.left, rect.top}, layout, brush,
                               D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);
        target->PopAxisAlignedClip();
        return;
    }
    const bool here = ScrollsHere(target, rect);
    m_scrolling = m_scrolling || here;
    const float shift =
        here ? (float)MarqueeShift(
                   NowSeconds() - std::max(m_scrollStart, m_scrollSince),
                   cached->width - width, m_scale)
             : 0.0f;
    target->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
    target->DrawTextLayout({rect.left - shift, rect.top}, layout, brush,
                           D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);
    target->PopAxisAlignedClip();
}

void MediaPanel::ReadDeviceVolume() {
    m_level = 0;
    m_muted = false;
    if (auto volume = g_island ? g_island->GetVolume(nullptr) : nullptr) {
        BOOL muted = FALSE;
        volume->GetMasterVolumeLevelScalar(&m_level);
        volume->GetMute(&muted);
        m_muted = muted;
    }
}

int MediaPanel::PlayingMixerApp() const {
    for (size_t n = 0; n < m_apps.size(); n++) {
        if (!m_apps[n].system &&
            ProcessIsMediaApp(m_apps[n].processId, m_state.appId)) {
            return (int)n;
        }
    }
    return -1;
}

double MediaPanel::AppLevelShown(int index) const {
    if (index < 0 || index >= (int)m_apps.size()) {
        return 0;
    }
    return index < (int)m_mixerMotion.size() ? m_mixerMotion[index].level
        : m_apps[index].muted ? 0 : m_apps[index].volume;
}

void MediaPanel::ReadVolume() {
    ReadDeviceVolume();
    ReadMixerApps(m_apps, std::numeric_limits<size_t>::max(), &m_deviceId);
    auto previous = std::move(m_mixerMotion);
    m_mixerMotion.clear();
    for (const auto& app : m_apps) {
        const auto old = std::find_if(previous.begin(), previous.end(),
            [&](const MixerMotion& motion) {
                return SameMixerApp(app, motion.system, motion.path,
                                     motion.processId);
            });
        if (old != previous.end()) {
            m_mixerMotion.push_back(*old);
        } else {
            m_mixerMotion.push_back(
                {app.processId, app.path, app.system,
                 app.muted ? 1.0 : 0.0, 0,
                 app.muted ? 0.0 : (double)app.volume, 0});
        }
    }
    m_playingApp = PlayingMixerApp();
    // External changes are animated too; idle panels still stop drawing.
    bool changed = m_deviceMute >= 0 &&
        (std::fabs(m_deviceMute - (m_muted ? 1 : 0)) > 0.001 ||
         std::fabs(m_deviceLevel - (m_muted ? 0 : m_level)) > 0.001);
    for (size_t n = 0; n < m_apps.size(); n++) {
        changed = changed ||
            std::fabs(m_mixerMotion[n].mute - (m_apps[n].muted ? 1 : 0)) > 0.001 ||
            std::fabs(m_mixerMotion[n].level -
                       (m_apps[n].muted ? 0 : m_apps[n].volume)) > 0.001;
    }
    if (changed) {
        RequestFrame();
    }
}

// Queue the current desired state; the worker resolves its own interfaces.
// A failed worker start retains the previous synchronous behavior.
void MediaPanel::QueueVolume(int index, bool setLevel) {
    MediaVolumeWriter::Write write;
    write.deviceId = m_deviceId;
    write.device = index < 0;
    write.level = write.device ? m_level : m_apps[index].volume;
    write.muted = write.device ? m_muted : m_apps[index].muted;
    write.setLevel = setLevel;
    if (!write.device) {
        if (m_apps[index].sessionIds.size() == m_apps[index].volumes.size()) {
            write.sessionIds = m_apps[index].sessionIds;
        }
    }
    if (!m_volumeWriter.Submit(std::move(write))) {
        if (index < 0) {
            if (auto volume = g_island ? g_island->GetVolume(nullptr) : nullptr) {
                if (setLevel) {
                    volume->SetMasterVolumeLevelScalar(m_level, &kMediaVolumeContext);
                }
                volume->SetMute(m_muted, &kMediaVolumeContext);
            }
        } else {
            for (const auto& volume : m_apps[index].volumes) {
                if (setLevel) {
                    volume->SetMasterVolume(m_apps[index].volume, &kMediaVolumeContext);
                }
                volume->SetMute(m_apps[index].muted, &kMediaVolumeContext);
            }
        }
    }
}

void MediaPanel::SetAppVolume(int index, float level) {
    if (index < 0 || index >= (int)m_apps.size()) {
        return;
    }
    auto& app = m_apps[index];
    app.volume = std::clamp(level, 0.0f, 1.0f);
    app.muted = app.volume <= 0.001f;
    if ((m_dragging == Part::PlayingVolume || m_dragging == Part::AppVolume) &&
        index == m_dragApp && index < (int)m_mixerMotion.size()) {
        m_mixerMotion[index].level = app.muted ? 0 : app.volume;
        m_mixerMotion[index].levelVelocity = 0;
    }
    QueueVolume(index);
    RequestFrame();
}

void MediaPanel::SetAppMute(int index, bool muted) {
    if (index < 0 || index >= (int)m_apps.size()) {
        return;
    }
    m_apps[index].muted = muted;
    QueueVolume(index, false);
    RequestFrame();
}

void MediaPanel::SetVolume(float level) {
    m_level = std::clamp(level, 0.0f, 1.0f);
    m_muted = m_level <= 0.001f;
    if (m_dragging == Part::Volume) {
        m_deviceLevel = m_muted ? 0 : m_level;
        m_deviceLevelVelocity = 0;
    }
    QueueVolume(-1);
    RequestFrame();
}

void MediaPanel::Layout() {
    const float s = m_scale;
    const float available = m_monitorRect.right > m_monitorRect.left
        ? (float)(m_monitorRect.right - m_monitorRect.left) - std::round(32 * s)
        : kMediaWidth * s;
    const float width = std::round(std::min(kMediaWidth * s,
                                            std::max(1.0f, available)));
    const float padding = std::round(16 * s);
    float y = padding;
    // All sources, wrapped rather than squeezed into a single row.
    m_sourceChips.clear();
    if (m_state.sources.size() > 1) {
        const size_t count = m_state.sources.size() + 1;
        const size_t columns = std::min<size_t>(
            count, std::clamp((int)((width - padding * 2) / (110 * s)), 1, 3));
        const float gap = std::round(6 * s);
        const float chipWidth =
            std::floor((width - padding * 2 - gap * (columns - 1)) / columns);
        const float chip = std::round(30 * s);
        for (size_t i = 0; i < count; i++) {
            const float left = padding + (chipWidth + gap) * (i % columns);
            const float top = y + (chip + gap) * (i / columns);
            m_sourceChips.push_back({left, top, left + chipWidth, top + chip});
        }
        y = m_sourceChips.back().bottom + std::round(14 * s);
    }
    const float cover = std::round(96 * s);
    m_cover = {padding, y, padding + cover, y + cover};
    y += cover + std::round(18 * s);
    if (m_state.duration > 0) {
        m_timeline = {padding, y, width - padding, y + std::round(6 * s)};
        y += std::round(28 * s);
    } else {
        m_timeline = {};
    }
    const float button = std::round(44 * s);
    const float big = std::round(52 * s);
    const float middle = width / 2;
    m_playPause = {middle - big / 2, y, middle + big / 2, y + big};
    m_previous = {m_playPause.left - std::round(18 * s) - button,
                  y + (big - button) / 2, m_playPause.left - std::round(18 * s),
                  y + (big + button) / 2};
    m_next = {m_playPause.right + std::round(18 * s), y + (big - button) / 2,
              m_playPause.right + std::round(18 * s) + button,
              y + (big + button) / 2};
    y += big + std::round(12 * s);
    m_playerBottom = y;
    m_playingVolume = m_playingVolumeIcon = {};
    // (While its slider is dragged, the app dragged: the app playing may
    // change meanwhile, see PlayingVolumeApp.)
    if (const int app = PlayingVolumeApp();
        app >= 0 && app < (int)m_apps.size()) {
        const float icon = std::round(28 * s);
        m_playingVolumeIcon = {padding, y, padding + icon, y + icon};
        m_playingVolume = {m_playingVolumeIcon.right + std::round(10 * s),
            y + icon / 2 - std::round(3 * s),
            width - padding - std::round(40 * s),
            y + icon / 2 + std::round(3 * s)};
        y += icon + std::round(12 * s);
    }
    // The device volume below the player, above the mixer.
    const float volumeHeight = std::round(28 * s);
    m_volumeIcon = {padding, y, padding + volumeHeight, y + volumeHeight};
    m_volume = {m_volumeIcon.right + std::round(10 * s),
                y + volumeHeight / 2 - std::round(3 * s),
                width - padding - std::round(40 * s),
                y + volumeHeight / 2 + std::round(3 * s)};
    y += volumeHeight;
    m_windowsMixer = {padding, y, width - padding, y + std::round(30 * s)};
    y = m_windowsMixer.bottom + std::round(12 * s);
    m_mixerToggle = {padding, y, width - padding, y + std::round(34 * s)};
    y = m_mixerToggle.bottom + std::round(4 * s);
    const float listTop = y;
    m_appIconsRect.clear();
    m_appSliders.clear();
    for (size_t i = 0; i < m_apps.size(); i++) {
        const float icon = std::round(36 * s);
        m_appIconsRect.push_back({padding, y, padding + icon, y + icon});
        m_appSliders.push_back(
            {padding + icon + std::round(12 * s), y + std::round(27 * s),
             width - padding - std::round(40 * s), y + std::round(33 * s)});
        y += std::round(52 * s);
    }
    if (m_apps.empty()) {
        y += std::round(28 * s);
    }
    m_mixerListHeight = y - listTop;
    m_mixerList = {padding, listTop, width - padding, listTop};
    LayoutMixer();
}

void MediaPanel::LayoutMixer() {
    const float s = m_scale;
    const float width = m_mixerToggle.right + std::round(16 * s);
    const float padding = std::round(16 * s);
    const float listTop = m_mixerList.top;
    const float listHeight = m_mixerListHeight;
    float y;
    const float reveal = (float)std::clamp(m_mixerReveal, 0.0, 1.0);
    y = listTop + std::round(listHeight * reveal);
    m_mixerList = {padding, listTop, width - padding, y};
    y += std::round(10 * s);
    // A compact footer, without separate cards around the two switches.
    const float row = std::round(30 * s);
    m_pillButtons = {padding, y, width - padding, y + row};
    y += row;
    m_pillTitle = {padding, y, width - padding, y + row};
    y += row + padding;
    float limit = MaxHeight();
    if (m_workRect.bottom > m_workRect.top) {
        const float room = m_direction == Direction::Down
            ? (float)(m_workRect.bottom - m_anchor.y)
            : m_direction == Direction::Up
                ? (float)(m_anchor.y - m_workRect.top)
                : (float)(m_workRect.bottom - m_workRect.top);
        limit = std::min(limit, std::max(1.0f, room - std::round(24 * s)));
    }
    const float height = std::min(y, limit);
    m_size = {(LONG)width, (LONG)std::ceil(height)};
    m_viewport = {0, 0, width, height};
    m_scroll.SetSizes(height, y);
    m_scroll.offset = std::clamp(m_scroll.offset, 0.0, (double)m_scroll.Max());
}

int MediaPanel::MixerAt(POINT pt, bool icon) const {
    if (!m_mixerExpanded || !PointInRect(pt, m_mixerList)) {
        return -1;
    }
    const auto& rects = icon ? m_appIconsRect : m_appSliders;
    for (size_t i = 0; i < rects.size(); i++) {
        auto rect = rects[i];
        if (!icon) {
            rect.top -= std::round(8 * m_scale);
            rect.bottom += std::round(8 * m_scale);
        }
        if (PointInRect(pt, rect)) {
            return (int)i;
        }
    }
    return -1;
}

MediaPanel::Part MediaPanel::HitTest(POINT pt) const {
    auto around = [&](const D2D1_RECT_F& r) {
        const float grow = std::round(8 * m_scale);
        return PointInRect(pt, {r.left - 2, r.top - grow, r.right + 2,
                                r.bottom + grow});
    };
    if (SourceAt(pt) >= 0) {
        return Part::Source;
    }
    if (PointInRect(pt, m_previous)) {
        return Part::Previous;
    }
    if (PointInRect(pt, m_playPause)) {
        return Part::PlayPause;
    }
    if (PointInRect(pt, m_next)) {
        return Part::Next;
    }
    if (m_state.present && PointInRect(pt, m_cover)) {
        return Part::Cover;
    }
    if (m_state.canSeek && m_timeline.right > m_timeline.left &&
        around(m_timeline)) {
        return Part::Timeline;
    }
    if (PointInRect(pt, m_mixerToggle)) {
        return Part::MixerToggle;
    }
    if (m_playingApp >= 0) {
        if (PointInRect(pt, m_playingVolumeIcon)) {
            return Part::PlayingMute;
        }
        if (around(m_playingVolume)) {
            return Part::PlayingVolume;
        }
    }
    if (PointInRect(pt, m_pillButtons)) {
        return Part::PillButtons;
    }
    if (PointInRect(pt, m_pillTitle)) {
        return Part::PillTitle;
    }
    if (around(m_volume)) {
        return Part::Volume;
    }
    if (PointInRect(pt, m_volumeIcon)) {
        return Part::DeviceMute;
    }
    if (PointInRect(pt, m_windowsMixer)) {
        return Part::WindowsMixer;
    }
    if (MixerAt(pt, true) >= 0) {
        return Part::AppMute;
    }
    if (MixerAt(pt, false) >= 0) {
        return Part::AppVolume;
    }
    return Part::None;
}

double MediaPanel::PositionAt(POINT pt) const {
    const float width = m_timeline.right - m_timeline.left;
    return width > 0 ? std::clamp((pt.x - m_timeline.left) / width, 0.0f,
                                  1.0f) *
                           m_state.duration
                     : 0;
}

// A picture's average color (WIC's Fant scaling to one pixel), or none
// (alpha 0).
D2D1_COLOR_F AverageColor(IWICImagingFactory* wic, IWICBitmapSource* source) {
    winrt::com_ptr<IWICBitmapScaler> scaler;
    BYTE pixel[4]{};
    if (!wic || !source || FAILED(wic->CreateBitmapScaler(scaler.put())) ||
        FAILED(scaler->Initialize(source, 1, 1,
                                  WICBitmapInterpolationModeFant)) ||
        FAILED(scaler->CopyPixels(nullptr, 4, 4, pixel)) || !pixel[3]) {
        return {0, 0, 0, 0};
    }
    const float alpha = pixel[3] / 255.0f;
    return {pixel[2] / (255.0f * alpha), pixel[1] / (255.0f * alpha),
            pixel[0] / (255.0f * alpha), 1};
}

// The cover, decoded once per song.
ID2D1Bitmap* MediaPanel::Thumbnail(ID2D1RenderTarget* target) {
    if (m_thumbnailSerial == m_state.thumbnailSerial) {
        return m_thumbnail.get();
    }
    m_thumbnailSerial = m_state.thumbnailSerial;
    RequestFrame();
    m_thumbnail = nullptr;
    m_coverColor = {0, 0, 0, 0};
    if (m_state.thumbnail.empty() || !m_wic) {
        return nullptr;
    }
    winrt::com_ptr<IStream> stream;
    stream.attach(SHCreateMemStream(m_state.thumbnail.data(),
                                    (UINT)m_state.thumbnail.size()));
    winrt::com_ptr<IWICBitmapDecoder> decoder;
    winrt::com_ptr<IWICBitmapFrameDecode> frame;
    winrt::com_ptr<IWICFormatConverter> converter;
    if (stream &&
        SUCCEEDED(m_wic->CreateDecoderFromStream(
            stream.get(), nullptr, WICDecodeMetadataCacheOnLoad,
            decoder.put())) &&
        SUCCEEDED(decoder->GetFrame(0, frame.put())) &&
        SUCCEEDED(m_wic->CreateFormatConverter(converter.put())) &&
        SUCCEEDED(converter->Initialize(
            frame.get(), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0,
            WICBitmapPaletteTypeMedianCut))) {
        target->CreateBitmapFromWicBitmap(converter.get(), nullptr,
                                          m_thumbnail.put());
        m_coverColor = AverageColor(m_wic.get(), converter.get());
    }
    return m_thumbnail.get();
}

ID2D1Bitmap* MediaPanel::LoadAppBitmap(ID2D1RenderTarget* target,
                                  const std::wstring& appId) {
    if (appId.empty()) {
        return nullptr;
    }
    const double now = NowSeconds();
    int misses = 0;
    for (auto it = m_appIcons.begin(); it != m_appIcons.end(); ++it) {
        if (it->appId == appId) {
            if (!it->retryAt || now < it->retryAt) {
                return it->bitmap.get();
            }
            misses = it->bitmap ? 0 : it->misses + 1;
            m_appIcons.erase(it);
            break;
        }
    }
    bool fromExplorer = false;
    auto bitmap = LoadAppIcon(target, m_wic.get(), m_hwnd, appId,
                               (int)std::round(24 * m_scale), &fromExplorer);
    m_appIcons.push_back(
        {appId, bitmap, AppIconRetryAt(fromExplorer, !!bitmap, misses, now),
         bitmap ? 0 : misses});
    return bitmap.get();
}

// Names, shell icons, cover decoding and text shaping are prepared outside
// Draw/AnimateContents. Drawing a frame only reads these bounded caches.
const std::wstring& MediaPanel::AppName(const std::wstring& appId) const {
    for (const auto& name : m_names) {
        if (name.first == appId) {
            return name.second;
        }
    }
    return m_emptyName;
}

ID2D1Bitmap* MediaPanel::AppBitmap(const std::wstring& appId) const {
    for (const auto& icon : m_appIcons) {
        if (icon.appId == appId) {
            return icon.bitmap.get();
        }
    }
    return nullptr;
}

void MediaPanel::CacheScrolling(const std::wstring& text,
                                IDWriteTextFormat* format, float height) {
    if (text.empty() || !format || !m_dwrite || height <= 0) {
        return;
    }
    for (const auto& item : m_texts) {
        if (item.text == text && item.format == format &&
                   std::fabs(item.height - height) < 0.01f) {
            return;
        }
    }
    ScrollingText item;
    item.text = text;
    item.format = format;
    item.height = height;
    DWRITE_TEXT_METRICS metrics{};
    if (SUCCEEDED(m_dwrite->CreateTextLayout(text.c_str(), (UINT32)text.size(),
            format, 10000, height, item.layout.put())) &&
        SUCCEEDED(item.layout->GetMetrics(&metrics))) {
        item.width = metrics.widthIncludingTrailingWhitespace;
        item.layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_texts.push_back(std::move(item));
    }
}

void MediaPanel::PrepareAssets() {
    auto* target = m_canvas.target.get();
    if (!target || !m_wic || !m_textFormat) {
        return;
    }
    std::vector<std::wstring> ids{m_state.appId, m_previousMedia.appId};
    for (const auto& source : m_state.sources) {
        ids.push_back(source.appId);
    }
    const auto unused = [&](const std::wstring& id) {
        return std::find(ids.begin(), ids.end(), id) == ids.end();
    };
    std::erase_if(m_names, [&](const auto& item) { return unused(item.first); });
    std::erase_if(m_appIcons, [&](const auto& item) { return unused(item.appId); });
    for (const auto& id : ids) {
        if (id.empty()) {
            continue;
        }
        if (std::none_of(m_names.begin(), m_names.end(),
                [&](const auto& item) { return item.first == id; })) {
            m_names.emplace_back(id, AppDisplayName(id));
        }
        LoadAppBitmap(target, id);
    }
    Thumbnail(target);
    for (auto& app : m_apps) {
        MixerAppBitmap(target, m_wic.get(), app);
    }
    PrepareTextCache();
}

std::wstring MediaPanel::SourceName(int source) const {
    if (source < 0) {
        return Tr(L"Automatic", L"Automático");
    }
    if (source >= (int)m_state.sources.size()) {
        return {};
    }
    const auto& entry = m_state.sources[source];
    const auto& name = AppName(entry.appId);
    return name.empty() ? entry.title : name;
}

// Includes every DrawScrolling caller; no text shaping in a drawing frame.
void MediaPanel::PrepareTextCache() {
    const float line = std::round(20 * m_scale);
    const std::wstring title = m_state.present ? m_state.title
        : std::wstring(Tr(L"Nothing playing", L"Nada tocando"));
    std::vector<std::wstring> texts{title, m_state.artist, AppName(m_state.appId)};
    if (!m_sourceChips.empty()) {
        for (int source = -1; source < (int)m_state.sources.size(); source++) {
            texts.push_back(SourceName(source));
        }
    }
    for (const auto& app : m_apps) {
        texts.push_back(app.name);
    }
    std::erase_if(m_texts, [&](const auto& item) {
        return std::find(texts.begin(), texts.end(), item.text) == texts.end();
    });
    CacheScrolling(title, m_titleFormat.get(), line);
    CacheScrolling(m_state.artist, m_textFormat.get(), line);
    CacheScrolling(AppName(m_state.appId), m_timeFormat.get(), line);
    if (!m_sourceChips.empty()) {
        const float chipLine = std::round(30 * m_scale);
        for (int source = -1; source < (int)m_state.sources.size(); source++) {
            CacheScrolling(SourceName(source), m_chipFormat.get(), chipLine);
        }
    }
    const float appLine = std::round(27 * m_scale) - m_scale;
    for (const auto& app : m_apps) {
        CacheScrolling(app.name, m_textFormat.get(), appLine);
    }
}

void MediaPanel::DrawVolumeBar(ID2D1RenderTarget* target,
                                ID2D1SolidColorBrush* brush,
                                const D2D1_RECT_F& rect, double level,
                                double hover, bool accent) {
    auto bar = rect;
    const float middle = (bar.top + bar.bottom) / 2;
    const float half = (2 + (float)std::clamp(hover, 0.0, 1.0)) * m_scale;
    bar.top = middle - half;
    bar.bottom = middle + half;
    brush->SetColor(Fg(0.18f));
    target->FillRoundedRectangle({bar, half, half}, brush);
    bar.right = bar.left + (bar.right - bar.left) *
                            (float)std::clamp(level, 0.0, 1.0);
    brush->SetColor(accent ? Accent(1) : Fg(0.9f));
    target->FillRoundedRectangle({bar, half, half}, brush);
}

void MediaPanel::DrawVolumeIcon(ID2D1RenderTarget* target,
                                 ID2D1SolidColorBrush* brush,
                                 const D2D1_RECT_F& rect, int index, Part part) {
    if (index >= (int)m_apps.size()) {
        return;
    }
    const bool device = index < 0;
    const double muted = device
        ? (m_deviceMute < 0 ? (m_muted ? 1 : 0) : m_deviceMute)
        : index < (int)m_mixerMotion.size() ? m_mixerMotion[index].mute
                                           : m_apps[index].muted ? 1 : 0;
    const float mute = (float)std::clamp(muted, 0.0, 1.0);
    const bool pressed = m_press.target == (int)part &&
        (device || index == m_pressedApp);
    const float press = pressed
        ? (float)std::clamp(m_press.Amount(NowSeconds()), -0.3, 1.0) : 0;
    const float scale = 1 - 0.16f * press;
    const D2D1_POINT_2F center{(rect.left + rect.right) / 2,
                               (rect.top + rect.bottom) / 2};
    D2D1_MATRIX_3X2_F base;
    target->GetTransform(&base);
    target->SetTransform(
        D2D1::Matrix3x2F::Scale(scale, scale, center) *
        *D2D1::Matrix3x2F::ReinterpretBaseType(&base));
    const float lit = Hover(HoverKey(6204 + (int)part, index + 1),
        m_hover == part && (device || m_hoverApp == index));
    brush->SetColor(Fg(0.1f * lit));
    GlassFill(target, {rect, 7 * m_scale, 7 * m_scale}, brush);
    if (device) {
        brush->SetColor(Fg(0.75f * (1 - mute)));
        DrawGlyph(target, &kGlyphSpeaker, 1, m_iconFormat.get(), rect, brush);
        brush->SetColor(Fg(0.75f * mute));
        DrawGlyph(target, &kGlyphMute, 1, m_iconFormat.get(), rect, brush);
    } else {
        if (auto bitmap = m_apps[index].bitmap.get()) {
            const float inset = std::round(4 * m_scale);
            DrawBitmapSmooth(target, bitmap,
                {rect.left + inset, rect.top + inset,
                 rect.right - inset, rect.bottom - inset}, 1 - 0.62f * mute);
        } else {
            const WCHAR glyph = m_apps[index].system ? kGlyphSpeakers : kGlyphMusic;
            brush->SetColor(Fg(0.85f - 0.47f * mute));
            DrawGlyph(target, &glyph, 1, m_iconFormat.get(), rect, brush);
        }
        if (mute > 0.001f) {
            const float size = std::round(18 * m_scale);
            const float badgeScale = 0.55f + 0.45f * mute;
            const D2D1_POINT_2F badge{rect.right - size / 3,
                                      rect.bottom - size / 3};
            D2D1_MATRIX_3X2_F iconBase;
            target->GetTransform(&iconBase);
            target->SetTransform(
                D2D1::Matrix3x2F::Scale(badgeScale, badgeScale, badge) *
                *D2D1::Matrix3x2F::ReinterpretBaseType(&iconBase));
            brush->SetColor(D2D1::ColorF(1.0f, 0.27f, 0.23f, mute));
            DrawGlyph(target, &kGlyphMute, 1, m_iconFormat.get(),
         {badge.x - size / 2, badge.y - size / 2,
          badge.x + size / 2, badge.y + size / 2}, brush);
            target->SetTransform(iconBase);
        }
    }
    target->SetTransform(base);
}

void MediaPanel::DrawPlayer(ID2D1RenderTarget* target,
                             ID2D1SolidColorBrush* brush,
                             const MediaState& state,
                             ID2D1Bitmap* coverBitmap, bool current) {
    const float s = m_scale;
    const float width = (float)m_size.cx;
    const float padding = std::round(16 * s);
    auto scrollText = [&](const std::wstring& text, IDWriteTextFormat* format,
                          const D2D1_RECT_F& rect) {
        if (current) {
            DrawScrolling(target, brush, text, format, rect);
        } else {
            DrawGlyph(target, text.c_str(), (UINT32)text.size(), format, rect,
                      brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    };
    // The cover (or a note), rounded.
    const float radius = std::round(10 * s);
    if (ID2D1Bitmap* cover = coverBitmap) {
        winrt::com_ptr<ID2D1Factory> factory;
        target->GetFactory(factory.put());
        winrt::com_ptr<ID2D1RoundedRectangleGeometry> clip;
        factory->CreateRoundedRectangleGeometry({m_cover, radius, radius},
                                                clip.put());
        winrt::com_ptr<ID2D1Layer> layer;
        target->CreateLayer(nullptr, layer.put());
        target->PushLayer(
            D2D1::LayerParameters(D2D1::InfiniteRect(), clip.get()),
            layer.get());
        // Filled, keeping its proportions.
        const D2D1_SIZE_F size = cover->GetSize();
        const float coverSize = m_cover.right - m_cover.left;
        const float fill =
            std::max(coverSize / std::max(size.width, 1.0f),
                     coverSize / std::max(size.height, 1.0f));
        const float w = size.width * fill;
        const float h = size.height * fill;
        const float cx = (m_cover.left + m_cover.right) / 2;
        const float cy = (m_cover.top + m_cover.bottom) / 2;
        DrawBitmapSmooth(target, cover,
                         {cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2});
        target->PopLayer();
    } else {
        brush->SetColor(Fg(0.1f));
        GlassFill(target, {m_cover, radius, radius}, brush);
        brush->SetColor(Fg(0.6f));
        DrawGlyph(target, &kGlyphMusic, 1, m_bigIconFormat.get(), m_cover,
                  brush);
    }
    // A click on it brings the app to the front: lit under the mouse.
    if (const float lit = current ? Hover(m_cover, m_hover == Part::Cover) : 0;
        lit > 0.01f && state.present) {
        brush->SetColor(Fg(0.14f * lit));
        GlassFill(target, {m_cover, radius, radius}, brush);
    }

    // The song, the artist, the app.
    const float textLeft = m_cover.right + std::round(14 * s);
    const float textRight = width - padding;
    const float line = std::round(20 * s);
    float textTop = m_cover.top + std::round(14 * s);
    const std::wstring title =
        state.present ? state.title
                        : std::wstring(Tr(L"Nothing playing", L"Nada tocando"));
    brush->SetColor(Fg(0.95f));
    scrollText(title, m_titleFormat.get(),
                  {textLeft, textTop, textRight, textTop + line});
    textTop += line;
    brush->SetColor(Fg(0.65f));
    scrollText(state.artist, m_textFormat.get(),
                  {textLeft, textTop, textRight, textTop + line});
    textTop += line;
    const std::wstring& app = AppName(state.appId);
    brush->SetColor(Fg(0.4f));
    float appLeft = textLeft;
    if (auto icon = AppBitmap(state.appId)) {
        const float size = std::round(14 * s);
        const float top = textTop + (line - size) / 2;
        DrawBitmapSmooth(target, icon,
                         {textLeft, top, textLeft + size, top + size}, 0.8f);
        appLeft += size + std::round(5 * s);
    }
    scrollText(app, m_timeFormat.get(),
                  {appLeft, textTop, textRight, textTop + line});

    // The timeline: elapsed and remaining.
    if (state.duration > 0 && m_timeline.right > m_timeline.left) {
        const double position = current && m_dragging == Part::Timeline
                                    ? m_dragPosition
                                    : state.Now();
        const float done = (float)std::clamp(
            position / std::max(state.duration, 0.001), 0.0, 1.0);
        D2D1_RECT_F bar = m_timeline;
        {
            const float middle = (bar.top + bar.bottom) / 2;
            const float half =
                2 * s + (bar.bottom - bar.top - 4 * s) / 2 *
                            (float)std::clamp(m_timelineHover, 0.0, 1.0);
            bar.top = middle - half;
            bar.bottom = middle + half;
        }
        const float barRadius = (bar.bottom - bar.top) / 2;
        brush->SetColor(Fg(0.18f));
        target->FillRoundedRectangle({bar, barRadius, barRadius}, brush);
        brush->SetColor(Fg(0.9f));
        target->FillRoundedRectangle(
            {{bar.left, bar.top, bar.left + (bar.right - bar.left) * done,
              bar.bottom},
             barRadius,
             barRadius},
            brush);
        const std::wstring elapsed = FormatMediaTime(position);
        const std::wstring remaining =
            L"-" + FormatMediaTime(state.duration - position);
        const D2D1_RECT_F times{m_timeline.left,
                                m_timeline.bottom + std::round(4 * s),
                                m_timeline.right,
                                m_timeline.bottom + std::round(20 * s)};
        brush->SetColor(Fg(0.5f));
        target->DrawText(elapsed.c_str(), (UINT32)elapsed.size(),
                         m_timeFormat.get(), times, brush);
        target->DrawText(remaining.c_str(), (UINT32)remaining.size(),
                         m_timeRightFormat.get(), times, brush);
    }

    // Previous, play/pause, next: dimmed when the app doesn't allow them.
    // Pressed, a button and its circle shrink; play turning into pause
    // shrinks and grows again.
    auto button = [&](const D2D1_RECT_F& rect, WCHAR glyph, bool enabled,
                      Part part, IDWriteTextFormat* format) {
        const D2D1_POINT_2F center{(rect.left + rect.right) / 2,
                                   (rect.top + rect.bottom) / 2};
        float size = 1;
        const bool pressed = current && m_press.target == (int)part;
        const float press =
            pressed ? (float)m_press.Amount(NowSeconds()) : 0.0f;
        if (pressed) {
            size *= 1 - 0.18f * std::clamp(press, -0.3f, 1.0f);
        }
        const bool scaled = std::fabs(size - 1) > 0.001f;
        // On top of the panel's own placement.
        D2D1_MATRIX_3X2_F base;
        target->GetTransform(&base);
        const D2D1::Matrix3x2F& baseMatrix =
            *D2D1::Matrix3x2F::ReinterpretBaseType(&base);
        if (scaled) {
            target->SetTransform(D2D1::Matrix3x2F::Scale(size, size, center) *
                                 baseMatrix);
        }
        if (const float lit =
                current ? Hover(rect, (m_hover == part || pressed) && enabled) : 0;
            lit > 0.01f) {
            const float r = (rect.right - rect.left) / 2;
            brush->SetColor(
                Fg((0.12f + 0.1f * std::clamp(press, 0.0f, 1.0f)) * lit));
            GlassFillEllipse(target, {center, r, r}, brush);
        }
        float glyphSize = 1;
        if (current && part == Part::PlayPause) {
            glyphSize -= 0.55f * (float)std::clamp(m_playSwap, 0.0, 1.0);
        }
        if (glyphSize < 1) {
            target->SetTransform(D2D1::Matrix3x2F::Scale(glyphSize, glyphSize,
                                                         center) *
                                 D2D1::Matrix3x2F::Scale(size, size, center) *
                                 baseMatrix);
        }
        brush->SetColor(Fg(enabled ? 0.95f : 0.3f));
        DrawGlyph(target, &glyph, 1, format, rect, brush);
        if (scaled || glyphSize < 1) {
            target->SetTransform(base);
        }
    };
    button(m_previous, kGlyphPreviousTrack,
           state.present && state.canPrevious, Part::Previous,
           m_iconFormat.get());
    button(m_playPause, state.playing ? kGlyphPause : kGlyphPlay,
           state.present && state.canPlayPause, Part::PlayPause,
           m_bigIconFormat.get());
    button(m_next, kGlyphNextTrack, state.present && state.canNext,
           Part::Next, m_iconFormat.get());

}

void MediaPanel::Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
    if (!m_titleFormat || !m_textFormat || !m_timeFormat ||
        !m_timeRightFormat || !m_iconFormat || !m_bigIconFormat) {
        return;
    }
    const float s = m_scale;
    const float width = (float)m_size.cx;
    const float padding = std::round(16 * s);
    // (A text scrolling says so while drawn.)
    m_scrolling = false;

    ID2D1Bitmap* coverBitmap = m_thumbnail.get();
    if (!m_tintReady) {
        const double color[] = {m_coverColor.r, m_coverColor.g,
                                 m_coverColor.b, m_coverColor.a};
        for (int n = 0; n < 4; n++) {
            m_tintShown[n] = color[n];
            m_tintVelocity[n] = 0;
        }
        m_tintReady = true;
    }
    // One uniform tint, painted before the scroll area's edge fades.
    const float tintAlpha = 0.20f -
        0.08f * std::clamp(g_theme.light, 0.0f, 1.0f);
    brush->SetColor({
        (float)std::clamp(m_tintShown[0], 0.0, 1.0),
        (float)std::clamp(m_tintShown[1], 0.0, 1.0),
        (float)std::clamp(m_tintShown[2], 0.0, 1.0),
        tintAlpha * (float)std::clamp(m_tintShown[3], 0.0, 1.0)});
    // Cover the animated shell too while it shrinks, or when the panel
    // follows a pill wider than its contents. The panel clips its corners.
    const float tintWidth = std::max(width, (float)m_shownWidth);
    const float tintOver = (tintWidth - width) / 2;
    target->FillRectangle(
        {-tintOver, 0, width + tintOver,
         std::max((float)m_size.cy, (float)m_shownHeight)}, brush);
    BeginScrollArea(target, m_viewport, m_scroll);
    // The chosen source is lit; paused apps keep their own icon, dimmed.
    for (size_t i = 0; i < m_sourceChips.size() && m_chipFormat; i++) {
        const D2D1_RECT_F& chip = m_sourceChips[i];
        const bool automatic = i == 0;
        const int source = (int)i - 1;
        const bool lit = automatic ? !m_state.chosen
                                   : m_state.chosen &&
                                         m_state.sourceIndex == source;
        const float chipRadius = (chip.bottom - chip.top) / 2;
        const float hover = Hover(chip, m_sourceHover == (int)i);
        const float selected = Hover(HoverKey(6203, (int)i), lit);
        brush->SetColor(Fg(0.08f + 0.08f * hover));
        GlassFill(target, {chip, chipRadius, chipRadius}, brush);
        brush->SetColor(Accent(selected));
        target->FillRoundedRectangle({chip, chipRadius, chipRadius}, brush);
        const std::wstring name = SourceName(source);
        // (The chips are the last Layout's: a source may be gone since.)
        const bool known = !automatic && source < (int)m_state.sources.size();
        const bool playing = known && m_state.sources[source].playing;
        const float iconSize = std::round(16 * s);
        const float iconLeft = chip.left + std::round(8 * s);
        const float iconTop = (chip.top + chip.bottom - iconSize) / 2;
        const D2D1_RECT_F iconRect{
            iconLeft, iconTop, iconLeft + iconSize, iconTop + iconSize};
        ID2D1Bitmap* icon =
            known ? AppBitmap(m_state.sources[source].appId) : nullptr;
        if (icon) {
            DrawBitmapSmooth(target, icon, iconRect, playing ? 1.0f : 0.38f);
        } else {
            const WCHAR glyph = automatic ? kMediaAutomaticGlyph : kGlyphMusic;
            brush->SetColor(lit ? AccentInk(automatic || playing ? 1 : 0.4f)
                                : Fg(automatic || playing ? 0.85f : 0.38f));
            DrawGlyph(target, &glyph, 1, m_iconFormat.get(), iconRect, brush,
                      D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
        const float textLeft = iconRect.right + std::round(5 * s);
        brush->SetColor(lit ? AccentInk(1) : Fg(0.85f));
        DrawScrolling(target, brush, name, m_chipFormat.get(),
                      {textLeft, chip.top, chip.right - std::round(8 * s),
                       chip.bottom});
    }

    const float blend = (float)std::clamp(m_mediaBlend, 0.0, 1.0);
    auto drawPlayerLayer = [&](const MediaState& state, ID2D1Bitmap* cover,
                               float opacity, float shift, bool current) {
        if (opacity < 0.001f) {
            return;
        }
        if (opacity >= 0.999f && std::fabs(shift) < 0.01f) {
            DrawPlayer(target, brush, state, cover, current);
            return;
        }
        D2D1_MATRIX_3X2_F base;
        target->GetTransform(&base);
        winrt::com_ptr<ID2D1Layer> layer;
        target->CreateLayer(nullptr, layer.put());
        target->PushLayer(
            D2D1::LayerParameters(
                {0, m_cover.top - 2 * s, width, m_playerBottom},
                nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                D2D1::IdentityMatrix(), opacity), layer.get());
        target->SetTransform(
            D2D1::Matrix3x2F::Translation(shift, 0) *
            *D2D1::Matrix3x2F::ReinterpretBaseType(&base));
        DrawPlayer(target, brush, state, cover, current);
        target->SetTransform(base);
        target->PopLayer();
    };
    if (blend < 0.999f) {
        drawPlayerLayer(m_previousMedia, m_previousThumbnail.get(),
                         1 - blend, -8 * s * blend, false);
    }
    drawPlayerLayer(m_state, coverBitmap, blend, 8 * s * (1 - blend), true);

    // The playing app's own volume stays in the player, even with the
    // mixer collapsed. It uses the same sessions and mute animation.
    if (const int app = PlayingVolumeApp();
        app >= 0 && app < (int)m_apps.size()) {
        DrawVolumeIcon(target, brush, m_playingVolumeIcon, app,
                        Part::PlayingMute);
        DrawVolumeBar(target, brush, m_playingVolume, AppLevelShown(app),
                      m_playingHover, true);
        const auto percent = std::to_wstring(
            (int)std::round(AppLevelShown(app) * 100)) + L"%";
        brush->SetColor(Fg(0.55f));
        target->DrawText(percent.c_str(), (UINT32)percent.size(),
                         m_timeRightFormat.get(),
                         {m_playingVolume.right, m_playingVolumeIcon.top,
                          width - padding, m_playingVolumeIcon.bottom}, brush);
    }

    DrawVolumeIcon(target, brush, m_volumeIcon, -1, Part::DeviceMute);
    DrawVolumeBar(target, brush, m_volume,
                   m_deviceLevel < 0 ? (m_muted ? 0 : m_level) : m_deviceLevel,
                   m_volumeHover, false);
    // Output percentage and the Windows mixer shortcut.
    const auto devicePercent =
        std::to_wstring((int)std::round((m_deviceLevel < 0 ?
            (m_muted ? 0 : m_level) : m_deviceLevel) * 100)) + L"%";
    brush->SetColor(Fg(0.65f));
    target->DrawText(devicePercent.c_str(), (UINT32)devicePercent.size(),
                     m_timeRightFormat.get(),
                     {m_volume.right, m_volumeIcon.top, width - padding,
                      m_volumeIcon.bottom}, brush);
    brush->SetColor(Fg(0.6f + 0.3f *
                       Hover(m_windowsMixer, m_hover == Part::WindowsMixer)));
    const PCWSTR openMixer = Tr(L"Open Windows mixer", L"Abrir o mixer do Windows");
    target->DrawText(openMixer, (UINT32)wcslen(openMixer), m_textFormat.get(),
                     m_windowsMixer, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    brush->SetColor(Fg(0.12f));
    target->DrawLine({padding, m_mixerToggle.top - 4 * s},
                     {width - padding, m_mixerToggle.top - 4 * s}, brush);
    brush->SetColor(Fg(0.07f * Hover(m_mixerToggle, m_hover == Part::MixerToggle)));
    GlassFill(target, {m_mixerToggle, 10 * s, 10 * s}, brush);
    const D2D1_RECT_F arrow{padding + 2 * s, m_mixerToggle.top,
                            padding + 24 * s, m_mixerToggle.bottom};
    const D2D1_POINT_2F pivot{(arrow.left + arrow.right) / 2,
                              (arrow.top + arrow.bottom) / 2};
    D2D1_MATRIX_3X2_F arrowBase;
    target->GetTransform(&arrowBase);
    target->SetTransform(
        D2D1::Matrix3x2F::Rotation(
            90 * (float)std::clamp(m_mixerReveal, 0.0, 1.0), pivot) *
        *D2D1::Matrix3x2F::ReinterpretBaseType(&arrowBase));
    brush->SetColor(Fg(0.75f));
    DrawGlyph(target, &kGlyphNext, 1, m_iconFormat.get(), arrow, brush);
    target->SetTransform(arrowBase);
    const PCWSTR mixer = Tr(L"Mixer", L"Mixer");
    target->DrawText(mixer, (UINT32)wcslen(mixer), m_titleFormat.get(),
                     {padding + 32 * s, m_mixerToggle.top, width - padding,
                      m_mixerToggle.bottom}, brush);
    const float reveal = (float)std::clamp(m_mixerReveal, 0.0, 1.0);
    if (reveal > 0.001f) {
        winrt::com_ptr<ID2D1Layer> listLayer;
        target->CreateLayer(nullptr, listLayer.put());
        target->PushLayer(
            D2D1::LayerParameters(m_mixerList, nullptr,
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, D2D1::IdentityMatrix(),
                reveal), listLayer.get());
        if (m_apps.empty()) {
            const PCWSTR empty = Tr(L"No apps using this output",
                                    L"Nenhum app usando esta saída");
            brush->SetColor(Fg(0.5f));
            target->DrawText(empty, (UINT32)wcslen(empty), m_timeFormat.get(),
                             {padding, m_mixerList.top,
                              width - padding, m_mixerList.top + std::round(28 * s)},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
        for (size_t i = 0; i < m_apps.size(); i++) {
            auto& app = m_apps[i];
            const auto& icon = m_appIconsRect[i];
            auto slider = m_appSliders[i];
            if (icon.bottom < m_scroll.Offset() || icon.top > m_mixerList.bottom ||
                icon.top > m_scroll.Offset() + m_scroll.viewport) {
                continue;
            }
            DrawVolumeIcon(target, brush, icon, (int)i, Part::AppMute);
            brush->SetColor(Fg(0.75f));
            DrawScrolling(target, brush, app.name, m_textFormat.get(),
                          {slider.left, icon.top - 2 * s,
                           width - padding - 12 * s, slider.top - 3 * s});
            const float middle = (slider.top + slider.bottom) / 2;
            DrawVolumeBar(target, brush, slider, AppLevelShown((int)i),
                m_hoverApp == (int)i || m_dragApp == (int)i ? m_appVolumeHover : 0,
                true);
            const auto percent =
                std::to_wstring((int)std::round(AppLevelShown((int)i) * 100)) + L"%";
            brush->SetColor(Fg(0.55f));
            target->DrawText(percent.c_str(), (UINT32)percent.size(),
                             m_timeRightFormat.get(),
                             {slider.right, middle - 10 * s, width - padding,
                              middle + 10 * s}, brush);
        }

        target->PopLayer();
    }

    // "Buttons on the pill" and "Cover and title on the pill", with their
    // switches: the track goes from gray to blue; the knob slides, a bit
    // springy.
    auto switchRow = [&](const D2D1_RECT_F& row, Part part, PCWSTR label,
                         double knob, bool value) {
        brush->SetColor(Fg(0.06f * Hover(row, m_hover == part)));
        GlassFill(target, {row, 10 * s, 10 * s}, brush);
        brush->SetColor(Fg(0.9f));
        target->DrawText(label, (UINT32)wcslen(label), m_textFormat.get(),
                         {row.left + std::round(10 * s), row.top,
                          row.right - std::round(56 * s), row.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        const double at = knob < 0 ? (value ? 1.0 : 0.0) : knob;
        const float on = (float)std::clamp(at, 0.0, 1.0);
        const float knobAt = (float)std::clamp(at, -0.1, 1.1);
        const float toggleWidth = std::round(36 * s);
        const float toggleHeight = std::round(20 * s);
        const D2D1_RECT_F toggle{
            row.right - std::round(10 * s) - toggleWidth,
            std::round((row.top + row.bottom - toggleHeight) / 2),
            row.right - std::round(10 * s),
            std::round((row.top + row.bottom - toggleHeight) / 2) +
                toggleHeight};
        const float trackRadius = toggleHeight / 2;
        const D2D1_COLOR_F off = Fg(0.22f);
        const D2D1_COLOR_F accent = Accent(1);
        brush->SetColor(D2D1::ColorF(off.r + (accent.r - off.r) * on,
                                     off.g + (accent.g - off.g) * on,
                                     off.b + (accent.b - off.b) * on,
                                     off.a + (1.0f - off.a) * on));
        GlassFill(target, {toggle, trackRadius, trackRadius}, brush);
        brush->SetColor(KnobColor(on));
        const float travel = toggle.right - toggle.left - trackRadius * 2;
        target->FillEllipse(
            {{toggle.left + trackRadius + travel * knobAt,
              toggle.top + trackRadius},
             trackRadius - std::round(2 * s),
             trackRadius - std::round(2 * s)},
            brush);
    };
    switchRow(m_pillButtons, Part::PillButtons,
              Tr(L"Buttons on the pill", L"Botões na pílula"), m_pillKnob,
              g_mediaButtonsInPill);
    switchRow(m_pillTitle, Part::PillTitle,
              Tr(L"Cover and title on the pill", L"Capa e título na pílula"),
              m_titleKnob, g_mediaTitleInPill);
    EndScrollArea(target);
    DrawScrollBar(target, brush, m_viewport, m_scroll);
}

bool MediaPanel::OnMouseMove(POINT pt) {
    pt = ContentPoint(pt);
    // Moved off a pressed button: it lets go.
    if (m_press.held &&
        ((int)HitTest(pt) != m_press.target ||
         (m_press.target == (int)Part::AppMute &&
          MixerAt(pt, true) != m_pressedApp) ||
         (m_press.target == (int)Part::PlayingMute &&
          m_playingApp != m_pressedApp))) {
        m_press.Release();
        RequestFrame();
    }
    if (m_dragging == Part::Timeline) {
        m_dragPosition = PositionAt(pt);
        return true;
    }
    if (m_dragging == Part::Volume) {
        SetVolume((pt.x - m_volume.left) / (m_volume.right - m_volume.left));
        return true;
    }
    if (m_dragging == Part::PlayingVolume) {
        SetAppVolume(m_dragApp, PlayingVolumeAt(pt));
        return true;
    }
    if (m_dragging == Part::AppVolume) {
        if (m_dragApp >= 0 && m_dragApp < (int)m_appSliders.size()) {
            const auto& slider = m_appSliders[m_dragApp];
            SetAppVolume(m_dragApp, (pt.x - slider.left) /
                                      (slider.right - slider.left));
        }
        return true;
    }
    const Part hover = HitTest(pt);
    const int source = SourceAt(pt);
    const int app = hover == Part::PlayingMute || hover == Part::PlayingVolume
        ? m_playingApp : MixerAt(pt, hover == Part::AppMute);
    if (hover == m_hover && source == m_sourceHover && app == m_hoverApp) {
        return false;
    }
    m_hover = hover;
    m_sourceHover = source;
    m_hoverApp = app;
    RequestFrame();
    return true;
}

bool MediaPanel::OnMouseLeave() {
    if (m_press.held) {
        m_press.Release();
        RequestFrame();
    }
    m_sourceHover = -1;
    m_hoverApp = -1;
    if (m_hover == Part::None) {
        return false;
    }
    m_hover = Part::None;
    RequestFrame();
    return true;
}

bool MediaPanel::OnMouseDown(POINT pt) {
    pt = ContentPoint(pt);
    const Part hit = HitTest(pt);
    if (hit != Part::None && hit != Part::Timeline &&
        hit != Part::Volume && hit != Part::AppVolume &&
        hit != Part::PlayingVolume) {
        m_pressedApp = hit == Part::PlayingMute
            ? m_playingApp : MixerAt(pt, true);
        const int source = SourceAt(pt) - 1;
        m_pressedSource = source >= 0 && source < (int)m_state.sources.size()
                              ? m_state.sources[source].appId : L"";
        m_press.Press((int)hit);
        RequestFrame();
        return true;
    }
    if (hit == Part::Timeline) {
        m_dragging = Part::Timeline;
        m_dragPosition = PositionAt(pt);
        SetCapture(m_hwnd);
        return true;
    }
    if (hit == Part::Volume) {
        m_dragging = Part::Volume;
        SetCapture(m_hwnd);
        SetVolume((pt.x - m_volume.left) / (m_volume.right - m_volume.left));
        return true;
    }
    if (hit == Part::PlayingVolume) {
        m_dragging = Part::PlayingVolume;
        m_dragApp = m_playingApp;
        SetCapture(m_hwnd);
        SetAppVolume(m_dragApp, PlayingVolumeAt(pt));
        return true;
    }
    if (hit == Part::AppVolume) {
        m_dragging = Part::AppVolume;
        m_dragApp = MixerAt(pt, false);
        SetCapture(m_hwnd);
        if (m_dragApp >= 0 && m_dragApp < (int)m_appSliders.size()) {
            const auto& slider = m_appSliders[m_dragApp];
            SetAppVolume(m_dragApp, (pt.x - slider.left) /
                                      (slider.right - slider.left));
        }
        return true;
    }
    return false;
}

bool MediaPanel::OnMouseUp(POINT pt) {
    pt = ContentPoint(pt);
    if (m_dragging != Part::None) {
        if (m_dragging == Part::Timeline) {
            SendMediaCommand(MediaCommand::Seek, m_dragPosition);
            // Shown there until the app says where it is.
            m_state.position = m_dragPosition;
            m_state.updatedAt = NowSeconds();
        }
        // Mouse-up can carry a final position not seen by WM_MOUSEMOVE.
        if (m_dragging == Part::Volume) {
            SetVolume((pt.x - m_volume.left) / (m_volume.right - m_volume.left));
        } else if (m_dragging == Part::PlayingVolume) {
            SetAppVolume(m_dragApp, PlayingVolumeAt(pt));
        } else if (m_dragging == Part::AppVolume &&
                   m_dragApp >= 0 && m_dragApp < (int)m_appSliders.size()) {
            const auto& slider = m_appSliders[m_dragApp];
            SetAppVolume(m_dragApp, (pt.x - slider.left) / (slider.right - slider.left));
        }
        m_dragging = Part::None;
        m_dragApp = -1;
        ReleaseCapture();
        RequestFrame();
        return true;
    }
    const Part hit = HitTest(pt);
    const bool clicked = m_press.held && m_press.target == (int)hit &&
        (hit != Part::AppMute || MixerAt(pt, true) == m_pressedApp) &&
        (hit != Part::PlayingMute || m_playingApp == m_pressedApp);
    m_press.Release();
    RequestFrame();
    if (!clicked) {
        return false;
    }
    switch (hit) {
        case Part::MixerToggle:
            m_mixerExpanded = !m_mixerExpanded;
            m_hover = Part::None;
            RequestFrame();
            return true;
        case Part::PlayingMute:
            if (m_playingApp >= 0 && m_playingApp < (int)m_apps.size()) {
                SetAppMute(m_playingApp, !m_apps[m_playingApp].muted);
            }
            return true;
        case Part::Source:
            // 0 is "Automatic", then each app.
            if (const int source = SourceAt(pt) - 1; source >= 0 &&
                (source >= (int)m_state.sources.size() ||
                 m_state.sources[source].appId != m_pressedSource)) {
                return false;
            }
            ChooseMediaSource(m_state, SourceAt(pt) - 1);
            return true;
        case Part::PillButtons:
            SetIslandPref(IslandPref::MediaButtons, !g_mediaButtonsInPill);
            RequestFrame();
            return true;
        case Part::PillTitle:
            SetIslandPref(IslandPref::MediaTitle, !g_mediaTitleInPill);
            RequestFrame();
            return true;
        case Part::AppMute: {
            const int index = MixerAt(pt, true);
            if (index >= 0) {
                SetAppMute(index, !m_apps[index].muted);
            }
            return true;
        }
        case Part::DeviceMute:
            m_muted = !m_muted;
            QueueVolume(-1, false);
            RequestFrame();
            return true;
        case Part::WindowsMixer:
            OpenWindowsMixer();
            return true;
        case Part::Cover:
            if (OpenMediaApp(m_state.appId) && g_island) {
                g_island->WatchMediaAppOpen(m_state.appId);
            }
            return true;
        case Part::Previous:
            SendMediaCommand(MediaCommand::Previous);
            break;
        case Part::PlayPause:
            SendMediaCommand(MediaCommand::PlayPause);
            break;
        case Part::Next:
            SendMediaCommand(MediaCommand::Next);
            break;
        default:
            break;
    }
    return false;
}

// A long modal scrolls over its app list; the output row still controls
// the device. In a short modal, the wheel also controls an app under it.
bool MediaPanel::OnWheel(int delta) {
    if (m_dragging != Part::None || m_press.held) {
        return false;
    }
    POINT pt{};
    GetCursorPos(&pt);
    ScreenToClient(m_hwnd, &pt);
    pt.x -= m_contentOffset.x;
    pt.y -= m_contentOffset.y;
    pt = ContentPoint(pt);
    const Part hit = HitTest(pt);
    if (hit == Part::PlayingVolume || hit == Part::PlayingMute) {
        if (m_playingApp >= 0 && m_playingApp < (int)m_apps.size()) {
            float level = m_apps[m_playingApp].volume;
            SetAppVolume(m_playingApp, level + 0.02f * delta / WHEEL_DELTA);
        }
        return true;
    }
    if (m_scroll.Max() > 0 &&
        hit != Part::Volume && hit != Part::DeviceMute) {
        m_hover = Part::None;
        m_sourceHover = m_hoverApp = -1;
        m_scroll.Wheel(delta, std::round(52 * m_scale));
        return true;
    }
    if (hit == Part::AppVolume || hit == Part::AppMute) {
        const int index = MixerAt(pt, hit == Part::AppMute);
        if (index >= 0) {
            float level = m_apps[index].volume;
            SetAppVolume(index, level + 0.02f * delta / WHEEL_DELTA);
        }
        return true;
    }
    SetVolume(m_level + 0.02f * delta / WHEEL_DELTA);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// The island's own options.

// Restarting the island (another style) is done from a thread of its own,
// since the island's thread can't wait for itself to end. Its handle is
// written on the island's thread and read on Windhawk's: under its own lock
// (not the life lock below, which is held while the island's thread ends).
HANDLE g_islandRestartThread;
SRWLOCK g_islandRestartLock = SRWLOCK_INIT;

void StopIslandThread();
void StartIslandThread();

// Restarting the island, from Windhawk's settings change or from the island
// itself (another style), one at a time.
SRWLOCK g_islandLifeLock = SRWLOCK_INIT;

DWORD WINAPI IslandRestartThreadProc(LPVOID) {
    AcquireSRWLockExclusive(&g_islandLifeLock);
    StopIslandThread();
    if (!g_unloading) {
        StartIslandThread();
    }
    ReleaseSRWLockExclusive(&g_islandLifeLock);
    return 0;
}

void RestartIslandSoon() {
    AcquireSRWLockExclusive(&g_islandRestartLock);
    bool start = !g_unloading;
    if (start && g_islandRestartThread) {
        if (WaitForSingleObject(g_islandRestartThread, 0) != WAIT_OBJECT_0) {
            start = false;
        } else {
            CloseHandle(g_islandRestartThread);
            g_islandRestartThread = nullptr;
        }
    }
    if (start) {
        g_islandRestartThread = CreateThread(
            nullptr, 0, IslandRestartThreadProc, nullptr, 0, nullptr);
    }
    ReleaseSRWLockExclusive(&g_islandRestartLock);
}

// Before stopping the island elsewhere (unloading, settings changed).
void WaitForIslandRestart() {
    AcquireSRWLockExclusive(&g_islandRestartLock);
    const HANDLE thread = g_islandRestartThread;
    g_islandRestartThread = nullptr;
    ReleaseSRWLockExclusive(&g_islandRestartLock);
    if (thread) {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
}

// Seconds on the island's clock: as chosen, or like the Windows clock.
bool ClockShowsSeconds() {
    if (g_settings.clockSeconds >= 0) {
        return g_settings.clockSeconds != 0;
    }
    DWORD seconds = 0;
    DWORD size = sizeof(seconds);
    RegGetValue(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                L"Advanced",
                L"ShowSecondsInSystemClock", RRF_RT_REG_DWORD, nullptr,
                &seconds, &size);
    return seconds != 0;
}

int GlobalClockParts() {
    return (ClockShowsSeconds() ? 1 : 0) | (g_settings.clockWeekday ? 2 : 0) |
           (g_settings.clockDate ? 4 : 0) | (g_settings.clockYear ? 8 : 0);
}

