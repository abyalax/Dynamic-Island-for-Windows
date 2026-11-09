#pragma once
class NotesPanel : public EditingPanel {
   protected:
    void Prepare(float scale) override {
        PrepareFormats(scale);
        Load();
        Layout();
    }

    void OnOpen() override {
        m_hover = {};
        m_scroll.Reset();
        m_edit.caret = m_text.size();
        g_lastTyped = NowSeconds();
        Layout();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

    bool OnMouseMove(POINT pt) override {
        const Hit hit = HitTest(pt);
        if (hit == m_hover) {
            return false;
        }
        m_hover = hit;
        return true;
    }
    bool OnMouseLeave() override {
        m_hover = {};
        return true;
    }
    bool OnMouseDown(POINT pt) override {
        if (PointInRect(pt, m_box)) {
            EditTextClick(m_text, m_edit, pt);
            return true;
        }
        return false;
    }
    bool OnMouseUp(POINT pt) override {
        const Hit hit = HitTest(pt);
        switch (hit.part) {
            case Part::Add:
                Add();
                return true;
            case Part::Cancel:
                StopEditing();
                return true;
            case Part::Topic:
                ToggleTopic();
                return true;
            case Part::Copy:
                if (const Note* note = Find(hit.id)) {
                    CopyTextToClipboard(m_hwnd, note->text);
                    m_copiedId = hit.id;
                    m_copiedAt = NowSeconds();
                    StartIslandAnimation();
                }
                return true;
            case Part::Delete:
                // It shrinks away, then goes.
                if (m_editingId == hit.id) {
                    StopEditing();
                }
                RowLeaves(hit.id);
                return true;
            case Part::Row:
            case Part::Edit:
                if (const Note* note = Find(hit.id);
                    note && !RowLeaving(hit.id)) {
                    m_editingId = note->id;
                    m_text = note->text;
                    m_edit.Reset();
                    m_edit.caret = m_text.size();
                    g_lastTyped = NowSeconds();
                    Layout();
                }
                return true;
            default:
                return false;
        }
    }
    bool OnWheel(int delta) override {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(m_hwnd, &pt);
        pt.x -= m_contentOffset.x;
        pt.y -= m_contentOffset.y;
        if (PointInRect(pt, m_box)) {
            EditTextWheel(m_edit, delta);
            return true;
        }
        return m_scroll.Wheel(delta, std::round(60 * m_scale));
    }
    bool OnKey(WPARAM key) override {
        if (key == VK_ESCAPE && m_editingId) {
            StopEditing();
            return true;
        }
        return EditTextKey(m_text, m_edit, key);
    }
    bool OnChar(WCHAR c) override {
        // Enter: a new line (going on with a topic, or ending the topics on
        // an empty one); Ctrl+Enter: added (or saved).
        if (c == 0x0A) {
            Add();
            return true;
        }
        if (c == 0x1B) {
            return false;
        }
        if (c == L'\r' && !m_edit.Selecting()) {
            const size_t start = LineStart();
            const std::wstring mark = kTopicMark;
            if (m_text.compare(start, mark.size(), mark) == 0) {
                if (m_edit.caret == start + mark.size() &&
                    (start + mark.size() == m_text.size() ||
                     m_text[start + mark.size()] == L'\n')) {
                    m_text.erase(start, mark.size());
                    m_edit.caret = start;
                } else {
                    EditTextChar(m_text, m_edit, L'\n', kMaxNoteLength,
                                 m_hwnd);
                    for (WCHAR part : mark) {
                        EditTextChar(m_text, m_edit, part, kMaxNoteLength,
                                     m_hwnd);
                    }
                }
                Layout();
                return true;
            }
        }
        const bool taken = EditTextChar(m_text, m_edit, c == L'\r' ? L'\n' : c,
                                        kMaxNoteLength, m_hwnd);
        if (taken) {
            Layout();
        }
        return taken;
    }
    bool AnimateContents(double dt) override {
        const bool scrolling = m_scroll.Animate(dt);
        const bool copied = m_copiedId && NowSeconds() - m_copiedAt < 1.4;
        if (!copied) {
            m_copiedId = 0;
        }
        std::vector<long long> gone;
        const bool rows = AnimateRows(dt, &gone);
        if (!gone.empty()) {
            std::erase_if(m_notes, [&](const Note& note) {
                return std::find(gone.begin(), gone.end(), note.id) !=
                       gone.end();
            });
            Save();
        }
        if (rows || !gone.empty()) {
            Layout();
        }
        return AnimateTyping(true, scrolling || copied || rows);
    }

   private:
    enum class Part { None, Add, Cancel, Topic, Row, Edit, Copy, Delete };

    // Where the caret's line starts.
    size_t LineStart() const {
        const size_t caret = std::min(m_edit.caret, m_text.size());
        const size_t newLine = caret ? m_text.rfind(L'\n', caret - 1)
                                     : std::wstring::npos;
        return newLine == std::wstring::npos ? 0 : newLine + 1;
    }
    // The caret's line becomes a topic (or stops being one).
    void ToggleTopic() {
        m_edit.anchor = std::wstring::npos;
        const size_t start = LineStart();
        const std::wstring mark = kTopicMark;
        if (m_text.compare(start, mark.size(), mark) == 0) {
            m_text.erase(start, mark.size());
            m_edit.caret = m_edit.caret >= start + mark.size()
                               ? m_edit.caret - mark.size()
                               : start;
        } else if (m_text.size() + mark.size() <= kMaxNoteLength) {
            m_text.insert(start, mark);
            m_edit.caret += mark.size();
        }
        m_edit.follow = true;
        g_lastTyped = NowSeconds();
        Layout();
    }
    struct Hit {
        Part part = Part::None;
        long long id = 0;
        bool operator==(const Hit& other) const {
            return part == other.part && id == other.id;
        }
    };
    struct Row {
        long long id;
        D2D1_RECT_F rect;
        D2D1_RECT_F edit;
        D2D1_RECT_F copy;
        D2D1_RECT_F remove;
    };

    // Read again as it opens (another island may have changed them).
    void Load() {
        m_notes.clear();
        // (Up to 100 notes of 2000 characters, line breaks written as two.)
        for (const auto& record : LoadRecords(L"notes", 3, 512 * 1024)) {
            m_notes.push_back({_wtoi64(record[0].c_str()),
                               _wtoi64(record[1].c_str()), record[2]});
        }
    }
    void Save() {
        std::vector<std::vector<std::wstring>> records;
        for (const auto& note : m_notes) {
            records.push_back({std::to_wstring(note.id),
                               std::to_wstring(note.stamp), note.text});
        }
        SaveRecords(L"notes", records);
    }
    const Note* Find(long long id) const {
        for (const auto& note : m_notes) {
            if (note.id == id) {
                return &note;
            }
        }
        return nullptr;
    }
    void Add() {
        // Nothing but spaces: nothing to keep.
        if (m_text.find_first_not_of(L" \n\t") == std::wstring::npos) {
            return;
        }
        if (m_editingId) {
            for (auto& note : m_notes) {
                if (note.id == m_editingId) {
                    note.text = m_text;
                    note.stamp = NowStamp();
                }
            }
            std::erase_if(m_lineCounts, [&](const auto& entry) {
                return entry.first == m_editingId;
            });
        } else {
            if (m_notes.size() >= kMaxNotes) {
                m_notes.pop_back();
            }
            const long long now = NowStamp();
            m_notes.insert(m_notes.begin(), {now, now, m_text});
            RowArrives(now);
        }
        Save();
        m_editingId = 0;
        m_text.clear();
        m_edit.Reset();
        m_scroll.Reset();
        Layout();
    }
    void StopEditing() {
        m_editingId = 0;
        m_text.clear();
        m_edit.Reset();
        Layout();
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(kNotesWidth * s);
        const float padding = std::round(12 * s);
        const float gap = std::round(8 * s);
        float y = padding;
        m_header = {padding + std::round(4 * s), y, width - padding,
                    y + std::round(26 * s)};
        const float header = std::round(28 * s);
        // "Topics", as wide as its icon and name.
        float topicWidth = std::round(92 * s);
        {
            PCWSTR topics = Tr(L"Topics", L"Tópicos");
            winrt::com_ptr<IDWriteTextLayout> measure;
            if (m_smallFormat &&
                SUCCEEDED(m_dwrite->CreateTextLayout(
                    topics, (UINT32)wcslen(topics), m_smallFormat.get(), 1000,
                    header, measure.put()))) {
                DWRITE_TEXT_METRICS metrics{};
                measure->GetMetrics(&metrics);
                topicWidth = std::round(28 * s) + std::ceil(metrics.width) +
                             std::round(12 * s);
            }
        }
        m_topic = {width - padding - topicWidth, y, width - padding,
                   y + header};
        y += std::round(32 * s);
        const float line = std::round(18 * s);
        const float written =
            line * std::max(1, WrappedLineCount(
                                   m_dwrite, m_textFormat.get(),
                                   m_text.empty() ? std::wstring(L" ") : m_text,
                                   width - padding * 2 - std::round(20 * s),
                                   line)) +
            std::round(16 * s) + line;
        const float most = std::max(
            std::round(112 * s),
            std::min(ListMaxHeight() * 0.45f,
                     ListMaxHeight() - y - std::round(170 * s)));
        const float box = std::clamp(written, std::round(112 * s), most);
        m_box = {padding, y, width - padding, y + box};
        y += box;
        // How long it is, of the most it can be.
        m_counter = {padding, y + std::round(2 * s),
                     width - padding - std::round(4 * s),
                     y + std::round(18 * s)};
        y += std::round(18 * s) + std::round(4 * s);
        const float button = std::round(32 * s);
        if (m_editingId) {
            const float half = std::floor((width - padding * 2 - gap) / 2);
            m_cancel = {padding, y, padding + half, y + button};
            m_add = {width - padding - half, y, width - padding, y + button};
        } else {
            m_cancel = {};
            m_add = {padding, y, width - padding, y + button};
        }
        y += button + gap;
        // The notes, scrolling when there are many: each as tall as its
        // first lines (as they're shown), growing in and shrinking away.
        m_rows.clear();
        const float icon = std::round(28 * s);
        const float textWidth =
            width - padding * 2 - icon * 2 - std::round(18 * s);
        float listY = 0;
        for (const auto& note : m_notes) {
            int lines = 1;
            for (const auto& [id, count] : m_lineCounts) {
                if (id == note.id) {
                    lines = count;
                }
            }
            if (std::none_of(m_lineCounts.begin(), m_lineCounts.end(),
                             [&](const auto& entry) {
                                 return entry.first == note.id;
                             })) {
                lines = std::min(
                    kNoteLinesShown,
                    WrappedLineCount(m_dwrite, m_textFormat.get(), note.text,
                                     textWidth, line));
                m_lineCounts.push_back({note.id, lines});
            }
            const float room = RowRoom(note.id);
            const float row =
                std::max(std::round(30 * s) + line * std::max(lines, 1),
                         icon * 2 + std::round(10 * s)) *
                room;
            const D2D1_RECT_F rect{padding, listY, width - padding,
                                   listY + row};
            const float middle = rect.top + std::round(19 * s);
            m_rows.push_back(
                {note.id, rect,
                 {rect.left + std::round(4 * s), middle - icon / 2,
                  rect.left + std::round(4 * s) + icon, middle + icon / 2},
                 {rect.right - icon - std::round(4 * s),
                  rect.top + std::round(3 * s), rect.right - std::round(4 * s),
                  rect.top + std::round(3 * s) + icon},
                 {rect.right - icon - std::round(4 * s),
                  rect.top + std::round(3 * s) + icon,
                  rect.right - std::round(4 * s),
                  rect.top + std::round(3 * s) + icon * 2}});
            listY += row + std::round(6 * s) * room;
        }
        const float content = m_rows.empty() ? std::round(40 * s)
                                              : listY - std::round(6 * s);
        const float room = std::max(std::round(80 * s),
                                    ListMaxHeight() - y - padding);
        const float viewport = std::min(content, room);
        m_listViewport = {0, y, width, y + viewport};
        m_scroll.SetSizes(viewport, content);
        y += viewport + padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    // A note's first lines laid out, and when it was written, made once a
    // version (scrolling only moves them).
    struct NoteView {
        long long id;
        long long stamp;
        float width;
        float scale;
        std::wstring when;
        winrt::com_ptr<IDWriteTextLayout> layout;
    };
    const NoteView& ViewOf(const Note& note, float width) {
        for (auto& view : m_noteViews) {
            if (view.id == note.id) {
                if (view.stamp == note.stamp && view.width == width &&
                    view.scale == m_scale) {
                    return view;
                }
                view = MakeView(note, width);
                return view;
            }
        }
        if (m_noteViews.size() > kMaxNotes * 2) {
            m_noteViews.clear();
        }
        return m_noteViews.emplace_back(MakeView(note, width));
    }
    NoteView MakeView(const Note& note, float width) {
        NoteView view{note.id, note.stamp, width, m_scale,
                      StampText(note.stamp), nullptr};
        const float line = std::round(18 * m_scale);
        view.layout = WrappedLayout(m_dwrite, m_textFormat.get(), note.text,
                                    width, line * kNoteLinesShown, line);
        if (view.layout) {
            const DWRITE_TRIMMING trimming{
                DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
            winrt::com_ptr<IDWriteInlineObject> ellipsis;
            m_dwrite->CreateEllipsisTrimmingSign(m_textFormat.get(),
                                                 ellipsis.put());
            view.layout->SetTrimming(&trimming, ellipsis.get());
        }
        return view;
    }
    std::vector<NoteView> m_noteViews;

    Hit HitTest(POINT pt) const {
        if (PointInRect(pt, m_add)) {
            return {Part::Add};
        }
        if (PointInRect(pt, m_cancel)) {
            return {Part::Cancel};
        }
        if (PointInRect(pt, m_topic)) {
            return {Part::Topic};
        }
        POINT inList;
        if (!ListPoint(pt, &inList)) {
            return {};
        }
        for (const auto& row : m_rows) {
            if (!PointInRect(inList, row.rect)) {
                continue;
            }
            if (PointInRect(inList, row.copy)) {
                return {Part::Copy, row.id};
            }
            if (PointInRect(inList, row.remove)) {
                return {Part::Delete, row.id};
            }
            if (PointInRect(inList, row.edit)) {
                return {Part::Edit, row.id};
            }
            return {Part::Row, row.id};
        }
        return {};
    }

    std::vector<Note> m_notes;
    // How many lines each note shows (measured once a version).
    std::vector<std::pair<long long, int>> m_lineCounts;
    std::wstring m_text;
    TextEdit m_edit;
    long long m_editingId = 0;
    long long m_copiedId = 0;
    double m_copiedAt = 0;
    Hit m_hover;
    D2D1_RECT_F m_header{}, m_box{}, m_add{}, m_cancel{}, m_topic{},
        m_counter{};
    std::vector<Row> m_rows;
};

void NotesPanel::Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
    if (!Ready()) {
        return;
    }
    const float s = m_scale;
    // The note icon, the title and how many.
    brush->SetColor(D2D1::ColorF(1.0f, 0.8f, 0.25f, 1));
    DrawGlyph(target, &kGlyphNote, 1, m_bigIconFormat.get(),
              {m_header.left - std::round(4 * s), m_header.top,
               m_header.left + std::round(20 * s), m_header.bottom},
              brush);
    std::wstring title = Tr(L"Notes", L"Anotações");
    if (!m_notes.empty()) {
        title += L"  ·  " + std::to_wstring(m_notes.size());
    }
    brush->SetColor(Fg(0.95f));
    target->DrawText(title.c_str(), (UINT32)title.size(), m_titleFormat.get(),
                     {m_header.left + std::round(24 * s), m_header.top,
                      m_header.right, m_header.bottom},
                     brush);
    // Topics: the caret's line gets a "•" (Enter goes on with them).
    {
        const float lit = Hover(m_topic, m_hover.part == Part::Topic);
        const float radius = (m_topic.bottom - m_topic.top) / 2;
        brush->SetColor(Fg(0.08f + 0.08f * lit));
        GlassFill(target, {m_topic, radius, radius}, brush);
        const WCHAR list = 0xE8FD;
        brush->SetColor(Fg(0.9f));
        DrawGlyph(target, &list, 1, m_iconFormat.get(),
                  {m_topic.left + std::round(4 * s), m_topic.top,
                   m_topic.left + std::round(28 * s), m_topic.bottom},
                  brush);
        PCWSTR topics = Tr(L"Topics", L"Tópicos");
        target->DrawText(topics, (UINT32)wcslen(topics), m_smallFormat.get(),
                         {m_topic.left + std::round(28 * s), m_topic.top,
                          m_topic.right - std::round(6 * s), m_topic.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    DrawTextBox(target, brush, m_box, m_text, m_edit,
                Tr(L"Type your note here…", L"Escreva sua anotação aqui…"),
                true);
    // How long it is, of the most it can be (red at the limit).
    {
        const std::wstring length = std::to_wstring(m_text.size()) + L" / " +
                                    std::to_wstring(kMaxNoteLength);
        brush->SetColor(m_text.size() >= kMaxNoteLength
                            ? D2D1::ColorF(1.0f, 0.4f, 0.35f, 1)
                        : m_text.size() > kMaxNoteLength * 9 / 10
                            ? D2D1::ColorF(1.0f, 0.75f, 0.3f, 1)
                            : Fg(0.4f));
        m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        target->DrawText(length.c_str(), (UINT32)length.size(),
                         m_smallFormat.get(), m_counter, brush);
        m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    }
    const bool empty =
        m_text.find_first_not_of(L" \n\t") == std::wstring::npos;
    if (m_editingId) {
        DrawButton(target, brush, m_cancel, Tr(L"Cancel", L"Cancelar"), false,
                   m_hover.part == Part::Cancel);
        DrawButton(target, brush, m_add, Tr(L"Save", L"Salvar"), true,
                   m_hover.part == Part::Add, empty ? 0.5f : 1.0f);
    } else {
        DrawButton(target, brush, m_add,
                   Tr(L"Add note  (Ctrl+Enter)",
                      L"Adicionar anotação  (Ctrl+Enter)"),
                   !empty, m_hover.part == Part::Add);
    }

    if (m_notes.empty()) {
        PCWSTR none = Tr(L"No notes yet", L"Nenhuma anotação ainda");
        brush->SetColor(Fg(0.4f));
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        target->DrawText(none, (UINT32)wcslen(none), m_textFormat.get(),
                         m_listViewport, brush);
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        return;
    }
    BeginScrollArea(target, m_listViewport, m_scroll);
    const float top = m_scroll.Offset();
    const float bottom = top + m_listViewport.bottom - m_listViewport.top;
    for (size_t i = 0; i < m_rows.size() && i < m_notes.size(); i++) {
        const Row& row = m_rows[i];
        const Note& note = m_notes[i];
        if (row.rect.bottom < top || row.rect.top > bottom) {
            continue;
        }
        const bool editing = note.id == m_editingId;
        const bool hover = m_hover.id == note.id;
        const float room = RowRoom(note.id);
        if (room < 0.02f) {
            continue;
        }
        // On a layer only while growing in or shrinking away (a layer per
        // row makes scrolling heavy).
        winrt::com_ptr<ID2D1Layer> layer;
        const bool layered = room < 0.999f;
        if (layered) {
            target->CreateLayer(nullptr, layer.put());
            target->PushLayer(
                D2D1::LayerParameters(row.rect, nullptr,
                                      D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                      D2D1::IdentityMatrix(), room),
                layer.get());
        }
        const float radius = std::round(10 * s);
        brush->SetColor(Fg((editing ? 0.12f : 0.06f) +
                           0.05f * Hover(HoverKey(7000, note.id), hover)));
        GlassFill(target, {row.rect, radius, radius}, brush);
        if (editing) {
            brush->SetColor(Accent(0.8f));
            target->DrawRoundedRectangle(
                {{row.rect.left + 0.75f, row.rect.top + 0.75f,
                  row.rect.right - 0.75f, row.rect.bottom - 0.75f},
                 radius,
                 radius},
                brush, 1.5f);
        }
        DrawIconButton(target, brush, row.edit, kGlyphPencil,
                       hover && m_hover.part == Part::Edit, 0.7f);
        // When it was written, then its first lines as written (each line
        // break kept), ending with "…" when there's more.
        const float textLeft = row.edit.right + std::round(6 * s);
        const float textRight = row.copy.left - std::round(4 * s);
        const NoteView& view = ViewOf(note, textRight - textLeft);
        const std::wstring& stamp = view.when;
        brush->SetColor(Fg(0.45f));
        target->DrawText(stamp.c_str(), (UINT32)stamp.size(),
                         m_smallFormat.get(),
                         {textLeft, row.rect.top + std::round(8 * s),
                          textRight, row.rect.top + std::round(24 * s)},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        const D2D1_RECT_F textRect{textLeft, row.rect.top + std::round(25 * s),
                                   textRight,
                                   row.rect.bottom - std::round(5 * s)};
        if (IDWriteTextLayout* layout = view.layout.get()) {
            target->PushAxisAlignedClip(textRect, D2D1_ANTIALIAS_MODE_ALIASED);
            brush->SetColor(Fg(0.92f));
            target->DrawTextLayout({textRect.left, textRect.top}, layout, brush,
                                   D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
            target->PopAxisAlignedClip();
        }
        // Copied: a check for a moment.
        const bool copied = m_copiedId == note.id;
        DrawIconButton(target, brush, row.copy,
                       copied ? kGlyphCheck : kGlyphCopy,
                       hover && m_hover.part == Part::Copy,
                       copied ? 0.95f : 0.5f);
        DrawIconButton(target, brush, row.remove, kGlyphClose,
                       hover && m_hover.part == Part::Delete, 0.35f);
        if (layered) {
            target->PopLayer();
        }
    }
    EndScrollArea(target);
    DrawScrollBar(target, brush, m_listViewport, m_scroll);
}

////////////////////////////////////////////////////////////////////////////////
// Tasks: "+ New task" (a box: Enter adds it, Escape gives up), the ones in
// progress, then the last completed ones, dimmed; the circle completes one
// (its check fills, then it slides down to the completed ones) or brings it
// back; editing and deleting show under the mouse.

struct Task {
    long long id = 0;
    long long stamp = 0;
    bool done = false;
    std::wstring title;
};
constexpr size_t kMaxTasks = 200;
constexpr size_t kMaxTaskLength = 300;
// Completed ones kept (the oldest go).
constexpr size_t kMaxDoneTasks = 10;
constexpr float kTasksWidth = 440;
// A checked task: its check fills, then it goes down to the completed ones.
constexpr double kTaskCheckSeconds = 0.28;
constexpr double kTaskCompleteSeconds = 0.6;
constexpr WCHAR kGlyphDelete = 0xE74D;

class TasksPanel : public EditingPanel {
   protected:
    void Prepare(float scale) override {
        PrepareFormats(scale);
        Load();
        Layout();
    }

    void OnOpen() override {
        m_hover = {};
        m_scroll.Reset();
        m_typing = false;
        m_editingId = 0;
        m_text.clear();
        m_edit.Reset();
        m_rowY.clear();
        Layout();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

    bool OnMouseMove(POINT pt) override {
        const Hit hit = HitTest(pt);
        if (hit == m_hover) {
            return false;
        }
        m_hover = hit;
        return true;
    }
    bool OnMouseLeave() override {
        m_hover = {};
        return true;
    }
    bool OnMouseDown(POINT pt) override {
        if (m_typing && PointInRect(pt, m_box)) {
            EditTextClick(m_text, m_edit, pt);
            return true;
        }
        return false;
    }
    bool OnMouseUp(POINT pt) override {
        const Hit hit = HitTest(pt);
        switch (hit.part) {
            case Part::New:
                StartTyping(0);
                return true;
            case Part::Add:
                Commit();
                return true;
            case Part::Cancel:
                StopTyping();
                return true;
            case Part::Check:
                Toggle(hit.id);
                return true;
            case Part::Edit:
                StartTyping(hit.id);
                return true;
            case Part::Delete:
                if (!RowLeaving(hit.id)) {
                    std::erase_if(m_completing, [&](const auto& entry) {
                        return entry.first == hit.id;
                    });
                    m_deleting.push_back(hit.id);
                    RowLeaves(hit.id);
                }
                return true;
            default:
                return false;
        }
    }
    bool OnWheel(int delta) override {
        return m_scroll.Wheel(delta, std::round(60 * m_scale));
    }
    bool OnKey(WPARAM key) override {
        if (!m_typing) {
            return false;
        }
        if (key == VK_ESCAPE) {
            StopTyping();
            return true;
        }
        return EditTextKey(m_text, m_edit, key);
    }
    bool OnChar(WCHAR c) override {
        if (!m_typing) {
            // Typing starts a new one.
            if (c >= 0x20) {
                StartTyping(0);
            } else {
                return false;
            }
        }
        if (c == L'\r' || c == 0x0A) {
            Commit();
            return true;
        }
        if (c == 0x1B) {
            return false;
        }
        // One line.
        const bool taken = c != L'\n' && EditTextChar(m_text, m_edit, c,
                                                      kMaxTaskLength, m_hwnd);
        std::replace(m_text.begin(), m_text.end(), L'\n', L' ');
        return taken;
    }
    bool AnimateContents(double dt) override {
        const bool scrolling = m_scroll.Animate(dt);
        // Checked ones, once their check has filled: down to the completed
        // ones.
        const double now = NowSeconds();
        std::vector<long long> finished;
        for (const auto& [id, at] : m_completing) {
            if (now - at >= kTaskCompleteSeconds) {
                finished.push_back(id);
            }
        }
        for (long long id : finished) {
            std::erase_if(m_completing, [&](const auto& entry) {
                return entry.first == id;
            });
            MoveTask(id, true);
        }
        std::vector<long long> gone;
        const bool rows = AnimateRows(dt, &gone);
        for (long long id : gone) {
            if (std::find(m_deleting.begin(), m_deleting.end(), id) !=
                m_deleting.end()) {
                std::erase_if(m_tasks,
                              [&](const Task& task) { return task.id == id; });
            }
            std::erase(m_deleting, id);
        }
        if (!gone.empty() || !finished.empty()) {
            Save();
        }
        if (rows || !gone.empty() || !finished.empty()) {
            Layout();
        }
        // Each row slides to its place (a task moving between the lists,
        // the others making room).
        bool sliding = false;
        for (const auto& row : m_rows) {
            RowY& y = FindRowY(row.id, row.rect.top);
            if (SpringTowards(y.y, y.velocity, row.rect.top, 0.5, dt, 0.92)) {
                sliding = true;
            }
        }
        return AnimateTyping(m_typing, scrolling || rows || sliding ||
                                           !m_completing.empty());
    }

   private:
    enum class Part { None, New, Add, Cancel, Check, Row, Edit, Delete };
    struct Hit {
        Part part = Part::None;
        long long id = 0;
        bool operator==(const Hit& other) const {
            return part == other.part && id == other.id;
        }
    };
    struct Row {
        long long id;
        D2D1_RECT_F rect;
        D2D1_RECT_F check;
        D2D1_RECT_F edit;
        D2D1_RECT_F remove;
        bool done;
    };
    // Where each row is drawn (sliding to its place).
    struct RowY {
        long long id;
        double y;
        double velocity;
    };
    RowY& FindRowY(long long id, float target) {
        for (auto& entry : m_rowY) {
            if (entry.id == id) {
                return entry;
            }
        }
        return m_rowY.emplace_back(RowY{id, target, 0});
    }
    float ShownY(const Row& row) const {
        for (const auto& entry : m_rowY) {
            if (entry.id == row.id) {
                return (float)entry.y;
            }
        }
        return row.rect.top;
    }

    // Read again as it opens (another island may have changed them).
    void Load() {
        m_tasks.clear();
        for (const auto& record : LoadRecords(L"tasks", 4)) {
            m_tasks.push_back({_wtoi64(record[0].c_str()),
                               _wtoi64(record[1].c_str()), record[2] == L"1",
                               record[3]});
        }
        // The ones in progress first, then the completed ones (as kept),
        // only the latest of those.
        std::stable_partition(m_tasks.begin(), m_tasks.end(),
                              [](const Task& task) { return !task.done; });
        size_t doneCount = 0;
        std::erase_if(m_tasks, [&](const Task& t) {
            return t.done && ++doneCount > kMaxDoneTasks;
        });
    }
    void Save() {
        std::vector<std::vector<std::wstring>> records;
        for (const auto& task : m_tasks) {
            records.push_back({std::to_wstring(task.id),
                               std::to_wstring(task.stamp),
                               task.done ? L"1" : L"0", task.title});
        }
        SaveRecords(L"tasks", records);
    }
    // Completed (the first of the completed ones) or back in progress (the
    // last of the ones in progress); only the latest completed ones kept.
    void MoveTask(long long id, bool done) {
        const auto found =
            std::find_if(m_tasks.begin(), m_tasks.end(),
                         [&](const Task& task) { return task.id == id; });
        if (found == m_tasks.end()) {
            return;
        }
        Task task = std::move(*found);
        m_tasks.erase(found);
        task.done = done;
        m_tasks.insert(std::find_if(m_tasks.begin(), m_tasks.end(),
                                    [](const Task& t) { return t.done; }),
                       std::move(task));
        size_t doneCount = 0;
        std::erase_if(m_tasks, [&](const Task& t) {
            return t.done && ++doneCount > kMaxDoneTasks;
        });
    }
    void Toggle(long long id) {
        const auto found =
            std::find_if(m_tasks.begin(), m_tasks.end(),
                         [&](const Task& task) { return task.id == id; });
        if (found == m_tasks.end() || RowLeaving(id)) {
            return;
        }
        const bool completing =
            std::any_of(m_completing.begin(), m_completing.end(),
                        [&](const auto& entry) { return entry.first == id; });
        if (completing) {
            // Clicked again before it went: it stays.
            std::erase_if(m_completing, [&](const auto& entry) {
                return entry.first == id;
            });
        } else if (!found->done) {
            m_completing.push_back({id, NowSeconds()});
        } else {
            MoveTask(id, false);
            Save();
            Layout();
        }
        StartIslandAnimation();
    }
    void StartTyping(long long id) {
        m_typing = true;
        m_editingId = id;
        m_text.clear();
        for (const auto& task : m_tasks) {
            if (task.id == id) {
                m_text = task.title;
            }
        }
        m_edit.Reset();
        m_edit.caret = m_text.size();
        g_lastTyped = NowSeconds();
        Layout();
        StartIslandAnimation();
    }
    void StopTyping() {
        m_typing = false;
        m_editingId = 0;
        m_text.clear();
        m_edit.Reset();
        Layout();
    }
    void Commit() {
        const size_t first = m_text.find_first_not_of(L' ');
        if (first == std::wstring::npos) {
            StopTyping();
            return;
        }
        const std::wstring title = m_text.substr(first);
        if (m_editingId) {
            for (auto& task : m_tasks) {
                if (task.id == m_editingId) {
                    task.title = title;
                }
            }
        } else {
            if (m_tasks.size() >= kMaxTasks) {
                // The oldest completed one goes (or the oldest).
                auto oldest = std::find_if(m_tasks.rbegin(), m_tasks.rend(),
                                           [](const Task& t) { return t.done; });
                m_tasks.erase(oldest != m_tasks.rend()
                                  ? std::next(oldest).base()
                                  : m_tasks.end() - 1);
            }
            const long long now = NowStamp();
            m_tasks.insert(m_tasks.begin(), {now, now, false, title});
            RowArrives(now);
        }
        Save();
        StopTyping();
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(kTasksWidth * s);
        const float padding = std::round(12 * s);
        const float gap = std::round(8 * s);
        float y = padding;
        const float header = std::round(30 * s);
        m_new = {padding, y, padding + std::round(150 * s), y + header};
        m_count = {m_new.right, y, width - padding - std::round(4 * s),
                   y + header};
        y += header + gap;
        if (m_typing) {
            m_box = {padding, y, width - padding, y + std::round(36 * s)};
            y += m_box.bottom - m_box.top + gap;
            // "Cancel" and "Add" (or "Save").
            const float half = std::floor((width - padding * 2 - gap) / 2);
            m_cancel = {padding, y, padding + half, y + std::round(30 * s)};
            m_add = {width - padding - half, y, width - padding,
                     y + std::round(30 * s)};
            y += std::round(30 * s) + gap;
        } else {
            m_box = m_cancel = m_add = {};
        }
        m_rows.clear();
        const float row = std::round(46 * s);
        const float circle = std::round(28 * s);
        const float button = std::round(28 * s);
        float listY = 0;
        auto addRow = [&](const Task& task) {
            const float room = RowRoom(task.id);
            const D2D1_RECT_F rect{padding, listY, width - padding,
                                   listY + row * room};
            const float middle = rect.top + row * room / 2;
            const D2D1_RECT_F remove{rect.right - std::round(8 * s) - button,
                                     middle - button / 2,
                                     rect.right - std::round(8 * s),
                                     middle + button / 2};
            const D2D1_RECT_F edit{remove.left - std::round(4 * s) - button,
                                   middle - button / 2,
                                   remove.left - std::round(4 * s),
                                   middle + button / 2};
            m_rows.push_back(
                {task.id, rect,
                 {rect.left + std::round(8 * s), middle - circle / 2,
                  rect.left + std::round(8 * s) + circle, middle + circle / 2},
                 edit, remove, task.done});
            listY += (row + std::round(6 * s)) * room;
        };
        size_t inProgress = 0;
        size_t done = 0;
        for (const auto& task : m_tasks) {
            if (!task.done) {
                addRow(task);
                inProgress++;
            } else {
                done++;
            }
        }
        m_none = {};
        if (!inProgress) {
            m_none = {padding, listY, width - padding,
                      listY + std::round(44 * s)};
            listY += std::round(44 * s);
        }
        m_doneHeader = {};
        if (done) {
            listY += std::round(4 * s);
            m_doneHeader = {padding, listY, width - padding,
                            listY + std::round(26 * s)};
            listY += std::round(28 * s);
            for (const auto& task : m_tasks) {
                if (task.done) {
                    addRow(task);
                }
            }
        }
        std::erase_if(m_rowY, [&](const RowY& entry) {
            return std::none_of(m_rows.begin(), m_rows.end(),
                                [&](const Row& r) { return r.id == entry.id; });
        });
        const float content = std::max(std::round(44 * s), listY);
        const float room =
            std::max(std::round(80 * s), ListMaxHeight() - y - padding);
        const float viewport = std::min(content, room);
        m_listViewport = {0, y, width, y + viewport};
        m_scroll.SetSizes(viewport, content);
        y += viewport + padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    Hit HitTest(POINT pt) const {
        if (PointInRect(pt, m_new)) {
            return {Part::New};
        }
        if (PointInRect(pt, m_add)) {
            return {Part::Add};
        }
        if (PointInRect(pt, m_cancel)) {
            return {Part::Cancel};
        }
        POINT inList;
        if (!ListPoint(pt, &inList)) {
            return {};
        }
        for (const auto& row : m_rows) {
            if (!PointInRect(inList, row.rect)) {
                continue;
            }
            if (PointInRect(inList, row.check)) {
                return {Part::Check, row.id};
            }
            if (PointInRect(inList, row.edit)) {
                return {Part::Edit, row.id};
            }
            if (PointInRect(inList, row.remove)) {
                return {Part::Delete, row.id};
            }
            return {Part::Row, row.id};
        }
        return {};
    }

    // How far a checked task's check has filled (0 to 1), or -1.
    float CheckProgress(long long id) const {
        for (const auto& [taskId, at] : m_completing) {
            if (taskId == id) {
                return (float)std::clamp((NowSeconds() - at) / kTaskCheckSeconds,
                                         0.0, 1.0);
            }
        }
        return -1;
    }

    std::vector<Task> m_tasks;
    // Being checked (their check filling), and being deleted (once out of
    // view).
    std::vector<std::pair<long long, double>> m_completing;
    std::vector<long long> m_deleting;
    std::vector<RowY> m_rowY;
    bool m_typing = false;
    long long m_editingId = 0;
    std::wstring m_text;
    TextEdit m_edit;
    Hit m_hover;
    D2D1_RECT_F m_new{}, m_count{}, m_box{}, m_add{}, m_cancel{}, m_none{},
        m_doneHeader{};
    std::vector<Row> m_rows;
};

void TasksPanel::Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
    if (!Ready()) {
        return;
    }
    const float s = m_scale;
    // "+ New task" ("Edit task" while one is changed).
    {
        const bool editing = m_typing && m_editingId;
        const float lit = Hover(m_new, m_hover.part == Part::New);
        const float radius = std::round(9 * s);
        if (lit > 0.01f) {
            brush->SetColor(Fg(0.08f * lit));
            GlassFill(target, {m_new, radius, radius}, brush);
        }
        brush->SetColor(Fg(0.95f));
        DrawGlyph(target, editing ? &kGlyphPencil : &kGlyphAdd, 1,
                  m_bigIconFormat.get(),
                  {m_new.left, m_new.top,
                   m_new.left + std::round(28 * s), m_new.bottom},
                  brush);
        PCWSTR text = editing ? Tr(L"Edit task", L"Editar tarefa")
                              : Tr(L"New task", L"Nova tarefa");
        target->DrawText(text, (UINT32)wcslen(text), m_boldFormat.get(),
                         {m_new.left + std::round(28 * s), m_new.top,
                          m_new.right, m_new.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    // How many are left.
    size_t inProgress = 0;
    size_t completed = 0;
    for (const auto& task : m_tasks) {
        (task.done ? completed : inProgress)++;
    }
    {
        const std::wstring count =
            std::to_wstring(inProgress) +
            Tr(inProgress == 1 ? L" in progress" : L" in progress",
               inProgress == 1 ? L" em andamento" : L" em andamento");
        brush->SetColor(D2D1::ColorF(0.3f, 0.85f, 0.75f, 1));
        m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        target->DrawText(count.c_str(), (UINT32)count.size(),
                         m_smallFormat.get(), m_count, brush,
                         D2D1_DRAW_TEXT_OPTIONS_CLIP);
        m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    }
    if (m_typing) {
        DrawTextBox(target, brush, m_box, m_text, m_edit,
                    m_editingId ? Tr(L"Task", L"Tarefa")
                                : Tr(L"What needs doing?",
                                     L"O que precisa ser feito?"),
                    true);
        const bool empty = m_text.find_first_not_of(L' ') == std::wstring::npos;
        DrawButton(target, brush, m_cancel, Tr(L"Cancel", L"Cancelar"), false,
                   m_hover.part == Part::Cancel);
        DrawButton(target, brush, m_add,
                   m_editingId ? Tr(L"Save  (Enter)", L"Salvar  (Enter)")
                               : Tr(L"Add  (Enter)", L"Adicionar  (Enter)"),
                   true, m_hover.part == Part::Add, empty ? 0.5f : 1.0f);
    }
    BeginScrollArea(target, m_listViewport, m_scroll);
    const float top = m_scroll.Offset();
    const float bottom = top + m_listViewport.bottom - m_listViewport.top;
    // Nothing left to do.
    if (m_none.bottom > m_none.top) {
        PCWSTR none = completed ? Tr(L"All done", L"Tudo feito")
                                : Tr(L"Nothing to do", L"Nada pendente");
        brush->SetColor(Fg(0.4f));
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        target->DrawText(none, (UINT32)wcslen(none), m_textFormat.get(),
                         m_none, brush);
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    }
    // "Completed", over the last ones done.
    if (m_doneHeader.bottom > m_doneHeader.top) {
        const std::wstring header =
            std::wstring(Tr(L"Completed", L"Concluídas")) + L"  ·  " +
            std::to_wstring(completed);
        brush->SetColor(Fg(0.45f));
        target->DrawText(header.c_str(), (UINT32)header.size(),
                         m_smallFormat.get(),
                         {m_doneHeader.left + std::round(4 * s),
                          m_doneHeader.top, m_doneHeader.right,
                          m_doneHeader.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        brush->SetColor(Fg(0.08f));
        target->FillRectangle(
            {m_doneHeader.left + std::round(4 * s), m_doneHeader.bottom - 1,
             m_doneHeader.right - std::round(4 * s), m_doneHeader.bottom},
            brush);
    }
    for (const auto& row : m_rows) {
        // Where it is drawn now (sliding to its place).
        const float dy = ShownY(row) - row.rect.top;
        auto at = [dy](D2D1_RECT_F r) {
            r.top += dy;
            r.bottom += dy;
            return r;
        };
        const D2D1_RECT_F rect = at(row.rect);
        if (rect.bottom < top || rect.top > bottom) {
            continue;
        }
        const Task* task = nullptr;
        for (const auto& t : m_tasks) {
            if (t.id == row.id) {
                task = &t;
            }
        }
        if (!task) {
            continue;
        }
        const float room = RowRoom(row.id);
        if (room < 0.02f) {
            continue;
        }
        // The completed ones dimmed; growing in and shrinking away fade
        // (drawn on a layer only then: a layer per row makes scrolling heavy).
        const float alpha = room * (row.done ? 0.55f : 1.0f);
        winrt::com_ptr<ID2D1Layer> layer;
        const bool layered = alpha < 0.999f;
        if (layered) {
            target->CreateLayer(nullptr, layer.put());
            target->PushLayer(
                D2D1::LayerParameters(rect, nullptr,
                                      D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                      D2D1::IdentityMatrix(), alpha),
                layer.get());
        }
        const bool hover = m_hover.id == row.id && !RowLeaving(row.id);
        const float lit = Hover(HoverKey(7100, row.id), hover);
        const float radius = std::round(10 * s);
        // (The one being edited lit too.)
        const bool editing = m_typing && m_editingId == row.id;
        brush->SetColor(Fg(editing ? 0.12f : 0.05f + 0.05f * lit));
        GlassFill(target, {rect, radius, radius}, brush);
        brush->SetColor(Fg(0.09f));
        target->DrawRoundedRectangle(
            {{rect.left + 0.5f, rect.top + 0.5f, rect.right - 0.5f,
              rect.bottom - 0.5f},
             radius,
             radius},
            brush, 1);
        // The circle: filled with a check when done (filling as it's
        // checked).
        const D2D1_RECT_F check = at(row.check);
        const D2D1_POINT_2F center{(check.left + check.right) / 2,
                                   (check.top + check.bottom) / 2};
        const float circle = std::round(8.5f * s);
        const float checkLit = Hover(
            HoverKey(7101, row.id), hover && m_hover.part == Part::Check);
        const float progress = CheckProgress(row.id);
        const float filled =
            row.done ? 1.0f
            : progress >= 0
                ? (float)EaseInOutCubic(progress)
                : 0.0f;
        brush->SetColor(Fg(0.6f + 0.35f * checkLit));
        target->DrawEllipse({center, circle, circle}, brush,
                            std::max(1.0f, 1.5f * s));
        if (filled > 0.01f) {
            // A little bigger as it fills, then back.
            const float pop = 1 + 0.25f * std::sin(filled * 3.14159f);
            const float r = circle * filled * pop;
            brush->SetColor(D2D1::ColorF(0.2f, 0.78f, 0.35f, 1));
            target->FillEllipse({center, r, r}, brush);
            brush->SetColor(D2D1::ColorF(1, 1, 1, filled));
            // On top of the list's own placement (it scrolls).
            D2D1_MATRIX_3X2_F before;
            target->GetTransform(&before);
            target->SetTransform(
                D2D1::Matrix3x2F::Scale(pop, pop, center) *
                *D2D1::Matrix3x2F::ReinterpretBaseType(&before));
            DrawGlyph(target, &kGlyphCheck, 1, m_iconFormat.get(), check,
                      brush);
            target->SetTransform(before);
        }
        // The title, and under the mouse its date; its buttons on the right
        // (the title makes room for them).
        const D2D1_RECT_F edit = at(row.edit);
        const D2D1_RECT_F remove = at(row.remove);
        const float buttons = Hover(HoverKey(7102, row.id), hover);
        const float textLeft = check.right + std::round(8 * s);
        const float textRight =
            rect.right - std::round(10 * s) -
            (rect.right - edit.left - std::round(4 * s)) * buttons;
        const bool struck = row.done || filled > 0.01f;
        brush->SetColor(Fg(struck ? 0.95f - 0.4f * filled : 0.95f));
        const float titleBottom = buttons > 0.01f
                                      ? rect.top + std::round(28 * s)
                                      : rect.bottom;
        target->DrawText(task->title.c_str(), (UINT32)task->title.size(),
                         m_textFormat.get(),
                         {textLeft, rect.top, textRight,
                          titleBottom + (rect.bottom - titleBottom) *
                                            (1 - buttons)},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        if (struck) {
            // Struck through (drawn across as it's checked).
            winrt::com_ptr<IDWriteTextLayout> layout;
            if (SUCCEEDED(m_dwrite->CreateTextLayout(
                    task->title.c_str(), (UINT32)task->title.size(),
                    m_textFormat.get(), std::max(1.0f, textRight - textLeft),
                    100, layout.put()))) {
                DWRITE_TEXT_METRICS metrics{};
                layout->GetMetrics(&metrics);
                const float middle =
                    buttons > 0.01f
                        ? rect.top + std::round(14 * s)
                        : (rect.top + rect.bottom) / 2;
                const float lineY =
                    middle + (((rect.top + rect.bottom) / 2) - middle) *
                                 (1 - buttons);
                const float length =
                    std::min(metrics.width, textRight - textLeft) * filled;
                brush->SetColor(Fg(0.5f));
                target->FillRectangle({textLeft, lineY, textLeft + length,
                                       lineY + std::max(1.0f, s)},
                                      brush);
            }
        }
        if (buttons > 0.01f) {
            const std::wstring stamp = StampText(task->stamp);
            brush->SetColor(Fg(0.45f * buttons));
            target->DrawText(stamp.c_str(), (UINT32)stamp.size(),
                             m_smallFormat.get(),
                             {textLeft, rect.top + std::round(25 * s),
                              textRight, rect.bottom - std::round(3 * s)},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            DrawIconButton(target, brush, edit, kGlyphPencil,
                           hover && m_hover.part == Part::Edit,
                           0.6f * buttons);
            DrawIconButton(target, brush, remove, kGlyphDelete,
                           hover && m_hover.part == Part::Delete,
                           0.6f * buttons);
        }
        if (layered) {
            target->PopLayer();
        }
    }
    EndScrollArea(target);
    DrawScrollBar(target, brush, m_listViewport, m_scroll);
}

////////////////////////////////////////////////////////////////////////////////
// Your commands (islandCommands, #6165 #6161): shortcuts the user adds to
// programs and files, each a name and what it does: open something (a
// program with its arguments, a file, a folder, a link or a settings page,
// as Windows' Run box does) or run a command line (in cmd, without a window;
// if it fails, what it printed shows in a terminal, as komorebi's buttons
// do: see StartKomorebiJob). Each is a row in their panel with the icon of
// what it opens, and a button of its own on the pill only if chosen; nothing
// runs by itself.

struct UserCommand {
    long long id = 0;
    bool terminal = false;
    // A button of its own on the pill (only if chosen).
    bool pinned = false;
    std::wstring name;
    std::wstring target;
    std::wstring arguments;
    // The file its icon comes from (see CommandIconPath), found as it's
    // read; empty for its kind's glyph.
    std::wstring iconPath;
};
constexpr size_t kMaxCommandText = 1024;
constexpr float kCommandsWidth = 560;
// A command line's, and an opening one's without an icon of its own.
constexpr WCHAR kGlyphCommand = 0xE756;
constexpr WCHAR kGlyphOpenCommand = 0xE8A7;

WCHAR CommandGlyph(const UserCommand& command) {
    return command.terminal ? kGlyphCommand : kGlyphOpenCommand;
}

// The file whose icon stands for an opening command (the shell gives it, see
// LoadAppIconBitmap): what it opens, if it's there on a drive (never a
// network path, which can hang); a program by its name as the Run box finds
// it (the PATH, then App Paths); a link's or a settings page's program (like
// the browser for https:). Empty for a command line, and when none is found.
std::wstring CommandIconPath(const UserCommand& command) {
    if (command.terminal || command.target.empty()) {
        return {};
    }
    auto unquoted = [](std::wstring text) {
        if (text.size() >= 2 && text.front() == L'"' && text.back() == L'"') {
            text = text.substr(1, text.size() - 2);
        }
        return text;
    };
    auto there = [](const std::wstring& file) {
        return IsLocalDrivePath(file) &&
                       GetFileAttributes(file.c_str()) !=
                           INVALID_FILE_ATTRIBUTES
                   ? file
                   : std::wstring();
    };
    const std::wstring path = ExpandedAsNative(unquoted(command.target));
    if (path.empty()) {
        return {};
    }
    if (IsLocalDrivePath(path)) {
        return there(path);
    }
    // A link or a settings page ("https:", "ms-settings:"): its protocol's
    // program.
    const size_t colon = path.find(L':');
    if (colon != std::wstring::npos && colon > 1 &&
        std::all_of(path.begin(), path.begin() + colon, [](WCHAR c) {
            return iswalnum(c) || c == L'+' || c == L'-' || c == L'.';
        })) {
        const std::wstring scheme = path.substr(0, colon);
        WCHAR program[MAX_PATH];
        DWORD size = ARRAYSIZE(program);
        if (SUCCEEDED(AssocQueryString(ASSOCF_IS_PROTOCOL, ASSOCSTR_EXECUTABLE,
                                       scheme.c_str(), nullptr, program,
                                       &size))) {
            return there(unquoted(program));
        }
        return {};
    }
    // A program by its name ("notepad", "chrome.exe"), as the 64-bit Run box
    // finds it.
    if (path.find_first_of(L"\\/:") == std::wstring::npos) {
        WCHAR found[MAX_PATH];
        if (SearchPath(nullptr, path.c_str(), L".exe", ARRAYSIZE(found), found,
                       nullptr)) {
            if (std::wstring file = there(ExpandedAsNative(found));
                !file.empty()) {
                return file;
            }
        }
        if (IsWow64()) {
            WCHAR windows[MAX_PATH];
            if (GetWindowsDirectory(windows, ARRAYSIZE(windows)) &&
                SearchPath((std::wstring(windows) + L"\\Sysnative").c_str(),
                           path.c_str(), L".exe", ARRAYSIZE(found), found,
                           nullptr)) {
                return there(found);
            }
        }
        std::wstring key =
            L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\" +
            path;
        if (path.find(L'.') == std::wstring::npos) {
            key += L".exe";
        }
        for (HKEY root : {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE}) {
            WCHAR value[MAX_PATH];
            DWORD size = sizeof(value);
            if (RegGetValue(root, key.c_str(), nullptr,
                            RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ |
                                RRF_SUBKEY_WOW6464KEY,
                            nullptr, value, &size) == ERROR_SUCCESS) {
                return there(ExpandedAsNative(unquoted(value)));
            }
        }
    }
    return {};
}

// As saved ("commands", the account's own, see UserValueName: id, a
// command line, on the pill, name, target, arguments), read once and again
// after a change (see CommandsChanged).
std::vector<UserCommand> g_commands;
bool g_commandsRead = false;

const std::vector<UserCommand>& Commands() {
    if (!g_commandsRead) {
        g_commandsRead = true;
        g_commands.clear();
        for (const auto& record : LoadRecords(
                 UserValueName(L"commands").c_str(), 6, 1024 * 1024)) {
            UserCommand command{_wtoi64(record[0].c_str()), record[1] == L"1",
                                record[2] == L"1", record[3], record[4],
                                record[5]};
            command.iconPath = CommandIconPath(command);
            g_commands.push_back(std::move(command));
        }
    }
    return g_commands;
}

void SaveCommands(const std::vector<UserCommand>& commands) {
    std::vector<std::vector<std::wstring>> records;
    for (const auto& command : commands) {
        records.push_back({std::to_wstring(command.id),
                           command.terminal ? L"1" : L"0",
                           command.pinned ? L"1" : L"0", command.name,
                           command.target, command.arguments});
    }
    SaveRecords(UserValueName(L"commands").c_str(), records);
    g_commands = commands;
    for (auto& command : g_commands) {
        command.iconPath = CommandIconPath(command);
    }
    g_commandsRead = true;
}

// The islands' pills laid out again (a command pinned or not). Defined with
// the islands.
void CommandsChanged();

// An opening one, on a thread of its own that owns it (opening can wait: a
// network folder, a program asking for administrator rights).
struct CommandOpenJob {
    std::wstring target;
    std::wstring arguments;
};

DWORD WINAPI CommandOpenThreadProc(LPVOID parameter) {
    std::unique_ptr<CommandOpenJob> job((CommandOpenJob*)parameter);
    const HRESULT com = CoInitializeEx(
        nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    SHELLEXECUTEINFO info{sizeof(info)};
    // (%USERPROFILE% and the like as the 64-bit Run box takes them.)
    std::wstring target = ExpandedAsNative(job->target);
    if (target.empty()) {
        target = job->target;
    }
    info.fMask = SEE_MASK_NOASYNC;
    info.lpVerb = L"open";
    info.lpFile = target.c_str();
    info.lpParameters =
        job->arguments.empty() ? nullptr : job->arguments.c_str();
    info.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteEx(&info)) {
        IslandLog(L"A command didn't open: %u", GetLastError());
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    return 0;
}

// The commands under way (the islands' thread's): opening ones' threads,
// and command lines' (with each one's stop event).
std::vector<HANDLE> g_commandThreads;
std::vector<std::pair<HANDLE, HANDLE>> g_commandJobs;

// One run (false if it couldn't start). Not a device's path (\\.\ or \\?\,
// which can hang a call).
bool RunUserCommand(const UserCommand& command) {
    std::erase_if(g_commandThreads, [](HANDLE thread) {
        if (WaitForSingleObject(thread, 0) != WAIT_OBJECT_0) {
            return false;
        }
        CloseHandle(thread);
        return true;
    });
    std::erase_if(g_commandJobs, [](const auto& job) {
        if (WaitForSingleObject(job.first, 0) != WAIT_OBJECT_0) {
            return false;
        }
        CloseHandle(job.first);
        CloseHandle(job.second);
        return true;
    });
    if (command.target.empty() || command.target.starts_with(L"\\\\.\\") ||
        command.target.starts_with(L"\\\\?\\")) {
        return false;
    }
    if (command.terminal) {
        HANDLE thread = nullptr;
        HANDLE stop = nullptr;
        if (!StartKomorebiJob(command.target, -1, nullptr, &thread, &stop)) {
            return false;
        }
        g_commandJobs.push_back({thread, stop});
        return true;
    }
    auto* job = new CommandOpenJob{command.target, command.arguments};
    HANDLE thread =
        CreateThread(nullptr, 0, CommandOpenThreadProc, job, 0, nullptr);
    if (!thread) {
        delete job;
        return false;
    }
    g_commandThreads.push_back(thread);
    return true;
}

// As the islands go: the command lines told to stop waiting (their programs
// go on: the mod never ends them), and every thread waited for a moment.
void StopUserCommands() {
    std::vector<HANDLE> threads = std::move(g_commandThreads);
    g_commandThreads.clear();
    for (const auto& [thread, stop] : g_commandJobs) {
        SetEvent(stop);
        threads.push_back(thread);
    }
    JoinOwningThreads(threads, 3000);
    for (const auto& job : g_commandJobs) {
        CloseHandle(job.second);
    }
    g_commandJobs.clear();
    g_commandsRead = false;
}

// A program or a file chosen for a command (on a thread of its own: the
// dialog waits there); its path comes back to the panel (lParam: an
// std::wstring* it takes; null when none). A shortcut chosen stays one (its
// arguments and icon with it).
constexpr UINT WM_APP_COMMAND_PICKED = WM_APP + 103;
// (Defined with the shortcuts' panel, which picks a file the same way.)
HWND CreateFileDialogOwner(HWND panel);
void EndFileDialogThread(HANDLE* thread);

