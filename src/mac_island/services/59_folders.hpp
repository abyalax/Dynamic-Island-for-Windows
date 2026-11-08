// The shelf's item clicked: its files, each with its thumbnail (or its
// type's icon), its name and its folder; one dragged out goes into the app
// it's dropped on (copied or moved as chosen for the folders' panel, or as
// the app does), a click opens it, its "x" takes it off; "Clear" empties
// the shelf. Its files stay where they are.
class ShelfPanel : public Panel {
   public:
    // Its files changed: shown as they are (`toTop`: from the newest, the
    // ones just dropped on the pill).
    void Refresh(bool toTop) {
        if (IsOpen()) {
            if (toTop) {
                m_top = 0;
            }
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
            m_iconFormat = MakeFormat(IconFont(), DWRITE_FONT_WEIGHT_NORMAL,
                                      14 * scale);
            m_bigIconFormat = MakeFormat(IconFont(), DWRITE_FONT_WEIGHT_NORMAL,
                                         26 * scale);
            m_titleFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 13 * scale,
                           DWRITE_TEXT_ALIGNMENT_LEADING);
            m_nameFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           12.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_folderFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_buttonFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11 * scale, DWRITE_TEXT_ALIGNMENT_CENTER);
            for (auto* format : {m_nameFormat.get(), m_folderFormat.get()}) {
                if (format) {
                    format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER,
                                             0, 0};
                    format->SetTrimming(&trimming, nullptr);
                }
            }
            m_icons.clear();
            m_thumbs.ForgetBitmaps();
        }
        Layout();
    }

    void OnOpen() override {
        if (PruneShelf()) {
            AfterPrune();
        }
        m_top = 0;
        m_hover = {};
        m_press = -1;
        m_asking = false;
        m_dragOnce = 0;
        m_icons.clear();
        m_kinds.clear();
        m_thumbs.ForgetBitmaps();
        Layout();
    }

    // (Not closed while one of its files is being dragged into an app.)
    bool StaysWhenDeactivated() const override { return m_dragging; }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override {
        if (!m_titleFormat || !m_nameFormat || !m_folderFormat ||
            !m_buttonFormat || !m_iconFormat || !m_bigIconFormat) {
            return;
        }
        const float s = m_scale;
        const auto& files = Shelf();
        brush->SetColor(Fg(0.95f));
        PCWSTR title = Tr(L"Shelf", L"Prateleira");
        target->DrawText(title, (UINT32)wcslen(title), m_titleFormat.get(),
                         m_title, brush);
        // "Clear all" (only with something on it).
        if (!files.empty()) {
            const float lit = Hover(HoverKey(6600, 0), m_hover.clear);
            const float radius = (m_clear.bottom - m_clear.top) / 2;
            brush->SetColor(Fg(0.1f + 0.08f * lit));
            GlassFill(target, {m_clear, radius, radius}, brush);
            brush->SetColor(Fg(0.9f));
            PCWSTR clear = Tr(L"Clear all", L"Limpar tudo");
            target->DrawText(clear, (UINT32)wcslen(clear),
                             m_buttonFormat.get(), m_clear, brush);
        }
        if (files.empty()) {
            const WCHAR glyph = 0xE7B8;
            brush->SetColor(Fg(0.35f));
            DrawGlyph(target, &glyph, 1, m_bigIconFormat.get(), m_emptyIcon,
                      brush);
            PCWSTR how = Tr(L"Drop files on the pill to keep them here, then "
                            L"drag them into another app",
                            L"Solte arquivos na pílula para guardá-los aqui, "
                            L"depois arraste para outro app");
            brush->SetColor(Fg(0.55f));
            target->DrawText(how, (UINT32)wcslen(how), m_folderFormat.get(),
                             m_emptyText, brush);
            return;
        }
        const float radius = std::round(8 * s);
        const float pad = std::round(8 * s);
        const float thumb = m_rowHeight - pad;
        target->PushAxisAlignedClip(m_list, D2D1_ANTIALIAS_MODE_ALIASED);
        for (int i = m_top; i < (int)files.size(); i++) {
            const D2D1_RECT_F row = RowRect(i);
            if (row.top >= m_list.bottom) {
                break;
            }
            const std::wstring& path = files[i];
            const bool lit = m_hover.row == i;
            const float shown = Hover(row, lit);
            if (shown > 0.01f) {
                brush->SetColor(Fg(0.09f * shown));
                GlassFill(target, {row, radius, radius}, brush);
            }
            const D2D1_RECT_F box{row.left + pad / 2, row.top + pad / 2,
                                  row.left + pad / 2 + thumb,
                                  row.bottom - pad / 2};
            ID2D1Bitmap* picture =
                g_settings.islandFolderThumbnails
                    ? m_thumbs.Get(target, m_wic.get(), m_hwnd, path,
                                   (int)std::ceil(thumb))
                    : nullptr;
            if (!picture) {
                picture = TypeIcon(target, path, IsFolder(path));
            }
            if (picture) {
                // (A thumbnail at its proportions; an icon at its size, not
                // stretched blurry.)
                const D2D1_SIZE_F size = picture->GetSize();
                const D2D1_RECT_F inBox =
                    size.width >= thumb || size.height >= thumb
                        ? box
                        : D2D1_RECT_F{(box.left + box.right - size.width) / 2,
                                      (box.top + box.bottom - size.height) / 2,
                                      (box.left + box.right + size.width) / 2,
                                      (box.top + box.bottom + size.height) / 2};
                DrawBitmapSmooth(target, picture, FitInBox(size, inBox));
            }
            const size_t slash = path.find_last_of(L'\\');
            const std::wstring name =
                slash == std::wstring::npos ? path : path.substr(slash + 1);
            const std::wstring where =
                slash == std::wstring::npos ? L"" : path.substr(0, slash);
            const float textLeft = box.right + std::round(10 * s);
            const float textRight =
                row.right - std::round(lit ? 34 : 10) * s;
            const float middle = (row.top + row.bottom) / 2;
            brush->SetColor(Fg(0.95f));
            target->DrawText(name.c_str(), (UINT32)name.size(),
                             m_nameFormat.get(),
                             {textLeft, row.top + pad / 2, textRight, middle},
                             brush);
            brush->SetColor(Fg(0.5f));
            target->DrawText(where.c_str(), (UINT32)where.size(),
                             m_folderFormat.get(),
                             {textLeft, middle, textRight,
                              row.bottom - pad / 2},
                             brush);
            if (lit) {
                const WCHAR close = 0xE711;
                brush->SetColor(m_hover.remove
                                    ? D2D1::ColorF(1, 0.36f, 0.3f, 1)
                                    : Fg(0.55f));
                DrawGlyph(target, &close, 1, m_iconFormat.get(),
                          RemoveRect(row), brush);
            }
        }
        target->PopAxisAlignedClip();
        if (m_asking) {
            DrawAsk(target, brush);
        }
    }

    // Copy or move? (a file dragged out, asked as the folders panel does.)
    void DrawAsk(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
        const float s = m_scale;
        const float r = std::round(10 * s);
        brush->SetColor(Fg(0.06f));
        GlassFill(target, {m_askRect, r, r}, brush);
        PCWSTR question = Tr(L"Files dragged out of the island are:",
                             L"Arquivos arrastados para fora da ilha são:");
        brush->SetColor(Fg(0.9f));
        target->DrawText(question, (UINT32)wcslen(question),
                         m_nameFormat.get(),
                         {m_askRect.left + std::round(10 * s),
                          m_askRect.top + std::round(6 * s),
                          m_askRect.right - std::round(10 * s),
                          m_askRect.top + std::round(28 * s)},
                         brush);
        auto button = [&](const D2D1_RECT_F& rect, PCWSTR text, bool lit) {
            const float radius = (rect.bottom - rect.top) / 2;
            brush->SetColor(lit ? Accent(1) : Fg(0.12f));
            GlassFill(target, {rect, radius, radius}, brush);
            brush->SetColor(lit ? AccentInk(1) : Fg(0.9f));
            target->DrawText(text, (UINT32)wcslen(text), m_buttonFormat.get(),
                             rect, brush);
        };
        button(m_askCopy, Tr(L"Copied", L"Copiados"),
               m_hover.ask == kAskCopy);
        button(m_askMove, Tr(L"Moved", L"Movidos"), m_hover.ask == kAskMove);
        // "Remember my choice": a box, ticked or not.
        const float box = std::round(14 * s);
        const float middle =
            (m_askRememberRect.top + m_askRememberRect.bottom) / 2;
        const D2D1_RECT_F tick{m_askRememberRect.left + std::round(10 * s),
                               middle - box / 2,
                               m_askRememberRect.left + std::round(10 * s) + box,
                               middle + box / 2};
        brush->SetColor(m_askRemember ? Accent(1) : Fg(0.5f));
        if (m_askRemember) {
            target->FillRoundedRectangle({tick, 3 * s, 3 * s}, brush);
            const WCHAR check = 0xE73E;
            brush->SetColor(AccentInk(1));
            DrawGlyph(target, &check, 1, m_iconFormat.get(), tick, brush);
        } else {
            target->DrawRoundedRectangle({tick, 3 * s, 3 * s}, brush,
                                         std::max(1.0f, s));
        }
        PCWSTR remember =
            Tr(L"Remember my choice (it's in the island's settings)",
               L"Lembrar minha escolha (fica nas configurações da ilha)");
        brush->SetColor(Fg(m_hover.ask == kAskRemember ? 0.95f : 0.75f));
        target->DrawText(remember, (UINT32)wcslen(remember),
                         m_folderFormat.get(),
                         {tick.right + std::round(8 * s), m_askRememberRect.top,
                          m_askRememberRect.right, m_askRememberRect.bottom},
                         brush);
    }

    bool OnMouseMove(POINT pt) override {
        // A file held: dragged out once the mouse moves a little.
        if (m_press >= 0 && !m_dragging) {
            POINT cursor{};
            GetCursorPos(&cursor);
            const float reach = std::round(6 * m_scale);
            if (std::abs(cursor.x - m_pressAt.x) >= reach ||
                std::abs(cursor.y - m_pressAt.y) >= reach) {
                const int row = m_press;
                m_press = -1;
                if (GetCapture() == m_hwnd) {
                    ReleaseCapture();
                }
                DragOut(row);
                return true;
            }
            return false;
        }
        const Hit hover = HitTest(pt);
        if (hover == m_hover) {
            return false;
        }
        m_hover = hover;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hover == Hit{}) {
            return false;
        }
        m_hover = {};
        return true;
    }

    bool OnMouseDown(POINT pt) override {
        const Hit hit = HitTest(pt);
        if (hit.row >= 0 && !hit.remove) {
            m_press = hit.row;
            GetCursorPos(&m_pressAt);
            SetCapture(m_hwnd);
        }
        return false;
    }

    void OnCaptureLost() override { m_press = -1; }

    bool OnMouseUp(POINT pt) override {
        const int pressed = m_press;
        m_press = -1;
        if (GetCapture() == m_hwnd) {
            ReleaseCapture();
        }
        const Hit hit = HitTest(pt);
        const auto& files = Shelf();
        // Copy or move, asked: kept if chosen to be remembered (in the
        // island's settings too), otherwise for the next drag only; the file
        // is dragged again.
        if (hit.ask == kAskRemember) {
            m_askRemember = !m_askRemember;
            return true;
        }
        if (hit.ask == kAskCopy || hit.ask == kAskMove) {
            const int choice = hit.ask == kAskCopy ? 1 : 2;
            if (m_askRemember) {
                SetIslandPref(IslandPref::FolderDrag, choice);
            } else {
                m_dragOnce = choice;
            }
            m_asking = false;
            Layout();
            Render();
            return true;
        }
        if (hit.clear) {
            ClearShelf();
            AfterChange();
            return true;
        }
        if (hit.row < 0 || hit.row >= (int)files.size()) {
            return false;
        }
        if (hit.remove) {
            RemoveFromShelf(files[hit.row]);
            AfterChange();
            return true;
        }
        // A click (not a drag): it opens.
        if (pressed == hit.row) {
            const std::wstring path = files[hit.row];
            Close();
            OpenPath(path);
        }
        return false;
    }

    // The wheel scrolls a list longer than the panel.
    bool OnWheel(int delta) override {
        const int rows = (int)Shelf().size();
        m_wheelRest += delta;
        const int steps = m_wheelRest / (WHEEL_DELTA / 2);
        m_wheelRest -= steps * (WHEEL_DELTA / 2);
        const int top =
            std::clamp(m_top - steps, 0, std::max(0, rows - m_shownRows));
        if (top == m_top) {
            return false;
        }
        m_top = top;
        m_hover = {};
        return true;
    }

    bool OnAppMessage(UINT msg, WPARAM, LPARAM lParam) override {
        if (msg != WM_APP_FILE_THUMBNAIL) {
            return false;
        }
        m_thumbs.Take(lParam);
        return true;
    }

    void OnDestroy() override { m_thumbs.Stop(); }

   private:
    struct Hit {
        int row = -1;
        bool remove = false;
        bool clear = false;
        // The question's parts (see kAskCopy).
        int ask = 0;
        bool operator==(const Hit&) const = default;
    };
    static constexpr int kShownMost = 7;
    // Files dragged out: the question's parts.
    static constexpr int kAskCopy = 1;
    static constexpr int kAskMove = 2;
    static constexpr int kAskRemember = 3;

    // Its files changed here: the panel and the pill follow (closed once
    // it's empty).
    void AfterChange();
    // Files gone dropped as it opens: the pills' counts and items again.
    void AfterPrune();

    // A file dragged into another app: copied or moved as chosen for the
    // folders' panel (asked like there); off the shelf once it's moved
    // away.
    void DragOut(int row) {
        const auto& files = Shelf();
        if (row < 0 || row >= (int)files.size()) {
            return;
        }
        const std::wstring path = files[row];
        // Asked first (the question shows under the list, then it's dragged
        // again).
        const int choice =
            m_dragOnce ? m_dragOnce : g_settings.islandFolderDrag;
        if (FolderDragAsks(choice)) {
            m_asking = true;
            m_askRemember = choice == 0;
            Layout();
            return;
        }
        m_dragOnce = 0;
        PVOID redirection = nullptr;
        const bool unredirected =
            !Wow64WindowsFolderOf(path).empty() &&
            Wow64DisableWow64FsRedirection(&redirection) != FALSE;
        winrt::com_ptr<IShellItem> item;
        winrt::com_ptr<IDataObject> data;
        const bool found =
            SUCCEEDED(SHCreateItemFromParsingName(path.c_str(), nullptr,
                                                  IID_PPV_ARGS(item.put()))) &&
            SUCCEEDED(item->BindToHandler(nullptr, kBHID_DataObject,
                                          IID_PPV_ARGS(data.put())));
        if (unredirected) {
            Wow64RevertWow64FsRedirection(redirection);
        }
        if (!found) {
            // (Gone from its place: off the shelf.)
            RemoveFromShelf(path);
            AfterChange();
            return;
        }
        const DWORD allowed =
            choice == 2 ? DROPEFFECT_MOVE : DROPEFFECT_COPY;
        m_dragging = true;
        DWORD effect = DROPEFFECT_NONE;
        SHDoDragDrop(m_hwnd, data.get(), nullptr, allowed, &effect);
        m_dragging = false;
        if (effect == DROPEFFECT_MOVE ||
            (IsLocalDrivePath(path) &&
             GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES)) {
            RemoveFromShelf(path);
            AfterChange();
        }
    }

    ID2D1Bitmap* TypeIcon(ID2D1RenderTarget* target,
                          const std::wstring& path,
                          bool folder) {
        std::wstring type = L"\\";
        if (!folder) {
            const size_t slash = path.find_last_of(L'\\');
            const size_t dot = path.find_last_of(L'.');
            type = dot == std::wstring::npos ||
                           (slash != std::wstring::npos && dot < slash)
                       ? L"."
                       : path.substr(dot);
            std::transform(type.begin(), type.end(), type.begin(), towlower);
        }
        for (const auto& [key, bitmap] : m_icons) {
            if (key == type) {
                return bitmap.get();
            }
        }
        m_icons.push_back({type, FileTypeIcon(target, m_wic.get(), type, true)});
        return m_icons.back().second.get();
    }

    // Whether a path is a folder, asked once (not each frame; one on
    // another computer is taken for a file).
    bool IsFolder(const std::wstring& path) {
        for (const auto& [kept, folder] : m_kinds) {
            if (kept == path) {
                return folder;
            }
        }
        const bool folder = IsLocalDrivePath(path) &&
                            PathIsDirectory(path.c_str()) != FALSE;
        m_kinds.push_back({path, folder});
        return folder;
    }

    D2D1_RECT_F RowRect(int i) const {
        const float top = m_list.top + (i - m_top) * m_rowHeight;
        return {m_list.left, top, m_list.right, top + m_rowHeight};
    }

    D2D1_RECT_F RemoveRect(const D2D1_RECT_F& row) const {
        return {row.right - std::round(30 * m_scale), row.top,
                row.right - std::round(4 * m_scale), row.bottom};
    }

    Hit HitTest(POINT pt) const {
        Hit hit;
        const auto& files = Shelf();
        if (m_asking) {
            hit.ask = PointInRect(pt, m_askCopy)         ? kAskCopy
                      : PointInRect(pt, m_askMove)       ? kAskMove
                      : PointInRect(pt, m_askRememberRect) ? kAskRemember
                                                         : 0;
            if (hit.ask) {
                return hit;
            }
        }
        if (!files.empty() && PointInRect(pt, m_clear)) {
            hit.clear = true;
            return hit;
        }
        if (!PointInRect(pt, m_list)) {
            return hit;
        }
        const int row =
            m_top + (int)std::floor((pt.y - m_list.top) / m_rowHeight);
        if (row < 0 || row >= (int)files.size()) {
            return hit;
        }
        hit.row = row;
        hit.remove = PointInRect(pt, RemoveRect(RowRect(row)));
        return hit;
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(340 * s);
        const float padding = std::round(12 * s);
        m_rowHeight = std::round(52 * s);
        m_title = {padding + std::round(4 * s), padding, width - padding,
                   padding + std::round(26 * s)};
        // (Room for "Clear all" in the longer languages.)
        const float buttonWidth = std::round(96 * s);
        m_clear = {width - padding - buttonWidth, padding + std::round(1 * s),
                   width - padding, padding + std::round(25 * s)};
        const float top = m_title.bottom + std::round(6 * s);
        const int rows = (int)Shelf().size();
        m_shownRows = std::min(rows, kShownMost);
        m_top = std::clamp(m_top, 0, std::max(0, rows - m_shownRows));
        if (rows == 0) {
            const float icon = std::round(40 * s);
            m_emptyIcon = {(width - icon) / 2, top, (width + icon) / 2,
                           top + icon};
            m_emptyText = {padding + std::round(6 * s),
                           m_emptyIcon.bottom + std::round(4 * s),
                           width - padding - std::round(6 * s),
                           m_emptyIcon.bottom + std::round(40 * s)};
            m_list = {};
            m_size = {(LONG)width, (LONG)std::ceil(m_emptyText.bottom + padding)};
            return;
        }
        m_list = {padding, top, width - padding,
                  top + m_shownRows * m_rowHeight};
        float bottom = m_list.bottom;
        // The question about files dragged out, under the list.
        m_askRect = m_askCopy = m_askMove = m_askRememberRect = {};
        if (m_asking) {
            const float gap = std::round(8 * s);
            const float askTop = bottom + gap;
            const float button = std::round(30 * s);
            const float right = width - padding;
            m_askRect = {padding, askTop, right, askTop + std::round(96 * s)};
            const float buttons = askTop + std::round(30 * s);
            const float half = (right - padding - gap) / 2;
            m_askCopy = {padding + std::round(8 * s), buttons, padding + half,
                         buttons + button};
            m_askMove = {padding + half + gap, buttons,
                         right - std::round(8 * s), buttons + button};
            m_askRememberRect = {padding, buttons + button + std::round(4 * s),
                                 right, m_askRect.bottom};
            bottom = m_askRect.bottom;
        }
        m_size = {(LONG)width, (LONG)std::ceil(bottom + padding)};
    }

    winrt::com_ptr<IWICImagingFactory> m_wic;
    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_bigIconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_titleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_nameFormat;
    winrt::com_ptr<IDWriteTextFormat> m_folderFormat;
    winrt::com_ptr<IDWriteTextFormat> m_buttonFormat;
    float m_preparedScale = 0;
    float m_rowHeight = 0;
    int m_top = 0;
    int m_shownRows = 0;
    int m_wheelRest = 0;
    Hit m_hover;
    // The file held (dragged out once the mouse moves), where it was held
    // (on the screen), and one being dragged now.
    int m_press = -1;
    POINT m_pressAt{};
    bool m_dragging = false;
    D2D1_RECT_F m_title{}, m_clear{}, m_list{}, m_emptyIcon{}, m_emptyText{};
    // Copy or move? asked (see DragOut): its parts, "Remember my choice"
    // ticked or not, and a choice for the next drag only.
    bool m_asking = false;
    bool m_askRemember = true;
    int m_dragOnce = 0;
    D2D1_RECT_F m_askRect{}, m_askCopy{}, m_askMove{}, m_askRememberRect{};
    // Types' icons (an extension, or "\\" for folders), and the files'
    // thumbnails.
    std::vector<std::pair<std::wstring, winrt::com_ptr<ID2D1Bitmap>>> m_icons;
    ThumbnailCache m_thumbs;
    std::vector<std::pair<std::wstring, bool>> m_kinds;
};

