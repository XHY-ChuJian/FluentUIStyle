#include "audiomaticplayerwidget.h"
#include "ui_audiomaticplayerwidget.h"

#include "audiospectrumanalyzer.h"
#include "spectrumwidget.h"

#include <QAudioOutput>
#include <QApplication>
#include <QDirIterator>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QListWidget>
#include <QMediaPlayer>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QToolButton>

#include "fluentui3styleproperties.h"
#include <QAction>
#include <QActionGroup>

namespace
{

    constexpr QChar kGlyphShuffle(0xE8B1);
    constexpr QChar kGlyphPrevious(0xE892);
    constexpr QChar kGlyphPlay(0xE768);
    constexpr QChar kGlyphPause(0xE769);
    constexpr QChar kGlyphNext(0xE893);
    constexpr QChar kGlyphRepeat(0xE8EE);
    constexpr QChar kGlyphRepeatOne(0xE8ED);
    constexpr QChar kGlyphVolume(0xE767);
    constexpr QChar kGlyphFolder(0xE8B7);
    constexpr QChar kGlyphSettings(0xE713);
    constexpr QChar kGlyphMusic(0xE8D6);

    QFont fluentIconFont(int pixelSize)
    {
        QFont font(QStringLiteral("Segoe Fluent Icons"));
        font.setPixelSize(pixelSize);
        font.setStyleStrategy(QFont::NoFontMerging);
        return font;
    }

    QString formatTime(qint64 ms)
    {
        if (ms < 0)
        {
            ms = 0;
        }
        const qint64 totalSeconds = ms / 1000;
        const qint64 minutes = totalSeconds / 60;
        const qint64 seconds = totalSeconds % 60;
        return QStringLiteral("%1:%2")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'));
    }

    bool isAudioFile(const QFileInfo &info)
    {
        static const QStringList suffixes = {
            QStringLiteral("mp3"),
            QStringLiteral("flac"),
            QStringLiteral("wav"),
            QStringLiteral("ogg"),
            QStringLiteral("aac"),
            QStringLiteral("wma"),
            QStringLiteral("m4a"),
            QStringLiteral("opus"),
            QStringLiteral("ape"),
            QStringLiteral("aiff"),
        };
        return suffixes.contains(info.suffix().toLower());
    }

} // namespace

AudiomaticPlayerWidget::AudiomaticPlayerWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::AudiomaticPlayerWidget)
{
    ui->setupUi(this);
    setObjectName(QStringLiteral("audiomatic-central"));
    setAttribute(Qt::WA_StyledBackground, true);

    ui->contentTabs->tabBar()->setProperty("tabBarStyle", 3);
    ui->contentTabs->setStyleSheet("QTabWidget::pane{border:none;}");

    setupChrome();
    setupPlayer();
    setupConnections();
    setupSettingsMenu();

    ui->contentTabs->setCurrentIndex(0);
}

AudiomaticPlayerWidget::~AudiomaticPlayerWidget()
{
    if (m_player)
    {
        m_player->stop();
    }
    delete ui;
}

void AudiomaticPlayerWidget::setupChrome()
{
    ui->labelCover->setObjectName(QStringLiteral("audiomatic-cover"));
    ui->labelCover->setText(QString(kGlyphMusic));
    ui->labelCover->setFont(fluentIconFont(24));

    ui->labelTitle->setObjectName(QStringLiteral("audiomatic-title"));
    ui->labelTimeCurrent->setObjectName(QStringLiteral("audiomatic-time"));
    ui->labelTimeTotal->setObjectName(QStringLiteral("audiomatic-time"));
    ui->labelTrackCount->setObjectName(QStringLiteral("audiomatic-time"));

    ui->sliderPosition->setRange(0, 0);
    ui->sliderPosition->setProperty(SliderValueTipProperty, true);

    ui->sliderVolume->setRange(0, 100);

    ui->btnPlayPause->setObjectName(QStringLiteral("audiomatic-play"));
    ui->btnPlayPause->setProperty(ButtonAccentStyleProperty, true);
    ui->btnPlayPause->setIconSize(QSize(18, 18));

    ui->labelVolumeIcon->setFont(fluentIconFont(14));
    ui->labelVolumeIcon->setText(QString(kGlyphVolume));

    updateIcons();
}

void AudiomaticPlayerWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange)
    {
        updateIcons();
    }
}

void AudiomaticPlayerWidget::updateIcons()
{
    const auto setTransport = [this](QToolButton *button, const QChar &glyph, int size)
    {
        button->setIcon(transportIcon(glyph, 16));
        button->setIconSize(QSize(16, 16));
        button->setFixedSize(size, size);
    };

    setTransport(ui->btnShuffle, kGlyphShuffle, 32);
    setTransport(ui->btnPrevious, kGlyphPrevious, 32);
    setTransport(ui->btnNext, kGlyphNext, 32);
    setTransport(ui->btnSettings, kGlyphSettings, 32);

    const bool playing = m_player && m_player->playbackState() == QMediaPlayer::PlayingState;
    ui->btnPlayPause->setIcon(transportIcon(playing ? kGlyphPause : kGlyphPlay, 18, true));

    const QChar repeatGlyph = m_repeat == RepeatMode::One ? kGlyphRepeatOne : kGlyphRepeat;
    setTransport(ui->btnRepeat, repeatGlyph, 32);

    ui->btnChooseFolder->setIcon(transportIcon(kGlyphFolder, 14));
}

QColor AudiomaticPlayerWidget::iconForegroundColor(bool onAccent) const
{
    if (onAccent)
    {
        return QColor(255, 255, 255);
    }

    const QVariant scheme = qApp->property("_q_colorscheme");
    const bool dark = scheme.isValid() ? scheme.toInt() == 1
                                       : qApp->palette().color(QPalette::Window).lightness() < 128;
    return dark ? QColor(255, 255, 255) : QColor(0, 0, 0);
}

void AudiomaticPlayerWidget::setupPlayer()
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(ui->sliderVolume->value() / 100.0);
    m_player->setAudioOutput(m_audioOutput);

    m_spectrumAnalyzer = new AudioSpectrumAnalyzer(this);
    connect(m_spectrumAnalyzer,
            &AudioSpectrumAnalyzer::pcmDataReady,
            ui->visualizer,
            &SpectrumWidget::setAudioData);
    connect(m_spectrumAnalyzer,
            &AudioSpectrumAnalyzer::sampleRateChanged,
            ui->visualizer,
            &SpectrumWidget::setSampleRate);

    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 position)
            {
                if (!m_seeking)
                {
                    ui->sliderPosition->setValue(int(position));
                    ui->labelTimeCurrent->setText(formatTime(position));
                }
                if (m_spectrumAnalyzer)
                {
                    m_spectrumAnalyzer->setPlaybackPositionMs(position);
                }
            });

    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duration)
            {
                ui->sliderPosition->setRange(0, int(duration));
                ui->labelTimeTotal->setText(formatTime(duration));
            });

    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state)
            {
                const bool playing = state == QMediaPlayer::PlayingState;
                if (m_spectrumAnalyzer)
                {
                    if (playing && m_player)
                    {
                        m_spectrumAnalyzer->setPlaybackPositionMs(m_player->position());
                    }
                    m_spectrumAnalyzer->setActive(playing);
                }
                updateIcons();
                updateTransportButtons();
            });

    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status)
            {
                if (status == QMediaPlayer::EndOfMedia)
                {
                    if (m_repeat == RepeatMode::One)
                    {
                        m_player->setPosition(0);
                        m_player->play();
                    }
                    else if (m_repeat == RepeatMode::All || m_shuffle)
                    {
                        const int next = m_shuffle ? QRandomGenerator::global()->bounded(m_tracks.size())
                                                   : (m_currentIndex + 1) % m_tracks.size();
                        playIndex(next);
                    }
                    else if (m_currentIndex + 1 < m_tracks.size())
                    {
                        playIndex(m_currentIndex + 1);
                    }
                }
            });
}

