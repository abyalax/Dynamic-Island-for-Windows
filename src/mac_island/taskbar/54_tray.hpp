#pragma once
// The other apps' tray icons that aren't pinned, opened from the arrow when
// they don't open sideways in the pill (see islandTrayStyle): a grid of
// icons like Windows' own (the name of the one under the mouse at its
// bottom), or a list with their names. Clicks go to their apps as on the
// pill; the pin (after a moment on an icon) or the middle button pins one.
struct TrayListEntry {
    TrayIcon icon;
    std::wstring name;
};

class TrayListPanel : public Panel {
   public:
    void SetIcons(std::vector<TrayListEntry> icons, bool names) {
        m_icons = std::move(icons);
        m_names = names;
        if (m_icons.empty() && IsOpen()) {
            // (It still takes the mouse while it closes: no cells left.)
            m_cells.clear();
            m_hover = -1;
            Close();
            return;
        }
        if (IsOpen()) {
            Layout();
            Render();
        }
    }

   protected:
    void Prepare(float scale) override {
        if (!m_wic) {
            CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                             CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_wic.put()));
        }
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           13 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_nameFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           12 * scale, DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pinFormat = MakeFormat(IconFont(),
                                     DWRITE_FONT_WEIGHT_NORMAL, 9 * scale);
        }
        Layout();
    }

    void OnOpen() override {
        StopMenuWatch();
        m_hover = -1;
        m_pinHover = false;
        m_pinKey.clear();
        // (Made again on this window's target.)
        m_bitmaps.clear();
        Layout();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

    bool OnMouseMove(POINT pt) override {
        if (!g_trayDrag.key.empty() && g_trayDrag.fromList &&
            g_trayDrag.owner == m_owner) {
            return DragIcon();
        }
        bool pin = false;
        const int hover = HitTest(pt, &pin);
        if (hover == m_hover && pin == m_pinHover) {
            return false;
        }
        if (hover != m_hover) {
            // The pin shows after a moment on the same icon.
            m_pinKey.clear();
            if (hover >= 0) {
                SetTimer(m_hwnd, kPinTimerId, kPinDelayMs, nullptr);
            }
        }
        m_hover = hover;
        m_pinHover = pin;
        return true;
    }

    bool OnMouseLeave() override {
        m_hover = -1;
        m_pinHover = false;
        m_pinKey.clear();
        return true;
    }

    bool OnTimer(UINT_PTR id) override {
        // After an app's menu was asked for: a mouse button pressed outside
        // the list closes it.
        if (id == kMenuTimerId) {
            if (!IsOpen()) {
                StopMenuWatch();
                return false;
            }
            const int buttons = MouseButtonsDown();
            const int pressed = buttons & ~m_menuButtons;
            m_menuButtons = buttons;
            POINT cursor;
            GetCursorPos(&cursor);
            const RECT list{m_position.x, m_position.y,
                            m_position.x + m_size.cx,
                            m_position.y + m_size.cy};
            if (pressed && !PtInRect(&list, cursor)) {
                StopMenuWatch();
                Close();
                return false;
            }
            SetTimer(m_hwnd, kMenuTimerId, kMenuCheckMs, nullptr);
            return false;
        }
        if (id != kPinTimerId || m_hover < 0 ||
            m_hover >= (int)m_icons.size()) {
            return false;
        }
        m_pinKey = m_icons[m_hover].icon.key;
        return true;
    }

    // A double click is told apart here (the window takes single clicks).
    // Its app gets the press as the button comes up; moved a little, the
    // icon is dragged instead (see DragIcon).
    bool OnMouseDown(POINT pt) override {
        bool pin = false;
        const int hit = HitTest(pt, &pin);
        if (hit < 0 || pin) {
            return false;
        }
        const std::wstring& key = m_icons[hit].icon.key;
        const DWORD now = GetTickCount();
        const bool twice = key == m_lastDownKey &&
                           now - m_lastDownTime <= GetDoubleClickTime();
        m_lastDownKey = twice ? L"" : key;
        m_lastDownTime = now;
        m_pressTwice = twice;
        g_trayDrag = {key, m_owner, true};
        GetCursorPos(&g_trayDrag.from);
        SetCapture(m_hwnd);
        return false;
    }

    bool OnMouseUp(POINT pt) override;
    bool OnOtherButton(UINT msg, POINT pt) override;
    // A held icon (see g_trayDrag) dragged: its place in the list, or on
    // the island under the mouse; let go (put there, or back).
    bool DragIcon();
    void EndDrag(bool drop);
    // Where in the list (an icon's index, the count: last), and the line
    // showing it.
    int DropIndex(POINT pt, D2D1_RECT_F* mark) const;

    void OnDestroy() override { StopMenuWatch(); }
    // An icon held as the list closes (or loses the mouse): back.
    void OnCaptureLost() override {
        if (!g_trayDrag.key.empty() && g_trayDrag.fromList &&
            g_trayDrag.owner == m_owner) {
            EndDrag(false);
        }
    }

   private:
    static constexpr UINT_PTR kPinTimerId = 1;
    static constexpr UINT kPinDelayMs = 700;
    static constexpr UINT_PTR kMenuTimerId = 2;
    static constexpr UINT kMenuCheckMs = 50;

    // The mouse buttons held now (a bit each: left, right, middle).
    static int MouseButtonsDown() {
        int buttons = 0;
        int bit = 1;
        for (int key : {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON}) {
            if (GetAsyncKeyState(key) < 0) {
                buttons |= bit;
            }
            bit <<= 1;
        }
        return buttons;
    }

    // An app's menu asked for (its right click): whatever comes in front
    // (the menu, from whichever of the app's windows or processes), the list
    // stays until a click outside it (see OnTimer).
    void KeepOpenForMenu() {
        m_menuButtons = MouseButtonsDown();
        // (Not closed when it stops being the active window.)
        m_dialogShown = true;
        SetTimer(m_hwnd, kMenuTimerId, kMenuCheckMs, nullptr);
    }

    void StopMenuWatch() {
        if (m_hwnd) {
            KillTimer(m_hwnd, kMenuTimerId);
        }
        m_dialogShown = false;
    }

    // The buttons held at the last look (a press is a new one).
    int m_menuButtons = 0;
    // At most this many rows of names (the grid's columns are chosen: see
    // islandTrayColumns).
    static constexpr int kListRows = 12;

    // The click passed on to the icon's app (see Island::ClickTrayIcon).
    void Pass(const std::wstring& key, UINT msg);
    void Pin(const std::wstring& key);

    void Layout() {
        const float s = m_scale;
        const float padding = std::round(8 * s);
        const int count = (int)m_icons.size();
        m_cells.clear();
        if (m_names) {
            const int columns =
                std::max(1, (count + kListRows - 1) / kListRows);
            const int rows = std::max(1, (count + columns - 1) / columns);
            const float cellWidth = std::round(230 * s);
            const float cellHeight = std::round(34 * s);
            for (int i = 0; i < count; i++) {
                const float x = padding + (i / rows) * cellWidth;
                const float y = padding + (i % rows) * cellHeight;
                m_cells.push_back({x, y, x + cellWidth, y + cellHeight});
            }
            const float width = padding * 2 + columns * cellWidth;
            LayoutFooter(padding, padding + rows * cellHeight, width);
            m_size = {(LONG)width, (LONG)(m_footer.bottom + padding / 2)};
            return;
        }
        const int columns =
            std::clamp(count, 1, g_settings.islandTrayColumns);
        const int rows = std::max(1, (count + columns - 1) / columns);
        const float cell = std::round(40 * s);
        for (int i = 0; i < count; i++) {
            const float x = padding + (i % columns) * cell;
            const float y = padding + (i / columns) * cell;
            m_cells.push_back({x, y, x + cell, y + cell});
        }
        // The name of the icon under the mouse, under them.
        const float width =
            std::max(padding * 2 + columns * cell, std::round(200 * s));
        LayoutFooter(padding, padding + rows * cell, width);
        m_size = {(LONG)width, (LONG)(m_footer.bottom + padding / 2)};
    }

    // The line under the icons: how to pin one (on more lines when
    // narrow), or the grid's icon under the mouse.
    void LayoutFooter(float padding, float top, float width) {
        const float s = m_scale;
        const std::wstring hint =
            Tr(L"Drag to change the order or to put one on the pill · middle "
               L"click, or the pin after a moment on an icon: kept on the pill",
               L"Arraste para mudar a ordem ou levar um para a pílula · botão "
               L"do meio, ou o alfinete após um instante no ícone: fica na "
               L"pílula");
        const float hintWidth = width - padding * 2;
        float height = std::round(26 * s);
        m_hintLayout = nullptr;
        if (m_nameFormat && hintWidth > 1 &&
            SUCCEEDED(m_dwrite->CreateTextLayout(
                hint.c_str(), (UINT32)hint.size(), m_nameFormat.get(),
                hintWidth, 10000, m_hintLayout.put()))) {
            m_hintLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
            m_hintLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            DWRITE_TEXT_METRICS metrics{};
            m_hintLayout->GetMetrics(&metrics);
            height = std::max(height,
                              std::ceil(metrics.height) + std::round(10 * s));
            m_hintLayout->SetMaxHeight(height);
            m_hintLayout->SetParagraphAlignment(
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
        m_footer = {padding, top, width - padding, top + height};
    }

    // The pin's circle, at a cell's top right (at the row's end with names).
    D2D1_RECT_F PinRect(const D2D1_RECT_F& cell) const {
        const float size = std::round(16 * m_scale);
        if (m_names) {
            const float right = cell.right - std::round(8 * m_scale);
            const float middle = (cell.top + cell.bottom) / 2;
            return {right - size, middle - size / 2, right, middle + size / 2};
        }
        const float right = cell.right - std::round(1 * m_scale);
        const float top = cell.top + std::round(1 * m_scale);
        return {right - size, top, right, top + size};
    }

    int HitTest(POINT pt, bool* pin) const {
        for (size_t i = 0; i < std::min(m_cells.size(), m_icons.size()); i++) {
            if (PointInRect(pt, m_cells[i])) {
                *pin = !m_pinKey.empty() && m_icons[i].icon.key == m_pinKey &&
                       PointInRect(pt, PinRect(m_cells[i]));
                return (int)i;
            }
        }
        *pin = false;
        return -1;
    }

    ID2D1Bitmap* Bitmap(ID2D1RenderTarget* target, const TrayIcon& icon) {
        for (auto& [key, serial, bitmap] : m_bitmaps) {
            if (key == icon.key && serial == icon.serial) {
                return bitmap.get();
            }
        }
        std::erase_if(m_bitmaps, [&](const auto& entry) {
            return std::get<0>(entry) == icon.key;
        });
        winrt::com_ptr<ID2D1Bitmap> bitmap;
        winrt::com_ptr<IWICBitmap> wicBitmap;
        winrt::com_ptr<IWICFormatConverter> converter;
        if (m_wic && icon.icon &&
            SUCCEEDED(m_wic->CreateBitmapFromHICON(icon.icon->icon,
                                                   wicBitmap.put())) &&
            SUCCEEDED(m_wic->CreateFormatConverter(converter.put())) &&
            SUCCEEDED(converter->Initialize(
                wicBitmap.get(), GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0,
                WICBitmapPaletteTypeMedianCut))) {
            target->CreateBitmapFromWicBitmap(converter.get(), nullptr,
                                              bitmap.put());
        }
        m_bitmaps.push_back({icon.key, icon.serial, bitmap});
        return bitmap.get();
    }

    winrt::com_ptr<IWICImagingFactory> m_wic;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_nameFormat;
    winrt::com_ptr<IDWriteTextFormat> m_pinFormat;
    float m_preparedScale = 0;
    std::vector<TrayListEntry> m_icons;
    bool m_names = false;
    winrt::com_ptr<IDWriteTextLayout> m_hintLayout;
    // The press held was a double click's second one; where a dragged icon
    // goes in the list (-1: not in it) and its line.
    bool m_pressTwice = false;
    int m_dropIndex = -1;
    D2D1_RECT_F m_dropMark{};
    std::vector<D2D1_RECT_F> m_cells;
    D2D1_RECT_F m_footer{};
    std::vector<std::tuple<std::wstring, UINT, winrt::com_ptr<ID2D1Bitmap>>>
        m_bitmaps;
    int m_hover = -1;
    bool m_pinHover = false;
    // The icon showing its pin.
    std::wstring m_pinKey;
    std::wstring m_lastDownKey;
    DWORD m_lastDownTime = 0;
};

void TrayListPanel::Draw(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush) {
    if (!m_textFormat || !m_nameFormat || !m_pinFormat) {
        return;
    }
    const float s = m_scale;
    const float radius = std::round(8 * s);
    const float iconSize = std::round(16 * s);
    for (size_t i = 0; i < m_icons.size() && i < m_cells.size(); i++) {
        const TrayListEntry& entry = m_icons[i];
        const D2D1_RECT_F& cell = m_cells[i];
        const float lit = Hover(HoverKey(6300, (int)i), m_hover == (int)i);
        if (lit > 0.01f) {
            brush->SetColor(Fg(0.09f * lit));
            GlassFill(target, {cell, radius, radius}, brush);
        }
        const float iconX = m_names ? cell.left + std::round(10 * s)
                                    : (cell.left + cell.right - iconSize) / 2;
        const float iconY = (cell.top + cell.bottom - iconSize) / 2;
        // (The one dragged: faint where it was.)
        const bool dragged =
            g_trayDrag.moved && g_trayDrag.key == entry.icon.key;
        if (ID2D1Bitmap* bitmap = Bitmap(target, entry.icon)) {
            DrawBitmapSmooth(target, bitmap,
                             {iconX, iconY, iconX + iconSize, iconY + iconSize},
                             dragged ? 0.3f : 1.0f);
        }
        const bool pinShown = !m_pinKey.empty() && entry.icon.key == m_pinKey;
        if (m_names) {
            const float right = pinShown
                                    ? PinRect(cell).left - std::round(4 * s)
                                    : cell.right - std::round(8 * s);
            brush->SetColor(Fg(0.92f));
            target->DrawText(entry.name.c_str(), (UINT32)entry.name.size(),
                             m_textFormat.get(),
                             {iconX + iconSize + std::round(10 * s), cell.top,
                              right, cell.bottom},
                             brush);
        }
        if (pinShown) {
            const D2D1_RECT_F pin = PinRect(cell);
            const D2D1_POINT_2F center{(pin.left + pin.right) / 2,
                                       (pin.top + pin.bottom) / 2};
            const float half = (pin.right - pin.left) / 2;
            brush->SetColor(m_pinHover ? Accent(1)
                                       : D2D1::ColorF(0.25f, 0.25f, 0.25f,
                                                      0.9f));
            target->FillEllipse({center, half, half}, brush);
            brush->SetColor(D2D1::ColorF(1, 1, 1, 1));
            DrawGlyph(target, &kGlyphPin, 1, m_pinFormat.get(), pin, brush);
        }
    }
    // A dragged icon: the line where it goes, and the icon under the mouse.
    if (g_trayDrag.moved && g_trayDrag.fromList &&
        g_trayDrag.owner == m_owner && m_dropIndex >= 0) {
        const float radius = std::min(m_dropMark.right - m_dropMark.left,
                                      m_dropMark.bottom - m_dropMark.top) /
                             2;
        brush->SetColor(Accent(1));
        target->FillRoundedRectangle({m_dropMark, radius, radius}, brush);
        for (const auto& entry : m_icons) {
            if (entry.icon.key != g_trayDrag.key) {
                continue;
            }
            if (ID2D1Bitmap* bitmap = Bitmap(target, entry.icon)) {
                POINT cursor;
                GetCursorPos(&cursor);
                const float x = (float)(cursor.x - m_position.x);
                const float y = (float)(cursor.y - m_position.y);
                const float half = std::round(9 * s);
                DrawBitmapSmooth(target, bitmap,
                                 {x - half, y - half, x + half, y + half},
                                 0.9f);
            }
        }
    }
    // The grid: the name of the icon under the mouse; otherwise how to pin
    // one.
    if (!m_names && m_hover >= 0 && m_hover < (int)m_icons.size()) {
        const std::wstring& name = m_icons[m_hover].name;
        brush->SetColor(Fg(0.75f));
        target->DrawText(name.c_str(), (UINT32)name.size(), m_nameFormat.get(),
                         m_footer, brush);
    } else if (m_hintLayout) {
        brush->SetColor(Fg(0.45f));
        target->DrawTextLayout({m_footer.left, m_footer.top},
                               m_hintLayout.get(), brush);
    }
}

