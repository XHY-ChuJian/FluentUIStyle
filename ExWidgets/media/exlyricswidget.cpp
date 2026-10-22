#include "exlyricswidget.h"

#include <QApplication>
#include <QDateTime>
#include <QEasingCurve>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QTimer>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <algorithm>

namespace {

QColor resolveAccentColor( const QPalette& palette )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    const QColor accent = palette.color( QPalette::Accent );
    if ( accent.isValid() )
    {
        return accent;
    }
#endif
    return palette.color( QPalette::Highlight );
}

struct LrcStamp
{
    qint64 timeMs { 0 };
    int startPos { 0 };
    int endPos { 0 };
};

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

} // namespace

class ExLyricsWidgetPrivate
{
public:
    explicit ExLyricsWidgetPrivate( ExLyricsWidget* q )
        : q_ptr( q )
    {
    }

    ExLyricsWidget* q_ptr{ nullptr };
    Q_DECLARE_PUBLIC( ExLyricsWidget )

    // 属性
    QColor highlightColor;
    int fontSizeLevel{ ExLyricsWidget::DefaultFontSizeLevel };
    bool autoScroll{ true };
    int scrollDurationMs{ ExLyricsWidget::DefaultScrollDurationMs };
    bool karaokeProgress{ ExLyricsWidget::DefaultKaraokeProgress };
    ExLyricsWidget::ViewMode viewMode{ ExLyricsWidget::DefaultViewMode };

    // 歌词数据
    QList<ExLyricLine> lines;
    int currentIndex{ -1 };
    qint64 currentPositionMs{ 0 };
    QString statusMessage{ QStringLiteral( "暂无歌词" ) };

    // 滚动动画与交互
    qreal currentScrollY{ 0.0 };
    qreal targetScrollY{ 0.0 };
    QVariantAnimation* scrollAnimation{ nullptr };
    QTimer* userScrollResetTimer{ nullptr };
    bool isUserScrolling{ false };

    // 布局缓存
    QVector<qreal> lineHeights;
    QVector<qreal> lineOffsets;
    qreal totalContentHeight{ 0.0 };

    void init();
    void recalculateLayout();
    void updateTargetScroll( bool animated );
    int findLineIndexAtPosition( qint64 ms ) const;
    QFont fontForLine( int index ) const;
    int activeFontSizePt() const;
    int normalFontSizePt() const;
    qreal lineHeightForLevel() const;
    QColor effectiveHighlightColor() const;
};

void ExLyricsWidgetPrivate::init()
{
    Q_Q( ExLyricsWidget );

    scrollAnimation = new QVariantAnimation( q );
    scrollAnimation->setDuration( scrollDurationMs );
    scrollAnimation->setEasingCurve( QEasingCurve::OutCubic );

    QObject::connect( scrollAnimation, &QVariantAnimation::valueChanged, q, [ this, q ]( const QVariant& value ) {
        currentScrollY = value.toReal();
        q->update();
    } );

    userScrollResetTimer = new QTimer( q );
    userScrollResetTimer->setSingleShot( true );
    userScrollResetTimer->setInterval( 3000 ); // 3秒无操作自动回正

    QObject::connect( userScrollResetTimer, &QTimer::timeout, q, [ this, q ]() {
        if ( isUserScrolling )
        {
            isUserScrolling = false;
            emit q->userScrollingChanged( false );
            updateTargetScroll( true );
        }
    } );
}

int ExLyricsWidgetPrivate::activeFontSizePt() const
{
    switch ( fontSizeLevel )
    {
    case 0:
        return 11; // 小
    case 1:
        return 13; // 标准
    case 2:
        return 15; // 大
    case 3:
        return 17; // 特大
    default:
        return 13;
    }
}

int ExLyricsWidgetPrivate::normalFontSizePt() const
{
    switch ( fontSizeLevel )
    {
    case 0:
        return 9;
    case 1:
        return 10;
    case 2:
        return 12;
    case 3:
        return 13;
    default:
        return 10;
    }
}

qreal ExLyricsWidgetPrivate::lineHeightForLevel() const
{
    switch ( fontSizeLevel )
    {
    case 0:
        return 28.0;
    case 1:
        return 34.0;
    case 2:
        return 40.0;
    case 3:
        return 48.0;
    default:
        return 34.0;
    }
}

QFont ExLyricsWidgetPrivate::fontForLine( int index ) const
{
    Q_Q( const ExLyricsWidget );
    QFont f = q->font();
    if ( index == currentIndex )
    {
        f.setPointSize( activeFontSizePt() );
        f.setBold( true );
    }
    else
    {
        f.setPointSize( normalFontSizePt() );
        f.setBold( false );
    }
    return f;
}

