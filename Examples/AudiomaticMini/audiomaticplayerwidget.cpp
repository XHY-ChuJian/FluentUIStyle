#include "audiomaticplayerwidget.h"
#include "audiomaticimmersiveview.h"
#include "exlyricswidget.h"
#include "exfonticon.h"

#include <QActionGroup>
#include <QColorDialog>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QAudioOutput>
#include <QDesktopServices>
#include <QDirIterator>
#include <QEvent>
#include <QShowEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QLabel>
#include <QInputDialog>
#include <QListWidget>
#include <QMediaPlayer>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QSignalBlocker>
#include <QPainterPath>
#include <QPixmap>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <QUrl>

#include "audiospectrumanalyzer.h"
#include "excolorpickerdialog.h"
#include "exspectrumwidget.h"
#include "jamendoclient.h"
#include "lyricsmanager.h"
#include "systemaudiocapture.h"
#include "systemmediawatcher.h"
#include "ui_audiomaticplayerwidget.h"
#include <QSettings>

#ifdef Q_OS_WIN
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#    ifdef small
#        undef small
#    endif
#endif

#include <QAction>
#include <QActionGroup>

#include <algorithm>

#include "fluentui3styleproperties.h"

#include <QMouseEvent>
#include <QKeyEvent>

namespace {

constexpr QChar kGlyphShuffle( 0xE8B1 );
constexpr QChar kGlyphPrevious( 0xE892 );
constexpr QChar kGlyphPlay( 0xE768 );
constexpr QChar kGlyphPause( 0xE769 );
constexpr QChar kGlyphNext( 0xE893 );
constexpr QChar kGlyphRepeat( 0xE8EE );
constexpr QChar kGlyphRepeatOne( 0xE8ED );
constexpr QChar kGlyphVolume( 0xE767 );
constexpr QChar kGlyphFolder( 0xE8B7 );
constexpr QChar kGlyphSettings( 0xE713 );
constexpr QChar kGlyphMusic( 0xE8D6 );
constexpr QChar kGlyphPin( 0xE718 );
constexpr QChar kGlyphTransparency( 0xE790 );
constexpr QChar kGlyphEqualizer( 0xE9E9 );

QFont fluentIconFont( int pixelSize )
{
    QFont font( QStringLiteral( "Segoe Fluent Icons" ) );
    font.setPixelSize( pixelSize );
    font.setStyleStrategy( QFont::NoFontMerging );
    return font;
}

QString formatTime( qint64 ms )
{
    if ( ms < 0 )
    {
        ms = 0;
    }
    const qint64 totalSeconds = ms / 1000;
    const qint64 minutes      = totalSeconds / 60;
    const qint64 seconds      = totalSeconds % 60;
    return QStringLiteral( "%1:%2" ).arg( minutes ).arg( seconds, 2, 10, QChar( '0' ) );
}

bool isAudioFile( const QFileInfo& info )
{
    const QString ext = info.suffix().toLower();
    return ext == QStringLiteral( "mp3" ) || ext == QStringLiteral( "wav" ) ||
           ext == QStringLiteral( "flac" ) || ext == QStringLiteral( "ogg" ) ||
           ext == QStringLiteral( "m4a" ) || ext == QStringLiteral( "aac" );
}

// 本地文件名按「歌名 - 歌手」拆开，和 Jamendo 的 name / artist_name 同一方向。
PlaylistEntry playlistEntryFromFile( const QFileInfo& info )
{
    PlaylistEntry entry;
    entry.source = QUrl::fromLocalFile( info.absoluteFilePath() );
    const QString base = info.completeBaseName();
    const int dash = base.indexOf( QStringLiteral( " - " ) );
    if ( dash > 0 )
    {
        entry.title = base.left( dash ).trimmed();
        entry.artist = base.mid( dash + 3 ).trimmed();
    }
    else
    {
        entry.title = base;
    }
    return entry;
}

QString formatAppFriendlyName( const QString& rawAppId )
{
    if ( rawAppId.isEmpty() )
    {
        return QString();
    }

    const QString lower = rawAppId.toLower();
    if ( lower.contains( QStringLiteral( "qqmusic" ) ) ) return QStringLiteral( "QQ音乐" );
    if ( lower.contains( QStringLiteral( "cloudmusic" ) ) || lower.contains( QStringLiteral( "netease" ) ) ) return QStringLiteral( "网易云音乐" );
    if ( lower.contains( QStringLiteral( "kugou" ) ) ) return QStringLiteral( "酷狗音乐" );
    if ( lower.contains( QStringLiteral( "kuwo" ) ) ) return QStringLiteral( "酷我音乐" );
    if ( lower.contains( QStringLiteral( "spotify" ) ) ) return QStringLiteral( "Spotify" );
    if ( lower.contains( QStringLiteral( "applemusic" ) ) || lower.contains( QStringLiteral( "itunes" ) ) ) return QStringLiteral( "Apple Music" );
    if ( lower.contains( QStringLiteral( "foobar" ) ) ) return QStringLiteral( "foobar2000" );
    if ( lower.contains( QStringLiteral( "aimp" ) ) ) return QStringLiteral( "AIMP" );
    if ( lower.contains( QStringLiteral( "potplayer" ) ) ) return QStringLiteral( "PotPlayer" );
    if ( lower.contains( QStringLiteral( "vlc" ) ) ) return QStringLiteral( "VLC" );
    if ( lower.contains( QStringLiteral( "edge" ) ) ) return QStringLiteral( "Microsoft Edge" );
    if ( lower.contains( QStringLiteral( "chrome" ) ) ) return QStringLiteral( "Google Chrome" );
    if ( lower.contains( QStringLiteral( "firefox" ) ) ) return QStringLiteral( "Firefox" );

    QString clean = rawAppId;
    const int lastSlash = qMax( clean.lastIndexOf( QChar( '/' ) ), clean.lastIndexOf( QChar( '\\' ) ) );
    if ( lastSlash >= 0 ) clean = clean.mid( lastSlash + 1 );
    if ( clean.endsWith( QStringLiteral( ".exe" ), Qt::CaseInsensitive ) ) clean.chop( 4 );
    return clean;
}

bool isNonMusicMediaApp( const QString& appId, const QString& title, const QString& artist )
{
    const QString idLower = appId.toLower();
    if ( idLower.contains( QStringLiteral( "chrome" ) ) ||
         idLower.contains( QStringLiteral( "msedge" ) ) ||
         idLower.contains( QStringLiteral( "firefox" ) ) ||
         idLower.contains( QStringLiteral( "opera" ) ) ||
         idLower.contains( QStringLiteral( "brave" ) ) ||
         idLower.contains( QStringLiteral( "potplayer" ) ) ||
         idLower.contains( QStringLiteral( "vlc" ) ) ||
         idLower.contains( QStringLiteral( "bilibili" ) ) ||
         idLower.contains( QStringLiteral( "douyin" ) ) ||
         idLower.contains( QStringLiteral( "mpv" ) ) ||
         idLower.contains( QStringLiteral( "tencentmeeting" ) ) ||
         idLower.contains( QStringLiteral( "teams" ) ) ||
         idLower.contains( QStringLiteral( "zoom" ) ) )
    {
        return true;
    }

    const QString textLower = ( title + QStringLiteral( " " ) + artist ).toLower();
    if ( textLower.contains( QStringLiteral( "bilibili" ) ) ||
         textLower.contains( QStringLiteral( "youtube" ) ) ||
         textLower.contains( QStringLiteral( "twitch" ) ) ||
         textLower.contains( QStringLiteral( "douyu" ) ) ||
         textLower.contains( QStringLiteral( "huya" ) ) )
    {
        return true;
    }

    return false;
}

QSettings getAudiomaticSettings()
{
    return QSettings( QSettings::IniFormat, QSettings::UserScope, QStringLiteral( "FluentUIStyle" ), QStringLiteral( "AudiomaticMini" ) );
}

} // namespace

AudiomaticPlayerWidget::AudiomaticPlayerWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::AudiomaticPlayerWidget )
{
    ui->setupUi( this );
    setObjectName( QStringLiteral( "audiomatic-central" ) );
    setAttribute( Qt::WA_StyledBackground, true );

    ui->contentTabs->tabBar()->setProperty( TabBarStyleProperty, TabBarStyle::Pivot_Slide );
    ui->contentTabs->setStyleSheet( "QTabWidget::pane{border:none;}" );

    ui->btnAlwaysOnTop->setCheckable( true );
    ui->btnAlwaysOnTop->setAutoRaise( true );
    ui->btnTranslucent->setCheckable( true );
    ui->btnTranslucent->setAutoRaise( true );

    setupChrome();
    // 默认监听系统声音。这里不创建 QMediaPlayer：它会立刻用 FFmpeg 打开渲染设备，
    // 和本进程或已经在跑的 AudiomaticMini 的 WASAPI 回环撞在一起，MMDevApi 里会堆损坏。
    setupSystemAudioCapture();
    setupSystemMediaWatcher();
    setupConnections();
    setupOnlineMusic();
    setupSettingsMenu();
    setupImmersiveView();
    setupLyricsManager();
    setupCpuUsageMonitor();

    loadSettings();

    updateInputSourceUi();
    updateTrackLabel();
    updateTransportButtons();
    updateIcons();

    ui->stackedWidget->setCurrentIndex( m_immersiveMode ? 1 : 0 );
    ui->contentTabs->setCurrentIndex( 0 );
    loadApplicationDirectoryPlaylist();
}

AudiomaticPlayerWidget::~AudiomaticPlayerWidget()
{
    saveSettings();

    if ( m_mediaWatcher )
    {
        m_mediaWatcher->stop();
    }
    if ( m_systemCapture )
    {
        m_systemCapture->stop();
    }
    if ( m_player )
    {
        m_player->stop();
    }
    delete ui;
}

void AudiomaticPlayerWidget::setupChrome()
{
    ui->labelCover->setObjectName( QStringLiteral( "audiomatic-cover" ) );
    ui->labelCover->setCursor( Qt::PointingHandCursor );
    ui->labelCover->setToolTip( tr( "双击进入沉浸模式 (可显示专辑大图与歌词)" ) );
    ui->labelCover->installEventFilter( this );

    ui->labelTitle->setObjectName( QStringLiteral( "audiomatic-title" ) );
    ui->labelTitle->setTextInteractionFlags( Qt::NoTextInteraction );

    if ( ui->labelArtist )
    {
        ui->labelArtist->setObjectName( QStringLiteral( "audiomatic-artist" ) );
        ui->labelArtist->setTextInteractionFlags( Qt::NoTextInteraction );
    }

    ui->labelTimeCurrent->setObjectName( QStringLiteral( "audiomatic-time" ) );
    ui->labelTimeTotal->setObjectName( QStringLiteral( "audiomatic-time" ) );
    ui->labelTrackCount->setObjectName( QStringLiteral( "audiomatic-time" ) );
    if ( ui->labelCpuUsage )
    {
        ui->labelCpuUsage->setObjectName( QStringLiteral( "audiomatic-time" ) );
    }

    if ( ui->btnAudioSource )
    {
        ui->btnAudioSource->setObjectName( QStringLiteral( "audiomatic-audiosource" ) );
        ui->btnAudioSource->setCursor( Qt::PointingHandCursor );
        ui->btnAudioSource->setToolTip( tr( "切换音频输入源 (电脑声音 / 媒体播放器)" ) );
    }

    ui->sliderPosition->setRange( 0, 0 );
    ui->sliderPosition->setProperty( "showValueTip", false );
    ui->sliderVolume->setRange( 0, 100 );
    ui->sliderVolume->setValue( 70 );

    ui->btnPlayPause->setObjectName( QStringLiteral( "audiomatic-play" ) );
    ui->btnPlayPause->setProperty( ButtonAccentStyleProperty, true );
    ui->btnPlayPause->setIconSize( QSize( 18, 18 ) );

    // 音量图标 Segoe Fluent Icons (14px)
    ui->labelVolumeIcon->setFont( fluentIconFont( 14 ) );
    ui->labelVolumeIcon->setText( QString( kGlyphVolume ) );

    if ( ui->visualizer )
    {
        ui->visualizer->installEventFilter( this );
    }

    if ( ui->lyricsWidget )
    {
        ui->lyricsWidget->setViewMode( ExLyricsWidget::ViewMode::MultiLine );
        ui->lyricsWidget->setKaraokeProgress( true );
        ui->lyricsWidget->setHighlightColor( lyricHighlightColor() );
        ui->lyricsWidget->setFontSizeLevel( m_lyricFontSizeLevel );
        connect( ui->lyricsWidget, &ExLyricsWidget::lineClicked, this, [ this ]( int, qint64 positionMs, const QString& ) {
            if ( m_inputSource == AudioInputSource::MediaPlayer && m_player )
            {
                m_player->setPosition( positionMs );
            }
        } );
    }

    ui->btnTranslucent->setCheckable( true );
    ui->btnTranslucent->setAutoRaise( true );
    ui->btnAlwaysOnTop->setCheckable( true );
    ui->btnAlwaysOnTop->setAutoRaise( true );

    ui->sliderOpacity->setRange( 30, 100 );
    ui->sliderOpacity->setValue( m_translucentOpacity );
    ui->labelOpacityValue->setText( QStringLiteral( "%1%" ).arg( m_translucentOpacity ) );
    updateOverlayButtonsState();

    updateIcons();
}

