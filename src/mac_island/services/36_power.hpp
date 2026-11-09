#pragma once
enum class PowerAction { Sleep, Restart, ShutDown, Lock, SignOut, Count };

#ifndef EWX_HYBRID_SHUTDOWN
#define EWX_HYBRID_SHUTDOWN 0x00400000
#endif

// Lets this process shut Windows down or put it to sleep: a privilege every
// user has, off until a program asks for it.
bool EnableShutdownPrivilege() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        return false;
    }
    TOKEN_PRIVILEGES privileges{1};
    bool enabled = LookupPrivilegeValue(nullptr, SE_SHUTDOWN_NAME,
                                        &privileges.Privileges[0].Luid);
    if (enabled) {
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        enabled = AdjustTokenPrivileges(token, FALSE, &privileges, 0, nullptr,
                                        nullptr) &&
                  GetLastError() == ERROR_SUCCESS;
    }
    CloseHandle(token);
    return enabled;
}

// "Fast startup" (read only): with it on, the Start menu's "Shut down" saves
// the system's session to start faster, and so does this one.
bool IsFastStartupOn() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    RegGetValue(HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power",
                L"HiberbootEnabled", RRF_RT_REG_DWORD, nullptr, &value, &size);
    return value != 0;
}

void RunPowerAction(PowerAction action, HWND window) {
    constexpr DWORD kReason = SHTDN_REASON_MAJOR_OTHER |
                              SHTDN_REASON_MINOR_OTHER |
                              SHTDN_REASON_FLAG_PLANNED;
    switch (action) {
        case PowerAction::Lock:
            LockWorkStation();
            break;
        case PowerAction::Sleep: {
            // Returns once the PC wakes up (the island has nothing to do
            // meanwhile).
            EnableShutdownPrivilege();
            using SetSuspendState_t =
                BOOLEAN(WINAPI*)(BOOLEAN, BOOLEAN, BOOLEAN);
            bool slept = false;
            DWORD error = ERROR_PROC_NOT_FOUND;
            if (HMODULE powrprof = Powrprof()) {
                if (auto suspend = (SetSuspendState_t)GetProcAddress(
                        powrprof, "SetSuspendState")) {
                    slept = suspend(FALSE, FALSE, FALSE) != FALSE;
                    error = GetLastError();
                }
            }
            if (!slept) {
                // PCs with Modern Standby don't sleep that way: turning the
                // screen off is what puts them to sleep.
                IslandLog(L"SetSuspendState failed (%u), turning the screen off",
                       error);
                DefWindowProc(window, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
            }
            break;
        }
        case PowerAction::Restart:
        case PowerAction::ShutDown:
        case PowerAction::SignOut: {
            const bool signOut = action == PowerAction::SignOut;
            const UINT flags =
                action == PowerAction::Restart ? EWX_REBOOT
                : signOut ? EWX_LOGOFF
                          : EWX_SHUTDOWN | EWX_POWEROFF |
                                (IsFastStartupOn() ? EWX_HYBRID_SHUTDOWN : 0);
            if (!signOut) {
                EnableShutdownPrivilege();
            }
            if (!ExitWindowsEx(flags, signOut ? 0 : kReason)) {
                IslandLog(L"ExitWindowsEx failed: %u", GetLastError());
            }
            break;
        }
        case PowerAction::Count:
            break;
    }
}

// The menu's timer: the action runs once the menu has closed.
constexpr UINT_PTR kPowerTimerId = 40;
constexpr float kPowerWidth = 236;

class PowerPanel : public Panel {
   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_iconFormat = MakeFormat(IconFont(),
                                      DWRITE_FONT_WEIGHT_NORMAL, 15 * scale);
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           13 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_questionFormat = MakeFormat(
                L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_SEMI_BOLD,
                12.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_smallFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_buttonFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 12 * scale);
            if (m_smallFormat) {
                m_smallFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
                m_smallFormat->SetParagraphAlignment(
                    DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            }
        }
        Layout();
    }

    void OnOpen() override {
        m_hover = -1;
        m_confirm = -1;
        m_confirmShown = -1;
        m_confirmRoom = 0;
        m_confirmVelocity = 0;
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
        if (m_hover < 0) {
            return false;
        }
        m_hover = -1;
        return true;
    }

    bool OnMouseUp(POINT pt) override {
        const int hit = HitTest(pt);
        if (hit == kHitCancel) {
            Ask(-1);
            return true;
        }
        if (hit == kHitConfirm && m_confirm >= 0) {
            Run((PowerAction)m_confirm);
            return false;
        }
        if (hit < 0) {
            return false;
        }
        if (NeedsConfirmation((PowerAction)hit)) {
            Ask(m_confirm == hit ? -1 : hit);
            return true;
        }
        Run((PowerAction)hit);
        return false;
    }

    // Escape closes the confirmation first; Enter confirms it.
    bool OnKey(WPARAM key) override {
        if (m_confirm < 0) {
            return false;
        }
        if (key == VK_ESCAPE) {
            Ask(-1);
            return true;
        }
        if (key == VK_RETURN) {
            Run((PowerAction)m_confirm);
        }
        return false;
    }

    bool OnTimer(UINT_PTR id) override {
        if (id >= kPowerTimerId &&
            id < kPowerTimerId + (UINT_PTR)PowerAction::Count) {
            RunPowerAction((PowerAction)(id - kPowerTimerId), m_hwnd);
        }
        return false;
    }

    // The confirmation opens and closes (its room, and the rows under it
    // making way); once closed, it's gone.
    bool AnimateContents(double dt) override {
        const bool moving =
            SpringTowards(m_confirmRoom, m_confirmVelocity,
                          m_confirm >= 0 ? 1 : 0, 0.36, dt, kBounceDamping);
        if (!moving && m_confirm < 0 && m_confirmShown >= 0) {
            m_confirmShown = -1;
            Layout();
        } else if (moving) {
            Layout();
        }
        return moving;
    }

   private:
    static constexpr int kHitCancel = 100;
    static constexpr int kHitConfirm = 101;
    static constexpr int kRows = (int)PowerAction::Count;

    static bool NeedsConfirmation(PowerAction action) {
        return action == PowerAction::Restart ||
               action == PowerAction::ShutDown ||
               action == PowerAction::SignOut;
    }

    static WCHAR Glyph(PowerAction action) {
        switch (action) {
            case PowerAction::Sleep:
                return 0xE708;
            case PowerAction::Restart:
                return 0xE777;
            case PowerAction::ShutDown:
                return kGlyphPower;
            case PowerAction::Lock:
                return 0xE72E;
            default:
                return 0xF3B1;
        }
    }

    static PCWSTR Name(PowerAction action) {
        switch (action) {
            case PowerAction::Sleep:
                return Tr(L"Sleep", L"Suspender");
            case PowerAction::Restart:
                return Tr(L"Restart", L"Reiniciar");
            case PowerAction::ShutDown:
                return Tr(L"Shut down", L"Desligar");
            case PowerAction::Lock:
                return Tr(L"Lock", L"Bloquear");
            default:
                return Tr(L"Sign out", L"Sair");
        }
    }

    static PCWSTR Question(PowerAction action) {
        switch (action) {
            case PowerAction::Restart:
                return Tr(L"Restart now?", L"Reiniciar agora?");
            case PowerAction::ShutDown:
                return Tr(L"Shut down now?", L"Desligar agora?");
            default:
                return Tr(L"Sign out now?", L"Sair agora?");
        }
    }

    static PCWSTR ConfirmName(PowerAction action) {
        switch (action) {
            case PowerAction::Restart:
                return Tr(L"Restart", L"Reiniciar");
            case PowerAction::ShutDown:
                return Tr(L"Shut down", L"Desligar");
            default:
                return Tr(L"Sign out", L"Sair");
        }
    }

    // Opens the confirmation under a row (-1 closes it, animated). Another
    // row's opens from nothing.
    void Ask(int row) {
        if (row >= 0 && row != m_confirmShown) {
            m_confirmShown = row;
            m_confirmRoom = 0;
            m_confirmVelocity = 0;
        }
        m_confirm = row;
        Layout();
        StartIslandAnimation();
    }

    void Run(PowerAction action) {
        Close();
        SetTimer(m_hwnd, kPowerTimerId + (UINT_PTR)action, 380, nullptr);
    }

    static PCWSTR Hint() {
        return Tr(L"Apps with unsaved work can stop it.",
                  L"Apps com trabalho não salvo podem impedir.");
    }

    // The question, the hint under it (on as many lines as it takes: never
    // cut), then the buttons.
    float HintHeight(float width) const {
        winrt::com_ptr<IDWriteTextLayout> layout;
        DWRITE_TEXT_METRICS metrics{};
        PCWSTR hint = Hint();
        if (!m_smallFormat || !m_dwrite ||
            FAILED(m_dwrite->CreateTextLayout(hint, (UINT32)wcslen(hint),
                                              m_smallFormat.get(), width,
                                              10000, layout.put())) ||
            FAILED(layout->GetMetrics(&metrics))) {
            return std::round(16 * m_scale);
        }
        return std::ceil(metrics.height);
    }

    float ConfirmHeight(float width) const {
        const float s = m_scale;
        const float inner = std::round(10 * s);
        return std::round(4 * s) + std::round(19 * s) +
               HintHeight(width - inner * 2) + std::round(8 * s) +
               std::round(28 * s) + inner;
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(kPowerWidth * s);
        const float padding = std::round(6 * s);
        const float row = std::round(36 * s);
        const float separator = std::round(9 * s);
        const float room = (float)std::clamp(m_confirmRoom, 0.0, 1.1);
        float y = padding;
        m_confirmRect = {};
        for (int i = 0; i < kRows; i++) {
            m_rows[i] = {padding, y, width - padding, y + row};
            y += row;
            if (i == m_confirmShown) {
                const float height = ConfirmHeight(width - padding * 2);
                m_confirmRect = {padding, y, width - padding,
                                 y + height * room};
                const float button = std::round(28 * s);
                const float gap = std::round(8 * s);
                const float inner = std::round(10 * s);
                const float buttonWidth =
                    (width - padding * 2 - inner * 2 - gap) / 2;
                const float top = y + height - inner - button;
                m_cancelRect = {padding + inner, top,
                                padding + inner + buttonWidth, top + button};
                m_confirmButton = {m_cancelRect.right + gap, top,
                                   width - padding - inner, top + button};
                y += height * std::max(room, 0.0f);
            }
            if ((PowerAction)i == PowerAction::ShutDown) {
                m_separatorY = y + separator / 2;
                y += separator;
            }
        }
        y += padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    int HitTest(POINT pt) const {
        if (m_confirm >= 0 && m_confirmRoom > 0.85) {
            if (PointInRect(pt, m_cancelRect)) {
                return kHitCancel;
            }
            if (PointInRect(pt, m_confirmButton)) {
                return kHitConfirm;
            }
            if (PointInRect(pt, m_confirmRect)) {
                return -1;
            }
        }
        for (int i = 0; i < kRows; i++) {
            if (PointInRect(pt, m_rows[i])) {
                return i;
            }
        }
        return -1;
    }

    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_questionFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallFormat;
    winrt::com_ptr<IDWriteTextFormat> m_buttonFormat;
    float m_preparedScale = 0;
    D2D1_RECT_F m_rows[kRows]{};
    float m_separatorY = 0;
    int m_hover = -1;
    // The row asking for confirmation (-1: none), the one whose confirmation
    // is shown (still while it closes), and how far it has opened (0 to 1, a
    // bit more while bouncing).
    int m_confirm = -1;
    int m_confirmShown = -1;
    double m_confirmRoom = 0;
    double m_confirmVelocity = 0;
    D2D1_RECT_F m_confirmRect{}, m_cancelRect{}, m_confirmButton{};
};

void PowerPanel::Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
    if (!m_iconFormat || !m_textFormat || !m_questionFormat || !m_smallFormat ||
        !m_buttonFormat) {
        return;
    }
    const float s = m_scale;
    const float radius = std::round(9 * s);
    const float icon = std::round(36 * s);
    for (int i = 0; i < kRows; i++) {
        const D2D1_RECT_F& row = m_rows[i];
        const bool asking = i == m_confirm;
        const float lit = Hover(HoverKey(6000, i), m_hover == i);
        if (lit > 0.01f || asking) {
            brush->SetColor(Fg(asking ? 0.12f : 0.09f * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        const WCHAR glyph = Glyph((PowerAction)i);
        brush->SetColor(Fg(0.95f));
        DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                  {row.left, row.top, row.left + icon, row.bottom},
                  brush);
        PCWSTR name = Name((PowerAction)i);
        target->DrawText(name, (UINT32)wcslen(name), m_textFormat.get(),
                         {row.left + icon, row.top,
                          row.right - std::round(8 * s), row.bottom},
                         brush);
    }
    brush->SetColor(Fg(0.10f));
    target->FillRectangle({m_rows[0].left + std::round(8 * s), m_separatorY,
                           m_rows[0].right - std::round(8 * s),
                           m_separatorY + std::max(1.0f, std::round(s))},
                          brush);

    // The confirmation, opening under its row: it's revealed as its room
    // grows, fading in.
    if (m_confirmShown < 0 || m_confirmRect.bottom - m_confirmRect.top < 0.5f) {
        return;
    }
    const PowerAction action = (PowerAction)m_confirmShown;
    const float fade =
        (float)std::clamp((m_confirmRoom - 0.25) / 0.6, 0.0, 1.0);
    target->PushAxisAlignedClip(m_confirmRect, D2D1_ANTIALIAS_MODE_ALIASED);
    winrt::com_ptr<ID2D1Layer> layer;
    target->CreateLayer(nullptr, layer.put());
    target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                            D2D1::IdentityMatrix(), fade),
                      layer.get());
    const float inner = std::round(10 * s);
    const float top = m_confirmRect.top + std::round(4 * s);
    PCWSTR question = Question(action);
    brush->SetColor(Fg(0.95f));
    target->DrawText(question, (UINT32)wcslen(question), m_questionFormat.get(),
                     {m_confirmRect.left + inner, top,
                      m_confirmRect.right - inner, top + std::round(18 * s)},
                     brush);
    PCWSTR hint = Hint();
    brush->SetColor(Fg(0.55f));
    target->DrawText(hint, (UINT32)wcslen(hint), m_smallFormat.get(),
                     {m_confirmRect.left + inner, top + std::round(19 * s),
                      m_confirmRect.right - inner, m_cancelRect.top},
                     brush);
    const float buttonRadius = (m_cancelRect.bottom - m_cancelRect.top) / 2;
    brush->SetColor(
        Fg(0.13f + 0.07f * Hover(HoverKey(6001, 0), m_hover == kHitCancel)));
    GlassFill(target, {m_cancelRect, buttonRadius, buttonRadius}, brush);
    PCWSTR cancel = Tr(L"Cancel", L"Cancelar");
    brush->SetColor(Fg(0.95f));
    target->DrawText(cancel, (UINT32)wcslen(cancel), m_buttonFormat.get(),
                     m_cancelRect, brush);
    brush->SetColor(Accent(1.0f - 0.15f * Hover(HoverKey(6002, 0),
                                                      m_hover == kHitConfirm)));
    target->FillRoundedRectangle({m_confirmButton, buttonRadius, buttonRadius},
                                 brush);
    PCWSTR confirm = ConfirmName(action);
    brush->SetColor(AccentInk(1));
    target->DrawText(confirm, (UINT32)wcslen(confirm), m_buttonFormat.get(),
                     m_confirmButton, brush);
    target->PopLayer();
    target->PopAxisAlignedClip();
}

