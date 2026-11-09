#pragma once
// The media thread's own copies of its events (stop, changed, command), so
// StopMediaWatch can give up waiting for it (a media app's call that hangs)
// and close the island's; the thread closes these when it ends.
struct MediaThreadEvents {
    HANDLE events[3];
};

DWORD WINAPI MediaThreadProc(LPVOID parameter) {
    using namespace winrt::Windows::Media::Control;
    const std::unique_ptr<MediaThreadEvents> own(
        (MediaThreadEvents*)parameter);
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    {
        GlobalSystemMediaTransportControlsSessionManager manager{nullptr};
        GlobalSystemMediaTransportControlsSessionManager::
            CurrentSessionChanged_revoker sessionRevoker;
        GlobalSystemMediaTransportControlsSessionManager::
            SessionsChanged_revoker sessionsRevoker;
        auto changed = [](auto&&...) { TellMediaChanged(); };
        try {
            manager =
                GlobalSystemMediaTransportControlsSessionManager::RequestAsync()
                    .get();
            sessionRevoker =
                manager.CurrentSessionChanged(winrt::auto_revoke, changed);
            sessionsRevoker =
                manager.SessionsChanged(winrt::auto_revoke, changed);
        } catch (winrt::hresult_error const& e) {
            IslandLog(L"Media controls: %08X", (unsigned)e.code());
            manager = nullptr;
        }
        if (manager) {
            GlobalSystemMediaTransportControlsSession session{nullptr};
            GlobalSystemMediaTransportControlsSession::
                MediaPropertiesChanged_revoker propertiesRevoker;
            GlobalSystemMediaTransportControlsSession::
                PlaybackInfoChanged_revoker playbackRevoker;
            GlobalSystemMediaTransportControlsSession::
                TimelinePropertiesChanged_revoker timelineRevoker;
            std::wstring lastTitle;
            double titleChangedAt = 0;
            UINT thumbnailSerial = 0;
            // Every session, watched for its title and its state (for the
            // list of apps to choose from). Its title is kept, and read again
            // only when that session says it changed (one call to its app).
            struct WatchedSession {
                GlobalSystemMediaTransportControlsSession session{nullptr};
                GlobalSystemMediaTransportControlsSession::
                    MediaPropertiesChanged_revoker properties;
                GlobalSystemMediaTransportControlsSession::
                    PlaybackInfoChanged_revoker playback;
                std::wstring title;
                std::shared_ptr<std::atomic<bool>> titleStale;
            };
            std::vector<WatchedSession> watched;
            const HANDLE* events = own->events;
            SetEvent(events[1]);

            for (;;) {
                const DWORD wait =
                    WaitForMultipleObjects(3, events, FALSE, INFINITE);
                if (wait == WAIT_OBJECT_0) {
                    break;
                }
                try {
                    if (wait == WAIT_OBJECT_0 + 2) {
                        // The island's buttons.
                        AcquireSRWLockExclusive(&g_mediaLock);
                        auto commands = std::move(g_mediaCommands);
                        g_mediaCommands.clear();
                        ReleaseSRWLockExclusive(&g_mediaLock);
                        for (const auto& [command, position] : commands) {
                            if (!session) {
                                break;
                            }
                            switch (command) {
                                case MediaCommand::PlayPause:
                                    session.TryTogglePlayPauseAsync().get();
                                    break;
                                case MediaCommand::Previous:
                                    session.TrySkipPreviousAsync().get();
                                    break;
                                case MediaCommand::Next:
                                    session.TrySkipNextAsync().get();
                                    break;
                                case MediaCommand::Seek:
                                    session
                                        .TryChangePlaybackPositionAsync(
                                            (int64_t)(position * 10000000))
                                        .get();
                                    break;
                            }
                        }
                        continue;
                    }

                    // Every app with media, and the one used: the one
                    // chosen while it's there, otherwise Windows' current.
                    std::vector<GlobalSystemMediaTransportControlsSession> all;
                    for (const auto& each : manager.GetSessions()) {
                        all.push_back(each);
                    }
                    if (all.size() != watched.size() ||
                        !std::equal(all.begin(), all.end(), watched.begin(),
                                    [](const auto& a, const WatchedSession& b) {
                                        return a == b.session;
                                    })) {
                        // The ones still there keep their title.
                        std::vector<WatchedSession> before =
                            std::move(watched);
                        watched.clear();
                        for (const auto& each : all) {
                            auto old = std::find_if(
                                before.begin(), before.end(),
                                [&](const WatchedSession& entry) {
                                    return entry.session == each;
                                });
                            if (old != before.end()) {
                                watched.push_back(std::move(*old));
                                continue;
                            }
                            WatchedSession entry;
                            entry.session = each;
                            entry.titleStale =
                                std::make_shared<std::atomic<bool>>(true);
                            entry.properties = each.MediaPropertiesChanged(
                                winrt::auto_revoke,
                                [stale = entry.titleStale](auto&&...) {
                                    *stale = true;
                                    TellMediaChanged();
                                });
                            entry.playback = each.PlaybackInfoChanged(
                                winrt::auto_revoke, changed);
                            watched.push_back(std::move(entry));
                        }
                    }
                    std::vector<MediaSource> sources;
                    for (auto& entry : watched) {
                        const auto& each = entry.session;
                        MediaSource source;
                        source.appId = each.SourceAppUserModelId();
                        if (entry.titleStale->exchange(false)) {
                            try {
                                if (auto info =
                                        each.TryGetMediaPropertiesAsync()
                                            .get()) {
                                    entry.title = info.Title();
                                }
                            } catch (winrt::hresult_error const&) {
                            }
                        }
                        source.title = entry.title;
                        if (auto playback = each.GetPlaybackInfo()) {
                            source.playing =
                                playback.PlaybackStatus() ==
                                GlobalSystemMediaTransportControlsSessionPlaybackStatus::
                                    Playing;
                        }
                        sources.push_back(std::move(source));
                    }
                    AcquireSRWLockShared(&g_mediaLock);
                    const std::wstring chosenApp = g_mediaChosenApp;
                    const int chosenOccurrence = g_mediaChosenOccurrence;
                    ReleaseSRWLockShared(&g_mediaLock);
                    GlobalSystemMediaTransportControlsSession current{nullptr};
                    bool chosen = false;
                    if (!chosenApp.empty()) {
                        int occurrence = 0;
                        GlobalSystemMediaTransportControlsSession first{nullptr};
                        for (size_t i = 0; i < all.size(); i++) {
                            if (sources[i].appId != chosenApp) {
                                continue;
                            }
                            if (!first) {
                                first = all[i];
                            }
                            if (occurrence++ == chosenOccurrence) {
                                current = all[i];
                            }
                        }
                        if (!current) {
                            current = first;
                        }
                        chosen = current != nullptr;
                        if (!chosen) {
                            // Gone: Windows' choice again.
                            AcquireSRWLockExclusive(&g_mediaLock);
                            if (g_mediaChosenApp == chosenApp) {
                                g_mediaChosenApp.clear();
                                g_mediaChosenOccurrence = 0;
                            }
                            ReleaseSRWLockExclusive(&g_mediaLock);
                        }
                    }
                    if (!current) {
                        current = manager.GetCurrentSession();
                    }
                    if (current != session) {
                        session = current;
                        propertiesRevoker = {};
                        playbackRevoker = {};
                        timelineRevoker = {};
                        if (session) {
                            propertiesRevoker = session.MediaPropertiesChanged(
                                winrt::auto_revoke, changed);
                            playbackRevoker = session.PlaybackInfoChanged(
                                winrt::auto_revoke, changed);
                            timelineRevoker = session.TimelinePropertiesChanged(
                                winrt::auto_revoke, changed);
                        }
                    }
                    MediaState state;
                    state.sources = std::move(sources);
                    state.chosen = chosen;
                    for (size_t i = 0; i < all.size(); i++) {
                        if (all[i] == session) {
                            state.sourceIndex = (int)i;
                        }
                    }
                    if (session) {
                        state.present = true;
                        state.appId = session.SourceAppUserModelId();
                        // Any of these may be missing (a browser changing
                        // pages, or going full screen).
                        auto properties =
                            session.TryGetMediaPropertiesAsync().get();
                        if (properties) {
                            state.title = properties.Title();
                            state.artist = properties.Artist();
                            // (Browsers say "music" for anything: only a
                            // video said so counts.)
                            if (auto type = properties.PlaybackType()) {
                                using winrt::Windows::Media::MediaPlaybackType;
                                const MediaPlaybackType kind = type.Value();
                                state.video =
                                    kind == MediaPlaybackType::Video ||
                                    kind == MediaPlaybackType::Image;
                            }
                        }
                        if (auto playback = session.GetPlaybackInfo()) {
                            state.playing =
                                playback.PlaybackStatus() ==
                                GlobalSystemMediaTransportControlsSessionPlaybackStatus::
                                    Playing;
                            if (auto controls = playback.Controls()) {
                                state.canPrevious = controls.IsPreviousEnabled();
                                state.canNext = controls.IsNextEnabled();
                                state.canPlayPause =
                                    controls.IsPlayPauseToggleEnabled() ||
                                    controls.IsPauseEnabled() ||
                                    controls.IsPlayEnabled();
                                state.canSeek =
                                    controls.IsPlaybackPositionEnabled();
                            }
                        }
                        using seconds = std::chrono::duration<double>;
                        auto timeline = session.GetTimelineProperties();
                        if (timeline) {
                            state.duration =
                                std::chrono::duration_cast<seconds>(
                                    timeline.EndTime() - timeline.StartTime())
                                    .count();
                            state.position =
                                std::chrono::duration_cast<seconds>(
                                    timeline.Position() - timeline.StartTime())
                                    .count();
                        }
                        // Since the app last said where it was.
                        if (state.playing && timeline) {
                            const double since =
                                std::chrono::duration_cast<seconds>(
                                    winrt::clock::now() -
                                    timeline.LastUpdatedTime())
                                    .count();
                            if (since > 0 && since < 3600) {
                                state.position += since;
                            }
                        }
                        state.updatedAt = NowSeconds();
                        // The cover, read again when the song changes. Some
                        // apps (like Spotify) give the new title first and
                        // the new cover a moment later: it's read again for
                        // a few seconds after a change, and while missing.
                        const std::wstring key =
                            state.appId + L"|" + state.title + L"|" +
                            state.artist;
                        if (key != lastTitle) {
                            lastTitle = key;
                            titleChangedAt = NowSeconds();
                        }
                        AcquireSRWLockShared(&g_mediaLock);
                        state.thumbnail = g_media.thumbnail;
                        ReleaseSRWLockShared(&g_mediaLock);
                        if (NowSeconds() - titleChangedAt < 6 ||
                            state.thumbnail.empty()) {
                            std::vector<BYTE> cover;
                            auto thumbnail =
                                properties ? properties.Thumbnail() : nullptr;
                            auto stream =
                                thumbnail ? thumbnail.OpenReadAsync().get()
                                          : nullptr;
                            if (stream) {
                                const uint32_t size =
                                    (uint32_t)std::min<uint64_t>(stream.Size(),
                                                                 8 << 20);
                                winrt::Windows::Storage::Streams::DataReader
                                    reader(stream);
                                reader.LoadAsync(size).get();
                                cover.resize(size);
                                reader.ReadBytes(cover);
                            }
                            if (cover != state.thumbnail) {
                                state.thumbnail = std::move(cover);
                                thumbnailSerial++;
                            }
                        }
                        state.thumbnailSerial = thumbnailSerial;
                    }
                    // (Given up on meanwhile, see StopMediaWatch: a newer
                    // thread may own it now.)
                    if (WaitForSingleObject(events[0], 0) == WAIT_OBJECT_0) {
                        break;
                    }
                    AcquireSRWLockExclusive(&g_mediaLock);
                    g_media = std::move(state);
                    ReleaseSRWLockExclusive(&g_mediaLock);
                    if (HWND island = g_islandWnd) {
                        PostMessage(island, WM_APP_MEDIA, 0, 0);
                    }
                } catch (winrt::hresult_error const& e) {
                    IslandLog(L"Media: %08X", (unsigned)e.code());
                }
            }
        }
    }
    winrt::uninit_apartment();
    for (HANDLE event : own->events) {
        CloseHandle(event);
    }
    return 0;
}

