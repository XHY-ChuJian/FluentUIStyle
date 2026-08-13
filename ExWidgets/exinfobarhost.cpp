#include "exinfobarhost.h"

#include <QElapsedTimer>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QTimer>
#include <QVariantAnimation>
#include <QVector>
#include <QWidget>

#include <functional>
#include <utility>

namespace
{
constexpr int PositionCount = 6;
constexpr int FlyoutShadowBorderWidth = 6;
constexpr int FlyoutCornerRadius = 4;
constexpr int ElementTransitionDuration = 300;
constexpr int ElementPositionDuration = 400;
constexpr const char* PopupSurfaceProperty = "_q_exInfoBarPopupSurface";

QPointer<QWidget> defaultInfoBarTarget;
QPointer<ExInfoBarHost> defaultInfoBarHost;

bool isTopPosition( ExInfoBarHost::Position position )
{
    return position == ExInfoBarHost::TopLeft
           || position == ExInfoBarHost::Top
           || position == ExInfoBarHost::TopRight;
}

int positionIndex( ExInfoBarHost::Position position )
{
    return qBound( 0, static_cast<int>( position ), PositionCount - 1 );
}

class InfoBarPopupSurface final : public QWidget
{
public:
    explicit InfoBarPopupSurface( QWidget* parent )
        : QWidget( parent )
        , m_animation( new QVariantAnimation( this ) )
        , m_positionAnimation( new QPropertyAnimation( this, "pos", this ) )
    {
        setAttribute( Qt::WA_StyledBackground, false );
        setAutoFillBackground( false );
        QSizePolicy policy( QSizePolicy::Expanding, QSizePolicy::Maximum );
        policy.setHeightForWidth( true );
        setSizePolicy( policy );

        m_animation->setDuration( ElementTransitionDuration );
        QEasingCurve elementEase( QEasingCurve::BezierSpline );
        elementEase.addCubicBezierSegment( QPointF( 0.25, 0.1 ),
                                           QPointF( 0.25, 1.0 ),
                                           QPointF( 1.0, 1.0 ) );
        m_animation->setEasingCurve( elementEase );
        m_positionAnimation->setDuration( ElementPositionDuration );
        m_positionAnimation->setEasingCurve( elementEase );
        connect( m_animation, &QVariantAnimation::valueChanged, this, [this]( const QVariant& value )
        {
            m_progress = value.toReal();
            if ( m_opacityEffect )
            {
                m_opacityEffect->setOpacity( m_leaving ? m_progress : 1.0 );
            }
            updateInfoBarGeometry();
            update();
        } );
        connect( m_animation, &QVariantAnimation::finished, this, [this]
        {
            if ( m_leaving && m_leaveFinished )
            {
                const auto callback = std::move( m_leaveFinished );
                callback();
            }
        } );
    }

    void moveTo( const QPoint& target, bool animate, int duration )
    {
        if ( m_positionAnimation->state() == QAbstractAnimation::Running )
        {
            // LayoutRequest/Show 等事件可能在动画期间再次请求同一个目标位置。
            // 保留当前动画，避免一次无动画的重排把 Element 补位效果截断。
            if ( m_positionAnimation->endValue().toPoint() == target )
            {
                return;
            }
            m_positionAnimation->stop();
        }
        if ( pos() == target )
        {
            return;
        }
        if ( !animate || !isVisible() )
        {
            move( target );
            return;
        }
        m_positionAnimation->setStartValue( pos() );
        m_positionAnimation->setEndValue( target );
        m_positionAnimation->setDuration( duration );
        m_positionAnimation->start();
    }

    void setInfoBar( ExInfoBar* infoBar )
    {
        if ( !infoBar )
        {
            return;
        }
        m_infoBar = infoBar;
        infoBar->setParent( this );
        infoBar->installEventFilter( this );
        m_opacityEffect = new QGraphicsOpacityEffect( infoBar );
        m_opacityEffect->setOpacity( 1.0 );
        infoBar->setGraphicsEffect( m_opacityEffect );
        updateGeometry();
        updateInfoBarGeometry();
    }

