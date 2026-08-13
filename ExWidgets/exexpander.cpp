#include "exexpander.h"

#include "fluentui3colors.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEasingCurve>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleOptionFocusRect>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace
{
constexpr int CornerRadius = 4;
constexpr int ExpanderMinWidth = 96;
constexpr int HeaderMinHeight = 48;
constexpr int HeaderContentPadding = 16;
constexpr int ContentPadding = 16;
constexpr int ChevronContentSpacing = 20;
constexpr int ChevronTrailingMargin = 8;
constexpr int ChevronButtonSize = 32;

bool isDarkPalette( const QPalette& palette )
{
    if ( qApp )
    {
        const QVariant colorScheme = qApp->property( "_q_colorscheme" );
        if ( colorScheme.isValid() )
        {
            return colorScheme.toInt() == 1;
        }
    }
    return palette.color( QPalette::Window ).lightness() < 128;
}

QColor cardBackground( const QPalette& palette )
{
    const bool dark = isDarkPalette( palette );
    const bool wallpaperMode = qApp && qApp->property( "_q_widget_mode" ).toInt() >= 1;
    return winUI3CardBackgroundColor( palette, dark, wallpaperMode );
}

QColor cardBorder( const QPalette& palette, bool )
{
    return WINUI3Colors[ isDarkPalette( palette ) ? 1 : 0 ][ cardStrokeColorBalanced ];
}

QPainterPath roundedPanelPath( const QRectF& rect,
                               bool roundTopLeft,
                               bool roundTopRight,
                               bool roundBottomRight,
                               bool roundBottomLeft )
{
    const qreal radius = qMin<qreal>( CornerRadius, qMin( rect.width(), rect.height() ) * 0.5 );
    QPainterPath path;
    path.moveTo( rect.left() + ( roundTopLeft ? radius : 0.0 ), rect.top() );
    path.lineTo( rect.right() - ( roundTopRight ? radius : 0.0 ), rect.top() );
    if ( roundTopRight )
    {
        path.quadTo( rect.right(), rect.top(), rect.right(), rect.top() + radius );
    }
    else
    {
        path.lineTo( rect.right(), rect.top() );
    }
    path.lineTo( rect.right(), rect.bottom() - ( roundBottomRight ? radius : 0.0 ) );
    if ( roundBottomRight )
    {
        path.quadTo( rect.right(), rect.bottom(), rect.right() - radius, rect.bottom() );
    }
    else
    {
        path.lineTo( rect.right(), rect.bottom() );
    }
    path.lineTo( rect.left() + ( roundBottomLeft ? radius : 0.0 ), rect.bottom() );
    if ( roundBottomLeft )
    {
        path.quadTo( rect.left(), rect.bottom(), rect.left(), rect.bottom() - radius );
    }
    else
    {
        path.lineTo( rect.left(), rect.bottom() );
    }
    path.lineTo( rect.left(), rect.top() + ( roundTopLeft ? radius : 0.0 ) );
    if ( roundTopLeft )
    {
        path.quadTo( rect.left(), rect.top(), rect.left() + radius, rect.top() );
    }
    else
    {
        path.lineTo( rect.left(), rect.top() );
    }
    path.closeSubpath();
    return path;
}
} // namespace

class ExExpander::ContentPanel final : public QWidget
{
public:
    ContentPanel( ExExpander* expander, QWidget* content )
        : QWidget( expander )
        , m_expander( expander )
    {
        setMinimumHeight( HeaderMinHeight );
        setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );
        auto* layout = new QVBoxLayout( this );
        layout->setContentsMargins( ContentPadding, ContentPadding,
                                    ContentPadding, ContentPadding );
        layout->setSpacing( 0 );
        content->setParent( this );
        layout->addWidget( content );
        content->show();
    }

    void setOuterEdge( bool outerEdge )
    {
        if ( m_outerEdge == outerEdge )
        {
            return;
        }
        m_outerEdge = outerEdge;
        update();
    }

