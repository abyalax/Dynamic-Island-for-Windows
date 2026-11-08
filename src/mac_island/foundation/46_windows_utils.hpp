// The Windows updates waiting (their titles), or false when Windows Update
// couldn't be asked. Offline: what Windows' own last scan found, without
// contacting a server.
bool ListWindowsUpdates(std::vector<std::wstring>* titles) {
    constexpr GUID kCLSID_UpdateSession = {
        0x4cb43d7f, 0x7eee, 0x4906, {0x86, 0x98, 0x60, 0xda, 0x1c, 0x38, 0xf2, 0xfe}};
    constexpr GUID kIID_IUpdateSession = {
        0x816858a4, 0x260d, 0x4260, {0x93, 0x3a, 0x25, 0x85, 0xf1, 0xab, 0xc7, 0x6b}};
    winrt::com_ptr<IUpdateSession> session;
    winrt::com_ptr<IUpdateSearcher> searcher;
    winrt::com_ptr<ISearchResult> result;
    winrt::com_ptr<IUpdateCollection> updates;
    BSTR criteria = SysAllocString(L"IsInstalled=0 and IsHidden=0");
    bool ok = false;
    if (SUCCEEDED(CoCreateInstance(kCLSID_UpdateSession, nullptr,
                                   CLSCTX_INPROC_SERVER, kIID_IUpdateSession,
                                   session.put_void())) &&
        SUCCEEDED(session->CreateUpdateSearcher(searcher.put())) &&
        SUCCEEDED(searcher->put_Online(VARIANT_FALSE)) &&
        SUCCEEDED(searcher->Search(criteria, result.put())) &&
        SUCCEEDED(result->get_Updates(updates.put()))) {
        ok = true;
        LONG count = 0;
        updates->get_Count(&count);
        for (LONG i = 0; i < count && i < 50; i++) {
            winrt::com_ptr<IUpdate> update;
            BSTR title = nullptr;
            if (SUCCEEDED(updates->get_Item(i, update.put())) &&
                SUCCEEDED(update->get_Title(&title)) && title) {
                titles->push_back(title);
            }
            SysFreeString(title);
        }
    }
    SysFreeString(criteria);
    return ok;
}

// The apps sent to the terminal, each with the version it was to get (see
// AppUpdate::state 5). Under g_updatesLock.
std::vector<std::pair<std::wstring, std::wstring>> g_updatesRun;

// Opens winget in a terminal for these apps, one after the other (it stays
// open to show how it went, and asks there for any terms to accept). With no
// apps, the list itself (to see why it couldn't be read).
void UpdateInTerminal(const std::vector<std::wstring>& ids) {
    std::wstring commands;
    std::vector<std::wstring> run;
    for (const auto& id : ids) {
        if (!IsPackageId(id)) {
            continue;
        }
        if (!commands.empty()) {
            commands += L" & ";
        }
        commands +=
            L"winget upgrade --id \"" + id + L"\" --exact --source winget";
        run.push_back(id);
    }
    if (commands.empty()) {
        if (!ids.empty()) {
            return;
        }
        commands = L"winget upgrade --source winget";
    }
    ShellExecute(nullptr, L"open", L"cmd.exe", (L"/k " + commands).c_str(),
                 nullptr, SW_SHOWNORMAL);
    AcquireSRWLockExclusive(&g_updatesLock);
    for (auto& app : g_updates.apps) {
        if (std::find(run.begin(), run.end(), app.id) != run.end()) {
            app.state = 4;
            app.ticked = false;
            std::erase_if(g_updatesRun, [&](const auto& entry) {
                return entry.first == app.id;
            });
            g_updatesRun.push_back({app.id, app.available});
        }
    }
    ReleaseSRWLockExclusive(&g_updatesLock);
}

// Looking for updates, on a thread. `stop` is the thread's own copy of the
// island's event (it closes it).
struct UpdatesJob {
    HWND notify;
    HANDLE stop;
};

