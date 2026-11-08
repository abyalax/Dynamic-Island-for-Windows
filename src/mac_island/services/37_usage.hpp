struct UsageDetails {
    // Percent.
    int cpu = -1;
    int cpuMhz = -1;
    int cpuBaseMhz = -1;
    int cores = 0;
    int threads = 0;
    int ram = -1;
    ULONGLONG ramUsed = 0;
    ULONGLONG ramTotal = 0;
    ULONGLONG ramCached = 0;
    ULONGLONG ramAvailable = 0;
    ULONGLONG commitUsed = 0;
    ULONGLONG commitLimit = 0;
    std::wstring gpuName;
    int gpu = -1;
    long long vramUsed = -1;
    long long vramTotal = -1;
    long long sharedUsed = -1;
    long long sharedTotal = -1;
    // Tenths of a degree, RPM, tenths of a percent of its power limit.
    int gpuTemperature = -1;
    int gpuFan = -1;
    int gpuPower = -1;
    // The last measures (oldest first), in percent (the temperature in
    // degrees), and when the last one came.
    std::vector<float> history[4];
    double measuredAt = 0;
};
SRWLOCK g_usageLock = SRWLOCK_INIT;
UsageDetails g_usage;
// Measured every second (with the panel open), and the GPU's details too.
std::atomic<bool> g_usageFast{false};
std::atomic<bool> g_usageGpuWanted{false};

UsageDetails GetUsage() {
    AcquireSRWLockShared(&g_usageLock);
    UsageDetails usage = g_usage;
    ReleaseSRWLockShared(&g_usageLock);
    return usage;
}

struct UsageWatch {
    HWND hwnd;
    HANDLE stop;
    // Set to measure right away (the panel opened).
    HANDLE wake;
};

// The performance counters' functions, looked up once.
struct PdhApi {
    using OpenQuery_t = LONG(WINAPI*)(LPCWSTR, DWORD_PTR, HANDLE*);
    using AddCounter_t = LONG(WINAPI*)(HANDLE, LPCWSTR, DWORD_PTR, HANDLE*);
    using Collect_t = LONG(WINAPI*)(HANDLE);
    using GetValue_t = LONG(WINAPI*)(HANDLE, DWORD, LPDWORD,
                                     PDH_FMT_COUNTERVALUE*);
    using GetArray_t = LONG(WINAPI*)(HANDLE, DWORD, LPDWORD, LPDWORD, void*);
    using CloseQuery_t = LONG(WINAPI*)(HANDLE);
    HMODULE module = nullptr;
    OpenQuery_t openQuery = nullptr;
    AddCounter_t addCounter = nullptr;
    Collect_t collect = nullptr;
    GetValue_t getValue = nullptr;
    GetArray_t getArray = nullptr;
    CloseQuery_t closeQuery = nullptr;
    bool tried = false;

    bool Load() {
        if (tried) {
            return module != nullptr;
        }
        tried = true;
        module = LoadLibraryEx(L"pdh.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module) {
            return false;
        }
        openQuery = (OpenQuery_t)GetProcAddress(module, "PdhOpenQueryW");
        addCounter =
            (AddCounter_t)GetProcAddress(module, "PdhAddEnglishCounterW");
        collect = (Collect_t)GetProcAddress(module, "PdhCollectQueryData");
        getValue =
            (GetValue_t)GetProcAddress(module, "PdhGetFormattedCounterValue");
        getArray = (GetArray_t)GetProcAddress(module,
                                              "PdhGetFormattedCounterArrayW");
        closeQuery = (CloseQuery_t)GetProcAddress(module, "PdhCloseQuery");
        if (!openQuery || !addCounter || !collect || !getValue || !getArray ||
            !closeQuery) {
            FreeLibrary(module);
            module = nullptr;
        }
        return module != nullptr;
    }
    ~PdhApi() {
        if (module) {
            FreeLibrary(module);
        }
    }
};

// A query of a few counters (wildcards allowed), opened once.
struct PdhQuery {
    PdhApi* api = nullptr;
    HANDLE query = nullptr;
    std::vector<HANDLE> counters;
    bool failed = false;