protected:
    void paintEvent( QPaintEvent* ) override
    {
        QPainter painter( this );
        painter.setRenderHint( QPainter::Antialiasing );

        const bool down = m_expander->expandDirection() == ExExpander::Down;
        const QRectF bounds = QRectF( rect() ).adjusted( 0.5, 0.5, -0.5, -0.5 );
        // 所有 Content 与 Header 组成一个连续容器。只有距离 Header
        // 最远的一项保留外侧圆角，其余连接边均为直角。
        const QPainterPath fillPath = roundedPanelPath(
            bounds,
            !down && m_outerEdge,
            !down && m_outerEdge,
            down && m_outerEdge,
            down && m_outerEdge );
        painter.fillPath( fillPath, cardBackground( palette() ) );

        // 每项只绘制左右边和远离 Header 的横边。该横边同时作为下一项
        // 的分隔线；靠近 Header 的边由 Header 或相邻 Content 绘制。
        QPainterPath borderPath;
        if ( down )
        {
            borderPath.moveTo( bounds.left(), bounds.top() );
            borderPath.lineTo( bounds.left(), bounds.bottom() - ( m_outerEdge ? CornerRadius : 0 ) );
            if ( m_outerEdge )
            {
                borderPath.quadTo( bounds.left(), bounds.bottom(),
                                   bounds.left() + CornerRadius, bounds.bottom() );
            }
            else
            {
                borderPath.lineTo( bounds.left(), bounds.bottom() );
            }
            borderPath.lineTo( bounds.right() - ( m_outerEdge ? CornerRadius : 0 ), bounds.bottom() );
            if ( m_outerEdge )
            {
                borderPath.quadTo( bounds.right(), bounds.bottom(),
                                   bounds.right(), bounds.bottom() - CornerRadius );
            }
            else
            {
                borderPath.lineTo( bounds.right(), bounds.bottom() );
            }
            borderPath.lineTo( bounds.right(), bounds.top() );
        }
        else
        {
            borderPath.moveTo( bounds.left(), bounds.bottom() );
            borderPath.lineTo( bounds.left(), bounds.top() + ( m_outerEdge ? CornerRadius : 0 ) );
            if ( m_outerEdge )
            {
                borderPath.quadTo( bounds.left(), bounds.top(),
                                   bounds.left() + CornerRadius, bounds.top() );
            }
            else
            {
                borderPath.lineTo( bounds.left(), bounds.top() );
            }
            borderPath.lineTo( bounds.right() - ( m_outerEdge ? CornerRadius : 0 ), bounds.top() );
            if ( m_outerEdge )
            {
                borderPath.quadTo( bounds.right(), bounds.top(),
                                   bounds.right(), bounds.top() + CornerRadius );
            }
            else
            {
                borderPath.lineTo( bounds.right(), bounds.top() );
            }
            borderPath.lineTo( bounds.right(), bounds.bottom() );
        }
        painter.setBrush( Qt::NoBrush );
        painter.setPen( QPen( cardBorder( palette(), isEnabled() ), 1.0 ) );
        painter.drawPath( borderPath );
    }

private:
    ExExpander* m_expander = nullptr;
    bool m_outerEdge = false;
};

class ExExpander::ContentStack final : public QWidget
{
public:
    explicit ContentStack( ExExpander* expander )
        : QWidget( expander )
    {
        setMinimumWidth( ExpanderMinWidth );
        setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );
    }
};

class ExExpander::ContentViewport final : public QWidget
{
public:
    ContentViewport( ExExpander* expander, ContentStack* panel )
        : QWidget( expander )
        , m_expander( expander )
        , m_panel( panel )
    {
        setMinimumWidth( ExpanderMinWidth );
        setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );
        panel->setParent( this );
        panel->installEventFilter( this );
        panel->show();
    }

    void prepareAnimation()
    {
        m_fullHeight = fullPanelHeight();
        updatePanelGeometry();
    }

    void setViewportProgress( qreal progress )
    {
        m_viewportProgress = qBound( 0.0, progress, 1.0 );
        updatePanelGeometry();

        // Viewport 的建议高度发生变化后，将几何变化继续传递给
        // ExExpander 及其外层布局，使相邻控件逐帧补位。
        updateGeometry();
        if ( m_expander->layout() )
        {
            m_expander->layout()->invalidate();
        }
        m_expander->updateGeometry();
    }

    QSize sizeHint() const override
    {
        QSize result = m_panel->sizeHint().expandedTo( QSize( ExpanderMinWidth, 0 ) );
        result.setHeight( qRound( m_fullHeight * m_viewportProgress ) );
        return result;
    }

    QSize minimumSizeHint() const override
    {
        // 父容器空间不足时允许裁剪；正常布局高度由完整 Content 决定。
        return QSize( ExpanderMinWidth, 0 );
    }

