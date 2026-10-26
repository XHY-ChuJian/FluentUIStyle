#pragma once

#include "exlyricswidget.h"
#include "extour.h"
#include "jamendoclient.h"

#include <QElapsedTimer>
#include <QList>
#include <QUrl>
#include <QWidget>

class QMediaPlayer;
class QAudioOutput;
class AudioSpectrumAnalyzer;
class SystemAudioCapture;
class SystemMediaWatcher;
struct SystemMediaTrackInfo;
class LyricsManager;
class AudiomaticImmersiveView;
class QMenu;
class QAction;
class QActionGroup;
class QTimer;
class QLabel;

namespace Ui {
class AudiomaticPlayerWidget;
}

struct PlaylistEntry
{
    QString title;
    QString artist;
    QUrl source;

    QString listLabel() const
    {
        if ( title.isEmpty() )
        {
            return artist;
        }
        if ( artist.isEmpty() )
        {
            return title;
        }
        return QStringLiteral( "%1 - %2" ).arg( title, artist );
    }
};

class AudiomaticPlayerWidget : public QWidget
{
    Q_OBJECT

public:
    enum class AudioInputSource
    {
        MediaPlayer,
        SystemAudio
    };

    explicit AudiomaticPlayerWidget(QWidget *parent = nullptr);
    ~AudiomaticPlayerWidget() override;

    AudioInputSource audioInputSource() const { return m_inputSource; }
    void setAudioInputSource(AudioInputSource source);

    bool isTranslucentEnabled() const { return m_translucentEnabled; }
    int translucentOpacity() const { return m_translucentOpacity; }
    bool isAlwaysOnTop() const { return m_alwaysOnTop; }

    bool isImmersiveMode() const { return m_immersiveMode; }
    void setImmersiveMode(bool immersive);

    Q_INVOKABLE void startTour();

signals:
    void immersiveModeChanged(bool immersive);
    void windowChromeChanged();

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class RepeatMode
    {
        None,
        All,
        One
    };

    enum class OnlineListMode
    {
        Tracks,
        Radios
    };

    enum class PlaybackQueue
    {
        Local,
        Online,
        Radio
    };

    void setupChrome();
    void ensureMediaPlayer();
    void releaseMediaOutput();
    void setupSystemAudioCapture();
    void setupSystemMediaWatcher();
    void setupConnections();
    void setupSettingsMenu();
    void setupOnlineMusic();
    void showSettingsMenu();
    void showAudioSourceMenu();
    void updateInputSourceUi();
    void configureJamendoClientId();
    void searchJamendo();
    void setJamendoBusy(bool busy);
    void onJamendoRequestStarted(JamendoRequestKind kind);
    void onJamendoTracksReady(JamendoRequestKind kind, const QList<JamendoTrack> &tracks);
    void onJamendoRadiosReady(const QList<JamendoRadio> &radios);
    void onJamendoRadioStreamReady(const JamendoRadio &radio);
    void onJamendoRequestFailed(JamendoRequestKind kind, const QString &message);
    void playJamendoRadio(int index);
    void updateIcons();
    void scanFolder(const QString &folder);
    void loadApplicationDirectoryPlaylist();
    void playIndex(int index);
    void playOnlineIndex(int index);
    void startMediaPlayback(const QUrl &source, bool enableAnalyzer);
    void stepPlayback(int direction, bool wrap);
    const PlaylistEntry *currentMediaEntry() const;
    void setPlaylist(const QList<PlaylistEntry> &entries, int startIndex = 0);
    void updateTrackLabel();
    void updateTransportButtons();
    void updateTrackCountLabel();
    void updatePlaylistView();
    void updateOnlineResultsView();
    QColor iconForegroundColor(bool onAccent) const;
    QIcon transportIcon(const QChar &glyph, int pixelSize, bool onAccent = false) const;

    void setCoverPixmap(const QPixmap &pixmap);
    QPixmap createPlaceholderCover(int size, const QString &title = QString());
    void updateCoverDisplay();

    void setTranslucentEnabled(bool enabled);
    void setTranslucentOpacity(int percent);
    void setAlwaysOnTop(bool top);
    void updateOverlayButtonsState();

    void setVisualizerAccentColor(const QColor &color);
    QColor visualizerAccentColor() const { return m_visualizerAccentColor; }
    void setVisualizerRainbow(bool enabled);
    void showVisualizerColorDialog();

    void setVisualizerReflectionOpacity(float opacity);
    float visualizerReflectionOpacity() const { return m_visualizerReflectionOpacity; }

    void loadSettings();
    void saveSettings();

