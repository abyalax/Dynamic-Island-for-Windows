#pragma once
constexpr size_t kThumbnailsKept = 240;

struct ThumbnailMade {
    std::wstring path;
    int pixels;
    HBITMAP bitmap;  // Null: it has none.
    bool alpha;      // Its own transparency (not a photo's empty one).
};

struct ThumbnailQueue {
    SRWLOCK lock = SRWLOCK_INIT;
    std::vector<std::pair<std::wstring, int>> asked;
    HANDLE wake = nullptr;
    HANDLE stop = nullptr;
    HWND notify = nullptr;
};

DWORD WINAPI ThumbnailThreadProc(LPVOID parameter) {
    const std::unique_ptr<std::shared_ptr<ThumbnailQueue>> held(
        (std::shared_ptr<ThumbnailQueue>*)parameter);
    ThumbnailQueue& queue = **held;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const HANDLE waits[2] = {queue.stop, queue.wake};
    while (WaitForMultipleObjects(2, waits, FALSE, INFINITE) ==
           WAIT_OBJECT_0 + 1) {
        for (;;) {
            std::pair<std::wstring, int> job;
            AcquireSRWLockExclusive(&queue.lock);
            const bool any = !queue.asked.empty();
            if (any) {
                // (The newest first: the rows shown now.)
                job = std::move(queue.asked.back());
                queue.asked.pop_back();
            }
            ReleaseSRWLockExclusive(&queue.lock);
            if (!any || WaitForSingleObject(queue.stop, 0) == WAIT_OBJECT_0) {
                break;
            }
            auto made = std::make_unique<ThumbnailMade>(
                ThumbnailMade{job.first, job.second, nullptr, false});
            winrt::com_ptr<IShellItemImageFactory> factory;
            if (SUCCEEDED(SHCreateItemFromParsingName(
                    job.first.c_str(), nullptr,
                    IID_PPV_ARGS(factory.put())))) {
                factory->GetImage({job.second, job.second},
                                  SIIGBF_THUMBNAILONLY | SIIGBF_BIGGERSIZEOK,
                                  &made->bitmap);
            }
            // (A photo's comes with every pixel's alpha at 0.)
            if (BITMAP info{}; made->bitmap &&
                               GetObject(made->bitmap, sizeof(info), &info) &&
                               info.bmBitsPixel == 32 && info.bmBits) {
                const BYTE* bits = (const BYTE*)info.bmBits;
                const size_t count = (size_t)info.bmWidth * info.bmHeight;
                for (size_t i = 0; i < count && !made->alpha; i++) {
                    made->alpha = bits[i * 4 + 3] != 0;
                }
            }
            if (PostMessage(queue.notify, WM_APP_FILE_THUMBNAIL, 0,
                            (LPARAM)made.get())) {
                made.release();
            } else if (made->bitmap) {
                DeleteObject(made->bitmap);
            }
        }
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    return 0;
}

class ThumbnailCache {
   public:
    // The thumbnail of the file at `path`, `pixels` big at most (its
    // proportions kept), as a picture of `target`; null while it's being
    // made (asked once) or when it has none.
    ID2D1Bitmap* Get(ID2D1RenderTarget* target,
                     IWICImagingFactory* wic,
                     HWND notify,
                     const std::wstring& path,
                     int pixels) {
        for (auto& kept : m_kept) {
            if (kept.pixels == pixels && kept.path == path) {
                kept.used = ++m_uses;
                if (!kept.bitmap && kept.source && wic) {
                    kept.bitmap = MakeBitmap(target, wic, kept);
                }
                return kept.bitmap.get();
            }
        }
        if (!Start(notify)) {
            return nullptr;
        }
        m_kept.push_back({path, pixels, nullptr, nullptr, false, ++m_uses});
        Trim();
        // (A file on another computer has none asked for: a server that's off
        // would hold the thread, and the panel's end waiting for it.)
        if (!IsLocalDrivePath(path)) {
            return nullptr;
        }
        AcquireSRWLockExclusive(&m_queue->lock);
        m_queue->asked.push_back({path, pixels});
        ReleaseSRWLockExclusive(&m_queue->lock);
        SetEvent(m_queue->wake);
        return nullptr;
    }

    // One made (WM_APP_FILE_THUMBNAIL's): kept, the panel drawn again.
    void Take(LPARAM lParam) {
        std::unique_ptr<ThumbnailMade> made((ThumbnailMade*)lParam);
        for (auto& kept : m_kept) {
            if (kept.pixels == made->pixels && kept.path == made->path &&
                !kept.source) {
                kept.source = made->bitmap;
                kept.alpha = made->alpha;
                made->bitmap = nullptr;
                break;
            }
        }
        if (made->bitmap) {
            DeleteObject(made->bitmap);
        }
    }

    // Its target went (or another scale): the pictures made again from the
    // thumbnails kept.
    void ForgetBitmaps() {
        for (auto& kept : m_kept) {
            kept.bitmap = nullptr;
        }
    }

    // The thread stopped (its panel going) and everything let go.
    void Stop() {
        if (m_thread) {
            SetEvent(m_queue->stop);
            WaitForSingleObject(m_thread, INFINITE);
            CloseHandle(m_thread);
            m_thread = nullptr;
            // (Posted and not taken yet.)
            MSG msg;
            while (m_queue->notify &&
                   PeekMessage(&msg, m_queue->notify, WM_APP_FILE_THUMBNAIL,
                               WM_APP_FILE_THUMBNAIL, PM_REMOVE)) {
                Take(msg.lParam);
            }
        }
        if (m_queue) {
            CloseHandle(m_queue->wake);
            CloseHandle(m_queue->stop);
            m_queue = nullptr;
        }
        for (auto& kept : m_kept) {
            if (kept.source) {
                DeleteObject(kept.source);
            }
        }
        m_kept.clear();
    }

   private:
    struct Kept {
        std::wstring path;
        int pixels;
        HBITMAP source;  // As Windows made it (null: none, or not yet).
        winrt::com_ptr<ID2D1Bitmap> bitmap;
        bool alpha;
        ULONGLONG used;
    };

    bool Start(HWND notify) {
        if (m_thread) {
            return true;
        }
        if (!notify) {
            return false;
        }
        auto queue = std::make_shared<ThumbnailQueue>();
        queue->notify = notify;
        queue->wake = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        queue->stop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (!queue->wake || !queue->stop) {
            if (queue->wake) {
                CloseHandle(queue->wake);
            }
            if (queue->stop) {
                CloseHandle(queue->stop);
            }
            return false;
        }
        auto* held = new std::shared_ptr<ThumbnailQueue>(queue);
        m_thread = CreateThread(nullptr, 0, ThumbnailThreadProc, held, 0,
                                nullptr);
        if (!m_thread) {
            delete held;
            CloseHandle(queue->wake);
            CloseHandle(queue->stop);
            return false;
        }
        m_queue = std::move(queue);
        return true;
    }

    // The least used ones go past kThumbnailsKept.
    void Trim() {
        while (m_kept.size() > kThumbnailsKept) {
            auto least = std::min_element(
                m_kept.begin(), m_kept.end(),
                [](const Kept& a, const Kept& b) { return a.used < b.used; });
            if (least->source) {
                DeleteObject(least->source);
            }
            m_kept.erase(least);
        }
    }

    static winrt::com_ptr<ID2D1Bitmap> MakeBitmap(ID2D1RenderTarget* target,
                                                  IWICImagingFactory* wic,
                                                  const Kept& kept) {
        winrt::com_ptr<IWICBitmap> wicBitmap;
        winrt::com_ptr<IWICFormatConverter> converter;
        winrt::com_ptr<ID2D1Bitmap> bitmap;
        if (SUCCEEDED(wic->CreateBitmapFromHBITMAP(
                kept.source, nullptr,
                kept.alpha ? WICBitmapUsePremultipliedAlpha
                           : WICBitmapIgnoreAlpha,
                wicBitmap.put())) &&
            SUCCEEDED(wic->CreateFormatConverter(converter.put())) &&
            SUCCEEDED(converter->Initialize(
                wicBitmap.get(), GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0,
                WICBitmapPaletteTypeMedianCut))) {
            target->CreateBitmapFromWicBitmap(converter.get(), nullptr,
                                              bitmap.put());
        }
        return bitmap;
    }

    std::vector<Kept> m_kept;
    ULONGLONG m_uses = 0;
    std::shared_ptr<ThumbnailQueue> m_queue;
    HANDLE m_thread = nullptr;
};

// `size` fitted whole in `box`, in its middle (its proportions kept).
D2D1_RECT_F FitInBox(D2D1_SIZE_F size, const D2D1_RECT_F& box) {
    const float width = box.right - box.left;
    const float height = box.bottom - box.top;
    if (size.width <= 0 || size.height <= 0) {
        return box;
    }
    const float scale = std::min(width / size.width, height / size.height);
    const float w = size.width * scale;
    const float h = size.height * scale;
    const float x = box.left + (width - w) / 2;
    const float y = box.top + (height - h) / 2;
    return {x, y, x + w, y + h};
}

// A type's icon (an extension like ".pdf", "." for none, or "\\" for
// folders), small or large, by its type only (no file is opened).
winrt::com_ptr<ID2D1Bitmap> FileTypeIcon(ID2D1RenderTarget* target,
                                         IWICImagingFactory* wic,
                                         const std::wstring& type,
                                         bool large) {
    SHFILEINFO info{};
    const std::wstring name = type == L"\\" ? L"folder" : L"file" + type;
    winrt::com_ptr<ID2D1Bitmap> bitmap;
    if (SHGetFileInfo(name.c_str(),
                      type == L"\\" ? FILE_ATTRIBUTE_DIRECTORY
                                    : FILE_ATTRIBUTE_NORMAL,
                      &info, sizeof(info),
                      SHGFI_ICON | (large ? SHGFI_LARGEICON : SHGFI_SMALLICON) |
                          SHGFI_USEFILEATTRIBUTES) &&
        info.hIcon) {
        winrt::com_ptr<IWICBitmap> wicBitmap;
        winrt::com_ptr<IWICFormatConverter> converter;
        if (wic &&
            SUCCEEDED(wic->CreateBitmapFromHICON(info.hIcon,
                                                 wicBitmap.put())) &&
            SUCCEEDED(wic->CreateFormatConverter(converter.put())) &&
            SUCCEEDED(converter->Initialize(
                wicBitmap.get(), GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0,
                WICBitmapPaletteTypeMedianCut))) {
            target->CreateBitmapFromWicBitmap(converter.get(), nullptr,
                                              bitmap.put());
        }
        DestroyIcon(info.hIcon);
    }
    return bitmap;
}

// A path without its last backslash ("C:\" stays "C:"), to compare folders.
std::wstring FolderKey(std::wstring path) {
    while (!path.empty() && path.back() == L'\\') {
        path.pop_back();
    }
    return path;
}

// Held this long on a folder (or the island's folders item) while dragging
// files: it opens.
constexpr UINT kFolderOpenMs = 600;

// A file or a folder opened as the user would. Under System32 through
// explorer.exe: the 32-bit Windhawk process would open SysWOW64's copy (a
// 32-bit cmd.exe, or nothing for what only the 64-bit one has).
void OpenPath(const std::wstring& path) {
    if (const std::wstring windows = Wow64WindowsFolderOf(path);
        !windows.empty()) {
        const std::wstring explorer = windows + L"\\explorer.exe";
        const std::wstring quoted = L"\"" + path + L"\"";
        ShellExecute(nullptr, L"open", explorer.c_str(), quoted.c_str(),
                     nullptr, SW_SHOWNORMAL);
        return;
    }
    ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr,
                 SW_SHOWNORMAL);
}