    bool Open(PdhApi& pdh, std::initializer_list<PCWSTR> paths) {
        if (query || failed) {
            return query != nullptr;
        }
        failed = true;
        if (!pdh.Load() || pdh.openQuery(nullptr, 0, &query) != ERROR_SUCCESS) {
            query = nullptr;
            return false;
        }
        api = &pdh;
        for (PCWSTR path : paths) {
            HANDLE counter = nullptr;
            if (pdh.addCounter(query, path, 0, &counter) != ERROR_SUCCESS) {
                counter = nullptr;
            }
            counters.push_back(counter);
        }
        failed = false;
        return true;
    }
    bool Collect() { return query && api->collect(query) == ERROR_SUCCESS; }
    // PDH_FMT_DOUBLE.
    double Value(size_t index) {
        PDH_FMT_COUNTERVALUE value{};
        if (index >= counters.size() || !counters[index] ||
            api->getValue(counters[index], 0x00000200, nullptr, &value) !=
                ERROR_SUCCESS ||
            value.CStatus != ERROR_SUCCESS) {
            return -1;
        }
        return value.doubleValue;
    }
    // Each instance's value (PDH_FMT_LARGE or PDH_FMT_DOUBLE).
    std::vector<std::pair<std::wstring, double>> Values(size_t index) {
        std::vector<std::pair<std::wstring, double>> values;
        constexpr LONG kMoreData = (LONG)0x800007D2;
        DWORD size = 0;
        DWORD count = 0;
        if (index >= counters.size() || !counters[index] ||
            api->getArray(counters[index], 0x00000200, &size, &count,
                          nullptr) != kMoreData ||
            !size) {
            return values;
        }
        std::vector<BYTE> buffer(size);
        if (api->getArray(counters[index], 0x00000200, &size, &count,
                          buffer.data()) != ERROR_SUCCESS) {
            return values;
        }
        auto* items = (PDH_FMT_COUNTERVALUE_ITEM_W*)buffer.data();
        for (DWORD i = 0; i < count; i++) {
            if (items[i].FmtValue.CStatus == ERROR_SUCCESS && items[i].szName) {
                values.push_back(
                    {items[i].szName, items[i].FmtValue.doubleValue});
            }
        }
        return values;
    }
    ~PdhQuery() {
        if (query) {
            api->closeQuery(query);
        }
    }
};

// The display driver's figures (d3dkmthk.h).
struct KmtOpenAdapterFromLuid {
    LUID luid;
    UINT adapter;
};
struct KmtQueryAdapterInfo {
    UINT adapter;
    UINT type;
    void* data;
    UINT size;
};
struct KmtSegmentSizeInfo {
    ULONGLONG dedicatedVideoMemory;
    ULONGLONG dedicatedSystemMemory;
    ULONGLONG sharedSystemMemory;
};
struct KmtAdapterPerfData {
    UINT physicalAdapterIndex;
    ULONGLONG memoryFrequency;
    ULONGLONG maxMemoryFrequency;
    ULONGLONG maxMemoryFrequencyOC;
    ULONGLONG memoryBandwidth;
    ULONGLONG pcieBandwidth;
    ULONG fanRPM;
    ULONG power;
    ULONG temperature;
    UCHAR powerStateOverride;
};
struct KmtCloseAdapter {
    UINT adapter;
};
// KMTQAITYPE_GETSEGMENTSIZE and KMTQAITYPE_ADAPTERPERFDATA.
constexpr UINT kKmtSegmentSize = 3;
constexpr UINT kKmtAdapterPerfData = 62;

// The GPU's total memories, and its temperature, fan and power, by its LUID
// (as in its counters' names: "luid_0x00000000_0x0000AD41_phys_0").
void ReadGpuDriverFigures(const std::wstring& instance, UsageDetails* usage) {
    unsigned long high = 0, low = 0;
    if (swscanf_s(instance.c_str(), L"luid_0x%lx_0x%lx", &high, &low) != 2) {
        return;
    }
    HMODULE gdi = GetModuleHandle(L"gdi32.dll");
    using Open_t = LONG(WINAPI*)(KmtOpenAdapterFromLuid*);
    using Query_t = LONG(WINAPI*)(KmtQueryAdapterInfo*);
    using Close_t = LONG(WINAPI*)(KmtCloseAdapter*);
    auto open = gdi ? (Open_t)GetProcAddress(gdi, "D3DKMTOpenAdapterFromLuid")
                    : nullptr;
    auto query =
        gdi ? (Query_t)GetProcAddress(gdi, "D3DKMTQueryAdapterInfo") : nullptr;
    auto close =
        gdi ? (Close_t)GetProcAddress(gdi, "D3DKMTCloseAdapter") : nullptr;
    KmtOpenAdapterFromLuid opened{};
    opened.luid.LowPart = low;
    opened.luid.HighPart = (LONG)high;
    if (!open || !query || !close || open(&opened) != 0) {
        return;
    }
    KmtSegmentSizeInfo sizes{};
    KmtQueryAdapterInfo sizeQuery{opened.adapter, kKmtSegmentSize, &sizes,
                                  sizeof(sizes)};
    if (query(&sizeQuery) == 0) {
        usage->vramTotal = (long long)sizes.dedicatedVideoMemory;
        usage->sharedTotal = (long long)sizes.sharedSystemMemory;
    }
    KmtAdapterPerfData perf{};
    KmtQueryAdapterInfo perfQuery{opened.adapter, kKmtAdapterPerfData, &perf,
                                  sizeof(perf)};
    if (query(&perfQuery) == 0) {
        usage->gpuTemperature = perf.temperature ? (int)perf.temperature : -1;
        usage->gpuFan = (int)perf.fanRPM;
        usage->gpuPower = (int)perf.power;
    }
    KmtCloseAdapter closing{opened.adapter};
    close(&closing);
}

