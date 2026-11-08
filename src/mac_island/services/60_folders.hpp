ID2D1Bitmap* FoldersPanel::TypeIcon(ID2D1RenderTarget* target,
                                    const FolderEntry& item) {
    std::wstring type = L"\\";
    if (item.kind == FolderKind::File) {
        const size_t dot = item.name.find_last_of(L'.');
        type = dot == std::wstring::npos ? L"." : item.name.substr(dot);
        std::transform(type.begin(), type.end(), type.begin(), towlower);
    }
    for (const auto& [key, bitmap] : m_icons) {
        if (key == type) {
            return bitmap.get();
        }
    }
    winrt::com_ptr<ID2D1Bitmap> bitmap =
        FileTypeIcon(target, m_wic.get(), type, false);
    m_icons.push_back({type, bitmap});
    return bitmap.get();
}

bool FoldersPanel::OnAppMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_APP_FILE_THUMBNAIL) {
        m_thumbs.Take(lParam);
        return true;
    }
    if (msg == WM_APP_FOLDER_LIST) {
        std::unique_ptr<FolderListJob> job((FolderListJob*)lParam);
        // (Read again: for the folder still shown there, whatever opened
        // meanwhile.)
        if (job->column >= (int)m_columns.size() ||
            (job->quiet ? m_columns[job->column].path != job->path
                        : job->generation != m_generation)) {
            return false;
        }
        Column& column = m_columns[job->column];
        // (Its quiet read done: another one wanted meanwhile goes now, below.)
        bool again = false;
        if (job->quiet) {
            column.reading = false;
            again = std::exchange(column.readAgain, false);
        }
        // (The order or the kind shown switched while it was read: a folder
        // cut is read again, for the first ones in the new order or kind.)
        const bool reorder = job->order != column.order;
        if (reorder) {
            SortFolderItems(job->items, column.order);
        }
        const bool readAgain =
            (reorder && Cut(job->items)) ||
            (job->filter != column.filter &&
             (Cut(job->items) || job->filtered != FolderFilter::All));
        column.readFilter = job->filtered;
        const double now = NowSeconds();
        if (job->quiet) {
            for (const auto& item : job->items) {
                if (std::none_of(column.all.begin(), column.all.end(),
                                 [&](const FolderEntry& old) {
                                     return old.path == item.path &&
                                            old.kind == item.kind;
                                 })) {
                    m_arrivals.push_back({item.path, now});
                }
            }
        } else {
            column.shownAt = now;
        }
        const int top = column.top;
        column.all = std::move(job->items);
        SYSTEMTIME today;
        GetLocalTime(&today);
        for (auto& item : column.all) {
            if (item.kind == FolderKind::File ||
                item.kind == FolderKind::Folder) {
                item.date = FolderItemDate(item.modified, today);
            }
            // (Folders have none, as in Explorer.)
            if (item.kind == FolderKind::File) {
                WCHAR size[32] = L"";
                StrFormatByteSizeW((LONGLONG)item.size, size, ARRAYSIZE(size));
                item.sizeText = size;
            }
        }
        column.loading = false;
        ApplySearch();
        if (job->quiet) {
            column.top =
                std::clamp(top, 0, std::max(0, (int)column.items.size() - 1));
            FindOpened(job->column, false);
        }
        if ((size_t)job->column < m_columns.size()) {
            RevealDropped(job->column);
            if (readAgain || again) {
                ReloadColumn(job->column);
            }
        }
        StartIslandAnimation();
        Layout();
        return true;
    }
    if (msg == WM_APP_FOLDER_CHANGED) {
        // (From the folders watched now: their columns read again.)
        if (!m_watch || (UINT)wParam != m_watchSet) {
            return false;
        }
        // A drive came or went: the favorite files on it.
        if (lParam == -1) {
            return m_shown && CheckFavorites(L"");
        }
        if ((size_t)lParam >= m_watchedPaths.size()) {
            return false;
        }
        const std::wstring changed =
            FolderKey(m_watchedPaths[(size_t)lParam]);
        // A favorite file in it gone (and its column, if shown, read again).
        const bool favorites = m_shown && CheckFavorites(changed);
        bool marked = false;
        for (size_t i = 1; i < m_columns.size(); i++) {
            if (_wcsicmp(FolderKey(m_columns[i].path).c_str(),
                         changed.c_str()) == 0) {
                m_columns[i].changed = true;
                marked = true;
            }
        }
        if (marked && !m_changePending && m_shown) {
            m_changePending = true;
            SetTimer(m_hwnd, kFolderChangeTimerId, kFolderChangeMs, nullptr);
        }
        return favorites;
    }
    if (msg == WM_APP_COLOR_DONE) {
        return OnLookColorDone(lParam);
    }
    if (msg == WM_APP_FOLDER_PICKED) {
        std::unique_ptr<FolderPickJob> job((FolderPickJob*)lParam);
        m_dialogShown = false;
        if (!job->path.empty() &&
            std::find(m_pins.begin(), m_pins.end(), job->path) ==
                m_pins.end()) {
            m_pins.push_back(job->path);
            SavePins();
            LoadPins();
            Layout();
        }
        return true;
    }
    return false;
}

bool FoldersPanel::OnTimer(UINT_PTR id) {
    // The mouse rested on a file: its big thumbnail.
    if (id == kFolderPreviewTimerId) {
        KillTimer(m_hwnd, kFolderPreviewTimerId);
        m_previewShown = m_hover == m_previewSpot && m_previewSpot.column >= 0;
        return m_previewShown;
    }
    // The check on a copy button goes.
    if (id == kCopiedTimerId) {
        m_copiedPath.clear();
        m_copiedButton = RowButton::None;
        return true;
    }
    // Files held on a folder: it opens beside it.
    if (id == kFolderOpenTimerId) {
        if (Navigable(m_dropSpot) &&
            m_columns[m_dropSpot.column].opened != m_dropSpot.row) {
            OpenColumn(m_dropSpot);
            return true;
        }
        return false;
    }
    if (id == kFolderChangeTimerId) {
        m_changePending = false;
        for (size_t i = 1; i < m_columns.size(); i++) {
            if (m_columns[i].changed) {
                m_columns[i].changed = false;
                ReloadColumn((int)i);
            }
        }
    }
    return false;
}