// A type's name as Explorer shows it ("PNG File", "File folder"), and the
// kind of file it is, by the type only (the file isn't opened), each type
// asked once per read.
void NameFolderTypes(std::vector<FolderEntry>& items) {
    std::unordered_map<std::wstring, std::pair<std::wstring, FolderFilter>>
        types;
    for (auto& item : items) {
        std::wstring type = L"\\";
        if (item.kind == FolderKind::File) {
            const size_t dot = item.name.find_last_of(L'.');
            type = dot == std::wstring::npos ? L"." : item.name.substr(dot);
            std::transform(type.begin(), type.end(), type.begin(), towlower);
        }
        auto it = types.find(type);
        if (it == types.end()) {
            SHFILEINFO info{};
            const std::wstring name =
                type == L"\\" ? L"folder" : L"file" + type;
            SHGetFileInfo(name.c_str(),
                          type == L"\\" ? FILE_ATTRIBUTE_DIRECTORY
                                        : FILE_ATTRIBUTE_NORMAL,
                          &info, sizeof(info),
                          SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);
            it = types
                     .emplace(type, std::pair{std::wstring(info.szTypeName),
                                              type == L"\\"
                                                  ? FolderFilter::Folders
                                                  : FolderFilterOf(type)})
                     .first;
        }
        item.type = it->second.first;
        item.filter = it->second.second;
    }
}

