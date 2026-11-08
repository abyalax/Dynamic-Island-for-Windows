// A reply being typed: where the caret is in it, and how far it's scrolled
// (it follows the caret, unless the wheel moved it), with its layout as last
// shown (for the up and down arrows).
struct TextEdit {
    size_t caret = 0;
    // Where the selection starts (it ends at the caret), or npos.
    size_t anchor = std::wstring::npos;
    float scroll = 0;
    bool follow = true;
    winrt::com_ptr<IDWriteTextLayout> layout;
    float lineHeight = 0;
    // Where the text was drawn (its top left, scrolled), and the box's
    // height, for clicks and Page Up / Page Down.
    D2D1_POINT_2F origin{};
    float visibleHeight = 0;
    void Reset() {
        caret = 0;
        anchor = std::wstring::npos;
        scroll = 0;
        follow = true;
        layout = nullptr;
    }
    bool Selecting() const { return anchor != std::wstring::npos && anchor != caret; }
    size_t SelectionStart() const { return std::min(anchor, caret); }
    size_t SelectionEnd() const { return std::max(anchor, caret); }
};

bool CopyTextToClipboard(HWND owner, const std::wstring& text) {
    if (!OpenClipboard(owner)) {
        return false;
    }
    EmptyClipboard();
    bool copied = false;
    const size_t bytes = (text.size() + 1) * sizeof(WCHAR);
    if (HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
        if (void* data = GlobalLock(memory)) {
            memcpy(data, text.c_str(), bytes);
            GlobalUnlock(memory);
            copied = SetClipboardData(CF_UNICODETEXT, memory) != nullptr;
        }
        if (!copied) {
            GlobalFree(memory);
        }
    }
    CloseClipboard();
    return copied;
}

// A file put on the clipboard as Explorer's "Copy" does (pasted in a folder,
// it's copied there): its path as a file list, and copy as what a paste
// does.
bool CopyFileToClipboard(HWND owner, const std::wstring& path) {
    // The list: DROPFILES, then the paths, each ending in a 0, and one more.
    const size_t bytes = sizeof(DROPFILES) + (path.size() + 2) * sizeof(WCHAR);
    HGLOBAL files = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bytes);
    HGLOBAL effect = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
    auto* drop = files ? (DROPFILES*)GlobalLock(files) : nullptr;
    auto* value = effect ? (DWORD*)GlobalLock(effect) : nullptr;
    if (drop) {
        drop->pFiles = sizeof(DROPFILES);
        drop->fWide = TRUE;
        memcpy((BYTE*)drop + sizeof(DROPFILES), path.c_str(),
               path.size() * sizeof(WCHAR));
        GlobalUnlock(files);
    }
    if (value) {
        *value = DROPEFFECT_COPY;
        GlobalUnlock(effect);
    }
    bool copied = false;
    if (drop && value && OpenClipboard(owner)) {
        EmptyClipboard();
        // (The system owns each one it takes.)
        if (SetClipboardData(CF_HDROP, files)) {
            files = nullptr;
            copied = true;
            static const UINT kDropEffect =
                RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
            if (SetClipboardData(kDropEffect, effect)) {
                effect = nullptr;
            }
        }
        CloseClipboard();
    }
    if (files) {
        GlobalFree(files);
    }
    if (effect) {
        GlobalFree(effect);
    }
    return copied;
}

// The selection goes (replaced by what's typed, or deleted).
void DeleteSelection(std::wstring& text, TextEdit& edit) {
    if (!edit.Selecting()) {
        edit.anchor = std::wstring::npos;
        return;
    }
    const size_t start = std::min(edit.SelectionStart(), text.size());
    const size_t end = std::min(edit.SelectionEnd(), text.size());
    text.erase(start, end - start);
    edit.caret = start;
    edit.anchor = std::wstring::npos;
}

