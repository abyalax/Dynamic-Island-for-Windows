#pragma once
void Island::WatchTrayAppWindows(const TrayIcon& icon) {
    StopWatchingTrayAppWindows();
    m_trayAppAnchor = {-1, -1};
    // On the lower half of the screen (or a side), the app's window already
    // opens near the island.
    if (AtBottom() || m_vertical) {
        return;
    }
    for (const auto& slot : m_slots) {
        if (slot.item == IslandItem::TrayIcon && slot.text == icon.key) {
            m_trayAppAnchor = {
                m_position.x + (LONG)((slot.rect.left + slot.rect.right) / 2),
                m_position.y + (LONG)(m_pill.bottom + kPanelGap * m_scale)};
        }
    }
    // (Not without the app's process: the hooks would see every app's
    // windows.)
    if (m_trayAppAnchor.x < 0 || !icon.processId) {
        m_trayAppAnchor = {-1, -1};
        return;
    }
    m_trayAppProcess = icon.processId;
    // Shown or brought to the front within a moment.
    m_trayAppHooks[0] = SetWinEventHook(
        EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, nullptr, TrayAppWinEventProc,
        icon.processId, 0, WINEVENT_OUTOFCONTEXT);
    m_trayAppHooks[1] = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
        TrayAppWinEventProc, icon.processId, 0, WINEVENT_OUTOFCONTEXT);
    SetTimer(m_hwnd, kTrayAppTimerId, 2000, nullptr);
}

void Island::StopWatchingTrayAppWindows() {
    for (auto& hook : m_trayAppHooks) {
        if (hook) {
            UnhookWinEvent(hook);
            hook = nullptr;
        }
    }
    if (m_hwnd) {
        KillTimer(m_hwnd, kTrayAppTimerId);
    }
}