// A folder's contents (folders first, then files, sorted as Explorer
// sorts), or the drives for This PC.
DWORD WINAPI FolderListThreadProc(LPVOID parameter) {
    auto* job = (FolderListJob*)parameter;
    // (For the types' names.)
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (job->path == std::wstring(kPlacePrefix) + L"thispc") {
        const DWORD drives = GetLogicalDrives();
        for (int i = 0; i < 26; i++) {
            if (!(drives & (1u << i))) {
                continue;
            }
            const std::wstring root = std::wstring(1, (WCHAR)(L'A' + i)) +
                                      L":\\";
            // (Not asked of network and disc drives: an offline one can take
            // long to answer.)
            WCHAR label[MAX_PATH + 1] = L"";
            const UINT type = GetDriveType(root.c_str());
            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
                GetVolumeInformation(root.c_str(), label, ARRAYSIZE(label),
                                     nullptr, nullptr, nullptr, nullptr, 0);
            }
            const std::wstring name =
                (label[0] ? std::wstring(label)
                          : std::wstring(Tr(L"Drive", L"Unidade"))) +
                L" (" + root.substr(0, 2) + L")";
            job->items.push_back({FolderKind::Drive, name, root});
        }
    } else {
        // Hidden files as Explorer shows them.
        SHELLSTATE shell{};
        SHGetSetSettings(&shell, SSF_SHOWALLOBJECTS | SSF_SHOWSUPERHIDDEN,
                         FALSE);
        std::wstring folder = job->path;
        if (!folder.empty() && folder.back() != L'\\') {
            folder += L'\\';
        }
        // (The 32-bit Windhawk process would list SysWOW64 for System32:
        // the redirection is off for this thread while it lists.)
        PVOID redirection = nullptr;
        const bool unredirected =
            Wow64DisableWow64FsRedirection(&redirection) != FALSE;
        WIN32_FIND_DATA data;
        HANDLE find = FindFirstFileEx((folder + L"*").c_str(),
                                      FindExInfoBasic, &data,
                                      FindExSearchNameMatch, nullptr,
                                      FIND_FIRST_EX_LARGE_FETCH);
        std::vector<FolderEntry> folders, files;
        bool more = false;
        if (find != INVALID_HANDLE_VALUE) {
            do {
                const DWORD attributes = data.dwFileAttributes;
                if (wcscmp(data.cFileName, L".") == 0 ||
                    wcscmp(data.cFileName, L"..") == 0 ||
                    ((attributes & FILE_ATTRIBUTE_HIDDEN) &&
                     !shell.fShowAllObjects) ||
                    ((attributes & FILE_ATTRIBUTE_HIDDEN) &&
                     (attributes & FILE_ATTRIBUTE_SYSTEM) &&
                     !shell.fShowSuperHidden)) {
                    continue;
                }
                const size_t count = folders.size() + files.size();
                if (count >= kFolderReadMax) {
                    more = true;
                    break;
                }
                // (A long read stops once the panel goes.)
                if (count % 256 == 255 && job->stop && *job->stop) {
                    break;
                }
                const bool isFolder =
                    (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                FolderEntry entry{
                    isFolder ? FolderKind::Folder : FolderKind::File,
                    data.cFileName, folder + data.cFileName, false,
                    ((ULONGLONG)data.ftLastWriteTime.dwHighDateTime << 32) |
                        data.ftLastWriteTime.dwLowDateTime};
                if (!isFolder) {
                    entry.size = ((ULONGLONG)data.nFileSizeHigh << 32) |
                                 data.nFileSizeLow;
                }
                (isFolder ? folders : files).push_back(std::move(entry));
            } while (FindNextFile(find, &data));
            FindClose(find);
        }
        if (unredirected) {
            Wow64RevertWow64FsRedirection(redirection);
        }
        // (Nothing more to do for a panel gone.)
        if (job->stop && *job->stop) {
            folders.clear();
            files.clear();
            more = false;
        }
        job->items = std::move(folders);
        job->items.insert(job->items.end(),
                          std::make_move_iterator(files.begin()),
                          std::make_move_iterator(files.end()));
        NameFolderTypes(job->items);
        SortFolderItems(job->items, job->order);
        // Too many for the column: the first of the kind shown (the column
        // reads them again for another kind).
        if (job->items.size() > kFolderListMax &&
            job->filter != FolderFilter::All) {
            std::erase_if(job->items, [&](const FolderEntry& item) {
                return !FolderFilterShows(job->filter, item);
            });
            job->filtered = job->filter;
        }
        if (job->items.size() > kFolderListMax) {
            job->items.resize(kFolderListMax);
            job->items.shrink_to_fit();
            more = true;
        }
        if (more) {
            job->items.push_back(
                {FolderKind::More,
                 Tr(L"More in Explorer", L"Mais no Explorador"),
                 job->path});
        }
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    if (!PostMessage(job->panel, WM_APP_FOLDER_LIST, 0, (LPARAM)job)) {
        delete job;
    }
    return 0;
}

// A folder chosen with Windows' folder dialog (on a thread of its own, like
// the backup's file dialog).
struct FolderPickJob {
    HWND panel;
    PCWSTR title;
    // What it's for (the panel's own use), and the folder chosen.
    int purpose;
    std::wstring path;
    // A picture instead (an icon's, see SetIconFile).
    bool picture = false;
    // Windows' icons saved into `path` (see IconSaveThreadProc): how many.
    int saved = 0;
};

// What the settings' icon jobs are for (FolderPickJob::purpose; a
// picture's is its icon): a folder chosen, one chosen with Windows' icons
// saved into it then, and Windows' icons saved.
constexpr int kIconFolderPick = 0;
constexpr int kIconFolderPickSave = 1;
constexpr int kIconFolderSaved = 2;

DWORD WINAPI FolderPickThreadProc(LPVOID parameter) {
    auto* job = (FolderPickJob*)parameter;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    winrt::com_ptr<IFileDialog> dialog;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(dialog.put())))) {
        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_FORCEFILESYSTEM |
                           (job->picture ? 0 : FOS_PICKFOLDERS));
        if (job->picture) {
            const COMDLG_FILTERSPEC filters[] = {
                {Tr(L"Pictures", L"Imagens"),
                 L"*.png;*.svg;*.ico;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff"}};
            dialog->SetFileTypes(ARRAYSIZE(filters), filters);
        }
        dialog->SetTitle(job->title);
        const HWND owner = CreateFileDialogOwner(job->panel);
        winrt::com_ptr<IShellItem> item;
        PWSTR path = nullptr;
        if (SUCCEEDED(dialog->Show(owner)) &&
            SUCCEEDED(dialog->GetResult(item.put())) &&
            SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
            job->path = path;
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
    if (!PostMessage(job->panel, WM_APP_FOLDER_PICKED, 0, (LPARAM)job)) {
        delete job;
    }
    return 0;
}