void StartMediaWatch() {
    if (g_mediaThread) {
        return;
    }
    g_mediaStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    g_mediaChangedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    g_mediaCommandEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!g_mediaStopEvent || !g_mediaChangedEvent || !g_mediaCommandEvent) {
        return;
    }
    auto* own = new MediaThreadEvents{};
    int copied = 0;
    for (HANDLE event :
         {g_mediaStopEvent, g_mediaChangedEvent, g_mediaCommandEvent}) {
        if (!DuplicateHandle(GetCurrentProcess(), event, GetCurrentProcess(),
                             &own->events[copied], 0, FALSE,
                             DUPLICATE_SAME_ACCESS)) {
            break;
        }
        copied++;
    }
    if (copied == 3) {
        g_mediaThread =
            CreateThread(nullptr, 0, MediaThreadProc, own, 0, nullptr);
    }
    if (!g_mediaThread) {
        for (int i = 0; i < copied; i++) {
            CloseHandle(own->events[i]);
        }
        delete own;
    }
}

void StopMediaWatch() {
    if (g_mediaThread) {
        SetEvent(g_mediaStopEvent);
        // (A media app's call that hangs: the thread ends by itself later,
        // with its own events.)
        WaitForSingleObject(g_mediaThread, 5000);
        CloseHandle(g_mediaThread);
        g_mediaThread = nullptr;
    }
    AcquireSRWLockExclusive(&g_mediaEventsLock);
    for (HANDLE* event :
         {&g_mediaStopEvent, &g_mediaChangedEvent, &g_mediaCommandEvent}) {
        if (*event) {
            CloseHandle(*event);
            *event = nullptr;
        }
    }
    ReleaseSRWLockExclusive(&g_mediaEventsLock);
    AcquireSRWLockExclusive(&g_mediaLock);
    g_media = {};
    g_mediaCommands.clear();
    g_mediaChosenApp.clear();
    g_mediaChosenOccurrence = 0;
    ReleaseSRWLockExclusive(&g_mediaLock);
}