// The GPU's name, by its LUID (DXGI, loaded once).
std::wstring GpuName(const std::wstring& instance) {
    unsigned long high = 0, low = 0;
    if (swscanf_s(instance.c_str(), L"luid_0x%lx_0x%lx", &high, &low) != 2) {
        return {};
    }
    HMODULE dxgi =
        LoadLibraryEx(L"dxgi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!dxgi) {
        return {};
    }
    std::wstring name;
    using Create_t = HRESULT(WINAPI*)(REFIID, void**);
    auto create = (Create_t)GetProcAddress(dxgi, "CreateDXGIFactory1");
    winrt::com_ptr<IDXGIFactory1> factory;
    if (create && SUCCEEDED(create(__uuidof(IDXGIFactory1), factory.put_void()))) {
        winrt::com_ptr<IDXGIAdapter1> adapter;
        for (UINT i = 0;
             factory->EnumAdapters1(i, adapter.put()) == S_OK; i++) {
            DXGI_ADAPTER_DESC1 desc{};
            if (SUCCEEDED(adapter->GetDesc1(&desc)) &&
                desc.AdapterLuid.LowPart == low &&
                (unsigned long)desc.AdapterLuid.HighPart == high) {
                name = desc.Description;
            }
            adapter = nullptr;
        }
    }
    factory = nullptr;
    FreeLibrary(dxgi);
    return name;
}

// The cores and threads, once.
void CountCores(UsageDetails* usage) {
    DWORD size = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &size);
    std::vector<BYTE> buffer(size);
    if (size &&
        GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data(), &size)) {
        for (DWORD at = 0; at < size;) {
            auto* info =
                (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buffer.data() + at);
            usage->cores++;
            at += info->Size;
        }
    }
    usage->threads = (int)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
}