// A character typed into a reply, at the caret: Shift+Enter (passed as
// '\n') and Ctrl+Enter a new line, Ctrl+Backspace the word before the caret,
// Ctrl+V pastes. Returns false for characters it doesn't take.
bool EditTextChar(std::wstring& text,
                  TextEdit& edit,
                  WCHAR c,
                  size_t maxLength,
                  HWND owner) {
    edit.caret = std::min(edit.caret, text.size());
    edit.follow = true;
    // Ctrl+A selects it all; Ctrl+C copies the selection, Ctrl+X cuts it.
    if (c == 0x01) {
        edit.anchor = 0;
        edit.caret = text.size();
        return true;
    }
    if (c == 0x03 || c == 0x18) {
        if (edit.Selecting()) {
            const size_t start = std::min(edit.SelectionStart(), text.size());
            const size_t end = std::min(edit.SelectionEnd(), text.size());
            CopyTextToClipboard(owner, text.substr(start, end - start));
            if (c == 0x18) {
                DeleteSelection(text, edit);
            }
        }
        return true;
    }
    // What's typed (or erased) replaces the selection.
    if (edit.Selecting() &&
        (c == L'\n' || c == 0x7F || c == L'\b' || c == 0x16 || c >= 0x20)) {
        DeleteSelection(text, edit);
        if (c == 0x7F || c == L'\b') {
            return true;
        }
    }
    edit.anchor = std::wstring::npos;
    auto insert = [&](const std::wstring& part) {
        const size_t room =
            maxLength > text.size() ? maxLength - text.size() : 0;
        const std::wstring added = part.substr(0, room);
        text.insert(edit.caret, added);
        edit.caret += added.size();
    };
    if (c == L'\n') {
        insert(L"\n");
    } else if (c == 0x7F) {
        size_t start = edit.caret;
        while (start > 0 && iswspace(text[start - 1])) {
            start--;
        }
        while (start > 0 && !iswspace(text[start - 1])) {
            start--;
        }
        text.erase(start, edit.caret - start);
        edit.caret = start;
    } else if (c == L'\b') {
        if (edit.caret > 0) {
            text.erase(edit.caret - 1, 1);
            edit.caret--;
        }
    } else if (c == 0x16) {
        if (OpenClipboard(owner)) {
            if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
                if (auto pasted = (PCWSTR)GlobalLock(data)) {
                    std::wstring part;
                    for (PCWSTR p = pasted; *p; p++) {
                        if (*p != L'\r') {
                            part += *p == L'\n' || *p >= 0x20 ? *p : L' ';
                        }
                    }
                    GlobalUnlock(data);
                    insert(part);
                }
            }
            CloseClipboard();
        }
    } else if (c >= 0x20) {
        insert(std::wstring(1, c));
    } else {
        return false;
    }
    return true;
}