void AudiomaticPlayerWidget::showEvent( QShowEvent* event )
{
    QWidget::showEvent( event );
    // 通知宿主窗口。无边框标题栏在这次 Show 里才装上，透明度要等这一轮事件结束再设一次。
    emit windowChromeChanged();
    QTimer::singleShot( 0, this, [ this ]() { emit windowChromeChanged(); } );
    maybeStartFirstRunTour();
}

void AudiomaticPlayerWidget::maybeStartFirstRunTour()
{
    if ( m_firstRunTourChecked )
    {
        return;
    }

    // 等本次显示完成后再判断。构造期间控件还不可见，向导会把步骤全部跳过。
    QTimer::singleShot( 0, this, [ this ]() {
        if ( m_firstRunTourChecked || !isVisible() )
        {
            return;
        }
        m_firstRunTourChecked = true;

        QSettings settings = getAudiomaticSettings();
        if ( settings.value( QStringLiteral( "General/FirstRunTourCompleted" ), false ).toBool() )
        {
            return;
        }
        startTour();
    } );
}

void AudiomaticPlayerWidget::changeEvent( QEvent* event )
{
    QWidget::changeEvent( event );
    if ( event->type() == QEvent::PaletteChange ||
         event->type() == QEvent::ApplicationPaletteChange ||
         event->type() == QEvent::StyleChange ||
         event->type() == QEvent::ThemeChange )
    {
        updateIcons();
        if ( !m_visualizerAccentColor.isValid() )
        {
            setVisualizerAccentColor( QColor() );
        }
        refreshLyricsStyle();
        updateTrackLabel();
        updateOverlayButtonsState();
        if ( ui->lyricsWidget )
        {
            ui->lyricsWidget->update();
        }
        if ( m_immersiveView )
        {
            m_immersiveView->update();
        }
        update();
    }
}

void AudiomaticPlayerWidget::updateIcons()
{
    const auto setTransport = [ this ]( QToolButton* button, const QChar& glyph, int size )
    {
        if ( !button ) return;
        button->setIcon( transportIcon( glyph, 16 ) );
        button->setIconSize( QSize( 16, 16 ) );
        button->setFixedSize( size, size );
    };

    setTransport( ui->btnShuffle, kGlyphShuffle, 32 );
    setTransport( ui->btnPrevious, kGlyphPrevious, 32 );
    setTransport( ui->btnNext, kGlyphNext, 32 );
    setTransport( ui->btnSettings, kGlyphSettings, 32 );

    const bool isSystem                 = ( m_inputSource == AudioInputSource::SystemAudio );
    const bool capturing                = m_systemCapture && m_systemCapture->isCapturing();
    const SystemMediaTrackInfo sysTrack = ( isSystem && m_mediaWatcher ) ? m_mediaWatcher->currentTrackInfo() : SystemMediaTrackInfo{};
    const bool playing                  = isSystem ? ( sysTrack.isValid() ? sysTrack.isPlaying : capturing )
                                                   : ( m_player && m_player->playbackState() == QMediaPlayer::PlayingState );
    ui->btnPlayPause->setIcon( transportIcon( playing ? kGlyphPause : kGlyphPlay, 18, true ) );
    ui->btnPlayPause->setFixedSize( 38, 38 );

    const QChar repeatGlyph = ( m_repeat == RepeatMode::One ) ? kGlyphRepeatOne : kGlyphRepeat;
    setTransport( ui->btnRepeat, repeatGlyph, 32 );

    // 音量图标 Segoe Fluent Icons (14px)
    ui->labelVolumeIcon->setFont( fluentIconFont( 14 ) );
    ui->labelVolumeIcon->setText( QString( kGlyphVolume ) );

    ui->btnChooseFolder->setIcon( transportIcon( kGlyphFolder, 14 ) );

    if ( ui->btnAlwaysOnTop )
    {
        ui->btnAlwaysOnTop->setIcon( transportIcon( kGlyphPin, 14, ui->btnAlwaysOnTop->isChecked() ) );
        ui->btnAlwaysOnTop->setIconSize( QSize( 14, 14 ) );
        ui->btnAlwaysOnTop->setFixedSize( 28, 28 );
    }

    if ( ui->btnTranslucent )
    {
        ui->btnTranslucent->setIcon( transportIcon( kGlyphTransparency, 14, ui->btnTranslucent->isChecked() ) );
        ui->btnTranslucent->setIconSize( QSize( 14, 14 ) );
        ui->btnTranslucent->setFixedSize( 28, 28 );
    }

    if ( m_actSourceSystem ) m_actSourceSystem->setIcon( ExFontIcon::instance()->getIcon( SegoeIcon::Volume ) );
    if ( m_actSourcePlayer ) m_actSourcePlayer->setIcon( ExFontIcon::instance()->getIcon( SegoeIcon::MusicInfo ) );
}

QColor AudiomaticPlayerWidget::iconForegroundColor( bool onAccent ) const
{
    if ( onAccent )
    {
        return QColor( 255, 255, 255 );
    }

    const QVariant scheme = qApp->property( "_q_colorscheme" );
    const bool dark = scheme.isValid() ? scheme.toInt() == 1
                                       : qApp->palette().color( QPalette::Window ).lightness() < 128;
    return dark ? QColor( 255, 255, 255 ) : QColor( 0, 0, 0 );
}

void AudiomaticPlayerWidget::releaseMediaOutput()
{
    if ( m_spectrumAnalyzer )
    {
        m_spectrumAnalyzer->setActive( false );
    }
    if ( m_player )
    {
        m_player->pause();
        m_player->setAudioOutput( nullptr );
    }
    delete m_audioOutput;
    m_audioOutput = nullptr;
}

void AudiomaticPlayerWidget::ensureMediaPlayer()
{
    if ( !m_player )
    {
        m_player = new QMediaPlayer( this );
    }

    if ( !m_audioOutput )
    {
        m_audioOutput = new QAudioOutput( this );
        m_audioOutput->setVolume( ui->sliderVolume->value() / 100.0 );
        m_player->setAudioOutput( m_audioOutput );
    }

    static const QList<qreal> speeds = { 0.5, 0.75, 1.0, 1.25, 1.5, 2.0 };
    if ( m_speedIndex >= 0 && m_speedIndex < speeds.size() )
    {
        m_player->setPlaybackRate( speeds.at( m_speedIndex ) );
    }

    if ( m_spectrumAnalyzer )
    {
        return;
    }

    m_spectrumAnalyzer = new AudioSpectrumAnalyzer( this );

    connect( m_player, &QMediaPlayer::durationChanged, this, [ this ]( qint64 duration ) {
        if ( m_inputSource == AudioInputSource::MediaPlayer )
        {
            ui->sliderPosition->setRange( 0, static_cast<int>( duration ) );
            ui->labelTimeTotal->setText( formatTime( duration ) );
        }
    } );

    connect( m_player, &QMediaPlayer::positionChanged, this, [ this ]( qint64 position ) {
        if ( m_inputSource != AudioInputSource::MediaPlayer )
        {
            return;
        }
        if ( !m_seeking )
        {
            ui->sliderPosition->setValue( static_cast<int>( position ) );
            ui->labelTimeCurrent->setText( formatTime( position ) );
            updateLyricsDisplay( position );
        }
        if ( m_spectrumAnalyzer )
        {
            m_spectrumAnalyzer->setPlaybackPositionMs( position );
        }
    } );

    connect( m_player, &QMediaPlayer::playbackStateChanged, this, [ this ]( QMediaPlayer::PlaybackState state ) {
        const bool playing = state == QMediaPlayer::PlayingState;
        if ( m_spectrumAnalyzer && m_inputSource == AudioInputSource::MediaPlayer )
        {
            if ( playing && m_player )
            {
                m_spectrumAnalyzer->setPlaybackPositionMs( m_player->position() );
            }
            m_spectrumAnalyzer->setActive( playing );
        }
        updateTransportButtons();
        updateIcons();
    } );

    connect( m_player, &QMediaPlayer::mediaStatusChanged, this, [ this ]( QMediaPlayer::MediaStatus status ) {
        if ( status != QMediaPlayer::EndOfMedia
             || m_inputSource != AudioInputSource::MediaPlayer
             || !m_player )
        {
            return;
        }

        if ( m_repeat == RepeatMode::One )
        {
            m_player->setPosition( 0 );
            m_player->play();
        }
        else if ( m_repeat == RepeatMode::All || m_shuffle )
        {
            stepPlayback( 1, true );
        }
        else
        {
            stepPlayback( 1, false );
        }
    } );

    connect( m_spectrumAnalyzer, &AudioSpectrumAnalyzer::pcmDataReady, this,
             [ this ]( const QByteArray& pcmData ) {
                 if ( m_inputSource == AudioInputSource::MediaPlayer )
                 {
                     if ( ui->visualizer )
                     {
                         ui->visualizer->setAudioData( pcmData );
                     }
                     if ( m_immersiveView )
                     {
                         m_immersiveView->setAudioData( pcmData );
                     }
                 }
             } );

    connect( m_spectrumAnalyzer, &AudioSpectrumAnalyzer::sampleRateChanged, this,
             [ this ]( int rate ) {
                 if ( m_inputSource == AudioInputSource::MediaPlayer )
                 {
                     if ( ui->visualizer )
                     {
                         ui->visualizer->setSampleRate( rate );
                     }
                     if ( m_immersiveView )
                     {
                         m_immersiveView->setSampleRate( rate );
                     }
                 }
             } );
}

void AudiomaticPlayerWidget::setupSystemAudioCapture()
{
    m_systemCapture = new SystemAudioCapture( this );

    m_systemLiveTimer = new QTimer( this );
    m_systemLiveTimer->setInterval( 200 );
    connect( m_systemLiveTimer,
             &QTimer::timeout,
             this,
             [ this ]()
             {
                 if ( m_inputSource != AudioInputSource::SystemAudio )
                 {
                     return;
                 }
                 const SystemMediaTrackInfo sysTrack = m_mediaWatcher ? m_mediaWatcher->currentTrackInfo() : SystemMediaTrackInfo{};
                 if ( !sysTrack.isValid() )
                 {
                     m_systemLiveElapsedMs += 200;
                     ui->labelTimeCurrent->setText( formatTime( m_systemLiveElapsedMs ) );
                     ui->labelTimeTotal->setText( tr( "实时捕获" ) );
                     ui->sliderPosition->setRange( 0, 100 );
                     ui->sliderPosition->setValue( 50 );
                 }
                 else
                 {
                     if ( sysTrack.durationMs > 0 )
                     {
                         ui->sliderPosition->setRange( 0, static_cast<int>( sysTrack.durationMs ) );
                         ui->labelTimeTotal->setText( formatTime( sysTrack.durationMs ) );
                     }
                 }
             } );

    connect( m_systemCapture,
             &SystemAudioCapture::pcmDataReady,
             this,
             [ this ]( const QByteArray& pcmData, int channels )
             {
                 if ( m_inputSource != AudioInputSource::SystemAudio )
                 {
                     return;
                 }
                 if ( ui->visualizer )
                 {
                     ui->visualizer->pushAudioData( pcmData, channels );
                 }
                 if ( m_immersiveView )
                 {
                     m_immersiveView->pushAudioData( pcmData, channels );
                 }
             } );

    connect( m_systemCapture,
             &SystemAudioCapture::sampleRateChanged,
             this,
             [ this ]( int sampleRate )
             {
                 if ( m_inputSource != AudioInputSource::SystemAudio )
                 {
                     return;
                 }
                 if ( ui->visualizer )
                 {
                     ui->visualizer->setSampleRate( sampleRate );
                 }
                 if ( m_immersiveView )
                 {
                     m_immersiveView->setSampleRate( sampleRate );
                 }
             } );

    connect( m_systemCapture,
             &SystemAudioCapture::capturingChanged,
             this,
             [ this ]( bool isCapturing )
             {
                 if ( m_inputSource == AudioInputSource::SystemAudio )
                 {
                     updateTransportButtons();
                     updateIcons();
                 }
             } );

    connect( m_systemCapture,
             &SystemAudioCapture::errorOccurred,
             this,
             [ this ]( const QString& err )
             {
                 qWarning() << "[SystemAudioCapture]" << err;
             } );

    if ( m_inputSource == AudioInputSource::SystemAudio )
    {
        m_systemLiveElapsedMs = 0;
        if ( m_systemLiveTimer )
        {
            m_systemLiveTimer->start();
        }
        if ( m_systemCapture )
        {
            m_systemCapture->start();
        }
    }
}