DWORD WINAPI UsageThreadProc(LPVOID parameter) {
    const UsageWatch watch = *(UsageWatch*)parameter;
    delete (UsageWatch*)parameter;
    auto ticks = [](const FILETIME& time) {
        return ((ULONGLONG)time.dwHighDateTime << 32) | time.dwLowDateTime;
    };
    FILETIME idle, kernel, user;
    GetSystemTimes(&idle, &kernel, &user);
    ULONGLONG lastIdle = ticks(idle);
    ULONGLONG lastBusy = ticks(kernel) + ticks(user);
    PdhApi pdh;
    PdhQuery cpuQuery;
    PdhQuery gpuQuery;
    UsageDetails usage;
    CountCores(&usage);
    std::wstring gpuInstance;
    using GetPerformanceInfo_t = BOOL(WINAPI*)(PPERFORMANCE_INFORMATION, DWORD);
    auto getPerformanceInfo = (GetPerformanceInfo_t)GetProcAddress(
        GetModuleHandle(L"kernel32.dll"), "K32GetPerformanceInfo");
    auto keep = [](std::vector<float>& history, float value) {
        history.push_back(value);
        if (history.size() > kUsageHistory) {
            history.erase(history.begin());
        }
    };
    // The first a moment after starting.
    DWORD wait = 500;
    const HANDLE handles[] = {watch.stop, watch.wake};
    for (;;) {
        const DWORD woke = WaitForMultipleObjects(2, handles, FALSE, wait);
        if (woke != WAIT_TIMEOUT && woke != WAIT_OBJECT_0 + 1) {
            break;
        }
        wait = g_usageFast ? 1000 : 2000;
        // The processor (the kernel's time includes the idle time), and its
        // frequency now: its base one times how hard it runs.
        if (GetSystemTimes(&idle, &kernel, &user)) {
            const ULONGLONG nowIdle = ticks(idle);
            const ULONGLONG nowBusy = ticks(kernel) + ticks(user);
            const ULONGLONG total = nowBusy - lastBusy;
            const ULONGLONG idleTime = nowIdle - lastIdle;
            if (total > 0 && idleTime <= total) {
                usage.cpu =
                    (int)std::lround(100.0 * (total - idleTime) / total);
            }
            lastIdle = nowIdle;
            lastBusy = nowBusy;
        }
        if (cpuQuery.Open(
                pdh, {L"\\Processor Information(_Total)\\Processor Frequency",
                      L"\\Processor Information(_Total)\\% Processor "
                      L"Performance"}) &&
            cpuQuery.Collect()) {
            const double base = cpuQuery.Value(0);
            const double performance = cpuQuery.Value(1);
            usage.cpuBaseMhz = base > 0 ? (int)std::lround(base) : -1;
            usage.cpuMhz = base > 0 && performance > 0
                               ? (int)std::lround(base * performance / 100)
                               : -1;
        }
        // The memory.
        MEMORYSTATUSEX memory{sizeof(memory)};
        if (GlobalMemoryStatusEx(&memory)) {
            usage.ram = (int)memory.dwMemoryLoad;
            usage.ramTotal = memory.ullTotalPhys;
            usage.ramAvailable = memory.ullAvailPhys;
            usage.ramUsed = memory.ullTotalPhys - memory.ullAvailPhys;
        }
        PERFORMANCE_INFORMATION performance{sizeof(performance)};
        if (getPerformanceInfo &&
            getPerformanceInfo(&performance, sizeof(performance))) {
            usage.ramCached =
                (ULONGLONG)performance.SystemCache * performance.PageSize;
            usage.commitUsed =
                (ULONGLONG)performance.CommitTotal * performance.PageSize;
            usage.commitLimit =
                (ULONGLONG)performance.CommitLimit * performance.PageSize;
        }
        // The GPU with the most video memory in use, and how busy its 3D
        // engines are.
        if (g_usageGpuWanted &&
            gpuQuery.Open(pdh, {L"\\GPU Adapter Memory(*)\\Dedicated Usage",
                                L"\\GPU Adapter Memory(*)\\Shared Usage",
                                L"\\GPU Engine(*engtype_3D)\\Utilization "
                                L"Percentage"}) &&
            gpuQuery.Collect()) {
            double most = -1;
            std::wstring instance;
            for (const auto& [name, value] : gpuQuery.Values(0)) {
                if (value > most) {
                    most = value;
                    instance = name;
                }
            }
            usage.vramUsed = most >= 0 ? (long long)most : -1;
            usage.sharedUsed = -1;
            for (const auto& [name, value] : gpuQuery.Values(1)) {
                if (name == instance) {
                    usage.sharedUsed = (long long)value;
                }
            }
            // The engines are named "pid_..._luid_0x..._phys_0_eng_..".
            const size_t luid = instance.find(L"_phys");
            const std::wstring prefix =
                luid == std::wstring::npos ? instance : instance.substr(0, luid);
            double busy = 0;
            bool any = false;
            for (const auto& [name, value] : gpuQuery.Values(2)) {
                if (!prefix.empty() && name.find(prefix) != std::wstring::npos) {
                    busy += value;
                    any = true;
                }
            }
            usage.gpu = any ? (int)std::lround(std::min(busy, 100.0)) : -1;
            if (instance != gpuInstance) {
                gpuInstance = instance;
                usage.gpuName = GpuName(instance);
            }
            ReadGpuDriverFigures(instance, &usage);
        } else if (!g_usageGpuWanted) {
            usage.vramUsed = usage.sharedUsed = -1;
            usage.gpu = -1;
        }
        keep(usage.history[0], (float)std::max(usage.cpu, 0));
        keep(usage.history[1], (float)std::max(usage.ram, 0));
        if (usage.gpu >= 0) {
            keep(usage.history[2], (float)usage.gpu);
        }
        if (usage.gpuTemperature >= 0) {
            keep(usage.history[3], usage.gpuTemperature / 10.0f);
        }
        usage.measuredAt = NowSeconds();
        AcquireSRWLockExclusive(&g_usageLock);
        g_usage = usage;
        ReleaseSRWLockExclusive(&g_usageLock);
        PostMessage(watch.hwnd, WM_APP_USAGE, 0, 0);
    }
    return 0;
}

// What the processor, the memory or the GPU is doing, out of its item on the
// pill: graphs of the last measures (moving along as new ones come), and the
// figures in tiles.
constexpr float kUsageWidth = 330;

class UsagePanel : public Panel {
   public:
    enum class Kind { Cpu, Ram, Gpu };
    void SetKind(Kind kind) {
        m_kind = kind;
        if (IsOpen()) {
            Layout();
            Render();
        }
    }
    Kind GetKind() const { return m_kind; }
    // New measures came.
    void Refresh() {
        if (IsOpen()) {
            m_usage = GetUsage();
            if (m_usage.measuredAt != m_shownAt) {
                m_shownAt = m_usage.measuredAt;
                m_shift = 1;
            }
            Layout();
            StartIslandAnimation();
        }
    }