    void startEnter( ExInfoBarHost::Position position )
    {
        m_animation->stop();
        m_leaving = false;
        m_leaveOffset = QPointF();
        m_leaveFinished = {};
        if ( position == ExInfoBarHost::TopLeft || position == ExInfoBarHost::BottomLeft )
        {
            m_enterDirection = QPointF( -1.0, 0.0 );
        }
        else if ( position == ExInfoBarHost::TopRight || position == ExInfoBarHost::BottomRight )
        {
            m_enterDirection = QPointF( 1.0, 0.0 );
        }
        else
        {
            m_enterDirection = QPointF( 0.0, position == ExInfoBarHost::Top ? -1.0 : 1.0 );
        }
        m_progress = 0.0;
        if ( m_opacityEffect )
        {
            m_opacityEffect->setOpacity( 1.0 );
        }
        updateInfoBarGeometry();
        update();
        m_animation->setStartValue( 0.0 );
        m_animation->setEndValue( 1.0 );
        m_animation->start();
    }

    void startLeave( std::function<void()> finished )
    {
        if ( m_leaving )
        {
            return;
        }
        const QRectF panelRect = QRectF( rect() ).adjusted( FlyoutShadowBorderWidth,
                                                            FlyoutShadowBorderWidth,
                                                            -FlyoutShadowBorderWidth,
                                                            -FlyoutShadowBorderWidth );
        // 如果进入动画尚未结束，从当前屏幕位置直接淡出，不能先跳到
        // 最终位置再消失。
        const QPointF currentVisualOffset = currentOffset( panelRect );
        m_animation->stop();
        m_leaving = true;
        m_enterDirection = QPointF();
        m_leaveOffset = currentVisualOffset;
        m_leaveFinished = std::move( finished );
        m_progress = 1.0;
        if ( m_opacityEffect )
        {
            m_opacityEffect->setOpacity( 1.0 );
        }
        updateInfoBarGeometry();
        update();
        m_animation->setStartValue( 1.0 );
        m_animation->setEndValue( 0.0 );
        m_animation->start();
    }

    void cancelLeave()
    {
        if ( !m_leaving )
        {
            return;
        }
        m_animation->stop();
        m_leaving = false;
        m_leaveOffset = QPointF();
        m_leaveFinished = {};
        m_progress = 1.0;
        if ( m_opacityEffect )
        {
            m_opacityEffect->setOpacity( 1.0 );
        }
        updateInfoBarGeometry();
        update();
    }

    void deactivateForQueue()
    {
        m_animation->stop();
        m_positionAnimation->stop();
        m_leaving = false;
        m_leaveOffset = QPointF();
        m_leaveFinished = {};
        hide();
    }

    QSize sizeHint() const override
    {
        if ( !m_infoBar )
        {
            return QWidget::sizeHint();
        }
        return m_infoBar->sizeHint() + QSize( FlyoutShadowBorderWidth * 2,
                                              FlyoutShadowBorderWidth * 2 );
    }

    QSize minimumSizeHint() const override
    {
        if ( !m_infoBar )
        {
            return QWidget::minimumSizeHint();
        }
        return m_infoBar->minimumSizeHint() + QSize( FlyoutShadowBorderWidth * 2,
                                                     FlyoutShadowBorderWidth * 2 );
    }

    bool hasHeightForWidth() const override
    {
        return true;
    }

    int heightForWidth( int width ) const override
    {
        if ( !m_infoBar )
        {
            return sizeHint().height();
        }
        const int contentWidth = qMax( 0, width - FlyoutShadowBorderWidth * 2 );
        const int contentHeight = m_infoBar->hasHeightForWidth()
                                      ? m_infoBar->heightForWidth( contentWidth )
                                      : m_infoBar->sizeHint().height();
        return contentHeight + FlyoutShadowBorderWidth * 2;
    }

protected:
    bool eventFilter( QObject* watched, QEvent* event ) override
    {
        if ( watched == m_infoBar
             && ( event->type() == QEvent::LayoutRequest || event->type() == QEvent::Show
                  || event->type() == QEvent::Hide ) )
        {
            updateGeometry();
            updateInfoBarGeometry();
        }
        return QWidget::eventFilter( watched, event );
    }

    void resizeEvent( QResizeEvent* event ) override
    {
        QWidget::resizeEvent( event );
        updateInfoBarGeometry();
    }