protected:
    void resizeEvent( QResizeEvent* event ) override
    {
        QWidget::resizeEvent( event );
        // Viewport 隐藏期间不会跟随父控件调整宽度。重新参与布局后，
        // 必须按当前宽度刷新 heightForWidth，不能沿用收起前的高度。
        m_fullHeight = fullPanelHeight();
        updatePanelGeometry();
        updateGeometry();
    }

    bool eventFilter( QObject* watched, QEvent* event ) override
    {
        if ( watched == m_panel
             && ( event->type() == QEvent::LayoutRequest || event->type() == QEvent::Show
                  || event->type() == QEvent::Hide ) )
        {
            m_fullHeight = fullPanelHeight();
            updatePanelGeometry();
            updateGeometry();
        }
        return QWidget::eventFilter( watched, event );
    }

private:
    int fullPanelHeight() const
    {
        const int panelWidth = qMax( 0, width() );
        int result = m_panel->sizeHint().height();
        if ( m_panel->hasHeightForWidth() && panelWidth > 0 )
        {
            const int heightForWidth = m_panel->heightForWidth( panelWidth );
            if ( heightForWidth >= 0 )
            {
                result = heightForWidth;
            }
        }
        return qMax( 0, result );
    }

    void updatePanelGeometry()
    {
        if ( m_fullHeight <= 0 )
        {
            m_fullHeight = fullPanelHeight();
        }
        const int visibleHeight = qRound( m_fullHeight * m_viewportProgress );
        if ( maximumHeight() != visibleHeight )
        {
            setMaximumHeight( visibleHeight );
        }

        // Panel 本身不参与位移动画；Viewport 只改变裁剪高度。
        // Up 模式从 Header 一侧向上裁剪，因此锚定到底部。
        const int panelY = m_expander->expandDirection() == ExExpander::Up
                               ? height() - m_fullHeight
                               : 0;
        m_panel->setGeometry( 0, panelY, width(), m_fullHeight );
    }

    ExExpander* m_expander = nullptr;
    ContentStack* m_panel = nullptr;
    int m_fullHeight = 0;
    qreal m_viewportProgress = 0.0;
};

class ExExpander::HeaderButton final : public QAbstractButton
{
public:
    explicit HeaderButton( ExExpander* expander )
        : QAbstractButton( expander )
        , m_expander( expander )
    {
        setCheckable( true );
        setFocusPolicy( Qt::StrongFocus );
        setMinimumSize( ExpanderMinWidth, HeaderMinHeight );
        // Header 只采用自身内容高度，不参与 Content 的展开/收起动画。
        setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
        setAttribute( Qt::WA_Hover );

        m_layout = new QHBoxLayout( this );
        m_layout->setSpacing( 0 );
        updateLayoutMargins();

        m_label = new QLabel( this );
        m_label->setWordWrap( true );
        m_label->setAttribute( Qt::WA_TransparentForMouseEvents );
        m_layout->addWidget( m_label, 1 );
    }

    void setHeader( const QString& text )
    {
        m_label->setText( text );
        m_label->setVisible( !m_headerWidget && !text.isEmpty() );
        setAccessibleName( text );
        updateGeometry();
    }

    QWidget* headerWidget() const
    {
        return m_headerWidget.data();
    }

