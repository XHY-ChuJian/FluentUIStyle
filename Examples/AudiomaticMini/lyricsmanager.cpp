#include "lyricsmanager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrlQuery>

#include <algorithm>

namespace {

/**
 * @brief 制作人员、工作室信息不是歌词。网易云常把它们写成 [00:00.00]作词 : …
 */
bool isNonLyricMetaLine( const QString& text )
{
    const QString t = text.trimmed();
    if ( t.isEmpty() )
    {
        return true;
    }

    static const QRegularExpression roleRe(
        QStringLiteral( "(作词|作曲|编曲|制作人|录音师|录音室|混音师|混音室|母带|出品|监制|和声|企划|统筹|推广|发行|弦乐|小提琴|中提琴|大提琴"
                        "|lyricist|composer|arrangement|producer|engineer|studio|violin|viola|cello|strings"
                        "|^\\s*(?:op|sp)\\s*[:：])" ),
        QRegularExpression::CaseInsensitiveOption );
    if ( !roleRe.match( t ).hasMatch() )
    {
        return false;
    }

    const bool hasColon = t.contains( QStringLiteral( ":" ) ) || t.contains( QStringLiteral( "：" ) );
    if ( hasColon )
    {
        return true;
    }

    static const QRegularExpression leadingRe(
        QStringLiteral( "^(作词|作曲|编曲|制作人|出品|监制|和声|企划|统筹|推广|发行|录音|混音|母带)" ) );
    return leadingRe.match( t ).hasMatch();
}

qint64 lrcFractionMs( const QString& msStr )
{
    if ( msStr.isEmpty() )
    {
        return 0;
    }
    if ( msStr.length() == 1 )
    {
        return msStr.toLongLong() * 100;
    }
    if ( msStr.length() == 2 )
    {
        return msStr.toLongLong() * 10;
    }
    return msStr.left( 3 ).toLongLong();
}

/**
 * @brief 解析标准 LRC 文本为时间戳有序行列表。
 */
QList<LyricLine> parseLrcContent( const QString& lrcText )
{
    QList<LyricLine> lines;
    if ( lrcText.trimmed().isEmpty() )
    {
        return lines;
    }

    // 匹配全局时间偏移标签 [offset: +/-ms]
    // 很多官方/精制标准 LRC 文件包含 [offset: 500]（延迟500ms）或 [offset: -300]（提前300ms）
    static const QRegularExpression offsetRegex( QStringLiteral( R"(\[offset\s*:\s*([+-]?\d+)\s*\])" ), QRegularExpression::CaseInsensitiveOption );
    qint64 globalOffsetMs = 0;
    const auto offsetMatch = offsetRegex.match( lrcText );
    if ( offsetMatch.hasMatch() )
    {
        globalOffsetMs = offsetMatch.captured( 1 ).toLongLong();
    }

    // [mm:ss]、[mm:ss.xx]、[mm:ss.xxx]、[mm:ss:xx]
    static const QRegularExpression timeRegex( QStringLiteral( "\\[(\\d{1,3}):(\\d{1,2})(?:[.:](\\d{1,3}))?\\]" ) );
    static const QRegularExpression wordTagRegex( QStringLiteral( "<\\d{1,3}:\\d{1,2}(?:[.:]\\d{1,3})?>" ) );

    const QStringList rawLines = lrcText.split( QLatin1Char( '\n' ) );
    for ( const QString& rawLine : rawLines )
    {
        const QString trimmed = rawLine.trimmed();
        if ( trimmed.isEmpty() )
        {
            continue;
        }

        struct Stamp
        {
            qint64 timeMs { 0 };
            int startPos { 0 };
            int endPos { 0 };
        };
        QList<Stamp> stamps;
        auto matchIterator = timeRegex.globalMatch( trimmed );
        while ( matchIterator.hasNext() )
        {
            const auto match = matchIterator.next();
            const qint64 mins = match.captured( 1 ).toLongLong();
            const qint64 secs = match.captured( 2 ).toLongLong();
            const qint64 ms   = lrcFractionMs( match.captured( 3 ) );
            stamps.append( { mins * 60000 + secs * 1000 + ms,
                             static_cast<int>( match.capturedStart() ),
                             static_cast<int>( match.capturedEnd() ) } );
        }

        if ( stamps.isEmpty() )
        {
            continue;
        }

        // 逐字时间轴形如 [00:01.00]谁想[00:01.40]一眼。字在标签之间，不能只留最后一个标签后面的尾巴。
        QString joined;
        bool inlineText = false;
        for ( int i = 0; i < stamps.size(); ++i )
        {
            const int from = stamps[ i ].endPos;
            const int to   = ( i + 1 < stamps.size() ) ? stamps[ i + 1 ].startPos : static_cast<int>( trimmed.size() );
            const QString piece = trimmed.mid( from, to - from );
            if ( i + 1 < stamps.size() && !piece.trimmed().isEmpty() )
            {
                inlineText = true;
            }
            joined += piece;
        }
        joined.remove( wordTagRegex );
        joined = joined.trimmed();
        if ( isNonLyricMetaLine( joined ) )
        {
            continue;
        }

        if ( inlineText )
        {
            const qint64 adjustedTime = qMax<qint64>( 0, stamps.first().timeMs + globalOffsetMs );
            lines.append( { adjustedTime, joined } );
        }
        else
        {
            for ( const Stamp& stamp : stamps )
            {
                const qint64 adjustedTime = qMax<qint64>( 0, stamp.timeMs + globalOffsetMs );
                lines.append( { adjustedTime, joined } );
            }
        }
    }

    std::sort( lines.begin(), lines.end(), []( const LyricLine& a, const LyricLine& b ) {
        return a.timeMs < b.timeMs;
    } );

    return lines;
}

/**
 * @brief 剥离歌曲名末尾的现场版、重置版等括号后缀。
 */
QString stripTitleSuffix( const QString& title )
{
    QString stripped = title;
    static const QRegularExpression parenRegex( QStringLiteral( R"(\s*(\([^\)]*\)|（[^）]*）|\[[^\]]*\]|【[^】]*】)\s*$)" ) );
    stripped.remove( parenRegex );
    return stripped.trimmed();
}

/**
 * @brief 计算本地持久专辑封面缓存的文件路径。
 */
QString coverCacheFilePathForTrack( const QString& title, const QString& artist )
{
    const QString cacheDir = QStandardPaths::writableLocation( QStandardPaths::AppLocalDataLocation ) + QStringLiteral( "/covers" );
    QDir().mkpath( cacheDir );
    const QByteArray key   = QStringLiteral( "%1_%2" ).arg( title.trimmed().toLower(), artist.trimmed().toLower() ).toUtf8();
    const QString filename = QString::fromLatin1( QCryptographicHash::hash( key, QCryptographicHash::Md5 ).toHex() ) + QStringLiteral( ".jpg" );
    return cacheDir + QStringLiteral( "/" ) + filename;
}


/**
 * @brief 多维度综合打分评估候选歌曲与当前播放歌曲的吻合度。
 *
 * 打分维度：
 * 1. 歌名相似度（最高 40 分）：完全一致 40 分，互相包含 25 分；
 * 2. 歌手吻合度（最高 35 分）：包含目标歌手 35 分，目标未指定中立 15 分，明确冲突扣 35 分；
 * 3. 歌曲总时长吻合（最高 25 分）：误差 < 3.5s 给 25 分，误差 < 7s 给 18 分，相差 > 25s（典型现场版/剪辑版）扣 35 分；
 * 4. 排除/惩罚干扰关键词（避免现场版、伴奏、纯音乐、翻唱）：若非目标版本，候选包含 Live 扣 30 分，包含伴奏/消音扣 40 分，包含翻唱扣 25 分。
 */
struct CandidateScore
{
    int total { 0 };
    int title { 0 };