   protected:
    void Prepare(float scale) override {
        if (scale != m_preparedScale) {
            m_preparedScale = scale;
            m_titleFormat = MakeFormat(L"Segoe UI Variable Text",
                                       DWRITE_FONT_WEIGHT_SEMI_BOLD, 14 * scale,
                                       DWRITE_TEXT_ALIGNMENT_LEADING);
            m_labelFormat =
                MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                           11 * scale, DWRITE_TEXT_ALIGNMENT_LEADING);
            m_valueFormat = MakeFormat(L"Segoe UI Variable Display",
                                       DWRITE_FONT_WEIGHT_SEMI_BOLD, 17 * scale,
                                       DWRITE_TEXT_ALIGNMENT_LEADING);
            m_tabFormat = MakeFormat(L"Segoe UI Variable Text",
                                     DWRITE_FONT_WEIGHT_SEMI_BOLD, 12 * scale);
            if (!m_tabular &&
                SUCCEEDED(m_dwrite->CreateTypography(m_tabular.put()))) {
                m_tabular->AddFontFeature(
                    {DWRITE_FONT_FEATURE_TAG_TABULAR_FIGURES, 1});
            }
        }
        m_usage = GetUsage();
        Layout();
    }

    void OnOpen() override {
        m_usage = GetUsage();
        m_shownAt = m_usage.measuredAt;
        m_shift = 0;
        m_hover = -1;
        Layout();
    }

    // Measured only while it's open, when it's not on the pill (see
    // Island::UpdateUsageWatch).
    void OnClose() override { TellOwner(); }

    // A tab for each of them, to see another one in the same panel.
    int HitTest(POINT pt) const {
        if (!PointInRect(pt, m_tabsRect)) {
            return -1;
        }
        const float segment = (m_tabsRect.right - m_tabsRect.left) / kTabs;
        return std::clamp((int)((pt.x - m_tabsRect.left) / segment), 0,
                          kTabs - 1);
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
        if (m_hover < 0) {
            return false;
        }
        m_hover = -1;
        return true;
    }

    bool OnMouseUp(POINT pt) override {
        const int tab = HitTest(pt);
        if (tab >= 0 && tab != (int)m_kind) {
            SetKind((Kind)tab);
            TellOwner();
        }
        return false;
    }

    bool AnimateContents(double dt) override {
        // The graphs move along by one measure.
        if (m_shift > 0) {
            m_shift = std::max(0.0, m_shift - dt / 0.45);
            return true;
        }
        return false;
    }

    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;

   private:
    struct Tile {
        PCWSTR label;
        std::wstring value;
    };
    std::vector<Tile> Tiles() const;
    static constexpr int kTabs = 3;

    void TellOwner() {
        if (HWND island = IslandWindow(m_owner)) {
            PostMessage(island, WM_APP_USAGE_PANEL, 0, 0);
        }
    }

    void Layout() {
        const float s = m_scale;
        const float width = std::round(kUsageWidth * s);
        const float padding = std::round(14 * s);
        float y = padding;
        m_tabsRect = {padding, y, width - padding, y + std::round(28 * s)};
        y += std::round(38 * s);
        m_title = {padding, y, width - padding, y + std::round(22 * s)};
        y += std::round(30 * s);
        m_graphs.clear();
        const float graph = std::round(62 * s);
        const int graphs = m_kind == Kind::Gpu && !m_usage.history[3].empty()
                               ? 2
                               : 1;
        for (int i = 0; i < graphs; i++) {
            y += std::round(16 * s);
            m_graphs.push_back({padding, y, width - padding, y + graph});
            y += graph + std::round(8 * s);
        }
        const size_t tiles = Tiles().size();
        const float tile = std::round(56 * s);
        const float gap = std::round(8 * s);
        m_tiles.clear();
        const float half = (width - padding * 2 - gap) / 2;
        for (size_t i = 0; i < tiles; i++) {
            const float left = padding + (half + gap) * (i % 2);
            m_tiles.push_back({std::round(left), y, std::round(left + half),
                               y + tile});
            if (i % 2 == 1 || i + 1 == tiles) {
                y += tile + gap;
            }
        }
        y += padding - gap;
        m_size = {(LONG)width, (LONG)std::ceil(y)};
    }

    void DrawTabs(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawGraph(ID2D1RenderTarget* target,
                   ID2D1SolidColorBrush* brush,
                   const D2D1_RECT_F& rect,
                   const std::vector<float>& values,
                   float top,
                   D2D1_COLOR_F color,
                   PCWSTR label);

    Kind m_kind = Kind::Cpu;
    int m_hover = -1;
    D2D1_RECT_F m_tabsRect{};
    winrt::com_ptr<IDWriteTextFormat> m_tabFormat;
    UsageDetails m_usage;
    double m_shownAt = 0;
    // How far the graphs still have to move along (1 to 0).
    double m_shift = 0;
    float m_preparedScale = 0;
    winrt::com_ptr<IDWriteTextFormat> m_titleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_labelFormat;
    winrt::com_ptr<IDWriteTextFormat> m_valueFormat;
    winrt::com_ptr<IDWriteTypography> m_tabular;
    D2D1_RECT_F m_title{};
    std::vector<D2D1_RECT_F> m_graphs;
    std::vector<D2D1_RECT_F> m_tiles;
};

// "6.3 GB", "414 MB".
std::wstring BytesText(long long bytes) {
    if (bytes < 0) {
        return L"–";
    }
    WCHAR text[32];
    if (bytes >= 1000LL * 1024 * 1024) {
        swprintf_s(text, L"%.1f GB", bytes / (1024.0 * 1024 * 1024));
    } else {
        swprintf_s(text, L"%lld MB", bytes / (1024 * 1024));
    }
    return text;
}