void Island::OnTrayAppWindow(HWND hWnd) {
    DWORD processId = 0;
    GetWindowThreadProcessId(hWnd, &processId);
    if (GetAncestor(hWnd, GA_ROOT) != hWnd || !IsWindowVisible(hWnd) ||
        IsZoomed(hWnd) || m_trayAppAnchor.x < 0 ||
        processId != m_trayAppProcess) {
        return;
    }
    RECT rect;
    MONITORINFO monitor{sizeof(monitor)};
    if (!GetWindowRect(hWnd, &rect) ||
        !GetMonitorInfo(MonitorFromPoint(m_trayAppAnchor,
                                         MONITOR_DEFAULTTONEAREST),
                        &monitor)) {
        return;
    }
    const RECT& screen = monitor.rcMonitor;
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    // Only a small window in the lower part of the screen (a flyout placed
    // next to where the taskbar's icon would be), not the app's own window.
    if (width <= 0 || height <= 0 ||
        width > (screen.right - screen.left) * 6 / 10 ||
        height > (screen.bottom - screen.top) * 7 / 10 ||
        rect.top < screen.top + (screen.bottom - screen.top) / 3) {
        return;
    }
    const int x = std::clamp((int)m_trayAppAnchor.x - width / 2,
                             (int)screen.left + 8,
                             std::max((int)screen.left + 8,
                                      (int)screen.right - width - 8));
    SetWindowPos(hWnd, nullptr, x, m_trayAppAnchor.y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    StopWatchingTrayAppWindows();
}

// A chat app's tray icon blinking (it changes, or hides and shows again, at
// least 4 times with less than 2 seconds between them): its app on every
// island's pill, with its dot (once each time it starts blinking; opening
// the app takes it away, as for a notification). Its program is its ID (see
// WindowMatchesApp).
void Island::CheckTrayBlinks() {
    static constexpr PCWSTR kBlinkingApps[] = {L"WeChat.exe", L"Weixin.exe",
                                               L"WXWork.exe", L"DingTalk.exe"};
    const double now = NowSeconds();
    std::vector<std::wstring> started;
    AcquireSRWLockShared(&g_trayIconsLock);
    for (const auto& icon : g_trayIcons) {
        const std::wstring program = icon.key.substr(0, icon.key.find(L'|'));
        PCWSTR file = PathFindFileName(program.c_str());
        if (std::none_of(std::begin(kBlinkingApps), std::end(kBlinkingApps),
                         [&](PCWSTR name) {
                             return _wcsicmp(name, file) == 0;
                         })) {
            continue;
        }
        auto it = std::find_if(
            m_trayBlinks.begin(), m_trayBlinks.end(),
            [&](const TrayBlink& blink) { return blink.key == icon.key; });
        if (it == m_trayBlinks.end()) {
            m_trayBlinks.push_back(
                {icon.key, icon.serial, icon.hidden, now, 0, false});
            continue;
        }
        if (it->serial == icon.serial && it->hidden == icon.hidden) {
            continue;
        }
        it->serial = icon.serial;
        it->hidden = icon.hidden;
        if (now - it->last > 2.0) {
            it->changes = 0;
            it->blinking = false;
        }
        it->last = now;
        if (++it->changes >= 4 && !it->blinking) {
            it->blinking = true;
            started.push_back(program);
        }
    }
    ReleaseSRWLockShared(&g_trayIconsLock);
    // (Icons gone: forgotten.)
    std::erase_if(m_trayBlinks, [](const TrayBlink& blink) {
        AcquireSRWLockShared(&g_trayIconsLock);
        const bool present = std::any_of(
            g_trayIcons.begin(), g_trayIcons.end(),
            [&](const TrayIcon& icon) { return icon.key == blink.key; });
        ReleaseSRWLockShared(&g_trayIconsLock);
        return !present;
    });
    for (const auto& program : started) {
        IslandToast toast;
        toast.appId = program;
        ForEachIsland([&](Island& island) { island.AddUnread(toast); });
    }
}

// Takes the tray icons from the taskbar.
void Island::UpdateTrayIcons() {
    std::vector<TrayIcon> before = std::move(m_trayIcons);
    m_trayIcons.clear();
    AcquireSRWLockShared(&g_trayIconsLock);
    for (const auto& icon : g_trayIcons) {
        if (!icon.hidden && icon.icon) {
            m_trayIcons.push_back(icon);
        }
    }
    ReleaseSRWLockShared(&g_trayIconsLock);
    if (this == MainIsland()) {
        CheckTrayBlinks();
    }
    SortTrayIcons();
    // A new one grows in (not the first ones); one gone shrinks away where
    // it was.
    for (auto& icon : m_trayIcons) {
        auto old = std::find_if(before.begin(), before.end(),
                                [&](const TrayIcon& entry) {
                                    return entry.key == icon.key;
                                });
        if (old != before.end()) {
            icon.shown = old->shown;
            icon.shownVelocity = old->shownVelocity;
        } else if (m_trayIconsSeen) {
            icon.shown = 0;
            icon.shownVelocity = 0;
        }
    }
    for (size_t i = 0; i < before.size(); i++) {
        TrayIcon& old = before[i];
        if (old.shown <= 0 || FindTrayIcon(old.key)) {
            continue;
        }
        old.leaving = true;
        m_trayIcons.insert(
            m_trayIcons.begin() + std::min(i, m_trayIcons.size()),
            std::move(old));
    }
    m_trayIconsSeen = true;
    StartAnimating();
    // Bitmaps of icons that are gone.
    std::erase_if(m_trayBitmaps, [&](const auto& entry) {
        return !FindTrayIcon(std::get<0>(entry));
    });
    if (m_trayListPanel.IsOpen()) {
        UpdateTrayList();
    }
}

const TrayIcon* Island::FindTrayIcon(const std::wstring& key) const {
    for (const auto& icon : m_trayIcons) {
        if (icon.key == key) {
            return &icon;
        }
    }
    return nullptr;
}

ID2D1Bitmap* Island::GetTrayBitmap(const TrayIcon& icon) {
    for (auto& [key, serial, bitmap] : m_trayBitmaps) {
        if (key == icon.key) {
            if (serial == icon.serial) {
                return bitmap.get();
            }
            break;
        }
    }
    std::erase_if(m_trayBitmaps, [&](const auto& entry) {
        return std::get<0>(entry) == icon.key;
    });
    winrt::com_ptr<ID2D1Bitmap> bitmap;
    winrt::com_ptr<IWICBitmap> wicBitmap;
    winrt::com_ptr<IWICFormatConverter> converter;
    if (m_wic && icon.icon &&
        SUCCEEDED(m_wic->CreateBitmapFromHICON(icon.icon->icon,
                                               wicBitmap.put())) &&
        SUCCEEDED(m_wic->CreateFormatConverter(converter.put())) &&
        SUCCEEDED(converter->Initialize(
            wicBitmap.get(), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0,
            WICBitmapPaletteTypeMedianCut))) {
        m_canvas.target->CreateBitmapFromWicBitmap(converter.get(), nullptr,
                                                   bitmap.put());
    }
    m_trayBitmaps.push_back({icon.key, icon.serial, bitmap});
    return bitmap.get();
}

float Island::PinAmount(const std::wstring& key) const {
    for (const auto& state : m_pinStates) {
        if (state.key == key) {
            return (float)std::clamp(state.amount, 0.0, 1.0);
        }
    }
    return IsTrayPinned(key) ? 1.0f : 0.0f;
}

bool Island::IsTrayPinned(const std::wstring& key) const {
    return std::find(g_trayPinned.begin(), g_trayPinned.end(), key) !=
           g_trayPinned.end();
}

void Island::SetTrayPinned(const std::wstring& key, bool pinned) {
    // It slides from where it is now.
    if (std::none_of(m_pinStates.begin(), m_pinStates.end(),
                     [&](const PinState& state) { return state.key == key; })) {
        m_pinStates.push_back({key, IsTrayPinned(key) ? 1.0 : 0.0, 0});
    }
    std::erase(g_trayPinned, key);
    if (pinned) {
        g_trayPinned.push_back(key);
    } else {
        // (Pinned again, it's with the tray.)
        std::erase_if(g_trayAnchors,
                      [&](const auto& anchor) { return anchor.first == key; });
    }
    StartAnimating();
    SaveTrayArrangement();
    OnTrayArranged();
}

void Island::SortTrayIcons() {
    auto rank = [](const std::wstring& key) {
        const auto it = std::find(g_trayOrder.begin(), g_trayOrder.end(), key);
        return (size_t)(it - g_trayOrder.begin());
    };
    std::stable_sort(m_trayIcons.begin(), m_trayIcons.end(),
                     [&](const TrayIcon& a, const TrayIcon& b) {
                         return rank(a.key) < rank(b.key);
                     });
}

void Island::OnTrayArranged() {
    ForEachIsland([](Island& island) {
        island.SortTrayIcons();
        island.UpdateTrayList();
        island.Relayout();
        island.StartAnimating();
        island.Render();
    });
}

void Island::RenderIslands(std::initializer_list<Island*> islands) {
    for (Island* island : islands) {
        // (Still there; the only one before they're all made.)
        if (island &&
            (g_islands.empty() ? island == g_island
                               : std::find(g_islands.begin(), g_islands.end(),
                                           island) != g_islands.end())) {
            IslandScope scope(island);
            island->Render(true);
        }
    }
}

bool Island::TrayDropAt(POINT screen,
                        const std::wstring& key,
                        TrayDrop* drop) const {
    if (!m_hwnd || !IsWindowVisible(m_hwnd) || m_away ||
        (m_minimized && !m_bar)) {
        return false;
    }
    const float s = m_scale;
    const POINT pt{screen.x - m_position.x, screen.y - m_position.y};
    const float grow = std::round(10 * s);
    if (pt.x < m_pill.left - grow || pt.x > m_pill.right + grow ||
        pt.y < m_pill.top - grow || pt.y > m_pill.bottom + grow) {
        return false;
    }
    // Along the pill, whichever way it stands.
    auto from = [&](const D2D1_RECT_F& r) {
        return m_vertical ? r.top : r.left;
    };
    auto to = [&](const D2D1_RECT_F& r) {
        return m_vertical ? r.bottom : r.right;
    };
    auto middle = [&](const IslandSlot* slot) {
        return (from(slot->rect) + to(slot->rect)) / 2;
    };
    const float along = (float)(m_vertical ? pt.y : pt.x);
    const float gap = std::round(3 * s);
    auto markAt = [&](float at) {
        const float inset = std::round(6 * s);
        drop->mark = m_vertical ? D2D1_RECT_F{m_pill.left + inset, at - 1.5f,
                                              m_pill.right - inset, at + 1.5f}
                                : D2D1_RECT_F{at - 1.5f, m_pill.top + inset,
                                              at + 1.5f, m_pill.bottom - inset};
    };
    // The items it can go between, and the ones not pinned open sideways.
    std::vector<const IslandSlot*> items;
    std::vector<const IslandSlot*> listed;
    bool anyListed = false;
    float listFrom = 0;
    float listTo = 0;
    for (const auto& slot : m_slots) {
        if (to(slot.rect) - from(slot.rect) < 2) {
            continue;
        }
        const bool tray = slot.item == IslandItem::TrayIcon;
        if (slot.inList) {
            listFrom = anyListed ? std::min(listFrom, from(slot.rect))
                                 : from(slot.rect);
            listTo =
                anyListed ? std::max(listTo, to(slot.rect)) : to(slot.rect);
            anyListed = true;
            if (tray && slot.text != key) {
                listed.push_back(&slot);
            }
            continue;
        }
        if (slot.font == kTrayListFont || slot.item == IslandItem::None ||
            slot.item == IslandItem::TraySeparator || slot.group < 0 ||
            (tray && slot.text == key)) {
            continue;
        }
        items.push_back(&slot);
    }
    // Into that list: not pinned, in its place among them.
    if (m_trayAmount > 0.5 && anyListed && along >= listFrom - gap &&
        along <= listTo + gap) {
        drop->pinned = false;
        drop->anchor = -1;
        const auto next = std::find_if(
            listed.begin(), listed.end(),
            [&](const IslandSlot* slot) { return middle(slot) > along; });
        if (next != listed.end()) {
            drop->before = (*next)->text;
            markAt(from((*next)->rect) - gap);
        } else if (!listed.empty()) {
            drop->after = listed.back()->text;
            markAt(to(listed.back()->rect) + gap);
        } else {
            markAt(listFrom);
        }
        return true;
    }
    // On the pill, between two items: beside the one before (or the one
    // after, at the start), with the pinned icons there in their order.
    std::stable_sort(items.begin(), items.end(),
                     [&](const IslandSlot* a, const IslandSlot* b) {
                         return middle(a) < middle(b);
                     });
    size_t next = 0;
    while (next < items.size() && middle(items[next]) <= along) {
        next++;
    }
    const IslandSlot* after = next > 0 ? items[next - 1] : nullptr;
    const IslandSlot* before = next < items.size() ? items[next] : nullptr;
    auto icon = [](const IslandSlot* slot) {
        return slot && slot->item == IslandItem::TrayIcon;
    };
    drop->pinned = true;
    if (icon(before)) {
        drop->anchor = TrayAnchor(before->text);
        drop->before = before->text;
    } else if (icon(after)) {
        drop->anchor = TrayAnchor(after->text);
        drop->after = after->text;
    } else if (after) {
        drop->anchor = after->group == (int)PillGroup::Tray ? -1 : after->group;
    } else if (before) {
        drop->anchor = before->group == (int)PillGroup::Tray
                           ? -1
                           : TrayAnchorBefore((PillGroup)before->group);
    } else {
        return false;
    }
    markAt(after && before ? (to(after->rect) + from(before->rect)) / 2
           : after         ? to(after->rect) + gap
                           : from(before->rect) - gap);
    return true;
}

TrayDrop Island::TrayDropUnder(Island* owner,
                               POINT screen,
                               const std::wstring& key) {
    std::vector<Island*> islands = g_islands;
    if (islands.empty()) {
        islands.push_back(owner);
    }
    // (Its monitor's: the notches share the icons.)
    for (Island* island : islands) {
        if (island != owner &&
            !EqualRect(&island->m_monitor, &owner->m_monitor)) {
            continue;
        }
        IslandScope scope(island);
        TrayDrop here;
        if (island->TrayDropAt(screen, key, &here)) {
            here.island = island;
            return here;
        }
    }
    return {};
}

void Island::MoveTrayIcon(const std::wstring& key, const TrayDrop& drop) {
    // The order: the icons here now (the ones never moved in Windows'
    // order), then the ones remembered that aren't, with this one in its
    // new place.
    std::vector<std::wstring> order;
    for (const auto& icon : m_trayIcons) {
        if (!icon.leaving && icon.key != key) {
            order.push_back(icon.key);
        }
    }
    const size_t here = order.size();
    for (const auto& kept : g_trayOrder) {
        if (kept != key &&
            std::find(order.begin(), order.end(), kept) == order.end()) {
            order.push_back(kept);
        }
    }
    auto at = order.begin() + here;
    if (!drop.before.empty()) {
        if (auto it = std::find(order.begin(), order.end(), drop.before);
            it != order.end()) {
            at = it;
        }
    } else if (!drop.after.empty()) {
        if (auto it = std::find(order.begin(), order.end(), drop.after);
            it != order.end()) {
            at = it + 1;
        }
    }
    order.insert(at, key);
    order.resize(std::min(order.size(), std::max(kTrayOrderMax, here + 1)));
    g_trayOrder = std::move(order);
    std::erase_if(g_trayAnchors,
                  [&](const auto& anchor) { return anchor.first == key; });
    if (drop.pinned && drop.anchor != -1) {
        g_trayAnchors.push_back({key, drop.anchor});
    }
    if (drop.pinned != IsTrayPinned(key)) {
        SetTrayPinned(key, drop.pinned);
        return;
    }
    SaveTrayArrangement();
    OnTrayArranged();
}

void Island::DragTrayIcon() {
    POINT cursor;
    GetCursorPos(&cursor);
    if (!g_trayDrag.moved) {
        const int threshold = (int)std::round(6 * m_scale);
        if (std::abs(cursor.x - g_trayDrag.from.x) <= threshold &&
            std::abs(cursor.y - g_trayDrag.from.y) <= threshold) {
            return;
        }
        g_trayDrag.moved = true;
        KillTimer(m_hwnd, kTipTimerId);
        m_pendingTipKey.clear();
        ShowTrayTip({});
        KillTimer(m_hwnd, kPinTimerId);
        m_pinShownKey.clear();
        KillTimer(m_hwnd, kTrayCloseTimerId);
    }
    const TrayDrop before = g_trayDrag.drop;
    g_trayDrag.drop = TrayDropUnder(this, cursor, g_trayDrag.key);
    RenderIslands({this, before.island, g_trayDrag.drop.island});
}

void Island::FinishTrayDrag(bool drop) {
    const TrayDrag drag = std::move(g_trayDrag);
    g_trayDrag = {};
    if (GetCapture() == m_hwnd) {
        ReleaseCapture();
    }
    if (!drag.moved) {
        // A click: its app gets it now.
        if (drop) {
            ClickTrayIcon(drag.key, WM_LBUTTONDOWN);
            ClickTrayIcon(drag.key, WM_LBUTTONUP);
        }
        return;
    }
    if (drop && drag.drop.island) {
        IslandScope scope(drag.drop.island);
        drag.drop.island->MoveTrayIcon(drag.key, drag.drop);
    }
    RenderIslands({this, drag.drop.island});
}

void Island::DrawTrayDrag(ID2D1RenderTarget* target,
                          ID2D1SolidColorBrush* brush) {
    if (!g_trayDrag.moved) {
        return;
    }
    const TrayDrop& drop = g_trayDrag.drop;
    if (drop.island == this) {
        const D2D1_RECT_F& mark = drop.mark;
        const float radius =
            std::min(mark.right - mark.left, mark.bottom - mark.top) / 2;
        brush->SetColor(Accent(1));
        target->FillRoundedRectangle({mark, radius, radius}, brush);
    }
    // The icon under the mouse, in the window there: the one it would go
    // on, or the pill it's from (a list's panel draws its own).
    if (drop.island ? drop.island != this
                    : g_trayDrag.fromList || g_trayDrag.owner != this) {
        return;
    }
    const TrayIcon* icon = FindTrayIcon(g_trayDrag.key);
    ID2D1Bitmap* bitmap = icon ? GetTrayBitmap(*icon) : nullptr;
    if (!bitmap) {
        return;
    }
    POINT cursor;
    GetCursorPos(&cursor);
    const float size = std::round(18 * m_scale);
    const float x = (float)(cursor.x - m_position.x);
    const float y = (float)(cursor.y - m_position.y);
    DrawBitmapSmooth(target, bitmap,
                     {x - size / 2, y - size / 2, x + size / 2, y + size / 2},
                     0.9f);
}

// The pin's circle, at the top right of a tray icon's slot.
float Island::PinShown(double now) const {
    if (m_pinShownKey.empty()) {
        return 0;
    }
    const double grow = AnimationSeconds(kPinGrowSeconds);
    const double t = grow > 0 ? (now - m_pinShownAt) / grow : 1;
    return t >= 1 ? 1.0f : (float)SpringOut(std::max(t, 0.0));
}

D2D1_RECT_F Island::TrayPinRect(const D2D1_RECT_F& slot) const {
    const float size = std::round(11 * m_scale);
    const float right = slot.right - std::round(1 * m_scale);
    const float top = slot.top + std::round(2 * m_scale);
    return {right - size, top, right, top + size};
}

// Passes a mouse message on to the app, as the taskbar does: newer apps
// (version 4) get the position and the message with the icon's ID; older ones
// the ID and the message.
void Island::SendToTrayIcon(const std::wstring& key, UINT message) {
    const TrayIcon* icon = FindTrayIcon(key);
    if (!icon || !icon->callback || !IsWindow(icon->owner)) {
        return;
    }
    PostTrayIconMessage(*icon, message);
}

// The icon's name under it, or nothing: a tray icon's tip, or an app's name
// with how many notifications it has (its key starts with kAppTipPrefix).
void Island::ShowTrayTip(const std::wstring& key) {
    if (key == m_tooltipKey) {
        return;
    }
    m_tooltipKey = key;
    const bool app = key.starts_with(kAppTipPrefix);
    const bool itemTip = key.starts_with(kItemTipPrefix);
    const std::wstring slotKey = app ? key.substr(wcslen(kAppTipPrefix)) : key;
    std::wstring text;
    IslandItem tipItem = IslandItem::None;
    std::wstring tipText;
    if (itemTip) {
        const std::wstring rest = key.substr(wcslen(kItemTipPrefix));
        const size_t colon = rest.find(L':');
        tipItem = (IslandItem)_wtoi(rest.c_str());
        tipText = colon == std::wstring::npos ? L"" : rest.substr(colon + 1);
        text = ItemTip(tipItem, tipText);
    } else if (app) {
        text = AppDisplayName(slotKey);
        if (const int count = AppCount(slotKey)) {
            text += L" - " + std::to_wstring(count) + L" " +
                    (count == 1 ? Tr(L"notification", L"notificação")
                                : Tr(L"notifications", L"notificações"));
        }
    } else if (const TrayIcon* icon =
                   key.empty() ? nullptr : FindTrayIcon(key)) {
        text = TrayIconName(*icon);
    }
    // Under the icon (over it on the lower half of the screen), or beside
    // it towards the screen's middle when upright.
    const IslandItem item = itemTip ? tipItem
                            : app   ? IslandItem::App
                                    : IslandItem::TrayIcon;
    // (The first: a copy shrinking away is after it.)
    const IslandSlot* at = nullptr;
    for (const auto& slot : m_slots) {
        // (What's playing by its part: its note, or its cover and title.)
        const std::wstring part = slot.item == IslandItem::Media
                                      ? std::wstring(MediaSlotPart(slot))
                                      : SlotKey(slot);
        if (slot.item == item && part == (itemTip ? tipText : slotKey)) {
            at = &slot;
            break;
        }
    }
    winrt::com_ptr<IDWriteTextLayout> layout;
    if (!g_settings.islandTips || text.empty() || !at || !m_textFormat ||
        FAILED(m_dwrite->CreateTextLayout(
            text.c_str(), (UINT32)text.size(), m_textFormat.get(),
            std::round(320 * m_scale), 1000, layout.put()))) {
        HideTrayTip();
        return;
    }
    // A long tip goes on more lines; the box is as wide as its text.
    layout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    layout->SetMaxWidth(std::ceil(metrics.width));
    layout->SetMaxHeight(std::ceil(metrics.height));
    m_tipLayout = layout;
    const float gap = std::round(kGearBubbleGap * m_scale);
    if (m_vertical) {
        m_tipSide = m_sideLeft ? TipSide::Right : TipSide::Left;
        m_tipAnchor = {
            m_position.x + (LONG)std::lround(m_sideLeft ? m_pill.right + gap
                                                        : m_pill.left - gap),
            m_position.y +
                (LONG)std::lround((at->rect.top + at->rect.bottom) / 2)};
    } else {
        const bool over = !m_bar && AtBottom();
        m_tipSide = over ? TipSide::Over : TipSide::Under;
        m_tipAnchor = {
            m_position.x +
                (LONG)std::lround((at->rect.left + at->rect.right) / 2),
            m_position.y + (LONG)std::lround(over ? m_pill.top - gap
                                                  : m_pill.bottom + gap)};
    }
    // Shown already (or still going): it moves to this icon at once.
    const double now = NowSeconds();
    if (!m_tipShownAt || (m_tipHideAt && TipAmount(now) < 0.5f)) {
        m_tipShownAt = now;
    }
    m_tipHideAt = 0;
    StartAnimating();
}

void Island::HideTrayTip() {
    if (m_tipShownAt && !m_tipHideAt) {
        m_tipHideAt = NowSeconds();
        StartAnimating();
    }
}

// How far the icon's name is out, from 0 to 1 (a bit more while bouncing).
float Island::TipAmount(double now) const {
    if (!m_tipShownAt) {
        return 0;
    }
    const double t = now - m_tipShownAt;
    const double in = AnimationSeconds(kTipIn);
    const double out = AnimationSeconds(kTipOut);
    float amount = t < in ? (float)SpringOut(t / in) : 1;
    if (m_tipHideAt && now >= m_tipHideAt) {
        amount = std::min(
            amount, out > 0 ? 1 - (float)EaseInOutCubic(std::min(
                                      1.0, (now - m_tipHideAt) / out))
                            : 0.0f);
    }
    return std::max(amount, 0.0f);
}

// The icon's name, in its own window (it doesn't take the mouse), growing
// out of the pill's side like the settings bubble, in the island's colors.
void Island::RenderTrayTip() {
    const float amount = m_away ? 0.0f : TipAmount(NowSeconds());
    if (amount <= 0.001f || !m_tipLayout) {
        if (m_tipWnd && IsWindowVisible(m_tipWnd)) {
            ShowWindow(m_tipWnd, SW_HIDE);
        }
        m_tipDrawnKey.clear();
        return;
    }
    if (!m_tipWnd) {
        m_tipWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT |
                                      WS_EX_TOPMOST | WS_EX_TOOLWINDOW |
                                      WS_EX_NOACTIVATE,
                                  L"STATIC", nullptr, WS_POPUP, 0, 0, 0, 0,
                                  nullptr, nullptr, nullptr, nullptr);
        if (!m_tipWnd) {
            return;
        }
        if (!m_tipCanvas.Create(m_d2d.get())) {
            DestroyWindow(m_tipWnd);
            m_tipWnd = nullptr;
            return;
        }
    }
    DWRITE_TEXT_METRICS metrics{};
    m_tipLayout->GetMetrics(&metrics);
    const float padX = std::round(12 * m_scale);
    const float padY = std::round(5 * m_scale);
    const float width = std::ceil(metrics.width) + padX * 2;
    const float height = std::max(std::ceil(metrics.height) + padY * 2,
                                  std::round(26 * m_scale));
    // Its window, with room to bounce; the bubble against the side it grows
    // from.
    const SIZE size{(LONG)std::ceil(width * 1.1f) + 2,
                    (LONG)std::ceil(height * 1.1f) + 2};
    D2D1_RECT_F full{(size.cx - width) / 2, 0, (size.cx + width) / 2, height};
    D2D1_POINT_2F from{size.cx / 2.0f, 0};
    POINT position{m_tipAnchor.x - size.cx / 2, m_tipAnchor.y};
    switch (m_tipSide) {
        case TipSide::Under:
            break;
        case TipSide::Over:
            full.top = size.cy - height;
            full.bottom = (float)size.cy;
            from.y = (float)size.cy;
            position.y = m_tipAnchor.y - size.cy;
            break;
        case TipSide::Right:
        case TipSide::Left: {
            const bool right = m_tipSide == TipSide::Right;
            full = {right ? 0 : size.cx - width, (size.cy - height) / 2,
                    right ? width : (float)size.cx, (size.cy + height) / 2};
            from = {right ? 0 : (float)size.cx, size.cy / 2.0f};
            position = {right ? m_tipAnchor.x : m_tipAnchor.x - size.cx,
                        m_tipAnchor.y - size.cy / 2};
            break;
        }
    }
    // Inside the monitor.
    position.x =
        std::clamp(position.x, m_monitor.left,
                   std::max(m_monitor.left, m_monitor.right - size.cx));
    position.y =
        std::clamp(position.y, m_monitor.top,
                   std::max(m_monitor.top, m_monitor.bottom - size.cy));
    const std::wstring key =
        std::to_wstring((int)(amount * 1000)) + L"|" + m_tooltipKey + L"|" +
        std::to_wstring(position.x) + L"," + std::to_wstring(position.y) +
        L"|" + std::to_wstring(size.cx) + L"|" +
        std::to_wstring(g_themeChangeStart) + L"|" +
        std::to_wstring(Bg(1).r + Bg(1).g * 3 + Bg(1).b * 7 + Bg(1).a * 11 +
                        Fg(1).r * 13 + Fg(1).g * 17 + Fg(1).b * 19);
    if (key == m_tipDrawnKey || !m_tipCanvas.Begin(size)) {
        return;
    }
    m_tipDrawnKey = key;
    auto* target = m_tipCanvas.target.get();
    const float k = std::clamp(amount, 0.0f, 1.1f);
    const D2D1_RECT_F bubble{from.x + (full.left - from.x) * k,
                             from.y + (full.top - from.y) * k,
                             from.x + (full.right - from.x) * k,
                             from.y + (full.bottom - from.y) * k};
    const float opacity = std::clamp(amount * 1.6f, 0.0f, 1.0f);
    // Round ends on one line; rounded corners on more.
    const float radius = std::min(
        metrics.lineCount > 1 ? std::round(12 * m_scale) : height / 2,
        std::min(bubble.bottom - bubble.top, bubble.right - bubble.left) / 2);
    D2D1_COLOR_F fill = BgSolid(1);
    fill.a *= opacity;
    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(target->CreateSolidColorBrush(fill, brush.put()))) {
        target->FillRoundedRectangle({bubble, radius, radius}, brush.get());
        brush->SetColor(Fg(0.10f * EdgeAmount() * opacity));
        target->DrawRoundedRectangle(
            {{bubble.left + 0.5f, bubble.top + 0.5f, bubble.right - 0.5f,
              bubble.bottom - 0.5f},
             radius,
             radius},
            brush.get(), 1);
        brush->SetColor(
            Fg(0.9f * std::clamp((amount - 0.3f) / 0.5f, 0.0f, 1.0f)));
        const D2D1_POINT_2F origin{
            std::round((bubble.left + bubble.right - std::ceil(metrics.width)) /
                       2),
            std::round((bubble.top + bubble.bottom -
                        std::ceil(metrics.height)) /
                       2)};
        target->PushAxisAlignedClip(bubble, D2D1_ANTIALIAS_MODE_ALIASED);
        target->DrawTextLayout(origin, m_tipLayout.get(), brush.get());
        target->PopAxisAlignedClip();
    }
    if (m_tipCanvas.End(m_tipWnd, position, 1) && !IsWindowVisible(m_tipWnd)) {
        SetWindowPos(m_tipWnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                         SWP_SHOWWINDOW);
    }
}