// The arrows (Ctrl: by word), Home and End (of the line), and Delete, in a
// reply. Returns whether it took the key.
bool EditTextKey(std::wstring& text, TextEdit& edit, WPARAM key) {
    edit.caret = std::min(edit.caret, text.size());
    const bool ctrl = GetKeyState(VK_CONTROL) < 0;
    const bool shift = GetKeyState(VK_SHIFT) < 0;
    size_t& caret = edit.caret;
    const bool moves = key == VK_LEFT || key == VK_RIGHT || key == VK_HOME ||
                       key == VK_END || key == VK_UP || key == VK_DOWN ||
                       key == VK_PRIOR || key == VK_NEXT;
    // Delete takes the selection; moving with Shift selects, without it the
    // selection goes.
    if (key == VK_DELETE && edit.Selecting()) {
        DeleteSelection(text, edit);
        edit.follow = true;
        return true;
    }
    if (moves) {
        if (shift) {
            if (edit.anchor == std::wstring::npos) {
                edit.anchor = caret;
            }
        } else {
            edit.anchor = std::wstring::npos;
        }
    }
    // Ctrl+Home / Ctrl+Page Up: the start; Ctrl+End / Ctrl+Page Down: the
    // end.
    if (ctrl && (key == VK_HOME || key == VK_PRIOR)) {
        caret = 0;
        edit.follow = true;
        return true;
    }
    if (ctrl && (key == VK_END || key == VK_NEXT)) {
        caret = text.size();
        edit.follow = true;
        return true;
    }
    switch (key) {
        case VK_LEFT:
            if (caret > 0) {
                caret--;
                if (ctrl) {
                    while (caret > 0 && iswspace(text[caret])) {
                        caret--;
                    }
                    while (caret > 0 && !iswspace(text[caret - 1])) {
                        caret--;
                    }
                }
            }
            break;
        case VK_RIGHT:
            if (caret < text.size()) {
                caret++;
                if (ctrl) {
                    while (caret < text.size() && !iswspace(text[caret])) {
                        caret++;
                    }
                    while (caret < text.size() && iswspace(text[caret])) {
                        caret++;
                    }
                }
            }
            break;
        case VK_HOME:
            while (caret > 0 && text[caret - 1] != L'\n') {
                caret--;
            }
            break;
        case VK_END:
            while (caret < text.size() && text[caret] != L'\n') {
                caret++;
            }
            break;
        case VK_DELETE:
            if (caret < text.size()) {
                text.erase(caret, 1);
            }
            break;
        case VK_UP:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT: {
            // The same place on the line above or below (as shown), or a
            // box's height up or down.
            DWRITE_HIT_TEST_METRICS hit{};
            float x = 0, y = 0;
            if (!edit.layout || text.empty() ||
                FAILED(edit.layout->HitTestTextPosition(
                    (UINT32)std::min(caret, text.size()), FALSE, &x, &y,
                    &hit))) {
                return false;
            }
            const float line = edit.lineHeight > 0 ? edit.lineHeight : 16;
            const float page = std::max(line, edit.visibleHeight);
            const float to = key == VK_UP     ? y - line / 2
                             : key == VK_DOWN ? y + line * 1.5f
                             : key == VK_PRIOR
                                 ? y + line / 2 - page
                                 : y + line / 2 + page;
            if (to < 0) {
                caret = 0;
                break;
            }
            DWRITE_TEXT_METRICS metrics{};
            edit.layout->GetMetrics(&metrics);
            if (to > metrics.height) {
                caret = text.size();
                break;
            }
            BOOL trailing = FALSE, inside = FALSE;
            if (SUCCEEDED(edit.layout->HitTestPoint(x, to, &trailing, &inside,
                                                    &hit))) {
                caret = std::min(
                    text.size(),
                    (size_t)hit.textPosition + (trailing ? 1 : 0));
            }
            break;
        }
        default:
            return false;
    }
    edit.follow = true;
    return true;
}

// Text boxes work with the mouse like Windows' own: a drag from a click
// selects, a double click selects a word, Shift+click selects up to the
// click (see EditTextClick and Panel::HandleMessage).
struct EditDrag {
    // The box clicked last, and where (in its own coordinates).
    const std::wstring* text = nullptr;
    TextEdit* edit = nullptr;
    POINT click{};
    // The panel holding the mouse for a drag, the click's point minus the
    // window's, and whether the mouse moved (a selection, not a click).
    HWND window = nullptr;
    POINT shift{};
    bool moved = false;
    // The click before, for double clicks.
    const TextEdit* lastEdit = nullptr;
    DWORD lastTime = 0;
    POINT lastPoint{};
};
EditDrag g_editDrag;

// Where a point (where the box was drawn) falls in its text.
bool EditTextPosition(const std::wstring& text,
                      const TextEdit& edit,
                      POINT pt,
                      size_t* position) {
    BOOL trailing = FALSE, inside = FALSE;
    DWRITE_HIT_TEST_METRICS hit{};
    if (!edit.layout ||
        FAILED(edit.layout->HitTestPoint(pt.x - edit.origin.x,
                                         pt.y - edit.origin.y, &trailing,
                                         &inside, &hit))) {
        return false;
    }
    *position =
        std::min(text.size(), (size_t)hit.textPosition + (trailing ? 1 : 0));
    return true;
}