QColor ExLyricsWidgetPrivate::effectiveHighlightColor() const
{
    Q_Q( const ExLyricsWidget );
    if ( highlightColor.isValid() )
    {
        return highlightColor;
    }
    return resolveAccentColor( q->palette() );
}

void ExLyricsWidgetPrivate::recalculateLayout()
{
    Q_Q( ExLyricsWidget );
    lineHeights.clear();
    lineOffsets.clear();
    totalContentHeight = 0.0;

    const int n = lines.size();
    if ( n <= 0 )
    {
        return;
    }

    lineHeights.resize( n );
    lineOffsets.resize( n );

    const qreal baseLineHeight = lineHeightForLevel();
    const qreal availableWidth = qMax( 60, q->width() - 32 );

    QFont normFont = q->font();
    normFont.setPointSize( normalFontSizePt() );
    QFontMetrics normFm( normFont );

    QFont activeFont = q->font();
    activeFont.setPointSize( activeFontSizePt() );
    activeFont.setBold( true );
    QFontMetrics activeFm( activeFont );

    qreal currentOffset = 0.0;
    for ( int i = 0; i < n; ++i )
    {
        lineOffsets[ i ] = currentOffset;

        // 根据文本是否过长自动计算高度（支持自适应折行）
        const QFontMetrics& fm = ( i == currentIndex ) ? activeFm : normFm;
        const QRect textRect   = fm.boundingRect( QRect( 0, 0, static_cast<int>( availableWidth ), 9999 ),
                                                Qt::AlignCenter | Qt::TextWordWrap,
                                                lines[ i ].text );

        const qreal h    = qMax( baseLineHeight, static_cast<qreal>( textRect.height() + 8 ) );
        lineHeights[ i ] = h;
        currentOffset += h;
    }

    totalContentHeight = currentOffset;
}

void ExLyricsWidgetPrivate::updateTargetScroll( bool animated )
{
    Q_Q( ExLyricsWidget );
    if ( lines.isEmpty() || currentIndex < 0 || currentIndex >= lines.size() )
    {
        targetScrollY = 0.0;
    }
    else if ( currentIndex < lineOffsets.size() && currentIndex < lineHeights.size() )
    {
        // 目标：让当前高亮行的中心正对控件的几何垂直中心
        targetScrollY = lineOffsets[ currentIndex ] + ( lineHeights[ currentIndex ] / 2.0 );
    }

    if ( isUserScrolling )
    {
        return;
    }

    if ( !animated || scrollDurationMs <= 0 )
    {
        scrollAnimation->stop();
        currentScrollY = targetScrollY;
        q->update();
    }
    else
    {
        scrollAnimation->stop();
        scrollAnimation->setStartValue( currentScrollY );
        scrollAnimation->setEndValue( targetScrollY );
        scrollAnimation->setDuration( scrollDurationMs );
        scrollAnimation->start();
    }
}

int ExLyricsWidgetPrivate::findLineIndexAtPosition( qint64 ms ) const
{
    if ( lines.isEmpty() )
    {
        return -1;
    }

    // 二分查找：找到 lines[i].timeMs <= ms 的最大 i
    int left  = 0;
    int right = lines.size() - 1;
    int ans   = -1;

    while ( left <= right )
    {
        const int mid = left + ( right - left ) / 2;
        if ( lines[ mid ].timeMs <= ms )
        {
            ans  = mid;
            left = mid + 1; // 往右找更晚的时间点
        }
        else
        {
            right = mid - 1;
        }
    }

    return ans;
}

// -----------------------------------------------------------------------------
// ExLyricsWidget 实现
// -----------------------------------------------------------------------------

ExLyricsWidget::ExLyricsWidget( QWidget* parent )
    : QWidget( parent )
    , d_ptr( new ExLyricsWidgetPrivate( this ) )
{
    d_ptr->init();

    setAttribute( Qt::WA_Hover, true );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setMinimumHeight( 60 );
}

ExLyricsWidget::~ExLyricsWidget() = default;

QColor ExLyricsWidget::highlightColor() const
{
    Q_D( const ExLyricsWidget );
    return d->highlightColor;
}

void ExLyricsWidget::setHighlightColor( QColor color )
{
    Q_D( ExLyricsWidget );
    if ( d->highlightColor == color )
    {
        return;
    }
    d->highlightColor = color;
    emit highlightColorChanged( color );
    update();
}

int ExLyricsWidget::fontSizeLevel() const
{
    Q_D( const ExLyricsWidget );
    return d->fontSizeLevel;
}