    // 歌名必须接近。只靠同歌手或时长接近不能收下一首别的歌。
    bool isAcceptable() const
    {
        return title >= 30 && total >= 40;
    }
};

CandidateScore calculateCandidateScore( const QString& candTitle,
                                        const QString& candArtist,
                                        qint64 candDurationMs,
                                        const QString& targetTitle,
                                        const QString& targetArtist,
                                        qint64 targetDurationMs )
{
    const QString cleanTargetTitle = stripTitleSuffix( targetTitle ).trimmed().toLower();
    const QString cleanCandTitle   = stripTitleSuffix( candTitle ).trimmed().toLower();
    const QString cleanTargetArtist = targetArtist.trimmed().toLower();
    const QString cleanCandArtist   = candArtist.trimmed().toLower();

    CandidateScore score;

    // 1. 歌名匹配（最高 40 分）
    if ( !cleanTargetTitle.isEmpty() && !cleanCandTitle.isEmpty() )
    {
        if ( cleanTargetTitle == cleanCandTitle )
        {
            score.title = 40;
        }
        else if ( cleanCandTitle.contains( cleanTargetTitle ) || cleanTargetTitle.contains( cleanCandTitle ) )
        {
            const int lenDiff = qAbs( cleanCandTitle.length() - cleanTargetTitle.length() );
            score.title = ( lenDiff <= 4 ) ? 30 : 20;
        }
        score.total += score.title;
    }

    // 2. 歌手匹配（最高 35 分）
    if ( cleanTargetArtist.isEmpty() )
    {
        score.total += 15; // 目标未指定歌手，给中立分
    }
    else
    {
        if ( cleanCandArtist.contains( cleanTargetArtist ) || cleanTargetArtist.contains( cleanCandArtist ) )
        {
            score.total += 35;
        }
        else if ( cleanCandArtist.isEmpty() )
        {
            score.total += 5;
        }
        else
        {
            score.total -= 35; // 目标指定了歌手，但候选是别人翻唱的，大幅扣分
        }
    }

    // 3. 时长吻合度（最高 25 分）
    if ( targetDurationMs > 0 && candDurationMs > 0 )
    {
        const qint64 diff = qAbs( targetDurationMs - candDurationMs );
        if ( diff <= 3500 )
        {
            score.total += 25; // 误差 3.5 秒以内：极大概率是相同录音室母带
        }
        else if ( diff <= 7000 )
        {
            score.total += 18;
        }
        else if ( diff <= 15000 )
        {
            score.total += 8;
        }
        else if ( diff >= 25000 )
        {
            score.total -= 35; // 相差 25 秒以上（现场版有欢呼/独白或伴奏剪辑），严重惩罚
        }
    }
    else
    {
        score.total += 10; // 无时长信息时给中立分
    }

    // 4. 惩罚干扰关键词
    const bool targetWantsLive = targetTitle.contains( QStringLiteral( "live" ), Qt::CaseInsensitive )
                              || targetTitle.contains( QStringLiteral( "现场" ) )
                              || targetTitle.contains( QStringLiteral( "演唱会" ) );
    const bool targetWantsInst = targetTitle.contains( QStringLiteral( "伴奏" ) )
                              || targetTitle.contains( QStringLiteral( "instrumental" ), Qt::CaseInsensitive )
                              || targetTitle.contains( QStringLiteral( "纯音乐" ) );
    const bool targetWantsCover = targetTitle.contains( QStringLiteral( "cover" ), Qt::CaseInsensitive )
                               || targetTitle.contains( QStringLiteral( "翻唱" ) );

    const QString lowerCandTitle = candTitle.toLower();
    const QString lowerCandArtist = candArtist.toLower();

    if ( !targetWantsLive && ( lowerCandTitle.contains( QStringLiteral( "live" ) )
                            || lowerCandTitle.contains( QStringLiteral( "现场" ) )
                            || lowerCandTitle.contains( QStringLiteral( "演唱会" ) )
                            || lowerCandTitle.contains( QStringLiteral( "音乐节" ) ) ) )
    {
        score.total -= 30;
    }

    if ( !targetWantsInst && ( lowerCandTitle.contains( QStringLiteral( "伴奏" ) )
                            || lowerCandTitle.contains( QStringLiteral( "instrumental" ) )
                            || lowerCandTitle.contains( QStringLiteral( "karaoke" ) )
                            || lowerCandTitle.contains( QStringLiteral( "消音" ) )
                            || lowerCandTitle.contains( QStringLiteral( "纯音" ) ) ) )
    {
        score.total -= 40;
    }

    if ( !targetWantsCover && ( lowerCandTitle.contains( QStringLiteral( "翻唱" ) )
                             || lowerCandTitle.contains( QStringLiteral( "cover" ) )
                             || lowerCandArtist.contains( QStringLiteral( "翻唱" ) ) ) )
    {
        score.total -= 25;
    }

    return score;
}

} // namespace