DWORD WINAPI UpdatesThreadProc(LPVOID parameter) {
    std::unique_ptr<UpdatesJob> job((UpdatesJob*)parameter);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    std::wstring output;
    DWORD code = 0;
    // Only the community source: it has no terms to accept (the Store's
    // has; nothing is accepted here).
    const bool listed =
        RunWinget(L"upgrade --source winget --disable-interactivity",
                  2 * 60 * 1000, job->stop, &output, &code);
    std::vector<AppUpdate> apps =
        listed ? ParseWingetUpgrades(output) : std::vector<AppUpdate>();
    std::vector<std::wstring> windows;
    if (WaitForSingleObject(job->stop, 0) != WAIT_OBJECT_0) {
        ListWindowsUpdates(&windows);
    }
    // Stopped (the island restarting): nothing was checked, so the lists
    // kept stay, and the next island checks again on its own schedule
    // (never "all up to date" for 6 hours from a cut-short check).
    if (WaitForSingleObject(job->stop, 0) == WAIT_OBJECT_0) {
        AcquireSRWLockExclusive(&g_updatesLock);
        g_updates.checking = false;
        ReleaseSRWLockExclusive(&g_updatesLock);
        if (SUCCEEDED(com)) {
            CoUninitialize();
        }
        CloseHandle(job->stop);
        return 0;
    }
    AcquireSRWLockExclusive(&g_updatesLock);
    // The ones ticked stay ticked; one updated in the terminal still asking
    // for the same version finishes when it closes.
    for (auto& app : apps) {
        for (const auto& before : g_updates.apps) {
            if (before.id == app.id) {
                app.ticked = before.ticked;
            }
        }
        for (const auto& [id, version] : g_updatesRun) {
            if (id == app.id && version == app.available) {
                app.state = 5;
            }
        }
    }
    g_updates.apps = std::move(apps);
    g_updates.windows = std::move(windows);
    g_updates.wingetMissing = !listed && output.empty();
    g_updates.wingetFailed =
        listed && code != 0 && g_updates.apps.empty();
    g_updates.checking = false;
    g_updates.checked = true;
    g_updates.checkedAt = NowSeconds();
    ReleaseSRWLockExclusive(&g_updatesLock);
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    PostMessage(job->notify, WM_APP_UPDATES, 0, 0);
    CloseHandle(job->stop);
    return 0;
}

constexpr float kUpdatesWidth = 440;

class UpdatesPanel : public EditingPanel {
   public:
    // Asks the island to look for updates again.
    std::function<void()> check;
    // Tells the island the list changed (an app ignored or shown again).
    std::function<void()> changed;
    // Tells it how often they're looked for changed (see g_updatesEvery).
    std::function<void()> everyChanged;
    void Refresh() {
        if (IsOpen()) {
            UpdatesData data = GetUpdates();
            // Another list (not only a box ticked): it comes in smoothly.
            auto signature = [](const UpdatesData& updates) {
                std::wstring text = std::to_wstring(updates.checking) +
                                    std::to_wstring(updates.windows.size());
                for (const auto& app : updates.apps) {
                    text += app.id + std::to_wstring(app.ignored) +
                            std::to_wstring(app.state);
                }
                return text;
            };
            if (signature(data) != signature(m_data)) {
                PulseContents();
            }
            m_data = std::move(data);
            Layout();
            Render();
        }
    }