void AudiomaticPlayerWidget::setupSystemMediaWatcher()
{
    m_mediaWatcher = new SystemMediaWatcher( this );
    connect( m_mediaWatcher,
             &SystemMediaWatcher::trackInfoChanged,
             this,
             [ this ]( const SystemMediaTrackInfo& info )
             {
                 updateTrackLabel();
                 updateTransportButtons();
                 updateIcons();

                 if ( m_inputSource == AudioInputSource::SystemAudio && info.isValid() )
                 {
                     if ( !info.isPlaying )
                     {
                         m_systemLyricsBaseMs = info.positionMs;
                         m_systemLyricsElapsed.invalidate();
                         updateLyricsDisplay( info.positionMs );
                         if ( !m_seeking && info.durationMs > 0 )
                         {
                             const qint64 displayPos = qMin( info.positionMs, info.durationMs );
                             ui->sliderPosition->setValue( int( displayPos ) );
                             ui->labelTimeCurrent->setText( formatTime( displayPos ) );
                         }
                     }
                     else if ( !m_systemLyricsElapsed.isValid() )
                     {
                         m_systemLyricsBaseMs = info.positionMs;
                         m_systemLyricsElapsed.restart();
                     }
                 }
             } );
    connect( m_mediaWatcher,
             &SystemMediaWatcher::positionChanged,
             this,
             [ this ]( qint64 positionMs, qint64 durationMs )
             {
                 if ( m_inputSource == AudioInputSource::SystemAudio && positionMs >= 0 )
                 {
                     m_lastMediaWatcherPositionMs = positionMs;
                     m_mediaWatcherPosElapsed.restart();

                     const SystemMediaTrackInfo sysTrack = m_mediaWatcher ? m_mediaWatcher->currentTrackInfo() : SystemMediaTrackInfo{};
                     const bool isPlaying = sysTrack.isValid() ? sysTrack.isPlaying : false;

                     if ( !isPlaying )
                     {
                         m_systemLyricsBaseMs = positionMs;
                         m_systemLyricsElapsed.invalidate();
                     }
                     else
                     {
                         m_systemLyricsBaseMs = positionMs;
                         m_systemLyricsElapsed.restart();
                     }

                     updateLyricsDisplay( positionMs );

                     if ( !m_seeking && durationMs > 0 )
                     {
                         const qint64 displayPos = qMin( positionMs, durationMs );
                         ui->sliderPosition->setValue( int( displayPos ) );
                         ui->labelTimeCurrent->setText( formatTime( displayPos ) );
                     }
                 }
             } );
    m_mediaWatcher->start();
}

void AudiomaticPlayerWidget::setAudioInputSource( AudioInputSource source )
{
    if ( m_inputSource == source )
    {
        return;
    }

    m_inputSource = source;

    if ( m_inputSource == AudioInputSource::SystemAudio )
    {
        // 先放开渲染设备，再开回环。两个客户端同时占着默认输出端点会把 MMDevApi 的堆写坏。
        releaseMediaOutput();
        m_systemLiveElapsedMs = 0;
        if ( m_systemLiveTimer )
        {
            m_systemLiveTimer->start();
        }
        if ( m_systemCapture )
        {
            m_systemCapture->start();
        }
    }
    else
    {
        if ( m_systemLiveTimer )
        {
            m_systemLiveTimer->stop();
        }
        if ( m_systemCapture )
        {
            m_systemCapture->stop();
        }
        if ( ui->visualizer )
        {
            ui->visualizer->clear();
        }
        if ( m_immersiveView )
        {
            m_immersiveView->clearAudio();
        }
        ensureMediaPlayer();
        if ( m_player )
        {
            const qint64 duration = qMax<qint64>( 0, m_player->duration() );
            const qint64 position = qMax<qint64>( 0, m_player->position() );
            ui->sliderPosition->setRange( 0, static_cast<int>( duration ) );
            ui->sliderPosition->setValue( static_cast<int>( position ) );
            ui->labelTimeCurrent->setText( formatTime( position ) );
            ui->labelTimeTotal->setText( formatTime( duration ) );
            updateLyricsDisplay( position );
        }
        if ( m_spectrumAnalyzer && m_player )
        {
            const bool playing = m_player->playbackState() == QMediaPlayer::PlayingState;
            if ( playing )
            {
                m_spectrumAnalyzer->setPlaybackPositionMs( m_player->position() );
            }
            m_spectrumAnalyzer->setActive( playing );
        }
    }

    updateInputSourceUi();
    updateTrackLabel();
    updateTransportButtons();
    updateIcons();
}

void AudiomaticPlayerWidget::updateInputSourceUi()
{
    const bool isSystem = ( m_inputSource == AudioInputSource::SystemAudio );
    if ( ui->btnAudioSource )
    {
        ui->btnAudioSource->setText( isSystem ? tr( "电脑声音" ) : tr( "媒体播放器" ) );
        ui->btnAudioSource->setIcon( isSystem ? ExFontIcon::instance()->getIcon( SegoeIcon::Volume ) : ExFontIcon::instance()->getIcon( SegoeIcon::MusicInfo ) );
    }
    if ( m_actSourceSystem ) { m_actSourceSystem->setChecked( isSystem ); }
    if ( m_actSourcePlayer ) { m_actSourcePlayer->setChecked( !isSystem ); }
}

void AudiomaticPlayerWidget::showAudioSourceMenu()
{
    QMenu menu( this );
    auto* actSys = menu.addAction( ExFontIcon::instance()->getIcon( SegoeIcon::Volume ), tr( "电脑声音 (系统音频实时捕获)" ) );
    actSys->setCheckable( true );
    actSys->setChecked( m_inputSource == AudioInputSource::SystemAudio );
    connect( actSys, &QAction::triggered, this, [ this ]() {
        setAudioInputSource( AudioInputSource::SystemAudio );
    } );

    auto* actPlayer = menu.addAction( ExFontIcon::instance()->getIcon( SegoeIcon::MusicInfo ), tr( "媒体播放器 (本地文件 / Jamendo 电台)" ) );
    actPlayer->setCheckable( true );
    actPlayer->setChecked( m_inputSource == AudioInputSource::MediaPlayer );
    connect( actPlayer, &QAction::triggered, this, [ this ]() {
        setAudioInputSource( AudioInputSource::MediaPlayer );
    } );

    menu.exec( QCursor::pos() );
}

void AudiomaticPlayerWidget::updateTrackLabel()
{
    QString titleText;
    QString artistText;
    QString statusDeviceText;
    SystemMediaTrackInfo sysTrack;

    if ( m_inputSource == AudioInputSource::SystemAudio )
    {
        const QString devName = m_systemCapture ? m_systemCapture->currentDeviceName() : tr( "系统音频" );
        statusDeviceText      = tr( "正在监听: %1" ).arg( devName );

        sysTrack = m_mediaWatcher ? m_mediaWatcher->currentTrackInfo() : SystemMediaTrackInfo{};
        if ( sysTrack.isValid() )
        {
            titleText  = sysTrack.title;
            artistText = sysTrack.artist;
            if ( !sysTrack.appId.isEmpty() )
            {
                const QString appFriendly = formatAppFriendlyName( sysTrack.appId );
                if ( !artistText.isEmpty() )
                {
                    artistText = QStringLiteral( "%1 · %2" ).arg( artistText, appFriendly );
                }
                else
                {
                    artistText = appFriendly;
                }
            }

            ui->labelTitle->setText( titleText.isEmpty() ? sysTrack.displayText() : titleText );
            if ( ui->labelArtist )
            {
                ui->labelArtist->setText( artistText );
                ui->labelArtist->setVisible( !artistText.isEmpty() );
            }
            ui->labelTitle->setToolTip( tr( "当前播放: %1\n声卡设备: %2" ).arg( sysTrack.displayText(), devName ) );
        }
        else
        {
            ui->labelTitle->setText( statusDeviceText );
            if ( ui->labelArtist )
            {
                ui->labelArtist->clear();
                ui->labelArtist->hide();
            }
            ui->labelTitle->setToolTip( statusDeviceText );
        }
    }
    else
    {
        if ( const PlaylistEntry* entry = currentMediaEntry() )
        {
            titleText = entry->title;
            artistText = entry->artist;
            ui->labelTitle->setText( titleText );
            if ( ui->labelArtist )
            {
                ui->labelArtist->setText( artistText );
                ui->labelArtist->setVisible( !artistText.isEmpty() );
            }
        }
        else
        {
            titleText = tr( "未选择音乐" );
            ui->labelTitle->setText( titleText );
            if ( ui->labelArtist )
            {
                ui->labelArtist->clear();
                ui->labelArtist->hide();
            }
        }
        ui->labelTitle->setToolTip( QString() );
    }

    // 同步沉浸模式歌曲与歌手信息
    if ( m_immersiveView )
    {
        m_immersiveView->setTrackInfo( titleText, artistText );
    }

    // 检查并提取原生系统播放器上报的高清专辑缩略图
    if ( m_inputSource == AudioInputSource::SystemAudio )
    {
        if ( !sysTrack.thumbnailData.isEmpty() )
        {
            QPixmap thumb;
            if ( thumb.loadFromData( sysTrack.thumbnailData ) )
            {
                setCoverPixmap( thumb );
            }
        }
        else if ( !titleText.isEmpty() && m_lyricsManager && !m_lyricsManager->currentCoverPixmap().isNull() )
        {
            setCoverPixmap( m_lyricsManager->currentCoverPixmap() );
        }
        else if ( !sysTrack.isValid() )
        {
            setCoverPixmap( QPixmap() );
        }
    }

    // 检查是否属于非音乐媒体应用（浏览器、视频播放器、会议等），智能免除歌词检索
    const bool isNonMusic = ( m_inputSource == AudioInputSource::SystemAudio &&
                              m_ignoreNonMusicLyrics &&
                              isNonMusicMediaApp( sysTrack.appId, titleText, sysTrack.artist ) );
    if ( isNonMusic )
    {
        m_currentLyricsQueryTitle.clear();
        m_currentLyricsQueryArtist.clear();
        if ( m_lyricsManager )
        {
            m_lyricsManager->clear();
        }
        m_currentLyricLineIndex = -1;
        if ( ui->lyricsWidget )
        {
            ui->lyricsWidget->setStatusMessage( tr( "视频/网页音频播放中" ) );
            ui->lyricsWidget->clear();
        }
        if ( m_immersiveView )
        {
            m_immersiveView->setLyricStatusMessage( tr( "视频/网页音频播放中" ) );
            m_immersiveView->clearLyrics();
        }
        return;
    }

    // 歌词查询必须用当前输入源自己的歌名和歌手。
    // 系统声音用 SMTC 的原始歌手；本地文件用文件名里拆出的歌手。
    // 占位文案（未选择音乐、正在监听设备）不是曲目，不能拿去检索。
    QString queryArtist;
    qint64 queryDuration = 0;
    bool hasLyricTarget = false;
    if ( m_inputSource == AudioInputSource::SystemAudio )
    {
        hasLyricTarget = sysTrack.isValid() && !titleText.isEmpty();
        queryArtist = sysTrack.artist;
        queryDuration = sysTrack.durationMs;
    }
    else
    {
        hasLyricTarget = currentMediaEntry() && !titleText.isEmpty();
        queryArtist = artistText;
        if ( m_player && m_player->duration() > 0 )
        {
            queryDuration = m_player->duration();
        }
    }

    if ( hasLyricTarget )
    {
        if ( titleText != m_currentLyricsQueryTitle || queryArtist != m_currentLyricsQueryArtist )
        {
            m_currentLyricsQueryTitle  = titleText;
            m_currentLyricsQueryArtist = queryArtist;
            m_currentLyricLineIndex    = -1;
            m_lyricsUserOffsetMs       = 0;
            if ( ui->lyricsWidget )
            {
                ui->lyricsWidget->setStatusMessage( tr( "正在搜索歌词..." ) );
                ui->lyricsWidget->clear();
            }
            if ( m_immersiveView )
            {
                m_immersiveView->setLyricStatusMessage( tr( "正在搜索歌词..." ) );
                m_immersiveView->clearLyrics();
            }
            if ( m_lyricsManager )
            {
                m_lyricsManager->fetchLyrics( titleText, queryArtist, queryDuration );
            }
        }
    }
    else
    {
        m_currentLyricsQueryTitle.clear();
        m_currentLyricsQueryArtist.clear();
        if ( m_lyricsManager )
        {
            m_lyricsManager->clear();
        }
        m_currentLyricLineIndex = -1;
        if ( ui->lyricsWidget )
        {
            ui->lyricsWidget->setStatusMessage( QString() );
            ui->lyricsWidget->clear();
        }
        if ( m_immersiveView )
        {
            m_immersiveView->setLyricStatusMessage( QString() );
            m_immersiveView->clearLyrics();
        }
    }
}