// Windows' icons saved into a folder (see SaveWindowsIcons: a few hundred
// files, on a thread of their own), told like a folder chosen.
DWORD WINAPI IconSaveThreadProc(LPVOID parameter) {
    auto* job = (FolderPickJob*)parameter;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    job->saved = SaveWindowsIcons(job->path);
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    if (!PostMessage(job->panel, WM_APP_FOLDER_PICKED, 0, (LPARAM)job)) {
        delete job;
    }
    return 0;
}

// The pins, kept as lines ("place:documents" or a folder's path); the first
// time, a few of Windows' places.
std::vector<std::wstring> LoadFolderPins() {
    std::vector<std::wstring> pins;
    const std::wstring text = GetWholeUserStringValue(L"folderPins", 16 * 1024);
    if (text.empty()) {
        return {std::wstring(kPlacePrefix) + L"desktop",
                std::wstring(kPlacePrefix) + L"documents",
                std::wstring(kPlacePrefix) + L"downloads",
                std::wstring(kPlacePrefix) + L"thispc"};
    }
    size_t at = 0;
    while (at <= text.size()) {
        size_t end = text.find(L'\n', at);
        if (end == std::wstring::npos) {
            end = text.size();
        }
        // (The first line is a mark, so no pins at all isn't the first time.)
        std::wstring line = text.substr(at, end - at);
        if (at > 0 && !line.empty()) {
            pins.push_back(std::move(line));
        }
        at = end + 1;
    }
    return pins;
}