// A click in a reply (`pt` where it was drawn): the caret goes there.
void EditTextClick(const std::wstring& text, TextEdit& edit, POINT pt) {
    g_lastTyped = NowSeconds();
    const size_t from = edit.Selecting() ? edit.anchor : edit.caret;
    edit.anchor = std::wstring::npos;
    g_editDrag.text = &text;
    g_editDrag.edit = &edit;
    g_editDrag.click = pt;
    g_editDrag.window = nullptr;
    g_editDrag.moved = false;
    size_t position;
    if (text.empty() || !EditTextPosition(text, edit, pt, &position)) {
        return;
    }
    edit.caret = position;
    edit.follow = true;
    if (GetKeyState(VK_SHIFT) < 0) {
        if (from != position) {
            edit.anchor = std::min(from, text.size());
        }
        return;
    }
    // A second click soon after, at the same place: the word under it.
    const DWORD now = GetTickCount();
    if (g_editDrag.lastEdit == &edit &&
        now - g_editDrag.lastTime <= GetDoubleClickTime() &&
        std::abs(pt.x - g_editDrag.lastPoint.x) <=
            GetSystemMetrics(SM_CXDOUBLECLK) / 2 &&
        std::abs(pt.y - g_editDrag.lastPoint.y) <=
            GetSystemMetrics(SM_CYDOUBLECLK) / 2) {
        g_editDrag.lastTime = 0;
        auto inWord = [&](size_t i) {
            return iswalnum(text[i]) || text[i] == L'_';
        };
        size_t start = position;
        size_t end = position;
        while (start > 0 && inWord(start - 1)) {
            start--;
        }
        while (end < text.size() && inWord(end)) {
            end++;
        }
        if (start < end) {
            edit.anchor = start;
            edit.caret = end;
        }
        return;
    }
    g_editDrag.lastEdit = &edit;
    g_editDrag.lastTime = now;
    g_editDrag.lastPoint = pt;
}

// The mouse pressed in a panel (`pt` in its window): a click just given to
// one of its boxes holds the mouse, so a drag selects.
void StartEditDrag(HWND window, POINT pt) {
    if (!g_editDrag.edit || g_editDrag.window) {
        return;
    }
    g_editDrag.window = window;
    g_editDrag.shift = {g_editDrag.click.x - pt.x, g_editDrag.click.y - pt.y};
    SetCapture(window);
}

// The mouse moved with its button down: the selection goes from the click
// to it. Returns true when it changed.
bool EditTextDrag(HWND window, POINT pt) {
    EditDrag& drag = g_editDrag;
    if (drag.window != window || !drag.edit || drag.text->empty()) {
        return false;
    }
    const POINT at{pt.x + drag.shift.x, pt.y + drag.shift.y};
    if (!drag.moved) {
        if (std::abs(at.x - drag.click.x) < GetSystemMetrics(SM_CXDRAG) &&
            std::abs(at.y - drag.click.y) < GetSystemMetrics(SM_CYDRAG)) {
            return false;
        }
        drag.moved = true;
        drag.lastTime = 0;
        if (!drag.edit->Selecting()) {
            drag.edit->anchor = drag.edit->caret;
        }
    }
    size_t position;
    if (!EditTextPosition(*drag.text, *drag.edit, at, &position) ||
        position == drag.edit->caret) {
        return false;
    }
    drag.edit->caret = position;
    drag.edit->follow = true;
    g_lastTyped = NowSeconds();
    return true;
}

// The mouse let go (or taken by another window). Returns true when it was
// selecting: the release is only the drag's end, not a click.
bool EndEditDrag(HWND window) {
    if (g_editDrag.window != window) {
        return false;
    }
    const bool moved = g_editDrag.moved;
    g_editDrag.window = nullptr;
    g_editDrag.text = nullptr;
    g_editDrag.edit = nullptr;
    g_editDrag.moved = false;
    if (GetCapture() == window) {
        ReleaseCapture();
    }
    return moved;
}