    void setHeaderWidget( QWidget* widget )
    {
        if ( m_headerWidget == widget )
        {
            m_label->setVisible( !widget && !m_label->text().isEmpty() );
            return;
        }
        if ( m_headerWidget )
        {
            m_layout->removeWidget( m_headerWidget );
            m_headerWidget->setParent( nullptr );
        }
        m_headerWidget = widget;
        if ( widget )
        {
            widget->setParent( this );
            m_layout->addWidget( widget, 1 );
            widget->show();
        }
        m_label->setVisible( !widget && !m_label->text().isEmpty() );
        updateGeometry();
    }

    QWidget* takeHeaderWidget()
    {
        QWidget* widget = m_headerWidget;
        if ( widget )
        {
            m_layout->removeWidget( widget );
            widget->setParent( nullptr );
            m_headerWidget = nullptr;
            m_label->setVisible( !m_label->text().isEmpty() );
            updateGeometry();
        }
        return widget;
    }

    void headerWidgetDestroyed()
    {
        // QWidget 析构时会自行从 QLayout 移除；这里只同步状态。
        // 不能再次调用 removeWidget()，否则可能重入正在析构的布局。
        m_headerWidget = nullptr;
        m_label->setVisible( !m_label->text().isEmpty() );
        updateGeometry();
    }

    void setExpansionProgress( qreal progress )
    {
        m_progress = qBound( 0.0, progress, 1.0 );
        update();
    }

protected:
    bool event( QEvent* event ) override
    {
        const bool result = QAbstractButton::event( event );
        if ( event->type() == QEvent::Enter || event->type() == QEvent::Leave
             || event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave
             || event->type() == QEvent::EnabledChange || event->type() == QEvent::LayoutDirectionChange )
        {
            if ( event->type() == QEvent::LayoutDirectionChange )
            {
                updateLayoutMargins();
            }
            update();
        }
        return result;
    }

    void paintEvent( QPaintEvent* ) override
    {
        QPainter painter( this );
        painter.setRenderHint( QPainter::Antialiasing );

        const bool connected = m_expander->isExpanded() && m_expander->hasContentWidgets();
        const bool down = m_expander->expandDirection() == ExExpander::Down;
        const QRectF bounds = QRectF( rect() ).adjusted( 0.5, 0.5, -0.5, -0.5 );
        const QPainterPath headerPath = roundedPanelPath(
            bounds,
            !connected || down,
            !connected || down,
            !connected || !down,
            !connected || !down );
        painter.fillPath( headerPath, cardBackground( palette() ) );
        painter.setBrush( Qt::NoBrush );
        painter.setPen( QPen( cardBorder( palette(), isEnabled() ), 1.0 ) );
        painter.drawPath( headerPath );

        const bool rtl = layoutDirection() == Qt::RightToLeft;
        const qreal chevronX = rtl ? ChevronTrailingMargin
                                   : width() - ChevronTrailingMargin - ChevronButtonSize;
        const QRectF chevronRect( chevronX,
                                  ( height() - ChevronButtonSize ) * 0.5,
                                  ChevronButtonSize,
                                  ChevronButtonSize );
        if ( isEnabled() && ( underMouse() || isDown() ) )
        {
            QColor subtle = palette().color( QPalette::Button );
            const bool dark = palette().color( QPalette::Window ).lightness() < 128;
            subtle.setAlpha( isDown() ? ( dark ? 38 : 16 ) : ( dark ? 25 : 10 ) );
            painter.setPen( Qt::NoPen );
            painter.setBrush( subtle );
            painter.drawRoundedRect( chevronRect, CornerRadius, CornerRadius );
        }

        const qreal angle = down ? 180.0 * m_progress : 180.0 * ( 1.0 - m_progress );
        painter.save();
        painter.translate( chevronRect.center() );
        painter.rotate( angle );
        painter.setPen( palette().color( isEnabled() ? QPalette::Active : QPalette::Disabled,
                                         QPalette::Text ) );
        QFont iconFont( QStringLiteral( "Segoe Fluent Icons" ) );
        iconFont.setPixelSize( 15 );
        iconFont.setStyleStrategy( QFont::NoFontMerging );
        painter.setFont( iconFont );
        const QRectF glyphRect( -ChevronButtonSize * 0.5,
                                -ChevronButtonSize * 0.5,
                                ChevronButtonSize,
                                ChevronButtonSize );
        painter.drawText( glyphRect, Qt::AlignCenter, QString( QChar( 0xE972 ) ) );
        painter.restore();

        if ( hasFocus() )
        {
            QStyleOptionFocusRect option;
            option.initFrom( this );
            option.rect = rect().adjusted( 3, 3, -3, -3 );
            option.backgroundColor = palette().color( QPalette::Window );
            style()->drawPrimitive( QStyle::PE_FrameFocusRect, &option, &painter, this );
        }
    }

private:
    void updateLayoutMargins()
    {
        const int chevronSide = ChevronContentSpacing + ChevronButtonSize + ChevronTrailingMargin;
        if ( layoutDirection() == Qt::RightToLeft )
        {
            m_layout->setContentsMargins( chevronSide, 0, HeaderContentPadding, 0 );
        }
        else
        {
            m_layout->setContentsMargins( HeaderContentPadding, 0, chevronSide, 0 );
        }
    }