// A tray icon's mouse message, passed on to its app (a double click opens
// it, the right button shows its menu, like on the taskbar); the middle
// button pins it.
void Island::ClickTrayIcon(const std::wstring& key, UINT msg) {
    const TrayIcon* icon = FindTrayIcon(key);
    const bool reaches = icon && icon->callback && IsWindow(icon->owner);
    switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
            SendToTrayIcon(key, msg);
            break;
        case WM_LBUTTONUP:
            if (icon) {
                WatchTrayAppWindows(*icon);
            }
            SendToTrayIcon(key, WM_LBUTTONUP);
            if (reaches) {
                // (How it was told, for an app that doesn't open.)
                IslandLog(L"A tray icon's click: version %d, ID %u",
                          icon->version == kTrayVersionUnknown
                              ? -1
                              : (int)icon->version,
                          icon->id);
                PostTrayIconRelease(*icon, WM_LBUTTONUP);
            } else {
                IslandLog(L"A tray icon's click didn't reach its app");
            }
            break;
        case WM_RBUTTONUP:
            SendToTrayIcon(key, WM_RBUTTONUP);
            if (reaches) {
                // (How it was told, for a menu that misbehaves.)
                IslandLog(L"A tray icon's menu: version %d, ID %u",
                          icon->version == kTrayVersionUnknown
                              ? -1
                              : (int)icon->version,
                          icon->id);
                PostTrayIconRelease(*icon, WM_RBUTTONUP);
            }
            break;
        case WM_MBUTTONUP:
            ToggleTrayPin(key);
            break;
    }
}