std::vector<UsagePanel::Tile> UsagePanel::Tiles() const {
    const UsageDetails& u = m_usage;
    auto percent = [](int value) {
        return value < 0 ? std::wstring(L"–")
                         : std::to_wstring(value) + L"%";
    };
    auto mhz = [](int value) {
        return value < 0 ? std::wstring(L"–")
                         : std::to_wstring(value) + L" MHz";
    };
    switch (m_kind) {
        case Kind::Cpu:
            return {
                {Tr(L"Usage", L"Uso"), percent(u.cpu)},
                {Tr(L"Frequency", L"Frequência"), mhz(u.cpuMhz)},
                {Tr(L"Cores / threads", L"Núcleos / threads"),
                 std::to_wstring(u.cores) + L" / " + std::to_wstring(u.threads)},
                {Tr(L"Base frequency", L"Frequência base"), mhz(u.cpuBaseMhz)},
            };
        case Kind::Ram:
            return {
                {Tr(L"In use", L"Em uso"), BytesText((long long)u.ramUsed)},
                {Tr(L"Total", L"Total"), BytesText((long long)u.ramTotal)},
                {Tr(L"Cached", L"Em cache"), BytesText((long long)u.ramCached)},
                {Tr(L"Available", L"Disponível"),
                 BytesText((long long)u.ramAvailable)},
                {Tr(L"Committed", L"Confirmada"),
                 BytesText((long long)u.commitUsed) + L" / " +
                     BytesText((long long)u.commitLimit)},
                {Tr(L"Utilization", L"Utilização"), percent(u.ram)},
            };
        case Kind::Gpu: {
            std::vector<Tile> tiles{
                {Tr(L"Usage", L"Uso"), percent(u.gpu)},
                {Tr(L"Memory", L"Memória"),
                 BytesText(u.vramUsed) +
                     (u.vramTotal > 0 ? L" / " + BytesText(u.vramTotal)
                                      : std::wstring())},
            };
            if (u.gpuTemperature >= 0) {
                WCHAR text[16];
                swprintf_s(text, L"%.0f °C", u.gpuTemperature / 10.0);
                tiles.push_back({Tr(L"Temperature", L"Temperatura"), text});
            }
            if (u.gpuPower > 0) {
                WCHAR text[16];
                swprintf_s(text, L"%.0f%%", u.gpuPower / 10.0);
                tiles.push_back({Tr(L"Power draw", L"Energia"), text});
            }
            if (u.gpuFan >= 0 && u.gpuTemperature >= 0) {
                tiles.push_back({Tr(L"Fan", L"Ventoinha"),
                                 std::to_wstring(u.gpuFan) + L" RPM"});
            }
            tiles.push_back(
                {Tr(L"Shared memory", L"Memória compartilhada"),
                 BytesText(u.sharedUsed) +
                     (u.sharedTotal > 0 ? L" / " + BytesText(u.sharedTotal)
                                        : std::wstring())});
            return tiles;
        }
    }
    return {};
}

void UsagePanel::DrawTabs(ID2D1RenderTarget* target,
                          ID2D1SolidColorBrush* brush) {
    const D2D1_RECT_F& track = m_tabsRect;
    const float radius = (track.bottom - track.top) / 2;
    brush->SetColor(Fg(0.08f));
    GlassFill(target, {track, radius, radius}, brush);
    const float segment = (track.right - track.left) / kTabs;
    const float inset = std::round(2 * m_scale);
    const D2D1_RECT_F lit{track.left + segment * (int)m_kind + inset,
                          track.top + inset,
                          track.left + segment * ((int)m_kind + 1) - inset,
                          track.bottom - inset};
    const float litRadius = (lit.bottom - lit.top) / 2;
    brush->SetColor(Fg(0.2f));
    GlassFill(target, {lit, litRadius, litRadius}, brush);
    static constexpr PCWSTR kNames[kTabs] = {L"CPU", L"RAM", L"VRAM"};
    for (int i = 0; i < kTabs; i++) {
        const float hover = Hover(HoverKey(6650, i), m_hover == i);
        brush->SetColor(Fg(i == (int)m_kind ? 0.95f : 0.6f + 0.2f * hover));
        target->DrawText(kNames[i], (UINT32)wcslen(kNames[i]),
                         m_tabFormat.get(),
                         {track.left + segment * i, track.top,
                          track.left + segment * (i + 1), track.bottom},
                         brush);
    }
}