void AudiomaticPlayerWidget::setupConnections()
{
    connect( ui->btnChooseFolder, &QPushButton::clicked, this, [ this ]() {
        const QString folder = QFileDialog::getExistingDirectory(
            this, tr( "选择音乐文件夹" ), QStandardPaths::writableLocation( QStandardPaths::MusicLocation ) );
        if ( !folder.isEmpty() )
        {
            scanFolder( folder );
        }
    } );

    connect( ui->btnAudioSource, &QPushButton::clicked, this, &AudiomaticPlayerWidget::showAudioSourceMenu );
    connect( ui->btnImmersive, &QPushButton::clicked, this, [ this ]() { setImmersiveMode( true ); } );
    connect( ui->btnTranslucent, &QToolButton::clicked, this, [ this ]( bool checked ) { setTranslucentEnabled( checked ); } );
    connect( ui->sliderOpacity, &QSlider::valueChanged, this, [ this ]( int value ) { setTranslucentOpacity( value ); } );
    connect( ui->btnAlwaysOnTop, &QToolButton::clicked, this, [ this ]( bool checked ) { setAlwaysOnTop( checked ); } );

    connect( ui->btnShuffle, &QToolButton::toggled, this, [ this ]( bool checked ) { m_shuffle = checked; } );
    connect( ui->btnRepeat, &QToolButton::clicked, this, [ this ]() {
        if ( m_repeat == RepeatMode::None )
        {
            m_repeat = RepeatMode::All;
            ui->btnRepeat->setChecked( true );
        }
        else if ( m_repeat == RepeatMode::All )
        {
            m_repeat = RepeatMode::One;
        }
        else
        {
            m_repeat = RepeatMode::None;
            ui->btnRepeat->setChecked( false );
        }
        updateIcons();
    } );

    connect( ui->btnSpeed, &QToolButton::clicked, this, [ this ]() {
        static const QList<qreal> speeds = { 0.5, 0.75, 1.0, 1.25, 1.5, 2.0 };
        m_speedIndex = ( m_speedIndex + 1 ) % speeds.size();
        const qreal speed = speeds[ m_speedIndex ];
        ui->btnSpeed->setText( QStringLiteral( "%1x" ).arg( speed ) );
        if ( m_player )
        {
            m_player->setPlaybackRate( speed );
        }
    } );

    connect( ui->btnPrevious, &QToolButton::clicked, this, [ this ]() {
        if ( m_inputSource == AudioInputSource::SystemAudio )
        {
            if ( m_mediaWatcher ) m_mediaWatcher->previousTrack();
            return;
        }
        stepPlayback( -1, true );
    } );

    connect( ui->btnNext, &QToolButton::clicked, this, [ this ]() {
        if ( m_inputSource == AudioInputSource::SystemAudio )
        {
            if ( m_mediaWatcher ) m_mediaWatcher->nextTrack();
            return;
        }
        stepPlayback( 1, true );
    } );

    connect( ui->btnPlayPause, &QToolButton::clicked, this, [ this ]() {
        if ( m_inputSource == AudioInputSource::SystemAudio )
        {
            const SystemMediaTrackInfo sysTrack = m_mediaWatcher ? m_mediaWatcher->currentTrackInfo() : SystemMediaTrackInfo{};
            if ( sysTrack.isValid() && m_mediaWatcher )
            {
                m_mediaWatcher->playPause();
            }
            else if ( m_systemCapture )
            {
                if ( m_systemCapture->isCapturing() )
                {
                    m_systemCapture->stop();
                    if ( m_systemLiveTimer ) m_systemLiveTimer->stop();
                    if ( ui->visualizer ) ui->visualizer->clear();
                    if ( m_immersiveView ) m_immersiveView->clearAudio();
                }
                else
                {
                    m_systemCapture->start();
                    if ( m_systemLiveTimer ) m_systemLiveTimer->start();
                }
                updateIcons();
                updateTrackLabel();
                updateTransportButtons();
            }
            return;
        }

        ensureMediaPlayer();
        if ( !m_player )
        {
            return;
        }
        if ( m_player->playbackState() == QMediaPlayer::PlayingState )
        {
            m_player->pause();
        }
        else if ( currentMediaEntry() )
        {
            m_player->play();
        }
        else if ( !m_playlist.isEmpty() )
        {
            playIndex( 0 );
        }
        else if ( !m_onlineTracks.isEmpty() )
        {
            playOnlineIndex( 0 );
        }
    } );

    connect( ui->sliderPosition, &QSlider::sliderPressed, this, [ this ]() { m_seeking = true; } );
    connect( ui->sliderPosition, &QSlider::sliderMoved, this, [ this ]( int value ) {
        ui->labelTimeCurrent->setText( formatTime( value ) );
        if ( m_inputSource == AudioInputSource::MediaPlayer && m_player )
        {
            m_player->setPosition( value );
            if ( m_spectrumAnalyzer ) m_spectrumAnalyzer->setPlaybackPositionMs( value );
        }
    } );
    connect( ui->sliderPosition, &QSlider::sliderReleased, this, [ this ]() {
        if ( m_inputSource == AudioInputSource::SystemAudio )
        {
            if ( m_mediaWatcher ) m_mediaWatcher->seekTo( ui->sliderPosition->value() );
        }
        else if ( m_player )
        {
            m_player->setPosition( ui->sliderPosition->value() );
        }
        m_seeking = false;
        if ( ui->visualizer ) ui->visualizer->clear();
        if ( m_spectrumAnalyzer ) m_spectrumAnalyzer->restart();
    } );

    connect( ui->sliderVolume, &QSlider::valueChanged, this, [ this ]( int value ) {
        if ( m_audioOutput ) m_audioOutput->setVolume( value / 100.0 );
    } );

    connect( ui->listPlaylist, &QListWidget::itemDoubleClicked, this, [ this ]( QListWidgetItem* item ) {
        if ( !item ) return;
        playIndex( ui->listPlaylist->row( item ) );
    } );
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
                    playOnlineIndex(row);
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
    m_onlineTracks.clear();
    m_onlineTracks.reserve(tracks.size());
    for (const JamendoTrack &track : tracks)
    {
        PlaylistEntry entry;
        entry.title = track.name;
        entry.artist = track.artistName;
        entry.source = track.audioUrl;
        m_onlineTracks.append(entry);
    }
    m_onlineIndex = -1;

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

    if (!m_onlineTracks.isEmpty())
    {
        playOnlineIndex(0);
    }
}

void AudiomaticPlayerWidget::onJamendoRadiosReady(const QList<JamendoRadio> &radios)
{
    setJamendoBusy(false);
    m_onlineListMode = OnlineListMode::Radios;
    m_jamendoRadios = radios;
    updateOnlineResultsView();
    ui->contentTabs->setCurrentWidget(ui->tabOnline);
    ui->labelJamendoHint->setText(tr("Jamendo 电台 · 双击开始收听"));
}

void AudiomaticPlayerWidget::onJamendoRadioStreamReady(const JamendoRadio &radio)
{
    setJamendoBusy(false);

    m_onlineListMode = OnlineListMode::Radios;
    m_playbackQueue = PlaybackQueue::Radio;
    m_radioNowPlaying = {};
    m_radioNowPlaying.title = tr("电台 · %1").arg(radio.displayName);
    m_radioNowPlaying.source = radio.streamUrl;
    m_radioIndex = -1;
    for (int i = 0; i < m_jamendoRadios.size(); ++i)
    {
        if (m_jamendoRadios[i].id == radio.id)
        {
            m_radioIndex = i;
            break;
        }
    }

    startMediaPlayback(radio.streamUrl, false);
    ui->sliderPosition->setRange(0, 0);
    ui->labelTimeCurrent->setText(formatTime(0));
    ui->labelTimeTotal->setText(QString());
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

QIcon AudiomaticPlayerWidget::transportIcon(const QChar &glyph, int pixelSize, bool onAccent) const
{
    QPixmap pixmap(pixelSize, pixelSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setFont(fluentIconFont(pixelSize));
    painter.setPen(iconForegroundColor(onAccent));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, QString(glyph));
    return QIcon(pixmap);
}

void AudiomaticPlayerWidget::loadApplicationDirectoryPlaylist()
{
    const QString folder = QCoreApplication::applicationDirPath();
    QList<PlaylistEntry> entries;
    const QFileInfoList files = QDir( folder ).entryInfoList( QDir::Files );
    for ( const QFileInfo& info : files )
    {
        if ( isAudioFile( info ) )
        {
            entries.append( playlistEntryFromFile( info ) );
        }
    }

    std::sort( entries.begin(), entries.end(), []( const PlaylistEntry& a, const PlaylistEntry& b ) {
        return a.listLabel().compare( b.listLabel(), Qt::CaseInsensitive ) < 0;
    } );

    m_playlist = entries;
    if ( m_playbackQueue == PlaybackQueue::Local )
    {
        m_currentIndex = -1;
    }
    updateTrackCountLabel();
    updatePlaylistView();

    if ( !entries.isEmpty() && ui->contentTabs && ui->tabPlaylist )
    {
        ui->contentTabs->setCurrentWidget( ui->tabPlaylist );
    }
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
            entries.append(playlistEntryFromFile(info));
        }
    }

    std::sort(entries.begin(), entries.end(), [](const PlaylistEntry &a, const PlaylistEntry &b)
              { return a.listLabel().compare(b.listLabel(), Qt::CaseInsensitive) < 0; });

    if (entries.isEmpty())
    {
        m_playlist.clear();
        if (m_playbackQueue == PlaybackQueue::Local)
        {
            m_currentIndex = -1;
        }
        updateTrackCountLabel();
        updatePlaylistView();
        updateTrackLabel();
        return;
    }

    setPlaylist(entries, 0);
}

void AudiomaticPlayerWidget::setPlaylist(const QList<PlaylistEntry> &entries, int startIndex)
{
    m_playlist = entries;
    m_currentIndex = -1;
    updateTrackCountLabel();
    updatePlaylistView();

    if (!m_playlist.isEmpty())
    {
        playIndex(qBound(0, startIndex, m_playlist.size() - 1));
    }
    else
    {
        updateTrackLabel();
    }
}

const PlaylistEntry *AudiomaticPlayerWidget::currentMediaEntry() const
{
    if (m_playbackQueue == PlaybackQueue::Online)
    {
        if (m_onlineIndex >= 0 && m_onlineIndex < m_onlineTracks.size())
        {
            return &m_onlineTracks[m_onlineIndex];
        }
        return nullptr;
    }
    if (m_playbackQueue == PlaybackQueue::Radio)
    {
        if (!m_radioNowPlaying.source.isEmpty())
        {
            return &m_radioNowPlaying;
        }
        return nullptr;
    }
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size())
    {
        return &m_playlist[m_currentIndex];
    }
    return nullptr;
}

void AudiomaticPlayerWidget::startMediaPlayback(const QUrl &source, bool enableAnalyzer)
{
    if (m_inputSource != AudioInputSource::MediaPlayer)
    {
        setAudioInputSource(AudioInputSource::MediaPlayer);
    }
    else
    {
        ensureMediaPlayer();
    }
    if (!m_player)
    {
        return;
    }

    m_player->setSource(source);
    if (m_spectrumAnalyzer)
    {
        if (enableAnalyzer)
        {
            m_spectrumAnalyzer->setSource(source);
            if (m_player->playbackState() == QMediaPlayer::PlayingState)
            {
                m_spectrumAnalyzer->restart();
            }
        }
        else
        {
            m_spectrumAnalyzer->setActive(false);
        }
    }
    m_player->play();
    updateTrackLabel();
    updateTransportButtons();
    updatePlaylistView();
    updateOnlineResultsView();
}

void AudiomaticPlayerWidget::playIndex(int index)
{
    if (index < 0 || index >= m_playlist.size())
    {
        return;
    }

    m_playbackQueue = PlaybackQueue::Local;
    m_currentIndex = index;
    startMediaPlayback(m_playlist[index].source, true);
}

void AudiomaticPlayerWidget::playOnlineIndex(int index)
{
    if (index < 0 || index >= m_onlineTracks.size())
    {
        return;
    }

    m_playbackQueue = PlaybackQueue::Online;
    m_onlineIndex = index;
    startMediaPlayback(m_onlineTracks[index].source, true);
}

void AudiomaticPlayerWidget::stepPlayback(int direction, bool wrap)
{
    if (m_playbackQueue == PlaybackQueue::Radio)
    {
        const int count = m_jamendoRadios.size();
        if (count <= 0 || m_radioIndex < 0)
        {
            return;
        }
        int next = m_radioIndex;
        if (m_shuffle)
        {
            next = QRandomGenerator::global()->bounded(count);
        }
        else if (wrap)
        {
            next = (m_radioIndex + direction + count) % count;
        }
        else if (direction > 0 && m_radioIndex + 1 < count)
        {
            next = m_radioIndex + 1;
        }
        else
        {
            return;
        }
        playJamendoRadio(next);
        return;
    }

    const bool online = m_playbackQueue == PlaybackQueue::Online;
    const QList<PlaylistEntry> &list = online ? m_onlineTracks : m_playlist;
    const int index = online ? m_onlineIndex : m_currentIndex;
    if (list.isEmpty())
    {
        return;
    }

    int next = index;
    if (index < 0)
    {
        next = direction < 0 ? list.size() - 1 : 0;
    }
    else if (m_shuffle)
    {
        next = QRandomGenerator::global()->bounded(list.size());
    }
    else if (wrap)
    {
        next = (index + direction + list.size()) % list.size();
    }
    else if (direction > 0 && index + 1 < list.size())
    {
        next = index + 1;
    }
    else
    {
        return;
    }

    if (online)
    {
        playOnlineIndex(next);
    }
    else
    {
        playIndex(next);
    }
}

