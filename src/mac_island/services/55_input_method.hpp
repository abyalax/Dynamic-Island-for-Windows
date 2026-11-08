// The keyboards installed (Windows' input languages), opened from the
// keyboard language's item, like Windows' own indicator: each with its
// language and its layout's name, the one in use checked. A click switches
// the app in front to it (WM_INPUTLANGCHANGEREQUEST, as the language bar
// does); the last row opens Windows' language settings.
struct KeyboardEntry {
    HKL layout;
    std::wstring language;
    std::wstring name;
};

// A layout's name ("Portuguese (Brazil ABNT2)"), from its entry under
// Keyboard Layouts: its ID is the HKL's high word, or for a variant (0xF...)
// the entry whose "Layout Id" is its low 12 bits.
std::wstring KeyboardLayoutName(HKL layout) {
    constexpr WCHAR kLayouts[] =
        L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts";
    const WORD device = HIWORD((UINT_PTR)layout);
    WCHAR id[16] = L"";
    if ((device & 0xF000) == 0xF000) {
        WCHAR wanted[8];
        swprintf_s(wanted, L"%04x", device & 0x0FFF);
        HKEY layouts;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, kLayouts, 0, KEY_READ,
                         &layouts) == ERROR_SUCCESS) {
            WCHAR subkey[16];
            for (DWORD i = 0; !id[0]; i++) {
                DWORD length = ARRAYSIZE(subkey);
                if (RegEnumKeyEx(layouts, i, subkey, &length, nullptr,
                                 nullptr, nullptr,
                                 nullptr) != ERROR_SUCCESS) {
                    break;
                }
                WCHAR value[8] = L"";
                DWORD size = sizeof(value);
                if (RegGetValue(layouts, subkey, L"Layout Id", RRF_RT_REG_SZ,
                                nullptr, value, &size) == ERROR_SUCCESS &&
                    _wcsicmp(value, wanted) == 0) {
                    wcscpy_s(id, subkey);
                }
            }
            RegCloseKey(layouts);
        }
    } else {
        swprintf_s(id, L"%08X", device);
    }
    if (!id[0]) {
        return {};
    }
    const std::wstring path = std::wstring(kLayouts) + L"\\" + id;
    HKEY key;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &key) !=
        ERROR_SUCCESS) {
        return {};
    }
    WCHAR name[256] = L"";
    DWORD size = sizeof(name);
    if (RegLoadMUIString(key, L"Layout Display Name", name, sizeof(name),
                         nullptr, 0, nullptr) != ERROR_SUCCESS) {
        name[0] = 0;
        RegGetValue(key, nullptr, L"Layout Text", RRF_RT_REG_SZ, nullptr,
                    name, &size);
    }
    RegCloseKey(key);
    return name;
}

