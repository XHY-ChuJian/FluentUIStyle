#pragma once

#include <QWidget>
#include <QStringList>

class QMediaPlayer;
class QAudioOutput;
class AudioSpectrumAnalyzer;
class QMenu;

namespace Ui {
class AudiomaticPlayerWidget;
}

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

    void setupChrome();
    void setupPlayer();
    void setupConnections();
    void setupSettingsMenu();
    void showSettingsMenu();
    void updateIcons();
    void scanFolder(const QString &folder);
    void playIndex(int index);
    void updateTrackLabel();
    void updateTransportButtons();
    void updateTrackCountLabel();
    void updatePlaylistView();
    QColor iconForegroundColor(bool onAccent) const;
    QIcon transportIcon(const QChar &glyph, int pixelSize, bool onAccent = false) const;

    Ui::AudiomaticPlayerWidget *ui{nullptr};
    QMediaPlayer *m_player{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
    AudioSpectrumAnalyzer *m_spectrumAnalyzer{nullptr};

    QMenu *m_settingsMenu{nullptr};

    QStringList m_tracks;
    int m_currentIndex{-1};
    bool m_shuffle{false};
    RepeatMode m_repeat{RepeatMode::None};
    int m_speedIndex{2};
    bool m_seeking{false};
};