bool FoldersPanel::OnMouseUp(POINT pt) {
    // Copy or move, asked once: kept if chosen to be remembered (in the
    // island's settings too), otherwise for the next drag only.
    if (m_asking) {
        switch (AskHit(pt)) {
            case kAskRemember:
                m_askRemember = !m_askRemember;
                return true;
            case kAskCopy:
            case kAskMove: {
                const int choice = AskHit(pt) == kAskCopy ? 1 : 2;
                if (m_askRemember) {
                    SetIslandPref(IslandPref::FolderDrag, choice);
                } else {
                    m_dragOnce = choice;
                }
                m_asking = false;
                Layout();
                return true;
            }
            default:
                break;
        }
    }
    if (m_fileDrag.column >= 0) {
        EndFileDrag();
    }
    // A pin let go after a drag: in its new place (kept).
    if (m_dragRow >= 0) {
        const int from = m_dragRow;
        const int to = m_dropRow;
        const bool dragged = m_dragging;
        EndPinDrag();
        if (dragged) {
            if (from != to && from < (int)m_pins.size() &&
                to < (int)m_pins.size()) {
                std::wstring pin = std::move(m_pins[from]);
                m_pins.erase(m_pins.begin() + from);
                m_pins.insert(m_pins.begin() + to, std::move(pin));
                SavePins();
                // The first column again, the rest as it was (an open
                // pin's column follows it).
                const int opened = m_columns[0].opened;
                std::vector<Column> rest(m_columns.begin() + 1,
                                         m_columns.end());
                LoadPins();
                if (opened >= 0) {
                    m_columns[0].opened = opened == from ? to
                                          : from < opened && opened <= to
                                              ? opened - 1
                                          : to <= opened && opened < from
                                              ? opened + 1
                                              : opened;
                }
                m_columns.insert(m_columns.end(), rest.begin(), rest.end());
                m_first = m_columns.size() > kShownColumns
                              ? m_columns.size() - kShownColumns
                              : 0;
                Layout();
            }
            return true;
        }
    }
    RowButton button = RowButton::None;
    const Spot spot = HitTest(pt, &button);
    // A column's menu open: a click elsewhere only closes it.
    const bool inMenu = spot.row <= kMenuRow && spot.row > kLookRow;
    if (!inMenu && !(spot.row == -2 && button == RowButton::Sort) &&
        CloseMenus()) {
        Layout();
        return true;
    }
    // Back: the last column goes (the ones before it come back into view).
    if (spot.column == -2) {
        if (m_columns.size() > 1) {
            // (It goes out sliding with the others, or fading where it was.)
            m_leaving = std::move(m_columns.back());
            m_leavingAt = NowSeconds();
            m_columns.pop_back();
            m_columns.back().opened = -1;
            m_generation++;
            ClearSearch();
            ApplySearch();
            const size_t first = m_first;
            m_first = m_columns.size() > kShownColumns
                          ? m_columns.size() - kShownColumns
                          : 0;
            Layout();
            SlideColumns(first);
            if (m_first != first) {
                const D2D1_RECT_F& last = m_columns.back().rect;
                m_leaving.rect = {last.left + m_columnStep, last.top,
                                  last.right + m_columnStep, last.bottom};
            }
            StartIslandAnimation();
        }
        return true;
    }
    if (spot.column < 0) {
        return false;
    }
    // The order of a folder's items (that column's own): its menu, opened or
    // closed, and its direction, switched.
    if (spot.row == -2 && button == RowButton::Sort) {
        const bool open = !m_columns[spot.column].menu;
        CloseMenus();
        m_columns[spot.column].menu = open;
        m_hover = {-1, -1};
        Layout();
        return true;
    }
    if (spot.row == -2 && button == RowButton::Direction) {
        FolderOrder order = m_columns[spot.column].order;
        order.descending = !order.descending;
        Reorder(spot.column, order, m_columns[spot.column].filter);
        return true;
    }
    // A row of the menu: an order (the one already chosen goes the other
    // way; a new one starts as Explorer's does, dates and sizes the newest
    // and largest first), folders mixed with the files or not, or the kind
    // shown. It closes.
    if (inMenu) {
        const MenuRow& choice = MenuRows()[kMenuRow - spot.row];
        Column& column = m_columns[spot.column];
        FolderOrder order = column.order;
        FolderFilter filter = column.filter;
        switch (choice.part) {
            case MenuPart::Sort:
                if ((int)order.sort == choice.value) {
                    order.descending = !order.descending;
                } else {
                    order.sort = (FolderSort)choice.value;
                    order.descending = order.sort == FolderSort::Date ||
                                       order.sort == FolderSort::Size;
                }
                break;
            case MenuPart::Mixed:
                order.mixed = !order.mixed;
                break;
            case MenuPart::Filter:
                filter = (FolderFilter)choice.value;
                break;
            default:
                break;
        }
        column.menu = false;
        Reorder(spot.column, order, filter);
        return true;
    }
    // A pin's icons and colors: one chosen (chosen again: its usual one),
    // any other color (Windows' dialog), both back to default, or closed.
    if (spot.row <= kLookRow) {
        const int k = kLookRow - spot.row;
        if (k == kLookApply) {
            CloseLook();
            return true;
        }
        const std::wstring pin = m_columns[spot.column].lookPin;
        const FolderLook* look = LookOf(pin);
        WCHAR glyph = look ? look->glyph : 0;
        int color = look ? look->color : -1;
        if (k < kLookGlyphs) {
            glyph = kFolderLookGlyphs[k] == glyph ? 0 : kFolderLookGlyphs[k];
        } else if (k < kLookReset - 1) {
            const int chosen = kFolderLookColors[k - kLookGlyphs];
            color = chosen == color ? -1 : chosen;
        } else if (k == kLookReset - 1) {
            StartLookColorDialog(pin);
            return false;
        } else {
            glyph = 0;
            color = -1;
        }
        SetLook(pin, glyph, color);
        return true;
    }
    // "Open in Explorer": the column's folder.
    if (spot.row == -2) {
        const std::wstring path = m_columns[spot.column].path;
        Close();
        OpenPath(path.starts_with(kPlacePrefix) ? L"shell:MyComputerFolder"
                                                : path);
        return false;
    }
    const FolderEntry* item = ItemAt(spot);
    if (!item) {
        return false;
    }
    switch (item->kind) {
        case FolderKind::AddFolder: {
            if (m_dialogShown) {
                return false;
            }
            auto* job = new FolderPickJob{
                m_hwnd, Tr(L"Pin a folder to the island",
                           L"Fixar uma pasta na ilha"),
                kPickPin, {}};
            m_dialogShown = true;
            if (HANDLE thread = CreateThread(nullptr, 0, FolderPickThreadProc,
                                             job, 0, nullptr)) {
                m_threads.push_back(thread);
            } else {
                m_dialogShown = false;
                delete job;
            }
            return false;
        }
        case FolderKind::WindowsPlaces:
            OpenColumn(spot);
            return true;
        // A Windows place: its pin pins it; a click elsewhere on it opens it
        // like a pinned one (below).
        case FolderKind::PlaceToPin:
            if (button != RowButton::Pin) {
                break;
            }
            m_pins.push_back(std::wstring(kPlacePrefix) + item->path);
            SavePins();
            LoadPins();
            Layout();
            return true;
        default:
            break;
    }
    // A favorite file, used (opened, copied): still there? Gone from its
    // place, it's let go; on a drive not there now, said so. (Its "x" takes
    // it out either way.)
    if (item->pinned && item->kind == FolderKind::File &&
        button != RowButton::Remove) {
        const FavoritePlace place = FavoritePlaceNow(item->path);
        if (place != FavoritePlace::Here) {
            const std::wstring name = item->name;
            if (place == FavoritePlace::Gone) {
                m_pins.erase(m_pins.begin() + spot.row);
                SavePins();
                ReloadPinsKeepingColumns();
                StartIslandAnimation();
                Layout();
            }
            ShowIslandNotice(
                0xE734, 0xFF9F0A,
                place == FavoritePlace::Gone
                    ? Tr(L"Favorite moved or deleted",
                         L"Favorito movido ou excluído")
                    : Tr(L"The favorite's drive isn't connected",
                         L"A unidade do favorito não está conectada"),
                name);
            return true;
        }
    }
    // A file's star: a favorite (with the pins), or not anymore.
    if (button == RowButton::Pin && item->kind == FolderKind::File) {
        ToggleFavorite(item->path);
        return true;
    }
    // A pin's pencil: its icons and colors beside it (again: they go).
    if (button == RowButton::Edit && item->pinned) {
        if (m_columns.size() > 1 &&
            m_columns[1].lookPin == m_pins[spot.row]) {
            CloseLook();
        } else {
            OpenLook(spot.row);
        }
        return true;
    }
    if (button == RowButton::Remove && item->pinned) {
        // Unpinned (one row of the first column), its icon and color gone.
        const std::wstring pin = m_pins[spot.row];
        m_pins.erase(m_pins.begin() + spot.row);
        SavePins();
        SetLook(pin, 0, -1);
        LoadPins();
        Layout();
        return true;
    }
    if (button == RowButton::Pin) {
        if (std::find(m_pins.begin(), m_pins.end(), item->path) ==
            m_pins.end()) {
            m_pins.push_back(item->path);
            SavePins();
            // The first column again, the rest as it was.
            std::vector<Column> rest(m_columns.begin() + 1, m_columns.end());
            LoadPins();
            m_columns.insert(m_columns.end(), rest.begin(), rest.end());
            m_first = m_columns.size() > kShownColumns
                          ? m_columns.size() - kShownColumns
                          : 0;
            Layout();
        }
        return true;
    }
    // A file's copy buttons: a check on the one used a moment.
    if (button == RowButton::CopyFile || button == RowButton::CopyPath) {
        const bool copied =
            button == RowButton::CopyFile
                ? CopyFileToClipboard(m_hwnd, item->path)
                : CopyTextToClipboard(m_hwnd, item->path);
        if (copied) {
            m_copiedPath = item->path;
            m_copiedButton = button;
            SetTimer(m_hwnd, kCopiedTimerId, kCopiedMs, nullptr);
        }
        return true;
    }
    // A folder (or a drive, This PC): what's in it, beside it.
    if (Navigable(spot)) {
        OpenColumn(spot);
        return true;
    }
    const std::wstring path = ItemPath(*item);
    if (path.empty()) {
        return false;
    }
    Close();
    OpenPath(path.starts_with(kPlacePrefix) ? L"shell:MyComputerFolder" : path);
    return false;
}