std::wstring AppDisplayName(const std::wstring& appId);

// How far a text too long for its place has scrolled `elapsed` seconds after
// it started: a pause, on to its end, a pause, back, like on macOS.
double MarqueeShift(double elapsed, double distance, float scale) {
    const double speed = 30 * scale;
    const double pause = 1.5;
    const double travel = distance / speed;
    const double cycle = (pause + travel) * 2;
    const double t = std::fmod(std::max(elapsed, 0.0), cycle);
    double shift;
    if (t < pause) {
        shift = 0;
    } else if (t < pause + travel) {
        shift = (t - pause) * speed;
    } else if (t < pause * 2 + travel) {
        shift = distance;
    } else {
        shift = distance - (t - pause * 2 - travel) * speed;
    }
    return std::clamp(shift, 0.0, distance);
}

// "3:07", or "1:02:45".
std::wstring FormatMediaTime(double seconds) {
    const int total = (int)std::max(0.0, seconds);
    WCHAR text[32];
    if (total >= 3600) {
        swprintf_s(text, L"%d:%02d:%02d", total / 3600, total / 60 % 60,
                   total % 60);
    } else {
        swprintf_s(text, L"%d:%02d", total / 60, total % 60);
    }
    return text;
}

// Identifies writes from this panel; output notifications carry their state,
// so they need no microphone enumeration or endpoint activation on the UI.
constexpr GUID kMediaVolumeContext{
    0xd90aca71, 0x9f42, 0x4a21, {0xb3, 0x09, 0x57, 0x60, 0xe1, 0x9d, 0x65, 0x12}};

