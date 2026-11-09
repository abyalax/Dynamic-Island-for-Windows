#pragma once
std::vector<std::pair<std::wstring, int>> NotificationPanel::CountsInCenter() {
    std::vector<std::pair<std::wstring, int>> counts;
    bool whatsApp = false;
    ReadCenter(
        "SELECT h.PrimaryId, COUNT(*) FROM Notification n JOIN "
        "NotificationHandler h ON h.RecordId = n.HandlerId WHERE n.Type = "
        "'toast' GROUP BY h.PrimaryId",
        [&](void* statement) {
            if (auto app = (PCWSTR)m_sqlite.column_text16(statement, 0)) {
                counts.push_back(
                    {app, (int)m_sqlite.column_int64(statement, 1)});
                whatsApp = whatsApp || StrStrI(app, L"WhatsApp");
            }
        });
    // WhatsApp's: counted by the messages each one says it has.
    if (whatsApp) {
        std::vector<std::pair<std::wstring, int>> messages;
        ReadCenter(
            "SELECT h.PrimaryId, n.Payload FROM Notification n JOIN "
            "NotificationHandler h ON h.RecordId = n.HandlerId WHERE n.Type = "
            "'toast' AND h.PrimaryId LIKE '%WhatsApp%'",
            [&](void* statement) {
                auto app = (PCWSTR)m_sqlite.column_text16(statement, 0);
                auto payload = (const char*)m_sqlite.column_blob(statement, 1);
                if (!app || !payload) {
                    return;
                }
                const int count = UnreadInToastTexts(
                    ToastTexts(payload, m_sqlite.column_bytes(statement, 1)));
                auto it = std::find_if(
                    messages.begin(), messages.end(),
                    [&](const auto& entry) { return entry.first == app; });
                if (it == messages.end()) {
                    messages.push_back({app, count});
                } else {
                    it->second += count;
                }
            });
        for (const auto& [app, count] : messages) {
            for (auto& entry : counts) {
                if (entry.first == app) {
                    entry.second = std::max(entry.second, count);
                }
            }
        }
    }
    // An app's notifications under IDs of their own (see
    // NotificationAppKey): their counts together, for the app they're shown
    // under.
    std::vector<std::pair<std::wstring, int>> grouped;
    for (const auto& [app, count] : counts) {
        const std::wstring key = NotificationAppKey(app);
        if (key == app) {
            continue;
        }
        auto it = std::find_if(grouped.begin(), grouped.end(),
                               [&](const auto& entry) {
                                   return entry.first == key;
                               });
        if (it == grouped.end()) {
            grouped.push_back({key, count});
        } else {
            it->second += count;
        }
    }
    for (const auto& [key, count] : grouped) {
        auto own = std::find_if(counts.begin(), counts.end(),
                                [&](const auto& entry) {
                                    return entry.first == key;
                                });
        if (own == counts.end()) {
            counts.push_back({key, count});
        } else {
            own->second += count;
        }
    }
    return counts;
}

std::vector<IslandToast> NotificationPanel::MissedToasts(long long seenId,
                                                         long long* newestId) {
    constexpr size_t kMaxMissedApps = 12;
    LoadCleared();
    // Newest first: the first one of each app.
    std::vector<std::pair<long long, std::wstring>> latest;
    const std::string query =
        "SELECT n.Id, h.PrimaryId, n.ArrivalTime FROM Notification n JOIN "
        "NotificationHandler h ON h.RecordId = n.HandlerId WHERE n.Type = "
        "'toast' AND n.Id > " +
        std::to_string(seenId) + " ORDER BY n.Id DESC LIMIT 200";
    ReadCenter(query.c_str(), [&](void* statement) {
        const long long id = m_sqlite.column_int64(statement, 0);
        *newestId = std::max(*newestId, id);
        auto app = (PCWSTR)m_sqlite.column_text16(statement, 1);
        if (!app || latest.size() >= kMaxMissedApps ||
            m_sqlite.column_int64(statement, 2) <= m_clearedUntil ||
            std::find(m_dismissed.begin(), m_dismissed.end(), id) !=
                m_dismissed.end() ||
            std::any_of(latest.begin(), latest.end(), [&](const auto& entry) {
                return SameNotificationApp(entry.second, app);
            })) {
            return;
        }
        latest.push_back({id, app});
    });
    std::vector<IslandToast> toasts;
    const std::string path = GetNotificationDatabasePath();
    for (auto it = latest.rbegin(); it != latest.rend(); ++it) {
        long long lastId = it->first - 1;
        std::vector<std::wstring> apps;
        ReadNewToasts(m_sqlite, path, lastId, apps, toasts, it->first, false);
    }
    return toasts;
}

// How long a card glows (see FocusApp).
constexpr double kCardGlowSeconds = 1.6;