bool TrayListPanel::OnMouseUp(POINT pt) {
    if (!g_trayDrag.key.empty() && g_trayDrag.fromList &&
        g_trayDrag.owner == m_owner) {
        if (g_trayDrag.moved) {
            EndDrag(true);
            return true;
        }
        // A click: the press and the release, to the icon pressed.
        const std::wstring key = g_trayDrag.key;
        g_trayDrag = {};
        if (GetCapture() == m_hwnd) {
            ReleaseCapture();
        }
        Pass(key, m_pressTwice ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN);
        Pass(key, WM_LBUTTONUP);
        return false;
    }
    bool pin = false;
    const int hit = HitTest(pt, &pin);
    if (hit < 0) {
        return false;
    }
    const std::wstring key = m_icons[hit].icon.key;
    if (pin) {
        Pin(key);
        return true;
    }
    Pass(key, WM_LBUTTONUP);
    return false;
}

bool TrayListPanel::OnOtherButton(UINT msg, POINT pt) {
    bool pin = false;
    const int hit = HitTest(pt, &pin);
    if (hit < 0 || pin) {
        return false;
    }
    const std::wstring key = m_icons[hit].icon.key;
    if (msg == WM_MBUTTONUP) {
        Pin(key);
        return true;
    }
    // (From the press: some apps take the focus for their menu then.)
    if (msg == WM_RBUTTONDOWN) {
        KeepOpenForMenu();
    }
    Pass(key, msg);
    return false;
}