void AudiomaticPlayerWidget::updateTransportButtons()
{
    if ( m_inputSource == AudioInputSource::SystemAudio )
    {
        const SystemMediaTrackInfo sysTrack = m_mediaWatcher ? m_mediaWatcher->currentTrackInfo() : SystemMediaTrackInfo{};
        const bool hasTrack                 = sysTrack.isValid();
        ui->btnPrevious->setEnabled( hasTrack && sysTrack.canPrevious );
        ui->btnNext->setEnabled( hasTrack && sysTrack.canNext );
        ui->btnShuffle->setEnabled( false );
        ui->btnRepeat->setEnabled( false );
        ui->btnSpeed->setEnabled( false );
        ui->btnPlayPause->setEnabled( hasTrack ? sysTrack.canPlayPause : true );
        ui->sliderPosition->setEnabled( hasTrack && sysTrack.canSeek && sysTrack.durationMs > 0 );
        return;
    }

    const bool hasTracks = m_playbackQueue == PlaybackQueue::Online ? !m_onlineTracks.isEmpty()
                          : m_playbackQueue == PlaybackQueue::Radio ? m_radioIndex >= 0
                                                                    : !m_playlist.isEmpty();
    ui->btnPrevious->setEnabled( hasTracks );
    ui->btnNext->setEnabled( hasTracks );
    ui->btnShuffle->setEnabled( true );
    ui->btnRepeat->setEnabled( true );
    ui->btnSpeed->setEnabled( true );
    ui->btnPlayPause->setEnabled( hasTracks );
    ui->sliderPosition->setEnabled( hasTracks );
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
        ui->listPlaylist->addItem(entry.listLabel());
    }

    if (m_playbackQueue == PlaybackQueue::Local && m_currentIndex >= 0 && m_currentIndex < ui->listPlaylist->count())
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

        if (m_playbackQueue == PlaybackQueue::Radio && m_radioIndex >= 0 && m_radioIndex < ui->listJamendoResults->count())
        {
            ui->listJamendoResults->setCurrentRow(m_radioIndex);
        }
        return;
    }

    for (const PlaylistEntry &entry : m_onlineTracks)
    {
        ui->listJamendoResults->addItem(entry.listLabel());
    }

    if (m_playbackQueue == PlaybackQueue::Online && m_onlineIndex >= 0 && m_onlineIndex < ui->listJamendoResults->count())
    {
        ui->listJamendoResults->setCurrentRow(m_onlineIndex);
    }
}

void AudiomaticPlayerWidget::setTranslucentEnabled( bool enabled )
{
    m_translucentEnabled = enabled;
    updateOverlayButtonsState();
    saveSettings();
    emit windowChromeChanged();
}

void AudiomaticPlayerWidget::setTranslucentOpacity( int percent )
{
    m_translucentOpacity = qBound( 30, percent, 100 );
    if ( ui->labelOpacityValue )
    {
        ui->labelOpacityValue->setText( QStringLiteral( "%1%" ).arg( m_translucentOpacity ) );
    }
    saveSettings();
    emit windowChromeChanged();
}

void AudiomaticPlayerWidget::setAlwaysOnTop( bool top )
{
    m_alwaysOnTop = top;
    updateOverlayButtonsState();
    saveSettings();
    emit windowChromeChanged();
}

void AudiomaticPlayerWidget::updateOverlayButtonsState()
{
    if ( ui->btnAlwaysOnTop )
    {
        ui->btnAlwaysOnTop->setChecked( m_alwaysOnTop );
        ui->btnAlwaysOnTop->setToolTip( m_alwaysOnTop ? tr( "已开启窗口置顶" ) : tr( "窗口置顶" ) );
    }
    if ( ui->btnTranslucent )
    {
        ui->btnTranslucent->setChecked( m_translucentEnabled );
        ui->btnTranslucent->setToolTip( m_translucentEnabled ? tr( "已开启半透明模式" ) : tr( "半透明模式" ) );
    }
    if ( ui->sliderOpacity )
    {
        ui->sliderOpacity->setEnabled( m_translucentEnabled );
    }
    if ( m_actAlwaysOnTop )
    {
        m_actAlwaysOnTop->setChecked( m_alwaysOnTop );
    }
    if ( m_actTranslucent )
    {
        m_actTranslucent->setChecked( m_translucentEnabled );
    }
    if ( m_menuOpacity )
    {
        m_menuOpacity->setEnabled( m_translucentEnabled );
    }
    if ( m_opacityGroup )
    {
        for ( auto* act : m_opacityGroup->actions() )
        {
            const bool match = qAbs( act->data().toInt() - m_translucentOpacity ) <= 5;
            if ( act->isChecked() != match )
            {
                const QSignalBlocker blocker( act );
                act->setChecked( match );
            }
        }
    }
    updateIcons();
}

void AudiomaticPlayerWidget::showVisualizerColorDialog()
{
    ExColorPickerDialog dlg( this );
    dlg.setTitle( tr( "选择波形颜色" ) );
    const QColor initialCol = m_visualizerAccentColor.isValid()
        ? m_visualizerAccentColor
        : palette().color( QPalette::Highlight );
    dlg.setColor( initialCol );

    const QColor oldCol = m_visualizerAccentColor;
    connect( &dlg, &ExColorPickerDialog::colorChanged, this, &AudiomaticPlayerWidget::setVisualizerAccentColor );
    connect( &dlg, &ExColorPickerDialog::colorSelected, this, [ this ]( const QColor& col ) {
        setVisualizerAccentColor( col );
        saveSettings();
    } );

    if ( dlg.exec() != QDialog::Accepted )
    {
        setVisualizerAccentColor( oldCol );
    }
}

void AudiomaticPlayerWidget::setVisualizerRainbow( bool enabled )
{
    m_visualizerRainbow = enabled;
    if ( ui->visualizer )
    {
        ui->visualizer->setRainbowEnabled( enabled );
    }
    if ( m_immersiveView )
    {
        m_immersiveView->setRainbowEnabled( enabled );
    }
    if ( m_actRainbow && m_actRainbow->isChecked() != enabled )
    {
        const QSignalBlocker blocker( m_actRainbow );
        m_actRainbow->setChecked( enabled );
    }
    saveSettings();
}

void AudiomaticPlayerWidget::setVisualizerAccentColor( const QColor& color )
{
    m_visualizerAccentColor = color;
    if ( ui->visualizer )
    {
        ui->visualizer->setAccentColor( color );
    }
    if ( m_immersiveView )
    {
        m_immersiveView->setAccentColor( color );
    }

    refreshLyricsStyle();
    updateCoverDisplay();
    saveSettings();
}

void AudiomaticPlayerWidget::setVisualizerReflectionOpacity( float opacity )
{
    m_visualizerReflectionOpacity = qBound( 0.0f, opacity, 1.0f );
    if ( ui->visualizer )
    {
        ui->visualizer->setReflectionOpacity( m_visualizerReflectionOpacity );
    }
    if ( m_immersiveView )
    {
        m_immersiveView->setReflectionOpacity( m_visualizerReflectionOpacity );
    }
    saveSettings();
}

void AudiomaticPlayerWidget::showLyricColorDialog()
{
    ExColorPickerDialog dlg( this );
    dlg.setTitle( tr( "选择歌词高亮颜色" ) );
    const QColor initialCol = lyricHighlightColor();
    dlg.setColor( initialCol );

    const QColor oldCol = m_lyricHighlightColor;
    connect( &dlg, &ExColorPickerDialog::colorChanged, this, &AudiomaticPlayerWidget::setLyricHighlightColor );
    connect( &dlg, &ExColorPickerDialog::colorSelected, this, [ this ]( const QColor& col ) {
        setLyricHighlightColor( col );
        saveSettings();
    } );

    if ( dlg.exec() != QDialog::Accepted )
    {
        setLyricHighlightColor( oldCol );
    }
}

void AudiomaticPlayerWidget::setLyricHighlightColor( const QColor& color )
{
    m_lyricHighlightColor = color;
    refreshLyricsStyle();
    saveSettings();
}

QColor AudiomaticPlayerWidget::lyricHighlightColor() const
{
    if ( m_lyricHighlightColor.isValid() )
    {
        return m_lyricHighlightColor;
    }
    if ( m_visualizerAccentColor.isValid() )
    {
        return m_visualizerAccentColor;
    }
    return palette().color( QPalette::Highlight );
}

void AudiomaticPlayerWidget::setLyricFontSizeLevel( int level )
{
    m_lyricFontSizeLevel = qBound( 0, level, 2 );
    refreshLyricsStyle();
    saveSettings();
}

int AudiomaticPlayerWidget::lyricNormalFontSize() const
{
    switch ( m_lyricFontSizeLevel )
    {
    case 0: return 12;
    case 2: return 16;
    default: return 14;
    }
}

int AudiomaticPlayerWidget::lyricActiveFontSize() const
{
    switch ( m_lyricFontSizeLevel )
    {
    case 0: return 15;
    case 2: return 20;
    default: return 17;
    }
}

int AudiomaticPlayerWidget::lyricImmersiveFontSize() const
{
    switch ( m_lyricFontSizeLevel )
    {
    case 0: return 20;
    case 2: return 28;
    default: return 24;
    }
}

void AudiomaticPlayerWidget::refreshLyricsStyle()
{
    const QColor hiColor = lyricHighlightColor();
    if ( ui->lyricsWidget )
    {
        ui->lyricsWidget->setHighlightColor( hiColor );
        ui->lyricsWidget->setFontSizeLevel( m_lyricFontSizeLevel );
        ui->lyricsWidget->update();
    }
    if ( m_immersiveView )
    {
        m_immersiveView->setLyricHighlightColor( hiColor );
        m_immersiveView->setLyricFontSizeLevel( m_lyricFontSizeLevel );
    }
}

void AudiomaticPlayerWidget::setIgnoreNonMusicLyrics( bool ignore )
{
    if ( m_ignoreNonMusicLyrics == ignore )
    {
        return;
    }
    m_ignoreNonMusicLyrics = ignore;
    saveSettings();
    updateTrackLabel();
}