    ExExpander* m_expander = nullptr;
    QHBoxLayout* m_layout = nullptr;
    QLabel* m_label = nullptr;
    QPointer<QWidget> m_headerWidget;
    qreal m_progress = 0.0;
};

ExExpander::ExExpander( QWidget* parent )
    : QWidget( parent )
    , m_rootLayout( new QVBoxLayout( this ) )
    , m_headerButton( new HeaderButton( this ) )
    , m_contentStack( new ContentStack( this ) )
    , m_contentContainer( new ContentViewport( this, m_contentStack ) )
    , m_contentLayout( new QVBoxLayout( m_contentStack ) )
    , m_expansionAnimation( new QVariantAnimation( this ) )
{
    setMinimumWidth( ExpanderMinWidth );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );
    m_rootLayout->setContentsMargins( 0, 0, 0, 0 );
    m_rootLayout->setSpacing( 0 );

    m_contentLayout->setContentsMargins( 0, 0, 0, 0 );
    m_contentLayout->setSpacing( 0 );
    m_contentContainer->prepareAnimation();
    m_contentContainer->setViewportProgress( 0.0 );
    m_contentContainer->hide();

    rebuildLayout();

    connect( m_headerButton, &QAbstractButton::clicked, this, &ExExpander::toggle );
    connect( m_expansionAnimation, &QVariantAnimation::valueChanged, this, [this]( const QVariant& value )
    {
        m_expansionProgress = value.toReal();
        m_headerButton->setExpansionProgress( m_expansionProgress );
        m_contentContainer->setViewportProgress( m_expansionProgress );
    } );
    connect( m_expansionAnimation, &QVariantAnimation::finished,
             this, &ExExpander::finishTransition );
}

ExExpander::~ExExpander()
{
    // QWidget 的子控件在派生类析构体之后才被逐个删除。提前断开监听，
    // 避免 destroyed 回调访问已经进入析构流程的 Header/Layout/Viewport。
    disconnect( m_headerWidgetDestroyedConnection );
    for ( const QMetaObject::Connection& connection : m_contentWidgetDestroyedConnections )
    {
        disconnect( connection );
    }
}

QString ExExpander::header() const
{
    return m_header;
}

void ExExpander::setHeader( const QString& header )
{
    if ( m_header == header )
    {
        return;
    }
    m_header = header;
    m_headerButton->setHeader( header );
    emit headerChanged( header );
}

bool ExExpander::isExpanded() const
{
    return m_expanded;
}

ExExpander::ExpandDirection ExExpander::expandDirection() const
{
    return m_expandDirection;
}

void ExExpander::setExpandDirection( ExpandDirection direction )
{
    if ( m_expandDirection == direction )
    {
        return;
    }
    m_expandDirection = direction;
    rebuildContentLayout();
    rebuildLayout();
    m_headerButton->setExpansionProgress( m_expansionProgress );
    m_contentContainer->setViewportProgress( m_expansionProgress );
    m_headerButton->update();
    for ( ContentPanel* panel : m_contentPanels )
    {
        panel->update();
    }
    updateGeometry();
    emit expandDirectionChanged( direction );
}

