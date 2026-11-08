// Folders and places on the island (an option): shortcuts the user pins
// (folders, and Windows' places like This PC or the Recycle Bin), in a list
// opened from the pill. A click on a folder shows what's in it in a column
// beside it (and so on, deeper), like the Finder's columns, with a button
// back; "Open in Explorer" opens the column's folder, and a click on a file
// opens it with its app; a folder's pin puts it with the pinned ones. Only
// names are shown (with a small icon by their type), read when the folder
// is shown, on a thread; nothing is kept or sent anywhere. Hidden files
// show as Explorer is set to show them (its own setting, only read).
struct FolderPlace {
    PCWSTR token;
    // A folder of the user's (navigable), or a place opened as is.
    GUID folder;
    PCWSTR open;
    WCHAR glyph;
    PCWSTR english;
    PCWSTR portuguese;
};
constexpr FolderPlace kFolderPlaces[] = {
    {L"desktop",
     {0xB4BFCC3A, 0xDB2C, 0x424C, {0xB0, 0x29, 0x7F, 0xE9, 0x9A, 0x87, 0xC6, 0x41}},
     nullptr, 0xE7F4, L"Desktop", L"Área de Trabalho"},
    {L"documents",
     {0xFDD39AD0, 0x238F, 0x46AF, {0xAD, 0xB4, 0x6C, 0x85, 0x48, 0x03, 0x69, 0xC7}},
     nullptr, 0xE8A5, L"Documents", L"Documentos"},
    {L"downloads",
     {0x374DE290, 0x123F, 0x4565, {0x91, 0x64, 0x39, 0xC4, 0x92, 0x5E, 0x46, 0x7B}},
     nullptr, 0xE896, L"Downloads", L"Downloads"},
    {L"pictures",
     {0x33E28130, 0x4E1E, 0x4676, {0x83, 0x5A, 0x98, 0x39, 0x5C, 0x3B, 0xC3, 0xBB}},
     nullptr, 0xE91B, L"Pictures", L"Imagens"},
    {L"music",
     {0x4BD8D571, 0x6D19, 0x48D3, {0xBE, 0x97, 0x42, 0x22, 0x20, 0x08, 0x0E, 0x43}},
     nullptr, 0xE8D6, L"Music", L"Músicas"},
    {L"videos",
     {0x18989B1D, 0x99B5, 0x455B, {0x84, 0x1C, 0xAB, 0x7C, 0x74, 0xE4, 0xDD, 0xFC}},
     nullptr, 0xE714, L"Videos", L"Vídeos"},
    {L"home",
     {0x5E6C858F, 0x0E22, 0x4760, {0x9A, 0xFE, 0xEA, 0x33, 0x17, 0xB6, 0x71, 0x73}},
     nullptr, 0xE80F, L"Home folder", L"Pasta pessoal"},
    {L"thispc", {}, L"shell:MyComputerFolder", 0xE7F8, L"This PC",
     L"Este Computador"},
    {L"recycle", {}, L"shell:RecycleBinFolder", 0xE74D, L"Recycle Bin",
     L"Lixeira"},
    {L"control", {}, L"shell:ControlPanelFolder", 0xE7F4, L"Control Panel",
     L"Painel de Controle"},
    {L"settings", {}, L"ms-settings:", 0xE713, L"Settings",
     L"Configurações"},
    {L"network", {}, L"ms-settings:network", 0xE701, L"Network & internet",
     L"Rede e internet"},
    {L"bluetooth", {}, L"ms-settings:bluetooth", 0xE702,
     L"Bluetooth & devices", L"Bluetooth e dispositivos"},
    {L"display", {}, L"ms-settings:display", 0xE7F4, L"Display", L"Tela"},
    {L"sound", {}, L"ms-settings:sound", 0xE767, L"Sound", L"Som"},
    {L"personalization", {}, L"ms-settings:personalization", 0xE771,
     L"Personalization", L"Personalização"},
    {L"apps", {}, L"ms-settings:appsfeatures", 0xE71D, L"Installed apps",
     L"Aplicativos instalados"},
    {L"update", {}, L"ms-settings:windowsupdate", 0xE895, L"Windows Update",
     L"Windows Update"},
    {L"devices", {}, L"devmgmt.msc", 0xE772, L"Device Manager",
     L"Gerenciador de Dispositivos"},
    {L"programs", {}, L"appwiz.cpl", 0xE74C, L"Programs and Features",
     L"Programas e Recursos"},
    {L"power", {}, L"powercfg.cpl", 0xE7E8, L"Power options",
     L"Opções de energia"},
    {L"connections", {}, L"ncpa.cpl", 0xE839, L"Network connections",
     L"Conexões de rede"},
};
constexpr WCHAR kPlacePrefix[] = L"place:";
// A favorite file among the pins ("file:<its path>"), starred on its row:
// shown with them, a click opens it, its buttons copy it or its path.
constexpr WCHAR kFilePinPrefix[] = L"file:";
constexpr size_t kMostFavoriteFiles = 40;