void AudiomaticPlayerWidget::setupConnections()
{
    connect(ui->btnChooseFolder, &QPushButton::clicked, this, [this]()
            {
                const QString folder = QFileDialog::getExistingDirectory(this, tr("选择音乐文件夹"), QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
                if (!folder.isEmpty())
                {
                    scanFolder(folder);
                }
            });

    connect(ui->btnShuffle, &QToolButton::toggled, this, [this](bool checked)
            { m_shuffle = checked; });

    connect(ui->btnRepeat, &QToolButton::clicked, this, [this]()
            {
                if (m_repeat == RepeatMode::None)
                {
                    m_repeat = RepeatMode::All;
                    ui->btnRepeat->setChecked(true);
                }
                else if (m_repeat == RepeatMode::All)
                {
                    m_repeat = RepeatMode::One;
                }
                else
                {
                    m_repeat = RepeatMode::None;
                    ui->btnRepeat->setChecked(false);
                }
                updateIcons();
            });

    connect(ui->btnSpeed, &QToolButton::clicked, this, [this]()
            {
                static const QList<qreal> speeds = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0};
                m_speedIndex = (m_speedIndex + 1) % speeds.size();
                const qreal speed = speeds[m_speedIndex];
                ui->btnSpeed->setText(QStringLiteral("%1x").arg(speed));
                if (m_player)
                {
                    m_player->setPlaybackRate(speed);
                }
            });

    connect(ui->btnPrevious, &QToolButton::clicked, this, [this]()
            {
                if (m_tracks.isEmpty())
                {
                    return;
                }
                if (m_shuffle)
                {
                    playIndex(QRandomGenerator::global()->bounded(m_tracks.size()));
                    return;
                }
                playIndex((m_currentIndex - 1 + m_tracks.size()) % m_tracks.size());
            });

    connect(ui->btnNext, &QToolButton::clicked, this, [this]()
            {
                if (m_tracks.isEmpty())
                {
                    return;
                }
                if (m_shuffle)
                {
                    playIndex(QRandomGenerator::global()->bounded(m_tracks.size()));
                    return;
                }
                playIndex((m_currentIndex + 1) % m_tracks.size());
            });

    connect(ui->btnPlayPause, &QToolButton::clicked, this, [this]()
            {
                if (!m_player)
                {
                    return;
                }
                if (m_player->playbackState() == QMediaPlayer::PlayingState)
                {
                    m_player->pause();
                }
                else if (!m_tracks.isEmpty())
                {
                    if (m_currentIndex < 0)
                    {
                        playIndex(0);
                    }
                    else
                    {
                        m_player->play();
                    }
                }
            });

    connect(ui->sliderPosition, &QSlider::sliderPressed, this, [this]()
            { m_seeking = true; });

    connect(ui->sliderPosition, &QSlider::sliderMoved, this, [this](int value)
            {
                ui->labelTimeCurrent->setText(formatTime(value));
                if (m_player)
                {
                    m_player->setPosition(value);
                }
                if (m_spectrumAnalyzer)
                {
                    m_spectrumAnalyzer->setPlaybackPositionMs(value);
                }
            });

    connect(ui->sliderPosition, &QSlider::sliderReleased, this, [this]()
            {
                if (m_player)
                {
                    m_player->setPosition(ui->sliderPosition->value());
                }
                m_seeking = false;
                if (m_spectrumAnalyzer)
                {
                    m_spectrumAnalyzer->restart();
                }
            });

    connect(ui->sliderVolume, &QSlider::valueChanged, this, [this](int value)
            {
                if (m_audioOutput)
                {
                    m_audioOutput->setVolume(value / 100.0);
                }
            });

    connect(ui->listPlaylist, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item)
            {
                if (!item)
                {
                    return;
                }
                playIndex(ui->listPlaylist->row(item));
            });
}

