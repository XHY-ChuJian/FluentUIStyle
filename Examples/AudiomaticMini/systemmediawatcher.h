#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class SystemMediaWatcherPrivate;

/**
 * @brief 系统当前媒体曲目信息（歌名、歌手、专辑、播放器应用标识等）。
 */
struct SystemMediaTrackInfo
{
    QString title;         ///< 歌曲名称（如 "一生所爱 (Live)"）
    QString artist;        ///< 歌手/表演者（如 "伍佰"）
    QString album;         ///< 专辑名称
    QString appId;         ///< 播放器应用标识（如 "QQMusic" / "cloudmusic.exe"）
    QByteArray thumbnailData; ///< 原始专辑封面图像数据（JPEG/PNG 字节流）
    bool isPlaying{false}; ///< 是否正在播放
    qint64 positionMs{0};  ///< 当前播放时间进度（毫秒）
    qint64 durationMs{0};  ///< 歌曲总时长（毫秒）
    bool canPlayPause{true}; ///< 是否支持播放/暂停控制
    bool canNext{true};      ///< 是否支持下一曲控制
    bool canPrevious{true};  ///< 是否支持上一曲控制
    bool canSeek{false};     ///< 是否支持进度条拖动(Seek)控制

    bool isValid() const
    {
        return !title.trimmed().isEmpty();
    }

    bool operator==( const SystemMediaTrackInfo& other ) const
    {
        return title == other.title &&
               artist == other.artist &&
               album == other.album &&
               appId == other.appId &&
               thumbnailData == other.thumbnailData &&
               isPlaying == other.isPlaying &&
               durationMs == other.durationMs &&
               canPlayPause == other.canPlayPause &&
               canNext == other.canNext &&
               canPrevious == other.canPrevious &&
               canSeek == other.canSeek;
    }

    bool operator!=( const SystemMediaTrackInfo& other ) const
    {
        return !( *this == other );
    }

    /**
     * @brief 格式化展示文本："歌名 - 歌手" 或单一 "歌名"。
     */
    QString displayText() const
    {
        if ( title.trimmed().isEmpty() )
        {
            return QString();
        }
        if ( artist.trimmed().isEmpty() )
        {
            return title.trimmed();
        }
        return QStringLiteral( "%1 - %2" ).arg( title.trimmed(), artist.trimmed() );
    }
};

/**
 * @brief 系统媒体信息监视器。
 *
 * 在 Windows 10/11 平台上通过 GlobalSystemMediaTransportControlsSessionManager (GSMTC)
 * 实时捕获系统所有正在播放音乐的应用程序（如 QQ音乐、网易云音乐、Spotify、浏览器等）的当前歌曲名、歌手与专辑；
 * 在后台工作线程中低开销周期轮询，零阻塞主界面渲染；
 * 采用 d 指针架构隔绝 Windows 平台 API 与宏污染。
 */
class SystemMediaWatcher : public QObject
{
    Q_OBJECT

public:
    explicit SystemMediaWatcher( QObject* parent = nullptr );
    ~SystemMediaWatcher() override;

    bool isWatching() const;
    SystemMediaTrackInfo currentTrackInfo() const;

public slots:
    void start( int pollIntervalMs = 1000 );
    void stop();
    void updateNow();

    // 跨进程系统媒体播控接口
    void playPause();
    void play();
    void pause();
    void nextTrack();
    void previousTrack();
    void seekTo( qint64 positionMs );

signals:
    void trackInfoChanged( const SystemMediaTrackInfo& trackInfo );
    void positionChanged( qint64 positionMs, qint64 durationMs );

private:
    SystemMediaWatcherPrivate* d{ nullptr };
};