void FoldersPanel::Draw(ID2D1RenderTarget* target,
                        ID2D1SolidColorBrush* brush) {
    if (!m_rowFormat || !m_rowDateFormat || !m_rowIconFormat ||
        !m_rowSmallIconFormat) {
        return;
    }
    const float s = m_scale;
    const float radius = std::round(8 * s);
    const float icon = std::round(30 * s);
    const float iconSize = std::round(16 * s);
    const double now = NowSeconds();
    // (`tint`: a pin's own color for its icon, 0xRRGGBB, or -1.)
    auto drawRow = [&](const D2D1_RECT_F& row, bool lit, bool opened,
                       WCHAR glyph, ID2D1Bitmap* bitmap,
                       const std::wstring& text, bool arrow, float alpha,
                       float endRoom = 0, int tint = -1) {
        const float shown = Hover(row, lit);
        if (opened || shown > 0.01f) {
            brush->SetColor(opened ? Accent(0.2f) : Fg(0.09f * shown));
            GlassFill(target, {row, radius, radius}, brush);
        }
        if (bitmap) {
            // (A thumbnail keeps its proportions.)
            const float x = row.left + (icon - iconSize) / 2;
            const float y = (row.top + row.bottom - iconSize) / 2;
            DrawBitmapSmooth(target, bitmap,
                             FitInBox(bitmap->GetSize(),
                                      {x, y, x + iconSize, y + iconSize}),
                             alpha);
        } else if (glyph) {
            brush->SetColor(tint >= 0 ? RgbColor(tint, alpha)
                                      : Fg(0.85f * alpha));
            DrawGlyph(target, &glyph, 1, m_rowIconFormat.get(),
                      {row.left, row.top, row.left + icon, row.bottom},
                      brush);
        }
        brush->SetColor(Fg(0.92f * alpha));
        // (Room for a folder's pin under the mouse.)
        const float end =
            row.right - std::round(arrow ? (lit ? 50 : 22) : 8) * s;
        const float right = std::min(end, row.right - endRoom);
        target->DrawText(text.c_str(), (UINT32)text.size(), m_rowFormat.get(),
                         {row.left + icon + std::round(2 * s), row.top, right,
                          row.bottom},
                         brush);
        if (arrow) {
            const WCHAR next = 0xE76C;
            brush->SetColor(Fg(0.45f * alpha));
            DrawGlyph(target, &next, 1, m_rowSmallIconFormat.get(),
                      {end, row.top, row.right - std::round(4 * s),
                       row.bottom},
                      brush);
        }
    };
    // The search in the folder shown last.
    if (SearchShown()) {
        DrawTextBox(target, brush, m_searchBox, m_search, m_searchEdit,
                    Tr(L"Search here (a name or *.pdf)",
                       L"Buscar aqui (um nome ou *.pdf)"),
                    true);
    }
    // Copy or move? (the first file dragged out).
    if (m_asking) {
        const float s = m_scale;
        const float r = std::round(10 * s);
        brush->SetColor(Fg(0.06f));
        GlassFill(target, {m_askRect, r, r}, brush);
        PCWSTR question = Tr(L"Files dragged out of the island are:",
                             L"Arquivos arrastados para fora da ilha são:");
        brush->SetColor(Fg(0.9f));
        target->DrawText(question, (UINT32)wcslen(question),
                         m_rowFormat.get(),
                         {m_askRect.left + std::round(10 * s), m_askRect.top,
                          m_askRect.right - std::round(10 * s),
                          m_askRect.top + std::round(28 * s)},
                         brush);
        DrawButton(target, brush, m_askCopy, Tr(L"Copied", L"Copiados"), true,
                   m_askHover == kAskCopy);
        DrawButton(target, brush, m_askMove, Tr(L"Moved", L"Movidos"), true,
                   m_askHover == kAskMove);
        // "Remember my choice": a box, ticked or not.
        const float box = std::round(14 * s);
        const float middle = (m_askRememberRect.top + m_askRememberRect.bottom) / 2;
        const D2D1_RECT_F tick{m_askRememberRect.left + std::round(10 * s),
                               middle - box / 2,
                               m_askRememberRect.left + std::round(10 * s) + box,
                               middle + box / 2};
        brush->SetColor(m_askRemember ? Accent(1) : Fg(0.5f));
        if (m_askRemember) {
            target->FillRoundedRectangle({tick, 3 * s, 3 * s}, brush);
            const WCHAR check = 0xE73E;
            brush->SetColor(AccentInk(1));
            DrawGlyph(target, &check, 1, m_rowSmallIconFormat.get(), tick,
                      brush);
        } else {
            target->DrawRoundedRectangle({tick, 3 * s, 3 * s}, brush,
                                         std::max(1.0f, s));
        }
        PCWSTR remember =
            Tr(L"Remember my choice (it's in the island's settings)",
               L"Lembrar minha escolha (fica nas configurações da ilha)");
        brush->SetColor(Fg(m_askHover == kAskRemember ? 0.95f : 0.75f));
        target->DrawText(remember, (UINT32)wcslen(remember), m_rowFormat.get(),
                         {tick.right + std::round(8 * s), m_askRememberRect.top,
                          m_askRememberRect.right, m_askRememberRect.bottom},
                         brush);
    }
    // The button back, with the folder shown last.
    if (m_back.right > m_back.left) {
        const std::wstring back =
            std::wstring(Tr(L"Back", L"Voltar"));
        drawRow(m_back, m_hover == Spot{-2, -2}, false, (WCHAR)0xE72B, nullptr,
                back, false, 0.85f);
    }
    // The arrow under the mouse (which come first): its tip, drawn over the
    // columns once they're drawn.
    D2D1_RECT_F tipButton{};
    float tipShown = 0;
    PCWSTR tipFirst = nullptr;
    PCWSTR tipWhich = nullptr;
    // A column at `area` (`i`: its place, -1 for the one going out).
    auto drawColumn = [&](const Column& column, int i,
                          const D2D1_RECT_F& area) {
        float y = area.top;
        if (i != 0) {
            // A faint line between the columns.
            brush->SetColor(Fg(0.1f));
            target->FillRectangle({area.left - std::round(3 * s), area.top,
                                   area.left - std::round(2 * s), area.bottom},
                                  brush);
        }
        // A pin's icons and colors: their title, the icons (in its color),
        // the colors (the one chosen ringed; the last one any other),
        // "Back to default" and "Apply".
        if (!column.lookPin.empty()) {
            drawRow({area.left, y, area.right, y + m_rowHeight}, false, false,
                    (WCHAR)0xE790, nullptr,
                    Tr(L"Icon and color", L"Ícone e cor"), false, 0.75f);
            const FolderLook* look = LookOf(column.lookPin);
            const WCHAR current = look ? look->glyph : 0;
            const int tint = look ? look->color : -1;
            const bool custom =
                tint >= 0 && std::find(std::begin(kFolderLookColors),
                                       std::end(kFolderLookColors),
                                       tint) == std::end(kFolderLookColors);
            for (int k = 0; k <= kLookApply; k++) {
                const D2D1_RECT_F cell = LookCell(area, k);
                if (cell.bottom > area.bottom + 0.5f) {
                    continue;
                }
                const bool lit = i >= 0 && m_hover == Spot{i, kLookRow - k};
                if (k >= kLookReset) {
                    const bool apply = k == kLookApply;
                    drawRow(cell, lit, false,
                            apply ? (WCHAR)0xE73E : (WCHAR)0xE7A7, nullptr,
                            apply ? Tr(L"Apply", L"Aplicar")
                                  : Tr(L"Back to default", L"Restaurar padrão"),
                            false, 0.85f);
                    continue;
                }
                const float shown = Hover(cell, lit);
                const float inset = std::round(2 * s);
                const D2D1_RECT_F inner{cell.left + inset, cell.top + inset,
                                        cell.right - inset,
                                        cell.bottom - inset};
                const bool chosen =
                    k < kLookGlyphs && kFolderLookGlyphs[k] == current;
                if (chosen || shown > 0.01f) {
                    brush->SetColor(chosen ? Accent(0.25f)
                                           : Fg(0.09f * shown));
                    GlassFill(target, {inner, radius, radius}, brush);
                }
                if (k < kLookGlyphs) {
                    brush->SetColor(tint >= 0 ? RgbColor(tint, 1)
                                              : Fg(0.85f));
                    DrawGlyph(target, &kFolderLookGlyphs[k], 1,
                              m_rowIconFormat.get(), cell, brush);
                    continue;
                }
                const D2D1_POINT_2F middle{(cell.left + cell.right) / 2,
                                           (cell.top + cell.bottom) / 2};
                const float dot = std::round(8 * s);
                const bool last = k == kLookReset - 1;
                const int color =
                    last ? (custom ? tint : -1)
                         : kFolderLookColors[k - kLookGlyphs];
                if (color >= 0) {
                    brush->SetColor(RgbColor(color, 1));
                    target->FillEllipse({middle, dot, dot}, brush);
                } else {
                    // (Any other color: the palette.)
                    const WCHAR palette = 0xE790;
                    brush->SetColor(Fg(0.7f));
                    DrawGlyph(target, &palette, 1, m_rowSmallIconFormat.get(),
                              cell, brush);
                }
                if (color >= 0 && color == tint) {
                    brush->SetColor(Fg(0.9f));
                    target->DrawEllipse(
                        {middle, dot + std::round(3 * s),
                         dot + std::round(3 * s)},
                        brush, std::max(1.0f, 1.5f * s));
                }
            }
            return;
        }
        if (i != 0) {
            const D2D1_RECT_F header{area.left, y, area.right,
                                     y + m_rowHeight};
            const bool onHeader = m_hover == Spot{(int)i, -2};
            drawRow(header, onHeader && m_buttonHover == RowButton::None, false,
                    (WCHAR)0xE8A7, nullptr,
                    Tr(L"Open in Explorer", L"Abrir no Explorador"), false,
                    0.75f, HeaderButtonsWidth());
            // The order: its menu (with the order's name, a funnel while one
            // kind is shown), and beside it an arrow for its direction (down:
            // Z to A, the newest, the largest first).
            {
                const float buttonTop = header.top + std::round(4 * s);
                const float buttonBottom = header.bottom - std::round(4 * s);
                const float sortRight = header.right - HeaderButtonsWidth() +
                                        SortButtonWidth();
                const D2D1_RECT_F sort{sortRight - SortButtonWidth(),
                                       buttonTop, sortRight, buttonBottom};
                const bool sortLit =
                    (onHeader && m_buttonHover == RowButton::Sort) ||
                    column.menu;
                const float sortRadius = (sort.bottom - sort.top) / 2;
                brush->SetColor(Fg(0.06f + 0.08f * Hover(sort, sortLit)));
                GlassFill(target, {sort, sortRadius, sortRadius}, brush);
                const bool filtered = column.filter != FolderFilter::All;
                const WCHAR glyph = filtered ? 0xE71C : 0xE8CB;
                brush->SetColor(filtered ? Accent(1) : Fg(0.7f));
                DrawGlyph(target, &glyph, 1, m_rowSmallIconFormat.get(),
                          {sort.left + std::round(4 * s), sort.top,
                           sort.left + std::round(22 * s), sort.bottom},
                          brush);
                PCWSTR order =
                    column.order.sort == FolderSort::Date ? Tr(L"Date", L"Data")
                    : column.order.sort == FolderSort::Type
                        ? Tr(L"Type", L"Tipo")
                    : column.order.sort == FolderSort::Size
                        ? Tr(L"Size", L"Tamanho")
                        : Tr(L"Name", L"Nome");
                brush->SetColor(Fg(0.7f));
                target->DrawText(order, (UINT32)wcslen(order),
                                 m_rowFormat.get(),
                                 {sort.left + std::round(24 * s), sort.top,
                                  sort.right - std::round(4 * s), sort.bottom},
                                 brush);
                const D2D1_RECT_F direction{
                    header.right - DirectionButtonWidth(), buttonTop,
                    header.right, buttonBottom};
                const bool directionLit =
                    onHeader && m_buttonHover == RowButton::Direction;
                brush->SetColor(
                    Fg(0.06f + 0.08f * Hover(direction, directionLit)));
                GlassFill(target, {direction, sortRadius, sortRadius}, brush);
                // (Arrow up / arrow down.)
                const WCHAR arrow = column.order.descending ? 0xE74B : 0xE74A;
                brush->SetColor(Fg(0.7f));
                DrawGlyph(target, &arrow, 1, m_rowSmallIconFormat.get(),
                          direction, brush);
                if (i >= 0) {
                    const float tip = Hover(HoverKey(7300, i), directionLit);
                    if (tip > 0.01f) {
                        tipButton = direction;
                        tipShown = tip;
                        tipFirst = OrderWay(column.order);
                        tipWhich =
                            column.order.sort == FolderSort::Date
                                ? Tr(L"By date modified",
                                     L"Pela data de modificação")
                            : column.order.sort == FolderSort::Type
                                ? Tr(L"By type", L"Pelo tipo")
                            : column.order.sort == FolderSort::Size
                                ? Tr(L"By size", L"Pelo tamanho")
                                : Tr(L"By name", L"Pelo nome");
                    }
                }
            }
            y += m_rowHeight;
        }
        // Its menu open: the orders (the one chosen ticked, with its
        // direction), folders mixed with the files, and the kinds shown.
        if (column.menu) {
            const auto& rows = MenuRows();
            for (size_t k = 0; k < rows.size(); k++) {
                if (y + m_rowHeight > area.bottom + 0.5f) {
                    break;
                }
                const D2D1_RECT_F rect{area.left, y, area.right,
                                       y + m_rowHeight};
                y += m_rowHeight;
                const MenuRow& row = rows[k];
                if (row.part == MenuPart::Caption) {
                    PCWSTR caption = row.value == 0
                                         ? Tr(L"Sort by", L"Ordenar por")
                                         : Tr(L"Show", L"Mostrar");
                    brush->SetColor(Fg(0.5f));
                    target->DrawText(caption, (UINT32)wcslen(caption),
                                     m_rowFormat.get(),
                                     {rect.left + std::round(10 * s), rect.top,
                                      rect.right, rect.bottom},
                                     brush);
                    continue;
                }
                const bool lit =
                    i >= 0 && m_hover == Spot{i, kMenuRow - (int)k};
                const bool sorted = row.part == MenuPart::Sort &&
                                    (int)column.order.sort == row.value;
                const bool ticked =
                    sorted ||
                    (row.part == MenuPart::Mixed && column.order.mixed) ||
                    (row.part == MenuPart::Filter &&
                     (int)column.filter == row.value);
                PCWSTR text =
                    row.part == MenuPart::Sort
                        ? SortName((FolderSort)row.value)
                    : row.part == MenuPart::Mixed
                        ? Tr(L"Folders mixed with files",
                             L"Pastas junto com os arquivos")
                        : FilterName((FolderFilter)row.value);
                drawRow(rect, lit, false, ticked ? (WCHAR)0xE73E : 0, nullptr,
                        text, false, 1.0f, sorted ? std::round(30 * s) : 0);
                if (sorted) {
                    const WCHAR arrow =
                        column.order.descending ? 0xE74B : 0xE74A;
                    brush->SetColor(Fg(0.6f));
                    DrawGlyph(target, &arrow, 1, m_rowSmallIconFormat.get(),
                              {rect.right - std::round(28 * s),
                               rect.top, rect.right - std::round(4 * s),
                               rect.bottom},
                              brush);
                }
            }
            return;
        }
        // Files held over the column's own folder: the column outlined.
        if (m_dropSpot == Spot{(int)i, -3}) {
            brush->SetColor(Accent(0.9f));
            target->DrawRoundedRectangle(
                {{area.left + s, area.top + s, area.right - s, area.bottom - s},
                 radius, radius},
                brush, std::max(1.0f, 1.5f * s));
        }
        if (column.loading) {
            // (Only for a folder that takes a while.)
            if (now - column.loadingSince >= kReadingDelay) {
                brush->SetColor(Fg(0.5f));
                PCWSTR text = Tr(L"Reading…", L"Lendo…");
                target->DrawText(text, (UINT32)wcslen(text), m_rowFormat.get(),
                                 {area.left + icon, y, area.right,
                                  y + m_rowHeight},
                                 brush);
            }
            return;
        }
        if (column.items.empty() && i != 0) {
            brush->SetColor(Fg(0.5f));
            PCWSTR text = !column.all.empty()
                              ? Tr(L"Nothing found", L"Nada encontrado")
                              : Tr(L"Empty", L"Vazia");
            target->DrawText(text, (UINT32)wcslen(text), m_rowFormat.get(),
                             {area.left + icon, y, area.right,
                              y + m_rowHeight},
                             brush);
            return;
        }
        for (int row = column.top; row < (int)column.items.size(); row++) {
            if (y + m_rowHeight > area.bottom + 0.5f) {
                break;
            }
            // (The pins in the order they'd have, one being dragged.)
            const int source = i == 0 ? PinShownAt(row) : row;
            const FolderEntry& item = column.items[source];
            // (Coming in: fading in as it rises into place.)
            const float in = RowIn(column, row - column.top, item.path, now);
            const float rise = (1 - in) * std::round(6 * s);
            const D2D1_RECT_F rect{area.left, y + rise, area.right,
                                   y + m_rowHeight + rise};
            const bool held = i == 0 && m_dragging && row == m_dropRow;
            // (Files held over it: lit too.)
            const bool lit = held ||
                             (!m_dragging && m_hover == Spot{(int)i, row}) ||
                             m_dropSpot == Spot{(int)i, row};
            // (A row's buttons only under the mouse, not under files.)
            const bool buttons = lit && m_dropSpot == Spot{-1, -1};
            const FolderPlace* place =
                item.kind == FolderKind::Place ||
                        item.kind == FolderKind::PlaceToPin
                    ? FindFolderPlace(item.path)
                    : nullptr;
            // (A pin: its own icon, or a folder's.)
            const WCHAR glyph = item.glyph ? item.glyph
                                : place    ? place->glyph
                                : item.kind == FolderKind::Folder && item.pinned
                                    ? (WCHAR)0xE8B7
                                : item.kind == FolderKind::Drive
                                    ? (WCHAR)0xEDA2
                                : item.kind == FolderKind::AddFolder
                                    ? (WCHAR)0xE710
                                : item.kind == FolderKind::WindowsPlaces
                                    ? (WCHAR)0xE80F
                                : item.kind == FolderKind::More
                                    ? (WCHAR)0xE712
                                : item.kind == FolderKind::PlaceToPin
                                    ? (WCHAR)0xE718
                                    : (WCHAR)0;
            ID2D1Bitmap* thumbnail =
                item.kind == FolderKind::File &&
                        g_settings.islandFolderThumbnails
                    ? m_thumbs.Get(target, m_wic.get(), m_hwnd, item.path,
                                   (int)std::ceil(iconSize))
                    : nullptr;
            ID2D1Bitmap* bitmap =
                thumbnail ? thumbnail
                : item.kind == FolderKind::File ||
                        (item.kind == FolderKind::Folder && !item.pinned)
                    ? TypeIcon(target, item)
                    : nullptr;
            const bool navigable = i >= 0 && Navigable({(int)i, source});
            // (Just dropped here: lit a moment as it shows.)
            if (const float dropped = DroppedLit(column, item, now);
                dropped > 0) {
                brush->SetColor(Accent(0.35f * dropped));
                target->FillRoundedRectangle({rect, radius, radius}, brush);
            }
            // Sorted by date, type or size: that at the end (a folder has no
            // size), in one column for files and folders, the name cut
            // before it. A folder's arrow shows there only under the mouse
            // (with its pin, the date left of them).
            const std::wstring* info =
                column.order.sort == FolderSort::Date   ? &item.date
                : column.order.sort == FolderSort::Type ? &item.type
                : column.order.sort == FolderSort::Size ? &item.sizeText
                                                        : nullptr;
            const bool dated = info && !info->empty();
            const bool arrow = navigable && !(buttons && item.pinned) &&
                               (!dated || lit);
            // (A file's star and copy buttons under the mouse, see below.)
            const bool copyButtons =
                buttons && item.kind == FolderKind::File && !m_dragging;
            const float dateRight =
                rect.right - std::round(arrow ? 50 : copyButtons ? 80 : 8) * s;
            const float dateLeft =
                dateRight -
                std::round(
                    (column.order.sort == FolderSort::Type ? 130 : 112) * s);
            // (A pin's buttons under the mouse: its name cut before them.)
            const float endRoom =
                dated ? rect.right - dateLeft + std::round(4 * s)
                : copyButtons ? std::round(80 * s)
                : buttons && item.pinned && !m_dragging ? std::round(56 * s)
                : buttons && item.kind == FolderKind::PlaceToPin
                    ? std::round(50 * s)
                    : 0;
            drawRow(rect, lit, column.opened == source, bitmap ? 0 : glyph,
                    bitmap, item.name, arrow, in, endRoom,
                    item.pinned ? item.color : -1);
            if (dated) {
                brush->SetColor(Fg(0.5f * in));
                target->DrawText(info->c_str(), (UINT32)info->size(),
                                 m_rowDateFormat.get(),
                                 {dateLeft, rect.top, dateRight, rect.bottom},
                                 brush);
            }
            // A file: copy it (pasted in a folder, it goes there), and copy
            // its path (the one just used shows a check a moment), ending
            // `right` (its path's button there).
            auto drawCopyButtons = [&](float right) {
                const RowButton kinds[2] = {RowButton::CopyFile,
                                            RowButton::CopyPath};
                const WCHAR glyphs[2] = {0xE8C8, 0xE71B};
                for (int k = 0; k < 2; k++) {
                    const bool done = m_copiedButton == kinds[k] &&
                                      m_copiedPath == item.path;
                    const WCHAR shown = done ? (WCHAR)0xE73E : glyphs[k];
                    brush->SetColor(done ? Accent(0.95f)
                                    : m_buttonHover == kinds[k] ? Fg(0.95f)
                                                                : Fg(0.5f));
                    const float end = right - std::round(24 * (1 - k) * s);
                    DrawGlyph(target, &shown, 1, m_rowSmallIconFormat.get(),
                              {end - std::round(24 * s), rect.top, end,
                               rect.bottom},
                              brush);
                }
            };
            // A pinned row under the mouse: its "x" and its pencil (its icon
            // and color), a favorite file's copy buttons instead; a folder or
            // a Windows place: its pin; a file: its star, then its copy
            // buttons.
            if (buttons && item.pinned && !m_dragging) {
                const WCHAR close = 0xE711;
                brush->SetColor(m_buttonHover == RowButton::Remove
                                    ? D2D1::ColorF(1, 0.36f, 0.3f, 1)
                                    : Fg(0.55f));
                DrawGlyph(target, &close, 1, m_rowSmallIconFormat.get(),
                          {rect.right - std::round(28 * s), rect.top,
                           rect.right - std::round(4 * s), rect.bottom},
                          brush);
                if (item.kind == FolderKind::File) {
                    drawCopyButtons(rect.right - std::round(28 * s));
                    y += m_rowHeight;
                    continue;
                }
                const WCHAR edit = 0xE70F;
                brush->SetColor(m_buttonHover == RowButton::Edit ? Fg(0.95f)
                                                                 : Fg(0.55f));
                DrawGlyph(target, &edit, 1, m_rowSmallIconFormat.get(),
                          {rect.right - std::round(52 * s), rect.top,
                           rect.right - std::round(28 * s), rect.bottom},
                          brush);
            } else if (buttons && (item.kind == FolderKind::Folder ||
                                   item.kind == FolderKind::PlaceToPin)) {
                const bool pinned =
                    std::find(m_pins.begin(), m_pins.end(), item.path) !=
                    m_pins.end();
                const WCHAR pin = kGlyphPin;
                brush->SetColor(pinned ? Accent(0.9f)
                                : m_buttonHover == RowButton::Pin
                                    ? Fg(0.95f)
                                    : Fg(0.5f));
                DrawGlyph(target, &pin, 1, m_rowSmallIconFormat.get(),
                          {rect.right - std::round(50 * s), rect.top,
                           rect.right - std::round(26 * s), rect.bottom},
                          brush);
            } else if (copyButtons) {
                // (Its star lit in the accent color while it's a favorite.)
                const bool favorite = IsFavorite(item.path);
                const WCHAR star = favorite ? (WCHAR)0xE735 : (WCHAR)0xE734;
                brush->SetColor(favorite ? Accent(0.95f)
                                : m_buttonHover == RowButton::Pin ? Fg(0.95f)
                                                                  : Fg(0.5f));
                DrawGlyph(target, &star, 1, m_rowSmallIconFormat.get(),
                          {rect.right - std::round(76 * s), rect.top,
                           rect.right - std::round(52 * s), rect.bottom},
                          brush);
                drawCopyButtons(rect.right - std::round(4 * s));
            }
            y += m_rowHeight;
        }
    };
    // The columns, sliding while one goes out of view on a side (the one
    // leaving drawn too), kept between the panel's sides.
    const float slide = ColumnSlide(now);
    const bool leaving = now - m_leavingAt < kSlideSeconds;
    const bool moving = slide != 0 || leaving;
    D2D1_MATRIX_3X2_F base;
    target->GetTransform(&base);
    if (moving && m_first < m_columns.size()) {
        const D2D1_RECT_F& first = m_columns[m_first].rect;
        target->PushAxisAlignedClip(
            {first.left - std::round(4 * s), first.top,
             (float)m_size.cx - std::round(4 * s), (float)m_size.cy},
            D2D1_ANTIALIAS_MODE_ALIASED);
        target->SetTransform(D2D1::Matrix3x2F::Translation(slide, 0) * base);
    }
    if (slide > 0 && m_first > 0 && m_first < m_columns.size()) {
        const D2D1_RECT_F& first = m_columns[m_first].rect;
        drawColumn(m_columns[m_first - 1], (int)m_first - 1,
                   {first.left - m_columnStep, first.top,
                    first.right - m_columnStep, first.bottom});
    }
    for (size_t i = m_first; i < m_columns.size(); i++) {
        drawColumn(m_columns[i], (int)i, m_columns[i].rect);
    }
    if (leaving) {
        // (Fading where it was, when the others stay.)
        const float alpha =
            slide != 0
                ? 1.0f
                : (float)std::clamp(1 - (now - m_leavingAt) / kSlideSeconds,
                                    0.0, 1.0);
        winrt::com_ptr<ID2D1Layer> layer;
        if (alpha < 0.999f) {
            target->CreateLayer(nullptr, layer.put());
            target->PushLayer(
                D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                                      D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                      D2D1::IdentityMatrix(), alpha),
                layer.get());
        }
        drawColumn(m_leaving, -1, m_leaving.rect);
        if (layer) {
            target->PopLayer();
        }
    }
    if (moving && m_first < m_columns.size()) {
        target->SetTransform(base);
        target->PopAxisAlignedClip();
    }
    // The arrow's tip: which come first, and by what, in a bubble under it
    // (its right side at the arrow's).
    if (tipShown > 0.01f && !moving && tipFirst && tipWhich) {
        PCWSTR first = tipFirst;
        PCWSTR which = tipWhich;
        // (Laid out again only for other texts or size.)
        if (m_tipText != first || m_tipWhichText != which ||
            m_tipFormat != m_rowFormat.get()) {
            m_tipText = first;
            m_tipWhichText = which;
            m_tipFormat = m_rowFormat.get();
            m_tipFirst = m_tipWhich = nullptr;
            if (SUCCEEDED(m_dwrite->CreateTextLayout(
                    first, (UINT32)wcslen(first), m_rowFormat.get(), 1000,
                    m_rowHeight, m_tipFirst.put())) &&
                SUCCEEDED(m_dwrite->CreateTextLayout(
                    which, (UINT32)wcslen(which), m_rowDateFormat.get(), 1000,
                    m_rowHeight, m_tipWhich.put()))) {
                m_tipWhich->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                m_tipFirst->SetParagraphAlignment(
                    DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
                m_tipWhich->SetParagraphAlignment(
                    DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            } else {
                m_tipFirst = m_tipWhich = nullptr;
            }
        }
        IDWriteTextLayout* firstLayout = m_tipFirst.get();
        IDWriteTextLayout* whichLayout = m_tipWhich.get();
        DWRITE_TEXT_METRICS firstMetrics{}, whichMetrics{};
        if (firstLayout && whichLayout) {
            firstLayout->GetMetrics(&firstMetrics);
            whichLayout->GetMetrics(&whichMetrics);
            const float pad = std::round(8 * s);
            const float width =
                std::ceil(std::max(firstMetrics.width, whichMetrics.width)) +
                pad * 2;
            const float height = std::ceil(firstMetrics.height +
                                           whichMetrics.height) +
                                 pad * 1.5f;
            const float top = tipButton.bottom + std::round(6 * s);
            const D2D1_RECT_F bubble{tipButton.right - width, top,
                                     tipButton.right, top + height};
            const float r = std::round(6 * s);
            D2D1_COLOR_F fill = Bg(1);
            fill.a = tipShown;
            brush->SetColor(fill);
            target->FillRoundedRectangle({bubble, r, r}, brush);
            brush->SetColor(Fg(0.08f * tipShown));
            GlassFill(target, {bubble, r, r}, brush);
            brush->SetColor(Fg(0.15f * tipShown));
            target->DrawRoundedRectangle({bubble, r, r}, brush,
                                         std::max(1.0f, s));
            brush->SetColor(Fg(0.92f * tipShown));
            target->DrawTextLayout({bubble.left + pad,
                                    bubble.top + pad * 0.75f},
                                   firstLayout, brush);
            brush->SetColor(Fg(0.55f * tipShown));
            target->DrawTextLayout(
                {bubble.left + pad,
                 bubble.top + pad * 0.75f + firstMetrics.height},
                whichLayout, brush);
        }
    }
    // The file the mouse rests on: its thumbnail big beside its column (or
    // before it, without room after it), inside the columns.
    if (m_previewShown && m_hover == m_previewSpot &&
        g_settings.islandFolderThumbnails && !m_dragging) {
        const FolderEntry* item = ItemAt(m_previewSpot);
        ID2D1Bitmap* big =
            item && m_previewSpot.column < (int)m_columns.size()
                ? m_thumbs.Get(target, m_wic.get(), m_hwnd, item->path,
                               (int)std::round(200 * s))
                : nullptr;
        if (big) {
            const Column& column = m_columns[m_previewSpot.column];
            const float pad = std::round(8 * s);
            const D2D1_SIZE_F size = big->GetSize();
            const float most = std::round(200 * s);
            const float fit =
                std::min(1.0f, most / std::max(size.width, size.height));
            const float w = std::round(size.width * fit) + pad * 2;
            const float h = std::round(size.height * fit) + pad * 2;
            const float areaLeft = m_columns[m_first].rect.left;
            const float areaRight = m_columns.back().rect.right;
            const float areaTop = m_columns[m_first].rect.top;
            const float areaBottom = column.rect.bottom;
            const int header = m_previewSpot.column > 0 ? 1 : 0;
            const float rowTop =
                column.rect.top +
                (header + m_previewSpot.row - column.top) * m_rowHeight;
            float left = column.rect.right + std::round(6 * s);
            if (left + w > areaRight) {
                left = column.rect.left - std::round(6 * s) - w;
            }
            left = std::clamp(left, areaLeft, std::max(areaLeft, areaRight - w));
            const float top = std::clamp(rowTop, areaTop,
                                         std::max(areaTop, areaBottom - h));
            const D2D1_RECT_F box{left, top, left + w, top + h};
            const float r = std::round(10 * s);
            brush->SetColor(Bg(1));
            target->FillRoundedRectangle({box, r, r}, brush);
            brush->SetColor(Fg(0.15f));
            target->DrawRoundedRectangle({box, r, r}, brush,
                                         std::max(1.0f, s));
            DrawBitmapSmooth(target, big,
                             FitInBox(size, {box.left + pad, box.top + pad,
                                             box.right - pad,
                                             box.bottom - pad}));
        }
    }
}