void UsagePanel::DrawGraph(ID2D1RenderTarget* target,
                           ID2D1SolidColorBrush* brush,
                           const D2D1_RECT_F& rect,
                           const std::vector<float>& values,
                           float top,
                           D2D1_COLOR_F color,
                           PCWSTR label) {
    const float s = m_scale;
    brush->SetColor(Fg(0.5f));
    target->DrawText(label, (UINT32)wcslen(label), m_labelFormat.get(),
                     {rect.left, rect.top - std::round(17 * s), rect.right,
                      rect.top},
                     brush);
    // A faint grid.
    brush->SetColor(Fg(0.05f));
    target->FillRectangle(rect, brush);
    brush->SetColor(Fg(0.06f));
    for (int i = 1; i < 4; i++) {
        const float y = std::round(rect.top + (rect.bottom - rect.top) * i / 4);
        target->FillRectangle({rect.left, y, rect.right, y + 1}, brush);
    }
    if (values.size() < 2) {
        return;
    }
    winrt::com_ptr<ID2D1Factory> factory;
    target->GetFactory(factory.put());
    winrt::com_ptr<ID2D1PathGeometry> line;
    winrt::com_ptr<ID2D1PathGeometry> area;
    winrt::com_ptr<ID2D1GeometrySink> lineSink;
    winrt::com_ptr<ID2D1GeometrySink> areaSink;
    if (FAILED(factory->CreatePathGeometry(line.put())) ||
        FAILED(factory->CreatePathGeometry(area.put())) ||
        FAILED(line->Open(lineSink.put())) ||
        FAILED(area->Open(areaSink.put()))) {
        return;
    }
    // The newest at the right edge, the others a step apart each, sliding
    // left as a new one comes.
    const float step = (rect.right - rect.left) / (kUsageHistory - 1);
    const float shift = (float)EaseInOutCubic(1 - m_shift) - 1;
    auto point = [&](size_t i) {
        const float x = rect.right -
                        (values.size() - 1 - i + shift) * step;
        const float y =
            rect.bottom - (rect.bottom - rect.top) *
                              std::clamp(values[i] / top, 0.0f, 1.0f);
        return D2D1::Point2F(x, y);
    };
    lineSink->BeginFigure(point(0), D2D1_FIGURE_BEGIN_HOLLOW);
    areaSink->BeginFigure({point(0).x, rect.bottom}, D2D1_FIGURE_BEGIN_FILLED);
    areaSink->AddLine(point(0));
    for (size_t i = 1; i < values.size(); i++) {
        lineSink->AddLine(point(i));
        areaSink->AddLine(point(i));
    }
    areaSink->AddLine({point(values.size() - 1).x, rect.bottom});
    lineSink->EndFigure(D2D1_FIGURE_END_OPEN);
    areaSink->EndFigure(D2D1_FIGURE_END_CLOSED);
    lineSink->Close();
    areaSink->Close();
    target->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
    brush->SetColor(D2D1::ColorF(color.r, color.g, color.b, 0.18f));
    target->FillGeometry(area.get(), brush);
    brush->SetColor(color);
    target->DrawGeometry(line.get(), brush, std::max(1.0f, 1.5f * s));
    target->PopAxisAlignedClip();
}

