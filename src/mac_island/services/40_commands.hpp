#pragma once
DWORD WINAPI PickCommandThreadProc(LPVOID parameter) {
    const HWND panel = (HWND)parameter;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    std::wstring* picked = nullptr;
    winrt::com_ptr<IFileOpenDialog> dialog;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(dialog.put())))) {
        FILEOPENDIALOGOPTIONS options = 0;
        if (SUCCEEDED(dialog->GetOptions(&options))) {
            dialog->SetOptions(options | FOS_NODEREFERENCELINKS |
                               FOS_FILEMUSTEXIST);
        }
        dialog->SetTitle(Tr(L"Choose a program or a file",
                            L"Escolha um programa ou um arquivo"));
        const HWND owner = CreateFileDialogOwner(panel);
        winrt::com_ptr<IShellItem> item;
        PWSTR path = nullptr;
        if (SUCCEEDED(dialog->Show(owner)) &&
            SUCCEEDED(dialog->GetResult(item.put())) &&
            SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
            picked = new std::wstring(path);
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
    if (!PostMessage(panel, WM_APP_COMMAND_PICKED, 0, (LPARAM)picked)) {
        delete picked;
    }
    return 0;
}

// An app's icon (or a file's) for a panel (see LoadAppIcon, defined with
// the notifications' panel), and one from the shell only (LoadShellIcon).
winrt::com_ptr<ID2D1Bitmap> LoadShellIcon(ID2D1RenderTarget* target,
                                          IWICImagingFactory* wic,
                                          const std::wstring& appId,
                                          int size);
winrt::com_ptr<ID2D1Bitmap> LoadAppIcon(ID2D1RenderTarget* target,
                                        IWICImagingFactory* wic,
                                        HWND from,
                                        const std::wstring& appId,
                                        int size,
                                        bool* fromExplorer);

// Their panel: "+ New command" (its name, what kind, what it opens (typed
// or chosen) or the command line, an opening one's arguments, and whether
// it's a button on the pill), then each one with the icon of what it opens
// and its buttons: run, on the pill or not, edit, delete.
class CommandsPanel : public EditingPanel {
   protected:
    void Prepare(float scale) override {
        if (!m_wic) {
            CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                             CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_wic.put()));
        }
        if (scale != m_iconScale) {
            m_iconScale = scale;
            m_icons.clear();
        }
        PrepareFormats(scale);
        Layout();
    }

    void OnOpen() override {
        m_hover = {};
        m_scroll.Reset();
        m_commands = Commands();
        m_status.clear();
        PrepareIcons();
        StopTyping();
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
        if (!m_typing) {
            return false;
        }
        for (int field = 0; field < 3; field++) {
            if (PointInRect(pt, BoxOf(field))) {
                m_field = field;
                EditTextClick(TextOf(field), EditOf(field), pt);
                return true;
            }
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
            case Part::KindOpen:
            case Part::KindTerminal:
                m_terminal = hit.part == Part::KindTerminal;
                if (m_terminal && m_field == 2) {
                    m_field = 1;
                }
                Layout();
                return true;
            case Part::Choose:
                if (!m_pickThread) {
                    m_dialogShown = true;
                    m_pickThread = CreateThread(
                        nullptr, 0, PickCommandThreadProc, m_hwnd, 0, nullptr);
                    if (!m_pickThread) {
                        m_dialogShown = false;
                    }
                }
                return true;
            case Part::PinSwitch:
                m_pinned = !m_pinned;
                return true;
            case Part::Run:
                if (const UserCommand* command = Find(hit.id)) {
                    if (RunUserCommand(*command)) {
                        m_ranId = hit.id;
                        m_ranAt = NowSeconds();
                        m_status.clear();
                    } else {
                        m_status = Tr(L"It couldn't start.",
                                      L"Não deu para começar.");
                    }
                    Layout();
                }
                return true;
            case Part::Pin:
                for (auto& command : m_commands) {
                    if (command.id == hit.id) {
                        command.pinned = !command.pinned;
                    }
                }
                Save();
                return true;
            case Part::Edit:
                StartTyping(hit.id);
                return true;
            case Part::Delete:
                std::erase_if(m_commands, [&](const UserCommand& command) {
                    return command.id == hit.id;
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
        if (!m_typing) {
            return false;
        }
        if (key == VK_ESCAPE) {
            StopTyping();
            return true;
        }
        if (key == VK_TAB) {
            const int fields = m_terminal ? 2 : 3;
            m_field = (m_field + (GetKeyState(VK_SHIFT) < 0 ? fields - 1 : 1)) %
                      fields;
            return true;
        }
        return EditTextKey(TextOf(m_field), EditOf(m_field), key);
    }
    bool OnChar(WCHAR c) override {
        if (!m_typing) {
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
        if (c == 0x1B || c == L'\t') {
            return false;
        }
        std::wstring& text = TextOf(m_field);
        const bool taken = c != L'\n' && EditTextChar(text, EditOf(m_field), c,
                                                      kMaxCommandText, m_hwnd);
        std::replace(text.begin(), text.end(), L'\n', L' ');
        return taken;
    }
    bool AnimateContents(double dt) override {
        const bool scrolling = m_scroll.Animate(dt);
        // (A run's check for a moment.)
        const bool ran = m_ranId && NowSeconds() - m_ranAt < 1.5;
        if (!ran) {
            m_ranId = 0;
        }
        return AnimateTyping(m_typing, scrolling || ran);
    }
    // The file chosen (only from its thread): what the form opens, and its
    // name if it has none yet.
    bool OnAppMessage(UINT msg, WPARAM, LPARAM lParam) override {
        if (msg != WM_APP_COMMAND_PICKED || !m_pickThread) {
            return false;
        }
        m_dialogShown = false;
        WaitForSingleObject(m_pickThread, INFINITE);
        CloseHandle(m_pickThread);
        m_pickThread = nullptr;
        std::unique_ptr<std::wstring> picked((std::wstring*)lParam);
        if (picked && m_typing && !m_terminal) {
            m_target = *picked;
            m_targetEdit.Reset();
            m_targetEdit.caret = m_target.size();
            if (m_name.empty()) {
                m_name = PathFindFileName(picked->c_str());
                if (const size_t dot = m_name.find_last_of(L'.');
                    dot != std::wstring::npos && dot > 0) {
                    m_name.erase(dot);
                }
                m_nameEdit.Reset();
                m_nameEdit.caret = m_name.size();
            }
            m_field = 2;
        }
        // Back in front (the dialog had it).
        SetForegroundWindow(m_hwnd);
        Layout();
        return true;
    }
    // The island going away with the dialog still open: it's closed, and
    // what it chose (posted to this window, about to go) freed here.
    void OnDestroy() override {
        const bool picking = m_pickThread != nullptr;
        EndFileDialogThread(&m_pickThread);
        MSG msg;
        if (picking && PeekMessage(&msg, m_hwnd, WM_APP_COMMAND_PICKED,
                                   WM_APP_COMMAND_PICKED, PM_REMOVE)) {
            delete (std::wstring*)msg.lParam;
        }
    }

   private:
    enum class Part {
        None,
        New,
        Add,
        Cancel,
        KindOpen,
        KindTerminal,
        // "Choose..." beside what it opens, and the switch for the pill.
        Choose,
        PinSwitch,
        Row,
        Run,
        Pin,
        Edit,
        Delete,
    };
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
        D2D1_RECT_F run;
        D2D1_RECT_F pin;
        D2D1_RECT_F edit;
        D2D1_RECT_F remove;
    };

    const UserCommand* Find(long long id) const {
        for (const auto& command : m_commands) {
            if (command.id == id) {
                return &command;
            }
        }
        return nullptr;
    }
    // Kept, the pills laid out again, and the list read back (with each
    // one's icon file).
    void Save() {
        SaveCommands(m_commands);
        m_commands = Commands();
        CommandsChanged();
        PrepareIcons();
    }
    // The form's fields: 0 the name, 1 what it opens or the command line,
    // 2 an opening one's arguments.
    std::wstring& TextOf(int field) {
        return field == 0 ? m_name : field == 1 ? m_target : m_arguments;
    }
    TextEdit& EditOf(int field) {
        return field == 0 ? m_nameEdit : field == 1 ? m_targetEdit
                                                    : m_argumentsEdit;
    }
    D2D1_RECT_F BoxOf(int field) const {
        return field == 0 ? m_nameBox : field == 1 ? m_targetBox : m_argsBox;
    }

    void StartTyping(long long id) {
        m_typing = true;
        m_editingId = id;
        m_name.clear();
        m_target.clear();
        m_arguments.clear();
        m_terminal = false;
        m_pinned = false;
        if (const UserCommand* command = Find(id)) {
            m_name = command->name;
            m_target = command->target;
            m_arguments = command->arguments;
            m_terminal = command->terminal;
            m_pinned = command->pinned;
        }
        for (int field = 0; field < 3; field++) {
            EditOf(field).Reset();
            EditOf(field).caret = TextOf(field).size();
        }
        m_field = 0;
        m_status.clear();
        g_lastTyped = NowSeconds();
        Layout();
        StartIslandAnimation();
    }
    void StopTyping() {
        m_typing = false;
        m_editingId = 0;
        m_name.clear();
        m_target.clear();
        m_arguments.clear();
        for (int field = 0; field < 3; field++) {
            EditOf(field).Reset();
        }
        Layout();
    }
    // Kept (without what it'd open, nothing; without a name, what it opens
    // names it).
    void Commit() {
        auto trimmed = [](const std::wstring& text) {
            const size_t first = text.find_first_not_of(L' ');
            const size_t last = text.find_last_not_of(L' ');
            return first == std::wstring::npos
                       ? std::wstring()
                       : text.substr(first, last - first + 1);
        };
        const std::wstring target = trimmed(m_target);
        if (target.empty()) {
            m_field = 1;
            m_status = m_terminal ? Tr(L"Type the command line.",
                                       L"Digite a linha de comando.")
                                  : Tr(L"Type what it opens.",
                                       L"Digite o que ele abre.");
            Layout();
            return;
        }
        UserCommand command;
        command.name = trimmed(m_name).empty() ? target : trimmed(m_name);
        command.target = target;
        command.arguments = m_terminal ? std::wstring() : trimmed(m_arguments);
        command.terminal = m_terminal;
        command.pinned = m_pinned;
        const auto old = std::find_if(
            m_commands.begin(), m_commands.end(),
            [&](const UserCommand& each) { return each.id == m_editingId; });
        if (m_editingId && old != m_commands.end()) {
            command.id = old->id;
            *old = command;
        } else {
            command.id = NowStamp();
            m_commands.push_back(command);
        }
        Save();
        StopTyping();
    }

    // The commands' icons (see CommandIconPath), made as the panel opens or
    // its list changes, never inside a frame (each may ask Explorer); kept
    // between openings, the ones no longer listed let go, one not found
    // asked again later (see AppIconRetryAt).
    void PrepareIcons() {
        ID2D1RenderTarget* target = DrawingTarget();
        if (!target || !m_wic) {
            return;
        }
        std::erase_if(m_icons, [&](const KeptAppIcon& icon) {
            return std::none_of(m_commands.begin(), m_commands.end(),
                                [&](const UserCommand& command) {
                                    return command.iconPath == icon.appId;
                                });
        });
        const double now = NowSeconds();
        for (const auto& command : m_commands) {
            const std::wstring& path = command.iconPath;
            if (path.empty()) {
                continue;
            }
            int misses = 0;
            const auto kept = std::find_if(
                m_icons.begin(), m_icons.end(),
                [&](const KeptAppIcon& icon) { return icon.appId == path; });
            if (kept != m_icons.end()) {
                if (!kept->retryAt || now < kept->retryAt) {
                    continue;
                }
                misses = kept->bitmap ? 0 : kept->misses + 1;
                m_icons.erase(kept);
            }
            bool fromExplorer = false;
            winrt::com_ptr<ID2D1Bitmap> bitmap =
                LoadAppIcon(target, m_wic.get(), m_hwnd, path,
                            (int)std::round(32 * m_scale), &fromExplorer);
            m_icons.push_back({path, bitmap,
                               AppIconRetryAt(fromExplorer, !!bitmap, misses,
                                              now),
                               bitmap ? 0 : misses});
        }
    }
    // A command's icon, as made (see PrepareIcons).
    ID2D1Bitmap* IconOf(const std::wstring& path) const {
        for (const auto& icon : m_icons) {
            if (!path.empty() && icon.appId == path) {
                return icon.bitmap.get();
            }
        }
        return nullptr;
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(kCommandsWidth * s);
        const float padding = std::round(14 * s);
        const float gap = std::round(8 * s);
        float y = padding;
        const float header = std::round(32 * s);
        m_new = {padding, y, padding + std::round(240 * s), y + header};
        m_count = {m_new.right, y, width - padding - std::round(4 * s),
                   y + header};
        y += header + gap;
        const float box = std::round(38 * s);
        const float button = std::round(34 * s);
        // (A field's name over it.)
        const float label = std::round(18 * s);
        auto labelOver = [&](D2D1_RECT_F* rect) {
            *rect = {padding + std::round(2 * s), y, width - padding,
                     y + label};
            y += label + std::round(4 * s);
        };
        if (m_typing) {
            labelOver(&m_nameLabel);
            m_nameBox = {padding, y, width - padding, y + box};
            y += box + gap * 1.5f;
            const float half = std::floor((width - padding * 2 - gap) / 2);
            m_kindOpen = {padding, y, padding + half, y + button};
            m_kindTerminal = {width - padding - half, y, width - padding,
                              y + button};
            y += button + std::round(4 * s);
            // (What the kind chosen does, in a line or two.)
            m_kindHint = {padding + std::round(2 * s), y, width - padding,
                          y + std::round(30 * s)};
            y += std::round(30 * s) + gap / 2;
            // (Two lines: a path is often longer than the box. An opening
            // one's with "Choose..." beside it.)
            labelOver(&m_targetLabel);
            const float tall = std::round(56 * s);
            const float choose = m_terminal ? 0 : std::round(120 * s);
            m_targetBox = {padding, y,
                           width - padding - (choose > 0 ? choose + gap : 0),
                           y + tall};
            m_choose = {};
            if (choose > 0) {
                m_choose = {width - padding - choose, y, width - padding,
                            y + box};
            }
            y += tall + gap * 1.5f;
            m_argsLabel = m_argsBox = {};
            if (!m_terminal) {
                labelOver(&m_argsLabel);
                m_argsBox = {padding, y, width - padding, y + box};
                y += box + gap * 1.5f;
            }
            m_pinRow = {padding, y, width - padding, y + button};
            y += button + gap;
            m_cancel = {padding, y, padding + half, y + button};
            m_add = {width - padding - half, y, width - padding, y + button};
            y += button + gap;
        } else {
            m_nameBox = m_kindOpen = m_kindTerminal = m_targetBox = m_choose =
                m_argsBox = m_pinRow = m_cancel = m_add = m_nameLabel =
                    m_targetLabel = m_argsLabel = m_kindHint = {};
        }
        m_statusRect = {};
        if (!m_status.empty()) {
            m_statusRect = {padding, y, width - padding,
                            y + std::round(18 * s)};
            y += std::round(18 * s) + gap / 2;
        }
        m_rows.clear();
        const float row = std::round(56 * s);
        const float small = std::round(32 * s);
        float listY = 0;
        for (const auto& command : m_commands) {
            const D2D1_RECT_F rect{padding, listY, width - padding,
                                   listY + row};
            const float middle = rect.top + row / 2;
            auto at = [&](int fromRight) {
                const float right =
                    rect.right - std::round(10 * s) -
                    fromRight * (small + std::round(4 * s));
                return D2D1_RECT_F{right - small, middle - small / 2, right,
                                   middle + small / 2};
            };
            m_rows.push_back({command.id, rect, at(3), at(2), at(1), at(0)});
            listY += row + std::round(6 * s);
        }
        m_none = {};
        if (m_commands.empty()) {
            m_none = {padding, listY, width - padding,
                      listY + std::round(56 * s)};
            listY += std::round(56 * s);
        }
        const float content = std::max(std::round(56 * s), listY);
        const float room =
            std::max(std::round(120 * s), ListMaxHeight() - y - padding);
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
        if (PointInRect(pt, m_kindOpen)) {
            return {Part::KindOpen};
        }
        if (PointInRect(pt, m_kindTerminal)) {
            return {Part::KindTerminal};
        }
        if (PointInRect(pt, m_choose)) {
            return {Part::Choose};
        }
        if (PointInRect(pt, m_pinRow)) {
            return {Part::PinSwitch};
        }
        POINT inList;
        if (!ListPoint(pt, &inList)) {
            return {};
        }
        for (const auto& row : m_rows) {
            if (!PointInRect(inList, row.rect)) {
                continue;
            }
            if (PointInRect(inList, row.run)) {
                return {Part::Run, row.id};
            }
            if (PointInRect(inList, row.pin)) {
                return {Part::Pin, row.id};
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

    std::vector<UserCommand> m_commands;
    bool m_typing = false;
    long long m_editingId = 0;
    std::wstring m_name;
    std::wstring m_target;
    std::wstring m_arguments;
    TextEdit m_nameEdit;
    TextEdit m_targetEdit;
    TextEdit m_argumentsEdit;
    int m_field = 0;
    bool m_terminal = false;
    bool m_pinned = false;
    // Choosing a file: its dialog's thread.
    HANDLE m_pickThread = nullptr;
    // The icons of what they open, by file (for this scale).
    winrt::com_ptr<IWICImagingFactory> m_wic;
    std::vector<KeptAppIcon> m_icons;
    float m_iconScale = 0;
    // What went wrong last (a run that couldn't start, the form), and the
    // one just run (a check for a moment).
    std::wstring m_status;
    long long m_ranId = 0;
    double m_ranAt = 0;
    Hit m_hover;
    D2D1_RECT_F m_new{}, m_count{}, m_nameBox{}, m_kindOpen{},
        m_kindTerminal{}, m_targetBox{}, m_choose{}, m_argsBox{}, m_pinRow{},
        m_add{}, m_cancel{}, m_none{}, m_statusRect{}, m_nameLabel{},
        m_targetLabel{}, m_argsLabel{}, m_kindHint{};
    std::vector<Row> m_rows;
};

void CommandsPanel::Draw(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush) {
    if (!Ready()) {
        return;
    }
    const float s = m_scale;
    // "+ New command" ("Edit command" while one is changed).
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
                  {m_new.left, m_new.top, m_new.left + std::round(28 * s),
                   m_new.bottom},
                  brush);
        PCWSTR text = editing ? Tr(L"Edit command", L"Editar comando")
                              : Tr(L"New command", L"Novo comando");
        target->DrawText(text, (UINT32)wcslen(text), m_titleFormat.get(),
                         {m_new.left + std::round(28 * s), m_new.top,
                          m_new.right, m_new.bottom},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        // (How many, over the list: not while one is changed.)
        if (!m_typing) {
            const std::wstring count = std::to_wstring(m_commands.size());
            brush->SetColor(Fg(0.45f));
            m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
            target->DrawText(count.c_str(), (UINT32)count.size(),
                             m_smallFormat.get(), m_count, brush,
                             D2D1_DRAW_TEXT_OPTIONS_CLIP);
            m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        }
    }
    if (m_typing) {
        auto label = [&](const D2D1_RECT_F& rect, PCWSTR text) {
            brush->SetColor(Fg(0.6f));
            target->DrawText(text, (UINT32)wcslen(text), m_smallFormat.get(),
                             rect, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        };
        label(m_nameLabel, Tr(L"Name", L"Nome"));
        DrawTextBox(target, brush, m_nameBox, m_name, m_nameEdit,
                    Tr(L"Name (like Downloads)", L"Nome (como Downloads)"),
                    m_field == 0);
        DrawButton(target, brush, m_kindOpen,
                   Tr(L"Open something", L"Abrir algo"), !m_terminal,
                   m_hover.part == Part::KindOpen);
        DrawButton(target, brush, m_kindTerminal,
                   Tr(L"Run a command line", L"Rodar uma linha de comando"),
                   m_terminal, m_hover.part == Part::KindTerminal);
        {
            PCWSTR hint =
                m_terminal
                    ? Tr(L"Runs in cmd.exe without a window; a terminal shows "
                         L"only if it fails.",
                         L"Roda no cmd.exe sem janela; um terminal só aparece "
                         L"se ele falhar.")
                    : Tr(L"Opens a program, a file, a folder, a link or a "
                         L"settings page, like the Run box (Win+R).",
                         L"Abre um programa, arquivo, pasta, link ou página "
                         L"de configurações, como a caixa Executar (Win+R).");
            brush->SetColor(Fg(0.5f));
            target->DrawText(hint, (UINT32)wcslen(hint), m_smallFormat.get(),
                             m_kindHint, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
        label(m_targetLabel, m_terminal
                                 ? Tr(L"Command line", L"Linha de comando")
                                 : Tr(L"What it opens", L"O que ele abre"));
        DrawTextBox(target, brush, m_targetBox, m_target, m_targetEdit,
                    m_terminal
                        ? Tr(L"Command (cmd, without a window)",
                             L"Comando (cmd, sem janela)")
                        : Tr(L"Program, file, folder, link or ms-settings:",
                             L"Programa, arquivo, pasta, link ou "
                             L"ms-settings:"),
                    m_field == 1);
        if (m_choose.right > m_choose.left) {
            DrawButton(target, brush, m_choose,
                       Tr(L"Choose...", L"Escolher..."), false,
                       m_hover.part == Part::Choose,
                       m_pickThread ? 0.5f : 1.0f);
        }
        if (!m_terminal) {
            label(m_argsLabel,
                  Tr(L"Arguments (optional)", L"Argumentos (opcional)"));
            DrawTextBox(target, brush, m_argsBox, m_arguments,
                        m_argumentsEdit,
                        Tr(L"Arguments (if any)", L"Argumentos (se tiver)"),
                        m_field == 2);
        }
        // A button on the pill: its name, and a switch (green when on).
        {
            const float lit =
                Hover(m_pinRow, m_hover.part == Part::PinSwitch);
            const float radius = std::round(9 * s);
            brush->SetColor(Fg(0.05f + 0.05f * lit));
            GlassFill(target, {m_pinRow, radius, radius}, brush);
            const float middle = (m_pinRow.top + m_pinRow.bottom) / 2;
            const float knobRoom = std::round(11 * s);
            const D2D1_RECT_F knob{m_pinRow.right - std::round(10 * s) -
                                       std::round(40 * s),
                                   middle - knobRoom,
                                   m_pinRow.right - std::round(10 * s),
                                   middle + knobRoom};
            PCWSTR text = Tr(L"Button on the island (its icon on the pill)",
                             L"Botão na ilha (o ícone dele na pílula)");
            brush->SetColor(Fg(0.9f));
            target->DrawText(text, (UINT32)wcslen(text), m_textFormat.get(),
                             {m_pinRow.left + std::round(12 * s),
                              middle - std::round(9 * s),
                              knob.left - std::round(8 * s),
                              middle + std::round(10 * s)},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            brush->SetColor(m_pinned ? D2D1::ColorF(0.19f, 0.82f, 0.35f, 1)
                                     : Fg(0.22f));
            target->FillRoundedRectangle({knob, knobRoom, knobRoom}, brush);
            const float dot = knobRoom - std::round(2 * s);
            brush->SetColor(KnobColor(m_pinned ? 1.0f : 0.0f));
            target->FillEllipse(
                {{m_pinned ? knob.right - knobRoom : knob.left + knobRoom,
                  middle},
                 dot,
                 dot},
                brush);
        }
        DrawButton(target, brush, m_cancel, Tr(L"Cancel", L"Cancelar"), false,
                   m_hover.part == Part::Cancel);
        DrawButton(target, brush, m_add,
                   m_editingId ? Tr(L"Save  (Enter)", L"Salvar  (Enter)")
                               : Tr(L"Add  (Enter)", L"Adicionar  (Enter)"),
                   true, m_hover.part == Part::Add);
    }
    if (!m_status.empty()) {
        brush->SetColor(D2D1::ColorF(1.0f, 0.55f, 0.5f, 1));
        target->DrawText(m_status.c_str(), (UINT32)m_status.size(),
                         m_smallFormat.get(), m_statusRect, brush,
                         D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    BeginScrollArea(target, m_listViewport, m_scroll);
    if (m_none.bottom > m_none.top) {
        PCWSTR none = Tr(L"None yet: a program, a folder, a link or a command "
                         L"line, a click away",
                         L"Nenhum ainda: um programa, uma pasta, um link ou "
                         L"uma linha de comando, a um clique");
        brush->SetColor(Fg(0.4f));
        m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        target->DrawText(none, (UINT32)wcslen(none), m_smallFormat.get(),
                         m_none, brush);
        m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    }
    for (const auto& row : m_rows) {
        const UserCommand* command = Find(row.id);
        if (!command) {
            continue;
        }
        const bool hover = m_hover.id == row.id;
        const float radius = std::round(12 * s);
        brush->SetColor(Fg(hover ? 0.08f : 0.05f));
        GlassFill(target, {row.rect, radius, radius}, brush);
        // Its icon: what it opens' own, or its kind's.
        const float middle = (row.rect.top + row.rect.bottom) / 2;
        const float iconSize = std::round(32 * s);
        const D2D1_RECT_F icon{row.rect.left + std::round(12 * s),
                               middle - iconSize / 2,
                               row.rect.left + std::round(12 * s) + iconSize,
                               middle + iconSize / 2};
        if (ID2D1Bitmap* bitmap = IconOf(command->iconPath)) {
            DrawBitmapSmooth(target, bitmap, icon);
        } else {
            const WCHAR glyph = CommandGlyph(*command);
            brush->SetColor(Fg(0.85f));
            DrawGlyph(target, &glyph, 1, m_bigIconFormat.get(), icon, brush);
        }
        const float textLeft = icon.right + std::round(12 * s);
        const float textRight = row.run.left - std::round(6 * s);
        brush->SetColor(Fg(0.95f));
        target->DrawText(command->name.c_str(), (UINT32)command->name.size(),
                         m_boldFormat.get(),
                         {textLeft, row.rect.top + std::round(9 * s),
                          textRight, row.rect.top + std::round(29 * s)},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        const std::wstring what =
            command->arguments.empty()
                ? command->target
                : command->target + L" " + command->arguments;
        brush->SetColor(Fg(0.45f));
        target->DrawText(what.c_str(), (UINT32)what.size(),
                         m_smallFormat.get(),
                         {textLeft, row.rect.top + std::round(29 * s),
                          textRight, row.rect.bottom - std::round(6 * s)},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        DrawIconButton(target, brush, row.run,
                       m_ranId == row.id ? kGlyphCheck : kGlyphPlay,
                       hover && m_hover.part == Part::Run,
                       m_ranId == row.id ? 0.95f : 0.6f);
        DrawIconButton(target, brush, row.pin, kGlyphPin,
                       hover && m_hover.part == Part::Pin,
                       command->pinned ? 0.95f : 0.35f);
        DrawIconButton(target, brush, row.edit, kGlyphPencil,
                       hover && m_hover.part == Part::Edit, 0.45f);
        DrawIconButton(target, brush, row.remove, kGlyphDelete,
                       hover && m_hover.part == Part::Delete, 0.35f);
    }
    EndScrollArea(target);
    DrawScrollBar(target, brush, m_listViewport, m_scroll);
}

////////////////////////////////////////////////////////////////////////////////
// Keyboard shortcuts to remember: the keys (recorded by pressing them) and
// what they do, written by the user; a box filters them.

struct ShortcutNote {
    long long id = 0;
    std::wstring keys;
    std::wstring description;
};
constexpr size_t kMaxShortcuts = 300;
// The file picked comes back to the panel (lParam: the shortcuts read, an
// std::vector<ShortcutNote>* it takes; null when none).
constexpr UINT WM_APP_SHORTCUTS_IMPORTED = WM_APP + 69;
// A backup's file dialog closed (see BackupThreadProc).
constexpr UINT WM_APP_BACKUP_DONE = WM_APP + 71;

// "alt + shift + h" as "Alt+Shift+H".
std::wstring TidyKeys(const std::wstring& keys) {
    std::wstring tidy;
    size_t at = 0;
    while (at <= keys.size()) {
        size_t plus = keys.find(L'+', at);
        std::wstring key = keys.substr(
            at, plus == std::wstring::npos ? std::wstring::npos : plus - at);
        const size_t first = key.find_first_not_of(L" \t");
        const size_t last = key.find_last_not_of(L" \t");
        key = first == std::wstring::npos ? L""
                                          : key.substr(first, last - first + 1);
        std::wstring lower = key;
        for (auto& c : lower) {
            c = towlower(c);
        }
        static const struct {
            PCWSTR from;
            PCWSTR to;
        } kNames[] = {{L"alt", L"Alt"},      {L"ctrl", L"Ctrl"},
                      {L"control", L"Ctrl"}, {L"shift", L"Shift"},
                      {L"win", L"Win"},      {L"lwin", L"Win"},
                      {L"rwin", L"Win"},     {L"super", L"Win"},
                      {L"return", L"Enter"}, {L"enter", L"Enter"},
                      {L"left", L"\u2190"},  {L"right", L"\u2192"},
                      {L"up", L"\u2191"},    {L"down", L"\u2193"},
                      {L"space", L"Space"},  {L"tab", L"Tab"},
                      {L"escape", L"Esc"},   {L"esc", L"Esc"},
                      {L"oem_1", L";"},      {L"oem_2", L"/"},
                      {L"oem_3", L"`"},      {L"oem_4", L"["},
                      {L"oem_5", L"\\"},     {L"oem_6", L"]"},
                      {L"oem_7", L"'"},      {L"oem_plus", L"Plus"},
                      {L"oem_minus", L"-"},  {L"oem_comma", L"Comma"},
                      {L"oem_period", L"."}};
        bool named = false;
        for (const auto& name : kNames) {
            if (lower == name.from) {
                key = name.to;
                named = true;
            }
        }
        if (!named && !key.empty()) {
            key[0] = towupper(key[0]);
        }
        if (!key.empty()) {
            tidy += (tidy.empty() ? L"" : L"+") + key;
        }
        if (plus == std::wstring::npos) {
            break;
        }
        at = plus + 1;
    }
    return tidy;
}

// The shortcuts in a file: lines "keys : what they do" (whkd's format;
// "#" starts a comment, "." an option).
std::vector<ShortcutNote> ParseShortcutsFile(const std::wstring& text) {
    std::vector<ShortcutNote> found;
    long long id = NowStamp();
    for (size_t at = 0; at < text.size();) {
        size_t end = text.find(L'\n', at);
        if (end == std::wstring::npos) {
            end = text.size();
        }
        std::wstring line = text.substr(at, end - at);
        at = end + 1;
        while (!line.empty() && (line.back() == L'\r' || line.back() == L' ')) {
            line.pop_back();
        }
        const size_t start = line.find_first_not_of(L" \t");
        if (start == std::wstring::npos || line[start] == L'#' ||
            line[start] == L'.' || line[start] == L'[') {
            continue;
        }
        const size_t colon = line.find(L':', start);
        if (colon == std::wstring::npos || colon == start) {
            continue;
        }
        const std::wstring keys = TidyKeys(line.substr(start, colon - start));
        std::wstring what = line.substr(colon + 1);
        // Without a comment at its end.
        const size_t comment = what.find(L" #");
        if (comment != std::wstring::npos) {
            what.erase(comment);
        }
        const size_t whatStart = what.find_first_not_of(L" \t");
        what = whatStart == std::wstring::npos ? L"" : what.substr(whatStart);
        // Keys are short words joined by "+".
        if (keys.empty() || keys.size() > 60 || what.empty() ||
            keys.find(L' ') != std::wstring::npos) {
            continue;
        }
        found.push_back({id++, keys, what.substr(0, 200)});
    }
    return found;
}

// A file dialog's owner: a hidden window of the dialog's thread, not the
// panel, since the panel's thread mustn't be sent messages by the dialog (it
// may be waiting for this thread). Topmost, so the dialog shows over the
// island's panels; placed in the middle of the panel's screen.
HWND CreateFileDialogOwner(HWND panel) {
    MONITORINFO monitor{sizeof(monitor)};
    GetMonitorInfo(MonitorFromWindow(panel, MONITOR_DEFAULTTOPRIMARY),
                   &monitor);
    return CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"STATIC", nullptr,
                          WS_POPUP,
                          (monitor.rcWork.left + monitor.rcWork.right) / 2,
                          (monitor.rcWork.top + monitor.rcWork.bottom) / 2, 0,
                          0, nullptr, nullptr, nullptr, nullptr);
}

// A text file's text, up to `limit` bytes: UTF-8 (with or without its mark),
// or UTF-16. False if it can't be read.
bool ReadTextFile(PCWSTR path, size_t limit, std::wstring* text) {
    HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                             OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    std::string bytes(limit, '\0');
    DWORD read = 0;
    const bool ok =
        ReadFile(file, bytes.data(), (DWORD)bytes.size(), &read, nullptr);
    CloseHandle(file);
    if (!ok) {
        return false;
    }
    bytes.resize(read);
    text->clear();
    if (read >= 2 && (BYTE)bytes[0] == 0xFF && (BYTE)bytes[1] == 0xFE) {
        text->assign((const WCHAR*)(bytes.data() + 2), (read - 2) / 2);
        return true;
    }
    if (read >= 3 && (BYTE)bytes[0] == 0xEF) {
        bytes.erase(0, 3);
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, bytes.data(),
                                           (int)bytes.size(), nullptr, 0);
    text->resize(std::max(length, 0));
    MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(),
                        text->data(), length);
    return true;
}

// Closes a file dialog left open (its thread's), as if cancelled.
BOOL CALLBACK CloseFileDialog(HWND hwnd, LPARAM) {
    WCHAR className[16];
    if (GetClassName(hwnd, className, ARRAYSIZE(className)) &&
        wcscmp(className, L"#32770") == 0) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    return TRUE;
}

// Waits for a file dialog's thread to end, closing its dialog (again until
// it ends, in case it was only about to show). One still busy after a few
// seconds (a folder read on a sleeping network drive) is let go: each of
// these threads owns its job, and frees it when the window it posts to is
// gone.
void EndFileDialogThread(HANDLE* thread) {
    if (!*thread) {
        return;
    }
    for (int tries = 0;
         tries < 50 && WaitForSingleObject(*thread, 100) == WAIT_TIMEOUT;
         tries++) {
        EnumThreadWindows(GetThreadId(*thread), CloseFileDialog, 0);
    }
    CloseHandle(*thread);
    *thread = nullptr;
}