class LyricsManagerPrivate
{
public:
    LyricsManager* q{ nullptr };
    QNetworkAccessManager* netManager{ nullptr };
    QPointer<QNetworkReply> currentReply;
    QPointer<QNetworkReply> coverReply;

    quint64 currentTaskId{ 0 };
    QString currentTitle;
    QString currentArtist;
    QString rawLrc;
    QList<LyricLine> lines;
    QPixmap currentCover;

    ~LyricsManagerPrivate()
    {
        cancelCurrentRequest();
        cancelCoverRequest();
    }

    void cancelCurrentRequest()
    {
        if ( currentReply )
        {
            currentReply->abort();
            currentReply->deleteLater();
            currentReply = nullptr;
        }
    }

    void cancelCoverRequest()
    {
        if ( coverReply )
        {
            coverReply->abort();
            coverReply->deleteLater();
            coverReply = nullptr;
        }
    }

    void processLoadedLrc( const QString& title, const QString& artist, const QString& lrcContent )
    {
        currentTitle  = title;
        currentArtist = artist;
        rawLrc        = lrcContent;
        lines         = parseLrcContent( lrcContent );

        emit q->lyricsLoaded( currentTitle, currentArtist, lines );
    }

    void downloadCover( quint64 taskId, const QString& title, const QString& artist, const QString& coverUrl )
    {
        if ( coverUrl.trimmed().isEmpty() )
        {
            return;
        }

        cancelCoverRequest();

        if ( !netManager )
        {
            netManager = new QNetworkAccessManager( q );
        }

        QUrl url( coverUrl );
        QNetworkRequest req( url );
        req.setHeader( QNetworkRequest::UserAgentHeader, QStringLiteral( "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36" ) );

        QNetworkReply* reply = netManager->get( req );
        coverReply = reply;

        QObject::connect( reply, &QNetworkReply::finished, q, [ this, reply, taskId, title, artist ]() {
            reply->deleteLater();
            if ( taskId != currentTaskId )
            {
                return;
            }
            if ( coverReply == reply )
            {
                coverReply = nullptr;
            }
            if ( reply->error() == QNetworkReply::NoError )
            {
                const QByteArray imgData = reply->readAll();
                QPixmap pix;
                if ( pix.loadFromData( imgData ) && !pix.isNull() )
                {
                    currentCover = pix;
                    // 保存到本地磁盘缓存
                    const QString coverPath = coverCacheFilePathForTrack( title, artist );
                    QFile file( coverPath );
                    if ( file.open( QIODevice::WriteOnly ) )
                    {
                        file.write( imgData );
                        file.close();
                    }
                    emit q->coverImageLoaded( title, artist, currentCover );
                }
            }
        } );
    }