    void paintEvent( QPaintEvent* ) override
    {
        const QRectF panelRect = QRectF( rect() ).adjusted( FlyoutShadowBorderWidth,
                                                            FlyoutShadowBorderWidth,
                                                            -FlyoutShadowBorderWidth,
                                                            -FlyoutShadowBorderWidth );
        if ( !panelRect.isValid() )
        {
            return;
        }

        QPainter painter( this );
        painter.setRenderHint( QPainter::Antialiasing );
        painter.setPen( Qt::NoPen );
        painter.setOpacity( m_leaving ? m_progress : 1.0 );
        painter.translate( currentOffset( panelRect ) );

        // 与 FluentUI3Style::drawPopupShadow() 使用相同的 FluShadow 参数。
        constexpr int elevation = 5;
        constexpr int visibleLevels = elevation - 1;
        const bool dark = palette().color( QPalette::Window ).lightness() < 128;
        const QColor baseColor = dark ? QColor( 0, 0, 0 ) : QColor( 0x99, 0x99, 0x99 );
        for ( int level = 1; level <= visibleLevels; ++level )
        {
            const qreal extent = qreal( FlyoutShadowBorderWidth ) * level / visibleLevels;
            const QRectF outerRect = panelRect.adjusted( -extent, -extent, extent, extent );
            QPainterPath ringPath;
            ringPath.setFillRule( Qt::OddEvenFill );
            ringPath.addRoundedRect( outerRect,
                                     FlyoutCornerRadius + extent,
                                     FlyoutCornerRadius + extent );
            ringPath.addRoundedRect( panelRect, FlyoutCornerRadius, FlyoutCornerRadius );
            const qreal transitionOpacity = m_leaving ? m_progress : 1.0;
            painter.setOpacity( transitionOpacity * 0.01 * ( elevation - level + 1 ) );
            painter.setBrush( baseColor );
            painter.drawPath( ringPath );
        }
    }

private:
    QPointF currentOffset( const QRectF& panelRect ) const
    {
        if ( m_leaving )
        {
            // Element Plus 的 leave-to 只有 opacity: 0，不反向滑走。
            return m_leaveOffset;
        }
        return QPointF( m_enterDirection.x() * panelRect.width() * ( 1.0 - m_progress ),
                        m_enterDirection.y() * panelRect.height() * ( 1.0 - m_progress ) );
    }

    void updateInfoBarGeometry()
    {
        if ( !m_infoBar )
        {
            return;
        }
        const QRectF panelRect = QRectF( rect() ).adjusted( FlyoutShadowBorderWidth,
                                                            FlyoutShadowBorderWidth,
                                                            -FlyoutShadowBorderWidth,
                                                            -FlyoutShadowBorderWidth );
        const QPoint offset = currentOffset( panelRect ).toPoint();
        m_infoBar->setGeometry( panelRect.toRect().translated( offset ) );
    }

    QPointer<ExInfoBar> m_infoBar;
    QGraphicsOpacityEffect* m_opacityEffect = nullptr;
    QVariantAnimation* m_animation = nullptr;
    QPropertyAnimation* m_positionAnimation = nullptr;
    QPointF m_enterDirection;
    QPointF m_leaveOffset;
    std::function<void()> m_leaveFinished;
    qreal m_progress = 0.0;
    bool m_leaving = false;
};
} // namespace

class ExInfoBarHost::Private
{
public:
    struct Entry
    {
        QPointer<ExInfoBar> infoBar;
        QPointer<InfoBarPopupSurface> surface;
        Position position = TopRight;
        QTimer* timer = nullptr;
        QElapsedTimer elapsed;
        int timeout = 0;
        int remaining = 0;
        bool active = false;
        bool closing = false;
    };

    Private( ExInfoBarHost* host, QWidget* targetWidget )
        : q( host )
        , target( targetWidget )
    {
    }

    Entry* findEntry( ExInfoBar* infoBar ) const
    {
        for ( Entry* entry : entries )
        {
            if ( entry->infoBar == infoBar )
            {
                return entry;
            }
        }
        return nullptr;
    }

