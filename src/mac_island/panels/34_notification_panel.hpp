#pragma once
class NotificationPanel : public Panel {
   public:
    // An app's notifications were seen (it came to the front): cleared here.
    void ClearApp(const std::wstring& appId);
    // A notification deleted from the pill's banner (its "x"): gone from the
    // list too (and its app from the island, if nothing else of it is left).
    void DismissById(long long id) {
        LoadCleared();
        if (!m_clearedLoaded || id <= 0) {
            return;
        }
        Remove(id, 1);
    }
    // The apps with notifications in the Windows notification center now
    // (an app takes its own away, like a message read on the phone).
    // Returns false if the database can't be read.
    bool AppsInCenter(std::vector<std::wstring>* apps);
    // How many notifications each app has there now (none if it can't be
    // read).
    std::vector<std::pair<std::wstring, int>> CountsInCenter();
    // How many kept ones each app with a badge has in the list (see
    // MergeKeptToasts), for its count on the island.
    std::vector<std::pair<std::wstring, int>> KeptCounts();
    // The latest notification of each app that came after `seenId` (up to
    // a few apps, oldest first), not cleared here; `newestId` gets the
    // newest ID read.
    std::vector<IslandToast> MissedToasts(long long seenId,
                                          long long* newestId);
    // Its switches unfolded under the gear (the bell's right click): now if
    // open, otherwise when it opens.
    void ShowSettings() {
        if (IsOpen()) {
            m_settingsOpen = true;
            StartIslandAnimation();
        } else {
            m_openOnSettings = true;
        }
    }
    // Shows an app's notifications: the list scrolls to the first one, and
    // its cards glow for a moment (now if open, otherwise when it opens).
    void FocusApp(const std::wstring& appId) {
        m_focusApp = appId;
        if (IsOpen()) {
            ApplyFocus();
        }
    }
    // Called while open, to show new notifications (after cards sliding
    // away are gone).
    void Refresh() {
        if (IsOpen()) {
            if (!m_clearAllPending &&
                std::none_of(
                    m_motions.begin(), m_motions.end(),
                    [](const CardMotion& motion) { return motion.removing; })) {
                std::vector<long long> before;
                for (const auto& notification : m_notifications) {
                    before.push_back(notification.id);
                }
                Load();
                // New ones open their room and slide in from the side.
                if (!before.empty()) {
                    for (const auto& notification : m_notifications) {
                        if (std::find(before.begin(), before.end(),
                                      notification.id) == before.end()) {
                            if (CardMotion* motion =
                                    Motion(notification.id, true)) {
                                motion->room = 0;
                                motion->roomVelocity = 0;
                                motion->offset = -std::round(60 * m_scale);
                                motion->velocity = 0;
                                motion->target = 0;
                            }
                        }
                    }
                    StartIslandAnimation();
                }
            }
            Layout();
            Render();
        }
    }

   protected:
    void Prepare(float scale) override;
    void OnOpen() override;
    void Draw(ID2D1RenderTarget* target, ID2D1SolidColorBrush* brush) override;
    bool OnMouseMove(POINT pt) override;
    bool OnMouseLeave() override;
    bool OnMouseUp(POINT pt) override;
    bool OnWheel(int delta) override;
    void OnDestroy() override { m_sqlite.Unload(); }
    bool AnimateContents(double dt) override;
    bool OnChar(WCHAR c) override;
    bool OnKey(WPARAM key) override;

   private:
    struct Notification {
        long long id;
        std::wstring appId;
        // The app's own tag and group for it: one posted again with the same
        // ones replaces it (see DismissKey).
        std::wstring tag;
        std::wstring group;
        std::wstring title;
        std::wstring text;
        // With its own lines (see SplitToastTexts).
        std::wstring fullText;
        long long arrival;
        ToastReply reply;
        // A call's, an alarm's or a reminder's, and its sound (for showing
        // it again, snoozed).
        bool urgent = false;
        std::wstring sound;
        bool loop = false;
        // Kept by the island after its app took it out (see
        // MergeKeptToasts): pressing it opens the app, which has no use for
        // it any more.
        bool kept = false;
    };