void NotificationPanel::ApplyFocus() {
    if (m_focusApp.empty()) {
        return;
    }
    for (size_t i = 0; i < m_notifications.size() && i < m_cards.size(); i++) {
        if (SameNotificationApp(m_notifications[i].appId, m_focusApp)) {
            m_scroll.ShowRange(m_cards[i].top, m_cards[i].bottom);
            m_glowApp = m_notifications[i].appId;
            m_glowId = -1;
            m_glowStart = NowSeconds();
            StartIslandAnimation();
            break;
        }
    }
    m_focusApp.clear();
}

void NotificationPanel::OnOpen() {
    if (m_clearAllPending || !m_motions.empty()) {
        m_clearAllPending = false;
        m_motions.clear();
        Load();
        Layout();
    }
    m_scroll.Reset();
    m_glowStart = -10;
    ApplyFocus();
    // The new ones are seen now (the bell's dot goes).
    if (g_island) {
        ForEachIsland([&](Island& island) { island.OnNotificationsSeen(); });
    }
    m_hover = {};
    m_replyId = -1;
    m_replyText.clear();
    m_replyEdit.Reset();
    // Folded, or unfolded at once (see ShowSettings).
    m_settingsOpen = m_openOnSettings;
    m_openOnSettings = false;
    if (m_settingsAmount != (m_settingsOpen ? 1 : 0)) {
        m_settingsAmount = m_settingsOpen ? 1 : 0;
        m_settingsVelocity = 0;
        Layout();
    }
    // The switches start where they are.
    m_dndKnob = IsIslandQuiet() ? 1 : 0;
    m_contentKnob = g_showNotificationContent ? 1 : 0;
    m_bannerKnob = g_bannerEnabled ? 1 : 0;
    m_minimizedKnob = g_bannerWhenMinimized ? 1 : 0;
    m_dndKnobVelocity = m_contentKnobVelocity = 0;
    m_bannerKnobVelocity = m_minimizedKnobVelocity = 0;
}