// The wheel over a reply: it scrolls (the caret may go out of view).
void EditTextWheel(TextEdit& edit, int delta) {
    const float line = edit.lineHeight > 0 ? edit.lineHeight : 16;
    edit.scroll -= (float)delta / WHEEL_DELTA * line;
    edit.follow = false;
}

// What a box shows of a reply: wrapped, scrolled (to the caret, unless the
// wheel moved it), with the caret where it is.
struct WrappedField {
    winrt::com_ptr<IDWriteTextLayout> layout;
    float scroll = 0;
    float caretX = 0;
    float caretY = 0;
    float caretHeight = 0;
};

WrappedField LayOutEdit(IDWriteFactory* dwrite,
                        IDWriteTextFormat* format,
                        const std::wstring& text,
                        TextEdit& edit,
                        float width,
                        float visibleHeight,
                        float lineHeight) {
    WrappedField field;
    field.caretHeight = lineHeight;
    edit.caret = std::min(edit.caret, text.size());
    // A new line just started counts as a line (the caret goes there): a
    // space after it holds it open.
    const std::wstring shown =
        text + (!text.empty() && text.back() == L'\n' ? L" " : L"");
    field.layout =
        WrappedLayout(dwrite, format, shown, width, 100000, lineHeight);
    edit.layout = field.layout;
    edit.lineHeight = lineHeight;
    edit.visibleHeight = visibleHeight;
    if (!field.layout) {
        return field;
    }
    DWRITE_TEXT_METRICS metrics{};
    field.layout->GetMetrics(&metrics);
    DWRITE_HIT_TEST_METRICS hit{};
    float x = 0, y = 0;
    // Before the character at the caret; at the end, after the last one (or
    // at the start of the new line).
    const bool atEnd = edit.caret == text.size();
    const bool newLine = shown.size() > text.size();
    const UINT32 position =
        !atEnd || newLine ? (UINT32)edit.caret : (UINT32)text.size() - 1;
    if (!text.empty() &&
        SUCCEEDED(field.layout->HitTestTextPosition(
            position, atEnd && !newLine, &x, &y, &hit))) {
        field.caretX = x;
        field.caretY = y;
        field.caretHeight = hit.height > 0 ? hit.height : lineHeight;
    }
    if (edit.follow) {
        if (field.caretY < edit.scroll) {
            edit.scroll = field.caretY;
        } else if (field.caretY + field.caretHeight >
                   edit.scroll + visibleHeight) {
            edit.scroll = field.caretY + field.caretHeight - visibleHeight;
        }
    }
    edit.scroll = std::clamp(edit.scroll, 0.0f,
                             std::max(0.0f, metrics.height - visibleHeight));
    field.scroll = edit.scroll;
    return field;
}

// The selection of a box, over its text (laid out at `origin`).
void DrawTextSelection(ID2D1RenderTarget* target,
                       ID2D1SolidColorBrush* brush,
                       IDWriteTextLayout* layout,
                       D2D1_POINT_2F origin,
                       const TextEdit& edit,
                       float opacity = 1) {
    if (!layout || !edit.Selecting()) {
        return;
    }
    const UINT32 start = (UINT32)edit.SelectionStart();
    const UINT32 length = (UINT32)(edit.SelectionEnd() - edit.SelectionStart());
    UINT32 count = 0;
    layout->HitTestTextRange(start, length, origin.x, origin.y, nullptr, 0,
                             &count);
    if (!count) {
        return;
    }
    std::vector<DWRITE_HIT_TEST_METRICS> ranges(count);
    if (FAILED(layout->HitTestTextRange(start, length, origin.x, origin.y,
                                        ranges.data(), count, &count))) {
        return;
    }
    brush->SetColor(Accent(0.38f * opacity));
    for (UINT32 i = 0; i < count; i++) {
        const auto& range = ranges[i];
        // A line break selected: a little of it shows.
        const float width = std::max(range.width, 4.0f);
        target->FillRectangle(
            {range.left, range.top, range.left + width, range.top + range.height},
            brush);
    }
}