////////////////////////////////////////////////////////////////////////////////
// The profiles' button on the pill: the profiles to switch to with a click
// (the one applied last checked), the current settings saved as a new one,
// and the settings where they're renamed and deleted.

constexpr float kProfilesWidth = 236;

class ProfilesPanel : public Panel {
   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_iconFormat = MakeFormat(IconFont(),
                                      DWRITE_FONT_WEIGHT_NORMAL, 15 * scale);
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           13 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_smallFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            if (m_textFormat) {
                m_textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER,
                                         0, 0};
                m_textFormat->SetTrimming(&trimming, nullptr);
            }
        }
        Layout();
    }

    void OnOpen() override {
        m_hover = -1;
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
        if (m_hover < 0) {
            return false;
        }
        m_hover = -1;
        return true;
    }

    bool OnMouseUp(POINT pt) override {
        const int hit = HitTest(pt);
        if (hit == kHitSave) {
            AddProfile(NewProfileName());
            Layout();
            return true;
        }
        if (hit == kHitManage) {
            OpenIslandSettingsFromPanel();
            return false;
        }
        if (hit >= 0 && hit < (int)g_profiles.size()) {
            const int id = g_profiles[hit].id;
            Close();
            ApplyProfile(id);
        }
        return false;
    }

   private:
    static constexpr int kHitSave = 100;
    static constexpr int kHitManage = 101;

    void Layout() {
        const float s = m_scale;
        const float width = std::round(kProfilesWidth * s);
        const float padding = std::round(6 * s);
        const float row = std::round(36 * s);
        const float separator = std::round(9 * s);
        float y = padding;
        m_rows.clear();
        for (size_t i = 0; i < g_profiles.size(); i++) {
            m_rows.push_back({padding, y, width - padding, y + row});
            y += row;
        }
        if (g_profiles.empty()) {
            m_empty = {padding, y, width - padding, y + row};
            y += row;
        } else {
            m_empty = {};
        }
        m_separatorY = y + separator / 2;
        y += separator;
        m_save = {};
        if (g_profiles.size() < kMaxProfiles) {
            m_save = {padding, y, width - padding, y + row};
            y += row;
        }
        m_manage = {padding, y, width - padding, y + row};
        y += row + padding;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    int HitTest(POINT pt) const {
        for (size_t i = 0; i < m_rows.size(); i++) {
            if (PointInRect(pt, m_rows[i])) {
                return (int)i;
            }
        }
        if (PointInRect(pt, m_save)) {
            return kHitSave;
        }
        if (PointInRect(pt, m_manage)) {
            return kHitManage;
        }
        return -1;
    }

    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallFormat;
    float m_preparedScale = 0;
    std::vector<D2D1_RECT_F> m_rows;
    D2D1_RECT_F m_empty{}, m_save{}, m_manage{};
    float m_separatorY = 0;
    int m_hover = -1;
};

