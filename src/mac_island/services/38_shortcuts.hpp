constexpr WCHAR kGlyphKeyboardShortcuts = 0xE765;
constexpr WCHAR kGlyphCopy = 0xE8C8;
constexpr WCHAR kGlyphPencil = 0xE70F;
constexpr WCHAR kGlyphAdd = 0xE710;

// A panel with text boxes: their look, their caret (blinking a few seconds
// after typing, then steady, so nothing is drawn while nothing happens), and
// scrolling lists.
class EditingPanel : public Panel {
   protected:
    void PrepareFormats(float scale) {
        if (scale == m_preparedScale) {
            return;
        }
        m_preparedScale = scale;
        m_titleFormat = MakeFormat(L"Segoe UI Variable Text",
                                   DWRITE_FONT_WEIGHT_SEMI_BOLD, 14 * scale,
                                   DWRITE_TEXT_ALIGNMENT_LEADING);
        m_textFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       13 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_boldFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_SEMI_BOLD,
                       13 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_smallFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       11 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_buttonFormat = MakeFormat(L"Segoe UI Variable Text",
                                    DWRITE_FONT_WEIGHT_SEMI_BOLD, 12 * scale);
        m_iconFormat = MakeFormat(IconFont(),
                                  DWRITE_FONT_WEIGHT_NORMAL, 13 * scale);
        m_bigIconFormat = MakeFormat(IconFont(),
                                     DWRITE_FONT_WEIGHT_NORMAL, 15 * scale);
    }
    bool Ready() const {
        return m_titleFormat && m_textFormat && m_boldFormat && m_smallFormat &&
               m_buttonFormat && m_iconFormat && m_bigIconFormat;
    }

    // The caret blinks a moment after typing (`moving`: something else
    // moves too, drawn at full rate then).
    bool AnimateTyping(bool typing, bool moving) {
        const bool blinking = typing && CaretBlinking();
        m_idleMotion = blinking && !moving;
        return blinking || moving;
    }