void SaveFolderPins(const std::vector<std::wstring>& pins) {
    std::wstring text = L"pins";
    for (const auto& pin : pins) {
        text += L"\n" + pin;
    }
    SetUserStringValue(L"folderPins", text.c_str());
}

// Where a favorite file is (see kFilePinPrefix): there; gone from its place
// (deleted, moved or renamed: it's let go); or on a drive that isn't there
// now (a USB drive out, a network one unreachable: kept, not shown, until
// it's back). A network one is looked at only when `network` (a click on
// it): an offline server takes long to answer, and the panel opens at once.
enum class FavoritePlace { Here, Gone, Away };

FavoritePlace FavoriteFilePlace(const std::wstring& path, bool network) {
    if (path.size() < 3) {
        return FavoritePlace::Gone;
    }
    const bool unc = path.starts_with(L"\\\\");
    const std::wstring root = unc ? std::wstring() : path.substr(0, 3);
    const bool remote =
        unc || GetDriveType(root.c_str()) == DRIVE_REMOTE;
    if (remote && !network) {
        return FavoritePlace::Here;
    }
    // (The 32-bit Windhawk process would look in SysWOW64 for System32.)
    PVOID redirection = nullptr;
    const bool unredirected =
        Wow64DisableWow64FsRedirection(&redirection) != FALSE;
    FavoritePlace place = FavoritePlace::Here;
    if (!remote && GetFileAttributes(root.c_str()) == INVALID_FILE_ATTRIBUTES) {
        place = FavoritePlace::Away;
    } else if (const DWORD attributes = GetFileAttributes(path.c_str());
               attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        // (Not there, or its folder isn't: gone. A server not answering:
        // away. Not allowed to look: left as it is.)
        place = error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND
                    ? FavoritePlace::Gone
                : remote ? FavoritePlace::Away
                         : FavoritePlace::Here;
    } else if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
        place = FavoritePlace::Gone;
    }
    if (unredirected) {
        Wow64RevertWow64FsRedirection(redirection);
    }
    return place;
}