bool TrayListPanel::DragIcon() {
    // (Let go outside, or the mouse taken by another window.)
    if (GetKeyState(VK_LBUTTON) >= 0 || GetCapture() != m_hwnd) {
        EndDrag(false);
        return true;
    }
    POINT cursor;
    GetCursorPos(&cursor);
    if (!g_trayDrag.moved) {
        const int threshold = (int)std::round(6 * m_scale);
        if (std::abs(cursor.x - g_trayDrag.from.x) <= threshold &&
            std::abs(cursor.y - g_trayDrag.from.y) <= threshold) {
            return false;
        }
        g_trayDrag.moved = true;
        m_pinKey.clear();
        KillTimer(m_hwnd, kPinTimerId);
    }
    const POINT pt{cursor.x - m_position.x, cursor.y - m_position.y};
    const TrayDrop before = g_trayDrag.drop;
    g_trayDrag.drop = {};
    m_dropIndex = -1;
    if (pt.x >= 0 && pt.y >= 0 && pt.x < m_size.cx && pt.y < m_size.cy) {
        m_dropIndex = DropIndex(pt, &m_dropMark);
    } else if (m_owner) {
        g_trayDrag.drop =
            Island::TrayDropUnder(m_owner, cursor, g_trayDrag.key);
    }
    Island::RenderIslands({before.island, g_trayDrag.drop.island});
    return true;
}