// What was cleared, kept across restarts.
void NotificationPanel::LoadCleared() {
    if (m_clearedLoaded) {
        return;
    }
    m_clearedLoaded = true;
    m_clearedUntil =
        _wcstoi64(GetUserStringValue(L"notificationsClearedUntil", 64).c_str(),
                  nullptr, 10);
    // (An id is at most 19 digits, and a comma.)
    const std::wstring ids =
        GetUserStringValue(L"notificationsDismissed", kMaxDismissed * 20 + 1);
    for (PCWSTR p = ids.c_str(); *p;) {
        PWSTR end = nullptr;
        const long long id = _wcstoi64(p, &end, 10);
        if (end == p) {
            break;
        }
        m_dismissed.push_back(id);
        p = *end == L',' ? end + 1 : end;
    }
    const std::wstring texts =
        GetUserStringValue(L"notificationsDismissedTags",
                           kMaxDismissedTexts * (kDismissKeyMax + 1) + 1);
    for (PCWSTR line = texts.c_str(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        m_dismissedTexts.push_back(end ? std::wstring(line, end)
                                       : std::wstring(line));
        if (!end) {
            break;
        }
        line = end + 1;
    }
}

void NotificationPanel::SaveCleared() {
    if (m_dismissed.size() > kMaxDismissed) {
        m_dismissed.erase(m_dismissed.begin(),
                          m_dismissed.end() - kMaxDismissed);
    }
    SetUserStringValue(L"notificationsClearedUntil",
                       std::to_wstring(m_clearedUntil).c_str());
    std::wstring dismissed;
    for (long long id : m_dismissed) {
        dismissed += (dismissed.empty() ? L"" : L",") + std::to_wstring(id);
    }
    SetUserStringValue(L"notificationsDismissed", dismissed.c_str());
    if (m_dismissedTexts.size() > kMaxDismissedTexts) {
        m_dismissedTexts.erase(m_dismissedTexts.begin(),
                               m_dismissedTexts.end() - kMaxDismissedTexts);
    }
    std::wstring texts;
    for (const auto& key : m_dismissedTexts) {
        texts += key + L"\n";
    }
    SetUserStringValue(L"notificationsDismissedTags", texts.c_str());
}

// The latest toasts, newest first, without the ones cleared here.
void NotificationPanel::Load() {
    LoadCleared();
    m_doNotDisturb = IsIslandQuiet();
    m_notifications.clear();
    if (!m_sqliteLoaded) {
        m_sqliteLoaded = true;
        if (!m_sqlite.Load()) {
            m_sqlite.Unload();
        }
    }
    if (!m_sqlite.module || !m_sqlite.open_v2) {
        return;
    }
    const std::string path = GetNotificationDatabasePath();
    void* db = nullptr;
    if (path.empty() ||
        m_sqlite.open_v2(path.c_str(), &db, Sqlite::kOpenReadOnly, nullptr) !=
            0) {
        if (db) {
            m_sqlite.close(db);
        }
        return;
    }
    m_sqlite.busy_timeout(db, 500);
    void* statement = nullptr;
    if (m_sqlite.prepare_v2(
            db,
            "SELECT n.Id, h.PrimaryId, n.Payload, n.ArrivalTime, n.Tag, "
            "n.\"Group\" FROM Notification n JOIN NotificationHandler h ON "
            "h.RecordId = n.HandlerId WHERE n.Type = 'toast' AND "
            "n.ArrivalTime > ?1 ORDER BY n.ArrivalTime DESC LIMIT ?2",
            -1, &statement, nullptr) == 0) {
        m_sqlite.bind_int64(statement, 1, m_clearedUntil);
        m_sqlite.bind_int64(statement, 2,
                            kMaxNotifications + (long long)m_dismissed.size() +
                                (long long)m_dismissedTexts.size());
        while (m_sqlite.step(statement) == Sqlite::kRow &&
               (int)m_notifications.size() < kMaxNotifications) {
            Notification notification;
            notification.id = m_sqlite.column_int64(statement, 0);
            if (std::find(m_dismissed.begin(), m_dismissed.end(),
                          notification.id) != m_dismissed.end()) {
                continue;
            }
            if (auto app = (PCWSTR)m_sqlite.column_text16(statement, 1)) {
                notification.appId = app;
            }
            const auto payload = (const char*)m_sqlite.column_blob(statement, 2);
            const int size = m_sqlite.column_bytes(statement, 2);
            notification.arrival = m_sqlite.column_int64(statement, 3);
            if (auto tag = (PCWSTR)m_sqlite.column_text16(statement, 4)) {
                notification.tag = tag;
            }
            if (auto group = (PCWSTR)m_sqlite.column_text16(statement, 5)) {
                notification.group = group;
            }
            const auto texts = payload ? ToastTexts(payload, size)
                                       : std::vector<std::wstring>{};
            SplitToastTexts(texts, &notification.title, &notification.text,
                            &notification.fullText);
            if (payload) {
                notification.reply =
                    ParseToastReply(notification.appId, payload, size);
                // "Snooze" only where the island shows it again (with its
                // own notifications; otherwise Windows' banner has it).
                if (!WindowsQuietForIsland()) {
                    std::erase_if(notification.reply.actions,
                                  [](const ToastReply::Action& action) {
                                      return action.activationType ==
                                                 L"system" &&
                                             action.arguments == L"snooze";
                                  });
                }
                notification.urgent = IsUrgentToast(payload, size);
                notification.sound = ToastSound(payload, size);
                notification.loop = ToastSoundLoops(payload, size);
            }
            const std::wstring key = DismissKey(notification);
            const bool dismissed =
                !key.empty() &&
                std::find(m_dismissedTexts.begin(), m_dismissedTexts.end(),
                          key) != m_dismissedTexts.end();
            if ((!notification.title.empty() || !notification.text.empty()) &&
                !dismissed) {
                m_notifications.push_back(std::move(notification));
            }
        }
        m_sqlite.finalize(statement);
    }
    m_sqlite.close(db);
    MergeKeptToasts();
}

std::vector<std::pair<std::wstring, int>> NotificationPanel::KeptCounts() {
    LoadCleared();
    AcquireSRWLockShared(&g_badgeAppsLock);
    const std::vector<std::wstring> apps = g_badgeApps;
    ReleaseSRWLockShared(&g_badgeAppsLock);
    std::vector<std::pair<std::wstring, int>> counts;
    AcquireSRWLockShared(&g_keptToastsLock);
    for (const auto& kept : g_keptToasts) {
        const IslandToast& toast = kept.toast;
        if (toast.arrival <= m_clearedUntil ||
            std::find(m_dismissed.begin(), m_dismissed.end(), toast.id) !=
                m_dismissed.end() ||
            std::none_of(apps.begin(), apps.end(), [&](const std::wstring& id) {
                return _wcsicmp(id.c_str(), toast.appId.c_str()) == 0;
            })) {
            continue;
        }
        auto it = std::find_if(counts.begin(), counts.end(),
                               [&](const auto& entry) {
                                   return _wcsicmp(entry.first.c_str(),
                                                   toast.appId.c_str()) == 0;
                               });
        if (it == counts.end()) {
            counts.push_back({toast.appId, 1});
        } else {
            it->second++;
        }
    }
    ReleaseSRWLockShared(&g_keptToastsLock);
    return counts;
}

// The kept notifications (see g_keptToasts) their app took out of Windows'
// list while its taskbar button still counts them: back in the list, the
// newest ones, as many as the count says (all of them without a number).
// Forgotten once the count is gone (a few seconds after they came: the count
// may come after them).
void NotificationPanel::MergeKeptToasts() {
    AcquireSRWLockShared(&g_badgeAppsLock);
    const std::vector<std::wstring> apps = g_badgeApps;
    const std::vector<int> counts = g_badgeCounts;
    ReleaseSRWLockShared(&g_badgeAppsLock);
    // The app's count, 0 without a number, -1 without a badge.
    auto countOf = [&](const std::wstring& appId) {
        for (size_t i = 0; i < apps.size(); i++) {
            if (_wcsicmp(apps[i].c_str(), appId.c_str()) == 0) {
                return i < counts.size() ? counts[i] : 0;
            }
        }
        return -1;
    };
    const double now = NowSeconds();
    bool added = false;
    AcquireSRWLockExclusive(&g_keptToastsLock);
    std::erase_if(g_keptToasts, [&](const KeptToast& kept) {
        return countOf(kept.toast.appId) < 0 && now - kept.since > 10;
    });
    for (auto it = g_keptToasts.rbegin(); it != g_keptToasts.rend(); ++it) {
        const IslandToast& toast = it->toast;
        const int count = countOf(toast.appId);
        auto sameApp = [&](const Notification& n) {
            return _wcsicmp(n.appId.c_str(), toast.appId.c_str()) == 0;
        };
        if (count < 0 || toast.arrival <= m_clearedUntil ||
            std::find(m_dismissed.begin(), m_dismissed.end(), toast.id) !=
                m_dismissed.end() ||
            std::any_of(m_notifications.begin(), m_notifications.end(),
                        [&](const Notification& n) {
                            return n.id == toast.id;
                        }) ||
            (count > 0 &&
             std::count_if(m_notifications.begin(), m_notifications.end(),
                           sameApp) >= count)) {
            continue;
        }
        Notification notification;
        notification.id = toast.id;
        notification.appId = toast.appId;
        notification.title = toast.title;
        notification.text = toast.text;
        notification.fullText = toast.fullText;
        notification.arrival = toast.arrival;
        notification.reply = toast.reply;
        notification.urgent = toast.urgent;
        notification.sound = toast.sound;
        notification.loop = toast.loop;
        notification.kept = true;
        m_notifications.push_back(std::move(notification));
        added = true;
    }
    ReleaseSRWLockExclusive(&g_keptToastsLock);
    if (added) {
        std::stable_sort(m_notifications.begin(), m_notifications.end(),
                         [](const Notification& a, const Notification& b) {
                             return a.arrival > b.arrival;
                         });
        if ((int)m_notifications.size() > kMaxNotifications) {
            m_notifications.resize(kMaxNotifications);
        }
    }
}

void NotificationPanel::Layout() {
    const float s = m_scale;
    const float width = std::round(kNotificationsWidth * s);
    const float padding = std::round(12 * s);
    const float gap = std::round(8 * s);
    float y = padding;
    // "Notifications"  ...  "Clear all".
    const float header = std::round(kHeaderHeight * s);
    m_header = {padding + std::round(4 * s), y,
                width - padding - std::round(4 * s), y + header};
    m_clearAll = {m_header.right - std::round(110 * s), y, m_header.right,
                  y + header};
    // The gear right after the title ("Notifications  ·  3").
    const float gearSize = std::round(30 * s);
    float titleWidth = std::round(110 * s);
    {
        std::wstring title = Tr(L"Notifications", L"Notificações");
        if (m_notifications.size() > 1) {
            title += L"  ·  " + std::to_wstring(m_notifications.size());
        }
        winrt::com_ptr<IDWriteTextLayout> measure;
        if (m_headerFormat &&
            SUCCEEDED(m_dwrite->CreateTextLayout(
                title.c_str(), (UINT32)title.size(), m_headerFormat.get(),
                10000, header, measure.put()))) {
            DWRITE_TEXT_METRICS metrics{};
            measure->GetMetrics(&metrics);
            titleWidth = std::ceil(metrics.widthIncludingTrailingWhitespace);
        }
    }
    const float gearLeft =
        std::min(m_header.left + titleWidth + std::round(4 * s),
                 m_clearAll.left - std::round(6 * s) - gearSize);
    m_settingsButton = {gearLeft, y + (header - gearSize) / 2,
                        gearLeft + gearSize, y + (header + gearSize) / 2};
    y += header;
    const float sectionTop = y;
    // "Do not disturb" with its switch.
    // Taller rows: a name and what it does.
    const float row = std::round(kSwitchRowHeight * s);
    m_dndRow = {padding, y, width - padding, y + row};
    const float toggleWidth = std::round(36 * s);
    const float toggleHeight = std::round(20 * s);
    m_dndToggle = {m_dndRow.right - std::round(10 * s) - toggleWidth,
                   std::round((m_dndRow.top + m_dndRow.bottom - toggleHeight) /
                              2),
                   m_dndRow.right - std::round(10 * s),
                   std::round((m_dndRow.top + m_dndRow.bottom - toggleHeight) /
                              2) +
                       toggleHeight};
    // The other switches, under it.
    // (The pill's banners and their minimized case share a card.)
    auto nextRow = [&](D2D1_RECT_F* rowRect, D2D1_RECT_F* toggle,
                       bool sameCard = false) {
        y += row + (sameCard ? 0 : std::round(4 * s));
        *rowRect = {padding, y, width - padding, y + row};
        *toggle = {m_dndToggle.left, m_dndToggle.top + (y - m_dndRow.top),
                   m_dndToggle.right, m_dndToggle.bottom + (y - m_dndRow.top)};
    };
    nextRow(&m_bannerRow, &m_bannerToggle);
    nextRow(&m_minimizedRow, &m_minimizedToggle, true);
    nextRow(&m_contentRow, &m_contentToggle);
    y += row + gap;
    // Folded: only as much of it as is open (drawn clipped to it).
    const float folded = (float)std::clamp(m_settingsAmount, 0.0, 1.0);
    y = sectionTop + (y - sectionTop) * folded;
    m_settingsSection = {padding, sectionTop, width - padding, y};

    // The cards, in a list of their own that scrolls.
    m_cards.clear();
    const float card = std::round(kNotificationHeight * s);
    const float replyRow = std::round(kBannerReplyHeight * s);
    // How many lines each text takes, and the reply being typed.
    const float inner = std::round(12 * s);
    const float textWidth = width - padding * 2 - inner * 2;
    for (const auto& n : m_notifications) {
        if (std::none_of(m_cardLines.begin(), m_cardLines.end(),
                         [&](const auto& entry) { return entry.first == n.id; })) {
            m_cardLines.push_back(
                {n.id, n.fullText.empty() ? 1
                                      : WrappedLineCount(
                                            m_dwrite, m_textFormat.get(),
                                            n.fullText, textWidth,
                                            std::round(19 * s))});
        }
    }
    if (m_cardLines.size() > 200) {
        m_cardLines.erase(m_cardLines.begin(), m_cardLines.end() - 100);
    }
    m_replyLines = 1;
    if (m_replyId >= 0 && !m_replyText.empty()) {
        const std::wstring reply =
            m_replyText + (m_replyText.back() == L'\n' ? L" " : L"");
        m_replyLines = std::min(
            5, WrappedLineCount(m_dwrite, m_textFormat.get(), reply,
                                textWidth - inner - std::round(32 * s),
                                std::round(17 * s)));
    }
    float listY = 0;
    for (const auto& n : m_notifications) {
        // The one being answered has its reply box under it; one being
        // removed closes its room.
        float room = 1;
        for (const auto& motion : m_motions) {
            if (motion.id == n.id) {
                room = (float)std::clamp(motion.room, 0.0, 1.0);
            }
        }
        const float height =
            (card + CardExtra(n.id) +
             (n.id == m_replyId ? replyRow + ReplyExtra() : 0)) *
            room;
        m_cards.push_back({padding, listY, width - padding, listY + height});
        listY += (height + gap) * room;
    }
    // "No notifications" (room for it kept while the last cards go, so the
    // panel doesn't shrink and grow again).
    const float content = std::max(std::round(40 * s), listY - gap);
    // Up to half of the screen's height (and the room the panel has).
    const float link = std::round(kFooterHeight * s);
    const float half = (float)(m_monitorRect.bottom - m_monitorRect.top) / 2;
    const float room = std::min(half > 0 ? half : content,
                                MaxHeight() - y - gap - link - padding);
    const float viewport =
        std::min(content, std::max(room, std::round(120 * s)));
    m_listViewport = {0, y, width, y + viewport};
    m_scroll.SetSizes(viewport, content);
    y += viewport + gap;
    m_link = {padding, y, width - padding, y + link};
    y += link + padding / 2;
    m_size = {(LONG)width, (LONG)std::ceil(y)};
}

void NotificationPanel::CardParts(const D2D1_RECT_F& card,
                                  float extra,
                                  D2D1_RECT_F* replyButton,
                                  D2D1_RECT_F* field,
                                  D2D1_RECT_F* send) const {
    const float s = m_scale;
    const float inner = std::round(12 * s);
    const float base = card.top + std::round(kNotificationHeight * s) + extra;
    const float buttonHeight = std::round(24 * s);
    *replyButton = {card.right - inner - std::round(84 * s),
                    base - inner - buttonHeight + std::round(4 * s),
                    card.right - inner, base - inner + std::round(4 * s)};
    const float fieldHeight = std::round(32 * s);
    *field = {card.left + inner, base, card.right - inner,
              base + fieldHeight + ReplyExtra()};
    *send = {field->right - fieldHeight, field->bottom - fieldHeight,
             field->right, field->bottom};
}

D2D1_RECT_F NotificationPanel::CardActionRect(const D2D1_RECT_F& card,
                                              float extra,
                                              const Notification& n,
                                              size_t index) const {
    D2D1_RECT_F replyButton, field, send;
    CardParts(card, extra, &replyButton, &field, &send);
    const float s = m_scale;
    const float inner = std::round(12 * s);
    const float gap = std::round(6 * s);
    const float right =
        n.reply.CanReply() ? replyButton.left - gap : replyButton.right;
    const float count = (float)std::max<size_t>(1, n.reply.actions.size());
    const float width =
        std::min(std::round(96 * s),
                 (right - card.left - inner - gap * (count - 1)) / count);
    const float left = right - width * count - gap * (count - 1) +
                       (width + gap) * index;
    return {std::round(left), replyButton.top, std::round(left + width),
            replyButton.bottom};
}

long long NotificationPanel::HoveredCard() const {
    if ((m_hover.part == Part::Card || m_hover.part == Part::Dismiss ||
         m_hover.part == Part::ReplyButton) &&
        m_hover.index >= 0 && m_hover.index < (int)m_notifications.size()) {
        return m_notifications[m_hover.index].id;
    }
    if (m_hover.part == Part::CardAction && m_hover.index >= 0 &&
        m_hover.index / 16 < (int)m_notifications.size()) {
        return m_notifications[m_hover.index / 16].id;
    }
    return -1;
}

float NotificationPanel::HoverAmount(long long id) const {
    for (const auto& hover : m_cardHovers) {
        if (hover.id == id) {
            return (float)std::clamp(hover.amount, 0.0, 1.0);
        }
    }
    return 0;
}

float NotificationPanel::CardExtra(long long id) const {
    const float hover = HoverAmount(id);
    if (hover <= 0) {
        return 0;
    }
    // Room under the text for "Reply" and the card's own buttons, so they
    // don't cover it.
    float extra = 0;
    for (const auto& n : m_notifications) {
        if (n.id == id && (n.reply.CanReply() || !n.reply.actions.empty()) &&
            id != m_replyId) {
            extra += std::round(32 * m_scale);
        }
    }
    if (g_showNotificationContent) {
        int lines = 1;
        for (const auto& [card, count] : m_cardLines) {
            if (card == id) {
                lines = std::min(count, 3);
            }
        }
        extra += (lines - 1) * std::round(19 * m_scale);
    }
    return extra * hover;
}

NotificationPanel::Hit NotificationPanel::HitTest(POINT panelPt) const {
    // The cards, in the list (where it's shown).
    const POINT pt = ToList(panelPt);
    for (size_t i = 0; i < m_cards.size() && i < m_notifications.size() &&
                       PointInRect(panelPt, m_listViewport);
         i++) {
        if (!PointInRect(pt, m_cards[i])) {
            continue;
        }
        const Notification& n = m_notifications[i];
        if (n.id != m_replyId && HoverAmount(n.id) > 0.5f &&
            !(g_island && g_island->IsToastSnoozed(n.id))) {
            for (size_t a = 0; a < n.reply.actions.size() && a < 16; a++) {
                if (PointInRect(pt, CardActionRect(m_cards[i], CardExtra(n.id),
                                                   n, a))) {
                    return {Part::CardAction, (int)(i * 16 + a)};
                }
            }
        }
        if (n.reply.CanReply()) {
            D2D1_RECT_F replyButton, field, send;
            CardParts(m_cards[i], CardExtra(n.id), &replyButton, &field, &send);
            if (n.id == m_replyId) {
                if (PointInRect(pt, send) && !m_replyText.empty()) {
                    return {Part::ReplySend, (int)i};
                }
                if (PointInRect(pt, field)) {
                    return {Part::ReplyField, (int)i};
                }
            } else if (PointInRect(pt, replyButton)) {
                return {Part::ReplyButton, (int)i};
            }
        }
        // The "x" at the top right of the card.
        const float size = std::round(28 * m_scale);
        if (pt.x >= m_cards[i].right - size && pt.y < m_cards[i].top + size) {
            return {Part::Dismiss, (int)i};
        }
        return {Part::Card, (int)i};
    }
    // The rest isn't scrolled.
    if (!m_notifications.empty() && PointInRect(panelPt, m_clearAll)) {
        return {Part::ClearAll};
    }
    if (PointInRect(panelPt, m_settingsButton)) {
        return {Part::SettingsButton};
    }
    // The switches, only while unfolded.
    if (m_settingsAmount < 0.9 || !PointInRect(panelPt, m_settingsSection)) {
        if (PointInRect(panelPt, m_link)) {
            return {Part::Link};
        }
        return {};
    }
    if (PointInRect(panelPt, m_dndRow)) {
        return {Part::DndToggle};
    }
    if (PointInRect(panelPt, m_contentRow)) {
        return {Part::ContentToggle};
    }
    // (The banners are the main island's: dimmed on the others.)
    if (!OtherIslandShown() && PointInRect(panelPt, m_bannerRow)) {
        return {Part::BannerToggle};
    }
    if (!OtherIslandShown() && PointInRect(panelPt, m_minimizedRow)) {
        return {Part::MinimizedToggle};
    }
    if (PointInRect(panelPt, m_link)) {
        return {Part::Link};
    }
    return {};
}

// An app's name, as the Start menu shows it, by its AppUserModelID (as
// Windows' notification center shows it for a sender without a Start menu
// entry); remembered.
std::wstring AppDisplayName(const std::wstring& appId) {
    static std::vector<std::pair<std::wstring, std::wstring>> names;
    for (const auto& [id, name] : names) {
        if (id == appId) {
            return name;
        }
    }
    if (appId == kWidgetsBadgeId) {
        return Tr(L"Widgets", L"Widgets");
    }
    std::wstring name;
    winrt::com_ptr<IShellItem> item;
    if (!appId.empty() &&
        SUCCEEDED(SHCreateItemInKnownFolder(kFOLDERID_AppsFolder, 0,
                                            appId.c_str(),
                                            IID_PPV_ARGS(item.put())))) {
        PWSTR display = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_NORMALDISPLAY, &display))) {
            name = display;
            CoTaskMemFree(display);
        }
    }
    // A sender without one (Windows' own, like Windows Security's
    // "Windows.Defender.SecurityCenter"): the name it registered (not a
    // file's resource left unresolved).
    if (name.empty()) {
        name = RegisteredAppValue(appId, L"DisplayName");
        if (name.find(L":\\") != std::wstring::npos) {
            name.clear();
        }
    }
    // Only a program's file name ("Spotify.exe"): its own name while it
    // runs ("Windows PowerShell"), else the file's name without ".exe".
    if (name.empty() && appId.find(L'\\') == std::wstring::npos &&
        appId.size() > 4 &&
        _wcsicmp(appId.c_str() + appId.size() - 4, L".exe") == 0) {
        if (const std::wstring path = RunningProgramPath(appId);
            !path.empty()) {
            name = GetProgramName(path.c_str());
        }
        if (name.empty()) {
            name = appId.substr(0, appId.size() - 4);
            name[0] = towupper(name[0]);
        }
    }
    if (name.empty()) {
        // "Company.App_hash!App" or a program's path: the readable part.
        name = appId;
        if (size_t slash = name.find_last_of(L'\\'); slash != name.npos) {
            name = name.substr(slash + 1);
            // "notepad.exe": the program's name.
            if (size_t dot = name.find_last_of(L'.'); dot != name.npos) {
                name = name.substr(0, dot);
            }
            if (!name.empty()) {
                name[0] = towupper(name[0]);
            }
            names.push_back({appId, name});
            return name;
        }
        if (size_t bang = name.find(L'!'); bang != name.npos) {
            name = name.substr(0, bang);
        }
        if (size_t underscore = name.find(L'_'); underscore != name.npos) {
            name = name.substr(0, underscore);
        }
        if (size_t dot = name.find_last_of(L'.'); dot != name.npos &&
                                                  dot + 1 < name.size()) {
            name = name.substr(dot + 1);
        }
    }
    names.push_back({appId, name});
    return name;
}