// A network favorite looked at on its thread (see
// FoldersPanel::FavoritePlaceNow): what it found, away until then. The
// thread holds a reference of its own, so it can end after the panel.
struct FavoriteLook {
    std::wstring path;
    std::atomic<int> place{(int)FavoritePlace::Away};
};
constexpr DWORD kFavoriteLookMs = 1500;

DWORD WINAPI FavoriteLookProc(LPVOID param) {
    auto* look = (std::shared_ptr<FavoriteLook>*)param;
    (*look)->place = (int)FavoriteFilePlace((*look)->path, true);
    delete look;
    return 0;
}

// A short notice on the main island (see Island::ShowStatusNotice), from a
// panel; defined with the island.
void ShowIslandNotice(WCHAR glyph,
                      int color,
                      const std::wstring& title,
                      const std::wstring& text);

// A pin's own icon and color (chosen with its pencil; the island's only, the
// folder in Windows stays as it is).
struct FolderLook {
    std::wstring pin;
    WCHAR glyph = 0;
    int color = -1;
};

// The icons a pin can have (Segoe Fluent Icons, the island's own font).
constexpr WCHAR kFolderLookGlyphs[] = {
    0xE8B7, 0xE838, 0xE8F4, 0xE8F1, 0xE8A5, 0xE7C3, 0xE91B, 0xE8B9,
    0xE714, 0xE722, 0xE8D6, 0xE720, 0xE7F6, 0xE896, 0xE80F, 0xE7F4,
    0xE7F8, 0xEDA2, 0xE88E, 0xE753, 0xE734, 0xEB51, 0xE8EC, 0xE7C1,
    0xE943, 0xE7FC, 0xEA86, 0xE945, 0xE77B, 0xE716, 0xE715, 0xE774,
    0xE7BE, 0xE719, 0xE821, 0xE825, 0xE787, 0xE81C, 0xE72E, 0xE713,
};
// Its colors (the last cell picks any other, see FoldersPanel).
constexpr int kFolderLookColors[] = {0xFF453A, 0xFF9F0A, 0xFFD60A, 0x30D158,
                                     0x64D2FF, 0x0A84FF, 0x5E5CE6, 0xBF5AF2,
                                     0xFF375F, 0xAC8E68};

