#pragma once

#include <QList>
#include <QObject>
#include <QPixmap>
#include <QString>

/**
 * @brief 单行歌词信息。
 */
struct LyricLine
{
    qint64 timeMs{ 0 };  ///< 歌词开始时间戳（毫秒）
    QString text;        ///< 歌词文本内容

    bool isValid() const
    {
        return !text.trimmed().isEmpty();
    }
};

class LyricsManagerPrivate;

/**
 * @brief 在线歌词检索与实时解析管理器。
 *
 * 根据“歌名+歌手”每次重新检索网络标准 LRC 歌词，不在本地保留歌词文件。
 * 提供快速二分时间轴查找当前正在演唱的歌词行。
 */
class LyricsManager : public QObject
{
    Q_OBJECT

public:
    explicit LyricsManager( QObject* parent = nullptr );
    ~LyricsManager() override;

    /// 异步从网络获取歌词
    void fetchLyrics( const QString& title, const QString& artist, qint64 durationMs = 0 );

    /// 强制重新从网络检索歌词，并丢掉已缓存的封面
    void refetchLyrics( const QString& title, const QString& artist, qint64 durationMs = 0 );

    /// 获取当前加载的歌词列表
    QList<LyricLine> currentLines() const;

    /// 获取当前原始 LRC 文本
    QString currentRawLrc() const;

    /// 获取当前加载的专辑封面图像
    QPixmap currentCoverPixmap() const;

    /// 根据毫秒时间戳查找对应的歌词行索引（未开始返回 -1）
    int findLineIndex( qint64 timeMs ) const;

    /// 根据毫秒时间戳直接获取当前歌词文本
    QString lineText( qint64 timeMs ) const;

    /// 清空当前歌词与封面
    void clear();

signals:
    /// 歌词加载并解析成功
    void lyricsLoaded( const QString& title, const QString& artist, const QList<LyricLine>& lines );

    /// 歌词加载失败或未检索到
    void lyricsLoadFailed( const QString& title, const QString& artist, const QString& reason );

    /// 专辑封面图片加载成功
    void coverImageLoaded( const QString& title, const QString& artist, const QPixmap& coverPixmap );

private:
    LyricsManagerPrivate* d{ nullptr };
};