    void setupImmersiveView();
    void setupLyricsManager();
    void updateLyricsDisplay(qint64 positionMs);
    void setLyricHighlightIndex(int index);
    void setLyricHighlightColor(const QColor &color);
    QColor lyricHighlightColor() const;
    void showLyricColorDialog();
    void setLyricFontSizeLevel(int level);
    int lyricFontSizeLevel() const { return m_lyricFontSizeLevel; }
    int lyricNormalFontSize() const;
    int lyricActiveFontSize() const;
    int lyricImmersiveFontSize() const;
    void refreshLyricsStyle();
    bool ignoreNonMusicLyrics() const { return m_ignoreNonMusicLyrics; }
    void setIgnoreNonMusicLyrics(bool ignore);

    void setupCpuUsageMonitor();
    void updateCpuUsage();

    void setupTour();
    void maybeStartFirstRunTour();


    Ui::AudiomaticPlayerWidget *ui{nullptr};
    ExTour *m_tour{nullptr};
    bool m_firstRunTourChecked{false};
    QMediaPlayer *m_player{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
    AudioSpectrumAnalyzer *m_spectrumAnalyzer{nullptr};
    SystemAudioCapture *m_systemCapture{nullptr};
    SystemMediaWatcher *m_mediaWatcher{nullptr};
    LyricsManager *m_lyricsManager{nullptr};
    JamendoClient *m_jamendoClient{nullptr};

    QMenu *m_settingsMenu{nullptr};
    QAction *m_actSourcePlayer{nullptr};
    QAction *m_actSourceSystem{nullptr};
    QAction *m_actAlwaysOnTop{nullptr};
    QAction *m_actTranslucent{nullptr};
    QAction *m_actImmersive{nullptr};
    QAction *m_actVisualizerColor{nullptr};
    QAction *m_actRainbow{nullptr};
    QAction *m_actReflection{nullptr};
    QAction *m_actPeakCap{nullptr};
    QActionGroup *m_barCountGroup{nullptr};
    QActionGroup *m_gapGroup{nullptr};
    QActionGroup *m_radiusGroup{nullptr};
    QActionGroup *m_freqGroup{nullptr};
    QMenu *m_menuOpacity{nullptr};
    QActionGroup *m_opacityGroup{nullptr};
    QMenu *m_menuReflectionOpacity{nullptr};
    QActionGroup *m_reflectionOpacityGroup{nullptr};
    QActionGroup *m_lyricFontSizeGroup{nullptr};
    QAction *m_actIgnoreNonMusic{nullptr};
    QTimer *m_systemLiveTimer{nullptr};
    qint64 m_systemLiveElapsedMs{0};

    int m_currentLyricLineIndex{-1};
    QString m_currentLyricsQueryTitle;
    QString m_currentLyricsQueryArtist;
    qint64 m_lastMediaWatcherPositionMs{0};
    QElapsedTimer m_mediaWatcherPosElapsed;
    qint64 m_systemLyricsBaseMs{0};
    QElapsedTimer m_systemLyricsElapsed;
    qint64 m_lyricsUserOffsetMs{0};

    QColor m_lyricHighlightColor;
    int m_lyricFontSizeLevel{1};
    bool m_ignoreNonMusicLyrics{true};

    QTimer *m_cpuTimer{nullptr};
    quint64 m_lastCpuKernelTime{0};
    quint64 m_lastCpuUserTime{0};
    quint64 m_lastCpuIdleTime{0};
    quint64 m_lastProcessKernelTime{0};
    quint64 m_lastProcessUserTime{0};
    bool m_hasLastCpuSample{false};

    QColor m_visualizerAccentColor;
    bool m_visualizerRainbow{true};
    float m_visualizerReflectionOpacity{0.58f};

    bool m_loadingSettings{true};
    bool m_translucentEnabled{false};
    int m_translucentOpacity{85};
    bool m_alwaysOnTop{false};
    bool m_immersiveMode{false};
    AudiomaticImmersiveView *m_immersiveView{nullptr};
    QPixmap m_currentCoverPixmap;

    AudioInputSource m_inputSource{AudioInputSource::SystemAudio};
    QList<PlaylistEntry> m_playlist;
    QList<PlaylistEntry> m_onlineTracks;
    QList<JamendoRadio> m_jamendoRadios;
    PlaylistEntry m_radioNowPlaying;
    OnlineListMode m_onlineListMode{OnlineListMode::Tracks};
    PlaybackQueue m_playbackQueue{PlaybackQueue::Local};
    int m_currentIndex{-1};
    int m_onlineIndex{-1};
    int m_radioIndex{-1};
    bool m_shuffle{false};
    RepeatMode m_repeat{RepeatMode::None};
    int m_speedIndex{2};
    bool m_seeking{false};
};