void ExLyricsWidget::setFontSizeLevel( int level )
{
    Q_D( ExLyricsWidget );
    const int clamped = qBound( 0, level, 3 );
    if ( d->fontSizeLevel == clamped )
    {
        return;
    }
    d->fontSizeLevel = clamped;
    d->recalculateLayout();
    d->updateTargetScroll( false );
    emit fontSizeLevelChanged( clamped );
    update();
}

bool ExLyricsWidget::isAutoScroll() const
{
    Q_D( const ExLyricsWidget );
    return d->autoScroll;
}

void ExLyricsWidget::setAutoScroll( bool autoScroll )
{
    Q_D( ExLyricsWidget );
    if ( d->autoScroll == autoScroll )
    {
        return;
    }
    d->autoScroll = autoScroll;
    emit autoScrollChanged( autoScroll );
    if ( autoScroll )
    {
        d->updateTargetScroll( true );
    }
}

int ExLyricsWidget::scrollDurationMs() const
{
    Q_D( const ExLyricsWidget );
    return d->scrollDurationMs;
}

void ExLyricsWidget::setScrollDurationMs( int ms )
{
    Q_D( ExLyricsWidget );
    const int clamped = qMax( 0, ms );
    if ( d->scrollDurationMs == clamped )
    {
        return;
    }
    d->scrollDurationMs = clamped;
    if ( d->scrollAnimation )
    {
        d->scrollAnimation->setDuration( clamped );
    }
    emit scrollDurationMsChanged( clamped );
}

bool ExLyricsWidget::isKaraokeProgress() const
{
    Q_D( const ExLyricsWidget );
    return d->karaokeProgress;
}

void ExLyricsWidget::setKaraokeProgress( bool enabled )
{
    Q_D( ExLyricsWidget );
    if ( d->karaokeProgress == enabled )
    {
        return;
    }
    d->karaokeProgress = enabled;
    emit karaokeProgressChanged( enabled );
    update();
}

ExLyricsWidget::ViewMode ExLyricsWidget::viewMode() const
{
    Q_D( const ExLyricsWidget );
    return d->viewMode;
}

void ExLyricsWidget::setViewMode( ViewMode mode )
{
    Q_D( ExLyricsWidget );
    if ( d->viewMode == mode )
    {
        return;
    }
    d->viewMode = mode;
    updateGeometry();
    emit viewModeChanged( mode );
    update();
}

QSize ExLyricsWidget::sizeHint() const
{
    Q_D( const ExLyricsWidget );
    if ( d->viewMode == ViewMode::SingleLine )
    {
        const int h = qMax( 28, qRound( d->lineHeightForLevel() ) );
        return QSize( 300, h );
    }
    return QSize( 300, 240 );
}

QSize ExLyricsWidget::minimumSizeHint() const
{
    Q_D( const ExLyricsWidget );
    if ( d->viewMode == ViewMode::SingleLine )
    {
        const int h = qMax( 24, qRound( d->lineHeightForLevel() ) );
        return QSize( 100, h );
    }
    return QSize( 100, 80 );
}

