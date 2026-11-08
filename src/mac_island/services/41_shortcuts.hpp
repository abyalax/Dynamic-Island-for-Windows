// Picks a file (on a thread of its own: the dialog waits there) and reads
// its shortcuts.
DWORD WINAPI ImportShortcutsThreadProc(LPVOID parameter) {
    const HWND panel = (HWND)parameter;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    std::vector<ShortcutNote>* found = nullptr;
    winrt::com_ptr<IFileOpenDialog> dialog;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(dialog.put())))) {
        const COMDLG_FILTERSPEC filters[] = {
            {L"whkdrc, *.txt", L"whkdrc;*.whkdrc;*.txt"},
            {Tr(L"All files", L"Todos os arquivos"), L"*.*"}};
        dialog->SetFileTypes(ARRAYSIZE(filters), filters);
        dialog->SetTitle(Tr(L"Import shortcuts (like komorebi's whkdrc)",
                            L"Importar atalhos (como o whkdrc do komorebi)"));
        // Where komorebi keeps it.
        WCHAR config[MAX_PATH];
        winrt::com_ptr<IShellItem> folder;
        if (ExpandEnvironmentStrings(L"%USERPROFILE%\\.config", config,
                                     MAX_PATH) &&
            GetFileAttributes(config) != INVALID_FILE_ATTRIBUTES &&
            SUCCEEDED(SHCreateItemFromParsingName(config, nullptr,
                                                  IID_PPV_ARGS(folder.put())))) {
            dialog->SetFolder(folder.get());
        }
        const HWND owner = CreateFileDialogOwner(panel);
        winrt::com_ptr<IShellItem> item;
        PWSTR path = nullptr;
        if (SUCCEEDED(dialog->Show(owner)) &&
            SUCCEEDED(dialog->GetResult(item.put())) &&
            SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
            std::wstring text;
            if (ReadTextFile(path, 256 * 1024, &text)) {
                found = new std::vector<ShortcutNote>(ParseShortcutsFile(text));
            }
            CoTaskMemFree(path);
        }
        if (owner) {
            DestroyWindow(owner);
        }
    }
    dialog = nullptr;
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    if (!PostMessage(panel, WM_APP_SHORTCUTS_IMPORTED, 0, (LPARAM)found)) {
        delete found;
    }
    return 0;
}
constexpr float kShortcutsWidth = 440;

// A key's name, as on the keyboard.
std::wstring KeyName(UINT vk) {
    switch (vk) {
        case VK_LWIN:
        case VK_RWIN:
            return L"Win";
        case VK_CONTROL:
            return L"Ctrl";
        case VK_MENU:
            return L"Alt";
        case VK_SHIFT:
            return L"Shift";
        case VK_LEFT:
            return L"←";
        case VK_RIGHT:
            return L"→";
        case VK_UP:
            return L"↑";
        case VK_DOWN:
            return L"↓";
        case VK_SPACE:
            return Tr(L"Space", L"Espaço");
        case VK_RETURN:
            return L"Enter";
        case VK_TAB:
            return L"Tab";
        case VK_ESCAPE:
            return L"Esc";
        default:
            break;
    }
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
        return std::wstring(1, (WCHAR)vk);
    }
    if (vk >= VK_F1 && vk <= VK_F24) {
        return L"F" + std::to_wstring(vk - VK_F1 + 1);
    }
    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    LONG lParam = (LONG)(scanCode << 16);
    // The extended keys (Insert, Delete, Home, End, Page Up and Down).
    if ((vk >= VK_PRIOR && vk <= VK_HELP) || vk == VK_DIVIDE ||
        vk == VK_NUMLOCK) {
        lParam |= 1 << 24;
    }
    WCHAR name[64] = L"";
    if (GetKeyNameText(lParam, name, ARRAYSIZE(name)) > 0) {
        return name;
    }
    WCHAR hex[16];
    swprintf_s(hex, L"0x%02X", vk);
    return hex;
}

bool IsModifierKey(UINT vk) {
    return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
           vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
           vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
           vk == VK_LWIN || vk == VK_RWIN;
}

class ShortcutsPanel : public EditingPanel {
   public:
    // Alt with a key comes as a system key: it's recorded too.
    bool OnSysKey(WPARAM key) override {
        if (m_focus == Focus::Keys && !m_keysWritten) {
            Record((UINT)key);
            return true;
        }
        return false;
    }
    bool OnKeyUp(WPARAM key) override {
        if (m_focus != Focus::Keys || m_keysWritten || m_held.empty()) {
            return false;
        }
        std::erase(m_held, NormalKey((UINT)key));
        // All let go: that combination is done (another may follow).
        if (m_held.empty() && !m_chord.empty()) {
            if (!m_keys.empty()) {
                m_keys += L", ";
            }
            m_keys += ChordText();
            m_chord.clear();
            m_chords++;
        }
        return true;
    }

   protected:
    void Prepare(float scale) override {
        PrepareFormats(scale);
        if (scale != m_chipScale) {
            m_chipScale = scale;
            m_chipFormat = MakeFormat(L"Segoe UI Variable Text",
                                      DWRITE_FONT_WEIGHT_SEMI_BOLD, 11 * scale);
        }
        Load();
        Layout();
    }