// Line breaks (pasted) become spaces, in a one-line field.
void KeepOneLine(std::wstring& text) {
    std::replace(text.begin(), text.end(), L'\n', L' ');
}

// A one-line field (the settings' search, a profile's name, a Wi-Fi
// password): `shown` (what's typed, or a dot for each letter) on one line,
// slid sideways to keep the caret in view, with its selection. The layout and
// where it was drawn stay in `edit` (for clicks); returns the caret's x.
float DrawLineEdit(ID2D1RenderTarget* target,
                   ID2D1SolidColorBrush* brush,
                   IDWriteFactory* dwrite,
                   IDWriteTextFormat* format,
                   const D2D1_RECT_F& rect,
                   const std::wstring& shown,
                   TextEdit& edit,
                   const D2D1_COLOR_F& color) {
    winrt::com_ptr<IDWriteTextLayout> layout;
    if (!format ||
        FAILED(dwrite->CreateTextLayout(shown.c_str(), (UINT32)shown.size(),
                                        format, 10000, rect.bottom - rect.top,
                                        layout.put()))) {
        edit.layout = nullptr;
        return rect.left;
    }
    layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    edit.caret = std::min(edit.caret, shown.size());
    float caretX = 0, caretY = 0;
    DWRITE_HIT_TEST_METRICS caret{};
    layout->HitTestTextPosition((UINT32)edit.caret, FALSE, &caretX, &caretY,
                                &caret);
    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    const float room = std::max(1.0f, rect.right - rect.left - 3);
    if (caretX - edit.scroll > room) {
        edit.scroll = caretX - room;
    }
    if (caretX < edit.scroll) {
        edit.scroll = caretX;
    }
    edit.scroll = std::clamp(
        edit.scroll, 0.0f,
        std::max(0.0f, metrics.widthIncludingTrailingWhitespace - room));
    const D2D1_POINT_2F origin{rect.left - edit.scroll, rect.top};
    target->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
    DrawTextSelection(target, brush, layout.get(), origin, edit);
    brush->SetColor(color);
    target->DrawTextLayout(origin, layout.get(), brush);
    target->PopAxisAlignedClip();
    edit.layout = layout;
    edit.origin = origin;
    return origin.x + caretX;
}

// A layered window drawn with Direct2D into a 32-bit bitmap.
// A real loss of the drawing device: the graphics driver was updated or
// reset, or the GPU went away. Only these make the island start over (see
// OnDrawingDeviceLost); any other drawing error only loses that frame.
bool IsDrawingDeviceLost(HRESULT hr) {
    return hr == D2DERR_RECREATE_TARGET || hr == DXGI_ERROR_DEVICE_REMOVED ||
           hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_HUNG ||
           hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR;
}

// Restarts the island, which makes everything drawn again from scratch (its
// drawing targets, and every picture kept for them: app icons, covers,
// cards...). Defined with the island.
void OnDrawingDeviceLost();

