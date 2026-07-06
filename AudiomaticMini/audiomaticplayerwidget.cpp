#include "audiomaticplayerwidget.h"
#include "ui_audiomaticplayerwidget.h"

#include "audiospectrumanalyzer.h"
#include "exspectrumwidget.h"
#include "jamendoclient.h"

#include <QAudioOutput>
#include <QApplication>
#include <QDesktopServices>
#include <QDirIterator>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QInputDialog>
#include <QListWidget>
#include <QMediaPlayer>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QToolButton>
#include <QUrl>

#include <algorithm>

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
    setupOnlineMusic();
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
            &ExSpectrumWidget::setAudioData);
    connect(m_spectrumAnalyzer,
            &AudioSpectrumAnalyzer::sampleRateChanged,
            ui->visualizer,
            &ExSpectrumWidget::setSampleRate);

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
                        const int next = m_shuffle ? QRandomGenerator::global()->bounded(m_playlist.size())
                                                   : (m_currentIndex + 1) % m_playlist.size();
                        playIndex(next);
                    }
                    else if (m_currentIndex + 1 < m_playlist.size())
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
                if (m_playlist.isEmpty())
                {
                    return;
                }
                if (m_shuffle)
                {
                    playIndex(QRandomGenerator::global()->bounded(m_playlist.size()));
                    return;
                }
                playIndex((m_currentIndex - 1 + m_playlist.size()) % m_playlist.size());
            });

    connect(ui->btnNext, &QToolButton::clicked, this, [this]()
            {
                if (m_playlist.isEmpty())
                {
                    return;
                }
                if (m_shuffle)
                {
                    playIndex(QRandomGenerator::global()->bounded(m_playlist.size()));
                    return;
                }
                playIndex((m_currentIndex + 1) % m_playlist.size());
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
                else if (!m_playlist.isEmpty())
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
    m_settingsMenu->addSeparator();
    m_settingsMenu->addAction(tr("Jamendo Client ID…"), this, &AudiomaticPlayerWidget::configureJamendoClientId);
    connect(ui->btnSettings, &QToolButton::clicked, this, &AudiomaticPlayerWidget::showSettingsMenu);
}

void AudiomaticPlayerWidget::setupOnlineMusic()
{
    m_jamendoClient = new JamendoClient(this);
    connect(m_jamendoClient, &JamendoClient::requestStarted, this, &AudiomaticPlayerWidget::onJamendoRequestStarted);
    connect(m_jamendoClient, &JamendoClient::tracksReady, this, &AudiomaticPlayerWidget::onJamendoTracksReady);
    connect(m_jamendoClient, &JamendoClient::radiosReady, this, &AudiomaticPlayerWidget::onJamendoRadiosReady);
    connect(m_jamendoClient, &JamendoClient::radioStreamReady, this, &AudiomaticPlayerWidget::onJamendoRadioStreamReady);
    connect(m_jamendoClient, &JamendoClient::requestFailed, this, &AudiomaticPlayerWidget::onJamendoRequestFailed);

    connect(ui->btnJamendoSearch, &QPushButton::clicked, this, &AudiomaticPlayerWidget::searchJamendo);
    connect(ui->lineJamendoSearch, &QLineEdit::returnPressed, this, &AudiomaticPlayerWidget::searchJamendo);
    connect(ui->btnJamendoWeekly, &QPushButton::clicked, this, [this]()
            {
                if (m_jamendoClient)
                {
                    m_jamendoClient->fetchWeeklyTop(20);
                }
            });
    connect(ui->btnJamendoEditorial, &QPushButton::clicked, this, [this]()
            {
                if (m_jamendoClient)
                {
                    m_jamendoClient->fetchEditorialTracks(20);
                }
            });
    connect(ui->btnJamendoRadios, &QPushButton::clicked, this, [this]()
            {
                if (m_jamendoClient)
                {
                    m_jamendoClient->fetchRadios();
                }
            });
    connect(ui->listJamendoResults, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item)
            {
                if (!item)
                {
                    return;
                }
                const int row = ui->listJamendoResults->row(item);
                if (m_onlineListMode == OnlineListMode::Radios)
                {
                    playJamendoRadio(row);
                }
                else
                {
                    playIndex(row);
                }
            });

    if (m_jamendoClient->hasClientId())
    {
        ui->labelJamendoHint->setText(tr("Jamendo · 搜索或点上方按钮发现音乐"));
    }
}

void AudiomaticPlayerWidget::setJamendoBusy(bool busy)
{
    ui->btnJamendoSearch->setEnabled(!busy);
    ui->btnJamendoWeekly->setEnabled(!busy);
    ui->btnJamendoEditorial->setEnabled(!busy);
    ui->btnJamendoRadios->setEnabled(!busy);
    if (busy)
    {
        ui->btnJamendoSearch->setText(tr("加载中…"));
    }
    else
    {
        ui->btnJamendoSearch->setText(tr("搜索"));
    }
}