    void OnOpen() override {
        m_hover = {};
        m_scroll.Reset();
        m_importStatus.clear();
        m_filter.clear();
        m_filterEdit.Reset();
        m_form = false;
        m_focus = Focus::Filter;
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
        if (PointInRect(pt, m_filterBox)) {
            m_focus = Focus::Filter;
            EditTextClick(m_filter, m_filterEdit, pt);
            return true;
        }
        if (m_form && PointInRect(pt, m_keysMode)) {
            return false;
        }
        if (m_form && PointInRect(pt, m_keysBox)) {
            m_focus = Focus::Keys;
            g_lastTyped = NowSeconds();
            if (m_keysWritten) {
                EditTextClick(m_keys, m_keysEdit, pt);
            }
            return true;
        }
        if (m_form && PointInRect(pt, m_descriptionBox)) {
            m_focus = Focus::Description;
            EditTextClick(m_description, m_descriptionEdit, pt);
            return true;
        }
        return false;
    }
    bool OnMouseUp(POINT pt) override {
        const Hit hit = HitTest(pt);
        switch (hit.part) {
            case Part::New:
                StartForm(0);
                return true;
            case Part::Import:
                if (!m_importThread) {
                    m_dialogShown = true;
                    m_importThread = CreateThread(
                        nullptr, 0, ImportShortcutsThreadProc, m_hwnd, 0,
                        nullptr);
                    if (!m_importThread) {
                        m_dialogShown = false;
                    }
                }
                return true;
            case Part::KeysMode:
                // Written or pressed.
                m_keysWritten = !m_keysWritten;
                m_held.clear();
                m_chord.clear();
                m_focus = Focus::Keys;
                m_keysEdit.Reset();
                m_keysEdit.caret = m_keys.size();
                g_lastTyped = NowSeconds();
                return true;
            case Part::Cancel:
                StopForm();
                return true;
            case Part::Save:
                Commit();
                return true;
            case Part::Row:
                StartForm(hit.id);
                return true;
            case Part::Delete:
                std::erase_if(m_shortcuts, [&](const ShortcutNote& shortcut) {
                    return shortcut.id == hit.id;
                });
                Save();
                Layout();
                return true;
            default:
                return false;
        }
    }
    bool OnWheel(int delta) override {
        return m_scroll.Wheel(delta, std::round(60 * m_scale));
    }
    bool OnKey(WPARAM key) override {
        if (m_focus == Focus::Keys && m_keysWritten) {
            if (key == VK_ESCAPE) {
                StopForm();
                return true;
            }
            if (key == VK_TAB) {
                m_focus = Focus::Description;
                return true;
            }
            return EditTextKey(m_keys, m_keysEdit, key);
        }
        if (m_focus == Focus::Keys) {
            // Nothing held: Escape gives up, Tab goes on to what it does,
            // Backspace takes back the last combination.
            if (m_held.empty() && key == VK_ESCAPE) {
                StopForm();
                return true;
            }
            if (m_held.empty() && key == VK_TAB && !m_keys.empty()) {
                m_focus = Focus::Description;
                return true;
            }
            if (m_held.empty() && key == VK_BACK) {
                const size_t comma = m_keys.rfind(L", ");
                m_keys = comma == std::wstring::npos ? std::wstring()
                                                     : m_keys.substr(0, comma);
                m_chords = m_keys.empty() ? 0 : std::max(m_chords - 1, 1);
                return true;
            }
            Record((UINT)key);
            return true;
        }
        if (key == VK_ESCAPE && m_form) {
            StopForm();
            return true;
        }
        if (key == VK_TAB && m_form) {
            m_focus = m_focus == Focus::Description ? Focus::Keys
                                                    : Focus::Description;
            return true;
        }
        if (m_focus == Focus::Description) {
            return EditTextKey(m_description, m_descriptionEdit, key);
        }
        const bool taken = EditTextKey(m_filter, m_filterEdit, key);
        if (taken) {
            Layout();
        }
        return taken;
    }
    bool OnChar(WCHAR c) override {
        if (m_focus == Focus::Keys && m_keysWritten && c != L'\t' &&
            c != 0x1B) {
            if (c == L'\r' || c == 0x0A) {
                m_focus = Focus::Description;
                return true;
            }
            return c != L'\n' &&
                   EditTextChar(m_keys, m_keysEdit, c, 80, m_hwnd);
        }
        if (m_focus == Focus::Keys || c == L'\t' || c == 0x1B) {
            return false;
        }
        if (m_focus == Focus::Description) {
            if (c == L'\r' || c == 0x0A) {
                Commit();
                return true;
            }
            return c != L'\n' && EditTextChar(m_description, m_descriptionEdit,
                                               c, 200, m_hwnd);
        }
        if (c == L'\r' || c == 0x0A) {
            return false;
        }
        const bool taken = EditTextChar(m_filter, m_filterEdit, c, 80, m_hwnd);
        if (taken) {
            m_scroll.Reset();
            Layout();
        }
        return taken;
    }
    bool AnimateContents(double dt) override {
        const bool scrolling = m_scroll.Animate(dt);
        return AnimateTyping(true, scrolling);
    }

