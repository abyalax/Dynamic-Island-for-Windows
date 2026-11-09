#pragma once
ID2D1Bitmap* NotificationPanel::AppBitmap(ID2D1RenderTarget* target,
                                          const std::wstring& appId) {
    const double now = NowSeconds();
    int misses = 0;
    for (auto it = m_bitmaps.begin(); it != m_bitmaps.end(); ++it) {
        if (it->appId == appId) {
            if (!it->retryAt || now < it->retryAt) {
                return it->bitmap.get();
            }
            misses = it->bitmap ? 0 : it->misses + 1;
            m_bitmaps.erase(it);
            break;
        }
    }
    bool fromExplorer = false;
    winrt::com_ptr<ID2D1Bitmap> bitmap =
        LoadAppIcon(target, m_wic.get(), m_hwnd, appId,
                    (int)std::round(32 * m_scale), &fromExplorer);
    m_bitmaps.push_back(
        {appId, bitmap, AppIconRetryAt(fromExplorer, !!bitmap, misses, now),
         bitmap ? 0 : misses});
    return bitmap.get();
}

// "now", "5 min", the time today, "yesterday", or the date.
std::wstring NotificationPanel::TimeText(long long arrival) const {
    return NotificationTimeText(arrival);
}

void NotificationPanel::Draw(ID2D1RenderTarget* target,
                             ID2D1SolidColorBrush* brush) {
    if (!m_headerFormat || !m_appFormat || !m_timeFormat || !m_titleFormat ||
        !m_textFormat || !m_linkFormat || !m_iconFormat) {
        return;
    }
    const float s = m_scale;
    const float padding = std::round(12 * s);

    std::wstring title = Tr(L"Notifications", L"Notificações");
    if (m_notifications.size() > 1) {
        title += L"  \u00B7  " + std::to_wstring(m_notifications.size());
    }
    brush->SetColor(Fg(0.95f));
    target->DrawText(title.c_str(), (UINT32)title.size(), m_headerFormat.get(),
                     m_header, brush);
    if (!m_notifications.empty()) {
        PCWSTR clear = Tr(L"Clear all", L"Limpar tudo");
        brush->SetColor(
            Fg(0.55f + 0.4f * Hover(m_clearAll, m_hover.part == Part::ClearAll)));
        target->DrawText(clear, (UINT32)wcslen(clear), m_timeFormat.get(),
                         m_clearAll, brush);
    }

    // The gear that unfolds the switches (blue while they're open).
    {
        const float lit =
            Hover(m_settingsButton, m_hover.part == Part::SettingsButton);
        const float radius = (m_settingsButton.bottom - m_settingsButton.top) / 2;
        if (lit > 0.01f || m_settingsOpen) {
            brush->SetColor(m_settingsOpen
                                ? Accent(0.25f + 0.1f * lit)
                                : Fg(0.1f * lit));
            GlassFill(target, {m_settingsButton, radius, radius}, brush);
        }
        const D2D1_POINT_2F center{
            (m_settingsButton.left + m_settingsButton.right) / 2,
            (m_settingsButton.top + m_settingsButton.bottom) / 2};
        // On top of the panel's own placement.
        D2D1_MATRIX_3X2_F before;
        target->GetTransform(&before);
        target->SetTransform(
            D2D1::Matrix3x2F::Rotation((float)m_settingsAmount * 90, center) *
            *D2D1::Matrix3x2F::ReinterpretBaseType(&before));
        const WCHAR gear = 0xE713;
        brush->SetColor(Fg(0.6f + 0.35f * lit));
        DrawGlyph(target, &gear, 1, m_iconFormat.get(), m_settingsButton,
                  brush);
        target->SetTransform(before);
    }
    // The switches, unfolding (clipped to the part open, fading in).
    const float unfolded = (float)std::clamp(m_settingsAmount, 0.0, 1.0);
    winrt::com_ptr<ID2D1Layer> settingsLayer;
    const bool settingsShown = unfolded > 0.01f;
    if (settingsShown) {
        target->CreateLayer(nullptr, settingsLayer.put());
        target->PushLayer(
            D2D1::LayerParameters(m_settingsSection, nullptr,
                                  D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                  D2D1::IdentityMatrix(), unfolded),
            settingsLayer.get());
    }
    if (settingsShown) {
    // "Do not disturb", with the moon, and "Show content", with the eye.
    DrawSwitchRow(target, brush, m_dndRow, m_dndToggle, kGlyphMoon,
                  D2D1::ColorF(0.55f, 0.45f, 1.0f, 1),
                  Tr(L"Do not disturb", L"Não incomodar"),
                  g_settings.islandOwnsNotifications
                      ? Tr(L"The island's: no banners or sounds from it",
                           L"O da ilha: sem avisos nem sons dela")
                      : Tr(L"Windows' own: no notification sounds",
                           L"O do Windows: sem os sons das notificações"),
                  m_dndKnob, m_hover.part == Part::DndToggle);
    // The pill's banners, and under them, in the same card, whether they
    // also open the minimized pill (which only applies with them on). Only
    // the main island shows them: dimmed on the others.
    const bool otherIsland = OtherIslandShown();
    {
        const D2D1_RECT_F card{m_bannerRow.left, m_bannerRow.top,
                               m_minimizedRow.right, m_minimizedRow.bottom};
        const float radius = std::round(12 * s);
        brush->SetColor(Fg(0.08f));
        GlassFill(target, {card, radius, radius}, brush);
        brush->SetColor(Fg(0.12f));
        target->FillRectangle({m_minimizedRow.left + std::round(36 * s),
                               m_minimizedRow.top - 0.5f,
                               m_minimizedRow.right - std::round(10 * s),
                               m_minimizedRow.top + 0.5f},
                              brush);
    }
    DrawSwitchRow(target, brush, m_bannerRow, m_bannerToggle, kGlyphBell,
                  Accent(1),
                  Tr(L"Banners in the pill", L"Avisos na pílula"),
                  otherIsland
                      ? Tr(L"Only on the main island: change it in its "
                           L"settings",
                           L"Só na ilha principal: mude nas configurações "
                           L"dela")
                      : Tr(L"Each new one opens the pill for a moment",
                           L"Cada nova abre a pílula por alguns segundos"),
                  m_bannerKnob, m_hover.part == Part::BannerToggle,
                  !otherIsland, false);
    DrawSwitchRow(target, brush, m_minimizedRow, m_minimizedToggle,
                  kGlyphCollapse, Accent(1),
                  Tr(L"Even when minimized", L"Mesmo com a pílula minimizada"),
                  Tr(L"Off: only the app's icon peeks out of the line",
                     L"Desligado: só o ícone do app aparece na linha"),
                  m_minimizedKnob, m_hover.part == Part::MinimizedToggle,
                  g_bannerEnabled && !otherIsland, false);
    DrawSwitchRow(target, brush, m_contentRow, m_contentToggle, kGlyphShow,
                  Accent(1),
                  Tr(L"Show what they say", L"Mostrar o conteúdo"),
                  Tr(L"The title and the text, in the pill and in this list",
                     L"O título e o texto, na pílula e nesta lista"),
                  m_contentKnob, m_hover.part == Part::ContentToggle);
    }
    if (settingsShown) {
        target->PopLayer();
    }

    if (m_cards.empty() ||
        (m_clearAllPending &&
         std::all_of(m_motions.begin(), m_motions.end(),
                     [](const CardMotion& motion) {
                         return motion.removing && motion.room < 0.5;
                     }))) {
        PCWSTR empty = Tr(L"No notifications", L"Nenhuma notificação");
        brush->SetColor(Fg(0.5f));
        target->DrawText(empty, (UINT32)wcslen(empty), m_linkFormat.get(),
                         {padding, m_listViewport.top, m_size.cx - padding,
                          m_listViewport.top + std::round(40 * s)},
                         brush);
    }

    BeginScrollArea(target, m_listViewport, m_scroll);
    const float scrolled = m_scroll.Offset();
    const float viewportHeight = m_listViewport.bottom - m_listViewport.top;
    for (size_t i = 0; i < m_cards.size() && i < m_notifications.size(); i++) {
        const Notification& n = m_notifications[i];
        const D2D1_RECT_F& card = m_cards[i];
        // Only the ones in view.
        if (card.bottom < scrolled - 2 ||
            card.top > scrolled + viewportHeight + 2) {
            continue;
        }
        // Moved sideways, fading the farther it goes.
        float offset = 0;
        float room = 1;
        for (const auto& motion : m_motions) {
            if (motion.id == n.id) {
                offset = (float)motion.offset;
                room = (float)std::clamp(motion.room, 0.0, 1.0);
            }
        }
        if (room < 0.02f) {
            continue;
        }
        D2D1_MATRIX_3X2_F transform;
        target->GetTransform(&transform);
        target->SetTransform(D2D1::Matrix3x2F::Translation(offset, 0) *
                             transform);
        winrt::com_ptr<ID2D1Layer> cardLayer;
        const float fade =
            std::clamp(1 - std::fabs(offset) / (card.right - card.left), 0.0f,
                       1.0f) *
            room;
        target->CreateLayer(nullptr, cardLayer.put());
        target->PushLayer(D2D1::LayerParameters(
                              {card.left - 1, card.top - 1, card.right + 1,
                               card.bottom + 1},
                              nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                              D2D1::IdentityMatrix(), fade),
                          cardLayer.get());
        const float radius = std::round(16 * s);
        // Lighting up smoothly under the mouse.
        brush->SetColor(Fg(0.09f + 0.06f * HoverAmount(n.id)));
        GlassFill(target, {card, radius, radius}, brush);
        // Glowing: shown for its app, or clicked with nothing to open.
        const double glowing = NowSeconds() - m_glowStart;
        if (glowing < kCardGlowSeconds &&
            (n.id == m_glowId ||
             (m_glowId < 0 &&
              _wcsicmp(n.appId.c_str(), m_glowApp.c_str()) == 0))) {
            const float glow =
                (float)std::sin(std::min(glowing / 0.3, 1.0) * 1.5708) *
                (float)(1 - glowing / kCardGlowSeconds);
            brush->SetColor(Accent(0.9f * glow));
            target->DrawRoundedRectangle(
                {{card.left + 1, card.top + 1, card.right - 1, card.bottom - 1},
                 radius - 1,
                 radius - 1},
                brush, 2);
            brush->SetColor(Accent(0.12f * glow));
            target->FillRoundedRectangle({card, radius, radius}, brush);
        }

        const float inner = std::round(12 * s);
        const float icon = std::round(18 * s);
        const float top = card.top + std::round(10 * s);
        const D2D1_RECT_F iconRect{card.left + inner, top,
                                   card.left + inner + icon, top + icon};
        if (ID2D1Bitmap* bitmap = AppBitmap(target, n.appId)) {
            DrawBitmapSmooth(target, bitmap, iconRect);
        } else {
            brush->SetColor(Fg(0.3f));
            GlassFill(target, {iconRect, icon / 4, icon / 4}, brush);
        }
        // The time, or the "x" under the mouse.
        const float right = card.right - inner;
        const D2D1_RECT_F appLine{iconRect.right + std::round(8 * s), top,
                                  right, top + icon};
        const std::wstring& name = AppName(n.appId);
        brush->SetColor(Fg(0.6f));
        target->DrawText(name.c_str(), (UINT32)name.size(), m_appFormat.get(),
                         {appLine.left, appLine.top,
                          appLine.right - std::round(70 * s), appLine.bottom},
                         brush);
        // The time gives way to the "x" as the card gets the focus (both
        // fading, the "x" growing in).
        const float focus = HoverAmount(n.id);
        if (focus < 0.99f) {
            const std::wstring time = TimeText(n.arrival);
            brush->SetColor(Fg(0.6f * (1 - focus)));
            target->DrawText(time.c_str(), (UINT32)time.size(),
                             m_timeFormat.get(), appLine, brush);
        }
        if (focus > 0.01f) {
            const float circle = std::round(9 * s) * (0.6f + 0.4f * focus);
            const D2D1_POINT_2F center{
                right - std::round(9 * s) + std::round(2 * s), top + icon / 2};
            brush->SetColor(Fg(
                (m_hover.part == Part::Dismiss ? 0.35f : 0.18f) * focus));
            GlassFillEllipse(target, {center, circle, circle}, brush);
            brush->SetColor(Fg(0.95f * focus));
            DrawGlyph(target, &kGlyphClose, 1, m_iconFormat.get(),
                      {center.x - circle, center.y - circle,
                       center.x + circle, center.y + circle},
                      brush);
        }

        const float line = std::round(19 * s);
        const float textTop = top + icon + std::round(4 * s);
        // Only the app, when the content is hidden.
        const bool content = g_showNotificationContent;
        const std::wstring cardTitle =
            content ? n.title : std::wstring(Tr(L"Notification", L"Notificação"));
        const std::wstring cardText =
            content ? n.text
                    : std::wstring(Tr(L"Content hidden", L"Conteúdo oculto"));
        brush->SetColor(Fg(0.95f));
        target->DrawText(cardTitle.c_str(), (UINT32)cardTitle.size(),
                         m_titleFormat.get(),
                         {card.left + inner, textTop, card.right - inner,
                          textTop + line},
                         brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        // Snoozed: it says when it shows again, instead of its buttons.
        const bool snoozed = g_island && g_island->IsToastSnoozed(n.id);
        if (snoozed && focus > 0.01f && !n.reply.actions.empty()) {
            const D2D1_RECT_F row = CardActionRect(card, CardExtra(n.id), n, 0);
            PCWSTR label = Tr(L"Snoozed: shows again in 5 minutes",
                              L"Adiada: volta em 5 minutos");
            brush->SetColor(Fg(0.55f * focus));
            target->DrawText(label, (UINT32)wcslen(label), m_appFormat.get(),
                             {card.left + std::round(12 * s), row.top,
                              card.right - std::round(12 * s), row.bottom},
                             brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
        // Its own buttons (a call's, an alarm's...), fading and growing in
        // with the card's focus.
        if (focus > 0.01f && n.id != m_replyId && !snoozed) {
            m_appFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            for (size_t a = 0; a < n.reply.actions.size() && a < 16; a++) {
                const D2D1_RECT_F rect =
                    CardActionRect(card, CardExtra(n.id), n, a);
                const float grow = 0.85f + 0.15f * focus;
                const D2D1_POINT_2F middle{(rect.left + rect.right) / 2,
                                           (rect.top + rect.bottom) / 2};
                const float halfWidth = (rect.right - rect.left) / 2 * grow;
                const float halfHeight = (rect.bottom - rect.top) / 2 * grow;
                const D2D1_RECT_F button{middle.x - halfWidth,
                                         middle.y - halfHeight,
                                         middle.x + halfWidth,
                                         middle.y + halfHeight};
                const float lit = Hover(
                    HoverKey(3003, n.id * 16 + (long long)a),
                    m_hover.part == Part::CardAction &&
                        m_hover.index == (int)(i * 16 + a));
                brush->SetColor(Fg((0.2f + 0.1f * lit) * focus));
                GlassFill(target, {button, halfHeight, halfHeight}, brush);
                const std::wstring& label = n.reply.actions[a].label;
                brush->SetColor(Fg(0.95f * focus));
                target->DrawText(label.c_str(), (UINT32)label.size(),
                                 m_appFormat.get(),
                                 {rect.left + std::round(4 * s), rect.top,
                                  rect.right - std::round(4 * s), rect.bottom},
                                 brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
            m_appFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        }
        // "Reply" on the card under the mouse; the box when answering.
        if (n.reply.CanReply()) {
            D2D1_RECT_F replyButton, field, send;
            CardParts(card, CardExtra(n.id), &replyButton, &field, &send);
            if (n.id == m_replyId) {
                const float fieldRadius = std::min((field.bottom - field.top) / 2,
                                               std::round(16 * m_scale));
                brush->SetColor(Fg(0.14f));
                GlassFill(target, {field, fieldRadius, fieldRadius}, brush);
                brush->SetColor(Accent(1));
                target->DrawRoundedRectangle(
                    {{field.left + 0.75f, field.top + 0.75f,
                      field.right - 0.75f, field.bottom - 0.75f},
                     fieldRadius,
                     fieldRadius},
                    brush, 1.5f);
                // What's typed, wrapping onto more lines (the box grows,
                // up to five; then the end stays in view); the hint on one.
                const float base = std::round(32 * s);
                const float replyLine = std::round(17 * s);
                const float padTop = std::round((base - replyLine) / 2);
                const bool empty = m_replyText.empty();
                float caretX = field.left + inner;
                float caretTop = field.top + padTop;
                float caretHeight = replyLine;
                if (empty) {
                    const std::wstring hint =
                        n.reply.placeholder.empty()
                            ? std::wstring(Tr(L"Reply", L"Responder"))
                            : n.reply.placeholder;
                    brush->SetColor(Fg(0.45f));
                    target->DrawText(hint.c_str(), (UINT32)hint.size(),
                                     m_textFormat.get(),
                                     {field.left + inner, field.top,
                                      send.left, field.top + base},
                                     brush);
                } else {
                    const D2D1_RECT_F textRect{field.left + inner,
                                               field.top + padTop, send.left,
                                               field.bottom - padTop};
                    const WrappedField wrappedReply = LayOutEdit(
                        m_dwrite, m_textFormat.get(), m_replyText, m_replyEdit,
                        textRect.right - textRect.left,
                        textRect.bottom - textRect.top, replyLine);
                    if (wrappedReply.layout) {
                        target->PushAxisAlignedClip(
                            textRect, D2D1_ANTIALIAS_MODE_ALIASED);
                        DrawTextSelection(
                            target, brush, wrappedReply.layout.get(),
                            {textRect.left, textRect.top - wrappedReply.scroll},
                            m_replyEdit);
                        brush->SetColor(Fg(0.95f));
                        target->DrawTextLayout(
                            {textRect.left, textRect.top - wrappedReply.scroll},
                            wrappedReply.layout.get(), brush,
                            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
                        target->PopAxisAlignedClip();
                        caretX = textRect.left + wrappedReply.caretX;
                        caretTop = textRect.top - wrappedReply.scroll +
                                   wrappedReply.caretY;
                        caretHeight = wrappedReply.caretHeight;
                        m_replyEdit.origin = {
                            textRect.left, textRect.top - wrappedReply.scroll};
                    }
                }
                // The caret, only where the box shows it.
                if (caretTop >= field.top &&
                    caretTop + caretHeight <= field.bottom + 1) {
                    brush->SetColor(CaretColor());
                    target->FillRectangle({caretX + 1, caretTop + 1,
                                           caretX + 2.5f,
                                           caretTop + caretHeight - 1},
                                          brush);
                }
                // Send, blue, growing in once something is typed, and
                // lighter under the mouse.
                const float sendShown = Hover(HoverKey(3001, n.id), !empty);
                const float sendLit = Hover(
                    HoverKey(3002, n.id),
                    !empty && m_hover.part == Part::ReplySend);
                if (sendShown > 0.01f) {
                    const float sendRadius =
                        ((send.bottom - send.top) / 2 - std::round(3 * s)) *
                        (0.5f + 0.5f * sendShown);
                    const D2D1_POINT_2F sendCenter{(send.left + send.right) / 2,
                                                   (send.top + send.bottom) / 2};
                    brush->SetColor(AccentLit(sendShown, sendLit));
                    target->FillEllipse({sendCenter, sendRadius, sendRadius},
                                        brush);
                    brush->SetColor(AccentInk(sendShown));
                    DrawGlyph(target, &kGlyphSend, 1, m_iconFormat.get(), send,
                              brush);
                }
            } else if (focus > 0.01f) {
                // Fading and growing in with the card's focus.
                const float grow = 0.85f + 0.15f * focus;
                const D2D1_POINT_2F middle{
                    (replyButton.left + replyButton.right) / 2,
                    (replyButton.top + replyButton.bottom) / 2};
                const float halfWidth =
                    (replyButton.right - replyButton.left) / 2 * grow;
                const float halfHeight =
                    (replyButton.bottom - replyButton.top) / 2 * grow;
                const D2D1_RECT_F button{middle.x - halfWidth,
                                         middle.y - halfHeight,
                                         middle.x + halfWidth,
                                         middle.y + halfHeight};
                const float buttonRadius = halfHeight;
                brush->SetColor(Fg(
                    (m_hover.part == Part::ReplyButton ? 0.3f : 0.2f) * focus));
                GlassFill(target, {button, buttonRadius, buttonRadius}, brush);
                // "Reply" (the app's own button is "Send", inside the box).
                const std::wstring label = Tr(L"Reply", L"Responder");
                brush->SetColor(Fg(0.95f * focus));
                m_appFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                target->DrawText(label.c_str(), (UINT32)label.size(),
                                 m_appFormat.get(), replyButton, brush);
                m_appFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            }
        }
        brush->SetColor(Fg(content ? 0.7f : 0.45f));
        // Under the mouse, on up to three lines (opening as it grows).
        const float textExtra = CardExtra(n.id);
        winrt::com_ptr<IDWriteTextLayout> wrapped =
            content && textExtra > 0.5f
                ? WrappedLayout(m_dwrite, m_textFormat.get(), n.fullText,
                                card.right - card.left - inner * 2, line * 3,
                                line)
                : nullptr;
        if (wrapped) {
            target->PushAxisAlignedClip(
                {card.left + inner, textTop + line, card.right - inner,
                 textTop + line * 2 + textExtra},
                D2D1_ANTIALIAS_MODE_ALIASED);
            target->DrawTextLayout({card.left + inner, textTop + line},
                                   wrapped.get(), brush,
                                   D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
            target->PopAxisAlignedClip();
        } else {
            target->DrawText(cardText.c_str(), (UINT32)cardText.size(),
                             m_textFormat.get(),
                             {card.left + inner, textTop + line,
                              card.right - inner, textTop + line * 2},
                             brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        }
        target->PopLayer();
        target->SetTransform(transform);
    }
    EndScrollArea(target);
    DrawScrollBar(target, brush, m_listViewport, m_scroll);

    PCWSTR link = Tr(L"Open the Windows notification center",
                     L"Abrir a central de notificações do Windows");
    brush->SetColor(Fg(0.5f + 0.4f * Hover(m_link, m_hover.part == Part::Link)));
    target->DrawText(link, (UINT32)wcslen(link), m_linkFormat.get(), m_link,
                     brush);
}

// The switches slide to their state (they start there).
bool NotificationPanel::AnimateContents(double dt) {
    const double dnd = m_doNotDisturb ? 1 : 0;
    const double content = g_showNotificationContent ? 1 : 0;
    if (m_dndKnob < 0) {
        m_dndKnob = dnd;
    }
    if (m_contentKnob < 0) {
        m_contentKnob = content;
    }
    const bool a = SpringTowards(m_dndKnob, m_dndKnobVelocity, dnd, 0.3, dt,
                                 kBounceDamping);
    const bool b = SpringTowards(m_contentKnob, m_contentKnobVelocity, content,
                                 0.3, dt, kBounceDamping);
    const double banner = g_bannerEnabled ? 1 : 0;
    const double minimized = g_bannerWhenMinimized ? 1 : 0;
    if (m_bannerKnob < 0) {
        m_bannerKnob = banner;
    }
    if (m_minimizedKnob < 0) {
        m_minimizedKnob = minimized;
    }
    const bool c = SpringTowards(m_bannerKnob, m_bannerKnobVelocity, banner,
                                 0.3, dt, kBounceDamping);
    const bool d = SpringTowards(m_minimizedKnob, m_minimizedKnobVelocity,
                                 minimized, 0.3, dt, kBounceDamping);
    const bool folding =
        SpringTowards(m_settingsAmount, m_settingsVelocity,
                      m_settingsOpen ? 1 : 0, 0.4, dt);
    if (folding) {
        Layout();
    }

    // The cards: sliding, then closing their room, then gone.
    bool cards = false;
    std::vector<long long> removed;
    for (auto& motion : m_motions) {
        if (motion.id == m_dragId && m_dragMoved) {
            cards = true;
            continue;
        }
        if (motion.delay > 0) {
            motion.delay -= dt;
            cards = true;
            continue;
        }
        if (SpringTowards(motion.offset, motion.velocity, motion.target,
                          motion.removing ? 0.35 : 0.4, dt,
                          motion.removing ? 1.0 : kBounceDamping)) {
            cards = true;
        }
        // A new one opening its room.
        if (!motion.removing && motion.room < 1 &&
            SpringTowards(motion.room, motion.roomVelocity, 1, 0.35, dt)) {
            cards = true;
        }
        if (motion.removing &&
            std::fabs(motion.offset) >= std::fabs(motion.target) * 0.85) {
            if (SpringTowards(motion.room, motion.roomVelocity, 0, 0.3, dt)) {
                cards = true;
            } else {
                removed.push_back(motion.id);
            }
        }
    }
    // Settled back in place: no longer moving.
    std::erase_if(m_motions, [&](const CardMotion& motion) {
        return !motion.removing && motion.offset == 0 &&
               motion.velocity == 0 && motion.room >= 1 &&
               motion.id != m_dragId;
    });
    if (m_clearAllPending) {
        // All gone (it was kept when "Clear all" was clicked): the list
        // shows what came since.
        if (std::all_of(m_motions.begin(), m_motions.end(),
                        [](const CardMotion& motion) {
                            return motion.removing && motion.room <= 0 &&
                                   motion.roomVelocity == 0;
                        })) {
            m_clearAllPending = false;
            m_motions.clear();
            Load();
            cards = true;
        }
    } else if (!removed.empty()) {
        std::erase_if(m_motions, [&](const CardMotion& motion) {
            return std::find(removed.begin(), removed.end(), motion.id) !=
                   removed.end();
        });
        Load();
        cards = true;
    }
    // The card under the mouse lights up (and opens its text), the one left
    // goes back, smoothly.
    // The one being answered stays open, wherever the mouse is.
    const long long hovered = HoveredCard();
    for (long long id : {hovered, m_replyId}) {
        if (id >= 0 &&
            std::none_of(m_cardHovers.begin(), m_cardHovers.end(),
                         [&](const CardHover& hover) { return hover.id == id; })) {
            m_cardHovers.push_back({id});
        }
    }
    auto focused = [&](long long id) {
        return id == hovered || id == m_replyId;
    };
    bool hovering = false;
    for (auto& hover : m_cardHovers) {
        if (SpringTowards(hover.amount, hover.velocity,
                          focused(hover.id) ? 1 : 0, 0.3, dt)) {
            hovering = true;
        }
    }
    std::erase_if(m_cardHovers, [&](const CardHover& hover) {
        return !focused(hover.id) && hover.amount <= 0;
    });
    if (cards || !removed.empty() || hovering) {
        Layout();
    }
    const bool scrolling = m_scroll.Animate(dt);
    const bool glowing = NowSeconds() - m_glowStart < kCardGlowSeconds;
    const bool moving = a || b || c || d || folding || cards || scrolling ||
                        glowing || hovering;
    // A reply being typed: its caret blinks a moment (at the lower frame
    // rate).
    const bool caret = m_replyId >= 0 && CaretBlinking();
    m_idleMotion = caret && !moving;
    return moving || caret;
}

// A row with an icon (colored when on), a name, and a switch on the right
// whose knob is at `knob` (0 off, 1 on).
void NotificationPanel::DrawSwitchRow(ID2D1RenderTarget* target,
                                      ID2D1SolidColorBrush* brush,
                                      const D2D1_RECT_F& row,
                                      const D2D1_RECT_F& toggle,
                                      WCHAR glyph,
                                      const D2D1_COLOR_F& glyphColor,
                                      PCWSTR name,
                                      PCWSTR description,
                                      double knob,
                                      bool hover,
                                      bool enabled,
                                      bool ownCard) {
    const float s = m_scale;
    const float on = (float)std::clamp(knob < 0 ? 0.0 : knob, 0.0, 1.0);
    // Dimmed while it doesn't apply.
    winrt::com_ptr<ID2D1Layer> layer;
    if (!enabled) {
        target->CreateLayer(nullptr, layer.put());
        target->PushLayer(
            D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                  D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                  D2D1::IdentityMatrix(), 0.4f),
            layer.get());
        hover = false;
    }
    const float radius = std::round(12 * s);
    if (ownCard) {
        brush->SetColor(Fg(0.08f + 0.05f * Hover(row, hover)));
        GlassFill(target, {row, radius, radius}, brush);
    } else {
        // In a card shared with another row: lit inside it.
        const float inset = std::round(3 * s);
        brush->SetColor(Fg(0.05f * Hover(row, hover)));
        GlassFill(target,
                  {{row.left + inset, row.top + inset, row.right - inset,
                    row.bottom - inset},
                   radius - inset,
                   radius - inset},
                  brush);
    }
    const float icon = std::round(36 * s);
    auto mix = [on](float a, float b) { return a + (b - a) * on; };
    const D2D1_COLOR_F ink = Fg(0.95f);
    brush->SetColor(D2D1::ColorF(mix(ink.r, glyphColor.r),
                                 mix(ink.g, glyphColor.g),
                                 mix(ink.b, glyphColor.b), ink.a));
    DrawGlyph(target, &glyph, 1, m_iconFormat.get(),
              {row.left, row.top, row.left + icon, row.bottom}, brush);
    const float middle = (row.top + row.bottom) / 2;
    const float textRight = toggle.left - std::round(8 * s);
    brush->SetColor(Fg(0.95f));
    target->DrawText(name, (UINT32)wcslen(name), m_textFormat.get(),
                     {row.left + icon, middle - std::round(19 * s), textRight,
                      middle + std::round(1 * s)},
                     brush);
    brush->SetColor(Fg(0.5f));
    target->DrawText(description, (UINT32)wcslen(description),
                     m_appFormat.get(),
                     {row.left + icon, middle + std::round(1 * s), textRight,
                      middle + std::round(18 * s)},
                     brush);
    // The track goes from gray to blue; the knob slides, a bit springy.
    const float trackRadius = (toggle.bottom - toggle.top) / 2;
    const D2D1_COLOR_F track = Fg(0.22f);
    const D2D1_COLOR_F accent = Accent(1);
    brush->SetColor(D2D1::ColorF(mix(track.r, accent.r), mix(track.g, accent.g),
                                 mix(track.b, accent.b), mix(track.a, 1)));
    target->FillRoundedRectangle({toggle, trackRadius, trackRadius}, brush);
    const float knobRadius = trackRadius - std::round(2 * s);
    const float travel = toggle.right - toggle.left - trackRadius * 2;
    const float x = toggle.left + trackRadius +
                    travel * (float)std::clamp(knob < 0 ? 0.0 : knob, -0.1, 1.1);
    brush->SetColor(KnobColor(mix(0, 1)));
    target->FillEllipse({{x, toggle.top + trackRadius}, knobRadius, knobRadius},
                        brush);
    if (layer) {
        target->PopLayer();
    }
}

bool NotificationPanel::OnMouseMove(POINT pt) {
    // Dragging a card sideways: it follows the mouse.
    if (m_dragId >= 0 && (GetKeyState(VK_LBUTTON) & 0x8000)) {
        const int dx = pt.x - m_dragStartX;
        if (!m_dragMoved && std::abs(dx) > std::round(6 * m_scale)) {
            m_dragMoved = true;
        }
        if (m_dragMoved) {
            CardMotion* motion = Motion(m_dragId, true);
            motion->offset = motion->target = dx;
            motion->velocity = 0;
            return true;
        }
        return false;
    }
    const Hit hover = HitTest(pt);
    if (hover == m_hover) {
        return false;
    }
    m_hover = hover;
    // The cards' focus moves smoothly (see AnimateContents).
    StartIslandAnimation();
    return true;
}

bool NotificationPanel::OnMouseLeave() {
    if (m_hover.part == Part::None) {
        return false;
    }
    m_hover = {};
    StartIslandAnimation();
    return true;
}

NotificationPanel::CardMotion* NotificationPanel::Motion(long long id,
                                                       bool create) {
    for (auto& motion : m_motions) {
        if (motion.id == id) {
            return &motion;
        }
    }
    if (!create) {
        return nullptr;
    }
    m_motions.push_back({id});
    return &m_motions.back();
}

// Slides a card away (right for 1, left for -1), then closes its room.
// Gone from Windows' notification center too (the island's X, a swipe, a
// card's button, "Clear all"), like Windows' own list does: each by its tag
// and group (ToastNotificationHistory, documented; it refuses an empty
// group, like WhatsApp's); one without both only with all of its app's,
// when every one of that app in the center is among them, by their IDs read
// before any is removed (nothing the island's list doesn't show goes).
void NotificationPanel::RemoveFromCenter(const std::vector<Notification>& gone) {
    using winrt::Windows::UI::Notifications::ToastNotificationManager;
    const bool anyUntagged =
        std::any_of(gone.begin(), gone.end(), [](const Notification& n) {
            return !n.appId.empty() && (n.tag.empty() || n.group.empty());
        });
    std::vector<std::pair<std::wstring, long long>> inCenter;
    const bool read =
        anyUntagged &&
        ReadCenter(
            "SELECT h.PrimaryId, n.Id FROM Notification n JOIN "
            "NotificationHandler h ON h.RecordId = n.HandlerId WHERE "
            "n.Type = 'toast'",
            [&](void* statement) {
                if (auto app =
                        (PCWSTR)m_sqlite.column_text16(statement, 0)) {
                    inCenter.push_back(
                        {app, m_sqlite.column_int64(statement, 1)});
                }
            });
    std::vector<std::wstring> cleared;
    for (const auto& n : gone) {
        if (n.appId.empty()) {
            continue;
        }
        try {
            if (!n.tag.empty() && !n.group.empty()) {
                ToastNotificationManager::History().Remove(n.tag, n.group,
                                                           n.appId);
                continue;
            }
            const auto sameApp = [&](const std::wstring& app) {
                return _wcsicmp(app.c_str(), n.appId.c_str()) == 0;
            };
            if (!read ||
                std::any_of(cleared.begin(), cleared.end(), sameApp)) {
                continue;
            }
            bool any = false;
            const bool allGone = std::all_of(
                inCenter.begin(), inCenter.end(), [&](const auto& entry) {
                    if (!sameApp(entry.first)) {
                        return true;
                    }
                    any = true;
                    return std::any_of(gone.begin(), gone.end(),
                                       [&](const Notification& other) {
                                           return other.id == entry.second;
                                       });
                });
            if (any && allGone) {
                ToastNotificationManager::History().Clear(n.appId);
                cleared.push_back(n.appId);
            }
        } catch (winrt::hresult_error const& e) {
            IslandLog(L"Removing a notification from the center: %08X",
                   (unsigned)e.code());
        }
    }
}

void NotificationPanel::Remove(long long id,
                               int direction,
                               double delay,
                               bool center) {
    CardMotion* motion = Motion(id, true);
    motion->removing = true;
    motion->delay = delay;
    motion->target = direction * (kNotificationsWidth * m_scale * 1.1);
    if (id == m_replyId) {
        m_replyId = -1;
        m_replyText.clear();
        m_replyEdit.Reset();
    }
    StartIslandAnimation();
    // Kept at once (closing the list doesn't undo it): dismissed, and its
    // app leaves the island if nothing else of it is left.
    m_dismissed.push_back(id);
    std::wstring appId;
    for (const auto& n : m_notifications) {
        if (n.id == id) {
            appId = n.appId;
            if (const std::wstring key = DismissKey(n); !key.empty()) {
                m_dismissedTexts.push_back(key);
            }
            if (center) {
                RemoveFromCenter({n});
            }
        }
    }
    SaveCleared();
    const bool othersLeft =
        std::any_of(m_notifications.begin(), m_notifications.end(),
                    [&](const Notification& n) {
                        return n.id != id &&
                               _wcsicmp(n.appId.c_str(), appId.c_str()) == 0 &&
                               std::find(m_dismissed.begin(), m_dismissed.end(),
                                         n.id) == m_dismissed.end();
                    });
    if (g_island && !appId.empty() && !othersLeft) {
        ForEachIsland(
            [&](Island& island) { island.ClearAppsFromList({appId}, false); });
    }
}

bool NotificationPanel::OnMouseDown(POINT pt) {
    const Hit hit = HitTest(pt);
    // In the reply box: the caret goes where it's pressed (a drag from there
    // selects; the box is drawn in the list's coordinates).
    if (hit.part == Part::ReplyField) {
        EditTextClick(m_replyText, m_replyEdit, ToList(pt));
        return true;
    }
    if (hit.part != Part::Card || hit.index < 0 ||
        hit.index >= (int)m_notifications.size()) {
        return false;
    }
    // Maybe a drag: decided once the mouse moves.
    m_dragId = m_notifications[hit.index].id;
    m_dragStartX = pt.x;
    m_dragMoved = false;
    SetCapture(m_hwnd);
    return false;
}

bool NotificationPanel::OnMouseUp(POINT pt) {
    // The end of a drag: away if far enough, otherwise back.
    if (m_dragId >= 0) {
        const long long id = m_dragId;
        m_dragId = -1;
        if (GetCapture() == m_hwnd) {
            ReleaseCapture();
        }
        if (m_dragMoved) {
            CardMotion* motion = Motion(id, true);
            if (std::fabs(motion->offset) > kNotificationsWidth * m_scale * 0.3) {
                Remove(id, motion->offset > 0 ? 1 : -1);
            } else {
                motion->target = 0;
                StartIslandAnimation();
            }
            return true;
        }
    }
    const Hit hit = HitTest(pt);
    switch (hit.part) {
        case Part::Link:
            Close();
            PressKeys({VK_LWIN, 'N'});
            return false;
        case Part::Card:
            if (hit.index >= 0 && hit.index < (int)m_notifications.size()) {
                const Notification n = m_notifications[hit.index];
                if (OpenNotification(n.appId, n.kept ? ToastReply{} : n.reply)) {
                    if (g_island) {
                        Island::MarkAppRead(n.appId);
                    }
                    ClearApp(n.appId);
                    Close();
                    return false;
                }
                m_glowId = n.id;
                m_glowStart = NowSeconds();
                StartIslandAnimation();
            }
            return true;
        case Part::ReplyButton:
            if (hit.index >= 0 && hit.index < (int)m_notifications.size()) {
                m_replyId = m_notifications[hit.index].id;
                m_replyText.clear();
                m_replyEdit.Reset();
                Layout();
                // The reply box opens under the card: brought into view.
                if (hit.index < (int)m_cards.size()) {
                    m_scroll.ShowRange(m_cards[hit.index].top,
                                       m_cards[hit.index].bottom);
                }
            }
            return true;
        case Part::ReplyField:
            // (The caret went there as it was pressed.)
            return true;
        case Part::ReplySend:
            SendReply();
            return true;
        case Part::Dismiss:
            if (hit.index >= 0 && hit.index < (int)m_notifications.size()) {
                Remove(m_notifications[hit.index].id, 1);
                m_hover = {};
            }
            return true;
        case Part::CardAction: {
            // Its own button, pressed like Windows does: the app's, a link,
            // or "Dismiss". Either way it's done with: it leaves the list.
            const int card = hit.index / 16;
            const size_t button = (size_t)(hit.index % 16);
            if (card >= 0 && card < (int)m_notifications.size() &&
                button < m_notifications[card].reply.actions.size()) {
                const Notification n = m_notifications[card];
                const ToastReply::Action& action = n.reply.actions[button];
                // "Snooze": it shows again in the pill a few minutes later
                // (it stays in the list meanwhile).
                if (action.activationType == L"system" &&
                    action.arguments == L"snooze") {
                    if (g_island) {
                        IslandToast toast{n.appId};
                        toast.id = n.id;
                        toast.title = n.title;
                        toast.text = n.text;
                        toast.fullText = n.fullText;
                        toast.reply = n.reply;
                        toast.urgent = n.urgent;
                        toast.sound = n.sound;
                        toast.loop = n.loop;
                        g_island->SnoozeToast(std::move(toast));
                        Island::MarkAppRead(n.appId);
                    }
                    m_hover = {};
                    return true;
                }
                if (action.activationType != L"system") {
                    ActivateToast(n.appId, action.arguments,
                                  action.activationType);
                }
                if (g_island) {
                    Island::MarkAppRead(n.appId);
                }
                Remove(n.id, 1);
                m_hover = {};
            }
            return true;
        }
        case Part::ClearAll: {
            // The cards in view slide away one after the other; then
            // everything that arrived until now is cleared.
            double delay = 0;
            const float top = m_scroll.Offset();
            const float bottom =
                top + (m_listViewport.bottom - m_listViewport.top);
            for (size_t i = 0; i < m_notifications.size() && i < m_cards.size();
                 i++) {
                if (m_cards[i].bottom < top || m_cards[i].top > bottom) {
                    continue;
                }
                Remove(m_notifications[i].id, 1, delay, false);
                delay += 0.045;
            }
            // Every one the list shows leaves Windows' center too (the
            // ones it doesn't show stay there).
            RemoveFromCenter(m_notifications);
            m_clearAllPending = true;
            m_hover = {};
            // Kept at once: everything that arrived until now (the clock,
            // not only what the list shows: some were dismissed already),
            // and the island's apps.
            FILETIME now;
            GetSystemTimeAsFileTime(&now);
            m_clearedUntil =
                std::max(m_clearedUntil,
                         (long long)(((ULONGLONG)now.dwHighDateTime << 32) |
                                     now.dwLowDateTime));
            for (const auto& n : m_notifications) {
                m_clearedUntil = std::max(m_clearedUntil, n.arrival);
            }
            m_dismissed.clear();
            SaveCleared();
            if (g_island) {
                ForEachIsland([&](Island& island) {
                    island.ClearAppsFromList({}, true);
                });
            }
            if (m_motions.empty()) {
                StartIslandAnimation();
            }
            return true;
        }
        case Part::SettingsButton:
            m_settingsOpen = !m_settingsOpen;
            StartIslandAnimation();
            return true;
        case Part::DndToggle:
            if (SetIslandQuiet(!m_doNotDisturb)) {
                m_doNotDisturb = !m_doNotDisturb;
                if (g_island) {
                    ForEachIsland([&](Island& island) {
                        island.OnDoNotDisturbChanged();
                    });
                }
            }
            StartIslandAnimation();
            return true;
        case Part::ContentToggle:
            g_showNotificationContent = !g_showNotificationContent;
            Wh_SetIntValue(L"notificationContent", g_showNotificationContent);
            StartIslandAnimation();
            return true;
        case Part::BannerToggle:
            SetIslandPref(IslandPref::Banner, !g_bannerEnabled);
            StartIslandAnimation();
            return true;
        case Part::MinimizedToggle:
            if (!g_bannerEnabled) {
                return false;
            }
            SetIslandPref(IslandPref::BannerMinimized, !g_bannerWhenMinimized);
            StartIslandAnimation();
            return true;
        case Part::None:
            break;
    }
    return false;
}

void NotificationPanel::SendReply() {
    for (const auto& n : m_notifications) {
        if (n.id != m_replyId || m_replyText.empty()) {
            continue;
        }
        const bool sent =
            ActivateToast(n.appId, n.reply.arguments, n.reply.activationType,
                          n.reply.inputId, m_replyText);
        IslandLog(L"Reply from the list: %s", sent ? L"sent" : L"no activator");
        break;
    }
    m_replyId = -1;
    m_replyText.clear();
    m_replyEdit.Reset();
    Layout();
}

// Typing a reply: Enter sends, Ctrl+V pastes.
bool NotificationPanel::OnChar(WCHAR c) {
    if (m_replyId < 0) {
        return false;
    }
    constexpr size_t kMaxReply = 2000;
    // Enter sends; Shift+Enter starts a new line.
    if (c == L'\r' && GetKeyState(VK_SHIFT) >= 0) {
        SendReply();
        return true;
    }
    if (!EditTextChar(m_replyText, m_replyEdit, c == L'\r' ? L'\n' : c,
                      kMaxReply, m_hwnd)) {
        return false;
    }
    // The box grows (or shrinks) with it, kept in view.
    Layout();
    for (size_t i = 0; i < m_notifications.size() && i < m_cards.size(); i++) {
        if (m_notifications[i].id == m_replyId) {
            m_scroll.ShowRange(m_cards[i].top, m_cards[i].bottom);
        }
    }
    return true;
}

// Escape leaves the reply box first.
bool NotificationPanel::OnKey(WPARAM key) {
    if (m_replyId >= 0 && EditTextKey(m_replyText, m_replyEdit, key)) {
        Layout();
        return true;
    }
    if (key == VK_ESCAPE && m_replyId >= 0) {
        m_replyId = -1;
        m_replyText.clear();
        m_replyEdit.Reset();
        Layout();
        return true;
    }
    return false;
}

bool NotificationPanel::OnWheel(int delta) {
    if (m_replyId >= 0 && m_hover.part == Part::ReplyField &&
        m_replyLines >= 5) {
        EditTextWheel(m_replyEdit, delta);
        return true;
    }
    // About one card a notch.
    if (!m_scroll.Wheel(delta,
                        std::round((kNotificationHeight + 8) * m_scale))) {
        return false;
    }
    m_hover = {};
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// The media controls.

void OpenWindowsMixer() {
    // This shortcut was added in Windows 11 23H2. On older versions it
    // could open the clipboard history instead.
    OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    using RtlGetVersion_t = LONG(WINAPI*)(OSVERSIONINFOW*);
    auto getVersion = (RtlGetVersion_t)GetProcAddress(
        GetModuleHandle(L"ntdll.dll"), "RtlGetVersion");
    if (getVersion && getVersion(&version) == 0 &&
        version.dwBuildNumber >= 22631) {
        INPUT keys[6]{};
        const WORD codes[] = {VK_LWIN, VK_CONTROL, 'V', 'V', VK_CONTROL, VK_LWIN};
        for (int i = 0; i < 6; i++) {
            keys[i].type = INPUT_KEYBOARD;
            keys[i].ki.wVk = codes[i];
            keys[i].ki.dwFlags = i >= 3 ? KEYEVENTF_KEYUP : 0;
            if (codes[i] == VK_LWIN) {
                keys[i].ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
            }
        }
        const UINT sent = SendInput(6, keys, sizeof(INPUT));
        if (sent == 6) {
            return;
        }
        // Also release modifiers after a partial insertion.
        INPUT release[3]{};
        for (int i = 0; i < 3; i++) {
            release[i].type = INPUT_KEYBOARD;
            release[i].ki.wVk = codes[2 - i];
            release[i].ki.dwFlags = KEYEVENTF_KEYUP |
                (codes[2 - i] == VK_LWIN ? KEYEVENTF_EXTENDEDKEY : 0);
        }
        SendInput(3, release, sizeof(INPUT));
    }
    ShellExecute(nullptr, L"open", L"sndvol.exe", nullptr, nullptr, SW_SHOWNORMAL);
}

constexpr float kMediaWidth = 400;
constexpr WCHAR kMediaAutomaticGlyph = 0xE8B1;  // Shuffle: Windows picks the source.

void MediaPanel::SetState(const MediaState& state) {
    const bool otherApp = state.appId != m_state.appId;
    const bool sourcesChanged = state.sources.size() != m_state.sources.size() ||
        !std::equal(state.sources.begin(), state.sources.end(),
                    m_state.sources.begin(), m_state.sources.end(),
                    [](const MediaSource& a, const MediaSource& b) {
                        return a.appId == b.appId && a.title == b.title;
                    });
    const bool durationChanged = (state.duration > 0) != (m_state.duration > 0);
    const bool changed = otherApp || state.title != m_state.title ||
                         state.artist != m_state.artist ||
                         state.thumbnailSerial != m_state.thumbnailSerial ||
                         state.present != m_state.present;
    if (m_state.present && state.present &&
        m_state.playing != state.playing) {
        m_playSwap = 1;
        m_playSwapVelocity = 0;
        RequestFrame();
    }
    if (changed) {
        m_scrollStart = NowSeconds();
        if (IsOpen() && AnimationLevel() == 0) {
            m_previousMedia = m_state;
            m_previousThumbnail = m_thumbnail;
            m_mediaBlend = 0;
            m_mediaBlendVelocity = 0;
            RequestFrame();
        }
    }
    m_state = state;
    if (IsOpen()) {
        if (otherApp && m_dragging == Part::None && !m_press.held &&
            !m_volumeWriter.Busy()) {
            ReadVolume();
        } else if (otherApp) {
            m_playingApp = PlayingMixerApp();
        }
        if (changed || sourcesChanged || durationChanged) {
            Layout();
            PrepareAssets();
        }
        RequestFrame();
    }
}

void MediaPanel::Prepare(float scale) {
    if (!m_wic) {
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(m_wic.put()));
    }
    if (scale != m_preparedScale) {
        m_appIcons.clear();
        m_texts.clear();
        for (auto& app : m_apps) {
            app.bitmap = nullptr;
        }
        m_preparedScale = scale;
        m_titleFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_SEMI_BOLD,
                       14 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_textFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       12.5f * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_timeFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       11 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
        m_timeRightFormat =
            MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                       11 * scale, DWRITE_TEXT_ALIGNMENT_TRAILING);
        m_iconFormat = MakeFormat(IconFont(),
                                  DWRITE_FONT_WEIGHT_NORMAL, 18 * scale);
        m_bigIconFormat = MakeFormat(IconFont(),
                                     DWRITE_FONT_WEIGHT_NORMAL, 26 * scale);
        m_chipFormat = MakeFormat(L"Segoe UI Variable Text",
                                  DWRITE_FONT_WEIGHT_NORMAL, 11.5f * scale);
    }
    Layout();
    PrepareAssets();
}

void MediaPanel::OnOpen() {
    m_hover = Part::None;
    m_dragging = Part::None;
    m_press = {};
    m_scrollStart = NowSeconds();
    m_pillKnob = -1;
    m_titleKnob = -1;
    m_timelineHover = m_volumeHover = m_appVolumeHover = 0;
    m_scroll.Reset();
    m_mixerExpanded = false;
    m_mixerReveal = m_mixerRevealVelocity = 0;
    m_mediaBlend = 1;
    m_mediaBlendVelocity = 0;
    m_previousMedia = {};
    m_previousThumbnail = nullptr;
    m_tintReady = false;
    m_deviceMute = m_deviceLevel = -1;
    m_playingHover = m_playingHoverVelocity = 0;
    m_hoverApp = m_dragApp = -1;
    if (!m_volumeWriter.Busy()) {
        ReadVolume();
    }
    // (Laid out before by Prepare, with the apps' volumes of the last time.)
    Layout();
    PrepareAssets();
}

bool MediaPanel::AnimateContents(double dt) {
    bool moving = m_press.Animate(NowSeconds());
    moving |= m_scroll.Animate(dt);
    const double revealBefore = m_mixerReveal;
    moving |= SpringTowards(m_mixerReveal, m_mixerRevealVelocity,
                            m_mixerExpanded ? 1 : 0, 0.36, dt);
    if (m_mixerReveal != revealBefore) {
        LayoutMixer();
    }
    moving |= SpringDecoration(m_mediaBlend, m_mediaBlendVelocity, 1, 0.42, dt);
    if (m_mediaBlend == 1 &&
        (m_previousMedia.present || m_previousThumbnail)) {
        m_previousThumbnail = nullptr;
        m_previousMedia = {};
    }
    if (m_tintReady) {
        const double wanted[] = {m_coverColor.r, m_coverColor.g,
                                  m_coverColor.b, m_coverColor.a};
        for (int n = 0; n < 4; n++) {
            moving |= SpringDecoration(m_tintShown[n], m_tintVelocity[n],
                                        wanted[n], 0.55, dt);
        }
    }
    if (m_deviceMute < 0) {
        m_deviceMute = m_muted ? 1 : 0;
    }
    if (m_deviceLevel < 0) {
        m_deviceLevel = m_muted ? 0 : m_level;
    }
    moving |= SpringDecoration(m_deviceMute, m_deviceMuteVelocity,
                                m_muted ? 1 : 0, 0.28, dt);
    moving |= SpringDecoration(m_deviceLevel, m_deviceLevelVelocity,
                                m_muted ? 0 : m_level, 0.25, dt);
    for (size_t n = 0; n < m_apps.size() && n < m_mixerMotion.size(); n++) {
        auto& motion = m_mixerMotion[n];
        moving |= SpringDecoration(motion.mute, motion.muteVelocity,
                                    m_apps[n].muted ? 1 : 0, 0.28, dt);
        moving |= SpringDecoration(motion.level, motion.levelVelocity,
                                    m_apps[n].muted ? 0 : m_apps[n].volume, 0.25, dt);
    }
    const bool playingHover = m_hover == Part::PlayingVolume ||
                              m_dragging == Part::PlayingVolume;
    moving |= SpringTowards(m_playingHover, m_playingHoverVelocity,
                             playingHover ? 1 : 0, 0.25, dt);
    if (SpringDecoration(m_playSwap, m_playSwapVelocity, 0, 0.4, dt,
                      kBounceDamping)) {
        moving = true;
    }
    const double knob = g_mediaButtonsInPill ? 1 : 0;
    if (m_pillKnob < 0) {
        m_pillKnob = knob;
    }
    if (SpringTowards(m_pillKnob, m_pillKnobVelocity, knob, 0.3, dt,
                      kBounceDamping)) {
        moving = true;
    }
    const double titleKnob = g_mediaTitleInPill ? 1 : 0;
    if (m_titleKnob < 0) {
        m_titleKnob = titleKnob;
    }
    if (SpringTowards(m_titleKnob, m_titleKnobVelocity, titleKnob, 0.3, dt,
                      kBounceDamping)) {
        moving = true;
    }
    const bool timeline =
        m_hover == Part::Timeline || m_dragging == Part::Timeline;
    if (SpringTowards(m_timelineHover, m_timelineHoverVelocity,
                      timeline ? 1 : 0, 0.25, dt)) {
        moving = true;
    }
    const bool volume = m_hover == Part::Volume || m_dragging == Part::Volume;
    if (SpringTowards(m_volumeHover, m_volumeHoverVelocity, volume ? 1 : 0,
                      0.25, dt)) {
        moving = true;
    }
    const bool appVolume =
        m_hover == Part::AppVolume || m_dragging == Part::AppVolume;
    if (SpringTowards(m_appVolumeHover, m_appVolumeHoverVelocity,
                      appVolume ? 1 : 0, 0.25, dt)) {
        moving = true;
    }
    // A text scrolling never stops (see m_idleMotion).
    m_decorating = moving;
    m_idleMotion = m_scrolling && !moving;
    return moving || m_scrolling;
}

// Whether a process is the app whose media it is: the app's ID is its
// program's name (with or without ".exe") or path, or, for a Store app, its
// package family and app ("Family_publisher!App"), whose programs run from
// the package's folder.
bool ProcessIsMediaApp(DWORD processId, const std::wstring& appId) {
    if (!processId || appId.empty()) {
        return false;
    }
    HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process) {
        return false;
    }
    WCHAR path[MAX_PATH];
    DWORD length = ARRAYSIZE(path);
    const bool named = QueryFullProcessImageName(process, 0, path, &length);
    CloseHandle(process);
    if (!named) {
        return false;
    }
    std::wstring lowerPath = path;
    std::wstring id = appId;
    for (auto* text : {&lowerPath, &id}) {
        std::transform(text->begin(), text->end(), text->begin(), towlower);
    }
    if (const size_t bang = id.find(L'!'); bang != std::wstring::npos) {
        const std::wstring family = id.substr(0, bang);
        const size_t underscore = family.find(L'_');
        return underscore != std::wstring::npos &&
               lowerPath.find(L"\\windowsapps\\" +
                              family.substr(0, underscore) + L"_") !=
                   std::wstring::npos;
    }
    const size_t slash = lowerPath.find_last_of(L'\\');
    const std::wstring file =
        slash == std::wstring::npos ? lowerPath : lowerPath.substr(slash + 1);
    return file == id || file == id + L".exe" || lowerPath == id;
}

BOOL CALLBACK FindMediaWindowProc(HWND hWnd, LPARAM lParam) {
    auto* search = (AppWindowSearch*)lParam;
    if (!IsWindowVisible(hWnd) || GetWindow(hWnd, GW_OWNER) ||
        (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) ||
        !GetWindowTextLength(hWnd)) {
        return TRUE;
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(hWnd, &processId);
    if (WindowMatchesApp(hWnd, *search->appId) ||
        ProcessIsMediaApp(processId, *search->appId)) {
        search->found = hWnd;
        return FALSE;
    }
    return TRUE;
}

// A window of what's playing's app on the screen, or null.
HWND FindMediaWindow(const std::wstring& appId) {
    AppWindowSearch search{&appId, nullptr};
    EnumWindows(FindMediaWindowProc, (LPARAM)&search);
    return search.found;
}

// A Store app started by its ID (started again while it runs, it shows its
// window).
void LaunchMediaApp(const std::wstring& appId) {
    if (appId.find(L'!') != std::wstring::npos) {
        const std::wstring target = L"shell:AppsFolder\\" + appId;
        ShellExecute(nullptr, L"open", target.c_str(), nullptr, nullptr,
                     SW_SHOWNORMAL);
    }
}

// What's playing's app brought to the front (its cover clicked): its
// window, the app hidden in the tray (its icon clicked: true, for the
// island to see whether it came, see Island::WatchMediaAppOpen), or a Store
// app started by its ID.
bool OpenMediaApp(const std::wstring& appId) {
    if (appId.empty()) {
        return false;
    }
    if (HWND window = FindMediaWindow(appId)) {
        ActivateApp(window);
        return false;
    }
    if (OpenFromTrayIcon(appId)) {
        return true;
    }
    LaunchMediaApp(appId);
    return false;
}