const std::wstring& NotificationPanel::AppName(const std::wstring& appId) {
    for (const auto& [id, name] : m_names) {
        if (id == appId) {
            return name;
        }
    }
    m_names.push_back({appId, AppDisplayName(appId)});
    return m_names.back().second;
}

// An app's icon (or a file's, a folder's) from the shell, in the island's
// process (see LoadAppIconBitmap), at `size` pixels.
winrt::com_ptr<ID2D1Bitmap> LoadShellIcon(ID2D1RenderTarget* target,
                                          IWICImagingFactory* wic,
                                          const std::wstring& appId,
                                          int size) {
    winrt::com_ptr<ID2D1Bitmap> bitmap;
    if (!target || !wic) {
        return bitmap;
    }
    if (HBITMAP icon = LoadAppIconBitmap(nullptr, appId, size)) {
        winrt::com_ptr<IWICBitmap> wicBitmap;
        winrt::com_ptr<IWICFormatConverter> converter;
        if (SUCCEEDED(wic->CreateBitmapFromHBITMAP(
                icon, nullptr, WICBitmapUsePremultipliedAlpha,
                wicBitmap.put())) &&
            SUCCEEDED(wic->CreateFormatConverter(converter.put())) &&
            SUCCEEDED(converter->Initialize(
                wicBitmap.get(), GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0,
                WICBitmapPaletteTypeMedianCut))) {
            target->CreateBitmapFromWicBitmap(converter.get(), nullptr,
                                              bitmap.put());
        }
        DeleteObject(icon);
    }
    return bitmap;
}

