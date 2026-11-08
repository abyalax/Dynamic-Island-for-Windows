class Island final : public DropTarget::Receiver {
   public:
    bool Create();
    void Destroy();
    // A Windows banner it moved off screen back, and its banner hooks gone
    // (first thing as it goes; the main island's before the others go).
    void ReleaseWindowsBanner();
    void Tick();
    void CheckShown();
    // An app's lyrics turned on or off (see LyricsApps): every island's
    // lyrics again, and their settings shown.
    void LyricsAppsChanged();
    bool Animating() const { return m_animating; }
    void AnimationFrame();
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    // Files dragged over it (out of Explorer, the desktop...): held on the
    // folders' item a moment, its panel opens, to drop them in a folder
    // there. Nothing is dropped on the island itself.
    DWORD DragOverAt(IDataObject* data,
                     POINT screen,
                     DWORD keys,
                     DWORD allowed) override;
    void DragGone() override;
    void HideShelfBubble();
    DWORD DropAt(IDataObject* data,
                 POINT screen,
                 DWORD keys,
                 DWORD allowed) override;
    void StartAnimating();
    // The drawing device was lost (see OnDrawingDeviceLost).
    void OnDeviceLost();

    // Shared with the control center.
    IMMDeviceEnumerator* Devices() { return m_devices.get(); }
    winrt::com_ptr<IAudioEndpointVolume> GetVolume(std::wstring* deviceName);
    // The default microphones' volume (for everything, and for
    // communications when it's another), or nothing without one.
    std::vector<winrt::com_ptr<IAudioEndpointVolume>> GetMicrophones();
    // -1 without a microphone.
    int MicrophoneMuted();
    void ToggleMicrophone();
    NetworkInfo QueryNetwork();
    void OnVolumeChanged();
    // The screen's brightness for its item (see BrightnessJob): -1 until
    // it's read, or without a monitor that tells it. Read on a thread (the
    // monitors answer slowly) now and then while the item is on, set on
    // another one as the wheel turns.
    int m_brightness = -1;
    // What's left of a wheel step (see BrightnessWheelSteps).
    float m_brightnessWheel = 0;
    double m_brightnessReadAt = 0;
    std::shared_ptr<BrightnessJob> m_brightnessJob =
        std::make_shared<BrightnessJob>();
    std::vector<HANDLE> m_brightnessThreads;
    void ReadBrightnessSoon();
    void SetBrightness(int level);
    void StartBrightnessThread(LPTHREAD_START_ROUTINE proc);
    void UpdateVolumePercent();
    void UpdateBatteryPercent();
    // A text beside an item (the volume's and the battery's percentage),
    // measured once for each text and size.
    struct SideText {
        std::wstring text;
        std::wstring measured;
        float width = 0;
        float scale = 0;
    };
    SideText m_volumeText;
    SideText m_batteryText;
    // The items added in 2.1 (see ReadWorldClocks, NextTimedEvent and the
    // Recycle Bin's count): the clocks and the next event looked at again
    // each minute (or after their options change, see m_extrasKey).
    SideText m_recycleText;
    // (Whether its item shows, see RecycleBinShown: laid out again when it
    // comes or goes.)
    bool m_recycleShown = false;
    std::vector<WorldClock> m_worldClocks;
    std::wstring m_countdownTitle;
    std::wstring m_countdownText;
    int m_countdownDay[3] = {};
    // What it counts down to (a click opens it): an event (on its day), an
    // alarm, the timer, Pomodoro.
    static constexpr int kCountdownEvent = 0;
    static constexpr int kCountdownAlarm = 1;
    static constexpr int kCountdownTimer = 2;
    static constexpr int kCountdownPomodoro = 3;
    int m_countdownKind = kCountdownEvent;
    long long m_extrasKey = -1;
    void UpdateExtraItems();
    // The main island's: the network's speed measured once a second while
    // one shows it, the Recycle Bin watched (Windows tells its changes) and
    // counted on a thread.
    bool m_netSpeedWatched = false;
    void UpdateNetSpeedWatch();
    void OnNetSpeedTimer();
    ULONG m_recycleNotify = 0;
    HANDLE m_recycleThread = nullptr;
    bool m_recycleAgain = false;
    void UpdateRecycleWatch();
    void StartRecycleCount();
    void OnRecycleCounted(WPARAM items, LPARAM kilobytes);
    // The main island's: the AI agents' files watched and read while one
    // shows them (see AiUsageThreadProc), Codex's sessions among them; each
    // agent's app whose icon stands for it, and its use's text measured.
    void UpdateAiUsageWatch();
    void StopAiUsageWatch();
    void OnAiUsageRead(const AiUsageRead& read);
    void OnAiUsageMinute();
    HANDLE m_aiUsageThread = nullptr;
    HANDLE m_aiUsageStop = nullptr;
    bool m_aiCodexWatched = false;
    std::wstring AiAgentApp(int agent);
    SideText m_aiTexts[kAiAgents];
    // Status notices (see ShowStatusNotice): what was seen last (-1 not
    // yet: the first look only notes it), the lowest battery level told
    // since it charged, and the Bluetooth audio outputs on.
    void ShowStatusNotice(WCHAR glyph, int color, const std::wstring& title,
                          const std::wstring& text);
    void CheckPowerNotices();
    void NoteQuiet(bool quiet);
    void CheckBluetoothOutputs();
    int m_powerSeen = -1;
    int m_lowBatteryTold = 101;
    int m_quietSeen = -1;
    bool m_bluetoothKnown = false;
    std::vector<std::wstring> m_bluetoothOutputs;
    // Two short lines' width (see kStackedFont), kept for the texts seen.
    float StackedWidth(const std::wstring& label,
                       const std::wstring& value,
                       const std::wstring& widest = {});
    std::vector<std::pair<std::wstring, float>> m_stackedWidths;
    float m_stackedScale = 0;
    // The middle of an item on the pill (along it; the one with that key, see
    // SlotKey), or -1.
    float SlotMiddle(IslandItem item, const std::wstring& key = {}) const {
        for (const auto& slot : m_slots) {
            if (slot.item == item && (key.empty() || SlotKey(slot) == key)) {
                return (slot.rect.left + slot.rect.right) / 2;
            }
        }
        return -1;
    }
    void OnNetworkChanged();
    void OnDoNotDisturbChanged();
    void OpenNotifications();
    // Win+N (the notifications) or Win+A (the control center).
    void OpenFromShortcut(bool control);
    // The apps with unread notifications grouped again (see
    // islandGroupApps).
    void RegroupUnread();
    // A notch of the Fixed style (see islandNotches; one of `count`): 0 on
    // the left, 1 in the middle, 2 on the right, made with `base` (the one
    // with the bell, which shows the notifications and keeps the strip), or
    // that one itself (no base).
    void SetNotch(int notch, Island* base, int count) {
        m_notch = notch;
        m_notchBase = base;
        m_notchCount = count;
    }
    void AddNotchSibling(Island* island) { m_notchSiblings.push_back(island); }
    // The app in front's color, read (once) when any island follows it (see
    // islandWindowColor), each one in its own look.
    void ReadWindowColor();
    // Notches hiding together again: each one as the bell's.
    void MatchNotchBase() {
        if (m_notchBase && m_minimized != m_notchBase->m_minimized) {
            SetMinimized(m_notchBase->m_minimized);
        }
    }
    // All its notches made: the strip they keep, for all of them.
    void OnNotchesMade() { SetAppBarPosition(true); }
    int Notch() const { return m_notch; }
    int NotchCount() const { return m_notchCount; }
    // Whether its line (minimized) opens the island: every one's, or only the
    // notch chosen (when there's one like that).
    bool LineOpens() const {
        const int only = g_settings.islandNotchOpen - 1;
        if (only < 0 || m_notch < 0) {
            return true;
        }
        const std::vector<int> places = NotchPlaces(m_notchCount);
        return std::find(places.begin(), places.end(), only) == places.end() ||
               m_notch == only;
    }
    bool IsNotchSibling() const { return m_notchBase != nullptr; }
    // Shows the notifications (their banners, the peek out of the line,
    // what's copied): not the notches without the bell.
    bool ShowsNotices() const { return !m_notchBase; }
    // Shows the new notifications' banners (see g_notifyScreens).
    bool ShowsNotifications() const;
    // A group shown here (on a notch: one of its own).
    bool InThisNotch(PillGroup group) const {
        return m_notch < 0 || NotchOfGroup(group, m_notchCount) == m_notch;
    }
    // The bell went to another notch: the one with it shows the
    // notifications now (the islands start again).
    bool NotchBaseMoved() const {
        return m_notch >= 0 && !m_notchBase &&
               NotchOfGroup(PillGroup::Bell, m_notchCount) != m_notch;
    }
    void UpdateShortcutKeys();
    void OnPreferencesChanged();
    // Wi-Fi joining, or Windows checking for the internet just after
    // connecting (for the control center).
    bool IsNetworkConnecting() const {
        return m_netState == NetState::Connecting;
    }
    // The monitors' brightness list read again, in its control center (see
    // ControlPanel::ReloadBrightness).
    void ReloadBrightness() { m_control.ReloadBrightness(); }
    // Another order of the items (see SetPillOrder): they slide there.
    void OnOrderChanged();
    // The island's settings, in a window of their own like Windows'
    // Settings (the gear under the island, the control center's).
    // (`restore`: as it was before the islands started again.)
    void OpenSettingsWindow(const SettingsWindowState* restore = nullptr);
    // A profile was applied: its order, places, monitor and the control
    // center's controls shown.
    void OnProfileApplied();
    // Apps with notifications not looked at yet (see m_unreadApps): seen on
    // one island, seen on all of them (notches and other monitors too).
    static void MarkAppRead(const std::wstring& appId);
    // Notifications cleared in the list: their apps (or all, for "Clear
    // all") leave the island, the ones with a taskbar badge too (see
    // m_clearedBadges).
    void ClearAppsFromList(const std::vector<std::wstring>& apps, bool all);
    // (MarkAppRead on this island.)
    void MarkAppReadHere(const std::wstring& appId);
    // The notification list was opened: the bell's dot goes.
    void OnNotificationsSeen();
    void CatchUpToasts();
    // Once per version, a moment after the main island starts: a notice
    // from the pill with its button to What's new (islandWhatsNewNotice);
    // the version is kept as told then (for each account: see
    // UserValueName). On a first install, the setup assistant instead (see
    // OpenSetup).
    void StartWhatsNewNotice() {
        if (g_firstInstall ||
            (g_settings.islandWhatsNewNotice &&
             GetUserStringValue(L"whatsNewTold", 32) != L"" WH_MOD_VERSION)) {
            SetTimer(m_hwnd, kWhatsNewTimerId, kWhatsNewDelayMs, nullptr);
        }
    }
    void ShowWhatsNewNotice();
    // The settings' window on What's new (see the notice).
    void OpenWhatsNew();
    // The settings' window on the setup assistant, once, on a first install;
    // this version's What's new taken as told.
    void OpenSetup();
    // The main island's, once every island is made (see kGlassRenewMs).
    void StartGlassRenewals() {
        m_glassRenewals = 0;
        SetTimer(m_hwnd, kGlassRenewTimerId, kGlassRenewMs[0], nullptr);
    }
    // What the islands look for together (the usage, the weather, updates)
    // looked at again: the main island's, once every island is made (one
    // only another monitor's profile shows counts then).
    void RefreshWatches() {
        UpdateUsageWatch();
        UpdateWeatherWatch();
        UpdateUpdatesWatch();
        UpdateNetSpeedWatch();
        UpdateRecycleWatch();
        UpdateAiUsageWatch();
        UpdateWindowEvents();
    }
    // "Snooze" (Windows' own button, pressed on the island): the
    // notification shows again a few minutes later.
    void SnoozeToast(IslandToast toast);
    // Kept in the mod's storage (their ID and when they're due), so that a
    // restart of the island or of the computer doesn't lose them.
    void SaveSnoozed();
    void LoadSnoozed();
    // Whether a notification (by its ID) is snoozed now.
    bool IsToastSnoozed(long long id) const {
        return id > 0 &&
               std::any_of(m_snoozedToasts.begin(), m_snoozedToasts.end(),
                           [id](const auto& entry) {
                               return entry.first.id == id;
                           });
    }
    // Notifications changed in the Windows notification center: the ones
    // gone (read elsewhere, like on the phone, or a call that ended) leave
    // the island too.
    void OnToastsChanged();
    void DropGoneUnread(const std::vector<std::wstring>& inCenter);
    // Opens (or closes) the updates; looked for if not yet (or long ago).
    void OpenUpdates() {
        // Again too after apps went to the terminal (they may be updated).
        const UpdatesData updates = GetUpdates();
        const bool inTerminal =
            std::any_of(updates.apps.begin(), updates.apps.end(),
                        [](const AppUpdate& app) { return app.state == 4; });
        if (!updates.checking &&
            (!updates.checked || inTerminal ||
             NowSeconds() - updates.checkedAt > 30 * 60)) {
            StartUpdatesJob();
        }
        TogglePanel(m_updatesPanel, IslandItem::Updates);
    }
    // Opens (or closes) the weather; old weather is measured again.
    void OpenWeather() {
        const WeatherData weather = GetWeather();
        if (!weather.valid ||
            NowSeconds() - weather.fetchedAt > kWeatherRefreshSeconds / 2) {
            StartWeather({});
        }
        TogglePanel(m_weatherPanel, IslandItem::Weather);
    }
    // A pill item's button in the control center (see ControlItem): what
    // the item opens, out of the island where the item would be (in the
    // pill's middle when it isn't on it).
    void OpenFromControlCenter(IslandItem item) {
        switch (item) {
            case IslandItem::Overflow:
                // (In their list, whichever way the pill shows them.)
                UpdateTrayList();
                TogglePanel(m_trayListPanel, item);
                break;
            case IslandItem::PrivacyMicrophone:
                // Windows' page with all three (the pill's item opens the
                // one in use).
                ShellExecute(nullptr, L"open", L"ms-settings:privacy",
                             nullptr, nullptr, SW_SHOWNORMAL);
                break;
            case IslandItem::Countdown:
                // (Nothing coming: the calendar on today.)
                if (m_countdownText.empty()) {
                    TogglePanel(m_calendar, item);
                    break;
                }
                OnClick(item, {});
                break;
            default:
                OnClick(item, {});
                break;
        }
    }
    // The timer panel on a tab (0 the timer, 1 the stopwatch, 2 Pomodoro,
    // 3 the alarms), from the control center; on that tab if it's open.
    void OpenTimerTab(int tab) {
        m_timerPanel.ShowTab(tab);
        if (!m_timerPanel.IsOpen()) {
            TogglePanel(m_timerPanel, IslandItem::Timer);
        }
    }
    // Opens (or closes) the notes, the tasks or the shortcuts, from their
    // item on the pill or their button in the control center.
    void OpenListPanel(IslandItem item) {
        Panel& panel = item == IslandItem::Notes   ? (Panel&)m_notesPanel
                       : item == IslandItem::Tasks ? (Panel&)m_tasksPanel
                                                   : (Panel&)m_shortcutsPanel;
        TogglePanel(panel, item);
    }
    // Tells explorer the island is there (it sends what it has), and
    // whether it wants the apps' progress.
    void SayHello() {
        SendIpc(FindTaskbarWnd(), m_hwnd, kIpcHello, nullptr, 0);
        m_progressTold = -1;
        TellExplorerProgress();
    }
    // Explorer told whether an island fills with the apps' progress (it
    // watches the taskbar's bars only then), when that changes: by the
    // main island, as told last (-1: not yet).
    void TellExplorerProgress();
    int m_progressTold = -1;
    // Another monitor chosen, or following the mouse turned on or off.
    void OnMonitorChoiceChanged();
    // The monitor it's on now.
    HMONITOR CurrentMonitor() const;
    // Another place (or one changed): it slides there.
    void OnPlacesChanged();
    // The app in front changed (its place may be another).
    void OnForegroundChanged();
    // The app whose place and icons the pill takes now (see islandApps'
    // places per app; none: the general ones).
    void SetPlaceApp(const std::wstring& appId);
    void RecheckPlaceApp();
    // How much of its monitor's sides the taskbar takes (the side it's on,
    // even while hidden: the pill stays clear of it), and what the other
    // bars keep (not the island itself: for the bar).
    RECT TaskbarInsets() const;
    RECT ReservedInsets() const;
    // The bar's length between the strips other bars keep (`insets`): its
    // screen's width, or its height upright on a side.
    float BarLength(const RECT& insets) const {
        return m_vertical ? (float)(m_monitor.bottom - m_monitor.top -
                                    insets.top - insets.bottom)
                          : (float)(m_monitor.right - m_monitor.left -
                                    insets.left - insets.right);
    }
    // Its inner corners' radius (see islandBarInnerCorners), in pixels.
    float BarInnerCorner() const {
        return m_bar ? std::round(kBarInnerCornerRadii[std::clamp(
                                      g_settings.islandBarInnerCorners, 0,
                                      (int)std::size(kBarInnerCornerRadii) -
                                          1)] *
                                  m_scale)
                     : 0.0f;
    }
    // On the lower half of the screen: panels and banners open upwards.
    bool AtBottom() const { return !m_bar && !m_vertical && m_placeY > 0.5; }
    // Against the top or the bottom edge, or upright on a side: where it can
    // be minimized to a line.
    bool MinimizableAt(const IslandPlace& place) const;
    // Lays it out again (an item shown or hidden), animated.
    void Relayout() {
        StartAnimating();
        Render();
    }
    // The shelf's files changed: its item and its panel follow (`added`:
    // the list shown from its top, where they come in).
    void FollowShelf(bool added);
    // Drawn again with the icons read (see OnIconsRead): its open panels and
    // the settings window too.
    void RedrawIcons() {
        Render(true);
        for (Panel* panel : Panels()) {
            if (panel->IsOpen()) {
                panel->Render();
            }
        }
        if (m_settingsWindow.IsOpen()) {
            m_settingsWindow.RefreshSettings();
        }
    }
    // Drawn again with its glass and its open panels' made again (see
    // RenewAllGlass).
    void RenewGlass() {
        Render(true);
        for (Panel* panel : Panels()) {
            if (panel->IsOpen()) {
                panel->Render();
            }
        }
        if (m_settingsWindow.IsOpen()) {
            m_settingsWindow.Render();
        }
    }
    // Another size chosen for the panels (islandPanelSize): the open ones
    // laid out again at it.
    void RescalePanels() {
        for (Panel* panel : Panels()) {
            panel->Rescale(PanelScale());
        }
    }

   private:
    void UpdateClock();
    void UpdateStatus();
    void UpdateApps();
    void ScheduleTick();
    // Since when the screenshot's selection is in front (see Tick), or 0.
    double m_snippingSince = 0;
    static constexpr double kSnippingHideSeconds = 1;
    std::wstring NetworkGlyph();
    std::wstring VolumeGlyph();
    std::wstring BatteryGlyph();
    void UpdateFormats(float scale);
    // The icons' and texts' sizes the formats were made for (see
    // UpdateFormats).
    int m_formatIconSize = 0;
    int m_formatTextSize = 0;
    int m_formatBannerSize = -1;
    bool m_formatVertical = false;
    // The pill's texts' scale: the island's times their own size
    // (islandTextSize); upright, the island's (that pill is narrow). The uses'
    // two lines grow as far as they fit the pill.
    float m_textScale = 1;
    float m_usageScale = 1;
    static constexpr float kUsageMostScale = 1.15f;
    bool IsCovered() const;
    // The screen's scaling (m_scale also has the island's size in it).
    float m_dpiScale = 1;
    // The panels': the screen's, times their chosen size (islandPanelSize).
    float PanelScale() const {
        return m_dpiScale * g_settings.islandPanelSize / 100.0f;
    }
    void Layout();
    // A notch's place and window (see Layout), and its curves' size.
    void LayoutNotch(float contentWidth,
                     float height,
                     float padding,
                     float scale);
    float NotchCurve() const {
        return NotchRadius(kPillHeight * PillHeightScale() * m_scale);
    }
    float NotchShoulderWidth() const {
        return NotchShoulder(kPillHeight * PillHeightScale() * m_scale);
    }
    int m_notch = -1;
    int m_notchCount = 0;
    Island* m_notchBase = nullptr;
    std::vector<Island*> m_notchSiblings;
    // How wide its body would be with all its items (see NotchBodyMax).
    float m_notchWanted = 0;
    float NotchBodyMax(float scale);
    // All the notches on its monitor (this one alone when not a notch).
    std::vector<Island*> NotchSet() {
        Island* base = m_notchBase ? m_notchBase : this;
        std::vector<Island*> set{base};
        set.insert(set.end(), base->m_notchSiblings.begin(),
                   base->m_notchSiblings.end());
        return set;
    }
    // Where the screen's sides are in the window (the shape curves down
    // there).
    float m_notchEdgeLeft = 0;
    float m_notchEdgeRight = 0;
    // Its shape as last drawn (made again only when it changes, see Render).
    winrt::com_ptr<ID2D1PathGeometry> m_notchFill;
    winrt::com_ptr<ID2D1PathGeometry> m_notchLine;
    std::array<float, 8> m_notchShapeKey{};
    // Where it's kept minimized (each notch on its own).
    std::wstring MinimizedKey() const {
        return m_notch < 0 ? std::wstring(L"islandMinimized")
                           : L"islandMinimizedNotch" + std::to_wstring(m_notch);
    }
    // The room from the edges as shown (following the setting).
    float GapShown() {
        if (m_gapShown < 0) {
            m_gapShown = GapWanted();
        }
        return (float)m_gapShown;
    }
    void Render(bool force = false);
    void SetAppBarPosition(bool force = false);
    void RegisterAppBar();
    void CheckAppBarRoom();
    // Times in a row the strip was told again (see CheckAppBarRoom).
    int m_appBarRetries = 0;
    // What was told last (see SetAppBarPosition).
    UINT m_appBarEdge = ABE_TOP;
    int m_appBarThickness = -1;
    RECT m_appBarMonitor{};
    // The strip kept, as the system gave it.
    RECT m_appBarRect{};
    IslandItem HitTest(POINT pt, std::wstring* appId = nullptr) const;
    // What's playing's slot: its cover and title (font 6), or its note.
    static PCWSTR MediaSlotPart(const IslandSlot& slot) {
        return slot.font == 6 ? L"cover" : L"note";
    }
    void OnClick(IslandItem item, const std::wstring& appId);
    ID2D1Bitmap* GetAppBitmap(const std::wstring& appId);
    // The item clicked is the one with that key (see SlotKey), or the one
    // under the mouse; IslandItem::None opens out of the island's middle (the
    // settings), in every style.
    void TogglePanel(Panel& panel,
                     IslandItem item,
                     const std::wstring& key = {});
    // An open panel showing another item's view (the control center's Wi-Fi
    // page from its pill button): it glides to under that item.
    void MovePanel(Panel& panel,
                   IslandItem item,
                   const std::wstring& key = {});
    // Where a panel opens for that item (see TogglePanel): false if there's
    // no place for it.
    bool PanelPlace(IslandItem item,
                    const std::wstring& key,
                    POINT* anchor,
                    D2D1_RECT_F* source,
                    Panel::Direction* direction,
                    bool* fromPill,
                    bool* behind);
    // Kept for next time unless the auto-hide does it (`keep` false).
    void SetMinimized(bool minimized, bool keep = true);
    void SetBarHidden(bool hidden);
    // Hiding as its line while the mouse is away (islandAutoHide): turned on
    // or off, and checked on a timer while on.
    void UpdateAutoHide();
    void CheckAutoHide();
    void FollowWindowsMode();
    // When the mouse left it (0 while it's on it or something's open).
    double m_autoHideAwaySince = 0;
    // Minimized by the auto-hide (not by the user): it comes back by itself
    // once no window is in its way (see islandHideWhen).
    bool m_autoHidden = false;
    // A window in its way (see islandHideWhen): looked at again after
    // Windows' events (see UpdateWindowEvents), the place its open pill
    // takes on the screen.
    bool WindowInTheWay();
    bool SmartHideOn() const;
    bool m_windowInTheWay = false;
    WayLook m_wayLook;
    RECT m_wayRect{};
    // The main island's: Windows' events about the apps' windows watched
    // while an island looks for a window in its way.
    void UpdateWindowEvents();
    void StopWindowEvents();
    HWINEVENTHOOK m_windowEventHooks[std::size(kWindowEventRanges)]{};
    // Only the clock when hidden (islandHideClock): how far its small pill
    // is shown instead of the line, whether it's wanted now, its text
    // (measured once for each), where it is on the screen, and an app's
    // window under it (the line then, looked at like WindowInTheWay).
    // (The same small pill shows a timer going, islandTimerLine: its ring
    // and time, beside the clock's or alone; see TimerLineWanted.)
    bool HiddenClockWanted() const;
    bool HiddenTimeWanted() const;
    bool TimerLineWanted() const;
    std::wstring HiddenPillText() const;
    float HiddenRingSize(float scale) const {
        return std::round(
            std::clamp(m_pill.bottom - m_pill.top - 10 * scale, 10 * scale,
                       18 * scale));
    }
    void DrawHiddenPillContents(ID2D1RenderTarget* target,
                                ID2D1SolidColorBrush* brush,
                                const D2D1_RECT_F& rect,
                                float opacity,
                                float scale);
    bool HiddenClockCovered();
    D2D1_RECT_F HiddenClockRect(float scale);
    double m_hiddenClock = 0;
    double m_hiddenClockVelocity = 0;
    bool m_hiddenClockWanted = false;
    std::wstring m_hiddenClockText;
    std::wstring m_hiddenClockLaidOut;
    IDWriteTextFormat* m_hiddenClockFormat = nullptr;
    winrt::com_ptr<IDWriteTextLayout> m_hiddenClockLayout;
    RECT m_hiddenClockScreen{};
    bool m_hiddenClockCovered = false;
    WayLook m_hiddenClockLook;

    // New notifications taken (by the main island) and shown on each.
    void ShowBanner();
    void ShowToasts(std::vector<IslandToast> toasts);
    void HideBanner();
    void ShowReminder(const CalendarEvent& event);
    // The day of the reminder shown (its banner opens the calendar there).
    struct {
        int year = 0;
        int month = 0;
        int day = 0;
    } m_reminderDay;
    D2D1_RECT_F BannerRect() const;
    // The settings bubble: shown on a right click on the island (whatever
    // is on the pill, so the settings can always be reached), out of its
    // edge like a new app peeking out of the line; it goes after a few
    // seconds, or when it's clicked (the control center opens on the
    // settings). On the bar, the settings open right away.
    void ShowGearBubble();
    void HideGearBubble();
    D2D1_RECT_F GearBubbleRect() const;
    float GearBubbleWidth() const;
    float GearAmount(double now) const;
    double m_gearShownAt = 0;
    // The bubble showing a text instead (the profile chosen with Shift and
    // the wheel), under an item of the pill (its middle, or -1 the pill's).
    std::wstring m_gearText;
    float m_gearTextWidth = 0;
    float m_gearX = -1;
    void ShowTextBubble(const std::wstring& text, float x);
    // Caps Lock's state in the bubble (see islandCapsLock).
    void ShowCapsLock();
    // A text bubble shown (what the wheel changed): an item's name under the
    // mouse doesn't come over it.
    bool TextBubbleShown() const {
        return !m_gearText.empty() && m_gearShownAt &&
               NowSeconds() < m_gearHideAt;
    }
    // On the bar, the bubble is a window of its own under it (the bar's
    // window is only as tall as the strip it keeps, but for a banner), drawn
    // like the pill's.
    HWND m_bubbleWnd = nullptr;
    LayeredCanvas m_bubbleCanvas;
    void RenderBarBubble();
    // The frosted glass (islandFrosted, see UpdateGlass, with Windows'
    // transparency effects on: g_transparencyEffects): whether the bar is
    // drawn over the glass, and the glass strip under the bar or the Fixed
    // style's pill (a window of its own under the island) with its place and
    // color.
    bool m_barBlurred = false;
    bool BarClear() const;
    HWND m_glassStrip = nullptr;
    RECT m_glassRect{};
    D2D1_COLOR_F m_glassColor{};
    BYTE m_glassOpacity = 255;
    void UpdateGlass();
    // The Liquid Glass under the island's window (islandLiquidGlass, see
    // GlassLayer), and its shapes found while drawing (see Render).
    GlassLayer m_liquid;
    std::vector<GlassShape> m_liquidShapes;
    // What's copied, in the bubble (islandClipboard): listened to while it's
    // on, and read again a moment later while the clipboard is busy; the
    // last one shown, so the same copy isn't shown twice in a row.
    bool m_clipboardWatched = false;
    int m_clipboardTries = 0;
    std::wstring m_clipboardShown;
    double m_clipboardShownAt = 0;
    void UpdateClipboardWatch();
    void OnClipboardChanged();
    // The keyboard's input method on the pill (the main island watches it
    // and tells the others).
    void UpdateInputMethodWatch();
    void StartInputMethodQuery();
    void OnInputMethod(WPARAM state, LPARAM layouts);
    HWINEVENTHOOK m_inputMethodHooks[2] = {};
    void StopInputMethodThread();
    HANDLE m_inputMethodThread = nullptr;
    std::unique_ptr<InputMethodJob> m_inputMethodJob;
    UINT m_inputMethodId = 0;
    std::wstring m_inputMethodText;
    // The window in front when it was read (its top owner, see
    // OnInputMethod).
    HWND m_inputMethodWindow = nullptr;
    // Apps using the microphone, the camera or the location (the main
    // island watches them, see PrivacyThreadProc); the camera's icon blinks
    // a moment when it starts.
    void UpdatePrivacyWatch();
    void OnPrivacy();
    std::wstring PrivacyText() const;
    PrivacyUse m_privacy;
    double m_cameraSince = 0;
    // Drives plugged in or out (the main island lists them, see
    // DrivesThreadProc, and tells the others).
    void OnDeviceChange(WPARAM event, LPARAM data);
    void ListDrives(DWORD arrived);
    void UpdateDrivesWatch();
    // One listing at a time (a slow one can't bring back a drive gone
    // meanwhile); one asked meanwhile runs after it, with the drives that
    // arrived.
    HANDLE m_driveThread = nullptr;
    UINT m_drivesId = 0;
    bool m_drivesAgain = false;
    DWORD m_drivesArrived = 0;
    std::vector<RemovableDrive> m_drives;
    bool m_drivesListed = false;

   public:
    // The app in front, or its keyboard, may have changed: looked at a
    // moment later (several events come at once).
    void CheckInputMethodSoon() {
        if (m_hwnd && m_main && g_settings.islandInputMethod) {
            SetTimer(m_hwnd, kInputMethodTimerId, kInputMethodWaitMs, nullptr);
        }
    }

   private:
    // An app's icon (with unread notifications): its tip's key is this and
    // its ID.
    static constexpr WCHAR kAppTipPrefix[] = L"\x01" L"app:";
    // An item with a name to show (see ItemTip): the item's number, ":",
    // and its slot's text.
    static constexpr WCHAR kItemTipPrefix[] = L"\x01" L"item:";
    std::wstring ItemTipKey(IslandItem item, const std::wstring& text) const {
        return kItemTipPrefix + std::to_wstring((int)item) + L":" + text;
    }
    // What it shows: which apps use the microphone, camera or location, a
    // control center button's name, the current desktop's name.
    std::wstring ItemTip(IslandItem item, const std::wstring& text) const;
    // Shift and the wheel on the profiles' button or the volume: the next
    // profile or output device (or the one before), shown in the bubble,
    // applied once Shift is let go.
    enum class ShiftChoice { None, Profile, Output };
    void StepShiftChoice(ShiftChoice kind, int direction);
    void ApplyShiftChoice();
    ShiftChoice m_shiftChoice = ShiftChoice::None;
    int m_shiftStep = -1;
    std::vector<AudioOutput> m_shiftOutputs;
    double m_gearHideAt = 0;
    // The reply box (with the send button at its end) and "Open".
    void BannerParts(D2D1_RECT_F* field,
                     D2D1_RECT_F* send,
                     D2D1_RECT_F* open) const;
    // The "x" at its top right (deletes it) and the arrow left of it
    // (minimizes it), under the mouse.
    D2D1_RECT_F BannerCloseRect() const;
    D2D1_RECT_F BannerMinimizeRect() const;
    // A strip along the screen's edge it's against, across from the pill's
    // middle: the minimized line.
    D2D1_RECT_F EdgeLineRect(float length, float thickness) const;
    // The strip along that edge that brings the minimized line back
    // (islandActivationWidth), at `scale`.
    static float ActivationLength(float scale) {
        return std::round(g_settings.islandActivationWidth * scale);
    }
    bool BannerHovered() const {
        return m_hover == IslandItem::Banner ||
               m_hover == IslandItem::BannerReply ||
               m_hover == IslandItem::BannerOpen ||
               m_hover == IslandItem::BannerClose ||
               m_hover == IslandItem::BannerMinimize ||
               m_hover == IslandItem::BannerAction;
    }
    void StartBannerTyping();
    void EndBannerTyping();
    // A press in the reply box while typing: the caret goes there at once,
    // and a drag selects (see StartEditDrag). Returns true if it was there.
    bool PressBannerReply(POINT pt);
    bool m_replyPressed = false;
    void SendBannerReply();
    bool OnBannerChar(WCHAR c);
    void UpdateTrayIcons();
    const TrayIcon* FindTrayIcon(const std::wstring& key) const;
    ID2D1Bitmap* GetTrayBitmap(const TrayIcon& icon);
    bool IsTrayPinned(const std::wstring& key) const;
    void SetTrayPinned(const std::wstring& key, bool pinned);
    // The icons in the order kept (see g_trayOrder); every island laid out
    // again after a change (the ones with this look show it).
    void SortTrayIcons();
    void OnTrayArranged();
    // A tray icon held on the pill (see g_trayDrag): dragged once the mouse
    // moved a little, and let go (dropped, or `drop` false: back).
    void DragTrayIcon();
    void FinishTrayDrag(bool drop);
    void DrawTrayDrag(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush);
    void SendToTrayIcon(const std::wstring& key, UINT message);
    void ShowTrayTip(const std::wstring& key);
    void WatchTrayAppWindows(const TrayIcon& icon);
    void StopWatchingTrayAppWindows();
    // The icons that aren't pinned, to the list opened from the arrow.
    void UpdateTrayList();

   public:
    // A mouse message on a tray icon (on the pill or in the list opened
    // from the arrow), passed on to its app; and pinning one or not.
    void ClickTrayIcon(const std::wstring& key, UINT msg);
    void ToggleTrayPin(const std::wstring& key) {
        SetTrayPinned(key, !IsTrayPinned(key));
    }
    // Where an icon dragged to `screen` would go on this island (see
    // TrayDropAt), on the island there of `owner`'s monitor; and putting it
    // there (as the current island).
    bool TrayDropAt(POINT screen,
                    const std::wstring& key,
                    TrayDrop* drop) const;
    static TrayDrop TrayDropUnder(Island* owner,
                                  POINT screen,
                                  const std::wstring& key);
    void MoveTrayIcon(const std::wstring& key, const TrayDrop& drop);
    // Drawn again, each as itself (a drag's line and icon came or went).
    static void RenderIslands(std::initializer_list<Island*> islands);
    void OnWindowsBanner(HWND banner, bool appeared);
    // A Windows banner showed (whatever the island does with it).
    void OnWindowsBannerSeen() { m_windowsBannerSeenAt = NowSeconds(); }
    // The last Windows banner went (the stack ended).
    void OnWindowsBannersGone();
    // A read of Windows' banners is done (see ReadWindowsBanners).
    void OnWindowsBannersRead();
    void WatchWindowsBannerMoves(DWORD processId);
    void OnTrayAppWindow(HWND hWnd);

   private:
    D2D1_RECT_F TrayPinRect(const D2D1_RECT_F& slot) const;
    void OnMouseButton(UINT msg, LPARAM lParam);

    HWND m_hwnd = nullptr;
    bool m_bar = false;
    // The Bar style has no line to shrink into: the minimize shortcut hides
    // it altogether (not kept: it's back when the island starts again).
    bool m_barHidden = false;
    winrt::com_ptr<ID2D1Factory> m_d2d;
    winrt::com_ptr<IDWriteFactory> m_dwrite;
    LayeredCanvas m_canvas;
    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_smallIconFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    // The count on the icons of the apps with notifications.
    winrt::com_ptr<IDWriteTextFormat> m_badgeFormat;
    // Figures of equal width ("1" as wide as "0"), for the clock.
    winrt::com_ptr<IDWriteTypography> m_tabular;
    winrt::com_ptr<IMMDeviceEnumerator> m_devices;
    winrt::com_ptr<INetworkListManager> m_network;
    HANDLE m_wlan = nullptr;
    CalendarPanel m_calendar;
    ControlPanel m_control;
    // The settings' window (see OpenSettingsWindow), made the first time.
    ControlPanel m_settingsWindow;
    NotificationPanel m_notifications;
    MediaPanel m_mediaPanel;
    PowerPanel m_power;
    ProfilesPanel m_profilesPanel;
    DrivesPanel m_drivesPanel;
    RecycleBinPanel m_recyclePanel;
    AiUsagePanel m_aiUsagePanel;
    CommandsPanel m_commandsPanel;
    ShelfPanel m_shelfPanel;
    // How many files the shelf holds, beside its icon.
    SideText m_shelfText;
    LyricsPanel m_lyricsPanel;
    // The song's lyrics (see LyricsThreadProc): which song, looked for (the
    // main island looks; on a thread, one at a time, another song waiting),
    // the line sung now on the pill and the one before it, since when (the
    // next one comes up as it goes).
    std::wstring m_lyricsKey;
    SongLyrics m_lyrics;
    bool m_lyricsSearching = false;
    HANDLE m_lyricsThread = nullptr;
    std::wstring m_lyricsLine;
    std::wstring m_lyricsBefore;
    double m_lyricsChangedAt = 0;
    winrt::com_ptr<IDWriteTextFormat> m_lyricsFormat;
    float m_lyricsFormatSize = 0;
    // The line's own width on the pill (as laid out last): drawn that wide
    // while its room grows or shrinks, uncovered rather than squeezed.
    float m_lyricsWidth = 0;
    // The widest line since the last pause (none sung), on its own and in
    // the title's place: the lines keep that width (see Relayout).
    float m_lyricsHeldWidth = 0;
    float m_titleLyricsWidth = 0;
    void UpdateLyrics();
    bool StartLyricsSearch(const MediaState& media, const std::wstring& key);
    // An app listed for the lyrics (see LyricsApps): the settings shown
    // laid out again.
    void RefreshLyricsAppsSetting();
    void StepLyrics();
    IDWriteTextFormat* LyricsFormat();
    TimerPanel m_timerPanel;
    // The timer done or Pomodoro's part over (the main island's, each
    // second, see Tick): a notice that stays until clicked, with a sound.
    void CheckTimer();
    // An alarm due this minute: a notice that stays, with the alarm sound
    // when it has one (the main island's; each once a minute).
    void CheckAlarms();
    // An alarm's sound playing (see CheckAlarms, StopAlarmSound).
    bool m_alarmRinging = false;
    void StopAlarmSound();
    TrayListPanel m_trayListPanel;
    KeyboardPanel m_keyboardPanel;
    DesktopsPanel m_desktopsPanel;
    FoldersPanel m_foldersPanel;
    // Taking Windows' drags; files held on the folders' item (see
    // DragOverAt).
    DropRegistration m_drop;
    bool m_dropOnFolders = false;
    // Files dragged over the rest of the pill: put on the shelf when let
    // go (see DragOverAt).
    bool m_dropOnShelf = false;
    WallpaperPanel m_wallpaperPanel;
    KomorebiPanel m_komorebiPanel;
    // A wallpaper coming in (see WallpaperChangeThreadProc), and the folder's
    // pictures for the wheel (with the one last chosen).
    HANDLE m_wallpaperThread = nullptr;
    HANDLE m_wallpaperListThread = nullptr;
    std::vector<std::wstring> m_wallpapers;
    int m_wallpaperIndex = -1;
    void StepWallpaper(int step);

   public:
    void ChangeWallpaper(const std::vector<std::wstring>& paths, int index);
    static void ShareBrightness(HMONITOR screen, int level);
    void WatchMediaAppOpen(const std::wstring& appId);

    HWND LastAppWindow() const { return m_appWindow; }

   private:
    // The last app window in front (not the island's own windows), for the
    // keyboards' list and the shortcuts that type (see FrontLastApp).
    HWND m_appWindow = nullptr;
    // The virtual desktops shown, and their thread (the main island's).
    VirtualDesktops m_desktops;
    HANDLE m_desktopsThread = nullptr;
    HANDLE m_desktopsStop = nullptr;
    // Windows' additional clocks watched while an island shows them (see
    // WorldClocksThreadProc).
    HANDLE m_clocksThread = nullptr;
    HANDLE m_clocksStop = nullptr;
    void UpdateWorldClocksWatch();
    void StopWorldClocksWatch();
    void UpdateDesktopsWatch();
    void StopDesktopsWatch();
    // komorebi's state, read on a thread by the main island while an island
    // shows its layout (see KomorebiWatchThreadProc).
    HANDLE m_komorebiWatch = nullptr;
    HANDLE m_komorebiWatchStop = nullptr;
    void UpdateKomorebiWatch();
    void StopKomorebiWatch();
    // The app in focus on this island's monitor (see NoteFocusedApp): its
    // window, or the desktop; its key (for its icon) and its name. A window
    // left out (a menu, Start) isn't looked at again while it's in front.
    HWND m_focusWindow = nullptr;
    bool m_focusDesktop = false;
    std::wstring m_focusKey;
    std::wstring m_focusName;
    HWND m_focusSkipped = nullptr;
    void NoteFocusedApp(HWND window);
    void CheckFocusedApp();
    void ClearFocusedApp();
    // Its window's title too (islandFocusedTitle): read again each second
    // (see CheckFocusedApp); the name and title as shown.
    std::wstring m_focusTitle;
    bool ReadFocusTitle();
    std::wstring FocusedText() const;
    // komorebi's layout's and the app's widths on the pill.
    ItemWidth m_komorebiWidth;
    ItemWidth m_focusWidth;
    float MeasureItemName(ItemWidth& width,
                          const std::wstring& name,
                          float height);
    // A control center button on the pill, clicked (an index in
    // kPillActions).
    void RunPillAction(int action);
    // Whether a button on the pill shows its thing on (lit).
    bool PillActionLit(ControlId id) const;
    // Night light's state as last read (see PillActionLit).
    int m_nightLightState = -1;
    // All of them, for what's done to each.
    std::array<Panel*, 25> Panels() {
        return {&m_calendar,       &m_control,       &m_notifications,
                &m_mediaPanel,     &m_power,         &m_profilesPanel,
                &m_usagePanel,     &m_notesPanel,    &m_tasksPanel,
                &m_shortcutsPanel, &m_weatherPanel,  &m_updatesPanel,
                &m_drivesPanel,    &m_trayListPanel, &m_keyboardPanel,
                &m_desktopsPanel,  &m_foldersPanel,  &m_wallpaperPanel,
                &m_komorebiPanel,  &m_recyclePanel,  &m_timerPanel,
                &m_lyricsPanel,    &m_aiUsagePanel,  &m_commandsPanel,
                &m_shelfPanel};
    }
    std::array<const Panel*, 25> Panels() const {
        return {&m_calendar,       &m_control,       &m_notifications,
                &m_mediaPanel,     &m_power,         &m_profilesPanel,
                &m_usagePanel,     &m_notesPanel,    &m_tasksPanel,
                &m_shortcutsPanel, &m_weatherPanel,  &m_updatesPanel,
                &m_drivesPanel,    &m_trayListPanel, &m_keyboardPanel,
                &m_desktopsPanel,  &m_foldersPanel,  &m_wallpaperPanel,
                &m_komorebiPanel,  &m_recyclePanel,  &m_timerPanel,
                &m_lyricsPanel,    &m_aiUsagePanel,  &m_commandsPanel,
                &m_shelfPanel};
    }
    bool AnyPanelOpen() const {
        for (const Panel* panel : Panels()) {
            if (panel->IsOpen()) {
                return true;
            }
        }
        return false;
    }
    bool PanelShownFromPill() const {
        for (const Panel* panel : Panels()) {
            if (panel->ShownFromPill()) {
                return true;
            }
        }
        return false;
    }
    // The title's width while a panel out of the pill shows (0 none): the
    // pill, and the panel's corners against it, stay as they were; the
    // title scrolls in it, and the width follows once the panel has closed.
    float m_titleHeldWidth = 0;
    void CloseAllPanels() {
        for (Panel* panel : Panels()) {
            panel->Close();
        }
    }
    // What's playing, from the media thread.
    MediaState m_mediaState;
    // What the pill shows: it grows in and out (m_mediaAmount from 0 to 1, a
    // bit more while bouncing), and stays a moment after the media starts or
    // stops, so it doesn't flash. Kept as it last was while it goes away.
    MediaState m_mediaShown;
    bool m_mediaWanted = false;
    double m_mediaSince = 0;
    double m_mediaGoneAt = 0;
    double m_mediaAmount = 0;
    double m_mediaVelocity = 0;
    // Its buttons on the pill, growing in and out the same way.
    double m_mediaButtonsAmount = 0;
    double m_mediaButtonsVelocity = 0;
    // A button pressed on the pill shrinks, and springs back when released;
    // play turning into pause (and back) shrinks and grows again.
    PressAnimation m_press;
    // How much room they take: grows without bouncing (the icons bounce on
    // their own), so the pill widens smoothly.
    double m_mediaRoom = 0;
    double m_mediaRoomVelocity = 0;
    double m_mediaButtonsRoom = 0;
    double m_mediaButtonsRoomVelocity = 0;
    // The cover and the title instead of the note (0 to 1, animated), the
    // cover's bitmap, and the title's width, measured once per title.
    double m_mediaTitleRoom = 0;
    double m_mediaTitleRoomVelocity = 0;
    winrt::com_ptr<ID2D1Bitmap> m_mediaCover;
    UINT m_mediaCoverSerial = (UINT)-1;
    std::wstring m_mediaTitleMeasured;
    // The title as shown (with its artist, when short).
    std::wstring m_mediaTitleShown;
    float m_mediaTitleWidth = 0;
    // Its width without the limit: wider, it keeps scrolling while it plays
    // with islandMediaMarquee, on the screen (from m_mediaMarqueeStart, a new
    // title or playing again; m_mediaMarquee while it does). Its tip has it
    // all.
    float m_mediaTitleFullWidth = 0;
    double m_mediaMarqueeStart = 0;
    bool m_mediaMarquee = false;
    // The banner's title too long, under the mouse: scrolling from
    // m_bannerMarqueeStart (see Render), m_bannerMarquee while it does.
    double m_bannerMarqueeStart = 0;
    bool m_bannerMarquee = false;
    float m_bannerMarqueeDistance = 0;
    // How far it scrolls (as last drawn): its place goes in the render key.
    float m_mediaMarqueeDistance = 0;
    bool MediaTitleScrolls() const {
        return g_settings.islandMediaMarquee && m_mediaShown.playing &&
               !m_away && AnimationLevel() == 0 &&
               m_mediaTitleFullWidth > m_mediaTitleWidth + 1;
    }
    // The wheel on what's playing changing the song (islandMediaWheel): its
    // steps added up, the last one, and the last song change.
    int m_mediaWheelDelta = 0;
    double m_mediaWheelAt = 0;
    double m_mediaSkippedAt = 0;
    // What's playing's app clicked in the tray, and when (see
    // WatchMediaAppOpen).
    std::wstring m_mediaOpenApp;
    double m_mediaOpenAt = 0;
    // Another song: the title before slides up and away as the new one comes
    // up from below, the cover before fades into the new one (1 to 0), and
    // the pill's width follows the new title smoothly.
    std::wstring m_mediaTitleBefore;
    double m_mediaSwap = 0;
    double m_mediaSwapVelocity = 0;
    winrt::com_ptr<ID2D1Bitmap> m_mediaCoverBefore;
    double m_mediaCoverSwap = 0;
    double m_mediaCoverSwapVelocity = 0;
    double m_mediaTitleWidthShown = 0;
    double m_mediaTitleWidthVelocity = 0;
    ID2D1Bitmap* MediaCover();
    // Its color, bright (0xRRGGBB, -1 none; see kVisualizerCover).
    int m_mediaCoverRgb = -1;
    friend int MediaCoverRgb();
    std::wstring MediaTitleText() const;
    bool LyricsInTitle() const;
    double m_playSwap = 0;
    double m_playSwapVelocity = 0;
    void UpdateMediaShown();
    // The next (1) or previous (-1) app with media, chosen. The dots under
    // the cover show which one it is for a moment, then fade away.
    void StepMediaSource(int step) {
        const int count = (int)m_mediaShown.sources.size();
        if (count < 2) {
            return;
        }
        const int now = std::max(m_mediaShown.sourceIndex, 0);
        ChooseMediaSource(m_mediaShown, ((now + step) % count + count) % count);
        m_mediaDotsUntil = NowSeconds() + 1.5;
        StartAnimating();
    }
    double m_mediaDots = 0;
    double m_mediaDotsVelocity = 0;
    double m_mediaDotsUntil = 0;
    bool m_appBar = false;
    // Windows' dark mode (for the light and dark mode button), and its icon
    // turning half a turn when it's switched (0 light, 1 dark).
    bool m_darkMode = false;
    double m_themeTurn = 0;
    double m_themeTurnVelocity = 0;
    // The processor, memory and video memory use shown (see
    // UsageThreadProc), its thread while shown, and the room each takes:
    // each counts from the value before to the new one, one step at a time
    // (-1 unknown).
    double m_usageFrom[3] = {-1, -1, -1};
    double m_usageTo[3] = {-1, -1, -1};
    double m_usageChangedAt[3]{};
    std::wstring UsageText(int index) const;
    HANDLE m_usageThread = nullptr;
    HANDLE m_usageStop = nullptr;
    HANDLE m_usageWake = nullptr;
    UsagePanel m_usagePanel;
    NotesPanel m_notesPanel;
    TasksPanel m_tasksPanel;
    ShortcutsPanel m_shortcutsPanel;
    WeatherPanel m_weatherPanel;
    UpdatesPanel m_updatesPanel;
    // The updates' threads (looking, updating), stopped with the island;
    // how many are waiting (on the pill).
    std::vector<HANDLE> m_updatesThreads;
    HANDLE m_updatesStop = nullptr;
    int m_updatesCount = 0;
    bool m_updatesWatched = false;
    void StartUpdatesJob();
    void UpdateUpdatesWatch();
    void OnUpdates();
    // The weather's thread (one at a time), and what the pill shows ("sky
    // emoji, tab, temperature"), from g_weather.
    HANDLE m_weatherThread = nullptr;
    std::wstring m_weatherShown;
    void StartWeather(const std::wstring& search);
    void OnWeather(int problem);
    void ShowWeather(const WeatherData& weather);
    void UpdateWeatherWatch();
    bool WeatherWanted() const;
    // Some island's (see AnyIslandLook): the main island measures it then.
    bool AnyWeatherWanted() const;
    // Failed tries in a row (each waits longer for the next).
    int m_weatherRetries = 0;
    // A city typed while a request was running: looked for once it ends.
    std::wstring m_weatherPendingSearch;
    // The network as the pill shows it: off, connecting (Wi-Fi joining, or
    // Windows checking for the internet just after connecting), connected
    // without internet, or online; and its glyph changing (the one before
    // fading out as the new one grows in).
    enum class NetState { Off, Connecting, NoInternet, Online };
    NetState m_netState = NetState::Off;
    bool m_netWifi = false;
    int m_netBars = 3;
    double m_netLocalSince = 0;
    double m_netConnectingSince = 0;
    double m_netShowConnectingUntil = 0;
    std::wstring m_networkFrom;
    double m_networkSwap = 0;
    double m_networkSwapVelocity = 0;
    void UpdateNetwork();
    void SetNetworkGlyph(std::wstring glyph);
    void OnInternetBack();
    void DrawNetwork(ID2D1RenderTarget* target,
                     ID2D1SolidColorBrush* brush,
                     const IslandSlot& slot,
                     float opacity);
    // An app's icon on the pill (`icon` is where it goes): the Widgets
    // button's badge with the Widgets glyph (see kWidgetsBadgeId), a missing
    // icon as a faint square, unless `missingShown` is false.
    void DrawAppIcon(ID2D1RenderTarget* target,
                     ID2D1SolidColorBrush* brush,
                     const std::wstring& appId,
                     const D2D1_RECT_F& icon,
                     float opacity,
                     bool missingShown);
    float m_usageWidth = 0;
    winrt::com_ptr<IDWriteTextFormat> m_weatherEmojiFormat;
    float m_weatherEmojiScale = 0;
    winrt::com_ptr<IDWriteTextFormat> m_usageLabelFormat;
    winrt::com_ptr<IDWriteTextFormat> m_usageValueFormat;
    // What's playing's title on the pill.
    winrt::com_ptr<IDWriteTextFormat> m_mediaTitleFormat;
    void UpdateUsageWatch();
    void OnUsagePanelChanged();
    void StopUsageWatch();
    void OnUsage();
    // How far a value's change is (0 to 1; 1 when done).
    float UsageChange(int index) const {
        const double changedAt = m_usageChangedAt[index];
        const double length = AnimationSeconds(kUsageChangeSeconds, true);
        return changedAt && length > 0
                   ? (float)std::clamp((NowSeconds() - changedAt) / length,
                                       0.0, 1.0)
                   : 1.0f;
    }

    float m_scale = 0;
    RECT m_monitor{};
    // Its monitor's work area (where a placed banner goes, see
    // PlacedBannerRect).
    RECT m_workArea{};
    POINT m_position{};
    SIZE m_size{};
    // The expanded pill's rect, in the window.
    D2D1_RECT_F m_pill{};
    std::vector<IslandSlot> m_slots;
    // Where each group of items starts along the pill (as laid out), and how
    // far it's still shifted from there, sliding back to it after the order
    // changed (see OnOrderChanged).
    float m_groupBase[kPillGroups]{};
    double m_groupShift[kPillGroups]{};
    double m_groupShiftVelocity[kPillGroups]{};
    // A group going to another place by itself (the clock kept in the
    // middle, the app in front's own order): from 1 to 0, it fades out where
    // it was as it fades in at its place, instead of sliding over the
    // others.
    double m_groupSwap[kPillGroups]{};
    double m_groupSwapVelocity[kPillGroups]{};
    // How much each group is shown (0 to 1, a bit more while bouncing): it
    // shrinks away when turned off, or hidden for the app in front (see
    // IslandPlace::hidden), and grows back in.
    double m_groupShown[kPillGroups]{};
    double m_groupShownVelocity[kPillGroups]{};
    // Each group's items as last laid out while it was wanted, and their
    // width: shown while it shrinks away (its option turned off empties it
    // at once). And its width shown, following its items' with a spring: an
    // item in it coming or going (a percentage, a longer text) doesn't jump.
    std::vector<IslandSlot> m_groupKept[kPillGroups];
    float m_groupKeptWidth[kPillGroups]{};
    float m_groupWidthWanted[kPillGroups]{};
    double m_groupWidthShown[kPillGroups]{};
    double m_groupWidthVelocity[kPillGroups]{};
    bool GroupWanted(PillGroup group) const;
    bool PlaceShows(PillGroup group) const;
    IslandItem m_hover = IslandItem::None;
    // What's under the mouse, lighting up smoothly (see HoverFades).
    HoverFades m_hoverFades;
    // The app under the mouse, when m_hover is App.
    std::wstring m_hoverApp;
    bool m_tracking = false;
    bool m_visible = false;
    // Where it is (fractions of the room it moves in, see IslandPlace),
    // springing to its place, and the room in screen pixels (from Layout).
    double m_placeX = 0.5;
    double m_placeY = 0;
    double m_placeVelocityX = 0;
    double m_placeVelocityY = 0;
    float m_roomLeft = 0;
    float m_roomTop = 0;
    float m_roomWidth = 0;
    float m_roomHeight = 0;
    // Upright on the left or right edge (its items one under the other; its
    // panels and banners open towards the screen's middle).
    bool m_vertical = false;
    bool m_sideLeft = true;
    bool m_verticalKnown = false;
    // Turning upright or back: closing into its dot (1), then turned (2,
    // laid out the new way once), opening again.
    int m_reshape = 0;
    // The clock's two lines when upright: the time and a short date.
    std::wstring m_clockTime;
    std::wstring m_clockDate;
    // Its parts shown (see GlobalClockParts), and, when they change, the
    // text before fading out as the new one fades in, the width following.
    int m_clockParts = -1;
    std::wstring m_clockBefore;
    double m_clockSwap = 0;
    double m_clockSwapVelocity = 0;
    double m_clockWidth = -1;
    double m_clockWidthVelocity = 0;
    float m_clockWidthTarget = 0;
    // The clock's width as last measured, and for what (see Layout).
    std::wstring m_clockMeasuredKey;
    float m_clockMeasured = 0;
    std::wstring m_weatherMeasuredKey;
    // The lyrics' line on the pill measured (see PillGroup::Lyrics), for.
    std::wstring m_lyricsMeasuredKey;
    float m_lyricsMeasured = 0;
    float m_weatherMeasured = 0;
    // The app in front whose place is used (empty: the one for all apps).
    std::wstring m_placeApp;
    HWINEVENTHOOK m_foregroundHook = nullptr;
    const IslandPlace& CurrentPlace() const {
        static const IslandPlace kTop{L"", 0.5, 0};
        static const IslandPlace kNotches[] = {
            {L"", 0, 0}, {L"", 0.5, 0}, {L"", 1, 0}};
        if (m_bar) {
            return kTop;
        }
        if (m_notch >= 0) {
            return kNotches[m_notch];
        }
        if (const IslandPlace* place = FindAppPlace(m_placeApp)) {
            return *place;
        }
        return g_islandPlace;
    }
    // For the app in front: what's playing shows its buttons or its cover
    // and title (kAppMediaButtons, kAppMediaTitle), its room from the edges
    // and its size; its own choice, or the one for all apps.
    bool MediaPartWanted(int part) const {
        const IslandPlace* place = FindAppPlace(m_placeApp);
        if (place && place->media >= 0) {
            return (place->media & part) != 0;
        }
        return part == kAppMediaButtons ? g_mediaButtonsInPill.load()
                                        : g_mediaTitleInPill.load();
    }
    // What's playing's note on the pill, for the app in front (its own
    // choice: the group's icon) or for all apps; its buttons and its cover
    // and title show without it too.
    bool MediaIconWanted() const {
        const IslandPlace* place = FindAppPlace(m_placeApp);
        if (place && (place->own || (place->hidden &
                                     (1ull << (int)PillGroup::Media)))) {
            return !(place->hidden & (1ull << (int)PillGroup::Media));
        }
        return g_settings.islandMedia;
    }
    int GapWanted() const {
        // (A notch touches the edge.)
        if (m_notch >= 0) {
            return 0;
        }
        const IslandPlace* place = FindAppPlace(m_placeApp);
        return place && place->gap >= 0 && !m_bar ? place->gap
                                                  : g_settings.islandGap;
    }
    // The icons' scale: the island's, times their own size.
    float IconScale() const {
        return m_scale * g_settings.islandIconSize / 100.0f;
    }
    int SizeWanted() const {
        const IslandPlace* place = FindAppPlace(m_placeApp);
        return place && place->size >= 0 && !m_bar ? place->size
                                                   : g_settings.islandSize;
    }
    // Dragging it: pressed (it may become a drag), moved (it is one), where
    // the mouse holds the pill.
    bool m_dragArmed = false;
    bool m_dragMoved = false;
    POINT m_dragFrom{};
    POINT m_dragGrab{};
    void FinishDrag();
    // Faded away (over a full-screen app): kept off the screen rather than
    // hidden, since Windows' own animation for showing a window drew it as a
    // plain rectangle.
    bool m_away = true;
    bool m_shownOnce = false;
    // Showing and hiding (over full-screen apps): a dot fades in at the
    // pill's middle and opens into it; hiding, it closes back into the dot,
    // which fades away. 0 hidden, 1 shown (a bit more while bouncing).
    double m_appear = 0;
    double m_appearVelocity = 0;
    // The dot's opacity, and how far it has opened, from m_appear.
    float AppearFade() const {
        return (float)std::clamp(m_appear / 0.22, 0.0, 1.0);
    }
    float AppearOpen() const {
        return (float)std::clamp((m_appear - 0.18) / 0.82, 0.0, 1.12);
    }
    int m_ticks = 0;

    // Apps with notifications not looked at yet (from the notification
    // watch), shown with the badge apps (with the red dot) until the app comes
    // to the front, its icon or notification is clicked, or its
    // notifications are cleared; with the latest notification of each (to
    // open it). And whether some arrived since the list was last opened (the
    // bell's dot). Kept across restarts.
    struct UnreadApp {
        std::wstring id;
        ToastReply reply;
    };
    std::vector<UnreadApp> m_unreadApps;
    bool m_unseen = false;
    // Chat apps that don't send their messages to Windows' notifications
    // (the Chinese WeChat, WeCom, DingTalk) blink their tray icon instead
    // (#6074, #6215): one blinking shows its app on the pill with its dot,
    // like a notification (see CheckTrayBlinks). Each icon's last change,
    // how many came close together, and whether it's blinking now.
    struct TrayBlink {
        std::wstring key;
        UINT serial;
        bool hidden;
        double last;
        int changes;
        bool blinking;
    };
    std::vector<TrayBlink> m_trayBlinks;
    void CheckTrayBlinks();
    // The pill filling like a loading bar (see UpdateFill): wanted, how far
    // (and shown, following it), its color, with a band sliding along it,
    // and how much it shows.
    bool m_fillWanted = false;
    double m_fillTarget = 0;
    double m_fillShown = 0;
    double m_fillShownVelocity = 0;
    double m_fillAmount = 0;
    double m_fillAmountVelocity = 0;
    D2D1_COLOR_F m_fillColor{};
    bool m_fillMoving = false;
    // DrawFill's pill shape and glow brush, kept (made again when they
    // change).
    winrt::com_ptr<ID2D1RoundedRectangleGeometry> m_fillShape;
    D2D1_ROUNDED_RECT m_fillShapeRect{};
    winrt::com_ptr<ID2D1RadialGradientBrush> m_fillGlow;
    D2D1_COLOR_F m_fillGlowColor{};
    // The charge's fill moving on its timer (see UpdateFillTimer).
    bool m_chargeFillRunning = false;
    // (How far isn't known: the line under it only where the band is.)
    bool m_fillUnknown = false;
    // A download's state shown, and since when (an error shakes as it
    // comes); filling for a download (not the battery); and how it ended
    // (kFillEnd*: done flashes with a check, canceled drains back), since
    // when.
    int m_fillState = 0;
    double m_fillStateAt = 0;
    bool m_fillDownload = false;
    int m_fillEnd = 0;
    double m_fillEndAt = 0;
    void UpdateFill();
    void DrawFill(ID2D1RenderTarget* target,
                  ID2D1SolidColorBrush* brush,
                  const D2D1_RECT_F& rect,
                  float radius,
                  ID2D1Geometry* notch,
                  float opacity);
    // The visualizer: the default output's peak meter, its bars' heights (0
    // to 1), and whether its frames run (only while it's on the pill, seen,
    // and something plays).
    winrt::com_ptr<IAudioMeterInformation> m_vizMeter;
    std::array<float, kVizBars> m_vizLevels{};
    bool m_vizRunning = false;
    // The dragon's eyes (islandEyes, see EyesState): whether they're on the
    // pill and seen (their watch timer runs), whether something moves (and
    // frames run for it), the time gathered towards their next step, where the
    // mouse was when the timer last saw it move, and whether a notification
    // already excited them.
    EyesState m_eyes;
    bool m_eyesRunning = false;
    bool m_eyesMoving = false;
    bool m_eyesNotified = false;
    double m_eyesSince = 0;
    POINT m_eyesCursor{-1, -1};
    // Some sound on the output now (any app: also one that doesn't tell
    // Windows' media controls what it plays, like VLC), and when it was last
    // heard (see CheckAudioPlaying).
    bool m_audioPlaying = false;
    double m_audioHeardAt = 0;
    void CheckAudioPlaying();
    // The playing app's audio sessions' meters (for which app, looked for
    // again when, last heard when; see CheckAudioPlaying), and the loudest
    // peak of the last seconds.
    std::vector<winrt::com_ptr<IAudioMeterInformation>> m_vizSessionMeters;
    std::wstring m_vizApp;
    double m_vizLookAt = 0;
    double m_vizSessionHeardAt = 0;
    float m_vizLoudest = 0.05f;
    // The sound's level (see StepVisualizer): following it, up fast and
    // down slowly, and its average; where each bar is in its wave(s). With
    // the beat: each bar's speed since the last beat and its kick, and when
    // the last beat came (and how many).
    float m_vizFast = 0;
    float m_vizSlow = 0;
    std::array<double, kVizBars> m_vizPhases{0, 1.7, 3.1, 4.4, 2.3, 5.2};
    std::array<float, kVizBars> m_vizSpeeds{6.1f, 8.7f, 5.3f, 9.6f, 7.2f, 8.1f};
    std::array<float, kVizBars> m_vizKicks{};
    double m_vizBeatAt = 0;
    int m_vizBeats = 0;
    // Where each bar goes after the last reading, the time since it, and
    // since the bars were last moved.
    std::array<float, kVizBars> m_vizTargets{};
    double m_vizClock = 0;
    double m_vizSinceShown = 0;
    void UpdateVisualizerTimer();
    void UpdateFillTimer();
    void UpdateEyesTimer();
    void OnEyesTimer();
    bool StepEyes(double dt);
    void BlinkEyes();
    const IslandSlot* EyesSlot() const;
    bool EyesAlerted() const;
    EyesView EyesLook(const IslandSlot& slot) const;
    void StepVisualizer(double dt);
    void ReadVisualizerSound();
    std::vector<winrt::com_ptr<IAudioMeterInformation>> FindSessionMeters(
        const std::wstring& appId);
    // The folded apps (islandFoldApps) out, and how far (see Animate).
    bool m_appsOpen = false;
    double m_appsAmount = 0;
    double m_appsVelocity = 0;
    void OpenFoldedApps(UINT forMs);
    // Apps with a taskbar badge whose notifications were cleared in the
    // list: not shown until their badge goes away and comes back, or a new
    // notification of theirs arrives.
    std::vector<std::wstring> m_clearedBadges;
    // Apps whose banner was closed with its "x", and until when: like
    // Android's snoozing, their new notifications don't open the pill for a
    // while (their sound plays, and they still go to the list and the
    // island).
    std::vector<std::pair<std::wstring, double>> m_snoozedApps;
    bool IsSnoozed(const std::wstring& appId);
    void LoadUnread();
    void SaveUnread();
    void AddUnread(const IslandToast& toast);
    UnreadApp* FindUnread(const std::wstring& appId);
    // Snoozed notifications, and when they show again (see SnoozeToast).
    std::vector<std::pair<IslandToast, double>> m_snoozedToasts;
    // Apps with a notification badge (from the taskbar), and their icons.
    // Each grows in and out (`shown` from 0 to 1).
    std::vector<std::wstring> m_badgeApps;
    // Their badges' counts, and each app's notifications in the Windows
    // notification center (read as they change): the count on its icon.
    std::vector<int> m_badgeCounts;
    std::vector<std::pair<std::wstring, int>> m_centerCounts;
    void UpdateAppCounts();
    void ShowCenterCounts(
        const std::vector<std::pair<std::wstring, int>>& counts);
    int AppCount(const std::wstring& appId) const;
    struct AppEntry {
        std::wstring id;
        double shown = 0;
        double velocity = 0;
        bool present = true;
    };
    std::vector<AppEntry> m_appEntries;
    // The other apps' tray icons (from the taskbar), the ones pinned to show
    // always, and the others, shown when the list is open (m_trayAmount from
    // 0 to 1, animated).
    std::vector<TrayIcon> m_trayIcons;
    // Icons came once (later ones grow in).
    bool m_trayIconsSeen = false;
    // How pinned each icon is shown (0 in the list, 1 pinned), animated.
    struct PinState {
        std::wstring key;
        double amount;
        double velocity;
    };
    std::vector<PinState> m_pinStates;
    float PinAmount(const std::wstring& key) const;
    // The icon whose pin shows (see kPinDelayMs), and since when.
    std::wstring m_pinShownKey;
    double m_pinShownAt = 0;
    float PinShown(double now) const;
    bool m_trayOpen = false;
    // Kept open (the setting, or the arrow's right click).
    bool m_trayKeepOpen = false;
    double m_trayAmount = 0;
    double m_trayVelocity = 0;
    double m_trayBounce = 0;
    double m_trayBounceVelocity = 0;
    // The arrow turning from ‹ to › (0 to 1).
    double m_trayArrow = 0;
    // With many icons, the list stops at a few and the wheel scrolls it
    // (pixels along the pill); how far it can go, one icon's step, and
    // whether icons are hidden before or after the shown ones.
    double m_trayScroll = 0;
    double m_trayScrollVelocity = 0;
    double m_trayScrollTarget = 0;
    float m_trayOverflow = 0;
    float m_trayIconStep = 0;
    bool m_trayMoreBefore = false;
    bool m_trayMoreAfter = false;
    // The open list's rect, when it doesn't show every icon.
    const IslandSlot* TrayListSlot() const {
        for (const auto& slot : m_slots) {
            if (slot.font == kTrayListFont) {
                return &slot;
            }
        }
        return nullptr;
    }
    // Longer than its screen has room for: how much is out of sight, and
    // how far it's slid along (see Layout and ScrollPill).
    float m_pillOverflow = 0;
    double m_pillScroll = 0;
    double m_pillScrollTarget = 0;
    double m_pillScrollVelocity = 0;
    // The wheel on a pill longer than its screen: forward (positive) or
    // back, in notches.
    bool ScrollPill(double notches) {
        if (m_pillOverflow <= 0) {
            return false;
        }
        m_pillScrollTarget =
            std::clamp(m_pillScrollTarget +
                           notches * 2 * kIslandIconSlot * IconScale(),
                       0.0, (double)m_pillOverflow);
        StartAnimating();
        return true;
    }
    // The wheel over the tray: forward (positive) or back, in notches.
    bool ScrollTrayList(double notches) {
        if (!m_trayOpen || m_trayOverflow <= 0 ||
            (m_hover != IslandItem::TrayIcon &&
             m_hover != IslandItem::TrayPin &&
             m_hover != IslandItem::Overflow)) {
            return false;
        }
        m_trayScrollTarget =
            std::clamp(m_trayScrollTarget + notches * 2 * m_trayIconStep, 0.0,
                       (double)m_trayOverflow);
        StartAnimating();
        return true;
    }
    double m_trayArrowVelocity = 0;
    // The tooltip waits a moment, like the taskbar's.
    std::wstring m_pendingTipKey;
    // An app's window opened from its tray icon, moved under the island.
    HWINEVENTHOOK m_trayAppHooks[2]{};
    POINT m_trayAppAnchor{-1, -1};
    // The app whose window is waited for (its process).
    DWORD m_trayAppProcess = 0;
    // The Windows banner, moved away while the pill shows it.
    HWINEVENTHOOK m_bannerHook = nullptr;
    HWINEVENTHOOK m_bannerMoveHook = nullptr;
    void PublishTrayPlaces();
    // What was last sent to explorer (see PublishTrayPlaces).
    std::vector<BYTE> m_sentTrayPlaces;
    std::vector<std::tuple<std::wstring, UINT, winrt::com_ptr<ID2D1Bitmap>>>
        m_trayBitmaps;
    // The name of the icon under the mouse (a tray icon's tip, or an app's
    // name with how many notifications it has), drawn like the island's
    // bubbles in a window of its own beside the pill: whose it is, its text
    // laid out, where it grows from on the screen and towards which side
    // (see TipSide), and when it came and when it goes (see TipAmount).
    enum class TipSide { Under, Over, Right, Left };
    std::wstring m_tooltipKey;
    winrt::com_ptr<IDWriteTextLayout> m_tipLayout;
    POINT m_tipAnchor{};
    TipSide m_tipSide = TipSide::Under;
    double m_tipShownAt = 0;
    double m_tipHideAt = 0;
    HWND m_tipWnd = nullptr;
    LayeredCanvas m_tipCanvas;
    std::wstring m_tipDrawnKey;
    float TipAmount(double now) const;
    void HideTrayTip();
    void RenderTrayTip();
    // A new notification shown in the pill: it stretches into a banner
    // (m_bannerAmount from 0 to 1, a bit more while bouncing), stays a few
    // seconds (longer under the mouse), and goes back.
    IslandToast m_banner;
    // New notifications of other chats while a reply is typed: shown after,
    // one by one (the next when a banner goes).
    std::vector<IslandToast> m_waitingToasts;
    // Some panel was open at the last frame (see AnimationFrame).
    bool m_panelWasOpen = false;
    bool m_bannerWanted = false;
    // The Windows banners (see OnWindowsBanner). Windows 11 shows them in a
    // stack, in one window: it's uncloaked when the first comes, the next
    // ones join it, and it's cloaked when the last goes. The island can only
    // move the whole window, so it's off the screen only while every banner
    // in it is one the island shows: the notifications the island reads
    // while no stack shows are the next stack's first ones (kept a few
    // seconds), and one read while a stack shows has joined it. A call's,
    // an alarm's or a reminder's, or one the island can't show now, brings
    // the stack back at once, until it ends. A stack with none read yet
    // waits a moment, hidden, for the island to read its notification,
    // which can come a little later.
    double m_bannerShownAt = -100;
    // Where Windows keeps the stack's bottom (it grows upwards from there).
    LONG m_bannerHomeBottom = 0;
    struct OwedBanner {
        // Until when it's expected, and whether it goes.
        double until;
        bool take;
    };
    std::vector<OwedBanner> m_bannersOwed;
    bool m_stackShowing = false;
    // Off the screen (the island's), waiting for the island to read its
    // notification, or kept on the screen until it ends.
    bool m_stackHidden = false;
    bool m_stackWaiting = false;
    bool m_stackMustShow = false;
    // When a Windows banner last showed (whatever the island did with it).
    double m_windowsBannerSeenAt = -100;
    bool CanTakeBanners() const;
    // Whether an island that shows them can take them now.
    bool AnyIslandTakesBanners() const;
    // The same notification closed on the other islands showing it (opened,
    // deleted or minimized on this one).
    void CloseBannerElsewhere();
    void OweWindowsBanner(bool take);
    double BannerRestoreSeconds() const;
    void RestoreWindowsBanner();
    // The stack has one the island doesn't show: back on the screen, and
    // kept there until it ends.
    void KeepWindowsBannersShown() {
        m_stackMustShow = true;
        RestoreWindowsBanner();
    }
    // With the island's own notifications, the call, alarm or reminder on
    // the pill takes over Windows' banner for it (see WindowsBannerView):
    // hidden while the pill shows it, with its buttons on the pill. Ends
    // (and Windows' banner comes back) as soon as the pill doesn't show it,
    // unless one of Windows' buttons was pressed (it goes by itself then,
    // checked a moment later).
    bool m_stackTakeover = false;
    bool m_takeoverPressed = false;
    bool m_takeoverDismiss = false;
    std::vector<ToastReply::Action> m_bannerOwnActions;
    LONG m_stackHeight = 0;
    int m_bannerReadsBusy = 0;
    bool m_bannerReadAgain = false;
    int m_bannerReadRetries = 0;
    HANDLE m_bannerReadStop = nullptr;
    std::vector<HANDLE> m_bannerReadThreads;
    void ReadWindowsBanners(DWORD delayMs);
    void PressWindowsBanner(int button, const std::wstring& label);
    void StartWindowsBannerThread(DWORD delayMs,
                                  int press,
                                  const WindowsBannerRead& fallback);
    void EndTakeover();
    bool WantsTakeover() const;
    // A new stack, with the island's own notifications, is hidden at once
    // while the island can show calls, alarms and reminders in its pill;
    // when the stack last changed (one joining waits a moment for the
    // island to read it).
    bool CanTakeOverAhead() const;
    double m_stackChangedAt = -100;
    void OnBannerRestoreTimer();
    // The reply row: 0 hidden, 1 shown (under the mouse, or while typing).
    double m_bannerOpen = 0;
    double m_bannerOpenVelocity = 0;
    // Under the mouse (or while typing), the text on up to three lines (0
    // to 1, animated); how many it takes, and how many the reply takes
    // (wrapped, up to five shown), measured for what's shown.
    double m_bannerExpand = 0;
    double m_bannerExpandVelocity = 0;
    int m_bannerTextLines = 1;
    int m_bannerReplyLines = 1;
    std::wstring m_bannerMeasured;
    void MeasureBanner();
    float BannerTextExtra() const;
    float BannerReplyExtra() const;
    // The banner's size chosen (islandBannerSize), the island's scale times
    // it, and its width at a scale (islandBannerWidth too).
    static float BannerSizeFactor() {
        return kBannerSizes[std::clamp(g_settings.islandBannerSize, 0,
                                       (int)std::size(kBannerSizes) - 1)] /
               100.0f;
    }
    float BannerScale() const { return m_scale * BannerSizeFactor(); }
    // Its corners' radius (islandBannerCorners), as round as the panels'.
    float BannerRadius() const {
        return kIslandPanelRadius * BannerScale() * PanelCornerShare() *
               kBannerCornerShares[std::clamp(
                   g_settings.islandBannerCorners, 0,
                   (int)std::size(kBannerCornerShares) - 1)] /
               100.0f;
    }
    // At a point of the screen (islandBannerPlace), in a window of its own
    // (see RenderPlacedBanner), rather than out of the pill.
    static bool BannerPlaced() { return g_settings.islandBannerPlace > 0; }
    // How far the pill has turned into the banner (not at all when it's
    // placed elsewhere).
    float BannerMorph() const {
        return BannerPlaced()
                   ? 0.0f
                   : (float)std::clamp(m_bannerAmount, -0.05, 1.08);
    }
    D2D1_RECT_F PlacedBannerRect(float width, float height) const;
    void DrawBannerContents(ID2D1RenderTarget* target,
                            ID2D1SolidColorBrush* brush,
                            float contents);
    void RenderPlacedBanner();
    // The placed banner's window, and where it is on the screen.
    HWND m_bannerWnd = nullptr;
    POINT m_bannerWndAt{};

   public:
    // Its window's messages: its mouse goes to the island (see
    // RenderPlacedBanner).
    LRESULT HandleBannerWindow(HWND hWnd,
                               UINT msg,
                               WPARAM wParam,
                               LPARAM lParam);

   private:
    static float BannerWidthAt(float scale) {
        return kBannerWidth * scale * BannerSizeFactor() *
               kBannerWidths[std::clamp(g_settings.islandBannerWidth, 0,
                                        (int)std::size(kBannerWidths) - 1)] /
               100.0f;
    }
    // The most those take (the window has room for it).
    float BannerExtraRoom() const {
        return (m_bannerTextLines - 1) * std::round(18 * BannerScale()) +
               (BannerCanReply() ? BannerReplyExtra() : 0);
    }
    // A call's, an alarm's or a reminder's buttons, in a row under it
    // (instead of the reply box).
    bool BannerHasActions() const {
        return (m_banner.urgent || m_banner.glyph) &&
               !m_banner.reply.actions.empty();
    }
    // A button of the island's own notice (a timer's, an alarm's; see
    // CheckTimer).
    void OnOwnNoticeAction(const std::wstring& arguments);
    bool BannerCanReply() const {
        return m_banner.reply.CanReply() && !BannerHasActions();
    }
    D2D1_RECT_F BannerActionRect(size_t index) const;
    // A call's, an alarm's or a reminder's sound (see QueueUrgentSound).
    std::wstring m_urgentSoundApp;
    std::wstring m_urgentSound;
    bool m_urgentSoundLoop = false;
    double m_urgentSoundAt = -100;
    bool m_urgentRinging = false;
    void QueueUrgentSound(const IslandToast& toast);
    void PlayUrgentSound();
    void StopUrgentSound();
    // New notifications waiting for the pill: calls, alarms and reminders
    // first.
    void QueueToasts(std::vector<IslandToast> urgent,
                     std::vector<IslandToast> others);
    bool m_bannerTyping = false;
    std::wstring m_bannerReplyText;
    TextEdit m_bannerEdit;
    double m_bannerAmount = 0;
    double m_bannerVelocity = 0;
    // Another notification taking the banner's place: its contents fade in
    // (1 to 0).
    double m_bannerSwap = 0;
    double m_bannerSwapVelocity = 0;
    winrt::com_ptr<IDWriteTextFormat> m_bannerAppFormat;
    winrt::com_ptr<IDWriteTextFormat> m_bannerTimeFormat;
    winrt::com_ptr<IDWriteTextFormat> m_bannerTitleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_bannerTextFormat;
    // The upright clock's date (under its time).
    winrt::com_ptr<IDWriteTextFormat> m_clockDateFormat;
    // A name beside its icon on the pill (see kNamedItemFont).
    winrt::com_ptr<IDWriteTextFormat> m_itemNameFormat;
    // An app that just got a badge while the pill is minimized peeks out.
    std::wstring m_peekApp;
    double m_peekStart = 0;
    float PeekAmount(double now);
    std::vector<KeptAppIcon> m_appBitmaps;
    winrt::com_ptr<IWICImagingFactory> m_wic;
    // The banner's picture (see IslandToast::image), loaded for it.
    ID2D1Bitmap* GetBannerImage();
    std::wstring m_bannerImagePath;
    winrt::com_ptr<ID2D1Bitmap> m_bannerImage;

    // Minimizing (pill only): 1 expanded, 0 a thin line, animated.
    bool m_minimized = false;
    // The line growing under the mouse (0 to 1).
    double m_lineHover = 0;
    double m_lineHoverVelocity = 0;
    // The line fading away after a while (islandLineFade): 1 seen, 0 gone;
    // since when it's been left alone (minimized, no mouse, no banner).
    double m_lineFade = 1;
    double m_lineFadeVelocity = 0;
    double m_lineAloneSince = 0;
    bool LineFadeDue(double now) const;
    double m_expand = 1;
    double m_expandVelocity = 0;
    bool m_animating = false;
    double m_lastFrame = 0;
    bool m_freshStart = false;

    // Told of changes (see StartStatusWatch).
    void StartStatusWatch();
    void StopStatusWatch();
    void WatchVolume();
    // The default microphone (for communications too): muted or not, told
    // when it changes like the volume.
    void WatchMicrophone();
    winrt::com_ptr<IAudioEndpointVolume> m_watchedMicrophone;
    std::wstring MicrophoneGlyph();
    std::wstring m_microphoneGlyph;
    StatusEvents* m_statusEvents = nullptr;
    winrt::com_ptr<IAudioEndpointVolume> m_watchedVolume;
    DWORD m_networkCookie = 0;
    winrt::com_ptr<IConnectionPoint> m_networkPoint;
    bool m_wlanWatched = false;
    HPOWERNOTIFY m_powerNotifications[3]{};
    void* m_dndSubscription = nullptr;
    int m_raiseTicks = 0;

    // What's shown; a new frame is drawn only when it changes.
    std::wstring m_clock;
    std::wstring m_networkGlyph;
    std::wstring m_volumeGlyph;
    std::wstring m_batteryGlyph;
    std::wstring m_bellGlyph;
    std::wstring m_drawnKey;
    // An icon changing (the volume, the battery, the bell, the microphone):
    // the one before shrinks and fades out as the new one grows in (t from 1
    // to 0).
    struct GlyphSwap {
        IslandItem item;
        std::wstring shown;
        std::wstring from;
        double t;
        double velocity;
    };
    std::vector<GlyphSwap> m_glyphSwaps;
    // The red dots (updates waiting, unseen notifications) growing in and
    // out, and the count shown while the updates' one goes.
    double m_updatesDot = 0;
    double m_updatesDotVelocity = 0;
    int m_updatesDotCount = 0;
    double m_unseenDot = 0;
    double m_unseenDotVelocity = 0;
    // The weather's room on the pill following what it shows, and what it
    // showed before fading out (t from 1 to 0).
    double m_weatherWidth = -1;
    double m_weatherWidthVelocity = 0;
    float m_weatherWidthTarget = 0;
    std::wstring m_weatherBefore;
    double m_weatherSwap = 0;
    double m_weatherSwapVelocity = 0;
    // The microphone's icon as last shown, so it shrinks away when the
    // microphone goes.
    std::wstring m_microphoneShown;
    // The room from the screen's edges, following the setting smoothly.
    double m_gapShown = -1;
    double m_gapVelocity = 0;
    // The group right after the clock (kPillGroups: none, -1 not laid out
    // yet): with the clock kept in the middle, where it goes among the others
    // (see Layout).
    int m_clockAfter = -1;
    // The order last laid out (the app in front may have its own): another
    // one slides the groups to their new places.
    std::vector<PillGroup> m_layoutOrder;
    // A restart after a lost device waits for this timer (see OnDeviceLost).
    bool m_deviceRetryPending = false;
    // The glass made again so many times since the island started (see
    // kGlassRenewMs).
    int m_glassRenewals = 0;
    // The monitor it's on (see WantedMonitor), and moving to another one: it
    // closes into its dot (1), the dot glides there (2), and it opens again
    // (m_moveT from 0 to 1 while gliding, from where it was).
    HMONITOR m_monitorHandle = nullptr;
    std::vector<MonitorEntry> m_displayMonitors;
    int m_monitorMove = 0;
    HMONITOR m_moveTarget = nullptr;
    POINT m_moveFrom{};
    double m_moveT = 0;
    double m_moveTVelocity = 0;
    HMONITOR WantedMonitor() const;
    // The mouse on another monitor goes there only once it stays a moment
    // (not just passing over an edge).
    HMONITOR m_wantedMonitor = nullptr;
    double m_wantedSince = 0;
    void CheckMonitor();
    void SwitchMonitor(HMONITOR monitor);
    void UpdateMonitorWatch();
    // The islands (one on each monitor) no longer on the monitors there are.
    bool IslandMonitorsChanged() const;

   public:
    // Another island than the main one, on its own monitor (see g_islands),
    // with its profile (or -1, the main island's settings).
    void SetOtherMonitor(HMONITOR monitor,
                         const std::wstring& device,
                         int profile) {
        m_main = false;
        m_ownMonitor = monitor;
        m_ownDevice = device;
        m_profileId = profile;
        if (profile >= 0) {
            m_look = LookFromProfile(profile);
        }
    }
    bool IsMainIsland() const { return m_main; }
    // Its settings are the main island's (a notch: its monitor's island's).
    bool HasMainSettings() const {
        return m_notchBase ? m_notchBase->m_main : m_main;
    }
    HWND Window() const { return m_hwnd; }
    const std::wstring& OwnDevice() const { return m_ownDevice; }
    int MonitorProfile() const { return m_profileId; }
    bool HasOwnLook() const { return m_profileId >= 0; }
    // The island whose settings it shows (a notch: its base island).
    Island* SettingsIsland() { return m_notchBase ? m_notchBase : this; }
    // Its look kept in its profile (changed on it, see StoreProfileInt), and
    // shown by the other islands with it; not `show` as the islands go.
    void SaveOwnProfile(bool show = true);
    // Its look made again from its profile and laid out (the islands start
    // again for another kind of window or other notches).
    void ReloadLook();
    // Its monitor's profile chosen (on its own settings: not the main
    // island's), or -1.
    void SetMonitorProfile(int id);
    // Its look made again from its profile (the main settings or the
    // profiles changed).
    void RefreshLook();
    // The profile in use saved a moment later (see SaveActiveProfileSoon),
    // or not.
    void SaveProfileSoon() {
        if (m_hwnd) {
            SetTimer(m_hwnd, kProfileSaveTimerId, kProfileSaveMs, nullptr);
            m_profileSavePending = true;
        }
    }
    void CancelProfileSave() {
        if (m_hwnd) {
            KillTimer(m_hwnd, kProfileSaveTimerId);
        }
        m_profileSavePending = false;
    }
    // Becoming the current island, or no longer: its place for all apps (its
    // monitor's) put in and taken out of g_islandPlace.
    void KeepContext() {
        m_contextPlace = g_islandPlace;
        m_contextKept = true;
        if (m_lookApplied) {
            m_look = CaptureLook();
            PutLook(g_mainLook);
            m_lookApplied = false;
        }
    }
    void TakeContext() {
        if (m_contextKept) {
            g_islandPlace = m_contextPlace;
        }
        if (!m_placeDevice.empty()) {
            UseMonitorPlace(m_placeDevice);
        }
        if (HasOwnLook()) {
            g_mainLook = CaptureLook();
            PutLook(m_look);
            m_lookApplied = true;
        }
    }

   private:
    bool m_main = true;
    HMONITOR m_ownMonitor = nullptr;
    // The taskbar's edge last seen (see Tick).
    UINT m_taskbarEdge = ABE_BOTTOM;
    std::wstring m_ownDevice;
    int m_profileId = -1;
    IslandLook m_look;
    bool m_lookApplied = false;
    // The profile in use is waiting to be saved (see SaveProfileSoon): done
    // before the island goes, if a restart comes first.
    bool m_profileSavePending = false;
    IslandPlace m_contextPlace;
    bool m_contextKept = false;
    // Its monitor's name (for its place there).
    std::wstring m_placeDevice;
};