    void searchAndDownload( const QString& title, const QString& artist, qint64 durationMs )
    {
        cancelCurrentRequest();
        cancelCoverRequest();
        currentCover = QPixmap();
        const quint64 taskId = ++currentTaskId;

        if ( !netManager )
        {
            netManager = new QNetworkAccessManager( q );
        }

        // 构建搜索关键词：歌名 + 歌手
        QString queryStr = title;
        if ( !artist.isEmpty() )
        {
            queryStr += QStringLiteral( " " ) + artist;
        }

        // 阶段 1：首选网易云（无凭据开放接口，覆盖极广、响应极快，毫秒级 LRC）
        searchNetease( taskId, title, artist, queryStr, 0, durationMs );
    }

private:
    // =========================================================================
    // 源 1：网易云音乐（主源，免登录开放 API，毫秒级响应，标准 LRC 格式）
    // =========================================================================
    void searchNetease( quint64 taskId, const QString& title, const QString& artist, const QString& queryStr, int retryStage, qint64 durationMs )
    {
        QUrl searchUrl( QStringLiteral( "https://music.163.com/api/cloudsearch/pc" ) );
        QUrlQuery query;
        query.addQueryItem( QStringLiteral( "s" ), queryStr );
        query.addQueryItem( QStringLiteral( "type" ), QStringLiteral( "1" ) ); // 单曲
        query.addQueryItem( QStringLiteral( "offset" ), QStringLiteral( "0" ) );
        query.addQueryItem( QStringLiteral( "limit" ), QStringLiteral( "8" ) );
        searchUrl.setQuery( query );

        QNetworkRequest req( searchUrl );
        req.setHeader( QNetworkRequest::UserAgentHeader, QStringLiteral( "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36" ) );
        req.setRawHeader( "Referer", "https://music.163.com" );

        QNetworkReply* reply = netManager->get( req );
        currentReply = reply;

        QObject::connect( reply, &QNetworkReply::finished, q, [ this, reply, taskId, title, artist, queryStr, retryStage, durationMs ]() {
            reply->deleteLater();
            if ( taskId != currentTaskId )
            {
                return;
            }
            if ( currentReply == reply )
            {
                currentReply = nullptr;
            }

            if ( reply->error() != QNetworkReply::NoError )
            {
                // 网络失败，转入备源酷狗
                searchKugou( taskId, title, artist, queryStr, retryStage, durationMs );
                return;
            }

            const QByteArray data = reply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson( data );
            const QJsonObject resultObj = doc.object().value( QStringLiteral( "result" ) ).toObject();
            const QJsonArray songs = resultObj.value( QStringLiteral( "songs" ) ).toArray();

            if ( songs.isEmpty() )
            {
                // 网易云无结果，转入备源酷狗
                searchKugou( taskId, title, artist, queryStr, retryStage, durationMs );
                return;
            }

            // 遍历全部候选进行打分，挑选得分最高且满足最低阈值的歌曲
            qint64 bestSongId = 0;
            QString bestPicUrl;
            int bestScore = -999;

            for ( const auto& songVal : songs )
            {
                const QJsonObject songObj = songVal.toObject();
                const qint64 songId = songObj.value( QStringLiteral( "id" ) ).toVariant().toLongLong();
                if ( songId <= 0 )
                {
                    continue;
                }

                const QString candTitle = songObj.value( QStringLiteral( "name" ) ).toString();
                QStringList candArtists;
                const QJsonArray arList = songObj.value( QStringLiteral( "ar" ) ).isArray()
                    ? songObj.value( QStringLiteral( "ar" ) ).toArray()
                    : songObj.value( QStringLiteral( "artists" ) ).toArray();
                for ( const auto& ar : arList )
                {
                    const QString arName = ar.toObject().value( QStringLiteral( "name" ) ).toString().trimmed();
                    if ( !arName.isEmpty() )
                    {
                        candArtists.append( arName );
                    }
                }
                const QString candArtistStr = candArtists.join( QStringLiteral( " / " ) );
                const qint64 candDurationMs = songObj.value( QStringLiteral( "dt" ) ).toVariant().toLongLong();

                const CandidateScore score = calculateCandidateScore( candTitle, candArtistStr, candDurationMs, title, artist, durationMs );
                if ( !score.isAcceptable() )
                {
                    continue;
                }
                if ( score.total > bestScore )
                {
                    bestScore = score.total;
                    bestSongId = songId;

                    const QJsonObject alObj = songObj.value( QStringLiteral( "al" ) ).toObject();
                    bestPicUrl = alObj.value( QStringLiteral( "picUrl" ) ).toString();
                    if ( bestPicUrl.isEmpty() )
                    {
                        bestPicUrl = songObj.value( QStringLiteral( "album" ) ).toObject().value( QStringLiteral( "picUrl" ) ).toString();
                    }
                }
            }

            if ( bestSongId <= 0 )
            {
                // 网易云无有效歌曲，转入备源酷狗
                searchKugou( taskId, title, artist, queryStr, retryStage, durationMs );
                return;
            }

            if ( !bestPicUrl.isEmpty() )
            {
                downloadCover( taskId, title, artist, bestPicUrl );
            }

            // 获取网易云歌词
            downloadNeteaseLyric( taskId, title, artist, bestSongId, queryStr, retryStage, durationMs );
        } );
    }