void AudiomaticPlayerWidget::onJamendoRequestStarted(JamendoRequestKind kind)
{
    Q_UNUSED(kind)
    setJamendoBusy(true);
}

void AudiomaticPlayerWidget::onJamendoTracksReady(JamendoRequestKind kind, const QList<JamendoTrack> &tracks)
{
    setJamendoBusy(false);
    m_onlineListMode = OnlineListMode::Tracks;
    m_jamendoRadios.clear();

    QList<PlaylistEntry> entries;
    entries.reserve(tracks.size());
    for (const JamendoTrack &track : tracks)
    {
        entries.append({track.displayTitle(), track.audioUrl});
    }

    setPlaylist(entries, 0);
    ui->contentTabs->setCurrentWidget(ui->tabOnline);
    updateOnlineResultsView();

    switch (kind)
    {
    case JamendoRequestKind::WeeklyTop:
        ui->labelJamendoHint->setText(tr("本周热榜 · 双击切换曲目"));
        break;
    case JamendoRequestKind::Editorial:
        ui->labelJamendoHint->setText(tr("编辑精选 · 双击切换曲目"));
        break;
    case JamendoRequestKind::Search:
        ui->labelJamendoHint->setText(tr("搜索结果 · 双击切换曲目"));
        break;
    default:
        break;
    }

    if (!entries.isEmpty())
    {
        playIndex(0);
    }
}

void AudiomaticPlayerWidget::onJamendoRadiosReady(const QList<JamendoRadio> &radios)
{
    setJamendoBusy(false);
    m_onlineListMode = OnlineListMode::Radios;
    m_jamendoRadios = radios;
    m_playlist.clear();
    m_currentIndex = -1;
    updateTrackCountLabel();
    updatePlaylistView();
    updateOnlineResultsView();
    updateTrackLabel();
    updateTransportButtons();
    ui->contentTabs->setCurrentWidget(ui->tabOnline);
    ui->labelJamendoHint->setText(tr("Jamendo 电台 · 双击开始收听"));
}

void AudiomaticPlayerWidget::onJamendoRadioStreamReady(const JamendoRadio &radio)
{
    setJamendoBusy(false);

    m_onlineListMode = OnlineListMode::Radios;
    const QString title = tr("电台 · %1").arg(radio.displayName);
    m_playlist = {{title, radio.streamUrl}};
    m_currentIndex = 0;

    if (m_player)
    {
        m_player->setSource(radio.streamUrl);
        m_player->play();
    }
    if (m_spectrumAnalyzer)
    {
        m_spectrumAnalyzer->setActive(false);
    }

    ui->labelTitle->setText(title);
    ui->sliderPosition->setRange(0, 0);
    ui->labelTimeCurrent->setText(formatTime(0));
    ui->labelTimeTotal->setText(QString());
    updateTrackCountLabel();
    updateTransportButtons();
    updateOnlineResultsView();

    for (int i = 0; i < m_jamendoRadios.size(); ++i)
    {
        if (m_jamendoRadios[i].id == radio.id && ui->listJamendoResults)
        {
            ui->listJamendoResults->setCurrentRow(i);
            break;
        }
    }
}

void AudiomaticPlayerWidget::onJamendoRequestFailed(JamendoRequestKind kind, const QString &message)
{
    setJamendoBusy(false);

    QString title = tr("Jamendo");
    switch (kind)
    {
    case JamendoRequestKind::WeeklyTop:
        title = tr("本周热榜");
        break;
    case JamendoRequestKind::Editorial:
        title = tr("编辑精选");
        break;
    case JamendoRequestKind::Radios:
    case JamendoRequestKind::RadioStream:
        title = tr("Jamendo 电台");
        break;
    case JamendoRequestKind::Search:
        title = tr("Jamendo 搜索");
        break;
    }

    const QMessageBox::StandardButton openPortal = QMessageBox::warning(
        this,
        title,
        message,
        QMessageBox::Ok | QMessageBox::Open,
        QMessageBox::Ok);

    if (openPortal == QMessageBox::Open)
    {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://devportal.jamendo.com")));
    }
}

void AudiomaticPlayerWidget::playJamendoRadio(int index)
{
    if (!m_jamendoClient || index < 0 || index >= m_jamendoRadios.size())
    {
        return;
    }

    m_jamendoClient->fetchRadioStream(m_jamendoRadios[index].id);
}