   private:
    enum class Part { None, New, Import, Cancel, Save, KeysMode, Row, Delete };
    enum class Focus { Filter, Keys, Description };
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
        D2D1_RECT_F remove;
    };

    // Read again as it opens (another island may have changed them).
    void Load() {
        m_shortcuts.clear();
        for (const auto& record : LoadRecords(L"shortcuts", 3)) {
            m_shortcuts.push_back(
                {_wtoi64(record[0].c_str()), record[1], record[2]});
        }
    }
    void Save() {
        std::vector<std::vector<std::wstring>> records;
        for (const auto& shortcut : m_shortcuts) {
            records.push_back({std::to_wstring(shortcut.id), shortcut.keys,
                               shortcut.description});
        }
        SaveRecords(L"shortcuts", records);
    }
    // The same key whichever side it's on.
    static UINT NormalKey(UINT vk) {
        switch (vk) {
            case VK_LCONTROL:
            case VK_RCONTROL:
                return VK_CONTROL;
            case VK_LMENU:
            case VK_RMENU:
                return VK_MENU;
            case VK_LSHIFT:
            case VK_RSHIFT:
                return VK_SHIFT;
            case VK_RWIN:
                return VK_LWIN;
            default:
                return vk;
        }
    }
    // The keys held so far: the modifiers first (in the usual order), then
    // the others as they were pressed.
    std::wstring ChordText() const {
        std::wstring text;
        for (UINT modifier : {(UINT)VK_LWIN, (UINT)VK_CONTROL, (UINT)VK_MENU,
                              (UINT)VK_SHIFT}) {
            if (std::find(m_chord.begin(), m_chord.end(), modifier) !=
                m_chord.end()) {
                text += KeyName(modifier) + L"+";
            }
        }
        for (UINT vk : m_chord) {
            if (!IsModifierKey(vk)) {
                text += KeyName(vk) + L"+";
            }
        }
        if (!text.empty()) {
            text.pop_back();
        }
        return text;
    }
    void Record(UINT vk) {
        g_lastTyped = NowSeconds();
        vk = NormalKey(vk);
        // A new combination after one: up to three in a row.
        if (m_held.empty() && m_chord.empty() && m_chords >= 3) {
            return;
        }
        if (std::find(m_held.begin(), m_held.end(), vk) == m_held.end()) {
            m_held.push_back(vk);
        }
        if (std::find(m_chord.begin(), m_chord.end(), vk) == m_chord.end() &&
            m_chord.size() < 6) {
            m_chord.push_back(vk);
        }
    }
    // What's shown: the combinations done, and the one being pressed.
    std::wstring KeysShown() const {
        if (m_chord.empty()) {
            return m_keys;
        }
        return m_keys + (m_keys.empty() ? L"" : L", ") + ChordText();
    }
    void StartForm(long long id) {
        m_form = true;
        m_editingId = id;
        m_keys.clear();
        m_description.clear();
        for (const auto& shortcut : m_shortcuts) {
            if (shortcut.id == id) {
                m_keys = shortcut.keys;
                m_description = shortcut.description;
            }
        }
        m_descriptionEdit.Reset();
        m_descriptionEdit.caret = m_description.size();
        m_held.clear();
        m_chord.clear();
        m_chords = 0;
        for (size_t at = 0; !m_keys.empty() && at != std::wstring::npos;
             at = m_keys.find(L", ", at + 1)) {
            m_chords++;
        }
        m_focus = id ? Focus::Description : Focus::Keys;
        g_lastTyped = NowSeconds();
        Layout();
        StartIslandAnimation();
    }
    void StopForm() {
        m_form = false;
        m_editingId = 0;
        m_focus = Focus::Filter;
        Layout();
    }
    void Commit() {
        if (m_keysWritten) {
            std::wstring tidy;
            for (size_t i = 0; i < m_keys.size(); i++) {
                const WCHAR c = m_keys[i];
                const bool nearPlus =
                    c == L' ' &&
                    ((i + 1 < m_keys.size() && m_keys[i + 1] == L'+') ||
                     (!tidy.empty() && tidy.back() == L'+'));
                if (!nearPlus) {
                    tidy += c;
                }
            }
            m_keys = tidy;
            while (!m_keys.empty() && m_keys.back() == L' ') {
                m_keys.pop_back();
            }
        }
        if (m_keys.empty()) {
            m_focus = Focus::Keys;
            return;
        }
        if (m_editingId) {
            for (auto& shortcut : m_shortcuts) {
                if (shortcut.id == m_editingId) {
                    shortcut.keys = m_keys;
                    shortcut.description = m_description;
                }
            }
        } else if (m_shortcuts.size() < kMaxShortcuts) {
            m_shortcuts.push_back({NowStamp(), m_keys, m_description});
        }
        Save();
        StopForm();
    }
    std::vector<const ShortcutNote*> Shown() const {
        std::vector<const ShortcutNote*> shown;
        for (const auto& shortcut : m_shortcuts) {
            if (m_filter.empty() ||
                MatchesSearch(shortcut.keys + L" " + shortcut.description,
                              m_filter)) {
                shown.push_back(&shortcut);
            }
        }
        return shown;
    }

    void Layout() {
        const float s = m_scale;
        const float padding = std::round(12 * s);
        const float gap = std::round(8 * s);
        // As wide as the longest keys and description need (up to most of
        // the screen); then the descriptions wrap.
        const auto shown = Shown();
        float keys = std::round(120 * s);
        float description = 0;
        for (const ShortcutNote* shortcut : shown) {
            keys = std::max(keys, DrawKeys(nullptr, nullptr, shortcut->keys, 0,
                                           0, 100000));
            winrt::com_ptr<IDWriteTextLayout> layout;
            if (m_textFormat &&
                SUCCEEDED(m_dwrite->CreateTextLayout(
                    shortcut->description.c_str(),
                    (UINT32)shortcut->description.size(), m_textFormat.get(),
                    100000, 100, layout.put()))) {
                DWRITE_TEXT_METRICS metrics{};
                layout->GetMetrics(&metrics);
                description = std::max(description, std::ceil(metrics.width));
            }
        }
        const float extra = padding * 2 + std::round(8 * s) +
                            std::round(14 * s) + std::round(40 * s);
        const float most = std::max(
            std::round(kShortcutsWidth * s),
            (float)(m_monitorRect.right - m_monitorRect.left) -
                std::round(64 * s));
        keys = std::min(keys, most * 0.55f);
        const float width = std::clamp(extra + keys + description,
                                       std::round(kShortcutsWidth * s), most);
        m_keysColumn = keys;
        const float descriptionWidth = width - extra - keys;
        float y = padding;
        m_filterBox = {padding, y, width - padding, y + std::round(34 * s)};
        y += m_filterBox.bottom - m_filterBox.top + gap;
        m_rows.clear();
        const float line = std::round(18 * s);
        float listY = 0;
        for (const ShortcutNote* shortcut : shown) {
            const int lines =
                description > descriptionWidth
                    ? WrappedLineCount(m_dwrite, m_textFormat.get(),
                                       shortcut->description, descriptionWidth,
                                       line)
                    : 1;
            const float row =
                std::max(std::round(38 * s), line * lines + std::round(16 * s));
            const D2D1_RECT_F rect{padding, listY, width - padding,
                                   listY + row};
            const float middle = (rect.top + rect.bottom) / 2;
            const float close = std::round(26 * s);
            m_rows.push_back(
                {shortcut->id, rect,
                 {rect.right - close - std::round(6 * s), middle - close / 2,
                  rect.right - std::round(6 * s), middle + close / 2}});
            listY += row + std::round(2 * s);
        }
        const float content = m_rows.empty() ? std::round(48 * s) : listY;
        const float footer =
            m_form ? std::round(34 * s) * 2 + std::round(32 * s) + gap * 4
                   : std::round(32 * s) + gap;
        const float room = std::max(std::round(80 * s),
                                    MaxHeight() - y - footer - padding);
        const float viewport = std::min(content, room);
        m_listViewport = {0, y, width, y + viewport};
        m_scroll.SetSizes(viewport, content);
        y += viewport + gap;
        if (m_form) {
            const float half = std::floor((width - padding * 2 - gap) / 2);
            // The keys, and "Write" / "Press" beside them.
            const float mode = std::round(92 * s);
            m_keysBox = {padding, y, width - padding - mode - gap,
                         y + std::round(34 * s)};
            m_keysMode = {m_keysBox.right + gap, y, width - padding,
                          y + std::round(34 * s)};
            y += std::round(34 * s) + gap;
            m_descriptionBox = {padding, y, width - padding,
                                y + std::round(34 * s)};
            y += std::round(34 * s) + gap;
            m_cancel = {padding, y, padding + half, y + std::round(32 * s)};
            m_save = {width - padding - half, y, width - padding,
                      y + std::round(32 * s)};
            m_new = m_import = m_importStatusRect = {};
            y += std::round(32 * s);
        } else {
            m_keysBox = m_descriptionBox = m_cancel = m_save = m_keysMode = {};
            const float importWidth = std::round(150 * s);
            m_new = {padding, y, width - padding - importWidth - gap,
                     y + std::round(32 * s)};
            m_import = {width - padding - importWidth, y, width - padding,
                        y + std::round(32 * s)};
            y += std::round(32 * s);
            if (!m_importStatus.empty()) {
                m_importStatusRect = {padding, y + std::round(4 * s),
                                      width - padding, y + std::round(24 * s)};
                y += std::round(24 * s);
            } else {
                m_importStatusRect = {};
            }
        }
        y += padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    Hit HitTest(POINT pt) const {
        if (PointInRect(pt, m_new)) {
            return {Part::New};
        }
        if (PointInRect(pt, m_import)) {
            return {Part::Import};
        }
        if (PointInRect(pt, m_cancel)) {
            return {Part::Cancel};
        }
        if (PointInRect(pt, m_save)) {
            return {Part::Save};
        }
        if (PointInRect(pt, m_keysMode)) {
            return {Part::KeysMode};
        }
        POINT inList;
        if (!ListPoint(pt, &inList)) {
            return {};
        }
        for (const auto& row : m_rows) {
            if (!PointInRect(inList, row.rect)) {
                continue;
            }
            if (PointInRect(inList, row.remove)) {
                return {Part::Delete, row.id};
            }
            return {Part::Row, row.id};
        }
        return {};
    }

    // The keys as little keycaps joined by "+".
    // Returns where they end (only measured without a target).
    float DrawKeys(ID2D1RenderTarget* target,
                   ID2D1SolidColorBrush* brush,
                   const std::wstring& keys,
                   float x,
                   float middle,
                   float right);

    std::vector<ShortcutNote> m_shortcuts;
    std::wstring m_filter;
    TextEdit m_filterEdit;
    bool m_form = false;
    long long m_editingId = 0;
    std::wstring m_keys;
    // Being recorded: the keys down now, the ones pressed together so far,
    // and how many combinations are done.
    std::vector<UINT> m_held;
    std::vector<UINT> m_chord;
    int m_chords = 0;
    std::wstring m_description;
    TextEdit m_descriptionEdit;
    TextEdit m_keysEdit;
    Focus m_focus = Focus::Filter;
    Hit m_hover;
    float m_chipScale = 0;
    winrt::com_ptr<IDWriteTextFormat> m_chipFormat;
    // How much room the keys take (the widest of them).
    float m_keysColumn = 0;
    // Texts laid out once (a key's cap, a description), kept while their
    // text, size and scale stay: scrolling only moves them.
    struct CachedText {
        std::wstring text;
        float width;
        float height;
        float scale;
        bool wrapped;
        float measured;
        winrt::com_ptr<IDWriteTextLayout> layout;
    };
    std::vector<CachedText> m_textCache;
    CachedText& Cached(const std::wstring& text,
                       float width,
                       float height,
                       bool wrapped) {
        for (auto& entry : m_textCache) {
            if (entry.wrapped == wrapped && entry.width == width &&
                entry.height == height && entry.scale == m_scale &&
                entry.text == text) {
                return entry;
            }
        }
        if (m_textCache.size() >= 768) {
            m_textCache.clear();
        }
        CachedText entry{text, width, height, m_scale, wrapped, 0, nullptr};
        if (wrapped) {
            entry.layout = WrappedLayout(m_dwrite, m_textFormat.get(), text,
                                         width, height, std::round(18 * m_scale));
        } else if (m_chipFormat) {
            m_dwrite->CreateTextLayout(text.c_str(), (UINT32)text.size(),
                                       m_chipFormat.get(), width, height,
                                       entry.layout.put());
        }
        if (entry.layout) {
            DWRITE_TEXT_METRICS metrics{};
            entry.layout->GetMetrics(&metrics);
            entry.measured = wrapped ? metrics.height : metrics.width;
        }
        return m_textCache.emplace_back(std::move(entry));
    }
    D2D1_RECT_F m_filterBox{}, m_new{}, m_keysBox{}, m_descriptionBox{},
        m_cancel{}, m_save{}, m_keysMode{}, m_import{}, m_importStatusRect{};
    // The keys written (like "Ctrl+Shift+S") instead of pressed.
    bool m_keysWritten = false;
    // Importing from a file: its thread, and how it went.
    HANDLE m_importThread = nullptr;
    std::wstring m_importStatus;

   protected:
    bool OnAppMessage(UINT msg, WPARAM, LPARAM lParam) override {
        // (Only from its thread: the list is its.)
        if (msg != WM_APP_SHORTCUTS_IMPORTED || !m_importThread) {
            return false;
        }
        m_dialogShown = false;
        WaitForSingleObject(m_importThread, INFINITE);
        CloseHandle(m_importThread);
        m_importThread = nullptr;
        std::unique_ptr<std::vector<ShortcutNote>> found(
            (std::vector<ShortcutNote>*)lParam);
        if (found) {
            // The new ones (not the same keys and words again).
            size_t added = 0;
            for (auto& shortcut : *found) {
                const bool known = std::any_of(
                    m_shortcuts.begin(), m_shortcuts.end(),
                    [&](const ShortcutNote& other) {
                        return other.keys == shortcut.keys &&
                               other.description == shortcut.description;
                    });
                if (!known && m_shortcuts.size() < kMaxShortcuts) {
                    m_shortcuts.push_back(std::move(shortcut));
                    added++;
                }
            }
            Save();
            m_importStatus =
                std::to_wstring(added) + Tr(L" shortcuts imported",
                                            L" atalhos importados");
        }
        // Back in front (the dialog had it).
        SetForegroundWindow(m_hwnd);
        Layout();
        return true;
    }
    // The island going away with the dialog still open: it's closed, and
    // what it found (posted to this window, about to go) freed here.
    void OnDestroy() override {
        const bool importing = m_importThread != nullptr;
        EndFileDialogThread(&m_importThread);
        MSG msg;
        if (importing && PeekMessage(&msg, m_hwnd, WM_APP_SHORTCUTS_IMPORTED,
                                     WM_APP_SHORTCUTS_IMPORTED, PM_REMOVE)) {
            delete (std::vector<ShortcutNote>*)msg.lParam;
        }
    }

   private:
    std::vector<Row> m_rows;
};