const FolderPlace* FindFolderPlace(std::wstring_view token) {
    for (const auto& place : kFolderPlaces) {
        if (token == place.token) {
            return &place;
        }
    }
    return nullptr;
}

// A known folder's path now (it moves with the user's choice), or "".
std::wstring KnownFolderPath(const GUID& folder) {
    PWSTR path = nullptr;
    std::wstring result;
    if (SUCCEEDED(SHGetKnownFolderPath(folder, 0, nullptr, &path))) {
        result = path;
    }
    CoTaskMemFree(path);
    return result;
}

// What the list shows: a file, a folder (navigable), a drive, one of
// Windows' places, and the rows that do something.
enum class FolderKind {
    File,
    Folder,
    Drive,
    Place,
    // Pinned from the "Windows places" column.
    PlaceToPin,
    AddFolder,
    WindowsPlaces,
    // The column's folder in Explorer.
    OpenHere,
    // A folder with more than kFolderListMax: the rest in Explorer.
    More,
};

// What a folder's column shows: everything, only its folders, or one kind of
// file (see FolderFilterOf).
enum class FolderFilter {
    All,
    Folders,
    Pictures,
    Videos,
    Documents,
    Music,
    Compressed,
    Other,
    Count,
};

// A folder's items in its column's order, as Explorer sorts them: by name,
// date modified, type or size, either way; folders first (last the other
// way), or mixed with the files.
enum class FolderSort { Name, Date, Type, Size, Count };
struct FolderOrder {
    FolderSort sort = FolderSort::Name;
    bool descending = false;
    bool mixed = false;
    bool operator==(const FolderOrder&) const = default;
};

// Kept as a number (folderOrder): the last order chosen, for the folders
// opened next.
int FolderOrderValue(const FolderOrder& order) {
    return (int)order.sort | (order.descending ? 0x10 : 0) |
           (order.mixed ? 0x20 : 0);
}
FolderOrder FolderOrderOf(int value) {
    FolderOrder order;
    if ((value & 0xF) < (int)FolderSort::Count) {
        order.sort = (FolderSort)(value & 0xF);
    }
    order.descending = (value & 0x10) != 0;
    order.mixed = (value & 0x20) != 0;
    return order;
}

struct FolderEntry {
    FolderKind kind;
    std::wstring name;
    // A path, or a place's token.
    std::wstring path;
    // Pinned (in the first column).
    bool pinned = false;
    // When it was last changed (a FILETIME), for sorting by date, and as its
    // row shows it then (see FolderItemDate).
    ULONGLONG modified = 0;
    std::wstring date;
    // A file's size (as its row shows it sorted by size), its type as
    // Explorer names it ("PNG File"), and its kind (see FolderFilterOf).
    ULONGLONG size = 0;
    std::wstring sizeText;
    std::wstring type;
    FolderFilter filter = FolderFilter::Other;
    // A pin's own icon (a glyph, 0: its usual one) and color (-1: none).
    WCHAR glyph = 0;
    int color = -1;
};