    enum class Part {
        None,
        Card,
        Dismiss,
        ClearAll,
        SettingsButton,
        DndToggle,
        ContentToggle,
        BannerToggle,
        MinimizedToggle,
        ReplyButton,  // "Reply" on a card (index).
        CardAction,   // A card's own button (card * 16 + button).
        ReplyField,   // The box being typed in (index).
        ReplySend,    // Its send button (index).
        Link
    };
    struct Hit {
        Part part = Part::None;
        int index = -1;
        bool operator==(const Hit& other) const {
            return part == other.part && index == other.index;
        }
    };

    void Load();
    void MergeKeptToasts();
    void LoadCleared();
    void SaveCleared();
    void Layout();
    Hit HitTest(POINT pt) const;
    const std::wstring& AppName(const std::wstring& appId);
    ID2D1Bitmap* AppBitmap(ID2D1RenderTarget* target,
                           const std::wstring& appId);
    std::wstring TimeText(long long arrival) const;

    Sqlite m_sqlite;
    // Runs a query on the Windows notification database, read-only, calling
    // `row` for each row; false if it can't be read.
    bool ReadCenter(PCSTR query, const std::function<void(void*)>& row);
    bool m_sqliteLoaded = false;
    std::vector<Notification> m_notifications;
    // Cleared here (the Windows notification database is only read): all
    // that arrived up to `m_clearedUntil`, and the ones dismissed one by one.
    // One copy for every island's panel (all on the island thread), so a
    // panel saving it never brings back what another one cleared.
    static inline long long m_clearedUntil = 0;
    static inline std::vector<long long> m_dismissed;
    // Also by its tag and what it says: some apps (like Claude) post a
    // notification again, as a new one with the same tag, which stays
    // dismissed. Not for notifications without a tag: the same text can
    // come again as news (like "3 unread messages").
    static inline std::vector<std::wstring> m_dismissedTexts;
    // Only the latest ones dismissed one by one are worth keeping (and
    // their stored text is read back with room for all of them).
    static constexpr size_t kMaxDismissed = 200;
    static constexpr size_t kMaxDismissedTexts = 60;
    static constexpr size_t kDismissKeyMax = 300;
    static std::wstring DismissKey(const Notification& n) {
        if (n.tag.empty()) {
            return {};
        }
        std::wstring key = n.appId + L"\t" + n.tag + L"\t" + n.group + L"\t" +
                           n.title + L"\t" + n.text;
        std::replace(key.begin(), key.end(), L'\n', L' ');
        return key.substr(0, kDismissKeyMax);
    }
    static inline bool m_clearedLoaded = false;
    bool m_doNotDisturb = false;
    Hit m_hover;
    // The cards, in the list's coordinates (0 at its top), the room the list
    // has in the panel, and its scrolling.
    std::vector<D2D1_RECT_F> m_cards;
    D2D1_RECT_F m_listViewport{};
    SmoothScroll m_scroll;
    // The app to show (see FocusApp), and the cards glowing (an app's, or one
    // that has nothing to open), since when.
    std::wstring m_focusApp;
    std::wstring m_glowApp;
    long long m_glowId = -1;
    double m_glowStart = -10;
    void ApplyFocus();