   protected:
    void Prepare(float scale) override {
        PrepareFormats(scale);
        m_data = GetUpdates();
        Layout();
    }
    void OnOpen() override {
        m_data = GetUpdates();
        m_hover = {};
        m_scroll.Reset();
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
    bool OnMouseUp(POINT pt) override {
        const Hit hit = HitTest(pt);
        switch (hit.part) {
            case Part::Check:
                if (check && !m_data.checking) {
                    check();
                }
                return true;
            case Part::All: {
                // The ones ticked, or all (not the ignored ones).
                std::vector<std::wstring> ids;
                for (const auto& app : m_data.apps) {
                    if (app.ticked && !app.ignored) {
                        ids.push_back(app.id);
                    }
                }
                if (ids.empty()) {
                    for (const auto& app : m_data.apps) {
                        if (!app.ignored) {
                            ids.push_back(app.id);
                        }
                    }
                }
                UpdateInTerminal(ids);
                Refresh();
                return true;
            }
            case Part::App:
                if (hit.index >= 0 && hit.index < (int)m_data.apps.size()) {
                    UpdateInTerminal({m_data.apps[hit.index].id});
                    Refresh();
                }
                return true;
            case Part::Tick:
                if (hit.index >= 0 && hit.index < (int)m_data.apps.size()) {
                    AcquireSRWLockExclusive(&g_updatesLock);
                    for (auto& app : g_updates.apps) {
                        if (app.id == m_data.apps[hit.index].id) {
                            app.ticked = !app.ticked;
                        }
                    }
                    ReleaseSRWLockExclusive(&g_updatesLock);
                    Refresh();
                }
                return true;
            case Part::Ignore:
                if (hit.index >= 0 && hit.index < (int)m_data.apps.size()) {
                    const AppUpdate& app = m_data.apps[hit.index];
                    SetUpdateIgnored(app.id, !app.ignored);
                    m_hover = {};
                    Refresh();
                    // The count on the pill.
                    if (changed) {
                        changed();
                    }
                }
                return true;
            case Part::WingetTerminal:
                UpdateInTerminal({});
                return true;
            case Part::Windows:
                Close();
                ShellExecute(nullptr, L"open", L"ms-settings:windowsupdate",
                             nullptr, nullptr, SW_SHOWNORMAL);
                return false;
            case Part::Every:
                if (hit.index >= 0 && hit.index != g_updatesEvery) {
                    g_updatesEvery = hit.index;
                    Wh_SetIntValue(L"updatesCheckEvery", g_updatesEvery);
                    if (everyChanged) {
                        everyChanged();
                    }
                    Render();
                }
                return true;
            default:
                return false;
        }
    }
    bool OnWheel(int delta) override {
        return m_scroll.Wheel(delta, std::round(60 * m_scale));
    }
    bool AnimateContents(double dt) override {
        // While looking, its "…" moves.
        const bool busy = m_data.checking;
        const bool scrolling = m_scroll.Animate(dt);
        m_idleMotion = busy && !scrolling;
        return busy || scrolling;
    }

   private:
    enum class Part {
        None,
        Check,
        All,
        App,
        Tick,
        Ignore,
        WingetTerminal,
        Windows,
        // How often they're looked for (the choice's index).
        Every
    };
    struct Hit {
        Part part = Part::None;
        int index = -1;
        bool operator==(const Hit& other) const {
            return part == other.part && index == other.index;
        }
    };

    size_t TickedCount() const {
        return (size_t)std::count_if(
            m_data.apps.begin(), m_data.apps.end(),
            [](const AppUpdate& app) { return app.ticked && !app.ignored; });
    }
    size_t CountedApps() const {
        return (size_t)std::count_if(
            m_data.apps.begin(), m_data.apps.end(),
            [](const AppUpdate& app) { return !app.ignored; });
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(kUpdatesWidth * s);
        const float padding = std::round(14 * s);
        const float gap = std::round(8 * s);
        float y = padding;
        m_header = {padding, y, width - padding, y + std::round(30 * s)};
        m_checkButton = {width - padding - std::round(32 * s), y,
                         width - padding, y + std::round(30 * s)};
        y += std::round(30 * s) + gap;
        // The list: apps (with "Update all"), then Windows'.
        m_appRows.clear();
        const float row = std::round(46 * s);
        const float section = std::round(30 * s);
        float listY = 0;
        m_appsTitle = {padding, listY, width - padding, listY + section};
        m_all = {width - padding - std::round(150 * s),
                 listY + std::round(3 * s), width - padding,
                 listY + section - std::round(3 * s)};
        listY += section;
        m_ignoredTitle = {};
        for (size_t i = 0; i < m_data.apps.size(); i++) {
            // The ignored ones under their own title.
            if (m_data.apps[i].ignored && (i == 0 || !m_data.apps[i - 1].ignored)) {
                m_ignoredTitle = {padding, listY, width - padding,
                                  listY + section};
                listY += section;
            }
            m_appRows.push_back({padding, listY, width - padding, listY + row});
            listY += row;
        }
        m_wingetButton = {};
        if (m_data.apps.empty()) {
            listY += std::round(28 * s);
            if (m_data.wingetFailed) {
                m_wingetButton = {padding, listY, width - padding,
                                  listY + std::round(32 * s)};
                listY = m_wingetButton.bottom;
            }
        }
        listY += gap;
        m_windowsTitle = {padding, listY, width - padding, listY + section};
        listY += section;
        m_windowsRows = std::min<size_t>(m_data.windows.size(), 8);
        m_windowsTop = listY;
        listY += std::round(22 * s) * std::max<size_t>(m_windowsRows, 1);
        m_windowsButton = {padding, listY + gap, width - padding,
                           listY + gap + std::round(32 * s)};
        listY = m_windowsButton.bottom;
        // Under the list: "Check every" and its choices.
        const float label = std::round(24 * s);
        const float chip = std::round(28 * s);
        const float footer = gap + label + chip;
        const float room = std::max(std::round(120 * s),
                                    MaxHeight() - y - padding - footer);
        const float viewport = std::min(listY, room);
        m_listViewport = {0, y, width, y + viewport};
        m_scroll.SetSizes(viewport, listY);
        y += viewport + gap;
        m_everyLabel = {padding, y, width - padding, y + label};
        y += label;
        const size_t choices = std::size(kUpdatesEveryHours);
        const float chipWidth =
            std::floor((width - padding * 2 - gap * (choices - 1)) / choices);
        m_everyChips.clear();
        for (size_t i = 0; i < choices; i++) {
            const float left = padding + i * (chipWidth + gap);
            m_everyChips.push_back({left, y, left + chipWidth, y + chip});
        }
        y += chip + padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    D2D1_RECT_F AppButton(const D2D1_RECT_F& row) const {
        const float s = m_scale;
        // (Wide enough for "Close it to finish" whole.)
        return {row.right - std::round(136 * s), row.top + std::round(8 * s),
                row.right, row.bottom - std::round(8 * s)};
    }
    // "Ignore" (or "Show again"), left of it.
    D2D1_RECT_F IgnoreButton(const D2D1_RECT_F& row) const {
        const float s = m_scale;
        const D2D1_RECT_F button = AppButton(row);
        const float size = button.bottom - button.top;
        return {button.left - std::round(6 * s) - size, button.top,
                button.left - std::round(6 * s), button.bottom};
    }
    // The box to tick, at the row's start.
    D2D1_RECT_F TickBox(const D2D1_RECT_F& row) const {
        const float s = m_scale;
        const float box = std::round(18 * s);
        const float middle = (row.top + row.bottom) / 2;
        return {row.left, middle - box / 2, row.left + box, middle + box / 2};
    }

    Hit HitTest(POINT pt) const {
        if (PointInRect(pt, m_checkButton)) {
            return {Part::Check};
        }
        for (size_t i = 0; i < m_everyChips.size(); i++) {
            if (PointInRect(pt, m_everyChips[i])) {
                return {Part::Every, (int)i};
            }
        }
        POINT inList;
        if (!ListPoint(pt, &inList)) {
            return {};
        }
        if (CountedApps() > 1 && PointInRect(inList, m_all)) {
            return {Part::All};
        }
        for (size_t i = 0; i < m_appRows.size() && i < m_data.apps.size();
             i++) {
            if (PointInRect(inList, AppButton(m_appRows[i]))) {
                return {Part::App, (int)i};
            }
            if (PointInRect(inList, IgnoreButton(m_appRows[i]))) {
                return {Part::Ignore, (int)i};
            }
            // The box, or the app's name, ticks it.
            if (CountedApps() > 1 && !m_data.apps[i].ignored &&
                PointInRect(inList, m_appRows[i])) {
                return {Part::Tick, (int)i};
            }
        }
        if (PointInRect(inList, m_wingetButton)) {
            return {Part::WingetTerminal};
        }
        if (PointInRect(inList, m_windowsButton)) {
            return {Part::Windows};
        }
        return {};
    }

    UpdatesData m_data;
    Hit m_hover;
    D2D1_RECT_F m_header{}, m_checkButton{}, m_appsTitle{}, m_all{},
        m_windowsTitle{}, m_windowsButton{}, m_wingetButton{},
        m_ignoredTitle{}, m_everyLabel{};
    std::vector<D2D1_RECT_F> m_appRows;
    std::vector<D2D1_RECT_F> m_everyChips;
    size_t m_windowsRows = 0;
    float m_windowsTop = 0;
};

void UpdatesPanel::Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
    if (!Ready()) {
        return;
    }
    const float s = m_scale;
    // "…" moving while busy.
    const std::wstring dots(1 + (int)(NowSeconds() * 3) % 3, L'.');
    const WCHAR icon = 0xE895;
    brush->SetColor(Accent(1));
    DrawGlyph(target, &icon, 1, m_bigIconFormat.get(),
              {m_header.left, m_header.top,
               m_header.left + std::round(22 * s), m_header.bottom},
              brush);
    std::wstring title = Tr(L"Updates", L"Atualizações");
    if (m_data.checking) {
        title += std::wstring(L"  ·  ") + Tr(L"checking", L"verificando") +
                 dots;
    }
    brush->SetColor(Fg(0.95f));
    target->DrawText(title.c_str(), (UINT32)title.size(), m_titleFormat.get(),
                     {m_header.left + std::round(28 * s), m_header.top,
                      m_checkButton.left, m_header.bottom},
                     brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    DrawIconButton(target, brush, m_checkButton, 0xE72C,
                   m_hover.part == Part::Check, m_data.checking ? 0.3f : 0.7f);

    BeginScrollArea(target, m_listViewport, m_scroll);
    // Apps: updated in a terminal.
    std::wstring apps = Tr(L"Apps (winget, in a terminal)",
                           L"Apps (winget, no terminal)");
    const size_t counted = CountedApps();
    if (counted) {
        apps += L"  ·  " + std::to_wstring(counted);
    }
    brush->SetColor(Fg(0.6f));
    target->DrawText(apps.c_str(), (UINT32)apps.size(), m_smallFormat.get(),
                     {m_appsTitle.left, m_appsTitle.top,
                      counted > 1 ? m_all.left : m_appsTitle.right,
                      m_appsTitle.bottom},
                     brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    if (m_ignoredTitle.bottom > m_ignoredTitle.top) {
        PCWSTR ignored = Tr(L"Ignored (not counted)",
                            L"Ignorados (não contam)");
        brush->SetColor(Fg(0.5f));
        target->DrawText(ignored, (UINT32)wcslen(ignored), m_smallFormat.get(),
                         m_ignoredTitle, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    if (counted > 1) {
        const size_t ticked = TickedCount();
        const std::wstring all =
            ticked ? Tr(L"Update ticked", L"Atualizar marcados") +
                         std::wstring(L" (") + std::to_wstring(ticked) + L")"
                   : Tr(L"Update all", L"Atualizar tudo");
        DrawButton(target, brush, m_all, all.c_str(), true,
                   m_hover.part == Part::All);
    }
    if (m_data.apps.empty()) {
        PCWSTR none = !m_data.checked ? Tr(L"Not checked yet",
                                           L"Ainda não verificado")
                      : m_data.wingetMissing
                          ? Tr(L"winget isn't installed",
                               L"O winget não está instalado")
                      : m_data.wingetFailed
                          ? Tr(L"winget couldn't list them",
                               L"O winget não conseguiu listar")
                          : Tr(L"All up to date", L"Tudo atualizado");
        brush->SetColor(Fg(0.45f));
        target->DrawText(none, (UINT32)wcslen(none), m_textFormat.get(),
                         {m_appsTitle.left, m_appsTitle.bottom,
                          m_appsTitle.right,
                          m_appsTitle.bottom + std::round(28 * s)},
                         brush);
        if (m_data.wingetFailed) {
            DrawButton(target, brush, m_wingetButton,
                       Tr(L"See it in a terminal", L"Ver no terminal"), false,
                       m_hover.part == Part::WingetTerminal);
        }
    }
    for (size_t i = 0; i < m_appRows.size() && i < m_data.apps.size(); i++) {
        const AppUpdate& app = m_data.apps[i];
        const D2D1_RECT_F& row = m_appRows[i];
        const D2D1_RECT_F button = AppButton(row);
        const D2D1_RECT_F ignore = IgnoreButton(row);
        // Ignored: dimmed, without a box.
        const float dim = app.ignored ? 0.55f : 1.0f;
        float textLeft = row.left;
        if (counted > 1 && !app.ignored) {
            // Its box.
            const D2D1_RECT_F square = TickBox(row);
            const float corner = std::round(4 * s);
            const float lit = Hover(
                square, m_hover.part == Part::Tick && m_hover.index == (int)i);
            if (app.ticked) {
                brush->SetColor(Accent(1));
                target->FillRoundedRectangle({square, corner, corner}, brush);
                brush->SetColor(AccentInk(1));
                DrawGlyph(target, &kGlyphCheck, 1, m_iconFormat.get(), square,
                          brush);
            } else {
                brush->SetColor(Fg(0.4f + 0.45f * lit));
                target->DrawRoundedRectangle({square, corner, corner}, brush,
                                             std::max(1.0f, 1.25f * s));
            }
            textLeft = square.right + std::round(10 * s);
        }
        brush->SetColor(Fg(0.95f * dim));
        target->DrawText(app.name.c_str(), (UINT32)app.name.size(),
                         m_boldFormat.get(),
                         {textLeft, row.top + std::round(4 * s),
                          ignore.left - std::round(8 * s),
                          row.top + std::round(24 * s)},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        const std::wstring versions = app.version + L"  →  " + app.available;
        brush->SetColor(Fg(0.5f * dim));
        target->DrawText(versions.c_str(), (UINT32)versions.size(),
                         m_smallFormat.get(),
                         {textLeft, row.top + std::round(24 * s),
                          ignore.left - std::round(8 * s),
                          row.bottom - std::round(4 * s)},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        // Ignore it (an eye crossed out), or show it again (an eye).
        DrawIconButton(target, brush, ignore, app.ignored ? 0xE7B3 : 0xED1A,
                       m_hover.part == Part::Ignore && m_hover.index == (int)i,
                       0.7f);
        const bool hover = m_hover.part == Part::App && m_hover.index == (int)i;
        // Opened in a terminal, or updated there but finishing when the app
        // closes: can be opened again.
        DrawButton(target, brush, button,
                   app.state == 4   ? Tr(L"In the terminal", L"No terminal")
                   : app.state == 5 ? Tr(L"Close it to finish",
                                         L"Feche para concluir")
                                    : Tr(L"Update", L"Atualizar"),
                   false, hover, app.state >= 4 ? 0.6f : 1.0f);
        if (i + 1 < m_appRows.size()) {
            brush->SetColor(Fg(0.07f));
            target->FillRectangle({row.left, row.bottom - 0.5f, row.right,
                                   row.bottom + 0.5f},
                                  brush);
        }
    }
    // Windows'.
    std::wstring windows = L"Windows Update";
    if (!m_data.windows.empty()) {
        windows += L"  ·  " + std::to_wstring(m_data.windows.size());
    }
    brush->SetColor(Fg(0.6f));
    target->DrawText(windows.c_str(), (UINT32)windows.size(),
                     m_smallFormat.get(), m_windowsTitle, brush);
    const float line = std::round(22 * s);
    if (m_data.windows.empty()) {
        PCWSTR none = m_data.checked ? Tr(L"Nothing waiting", L"Nada pendente")
                                     : Tr(L"Not checked yet",
                                          L"Ainda não verificado");
        brush->SetColor(Fg(0.45f));
        target->DrawText(none, (UINT32)wcslen(none), m_textFormat.get(),
                         {m_windowsTitle.left, m_windowsTop,
                          m_windowsTitle.right, m_windowsTop + line},
                         brush);
    }
    for (size_t i = 0; i < m_windowsRows && i < m_data.windows.size(); i++) {
        const std::wstring text = L"•  " + m_data.windows[i];
        brush->SetColor(Fg(0.85f));
        target->DrawText(text.c_str(), (UINT32)text.size(), m_smallFormat.get(),
                         {m_windowsTitle.left, m_windowsTop + line * i,
                          m_windowsTitle.right, m_windowsTop + line * (i + 1)},
                         brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    DrawButton(target, brush, m_windowsButton,
               Tr(L"Open Windows Update", L"Abrir o Windows Update"), false,
               m_hover.part == Part::Windows);
    EndScrollArea(target);
    DrawScrollBar(target, brush, m_listViewport, m_scroll);

    // "Check every": 1, 3, 6, 12 or 24 hours (the one in use outlined).
    PCWSTR every = Tr(L"Check every", L"Verificar a cada");
    brush->SetColor(Fg(0.6f));
    target->DrawText(every, (UINT32)wcslen(every), m_smallFormat.get(),
                     m_everyLabel, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    for (size_t i = 0; i < m_everyChips.size(); i++) {
        const D2D1_RECT_F& chip = m_everyChips[i];
        const bool chosen = g_updatesEvery == (int)i;
        const float radius = (chip.bottom - chip.top) / 2;
        brush->SetColor(Fg(
            chosen ? 0.18f
                   : 0.08f + 0.08f * Hover(chip, m_hover.part == Part::Every &&
                                                     m_hover.index == (int)i)));
        GlassFill(target, {chip, radius, radius}, brush);
        if (chosen) {
            brush->SetColor(Fg(0.7f));
            target->DrawRoundedRectangle(
                {{chip.left + 0.75f, chip.top + 0.75f, chip.right - 0.75f,
                  chip.bottom - 0.75f},
                 radius,
                 radius},
                brush, 1.5f);
        }
        const std::wstring hours =
            std::to_wstring(kUpdatesEveryHours[i]) + L" h";
        brush->SetColor(Fg(0.9f));
        target->DrawText(hours.c_str(), (UINT32)hours.size(),
                         m_smallFormat.get(), chip, brush,
                         D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    m_smallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
}

class StatusEvents;

// How opaque the bar's and the strip's colors are over the frosted glass
// (times the chosen opacity), so the blur shows through.
constexpr float kFrostedFill = 0.45f;
// The see-through bar's (islandBarClear): only a light tint, the blur
// showing.
constexpr float kClearFrostedTint = 0.2f;

// Windows' blur behind a window, on or off: the accent policy of
// SetWindowCompositionAttribute (not documented, but what the shell itself
// uses for it; looked up at run time, and nothing happens without it). It
// fills the window's whole rectangle whatever its shape (a window region
// doesn't cut it), so it's only used on a rectangle of its own: the strip
// under the bar or under the pill keeping its room.
void SetWindowBlur(HWND hwnd, bool on) {
    struct AccentPolicy {
        int state;
        int flags;
        DWORD color;
        int animation;
    };
    struct CompositionData {
        int attribute;
        void* data;
        SIZE_T size;
    };
    using SetAttribute = BOOL(WINAPI*)(HWND, CompositionData*);
    static const auto setAttribute = reinterpret_cast<SetAttribute>(
        GetProcAddress(GetModuleHandle(L"user32.dll"),
                       "SetWindowCompositionAttribute"));
    if (!setAttribute) {
        return;
    }
    // ACCENT_ENABLE_BLURBEHIND (3) or ACCENT_DISABLED (0), as the
    // WCA_ACCENT_POLICY (19) attribute.
    AccentPolicy policy{on ? 3 : 0, 0, 0, 0};
    CompositionData data{19, &policy, sizeof(policy)};
    setAttribute(hwnd, &data);
}

// One color over a whole layered window, at `rect`.
void PaintFlatLayeredWindow(HWND hwnd, const RECT& rect, D2D1_COLOR_F color) {
    SIZE size{rect.right - rect.left, rect.bottom - rect.top};
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) {
        return;
    }
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = size.cx;
    info.bmiHeader.biHeight = -size.cy;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    if (HBITMAP bitmap =
            CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0)) {
        // Premultiplied.
        const float alpha = std::clamp(color.a, 0.0f, 1.0f);
        auto channel = [alpha](float value) {
            return (DWORD)std::lround(std::clamp(value, 0.0f, 1.0f) * alpha *
                                      255);
        };
        const DWORD pixel = (DWORD)std::lround(alpha * 255) << 24 |
                            channel(color.r) << 16 | channel(color.g) << 8 |
                            channel(color.b);
        std::fill_n((DWORD*)bits, (size_t)size.cx * size.cy, pixel);
        const HGDIOBJ before = SelectObject(dc, bitmap);
        POINT source{0, 0};
        POINT position{rect.left, rect.top};
        BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        UpdateLayeredWindow(hwnd, nullptr, &position, &size, dc, &source, 0,
                            &blend, ULW_ALPHA);
        SelectObject(dc, before);
        DeleteObject(bitmap);
    }
    DeleteDC(dc);
}

////////////////////////////////////////////////////////////////////////////////
// Apps using the microphone, the camera or the location (an option), watched
// on a thread of the main island, by events only; nothing is kept or sent:
// - the microphone: the apps recording (Core Audio's sessions on every
//   microphone, active ones);
// - the camera: the apps streaming from it (Media Foundation's sensor
//   activity monitor, made for privacy indicators);
// - the location: Windows' record of the apps using it (LastUsedTimeStop 0
//   under CapabilityAccessManager's ConsentStore in the registry, watched
//   with RegNotifyChangeKeyValue). Newer Windows builds keep it in a
//   database of their own and may not write it there any more.

struct PrivacyUse {
    // The apps using each, joined ("Discord, Chrome"), empty when none.
    std::wstring microphone;
    std::wstring camera;
    std::wstring location;
    bool Any() const {
        return !microphone.empty() || !camera.empty() || !location.empty();
    }
    bool operator==(const PrivacyUse&) const = default;
};
SRWLOCK g_privacyLock = SRWLOCK_INIT;
PrivacyUse g_privacy;
HANDLE g_privacyThread;
HANDLE g_privacyStop;
// Tells the island the apps using them changed (see g_privacy).
constexpr UINT WM_APP_PRIVACY = WM_APP + 75;
// When an app starts using the camera, rings spread out of its icon for this
// long, one every kCameraRingSeconds.
constexpr double kCameraBlinkSeconds = 4.8;
constexpr double kCameraRingSeconds = 1.6;

constexpr WCHAR kConsentStoreKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\"
    L"ConsentStore";

// Whether an app's key says it's using it now.
bool ConsentInUse(HKEY parent, PCWSTR name) {
    ULONGLONG start = 0;
    ULONGLONG stop = 1;
    DWORD size = sizeof(start);
    RegGetValue(parent, name, L"LastUsedTimeStart", RRF_RT_REG_QWORD, nullptr,
                &start, &size);
    size = sizeof(stop);
    RegGetValue(parent, name, L"LastUsedTimeStop", RRF_RT_REG_QWORD, nullptr,
                &stop, &size);
    return start != 0 && stop == 0;
}

// The apps using one (the store's "location"): Store apps by their
// package's name, programs by theirs (only while they're running: one that
// crashed while using it never says it stopped).
std::wstring AppsUsing(HKEY store,
                       PCWSTR capability,
                       const std::vector<std::wstring>& running) {
    HKEY key;
    if (RegOpenKeyEx(store, capability, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return {};
    }
    std::vector<std::wstring> names;
    auto add = [&](std::wstring name) {
        if (!name.empty() &&
            std::find(names.begin(), names.end(), name) == names.end()) {
            names.push_back(std::move(name));
        }
    };
    WCHAR sub[512];
    for (DWORD i = 0;; i++) {
        DWORD length = ARRAYSIZE(sub);
        if (RegEnumKeyEx(key, i, sub, &length, nullptr, nullptr, nullptr,
                         nullptr) != ERROR_SUCCESS) {
            break;
        }
        if (_wcsicmp(sub, L"NonPackaged") != 0) {
            // "Company.App_hash": the readable part.
            if (ConsentInUse(key, sub)) {
                std::wstring name = sub;
                name = name.substr(0, name.find(L'_'));
                if (size_t dot = name.find_last_of(L'.');
                    dot != std::wstring::npos && dot + 1 < name.size()) {
                    name = name.substr(dot + 1);
                }
                add(std::move(name));
            }
            continue;
        }
        HKEY programs;
        if (RegOpenKeyEx(key, sub, 0, KEY_READ, &programs) != ERROR_SUCCESS) {
            continue;
        }
        WCHAR program[MAX_PATH * 2];
        for (DWORD j = 0;; j++) {
            DWORD programLength = ARRAYSIZE(program);
            if (RegEnumKeyEx(programs, j, program, &programLength, nullptr,
                             nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
                break;
            }
            if (!ConsentInUse(programs, program)) {
                continue;
            }
            // Its path, with "#" for "\".
            std::wstring path = program;
            std::replace(path.begin(), path.end(), L'#', L'\\');
            const std::wstring file = path.substr(path.find_last_of(L'\\') + 1);
            if (std::none_of(running.begin(), running.end(),
                             [&](const std::wstring& exe) {
                                 return _wcsicmp(exe.c_str(), file.c_str()) ==
                                        0;
                             })) {
                continue;
            }
            add(GetProgramName(path.c_str()));
        }
        RegCloseKey(programs);
    }
    RegCloseKey(key);
    std::wstring joined;
    for (const auto& name : names) {
        joined += (joined.empty() ? L"" : L", ") + name;
    }
    return joined;
}

// The programs running (their files' names).
std::vector<std::wstring> RunningPrograms() {
    std::vector<std::wstring> running;
    if (HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 entry{sizeof(entry)};
        for (BOOL more = Process32First(snapshot, &entry); more;
             more = Process32Next(snapshot, &entry)) {
            running.push_back(entry.szExeFile);
        }
        CloseHandle(snapshot);
    }
    return running;
}

// A process's program file, or empty.
std::wstring ProcessImagePath(DWORD processId) {
    HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process) {
        return {};
    }
    std::wstring result;
    WCHAR path[MAX_PATH * 2];
    DWORD size = ARRAYSIZE(path);
    if (QueryFullProcessImageName(process, 0, path, &size)) {
        result.assign(path, size);
    }
    CloseHandle(process);
    return result;
}

// A process's program's name ("Discord"), or empty.
std::wstring ProcessProgramName(DWORD processId) {
    const std::wstring path = ProcessImagePath(processId);
    return path.empty() ? std::wstring{} : GetProgramName(path.c_str());
}

// Names joined ("Discord, Chrome"), each once.
std::wstring JoinNames(const std::vector<std::wstring>& names) {
    std::wstring joined;
    std::vector<std::wstring> seen;
    for (const auto& name : names) {
        if (name.empty() ||
            std::find(seen.begin(), seen.end(), name) != seen.end()) {
            continue;
        }
        seen.push_back(name);
        joined += (joined.empty() ? L"" : L", ") + name;
    }
    return joined;
}