void ExLyricsWidget::setLyrics( const QString& lrcContent )
{
    QList<ExLyricLine> parsedLines;
    if ( lrcContent.trimmed().isEmpty() )
    {
        setLyricsLines( parsedLines );
        return;
    }

    // [offset:+500] 按 LRC 约定加到每一行时间上。正数表示歌词整体推后。
    qint64 offsetMs = 0;
    static const QRegularExpression offsetRegex( QStringLiteral( "\\[offset:\\s*([+-]?\\d+)\\s*\\]" ),
                                                  QRegularExpression::CaseInsensitiveOption );
    const QRegularExpressionMatch offsetMatch = offsetRegex.match( lrcContent );
    if ( offsetMatch.hasMatch() )
    {
        offsetMs = offsetMatch.captured( 1 ).toLongLong();
    }

    // 支持 [mm:ss.xx]、[mm:ss.xxx]、[mm:ss:xx] 以及同一行多个时间戳
    static const QRegularExpression timeRegex( QStringLiteral( "\\[(\\d{1,3}):(\\d{1,2})(?:[.:](\\d{1,3}))?\\]" ) );

    const QStringList rawLines = lrcContent.split( QLatin1Char( '\n' ) );
    for ( const QString& rawLine : rawLines )
    {
        const QString trimmedLine = rawLine.trimmed();
        if ( trimmedLine.isEmpty() )
        {
            continue;
        }

        QList<LrcStamp> stamps;

        auto it = timeRegex.globalMatch( trimmedLine );
        while ( it.hasNext() )
        {
            auto match = it.next();
            const qint64 min = match.captured( 1 ).toLongLong();
            const qint64 sec = match.captured( 2 ).toLongLong();
            QString msStr = match.captured( 3 );
            qint64 ms = 0;
            if ( !msStr.isEmpty() )
            {
                ms = msStr.toLongLong();
                if ( msStr.length() == 1 )
                {
                    ms *= 100;
                }
                else if ( msStr.length() == 2 )
                {
                    ms *= 10;
                }
            }
            stamps.append( { qMax( qint64( 0 ), ( min * 60 + sec ) * 1000 + ms + offsetMs ),
                             static_cast<int>( match.capturedStart() ),
                             static_cast<int>( match.capturedEnd() ) } );
        }

        if ( stamps.isEmpty() )
        {
            continue;
        }

        // 逐字轴的字写在时间标签之间，拼回一整句，避免只剩下最后一个标签后的尾巴。
        QString lyricText;
        bool inlineText = false;
        for ( int i = 0; i < stamps.size(); ++i )
        {
            const int from = stamps[ i ].endPos;
            const int to   = ( i + 1 < stamps.size() ) ? stamps[ i + 1 ].startPos : static_cast<int>( trimmedLine.size() );
            const QString piece = trimmedLine.mid( from, to - from );
            if ( i + 1 < stamps.size() && !piece.trimmed().isEmpty() )
            {
                inlineText = true;
            }
            lyricText += piece;
        }
        static const QRegularExpression wordTagRegex( QStringLiteral( "<\\d{1,3}:\\d{1,2}(?:[.:]\\d{1,3})?>" ) );
        lyricText.remove( wordTagRegex );
        lyricText = lyricText.trimmed();
        if ( lyricText.isEmpty() )
        {
            continue;
        }

        if ( inlineText )
        {
            ExLyricLine line;
            line.timeMs = stamps.first().timeMs;
            line.text   = lyricText;
            parsedLines.append( line );
        }
        else
        {
            for ( const LrcStamp& stamp : stamps )
            {
                ExLyricLine line;
                line.timeMs = stamp.timeMs;
                line.text   = lyricText;
                parsedLines.append( line );
            }
        }
    }

    setLyricsLines( parsedLines );
}

void ExLyricsWidget::setLyricsLines( const QList<ExLyricLine>& lines )
{
    Q_D( ExLyricsWidget );
    const int oldIndex       = d->currentIndex;
    const bool wasScrolling  = d->isUserScrolling;

    d->lines.clear();
    d->lines.reserve( lines.size() );
    for ( const ExLyricLine& line : lines )
    {
        if ( !isNonLyricMetaLine( line.text ) )
        {
            d->lines.append( line );
        }
    }
    std::stable_sort( d->lines.begin(), d->lines.end(), []( const ExLyricLine& a, const ExLyricLine& b ) {
        return a.timeMs < b.timeMs;
    } );
    d->currentIndex = -1;
    d->currentPositionMs = 0;
    d->currentScrollY = 0.0;
    d->targetScrollY = 0.0;
    d->isUserScrolling = false;
    d->scrollAnimation->stop();
    d->userScrollResetTimer->stop();

    if ( d->lines.isEmpty() )
    {
        d->statusMessage = QStringLiteral( "暂无歌词" );
    }

    d->recalculateLayout();
    if ( oldIndex != -1 )
    {
        emit currentLineChanged( -1, QString(), 0 );
    }
    if ( wasScrolling )
    {
        emit userScrollingChanged( false );
    }
    update();
}

QList<ExLyricLine> ExLyricsWidget::lyricsLines() const
{
    Q_D( const ExLyricsWidget );
    return d->lines;
}

void ExLyricsWidget::setCurrentPositionMs( qint64 positionMs )
{
    Q_D( ExLyricsWidget );
    d->currentPositionMs = positionMs;

    if ( d->lines.isEmpty() )
    {
        return;
    }

    const int newIndex = d->findLineIndexAtPosition( positionMs );
    if ( newIndex != d->currentIndex )
    {
        d->currentIndex = newIndex;
        // 当前行用更大的字号，行高变了，先重排再滚动，否则高亮行会被裁切、居中也会偏。
        d->recalculateLayout();
        if ( d->currentIndex >= 0 && d->currentIndex < d->lines.size() )
        {
            emit currentLineChanged( d->currentIndex, d->lines[ d->currentIndex ].text, d->lines[ d->currentIndex ].timeMs );
        }
        else
        {
            emit currentLineChanged( -1, QString(), 0 );
        }

        if ( d->autoScroll && !d->isUserScrolling )
        {
            d->updateTargetScroll( true );
        }
        else
        {
            update();
        }
    }
    else
    {
        // 关键：在同一句歌词内部播放时，如果启用了跟随进度高亮（karaokeProgress）或处于单行沉浸模式，
        // 触发 update() 重绘，以便 paintEvent 根据新的 currentPositionMs 计算进度并从左向右推进扫光高亮！
        if ( ( d->karaokeProgress && d->currentIndex >= 0 ) || d->viewMode == ViewMode::SingleLine )
        {
            update();
        }
    }
}