    POINT ToList(POINT pt) const {
        return {pt.x, (LONG)std::lround(pt.y - m_listViewport.top +
                                        m_scroll.Offset())};
    }
    D2D1_RECT_F m_header{}, m_clearAll{}, m_dndRow{}, m_dndToggle{},
        m_contentRow{}, m_contentToggle{}, m_link{};
    // The switches' knobs, sliding (0 off, 1 on).
    double m_dndKnob = -1;
    double m_dndKnobVelocity = 0;
    double m_contentKnob = -1;
    double m_contentKnobVelocity = 0;
    // "In the pill" and "With the pill minimized".
    D2D1_RECT_F m_bannerRow{}, m_bannerToggle{}, m_minimizedRow{},
        m_minimizedToggle{};
    double m_bannerKnob = -1;
    double m_bannerKnobVelocity = 0;
    double m_minimizedKnob = -1;
    double m_minimizedKnobVelocity = 0;
    // The switches, folded away under the gear by the title (0 to 1); folded
    // again each time it opens, unless it's opened on them.
    bool m_settingsOpen = false;
    bool m_openOnSettings = false;
    double m_settingsAmount = 0;
    double m_settingsVelocity = 0;
    D2D1_RECT_F m_settingsButton{}, m_settingsSection{};
    // Cards moving: dragged sideways, sliding away (then their room closing),
    // or back.
    struct CardMotion {
        long long id;
        double offset = 0;
        double velocity = 0;
        double target = 0;
        double room = 1;
        double roomVelocity = 0;
        // Before it starts sliding ("Clear all" sends them one after
        // another).
        double delay = 0;
        bool removing = false;
    };
    std::vector<CardMotion> m_motions;
    CardMotion* Motion(long long id, bool create);
    // (`center`: gone from Windows' notification center too.)
    void Remove(long long id,
                int direction,
                double delay = 0,
                bool center = true);
    void RemoveFromCenter(const std::vector<Notification>& gone);
    // Dragging a card: which, from where, and whether it moved yet.
    long long m_dragId = -1;
    int m_dragStartX = 0;
    bool m_dragMoved = false;
    bool m_clearAllPending = false;
    bool OnMouseDown(POINT pt) override;
    // The notification being answered (by its ID), what's typed, and how
    // many lines it takes (the box grows, up to five).
    long long m_replyId = -1;
    std::wstring m_replyText;
    TextEdit m_replyEdit;
    int m_replyLines = 1;
    float ReplyExtra() const {
        return (m_replyLines - 1) * std::round(17 * m_scale);
    }
    void SendReply();
    // The card under the mouse lights up and, with a longer text, opens to
    // up to three lines, smoothly (each card's amount, 0 to 1); how many
    // lines each text takes, measured once per size.
    struct CardHover {
        long long id;
        double amount = 0;
        double velocity = 0;
    };
    std::vector<CardHover> m_cardHovers;
    std::vector<std::pair<long long, int>> m_cardLines;
    long long HoveredCard() const;
    float HoverAmount(long long id) const;
    // How much taller its text makes a card now.
    float CardExtra(long long id) const;
    // A card's "Reply" button, and the reply box with its send button
    // (below its text, `extra` taller).
    void CardParts(const D2D1_RECT_F& card,
                   float extra,
                   D2D1_RECT_F* replyButton,
                   D2D1_RECT_F* field,
                   D2D1_RECT_F* send) const;
    // A card's own buttons (see ToastReply::actions), in the row with
    // "Reply" (left of it), shown with the card's focus.
    D2D1_RECT_F CardActionRect(const D2D1_RECT_F& card,
                               float extra,
                               const Notification& n,
                               size_t index) const;
    void DrawSwitchRow(ID2D1RenderTarget* target,
                       ID2D1SolidColorBrush* brush,
                       const D2D1_RECT_F& row,
                       const D2D1_RECT_F& toggle,
                       WCHAR glyph,
                       const D2D1_COLOR_F& glyphColor,
                       PCWSTR name,
                       PCWSTR description,
                       double knob,
                       bool hover,
                       bool enabled = true,
                       bool ownCard = true);
    std::vector<std::pair<std::wstring, std::wstring>> m_names;
    std::vector<KeptAppIcon> m_bitmaps;
    winrt::com_ptr<IWICImagingFactory> m_wic;
    winrt::com_ptr<IDWriteTextFormat> m_headerFormat;
    winrt::com_ptr<IDWriteTextFormat> m_appFormat;
    winrt::com_ptr<IDWriteTextFormat> m_timeFormat;
    winrt::com_ptr<IDWriteTextFormat> m_titleFormat;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    winrt::com_ptr<IDWriteTextFormat> m_linkFormat;
    winrt::com_ptr<IDWriteTextFormat> m_iconFormat;
    float m_preparedScale = 0;
};