Island* SwitchIsland(Island* island) {
    Island* previous = g_island;
    if (island && island != previous) {
        if (previous) {
            previous->KeepContext();
        }
        g_island = island;
        island->TakeContext();
    }
    return previous;
}

// Runs `action` for each island, as the current one (the one there is,
// before they're all made).
template <typename Action>
void ForEachIsland(Action action) {
    if (g_islands.empty()) {
        if (g_island) {
            action(*g_island);
        }
        return;
    }
    for (Island* island : std::vector<Island*>(g_islands)) {
        IslandScope scope(island);
        action(*island);
    }
}

void Island::FollowShelf(bool added) {
    if (m_shelfPanel.IsOpen()) {
        if (Shelf().empty()) {
            m_shelfPanel.Close();
        } else {
            m_shelfPanel.Refresh(added);
        }
    }
    Relayout();
}

void ShelfPanel::AfterChange() {
    ForEachIsland([](Island& island) { island.FollowShelf(false); });
}

void ShelfPanel::AfterPrune() {
    ForEachIsland([](Island& island) { island.Relayout(); });
}

// Every glass's window made again (see g_glassGeneration): a while after the
// island starts, when the displays change, and when Windows' transparency
// effects or the energy saver let the glass show again.
void RenewAllGlass() {
    if (!LiquidGlassOn()) {
        return;
    }
    g_glassGeneration++;
    ForEachIsland([](Island& island) { island.RenewGlass(); });
}