void AudiomaticPlayerWidget::setupSettingsMenu()
{
    m_settingsMenu = new QMenu( this );

    // 1. 音频输入源
    auto* menuSource  = m_settingsMenu->addMenu( tr( "音频输入源" ) );
    auto* sourceGroup = new QActionGroup( this );
    sourceGroup->setExclusive( true );

    m_actSourcePlayer = menuSource->addAction( EXFONTICON->getIcon( SegoeIcon::MusicInfo, 16 ), tr( "媒体播放器 (本地与在线音乐)" ) );
    m_actSourcePlayer->setCheckable( true );
    m_actSourcePlayer->setChecked( m_inputSource == AudioInputSource::MediaPlayer );
    sourceGroup->addAction( m_actSourcePlayer );
    connect( m_actSourcePlayer, &QAction::triggered, this, [ this ]() { setAudioInputSource( AudioInputSource::MediaPlayer ); } );

    m_actSourceSystem = menuSource->addAction( EXFONTICON->getIcon( SegoeIcon::Volume3, 16 ), tr( "电脑声音 (系统音频 Loopback)" ) );
    m_actSourceSystem->setCheckable( true );
    m_actSourceSystem->setChecked( m_inputSource == AudioInputSource::SystemAudio );
    sourceGroup->addAction( m_actSourceSystem );
    connect( m_actSourceSystem, &QAction::triggered, this, [ this ]() { setAudioInputSource( AudioInputSource::SystemAudio ); } );

    // 2. 窗口与悬浮
    auto* menuWindow = m_settingsMenu->addMenu( tr( "窗口与悬浮" ) );
    m_actAlwaysOnTop = menuWindow->addAction( tr( "窗口置顶 (Always on Top)" ) );
    m_actAlwaysOnTop->setCheckable( true );
    m_actAlwaysOnTop->setChecked( m_alwaysOnTop );
    connect( m_actAlwaysOnTop, &QAction::toggled, this, &AudiomaticPlayerWidget::setAlwaysOnTop );

    m_actImmersive = menuWindow->addAction( tr( "沉浸视图 (波形主视图)" ) );
    m_actImmersive->setCheckable( true );
    m_actImmersive->setChecked( m_immersiveMode );
    connect( m_actImmersive, &QAction::toggled, this, &AudiomaticPlayerWidget::setImmersiveMode );

    m_settingsMenu->addSeparator();

    // 3. 半透明。沉浸视图会挡住底栏滑块，所以开关和档位直接放在这个菜单里。
    m_actTranslucent = m_settingsMenu->addAction( tr( "半透明模式" ) );
    m_actTranslucent->setCheckable( true );
    m_actTranslucent->setChecked( m_translucentEnabled );
    connect( m_actTranslucent, &QAction::toggled, this, &AudiomaticPlayerWidget::setTranslucentEnabled );

    m_menuOpacity = m_settingsMenu->addMenu( tr( "透明度" ) );
    m_opacityGroup = new QActionGroup( this );
    m_opacityGroup->setExclusive( true );
    const int opacityPercents[] = { 100, 90, 80, 70, 60, 50, 40, 30 };
    for ( int percent : opacityPercents )
    {
        auto* act = m_menuOpacity->addAction( tr( "%1%" ).arg( percent ) );
        act->setCheckable( true );
        act->setData( percent );
        act->setChecked( qAbs( m_translucentOpacity - percent ) <= 5 );
        m_opacityGroup->addAction( act );
        connect( act, &QAction::triggered, this, [ this, percent ]() {
            if ( ui->sliderOpacity )
            {
                ui->sliderOpacity->setValue( percent );
            }
            setTranslucentOpacity( percent );
            setTranslucentEnabled( true );
        } );
    }
    m_menuOpacity->setEnabled( m_translucentEnabled );

    // 4. 波形颜色 (Accent)
    m_actRainbow = m_settingsMenu->addAction( tr( "五颜六色 (从左至右)" ) );
    m_actRainbow->setCheckable( true );
    m_actRainbow->setChecked( m_visualizerRainbow );
    connect( m_actRainbow, &QAction::toggled, this, &AudiomaticPlayerWidget::setVisualizerRainbow );

    m_actVisualizerColor = m_settingsMenu->addAction( tr( "波形颜色 (Accent)..." ) );
    connect( m_actVisualizerColor, &QAction::triggered, this, &AudiomaticPlayerWidget::showVisualizerColorDialog );

    auto* actResetVisualizerColor = m_settingsMenu->addAction( tr( "恢复系统默认强调色" ) );
    connect( actResetVisualizerColor, &QAction::triggered, this, [ this ]() {
        setVisualizerAccentColor( QColor() );
        saveSettings();
    } );

    // 4. 歌词设置
    auto* menuLyrics = m_settingsMenu->addMenu( tr( "歌词设置" ) );
    auto* actLyricColor = menuLyrics->addAction( tr( "歌词高亮颜色..." ) );
    connect( actLyricColor, &QAction::triggered, this, &AudiomaticPlayerWidget::showLyricColorDialog );

    auto* actResetLyricColor = menuLyrics->addAction( tr( "恢复默认颜色 (跟随波形)" ) );
    connect( actResetLyricColor, &QAction::triggered, this, [ this ]() {
        setLyricHighlightColor( QColor() );
        saveSettings();
    } );

    menuLyrics->addSeparator();

    auto* actKaraoke = menuLyrics->addAction( tr( "跟随进度平滑高亮 (卡拉OK扫光)" ) );
    actKaraoke->setCheckable( true );
    actKaraoke->setChecked( ui->lyricsWidget ? ui->lyricsWidget->isKaraokeProgress() : true );
    connect( actKaraoke, &QAction::toggled, this, [ this ]( bool checked ) {
        if ( ui->lyricsWidget )
        {
            ui->lyricsWidget->setKaraokeProgress( checked );
        }
        saveSettings();
    } );

    menuLyrics->addSeparator();

    auto* menuFontSize = menuLyrics->addMenu( tr( "歌词字号" ) );
    m_lyricFontSizeGroup = new QActionGroup( this );
    const struct { const char* label; int level; } fontLevels[] = {
        { "小 (9pt / 11pt)", 0 },
        { "标准 (10pt / 12pt)", 1 },
        { "大 (11pt / 14pt)", 2 },
        { "特大 (12pt / 16pt)", 3 },
    };
    for ( const auto& item : fontLevels )
    {
        auto* act = menuFontSize->addAction( tr( item.label ) );
        act->setCheckable( true );
        act->setChecked( m_lyricFontSizeLevel == item.level );
        m_lyricFontSizeGroup->addAction( act );
        const int lvl = item.level;
        connect( act, &QAction::triggered, this, [ this, lvl ]() {
            setLyricFontSizeLevel( lvl );
            saveSettings();
        } );
    }

    menuLyrics->addSeparator();
    m_actIgnoreNonMusic = menuLyrics->addAction( tr( "忽略浏览器与视频软件 (免查歌词)" ) );
    m_actIgnoreNonMusic->setCheckable( true );
    m_actIgnoreNonMusic->setChecked( m_ignoreNonMusicLyrics );
    connect( m_actIgnoreNonMusic, &QAction::toggled, this, &AudiomaticPlayerWidget::setIgnoreNonMusicLyrics );

    auto* actRefetchLyric = menuLyrics->addAction( tr( "重新检索/匹配当前歌词" ) );
    connect( actRefetchLyric, &QAction::triggered, this, [ this ]() {
        if ( !m_lyricsManager ) return;
        QString qTitle  = m_currentLyricsQueryTitle;
        QString qArtist = m_currentLyricsQueryArtist;
        qint64 durMs    = 0;
        if ( m_inputSource == AudioInputSource::SystemAudio && m_mediaWatcher )
        {
            const SystemMediaTrackInfo curSt = m_mediaWatcher->currentTrackInfo();
            if ( qTitle.isEmpty() )
            {
                qTitle  = curSt.title.trimmed();
                qArtist = curSt.artist.trimmed();
            }
            durMs = curSt.durationMs;
        }
        else if ( m_inputSource == AudioInputSource::MediaPlayer && m_player )
        {
            durMs = m_player->duration();
        }
        if ( qTitle.isEmpty() ) return;

        m_currentLyricsQueryTitle  = qTitle;
        m_currentLyricsQueryArtist = qArtist;
        if ( ui->lyricsWidget ) ui->lyricsWidget->setStatusMessage( tr( "正在重新检索歌词…" ) );
        if ( m_immersiveView ) m_immersiveView->setLyricStatusMessage( tr( "正在重新检索歌词…" ) );
        m_lyricsManager->refetchLyrics( m_currentLyricsQueryTitle, m_currentLyricsQueryArtist, durMs );
    } );

    auto* menuOffset = menuLyrics->addMenu( tr( "歌词时间轴微调 (Offset)" ) );
    auto* actAdvance = menuOffset->addAction( tr( "提前 0.5 秒 (-0.5s)" ) );
    connect( actAdvance, &QAction::triggered, this, [ this ]() {
        m_lyricsUserOffsetMs -= 500;
        updateLyricsDisplay( m_lastMediaWatcherPositionMs );
    } );
    auto* actDelay = menuOffset->addAction( tr( "延后 0.5 秒 (+0.5s)" ) );
    connect( actDelay, &QAction::triggered, this, [ this ]() {
        m_lyricsUserOffsetMs += 500;
        updateLyricsDisplay( m_lastMediaWatcherPositionMs );
    } );
    auto* actResetOffset = menuOffset->addAction( tr( "恢复默认 (0.0s)" ) );
    connect( actResetOffset, &QAction::triggered, this, [ this ]() {
        m_lyricsUserOffsetMs = 0;
        updateLyricsDisplay( m_lastMediaWatcherPositionMs );
    } );

    m_settingsMenu->addSeparator();

    // 5. 波形水波倒影
    m_actReflection = m_settingsMenu->addAction( tr( "显示水波倒影" ) );
    m_actReflection->setCheckable( true );
    m_actReflection->setChecked( ui->visualizer ? ui->visualizer->isReflectionEnabled() : true );
    connect( m_actReflection, &QAction::toggled, this, [ this ]( bool checked ) {
        if ( ui->visualizer ) ui->visualizer->setReflectionEnabled( checked );
        if ( m_immersiveView ) m_immersiveView->syncSpectrumParameters( ui->visualizer );
        if ( m_menuReflectionOpacity ) m_menuReflectionOpacity->setEnabled( checked );
        saveSettings();
    } );

    m_menuReflectionOpacity = m_settingsMenu->addMenu( tr( "水波倒影透明度" ) );
    m_reflectionOpacityGroup = new QActionGroup( this );
    const struct { const char* label; float val; } opacities[] = {
        { "弱 (15%)", 0.15f },
        { "柔和 (35%)", 0.35f },
        { "标准 (50%)", 0.50f },
        { "清晰 (70%)", 0.70f }
    };
    for ( const auto& op : opacities )
    {
        auto* act = m_menuReflectionOpacity->addAction( tr( op.label ) );
        act->setCheckable( true );
        act->setChecked( qAbs( m_visualizerReflectionOpacity - op.val ) < 0.08f );
        m_reflectionOpacityGroup->addAction( act );
        const float v = op.val;
        connect( act, &QAction::triggered, this, [ this, v ]() {
            setVisualizerReflectionOpacity( v );
        } );
    }

    // 6. 浮动峰值帽
    m_actPeakCap = m_settingsMenu->addAction( tr( "显示浮动峰值帽" ) );
    m_actPeakCap->setCheckable( true );
    m_actPeakCap->setChecked( ui->visualizer ? ui->visualizer->isPeakCapEnabled() : true );
    connect( m_actPeakCap, &QAction::toggled, this, [ this ]( bool checked ) {
        if ( ui->visualizer ) ui->visualizer->setPeakCapEnabled( checked );
        if ( m_immersiveView ) m_immersiveView->syncSpectrumParameters( ui->visualizer );
        saveSettings();
    } );

    // 7. 柱子数量
    auto* menuBarCount  = m_settingsMenu->addMenu( tr( "柱子数量" ) );
    m_barCountGroup = new QActionGroup( this );
    m_barCountGroup->setExclusive( true );
    const int barCounts[] = { 32, 48, 64, 96, 128 };
    const int currentBars = ui->visualizer ? ui->visualizer->barCount() : 64;
    for ( int count : barCounts )
    {
        auto* action = menuBarCount->addAction( tr( "%1 柱" ).arg( count ) );
        action->setData( count );
        action->setCheckable( true );
        if ( count == currentBars ) action->setChecked( true );
        m_barCountGroup->addAction( action );
        connect( action, &QAction::triggered, this, [ this, count ]() {
            if ( ui->visualizer ) ui->visualizer->setBarCount( count );
            if ( m_immersiveView ) m_immersiveView->syncSpectrumParameters( ui->visualizer );
            saveSettings();
        } );
    }

    // 8. 柱子间距
    auto* menuGap  = m_settingsMenu->addMenu( tr( "柱子间距" ) );
    m_gapGroup = new QActionGroup( this );
    m_gapGroup->setExclusive( true );
    const QList<QPair<int, QString>> gaps = {
        { 1, tr( "紧凑 (1px)" ) },
        { 2, tr( "标准 (2px)" ) },
        { 4, tr( "宽松 (4px)" ) }
    };
    for ( const auto& p : gaps )
    {
        auto* action = menuGap->addAction( p.second );
        action->setCheckable( true );
        if ( p.first == 2 ) action->setChecked( true );
        m_gapGroup->addAction( action );
        connect( action, &QAction::triggered, this, [ this, gap = p.first ]() {
            if ( ui->visualizer ) ui->visualizer->setBarGap( gap );
            if ( m_immersiveView ) m_immersiveView->syncSpectrumParameters( ui->visualizer );
            saveSettings();
        } );
    }

    // 9. 柱顶圆角
    auto* menuRadius  = m_settingsMenu->addMenu( tr( "柱顶圆角" ) );
    m_radiusGroup = new QActionGroup( this );
    m_radiusGroup->setExclusive( true );
    const QList<QPair<int, QString>> radii = {
        { 0, tr( "直角 (0px)" ) },
        { 3, tr( "标准 (3px)" ) },
        { 6, tr( "圆润 (6px)" ) }
    };
    for ( const auto& p : radii )
    {
        auto* action = menuRadius->addAction( p.second );
        action->setCheckable( true );
        if ( p.first == 3 ) action->setChecked( true );
        m_radiusGroup->addAction( action );
        connect( action, &QAction::triggered, this, [ this, r = p.first ]() {
            if ( ui->visualizer ) ui->visualizer->setBarRadius( r );
            if ( m_immersiveView ) m_immersiveView->syncSpectrumParameters( ui->visualizer );
            saveSettings();
        } );
    }

    // 10. 最高分析频率
    auto* menuFreq  = m_settingsMenu->addMenu( tr( "最高分析频率" ) );
    m_freqGroup = new QActionGroup( this );
    m_freqGroup->setExclusive( true );
    const QList<QPair<float, QString>> freqs = {
        { 12000.0f, tr( "12 kHz (温暖)" )       },
        { 16000.0f, tr( "16 kHz (标准流行)" )   },
        { 20000.0f, tr( "20 kHz (Hi-Fi 无损)" ) }
    };
    for ( const auto& p : freqs )
    {
        auto* action = menuFreq->addAction( p.second );
        action->setCheckable( true );
        if ( int( p.first ) == 16000 ) action->setChecked( true );
        m_freqGroup->addAction( action );
        connect( action, &QAction::triggered, this, [ this, f = p.first ]() {
            if ( ui->visualizer ) ui->visualizer->setMaxFrequency( f );
            if ( m_immersiveView ) m_immersiveView->syncSpectrumParameters( ui->visualizer );
            saveSettings();
        } );
    }

    m_settingsMenu->addSeparator();
    m_settingsMenu->addAction( tr( "Jamendo Client ID…" ), this, &AudiomaticPlayerWidget::configureJamendoClientId );

    m_settingsMenu->addSeparator();
    auto* actTour = m_settingsMenu->addAction( tr( "新手使用向导 (Tour)..." ) );
    connect( actTour, &QAction::triggered, this, &AudiomaticPlayerWidget::startTour );

    connect( ui->btnSettings, &QToolButton::clicked, this, &AudiomaticPlayerWidget::showSettingsMenu );
}

void AudiomaticPlayerWidget::showSettingsMenu()
{
    if ( !m_settingsMenu )
    {
        return;
    }
    m_settingsMenu->exec( QCursor::pos() );
}

void AudiomaticPlayerWidget::setupImmersiveView()
{
    if ( !ui->pageImmersive || !ui->pageImmersiveRootLayout )
    {
        return;
    }

    if ( !m_immersiveView )
    {
        m_immersiveView = new AudiomaticImmersiveView( ui->pageImmersive );
        ui->pageImmersiveRootLayout->addWidget( m_immersiveView );

        connect( m_immersiveView, &AudiomaticImmersiveView::exitRequested, this, [ this ]() {
            setImmersiveMode( false );
        } );
        connect( m_immersiveView, &AudiomaticImmersiveView::contextMenuRequested, this, [ this ]( const QPoint& globalPos ) {
            Q_UNUSED( globalPos );
            showSettingsMenu();
        } );
    }

    if ( ui->visualizer )
    {
        m_immersiveView->syncSpectrumParameters( ui->visualizer );
    }
    m_immersiveView->setAccentColor( visualizerAccentColor() );
    m_immersiveView->setReflectionOpacity( visualizerReflectionOpacity() );
    m_immersiveView->setLyricHighlightColor( lyricHighlightColor() );
    m_immersiveView->setLyricFontSizeLevel( m_lyricFontSizeLevel );
    m_immersiveView->setCoverPixmap( m_currentCoverPixmap );
}