    void startTimer( Entry* entry, int timeout )
    {
        if ( !entry || !entry->infoBar || timeout <= 0 )
        {
            return;
        }
        if ( entry->timer )
        {
            resumeTimer( entry );
            return;
        }
        entry->remaining = timeout;
        entry->timer = new QTimer( q );
        entry->timer->setSingleShot( true );
        QObject::connect( entry->timer, &QTimer::timeout, q, [entry]
        {
            if ( entry->infoBar )
            {
                entry->infoBar->dismiss();
            }
        } );
        entry->elapsed.start();
        entry->timer->start( entry->remaining );
    }

    void pauseTimer( Entry* entry )
    {
        if ( !entry || !entry->timer || !entry->timer->isActive() )
        {
            return;
        }
        entry->remaining = qMax( 0, entry->remaining - static_cast<int>( entry->elapsed.elapsed() ) );
        entry->timer->stop();
    }

    void resumeTimer( Entry* entry )
    {
        if ( !entry || !entry->timer || entry->closing || !entry->infoBar )
        {
            return;
        }
        if ( entry->remaining <= 0 )
        {
            entry->infoBar->dismiss();
            return;
        }
        entry->elapsed.restart();
        entry->timer->start( entry->remaining );
    }

    void removeEntry( Entry* entry )
    {
        if ( !entry )
        {
            return;
        }
        ExInfoBar* infoBar = entry->infoBar.data();
        const Position position = entry->position;
        if ( entry->timer )
        {
            entry->timer->stop();
            QObject::disconnect( entry->timer, nullptr, q, nullptr );
            entry->timer->deleteLater();
        }
        if ( infoBar )
        {
            infoBar->removeEventFilter( q );
            QObject::disconnect( infoBar, nullptr, q, nullptr );
        }
        entries.removeOne( entry );
        emit q->infoBarClosed( infoBar, position );
        if ( entry->surface )
        {
            entry->surface->deleteLater();
        }
        else if ( infoBar )
        {
            infoBar->deleteLater();
        }
        delete entry;
        scheduleReposition( true );
    }

    int surfaceWidth() const
    {
        if ( !target )
        {
            return 0;
        }
        return qMin( maximumWidth, qMax( 0, target->width() - margin * 2 ) );
    }

    int surfaceHeight( const Entry* entry, int width ) const
    {
        if ( !entry || !entry->surface || width <= 0 )
        {
            return 0;
        }
        const int height = entry->surface->hasHeightForWidth()
                               ? entry->surface->heightForWidth( width )
                               : entry->surface->sizeHint().height();
        return qMax( entry->surface->minimumSizeHint().height(), height );
    }

    QVector<Entry*> activeEntries( Position position ) const
    {
        QVector<Entry*> result;
        for ( Entry* entry : entries )
        {
            if ( entry->position == position && entry->active && !entry->closing
                 && entry->infoBar && entry->surface )
            {
                result.append( entry );
            }
        }
        return result;
    }

    int horizontalPosition( Position position, int width ) const
    {
        if ( position == Top || position == Bottom )
        {
            return ( target->width() - width ) / 2;
        }
        if ( position == TopRight || position == BottomRight )
        {
            return target->width() - margin - width;
        }
        return margin;
    }

    void fitActiveEntries( Position position )
    {
        if ( !target )
        {
            return;
        }
        const int width = surfaceWidth();
        const int availableHeight = qMax( 0, target->height() - margin * 2 );
        int usedHeight = 0;
        int count = 0;
        bool blocked = width <= 0 || availableHeight <= 0;

        // 保留最早且能够完整放下的通知。一旦某项放不下，同一锚点的
        // 后续项全部返回等待状态，以维持严格 FIFO。
        for ( Entry* entry : entries )
        {
            if ( entry->position != position || !entry->active || entry->closing
                 || !entry->infoBar || !entry->surface )
            {
                continue;
            }
            const int height = surfaceHeight( entry, width );
            const int required = ( count > 0 ? spacing : 0 ) + height;
            if ( blocked || usedHeight + required > availableHeight )
            {
                blocked = true;
                entry->active = false;
                pauseTimer( entry );
                entry->surface->deactivateForQueue();
                continue;
            }
            usedHeight += required;
            ++count;
        }
    }