qint64 ExLyricsWidget::currentPositionMs() const
{
    Q_D( const ExLyricsWidget );
    return d->currentPositionMs;
}

int ExLyricsWidget::currentLineIndex() const
{
    Q_D( const ExLyricsWidget );
    return d->currentIndex;
}

QString ExLyricsWidget::currentLineText() const
{
    Q_D( const ExLyricsWidget );
    if ( d->currentIndex >= 0 && d->currentIndex < d->lines.size() )
    {
        return d->lines[ d->currentIndex ].text;
    }
    return QString();
}

void ExLyricsWidget::setStatusMessage( const QString& message )
{
    Q_D( ExLyricsWidget );
    if ( d->statusMessage != message )
    {
        d->statusMessage = message;
        update();
    }
}

QString ExLyricsWidget::statusMessage() const
{
    Q_D( const ExLyricsWidget );
    return d->statusMessage;
}

void ExLyricsWidget::clear()
{
    Q_D( ExLyricsWidget );
    const int oldIndex      = d->currentIndex;
    const bool wasScrolling = d->isUserScrolling;
    d->lines.clear();
    d->currentIndex = -1;
    d->currentPositionMs = 0;
    d->currentScrollY = 0.0;
    d->targetScrollY = 0.0;
    d->statusMessage = QStringLiteral( "暂无歌词" );
    d->scrollAnimation->stop();
    d->userScrollResetTimer->stop();
    d->isUserScrolling = false;
    d->recalculateLayout();
    if ( oldIndex != -1 )
    {
        emit currentLineChanged( -1, QString(), 0 );
    }
    if ( wasScrolling )
    {
        emit userScrollingChanged( false );
    }
    update();
}

void ExLyricsWidget::scrollToLine( int index, bool animated )
{
    Q_D( ExLyricsWidget );
    if ( index >= 0 && index < d->lines.size() )
    {
        const bool changed = d->currentIndex != index;
        d->currentIndex    = index;
        d->recalculateLayout();
        d->updateTargetScroll( animated );
        if ( changed )
        {
            emit currentLineChanged( index, d->lines[ index ].text, d->lines[ index ].timeMs );
        }
    }
}