// An app's icon for the island and its panels, at `size` pixels: from
// explorer, which has the shell give the icons of packaged apps too (not
// always given in the island's process), or loaded here if it can't
// (`fromExplorer` says which).
winrt::com_ptr<ID2D1Bitmap> LoadAppIcon(ID2D1RenderTarget* target,
                                        IWICImagingFactory* wic,
                                        HWND from,
                                        const std::wstring& appId,
                                        int size,
                                        bool* fromExplorer = nullptr) {
    winrt::com_ptr<ID2D1Bitmap> bitmap;
    if (fromExplorer) {
        *fromExplorer = false;
    }
    if (!target || !wic) {
        return bitmap;
    }
    auto convert = [&](IWICBitmapSource* source) {
        winrt::com_ptr<IWICFormatConverter> converter;
        if (SUCCEEDED(wic->CreateFormatConverter(converter.put())) &&
            SUCCEEDED(converter->Initialize(
                source, GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0,
                WICBitmapPaletteTypeMedianCut))) {
            target->CreateBitmapFromWicBitmap(converter.get(), nullptr,
                                              bitmap.put());
        }
    };
    HICON taskbarIcon = nullptr;
    if (HWND taskbar = FindTaskbarWnd()) {
        IpcWriter writer;
        writer.Put(size);
        writer.PutString(appId);
        COPYDATASTRUCT copyData{kIpcAppIcon, (DWORD)writer.bytes.size(),
                                writer.bytes.data()};
        DWORD_PTR result = 0;
        // From the island's window (explorer only takes requests from it).
        if (SendMessageTimeout(taskbar, WM_COPYDATA,
                               (WPARAM)(g_islandWnd ? (HWND)g_islandWnd : from),
                               (LPARAM)&copyData,
                               SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 2000,
                               &result) &&
            result) {
            taskbarIcon = CopyIcon((HICON)result);
        }
    }
    if (taskbarIcon) {
        winrt::com_ptr<IWICBitmap> wicBitmap;
        if (SUCCEEDED(wic->CreateBitmapFromHICON(taskbarIcon,
                                                 wicBitmap.put()))) {
            convert(wicBitmap.get());
        }
        DestroyIcon(taskbarIcon);
        if (fromExplorer) {
            *fromExplorer = bitmap != nullptr;
        }
    }
    if (!bitmap) {
        bitmap = LoadShellIcon(target, wic, appId, size);
    }
    return bitmap;
}