    void repositionPosition( Position position, bool animate )
    {
        if ( !target )
        {
            return;
        }
        const int width = surfaceWidth();
        const auto active = activeEntries( position );
        if ( width <= 0 )
        {
            for ( Entry* entry : active )
            {
                entry->surface->hide();
            }
            return;
        }

        const int x = qMax( 0, horizontalPosition( position, width ) );
        int y = isTopPosition( position ) ? margin : target->height() - margin;
        for ( Entry* entry : active )
        {
            const int height = surfaceHeight( entry, width );
            entry->surface->resize( width, height );
            if ( isTopPosition( position ) )
            {
                entry->surface->moveTo( QPoint( x, y ), animate,
                                        ElementPositionDuration );
                y += height + spacing;
            }
            else
            {
                y -= height;
                entry->surface->moveTo( QPoint( x, y ), animate,
                                        ElementTransitionDuration );
                y -= spacing;
            }
        }
    }

    bool activatePending( Position position, bool animateExisting )
    {
        if ( !target )
        {
            return false;
        }
        const int width = surfaceWidth();
        const int availableHeight = qMax( 0, target->height() - margin * 2 );
        if ( width <= 0 || availableHeight <= 0 )
        {
            return false;
        }

        // 淡出期间只允许已有通知补位。等待项必须等 closed/removeEntry
        // 完成后再进入，避免窗口缩放触发重排时与旧通知重叠显示。
        for ( Entry* entry : entries )
        {
            if ( entry->position == position && entry->closing
                 && entry->infoBar && entry->surface )
            {
                return false;
            }
        }

        int count = 0;
        int usedHeight = 0;
        for ( Entry* entry : entries )
        {
            if ( entry->position != position || !entry->active || entry->closing
                 || !entry->infoBar || !entry->surface )
            {
                continue;
            }
            usedHeight += ( count > 0 ? spacing : 0 ) + surfaceHeight( entry, width );
            ++count;
        }

        QVector<Entry*> activated;
        for ( Entry* entry : entries )
        {
            if ( entry->position != position || entry->active || entry->closing
                 || !entry->infoBar || !entry->surface )
            {
                continue;
            }
            const int height = surfaceHeight( entry, width );
            const int required = ( count > 0 ? spacing : 0 ) + height;
            if ( usedHeight + required > availableHeight )
            {
                // 同一锚点严格保持 FIFO；当前项放不下时，后续项继续等待。
                break;
            }
            entry->active = true;
            activated.append( entry );
            usedHeight += required;
            ++count;
        }

        if ( activated.isEmpty() )
        {
            return false;
        }

        // 隐藏的新项会直接放到目标位置；已有项在腾位时使用 Element 的位置过渡。
        repositionPosition( position, animateExisting );
        for ( Entry* entry : activated )
        {
            if ( !entry->infoBar || !entry->surface )
            {
                continue;
            }
            entry->infoBar->setOpen( true );
            if ( !entries.contains( entry ) || !entry->infoBar
                 || !entry->surface || !entry->infoBar->isOpen() )
            {
                continue;
            }
            entry->surface->startEnter( position );
            entry->surface->show();
            entry->surface->raise();
            startTimer( entry, entry->timeout );
            emit q->infoBarShown( entry->infoBar, position );
        }
        return true;
    }

    void scheduleReposition( bool animate = false )
    {
        animateReposition = animateReposition || animate;
        if ( repositionScheduled )
        {
            return;
        }
        repositionScheduled = true;
        QTimer::singleShot( 0, q, [this]
        {
            repositionScheduled = false;
            const bool animate = animateReposition;
            animateReposition = false;
            repositionAll( animate );
        } );
    }

    void repositionAll( bool animate = false )
    {
        if ( repositioning || !target )
        {
            return;
        }
        repositioning = true;
        for ( int index = 0; index < PositionCount; ++index )
        {
            const Position position = static_cast<Position>( index );
            fitActiveEntries( position );
            if ( !activatePending( position, animate ) )
            {
                repositionPosition( position, animate );
            }
        }
        repositioning = false;
    }

