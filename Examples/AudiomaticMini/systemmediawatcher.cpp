#include "systemmediawatcher.h"

#include <QAtomicInt>
#include <QDateTime>
#include <QDebug>
#include <QMutex>
#include <QQueue>
#include <QThread>
#include <chrono>

#if defined(Q_OS_WIN)
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>

#    if defined(_MSC_VER) && __has_include(<winrt/Windows.Media.Control.h>)
#        define AUDIOMATIC_HAS_WINRT_SMTC 1
#        include <winrt/Windows.Foundation.Collections.h>
#        include <winrt/Windows.Foundation.h>
#        include <winrt/Windows.Media.Control.h>
#        if __has_include(<winrt/Windows.Storage.Streams.h>)
#            define AUDIOMATIC_HAS_WINRT_STREAMS 1
#            include <winrt/Windows.Storage.Streams.h>
#        endif
#    endif
#endif

namespace {

#if defined(AUDIOMATIC_HAS_WINRT_SMTC)
namespace winrt_media = winrt::Windows::Media::Control;
#endif

enum class MediaControlCommand
{
    TogglePlayPause,
    Play,
    Pause,
    NextTrack,
    PreviousTrack,
    Seek
};

struct MediaCommandItem
{
    MediaControlCommand cmd;
    qint64 arg{ 0 };
};

#if defined(Q_OS_WIN)
void sendVirtualKeyFallback( BYTE vk )
{
    keybd_event( vk, 0, 0, 0 );
    keybd_event( vk, 0, KEYEVENTF_KEYUP, 0 );
}
#endif

}  // namespace

/**
 * @brief 后台工作线程：持有长效 WinRT SMTC 管理器，执行差异化查询与播控调度。
 *
 * 核心优化：
 * 1. 杜绝在循环中重复调用耗性能的 RequestAsync()，全线程生命周期只初始化一次 SessionManager；
 * 2. 媒体元数据（Title/Artist/Album）采用差异化缓存策略，仅在会话切换或曲目变更特征出现时才拉取，
 *    杜绝每秒跨进程 IPC 轰炸操作系统服务 NPSMSvc；
 * 3. 播控指令由工作线程队列化统一串行执行，杜绝外部临时线程并发竞争；
 * 4. 线程退出时确定性置空 WinRT 对象并调用 winrt::uninit_apartment()，彻底杜绝 NPSMSvc 残留自旋。
 */
class SystemMediaWorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit SystemMediaWorkerThread( QObject* parent = nullptr )
        : QThread( parent )
    {
    }

    ~SystemMediaWorkerThread() override
    {
        stopThread();
    }

    void stopThread()
    {
        m_running.storeRelaxed( 0 );
        m_wakeUp.storeRelaxed( 1 );
        if ( !wait( 2000 ) )
        {
            terminate();
            wait( 500 );
        }
    }

    void setPollInterval( int ms )
    {
        m_pollIntervalMs.storeRelaxed( qMax( 200, ms ) );
    }

    void triggerImmediateUpdate()
    {
        m_wakeUp.storeRelaxed( 1 );
    }

    void enqueueCommand( MediaControlCommand cmd, qint64 arg = 0 )
    {
        {
            QMutexLocker locker( &m_cmdMutex );
            m_cmdQueue.enqueue( { cmd, arg } );
        }
        m_wakeUp.storeRelaxed( 1 );
    }

signals:
    void trackInfoDetected( const SystemMediaTrackInfo& trackInfo );
    void positionDetected( qint64 positionMs, qint64 durationMs );