    void downloadNeteaseLyric( quint64 taskId, const QString& title, const QString& artist, qint64 songId, const QString& queryStr, int retryStage, qint64 durationMs )
    {
        QUrl lyricUrl( QStringLiteral( "https://music.163.com/api/song/lyric" ) );
        QUrlQuery query;
        query.addQueryItem( QStringLiteral( "os" ), QStringLiteral( "pc" ) );
        query.addQueryItem( QStringLiteral( "id" ), QString::number( songId ) );
        query.addQueryItem( QStringLiteral( "lv" ), QStringLiteral( "-1" ) );
        query.addQueryItem( QStringLiteral( "kv" ), QStringLiteral( "-1" ) );
        query.addQueryItem( QStringLiteral( "tv" ), QStringLiteral( "-1" ) );
        lyricUrl.setQuery( query );

        QNetworkRequest req( lyricUrl );
        req.setHeader( QNetworkRequest::UserAgentHeader, QStringLiteral( "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36" ) );
        req.setRawHeader( "Referer", "https://music.163.com" );

        QNetworkReply* reply = netManager->get( req );
        currentReply = reply;

        QObject::connect( reply, &QNetworkReply::finished, q, [ this, reply, taskId, title, artist, queryStr, retryStage, durationMs ]() {
            reply->deleteLater();
            if ( taskId != currentTaskId )
            {
                return;
            }
            if ( currentReply == reply )
            {
                currentReply = nullptr;
            }

            if ( reply->error() != QNetworkReply::NoError )
            {
                searchKugou( taskId, title, artist, queryStr, retryStage, durationMs );
                return;
            }

            const QByteArray data = reply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson( data );
            const QJsonObject root = doc.object();

            // 判断是否为纯音乐
            if ( root.value( QStringLiteral( "nolyric" ) ).toBool() )
            {
                const QString pureMusicLrc = QStringLiteral( "[00:00.00]纯音乐，请欣赏\n" );
                processLoadedLrc( title, artist, pureMusicLrc );
                return;
            }

            const QString lyricText = root.value( QStringLiteral( "lrc" ) ).toObject().value( QStringLiteral( "lyric" ) ).toString();
            if ( lyricText.trimmed().isEmpty() )
            {
                searchKugou( taskId, title, artist, queryStr, retryStage, durationMs );
                return;
            }

            const auto parsedLines = parseLrcContent( lyricText );
            if ( parsedLines.size() < 2 )
            {
                searchKugou( taskId, title, artist, queryStr, retryStage, durationMs );
                return;
            }

            processLoadedLrc( title, artist, lyricText );
        } );
    }