void ExLyricsWidget::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event )
    Q_D( ExLyricsWidget );

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing, true );
    painter.setRenderHint( QPainter::TextAntialiasing, true );

    const int w = width();
    const int h = height();
    if ( w <= 0 || h <= 0 )
    {
        return;
    }

    // A. 单行卡拉OK沉浸模式渲染
    if ( d->viewMode == ViewMode::SingleLine )
    {
        const QVariant scheme = qApp ? qApp->property( "_q_colorscheme" ) : QVariant();
        const bool isDark     = scheme.isValid() ? ( scheme.toInt() == 1 )
                                                 : ( palette().color( QPalette::Window ).lightness() < 128 );

        if ( d->lines.isEmpty() )
        {
            if ( !d->statusMessage.trimmed().isEmpty() )
            {
                QFont msgFont = font();
                msgFont.setPointSize( d->normalFontSizePt() + 1 );
                painter.setFont( msgFont );

                if ( isDark )
                {
                    // 柔和暗影
                    painter.setPen( QColor( 0, 0, 0, 160 ) );
                    painter.drawText( rect().adjusted( 1, 1, 1, 1 ), Qt::AlignCenter | Qt::TextWordWrap, d->statusMessage );
                }

                QColor msgColor = isDark ? QColor( 255, 255, 255 ) : QColor( 28, 28, 28 );
                msgColor.setAlphaF( isDark ? 0.65 : 0.60 );
                painter.setPen( msgColor );
                painter.drawText( rect(), Qt::AlignCenter | Qt::TextWordWrap, d->statusMessage );
            }
            return;
        }

        // 确定单行展示的内容索引（未开始或负索引取第 0 行）
        int lineIdx = d->currentIndex;
        if ( lineIdx < 0 || lineIdx >= d->lines.size() )
        {
            lineIdx = 0;
        }

        const QString& text = d->lines[ lineIdx ].text;
        if ( text.trimmed().isEmpty() )
        {
            return;
        }

        QFont lineFont = font();
        lineFont.setPointSize( d->activeFontSizePt() );
        lineFont.setBold( true );
        painter.setFont( lineFont );

        const QRect r = rect();
        const int alignFlags = Qt::AlignCenter | Qt::TextWordWrap;
        const QRectF textBounds = painter.fontMetrics().boundingRect( r, alignFlags, text );

        const QColor activeColor = d->effectiveHighlightColor();

        // 0. 柔和文字立体暗影（深色模式下强化波形冲顶时的文字轮廓，浅色模式使用微弱投影）
        if ( isDark )
        {
            painter.setPen( QColor( 0, 0, 0, 160 ) );
            painter.drawText( r.adjusted( 1, 1, 1, 1 ), alignFlags, text );
        }
        else
        {
            painter.setPen( QColor( 0, 0, 0, 18 ) );
            painter.drawText( r.adjusted( 1, 1, 1, 1 ), alignFlags, text );
        }

        // 1. 底层：未唱到的暗色底字（优雅轮廓，待点亮状态：深色模式使用柔白，浅色模式使用柔黑）
        QColor dimmedColor = isDark ? QColor( 255, 255, 255 ) : QColor( 28, 28, 28 );
        dimmedColor.setAlpha( isDark ? 85 : 70 ); // 约 28%~33% 柔和底色
        painter.setPen( dimmedColor );
        painter.drawText( r, alignFlags, text );

        // 2. 顶层：跟随进度高亮覆盖（裁切保护）
        if ( !d->karaokeProgress )
        {
            // 若未开启扫光，整句全亮
            QColor fullColor = activeColor;
            fullColor.setAlpha( 255 );
            painter.setPen( fullColor );
            painter.drawText( r, alignFlags, text );
        }
        else
        {
            // 开启扫光：计算当前句进度
            qreal progress = 1.0;
            const qint64 curPos = d->currentPositionMs;
            const qint64 startMs = d->lines[ lineIdx ].timeMs;
            qint64 endMs = startMs;

            if ( lineIdx + 1 < d->lines.size() )
            {
                const qint64 nextStartMs = d->lines[ lineIdx + 1 ].timeMs;
                const qint64 textLen = qMax( 1LL, static_cast<qint64>( text.trimmed().length() ) );
                const qint64 naturalDuration = qMax( 2000LL, textLen * 300 );
                if ( nextStartMs - startMs > naturalDuration + 1500 )
                {
                    endMs = startMs + naturalDuration;
                }
                else
                {
                    endMs = nextStartMs;
                }
            }
            else
            {
                const qint64 textLen = qMax( 1LL, static_cast<qint64>( text.trimmed().length() ) );
                endMs = startMs + qMax( 3000LL, textLen * 350 );
            }

            if ( curPos <= startMs )
            {
                progress = 0.0;
            }
            else if ( curPos >= endMs )
            {
                progress = 1.0;
            }
            else
            {
                const qint64 dur = endMs - startMs;
                if ( dur > 0 )
                {
                    progress = qBound( 0.0, static_cast<qreal>( curPos - startMs ) / static_cast<qreal>( dur ), 1.0 );
                }
            }

            if ( progress > 0.001 )
            {
                painter.save();

                if ( progress < 0.999 )
                {
                    const qreal sweepWidth = textBounds.width() * progress;
                    const QRectF clipRect( textBounds.left() - 4.0, r.top() - 4.0, sweepWidth + 4.0, r.height() + 8.0 );
                    painter.setClipRect( clipRect, Qt::IntersectClip );
                }

                QColor sweepColor = activeColor;
                sweepColor.setAlpha( 255 );
                painter.setPen( sweepColor );
                painter.drawText( r, alignFlags, text );

                painter.restore();
            }
        }
        return;
    }

    // B. 多行滚动视图渲染
    // 1. 无歌词时居中显示状态提示语
    if ( d->lines.isEmpty() )
    {
        QFont msgFont = font();
        msgFont.setPointSize( d->normalFontSizePt() + 1 );
        painter.setFont( msgFont );

        QColor msgColor = palette().color( QPalette::WindowText );
        msgColor.setAlphaF( 0.45 );
        painter.setPen( msgColor );
        painter.drawText( rect(), Qt::AlignCenter, d->statusMessage );
        return;
    }

    // 2. 绘制歌词多行流
    const qreal centerY = h / 2.0;
    const qreal activeHighlightAlpha = 1.0;
    const QColor activeColor = d->effectiveHighlightColor();
    const QColor baseTextColor = palette().color( QPalette::WindowText );

    QFont activeFont = font();
    activeFont.setPointSize( d->activeFontSizePt() );
    activeFont.setBold( true );

    QFont normalFont = font();
    normalFont.setPointSize( d->normalFontSizePt() );
    normalFont.setBold( false );

    const int n = d->lines.size();
    for ( int i = 0; i < n; ++i )
    {
        if ( i >= d->lineOffsets.size() || i >= d->lineHeights.size() )
        {
            break;
        }

        const qreal lineH = d->lineHeights[ i ];
        // 当 currentScrollY == lineOffsets[i] + lineH / 2.0 时，该行正好居中在 centerY
        const qreal lineY = centerY - d->currentScrollY + d->lineOffsets[ i ];

        // 视窗外完全裁剪剔除
        if ( lineY + lineH < -20.0 || lineY > h + 20.0 )
        {
            continue;
        }

        // 视窗上下边缘柔和渐变羽化遮罩（Fade Mask）
        const qreal lineMidY = lineY + lineH / 2.0;
        qreal edgeAlpha = 1.0;
        constexpr qreal kFadeDistance = 42.0;
        if ( lineMidY < kFadeDistance )
        {
            edgeAlpha = qBound( 0.0, lineMidY / kFadeDistance, 1.0 );
        }
        else if ( lineMidY > h - kFadeDistance )
        {
            edgeAlpha = qBound( 0.0, ( h - lineMidY ) / kFadeDistance, 1.0 );
        }

        const bool isActive = ( i == d->currentIndex );
        const int dist = qAbs( i - d->currentIndex );
        const QRectF lineRect( 16.0, lineY, qMax( 40.0, static_cast<qreal>( w - 32 ) ), lineH );

        if ( isActive )
        {
            painter.setFont( activeFont );

            if ( !d->karaokeProgress )
            {
                // 未启用跟随进度高亮：经典整行点亮
                QColor fullColor = activeColor;
                fullColor.setAlphaF( qBound( 0.0, activeHighlightAlpha * edgeAlpha, 1.0 ) );
                painter.setPen( fullColor );
                painter.drawText( lineRect, Qt::AlignCenter | Qt::TextWordWrap, d->lines[ i ].text );
            }
            else
            {
                // 启用跟随进度高亮：双层动态扫光渲染（Lyricify / Apple Music 质感）
                // 1. 计算当前句的毫秒级进度 progress [0.0, 1.0]
                qreal progress = 1.0;
                const qint64 curPos = d->currentPositionMs;
                const qint64 startMs = d->lines[ i ].timeMs;
                qint64 endMs = startMs;

                if ( i + 1 < d->lines.size() )
                {
                    const qint64 nextStartMs = d->lines[ i + 1 ].timeMs;
                    const qint64 textLen = qMax( 1LL, static_cast<qint64>( d->lines[ i ].text.trimmed().length() ) );
                    const qint64 naturalDuration = qMax( 2000LL, textLen * 300 );
                    if ( nextStartMs - startMs > naturalDuration + 1500 )
                    {
                        // 句间存在较长留白间奏，在 naturalDuration 内唱完并保持全亮
                        endMs = startMs + naturalDuration;
                    }
                    else
                    {
                        endMs = nextStartMs;
                    }
                }
                else
                {
                    // 最后一句
                    const qint64 textLen = qMax( 1LL, static_cast<qint64>( d->lines[ i ].text.trimmed().length() ) );
                    endMs = startMs + qMax( 3000LL, textLen * 350 );
                }

                if ( curPos <= startMs )
                {
                    progress = 0.0;
                }
                else if ( curPos >= endMs )
                {
                    progress = 1.0;
                }
                else
                {
                    const qint64 dur = endMs - startMs;
                    if ( dur > 0 )
                    {
                        progress = qBound( 0.0, static_cast<qreal>( curPos - startMs ) / static_cast<qreal>( dur ), 1.0 );
                    }
                }

                // 2. 测量居中文字排版实际外接矩形
                const QRectF textBounds = painter.fontMetrics().boundingRect(
                    lineRect.toRect(), Qt::AlignCenter | Qt::TextWordWrap, d->lines[ i ].text );

                // 3. 第一层：未唱到的暗色底字（保持优雅清晰轮廓，待点亮状态）
                QColor dimmedColor = activeColor;
                dimmedColor.setAlphaF( qBound( 0.0, 0.35 * edgeAlpha, 1.0 ) );
                painter.setPen( dimmedColor );
                painter.drawText( lineRect, Qt::AlignCenter | Qt::TextWordWrap, d->lines[ i ].text );

                // 4. 第二层：从左往右跟随进度高亮覆盖（裁切保护）
                if ( progress > 0.001 )
                {
                    painter.save();

                    if ( progress < 0.999 )
                    {
                        const qreal sweepWidth = textBounds.width() * progress;
                        const QRectF clipRect( textBounds.left() - 4.0, lineRect.top() - 4.0, sweepWidth + 4.0, lineRect.height() + 8.0 );
                        painter.setClipRect( clipRect, Qt::IntersectClip );
                    }

                    QColor sweepColor = activeColor;
                    sweepColor.setAlphaF( qBound( 0.0, activeHighlightAlpha * edgeAlpha, 1.0 ) );
                    painter.setPen( sweepColor );
                    painter.drawText( lineRect, Qt::AlignCenter | Qt::TextWordWrap, d->lines[ i ].text );

                    painter.restore();
                }
            }
        }
        else
        {
            // 非活跃行：按与当前行的距离呈现递进式次级灰度
            painter.setFont( normalFont );
            QColor textColor = baseTextColor;

            qreal levelAlpha = 0.22;
            if ( dist == 1 )
            {
                levelAlpha = 0.58;
            }
            else if ( dist == 2 )
            {
                levelAlpha = 0.38;
            }
            textColor.setAlphaF( qBound( 0.0, levelAlpha * edgeAlpha, 1.0 ) );

            painter.setPen( textColor );
            painter.drawText( lineRect, Qt::AlignCenter | Qt::TextWordWrap, d->lines[ i ].text );
        }
    }
}