float ShortcutsPanel::DrawKeys(ID2D1RenderTarget* target,
                               ID2D1SolidColorBrush* brush,
                               const std::wstring& keys,
                               float x,
                               float middle,
                               float right) {
    const float s = m_scale;
    const float height = std::round(22 * s);
    size_t at = 0;
    bool first = true;
    bool newChord = false;
    while (at <= keys.size() && x < right) {
        size_t plus = keys.find_first_of(L"+,", at);
        // A "+" or "," key itself ("Ctrl++").
        if (plus == at && plus + 1 <= keys.size()) {
            plus = keys.find_first_of(L"+,", at + 1);
        }
        std::wstring key = keys.substr(
            at, plus == std::wstring::npos ? std::wstring::npos : plus - at);
        while (!key.empty() && key[0] == L' ' && key.size() > 1) {
            key.erase(0, 1);
        }
        if (key.empty()) {
            break;
        }
        if (!first) {
            // "+" between keys pressed together, "," between combinations.
            const float jointWidth = std::round(14 * s);
            if (target) {
                if (IDWriteTextLayout* joint =
                        Cached(newChord ? L"," : L"+", jointWidth, height,
                               false)
                            .layout.get()) {
                    brush->SetColor(Fg(0.5f));
                    target->DrawTextLayout({x, middle - height / 2}, joint,
                                           brush);
                }
            }
            x += std::round(newChord ? 18 * s : 14 * s);
        }
        first = false;
        newChord = plus != std::wstring::npos && keys[plus] == L',';
        const float width =
            std::max(std::round(22 * s),
                     std::ceil(Cached(key, 400, height, false).measured) +
                         std::round(14 * s));
        const D2D1_RECT_F chip{x, middle - height / 2,
                               std::min(x + width, right),
                               middle + height / 2};
        const float radius = std::round(5 * s);
        if (!target) {
            x += width;
            if (plus == std::wstring::npos) {
                break;
            }
            at = plus + 1;
            continue;
        }
        brush->SetColor(Fg(0.1f));
        GlassFill(target, {chip, radius, radius}, brush);
        brush->SetColor(Fg(0.2f));
        target->DrawRoundedRectangle(
            {{chip.left + 0.5f, chip.top + 0.5f, chip.right - 0.5f,
              chip.bottom - 0.5f},
             radius,
             radius},
            brush, 1);
        brush->SetColor(Fg(0.92f));
        if (chip.right - chip.left < width - 0.5f) {
            // Cut at the column's end.
            target->DrawText(key.c_str(), (UINT32)key.size(),
                             m_chipFormat.get(), chip, brush,
                             D2D1_DRAW_TEXT_OPTIONS_CLIP);
        } else if (IDWriteTextLayout* cap =
                       Cached(key, width, height, false).layout.get()) {
            target->DrawTextLayout({chip.left, chip.top}, cap, brush);
        }
        x += width;
        if (plus == std::wstring::npos) {
            break;
        }
        at = plus + 1;
    }
    return x;
}