    // =========================================================================
    // 源 2：酷狗音乐（备选兜底源，海量移动端曲库）
    // =========================================================================
    void searchKugou( quint64 taskId, const QString& title, const QString& artist, const QString& queryStr, int retryStage, qint64 durationMs )
    {
        QUrl searchUrl( QStringLiteral( "http://krcs.kugou.com/search" ) );
        QUrlQuery query;
        query.addQueryItem( QStringLiteral( "ver" ), QStringLiteral( "1" ) );
        query.addQueryItem( QStringLiteral( "man" ), QStringLiteral( "yes" ) );
        query.addQueryItem( QStringLiteral( "client" ), QStringLiteral( "mobi" ) );
        query.addQueryItem( QStringLiteral( "keyword" ), queryStr );
        query.addQueryItem( QStringLiteral( "duration" ), QStringLiteral( "0" ) );
        query.addQueryItem( QStringLiteral( "hash" ), QString() );
        searchUrl.setQuery( query );

        QNetworkRequest req( searchUrl );
        req.setHeader( QNetworkRequest::UserAgentHeader, QStringLiteral( "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36" ) );

        QNetworkReply* reply = netManager->get( req );
        currentReply = reply;

        QObject::connect( reply, &QNetworkReply::finished, q, [ this, reply, taskId, title, artist, retryStage, durationMs ]() {
            reply->deleteLater();
            if ( taskId != currentTaskId )
            {
                return;
            }
            if ( currentReply == reply )
            {
                currentReply = nullptr;
            }

            if ( reply->error() != QNetworkReply::NoError )
            {
                advanceRetry( taskId, title, artist, retryStage, durationMs );
                return;
            }

            const QByteArray data = reply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson( data );
            const QJsonArray cands = doc.object().value( QStringLiteral( "candidates" ) ).toArray();

            if ( cands.isEmpty() )
            {
                advanceRetry( taskId, title, artist, retryStage, durationMs );
                return;
            }

            QString bestCid;
            QString bestCkey;
            int bestScore = -999;

            for ( const auto& itemVal : cands )
            {
                const QJsonObject cand = itemVal.toObject();
                const QString cid = cand.value( QStringLiteral( "id" ) ).toVariant().toString();
                const QString ckey = cand.value( QStringLiteral( "accesskey" ) ).toString();
                if ( cid.isEmpty() || ckey.isEmpty() )
                {
                    continue;
                }

                const QString candTitle = cand.value( QStringLiteral( "song" ) ).toString();
                const QString candArtist = cand.value( QStringLiteral( "singer" ) ).toString();
                const qint64 candDurationMs = cand.value( QStringLiteral( "duration" ) ).toVariant().toLongLong() * 1000;

                const CandidateScore score = calculateCandidateScore( candTitle, candArtist, candDurationMs, title, artist, durationMs );
                if ( !score.isAcceptable() )
                {
                    continue;
                }
                if ( score.total > bestScore )
                {
                    bestScore = score.total;
                    bestCid = cid;
                    bestCkey = ckey;
                }
            }

            if ( bestCid.isEmpty() || bestCkey.isEmpty() )
            {
                advanceRetry( taskId, title, artist, retryStage, durationMs );
                return;
            }

            downloadKugouLyric( taskId, title, artist, bestCid, bestCkey, retryStage, durationMs );
        } );
    }