    ExInfoBarHost* q = nullptr;
    QPointer<QWidget> target;
    QVector<Entry*> entries;
    int margin = 24;
    int spacing = 8;
    int maximumWidth = 360;
    int defaultTimeout = 4500;
    bool repositionScheduled = false;
    bool animateReposition = false;
    bool repositioning = false;
};

ExInfoBarHost::ExInfoBarHost( QWidget* target, QObject* parent )
    : QObject( parent ? parent : target )
    , d( new Private( this, target ) )
{
    if ( target )
    {
        target->installEventFilter( this );
    }
}

ExInfoBarHost::~ExInfoBarHost()
{
    if ( d->target )
    {
        d->target->removeEventFilter( this );
    }
    const auto entries = d->entries;
    for ( Private::Entry* entry : entries )
    {
        if ( entry->infoBar )
        {
            disconnect( entry->infoBar, nullptr, this, nullptr );
            entry->infoBar->removeEventFilter( this );
        }
        if ( entry->timer )
        {
            entry->timer->stop();
        }
    }
    for ( Private::Entry* entry : entries )
    {
        delete entry->surface;
        delete entry;
    }
    delete d;
}

void ExInfoBarHost::setDefaultTarget( QWidget* target )
{
    if ( defaultInfoBarTarget == target )
    {
        return;
    }

    // 切换默认窗口时，旧 Host 不再属于全局入口。立即释放可同时停止
    // 其计时器和未完成动画；QPointer 会处理窗口先行销毁的情况。
    ExInfoBarHost* oldHost = defaultInfoBarHost.data();
    defaultInfoBarHost = nullptr;
    defaultInfoBarTarget = target;
    delete oldHost;
}

QWidget* ExInfoBarHost::defaultTarget()
{
    return defaultInfoBarTarget.data();
}

ExInfoBarHost* ExInfoBarHost::defaultHost()
{
    QWidget* target = defaultInfoBarTarget.data();
    if ( !target )
    {
        return nullptr;
    }
    if ( !defaultInfoBarHost || defaultInfoBarHost->target() != target )
    {
        defaultInfoBarHost = new ExInfoBarHost( target, target );
    }
    return defaultInfoBarHost.data();
}

QWidget* ExInfoBarHost::target() const
{
    return d->target.data();
}

int ExInfoBarHost::margin() const
{
    return d->margin;
}

void ExInfoBarHost::setMargin( int margin )
{
    margin = qMax( 0, margin );
    if ( d->margin == margin )
    {
        return;
    }
    d->margin = margin;
    d->scheduleReposition();
}

int ExInfoBarHost::spacing() const
{
    return d->spacing;
}

void ExInfoBarHost::setSpacing( int spacing )
{
    spacing = qMax( 0, spacing );
    if ( d->spacing == spacing )
    {
        return;
    }
    d->spacing = spacing;
    d->scheduleReposition();
}

int ExInfoBarHost::maximumWidth() const
{
    return d->maximumWidth;
}

void ExInfoBarHost::setMaximumWidth( int width )
{
    width = qMax( 160, width );
    if ( d->maximumWidth == width )
    {
        return;
    }
    d->maximumWidth = width;
    d->scheduleReposition();
}

int ExInfoBarHost::defaultTimeout() const
{
    return d->defaultTimeout;
}

void ExInfoBarHost::setDefaultTimeout( int milliseconds )
{
    d->defaultTimeout = qMax( 0, milliseconds );
}

ExInfoBar* ExInfoBarHost::showInfoBar( ExInfoBar::Severity severity,
                                       const QString& title,
                                       const QString& message,
                                       Position position,
                                       int timeout )
{
    if ( !d->target )
    {
        return nullptr;
    }
    auto* infoBar = new ExInfoBar;
    infoBar->setSeverity( severity );
    infoBar->setTitle( title );
    infoBar->setMessage( message );
    addInfoBar( infoBar, position, timeout );
    return infoBar;
}