// A file's kind by its extension ("." and lower case): the usual ones by
// name, the others as Windows sees them (their perceived type).
FolderFilter FolderFilterOf(const std::wstring& type) {
    static constexpr std::pair<FolderFilter, PCWSTR> kKinds[] = {
        {FolderFilter::Pictures,
         L".jpg.jpeg.jfif.png.gif.bmp.webp.heic.heif.avif.tif.tiff.ico.svg"
         L".raw.dng.cr2.nef.arw.psd."},
        {FolderFilter::Videos,
         L".mp4.mkv.avi.mov.wmv.webm.flv.m4v.mpg.mpeg.3gp.ts.mts.m2ts."},
        {FolderFilter::Music,
         L".mp3.wav.flac.aac.ogg.oga.m4a.wma.opus.mid.midi.aiff."},
        {FolderFilter::Documents,
         L".pdf.doc.docx.xls.xlsx.ppt.pptx.odt.ods.odp.rtf.txt.md.csv.epub"
         L".xps.one."},
        {FolderFilter::Compressed,
         L".zip.rar.7z.tar.gz.tgz.bz2.xz.zst.cab.iso."},
    };
    if (type.size() < 2) {
        return FolderFilter::Other;
    }
    const std::wstring key = type + L".";
    for (const auto& [filter, list] : kKinds) {
        if (wcsstr(list, key.c_str())) {
            return filter;
        }
    }
    PERCEIVED perceived = PERCEIVED_TYPE_UNSPECIFIED;
    PERCEIVEDFLAG flags = 0;
    if (FAILED(AssocGetPerceivedType(type.c_str(), &perceived, &flags,
                                     nullptr))) {
        return FolderFilter::Other;
    }
    switch (perceived) {
        case PERCEIVED_TYPE_IMAGE:
            return FolderFilter::Pictures;
        case PERCEIVED_TYPE_VIDEO:
            return FolderFilter::Videos;
        case PERCEIVED_TYPE_AUDIO:
            return FolderFilter::Music;
        case PERCEIVED_TYPE_TEXT:
        case PERCEIVED_TYPE_DOCUMENT:
            return FolderFilter::Documents;
        case PERCEIVED_TYPE_COMPRESSED:
            return FolderFilter::Compressed;
        default:
            return FolderFilter::Other;
    }
}

// Shown in a column filtered so (rows that do something always are).
bool FolderFilterShows(FolderFilter filter, const FolderEntry& item) {
    if (filter == FolderFilter::All ||
        (item.kind != FolderKind::File && item.kind != FolderKind::Folder)) {
        return true;
    }
    return item.filter == filter;
}

// When a folder's item was last changed, as its row shows it when sorted by
// date: "Today" or the regional format's short date, and the time (as the
// clock shows it).
std::wstring FolderItemDate(ULONGLONG modified, const SYSTEMTIME& today) {
    const FILETIME utc{(DWORD)modified, (DWORD)(modified >> 32)};
    SYSTEMTIME universal, time;
    if (!modified || !FileTimeToSystemTime(&utc, &universal) ||
        !SystemTimeToTzSpecificLocalTime(nullptr, &universal, &time)) {
        return {};
    }
    const std::wstring clock = FormatClockTime(time, false);
    if (time.wYear == today.wYear && time.wMonth == today.wMonth &&
        time.wDay == today.wDay) {
        return std::wstring(Tr(L"Today", L"Hoje")) + L" " + clock;
    }
    WCHAR text[64] = L"";
    GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &time, nullptr,
                    text, ARRAYSIZE(text), nullptr);
    return std::wstring(text) + L" " + clock;
}