////////////////////////////////////////////////////////////////////////////////
// What's playing (music, videos), from the media controls Windows shows for
// apps (Windows.Media.Control, documented). A thread of its own is told when
// the playing app, the song or its state changes, reads it, and hands it to
// the island; the island's buttons are sent back to it.

// An app with media controls (playing or not), as the island lists them to
// choose which one the pill and the panel control.
struct MediaSource {
    std::wstring appId;
    std::wstring title;
    bool playing = false;
};

struct MediaState {
    bool present = false;
    // Every app with media controls, and which of them this is (-1 none);
    // chosen by the user rather than Windows' current one.
    std::vector<MediaSource> sources;
    int sourceIndex = -1;
    bool chosen = false;
    std::wstring appId;
    std::wstring title;
    std::wstring artist;
    // A video (or a picture), as the app says: no lyrics for it.
    bool video = false;
    bool playing = false;
    bool canPrevious = false;
    bool canNext = false;
    bool canPlayPause = false;
    bool canSeek = false;
    // In seconds; the position as of `updatedAt` (NowSeconds()).
    double position = 0;
    double duration = 0;
    double updatedAt = 0;
    // The cover (an image file's bytes), and a number that changes with it.
    std::vector<BYTE> thumbnail;
    UINT thumbnailSerial = 0;

    // Where it is now, moving while it plays.
    double Now() const {
        const double now =
            position + (playing ? NowSeconds() - updatedAt : 0);
        return duration > 0 ? std::clamp(now, 0.0, duration) : now;
    }
};

enum class MediaCommand { PlayPause, Previous, Next, Seek };

SRWLOCK g_mediaLock = SRWLOCK_INIT;
MediaState g_media;
// The app chosen for the media (its ID, and which of its sessions when it
// has several), or empty for the one Windows takes as current. Kept only
// while the app has media; then back to Windows' choice.
std::wstring g_mediaChosenApp;
int g_mediaChosenOccurrence = 0;
std::vector<std::pair<MediaCommand, double>> g_mediaCommands;
HANDLE g_mediaThread;
HANDLE g_mediaStopEvent;
HANDLE g_mediaChangedEvent;
HANDLE g_mediaCommandEvent;
// Held to close the events: a media app's late call (on a thread of its own)
// can come after its handler was revoked.
SRWLOCK g_mediaEventsLock = SRWLOCK_INIT;

void TellMediaChanged() {
    AcquireSRWLockShared(&g_mediaEventsLock);
    if (g_mediaChangedEvent) {
        SetEvent(g_mediaChangedEvent);
    }
    ReleaseSRWLockShared(&g_mediaEventsLock);
}
// Tells the island it changed.
constexpr UINT WM_APP_MEDIA = WM_APP + 54;

// Chooses the media the island controls: an index in MediaState::sources,
// or -1 for the one Windows takes as current.
void ChooseMediaSource(const MediaState& state, int index) {
    AcquireSRWLockExclusive(&g_mediaLock);
    if (index < 0 || index >= (int)state.sources.size()) {
        g_mediaChosenApp.clear();
        g_mediaChosenOccurrence = 0;
    } else {
        g_mediaChosenApp = state.sources[index].appId;
        g_mediaChosenOccurrence = 0;
        for (int i = 0; i < index; i++) {
            if (state.sources[i].appId == g_mediaChosenApp) {
                g_mediaChosenOccurrence++;
            }
        }
    }
    ReleaseSRWLockExclusive(&g_mediaLock);
    TellMediaChanged();
}

void SendMediaCommand(MediaCommand command, double position = 0) {
    AcquireSRWLockExclusive(&g_mediaLock);
    g_mediaCommands.push_back({command, position});
    ReleaseSRWLockExclusive(&g_mediaLock);
    if (g_mediaCommandEvent) {
        SetEvent(g_mediaCommandEvent);
    }
}