void AudiomaticPlayerWidget::setImmersiveMode( bool immersive )
{
    if ( m_immersiveMode == immersive )
    {
        return;
    }

    m_immersiveMode = immersive;
    ui->stackedWidget->setCurrentIndex( immersive ? 1 : 0 );

    if ( immersive )
    {
        if ( m_immersiveView )
        {
            m_immersiveView->syncSpectrumParameters( ui->visualizer );
            m_immersiveView->setFocus();
        }
        updateTrackLabel();
        updateCoverDisplay();
    }
    else
    {
        updateTransportButtons();
        updateTrackLabel();
        updateIcons();
        updateCoverDisplay();
    }

    if ( m_actImmersive )
    {
        m_actImmersive->blockSignals( true );
        m_actImmersive->setChecked( immersive );
        m_actImmersive->blockSignals( false );
    }

    emit immersiveModeChanged( immersive );
    saveSettings();
}

void AudiomaticPlayerWidget::setCoverPixmap( const QPixmap& pixmap )
{
    m_currentCoverPixmap = pixmap;
    updateCoverDisplay();
    if ( m_immersiveView )
    {
        m_immersiveView->setCoverPixmap( pixmap );
    }
}

QPixmap AudiomaticPlayerWidget::createPlaceholderCover( int size, const QString& title )
{
    Q_UNUSED( title );
    QPixmap pm( size, size );
    pm.fill( Qt::transparent );

    QPainter p( &pm );
    p.setRenderHint( QPainter::Antialiasing, true );

    const QColor accent = m_visualizerAccentColor.isValid()
        ? m_visualizerAccentColor
        : palette().color( QPalette::Highlight );

    QLinearGradient grad( 0, 0, size, size );
    grad.setColorAt( 0.0, accent.darker( 130 ) );
    grad.setColorAt( 1.0, accent.darker( 220 ) );

    const int radius = ( size >= 100 ) ? 12 : 6;
    QPainterPath path;
    path.addRoundedRect( 0, 0, size, size, radius, radius );
    p.fillPath( path, grad );

    p.setPen( QPen( QColor( 255, 255, 255, 35 ), 1.0 ) );
    p.drawPath( path );

    const int iconPixel = qMax( 12, int( size * 0.42 ) );
    QFont f = fluentIconFont( iconPixel );
    p.setFont( f );
    p.setPen( QColor( 255, 255, 255, 190 ) );
    p.drawText( QRect( 0, 0, size, size ), Qt::AlignCenter, ExFontIcon::iconString( SegoeIcon::MusicInfo ) );

    return pm;
}

void AudiomaticPlayerWidget::updateCoverDisplay()
{
    if ( ui->labelCover )
    {
        if ( !m_currentCoverPixmap.isNull() )
        {
            QPixmap coverSmall( 40, 40 );
            coverSmall.fill( Qt::transparent );
            QPainter p( &coverSmall );
            p.setRenderHint( QPainter::Antialiasing, true );
            p.setRenderHint( QPainter::SmoothPixmapTransform, true );
            QPainterPath path;
            path.addRoundedRect( 0, 0, 40, 40, 6, 6 );
            p.setClipPath( path );
            p.drawPixmap( 0, 0, 40, 40, m_currentCoverPixmap.scaled( 40, 40, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation ) );
            p.setClipping( false );
            p.setPen( QPen( QColor( 255, 255, 255, 30 ), 1.0 ) );
            p.drawPath( path );
            ui->labelCover->setPixmap( coverSmall );
        }
        else
        {
            ui->labelCover->setPixmap( createPlaceholderCover( 40 ) );
        }
    }
}

bool AudiomaticPlayerWidget::eventFilter( QObject* watched, QEvent* event )
{
    if ( event->type() == QEvent::KeyPress )
    {
        auto* keyEvent = static_cast<QKeyEvent*>( event );
        if ( keyEvent->key() == Qt::Key_Escape && m_immersiveMode )
        {
            setImmersiveMode( false );
            return true;
        }
    }

    if ( !m_immersiveMode && watched == ui->labelCover && event->type() == QEvent::MouseButtonRelease )
    {
        auto* mouseEvent = static_cast<QMouseEvent*>( event );
        if ( mouseEvent->button() == Qt::LeftButton )
        {
            setImmersiveMode( true );
            return true;
        }
    }

    if ( !m_immersiveMode && watched == ui->visualizer && event->type() == QEvent::MouseButtonDblClick )
    {
        setImmersiveMode( true );
        return true;
    }

    return QWidget::eventFilter( watched, event );
}

void AudiomaticPlayerWidget::loadSettings()
{
    QSettings settings = getAudiomaticSettings();

    m_translucentEnabled = settings.value( QStringLiteral( "Window/Translucent" ), false ).toBool();
    m_translucentOpacity = settings.value( QStringLiteral( "Window/Opacity" ), 85 ).toInt();
    m_alwaysOnTop        = settings.value( QStringLiteral( "Window/AlwaysOnTop" ), false ).toBool();
    m_immersiveMode      = settings.value( QStringLiteral( "Window/ImmersiveMode" ), false ).toBool();

    const QString accentName = settings.value( QStringLiteral( "Spectrum/AccentColor" ) ).toString();
    if ( !accentName.isEmpty() )
    {
        setVisualizerAccentColor( QColor( accentName ) );
    }
    setVisualizerRainbow( settings.value( QStringLiteral( "Spectrum/Rainbow" ), true ).toBool() );

    m_visualizerReflectionOpacity = settings.value( QStringLiteral( "Spectrum/ReflectionOpacity" ), 0.58f ).toFloat();
    setVisualizerReflectionOpacity( m_visualizerReflectionOpacity );

    const int barCount = settings.value( QStringLiteral( "Spectrum/BarCount" ), 64 ).toInt();
    const bool reflection = settings.value( QStringLiteral( "Spectrum/Reflection" ), true ).toBool();
    if ( ui->visualizer )
    {
        ui->visualizer->setReflectionEnabled( reflection );
    }
    if ( m_actReflection )
    {
        m_actReflection->setChecked( reflection );
    }
    if ( m_menuReflectionOpacity )
    {
        m_menuReflectionOpacity->setEnabled( reflection );
    }
    const bool peakCap = settings.value( QStringLiteral( "Spectrum/PeakCap" ), true ).toBool();
    if ( ui->visualizer )
    {
        ui->visualizer->setPeakCapEnabled( peakCap );
    }
    if ( m_actPeakCap )
    {
        m_actPeakCap->setChecked( peakCap );
    }
    const int barGap = settings.value( QStringLiteral( "Spectrum/BarGap" ), 2 ).toInt();
    if ( ui->visualizer )
    {
        ui->visualizer->setBarGap( barGap );
    }
    const int barRadius = settings.value( QStringLiteral( "Spectrum/BarRadius" ), 3 ).toInt();
    if ( ui->visualizer )
    {
        ui->visualizer->setBarRadius( barRadius );
    }
    const float maxFreq = float( settings.value( QStringLiteral( "Spectrum/MaxFrequency" ), 16000.0 ).toDouble() );
    if ( ui->visualizer )
    {
        ui->visualizer->setMaxFrequency( maxFreq );
    }

    if ( m_barCountGroup )
    {
        for ( auto* act : m_barCountGroup->actions() )
        {
            if ( act->data().toInt() == barCount )
            {
                act->setChecked( true );
                break;
            }
        }
    }
    if ( m_gapGroup )
    {
        for ( auto* act : m_gapGroup->actions() )
        {
            if ( act->text().contains( QString::number( barGap ) ) )
            {
                act->setChecked( true );
                break;
            }
        }
    }
    if ( m_radiusGroup )
    {
        for ( auto* act : m_radiusGroup->actions() )
        {
            if ( act->text().contains( QString::number( barRadius ) ) )
            {
                act->setChecked( true );
                break;
            }
        }
    }
    if ( m_freqGroup )
    {
        const int khz = int( maxFreq / 1000.0f );
        for ( auto* act : m_freqGroup->actions() )
        {
            if ( act->text().contains( QString::number( khz ) ) )
            {
                act->setChecked( true );
                break;
            }
        }
    }
    if ( ui->visualizer )
    {
        ui->visualizer->setBarCount( barCount );
    }
    if ( m_immersiveView )
    {
        m_immersiveView->syncSpectrumParameters( ui->visualizer );
    }

    m_lyricFontSizeLevel   = settings.value( QStringLiteral( "Lyrics/FontSizeLevel" ), 1 ).toInt();
    m_ignoreNonMusicLyrics = settings.value( QStringLiteral( "Lyrics/IgnoreNonMusic" ), true ).toBool();

    if ( ui->sliderOpacity )
    {
        ui->sliderOpacity->setValue( m_translucentOpacity );
    }
    if ( ui->labelOpacityValue )
    {
        ui->labelOpacityValue->setText( QStringLiteral( "%1%" ).arg( m_translucentOpacity ) );
    }

    updateOverlayButtonsState();
    refreshLyricsStyle();
    m_loadingSettings = false;
    emit windowChromeChanged();
}

void AudiomaticPlayerWidget::saveSettings()
{
    if ( m_loadingSettings )
    {
        return;
    }

    QSettings settings = getAudiomaticSettings();

    settings.setValue( QStringLiteral( "Window/Translucent" ), m_translucentEnabled );
    settings.setValue( QStringLiteral( "Window/Opacity" ), m_translucentOpacity );
    settings.setValue( QStringLiteral( "Window/AlwaysOnTop" ), m_alwaysOnTop );
    settings.setValue( QStringLiteral( "Window/ImmersiveMode" ), m_immersiveMode );

    settings.setValue( QStringLiteral( "Spectrum/AccentColor" ), m_visualizerAccentColor.isValid() ? m_visualizerAccentColor.name() : QString() );
    settings.setValue( QStringLiteral( "Spectrum/Rainbow" ), m_visualizerRainbow );
    settings.setValue( QStringLiteral( "Spectrum/ReflectionOpacity" ), m_visualizerReflectionOpacity );
    settings.setValue( QStringLiteral( "Spectrum/BarCount" ), ui->visualizer ? ui->visualizer->barCount() : 64 );
    settings.setValue( QStringLiteral( "Spectrum/Reflection" ), ui->visualizer ? ui->visualizer->isReflectionEnabled() : true );
    settings.setValue( QStringLiteral( "Spectrum/PeakCap" ), ui->visualizer ? ui->visualizer->isPeakCapEnabled() : true );
    settings.setValue( QStringLiteral( "Spectrum/BarGap" ), ui->visualizer ? ui->visualizer->barGap() : 2 );
    settings.setValue( QStringLiteral( "Spectrum/BarRadius" ), ui->visualizer ? ui->visualizer->barRadius() : 3 );
    settings.setValue( QStringLiteral( "Spectrum/MaxFrequency" ), ui->visualizer ? double( ui->visualizer->maxFrequency() ) : 16000.0 );

    settings.setValue( QStringLiteral( "Lyrics/FontSizeLevel" ), m_lyricFontSizeLevel );
    settings.setValue( QStringLiteral( "Lyrics/IgnoreNonMusic" ), m_ignoreNonMusicLyrics );
    settings.sync();
}