void AudiomaticPlayerWidget::setupSettingsMenu()
{
    m_settingsMenu = new QMenu(this);
    m_settingsMenu->addAction(tr("Spectrum refresh: 60 FPS"));
    connect(ui->btnSettings, &QToolButton::clicked, this, &AudiomaticPlayerWidget::showSettingsMenu);
}

void AudiomaticPlayerWidget::showSettingsMenu()
{
    if (!m_settingsMenu || !ui->btnSettings)
    {
        return;
    }

    m_settingsMenu->adjustSize();

    const QSize menuSize = m_settingsMenu->sizeHint();
    const QPoint topRight = ui->btnSettings->mapToGlobal(ui->btnSettings->rect().topRight());
    m_settingsMenu->popup(QPoint(topRight.x() - menuSize.width(), topRight.y() - menuSize.height()));
}

QIcon AudiomaticPlayerWidget::transportIcon(const QChar &glyph, int pixelSize, bool onAccent) const
{
    QPixmap pixmap(pixelSize, pixelSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setFont(fluentIconFont(pixelSize));
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, QString(glyph));
    return QIcon(pixmap);
}

void AudiomaticPlayerWidget::scanFolder(const QString &folder)
{
    m_tracks.clear();
    QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QFileInfo info(it.next());
        if (isAudioFile(info))
        {
            m_tracks.append(info.absoluteFilePath());
        }
    }
    m_tracks.sort(Qt::CaseInsensitive);
    m_currentIndex = -1;
    updateTrackCountLabel();
    updatePlaylistView();
    if (!m_tracks.isEmpty())
    {
        playIndex(0);
    }
    else
    {
        updateTrackLabel();
    }
}

void AudiomaticPlayerWidget::playIndex(int index)
{
    if (index < 0 || index >= m_tracks.size() || !m_player)
    {
        return;
    }

    m_currentIndex = index;
    const QUrl source = QUrl::fromLocalFile(m_tracks[index]);
    m_player->setSource(source);
    if (m_spectrumAnalyzer)
    {
        m_spectrumAnalyzer->setSource(source);
        if (m_player->playbackState() == QMediaPlayer::PlayingState)
        {
            m_spectrumAnalyzer->restart();
        }
    }
    m_player->play();
    updateTrackLabel();
    updateTransportButtons();
    updatePlaylistView();
}

void AudiomaticPlayerWidget::updateTrackLabel()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_tracks.size())
    {
        ui->labelTitle->setText(QFileInfo(m_tracks[m_currentIndex]).completeBaseName());
    }
    else
    {
        ui->labelTitle->setText(tr("未选择音乐"));
    }
}

void AudiomaticPlayerWidget::updateTransportButtons()
{
    const bool hasTracks = !m_tracks.isEmpty();
    ui->btnPrevious->setEnabled(hasTracks);
    ui->btnNext->setEnabled(hasTracks);
    ui->btnPlayPause->setEnabled(hasTracks);
    ui->sliderPosition->setEnabled(hasTracks);
}

void AudiomaticPlayerWidget::updateTrackCountLabel()
{
    ui->labelTrackCount->setText(tr("%1 tracks").arg(m_tracks.size()));
}

void AudiomaticPlayerWidget::updatePlaylistView()
{
    if (!ui || !ui->listPlaylist)
    {
        return;
    }

    ui->listPlaylist->clear();
    for (const QString &path : m_tracks)
    {
        ui->listPlaylist->addItem(QFileInfo(path).completeBaseName());
    }

    if (m_currentIndex >= 0 && m_currentIndex < ui->listPlaylist->count())
    {
        ui->listPlaylist->setCurrentRow(m_currentIndex);
    }
}