void ProfilesPanel::Draw(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush) {
    if (!m_iconFormat || !m_textFormat || !m_smallFormat) {
        return;
    }
    const float s = m_scale;
    const float radius = std::round(9 * s);
    const float icon = std::round(36 * s);
    auto drawRow = [&](const D2D1_RECT_F& row, int hit, WCHAR glyph,
                       const std::wstring& name, bool checked) {
        const float lit = Hover(HoverKey(6100, hit), m_hover == hit);
        if (lit > 0.01f) {
            brush->SetColor(Fg(0.09f * lit));
            GlassFill(target, {row, radius, radius}, brush);
        }
        if (glyph) {
            brush->SetColor(checked ? Accent(1)
                                    : Fg(0.95f));
            DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                      {row.left, row.top, row.left + icon, row.bottom},
                      brush);
        }
        brush->SetColor(Fg(0.95f));
        target->DrawText(name.c_str(), (UINT32)name.size(), m_textFormat.get(),
                         {row.left + icon, row.top,
                          row.right - std::round(8 * s), row.bottom},
                         brush);
    };
    const int chosen = ChosenProfile();
    for (size_t i = 0; i < g_profiles.size() && i < m_rows.size(); i++) {
        const bool active = g_profiles[i].id == chosen;
        drawRow(m_rows[i], (int)i, active ? (WCHAR)0xE73E : kGlyphProfiles,
                g_profiles[i].name, active);
    }
    if (g_profiles.empty()) {
        PCWSTR none = Tr(L"No profiles yet", L"Nenhum perfil ainda");
        brush->SetColor(Fg(0.5f));
        target->DrawText(none, (UINT32)wcslen(none), m_smallFormat.get(),
                         {m_empty.left + std::round(12 * s), m_empty.top,
                          m_empty.right, m_empty.bottom},
                         brush);
    }
    brush->SetColor(Fg(0.10f));
    target->FillRectangle({std::round(14 * s), m_separatorY,
                           (float)m_size.cx - std::round(14 * s),
                           m_separatorY + std::max(1.0f, std::round(s))},
                          brush);
    if (m_save.bottom > m_save.top) {
        drawRow(m_save, kHitSave, (WCHAR)0xE710,
                Tr(L"Save current settings", L"Salvar as configurações atuais"),
                false);
    }
    drawRow(m_manage, kHitManage, (WCHAR)0xE713,
            Tr(L"Manage profiles", L"Gerenciar perfis"), false);
}

