#pragma once

#include "jamendoclient.h"

#include <QList>
#include <QUrl>
#include <QWidget>

class QMediaPlayer;
class QAudioOutput;
class AudioSpectrumAnalyzer;
class QMenu;

namespace Ui {
class AudiomaticPlayerWidget;
}

struct PlaylistEntry
{
    QString title;
    QUrl source;
};

class AudiomaticPlayerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AudiomaticPlayerWidget(QWidget *parent = nullptr);
    ~AudiomaticPlayerWidget() override;

protected:
    void changeEvent(QEvent *event) override;

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

    void setupChrome();
    void setupPlayer();
    void setupConnections();
    void setupSettingsMenu();
    void setupOnlineMusic();
    void showSettingsMenu();
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
    void playIndex(int index);
    void setPlaylist(const QList<PlaylistEntry> &entries, int startIndex = 0);
    void updateTrackLabel();
    void updateTransportButtons();
    void updateTrackCountLabel();
    void updatePlaylistView();
    void updateOnlineResultsView();
    QColor iconForegroundColor(bool onAccent) const;
    QIcon transportIcon(const QChar &glyph, int pixelSize, bool onAccent = false) const;

    Ui::AudiomaticPlayerWidget *ui{nullptr};
    QMediaPlayer *m_player{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
    AudioSpectrumAnalyzer *m_spectrumAnalyzer{nullptr};
    JamendoClient *m_jamendoClient{nullptr};

    QMenu *m_settingsMenu{nullptr};

    QList<PlaylistEntry> m_playlist;
    QList<JamendoRadio> m_jamendoRadios;
    OnlineListMode m_onlineListMode{OnlineListMode::Tracks};
    int m_currentIndex{-1};
    bool m_shuffle{false};
    RepeatMode m_repeat{RepeatMode::None};
    int m_speedIndex{2};
    bool m_seeking{false};
};