class LayeredCanvas {
   public:
    bool Create(ID2D1Factory* factory) {
        const D2D1_RENDER_TARGET_PROPERTIES properties =
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                  D2D1_ALPHA_MODE_PREMULTIPLIED),
                96, 96);
        if (FAILED(factory->CreateDCRenderTarget(&properties, target.put()))) {
            return false;
        }
        // ClearType needs an opaque background.
        target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        m_dc = CreateCompatibleDC(nullptr);
        return m_dc != nullptr;
    }

    void Destroy() {
        if (m_dc) {
            if (m_selected) {
                SelectObject(m_dc, m_oldBitmap);
                m_selected = nullptr;
            }
            DeleteDC(m_dc);
            m_dc = nullptr;
        }
        for (HBITMAP& bitmap : m_bitmaps) {
            if (bitmap) {
                DeleteObject(bitmap);
                bitmap = nullptr;
            }
        }
        m_rooms = {};
    }

    // Starts drawing on a canvas of `size`, cleared. `part` 1: a second
    // window drawn by the same target (its pictures are this target's),
    // with a bitmap of its own (see Island::RenderPlacedBanner).
    bool Begin(SIZE size, int part = 0) {
        size = {std::max(size.cx, 1L), std::max(size.cy, 1L)};
        HBITMAP& bitmap = m_bitmaps[part];
        SIZE& room = m_rooms[part];
        // The bitmap is kept while the canvas fits in it (a window growing
        // or shrinking frame by frame, like a panel opening, doesn't make a
        // new one each frame): made again with room to spare when it doesn't
        // fit, or smaller once it's much too big.
        const bool fits = bitmap && size.cx <= room.cx && size.cy <= room.cy;
        const LONGLONG needed = std::max<LONGLONG>(
            (LONGLONG)size.cx * size.cy, (LONGLONG)kRoomStep * kRoomStep);
        if (!fits || (LONGLONG)room.cx * room.cy > needed * 4) {
            if (bitmap) {
                if (m_selected == bitmap) {
                    SelectObject(m_dc, m_oldBitmap);
                    m_selected = nullptr;
                }
                DeleteObject(bitmap);
                bitmap = nullptr;
            }
            const SIZE made{(size.cx + kRoomStep - 1) / kRoomStep * kRoomStep,
                            (size.cy + kRoomStep - 1) / kRoomStep * kRoomStep};
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(info.bmiHeader);
            info.bmiHeader.biWidth = made.cx;
            info.bmiHeader.biHeight = -made.cy;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            void* bits = nullptr;
            bitmap = CreateDIBSection(m_dc, &info, DIB_RGB_COLORS, &bits,
                                      nullptr, 0);
            if (!bitmap) {
                room = {};
                return false;
            }
            room = made;
        }
        if (m_selected != bitmap) {
            const HGDIOBJ before = SelectObject(m_dc, bitmap);
            if (!m_selected) {
                m_oldBitmap = before;
            }
            m_selected = bitmap;
        }
        // (Only this part of it is drawn and put on the window.)
        m_size = size;
        const RECT bounds{0, 0, size.cx, size.cy};
        if (const HRESULT hr = target->BindDC(m_dc, &bounds); FAILED(hr)) {
            Failed(hr);
            return false;
        }
        target->BeginDraw();
        target->SetTransform(D2D1::Matrix3x2F::Identity());
        target->Clear(D2D1::ColorF(0, 0, 0, 0));
        return true;
    }

    // Finishes drawing and puts it on the window, with an overall opacity.
    // False if it couldn't be drawn (nothing changed on the screen).
    bool End(HWND hWnd, POINT position, float opacity) {
        if (!Finish()) {
            return false;
        }
        POINT source{0, 0};
        BLENDFUNCTION blend{AC_SRC_OVER, 0,
                            (BYTE)std::clamp((int)(opacity * 255), 0, 255),
                            AC_SRC_ALPHA};
        UpdateLayeredWindow(hWnd, nullptr, &position, &m_size, m_dc, &source, 0,
                            &blend, ULW_ALPHA);
        return true;
    }

    // Finishes drawing without putting it on a window (see Panel::WarmUp).
    bool Finish() {
        if (const HRESULT hr = target->EndDraw(); FAILED(hr)) {
            Failed(hr);
            return false;
        }
        m_lastError = S_OK;
        return true;
    }

    winrt::com_ptr<ID2D1DCRenderTarget> target;
    // The island's (it starts over when its device is lost), or false for
    // one drawn on another thread.
    bool tellsLostDevice = true;

   private:
    // Logged once (not every frame); a lost device restarts the island.
    void Failed(HRESULT hr) {
        if (hr != m_lastError) {
            m_lastError = hr;
            IslandLog(L"Drawing failed: %08X", (unsigned)hr);
        }
        if (PlainScalingAfter(hr)) {
            IslandLog(L"Pictures made smaller the plain way from now on");
        }
        if (tellsLostDevice && IsDrawingDeviceLost(hr)) {
            OnDrawingDeviceLost();
        }
    }
    HRESULT m_lastError = S_OK;
    HDC m_dc = nullptr;
    // Each part's bitmap (the one in the DC, and what was there first).
    std::array<HBITMAP, 2> m_bitmaps{};
    HBITMAP m_selected = nullptr;
    HGDIOBJ m_oldBitmap = nullptr;
    // What's drawn, and each bitmap's own size (a step bigger).
    SIZE m_size{};
    std::array<SIZE, 2> m_rooms{};
    static constexpr LONG kRoomStep = 64;
};