void AudiomaticPlayerWidget::setupLyricsManager()
{
    m_lyricsManager = new LyricsManager( this );

    connect( m_lyricsManager, &LyricsManager::lyricsLoaded, this,
             [ this ]( const QString& title, const QString& artist, const QList<LyricLine>& lines ) {
        if ( title.trimmed() != m_currentLyricsQueryTitle.trimmed()
             || artist.trimmed() != m_currentLyricsQueryArtist.trimmed() )
        {
            return;
        }
        m_currentLyricLineIndex = -1;

        QList<ExLyricLine> exLines;
        exLines.reserve( lines.size() );
        for ( const auto& l : lines )
        {
            exLines.append( { l.timeMs, l.text } );
        }
        if ( ui->lyricsWidget )
        {
            ui->lyricsWidget->setLyricsLines( exLines );
        }
        if ( m_immersiveView )
        {
            m_immersiveView->setLyricsLines( exLines );
        }

        qint64 currentPos = 0;
        if ( m_inputSource == AudioInputSource::MediaPlayer && m_player )
        {
            currentPos = m_player->position();
        }
        else if ( m_inputSource == AudioInputSource::SystemAudio )
        {
            if ( m_mediaWatcherPosElapsed.isValid() )
            {
                currentPos = m_lastMediaWatcherPositionMs + m_mediaWatcherPosElapsed.elapsed();
            }
            else if ( m_systemLyricsElapsed.isValid() )
            {
                currentPos = m_systemLyricsBaseMs + m_systemLyricsElapsed.elapsed();
            }
            else
            {
                m_systemLyricsBaseMs = 0;
                m_systemLyricsElapsed.restart();
                currentPos = 0;
            }
        }
        updateLyricsDisplay( currentPos );
    } );

    connect( m_lyricsManager, &LyricsManager::coverImageLoaded, this,
             [ this ]( const QString& title, const QString& artist, const QPixmap& coverPixmap ) {
        Q_UNUSED( title );
        Q_UNUSED( artist );
        if ( !coverPixmap.isNull() )
        {
            if ( m_currentCoverPixmap.isNull() || m_inputSource == AudioInputSource::MediaPlayer )
            {
                setCoverPixmap( coverPixmap );
            }
        }
    } );

    connect( m_lyricsManager, &LyricsManager::lyricsLoadFailed, this,
             [ this ]( const QString& title, const QString& artist, const QString& reason ) {
        Q_UNUSED( title );
        Q_UNUSED( artist );
        Q_UNUSED( reason );
        m_currentLyricLineIndex = -1;

        if ( ui->lyricsWidget )
        {
            ui->lyricsWidget->setStatusMessage( tr( "纯音乐，请欣赏" ) );
            ui->lyricsWidget->clear();
        }
        if ( m_immersiveView )
        {
            m_immersiveView->setLyricStatusMessage( tr( "纯音乐，请欣赏" ) );
            m_immersiveView->clearLyrics();
        }
    } );

    // 33ms (~30FPS) 平滑高刷插值驱动定时器
    auto* interpTimer = new QTimer( this );
    interpTimer->setInterval( 33 );
    connect( interpTimer, &QTimer::timeout, this, [ this ]() {
        if ( m_inputSource == AudioInputSource::SystemAudio )
        {
            const SystemMediaTrackInfo sysTrack = m_mediaWatcher ? m_mediaWatcher->currentTrackInfo() : SystemMediaTrackInfo{};
            const bool hasTrack                 = sysTrack.isValid();
            const bool isPlaying                = hasTrack ? sysTrack.isPlaying : ( m_systemCapture && m_systemCapture->isCapturing() );

            if ( isPlaying )
            {
                if ( !m_systemLyricsElapsed.isValid() )
                {
                    m_systemLyricsElapsed.restart();
                }
                const qint64 rawPos = m_systemLyricsBaseMs + m_systemLyricsElapsed.elapsed();
                const qint64 currentPos = ( hasTrack && sysTrack.durationMs > 0 )
                    ? qMin( rawPos, sysTrack.durationMs )
                    : rawPos;
                updateLyricsDisplay( currentPos );

                if ( !m_seeking && hasTrack && sysTrack.durationMs > 0 )
                {
                    ui->sliderPosition->setValue( int( currentPos ) );
                    ui->labelTimeCurrent->setText( formatTime( currentPos ) );

                    if ( rawPos >= sysTrack.durationMs && m_systemLyricsElapsed.isValid() )
                    {
                        m_systemLyricsBaseMs = sysTrack.durationMs;
                        m_systemLyricsElapsed.invalidate();
                    }
                }
            }
            else
            {
                if ( m_systemLyricsElapsed.isValid() )
                {
                    m_systemLyricsElapsed.invalidate();
                }
            }
        }
        else if ( m_inputSource == AudioInputSource::MediaPlayer && m_player )
        {
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
            const bool isPlaying = ( m_player->playbackState() == QMediaPlayer::PlayingState );
#else
            const bool isPlaying = ( m_player->state() == QMediaPlayer::PlayingState );
#endif
            if ( isPlaying )
            {
                updateLyricsDisplay( m_player->position() );
            }
        }
    } );
    interpTimer->start();
}

void AudiomaticPlayerWidget::setLyricHighlightIndex( int index )
{
    m_currentLyricLineIndex = index;
    if ( ui->lyricsWidget && index >= 0 )
    {
        ui->lyricsWidget->scrollToLine( index, true );
    }
}

void AudiomaticPlayerWidget::updateLyricsDisplay( qint64 positionMs )
{
    const qint64 adjustedPos = positionMs + m_lyricsUserOffsetMs;
    if ( ui->lyricsWidget )
    {
        ui->lyricsWidget->setCurrentPositionMs( adjustedPos );
    }
    if ( m_immersiveView )
    {
        m_immersiveView->setLyricPosition( adjustedPos );
    }
}

void AudiomaticPlayerWidget::setupCpuUsageMonitor()
{
    m_cpuTimer = new QTimer( this );
    m_cpuTimer->setInterval( 1000 );
    if ( ui->labelCpuUsage )
    {
        ui->labelCpuUsage->setText( QStringLiteral( "CPU 0.0%" ) );
        ui->labelCpuUsage->setToolTip( tr( "程序及系统 CPU 占用率。" ) );
    }
    if ( m_immersiveView )
    {
        m_immersiveView->setCpuUsageText( QStringLiteral( "CPU 0.0%" ) );
    }

    connect( m_cpuTimer, &QTimer::timeout, this, &AudiomaticPlayerWidget::updateCpuUsage );
    m_cpuTimer->start();
}

void AudiomaticPlayerWidget::updateCpuUsage()
{
#ifdef Q_OS_WIN
    FILETIME idleTime, kernelTime, userTime;
    FILETIME creationTime, exitTime, procKernelTime, procUserTime;

    if ( !GetSystemTimes( &idleTime, &kernelTime, &userTime ) ||
         !GetProcessTimes( GetCurrentProcess(), &creationTime, &exitTime, &procKernelTime, &procUserTime ) )
    {
        return;
    }

    auto fileTimeToUint64 = []( const FILETIME& ft ) -> quint64 {
        return ( static_cast<quint64>( ft.dwHighDateTime ) << 32 ) | ft.dwLowDateTime;
    };

    const quint64 currentIdleTime   = fileTimeToUint64( idleTime );
    const quint64 currentKernelTime = fileTimeToUint64( kernelTime );
    const quint64 currentUserTime   = fileTimeToUint64( userTime );
    const quint64 currentProcKernel = fileTimeToUint64( procKernelTime );
    const quint64 currentProcUser   = fileTimeToUint64( procUserTime );

    if ( m_hasLastCpuSample )
    {
        const quint64 sysKernelDiff = currentKernelTime - m_lastCpuKernelTime;
        const quint64 sysUserDiff   = currentUserTime - m_lastCpuUserTime;
        const quint64 sysIdleDiff   = currentIdleTime - m_lastCpuIdleTime;
        const quint64 sysTotalDiff  = sysKernelDiff + sysUserDiff;

        const quint64 procKernelDiff = currentProcKernel - m_lastProcessKernelTime;
        const quint64 procUserDiff   = currentProcUser - m_lastProcessUserTime;
        const quint64 procTotalDiff  = procKernelDiff + procUserDiff;

        if ( sysTotalDiff > 0 )
        {
            double sysUsage = 0.0;
            if ( sysTotalDiff > sysIdleDiff )
            {
                sysUsage = ( double( sysTotalDiff - sysIdleDiff ) * 100.0 ) / double( sysTotalDiff );
            }
            double procUsage = ( double( procTotalDiff ) * 100.0 ) / double( sysTotalDiff );

            sysUsage  = qBound( 0.0, sysUsage, 100.0 );
            procUsage = qBound( 0.0, procUsage, 100.0 );

            const QString cpuText = QStringLiteral( "CPU %1%" ).arg( procUsage, 0, 'f', 1 );
            const QString cpuTip  = tr( "本程序 CPU: %1%\n系统总 CPU: %2%" )
                                       .arg( QString::number( procUsage, 'f', 1 ),
                                             QString::number( sysUsage, 'f', 1 ) );

            if ( ui->labelCpuUsage )
            {
                ui->labelCpuUsage->setText( cpuText );
                ui->labelCpuUsage->setToolTip( cpuTip );
            }
            if ( m_immersiveView )
            {
                m_immersiveView->setCpuUsageText( cpuText, cpuTip );
            }
        }
    }
    else
    {
        m_hasLastCpuSample = true;
    }

    m_lastCpuIdleTime       = currentIdleTime;
    m_lastCpuKernelTime     = currentKernelTime;
    m_lastCpuUserTime       = currentUserTime;
    m_lastProcessKernelTime = currentProcKernel;
    m_lastProcessUserTime   = currentProcUser;
#else
    if ( ui->labelCpuUsage )
    {
        ui->labelCpuUsage->setText( QStringLiteral( "CPU --%" ) );
    }
    if ( m_immersiveView )
    {
        m_immersiveView->setCpuUsageText( QStringLiteral( "CPU --%" ) );
    }
#endif
}

void AudiomaticPlayerWidget::setupTour()
{
    if ( m_tour )
    {
        return;
    }
    m_tour = new ExTour( this );
    connect( m_tour, &ExTour::finished, this, [ this ]() {
        QSettings settings = getAudiomaticSettings();
        settings.setValue( QStringLiteral( "General/FirstRunTourCompleted" ), true );
    } );
    connect( m_tour, &ExTour::cancelled, this, [ this ]() {
        QSettings settings = getAudiomaticSettings();
        settings.setValue( QStringLiteral( "General/FirstRunTourCompleted" ), true );
    } );
}

void AudiomaticPlayerWidget::startTour()
{
    if ( m_immersiveMode )
    {
        setImmersiveMode( false );
    }
    if ( ui->stackedWidget && ui->pageRegular )
    {
        ui->stackedWidget->setCurrentWidget( ui->pageRegular );
    }
    if ( ui->contentTabs && ui->tabSpectrum )
    {
        ui->contentTabs->setCurrentWidget( ui->tabSpectrum );
    }

    setupTour();

    QList<ExTourStep> steps;

    if ( isVisible() )
    {
        ExTourStep intro;
        intro.target       = this;
        intro.title        = tr( "Audiomatic Mini" );
        intro.description  = tr( "一边监听电脑里正在播放的声音，一边看频谱和歌词。\n也可以切换到媒体播放器，播放本机音乐。接下来会逐项介绍这些功能。" );
        intro.placement    = ExTourStep::Bottom;
        intro.targetRadius = 8.0;
        steps.append( intro );
    }

    if ( ui->btnAudioSource && ui->btnAudioSource->isVisible() )
    {
        ExTourStep step;
        step.target       = ui->btnAudioSource;
        step.title        = tr( "音频输入源切换" );
        step.description  = tr( "支持在【电脑声音（系统音频 Loopback 实时捕获）】与【媒体播放器】之间随时切换。\n默认自动监听系统声音，播放 QQ音乐、网易云等即可实时跳动！" );
        step.placement    = ExTourStep::Top;
        step.targetRadius = 4.0;
        steps.append( step );
    }

    if ( ui->visualizer && ui->visualizer->isVisible() )
    {
        ExTourStep step;
        step.target       = ui->visualizer;
        step.title        = tr( "动感音乐频谱" );
        step.description  = tr( "工业级声学频谱动画，支持倒影与 Accent 强调色。\n【双击波形区域】或【点击左上角小封面】即可瞬间进入极简【沉浸模式】，再次双击或按 Esc 即可退出！" );
        step.placement    = ExTourStep::Bottom;
        step.targetRadius = 6.0;
        steps.append( step );
    }

    if ( ui->lyricsWidget && ui->lyricsWidget->isVisible() )
    {
        ExTourStep step;
        step.target       = ui->lyricsWidget;
        step.title        = tr( "智能歌词与扫光高亮" );
        step.description  = tr( "自动匹配毫秒级网络歌词，支持滚轮自由浏览与点击跳转；\n当前唱到的歌词会跟随进度从左向右平滑点亮（卡拉OK扫光特效）。" );
        step.placement    = ExTourStep::Top;
        step.targetRadius = 6.0;
        steps.append( step );
    }

    if ( ui->btnPlayPause && ui->btnPlayPause->isVisible() )
    {
        ExTourStep step;
        step.target       = ui->btnPlayPause;
        step.title        = tr( "播控控制与进度跳转" );
        step.description  = tr( "包含播放/暂停、上一曲、下一曲控制，支持拖拽时间轴 Seek 跳转。\n在电脑声音模式下，可以直接跨进程遥控外部音乐软件！" );
        step.placement    = ExTourStep::Bottom;
        step.targetRadius = 19.0;
        steps.append( step );
    }

    if ( ui->btnTranslucent && ui->btnTranslucent->isVisible() )
    {
        ExTourStep step;
        step.target       = ui->btnTranslucent;
        step.title        = tr( "半透明模式与窗口置顶" );
        step.description  = tr( "点击开启黄金通透度半透明视效，支持右侧滑块 30%~100% 细粒度微调；\n配合置顶按钮，可轻盈悬浮在桌面边工作边欣赏律动！" );
        step.placement    = ExTourStep::Top;
        step.targetRadius = 4.0;
        steps.append( step );
    }

    if ( ui->btnSettings && ui->btnSettings->isVisible() )
    {
        ExTourStep step;
        step.target       = ui->btnSettings;
        step.title        = tr( "个性化设置与参数调节" );
        step.description  = tr( "点击齿轮图标可自定义波形强调色、柱子数量、水波倒影透明度、歌词字号；\n若需重新查看本向导，也可以在设置菜单中随时再次开启！" );
        step.placement    = ExTourStep::Top;
        step.targetRadius = 4.0;
        steps.append( step );
    }

    if ( steps.isEmpty() )
    {
        return;
    }

    m_tour->setSteps( steps );
    m_tour->start( 0 );
}