// A folder's items in a column's order, as Explorer sorts them: folders
// first (after the files the other way) or mixed with them, then the rest
// (drives, "More in Explorer") as they were. Same date, type or size: by
// name.
void SortFolderItems(std::vector<FolderEntry>& items,
                     const FolderOrder& order) {
    auto group = [&](const FolderEntry& item) {
        return item.kind == FolderKind::Folder ? (order.mixed ? 1 : 0)
               : item.kind == FolderKind::File ? 1
                                               : 2;
    };
    auto name = [](const FolderEntry& a, const FolderEntry& b) {
        return StrCmpLogicalW(a.name.c_str(), b.name.c_str());
    };
    std::stable_sort(
        items.begin(), items.end(),
        [&](const FolderEntry& a, const FolderEntry& b) {
            const int ga = group(a), gb = group(b);
            if (ga != gb) {
                return ga == 2 || gb == 2 || !order.descending ? ga < gb
                                                               : ga > gb;
            }
            if (ga == 2) {
                return false;
            }
            int key = 0;
            switch (order.sort) {
                case FolderSort::Date:
                    key = a.modified < b.modified   ? -1
                          : a.modified > b.modified ? 1
                                                    : 0;
                    break;
                case FolderSort::Type:
                    key = StrCmpLogicalW(a.type.c_str(), b.type.c_str());
                    break;
                case FolderSort::Size:
                    key = a.size < b.size ? -1 : a.size > b.size ? 1 : 0;
                    break;
                default:
                    key = name(a, b);
                    break;
            }
            if (key == 0) {
                return order.sort != FolderSort::Name && name(a, b) < 0;
            }
            return order.descending ? key > 0 : key < 0;
        });
}

// Folders with more are cut (the rest opens in Explorer), after they're
// sorted: up to kFolderReadMax are read, so the first ones shown are the
// first in the column's order, not in the disk's.
constexpr size_t kFolderListMax = 200;
constexpr size_t kFolderReadMax = 50000;
// Tells the list a folder was read (lParam: a FolderListJob).
constexpr UINT WM_APP_FOLDER_LIST = WM_APP + 79;
// Tells the list a folder was chosen to pin (lParam: a FolderPickJob).
constexpr UINT WM_APP_FOLDER_PICKED = WM_APP + 80;

struct FolderListJob {
    HWND panel;
    std::wstring path;
    int column;
    UINT generation;
    std::vector<FolderEntry> items;
    FolderOrder order;
    FolderFilter filter = FolderFilter::All;
    // Read again for a change in it (the column stays as it is meanwhile).
    bool quiet = false;
    // The filter the items were cut with (a folder with too many for the
    // column; All: they weren't filtered).
    FolderFilter filtered = FolderFilter::All;
    // Set as its panel goes (see FoldersPanel::OnDestroy): a long read
    // gives up.
    std::shared_ptr<std::atomic<bool>> stop;
};
// Tells the list a folder shown changed (wParam: the watched set's number,
// lParam: the folder's place in it; see FolderWatchThreadProc).
constexpr UINT WM_APP_FOLDER_CHANGED = WM_APP + 91;

// The folders a folders' panel shows, watched while it's open (see
// FoldersPanel::WatchColumns).
struct FolderWatch {
    HWND panel = nullptr;
    // Stop (manual reset), and the set changed (automatic).
    HANDLE stop = nullptr;
    HANDLE kick = nullptr;
    SRWLOCK lock = SRWLOCK_INIT;
    // Under the lock.
    std::vector<std::wstring> paths;
    UINT set = 0;
};

// A name added, removed or changed in one of the folders (a file or a
// folder, by anything) is posted to the panel; the set changed, they're
// watched again. Only waits (FindFirstChangeNotification), no disk polled;
// the list of drive letters is asked every kDrivesLookMs, and a drive come
// or gone (a USB drive) posted with lParam -1 (the favorite files on it).
constexpr DWORD kDrivesLookMs = 2000;