bool ExExpander::isAnimationEnabled() const
{
    return m_animationEnabled;
}

void ExExpander::setAnimationEnabled( bool enabled )
{
    if ( m_animationEnabled == enabled )
    {
        return;
    }
    m_animationEnabled = enabled;
    if ( !enabled && m_expansionAnimation->state() == QAbstractAnimation::Running )
    {
        m_expansionAnimation->stop();
        finishTransition();
    }
    emit animationEnabledChanged( enabled );
}

int ExExpander::animationDuration() const
{
    return m_animationDuration;
}

void ExExpander::setAnimationDuration( int duration )
{
    duration = qMax( 0, duration );
    if ( m_animationDuration == duration )
    {
        return;
    }
    m_animationDuration = duration;
    emit animationDurationChanged( duration );
}

QWidget* ExExpander::headerWidget() const
{
    return m_headerWidget.data();
}

void ExExpander::setHeaderWidget( QWidget* widget )
{
    if ( widget == this
         || widget == m_headerButton
         || widget == m_contentContainer
         || widget == m_contentStack
         || isInternalWidget( widget )
         || isContentWidgetOrDescendant( widget )
         || widget == m_headerWidget.data()
         || ( widget && m_headerWidget && m_headerWidget->isAncestorOf( widget ) )
         || ( widget && widget->isAncestorOf( this ) ) )
    {
        return;
    }
    if ( m_headerWidget )
    {
        disconnect( m_headerWidgetDestroyedConnection );
        delete m_headerButton->takeHeaderWidget();
    }
    m_headerButton->setHeaderWidget( widget );
    m_headerWidget = widget;
    if ( widget )
    {
        m_headerWidgetDestroyedConnection = connect( widget, &QObject::destroyed, this, [this]
        {
            m_headerWidget = nullptr;
            m_headerButton->headerWidgetDestroyed();
            updateGeometry();
        } );
    }
    updateGeometry();
}

QWidget* ExExpander::takeHeaderWidget()
{
    QWidget* widget = m_headerButton->takeHeaderWidget();
    disconnect( m_headerWidgetDestroyedConnection );
    m_headerWidget = nullptr;
    updateGeometry();
    return widget;
}

QList<QWidget*> ExExpander::contentWidgets() const
{
    QList<QWidget*> result;
    for ( const QPointer<QWidget>& widget : m_contentWidgets )
    {
        if ( widget )
        {
            result.append( widget.data() );
        }
    }
    return result;
}

void ExExpander::addContentWidget( QWidget* widget )
{
    if ( !widget
         || widget == this
         || widget == m_headerButton
         || widget == m_contentContainer
         || widget == m_contentStack
         || isInternalWidget( widget )
         || widget == m_headerWidget.data()
         || ( widget && m_headerWidget && m_headerWidget->isAncestorOf( widget ) )
         || isContentWidgetOrDescendant( widget )
         || ( widget && widget->isAncestorOf( this ) ) )
    {
        return;
    }
    for ( const QPointer<QWidget>& content : m_contentWidgets )
    {
        if ( content && widget->isAncestorOf( content ) )
        {
            return;
        }
    }

    auto* panel = new ContentPanel( this, widget );
    m_contentWidgets.append( widget );
    m_contentPanels.append( panel );
    m_contentWidgetDestroyedConnections.append(
        connect( widget, &QObject::destroyed, this, [this, panel]
        {
            const int index = m_contentPanels.indexOf( panel );
            if ( index < 0 )
            {
                return;
            }
            m_contentLayout->removeWidget( panel );
            m_contentWidgets.removeAt( index );
            m_contentPanels.removeAt( index );
            m_contentWidgetDestroyedConnections.removeAt( index );
            panel->deleteLater();
            // sender 正处于 QWidget 析构中，布局刷新延迟到本轮事件结束，
            // 避免重入其内部 QLayout 的子项移除流程。
            scheduleContentRefresh();
        } ) );
    rebuildContentLayout();
    refreshContentGeometry();
}