    void downloadKugouLyric( quint64 taskId, const QString& title, const QString& artist, const QString& id, const QString& accesskey, int retryStage, qint64 durationMs )
    {
        QUrl downUrl( QStringLiteral( "http://krcs.kugou.com/download" ) );
        QUrlQuery query;
        query.addQueryItem( QStringLiteral( "ver" ), QStringLiteral( "1" ) );
        query.addQueryItem( QStringLiteral( "client" ), QStringLiteral( "mobi" ) );
        query.addQueryItem( QStringLiteral( "id" ), id );
        query.addQueryItem( QStringLiteral( "accesskey" ), accesskey );
        query.addQueryItem( QStringLiteral( "fmt" ), QStringLiteral( "lrc" ) );
        query.addQueryItem( QStringLiteral( "charset" ), QStringLiteral( "utf8" ) );
        downUrl.setQuery( query );

        QNetworkRequest req( downUrl );
        req.setHeader( QNetworkRequest::UserAgentHeader, QStringLiteral( "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36" ) );

        QNetworkReply* reply = netManager->get( req );
        currentReply = reply;

        QObject::connect( reply, &QNetworkReply::finished, q, [ this, reply, taskId, title, artist, retryStage, durationMs ]() {
            reply->deleteLater();
            if ( taskId != currentTaskId )
            {
                return;
            }
            if ( currentReply == reply )
            {
                currentReply = nullptr;
            }

            if ( reply->error() != QNetworkReply::NoError )
            {
                advanceRetry( taskId, title, artist, retryStage, durationMs );
                return;
            }

            const QByteArray data = reply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson( data );
            const QString contentB64 = doc.object().value( QStringLiteral( "content" ) ).toString();

            if ( contentB64.trimmed().isEmpty() )
            {
                advanceRetry( taskId, title, artist, retryStage, durationMs );
                return;
            }

            const QByteArray decodedBytes = QByteArray::fromBase64( contentB64.toUtf8() );
            const QString decodedLrc = QString::fromUtf8( decodedBytes );
            const auto parsedLines = parseLrcContent( decodedLrc );

            if ( parsedLines.size() < 2 )
            {
                advanceRetry( taskId, title, artist, retryStage, durationMs );
                return;
            }

            processLoadedLrc( title, artist, decodedLrc );
        } );
    }