// A single lazy worker per panel. Only device/session IDs cross threads;
// every audio interface is created, used and released on the worker's MTA.
// Pending moves replace each other; the final position is never discarded.
class MediaVolumeWriter {
   public:
    static constexpr DWORD kBatchMilliseconds = 16;
    struct Write {
        std::wstring deviceId;
        std::vector<std::wstring> sessionIds;
        bool device = false;
        float level = 0;
        bool muted = false;
        bool setLevel = true;
        bool SameTarget(const Write& other) const {
            return device == other.device && deviceId == other.deviceId &&
                   sessionIds == other.sessionIds;
        }
    };

    static void KeepLatest(std::vector<Write>& pending, Write write) {
        auto old = std::find_if(pending.begin(), pending.end(),
            [&](const Write& item) { return item.SameTarget(write); });
        if (old != pending.end()) {
            if (!write.setLevel && old->setLevel) {
                write.level = old->level;
                write.setLevel = true;
            }
            *old = std::move(write);
        } else {
            pending.push_back(std::move(write));
        }
    }

    ~MediaVolumeWriter() { Stop(); }
    bool Busy() {
        AcquireSRWLockShared(&m_lock);
        const bool busy = m_writing || !m_pending.empty();
        ReleaseSRWLockShared(&m_lock);
        return busy;
    }
    bool Submit(Write write) {
        if (m_thread && WaitForSingleObject(m_thread, 0) == WAIT_OBJECT_0) {
            Stop();
            return false;
        }
        if (write.deviceId.empty() || (!write.device && write.sessionIds.empty())) {
            return false;
        }
        if (!m_thread) {
            m_wake = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            m_stop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            if (m_wake && m_stop) {
                m_thread = CreateThread(nullptr, 0, ThreadProc, this, 0, nullptr);
            }
            if (!m_thread) {
                Stop();
                IslandLog(L"Media volume worker could not start");
                return false;
            }
        }
        AcquireSRWLockExclusive(&m_lock);
        KeepLatest(m_pending, std::move(write));
        ReleaseSRWLockExclusive(&m_lock);
        SetEvent(m_wake);
        return true;
    }
    void Stop() {
        if (m_thread) {
            SetEvent(m_stop);
            SetEvent(m_wake);
            // No callback into the UI and no lock held across this join.
            // The worker flushes its pending final values before returning.
            WaitForSingleObject(m_thread, INFINITE);
            CloseHandle(m_thread);
            m_thread = nullptr;
        }
        if (m_wake) {
            CloseHandle(m_wake);
            m_wake = nullptr;
        }
        if (m_stop) {
            CloseHandle(m_stop);
            m_stop = nullptr;
        }
    }

