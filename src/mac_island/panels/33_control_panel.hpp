class ControlPanel : public Panel {
   public:
    ~ControlPanel() {
        ClearApps();
        StopWifi();
        WaitForWorkers();
        StopKomorebiJob();
    }

    // The controls chosen were changed elsewhere (a profile applied).
    void ReloadControls() {
        m_controlsLoaded = false;
        Layout();
    }

    // The monitors' list is old (the option for the external ones' DDC/CI was
    // switched, see g_externalBrightness): the sliders go, and it's read
    // again now if the control center is open, or when it opens. What was
    // asked for the old list's places isn't set (they're not the new ones').
    void ReloadBrightness() {
        m_brightnessMonitors.clear();
        m_brightness = -1;
        m_brightnessReadAt = 0;
        m_brightnessJob->Ask(-1, -1);
        if (IsOpen() && !m_windowed && !m_brightnessJob->running) {
            m_brightnessReadAt = GetTickCount64();
            StartBrightnessWorker(BrightnessListProc);
        }
        Layout();
    }

    // Windows' dark mode, switched from the pill's button (on a thread of
    // the panel's, like its own tile does).
    void SwitchDarkMode(bool dark) {
        m_darkMode = dark;
        SetDarkMode(dark);
        StartWorker(DarkModeThreadProc, nullptr);
    }
    // Called while open, to show changes made elsewhere.
    // (Not the settings: they change by a click or Windhawk's settings,
    // which lay them out again.)
    void Refresh() {
        if (IsOpen() && m_view != View::Settings && !m_dragging &&
            m_dragId < 0) {
            CheckConnecting();
            Update();
            Layout();
            Render();
        }
    }
    // The settings shown: laid out again, a list in them changed elsewhere
    // (an app seen playing, see SettingKind::LyricsApps).
    void RefreshSettings() {
        if (IsOpen() && m_view == View::Settings && !m_dragging &&
            m_dragId < 0) {
            Layout();
            Render();
        }
    }
    // Only the sound (opened from the volume, like on macOS), only the
    // battery and the power mode (from the battery), only the screen's
    // brightness (from its item), or all of it. Returns whether that changed;
    // while open, it shows the other one.
    // (View: one control's own view alone, from its button on the pill.)
    enum class Only { All, Sound, Battery, Display, View, Komorebi };
    // The arrow back: not on the settings opened on their own (from the
    // island's settings bubble); on a page opened alone from its icon on the
    // pill (Wi-Fi, Bluetooth...), back to the whole control center (#6160).
    bool ShowsBack() const { return m_view != View::Settings; }
    // The settings' window as it is now, and put back so (see
    // SettingsWindowState): its place before it shows, the rest after.
    void KeepWindowState(SettingsWindowState* state) const {
        GetWindowPlacement(m_hwnd, &state->placement);
        GetWindowRect(m_hwnd, &state->rect);
        state->tab = m_settingsTab;
        state->search = m_search;
        state->scroll = m_settingsScroll.offset;
        state->navScroll = m_navScroll.offset;
        state->setup = m_setup;
        state->restoreEntry = m_restoreEntry;
        state->restore.clear();
        for (const auto& change : m_restore) {
            state->restore.push_back({(int)change.pref, change.from, change.to});
        }
    }
    void PlaceWindow(const SettingsWindowState& state) {
        WINDOWPLACEMENT placement = state.placement;
        placement.showCmd = SW_HIDE;
        // (Twice: moved onto a monitor of another DPI, the window takes the
        // size Windows suggests for it, a size too big there; the second
        // time it's on that monitor already.)
        SetWindowPlacement(m_hwnd, &placement);
        SetWindowPlacement(m_hwnd, &placement);
        if (state.placement.showCmd == SW_SHOWNORMAL &&
            state.rect.right > state.rect.left) {
            SetWindowPos(m_hwnd, nullptr, state.rect.left, state.rect.top,
                         state.rect.right - state.rect.left,
                         state.rect.bottom - state.rect.top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        m_windowPlaced = true;
        m_windowDpi = GetDpiForWindow(m_hwnd);
    }
    void PutWindowState(const SettingsWindowState& state) {
        if (state.placement.showCmd == SW_SHOWMAXIMIZED) {
            ShowWindow(m_hwnd, SW_MAXIMIZE);
        }
        if (SettingTabShown(state.tab)) {
            m_settingsTab = state.tab;
        }
        m_setup = state.setup;
        m_search = state.search;
        m_searchEdit.Reset();
        m_restore.clear();
        for (const auto& [pref, from, to] : state.restore) {
            m_restore.push_back({(IslandPref)pref, from, to});
        }
        m_restoreEntry = m_restore.empty() ? -1 : state.restoreEntry;
        Layout();
        SettleSettings();
        m_settingsScroll.offset = m_settingsScroll.target =
            std::clamp(state.scroll, 0.0, (double)m_settingsScroll.Max());
        m_navScroll.offset = m_navScroll.target =
            std::clamp(state.navScroll, 0.0, (double)m_navScroll.Max());
        Render();
    }
    // On a tab (What's new from its notice: in the island's language).
    void GoToTab(int tab) {
        if (!SettingTabShown(tab)) {
            return;
        }
        if (tab == kTabWhatsNew) {
            g_notesLanguage = -1;
        }
        m_settingsTab = tab;
        m_search.clear();
        m_searchEdit.Reset();
        m_settingsScroll.Reset();
        OnSearchChanged();
        SettleSettings();
        Render();
    }
    // The island's settings (from the island's settings bubble): shown now
    // if it's open, or when it opens next.
    void ShowSettings() {
        if (IsOpen()) {
            m_editing = false;
            SetView(View::Settings);
            Render();
        } else {
            m_openOnSettings = true;
        }
    }
    // Whether it shows that control's view alone (see Only::View).
    bool ShowingControl(ControlId id) const {
        return m_only == Only::View &&
               ((id == ControlId::Network &&
                 (m_view == View::Wifi || m_view == View::Password)) ||
                (id == ControlId::Bluetooth && m_view == View::Bluetooth) ||
                (id == ControlId::Project && m_view == View::Project) ||
                (id == ControlId::Accessibility &&
                 m_view == View::Accessibility));
    }
    // The radios' state asked again (they light the pill's buttons): 0
    // Wi-Fi, 1 Bluetooth, 2 the hotspot, 3 airplane mode; 1 on, 0 off,
    // negative unknown.
    void ReadRadios() { SwitchRadio(-1, false); }
    int RadioState(int kind) const {
        return kind == 0   ? m_wifiRadio
               : kind == 1 ? m_bluetoothRadio
               : kind == 2 ? m_hotspot
                           : m_airplane;
    }
    // Switched from the pill (shown at once; the thread confirms). False
    // while its state isn't known.
    bool ToggleRadio(int kind) {
        int& state = kind == 2 ? m_hotspot : m_airplane;
        if (state < 0) {
            return false;
        }
        state = state == 1 ? 0 : 1;
        SwitchRadio(kind, state == 1);
        return true;
    }
    // Opened on a control's own view (Wi-Fi, Bluetooth, projecting,
    // accessibility) or on all of them, from its button on the pill: now if
    // it's open, or when it opens next.
    void ShowControl(ControlId id) {
        if (IsOpen()) {
            m_editing = false;
            OpenControlView(id);
            Render();
        } else {
            m_openOnControl = (int)id;
        }
    }
    bool SetOnly(Only only) {
        if (only == m_only) {
            return false;
        }
        m_only = only;
        if (IsOpen()) {
            m_editing = false;
            SetView(View::Main);
            Render();
        }
        return true;
    }

   protected:
    void Prepare(float scale) override;
    void OnOpen() override;
    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;
    // (The settings' window.)
    bool StaysWhenDeactivated() const override { return m_windowed; }
    void OnResized() override {
        Layout();
        SettleSettings();
    }
    void OnActivated() override { Layout(); }
    POINT WindowContentOffset() const override {
        if (m_view != View::AppPlaces) {
            return {};
        }
        RECT client{};
        GetClientRect(m_hwnd, &client);
        const LONG margin = std::lround(16 * m_scale);
        return {std::max(margin, (client.right - m_size.cx) / 2), margin};
    }
    bool OnMouseMove(POINT pt) override;
    bool OnMouseLeave() override;
    bool OnMouseDown(POINT pt) override;
    bool OnMouseUp(POINT pt) override;
    bool OnWheel(int delta) override;
    bool OnKey(WPARAM key) override;
    bool OnChar(WCHAR c) override;
    bool OnAppMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;
    void OnDestroy() override;
    void OnCaptureLost() override { m_barDrag = 0; }
    // (The settings' window: the apps' icons made on its target.)
    void OnWindowTargetMade() override {
        m_lyricsAppIcons.clear();
        PrepareLyricsAppIcons();
    }
    // (Keys being recorded: no more. The setup assistant closed before its
    // Finish: its Cancel, and the window opens on the first tab next time.)
    void OnClose() override {
        StartShortcutRecording(false);
        if (!m_setup.from.empty()) {
            EndSetup(true);
            m_settingsTab = kTabGeneral;
        }
    }

   private:
    enum class View {
        Main,
        Output,
        Mixer,
        Wifi,
        Password,
        Bluetooth,
        Project,
        Accessibility,
        // The island's own options (see Island::ShowGearBubble).
        Settings,
        // A place for some apps (from the island's settings).
        AppPlaces,
        // The power plans, to choose one.
        PowerPlans,
        // The mobile hotspot's network name and password (see
        // HotspotConfigThreadProc).
        Hotspot,
    };

    enum class Part {
        None,
        Network,
        Bluetooth,
        DoNotDisturb,
        NightLight,
        VolumeIcon,
        Slider,
        Device,
        MixerButton,
        Battery,
        More,
        Back,
        Output,      // A device in the output list (index).
        AppIcon,     // An app's icon in the mixer: mutes it (index).
        AppSlider,   // An app's volume in the mixer (index).
        WifiToggle,  // Wi-Fi on/off: the tile's icon, or the list's switch.
        BluetoothToggle,
        WifiRow,       // A Wi-Fi network (index).
        Disconnect,    // "Disconnect" on the connected network (index).
        Forget,        // "Forget" on a saved network (index).
        WifiConnect,   // "Connect" on a network (index).
        WifiLocation,  // The list's line without the location access.
        BluetoothRow,  // A Bluetooth device (index).
        PasswordField,
        ShowPassword,
        Connect,
        Shortcut,  // Project, cast, captions, accessibility (index).
        BrightnessSlider,
        PowerModeSegment,  // A power mode (index).
        PowerPlanRow,      // A power plan in their list (index).
        PowerPlan,         // The active power plan (opens their list).
        HotspotToggle,
        HotspotEdit,   // The hotspot's tile, beside its icon: its page.
        HotspotName,   // Its network name's field.
        HotspotSave,
        AirplaneToggle,
        DndToggle,
        ProjectRow,        // A display mode (index).
        KomorebiButton,    // komorebi's start, stop or reload (index).
        AccessibilityRow,  // An accessibility feature (index).
        EditButton,        // "Edit" at the bottom.
        SettingsButton,    // The island's settings, beside it.
        EditControl,       // A control, while editing (its ControlId).
        EditRemove,        // Its "-" (its ControlId).
        EditAdd,           // A control to add (its ControlId).
        EditDone,          // "Done".
        SettingToggle,     // An island option's switch (its IslandPref).
        // A dimmed option (its IslandPref, kDisabledChoice + its ChoiceId,
        // or kDisabledSlider + its slider): why it is, in a bubble.
        DisabledSetting,
        StyleSegment,    // See StyleSegmentStyle.
        ThemeSwatch,     // A theme (its index).
        PlaceDot,        // A ready place for all apps (0-8).
        AppPlacesRow,    // "Place per app".
        AppPlaceDot,     // An app's place (entry * 9 + place).
        AppPlaceRemove,  // An app's "x" (entry).
        AppPlaceAdd,     // An open app to give a place (its index).
        AppItemChip,     // An app's icon to show or hide (see m_appItemChips).
        LyricsAppChip,   // An app's lyrics on or off (its LyricsApps index).
        // The profiles (index: the profile's position; on another island
        // than the main one, its count for the main one's settings), the
        // "+" beside their title, and the one being edited.
        ProfileChip,
        ProfileAdd,
        ProfileEditButton,
        ProfileName,
        ProfileUpdate,
        ProfileDelete,
        ProfileDone,
        // What turns the one being edited on by itself (#6166): a style
        // (index: its segment), an app of it (index: in m_profileApps,
        // removed), the app last in front added, and staying after its app
        // or not (index 1: staying).
        ProfileStyleChip,
        ProfileAppChip,
        ProfileAppAdd,
        ProfileKeepChip,
        // The Monitors section (index: the part in m_monitorParts): the
        // mode (one monitor, all, following the mouse), a screen on the
        // map, and a profile for another monitor.
        MonitorMode,
        MonitorMap,
        MonitorProfile,
        // An island showing the notifications or not (its device; none for
        // the main one's).
        MonitorNotify,
        SizeSlider,     // The island's size.
        SizeReset,      // Back to its usual size.
        SearchField,    // The settings' search box.
        SearchClear,    // Its "x".
        OrderChip,      // An item in the order strip (its PillGroup).
        ChoiceSegment,  // A choice of a row (ChoiceId * 16 + its index).
        SettingsTab,    // A tab of the settings (its SettingTab).
        DisabledLink,    // A dimmed option's reason (see kDisabledStride).
        NeedsYes,        // The bubble of what it needs: "Yes, change".
        NeedsNo,         // ...and "Cancel".
        NeedsBox,        // ...anywhere else on it.
        RestoreNeeds,    // "Restore what changed" (see ApplyNeeds).
        NoteLink,        // An item of "What's new" (its m_noteLinks index).
        PageButton,      // A button of the pages about the mod (PageButton).
        ShortcutRecord,  // The minimize shortcut's keys: records new ones.
        ShortcutReset,   // Back to Win+J.
        IconCell,        // An icon of the Icons tab (its kIcons index).
        IconCellReset,   // Its "x": back to the pack's (its kIcons index).
        IconCellColor,   // Its brush: a color of its own (its kIcons index).
        IconPackDelete,  // An icon pack's bin: its pictures deleted (its pack).
        SetupButton,     // A button of the setup assistant (its SetupButton).
    };

    struct Hit {
        Part part = Part::None;
        int index = -1;
        bool operator==(const Hit& other) const {
            return part == other.part && index == other.index;
        }
    };

    // A Wi-Fi network in reach.
    struct WifiNetwork {
        std::wstring name;
        DOT11_SSID ssid{};
        int signal = 0;
        bool secure = false;
        bool connected = false;
        // Its saved profile, if any.
        std::wstring profile;
        DOT11_AUTH_ALGORITHM auth = DOT11_AUTH_ALGO_80211_OPEN;
        DOT11_CIPHER_ALGORITHM cipher = DOT11_CIPHER_ALGO_NONE;
    };

    struct BluetoothDevice {
        std::wstring name;
        bool connected;
        WCHAR glyph;
        ULONGLONG address = 0;
        // Headphones or speakers: connected from its row (others open
        // Windows' Bluetooth settings).
        bool audio = false;
    };

    // An accessibility feature with an on/off switch.
    enum class Feature {
        Magnifier,
        Narrator,
        ColorFilters,
        StickyKeys,
        OnScreenKeyboard
    };
    // A switch, or a button that starts the feature (the magnifier's and the
    // narrator's state isn't told reliably: a switch could show it wrong).
    struct AccessibilityItem {
        Feature feature;
        WCHAR glyph;
        PCWSTR name;
        bool on;
        bool button = false;
    };

    void Update();
    void UpdateOutputs();
    void UpdateApps();
    void ClearApps();
    void StartWifi();
    void StopWifi();
    // The networks in reach, read on a thread (reading them takes 10 to 40
    // ms; with `scan`, Windows looks for them again first).
    void UpdateWifi(bool scan = false);
    static DWORD WINAPI WifiListThreadProc(LPVOID parameter);
    void ForgetWifi(int index);
    // A long status (why joining failed) on as many lines as it needs.
    winrt::com_ptr<IDWriteTextLayout> StatusLayout(const std::wstring& status,
                                                   float width) const;
    void UpdateBluetooth();
    void ClickBluetoothDevice(int index);
    void CheckBluetoothStep();
    bool ConnectWifi(const WifiNetwork& network, const std::wstring* key);
    void JoinWifi(int index);
    void SubmitPassword();
    void SwitchRadio(int kind, bool on);
    bool StartWorker(LPTHREAD_START_ROUTINE proc, void* parameter);
    bool StartBrightnessWorker(LPTHREAD_START_ROUTINE proc);
    void WaitForWorkers();
    void UpdateDisplayMode();
    void UpdateAccessibility();
    void ToggleAccessibility(int index);
    void SetBrightnessAt(int row, POINT pt);
    void SetBrightness(int row, int level);

    public:
    bool TakeBrightness(HMONITOR screen, int level);

    private:
    std::wstring StatusText() const;
    void Layout();
    void SetView(View view);
    Hit HitTest(POINT pt) const;
    void SetVolumeAt(POINT pt);
    void SetAppVolumeAt(int index, POINT pt);
    void SetAppVolume(int index, float volume);
    void DrawTile(ID2D1RenderTarget* target,
                  ID2D1SolidColorBrush* brush,
                  const D2D1_RECT_F& rect,
                  WCHAR glyph,
                  bool active,
                  PCWSTR title,
                  const std::wstring& subtitle,
                  bool hover);
    void DrawSlider(ID2D1RenderTarget* target,
                    ID2D1SolidColorBrush* brush,
                    const D2D1_RECT_F& rect,
                    float level,
                    WCHAR glyph,
                    bool hoverIcon);
    void DrawHeader(ID2D1RenderTarget* target,
                    ID2D1SolidColorBrush* brush,
                    PCWSTR title,
                    int toggle = kRadioMissing);
    // `on` from 0 (off) to 1 (on), animated (see KnobFor).
    void DrawToggle(ID2D1RenderTarget* target,
                    ID2D1SolidColorBrush* brush,
                    const D2D1_RECT_F& rect,
                    float on);
    // A switch's knob, sliding to where it's now (a bit springy).
    float KnobFor(int key, bool on);
    void DrawStatus(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawWifi(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawPassword(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawHotspot(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawBluetooth(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    D2D1_RECT_F BluetoothButtonRect(const D2D1_RECT_F& row) const;
    void DrawProject(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawSettings(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    // One control of the main view, at its place.
    void DrawControl(ID2D1RenderTarget* target,
                     ID2D1SolidColorBrush* brush,
                     ControlId id,
                     const D2D1_RECT_F& rect);
    // Editing: each control drawn once into a picture, which then only
    // moves (sliding to its place, dragged); drawing them all again on every
    // frame costs a lot. While editing they don't change.
    struct ControlTile {
        ControlId id;
        D2D1_SIZE_F size;
        // The icons it was drawn with (see IconsGeneration).
        UINT icons = 0;
        winrt::com_ptr<ID2D1Bitmap> bitmap;
    };
    std::vector<ControlTile> m_tiles;
    ID2D1Bitmap* GetControlTile(ID2D1RenderTarget* target,
                                ID2D1SolidColorBrush* brush,
                                ControlId id,
                                const D2D1_RECT_F& rect,
                                float margin);
    // Editing: the "-" at a control's corner.
    void DrawRemoveBadge(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush,
                         const D2D1_RECT_F& rect,
                         float amount);
    void SetEditing(bool editing);
    // What only the main island does (it shows the alerts and takes the
    // shortcuts): dimmed in another monitor's island's settings.
    static bool MainIslandOnly(IslandPref pref) {
        switch (pref) {
            case IslandPref::OwnNotifications:
            case IslandPref::ShortcutNotifications:
            case IslandPref::ShortcutControl:
            case IslandPref::MinimizeKey:
            case IslandPref::CapsLock:
            case IslandPref::StatusNotices:
            case IslandPref::Banner:
            case IslandPref::BannerMinimized:
            case IslandPref::HideWindowsBanners:
            case IslandPref::WebPictures:
            case IslandPref::WhatsNewNotice:
                return true;
            default:
                return false;
        }
    }
    // A slider dimmed (it still moves, for later): the distance from the
    // edge with the bar or the notches, the speed without animations, the
    // delay without auto-hide, the notches' curves without them, the line's
    // with the bar, the title's width without it, the pill's height with the
    // bar, its corners with the bar or the notches, the glass's blur without
    // the glass, the grid's columns without the grid, and the visualizer's
    // sensitivity without it.
    static bool SliderDisabled(int slider) {
        const bool bar = g_settings.islandBar;
        const bool notches = NotchCountWanted() > 0;
        return ((slider == 1 || slider == 13) && (bar || notches)) ||
               (slider == 2 && AnimationLevel() >= 2) ||
               (slider == 5 && !g_settings.islandAutoHide) ||
               (slider == 6 && !notches) ||
               ((slider == 8 || slider == 9 || slider == 12) && bar) ||
               (slider == 11 && !g_mediaTitleInPill) ||
               (slider == 16 && !g_settings.islandLiquidGlass) ||
               (slider == 17 && g_settings.islandTrayStyle != 1) ||
               (slider == 18 && !g_settings.islandVisualizer);
    }
    // An option that depends on another one being on (or the main island's
    // only, on another one).
    static bool PrefDisabled(IslandPref pref) {
        const bool notches = NotchCountWanted() > 0;
        return (OtherIslandShown() && MainIslandOnly(pref)) ||
               (pref == IslandPref::FollowMouse &&
                (GetIslandPref(IslandPref::AllMonitors) || notches)) ||
               ((pref == IslandPref::FreeDrag ||
                 pref == IslandPref::VerticalSides ||
                 pref == IslandPref::Frosted) &&
                notches) ||
               (pref == IslandPref::Frosted &&
                (!g_transparencyEffects || g_settings.islandLiquidGlass)) ||
               (pref == IslandPref::NotchEach && !notches) ||
               (pref == IslandPref::MinimizeButton && g_settings.islandBar) ||
               (pref == IslandPref::BarCorners &&
                (GetIslandPref(IslandPref::Bar) != 1 ||
                 !GetIslandPref(IslandPref::ClockCenter))) ||
               (pref == IslandPref::GlassTranslucent &&
                !GetIslandPref(IslandPref::LiquidGlass)) ||
               (pref == IslandPref::ClockSeconds &&
                !GetIslandPref(IslandPref::Clock)) ||
               ((pref == IslandPref::CountdownEvents ||
                 pref == IslandPref::CountdownAlarms ||
                 pref == IslandPref::CountdownTimer ||
                 pref == IslandPref::CountdownPomodoro) &&
                !GetIslandPref(IslandPref::Countdown)) ||
               ((pref == IslandPref::AiClaude || pref == IslandPref::AiCodex) &&
                !GetIslandPref(IslandPref::AiUsage)) ||
               (pref == IslandPref::FocusedTitle &&
                !GetIslandPref(IslandPref::FocusedApp)) ||
               (pref == IslandPref::WeatherCards &&
                !GetIslandPref(IslandPref::Weather)) ||
               (pref == IslandPref::DndSilent &&
                !GetIslandPref(IslandPref::OwnNotifications)) ||
               (pref == IslandPref::DownloadStalled &&
                !GetIslandPref(IslandPref::DownloadFill)) ||
               (pref == IslandPref::FolderThumbnails &&
                !GetIslandPref(IslandPref::Folders)) ||
               (pref == IslandPref::TimerLine &&
                !GetIslandPref(IslandPref::Timer)) ||
               (pref == IslandPref::GlassLight &&
                !GetIslandPref(IslandPref::LiquidGlass)) ||
               // (The timer's own item shows them, see
               // Island::UpdateExtraItems.)
               ((pref == IslandPref::CountdownTimer ||
                 pref == IslandPref::CountdownPomodoro) &&
                GetIslandPref(IslandPref::Timer)) ||
               (pref == IslandPref::VolumePercent &&
                !GetIslandPref(IslandPref::Volume)) ||
               (pref == IslandPref::BatteryPercent &&
                !GetIslandPref(IslandPref::Battery)) ||
               (pref == IslandPref::TrayOpen &&
                (!GetIslandPref(IslandPref::Tray) ||
                 GetIslandPref(IslandPref::TrayStyle) != 0)) ||
               // (The Fixed style hiding as its line keeps no strip.)
               (pref == IslandPref::Frosted &&
                (!GetIslandPref(IslandPref::Bar) ||
                 (GetIslandPref(IslandPref::Bar) == 2 &&
                  GetIslandPref(IslandPref::AutoHide)))) ||
               (pref == IslandPref::MonthNumber &&
                !GetIslandPref(IslandPref::ClockDate)) ||
               (pref == IslandPref::MediaMarquee &&
                !GetIslandPref(IslandPref::MediaTitle)) ||
               (pref == IslandPref::LyricsInTitle &&
                (!GetIslandPref(IslandPref::Lyrics) ||
                 !GetIslandPref(IslandPref::MediaTitle))) ||
               (pref == IslandPref::VisualizerSmooth &&
                !GetIslandPref(IslandPref::Visualizer)) ||
               ((pref == IslandPref::BannerMinimized ||
                 pref == IslandPref::HideWindowsBanners ||
                 pref == IslandPref::WebPictures) &&
                !GetIslandPref(IslandPref::Banner)) ||
               (pref == IslandPref::HideWindowsBanners &&
                GetIslandPref(IslandPref::OwnNotifications)) ||
               (pref == IslandPref::KomorebiLayout && g_komorebiFound == 0) ||
               (pref == IslandPref::KomorebiHideEmpty &&
                !GetIslandPref(IslandPref::KomorebiLayout)) ||
               // (The Bar hides by the minimize shortcut, no auto-hide.)
               (pref == IslandPref::HideClock && !g_settings.islandBar &&
                (!g_settings.islandAutoHide || notches)) ||
               (pref == IslandPref::BarClear &&
                (GetIslandPref(IslandPref::Bar) != 1 ||
                 GetIslandPref(IslandPref::LiquidGlass)));
    }
    void DrawShortcut(ID2D1RenderTarget* target,
                      ID2D1SolidColorBrush* brush,
                      const D2D1_RECT_F& rect,
                      WCHAR glyph,
                      PCWSTR name,
                      bool active,
                      bool hover);
    // Text that scrolls back and forth when it doesn't fit (see
    // AnimateContents).
    void DrawMarquee(ID2D1RenderTarget* target,
                     ID2D1SolidColorBrush* brush,
                     const std::wstring& text,
                     IDWriteTextFormat* format,
                     const D2D1_RECT_F& rect,
                     bool centered = false);
    bool AnimateContents(double dt) override;
    // The controls shown, in order, and remembering them.
    void LoadControls();
    void SaveControls();
    void DrawAccessibility(ID2D1RenderTarget* target,
                           ID2D1SolidColorBrush* brush);
    void DrawRowToggle(ID2D1RenderTarget* target,
                       ID2D1SolidColorBrush* brush,
                       const D2D1_RECT_F& row,
                       WCHAR glyph,
                       PCWSTR name,
                       bool on,
                       bool hover,
                       bool toggle = true);
    void DrawMain(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawView(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawOutputs(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void DrawMixer(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);

    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_titleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_subtitleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_sectionFormat;
    winrt::com_ptr<IDWriteTextFormat> m_percentFormat;
    winrt::com_ptr<IDWriteTextFormat> m_linkFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallIconFormat;
    winrt::com_ptr<IWICImagingFactory> m_wic;
    float m_preparedScale = 0;

    View m_view = View::Main;
    // Opened from the volume or the battery: only those (see SetOnly).
    Only m_only = Only::All;
    // Changing views: the new one slides in and fades in (0 to 1).
    double m_viewIn = 1;
    double m_viewInVelocity = 0;
    int m_viewDirection = 1;

    // State shown.
    NetworkInfo m_network;
    float m_volume = 0;
    bool m_muted = false;
    bool m_hasVolume = false;
    std::wstring m_device;
    bool m_doNotDisturb = false;
    bool m_hasBattery = false;
    int m_batteryPercent = 0;
    // The power mode (see GetPowerMode), its section and its three choices.
    int m_powerMode = -1;
    // The microphone muted (1), on (0), or missing (-1).
    int m_microphoneMuted = -1;
    D2D1_RECT_F m_powerModeRect{};
    D2D1_RECT_F m_powerModeChoices[3]{};
    // komorebi's section (see FindKomorebic): its tool (empty: not
    // installed; looked for again at each opening), its version, whether it
    // runs, its three buttons, and the button whose command is running
    // (without a window, see KomorebiThreadProc; -1 none) and since when.
    std::wstring m_komorebic;
    std::wstring m_komorebiVersion;
    bool m_komorebiLooked = false;
    // komorebi and whkd looked for, for the settings (see LookForKomorebi).
    bool m_toolsLooked = false;
    bool m_komorebiRunning = false;
    int m_komorebiBusy = -1;
    double m_komorebiBusySince = 0;
    // The last command failed (shown in a terminal); until the next one.
    bool m_komorebiFailed = false;
    // The thread running a button's commands, and its stop event (see
    // KomorebiThreadProc).
    HANDLE m_komorebiThread = nullptr;
    HANDLE m_komorebiStop = nullptr;
    void RunKomorebi(int button);
    void StopKomorebiJob();
    D2D1_RECT_F m_komorebiButtons[3]{};
    winrt::com_ptr<IDWriteTextFormat> m_komorebiIconFormat;
    bool KomorebiShown() const;
    void PrepareFormats(float scale);
    bool KomorebiEnabled(int button) const;
    void UpdateKomorebi();
    // The power plans, the active one, and its row in the section (which
    // opens their list).
    std::vector<PowerPlan> m_powerPlans;
    int m_activePlan = -1;
    double m_powerReadAt = 0;
    D2D1_RECT_F m_powerPlanRow{};
    void DrawPowerPlans(ID2D1RenderTarget* target,
                        ID2D1SolidColorBrush* brush);
    bool m_charging = false;
    std::vector<AudioOutput> m_outputs;
    std::vector<MixerApp> m_apps;

    // Wi-Fi, with a WLAN handle of its own that's told of scans and
    // connections.
    HANDLE m_wlan = nullptr;
    GUID m_wifiInterface{};
    bool m_hasWifi = false;
    std::vector<WifiNetwork> m_networks;
    // Joining a network with a password.
    WifiNetwork m_joining;
    std::wstring m_password;
    TextEdit m_passwordEdit;
    bool m_showPassword = false;
    // The hotspot's page (it uses the password's field too): its network
    // name, which field is typed in, what it says, and a read or a change
    // under way.
    std::wstring m_hotspotName;
    TextEdit m_hotspotNameEdit;
    bool m_hotspotTypingName = true;
    std::wstring m_hotspotStatus;
    bool m_hotspotBusy = false;
    void OpenHotspot();
    void SaveHotspot();
    void StartHotspotWorker(HotspotRequest* request);
    // Profiles made to join whose join hasn't ended yet: each one is removed
    // if its own join fails (a wrong password isn't kept).
    std::vector<std::wstring> m_createdProfiles;
    // Joining `m_connectingProfile` (given up after a while: Windows may
    // never say how it went).
    bool m_connecting = false;
    std::wstring m_connectingProfile;
    ULONGLONG m_connectingSince = 0;
    static constexpr ULONGLONG kConnectingMs = 45000;
    void CheckConnecting();
    void OnWifiJoinEnded(const WlanConnectionEvent& event);
    std::wstring m_wifiStatus;
    // The thread reading the networks, and whether to read them (and scan)
    // again when it's done.
    HANDLE m_wifiListThread = nullptr;
    // Its number (in the answer's wParam: an older answer still queued is
    // dropped, not taken for the current thread's).
    UINT m_wifiListId = 0;
    bool m_wifiListAgain = false;
    bool m_wifiScanAgain = false;
    // Windows refused the networks in reach (no location access): the
    // list says so, and its line opens the page that allows it.
    bool m_wifiRefused = false;
    // The saved network whose "Forget" was clicked once (a second click
    // forgets it).
    std::wstring m_forgetProfile;
    // The networks' buttons (Connect, Disconnect, Forget), each row's.
    struct RowButton {
        size_t row;
        Part part;
        D2D1_RECT_F rect;
    };
    std::vector<RowButton> m_rowButtons;
    bool RowButtonShown(const RowButton& button, bool overRow) const;
    // The network cable (on the Wi-Fi view, above the networks).
    struct WiredState {
        bool present = false;
        bool connected = false;
        ULONG64 speed = 0;
    } m_wired;
    D2D1_RECT_F m_wiredRow{};
    // The radios: on (1), off (0), kRadioUnknown or kRadioMissing.
    int m_wifiRadio = kRadioUnknown;
    int m_bluetoothRadio = kRadioUnknown;
    std::vector<BluetoothDevice> m_bluetoothDevices;
    // A device connecting or disconnecting from its row (one at a time),
    // and since when (its failure: since it failed).
    BluetoothStep m_bluetoothStep = BluetoothStep::None;
    ULONGLONG m_bluetoothAddress = 0;
    ULONGLONG m_bluetoothStepAt = 0;
    // The radio and brightness threads (see StartWorker).
    std::vector<HANDLE> m_workers;
    // The mobile hotspot and airplane mode: like a radio.
    int m_hotspot = kRadioUnknown;
    int m_airplane = kRadioUnknown;
    // One radio request runs at a time (see SwitchRadio): the state wanted
    // for each kind meanwhile (Wi-Fi, Bluetooth, hotspot, airplane mode), or
    // -1.
    bool m_radioBusy = false;
    // (A request whose answer never came, the panel's window gone, doesn't
    // block the next ones for long.)
    ULONGLONG m_radioBusySince = 0;
    int m_radioWanted[4] = {-1, -1, -1, -1};
    // Night light: 1 on, 0 off, -1 unavailable. Windows writes its state
    // back a moment after a change, which can undo a quick second one: it's
    // checked after a moment, and set again if it isn't what was asked last
    // (see OnTimer).
    int m_nightLight = -1;
    bool m_nightLightChecking = false;
    ULONGLONG m_nightLightCheckSince = 0;
    int m_nightLightTries = 0;
    void SwitchNightLight();
    // A control's own view (see ShowControl); the main one for the others.
    void OpenControlView(ControlId id);
    int m_openOnControl = -1;
    bool OnTimer(UINT_PTR id) override;
    // The monitors' brightness, 0 to 100 (the first one's), or -1 (not
    // known or not adjustable); each monitor's, and the row dragged.
    int m_brightness = -1;
    std::vector<BrightnessMonitor> m_brightnessMonitors;
    int m_brightnessRow = 0;
    // What's left of a wheel step (see BrightnessWheelSteps).
    float m_brightnessWheel = 0;
    std::shared_ptr<BrightnessJob> m_brightnessJob =
        std::make_shared<BrightnessJob>();
    ULONGLONG m_brightnessReadAt = 0;
    // g_displayChanges when the list was read (its places are old after one).
    int m_brightnessDisplays = 0;
    // The display mode (DISPLAYCONFIG_TOPOLOGY_*).
    int m_displayMode = 0;
    std::vector<AccessibilityItem> m_accessibility;

    // Layout.
    D2D1_RECT_F m_sound{}, m_volumeIcon{}, m_slider{}, m_deviceRow{},
        m_mixerButton{}, m_batteryTile{}, m_more{}, m_settingsButton{},
        m_back{}, m_toggle{},
        m_networkIcon{}, m_bluetoothIcon{}, m_field{}, m_eye{}, m_connect{},
        m_status{}, m_display{},
        m_hotspotRow{}, m_airplaneRow{}, m_dndIcon{}, m_hotspotIcon{},
        m_hotspotField{};
    // The brightness's sliders: one, or one per monitor.
    std::vector<D2D1_RECT_F> m_brightnessSliders;
    // Where each control shown is.
    std::vector<std::pair<ControlId, D2D1_RECT_F>> m_controlRects;
    std::vector<ControlId> m_controls;
    bool m_controlsLoaded = false;
    D2D1_RECT_F m_edit{}, m_done{}, m_addHeader{};
    // Opens on the island's settings (see ShowSettings).
    bool m_openOnSettings = false;
    // Editing in place, like the iPhone's control center: the controls
    // each have a "-"; they're dragged to another place, and the
    // others (listed below) are added with a click. m_editAmount (0 to 1)
    // fades the "-" in and out.
    bool m_editing = false;
    double m_editAmount = 0;
    double m_editAmountVelocity = 0;
    std::vector<std::pair<ControlId, D2D1_RECT_F>> m_addRects;
    // Where each control is shown: it follows its place with a spring, so
    // the others slide out of the way, and an added one grows in.
    struct Motion {
        ControlId id;
        double x;
        double y;
        double vx = 0;
        double vy = 0;
        double scale = 1;
        double vscale = 0;
    };
    std::vector<Motion> m_motions;
    Motion* FindMotion(ControlId id);
    // A removed control shrinking away.
    struct Ghost {
        ControlId id;
        D2D1_RECT_F rect;
        double amount = 1;
        double velocity = 0;
    };
    std::vector<Ghost> m_ghosts;
    // A control being dragged: where the mouse took it, and where it is.
    int m_dragId = -1;
    bool m_dragMoved = false;
    POINT m_dragStart{};
    POINT m_dragPoint{};
    D2D1_POINT_2F m_dragGrab{};
    winrt::com_ptr<IDWriteTextFormat> m_badgeFormat;
    // The island's settings view.
    D2D1_RECT_F m_styleRow{}, m_themeRow{};
    // The place for all apps (a little screen with its 9 ready places), and
    // the places per app.
    D2D1_RECT_F m_placeRow{}, m_placeGrid{}, m_appPlacesRow{};
    // The sliders (see kIslandSliders): each one's row, slider and reset
    // button.
    D2D1_RECT_F m_sizeRow[kIslandSliderCount]{},
        m_sizeSlider[kIslandSliderCount]{}, m_sizeReset[kIslandSliderCount]{};
    // The rows with a few choices (see ChoiceId): their segments, and the
    // lit one sliding to the chosen one.
    D2D1_RECT_F m_choiceRow[kChoiceCount]{};
    std::vector<D2D1_RECT_F> m_choiceSegments[kChoiceCount];
    // (-1: not drawn yet, it starts where the choice is.)
    std::array<double, kChoiceCount> m_choiceKnob = [] {
        std::array<double, kChoiceCount> knobs;
        knobs.fill(-1);
        return knobs;
    }();
    double m_choiceKnobVelocity[kChoiceCount]{};
    // Exporting or importing a backup: the file dialog's thread, and how the
    // last one went (shown on its row).
    HANDLE m_backupThread = nullptr;
    std::wstring m_backupStatus;
    void StartBackupDialog(bool save);
    // Choosing the icon pack's folder: the folder dialog's thread, and how
    // saving Windows' icons into it went (shown on its row).
    HANDLE m_iconThread = nullptr;
    std::wstring m_iconStatus;
    void StartIconFolderDialog(bool save);
    // A picture chosen for one icon (see SetIconFile).
    void StartIconPictureDialog(WCHAR glyph);
    void SaveWindowsIconsInto(const std::wstring& folder);
    // Choosing a custom color (see CustomColor): Windows' color dialog's
    // thread.
    HANDLE m_colorThread = nullptr;
    void StartColorDialog(int color);
    void SetIslandSizeAt(int slider, POINT pt, bool keep);
    // The island's settings: each row (an option, or a section's title) is
    // an entry, laid out in two columns (one on a narrow screen) with a line
    // between them, and filtered by what's typed (the search box). The list
    // scrolls when the screen is too short for it, and its entries glide to
    // their places and fade in and out as they're filtered.
    enum class SettingKind {
        Label,
        Style,
        Theme,
        Size,
        Place,
        AppPlaces,
        Order,
        Pref,
        // A row with a few choices (its ChoiceId).
        Choice,
        // The profiles (see m_profileChips).
        Profiles,
        Monitors,
        // The apps that look for song lyrics, each on or off (see
        // m_lyricsAppChips).
        LyricsApps,
        // The minimize shortcut's keys (see m_shortcutChip).
        Shortcut,
        // The pages about the mod: the dock's options (in Windhawk), about
        // (the version, the author's pages), and the logs (copied or saved;
        // 1: the text itself); what's new (each version's notes, its
        // kReleaseNotes index).
        Dock,
        About,
        Logs,
        WhatsNew,
        // An IconGroup's icons, each one as it's drawn now (see
        // m_iconCells).
        Icons,
        // The setup assistant's text for its step (see SetupLines).
        Setup
    };
    struct SettingEntry {
        SettingKind kind;
        // The IslandPref, for an option.
        int value;
        int section;
        // Kept in the column of the option before (see SettingSpec).
        bool follows = false;
        // A selector's or a slider's: its name's height over it (see
        // SettingNameHeight); 0 for a name beside its control.
        float head = 0;
        // In the list (0 at its top).
        D2D1_RECT_F rect{};
        bool shown = true;
        // Where it's drawn (following rect with a spring), and its opacity.
        double x = 0;
        double y = 0;
        double vx = 0;
        double vy = 0;
        double alpha = 0;
        double valpha = 0;
    };
    std::vector<SettingEntry> m_settings;
    std::wstring m_search;
    TextEdit m_searchEdit;
    SmoothScroll m_settingsScroll;
    D2D1_RECT_F m_searchField{}, m_searchClear{}, m_settingsViewport{};
    // The settings in tabs, one for each section: the one shown, and where
    // each is. While searching, every section shows (its matches).
    int m_settingsTab = kTabGeneral;
    // Another tab's options fading in, all together (see BeginScrollArea).
    double m_tabFade = 1;
    double m_tabFadeVelocity = 0;
    std::vector<D2D1_RECT_F> m_tabs;
    // The "+" beside the profiles' title (a new profile).
    D2D1_RECT_F m_profileAdd{};
    // "Nothing found", fading in and out.
    bool m_nothingFound = false;
    double m_nothingAlpha = 0;
    double m_nothingAlphaVelocity = 0;
    void LayoutSettings();
    // Puts the entries where they're laid out, without moving them there.
    void SettleSettings();
    bool AnimateSettings(double dt);
    std::wstring SettingText(const SettingEntry& entry) const;
    // In pixels, for a column `width` wide (the names wrap, see
    // SettingName).
    float SettingHeight(const SettingEntry& entry, float width) const;
    // A choice's segments as they fit a row's width (see FitChoice), and its
    // control's height.
    struct ChoiceFit {
        float segment;
        int across;
        int rows;
    };
    ChoiceFit FitChoice(int id, float width) const;
    float ChoiceControlHeight(int id, float width) const;
    float ChoiceWidest(int id) const;
    void PlaceChoicePictures(int id, const D2D1_RECT_F& rect, float top,
                             float bottom);
    void DrawChoicePictures(ID2D1RenderTarget* target,
                            ID2D1SolidColorBrush* brush,
                            int id,
                            float opacity);
    // Each choice's longest name, measured once per scale and language.
    mutable std::vector<float> m_choiceWidest;
    mutable float m_choiceWidestScale = 0;
    mutable int m_choiceWidestLanguage = -1;
    float SettingNameHeight(const SettingEntry& entry, float width) const;
    std::wstring SettingName(const SettingEntry& entry) const;
    // The profiles' line of help (see DrawProfiles), by case.
    static PCWSTR ProfilesHint(int hint);
    static constexpr int kProfilesHints = 6;
    // A text on as many lines as it needs, in the middle of `rect` (see
    // WrappedHeight); in the settings, instead of scrolling.
    void DrawWrapped(ID2D1RenderTarget* target,
                     ID2D1SolidColorBrush* brush,
                     const std::wstring& text,
                     IDWriteTextFormat* format,
                     const D2D1_RECT_F& rect,
                     bool centered = false);
    float WrappedHeight(const std::wstring& text,
                        IDWriteTextFormat* format,
                        float width) const;
    void DrawSettingEntry(ID2D1RenderTarget* target,
                          ID2D1SolidColorBrush* brush,
                          const SettingEntry& entry);
    void DrawSearchField(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush);
    void OnSearchChanged();
    // A point of the panel in the list's coordinates.
    // (See Part::DisabledSetting.)
    static constexpr int kDisabledChoice = 10000;
    static constexpr int kDisabledAppPlaces = 20000;
    static constexpr int kDisabledSlider = 30000;
    // Why an option is dimmed (see PrefDisabled and ChoiceDisabled).
    static std::wstring DisabledReason(int index);
    // What a dimmed option waits for (to go to it from its reason, see
    // GoToSetting), its index for DisabledReason (-1: not dimmed), its
    // reason as written under it, and whether a click on it does something.
    struct SettingTarget {
        SettingKind kind;
        int value;
    };
    static SettingTarget DisabledTarget(int index);
    static int DisabledIndex(const SettingEntry& entry);
    static std::wstring SettingReason(const SettingEntry& entry);
    static bool ReasonLinks(int index);
    std::string DisabledKey() const;
    void GoToSetting(const SettingTarget& target);
    // The reasons' places (the entry's index, in the list), as last drawn;
    // the dimmed options' state they were laid out for; and the card shown
    // last by GoToSetting, glowing since when.
    std::vector<std::pair<int, D2D1_RECT_F>> m_reasonLinks;
    // What a dimmed option needs (see NeedsFor): each change, shown in a
    // bubble under it ("Yes, change" makes them all), and the ones made,
    // kept to be put back ("Restore what changed" on its card).
    struct NeedChange {
        IslandPref pref;
        int from;
        int to;
    };
    bool NeedOf(const SettingEntry& entry, int disabled, NeedChange* change);
    std::vector<NeedChange> NeedsFor(int index, bool* complete);
    void OpenNeeds(int index);
    void ApplyNeeds();
    void RestoreNeeds();
    std::wstring NeedText(const NeedChange& change) const;
    void DrawNeeds(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    // A pill-shaped button with its text, lit under the mouse (`main`: in
    // the accent color).
    void DrawButton(ID2D1RenderTarget* target,
                    ID2D1SolidColorBrush* brush,
                    const D2D1_RECT_F& rect,
                    PCWSTR text,
                    bool main,
                    bool hover);
    int m_needsEntry = -1;
    bool m_needsComplete = false;
    std::vector<NeedChange> m_needs;
    D2D1_RECT_F m_needsRect{}, m_needsYes{}, m_needsNo{};
    int m_restoreEntry = -1;
    std::vector<NeedChange> m_restore;
    D2D1_RECT_F m_restoreLink{};
    // A version's notes (see kReleaseNotes) in the language chosen on the
    // page, as lines: whether it's a heading, and the option an item tells
    // about (Label without one; a click on the item shows it). The items'
    // places with their options, as last drawn.
    struct NoteLine {
        std::wstring text;
        bool heading;
        SettingTarget link;
    };
    static std::vector<NoteLine> NotesLines(int index);
    static SettingTarget NoteTarget(std::wstring_view key);
    std::vector<std::pair<SettingTarget, D2D1_RECT_F>> m_noteLinks;
    // The setup assistant (its tab, see SetupState): what a step shows, its
    // text (on the last one, what was chosen, each line a link to its
    // option), a step shown, a set of the pill's items chosen, a button
    // pressed (the buttons under its page), and its end.
    SetupState m_setup;
    D2D1_RECT_F m_setupButtons[kSetupButtons]{};
    static bool SetupShows(int step, const SettingEntry& entry);
    std::vector<NoteLine> SetupLines() const;
    void ShowSetupStep(int step);
    void ChooseSetupItems(int set);
    void PressSetupButton(int button);
    void EndSetup(bool cancel);
    std::string m_disabledKey;
    int m_flashEntry = -1;
    double m_flashAt = 0;
    POINT ToSettingsList(POINT pt) const {
        return {pt.x, (LONG)std::lround(pt.y - m_settingsViewport.top +
                                        m_settingsScroll.Offset())};
    }
    // The order of the pill's items: a little pill with an icon per group,
    // dragged sideways to put them in another order (the island follows as
    // they move); a click shows or hides one.
    // The same for each app with a place of its own (its strip: its index
    // in g_appPlaces; -1 the one in the settings, for all apps).
    D2D1_RECT_F m_orderStrip{};
    std::vector<D2D1_RECT_F> m_appOrderStrips;
    struct OrderChip {
        PillGroup group;
        D2D1_RECT_F rect;
        double x;
        double vx = 0;
        int strip = -1;
    };
    std::vector<OrderChip> m_orderChips;
    OrderChip* FindOrderChip(PillGroup group, int strip = -1);
    void LayoutOrderChips(int strip = -1);
    // The chip dragged in the notches' strip, its middle there: into the
    // notch under it, among its items.
    void DragNotchChip(float middle);
    // Chips side by side in `rect`, in that order, centered (narrower when
    // they don't all fit).
    void PlaceOrderChips(const std::vector<PillGroup>& groups,
                         const D2D1_RECT_F& rect,
                         int strip);
    // With notches (see islandNotches), the settings' strip is a part for
    // each (which one, where), their items in them, dragged from one to
    // another.
    std::vector<std::pair<int, D2D1_RECT_F>> m_notchParts;
    void DrawOrderStrip(ID2D1RenderTarget* target,
                        ID2D1SolidColorBrush* brush,
                        const D2D1_RECT_F& row,
                        int strip = -1);
    // A strip's rect, order and items shown; its order set (and the island
    // following); a point in its coordinates (the settings scroll).
    const D2D1_RECT_F& StripRect(int strip) const {
        return strip < 0 || strip >= (int)m_appOrderStrips.size()
                   ? m_orderStrip
                   : m_appOrderStrips[strip];
    }
    std::vector<PillGroup> StripOrder(int strip) const;
    bool StripShows(int strip, PillGroup group) const;
    void SetStripOrder(int strip, const std::vector<PillGroup>& order);
    POINT StripPoint(int strip, POINT pt) const {
        return strip < 0 ? pt : ToAppPlacesList(pt);
    }
    // An order chip's hit index: its group, and its strip.
    static constexpr int kOrderStripStride = 64;
    static_assert(kPillGroups <= kOrderStripStride);
    static int OrderChipIndex(int strip, PillGroup group) {
        return (strip + 1) * kOrderStripStride + (int)group;
    }
    // Shows or hides one of an app's groups (its own choice from then on).
    void ToggleAppGroup(size_t entry, int group);
    // An app's room from the edges (0) or size (1), from a point on its
    // slider (see m_appItemChips), kept when `keep`.
    void SetAppSliderAt(int entry, int which, POINT pt, bool keep);
    void DrawSliderTrack(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush,
                         const D2D1_RECT_F& slider,
                         int min,
                         int max,
                         int usual,
                         int value,
                         bool active);
    // The profiles' chips (a new one comes from the "+" beside the title),
    // each profile's pencil, and the one being edited (its id, or -1) with
    // its name being typed, its buttons, and when it was last saved.
    std::vector<D2D1_RECT_F> m_profileChips;
    std::vector<D2D1_RECT_F> m_profilePencils;
    D2D1_RECT_F m_profileNameField{}, m_profileUpdate{}, m_profileDelete{},
        m_profileDone{};
    int m_profileEdit = -1;
    std::wstring m_profileName;
    TextEdit m_profileNameEdit;
    bool m_profileTyping = false;
    // "Delete" pressed once: it asks, and deletes when pressed again.
    bool m_profileDeleteArmed = false;
    // What turns it on by itself (see Part::ProfileStyleChip; only for the
    // main island's settings): its apps and the one to add.
    std::vector<D2D1_RECT_F> m_profileStyleChips;
    std::vector<D2D1_RECT_F> m_profileAppChips;
    std::vector<std::wstring> m_profileApps;
    std::wstring m_profileAddApp;
    D2D1_RECT_F m_profileAppAdd{}, m_profileKeep[2]{};
    // How many lines of apps it has (with the one to add).
    int ProfileAppRows() const;
    double m_profileSavedAt = -10;
    void StartProfileEdit(int id, bool fresh);
    void EndProfileEdit(bool keep);
    void DrawProfiles(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush,
                      const D2D1_RECT_F& row);
    // The Monitors section: its buttons and screens (what each chooses: a
    // mode, a screen's index, a profile's id; for which monitor; its name;
    // chosen or dimmed), and its lines of text.
    struct MonitorPart {
        D2D1_RECT_F rect;
        Part part;
        int value;
        std::wstring device;
        std::wstring text;
        bool chosen;
        bool disabled;
        // (A screen's: which island it has, under its number.)
        std::wstring label = {};
    };
    std::vector<MonitorPart> m_monitorParts;
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> m_monitorLabels;
    // The cards behind each monitor's options, and the section titles.
    std::vector<D2D1_RECT_F> m_monitorCards;
    std::vector<std::pair<D2D1_RECT_F, std::wstring>> m_monitorTitles;
    void LayoutMonitors(const D2D1_RECT_F& row);
    void DrawMonitors(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    int m_orderDragStrip = -1;
    int m_orderDrag = -1;
    bool m_orderMoved = false;
    int m_orderStartX = 0;
    float m_orderGrab = 0;
    float m_orderDragLeft = 0;
    // Each app's icons to show or hide (index: entry * kAppChipStride +
    // PillGroup).
    std::vector<std::pair<int, D2D1_RECT_F>> m_appItemChips;
    std::vector<D2D1_RECT_F> m_appPlaceRows;
    std::vector<D2D1_RECT_F> m_appPlaceGrids;
    std::vector<D2D1_RECT_F> m_appPlaceRemoves;
    D2D1_RECT_F m_addAppsHeader{};
    // The list under the header scrolls when it's taller than the screen
    // allows; a point of the panel in its coordinates.
    SmoothScroll m_appPlacesScroll;
    D2D1_RECT_F m_appPlacesViewport{};
    POINT ToAppPlacesList(POINT pt) const {
        return {pt.x, (LONG)std::lround(pt.y + m_appPlacesScroll.Offset())};
    }
    std::vector<std::wstring> m_openApps;
    std::vector<D2D1_RECT_F> m_openAppChips;
    // The apps' cards under "Apps that look for lyrics" (see LyricsApps),
    // and their icons (made in PrepareLyricsAppIcons, outside Draw).
    std::vector<D2D1_RECT_F> m_lyricsAppChips;
    // The Icons tab's cells (see SettingKind::Icons): an icon's (its kIcons
    // index), and its "x" when it has a picture of its own.
    struct IconCell {
        int icon;
        D2D1_RECT_F rect;
        D2D1_RECT_F reset;
        D2D1_RECT_F paint;
    };
    std::vector<IconCell> m_iconCells;
    // The icon packs' list: each one's bin (empty: none, not downloaded or
    // downloading) and whether it's on the disk (looked at as it's laid
    // out).
    std::array<D2D1_RECT_F, kIconPacks> m_iconPackDelete{};
    std::array<bool, kIconPacks> m_iconPackOnDisk{};
    // (Looked at again once g_iconDiskChanges changes.)
    UINT m_iconPackDiskSeen = UINT_MAX;
    std::vector<KeptAppIcon> m_lyricsAppIcons;
    void PrepareLyricsAppIcons();
    ID2D1Bitmap* LyricsAppBitmap(const std::wstring& appId) const;
    void ListOpenApps();
    // The ready place under a point of a grid (0-8), or -1.
    static int PlaceDotAt(const D2D1_RECT_F& grid, POINT pt);
    void DrawPlaceGrid(ID2D1RenderTarget* target,
                       ID2D1SolidColorBrush* brush,
                       const D2D1_RECT_F& grid,
                       double x,
                       double y,
                       int hover,
                       float opacity);
    void DrawAppPlaces(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    D2D1_RECT_F m_styleSegments[3]{};
    D2D1_RECT_F m_swatches[(int)ThemeId::Count]{};
    double m_styleKnob = -1;
    double m_styleKnobVelocity = 0;
    // The switches' knobs, by key.
    struct Knob {
        int key;
        double value;
        double velocity;
        double target;
    };
    std::vector<Knob> m_knobs;
    // Whether a text scrolls now (under the mouse, see ScrollsHere).
    bool m_marqueeActive = false;
    // Their layouts, made once instead of on every frame.
    struct MarqueeLayout {
        std::wstring text;
        IDWriteTextFormat* format;
        float fontSize;
        float height;
        winrt::com_ptr<IDWriteTextLayout> layout;
        float width;
    };
    std::vector<MarqueeLayout> m_marqueeLayouts;
    struct WrappedLayout {
        std::wstring text;
        IDWriteTextFormat* format;
        float fontSize;
        float width;
        float height;
        bool centered;
        winrt::com_ptr<IDWriteTextLayout> layout;
    };
    std::vector<WrappedLayout> m_wrappedLayouts;
    // The heights measured (see WrappedHeight), by text, format and width;
    // forgotten with the formats (see PrepareFormats).
    mutable std::unordered_map<std::wstring, float> m_wrappedHeights;
    bool m_darkMode = false;
    std::vector<D2D1_RECT_F> m_rows;
    std::vector<D2D1_RECT_F> m_rowIcons;
    std::vector<D2D1_RECT_F> m_rowSliders;
    Hit m_hover;
    // Dragging the main slider (index -1), an app's (its index), or the
    // brightness.
    static constexpr int kDragBrightness = -2;
    // The island's sliders: kDragIslandSize minus the slider; an app's:
    // kDragAppSlider minus its entry * 2 and which (see SetAppSliderAt).
    static constexpr int kDragIslandSize = -3;
    static constexpr int kDragAppSlider = -100;
    bool m_dragging = false;
    int m_dragIndex = -1;
    // The minimize shortcut's row: its keys' chip (a click records new
    // ones), its "restore" (Win+J), recording now, and what to tell about
    // the keys last recorded (taken, refused).
    D2D1_RECT_F m_shortcutChip{}, m_shortcutReset{};
    bool m_recordingShortcut = false;
    std::wstring m_shortcutNote;
    void StartShortcutRecording(bool start);
    void OnShortcutRecorded(int combo);
    static bool ShortcutDisabled() {
        return OtherIslandShown() || !GetIslandPref(IslandPref::MinimizeKey);
    }
    static constexpr int kDisabledShortcut = 40000;
    // A reason's hit: its entry's index times this, plus its DisabledReason
    // index.
    static constexpr int kDisabledStride = 65536;
    // The room of the small arrow at the end of a "What's new" item with
    // an option (DIPs; the whole item is its button).
    static constexpr float kNoteArrow = 28;
    // The room before a "What's new" line (DIPs): more before a heading
    // (not the first), the same between all items (a thin line in its
    // middle).
    static float NoteGap(bool heading, bool first) {
        return heading ? (first ? 6.0f : 18.0f) : first ? 6.0f : 12.0f;
    }
    static constexpr double kFlashSeconds = 1.6;
    // The pages about the mod's buttons (their PageButton, kPageLink plus
    // an About row for a link), what copying or saving the logs said, and
    // the logs shown (as the page is laid out).
    enum PageButton {
        kPageWindhawk,
        kPageCopyLogs,
        kPageSaveLogs,
        kPageLink = 10
    };
    std::vector<std::pair<int, D2D1_RECT_F>> m_pageButtons;
    std::wstring m_logsStatus;
    std::wstring m_logsShown;
    void StartLogsDialog();
    // The settings' window (see LayoutSettingsWindow): its side bar (only
    // its icons when narrow), the page's title, the room around each row in
    // its card, and the panel's button opening the window.
    bool m_navNarrow = false;
    std::vector<int> m_tabIds;
    SmoothScroll m_navScroll;
    D2D1_RECT_F m_navViewport{};
    // The settings' scroll bars (1 the page's, 2 the side bar's): the one
    // under the mouse, and the one dragged, from where on its thumb. The
    // side bar's takes the mouse only past its tabs' right end (DIPs).
    static constexpr float kNavBarStrip = 4;
    int m_barHover = 0;
    int m_barDrag = 0;
    float m_barGrab = 0;
    void DragScrollBar(POINT pt);
    // Small enough to share a line with another in a wide window (see
    // LayoutSettingsWindow): 1 a switch or a shortcut (beside its name), 2 a
    // slider or a choice whose buttons fit one line (under it), 0 not.
    int LineShare(const SettingEntry& entry, float width) const;
    D2D1_RECT_F m_pageTitle{};
    float m_cardPad = 0;
    // The island's preview at the page's top (the order strip, see
    // m_orderStrip), and whether this page has it.
    D2D1_RECT_F m_previewRow{};
    bool m_previewShown = false;
    winrt::com_ptr<IDWriteTextFormat> m_pageTitleFormat;
    void LayoutSettingsWindow();
    // An entry's row at `rect` (a column `column` wide), and the parts it
    // has (a selector, a slider, chips...).
    void PlaceSettingParts(SettingEntry& entry,
                           const D2D1_RECT_F& rect,
                           float column);
    void DrawSettingsNav(ID2D1RenderTarget* target,
                         ID2D1SolidColorBrush* brush);
    // What an option does (empty: nothing to say, or not in the window),
    // and how tall it is under its name.
    std::wstring SettingDescription(const SettingEntry& entry) const;
    float DescriptionHeight(const SettingEntry& entry, float width) const;
    // A row's card in the window (the row itself in the panel).
    D2D1_RECT_F CardOf(const D2D1_RECT_F& row) const {
        return {row.left - m_cardPad, row.top - m_cardPad,
                row.right + m_cardPad, row.bottom + m_cardPad};
    }
};

////////////////////////////////////////////////////////////////////////////////
// The notifications, like the macOS notification center: the latest ones
// Windows showed, newest first, read (read-only) from its notification
// database, as cards with the app's icon and name, the time, the title and the
// text. Clicking one opens the app. The cards take up to half of the screen's
// height and scroll smoothly inside it. Clearing them here only hides them
// here, since the database isn't written to (the Windows notification center
// is a link at the bottom).