// Kept as lines ("pin<tab>glyph<tab>color", hex; "-" for no color). Also
// read from a backup, so only icons of the font's own range and colors are
// taken.
std::vector<FolderLook> LoadFolderLooks() {
    std::vector<FolderLook> looks;
    const std::wstring text =
        GetWholeUserStringValue(L"folderLooks", 16 * 1024);
    size_t at = 0;
    while (at < text.size() && looks.size() < 1000) {
        size_t end = text.find(L'\n', at);
        if (end == std::wstring::npos) {
            end = text.size();
        }
        const std::wstring line = text.substr(at, end - at);
        at = end + 1;
        const size_t first = line.find(L'\t');
        const size_t second =
            first == std::wstring::npos ? first : line.find(L'\t', first + 1);
        if (second == std::wstring::npos || first == 0) {
            continue;
        }
        FolderLook look{line.substr(0, first)};
        const unsigned long glyph =
            wcstoul(line.c_str() + first + 1, nullptr, 16);
        if (glyph >= 0xE000 && glyph <= 0xF8FF) {
            look.glyph = (WCHAR)glyph;
        }
        if (line[second + 1] != L'-') {
            const unsigned long color =
                wcstoul(line.c_str() + second + 1, nullptr, 16);
            if (color <= 0xFFFFFF) {
                look.color = (int)color;
            }
        }
        if (look.glyph || look.color >= 0) {
            looks.push_back(std::move(look));
        }
    }
    return looks;
}

void SaveFolderLooks(const std::vector<FolderLook>& looks) {
    std::wstring text;
    for (const auto& look : looks) {
        WCHAR parts[32];
        if (look.color >= 0) {
            swprintf_s(parts, L"\t%X\t%06X", look.glyph, look.color);
        } else {
            swprintf_s(parts, L"\t%X\t-", look.glyph);
        }
        text += (text.empty() ? L"" : L"\n") + look.pin + parts;
    }
    SetUserStringValue(L"folderLooks", text.c_str());
}

////////////////////////////////////////////////////////////////////////////////
// The shelf (islandShelf): files dropped on the pill kept there
// (their paths, not copies; the newest first, at most
// kShelfLimits[islandShelfLimit], the oldest going), to be dragged out into
// another app later from its panel (ShelfPanel), like macOS' Yoink. The
// account's, stored as "shelf" (one path a line; not in backups: it's for
// the moment). The island's thread.
constexpr size_t kShelfRoom = 32 * 1024;
// (A path longer than this isn't kept: the stored text must stay within
// kShelfRoom.)
constexpr size_t kShelfPathMost = 1024;
[[clang::no_destroy]] std::vector<std::wstring> g_shelf;
bool g_shelfLoaded = false;