bool ExExpander::removeContentWidget( QWidget* widget )
{
    int index = -1;
    for ( int i = 0; i < m_contentWidgets.size(); ++i )
    {
        if ( m_contentWidgets.at( i ) == widget )
        {
            index = i;
            break;
        }
    }
    if ( index < 0 )
    {
        return false;
    }
    disconnect( m_contentWidgetDestroyedConnections.takeAt( index ) );
    m_contentWidgets.removeAt( index );
    ContentPanel* panel = m_contentPanels.takeAt( index );
    m_contentLayout->removeWidget( panel );
    delete panel;
    rebuildContentLayout();
    refreshContentGeometry();
    return true;
}

void ExExpander::clearContentWidgets()
{
    for ( const QMetaObject::Connection& connection : m_contentWidgetDestroyedConnections )
    {
        disconnect( connection );
    }
    m_contentWidgetDestroyedConnections.clear();
    m_contentWidgets.clear();
    const QList<ContentPanel*> panels = m_contentPanels;
    m_contentPanels.clear();
    for ( ContentPanel* panel : panels )
    {
        m_contentLayout->removeWidget( panel );
        delete panel;
    }
    refreshContentGeometry();
}

bool ExExpander::hasContentWidgets() const
{
    return !m_contentPanels.isEmpty();
}

bool ExExpander::isInternalWidget( const QWidget* widget ) const
{
    return widget
           && ( m_headerButton->isAncestorOf( widget )
                || m_contentContainer->isAncestorOf( widget ) );
}

bool ExExpander::isContentWidgetOrDescendant( const QWidget* widget ) const
{
    if ( !widget )
    {
        return false;
    }
    for ( const QPointer<QWidget>& content : m_contentWidgets )
    {
        if ( content && ( content.data() == widget || content->isAncestorOf( widget ) ) )
        {
            return true;
        }
    }
    return false;
}

void ExExpander::scheduleContentRefresh()
{
    if ( m_contentRefreshScheduled )
    {
        return;
    }
    m_contentRefreshScheduled = true;
    QTimer::singleShot( 0, this, [this]
    {
        m_contentRefreshScheduled = false;
        rebuildContentLayout();
        refreshContentGeometry();
    } );
}

void ExExpander::rebuildContentLayout()
{
    for ( ContentPanel* panel : m_contentPanels )
    {
        m_contentLayout->removeWidget( panel );
        panel->setOuterEdge( false );
    }
    if ( m_contentPanels.isEmpty() )
    {
        return;
    }

    // 第一个添加的 Content 始终紧邻 Header；向上展开时反向排列，
    // 后续添加项自然继续向远离 Header 的方向延伸。
    if ( m_expandDirection == Down )
    {
        for ( ContentPanel* panel : m_contentPanels )
        {
            m_contentLayout->addWidget( panel );
            panel->show();
        }
    }
    else
    {
        for ( int index = m_contentPanels.size() - 1; index >= 0; --index )
        {
            m_contentLayout->addWidget( m_contentPanels.at( index ) );
            m_contentPanels.at( index )->show();
        }
    }
    // 最后追加的 Content 位于距离 Header 最远的一端，负责整体外圆角。
    m_contentPanels.last()->setOuterEdge( true );
}

void ExExpander::refreshContentGeometry()
{
    m_contentLayout->activate();
    m_contentContainer->prepareAnimation();
    m_contentContainer->setViewportProgress( m_expansionProgress );
    const bool visible = m_expanded && hasContentWidgets();
    m_contentStack->setVisible( visible );
    m_contentContainer->setVisible( visible );
    m_headerButton->update();
    updateGeometry();
}

QSize ExExpander::sizeHint() const
{
    return m_rootLayout->sizeHint();
}

QSize ExExpander::minimumSizeHint() const
{
    return m_rootLayout->minimumSize();
}