Island* MainIsland() {
    return g_islands.empty() ? g_island : g_islands.front();
}

// Whether some island wants it, each as itself (a monitor can show a profile
// of its own): the main island runs what they share then, for all of them.
template <typename Wanted>
bool AnyIslandLook(Wanted wanted) {
    bool any = false;
    ForEachIsland([&](Island& island) { any = any || wanted(island); });
    return any;
}

// The other monitors' islands (their profiles are chosen from the main
// island's settings too).
std::vector<Island*> OtherIslands() {
    std::vector<Island*> others;
    for (Island* island : g_islands) {
        if (!island->IsMainIsland() && !island->IsNotchSibling()) {
            others.push_back(island);
        }
    }
    return others;
}

bool OwnLookShown() {
    return g_island && g_island->HasOwnLook();
}

bool OtherIslandShown() {
    return g_island && !g_island->HasMainSettings();
}

void SaveOwnLookSoon() {
    if (g_island) {
        g_island->SaveProfileSoon();
    }
}

// Whether `island` shows the main island's settings (none: the main ones).
bool IslandHasMainSettings(const Island* island) {
    return !island || island->HasMainSettings();
}

// What every island shares, whatever profile it shows (the main island runs
// it for all of them): always in the main settings.
bool IsSharedPref(IslandPref pref) {
    switch (pref) {
        case IslandPref::AllMonitors:
        case IslandPref::FollowMouse:
        case IslandPref::Animations:
        case IslandPref::HideWindowsBanners:
        case IslandPref::OwnNotifications:
        case IslandPref::ShortcutNotifications:
        case IslandPref::ShortcutControl:
        case IslandPref::MinimizeKey:
        case IslandPref::CapsLock:
        case IslandPref::ExternalBrightness:
            return true;
        default:
            return false;
    }
}