void AudiomaticPlayerWidget::configureJamendoClientId()
{
    if (!m_jamendoClient)
    {
        return;
    }

    bool ok = false;
    const QString current = m_jamendoClient->clientId();
    const QString clientId = QInputDialog::getText(
        this,
        tr("Jamendo Client ID"),
        tr("在 https://devportal.jamendo.com 免费注册应用后获取 Client ID："),
        QLineEdit::Normal,
        current,
        &ok);

    if (!ok)
    {
        return;
    }

    m_jamendoClient->setClientId(clientId.trimmed());
    if (m_jamendoClient->hasClientId())
    {
        ui->labelJamendoHint->setText(tr("Jamendo · 搜索或点上方按钮发现音乐"));
        QMessageBox::information(this, tr("Jamendo"), tr("Client ID 已保存。"));
    }
    else
    {
        ui->labelJamendoHint->setText(tr("Jamendo 开放音乐 · 需在设置中配置 Client ID"));
    }
}

void AudiomaticPlayerWidget::searchJamendo()
{
    if (!m_jamendoClient)
    {
        return;
    }

    m_jamendoClient->searchTracks(ui->lineJamendoSearch->text(), 20);
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
    QList<PlaylistEntry> entries;
    QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QFileInfo info(it.next());
        if (isAudioFile(info))
        {
            entries.append({info.completeBaseName(), QUrl::fromLocalFile(info.absoluteFilePath())});
        }
    }

    std::sort(entries.begin(), entries.end(), [](const PlaylistEntry &a, const PlaylistEntry &b)
              { return a.title.compare(b.title, Qt::CaseInsensitive) < 0; });

    if (entries.isEmpty())
    {
        m_playlist.clear();
        m_currentIndex = -1;
        updateTrackCountLabel();
        updatePlaylistView();
        updateOnlineResultsView();
        updateTrackLabel();
        return;
    }

    setPlaylist(entries, 0);
    m_onlineListMode = OnlineListMode::Tracks;
    m_jamendoRadios.clear();
}

void AudiomaticPlayerWidget::setPlaylist(const QList<PlaylistEntry> &entries, int startIndex)
{
    m_playlist = entries;
    m_currentIndex = -1;
    updateTrackCountLabel();
    updatePlaylistView();
    updateOnlineResultsView();

    if (!m_playlist.isEmpty())
    {
        playIndex(qBound(0, startIndex, m_playlist.size() - 1));
    }
    else
    {
        updateTrackLabel();
    }
}

void AudiomaticPlayerWidget::playIndex(int index)
{
    if (index < 0 || index >= m_playlist.size() || !m_player)
    {
        return;
    }

    m_currentIndex = index;
    const QUrl source = m_playlist[index].source;
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
    updateOnlineResultsView();
}

void AudiomaticPlayerWidget::updateTrackLabel()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size())
    {
        ui->labelTitle->setText(m_playlist[m_currentIndex].title);
    }
    else
    {
        ui->labelTitle->setText(tr("未选择音乐"));
    }
}

void AudiomaticPlayerWidget::updateTransportButtons()
{
    const bool hasTracks = !m_playlist.isEmpty();
    ui->btnPrevious->setEnabled(hasTracks);
    ui->btnNext->setEnabled(hasTracks);
    ui->btnPlayPause->setEnabled(hasTracks);
    ui->sliderPosition->setEnabled(hasTracks);
}

void AudiomaticPlayerWidget::updateTrackCountLabel()
{
    ui->labelTrackCount->setText(tr("%1 tracks").arg(m_playlist.size()));
}

void AudiomaticPlayerWidget::updatePlaylistView()
{
    if (!ui || !ui->listPlaylist)
    {
        return;
    }

    ui->listPlaylist->clear();
    for (const PlaylistEntry &entry : m_playlist)
    {
        ui->listPlaylist->addItem(entry.title);
    }

    if (m_currentIndex >= 0 && m_currentIndex < ui->listPlaylist->count())
    {
        ui->listPlaylist->setCurrentRow(m_currentIndex);
    }
}

void AudiomaticPlayerWidget::updateOnlineResultsView()
{
    if (!ui || !ui->listJamendoResults)
    {
        return;
    }

    ui->listJamendoResults->clear();

    if (m_onlineListMode == OnlineListMode::Radios)
    {
        for (const JamendoRadio &radio : m_jamendoRadios)
        {
            ui->listJamendoResults->addItem(radio.displayName);
        }

        if (m_currentIndex >= 0 && m_currentIndex < ui->listJamendoResults->count())
        {
            ui->listJamendoResults->setCurrentRow(m_currentIndex);
        }
        return;
    }

    for (const PlaylistEntry &entry : m_playlist)
    {
        if (entry.source.scheme() == QStringLiteral("http") || entry.source.scheme() == QStringLiteral("https"))
        {
            ui->listJamendoResults->addItem(entry.title);
        }
    }

    if (m_currentIndex >= 0 && m_currentIndex < ui->listJamendoResults->count())
    {
        ui->listJamendoResults->setCurrentRow(m_currentIndex);
    }
}