class FoldersPanel : public EditingPanel, public DropTarget::Receiver {
   public:
    // Opened under files dragged out of another app, it doesn't take the
    // focus (a click elsewhere wouldn't close it): closed once they left it
    // without being dropped (dropped, it stays to show them; a click in it
    // and then elsewhere, or on the item, closes it).
    void CloseIfDragLeft() {
        if (IsOpen() && !m_filesOver && !m_keepOpen &&
            GetForegroundWindow() != m_hwnd) {
            Close();
        }
    }

   protected:
    void Prepare(float scale) override {
        if (!m_wic) {
            CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                             CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_wic.put()));
        }
        PrepareFormats(scale);
        if (scale != m_rowScale) {
            m_rowScale = scale;
            m_rowFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           12.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_rowDateFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11.5f * scale, DWRITE_TEXT_ALIGNMENT_TRAILING);
            m_rowIconFormat = MakeFormat(IconFont(),
                                      DWRITE_FONT_WEIGHT_NORMAL, 14 * scale);
            m_rowSmallIconFormat = MakeFormat(IconFont(),
                                           DWRITE_FONT_WEIGHT_NORMAL,
                                           10 * scale);
            m_icons.clear();
            m_thumbs.ForgetBitmaps();
        }
        if (m_columns.empty()) {
            LoadPins();
        }
        Layout();
    }

    void OnOpen() override {
        m_icons.clear();
        m_thumbs.ForgetBitmaps();
        m_previewSpot = {-1, -1};
        m_previewShown = false;
        m_order = FolderOrderOf(GetUserIntValue(L"folderOrder", 0, true));
        LoadPins();
        m_hover = {-1, -1};
        m_buttonHover = RowButton::None;
        EndPinDrag();
        EndFileDrag();
        ClearSearch();
        m_asking = false;
        m_askRemember = true;
        m_dragOnce = 0;
        m_arrivals.clear();
        m_dropped.clear();
        m_keepOpen = false;
        m_slideFrom = 0;
        m_leavingAt = 0;
        m_dropSpot = {-1, -1};
        m_drop.Register(m_hwnd, this);
        m_shown = true;
        Layout();
    }

    // Files just dropped in it: it stays though the window they came from
    // takes the focus (see DropAt), until it's clicked.
    bool StaysWhenDeactivated() const override { return m_keepOpen; }

    void OnClose() override {
        m_shown = false;
        m_keepOpen = false;
        StopWatching();
        KillTimer(m_hwnd, kFolderOpenTimerId);
        KillTimer(m_hwnd, kFolderChangeTimerId);
        m_changePending = false;
    }

    // A pinned row held: dragged up or down, it goes where it's let go (see
    // OnMouseMove, OnMouseUp).
    bool OnMouseDown(POINT pt) override {
        // (Clicked: a panel like the others again, see m_keepOpen.)
        m_keepOpen = false;
        if (SearchShown() && PointInRect(pt, m_searchBox)) {
            EditTextClick(m_search, m_searchEdit, pt);
            g_lastTyped = NowSeconds();
            return true;
        }
        RowButton button = RowButton::None;
        const Spot spot = HitTest(pt, &button);
        const FolderEntry* item = ItemAt(spot);
        // A file or a folder held in a folder's column: dragged out once the
        // mouse moves (see StartFileDrag).
        if (spot.column > 0 && item && button == RowButton::None &&
            (item->kind == FolderKind::File ||
             item->kind == FolderKind::Folder) &&
            !m_asking) {
            m_fileDrag = spot;
            // (On the screen: the panel still growing or sliding under a
            // mouse that stays isn't a drag.)
            GetCursorPos(&m_fileFrom);
            SetCapture(m_hwnd);
            return false;
        }
        if (spot.column != 0 || !item || !item->pinned ||
            button != RowButton::None) {
            return false;
        }
        m_dragRow = spot.row;
        m_dropRow = spot.row;
        m_dragFrom = pt;
        m_dragging = false;
        SetCapture(m_hwnd);
        return false;
    }

    void OnCaptureLost() override {
        if (m_dragRow >= 0) {
            EndPinDrag();
            Render();
        }
        m_fileDrag = {-1, -1};
    }

    // Typing searches the folder shown last (Escape clears it first).
    bool OnKey(WPARAM key) override {
        if (!SearchShown()) {
            return false;
        }
        if (key == VK_ESCAPE) {
            if (m_search.empty()) {
                return false;
            }
            ClearSearch();
            SearchAgain();
            return true;
        }
        const std::wstring before = m_search;
        const bool taken = EditTextKey(m_search, m_searchEdit, key);
        if (taken && m_search != before) {
            SearchAgain();
        }
        return taken;
    }
    bool OnChar(WCHAR c) override {
        if (!SearchShown() || c == L'\r' || c == L'\n' || c == L'\t' ||
            c == 0x1B) {
            return false;
        }
        const bool taken = EditTextChar(m_search, m_searchEdit, c, 80, m_hwnd);
        if (taken) {
            SearchAgain();
        }
        return taken;
    }
    bool AnimateContents(double) override {
        const bool typing = AnimateTyping(SearchShown(), false);
        const double now = NowSeconds();
        bool moving = false;
        for (const auto& column : m_columns) {
            moving = moving || now - column.shownAt < kRowsInSeconds ||
                     (column.loading &&
                      now - column.loadingSince < kReadingDelay);
        }
        std::erase_if(m_arrivals, [now](const auto& arrival) {
            return now - arrival.second >= kRowInSeconds;
        });
        // (Dropped files never shown are let go after a minute.)
        std::erase_if(m_dropped, [now](const Dropped& dropped) {
            return dropped.seenAt ? now - dropped.seenAt >= kDroppedSeconds
                                  : now - dropped.droppedAt > 60;
        });
        moving = moving || now - m_slideAt < kSlideSeconds ||
                 now - m_leavingAt < kSlideSeconds ||
                 std::any_of(m_dropped.begin(), m_dropped.end(),
                             [](const Dropped& dropped) {
                                 return dropped.seenAt > 0;
                             });
        return typing || moving || !m_arrivals.empty();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

    // Files dragged over the panel (out of it, or out of Explorer, the
    // desktop...): dropped on a folder (its row, or a column's own folder
    // around its files), by that folder's own drop target. A folder held
    // under them a moment opens beside it.
    DWORD DragOverAt(IDataObject* data,
                     POINT screen,
                     DWORD keys,
                     DWORD allowed) override {
        IslandScope scope(Owner());
        m_filesOver = true;
        // (A column's order menu goes: its rows are where the files go.)
        if (CloseMenus()) {
            Layout();
        }
        POINT pt = screen;
        ScreenToClient(m_hwnd, &pt);
        pt.x -= m_contentOffset.x;
        pt.y -= m_contentOffset.y;
        Spot spot{-1, -1};
        const std::wstring folder = DropFolderAt(pt, &spot);
        if (!(spot == m_dropSpot)) {
            m_dropSpot = spot;
            m_hover = {-1, -1};
            KillTimer(m_hwnd, kFolderOpenTimerId);
            if (Navigable(spot) && m_columns[spot.column].opened != spot.row) {
                SetTimer(m_hwnd, kFolderOpenTimerId, kFolderOpenMs, nullptr);
            }
            Render();
        }
        const POINTL at{screen.x, screen.y};
        if (folder != m_dropFolder) {
            if (m_dropTarget) {
                m_dropTarget->DragLeave();
            }
            m_dropTarget = nullptr;
            m_dropFolder = folder;
            DWORD effect = allowed;
            if (!folder.empty()) {
                m_dropTarget = FolderDropTargetOf(folder);
                if (m_dropTarget &&
                    FAILED(m_dropTarget->DragEnter(data, keys, at, &effect))) {
                    m_dropTarget = nullptr;
                }
            }
        }
        DWORD effect = allowed;
        if (!m_dropTarget ||
            FAILED(m_dropTarget->DragOver(keys, at, &effect))) {
            effect = DROPEFFECT_NONE;
        }
        return effect;
    }
    void DragGone() override {
        IslandScope scope(Owner());
        if (m_dropTarget) {
            m_dropTarget->DragLeave();
        }
        EndDropHover();
        CloseIfDragLeft();
    }
    DWORD DropAt(IDataObject* data,
                 POINT screen,
                 DWORD keys,
                 DWORD allowed) override {
        IslandScope scope(Owner());
        DWORD effect = allowed;
        // (Kept open from before: Windows' drop brings the window the files
        // came from to the front meanwhile.)
        const bool keptOpen = m_keepOpen;
        m_keepOpen = true;
        // (A move within a drive comes back as DROPEFFECT_NONE: Windows'
        // "optimized move", telling the source not to delete them again.
        // Dropped all the same.)
        const bool dropped =
            m_dropTarget &&
            SUCCEEDED(m_dropTarget->Drop(data, keys, {screen.x, screen.y},
                                         &effect));
        if (!dropped) {
            effect = DROPEFFECT_NONE;
            m_keepOpen = keptOpen;
        }
        // Dropped: the panel stays, the folder they went in opens beside its
        // row, and they're lit as they show up there.
        const Spot spot = m_dropSpot;
        const std::wstring folder = m_dropFolder;
        if (dropped) {
            NoteDropped(data, folder);
        }
        EndDropHover();
        if (dropped && spot.row >= 0 && Navigable(spot) &&
            m_columns[spot.column].opened != spot.row) {
            OpenColumn(spot);
        }
        CloseIfDragLeft();
        Render();
        return effect;
    }

    bool OnMouseMove(POINT pt) override {
        if (m_fileDrag.column >= 0) {
            const float reach = std::round(6 * m_scale);
            POINT cursor{};
            GetCursorPos(&cursor);
            if (std::abs(cursor.x - m_fileFrom.x) < reach &&
                std::abs(cursor.y - m_fileFrom.y) < reach) {
                return false;
            }
            const Spot spot = m_fileDrag;
            EndFileDrag();
            StartFileDrag(spot.column, spot.row);
            return true;
        }
        if (m_asking) {
            const int hover = AskHit(pt);
            if (hover == m_askHover) {
                return false;
            }
            m_askHover = hover;
            return true;
        }
        if (m_dragRow >= 0 && !m_columns.empty()) {
            if (!m_dragging && std::abs(pt.y - m_dragFrom.y) <
                                   std::round(5 * m_scale)) {
                return false;
            }
            m_dragging = true;
            const Column& pins = m_columns[0];
            const int row =
                pins.top + (int)std::floor((pt.y - pins.rect.top) / m_rowHeight);
            const int drop =
                std::clamp(row, 0, std::max(0, (int)m_pins.size() - 1));
            if (drop == m_dropRow) {
                return false;
            }
            m_dropRow = drop;
            return true;
        }
        RowButton button = RowButton::None;
        const Spot hover = HitTest(pt, &button);
        if (hover == m_hover && button == m_buttonHover) {
            return false;
        }
        m_hover = hover;
        m_buttonHover = button;
        FollowPreview();
        return true;
    }

    bool OnMouseLeave() override {
        m_hover = {-1, -1};
        m_buttonHover = RowButton::None;
        FollowPreview();
        return true;
    }

    bool OnMouseUp(POINT pt) override;
    bool OnTimer(UINT_PTR id) override;

    // The wheel scrolls the column under the mouse.
    bool OnWheel(int delta) override {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(m_hwnd, &pt);
        pt.x -= m_contentOffset.x;
        pt.y -= m_contentOffset.y;
        for (size_t i = m_first; i < m_columns.size(); i++) {
            Column& column = m_columns[i];
            if (pt.x >= column.rect.left && pt.x < column.rect.right) {
                // (Its menu or a pin's icons shown: they all fit.)
                if (column.menu || !column.lookPin.empty()) {
                    return false;
                }
                // 3 rows a notch; a touchpad's smaller steps add up.
                m_wheelRest += delta;
                const int steps = m_wheelRest / (WHEEL_DELTA / 3);
                m_wheelRest -= steps * (WHEEL_DELTA / 3);
                // The rows it shows (the columns after the first start with
                // "Open in Explorer").
                const int shown =
                    (int)std::lround((column.rect.bottom - column.rect.top) /
                                     m_rowHeight) -
                    (i > 0 ? 1 : 0);
                const int rows = (int)column.items.size();
                const int top = std::clamp(column.top - steps, 0,
                                           std::max(0, rows - shown));
                if (top == column.top) {
                    return false;
                }
                column.top = top;
                m_hover = {-1, -1};
                return true;
            }
        }
        return false;
    }

    bool OnAppMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;

    void OnDestroy() override {
        // The folders being read give up first, so the waits below are
        // short (another panel later gets a flag of its own).
        *m_readStop = true;
        m_readStop = std::make_shared<std::atomic<bool>>(false);
        m_drop.Revoke();
        m_dropTarget = nullptr;
        StopWatching();
        m_thumbs.Stop();
        EndLookColorDialog();
        // (A folder dialog still open is closed.) What the threads posted to
        // this window, about to go, is freed here.
        for (HANDLE& thread : m_threads) {
            EndFileDialogThread(&thread);
        }
        m_threads.clear();
        // (A network favorite still being looked at: canceled; it holds its
        // own copy of what it needs.)
        for (HANDLE thread : m_lookThreads) {
            CancelSynchronousIo(thread);
            WaitForSingleObject(thread, 5000);
            CloseHandle(thread);
        }
        m_lookThreads.clear();
        MSG msg;
        while (m_hwnd && PeekMessage(&msg, m_hwnd, WM_APP_FOLDER_LIST,
                                     WM_APP_FOLDER_PICKED, PM_REMOVE)) {
            if (msg.message == WM_APP_FOLDER_LIST) {
                delete (FolderListJob*)msg.lParam;
            } else {
                delete (FolderPickJob*)msg.lParam;
            }
        }
    }

   private:
    // Where a favorite file is, on a click (see FavoriteFilePlace): one on a
    // network share looked at on a thread, a moment at most (a server that's
    // off takes up to a minute to say so, and the island would stand still
    // meanwhile); no answer by then: away, its look canceled.
    FavoritePlace FavoritePlaceNow(const std::wstring& path) {
        const bool remote =
            path.starts_with(L"\\\\") ||
            (path.size() >= 3 &&
             GetDriveType(path.substr(0, 3).c_str()) == DRIVE_REMOTE);
        if (!remote) {
            return FavoriteFilePlace(path, true);
        }
        auto look = std::make_shared<FavoriteLook>();
        look->path = path;
        auto* own = new std::shared_ptr<FavoriteLook>(look);
        HANDLE thread =
            CreateThread(nullptr, 0, FavoriteLookProc, own, 0, nullptr);
        if (!thread) {
            delete own;
            return FavoritePlace::Away;
        }
        if (WaitForSingleObject(thread, kFavoriteLookMs) == WAIT_OBJECT_0) {
            CloseHandle(thread);
            return (FavoritePlace)look->place.load();
        }
        CancelSynchronousIo(thread);
        // (Still going: waited for when the panel goes; the ones done by
        // now let go.)
        std::erase_if(m_lookThreads, [](HANDLE each) {
            if (WaitForSingleObject(each, 0) == WAIT_OBJECT_0) {
                CloseHandle(each);
                return true;
            }
            return false;
        });
        m_lookThreads.push_back(thread);
        return FavoritePlace::Away;
    }
    std::vector<HANDLE> m_lookThreads;

    // Files dragged out: the question's parts (see AskHit).
    static constexpr int kAskCopy = 1;
    static constexpr int kAskMove = 2;
    static constexpr int kAskRemember = 3;

    void EndFileDrag() {
        const bool held = m_fileDrag.column >= 0;
        m_fileDrag = {-1, -1};
        if (held && GetCapture() == m_hwnd) {
            ReleaseCapture();
        }
    }

    // A file or a folder dragged out, copied or moved as chosen (asked the
    // first time: the question shows under the columns, then it's dragged
    // again); Windows' own drag (its picture, the target's highlight).
    void StartFileDrag(int column, int row) {
        const FolderEntry* item = ItemAt({column, row});
        if (!item) {
            return;
        }
        const int choice =
            m_dragOnce ? m_dragOnce : g_settings.islandFolderDrag;
        if (FolderDragAsks(choice)) {
            m_asking = true;
            m_askHover = 0;
            m_askRemember = choice == 0;
            Layout();
            return;
        }
        m_dragOnce = 0;
        // (Under System32, found with the redirection off for a moment, as
        // the list was read: the 32-bit Windhawk process would look in
        // SysWOW64. The app it's dropped on gets the System32 path.)
        PVOID redirection = nullptr;
        const bool unredirected =
            !Wow64WindowsFolderOf(item->path).empty() &&
            Wow64DisableWow64FsRedirection(&redirection) != FALSE;
        winrt::com_ptr<IShellItem> shellItem;
        winrt::com_ptr<IDataObject> data;
        const bool found =
            SUCCEEDED(SHCreateItemFromParsingName(
                item->path.c_str(), nullptr, IID_PPV_ARGS(shellItem.put()))) &&
            SUCCEEDED(shellItem->BindToHandler(nullptr, kBHID_DataObject,
                                               IID_PPV_ARGS(data.put())));
        if (unredirected) {
            Wow64RevertWow64FsRedirection(redirection);
        }
        if (!found) {
            return;
        }
        // (Its column may change while it's dragged: a folder opening under
        // it, the list read again. The moved file's column follows by
        // itself, see WatchColumns.)
        m_dragOut = item->path;
        DWORD effect = DROPEFFECT_NONE;
        SHDoDragDrop(m_hwnd, data.get(), nullptr,
                     choice == 2 ? DROPEFFECT_MOVE : DROPEFFECT_COPY, &effect);
        m_dragOut.clear();
    }

    // The question's parts: its buttons and its box (0: none).
    int AskHit(POINT pt) const {
        return PointInRect(pt, m_askCopy)       ? kAskCopy
               : PointInRect(pt, m_askMove)     ? kAskMove
               : PointInRect(pt, m_askRememberRect) ? kAskRemember
                                                : 0;
    }

    // The list threads that ended let go (each read starts one).
    void ForgetEndedThreads() {
        std::erase_if(m_threads, [](HANDLE thread) {
            if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0) {
                CloseHandle(thread);
                return true;
            }
            return false;
        });
    }

    // A folder's column read again (its rows as they are now; shown as they
    // were until then, the new ones coming in).
    void ReloadColumn(int index) {
        Column& column = m_columns[index];
        if (column.path.empty() || column.loading) {
            return;
        }
        if (column.reading) {
            column.readAgain = true;
            return;
        }
        auto* job = new FolderListJob{m_hwnd,        column.path,  index,
                                      m_generation,  {},           column.order,
                                      column.filter, true};
        job->stop = m_readStop;
        ForgetEndedThreads();
        if (HANDLE thread =
                CreateThread(nullptr, 0, FolderListThreadProc, job, 0, nullptr)) {
            m_threads.push_back(thread);
            column.reading = true;
        } else {
            delete job;
        }
    }

    // The folders shown are watched while the panel is open (only those, and
    // only then, on a thread of their own): a file added, removed or renamed
    // in one, by anything, reads its column again (see OnAppMessage,
    // OnTimer).
    void WatchColumns() {
        std::vector<std::wstring> paths;
        for (size_t i = 1; m_shown && i < m_columns.size(); i++) {
            const std::wstring& path = m_columns[i].path;
            if (!path.empty() && !path.starts_with(kPlacePrefix)) {
                paths.push_back(path);
            }
        }
        // And the folders of the favorite files on this computer's drives:
        // one deleted, moved or renamed leaves the list (see CheckFavorites),
        // nothing looked at meanwhile (a disk that's stopped stays so). With
        // favorites, the thread also notices a drive coming or going.
        bool favorites = m_shown && !m_hiddenPins.empty();
        for (const auto& pin : m_pins) {
            if (!m_shown || !pin.starts_with(kFilePinPrefix)) {
                continue;
            }
            favorites = true;
            const std::wstring path = pin.substr(wcslen(kFilePinPrefix));
            const size_t slash = path.find_last_of(L'\\');
            if (slash == std::wstring::npos || !IsLocalDrivePath(path)) {
                continue;
            }
            const std::wstring folder = path.substr(0, slash + 1);
            if (std::none_of(paths.begin(), paths.end(),
                             [&](const std::wstring& each) {
                                 return _wcsicmp(FolderKey(each).c_str(),
                                                 FolderKey(folder).c_str()) ==
                                        0;
                             })) {
                paths.push_back(folder);
            }
        }
        const bool wanted = !paths.empty() || favorites;
        if (paths == m_watchedPaths && wanted == m_watchWanted) {
            return;
        }
        if (!wanted) {
            StopWatching();
            return;
        }
        m_watchWanted = true;
        m_watchedPaths = paths;
        if (m_watchThread) {
            AcquireSRWLockExclusive(&m_watch->lock);
            m_watch->paths = std::move(paths);
            m_watchSet = ++m_watch->set;
            ReleaseSRWLockExclusive(&m_watch->lock);
            SetEvent(m_watch->kick);
            return;
        }
        m_watch = std::make_unique<FolderWatch>();
        m_watch->panel = m_hwnd;
        m_watch->paths = std::move(paths);
        m_watchSet = m_watch->set;
        m_watch->stop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        m_watch->kick = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_watch->stop && m_watch->kick) {
            m_watchThread = CreateThread(nullptr, 0, FolderWatchThreadProc,
                                         m_watch.get(), 0, nullptr);
        }
        if (!m_watchThread) {
            const std::vector<std::wstring> wanted = m_watchedPaths;
            StopWatching();
            // (Not tried again for the same folders.)
            m_watchedPaths = wanted;
            m_watchWanted = true;
        }
    }
    void StopWatching() {
        if (m_watchThread) {
            SetEvent(m_watch->stop);
            WaitForSingleObject(m_watchThread, INFINITE);
            CloseHandle(m_watchThread);
            m_watchThread = nullptr;
        }
        if (m_watch) {
            if (m_watch->stop) {
                CloseHandle(m_watch->stop);
            }
            if (m_watch->kick) {
                CloseHandle(m_watch->kick);
            }
            m_watch.reset();
        }
        m_watchedPaths.clear();
        m_watchWanted = false;
    }

    // The column's opened row found again by the folder shown beside it (its
    // order or its contents changed); that folder gone, the columns after it
    // go too. `scroll`: the row brought into view.
    void FindOpened(size_t index, bool scroll) {
        Column& column = m_columns[index];
        if (index + 1 >= m_columns.size()) {
            column.opened = -1;
            return;
        }
        const std::wstring& path = m_columns[index + 1].path;
        column.opened = -1;
        for (size_t i = 0; i < column.items.size(); i++) {
            if (!path.empty() && ItemPath(column.items[i]) == path) {
                column.opened = (int)i;
                break;
            }
        }
        if (column.opened < 0) {
            m_columns.resize(index + 1);
            m_generation++;
            m_first = m_columns.size() > kShownColumns
                          ? m_columns.size() - kShownColumns
                          : 0;
            return;
        }
        if (scroll) {
            const int shown = std::max(1, m_visibleRows - 1);
            const int rows = (int)column.items.size();
            column.top = std::clamp(column.opened - shown / 2, 0,
                                    std::max(0, rows - shown));
        }
    }

    // The search: in the folder shown last (the box beside "Back"), by name
    // ("report") or by type ("*.pdf" or ".pdf").
    bool SearchShown() const {
        return m_columns.size() > 1 && m_columns.back().lookPin.empty();
    }
    void ClearSearch() {
        m_search.clear();
        m_searchEdit.Reset();
    }
    void ApplySearch() {
        std::wstring query = m_search;
        while (!query.empty() && query.back() == L' ') {
            query.pop_back();
        }
        while (!query.empty() && query.front() == L' ') {
            query.erase(0, 1);
        }
        std::wstring type;
        if (query.starts_with(L"*.")) {
            type = query.substr(1);
        } else if (query.starts_with(L".") && query.size() > 1) {
            type = query;
        }
        // (And each column's own filter.)
        for (size_t i = 1; i < m_columns.size(); i++) {
            Column& column = m_columns[i];
            const bool searched = i + 1 == m_columns.size() && !query.empty();
            column.items.clear();
            for (const auto& item : column.all) {
                if (!FolderFilterShows(column.filter, item)) {
                    continue;
                }
                const bool found =
                    !searched || item.kind == FolderKind::More ||
                    (!type.empty()
                         ? item.name.size() >= type.size() &&
                               _wcsicmp(item.name.c_str() + item.name.size() -
                                            type.size(),
                                        type.c_str()) == 0
                         : MatchesSearch(item.name, query));
                if (found) {
                    column.items.push_back(item);
                }
            }
            if (searched) {
                column.top = 0;
            }
        }
    }

    void EndPinDrag() {
        const bool held = m_dragRow >= 0;
        m_dragRow = m_dropRow = -1;
        m_dragging = false;
        if (held && GetCapture() == m_hwnd) {
            ReleaseCapture();
        }
    }

    // The pin shown on a row of the first column while one is dragged (the
    // dragged one at the row it would go to).
    int PinShownAt(int row) const {
        if (!m_dragging || m_dragRow < 0 || row >= (int)m_pins.size()) {
            return row;
        }
        if (row == m_dropRow) {
            return m_dragRow;
        }
        int from = row;
        if (m_dragRow < m_dropRow && row >= m_dragRow && row < m_dropRow) {
            from = row + 1;
        } else if (m_dragRow > m_dropRow && row > m_dropRow &&
                   row <= m_dragRow) {
            from = row - 1;
        }
        return from;
    }

    // At most this many columns shown (the first ones slide away).
    static constexpr size_t kShownColumns = 3;
    static constexpr int kPickPin = 1;

    // Row -2 of a column: "Open in Explorer"; column -2: the button back.
    struct Spot {
        int column;
        int row;
        bool operator==(const Spot&) const = default;
    };
    // The buttons at a row's end, under the mouse: a pinned one's "x" and
    // pencil (its icon and color), a folder's pin, the order of a folder's
    // items (its menu) and its direction (on "Open in Explorer").
    enum class RowButton {
        None,
        Remove,
        Pin,
        Sort,
        Direction,
        Edit,
        // A file's, under the mouse: the file itself, or its path, copied.
        CopyFile,
        CopyPath,
    };
    // A column's order menu (its rows shown instead of its items, see
    // MenuRows): row kMenuRow - k of a Spot is its row k.
    static constexpr int kMenuRow = -10;
    enum class MenuPart { Caption, Sort, Mixed, Filter };
    struct MenuRow {
        MenuPart part;
        int value;
    };
    // A pin's icons and colors (a column of their own, see LookCell): row
    // kLookRow - k of a Spot is its cell k.
    static constexpr int kLookRow = -100;
    struct Column {
        // The folder it lists ("" for the pins).
        std::wstring path;
        // What it shows (the search's matches in the last one, its kind of
        // file), and all of what's in it.
        std::vector<FolderEntry> items;
        std::vector<FolderEntry> all;
        bool loading = false;
        // Its items' order and the kind shown (each column its own), its
        // menu open, and the filter its items were cut with (see
        // FolderListJob::filtered).
        FolderOrder order;
        FolderFilter filter = FolderFilter::All;
        bool menu = false;
        FolderFilter readFilter = FolderFilter::All;
        // A pin's icons and colors instead (that pin).
        std::wstring lookPin;
        // Since when it's read, and when its rows came in (see RowIn).
        double loadingSince = 0;
        double shownAt = 0;
        // Changed meanwhile: read again (see MarkChanged).
        bool changed = false;
        // Read again quietly now (one at a time: a folder busy with changes,
        // like Downloads, isn't read many times at once, an older read
        // finishing last), and once more after it.
        bool reading = false;
        bool readAgain = false;
        // The row opened beside it, and the first row shown.
        int opened = -1;
        int top = 0;
        D2D1_RECT_F rect{};
    };

    const FolderEntry* ItemAt(const Spot& spot) const {
        if (spot.column < 0 || spot.column >= (int)m_columns.size()) {
            return nullptr;
        }
        const Column& column = m_columns[spot.column];
        return spot.row >= 0 && spot.row < (int)column.items.size()
                   ? &column.items[spot.row]
                   : nullptr;
    }

    bool Navigable(const Spot& spot) const {
        const FolderEntry* item = ItemAt(spot);
        if (!item) {
            return false;
        }
        if (item->kind == FolderKind::Place ||
            item->kind == FolderKind::PlaceToPin) {
            const FolderPlace* place = FindFolderPlace(item->path);
            return place && (!place->open || item->path == L"thispc");
        }
        return item->kind == FolderKind::Folder ||
               item->kind == FolderKind::Drive ||
               item->kind == FolderKind::WindowsPlaces;
    }

    // A folder's (or a place's) path to list or open.
    static std::wstring ItemPath(const FolderEntry& item) {
        if (item.kind != FolderKind::Place &&
            item.kind != FolderKind::PlaceToPin) {
            return item.path;
        }
        const FolderPlace* place = FindFolderPlace(item.path);
        if (!place) {
            return {};
        }
        if (item.path == L"thispc") {
            return std::wstring(kPlacePrefix) + L"thispc";
        }
        return place->open ? place->open : KnownFolderPath(place->folder);
    }

    // The search changed: the rows it brings in come in (the ones kept stay
    // still).
    void SearchAgain() {
        std::vector<std::wstring> before;
        if (!m_columns.empty()) {
            for (const auto& item : m_columns.back().items) {
                before.push_back(item.path);
            }
        }
        ApplySearch();
        const double now = NowSeconds();
        if (!m_columns.empty()) {
            for (const auto& item : m_columns.back().items) {
                if (std::find(before.begin(), before.end(), item.path) ==
                    before.end()) {
                    m_arrivals.push_back({item.path, now});
                }
            }
        }
        StartIslandAnimation();
        Layout();
    }

    // Rows coming in: a column's, one after the other (opened, read, sorted;
    // see Column::shownAt), or single ones (new in a folder, found by the
    // search; see m_arrivals). From 0 to 1.
    static constexpr double kRowInSeconds = 0.18;
    static constexpr double kRowInStep = 0.015;
    static constexpr int kRowInCascade = 10;
    static constexpr double kRowsInSeconds =
        kRowInSeconds + kRowInCascade * kRowInStep;
    // "Reading..." only for a folder that takes this long (no flash).
    static constexpr double kReadingDelay = 0.15;
    float RowIn(const Column& column,
                int shown,
                const std::wstring& path,
                double now) const {
        double since =
            now - column.shownAt - std::min(shown, kRowInCascade) * kRowInStep;
        for (const auto& [arrived, at] : m_arrivals) {
            if (arrived == path) {
                since = std::min(since, now - at);
            }
        }
        const double t = std::clamp(since / kRowInSeconds, 0.0, 1.0);
        return (float)(1 - (1 - t) * (1 - t) * (1 - t));
    }

    // Where files dragged over the panel would go: a folder's row (a pinned
    // one, a drive), or the column's own folder around its files; spot: the
    // row (or {column, -3}: the whole column), also a row that only opens
    // (This PC). Never where the file dragged out of the panel already is,
    // or into itself.
    std::wstring DropFolderAt(POINT pt, Spot* spot) const {
        *spot = {-1, -1};
        for (size_t i = m_first; i < m_columns.size(); i++) {
            const Column& column = m_columns[i];
            if (pt.x < column.rect.left || pt.x >= column.rect.right ||
                pt.y < column.rect.top || pt.y >= column.rect.bottom) {
                continue;
            }
            const int header = i > 0 ? 1 : 0;
            const int shown = (int)((pt.y - column.rect.top) / m_rowHeight);
            const int row = column.top + shown - header;
            const FolderEntry* item =
                shown >= header ? ItemAt({(int)i, row}) : nullptr;
            std::wstring folder;
            if (item && Navigable({(int)i, row})) {
                *spot = {(int)i, row};
                const FolderPlace* place =
                    item->kind == FolderKind::Place ||
                            item->kind == FolderKind::PlaceToPin
                        ? FindFolderPlace(item->path)
                        : nullptr;
                if (item->kind == FolderKind::Folder ||
                    item->kind == FolderKind::Drive ||
                    (place && !place->open)) {
                    folder = ItemPath(*item);
                }
            } else if (i > 0 && !column.loading && !column.path.empty() &&
                       !column.path.starts_with(kPlacePrefix) &&
                       (!item || item->kind == FolderKind::File ||
                        item->kind == FolderKind::More)) {
                *spot = {(int)i, -3};
                folder = column.path;
            }
            if (!m_dragOut.empty() && !folder.empty()) {
                const std::wstring key = FolderKey(folder);
                const std::wstring out = FolderKey(m_dragOut);
                const size_t slash = out.find_last_of(L'\\');
                const std::wstring parent = slash == std::wstring::npos
                                                ? std::wstring()
                                                : out.substr(0, slash);
                if (_wcsicmp(key.c_str(), parent.c_str()) == 0 ||
                    _wcsicmp(key.c_str(), out.c_str()) == 0 ||
                    (key.size() > out.size() && key[out.size()] == L'\\' &&
                     _wcsnicmp(key.c_str(), out.c_str(), out.size()) == 0)) {
                    *spot = {-1, -1};
                    folder.clear();
                }
            }
            return folder;
        }
        return {};
    }
    void EndDropHover() {
        m_filesOver = false;
        m_dropTarget = nullptr;
        m_dropFolder.clear();
        KillTimer(m_hwnd, kFolderOpenTimerId);
        if (!(m_dropSpot == Spot{-1, -1})) {
            m_dropSpot = {-1, -1};
            Render();
        }
    }

    // The columns shown slid by as many as went out of view on a side
    // (opening a folder past the third column, or going back), from where
    // they were (see ColumnSlide).
    static constexpr double kSlideSeconds = 0.24;
    void SlideColumns(size_t firstBefore) {
        if (m_first == firstBefore) {
            return;
        }
        m_slideFrom = ((float)m_first - (float)firstBefore) * m_columnStep;
        m_slideAt = NowSeconds();
        StartIslandAnimation();
    }
    float ColumnSlide(double now) const {
        const double t =
            std::clamp((now - m_slideAt) / kSlideSeconds, 0.0, 1.0);
        return (float)(m_slideFrom * (1 - t) * (1 - t) * (1 - t));
    }

    // Files dropped in `folder` (their names, from Windows' list of them).
    static constexpr double kDroppedSeconds = 1.6;
    void NoteDropped(IDataObject* data, const std::wstring& folder) {
        FORMATETC format{CF_HDROP, nullptr, DVASPECT_CONTENT, -1,
                         TYMED_HGLOBAL};
        STGMEDIUM medium{};
        if (folder.empty() || FAILED(data->GetData(&format, &medium))) {
            return;
        }
        if (auto drop = (HDROP)GlobalLock(medium.hGlobal)) {
            const double now = NowSeconds();
            const UINT count = DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0);
            for (UINT i = 0; i < count && i < 100; i++) {
                std::wstring path(DragQueryFile(drop, i, nullptr, 0), L'\0');
                DragQueryFile(drop, i, path.data(), (UINT)path.size() + 1);
                const size_t slash = FolderKey(path).find_last_of(L'\\');
                m_dropped.push_back({FolderKey(folder),
                                     slash == std::wstring::npos
                                         ? path
                                         : FolderKey(path).substr(slash + 1),
                                     now});
            }
            GlobalUnlock(medium.hGlobal);
        }
        ReleaseStgMedium(&medium);
    }
    // A column read: the files dropped in its folder lit now, the first of
    // them brought into view.
    void RevealDropped(size_t index) {
        Column& column = m_columns[index];
        const std::wstring folder = FolderKey(column.path);
        bool shown = false;
        for (auto& dropped : m_dropped) {
            if (dropped.seenAt || _wcsicmp(dropped.folder.c_str(),
                                           folder.c_str()) != 0) {
                continue;
            }
            for (size_t k = 0; k < column.items.size(); k++) {
                if (_wcsicmp(column.items[k].name.c_str(),
                             dropped.name.c_str()) != 0) {
                    continue;
                }
                dropped.seenAt = NowSeconds();
                const int rows = std::max(1, m_visibleRows - 1);
                const int row = (int)k;
                if (!shown &&
                    (row < column.top || row >= column.top + rows)) {
                    column.top = std::clamp(
                        row - rows / 2, 0,
                        std::max(0, (int)column.items.size() - rows));
                }
                shown = true;
                break;
            }
        }
        if (shown) {
            StartIslandAnimation();
        }
    }
    // How lit a row is for a file just dropped (0 to 1).
    float DroppedLit(const Column& column,
                     const FolderEntry& item,
                     double now) const {
        for (const auto& dropped : m_dropped) {
            if (dropped.seenAt > 0 &&
                _wcsicmp(dropped.name.c_str(), item.name.c_str()) == 0 &&
                _wcsicmp(dropped.folder.c_str(),
                         FolderKey(column.path).c_str()) == 0) {
                return (float)std::clamp(
                    1 - (now - dropped.seenAt) / kDroppedSeconds, 0.0, 1.0);
            }
        }
        return 0;
    }

    void LoadPins() {
        // (Only the ones still known, so a row is its pin; a favorite file on
        // a drive not there now kept apart, see m_hiddenPins.)
        m_pins.clear();
        m_hiddenPins.clear();
        m_looks = LoadFolderLooks();
        Column pins;
        bool gone = false;
        for (const auto& pin : LoadFolderPins()) {
            if (pin.starts_with(kFilePinPrefix)) {
                const std::wstring path = pin.substr(wcslen(kFilePinPrefix));
                switch (FavoriteFilePlace(path, false)) {
                    case FavoritePlace::Here: {
                        m_pins.push_back(pin);
                        const size_t slash = path.find_last_of(L'\\');
                        FolderEntry entry{
                            FolderKind::File,
                            slash == std::wstring::npos ? path
                                                        : path.substr(slash + 1),
                            path, true};
                        pins.items.push_back(std::move(entry));
                        break;
                    }
                    case FavoritePlace::Away:
                        m_hiddenPins.push_back(pin);
                        break;
                    case FavoritePlace::Gone:
                        gone = true;
                        break;
                }
            } else if (pin.starts_with(kPlacePrefix)) {
                const std::wstring token = pin.substr(wcslen(kPlacePrefix));
                if (const FolderPlace* place = FindFolderPlace(token)) {
                    m_pins.push_back(pin);
                    pins.items.push_back({FolderKind::Place,
                                          Tr(place->english, place->portuguese),
                                          token, true});
                }
            } else {
                m_pins.push_back(pin);
                const size_t slash = pin.find_last_of(L'\\', pin.size() - 2);
                std::wstring name = slash == std::wstring::npos
                                        ? pin
                                        : pin.substr(slash + 1);
                if (!name.empty() && name.back() == L'\\') {
                    name.pop_back();
                }
                pins.items.push_back({FolderKind::Folder, name, pin, true});
            }
        }
        // (The favorite files' types, for their icons.)
        NameFolderTypes(pins.items);
        ApplyLooks(pins);
        pins.items.push_back({FolderKind::AddFolder,
                              Tr(L"Add a folder", L"Adicionar pasta"), L""});
        pins.items.push_back({FolderKind::WindowsPlaces,
                              Tr(L"Windows places", L"Locais do Windows"),
                              L""});
        m_columns.clear();
        m_columns.push_back(std::move(pins));
        m_first = 0;
        m_generation++;
        if (gone) {
            SavePins();
        }
    }

    // The pins kept: the ones shown, and the favorite files on a drive not
    // there now (see LoadPins).
    void SavePins() const {
        std::vector<std::wstring> pins = m_pins;
        pins.insert(pins.end(), m_hiddenPins.begin(), m_hiddenPins.end());
        SaveFolderPins(pins);
    }

    // The first column made again (a pin added or gone), the others as they
    // were, the row they came from still lit.
    void ReloadPinsKeepingColumns() {
        const std::wstring opened =
            m_columns.size() > 1 ? m_columns[1].path : std::wstring();
        std::vector<Column> rest(m_columns.begin() + (m_columns.empty() ? 0 : 1),
                                 m_columns.end());
        LoadPins();
        m_columns.insert(m_columns.end(), rest.begin(), rest.end());
        if (!opened.empty()) {
            const auto& items = m_columns[0].items;
            for (size_t i = 0; i < items.size(); i++) {
                if (items[i].pinned && ItemPath(items[i]) == opened) {
                    m_columns[0].opened = (int)i;
                }
            }
        }
        m_first = m_columns.size() > kShownColumns
                      ? m_columns.size() - kShownColumns
                      : 0;
        m_hover = {-1, -1};
        m_buttonHover = RowButton::None;
    }

    // While it's open (see WatchColumns): a favorite file of `folder`, a
    // folder watched that changed, deleted, moved or renamed goes from the
    // list (only that folder's are looked at: no other disk woken). No
    // `folder`: a drive came or went, the favorites shown again (the ones on
    // it come or go). True if the list changed (drawn again).
    bool CheckFavorites(const std::wstring& folder) {
        if (m_dragging || m_dragRow >= 0 || m_fileDrag.column >= 0 ||
            m_asking) {
            return false;
        }
        bool changed = folder.empty() && !m_hiddenPins.empty();
        for (const auto& pin : m_pins) {
            if (changed || !pin.starts_with(kFilePinPrefix)) {
                continue;
            }
            const std::wstring path = pin.substr(wcslen(kFilePinPrefix));
            const size_t slash = path.find_last_of(L'\\');
            changed =
                folder.empty() ||
                (slash != std::wstring::npos &&
                 _wcsicmp(FolderKey(path.substr(0, slash)).c_str(),
                          folder.c_str()) == 0 &&
                 FavoriteFilePlace(path, false) != FavoritePlace::Here);
        }
        if (changed) {
            ReloadPinsKeepingColumns();
            StartIslandAnimation();
            Layout();
        }
        return changed;
    }

    // A file's star (in a folder): a favorite now, or not anymore.
    void ToggleFavorite(const std::wstring& path) {
        const std::wstring pin = kFilePinPrefix + path;
        if (const auto it = std::find(m_pins.begin(), m_pins.end(), pin);
            it != m_pins.end()) {
            m_pins.erase(it);
        } else {
            const size_t files =
                std::count_if(m_pins.begin(), m_pins.end(),
                              [](const std::wstring& each) {
                                  return each.starts_with(kFilePinPrefix);
                              }) +
                m_hiddenPins.size();
            if (files >= kMostFavoriteFiles) {
                ShowIslandNotice(0xE734, 0xFF9F0A,
                                 Tr(L"Favorites are full",
                                    L"Os favoritos estão cheios"),
                                 Tr(L"Up to 40 favorite files",
                                    L"Até 40 arquivos favoritos"));
                return;
            }
            m_pins.push_back(pin);
        }
        SavePins();
        ReloadPinsKeepingColumns();
        StartIslandAnimation();
        Layout();
    }

    bool IsFavorite(const std::wstring& path) const {
        return std::find(m_pins.begin(), m_pins.end(),
                         kFilePinPrefix + path) != m_pins.end();
    }

    // The pins' own icons and colors on their rows (see m_looks).
    void ApplyLooks(Column& pins) const {
        for (size_t i = 0; i < pins.items.size() && i < m_pins.size(); i++) {
            pins.items[i].glyph = 0;
            pins.items[i].color = -1;
            for (const auto& look : m_looks) {
                if (look.pin == m_pins[i]) {
                    pins.items[i].glyph = look.glyph;
                    pins.items[i].color = look.color;
                }
            }
        }
    }

    // A pin's icon or color chosen (`glyph` 0: its usual one; `color` -1:
    // none), kept; both reset by its "Back to default".
    void SetLook(const std::wstring& pin, WCHAR glyph, int color) {
        // (Read again, with the pins: another island's panel may have
        // changed them since.)
        m_looks = LoadFolderLooks();
        const std::vector<std::wstring> pins = LoadFolderPins();
        std::erase_if(m_looks, [&](const FolderLook& look) {
            return look.pin == pin ||
                   std::find(pins.begin(), pins.end(), look.pin) == pins.end();
        });
        if ((glyph || color >= 0) &&
            std::find(m_pins.begin(), m_pins.end(), pin) != m_pins.end()) {
            m_looks.push_back({pin, glyph, color});
        }
        SaveFolderLooks(m_looks);
        if (!m_columns.empty()) {
            ApplyLooks(m_columns[0]);
        }
    }
    const FolderLook* LookOf(const std::wstring& pin) const {
        for (const auto& look : m_looks) {
            if (look.pin == pin) {
                return &look;
            }
        }
        return nullptr;
    }

    // A pin's icons and colors closed (its pencil again, or "Apply").
    void CloseLook() {
        m_columns.resize(1);
        m_columns[0].opened = -1;
        m_generation++;
        Layout();
    }

    // A pin's icons and colors, in a column beside it (its pencil).
    void OpenLook(int row) {
        if (row < 0 || row >= (int)m_pins.size()) {
            return;
        }
        m_columns.resize(1);
        m_columns[0].opened = row;
        Column column;
        column.lookPin = m_pins[row];
        column.shownAt = NowSeconds();
        m_generation++;
        m_columns.push_back(std::move(column));
        m_first = 0;
        ClearSearch();
        ApplySearch();
        StartIslandAnimation();
        Layout();
    }

    // The order menus closed (true if one was open).
    bool CloseMenus() {
        bool closed = false;
        for (auto& column : m_columns) {
            closed = closed || column.menu;
            column.menu = false;
        }
        return closed;
    }

    // A column's menu: the orders (the one chosen again goes the other way),
    // folders mixed with the files, and the kinds shown.
    static const std::vector<MenuRow>& MenuRows() {
        static const std::vector<MenuRow> rows = [] {
            std::vector<MenuRow> rows{{MenuPart::Caption, 0}};
            for (int i = 0; i < (int)FolderSort::Count; i++) {
                rows.push_back({MenuPart::Sort, i});
            }
            rows.push_back({MenuPart::Mixed, 0});
            rows.push_back({MenuPart::Caption, 1});
            for (int i = 0; i < (int)FolderFilter::Count; i++) {
                rows.push_back({MenuPart::Filter, i});
            }
            return rows;
        }();
        return rows;
    }
    static PCWSTR SortName(FolderSort sort) {
        switch (sort) {
            case FolderSort::Date:
                return Tr(L"Date modified", L"Data de modificação");
            case FolderSort::Type:
                return Tr(L"Type", L"Tipo");
            case FolderSort::Size:
                return Tr(L"Size", L"Tamanho");
            default:
                return Tr(L"Name", L"Nome");
        }
    }
    static PCWSTR FilterName(FolderFilter filter) {
        switch (filter) {
            case FolderFilter::Folders:
                return Tr(L"Folders", L"Pastas");
            case FolderFilter::Pictures:
                return Tr(L"Pictures", L"Imagens");
            case FolderFilter::Videos:
                return Tr(L"Videos", L"Vídeos");
            case FolderFilter::Documents:
                return Tr(L"Documents", L"Documentos");
            case FolderFilter::Music:
                return Tr(L"Music", L"Músicas");
            case FolderFilter::Compressed:
                return Tr(L"Compressed", L"Compactados");
            case FolderFilter::Other:
                return Tr(L"Other files", L"Outros arquivos");
            default:
                return Tr(L"Everything", L"Tudo");
        }
    }
    // Which come first, the way a column is sorted (its arrow's tip).
    static PCWSTR OrderWay(const FolderOrder& order) {
        switch (order.sort) {
            case FolderSort::Date:
                return order.descending
                           ? Tr(L"Newest first", L"Mais recentes primeiro")
                           : Tr(L"Oldest first", L"Mais antigos primeiro");
            case FolderSort::Size:
                return order.descending
                           ? Tr(L"Largest first", L"Maiores primeiro")
                           : Tr(L"Smallest first", L"Menores primeiro");
            default:
                return order.descending ? Tr(L"Z to A", L"De Z a A")
                                        : Tr(L"A to Z", L"De A a Z");
        }
    }

    // A column's order or kind shown changed: sorted again, its folder
    // opened beside it kept (found again, brought into view), and read
    // again when it was cut (the first ones in the new order or kind). The
    // order is the one for the folders opened next too.
    void Reorder(int index, const FolderOrder& order, FolderFilter filter) {
        Column& column = m_columns[index];
        const bool filtered = filter != column.filter;
        column.order = order;
        column.filter = filter;
        if (m_order != order) {
            m_order = order;
            SetUserIntValue(L"folderOrder", FolderOrderValue(order));
        }
        SortFolderItems(column.all, column.order);
        column.top = 0;
        column.shownAt = NowSeconds();
        ApplySearch();
        if ((size_t)index + 1 < m_columns.size()) {
            FindOpened(index, true);
        }
        if (Cut(column.all) ||
            (filtered && column.readFilter != FolderFilter::All)) {
            ReloadColumn(index);
        }
        StartIslandAnimation();
        Layout();
    }

    // What's in a folder, in a column beside it (the ones after it go).
    void OpenColumn(Spot spot) {
        if (const FolderEntry* found = ItemAt(spot);
            found && !m_search.empty() && spot.column > 0) {
            const std::wstring path = found->path;
            ClearSearch();
            ApplySearch();
            const auto& all = m_columns[spot.column].items;
            for (size_t i = 0; i < all.size(); i++) {
                if (all[i].path == path) {
                    spot.row = (int)i;
                }
            }
        }
        const FolderEntry* item = ItemAt(spot);
        if (!item || m_columns[spot.column].opened == spot.row) {
            return;
        }
        m_columns.resize(spot.column + 1);
        m_columns[spot.column].opened = spot.row;
        Column column;
        m_generation++;
        if (item->kind == FolderKind::WindowsPlaces) {
            // The places not pinned yet.
            for (const auto& place : kFolderPlaces) {
                const std::wstring pin =
                    std::wstring(kPlacePrefix) + place.token;
                if (std::find(m_pins.begin(), m_pins.end(), pin) ==
                    m_pins.end()) {
                    column.items.push_back(
                        {FolderKind::PlaceToPin,
                         Tr(place.english, place.portuguese), place.token});
                }
            }
            column.all = column.items;
            column.shownAt = NowSeconds();
        } else {
            column = ReadColumn(ItemPath(*item), spot.column + 1);
        }
        AddColumn(std::move(column));
    }

    // A folder's column at `index`, read on a thread of its own (loading
    // until then).
    Column ReadColumn(const std::wstring& path, int index) {
        Column column;
        column.path = path;
        column.loading = true;
        column.loadingSince = NowSeconds();
        // (In the order chosen last.)
        column.order = m_order;
        auto* job = new FolderListJob{m_hwnd, column.path, index, m_generation,
                                      {}, column.order};
        job->stop = m_readStop;
        ForgetEndedThreads();
        if (HANDLE thread = CreateThread(nullptr, 0, FolderListThreadProc, job,
                                         0, nullptr)) {
            m_threads.push_back(thread);
        } else {
            delete job;
            column.loading = false;
        }
        return column;
    }

    // A new last column, sliding in (the first ones slide away past
    // kShownColumns).
    void AddColumn(Column column) {
        const size_t first = m_first;
        m_columns.push_back(std::move(column));
        m_first = m_columns.size() > kShownColumns
                      ? m_columns.size() - kShownColumns
                      : 0;
        ClearSearch();
        ApplySearch();
        StartIslandAnimation();
        Layout();
        SlideColumns(first);
    }

    void Layout() {
        const float s = m_scale;
        const float padding = std::round(8 * s);
        const float row = std::round(30 * s);
        const float gap = std::round(4 * s);
        // (Narrower on a screen without room for the columns shown.)
        const float fit =
            ((float)(m_workRect.right - m_workRect.left) -
             std::round(24 * s) - padding * 2) /
                kShownColumns -
            gap;
        const float width =
            std::max(std::round(200 * s),
                     std::min(std::round(360 * s), std::floor(fit)));
        // As tall as the longest column shown (its menu, or a pin's icons
        // and colors), up to the screen's room.
        m_rowHeight = row;
        size_t longest = 1;
        for (size_t i = m_first; i < m_columns.size(); i++) {
            const Column& column = m_columns[i];
            const size_t rows = column.menu ? MenuRows().size()
                                : !column.lookPin.empty()
                                    ? (size_t)LookRows(width)
                                    : column.items.size();
            longest = std::max(longest, rows + (i > 0 ? 1 : 0));
        }
        // A folder open: a button back above the columns.
        const float back = m_columns.size() > 1 ? row : 0;
        const float room =
            std::max(MaxHeight() - padding * 2 - back, row * 4);
        m_visibleRows = std::max(4, (int)(room / row));
        const int rows = std::min((int)longest, m_visibleRows);
        float x = padding;
        for (size_t i = m_first; i < m_columns.size(); i++) {
            m_columns[i].rect = {x, padding + back, x + width,
                                 padding + back + rows * row};
            x += width + gap;
        }
        m_back = back > 0 ? D2D1_RECT_F{padding, padding,
                                        padding + std::round(120 * s),
                                        padding + back}
                          : D2D1_RECT_F{};
        // The search beside "Back", as long as the columns.
        m_searchBox = back > 0 ? D2D1_RECT_F{m_back.right + gap * 2,
                                             padding + std::round(2 * s),
                                             x - gap,
                                             padding + back - std::round(2 * s)}
                               : D2D1_RECT_F{};
        float height = padding * 2 + back + rows * row;
        // The question about files dragged out, under the columns.
        m_askRect = m_askCopy = m_askMove = m_askRememberRect = {};
        if (m_asking) {
            const float top = height - padding + gap;
            const float button = std::round(30 * s);
            const float right = x - gap;
            m_askRect = {padding, top, right, top + std::round(96 * s)};
            const float buttons = top + std::round(30 * s);
            const float half = (right - padding - gap) / 2;
            m_askCopy = {padding, buttons, padding + half, buttons + button};
            m_askMove = {padding + half + gap, buttons, right, buttons + button};
            m_askRememberRect = {padding, buttons + button + std::round(4 * s),
                             right, top + std::round(96 * s)};
            height = m_askRect.bottom + padding;
        }
        m_size = {(LONG)(x - gap + padding), (LONG)std::ceil(height)};
        m_columnStep = width + gap;
        WatchColumns();
    }

    Spot HitTest(POINT pt, RowButton* button) const {
        *button = RowButton::None;
        if (m_back.right > m_back.left && PointInRect(pt, m_back)) {
            return {-2, -2};
        }
        for (size_t i = m_first; i < m_columns.size(); i++) {
            const Column& column = m_columns[i];
            if (pt.x < column.rect.left || pt.x >= column.rect.right ||
                pt.y < column.rect.top || pt.y >= column.rect.bottom) {
                continue;
            }
            const int shown = (int)((pt.y - column.rect.top) / m_rowHeight);
            // A pin's icons and colors: their cells (the title does
            // nothing).
            if (!column.lookPin.empty()) {
                for (int k = 0; k <= kLookApply; k++) {
                    if (PointInRect(pt, LookCell(column.rect, k))) {
                        return {(int)i, kLookRow - k};
                    }
                }
                return {-1, -1};
            }
            // Row -2: "Open in Explorer" (above the items), the order at its
            // end (its menu, then its direction).
            const int header = i > 0 ? 1 : 0;
            if (shown < header) {
                if (pt.x >= column.rect.right - DirectionButtonWidth()) {
                    *button = RowButton::Direction;
                } else if (pt.x >= column.rect.right - HeaderButtonsWidth()) {
                    *button = RowButton::Sort;
                }
                return {(int)i, -2};
            }
            // Its menu open: the menu's rows.
            if (column.menu) {
                const int k = shown - header;
                return k < (int)MenuRows().size() &&
                               MenuRows()[k].part != MenuPart::Caption
                           ? Spot{(int)i, kMenuRow - k}
                           : Spot{-1, -1};
            }
            const int row = column.top + shown - header;
            if (row >= (int)column.items.size()) {
                return {-1, -1};
            }
            const FolderEntry& item = column.items[row];
            const bool atEnd =
                pt.x >= column.rect.right - std::round(28 * m_scale);
            const bool file = item.kind == FolderKind::File;
            if (item.pinned && atEnd) {
                *button = RowButton::Remove;
            } else if (item.pinned && file &&
                       pt.x >= column.rect.right - std::round(52 * m_scale)) {
                *button = RowButton::CopyPath;
            } else if (item.pinned && file &&
                       pt.x >= column.rect.right - std::round(76 * m_scale)) {
                *button = RowButton::CopyFile;
            } else if (item.pinned && !file &&
                       pt.x >= column.rect.right - std::round(52 * m_scale)) {
                *button = RowButton::Edit;
            } else if ((item.kind == FolderKind::Folder ||
                        item.kind == FolderKind::PlaceToPin) &&
                       !item.pinned &&
                       pt.x >= column.rect.right - std::round(50 * m_scale) &&
                       !atEnd) {
                *button = RowButton::Pin;
            } else if (item.kind == FolderKind::File && atEnd) {
                *button = RowButton::CopyPath;
            } else if (item.kind == FolderKind::File &&
                       pt.x >= column.rect.right - std::round(52 * m_scale)) {
                *button = RowButton::CopyFile;
            } else if (item.kind == FolderKind::File && !item.pinned &&
                       pt.x >= column.rect.right - std::round(76 * m_scale)) {
                *button = RowButton::Pin;
            }
            return {(int)i, row};
        }
        return {-1, -1};
    }

    ID2D1Bitmap* TypeIcon(ID2D1RenderTarget* target, const FolderEntry& item);

    float SortButtonWidth() const { return std::round(90 * m_scale); }
    float DirectionButtonWidth() const { return std::round(30 * m_scale); }
    // A folder with more than kFolderListMax (its rows end in "More in
    // Explorer").
    static bool Cut(const std::vector<FolderEntry>& items) {
        return !items.empty() && items.back().kind == FolderKind::More;
    }
    // The order's buttons at the end of a column's "Open in Explorer".
    float HeaderButtonsWidth() const {
        return SortButtonWidth() + DirectionButtonWidth() +
               std::round(4 * m_scale);
    }

    // A pin's icons and colors (see OpenLook): cells as tall as a row under
    // its title, the icons, then the colors and one for any other, then
    // "Back to default" and "Apply" (they're kept as they're chosen: it
    // closes them) as rows.
    static constexpr int kLookGlyphs = (int)std::size(kFolderLookGlyphs);
    static constexpr int kLookColors = (int)std::size(kFolderLookColors) + 1;
    static constexpr int kLookReset = kLookGlyphs + kLookColors;
    static constexpr int kLookApply = kLookReset + 1;
    int LookPerRow(float width) const {
        return std::max(1, (int)(width / m_rowHeight));
    }
    // Its rows under the title.
    int LookRows(float width) const {
        const int perRow = LookPerRow(width);
        return (kLookGlyphs + perRow - 1) / perRow +
               (kLookColors + perRow - 1) / perRow + 2;
    }
    D2D1_RECT_F LookCell(const D2D1_RECT_F& area, int k) const {
        const int perRow = LookPerRow(area.right - area.left);
        const int glyphRows = (kLookGlyphs + perRow - 1) / perRow;
        float top = area.top + m_rowHeight;
        if (k >= kLookReset) {
            top += (glyphRows + (kLookColors + perRow - 1) / perRow +
                    (k - kLookReset)) *
                   m_rowHeight;
            return {area.left, top, area.right, top + m_rowHeight};
        }
        int at = k;
        if (k >= kLookGlyphs) {
            top += glyphRows * m_rowHeight;
            at -= kLookGlyphs;
        }
        const float x = area.left + (at % perRow) * m_rowHeight;
        const float y = top + (float)(at / perRow) * m_rowHeight;
        return {x, y, x + m_rowHeight, y + m_rowHeight};
    }

    winrt::com_ptr<IWICImagingFactory> m_wic;
    winrt::com_ptr<IDWriteTextFormat> m_rowFormat;
    // An item's date at its row's end (sorted by date).
    winrt::com_ptr<IDWriteTextFormat> m_rowDateFormat;
    // The order arrow's tip laid out (see Draw), for those texts and format.
    winrt::com_ptr<IDWriteTextLayout> m_tipFirst;
    winrt::com_ptr<IDWriteTextLayout> m_tipWhich;
    PCWSTR m_tipText = nullptr;
    PCWSTR m_tipWhichText = nullptr;
    IDWriteTextFormat* m_tipFormat = nullptr;
    winrt::com_ptr<IDWriteTextFormat> m_rowIconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_rowSmallIconFormat;
    float m_rowScale = 0;
    // The order chosen last (folderOrder), for the folders opened next.
    FolderOrder m_order;
    std::vector<std::wstring> m_pins;
    // The pins' own icons and colors (see SetLook).
    std::vector<FolderLook> m_looks;
    // Windows' color dialog for a pin's color (see StartLookColorDialog),
    // and that pin.
    HANDLE m_colorThread = nullptr;
    std::wstring m_colorPin;
    void StartLookColorDialog(const std::wstring& pin);
    bool OnLookColorDone(LPARAM lParam);
    void EndLookColorDialog();
    std::vector<Column> m_columns;
    size_t m_first = 0;
    UINT m_generation = 0;
    int m_visibleRows = 12;
    float m_rowHeight = 30;
    int m_wheelRest = 0;
    Spot m_hover{-1, -1};
    RowButton m_buttonHover = RowButton::None;
    // A file just copied (or its path): a check on that button for a moment.
    std::wstring m_copiedPath;
    RowButton m_copiedButton = RowButton::None;
    static constexpr UINT_PTR kCopiedTimerId = 43;
    static constexpr UINT kCopiedMs = 1500;
    // The favorite files on a drive not there now (see LoadPins).
    std::vector<std::wstring> m_hiddenPins;
    D2D1_RECT_F m_back{};
    // The search in the folder shown last (see ApplySearch).
    std::wstring m_search;
    TextEdit m_searchEdit;
    D2D1_RECT_F m_searchBox{};
    // A file or a folder held (to drag out), and where the mouse went down
    // (on the screen).
    Spot m_fileDrag{-1, -1};
    POINT m_fileFrom{};
    // The question shown (copy or move?), its box and parts, the part under
    // the mouse, "Remember my choice", and a choice for the next drag only.
    bool m_asking = false;
    D2D1_RECT_F m_askRect{}, m_askCopy{}, m_askMove{}, m_askRememberRect{};
    int m_askHover = 0;
    bool m_askRemember = true;
    int m_dragOnce = 0;
    // A pin held (its row), where it would go, and whether it moved yet.
    int m_dragRow = -1;
    int m_dropRow = -1;
    POINT m_dragFrom{};
    bool m_dragging = false;
    // Open (its folders watched), and their watch: the thread, the folders
    // and their set's number (see WatchColumns).
    bool m_shown = false;
    std::unique_ptr<FolderWatch> m_watch;
    HANDLE m_watchThread = nullptr;
    std::vector<std::wstring> m_watchedPaths;
    UINT m_watchSet = 0;
    // Watched (or tried) for the favorite files too, even without folders.
    bool m_watchWanted = false;
    // A change seen: the columns read again in a moment (once for many).
    static constexpr UINT_PTR kFolderChangeTimerId = 42;
    static constexpr UINT kFolderChangeMs = 300;
    bool m_changePending = false;
    // Files dragged over it: the window taking them, the folder they'd go
    // in, its drop target, the row lit (opened after kFolderOpenMs), and the
    // file dragged out of the panel itself.
    static constexpr UINT_PTR kFolderOpenTimerId = 41;
    DropRegistration m_drop;
    std::wstring m_dropFolder;
    winrt::com_ptr<IDropTarget> m_dropTarget;
    Spot m_dropSpot{-1, -1};
    bool m_filesOver = false;
    std::wstring m_dragOut;
    // Single rows coming in (their paths), since when (see RowIn).
    std::vector<std::pair<std::wstring, double>> m_arrivals;
    // Files dropped (on a folder of the panel): it stays open, and they're
    // lit as they show up there (see NoteDropped, RevealDropped).
    bool m_keepOpen = false;
    struct Dropped {
        std::wstring folder;
        std::wstring name;
        double droppedAt;
        // When it showed in its folder's column (0: not yet).
        double seenAt = 0;
    };
    std::vector<Dropped> m_dropped;
    // The columns sliding by one (see SlideColumns), the step between two
    // columns, and the column going back out (see OnMouseUp, Draw).
    float m_slideFrom = 0;
    double m_slideAt = 0;
    float m_columnStep = 0;
    Column m_leaving;
    double m_leavingAt = 0;
    std::vector<HANDLE> m_threads;
    // The folders' reads give up once it's set (see OnDestroy).
    std::shared_ptr<std::atomic<bool>> m_readStop =
        std::make_shared<std::atomic<bool>>(false);
    // Icons by type (an extension, or "\\" for folders).
    std::vector<std::pair<std::wstring, winrt::com_ptr<ID2D1Bitmap>>> m_icons;
    // The files' thumbnails (islandFolderThumbnails), and a big one beside a
    // file the mouse rests on (after kFolderPreviewMs), like macOS' Quick
    // Look.
    ThumbnailCache m_thumbs;
    static constexpr UINT_PTR kFolderPreviewTimerId = 44;
    static constexpr UINT kFolderPreviewMs = 500;
    Spot m_previewSpot{-1, -1};
    bool m_previewShown = false;
    void FollowPreview() {
        if (m_hover == m_previewSpot) {
            return;
        }
        KillTimer(m_hwnd, kFolderPreviewTimerId);
        m_previewShown = false;
        m_previewSpot = {-1, -1};
        const FolderEntry* item = ItemAt(m_hover);
        if (g_settings.islandFolderThumbnails && item &&
            item->kind == FolderKind::File) {
            m_previewSpot = m_hover;
            SetTimer(m_hwnd, kFolderPreviewTimerId, kFolderPreviewMs, nullptr);
        }
    }
};