bool IsLookValue(PCWSTR name) {
    for (const auto& info : kIslandPrefs) {
        if (info.key && wcscmp(info.key, name) == 0) {
            return !IsSharedPref(info.pref);
        }
    }
    for (PCWSTR look :
         {L"islandSize", L"islandGap", L"islandOpacity", L"islandAutoHideDelay",
          L"islandNotchCurve", L"islandIconSize", L"islandLineOpacity",
          L"islandActivationWidth", L"islandPanelSize",
          L"islandMediaTitleLimit", L"islandPillHeight", L"islandPillCorners",
          L"islandPillEnds", L"islandEdge", L"islandGlassBlur",
          L"islandTrayColumns", L"islandVizSensitivity", L"islandTextSize",
          L"trayListIsland"}) {
        if (wcscmp(look, name) == 0) {
            return true;
        }
    }
    return std::any_of(
        std::begin(kCustomColorKeys), std::end(kCustomColorKeys),
        [name](PCWSTR key) { return wcscmp(key, name) == 0; });
}

HWND IslandWindow(const Island* island) {
    return island ? island->Window() : nullptr;
}

int ChosenProfile() {
    return g_island && !g_island->HasMainSettings() ? g_island->MonitorProfile()
                                                 : g_activeProfile;
}

// The island whose tray icons explorer knows the places of (the mouse was on
// it last), or none yet (the main one).
Island* g_trayIsland;

