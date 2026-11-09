#pragma once
// The AI usage item clicked (islandAiUsage): each agent turned on, its
// session's and week's use as bars, with when each starts over; without its
// limits, how to have them: for Claude Code, the line for the user's own
// status line script is copied (Git Bash's or PowerShell's).
class AiUsagePanel : public Panel {
   public:
    // The limits read again: shown at once if it's open.
    void Refresh() {
        if (IsOpen()) {
            Layout();
            Render();
        }
    }

   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_iconFormat = MakeFormat(IconFont(), DWRITE_FONT_WEIGHT_NORMAL,
                                      20 * scale);
            m_titleFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 13 * scale,
                           DWRITE_TEXT_ALIGNMENT_LEADING);
            m_nameFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 12 * scale,
                           DWRITE_TEXT_ALIGNMENT_LEADING);
            m_textFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_valueFormat =
                MakeFormat(L"Segoe UI Variable Text",
                           DWRITE_FONT_WEIGHT_SEMI_BOLD, 11.5f * scale,
                           DWRITE_TEXT_ALIGNMENT_TRAILING);
            m_smallFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           10.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_updatedFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           10.5f * scale, DWRITE_TEXT_ALIGNMENT_TRAILING);
            m_helpFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            if (m_helpFormat) {
                m_helpFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
                m_helpFormat->SetParagraphAlignment(
                    DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            }
            m_buttonFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11 * scale, DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        Layout();
    }

    void OnOpen() override {
        m_hover = kNone;
        m_copiedAt = -10;
        m_bash = GitBashFound();
        Layout();
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override {
        if (m_iconFormat && m_titleFormat && m_nameFormat && m_textFormat &&
            m_valueFormat && m_smallFormat && m_updatedFormat &&
            m_helpFormat && m_buttonFormat) {
            Walk(target, brush);
        }
    }

    // ("Copied" said for a moment.)
    bool AnimateContents(double) override {
        return NowSeconds() - m_copiedAt < 2.2;
    }

    bool OnMouseMove(POINT pt) override {
        const int hover = HitTest(pt);
        if (hover == m_hover) {
            return false;
        }
        m_hover = hover;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hover == kNone) {
            return false;
        }
        m_hover = kNone;
        return true;
    }

    bool OnMouseUp(POINT pt) override {
        if (HitTest(pt) != kCopy) {
            return false;
        }
        // (Its file made first, only this account's.)
        if (PrepareClaudeUsageFile() &&
            CopyTextToClipboard(m_hwnd, ClaudeUsageLine(m_bash))) {
            m_copiedAt = NowSeconds();
        }
        return true;
    }

   private:
    static constexpr int kNone = -1;
    static constexpr int kCopy = 0;
    static constexpr int kParts = 1;
    static constexpr float kWidth = 320;

    int HitTest(POINT pt) const {
        for (int part = 0; part < kParts; part++) {
            if (PointInRect(pt, m_parts[part])) {
                return part;
            }
        }
        return kNone;
    }

    void Layout() {
        if (!m_titleFormat || !m_helpFormat) {
            return;
        }
        const float bottom = Walk(nullptr, nullptr);
        m_size = {(LONG)std::round(kWidth * m_scale), (LONG)std::ceil(bottom)};
    }

    // Its contents from the top, drawn (or, without a target, only
    // measured: where its buttons are); where they end.
    float Walk(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
        const float s = m_scale;
        const float width = std::round(kWidth * s);
        const float padding = std::round(14 * s);
        const float right = width - padding;
        const long long now = UnixNow();
        auto text = [&](const std::wstring& value, IDWriteTextFormat* format,
                        const D2D1_RECT_F& rect, const D2D1_COLOR_F& color) {
            if (target) {
                brush->SetColor(color);
                target->DrawText(value.c_str(), (UINT32)value.size(), format,
                                 rect, brush);
            }
        };
        // A wrapped text's height.
        auto wrapped = [&](const std::wstring& value,
                           IDWriteTextFormat* format) {
            winrt::com_ptr<IDWriteTextLayout> layout;
            DWRITE_TEXT_METRICS metrics{};
            if (SUCCEEDED(m_dwrite->CreateTextLayout(
                    value.c_str(), (UINT32)value.size(), format,
                    right - padding, 10000, layout.put()))) {
                layout->GetMetrics(&metrics);
            }
            return std::ceil(metrics.height);
        };
        // A wrapped text under the ones before.
        float top = padding;
        auto paragraph = [&](const std::wstring& value,
                             IDWriteTextFormat* format, D2D1_COLOR_F color) {
            top += std::round(6 * s);
            const float height = wrapped(value, format);
            text(value, format, {padding, top, right, top + height}, color);
            top += height;
        };
        // A button as wide as the panel's text.
        auto button = [&](int part, PCWSTR label) {
            top += std::round(8 * s);
            const float height = std::round(28 * s);
            m_parts[part] = {padding, top, right, top + height};
            if (target) {
                const float lit = Hover(HoverKey(7400, part), m_hover == part);
                const float radius = height / 2;
                brush->SetColor(Fg(0.1f + 0.08f * lit));
                GlassFill(target, {m_parts[part], radius, radius}, brush);
            }
            text(label, m_buttonFormat.get(), m_parts[part], Fg(0.9f));
            top += height;
        };
        for (D2D1_RECT_F& part : m_parts) {
            part = {};
        }
        const float icon = std::round(28 * s);
        if (target) {
            const WCHAR glyph = kGlyphAiUsage;
            brush->SetColor(Fg(0.95f));
            DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
                      {padding, top, padding + icon, top + icon}, brush);
        }
        text(Tr(L"AI usage", L"Uso de IA"), m_titleFormat.get(),
             {padding + icon + std::round(6 * s), top, right, top + icon},
             Fg(0.95f));
        top += icon;
        bool any = false;
        for (int agent = 0; agent < kAiAgents; agent++) {
            if (!AiAgentOn(agent)) {
                continue;
            }
            any = true;
            const AiUsage& usage = g_aiUsage[agent];
            const bool claude = agent == (int)AiAgent::Claude;
            top += std::round(12 * s);
            const float nameHeight = std::round(18 * s);
            const D2D1_RECT_F nameRow{padding, top, right, top + nameHeight};
            text(AiAgentName(agent), m_nameFormat.get(), nameRow, Fg(0.95f));
            if (usage.writtenAt > 0) {
                text(AiUpdatedText(usage.writtenAt, now), m_updatedFormat.get(),
                     nameRow, Fg(0.5f));
            }
            top += nameHeight;
            for (const AiLimit* limit : {&usage.session, &usage.week}) {
                const double used = AiUsedNow(*limit, now);
                if (used < 0) {
                    continue;
                }
                top += std::round(6 * s);
                const float row = std::round(16 * s);
                const D2D1_RECT_F labelRow{padding, top, right, top + row};
                text(limit == &usage.session
                         ? Tr(L"5-hour session", L"Sessão de 5 horas")
                         : Tr(L"Week", L"Semana"),
                     m_textFormat.get(), labelRow, Fg(0.7f));
                WCHAR percent[16];
                swprintf_s(percent, L"%.0f%%", used);
                const int rgb = AiUsageRgb(used);
                text(percent, m_valueFormat.get(), labelRow,
                     rgb >= 0 ? RgbColor(rgb, 1) : Fg(0.95f));
                top += row + std::round(4 * s);
                const float thickness = std::round(6 * s);
                if (target) {
                    const D2D1_RECT_F track{padding, top, right,
                                            top + thickness};
                    brush->SetColor(Fg(0.12f));
                    target->FillRoundedRectangle(
                        {track, thickness / 2, thickness / 2}, brush);
                    const float fill =
                        (float)(used / 100) * (track.right - track.left);
                    if (fill > 0.5f) {
                        brush->SetColor(rgb >= 0 ? RgbColor(rgb, 1)
                                                 : Accent(1));
                        target->FillRoundedRectangle(
                            {{track.left, track.top,
                              track.left + std::max(fill, thickness),
                              track.bottom},
                             thickness / 2,
                             thickness / 2},
                            brush);
                    }
                }
                top += thickness;
                const std::wstring left = AiTimeLeft(limit->resetsAt, now);
                if (!left.empty()) {
                    const float line = std::round(16 * s);
                    text(std::wstring(Tr(L"Starts over in", L"Recomeça em")) +
                             L" " + left + L" · " +
                             AiResetTime(limit->resetsAt),
                         m_smallFormat.get(), {padding, top, right, top + line},
                         Fg(0.5f));
                    top += line;
                }
            }
            const bool known = AiUsageKnown(usage);
            if (!claude) {
                if (!known) {
                    paragraph(Tr(L"Codex keeps its limits in its logs on this "
                                 L"PC: they show after its next answer",
                                 L"O Codex guarda os limites nos logs dele "
                                 L"neste PC: eles aparecem depois da próxima "
                                 L"resposta dele"),
                              m_helpFormat.get(), Fg(0.6f));
                }
                continue;
            }
            if (known) {
                continue;
            }
            paragraph(Tr(L"Only Claude Code in a terminal: the Claude "
                         L"desktop app doesn't leave its limits on this PC",
                         L"Só o Claude Code no terminal: o app de desktop do "
                         L"Claude não deixa os limites neste PC"),
                      m_helpFormat.get(), Fg(0.6f));
            paragraph(
                usage.writtenAt > 0
                    ? Tr(L"Claude Code tells its limits with a Pro or Max "
                         L"plan, after its first answer in a session",
                         L"O Claude Code informa os limites com um plano Pro "
                         L"ou Max, depois da primeira resposta numa sessão")
                : !m_bash
                    ? Tr(L"Without Git Bash, Claude Code runs its status line "
                         L"in PowerShell: copy the line into its script, "
                         L"after it reads its input into $stdin (see the "
                         L"README)",
                         L"Sem o Git Bash, o Claude Code roda a linha de "
                         L"status no PowerShell: copie a linha para o script "
                         L"dela, depois que ele lê a entrada em $stdin (veja "
                         L"o README)")
                    : Tr(L"Claude Code gives its limits to its status line, "
                         L"run in Git Bash: copy the line into its script, "
                         L"after it reads its input with input=$(cat) (see "
                         L"the README)",
                         L"O Claude Code passa os limites para a linha de "
                         L"status dele, que roda no Git Bash: copie a linha "
                         L"para o script dela, depois que ele lê a entrada "
                         L"com input=$(cat) (veja o README)"),
                m_helpFormat.get(), Fg(0.6f));
            if (usage.writtenAt <= 0) {
                button(kCopy, NowSeconds() - m_copiedAt < 2
                                  ? Tr(L"✓ Copied", L"✓ Copiada")
                                  : Tr(L"Copy the line", L"Copiar a linha"));
            }
        }
        if (!any) {
            paragraph(Tr(L"Turn on Claude Code or Codex in the settings",
                         L"Ligue o Claude Code ou o Codex nas configurações"),
                      m_helpFormat.get(), Fg(0.6f));
        }
        return top + padding;
    }

    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_titleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_nameFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_valueFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallFormat;
    winrt::com_ptr<IDWriteTextFormat> m_updatedFormat;
    winrt::com_ptr<IDWriteTextFormat> m_helpFormat;
    winrt::com_ptr<IDWriteTextFormat> m_buttonFormat;
    float m_preparedScale = 0;
    D2D1_RECT_F m_parts[kParts]{};
    int m_hover = kNone;
    double m_copiedAt = -10;
    // Whether Git Bash runs Claude Code's status line (which line is
    // copied), looked at as it opens.
    bool m_bash = false;
};