size_t ShelfLimit() {
    return (size_t)kShelfLimits[std::clamp(g_settings.islandShelfLimit, 0,
                                           (int)std::size(kShelfLimits) - 1)];
}

void SaveShelf() {
    std::wstring text;
    for (const auto& path : g_shelf) {
        text += (text.empty() ? L"" : L"\n") + path;
    }
    SetUserStringValue(L"shelf", text.c_str());
}

const std::vector<std::wstring>& Shelf() {
    if (!g_shelfLoaded) {
        g_shelfLoaded = true;
        const std::wstring text = GetUserStringValue(L"shelf", kShelfRoom);
        size_t at = 0;
        while (at < text.size()) {
            size_t end = text.find(L'\n', at);
            if (end == std::wstring::npos) {
                end = text.size();
            }
            if (end > at) {
                g_shelf.push_back(text.substr(at, end - at));
            }
            at = end + 1;
        }
    }
    // (A lower limit chosen since: the oldest go.)
    if (g_shelf.size() > ShelfLimit()) {
        g_shelf.resize(ShelfLimit());
        SaveShelf();
    }
    return g_shelf;
}

// Paths put on it, the newest first (one already there comes to the
// front); how many were (too long ones aren't).
size_t AddToShelf(const std::vector<std::wstring>& paths) {
    Shelf();
    size_t added = 0;
    for (const auto& path : paths) {
        if (path.empty() || path.size() > kShelfPathMost) {
            continue;
        }
        added++;
        std::erase_if(g_shelf, [&](const std::wstring& kept) {
            return _wcsicmp(kept.c_str(), path.c_str()) == 0;
        });
        g_shelf.insert(g_shelf.begin(), path);
    }
    if (g_shelf.size() > ShelfLimit()) {
        g_shelf.resize(ShelfLimit());
    }
    if (added) {
        SaveShelf();
    }
    return added;
}

void RemoveFromShelf(const std::wstring& path) {
    Shelf();
    std::erase_if(g_shelf, [&](const std::wstring& kept) {
        return _wcsicmp(kept.c_str(), path.c_str()) == 0;
    });
    SaveShelf();
}

void ClearShelf() {
    Shelf();
    g_shelf.clear();
    SaveShelf();
}

// The ones gone from their place (deleted, moved elsewhere) leave it; one
// on another computer is kept as it is (a sleeping server would hold the
// island). Whether any went.
bool PruneShelf() {
    Shelf();
    const size_t before = g_shelf.size();
    std::erase_if(g_shelf, [](const std::wstring& path) {
        return IsLocalDrivePath(path) &&
               GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES;
    });
    if (g_shelf.size() == before) {
        return false;
    }
    SaveShelf();
    return true;
}

// The files and folders a drag carries (Explorer's CF_HDROP), or none.
std::vector<std::wstring> DraggedPaths(IDataObject* data) {
    std::vector<std::wstring> paths;
    FORMATETC format{CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium{};
    if (!data || FAILED(data->GetData(&format, &medium))) {
        return paths;
    }
    if (HDROP drop = (HDROP)GlobalLock(medium.hGlobal)) {
        const UINT count = DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < count; i++) {
            const UINT length = DragQueryFile(drop, i, nullptr, 0);
            std::wstring path(length, L'\0');
            if (length &&
                DragQueryFile(drop, i, path.data(), length + 1) == length) {
                paths.push_back(std::move(path));
            }
        }
        GlobalUnlock(medium.hGlobal);
    }
    ReleaseStgMedium(&medium);
    return paths;
}

// Whether a drag carries files (asked as it comes, before it's let go).
bool DragHasFiles(IDataObject* data) {
    FORMATETC format{CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return data && data->QueryGetData(&format) == S_OK;
}