LRESULT CALLBACK IslandWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                               LPARAM lParam) {
    auto* island = (Island*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (!island) {
        island = g_island;
    }
    if (island) {
        IslandScope scope(island);
        return island->HandleMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// The control center.

void ControlPanel::Prepare(float scale) {
    if (!m_wic) {
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(m_wic.put()));
    }
    Update();
    if (scale != m_preparedScale) {
        PrepareFormats(scale);
    }
    // (After the formats: the settings' names are measured with them.)
    Layout();
}

void ControlPanel::PrepareFormats(float scale) {
    m_preparedScale = scale;
    m_wrappedHeights.clear();
    // (Their size goes with the scale.)
    m_lyricsAppIcons.clear();
    // In the window of its own, the sizes of Windows 11's Settings: 14 for
    // the names, 12 for what they do, 28 for the page's title.
    const bool window = m_windowed;
    m_iconFormat = MakeFormat(IconFont(), DWRITE_FONT_WEIGHT_NORMAL,
                              (window ? 16 : 15) * scale);
    m_titleFormat = MakeFormat(L"Segoe UI Variable Text",
                               DWRITE_FONT_WEIGHT_SEMI_BOLD,
                               (window ? 14 : 13) * scale,
                               DWRITE_TEXT_ALIGNMENT_LEADING);
    m_subtitleFormat =
        MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                   (window ? 14 : 11.5f) * scale,
                   DWRITE_TEXT_ALIGNMENT_LEADING);
    m_sectionFormat = MakeFormat(L"Segoe UI Variable Text",
                                 DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                 (window ? 14 : 13) * scale,
                                 DWRITE_TEXT_ALIGNMENT_LEADING);
    m_pageTitleFormat = MakeFormat(L"Segoe UI Variable Display",
                                   DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                   (window ? 28 : 22) * scale,
                                   DWRITE_TEXT_ALIGNMENT_LEADING);
    m_percentFormat =
        MakeFormat(L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL,
                   (window ? 13 : 12) * scale, DWRITE_TEXT_ALIGNMENT_TRAILING);
    m_linkFormat = MakeFormat(L"Segoe UI Variable Text",
                              DWRITE_FONT_WEIGHT_NORMAL,
                              (window ? 13 : 12) * scale);
    m_smallFormat = MakeFormat(L"Segoe UI Variable Text",
                               DWRITE_FONT_WEIGHT_NORMAL,
                               (window ? 12 : 10.5f) * scale);
    m_badgeFormat = MakeFormat(IconFont(), DWRITE_FONT_WEIGHT_NORMAL,
                               9 * scale);
    m_smallIconFormat = MakeFormat(IconFont(), DWRITE_FONT_WEIGHT_NORMAL,
                                   (window ? 14 : 13) * scale);
    m_komorebiIconFormat = MakeFormat(IconFont(),
                                      DWRITE_FONT_WEIGHT_NORMAL, 20 * scale);
}

bool ControlPanel::KomorebiShown() const {
    return m_only == Only::Komorebi ||
           (m_only == Only::All &&
            std::find(m_controls.begin(), m_controls.end(),
                      ControlId::Komorebi) != m_controls.end());
}

// Start while it's stopped, stop and reload while it runs; none while a
// command is running.
bool ControlPanel::KomorebiEnabled(int button) const {
    return !m_komorebic.empty() && m_komorebiBusy < 0 &&
           (button == 0 ? !m_komorebiRunning : m_komorebiRunning);
}

// A button's commands on a thread (not while another one runs).
void ControlPanel::RunKomorebi(int button) {
    if (m_komorebiThread) {
        if (WaitForSingleObject(m_komorebiThread, 0) == WAIT_TIMEOUT) {
            return;
        }
        StopKomorebiJob();
    }
    if (!StartKomorebiJob(KomorebiCommands(m_komorebic, button), button,
                          m_hwnd, &m_komorebiThread, &m_komorebiStop)) {
        return;
    }
    m_komorebiBusy = button;
    m_komorebiBusySince = NowSeconds();
    m_komorebiFailed = false;
}

void ControlPanel::StopKomorebiJob() {
    EndKomorebiJob(&m_komorebiThread, &m_komorebiStop);
}

// Read while its section is shown (with the rest, every couple of seconds
// while open).
void ControlPanel::UpdateKomorebi() {
    if (!m_komorebiLooked) {
        m_komorebiLooked = true;
        m_komorebic = FindKomorebic();
        m_komorebiVersion =
            m_komorebic.empty() ? L"" : KomorebiVersion(m_komorebic);
    }
    m_komorebiRunning = !m_komorebic.empty() && KomorebiRunning();
    // Its command done (it runs or stopped as asked; reloaded: running
    // again a moment later), or given up on.
    const double since = NowSeconds() - m_komorebiBusySince;
    if (m_komorebiBusy >= 0 &&
        ((m_komorebiBusy == 0 && m_komorebiRunning) ||
         (m_komorebiBusy == 1 && !m_komorebiRunning) ||
         (m_komorebiBusy == 2 && m_komorebiRunning && since > 3) ||
         since > 15)) {
        m_komorebiBusy = -1;
    }
}

void ControlPanel::OnOpen() {
    // (The settings' window shows only them, also when opened again.)
    m_view = m_openOnSettings || m_windowed ? View::Settings : View::Main;
    m_openOnSettings = false;
    const int openOnControl = m_openOnControl;
    m_openOnControl = -1;
    m_search.clear();
    m_searchEdit.Reset();
    m_settingsScroll.Reset();
    m_orderDrag = -1;
    m_orderDragStrip = -1;
    m_orderMoved = false;
    m_profileEdit = -1;
    m_profileTyping = false;
    m_profileDeleteArmed = false;
    m_backupStatus.clear();
    m_hover = {};
    m_dragging = false;
    m_editing = false;
    m_editAmount = 0;
    m_editAmountVelocity = 0;
    m_dragId = -1;
    m_motions.clear();
    m_ghosts.clear();
    m_styleKnob = -1;
    m_viewIn = 1;
    m_viewInVelocity = 0;
    // (Installed or removed meanwhile: looked for again.)
    m_komorebiLooked = false;
    m_toolsLooked = false;
    if (KomorebiShown()) {
        UpdateKomorebi();
    }
    Layout();
    if (m_view == View::Settings) {
        SettleSettings();
    }
    m_wifiStatus.clear();
    m_darkMode = IsDarkMode();
    // (The settings' window: none of the control center's.)
    if (m_windowed) {
        return;
    }
    // (Before a view is opened: Wi-Fi's looks for the networks.)
    StartWifi();
    if (openOnControl >= 0) {
        OpenControlView((ControlId)openOnControl);
        // (Opening already: no slide in.)
        m_viewIn = 1;
        m_viewInVelocity = 0;
    }
    // The radios' and the brightness's state come a moment later.
    SwitchRadio(-1, false);
    // The monitor is asked once, and again only after a while (asking over
    // DDC/CI can make some monitors flicker), or once a monitor came or went
    // (each slider knows its monitor by its place in the list).
    // A monitor just plugged in answers only after a few seconds: fewer
    // listed than there are (counted without asking them) is read again.
    // One turned off (not answering a level set) is read again too.
    // (With the external monitors' DDC/CI off there are none to count: not
    // even counting them talks to the monitors then, see
    // g_externalBrightness.)
    const ULONGLONG now = GetTickCount64();
    int physical = 0;
    ForEachPhysicalMonitor(
        [&](const PHYSICAL_MONITOR&, int, HMONITOR, int) { physical++; });
    const bool missed =
        !m_brightnessJob->running && m_brightnessJob->missed.exchange(false);
    if (!m_brightnessJob->running &&
        (!m_brightnessReadAt || now - m_brightnessReadAt > 10 * 60 * 1000 ||
         m_brightnessDisplays != g_displayChanges || missed ||
         ((int)m_brightnessMonitors.size() < physical &&
          now - m_brightnessReadAt > 2000))) {
        m_brightnessReadAt = now;
        // (Not changed meanwhile through places that may be old: after a
        // monitor came or went.)
        if (m_brightnessDisplays != g_displayChanges) {
            m_brightnessMonitors.clear();
        }
        m_brightnessDisplays = g_displayChanges;
        StartBrightnessWorker(BrightnessListProc);
    }
}

// A backup's file dialog: saving `text` to the file picked, or reading the
// file picked into it.
struct BackupJob {
    HWND panel;
    bool save;
    std::wstring text;
    bool done = false;
    // The logs (see LogsText), not a backup.
    bool logs = false;
};

DWORD WINAPI BackupThreadProc(LPVOID parameter) {
    auto* job = (BackupJob*)parameter;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    winrt::com_ptr<IFileDialog> dialog;
    if (SUCCEEDED(CoCreateInstance(
            job->save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(dialog.put())))) {
        const COMDLG_FILTERSPEC filters[] = {
            {Tr(L"Island backup (*.txt)", L"Backup da ilha (*.txt)"),
             L"*.txt"},
            {Tr(L"All files", L"Todos os arquivos"), L"*.*"}};
        dialog->SetFileTypes(ARRAYSIZE(filters), filters);
        dialog->SetDefaultExtension(L"txt");
        if (job->save) {
            dialog->SetTitle(job->logs
                                 ? Tr(L"Save the island's logs",
                                      L"Salvar os logs da ilha")
                                 : Tr(L"Export the island's backup",
                                      L"Exportar o backup da ilha"));
            SYSTEMTIME now;
            GetLocalTime(&now);
            WCHAR name[64];
            swprintf_s(name, L"Mac Island & Dock%s %04d-%02d-%02d.txt",
                       job->logs ? L" logs" : L"", now.wYear, now.wMonth,
                       now.wDay);
            dialog->SetFileName(name);
        } else {
            dialog->SetTitle(Tr(L"Import a backup of the island",
                                L"Importar um backup da ilha"));
        }
        const HWND owner = CreateFileDialogOwner(job->panel);
        winrt::com_ptr<IShellItem> item;
        PWSTR path = nullptr;
        if (SUCCEEDED(dialog->Show(owner)) &&
            SUCCEEDED(dialog->GetResult(item.put())) &&
            SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
            if (job->save) {
                // UTF-8 with its mark.
                const int length =
                    WideCharToMultiByte(CP_UTF8, 0, job->text.data(),
                                        (int)job->text.size(), nullptr, 0,
                                        nullptr, nullptr);
                std::string bytes = "\xEF\xBB\xBF";
                bytes.resize(3 + std::max(length, 0));
                WideCharToMultiByte(CP_UTF8, 0, job->text.data(),
                                    (int)job->text.size(), bytes.data() + 3,
                                    length, nullptr, nullptr);
                HANDLE file = CreateFile(path, GENERIC_WRITE, 0, nullptr,
                                         CREATE_ALWAYS, 0, nullptr);
                if (file != INVALID_HANDLE_VALUE) {
                    DWORD written = 0;
                    job->done = WriteFile(file, bytes.data(),
                                          (DWORD)bytes.size(), &written,
                                          nullptr) &&
                                written == bytes.size();
                    CloseHandle(file);
                }
            } else {
                job->done = ReadTextFile(path, 8 * 1024 * 1024, &job->text);
            }
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
    if (!PostMessage(job->panel, WM_APP_BACKUP_DONE, 0, (LPARAM)job)) {
        delete job;
    }
    return 0;
}

// Windows' color dialog for a custom color (on a thread of its own, like the
// backup's file dialog), starting from the one it has.
struct ColorJob {
    HWND panel;
    int color;
    // 0xRRGGBB.
    int value;
    bool done = false;
};

DWORD WINAPI ColorThreadProc(LPVOID parameter) {
    auto* job = (ColorJob*)parameter;
    const HWND owner = CreateFileDialogOwner(job->panel);
    COLORREF custom[16] = {};
    CHOOSECOLOR choose{sizeof(choose)};
    choose.hwndOwner = owner;
    choose.rgbResult = RGB((job->value >> 16) & 0xFF, (job->value >> 8) & 0xFF,
                           job->value & 0xFF);
    choose.lpCustColors = custom;
    choose.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColor(&choose)) {
        job->value = (GetRValue(choose.rgbResult) << 16) |
                     (GetGValue(choose.rgbResult) << 8) |
                     GetBValue(choose.rgbResult);
        job->done = true;
    }
    if (owner) {
        DestroyWindow(owner);
    }
    if (!PostMessage(job->panel, WM_APP_COLOR_DONE, 0, (LPARAM)job)) {
        delete job;
    }
    return 0;
}

// The next keys pressed go to the panel (see ShortcutKeyProc), or (`start`
// false) no more.
void ControlPanel::StartShortcutRecording(bool start) {
    if (start == m_recordingShortcut) {
        return;
    }
    m_recordingShortcut = start;
    // (Another island's settings may be recording now: left as they are.)
    if (start || g_shortcutRecorder == m_hwnd) {
        g_shortcutRecorder = start ? m_hwnd : nullptr;
        g_shortcutRecording = start;
    }
    UpdateShortcutHook();
}

void ControlPanel::OnShortcutRecorded(int combo) {
    // (Without Win, Ctrl or Alt it'd take the key from every app: it waits
    // for another.)
    if (combo >= 0 &&
        !((combo >> 8) & (kShortcutWin | kShortcutCtrl | kShortcutAlt))) {
        m_shortcutNote = Tr(L"use Win, Ctrl or Alt with a key",
                            L"use Win, Ctrl ou Alt com uma tecla");
        Layout();
        return;
    }
    StartShortcutRecording(false);
    m_shortcutNote = combo < 0 ? std::wstring() : SetMinimizeShortcut(combo);
    Layout();
}

// The island going away with a dialog still open: it's closed, and its job
// (posted to this window, about to go) freed here.
void ControlPanel::OnDestroy() {
    StartShortcutRecording(false);
    m_lyricsAppIcons.clear();
    StopWifi();
    const bool backup = m_backupThread != nullptr;
    const bool color = m_colorThread != nullptr;
    const bool icons = m_iconThread != nullptr;
    EndFileDialogThread(&m_backupThread);
    EndFileDialogThread(&m_colorThread);
    EndFileDialogThread(&m_iconThread);
    MSG msg;
    if (icons && PeekMessage(&msg, m_hwnd, WM_APP_FOLDER_PICKED,
                             WM_APP_FOLDER_PICKED, PM_REMOVE)) {
        delete (FolderPickJob*)msg.lParam;
    }
    if (backup && PeekMessage(&msg, m_hwnd, WM_APP_BACKUP_DONE,
                              WM_APP_BACKUP_DONE, PM_REMOVE)) {
        delete (BackupJob*)msg.lParam;
    }
    if (color && PeekMessage(&msg, m_hwnd, WM_APP_COLOR_DONE,
                             WM_APP_COLOR_DONE, PM_REMOVE)) {
        delete (ColorJob*)msg.lParam;
    }
}

// One dialog at a time (the panel stays open while one shows).
void ControlPanel::StartColorDialog(int color) {
    if (m_dialogShown) {
        return;
    }
    // (An icon's: its own color, or the icons' one.)
    const int icon =
        color >= kIconColorDialog
            ? IconColorOf((WCHAR)(color - kIconColorDialog))
            : -1;
    auto* job = new ColorJob{
        m_hwnd, color,
        color >= kIconColorDialog
            ? (icon >= 0 ? icon : g_customColors[kColorIcons])
        : color == kLyricsColorDialog     ? g_settings.islandLyricsOwnColor
        : color == kVisualizerColorDialog ? g_settings.islandVisualizerOwnColor
        : color >= kFillColorDialog ? FillChoiceRgb(color - kFillColorDialog, 3)
                                    : g_customColors[color]};
    m_dialogShown = true;
    m_colorThread = CreateThread(nullptr, 0, ColorThreadProc, job, 0, nullptr);
    if (!m_colorThread) {
        m_dialogShown = false;
        delete job;
    }
}

// The calendar's color dialog, like the control center's.
void CalendarPanel::StartColorDialog(const std::wstring& source, int color) {
    if (m_dialogShown) {
        return;
    }
    auto* job = new ColorJob{m_hwnd, 0, color};
    m_colorSource = source;
    m_dialogShown = true;
    m_colorThread = CreateThread(nullptr, 0, ColorThreadProc, job, 0, nullptr);
    if (!m_colorThread) {
        m_dialogShown = false;
        delete job;
    }
}

// (Only from its thread: the job is its.)
bool CalendarPanel::OnColorDone(LPARAM lParam) {
    if (!m_colorThread) {
        return false;
    }
    m_dialogShown = false;
    WaitForSingleObject(m_colorThread, INFINITE);
    CloseHandle(m_colorThread);
    m_colorThread = nullptr;
    std::unique_ptr<ColorJob> job((ColorJob*)lParam);
    if (job->done && m_colorSource.empty()) {
        m_todayColor = job->value & 0xFFFFFF;
        Wh_SetIntValue(L"calendarTodayColor", m_todayColor);
    } else if (job->done) {
        AcquireSRWLockExclusive(&g_agendasLock);
        for (auto& agenda : g_agendas) {
            if (agenda.source == m_colorSource) {
                agenda.color = job->value & 0xFFFFFF;
            }
        }
        IndexAgendaDays();
        ReleaseSRWLockExclusive(&g_agendasLock);
        SaveAgendas();
    }
    SetForegroundWindow(m_hwnd);
    Layout();
    return true;
}

// The calendar going away with the dialog still open: it's closed, and its
// job (posted to this window, about to go) freed here.
void CalendarPanel::EndColorDialog() {
    const bool color = m_colorThread != nullptr;
    EndFileDialogThread(&m_colorThread);
    MSG msg;
    if (color && PeekMessage(&msg, m_hwnd, WM_APP_COLOR_DONE, WM_APP_COLOR_DONE,
                             PM_REMOVE)) {
        delete (ColorJob*)msg.lParam;
    }
}

// A pin's color picked with Windows' color dialog, like the calendar's,
// starting from the one it has.
void FoldersPanel::StartLookColorDialog(const std::wstring& pin) {
    if (m_dialogShown) {
        return;
    }
    const FolderLook* look = LookOf(pin);
    auto* job = new ColorJob{m_hwnd, 0,
                             look && look->color >= 0 ? look->color : 0x0A84FF};
    m_colorPin = pin;
    m_dialogShown = true;
    m_colorThread = CreateThread(nullptr, 0, ColorThreadProc, job, 0, nullptr);
    if (!m_colorThread) {
        m_dialogShown = false;
        delete job;
    }
}

// (Only from its thread: the job is its.) The pin may be gone meanwhile.
bool FoldersPanel::OnLookColorDone(LPARAM lParam) {
    if (!m_colorThread) {
        return false;
    }
    m_dialogShown = false;
    WaitForSingleObject(m_colorThread, INFINITE);
    CloseHandle(m_colorThread);
    m_colorThread = nullptr;
    std::unique_ptr<ColorJob> job((ColorJob*)lParam);
    if (job->done) {
        const FolderLook* look = LookOf(m_colorPin);
        SetLook(m_colorPin, look ? look->glyph : 0, job->value & 0xFFFFFF);
    }
    SetForegroundWindow(m_hwnd);
    Layout();
    return true;
}

// The panel going away with the dialog still open: it's closed, and its job
// (posted to this window, about to go) freed here.
void FoldersPanel::EndLookColorDialog() {
    const bool color = m_colorThread != nullptr;
    EndFileDialogThread(&m_colorThread);
    MSG msg;
    if (color && PeekMessage(&msg, m_hwnd, WM_APP_COLOR_DONE, WM_APP_COLOR_DONE,
                             PM_REMOVE)) {
        delete (ColorJob*)msg.lParam;
    }
}

// The icon pack's folder chosen (Windows' icons saved into it then, when
// `save`).
void ControlPanel::StartIconFolderDialog(bool save) {
    if (m_dialogShown) {
        return;
    }
    auto* job = new FolderPickJob{
        m_hwnd, Tr(L"The folder with your icons", L"A pasta com seus ícones"),
        save ? kIconFolderPickSave : kIconFolderPick, {}};
    m_dialogShown = true;
    m_iconThread =
        CreateThread(nullptr, 0, FolderPickThreadProc, job, 0, nullptr);
    if (!m_iconThread) {
        m_dialogShown = false;
        delete job;
    }
}

void ControlPanel::StartIconPictureDialog(WCHAR glyph) {
    if (m_dialogShown) {
        return;
    }
    auto* job = new FolderPickJob{
        m_hwnd, Tr(L"A picture for this icon", L"Uma imagem para este ícone"),
        glyph, {}, true};
    m_dialogShown = true;
    m_iconThread =
        CreateThread(nullptr, 0, FolderPickThreadProc, job, 0, nullptr);
    if (!m_iconThread) {
        m_dialogShown = false;
        delete job;
    }
}

// Windows' icons saved into the folder (on a thread, see
// IconSaveThreadProc), then said on the row and the folder opened to change
// them. Only a folder on this computer (one from a backup may not be).
void ControlPanel::SaveWindowsIconsInto(const std::wstring& folder) {
    if (m_dialogShown) {
        return;
    }
    if (!IsLocalDrivePath(folder)) {
        m_iconStatus = Tr(L"Only a folder on this computer",
                          L"Só uma pasta deste computador");
        Layout();
        return;
    }
    auto* job = new FolderPickJob{m_hwnd, nullptr, kIconFolderSaved, folder};
    m_dialogShown = true;
    m_iconStatus =
        Tr(L"Saving Windows' icons…", L"Salvando os ícones do Windows…");
    m_iconThread =
        CreateThread(nullptr, 0, IconSaveThreadProc, job, 0, nullptr);
    if (!m_iconThread) {
        m_dialogShown = false;
        m_iconStatus.clear();
        delete job;
    }
    Layout();
}

void ControlPanel::StartBackupDialog(bool save) {
    if (m_dialogShown) {
        return;
    }
    std::wstring text;
    if (save) {
        IslandScope scope(MainIsland());
        SaveActiveProfile();
        text = CaptureBackup();
    }
    auto* job = new BackupJob{m_hwnd, save, std::move(text)};
    m_dialogShown = true;
    m_backupThread =
        CreateThread(nullptr, 0, BackupThreadProc, job, 0, nullptr);
    if (!m_backupThread) {
        m_dialogShown = false;
        delete job;
    }
}

// The logs saved to a file the user picks (the backup's dialog).
void ControlPanel::StartLogsDialog() {
    if (m_dialogShown) {
        return;
    }
    auto* job = new BackupJob{m_hwnd, true, LogsText()};
    job->logs = true;
    m_dialogShown = true;
    m_backupThread =
        CreateThread(nullptr, 0, BackupThreadProc, job, 0, nullptr);
    if (!m_backupThread) {
        m_dialogShown = false;
        delete job;
    }
}

void ControlPanel::StartWifi() {
    if (m_wlan || !m_hwnd) {
        return;
    }
    DWORD version;
    if (WlanOpenHandle(2, nullptr, &version, &m_wlan) != ERROR_SUCCESS) {
        m_wlan = nullptr;
        return;
    }
    PWLAN_INTERFACE_INFO_LIST interfaces = nullptr;
    if (WlanEnumInterfaces(m_wlan, nullptr, &interfaces) == ERROR_SUCCESS) {
        if (interfaces->dwNumberOfItems > 0) {
            m_wifiInterface = interfaces->InterfaceInfo[0].InterfaceGuid;
            m_hasWifi = true;
        }
        WlanFreeMemory(interfaces);
    }
    WlanRegisterNotification(m_wlan, WLAN_NOTIFICATION_SOURCE_ACM, TRUE,
                             WlanNotificationCallback, m_hwnd, nullptr,
                             nullptr);
}

void ControlPanel::StopWifi() {
    if (m_wifiListThread) {
        WaitForSingleObject(m_wifiListThread, INFINITE);
        CloseHandle(m_wifiListThread);
        m_wifiListThread = nullptr;
        MSG msg;
        while (m_hwnd && PeekMessage(&msg, m_hwnd, WM_APP_WIFI_LIST,
                                     WM_APP_WIFI_LIST, PM_REMOVE)) {
            if (msg.lParam != kWifiListRefused) {
                delete (std::vector<WifiNetwork>*)msg.lParam;
            }
        }
    }
    m_wifiListAgain = m_wifiScanAgain = false;
    if (!m_wlan) {
        return;
    }
    // Joins that ended meanwhile, then the ones still pending (the island
    // going away): a profile made for one stays only if it's the network
    // joined now.
    MSG ended;
    while (m_hwnd && PeekMessage(&ended, m_hwnd, WM_APP_WLAN_CONNECTION,
                                 WM_APP_WLAN_CONNECTION, PM_REMOVE)) {
        const std::unique_ptr<WlanConnectionEvent> event(
            (WlanConnectionEvent*)ended.lParam);
        OnWifiJoinEnded(*event);
    }
    if (!m_createdProfiles.empty()) {
        std::wstring joined;
        DWORD size = 0;
        WLAN_CONNECTION_ATTRIBUTES* connection = nullptr;
        if (WlanQueryInterface(m_wlan, &m_wifiInterface,
                               wlan_intf_opcode_current_connection, nullptr,
                               &size, (void**)&connection,
                               nullptr) == ERROR_SUCCESS &&
            connection) {
            if (connection->isState == wlan_interface_state_connected) {
                joined = connection->strProfileName;
            }
            WlanFreeMemory(connection);
        }
        for (const auto& profile : m_createdProfiles) {
            if (profile != joined) {
                WlanDeleteProfile(m_wlan, &m_wifiInterface, profile.c_str(),
                                  nullptr);
            }
        }
        m_createdProfiles.clear();
    }
    WlanRegisterNotification(m_wlan, WLAN_NOTIFICATION_SOURCE_NONE, TRUE,
                             nullptr, nullptr, nullptr, nullptr);
    WlanCloseHandle(m_wlan, nullptr);
    m_wlan = nullptr;
    m_hasWifi = false;
    // Joins' ends still queued (each one allocated by the callback).
    MSG msg;
    while (m_hwnd && PeekMessage(&msg, m_hwnd, WM_APP_WLAN_CONNECTION,
                                 WM_APP_WLAN_CONNECTION, PM_REMOVE)) {
        delete (WlanConnectionEvent*)msg.lParam;
    }
}

struct WifiListJob {
    HWND notify;
    HANDLE wlan;
    GUID wifiInterface;
    bool scan;
    UINT id;
};