int TrayListPanel::DropIndex(POINT pt, D2D1_RECT_F* mark) const {
    // Before the nearest icon, or after it past its middle.
    int nearest = -1;
    float distance = 0;
    for (size_t i = 0; i < m_cells.size(); i++) {
        const D2D1_RECT_F& cell = m_cells[i];
        const float dx = pt.x - (cell.left + cell.right) / 2;
        const float dy = pt.y - (cell.top + cell.bottom) / 2;
        if (nearest < 0 || dx * dx + dy * dy < distance) {
            nearest = (int)i;
            distance = dx * dx + dy * dy;
        }
    }
    if (nearest < 0) {
        *mark = {};
        return 0;
    }
    const D2D1_RECT_F& cell = m_cells[nearest];
    // (A grid's rows go sideways, the names' columns down.)
    const bool past = m_names ? pt.y > (cell.top + cell.bottom) / 2
                              : pt.x > (cell.left + cell.right) / 2;
    const float inset = std::round(4 * m_scale);
    if (m_names) {
        const float y = past ? cell.bottom : cell.top;
        *mark = {cell.left + inset, y - 1.5f, cell.right - inset, y + 1.5f};
    } else {
        const float x = past ? cell.right : cell.left;
        *mark = {x - 1.5f, cell.top + inset, x + 1.5f, cell.bottom - inset};
    }
    return nearest + (past ? 1 : 0);
}