void ExInfoBarHost::addInfoBar( ExInfoBar* infoBar, Position position, int timeout )
{
    if ( !infoBar || !d->target || infoBar == d->target
         || infoBar->isAncestorOf( d->target ) || d->findEntry( infoBar ) )
    {
        return;
    }
    position = static_cast<Position>( positionIndex( position ) );

    auto* entry = new Private::Entry;
    entry->infoBar = infoBar;
    entry->position = position;
    entry->timeout = timeout < 0 ? d->defaultTimeout : qMax( 0, timeout );
    d->entries.append( entry );

    auto* surface = new InfoBarPopupSurface( d->target );
    entry->surface = surface;
    infoBar->setParent( surface );
    infoBar->setProperty( PopupSurfaceProperty, true );
    infoBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Maximum );
    infoBar->installEventFilter( this );
    surface->setInfoBar( infoBar );
    surface->hide();

    connect( infoBar, &ExInfoBar::openChanged, this, [this, entry]( bool open )
    {
        if ( open )
        {
            if ( entry->closing )
            {
                entry->closing = false;
                if ( entry->surface )
                {
                    entry->surface->cancelLeave();
                }
                d->resumeTimer( entry );
                d->repositionPosition( entry->position, true );
                d->scheduleReposition( true );
            }
            return;
        }

        const Position closingPosition = entry->position;
        entry->closing = true;
        d->pauseTimer( entry );
        const QPointer<ExInfoBar> guardedInfoBar = entry->infoBar;
        if ( entry->surface )
        {
            entry->surface->startLeave( [guardedInfoBar]
            {
                if ( guardedInfoBar )
                {
                    guardedInfoBar->finishPopupClose();
                }
            } );
        }
        else if ( guardedInfoBar )
        {
            guardedInfoBar->finishPopupClose();
        }

        // Element 在当前项淡出的同时就更新其余实例的 top/bottom，
        // 因而后续项会并行滑动补位；等待队列仍在淡出完成后才补入。
        d->repositionPosition( closingPosition, true );
    } );
    connect( infoBar, &ExInfoBar::closed, this, [this, entry]
    {
        d->removeEntry( entry );
    } );
    connect( infoBar, &QObject::destroyed, this, [this, entry]
    {
        if ( d->entries.contains( entry ) )
        {
            entry->infoBar = nullptr;
            d->removeEntry( entry );
        }
    } );

    d->repositionAll();
}

void ExInfoBarHost::dismissAll()
{
    const auto entries = d->entries;
    for ( Private::Entry* entry : entries )
    {
        if ( entry->infoBar && !entry->closing )
        {
            entry->closing = true;
            if ( entry->infoBar->isOpen() )
            {
                entry->infoBar->dismiss();
            }
            else
            {
                entry->infoBar->finishPopupClose();
            }
        }
    }
}

void ExInfoBarHost::dismissAll( Position position )
{
    const auto entries = d->entries;
    for ( Private::Entry* entry : entries )
    {
        if ( entry->position == position && entry->infoBar && !entry->closing )
        {
            entry->closing = true;
            if ( entry->infoBar->isOpen() )
            {
                entry->infoBar->dismiss();
            }
            else
            {
                entry->infoBar->finishPopupClose();
            }
        }
    }
}

bool ExInfoBarHost::eventFilter( QObject* watched, QEvent* event )
{
    if ( watched == d->target )
    {
        if ( event->type() == QEvent::Resize || event->type() == QEvent::Move
             || event->type() == QEvent::Show || event->type() == QEvent::LayoutRequest
             || event->type() == QEvent::WindowStateChange )
        {
            d->scheduleReposition();
        }
    }
    else if ( auto* infoBar = qobject_cast<ExInfoBar*>( watched ) )
    {
        Private::Entry* entry = d->findEntry( infoBar );
        if ( event->type() == QEvent::Enter )
        {
            d->pauseTimer( entry );
        }
        else if ( event->type() == QEvent::Leave )
        {
            d->resumeTimer( entry );
        }
        else if ( event->type() == QEvent::Resize || event->type() == QEvent::Show
                  || event->type() == QEvent::Hide || event->type() == QEvent::LayoutRequest )
        {
            if ( !d->repositioning )
            {
                d->scheduleReposition();
            }
        }
    }
    else if ( event->type() == QEvent::Resize || event->type() == QEvent::Show
              || event->type() == QEvent::Hide || event->type() == QEvent::LayoutRequest )
    {
        d->scheduleReposition();
    }
    return QObject::eventFilter( watched, event );
}