DWORD WINAPI FolderWatchThreadProc(LPVOID parameter) {
    auto* watch = (FolderWatch*)parameter;
    // (The 32-bit Windhawk process would watch SysWOW64 for System32.)
    PVOID redirection = nullptr;
    const bool unredirected =
        Wow64DisableWow64FsRedirection(&redirection) != FALSE;
    DWORD drives = GetLogicalDrives();
    bool stop = false;
    while (!stop) {
        AcquireSRWLockShared(&watch->lock);
        const std::vector<std::wstring> paths = watch->paths;
        const UINT set = watch->set;
        ReleaseSRWLockShared(&watch->lock);
        std::vector<HANDLE> waits{watch->stop, watch->kick};
        std::vector<int> places;
        for (size_t i = 0;
             i < paths.size() && waits.size() < MAXIMUM_WAIT_OBJECTS; i++) {
            HANDLE change = FindFirstChangeNotification(
                paths[i].c_str(), FALSE,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
            if (change != INVALID_HANDLE_VALUE) {
                waits.push_back(change);
                places.push_back((int)i);
            }
        }
        for (;;) {
            const DWORD woken = WaitForMultipleObjects(
                (DWORD)waits.size(), waits.data(), FALSE, kDrivesLookMs);
            if (woken == WAIT_TIMEOUT) {
                if (const DWORD now = GetLogicalDrives(); now != drives) {
                    drives = now;
                    PostMessage(watch->panel, WM_APP_FOLDER_CHANGED, set,
                                (LPARAM)-1);
                }
                continue;
            }
            const size_t k = woken - WAIT_OBJECT_0;
            if (woken == WAIT_FAILED || k == 0) {
                stop = true;
                break;
            }
            if (k == 1) {
                break;
            }
            if (k >= waits.size()) {
                continue;
            }
            PostMessage(watch->panel, WM_APP_FOLDER_CHANGED, set,
                        places[k - 2]);
            // (A folder gone: no longer watched.)
            if (!FindNextChangeNotification(waits[k])) {
                FindCloseChangeNotification(waits[k]);
                waits.erase(waits.begin() + k);
                places.erase(places.begin() + (k - 2));
            }
        }
        for (size_t k = 2; k < waits.size(); k++) {
            FindCloseChangeNotification(waits[k]);
        }
    }
    if (unredirected) {
        Wow64RevertWow64FsRedirection(redirection);
    }
    return 0;
}

// Files dragged over one of the mod's windows (Windows' own drag, out of the
// mod or out of another app): what they'd do there, asked of the window's
// owner, with Windows' picture of them kept over the window. Registered on
// the island's thread (OLE started there, see IslandThreadProc).
class DropTarget final : public IDropTarget {
   public:
    class Receiver {
       public:
        // The files at `screen`: what they'd do (DROPEFFECT_NONE: nothing),
        // out of what their source allows.
        virtual DWORD DragOverAt(IDataObject* data,
                                 POINT screen,
                                 DWORD keys,
                                 DWORD allowed) = 0;
        // Gone out of the window (or the drag ended).
        virtual void DragGone() = 0;
        // Let go at `screen`: what was done.
        virtual DWORD DropAt(IDataObject* data,
                             POINT screen,
                             DWORD keys,
                             DWORD allowed) = 0;

       protected:
        ~Receiver() = default;
    };

    DropTarget(HWND hwnd, Receiver* owner) : m_hwnd(hwnd), m_owner(owner) {
        CoCreateInstance(kCLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(m_helper.put()));
    }
    // Its window going: nothing told to the owner any more.
    void Forget() {
        m_owner = nullptr;
        m_data = nullptr;
        m_helper = nullptr;
    }

    IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override {
        if (riid == IID_IUnknown || riid == IID_IDropTarget) {
            *object = static_cast<IDropTarget*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&m_refs);
    }
    IFACEMETHODIMP_(ULONG) Release() override {
        const ULONG refs = InterlockedDecrement(&m_refs);
        if (!refs) {
            delete this;
        }
        return refs;
    }

    IFACEMETHODIMP DragEnter(IDataObject* data,
                             DWORD keys,
                             POINTL pt,
                             DWORD* effect) override {
        m_data.copy_from(data);
        POINT screen{pt.x, pt.y};
        *effect = m_owner ? m_owner->DragOverAt(data, screen, keys, *effect)
                          : DROPEFFECT_NONE;
        if (m_helper) {
            m_helper->DragEnter(m_hwnd, data, &screen, *effect);
        }
        return S_OK;
    }
    IFACEMETHODIMP DragOver(DWORD keys, POINTL pt, DWORD* effect) override {
        POINT screen{pt.x, pt.y};
        *effect = m_owner && m_data ? m_owner->DragOverAt(m_data.get(), screen,
                                                          keys, *effect)
                                    : DROPEFFECT_NONE;
        if (m_helper) {
            m_helper->DragOver(&screen, *effect);
        }
        return S_OK;
    }
    IFACEMETHODIMP DragLeave() override {
        if (m_owner) {
            m_owner->DragGone();
        }
        if (m_helper) {
            m_helper->DragLeave();
        }
        m_data = nullptr;
        return S_OK;
    }
    IFACEMETHODIMP Drop(IDataObject* data,
                        DWORD keys,
                        POINTL pt,
                        DWORD* effect) override {
        POINT screen{pt.x, pt.y};
        // (The picture goes first: the folder's own drop may ask something.)
        if (m_helper) {
            m_helper->Drop(data, &screen, *effect);
        }
        *effect = m_owner ? m_owner->DropAt(data, screen, keys, *effect)
                          : DROPEFFECT_NONE;
        m_data = nullptr;
        return S_OK;
    }

   private:
    HWND m_hwnd;
    Receiver* m_owner;
    winrt::com_ptr<IDropTargetHelper> m_helper;
    winrt::com_ptr<IDataObject> m_data;
    ULONG m_refs = 1;
};

// A window taking Windows' drags, until Revoke (called while the window is
// still there).
class DropRegistration {
   public:
    void Register(HWND hwnd, DropTarget::Receiver* owner) {
        if (m_target || !hwnd) {
            return;
        }
        auto* target = new DropTarget(hwnd, owner);
        if (SUCCEEDED(RegisterDragDrop(hwnd, target))) {
            m_target = target;
            m_hwnd = hwnd;
        } else {
            target->Release();
        }
    }
    void Revoke() {
        if (!m_target) {
            return;
        }
        m_target->Forget();
        RevokeDragDrop(m_hwnd);
        m_target->Release();
        m_target = nullptr;
        m_hwnd = nullptr;
    }

   private:
    DropTarget* m_target = nullptr;
    HWND m_hwnd = nullptr;
};

// The Windows folder when the path is under System32 and this is the 32-bit
// Windhawk process, which sees SysWOW64 there (empty otherwise).
std::wstring Wow64WindowsFolderOf(const std::wstring& path) {
    BOOL wow64 = FALSE;
    WCHAR windows[MAX_PATH];
    if (!IsWow64Process(GetCurrentProcess(), &wow64) || !wow64 ||
        !GetWindowsDirectory(windows, ARRAYSIZE(windows))) {
        return {};
    }
    const std::wstring system = std::wstring(windows) + L"\\System32";
    if (_wcsnicmp(path.c_str(), system.c_str(), system.size()) != 0 ||
        (path.size() != system.size() && path[system.size()] != L'\\')) {
        return {};
    }
    return windows;
}

// A folder's own drop target (copies or moves what's dropped on it as
// Explorer does: its questions for names already there, its progress, undone
// with Ctrl+Z). Under System32, through Sysnative: the copy runs on a thread
// of this process, which would put the files in SysWOW64.
winrt::com_ptr<IDropTarget> FolderDropTargetOf(const std::wstring& path) {
    std::wstring parsed = path;
    if (const std::wstring windows = Wow64WindowsFolderOf(path);
        !windows.empty()) {
        parsed = windows + L"\\Sysnative" +
                 path.substr(windows.size() + wcslen(L"\\System32"));
    }
    winrt::com_ptr<IShellItem> item;
    winrt::com_ptr<IDropTarget> target;
    if (SUCCEEDED(SHCreateItemFromParsingName(parsed.c_str(), nullptr,
                                              IID_PPV_ARGS(item.put())))) {
        item->BindToHandler(nullptr, kBHID_SFUIObject,
                            IID_PPV_ARGS(target.put()));
    }
    return target;
}

// Files' thumbnails (islandFolderThumbnails) for a panel (the folders' and
// the shelf's): Windows' own (IShellItemImageFactory, only real thumbnails:
// pictures, videos, documents; nothing for the rest, which keep their type's
// icon), made one by one on a thread of the panel's and posted to it
// (WM_APP_FILE_THUMBNAIL, lParam a ThumbnailMade the panel takes), kept as
// pictures of its drawing target (forgotten with it), the most used ones.
constexpr UINT WM_APP_FILE_THUMBNAIL = WM_APP + 108;