void UsagePanel::Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) {
    if (!m_titleFormat || !m_labelFormat || !m_valueFormat || !m_tabFormat) {
        return;
    }
    const float s = m_scale;
    DrawTabs(target, brush);
    std::wstring title = m_kind == Kind::Cpu ? Tr(L"Processor", L"Processador")
                         : m_kind == Kind::Ram
                             ? Tr(L"Memory", L"Memória")
                             : (m_usage.gpuName.empty() ? std::wstring(L"GPU")
                                                        : m_usage.gpuName);
    brush->SetColor(Fg(0.95f));
    target->DrawText(title.c_str(), (UINT32)title.size(), m_titleFormat.get(),
                     m_title, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    const D2D1_COLOR_F blue = D2D1::ColorF(0.35f, 0.62f, 1.0f, 1);
    if (!m_graphs.empty()) {
        DrawGraph(target, brush, m_graphs[0],
                  m_usage.history[m_kind == Kind::Cpu   ? 0
                                  : m_kind == Kind::Ram ? 1
                                                        : 2],
                  100, blue, Tr(L"Utilization", L"Utilização"));
    }
    if (m_graphs.size() > 1) {
        DrawGraph(target, brush, m_graphs[1], m_usage.history[3], 100,
                  D2D1::ColorF(1.0f, 0.45f, 0.2f, 1),
                  Tr(L"Temperature", L"Temperatura"));
    }
    const auto tiles = Tiles();
    for (size_t i = 0; i < tiles.size() && i < m_tiles.size(); i++) {
        const D2D1_RECT_F& rect = m_tiles[i];
        const float radius = std::round(10 * s);
        brush->SetColor(Fg(0.07f));
        GlassFill(target, {rect, radius, radius}, brush);
        const float inner = std::round(12 * s);
        // Left in the left column, right in the right one.
        const auto alignment = i % 2 ? DWRITE_TEXT_ALIGNMENT_TRAILING
                                     : DWRITE_TEXT_ALIGNMENT_LEADING;
        m_labelFormat->SetTextAlignment(alignment);
        m_valueFormat->SetTextAlignment(alignment);
        // Smaller when too long for the tile, never cut.
        const float room = rect.right - rect.left - inner * 2;
        auto fitted = [&](const std::wstring& text, IDWriteTextFormat* format,
                          float height, bool figures) {
            winrt::com_ptr<IDWriteTextLayout> layout;
            if (FAILED(m_dwrite->CreateTextLayout(
                    text.c_str(), (UINT32)text.size(), format, room, height,
                    layout.put()))) {
                return layout;
            }
            const DWRITE_TEXT_RANGE all{0, (UINT32)text.size()};
            if (figures && m_tabular) {
                layout->SetTypography(m_tabular.get(), all);
            }
            layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            DWRITE_TEXT_METRICS metrics{};
            layout->GetMetrics(&metrics);
            if (metrics.widthIncludingTrailingWhitespace > room) {
                layout->SetFontSize(
                    format->GetFontSize() *
                        std::max(0.55f,
                                 room / metrics.widthIncludingTrailingWhitespace),
                    all);
            }
            return layout;
        };
        const std::wstring label = tiles[i].label;
        if (auto layout = fitted(label, m_labelFormat.get(),
                                 std::round(16 * s), false)) {
            brush->SetColor(Fg(0.55f));
            target->DrawTextLayout({rect.left + inner,
                                    rect.top + std::round(8 * s)},
                                   layout.get(), brush);
        }
        if (auto layout = fitted(tiles[i].value, m_valueFormat.get(),
                                 std::round(24 * s), true)) {
            brush->SetColor(Fg(0.95f));
            target->DrawTextLayout({rect.left + inner,
                                    rect.top + std::round(25 * s)},
                                   layout.get(), brush);
        }
        m_labelFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_valueFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Notes, tasks and keyboard shortcuts: the user's own, kept by the mod (a
// value each, a line per entry, its fields tab-separated with tabs, new
// lines and backslashes escaped), in panels out of their items on the pill
// or their buttons in the control center.

std::wstring EscapeField(const std::wstring& text) {
    std::wstring escaped;
    for (WCHAR c : text) {
        if (c == L'\\') {
            escaped += L"\\\\";
        } else if (c == L'\n') {
            escaped += L"\\n";
        } else if (c == L'\t') {
            escaped += L"\\t";
        } else if (c != L'\r') {
            escaped += c;
        }
    }
    return escaped;
}

std::wstring UnescapeField(const std::wstring& text) {
    std::wstring plain;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == L'\\' && i + 1 < text.size()) {
            const WCHAR next = text[++i];
            plain += next == L'n' ? L'\n' : next == L't' ? L'\t' : next;
        } else {
            plain += text[i];
        }
    }
    return plain;
}

// The entries of a value, each with `fields` fields.
std::vector<std::vector<std::wstring>> LoadRecords(
    PCWSTR name,
    size_t fields,
    size_t room = 256 * 1024) {
    std::vector<std::vector<std::wstring>> records;
    const std::wstring value = GetWholeStringValue(name, room);
    for (PCWSTR line = value.data(); *line;) {
        PCWSTR end = wcschr(line, L'\n');
        const std::wstring text =
            end ? std::wstring(line, end) : std::wstring(line);
        line = end ? end + 1 : line + wcslen(line);
        std::vector<std::wstring> record;
        size_t at = 0;
        while (record.size() + 1 < fields) {
            const size_t tab = text.find(L'\t', at);
            if (tab == std::wstring::npos) {
                break;
            }
            record.push_back(UnescapeField(text.substr(at, tab - at)));
            at = tab + 1;
        }
        if (record.size() + 1 == fields) {
            record.push_back(UnescapeField(text.substr(at)));
            records.push_back(std::move(record));
        }
    }
    return records;
}

void SaveRecords(PCWSTR name,
                 const std::vector<std::vector<std::wstring>>& records) {
    std::wstring value;
    for (const auto& record : records) {
        for (size_t i = 0; i < record.size(); i++) {
            value += (i ? L"\t" : L"") + EscapeField(record[i]);
        }
        value += L"\n";
    }
    SetStoredString(name, value.c_str());
}

// Now, as a number that also serves as an ID.
long long NowStamp() {
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    return ((long long)now.dwHighDateTime << 32) | now.dwLowDateTime;
}

// "06/10/2026 17:38", in the user's format.
std::wstring StampText(long long stamp) {
    FILETIME utc{(DWORD)stamp, (DWORD)(stamp >> 32)};
    FILETIME local;
    SYSTEMTIME time;
    if (!FileTimeToLocalFileTime(&utc, &local) ||
        !FileTimeToSystemTime(&local, &time)) {
        return {};
    }
    WCHAR date[64] = L"";
    GetDateFormatEx(DateLocale(), DATE_SHORTDATE, &time, nullptr, date,
                    ARRAYSIZE(date), nullptr);
    return std::wstring(date) + L"  " + FormatClockTime(time, false);
}

constexpr WCHAR kGlyphNote = 0xE70B;
constexpr WCHAR kGlyphTasks = 0xE9D5;