void ExLyricsWidget::wheelEvent( QWheelEvent* event )
{
    Q_D( ExLyricsWidget );
    if ( d->viewMode == ViewMode::SingleLine )
    {
        event->ignore();
        return;
    }

    if ( d->lines.isEmpty() )
    {
        QWidget::wheelEvent( event );
        return;
    }

    if ( !d->isUserScrolling )
    {
        d->isUserScrolling = true;
        emit userScrollingChanged( true );
    }
    d->scrollAnimation->stop();

    const qreal delta = -event->angleDelta().y() * 0.45;
    d->currentScrollY += delta;

    // 限制滚动边界
    const qreal maxScroll = qMax( 0.0, d->totalContentHeight );
    d->currentScrollY = qBound( 0.0, d->currentScrollY, maxScroll );

    d->userScrollResetTimer->start(); // 重新开始3秒倒计时
    update();

    event->accept();
}

void ExLyricsWidget::mousePressEvent( QMouseEvent* event )
{
    Q_D( ExLyricsWidget );
    if ( d->viewMode == ViewMode::SingleLine )
    {
        event->ignore();
        return;
    }
    QWidget::mousePressEvent( event );
}

void ExLyricsWidget::mouseDoubleClickEvent( QMouseEvent* event )
{
    Q_D( ExLyricsWidget );
    if ( d->viewMode == ViewMode::SingleLine )
    {
        event->ignore();
        return;
    }
    QWidget::mouseDoubleClickEvent( event );
}