protected:
    void run() override
    {
        m_running.storeRelaxed( 1 );

        bool coInitialized = false;
#if defined(AUDIOMATIC_HAS_WINRT_SMTC)
        try
        {
            winrt::init_apartment( winrt::apartment_type::multi_threaded );
            coInitialized = true;
        }
        catch ( ... )
        {
        }

        winrt_media::GlobalSystemMediaTransportControlsSessionManager manager{ nullptr };
        winrt_media::GlobalSystemMediaTransportControlsSession activeSession{ nullptr };

        QString cachedAppId;
        QString cachedTitle;
        QString cachedArtist;
        QString cachedAlbum;
        QByteArray cachedThumbnailData;
        qint64 cachedDurationMs{ 0 };
        qint64 lastRawPosMs{ 0 };
        int consecutiveErrors = 0;
        int propCheckCounter = 0;
#endif

        SystemMediaTrackInfo lastEmittedInfo;

        while ( m_running.loadRelaxed() )
        {
            // 1. 处理待执行的播控命令队列（复用长效 Session，避免并发临时线程抢占）
            processQueuedCommands(
#if defined(AUDIOMATIC_HAS_WINRT_SMTC)
                activeSession
#endif
            );

            SystemMediaTrackInfo currentInfo;

#if defined(AUDIOMATIC_HAS_WINRT_SMTC)
            try
            {
                // 长效复用 SessionManager，避免高频 RequestAsync() 触发 NPSMSvc 句柄泄露与 CPU 飙高
                if ( !manager )
                {
                    auto asyncOp = winrt_media::GlobalSystemMediaTransportControlsSessionManager::RequestAsync();
                    manager      = asyncOp.get();
                    consecutiveErrors = 0;
                }

                if ( manager )
                {
                    // 查找当前真正处于 Playing 或主活跃状态的 Session
                    winrt_media::GlobalSystemMediaTransportControlsSession session{ nullptr };
                    try
                    {
                        session = manager.GetCurrentSession();
                        if ( session )
                        {
                            auto pb = session.GetPlaybackInfo();
                            if ( !pb || pb.PlaybackStatus() != winrt_media::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing )
                            {
                                session = nullptr;
                            }
                        }
                    }
                    catch ( ... )
                    {
                        session = nullptr;
                    }

                    if ( !session )
                    {
                        try
                        {
                            auto sessions = manager.GetSessions();
                            for ( const auto& s : sessions )
                            {
                                try
                                {
                                    auto pb = s.GetPlaybackInfo();
                                    if ( pb && pb.PlaybackStatus() == winrt_media::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing )
                                    {
                                        session = s;
                                        break;
                                    }
                                }
                                catch ( ... )
                                {
                                }
                            }
                            if ( !session && sessions.Size() > 0 )
                            {
                                try
                                {
                                    session = manager.GetCurrentSession();
                                }
                                catch ( ... )
                                {
                                }
                                if ( !session )
                                {
                                    session = sessions.GetAt( 0 );
                                }
                            }
                        }
                        catch ( ... )
                        {
                        }
                    }

                    activeSession = session;

                    if ( session )
                    {
                        QString currentAppId;
                        try
                        {
                            currentAppId = QString::fromStdWString( session.SourceAppUserModelId().c_str() );
                        }
                        catch ( ... )
                        {
                        }

                        currentInfo.appId = currentAppId;

                        // 提取播放状态与可用控件能力
                        try
                        {
                            auto pb = session.GetPlaybackInfo();
                            if ( pb )
                            {
                                currentInfo.isPlaying = ( pb.PlaybackStatus() == winrt_media::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing );
                                auto ctrl = pb.Controls();
                                if ( ctrl )
                                {
                                    currentInfo.canPlayPause = ctrl.IsPlayPauseToggleEnabled() || ctrl.IsPlayEnabled() || ctrl.IsPauseEnabled();
                                    currentInfo.canNext      = ctrl.IsNextEnabled();
                                    currentInfo.canPrevious  = ctrl.IsPreviousEnabled();
                                    currentInfo.canSeek      = ctrl.IsPlaybackPositionEnabled();
                                }
                            }
                        }
                        catch ( ... )
                        {
                        }

                        // 提取时间轴属性并推演真实播放进度
                        qint64 rawPosMs = 0;
                        qint64 totalDurMs = 0;
                        try
                        {
                            auto tl = session.GetTimelineProperties();
                            if ( tl )
                            {
                                rawPosMs   = tl.Position().count() / 10000;
                                totalDurMs = tl.EndTime().count() / 10000;

                                auto lastUpdated = tl.LastUpdatedTime();
                                auto nowUtc      = winrt::clock::now();

                                if ( currentInfo.isPlaying && nowUtc > lastUpdated )
                                {
                                    auto diffMs = std::chrono::duration_cast<std::chrono::milliseconds>( nowUtc - lastUpdated ).count();
                                    if ( diffMs > 0 && diffMs < 3600000 )
                                    {
                                        rawPosMs += diffMs;
                                    }
                                }

                                if ( totalDurMs > 0 && rawPosMs > totalDurMs )
                                {
                                    rawPosMs = totalDurMs;
                                }

                                currentInfo.positionMs = rawPosMs;
                                currentInfo.durationMs = totalDurMs;
                            }
                        }
                        catch ( ... )
                        {
                        }

                        // 智能差异化拉取：
                        // 1. 应用切换（例如从 QQ音乐 切换到 网易云 或 浏览器）
                        const bool appChanged        = ( currentAppId != cachedAppId );
                        // 2. 总时长改变（曲目时长变动是切歌的强标志，取消宽容盲区）
                        const bool durationChanged   = ( totalDurMs > 0 && cachedDurationMs > 0 && totalDurMs != cachedDurationMs );
                        // 3. 播放时间归零或大幅回跳（从 >5秒 回跳到 <3秒，绝对是外部播放器切歌或重新播放）
                        const bool positionJumpBack  = ( rawPosMs < 3000 && lastRawPosMs > 5000 );
                        // 4. 初次启动或周期保底（每 3 秒做一次轻量级 TryGetMediaPropertiesAsync 查询保底）
                        const bool needsMetaRefresh  = appChanged || durationChanged || positionJumpBack || cachedTitle.isEmpty() || ( ++propCheckCounter >= 3 );

                        if ( needsMetaRefresh )
                        {
                            propCheckCounter = 0;
                            try
                            {
                                auto propsOp = session.TryGetMediaPropertiesAsync();
                                auto props   = propsOp.get();
                                if ( props )
                                {
                                    cachedTitle  = QString::fromStdWString( props.Title().c_str() );
                                    cachedArtist = QString::fromStdWString( props.Artist().c_str() );
                                    cachedAlbum  = QString::fromStdWString( props.AlbumTitle().c_str() );
                                    cachedAppId  = currentAppId;
                                    cachedDurationMs = totalDurMs;
                                    cachedThumbnailData.clear();

#if defined(AUDIOMATIC_HAS_WINRT_STREAMS)
                                    try
                                    {
                                        auto thumbRef = props.Thumbnail();
                                        if ( thumbRef )
                                        {
                                            auto streamOp = thumbRef.OpenReadAsync();
                                            auto stream   = streamOp.get();
                                            if ( stream )
                                            {
                                                const uint64_t streamSize = stream.Size();
                                                if ( streamSize > 0 && streamSize < 10 * 1024 * 1024 )
                                                {
                                                    winrt::Windows::Storage::Streams::Buffer winrtBuf( static_cast<uint32_t>( streamSize ) );
                                                    auto readOp = stream.ReadAsync( winrtBuf, static_cast<uint32_t>( streamSize ), winrt::Windows::Storage::Streams::InputStreamOptions::None );
                                                    auto resBuf = readOp.get();
                                                    if ( resBuf && resBuf.Length() > 0 )
                                                    {
                                                        auto reader = winrt::Windows::Storage::Streams::DataReader::FromBuffer( resBuf );
                                                        QByteArray rawBytes( static_cast<int>( resBuf.Length() ), '\0' );
                                                        reader.ReadBytes( winrt::array_view<uint8_t>(
                                                            reinterpret_cast<uint8_t*>( rawBytes.data() ),
                                                            reinterpret_cast<uint8_t*>( rawBytes.data() + rawBytes.size() ) ) );
                                                        cachedThumbnailData = rawBytes;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    catch ( ... )
                                    {
                                    }
#endif
                                }
                            }
                            catch ( ... )
                            {
                            }
                        }

                        lastRawPosMs = rawPosMs;

                        currentInfo.title         = cachedTitle;
                        currentInfo.artist        = cachedArtist;
                        currentInfo.album         = cachedAlbum;
                        currentInfo.thumbnailData = cachedThumbnailData;
                    }
                    else
                    {
                        // 当前无活跃 Session，清理缓存
                        cachedAppId.clear();
                        cachedTitle.clear();
                        cachedArtist.clear();
                        cachedAlbum.clear();
                        cachedThumbnailData.clear();
                        cachedDurationMs = 0;
                        lastRawPosMs     = 0;
                    }
                }
            }
            catch ( ... )
            {
                if ( ++consecutiveErrors > 3 )
                {
                    // 连续多次严重异常时重置 manager，允许下次安全自愈
                    manager = nullptr;
                    activeSession = nullptr;
                    consecutiveErrors = 0;
                }
            }
#endif

            // 派发状态变更信号
            if ( currentInfo != lastEmittedInfo )
            {
                lastEmittedInfo = currentInfo;
                emit trackInfoDetected( currentInfo );
            }
            if ( currentInfo.isValid() && currentInfo.positionMs >= 0 )
            {
                emit positionDetected( currentInfo.positionMs, currentInfo.durationMs );
            }

            m_wakeUp.storeRelaxed( 0 );

            // 分步睡眠响应退出与即时唤醒，杜绝无脑忙轮询
            const int interval = m_pollIntervalMs.loadRelaxed();
            const int steps    = qMax( 1, interval / 80 );
            for ( int i = 0; i < steps && m_running.loadRelaxed() && !m_wakeUp.loadRelaxed(); ++i )
            {
                QThread::msleep( 80 );
            }
        }

#if defined(AUDIOMATIC_HAS_WINRT_SMTC)
        // 关键清理：确定性显式置空所有持有的 WinRT COM 对象，并释放当前线程的 MTA 套间，
        // 确保操作系统 NPSMSvc 收到明确的注销断开通知，杜绝服务自旋与 CPU 飙高！
        activeSession = nullptr;
        manager       = nullptr;

        if ( coInitialized )
        {
            try
            {
                winrt::uninit_apartment();
            }
            catch ( ... )
            {
            }
        }
#endif
    }

private:
    void processQueuedCommands(
#if defined(AUDIOMATIC_HAS_WINRT_SMTC)
        winrt_media::GlobalSystemMediaTransportControlsSession& session
#endif
    )
    {
        QQueue<MediaCommandItem> tasks;
        {
            QMutexLocker locker( &m_cmdMutex );
            if ( m_cmdQueue.isEmpty() )
            {
                return;
            }
            tasks = m_cmdQueue;
            m_cmdQueue.clear();
        }

        while ( !tasks.isEmpty() )
        {
            const auto item = tasks.dequeue();
            bool handled    = false;

#if defined(AUDIOMATIC_HAS_WINRT_SMTC)
            if ( session )
            {
                try
                {
                    switch ( item.cmd )
                    {
                    case MediaControlCommand::TogglePlayPause:
                        session.TryTogglePlayPauseAsync();
                        handled = true;
                        break;
                    case MediaControlCommand::Play:
                        session.TryPlayAsync();
                        handled = true;
                        break;
                    case MediaControlCommand::Pause:
                        session.TryPauseAsync();
                        handled = true;
                        break;
                    case MediaControlCommand::NextTrack:
                        session.TrySkipNextAsync();
                        handled = true;
                        break;
                    case MediaControlCommand::PreviousTrack:
                        session.TrySkipPreviousAsync();
                        handled = true;
                        break;
                    case MediaControlCommand::Seek:
                        session.TryChangePlaybackPositionAsync( static_cast<int64_t>( item.arg * 10000 ) );
                        handled = true;
                        break;
                    }
                }
                catch ( ... )
                {
                    handled = false;
                }
            }
#endif

#if defined(Q_OS_WIN)
            if ( !handled )
            {
                switch ( item.cmd )
                {
                case MediaControlCommand::TogglePlayPause:
                case MediaControlCommand::Play:
                case MediaControlCommand::Pause:
                    sendVirtualKeyFallback( VK_MEDIA_PLAY_PAUSE );
                    break;
                case MediaControlCommand::NextTrack:
                    sendVirtualKeyFallback( VK_MEDIA_NEXT_TRACK );
                    break;
                case MediaControlCommand::PreviousTrack:
                    sendVirtualKeyFallback( VK_MEDIA_PREV_TRACK );
                    break;
                default:
                    break;
                }
            }
#endif
        }
    }

    QAtomicInt m_running{ 0 };
    QAtomicInt m_wakeUp{ 0 };
    QAtomicInt m_pollIntervalMs{ 1000 };
    QQueue<MediaCommandItem> m_cmdQueue;
    QMutex m_cmdMutex;
};

class SystemMediaWatcherPrivate : public QObject
{
    Q_OBJECT

public:
    explicit SystemMediaWatcherPrivate( SystemMediaWatcher* q_ptr )
        : q( q_ptr )
    {
        m_worker = new SystemMediaWorkerThread( this );
        connect( m_worker,
                 &SystemMediaWorkerThread::trackInfoDetected,
                 this,
                 [ this ]( const SystemMediaTrackInfo& info )
                 {
                     {
                         QMutexLocker locker( &m_mutex );
                         m_currentInfo = info;
                     }
                     emit q->trackInfoChanged( info );
                 } );
        connect( m_worker,
                 &SystemMediaWorkerThread::positionDetected,
                 this,
                 [ this ]( qint64 positionMs, qint64 durationMs )
                 {
                     {
                         QMutexLocker locker( &m_mutex );
                         m_currentInfo.positionMs = positionMs;
                         m_currentInfo.durationMs = durationMs;
                     }
                     emit q->positionChanged( positionMs, durationMs );
                 } );
    }

    ~SystemMediaWatcherPrivate() override
    {
        if ( m_worker )
        {
            m_worker->stopThread();
        }
    }

    void postCommand( MediaControlCommand cmd, qint64 arg = 0 )
    {
        if ( m_worker )
        {
            m_worker->enqueueCommand( cmd, arg );
        }
    }

    SystemMediaWatcher* q{ nullptr };
    SystemMediaWorkerThread* m_worker{ nullptr };
    SystemMediaTrackInfo m_currentInfo;
    mutable QMutex m_mutex;
};

SystemMediaWatcher::SystemMediaWatcher( QObject* parent )
    : QObject( parent )
    , d( new SystemMediaWatcherPrivate( this ) )
{
}

SystemMediaWatcher::~SystemMediaWatcher()
{
    delete d;
}

bool SystemMediaWatcher::isWatching() const
{
    return d->m_worker && d->m_worker->isRunning();
}

SystemMediaTrackInfo SystemMediaWatcher::currentTrackInfo() const
{
    QMutexLocker locker( &d->m_mutex );
    return d->m_currentInfo;
}

void SystemMediaWatcher::start( int pollIntervalMs )
{
    if ( d->m_worker )
    {
        d->m_worker->setPollInterval( pollIntervalMs );
        if ( !d->m_worker->isRunning() )
        {
            d->m_worker->start();
        }
    }
}

void SystemMediaWatcher::stop()
{
    if ( d->m_worker )
    {
        d->m_worker->stopThread();
    }
}

void SystemMediaWatcher::updateNow()
{
    if ( d->m_worker )
    {
        d->m_worker->triggerImmediateUpdate();
    }
}

void SystemMediaWatcher::playPause()
{
    d->postCommand( MediaControlCommand::TogglePlayPause );
}

void SystemMediaWatcher::play()
{
    d->postCommand( MediaControlCommand::Play );
}

void SystemMediaWatcher::pause()
{
    d->postCommand( MediaControlCommand::Pause );
}

void SystemMediaWatcher::nextTrack()
{
    d->postCommand( MediaControlCommand::NextTrack );
}

void SystemMediaWatcher::previousTrack()
{
    d->postCommand( MediaControlCommand::PreviousTrack );
}

void SystemMediaWatcher::seekTo( qint64 positionMs )
{
    d->postCommand( MediaControlCommand::Seek, positionMs );
}

#include "systemmediawatcher.moc"