void ExExpander::setExpanded( bool expanded )
{
    if ( m_expanded == expanded )
    {
        return;
    }
    m_expanded = expanded;
    m_headerButton->setChecked( expanded );
    if ( expanded )
    {
        emit expanding();
    }
    emit expandedChanged( expanded );
    m_headerButton->update();

    m_expansionAnimation->stop();
    const bool canAnimate = m_animationEnabled && m_animationDuration > 0
                            && parentWidget() && parentWidget()->isVisible();
    if ( !canAnimate )
    {
        m_expansionProgress = expanded ? 1.0 : 0.0;
        m_headerButton->setExpansionProgress( m_expansionProgress );
        if ( expanded )
        {
            m_contentContainer->setVisible( hasContentWidgets() );
            m_contentStack->setVisible( hasContentWidgets() );
            m_rootLayout->activate();
        }
        m_contentContainer->prepareAnimation();
        m_contentStack->setVisible( expanded && hasContentWidgets() );
        m_contentContainer->setViewportProgress( m_expansionProgress );
        m_contentContainer->setVisible( expanded && hasContentWidgets() );
        updateGeometry();
        if ( !expanded )
        {
            emit collapsed();
        }
        emit expansionFinished( expanded );
        return;
    }

    // ContentPanel 不做位移动画：展开时立即出现，收起时立即消失。
    // 动画只改变外层 ContentViewport 的裁剪高度和布局占用高度。
    m_contentContainer->setVisible( hasContentWidgets() );
    if ( expanded )
    {
        m_contentStack->setVisible( hasContentWidgets() );
    }
    m_rootLayout->activate();
    m_contentContainer->prepareAnimation();
    m_contentStack->setVisible( expanded && hasContentWidgets() );
    m_contentContainer->setViewportProgress( m_expansionProgress );

    if ( expanded )
    {
        const int duration = qMax( 1, m_animationDuration * 2 - 1 );
        if ( m_expansionProgress < 1.0 )
        {
            m_expansionAnimation->setDuration(
                qMax( 1, qRound( duration * ( 1.0 - m_expansionProgress ) ) ) );
            m_expansionAnimation->setEasingCurve( QEasingCurve::OutCubic );
            m_expansionAnimation->setStartValue( m_expansionProgress );
            m_expansionAnimation->setEndValue( 1.0 );
            m_expansionAnimation->start();
        }
        else
        {
            finishTransition();
        }
        return;
    }

    if ( m_expansionProgress > 0.0 )
    {
        m_expansionAnimation->setDuration(
            qMax( 1, qRound( m_animationDuration * m_expansionProgress ) ) );
        m_expansionAnimation->setEasingCurve( QEasingCurve::OutCubic );
        m_expansionAnimation->setStartValue( m_expansionProgress );
        m_expansionAnimation->setEndValue( 0.0 );
        m_expansionAnimation->start();
    }
    else
    {
        finishTransition();
    }
}

void ExExpander::toggle()
{
    setExpanded( !m_expanded );
}

void ExExpander::changeEvent( QEvent* event )
{
    QWidget::changeEvent( event );
    if ( event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange
         || event->type() == QEvent::StyleChange || event->type() == QEvent::LayoutDirectionChange
         || event->type() == QEvent::EnabledChange )
    {
        m_headerButton->update();
        for ( ContentPanel* panel : m_contentPanels )
        {
            panel->update();
        }
    }
}

void ExExpander::rebuildLayout()
{
    m_rootLayout->removeWidget( m_headerButton );
    m_rootLayout->removeWidget( m_contentContainer );
    if ( m_expandDirection == Down )
    {
        m_rootLayout->addWidget( m_headerButton );
        m_rootLayout->addWidget( m_contentContainer );
    }
    else
    {
        m_rootLayout->addWidget( m_contentContainer );
        m_rootLayout->addWidget( m_headerButton );
    }
}

void ExExpander::finishTransition()
{
    m_expansionProgress = m_expanded ? 1.0 : 0.0;
    m_headerButton->setExpansionProgress( m_expansionProgress );
    m_contentContainer->setViewportProgress( m_expansionProgress );
    m_contentStack->setVisible( m_expanded && hasContentWidgets() );
    m_contentContainer->setVisible( m_expanded && hasContentWidgets() );
    updateGeometry();
    if ( !m_expanded )
    {
        emit collapsed();
    }
    emit expansionFinished( m_expanded );
}