void ShortcutsPanel::Draw(ID2D1RenderTarget* target,
                          ID2D1SolidColorBrush* brush) {
    if (!Ready() || !m_chipFormat) {
        return;
    }
    const float s = m_scale;
    DrawTextBox(target, brush, m_filterBox, m_filter, m_filterEdit,
                Tr(L"Type to filter the shortcuts…",
                   L"Digite para filtrar os atalhos…"),
                m_focus == Focus::Filter);
    if (m_rows.empty()) {
        PCWSTR none = m_shortcuts.empty()
                          ? Tr(L"Add the shortcuts you want to remember",
                               L"Adicione os atalhos que você quer lembrar")
                          : Tr(L"None found", L"Nenhum encontrado");
        brush->SetColor(Fg(0.4f));
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        target->DrawText(none, (UINT32)wcslen(none), m_textFormat.get(),
                         m_listViewport, brush);
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    } else {
        BeginScrollArea(target, m_listViewport, m_scroll);
        const float top = m_scroll.Offset();
        const float bottom = top + m_listViewport.bottom - m_listViewport.top;
        const auto shown = Shown();
        for (size_t i = 0; i < m_rows.size() && i < shown.size(); i++) {
            const Row& row = m_rows[i];
            if (row.rect.bottom < top || row.rect.top > bottom) {
                continue;
            }
            const bool hover = m_hover.id == row.id;
            const float lit = Hover(HoverKey(7200, row.id), hover);
            const bool editing = m_form && m_editingId == row.id;
            if (lit > 0.01f || editing) {
                const float radius = std::round(8 * s);
                brush->SetColor(Fg(editing ? 0.12f : 0.07f * lit));
                GlassFill(target, {row.rect, radius, radius}, brush);
            }
            const float middle = (row.rect.top + row.rect.bottom) / 2;
            // The keys, then what they do (wrapped when long).
            const float keysLeft = row.rect.left + std::round(8 * s);
            DrawKeys(target, brush, shown[i]->keys, keysLeft, middle,
                     keysLeft + m_keysColumn);
            const float descriptionLeft =
                keysLeft + m_keysColumn + std::round(14 * s);
            const float descriptionRight =
                std::max(descriptionLeft, row.rect.right - std::round(32 * s));
            const CachedText& description =
                Cached(shown[i]->description,
                       descriptionRight - descriptionLeft,
                       row.rect.bottom - row.rect.top, true);
            if (description.layout) {
                brush->SetColor(Fg(0.9f));
                target->DrawTextLayout(
                    {descriptionLeft, middle - description.measured / 2},
                    description.layout.get(), brush,
                    D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
            }
            if (lit > 0.01f) {
                DrawIconButton(target, brush, row.remove, kGlyphClose,
                               hover && m_hover.part == Part::Delete,
                               0.45f * lit);
            }
        }
        EndScrollArea(target);
        DrawScrollBar(target, brush, m_listViewport, m_scroll);
    }
    if (m_form) {
        // The keys: pressed to record them (Backspace clears).
        const bool recording = m_focus == Focus::Keys;
        const std::wstring keysShown = KeysShown();
        if (m_keysWritten) {
            DrawTextBox(target, brush, m_keysBox, m_keys, m_keysEdit,
                        Tr(L"Like Ctrl+Shift+S, or Ctrl+K, Ctrl+S",
                           L"Como Ctrl+Shift+S, ou Ctrl+K, Ctrl+S"),
                        recording);
        } else if (keysShown.empty()) {
            DrawTextBox(target, brush, m_keysBox, m_keys, m_keysEdit,
                        recording ? Tr(L"Press the keys (Tab moves on)…",
                                       L"Pressione as teclas (Tab continua)…")
                                  : Tr(L"Click and press the keys",
                                       L"Clique e pressione as teclas"),
                        recording);
        } else {
            DrawTextBox(target, brush, m_keysBox, L"", m_keysEdit, L"",
                        recording);
            DrawKeys(target, brush, keysShown,
                     m_keysBox.left + std::round(8 * s),
                     (m_keysBox.top + m_keysBox.bottom) / 2,
                     m_keysBox.right - std::round(6 * s));
        }
        // Written or pressed.
        {
            const float lit =
                Hover(m_keysMode, m_hover.part == Part::KeysMode);
            const float radius = std::round(9 * s);
            brush->SetColor(Fg(0.1f + 0.08f * lit));
            GlassFill(target, {m_keysMode, radius, radius}, brush);
            PCWSTR mode = m_keysWritten ? Tr(L"Press", L"Gravar")
                                        : Tr(L"Write", L"Digitar");
            brush->SetColor(Fg(0.9f));
            target->DrawText(mode, (UINT32)wcslen(mode), m_buttonFormat.get(),
                             m_keysMode, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
        DrawTextBox(target, brush, m_descriptionBox, m_description,
                    m_descriptionEdit,
                    Tr(L"What it does", L"O que ele faz"),
                    m_focus == Focus::Description);
        DrawButton(target, brush, m_cancel, Tr(L"Cancel", L"Cancelar"), false,
                   m_hover.part == Part::Cancel);
        const bool ready = !m_keys.empty();
        DrawButton(target, brush, m_save,
                   m_editingId ? Tr(L"Save", L"Salvar")
                               : Tr(L"Add", L"Adicionar"),
                   true, m_hover.part == Part::Save, ready ? 1.0f : 0.5f);
    } else {
        DrawButton(target, brush, m_new,
                   Tr(L"+  New shortcut", L"+  Novo atalho"), false,
                   m_hover.part == Part::New);
        DrawButton(target, brush, m_import,
                   Tr(L"Import a file", L"Importar arquivo"), false,
                   m_hover.part == Part::Import,
                   m_importThread ? 0.5f : 1.0f);
        if (!m_importStatus.empty()) {
            brush->SetColor(Fg(0.55f));
            target->DrawText(m_importStatus.c_str(),
                             (UINT32)m_importStatus.size(),
                             m_smallFormat.get(), m_importStatusRect, brush);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// A small JSON reader, for the weather's answers.

struct Json {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type = Type::Null;
    double number = 0;
    bool boolean = false;
    std::wstring string;
    std::vector<Json> items;
    std::vector<std::pair<std::wstring, Json>> members;

    const Json& operator[](PCWSTR key) const {
        for (const auto& [name, value] : members) {
            if (name == key) {
                return value;
            }
        }
        return Missing();
    }
    const Json& operator[](size_t index) const {
        return index < items.size() ? items[index] : Missing();
    }
    double Number(double otherwise = 0) const {
        return type == Type::Number ? number : otherwise;
    }
    size_t Size() const { return items.size(); }
    static const Json& Missing() {
        static const Json missing;
        return missing;
    }
};

class JsonReader {
   public:
    explicit JsonReader(const std::wstring& text) : m_text(text) {}
    bool Read(Json* value) {
        return Value(value, 0) && (Skip(), m_at == m_text.size());
    }

   private:
    void Skip() {
        while (m_at < m_text.size() && iswspace(m_text[m_at])) {
            m_at++;
        }
    }
    bool Literal(PCWSTR word) {
        const size_t length = wcslen(word);
        if (m_text.compare(m_at, length, word) != 0) {
            return false;
        }
        m_at += length;
        return true;
    }
    bool String(std::wstring* out) {
        if (m_at >= m_text.size() || m_text[m_at] != L'"') {
            return false;
        }
        m_at++;
        while (m_at < m_text.size() && m_text[m_at] != L'"') {
            WCHAR c = m_text[m_at++];
            if (c == L'\\' && m_at < m_text.size()) {
                const WCHAR next = m_text[m_at++];
                switch (next) {
                    case L'n':
                        c = L'\n';
                        break;
                    case L't':
                        c = L'\t';
                        break;
                    case L'r':
                        c = L'\r';
                        break;
                    case L'b':
                    case L'f':
                        c = L' ';
                        break;
                    case L'u':
                        if (m_at + 4 > m_text.size()) {
                            return false;
                        }
                        c = (WCHAR)wcstoul(m_text.substr(m_at, 4).c_str(),
                                           nullptr, 16);
                        m_at += 4;
                        break;
                    default:
                        c = next;
                        break;
                }
            }
            *out += c;
        }
        if (m_at >= m_text.size()) {
            return false;
        }
        m_at++;
        return true;
    }
    bool Value(Json* value, int depth) {
        if (depth > 32) {
            return false;
        }
        Skip();
        if (m_at >= m_text.size()) {
            return false;
        }
        const WCHAR c = m_text[m_at];
        if (c == L'{') {
            m_at++;
            value->type = Json::Type::Object;
            Skip();
            if (m_at < m_text.size() && m_text[m_at] == L'}') {
                m_at++;
                return true;
            }
            for (;;) {
                Skip();
                std::wstring name;
                if (!String(&name)) {
                    return false;
                }
                Skip();
                if (m_at >= m_text.size() || m_text[m_at] != L':') {
                    return false;
                }
                m_at++;
                Json member;
                if (!Value(&member, depth + 1)) {
                    return false;
                }
                value->members.push_back({std::move(name), std::move(member)});
                Skip();
                if (m_at < m_text.size() && m_text[m_at] == L',') {
                    m_at++;
                    continue;
                }
                if (m_at < m_text.size() && m_text[m_at] == L'}') {
                    m_at++;
                    return true;
                }
                return false;
            }
        }
        if (c == L'[') {
            m_at++;
            value->type = Json::Type::Array;
            Skip();
            if (m_at < m_text.size() && m_text[m_at] == L']') {
                m_at++;
                return true;
            }
            for (;;) {
                Json item;
                if (!Value(&item, depth + 1)) {
                    return false;
                }
                value->items.push_back(std::move(item));
                Skip();
                if (m_at < m_text.size() && m_text[m_at] == L',') {
                    m_at++;
                    continue;
                }
                if (m_at < m_text.size() && m_text[m_at] == L']') {
                    m_at++;
                    return true;
                }
                return false;
            }
        }
        if (c == L'"') {
            value->type = Json::Type::String;
            return String(&value->string);
        }
        if (Literal(L"true") || Literal(L"false")) {
            value->type = Json::Type::Bool;
            value->boolean = m_text[m_at - 1] == L'e' && m_text[m_at - 2] == L'u';
            return true;
        }
        if (Literal(L"null")) {
            return true;
        }
        PCWSTR start = m_text.c_str() + m_at;
        PWSTR end = nullptr;
        value->number = wcstod(start, &end);
        if (end == start) {
            return false;
        }
        value->type = Json::Type::Number;
        m_at += end - start;
        return true;
    }

    const std::wstring& m_text;
    size_t m_at = 0;
};

// An https address's JSON (UTF-8).
bool HttpsGetJson(const std::wstring& url,
                  Json* json,
                  const std::atomic<bool>* stop = nullptr,
                  HttpsCancel* cancel = nullptr) {
    std::vector<BYTE> data;
    if (!HttpsGet(url, &data, 512 * 1024, 3000, stop, cancel)) {
        return false;
    }
    std::wstring text;
    const int length = MultiByteToWideChar(CP_UTF8, 0, (const char*)data.data(),
                                           (int)data.size(), nullptr, 0);
    if (length <= 0) {
        return false;
    }
    text.resize(length);
    MultiByteToWideChar(CP_UTF8, 0, (const char*)data.data(), (int)data.size(),
                        text.data(), length);
    return JsonReader(text).Read(json);
}

// Escaped for an address's query.
std::wstring UrlEncode(const std::wstring& text) {
    std::string utf8;
    const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
                                           (int)text.size(), nullptr, 0,
                                           nullptr, nullptr);
    utf8.resize(std::max(length, 0));
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(),
                        utf8.data(), length, nullptr, nullptr);
    std::wstring encoded;
    for (unsigned char c : utf8) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += (WCHAR)c;
        } else {
            WCHAR hex[4];
            swprintf_s(hex, L"%%%02X", c);
            encoded += hex;
        }
    }
    return encoded;
}

////////////////////////////////////////////////////////////////////////////////
// The weather, when chosen in the island's settings (off by default, since it
// goes online): from the free Open-Meteo service (open-meteo.com, no key), for
// the city typed in its panel (found with Open-Meteo's own geocoding), every
// 30 minutes while it's on the pill, on a thread of its own. Shown on the
// pill (its sky and temperature) and in its panel (now, the next hours, the
// week).

struct WeatherHour {
    std::wstring time;
    double temperature = 0;
    int code = 0;
    bool day = true;
    double wind = 0;
};
struct WeatherDay {
    std::wstring date;
    int code = 0;
    double high = 0;
    double low = 0;
};
struct WeatherData {
    bool valid = false;
    // Imperial units (mph), from Windows' measurement system; the
    // temperatures are in °C (shown in °F by Degrees).
    bool imperial = false;
    std::wstring city;
    double temperature = 0;
    double feelsLike = 0;
    int humidity = 0;
    int code = 0;
    bool day = true;
    int clouds = 0;
    double pressure = 0;
    double wind = 0;
    int rainChance = -1;
    double uv = -1;
    std::wstring sunrise;
    std::wstring sunset;
    std::vector<WeatherHour> hours;
    std::vector<WeatherDay> days;
    // The city's time now ("2026-10-06T00:45").
    std::wstring now;
    double fetchedAt = 0;
};
SRWLOCK g_weatherLock = SRWLOCK_INIT;
WeatherData g_weather;
// The city chosen: its name as shown, and where it is.
struct WeatherPlace {
    std::wstring name;
    double latitude = 0;
    double longitude = 0;
};
// Tells the island new weather came (wParam: 1 when the city wasn't found,
// 2 when the service didn't answer, 3 when Windows doesn't allow the
// computer's location, 4 when it wasn't found yet).
constexpr UINT WM_APP_WEATHER = WM_APP + 67;
constexpr double kWeatherRefreshSeconds = 30 * 60;

WeatherPlace LoadWeatherPlace() {
    WeatherPlace place;
    for (const auto& record : LoadRecords(L"weatherPlace", 3)) {
        place.name = record[0];
        place.latitude = _wtof(record[1].c_str());
        place.longitude = _wtof(record[2].c_str());
    }
    return place;
}

void SaveWeatherPlace(const WeatherPlace& place) {
    WCHAR latitude[32], longitude[32];
    swprintf_s(latitude, L"%.5f", place.latitude);
    swprintf_s(longitude, L"%.5f", place.longitude);
    SaveRecords(L"weatherPlace", {{place.name, latitude, longitude}});
}

bool UsesImperialUnits() {
    DWORD system = 0;
    return GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT,
                           LOCALE_IMEASURE | LOCALE_RETURN_NUMBER,
                           (LPWSTR)&system, sizeof(system) / sizeof(WCHAR)) &&
           system == 1;
}

// The temperatures shown in °F or °C (the current island's option, which
// follows Windows' measurement system unless one is chosen).
bool UsesFahrenheit() {
    return g_settings.weatherUnit == 0 ? UsesImperialUnits()
                                       : g_settings.weatherUnit == 2;
}

struct WeatherRequest {
    HWND notify;
    // A city to look for first (or the place known).
    std::wstring search;
    WeatherPlace place;
    // Where the computer is instead ("Use my location", weatherMyLocation),
    // and how many clicks on it had come when this was asked (a worker that
    // ends later must not undo a newer click, see g_weatherLocateClicks).
    bool locate = false;
    unsigned clicks = 0;
};

// Set when the island stops: the weather's thread gives up what it waits for
// (the location) instead of making the island's end wait for it. Read again
// as false by StartWeather.
std::atomic<bool> g_weatherStop{false};
// The clicks on "Use my location", counted on the island's thread.
std::atomic<unsigned> g_weatherLocateClicks{0};
// Its position not found this many times in a row since (a PC without
// Wi-Fi positioning): the city typed shows meanwhile, and the location is
// tried again at each fetch.
std::atomic<int> g_weatherLocateMisses{0};
constexpr int kWeatherLocateMisses = 3;
// A click came while the weather was being fetched: asked again as it ends.
bool g_weatherLocateAgain = false;

// Waits for one of Windows' asynchronous calls in short turns, so it can be
// canceled as the island stops (its status looked at: wait_for sets the
// call's Completed handler, which a call takes only once). False if it was
// (its result isn't there).
template <typename Operation>
bool WaitForWindows(const Operation& operation) {
    while (operation.Status() ==
           winrt::Windows::Foundation::AsyncStatus::Started) {
        if (g_weatherStop) {
            operation.Cancel();
            return false;
        }
        Sleep(100);
    }
    return true;
}