// The icons of the apps under "Apps that look for lyrics", made as the
// settings are laid out (drawing only reads them); one not found asked again
// later and later (see AppIconRetryAt), the ones no longer listed let go.
void ControlPanel::PrepareLyricsAppIcons() {
    ID2D1RenderTarget* target = DrawingTarget();
    if (!target || !m_wic) {
        return;
    }
    const auto& apps = LyricsApps();
    std::erase_if(m_lyricsAppIcons, [&](const KeptAppIcon& icon) {
        return std::none_of(apps.begin(), apps.end(), [&](const LyricsApp& app) {
            return app.appId == icon.appId;
        });
    });
    const double now = NowSeconds();
    for (const auto& app : apps) {
        int misses = 0;
        const auto kept = std::find_if(
            m_lyricsAppIcons.begin(), m_lyricsAppIcons.end(),
            [&](const KeptAppIcon& icon) { return icon.appId == app.appId; });
        if (kept != m_lyricsAppIcons.end()) {
            if (!kept->retryAt || now < kept->retryAt) {
                continue;
            }
            misses = kept->bitmap ? 0 : kept->misses + 1;
            m_lyricsAppIcons.erase(kept);
        }
        bool fromExplorer = false;
        auto bitmap = LoadAppIcon(target, m_wic.get(), m_hwnd, app.appId,
                                  (int)std::round(24 * m_scale), &fromExplorer);
        m_lyricsAppIcons.push_back(
            {app.appId, bitmap,
             AppIconRetryAt(fromExplorer, !!bitmap, misses, now),
             bitmap ? 0 : misses});
    }
}

ID2D1Bitmap* ControlPanel::LyricsAppBitmap(const std::wstring& appId) const {
    for (const auto& icon : m_lyricsAppIcons) {
        if (icon.appId == appId) {
            return icon.bitmap.get();
        }
    }
    return nullptr;
}