class KeyboardPanel : public Panel {
   public:
    // The window the keyboard is switched for (the app in front).
    void SetTarget(HWND window) { m_target = window; }

   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           13 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_smallFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_iconFormat = MakeFormat(IconFont(),
                                      DWRITE_FONT_WEIGHT_NORMAL, 14 * scale);
        }
        Load();
        Layout();
    }

    void OnOpen() override {
        m_hover = -1;
        Load();
        Layout();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

    bool OnMouseMove(POINT pt) override {
        const int hover = HitTest(pt);
        if (hover == m_hover) {
            return false;
        }
        m_hover = hover;
        return true;
    }

    bool OnMouseLeave() override {
        m_hover = -1;
        return true;
    }

    bool OnMouseUp(POINT pt) override;

   private:
    void Load() {
        m_entries.clear();
        const int count = GetKeyboardLayoutList(0, nullptr);
        std::vector<HKL> layouts(std::max(count, 0));
        if (count > 0) {
            GetKeyboardLayoutList(count, layouts.data());
        }
        for (HKL layout : layouts) {
            const LANGID language = LOWORD((UINT_PTR)layout);
            WCHAR locale[LOCALE_NAME_MAX_LENGTH] = L"";
            WCHAR display[128] = L"";
            if (LCIDToLocaleName(MAKELCID(language, SORT_DEFAULT), locale,
                                 ARRAYSIZE(locale), 0)) {
                GetLocaleInfoEx(locale, LOCALE_SLOCALIZEDDISPLAYNAME, display,
                                ARRAYSIZE(display));
            }
            m_entries.push_back({layout, display, KeyboardLayoutName(layout)});
        }
        m_current = IsWindow(m_target)
                        ? GetKeyboardLayout(
                              GetWindowThreadProcessId(m_target, nullptr))
                        : nullptr;
    }

    void Layout() {
        const float s = m_scale;
        const float padding = std::round(8 * s);
        const float width = std::round(300 * s);
        const float row = std::round(46 * s);
        float y = padding;
        m_rows.clear();
        for (size_t i = 0; i < m_entries.size(); i++) {
            m_rows.push_back({padding, y, width - padding, y + row});
            y += row;
        }
        // "Language settings", after a line.
        y += std::round(5 * s);
        m_settingsRow = {padding, y, width - padding,
                         y + std::round(34 * s)};
        y = m_settingsRow.bottom + padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    int HitTest(POINT pt) const {
        for (size_t i = 0; i < m_rows.size(); i++) {
            if (PointInRect(pt, m_rows[i])) {
                return (int)i;
            }
        }
        return PointInRect(pt, m_settingsRow) ? (int)m_rows.size() : -1;
    }

    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallFormat;
    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    float m_preparedScale = 0;
    HWND m_target = nullptr;
    HKL m_current = nullptr;
    std::vector<KeyboardEntry> m_entries;
    std::vector<D2D1_RECT_F> m_rows;
    D2D1_RECT_F m_settingsRow{};
    int m_hover = -1;
};

void KeyboardPanel::Draw(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush) {
    if (!m_textFormat || !m_smallFormat || !m_iconFormat) {
        return;
    }
    const float s = m_scale;
    const float radius = std::round(8 * s);
    const float inset = std::round(12 * s);
    const float check = std::round(28 * s);
    for (size_t i = 0; i < m_entries.size() && i < m_rows.size(); i++) {
        const KeyboardEntry& entry = m_entries[i];
        const D2D1_RECT_F& row = m_rows[i];
        const bool current = entry.layout == m_current;
        const float lit = Hover(HoverKey(6400, (int)i), m_hover == (int)i);
        if (current || lit > 0.01f) {
            brush->SetColor(current ? Accent(0.18f + 0.08f * lit)
                                    : Fg(0.09f * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        const float middle = (row.top + row.bottom) / 2;
        const float right = row.right - check;
        brush->SetColor(Fg(0.95f));
        target->DrawText(entry.language.c_str(), (UINT32)entry.language.size(),
                         m_textFormat.get(),
                         {row.left + inset, row.top + std::round(4 * s), right,
                          middle + std::round(2 * s)},
                         brush);
        brush->SetColor(Fg(0.6f));
        target->DrawText(entry.name.c_str(), (UINT32)entry.name.size(),
                         m_smallFormat.get(),
                         {row.left + inset, middle, right,
                          row.bottom - std::round(4 * s)},
                         brush);
        if (current) {
            const WCHAR glyph = 0xE73E;
            brush->SetColor(Accent(1));
            DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                      {right, row.top, row.right, row.bottom}, brush);
        }
    }
    // A line, then the settings.
    brush->SetColor(Fg(0.12f));
    const float lineY = m_settingsRow.top - std::round(3 * s);
    target->FillRectangle({m_settingsRow.left + inset, lineY,
                           m_settingsRow.right - inset, lineY + 1},
                          brush);
    const int settings = (int)m_rows.size();
    const float lit =
        Hover(HoverKey(6401, 0), m_hover == settings);
    if (lit > 0.01f) {
        brush->SetColor(Fg(0.09f * lit));
        GlassFill(target, {m_settingsRow, radius, radius}, brush);
    }
    const WCHAR gear = kGlyphSettings;
    brush->SetColor(Fg(0.8f));
    DrawGlyph(
        target, &gear, 1, m_iconFormat.get(),
        {m_settingsRow.left + std::round(4 * s), m_settingsRow.top,
         m_settingsRow.left + std::round(36 * s), m_settingsRow.bottom},
        brush);
    PCWSTR label = Tr(L"Language settings", L"Configurações de idioma");
    brush->SetColor(Fg(0.85f));
    target->DrawText(label, (UINT32)wcslen(label), m_textFormat.get(),
                     {m_settingsRow.left + std::round(40 * s),
                      m_settingsRow.top, m_settingsRow.right,
                      m_settingsRow.bottom},
                     brush);
}

bool KeyboardPanel::OnMouseUp(POINT pt) {
    const int hit = HitTest(pt);
    if (hit < 0) {
        return false;
    }
    Close();
    if (hit == (int)m_entries.size()) {
        ShellExecute(nullptr, L"open", L"ms-settings:regionlanguage", nullptr,
                     nullptr, SW_SHOWNORMAL);
        return false;
    }
    // The app comes back in front, and its focused window is asked to
    // switch (as Windows' indicator does for the app in front).
    if (IsWindow(m_target)) {
        SetForegroundWindow(m_target);
        GUITHREADINFO info{sizeof(info)};
        const DWORD thread = GetWindowThreadProcessId(m_target, nullptr);
        const HWND focus = GetGUIThreadInfo(thread, &info) && info.hwndFocus
                               ? info.hwndFocus
                               : m_target;
        PostMessage(focus, WM_INPUTLANGCHANGEREQUEST, 0,
                    (LPARAM)m_entries[hit].layout);
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// komorebi's layout on the pill (an option): what each monitor shows comes
// from komorebi itself, which writes its state into a pipe of the island's
// at every change (`komorebic subscribe-pipe`, as yasb and komorebi's own
// bar do), read on a thread of the main island's; nothing is polled. While
// komorebi doesn't run, the thread waits for komorebi's data folder to
// change (komorebi writes there as it starts), or for the island's own
// start button, then subscribes again. The menu's commands run hidden like
// the start button's (see KomorebiThreadProc).

// Tells the island komorebi's state changed (see g_komorebiState).
constexpr UINT WM_APP_KOMOREBI = WM_APP + 90;