// What the control center and the island show about the network.
struct NetworkInfo {
    bool connected = false;
    bool internet = false;
    // On Wi-Fi: its signal (0-100) and name, which Windows gives only to
    // apps allowed to use the location (on newer Windows 11 builds):
    // -1 and empty otherwise.
    bool wifi = false;
    int signal = -1;
    std::wstring wifiName;
    // A Wi-Fi adapter joining a network (looking, associating, signing in).
    bool wifiConnecting = false;
    // A physical adapter (a cable, Wi-Fi, a phone's) is connected: virtual
    // ones (VPNs, Hyper-V's switch) stay connected without any, so the
    // network list manager alone can't tell a cable pulled out.
    bool physical = true;
    // Which kind of physical adapter is connected (the interface table, which
    // needs no location access): the cable's glyph only with a cable.
    bool wifiAdapterUp = false;
    bool cableUp = false;
};

class Island;
// The islands: one, or one on each monitor (islandAllMonitors), all on the
// island's thread. The first is the main one: explorer talks to it, it plays
// the sounds and runs what they share (the weather, the updates, the media,
// the usage), and tells the others. g_island is the one the code runs for
// now: each island's windows and panels make theirs the current one (see
// IslandScope), with its own place.
Island* g_island;
std::vector<Island*> g_islands;
// Makes `island` the current one (nothing for none); returns the one before.
Island* SwitchIsland(Island* island);

class IslandScope {
   public:
    explicit IslandScope(Island* island) : m_previous(SwitchIsland(island)) {}
    ~IslandScope() { SwitchIsland(m_previous); }
    IslandScope(const IslandScope&) = delete;
    IslandScope& operator=(const IslandScope&) = delete;

   private:
    Island* m_previous;
};

// A panel's timer for the screenshot (see ControlPanel).
constexpr UINT_PTR kPanelScreenshotTimerId = 7;
// A window of its own being moved or sized, or its system menu open (a
// modal loop of Windows' own): the islands' frames run on this timer then
// (see RunIslandFrames).
constexpr UINT_PTR kPanelModalFramesTimerId = 0x4D46;
constexpr UINT kPanelModalFramesMs = 15;

// Runs the island's animation frames (see IslandThreadProc).
void StartIslandAnimation();
// A frame of each island animating (the islands' loop's, see
// IslandThreadProc; also from a modal loop, see kPanelModalFramesTimerId).
void RunIslandFrames();
void StartPanelAnimation(Island* owner);
// How many times frames were asked for (see StartIslandAnimation): an open
// panel draws itself again on the next frame after one (see Panel::Animate).
unsigned long long g_frameRequests = 0;

////////////////////////////////////////////////////////////////////////////////
// Liquid Glass (islandLiquidGlass): the island, its notches and its panels
// drawn as glass, like on macOS: Windows' own blur of what's behind them (the
// one of its acrylic), cut to their shapes, under a light tint, a soft light
// and a bright edge. Each glass is a window of its own right under the
// island's (or a panel's) window, drawn by Windows from composition visuals:
// screenshots and recordings see it.