void TrayListPanel::EndDrag(bool drop) {
    const TrayDrag drag = std::move(g_trayDrag);
    g_trayDrag = {};
    const int index = m_dropIndex;
    m_dropIndex = -1;
    if (GetCapture() == m_hwnd) {
        ReleaseCapture();
    }
    if (drop && drag.moved && m_owner && index >= 0) {
        // In the list: before the icon there (not itself), or last.
        TrayDrop place;
        for (size_t i = index; i < m_icons.size(); i++) {
            if (m_icons[i].icon.key != drag.key) {
                place.before = m_icons[i].icon.key;
                break;
            }
        }
        if (place.before.empty()) {
            for (size_t i = m_icons.size(); i-- > 0;) {
                if (m_icons[i].icon.key != drag.key) {
                    place.after = m_icons[i].icon.key;
                    break;
                }
            }
        }
        IslandScope scope(m_owner);
        m_owner->MoveTrayIcon(drag.key, place);
    } else if (drop && drag.moved && drag.drop.island) {
        IslandScope scope(drag.drop.island);
        drag.drop.island->MoveTrayIcon(drag.key, drag.drop);
    }
    Island::RenderIslands({drag.drop.island});
    Render();
}

void TrayListPanel::Pass(const std::wstring& key, UINT msg) {
    if (m_owner) {
        m_owner->ClickTrayIcon(key, msg);
    }
}