    // =========================================================================
    // 多阶段智能清洗重试流水线
    // =========================================================================
    void advanceRetry( quint64 taskId, const QString& title, const QString& artist, int retryStage, qint64 durationMs )
    {
        if ( retryStage == 0 )
        {
            // 阶段 1 重试：剥离括号与现场版后缀重试（如 "一生所爱 (Live)" -> "一生所爱 伍佰"）
            const QString strippedTitle = stripTitleSuffix( title );
            if ( !strippedTitle.isEmpty() && strippedTitle != title )
            {
                QString nextQuery = strippedTitle;
                if ( !artist.isEmpty() )
                {
                    nextQuery += QStringLiteral( " " ) + artist;
                }
                searchNetease( taskId, title, artist, nextQuery, 1, durationMs );
                return;
            }
            retryStage = 1; // 无后缀可剥离，直接跳入下一阶段
        }

        if ( retryStage == 1 )
        {
            // 阶段 2 重试：若带歌手搜不到，仅以纯歌名进行兜底检索，配合打分算法仍可挑出最高质量原版
            const QString strippedTitle = stripTitleSuffix( title );
            const QString soloTitle     = strippedTitle.isEmpty() ? title : strippedTitle;
            if ( !artist.isEmpty() && !soloTitle.isEmpty() )
            {
                searchNetease( taskId, title, artist, soloTitle, 2, durationMs );
                return;
            }
        }

        // 所有阶段均已尝试穷尽，发出失败信号
        emit q->lyricsLoadFailed( title, artist, QObject::tr( "未找到匹配的歌词" ) );
    }
};

LyricsManager::LyricsManager( QObject* parent )
    : QObject( parent )
    , d( new LyricsManagerPrivate )
{
    d->q = this;
}

LyricsManager::~LyricsManager()
{
    delete d;
}

void LyricsManager::fetchLyrics( const QString& title, const QString& artist, qint64 durationMs )
{
    const QString cleanTitle = title.trimmed();
    if ( cleanTitle.isEmpty() )
    {
        clear();
        emit lyricsLoadFailed( title, artist, tr( "歌曲名称为空" ) );
        return;
    }

    // 检查本地持久封面缓存
    const QString coverPath = coverCacheFilePathForTrack( cleanTitle, artist );
    if ( QFile::exists( coverPath ) )
    {
        QPixmap p;
        if ( p.load( coverPath ) && !p.isNull() )
        {
            d->currentCover = p;
            emit coverImageLoaded( cleanTitle, artist, p );
        }
    }

    d->searchAndDownload( cleanTitle, artist, durationMs );
}

void LyricsManager::refetchLyrics( const QString& title, const QString& artist, qint64 durationMs )
{
    const QString cleanTitle = title.trimmed();
    if ( cleanTitle.isEmpty() )
    {
        clear();
        emit lyricsLoadFailed( title, artist, tr( "歌曲名称为空" ) );
        return;
    }

    // 重新检索时丢掉已缓存的封面，歌词本身不再落盘。
    const QString coverPath = coverCacheFilePathForTrack( cleanTitle, artist );
    QFile::remove( coverPath );

    clear();
    d->searchAndDownload( cleanTitle, artist, durationMs );
}

QList<LyricLine> LyricsManager::currentLines() const
{
    return d->lines;
}

QString LyricsManager::currentRawLrc() const
{
    return d->rawLrc;
}

QPixmap LyricsManager::currentCoverPixmap() const
{
    return d->currentCover;
}

int LyricsManager::findLineIndex( qint64 timeMs ) const
{
    if ( d->lines.isEmpty() )
    {
        return -1;
    }

    int left  = 0;
    int right = d->lines.size() - 1;
    int ans   = -1;

    while ( left <= right )
    {
        const int mid = left + ( right - left ) / 2;
        if ( d->lines[ mid ].timeMs <= timeMs )
        {
            ans  = mid;
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }

    return ans;
}

QString LyricsManager::lineText( qint64 timeMs ) const
{
    const int idx = findLineIndex( timeMs );
    if ( idx >= 0 && idx < d->lines.size() )
    {
        return d->lines[ idx ].text;
    }
    return QString();
}

void LyricsManager::clear()
{
    d->cancelCurrentRequest();
    d->cancelCoverRequest();
    d->currentTitle.clear();
    d->currentArtist.clear();
    d->rawLrc.clear();
    d->lines.clear();
    d->currentCover = QPixmap();
}

