#pragma once

#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

#include <QColor>
#include <QList>
#include <QString>
#include <QWidget>

/**
 * @brief 单行歌词数据模型。
 */
struct ExLyricLine
{
    qint64 timeMs{ 0 };  ///< 歌词开始时间（毫秒）
    QString text;        ///< 歌词正文文本

    bool isValid() const
    {
        return !text.trimmed().isEmpty();
    }
};

class ExLyricsWidgetPrivate;

/**
 * @brief 现代化 WinUI 3 / Apple Music 风格自定义平滑滚动歌词控件。
 *
 * 【核心特性】
 * 1. 纯 QPainter 矢量渲染，彻底脱离 QListWidget 离散滚动与布局限制；
 * 2. 毫秒级物理缓动平滑居中滚动动画（OutCubic Easing），丝滑无顿挫；
 * 3. 动态视觉焦距梯队（高亮主行加粗强调，邻近行半透明次级，远端行渐隐淡出）；
 * 4. 视窗上下边缘柔和渐变羽化遮罩（Fade Mask），电影级视觉质感；
 * 5. 用户鼠标滚轮自由浏览，3 秒无操作自动丝滑回正；
 * 6. 状态自适应（占位提示、获取中、纯音乐欣赏、正常歌词）。
 */
class EXWIDGETS_EXPORT ExLyricsWidget : public QWidget
{
    Q_OBJECT

public:
    enum class ViewMode
    {
        MultiLine,   ///< 多行滚动视图（默认）：平滑居中滚动、视觉焦距梯队、边缘羽化遮罩、滚轮手动浏览与跳播
        SingleLine   ///< 单行卡拉OK视图：专注单行居中展示与进度扫光染色、边缘无羽化、鼠标滚轮智能放行穿透，专为沉浸/桌面悬浮设计
    };
    Q_ENUM( ViewMode )

    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY( QColor, highlightColor, highlightColor, setHighlightColor, highlightColorChanged )
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY( int, fontSizeLevel, fontSizeLevel, setFontSizeLevel, fontSizeLevelChanged )
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY( bool, autoScroll, isAutoScroll, setAutoScroll, autoScrollChanged )
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY( int, scrollDurationMs, scrollDurationMs, setScrollDurationMs, scrollDurationMsChanged )
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY( bool, karaokeProgress, isKaraokeProgress, setKaraokeProgress, karaokeProgressChanged )
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY( ViewMode, viewMode, viewMode, setViewMode, viewModeChanged )

    static constexpr int DefaultScrollDurationMs = 350;
    static constexpr int DefaultFontSizeLevel    = 1;  // 0: 小, 1: 标准, 2: 大, 3: 特大
    static constexpr bool DefaultKaraokeProgress = true;  // 默认启用跟随进度从左向右平滑高亮扫光
    static constexpr ViewMode DefaultViewMode    = ViewMode::MultiLine;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    explicit ExLyricsWidget( QWidget* parent = nullptr );
    ~ExLyricsWidget() override;

    /// 设置标准 LRC 歌词全文（内部自动解析时间轴）
    Q_INVOKABLE void setLyrics( const QString& lrcContent );

    /// 设置已解析的歌词行列表
    void setLyricsLines( const QList<ExLyricLine>& lines );

    /// 当前歌词行列表
    QList<ExLyricLine> lyricsLines() const;

    /// 设置当前播放时间戳（毫秒），控件自动二分查找并平滑滚动到对应歌词行
    Q_INVOKABLE void setCurrentPositionMs( qint64 positionMs );

    /// 当前播放位置（毫秒）
    qint64 currentPositionMs() const;

    /// 当前高亮行索引（未开始或无歌词返回 -1）
    int currentLineIndex() const;

    /// 当前高亮行歌词文本
    QString currentLineText() const;

    /// 设置居中占位提示语（如“正在获取歌词…”、“纯音乐，请欣赏”、“暂无歌词”）
    Q_INVOKABLE void setStatusMessage( const QString& message );

    /// 获取当前占位提示语
    QString statusMessage() const;

    /// 清空所有歌词并恢复为“暂无歌词”
    Q_INVOKABLE void clear();

    /// 手动滚动到指定行并居中
    Q_INVOKABLE void scrollToLine( int index, bool animated = true );

signals:
    /// 当前高亮行发生变更
    void currentLineChanged( int index, const QString& text, qint64 timeMs );

    /// 用户点击某行歌词
    void lineClicked( int index, qint64 timeMs, const QString& text );

    /// 用户滚轮手动浏览状态改变
    void userScrollingChanged( bool isScrolling );

protected:
    void paintEvent( QPaintEvent* event ) override;
    void wheelEvent( QWheelEvent* event ) override;
    void mousePressEvent( QMouseEvent* event ) override;
    void mouseDoubleClickEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;
    void resizeEvent( QResizeEvent* event ) override;
    void changeEvent( QEvent* event ) override;

private:
    Q_DECLARE_PRIVATE( ExLyricsWidget )
    QScopedPointer<ExLyricsWidgetPrivate> d_ptr;
};