   private:
    static DWORD WINAPI ThreadProc(void* param) {
        auto* self = static_cast<MediaVolumeWriter*>(param);
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(initialized)) {
            self->Run();
            // Run's local interfaces have already been released.
            CoUninitialize();
        } else {
            IslandLog(L"Media volume COM initialization failed: %08X",
                      (unsigned)initialized);
        }
        AcquireSRWLockExclusive(&self->m_lock);
        self->m_pending.clear();
        self->m_writing = false;
        ReleaseSRWLockExclusive(&self->m_lock);
        return 0;
    }
    void Run() {
        winrt::com_ptr<IMMDeviceEnumerator> devices;
        CoCreateInstance(kCLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL,
                         kIID_IMMDeviceEnumerator, devices.put_void());
        winrt::com_ptr<IMMDevice> endpoint;
        winrt::com_ptr<IAudioEndpointVolume> master;
        std::vector<winrt::com_ptr<ISimpleAudioVolume>> volumes;
        Write cached;
        for (;;) {
            WaitForSingleObject(m_wake, INFINITE);
            // At most one batch per 16 ms. This wait is on the worker only;
            // with no writes queued it sleeps indefinitely, without polling.
            const bool stopping = WaitForSingleObject(m_stop, kBatchMilliseconds) == WAIT_OBJECT_0;
            std::vector<Write> writes;
            AcquireSRWLockExclusive(&m_lock);
            writes.swap(m_pending);
            m_writing = !writes.empty();
            ReleaseSRWLockExclusive(&m_lock);
            for (const auto& write : writes) {
                if (!write.SameTarget(cached) || !endpoint) {
                    endpoint = nullptr;
                    master = nullptr;
                    volumes.clear();
                    cached = write;
                    if (devices && SUCCEEDED(devices->GetDevice(
                            write.deviceId.c_str(), endpoint.put()))) {
                        if (write.device) {
                            endpoint->Activate(kIID_IAudioEndpointVolume,
                                CLSCTX_ALL, nullptr, master.put_void());
                        } else {
                            winrt::com_ptr<IAudioSessionManager2> manager;
                            winrt::com_ptr<IAudioSessionEnumerator> sessions;
                            int count = 0;
                            if (SUCCEEDED(endpoint->Activate(
                                    kIID_IAudioSessionManager2, CLSCTX_ALL,
                                    nullptr, manager.put_void())) &&
                                SUCCEEDED(manager->GetSessionEnumerator(sessions.put())) &&
                                SUCCEEDED(sessions->GetCount(&count))) {
                                for (int n = 0; n < count; n++) {
                                    winrt::com_ptr<IAudioSessionControl> control;
                                    if (FAILED(sessions->GetSession(n, control.put()))) {
                                        continue;
                                    }
                                    auto control2 = control.try_as<IAudioSessionControl2>();
                                    LPWSTR id = nullptr;
                                    if (control2 && SUCCEEDED(
                                            control2->GetSessionInstanceIdentifier(&id))) {
                                        const bool matches = std::find(
                                            write.sessionIds.begin(),
                                            write.sessionIds.end(), id) !=
                                                write.sessionIds.end();
                                        CoTaskMemFree(id);
                                        if (matches) {
                                            if (auto volume = control.try_as<ISimpleAudioVolume>()) {
                                                volumes.push_back(std::move(volume));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if (write.device && master) {
                    if (write.setLevel) {
                        master->SetMasterVolumeLevelScalar(write.level, &kMediaVolumeContext);
                    }
                    BOOL muted = FALSE;
                    if (SUCCEEDED(master->GetMute(&muted)) && !!muted != write.muted) {
                        master->SetMute(write.muted, &kMediaVolumeContext);
                    }
                }
                for (const auto& volume : volumes) {
                    if (write.setLevel) {
                        volume->SetMasterVolume(write.level, &kMediaVolumeContext);
                    }
                    BOOL muted = FALSE;
                    if (SUCCEEDED(volume->GetMute(&muted)) && !!muted != write.muted) {
                        volume->SetMute(write.muted, &kMediaVolumeContext);
                    }
                }
            }
            AcquireSRWLockExclusive(&m_lock);
            m_writing = false;
            ReleaseSRWLockExclusive(&m_lock);
            if (stopping) {
                return;
            }
        }
    }
    SRWLOCK m_lock = SRWLOCK_INIT;
    HANDLE m_thread = nullptr, m_wake = nullptr, m_stop = nullptr;
    bool m_writing = false;
    std::vector<Write> m_pending;
};

////////////////////////////////////////////////////////////////////////////////
// The media controls, under the island: the cover, the song, a timeline to
// jump in it, previous / play / next, the volume (the wheel too), and the
// volume mixer for all the apps on the default output.

class MediaPanel : public Panel {
   public:
    ~MediaPanel() {
        m_volumeWriter.Stop();
        ClearMixerApps(m_apps);
    }

    void SetState(const MediaState& state);
    // Called every second while open, for the timeline and audio sessions.
    void Refresh() {
        if (IsOpen()) {
            // Keep a dragged row's identity and interfaces until release.
            if (m_dragging == Part::None && !m_press.held &&
                !m_volumeWriter.Busy() && !m_decorating) {
                ReadVolume();
                Layout();
                PrepareAssets();
            }
            RequestFrame();
        }
    }

   protected:
    bool DeferInputDraw() const override { return true; }
    void OnDestroy() override {
        m_volumeWriter.Stop();
        m_thumbnail = nullptr;
        m_previousThumbnail = nullptr;
        m_thumbnailSerial = (UINT)-1;
        m_appIcons.clear();
        m_texts.clear();
        for (auto& app : m_apps) {
            app.bitmap = nullptr;
        }
    }
    void Prepare(float scale) override;
    void OnOpen() override;
    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;
    bool OnMouseMove(POINT pt) override;
    bool OnMouseLeave() override;
    bool OnMouseDown(POINT pt) override;
    bool OnMouseUp(POINT pt) override;
    bool OnWheel(int delta) override;
    bool AnimateContents(double dt) override;
    void OnCaptureLost() override {
        m_dragging = Part::None;
        m_dragApp = -1;
        m_press.Release();
        RequestFrame();
    }
    void OnClose() override {
        OnCaptureLost();
    }

   private:
    enum class Part {
        None,
        Previous,
        PlayPause,
        Next,
        Timeline,
        Volume,
        // Each mixer row has its own slider and mute button.
        AppVolume,
        AppMute,
        DeviceMute,
        PlayingVolume,
        PlayingMute,
        MixerToggle,
        WindowsMixer,
        PillButtons,
        PillTitle,
        Source,
        // The cover: its app to the front.
        Cover,
    };
    // With several apps with media: "Automatic" (Windows' choice) and each
    // app, to choose which one is controlled; the one under the mouse.
    std::vector<D2D1_RECT_F> m_sourceChips;
    int SourceAt(POINT pt) const {
        for (size_t i = 0; i < m_sourceChips.size(); i++) {
            if (PointInRect(pt, m_sourceChips[i])) {
                return (int)i;
            }
        }
        return -1;
    }
    int m_sourceHover = -1;
    winrt::com_ptr<IDWriteTextFormat> m_chipFormat;
    // A button pressed shrinks, and springs back when released.
    PressAnimation m_press;
    double m_playSwap = 0;
    double m_playSwapVelocity = 0;
    // The switches' knobs, sliding (-1 until first drawn).
    double m_pillKnob = -1;
    double m_pillKnobVelocity = 0;
    double m_titleKnob = -1;
    double m_titleKnobVelocity = 0;
    // The timeline and the volume grow thicker under the mouse.
    double m_timelineHover = 0;
    double m_timelineHoverVelocity = 0;
    double m_volumeHover = 0;
    double m_volumeHoverVelocity = 0;

    void RequestFrame() {
        m_moved = true;
        StartPanelAnimation(Owner());
    }
    void QueueVolume(int index, bool setLevel = true);
    MediaVolumeWriter m_volumeWriter;
    std::wstring m_deviceId;
    void Layout();
    void LayoutMixer();
    float m_mixerListHeight = 0;
    void PrepareAssets();
    void PrepareTextCache();
    std::wstring SourceName(int source) const;
    ID2D1Bitmap* LoadAppBitmap(ID2D1RenderTarget* target, const std::wstring& appId);
    const std::wstring& AppName(const std::wstring& appId) const;
    std::vector<std::pair<std::wstring, std::wstring>> m_names;
    const std::wstring m_emptyName;
    struct ScrollingText {
        std::wstring text;
        IDWriteTextFormat* format = nullptr;
        float height = 0, width = 0;
        winrt::com_ptr<IDWriteTextLayout> layout;
    };
    std::vector<ScrollingText> m_texts;
    void CacheScrolling(const std::wstring& text, IDWriteTextFormat* format, float height);
    Part HitTest(POINT pt) const;
    ID2D1Bitmap* Thumbnail(ID2D1RenderTarget* target);
    void DrawPlayer(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush,
                    const MediaState& state, ID2D1Bitmap* coverBitmap,
                    bool current);
    void DrawVolumeIcon(ID2D1RenderTarget* target,
                        ID2D1SolidColorBrush* brush,
                        const D2D1_RECT_F& rect, int index, Part part);
    void DrawVolumeBar(ID2D1RenderTarget* target,
                       ID2D1SolidColorBrush* brush,
                       const D2D1_RECT_F& rect, double level,
                       double hover, bool accent);
    int PlayingMixerApp() const;
    double AppLevelShown(int index) const;
    // The device and all non-expired sessions on its default output.
    void ReadVolume();
    void ReadDeviceVolume();
    void SetVolume(float level);
    void SetAppVolume(int index, float level);
    void SetAppMute(int index, bool muted);
    int MixerAt(POINT pt, bool icon) const;
    POINT ContentPoint(POINT pt) const {
        pt.y += (LONG)std::round(m_scroll.Offset());
        return pt;
    }
    ID2D1Bitmap* AppBitmap(const std::wstring& appId) const;
    double PositionAt(POINT pt) const;

    MediaState m_state;
    winrt::com_ptr<IWICImagingFactory> m_wic;
    winrt::com_ptr<ID2D1Bitmap> m_thumbnail;
    UINT m_thumbnailSerial = (UINT)-1;
    D2D1_COLOR_F m_coverColor{0, 0, 0, 0};
    // The whole panel's tint follows the cover without a cut.
    double m_tintShown[4]{}, m_tintVelocity[4]{};
    bool m_tintReady = false;
    MediaState m_previousMedia;
    winrt::com_ptr<ID2D1Bitmap> m_previousThumbnail;
    double m_mediaBlend = 1, m_mediaBlendVelocity = 0;
    std::vector<KeptAppIcon> m_appIcons;
    SmoothScroll m_scroll;
    D2D1_RECT_F m_viewport{}, m_windowsMixer{};
    float m_playerBottom = 0;
    D2D1_RECT_F m_mixerToggle{}, m_mixerList{};
    bool m_mixerExpanded = false;
    double m_mixerReveal = 0, m_mixerRevealVelocity = 0;
    bool m_decorating = false;
    D2D1_RECT_F m_playingVolume{}, m_playingVolumeIcon{};
    // The app whose volume is under the player: the one playing, or the one
    // dragged until it's let go.
    int PlayingVolumeApp() const {
        return m_dragging == Part::PlayingVolume ? m_dragApp : m_playingApp;
    }
    // Where the mouse is on its slider (0 to 1; none laid out: 0).
    float PlayingVolumeAt(POINT pt) const {
        const float width = m_playingVolume.right - m_playingVolume.left;
        return width > 0
                   ? std::clamp((pt.x - m_playingVolume.left) / width, 0.0f, 1.0f)
                   : 0.0f;
    }
    int m_playingApp = -1;
    double m_playingHover = 0, m_playingHoverVelocity = 0;
    double m_deviceMute = -1, m_deviceMuteVelocity = 0;
    double m_deviceLevel = -1, m_deviceLevelVelocity = 0;
    struct MixerMotion {
        DWORD processId = 0;
        std::wstring path;
        bool system = false;
        double mute = 0, muteVelocity = 0;
        double level = 1, levelVelocity = 0;
    };
    std::vector<MixerMotion> m_mixerMotion;
    std::vector<MixerApp> m_apps;
    std::vector<D2D1_RECT_F> m_appIconsRect, m_appSliders;
    int m_hoverApp = -1, m_dragApp = -1;
    int m_pressedApp = -1;
    std::wstring m_pressedSource;
    winrt::com_ptr<IDWriteTextFormat> m_titleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_timeFormat;
    winrt::com_ptr<IDWriteTextFormat> m_timeRightFormat;
    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_bigIconFormat;
    float m_preparedScale = 0;
    D2D1_RECT_F m_cover{}, m_timeline{}, m_previous{}, m_playPause{}, m_next{},
        m_volume{}, m_volumeIcon{},
        m_pillButtons{}, m_pillTitle{};
    Part m_hover = Part::None;
    // Dragging the timeline (to a position) or the volume.
    Part m_dragging = Part::None;
    double m_dragPosition = 0;
    float m_level = 0;
    bool m_muted = false;
    double m_appVolumeHover = 0;
    double m_appVolumeHoverVelocity = 0;
    // A title or an artist too long scrolls under the mouse (from
    // m_scrollStart, another song, or the mouse coming onto it; m_scrolling
    // while one does).
    double m_scrollStart = 0;
    bool m_scrolling = false;
    void DrawScrolling(ID2D1RenderTarget* target,
                       ID2D1SolidColorBrush* brush,
                       const std::wstring& text,
                       IDWriteTextFormat* format,
                       const D2D1_RECT_F& rect);
};

// The Windows banner and the hook that follows it (see BannerWinEventProc).
extern HWND g_windowsBanner;
void CALLBACK BannerWinEventProc(HWINEVENTHOOK,
                                 DWORD,
                                 HWND,
                                 LONG,
                                 LONG,
                                 DWORD,
                                 DWORD);
bool IsShellExperienceHost(DWORD processId);

////////////////////////////////////////////////////////////////////////////////
// The power button's menu, like the macOS Apple menu: sleep, restart, shut
// down, lock and sign out. Restarting, shutting down and signing out ask
// first (a confirmation opens under the row); Windows then closes the apps as
// it always does, so an app with unsaved work can still stop it.