void ExLyricsWidget::mouseReleaseEvent( QMouseEvent* event )
{
    Q_D( ExLyricsWidget );
    if ( d->viewMode == ViewMode::SingleLine )
    {
        event->ignore();
        return;
    }

    if ( d->lines.isEmpty() )
    {
        QWidget::mouseReleaseEvent( event );
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    const qreal clickY = event->position().y();
#else
    const qreal clickY = event->pos().y();
#endif

    const qreal centerY = height() / 2.0;

    for ( int i = 0; i < d->lines.size(); ++i )
    {
        if ( i >= d->lineOffsets.size() || i >= d->lineHeights.size() )
        {
            break;
        }

        const qreal lineY = centerY - d->currentScrollY + d->lineOffsets[ i ];
        const qreal lineH = d->lineHeights[ i ];

        if ( clickY >= lineY && clickY <= lineY + lineH )
        {
            emit lineClicked( i, d->lines[ i ].timeMs, d->lines[ i ].text );
            break;
        }
    }

    QWidget::mouseReleaseEvent( event );
}

void ExLyricsWidget::resizeEvent( QResizeEvent* event )
{
    QWidget::resizeEvent( event );
    Q_D( ExLyricsWidget );
    d->recalculateLayout();
    d->updateTargetScroll( false );
}

void ExLyricsWidget::changeEvent( QEvent* event )
{
    QWidget::changeEvent( event );
    if ( event->type() == QEvent::PaletteChange ||
         event->type() == QEvent::ApplicationPaletteChange ||
         event->type() == QEvent::StyleChange ||
         event->type() == QEvent::ThemeChange ||
         event->type() == QEvent::FontChange )
    {
        Q_D( ExLyricsWidget );
        d->recalculateLayout();
        d->updateTargetScroll( false );
        update();
    }
}