////////////////////////////////////////////////////////////////////////////////
// Windows' own banners, read and pressed through UI Automation (Windows'
// accessibility tree), only with the island's own notifications: Windows
// shows calls, alarms and reminders itself then (its "Do not disturb" lets
// them through), and the island takes over the one on its pill, hiding
// Windows' banner and pressing its buttons (its "Snooze" too) when the
// island's are pressed. On a thread of its own (it's another process).

// A notification in Windows' stack of banners.
struct WindowsBannerView {
    std::wstring title;
    // A call's, an alarm's or a reminder's.
    bool priority = false;
    std::vector<std::wstring> buttons;
    // It has the "x" that moves it to the notification center.
    bool dismiss = false;
};

// What a read found (and whether a button was pressed first).
struct WindowsBannerRead {
    std::vector<WindowsBannerView> views;
    bool pressRequested = false;
    bool pressed = false;
    // If pressing failed, the island's own button to press instead (the
    // app's, with the same name).
    std::wstring fallbackAppId;
    std::wstring fallbackArguments;
    std::wstring fallbackActivationType;
};

SRWLOCK g_windowsBannerReadsLock = SRWLOCK_INIT;
std::vector<WindowsBannerRead> g_windowsBannerReads;
// Tells the island a read is done.
constexpr UINT WM_APP_BANNER_READ = WM_APP + 57;

// How much the processor, the memory and the GPU are used, for the pill and
// its panel: measured on a thread of its own, only while one of them is
// shown, every 2 seconds (every second with the panel open). The processor
// from the system's idle and busy times and its frequency counters; the
// memory from the memory and performance status; the GPU (the one with the
// most video memory in use) from the "GPU Adapter Memory" and "GPU Engine"
// performance counters (pdh.dll, loaded for it), and its total memory,
// temperature, fan and power from the display driver's own figures
// (D3DKMTQueryAdapterInfo, documented in the WDK, looked up in gdi32.dll;
// left out when the driver doesn't give them). -1 when unknown.
constexpr UINT WM_APP_USAGE = WM_APP + 65;
// Posted to an island by its usage panel when it closes or shows another
// one: what's measured follows (see Island::OnUsagePanelChanged).
constexpr UINT WM_APP_USAGE_PANEL = WM_APP + 107;
// How long a value takes to count to a new one on the pill, in seconds.
constexpr double kUsageChangeSeconds = 0.8;
// How many measures the panel's graphs keep.
constexpr size_t kUsageHistory = 60;