// Pinned, it leaves the list for the pill (see Island::UpdateTrayList).
void TrayListPanel::Pin(const std::wstring& key) {
    m_pinKey.clear();
    if (m_owner) {
        m_owner->ToggleTrayPin(key);
    }
}

void Island::UpdateTrayList() {
    std::vector<TrayListEntry> entries;
    for (const auto& icon : m_trayIcons) {
        if (!icon.leaving && !IsTrayPinned(icon.key)) {
            std::wstring name = TrayIconName(icon);
            name = name.substr(0, name.find_first_of(L"\r\n"));
            entries.push_back({icon, std::move(name)});
        }
    }
    m_trayListPanel.SetIcons(std::move(entries),
                             g_settings.islandTrayStyle == 2);
}

bool Island::PressBannerReply(POINT pt) {
    if (!m_bannerTyping || HitTest(pt) != IslandItem::BannerReply) {
        return false;
    }
    D2D1_RECT_F field, send, open;
    BannerParts(&field, &send, &open);
    if (!PointInRect(pt, field) || PointInRect(pt, send)) {
        return false;
    }
    EditTextClick(m_bannerReplyText, m_bannerEdit, pt);
    StartEditDrag(m_hwnd, pt);
    m_replyPressed = true;
    Render(true);
    return true;
}

// How a drop on the shelf is told to the drag's source: copied (it keeps
// its file; only its path is kept), or linked if it allows only that.
DWORD ShelfDropEffect(DWORD allowed) {
    return allowed & DROPEFFECT_COPY   ? DROPEFFECT_COPY
           : allowed & DROPEFFECT_LINK ? DROPEFFECT_LINK
                                       : DROPEFFECT_NONE;
}