    // A box: its frame (blue while typing in it), what's typed (wrapped,
    // following the caret) or its hint, and the caret.
    void DrawTextBox(ID2D1RenderTarget* target,
                     ID2D1SolidColorBrush* brush,
                     const D2D1_RECT_F& box,
                     const std::wstring& text,
                     TextEdit& edit,
                     PCWSTR hint,
                     bool focused) {
        const float s = m_scale;
        const float radius = std::round(9 * s);
        brush->SetColor(Fg(0.07f));
        GlassFill(target, {box, radius, radius}, brush);
        brush->SetColor(focused ? Accent(0.9f)
                                : Fg(0.12f));
        target->DrawRoundedRectangle(
            {{box.left + 0.5f, box.top + 0.5f, box.right - 0.5f,
              box.bottom - 0.5f},
             radius,
             radius},
            brush, focused ? 1.5f : 1.0f);
        const float line = std::round(18 * s);
        const float inner = std::round(10 * s);
        const float padTop = std::min(std::round(8 * s),
                                      std::round((box.bottom - box.top - line) /
                                                 2));
        const D2D1_RECT_F textRect{box.left + inner, box.top + padTop,
                                   box.right - inner, box.bottom - padTop};
        float caretX = textRect.left;
        float caretTop = textRect.top;
        float caretHeight = line;
        if (text.empty()) {
            brush->SetColor(Fg(0.4f));
            target->DrawText(hint, (UINT32)wcslen(hint), m_textFormat.get(),
                             {textRect.left, textRect.top, textRect.right,
                              textRect.top + line},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            edit.layout = nullptr;
            edit.origin = {textRect.left, textRect.top};
        } else {
            const WrappedField wrapped =
                LayOutEdit(m_dwrite, m_textFormat.get(), text, edit,
                           textRect.right - textRect.left,
                           textRect.bottom - textRect.top, line);
            if (wrapped.layout) {
                target->PushAxisAlignedClip(textRect,
                                            D2D1_ANTIALIAS_MODE_ALIASED);
                DrawTextSelection(target, brush, wrapped.layout.get(),
                                  {textRect.left, textRect.top - wrapped.scroll},
                                  edit);
                brush->SetColor(Fg(0.95f));
                target->DrawTextLayout(
                    {textRect.left, textRect.top - wrapped.scroll},
                    wrapped.layout.get(), brush,
                    D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
                target->PopAxisAlignedClip();
                caretX = textRect.left + wrapped.caretX;
                caretTop = textRect.top - wrapped.scroll + wrapped.caretY;
                caretHeight = wrapped.caretHeight;
                edit.origin = {textRect.left, textRect.top - wrapped.scroll};
            }
        }
        if (focused && caretTop >= textRect.top - 1 &&
            caretTop + caretHeight <= textRect.bottom + 1) {
            brush->SetColor(NowSeconds() - g_lastTyped < 5
                                ? CaretColor()
                                : Accent(1));
            target->FillRectangle({caretX + 1, caretTop + 1, caretX + 2.5f,
                                   caretTop + caretHeight - 1},
                                  brush);
        }
    }

    // A rounded button: blue when it's the main one, lighter under the
    // mouse.
    void DrawButton(ID2D1RenderTarget* target,
                    ID2D1SolidColorBrush* brush,
                    const D2D1_RECT_F& rect,
                    PCWSTR text,
                    bool main,
                    bool hover,
                    float opacity = 1) {
        const float radius = std::round(9 * m_scale);
        const float lit = Hover(rect, hover);
        if (main) {
            brush->SetColor(
                Accent((1.0f - 0.15f * lit) * opacity));
        } else {
            brush->SetColor(Fg((0.1f + 0.07f * lit) * opacity));
        }
        GlassFill(target, {rect, radius, radius}, brush);
        brush->SetColor(main ? AccentInk(opacity)
                             : Fg(0.9f * opacity));
        target->DrawText(text, (UINT32)wcslen(text), m_buttonFormat.get(), rect,
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    // A small icon button (copy, edit, delete), lit under the mouse.
    void DrawIconButton(ID2D1RenderTarget* target,
                        ID2D1SolidColorBrush* brush,
                        const D2D1_RECT_F& rect,
                        WCHAR glyph,
                        bool hover,
                        float rest = 0.45f) {
        const float lit = Hover(rect, hover);
        if (lit > 0.01f) {
            const float radius = (rect.bottom - rect.top) / 2;
            brush->SetColor(Fg(0.1f * lit));
            GlassFill(target, {rect, radius, radius}, brush);
        }
        brush->SetColor(Fg(rest + (0.95f - rest) * lit));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(), rect, brush);
    }

    // Rows growing in (added) and shrinking away (removed), fading.
    struct RowMotion {
        long long id;
        double room;
        double velocity;
        bool leaving;
    };
    float RowRoom(long long id) const {
        for (const auto& motion : m_rowMotions) {
            if (motion.id == id) {
                return (float)std::clamp(motion.room, 0.0, 1.0);
            }
        }
        return 1;
    }
    bool RowLeaving(long long id) const {
        return std::any_of(
            m_rowMotions.begin(), m_rowMotions.end(),
            [id](const RowMotion& motion) { return motion.id == id && motion.leaving; });
    }
    void RowArrives(long long id) {
        m_rowMotions.push_back({id, 0, 0, false});
        StartIslandAnimation();
    }
    void RowLeaves(long long id) {
        for (auto& motion : m_rowMotions) {
            if (motion.id == id) {
                motion.leaving = true;
                StartIslandAnimation();
                return;
            }
        }
        m_rowMotions.push_back({id, 1, 0, true});
        StartIslandAnimation();
    }
    // Moves them; the ones gone out are given back (to remove for good).
    bool AnimateRows(double dt, std::vector<long long>* gone) {
        bool moving = false;
        for (size_t i = 0; i < m_rowMotions.size();) {
            RowMotion& motion = m_rowMotions[i];
            const bool moves = SpringTowards(
                motion.room, motion.velocity, motion.leaving ? 0 : 1,
                motion.leaving ? 0.3 : 0.42, dt,
                motion.leaving ? 1.0 : kBounceDamping);
            if (moves) {
                moving = true;
                i++;
                continue;
            }
            if (motion.leaving) {
                gone->push_back(motion.id);
            }
            m_rowMotions.erase(m_rowMotions.begin() + i);
        }
        return moving;
    }
    std::vector<RowMotion> m_rowMotions;

    // The most a list panel takes: the room there, up to about two thirds
    // of the screen (longer lists scroll).
    float ListMaxHeight() const {
        const float screen = (float)(m_monitorRect.bottom - m_monitorRect.top);
        return std::min(MaxHeight(),
                        std::max(std::round(320 * m_scale), screen * 0.62f));
    }
    // A point on the panel, in the list (when it's over it).
    bool ListPoint(POINT pt, POINT* inList) const {
        if (!PointInRect(pt, m_listViewport)) {
            return false;
        }
        *inList = {pt.x, (LONG)(pt.y - m_listViewport.top + m_scroll.Offset())};
        return true;
    }

    float m_preparedScale = 0;
    winrt::com_ptr<IDWriteTextFormat> m_titleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_boldFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallFormat;
    winrt::com_ptr<IDWriteTextFormat> m_buttonFormat;
    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_bigIconFormat;
    D2D1_RECT_F m_listViewport{};
    SmoothScroll m_scroll;
};

////////////////////////////////////////////////////////////////////////////////
// Notes: a box to write one (Enter makes a new line, Ctrl+Enter adds it),
// and the notes under it, newest first: the pencil (or a click) edits one,
// the copy button copies it, the "x" deletes it.

struct Note {
    long long id = 0;
    long long stamp = 0;
    std::wstring text;
};
constexpr size_t kMaxNotes = 100;
constexpr size_t kMaxNoteLength = 2000;
// A topic's mark, at the start of a line.
constexpr WCHAR kTopicMark[] = L"\u2022 ";
constexpr float kNotesWidth = 440;
// The lines of a note shown in the list.
constexpr int kNoteLinesShown = 3;

