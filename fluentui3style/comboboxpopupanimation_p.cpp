#include "comboboxpopupanimation_p.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsProxyWidget>
#include <QKeyEvent>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QPropertyAnimation>
#include <QRegion>
#include <QScreen>
#include <QVariantAnimation>
#include <QWidget>

#include "fluentui3styleproperties.h"

static constexpr int AnimationDuration = 300;

static QGraphicsProxyWidget* comboBoxGraphicsProxy( const QComboBox* comboBox )
{
    if ( !comboBox )
    {
        return nullptr;
    }

    for ( const QWidget* ancestor = comboBox; ancestor; ancestor = ancestor->parentWidget() )
    {
        if ( QGraphicsProxyWidget* proxy = ancestor->graphicsProxyWidget() )
        {
            return proxy;
        }
    }

    return nullptr;
}

static void ensurePopupViewInteractive( QComboBox* comboBox )
{
    if ( !comboBox )
    {
        return;
    }

    if ( QAbstractItemView* popupView = comboBox->view() )
    {
        popupView->clearMask();
        popupView->setEnabled( true );
        if ( popupView->viewport() )
        {
            popupView->viewport()->clearMask();
            popupView->viewport()->setEnabled( true );
        }
    }
}

static bool comboBoxAnimationPropertyEnabled( const QComboBox* comboBox, const char* propertyName, bool defaultEnabled )
{
    const QVariant globalValue = qApp->property( propertyName );
    if ( globalValue.isValid() && !globalValue.toBool() )
    {
        return false;
    }

    if ( comboBox )
    {
        const QVariant localValue = comboBox->property( propertyName );
        if ( localValue.isValid() )
        {
            return localValue.toBool();
        }
    }

    return globalValue.isValid() ? globalValue.toBool() : defaultEnabled;
}

static bool comboBoxPopupAnimationEnabled( const QComboBox* comboBox )
{
    // 该 Qt 私有属性只选择 Qt 自己的 popup 行为；开启时不叠加任何
    // FluentUI3Style 自定义动画。
    if ( qApp->property( "_q_scrollHint_center" ).toBool() )
    {
        return false;
    }

    // QGraphicsView 缩放时，detach/move + mask 会让 popup 看起来对齐但鼠标
    // 命中仍落在旧的 proxy 几何上；GraphicsView 场景下先禁用展开动画。
    if ( comboBoxGraphicsProxy( comboBox ) )
    {
        return false;
    }

    return comboBoxAnimationPropertyEnabled( comboBox, ComboBoxPopupDropDownAnimationEnabledProperty, true );
}

// 普通 QComboBox 无法在 hidePopup() 前接管关闭过程，因此这里只通过
// QComboBoxPrivateContainer 的 Show 事件实现展开动画，关闭完全交还 Qt。
class ComboBoxPopupAnimatorImpl final : public QObject
{
public:
    explicit ComboBoxPopupAnimatorImpl( QComboBox* comboBox, QObject* parent )
        : QObject( parent ? parent : comboBox )
        , m_comboBox( comboBox )
    {
        attachPopup( comboBox->view()->window() );
    }

    ~ComboBoxPopupAnimatorImpl() override { stop(); }

    void stop()
    {
        if ( m_popup )
        {
            stopAnimation( m_popup );
        }
        removeApplicationEventFilter();
    }

protected:
    bool eventFilter( QObject* watched, QEvent* event ) override
    {
        if ( !m_comboBox || !m_popup )
        {
            return QObject::eventFilter( watched, event );
        }

        if ( event->type() == QEvent::ApplicationDeactivate
             || ( watched == m_comboBox->window() && ( event->type() == QEvent::Hide || event->type() == QEvent::Close ) ) )
        {
            stopAnimation( m_popup );
            removeApplicationEventFilter();
            return QObject::eventFilter( watched, event );
        }

        if ( watched == m_popup && event->type() == QEvent::Show )
        {
            if ( !m_popupShown )
            {
                m_popupShown = true;
                ComboBoxPopupAnimator::positionPopupForShadow( m_popup );
                if ( comboBoxPopupAnimationEnabled( m_comboBox ) )
                {
                    animatePopup( m_popup );
                }
            }
            return QObject::eventFilter( watched, event );
        }

        if ( watched == m_popup && event->type() == QEvent::Hide )
        {
            m_popupShown = false;
            m_popup->setProperty( ComboBoxPopupShadowPositionedProperty, false );
            stopAnimation( m_popup );
            removeApplicationEventFilter();
            return QObject::eventFilter( watched, event );
        }

        if ( !comboBoxPopupAnimationEnabled( m_comboBox ) )
        {
            stopAnimation( m_popup );
            removeApplicationEventFilter();
            return QObject::eventFilter( watched, event );
        }

        // 展开完成前吞掉后续输入，避免动画几何尚未恢复时关闭或选中。
        // view/viewport 会直接接收这些事件，所以动画期间临时从 qApp
        // 过滤，而不是给整个应用永久安装过滤器。
        if ( m_isOpening )
        {
            switch ( event->type() )
            {
                case QEvent::MouseButtonPress :
                case QEvent::MouseButtonRelease :
                case QEvent::MouseButtonDblClick :
                    return true;
                case QEvent::KeyPress :
                case QEvent::KeyRelease :
                {
                    const int key = static_cast<QKeyEvent*>( event )->key();
                    if ( key == Qt::Key_Escape || key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Space )
                    {
                        return true;
                    }
                    break;
                }
                default :
                    break;
            }
        }

        return QObject::eventFilter( watched, event );
    }

private:
    void attachPopup( QWidget* popup )
    {
        if ( !popup || !popup->inherits( "QComboBoxPrivateContainer" ) || m_popup == popup )
        {
            return;
        }

        if ( m_popup )
        {
            m_popup->removeEventFilter( this );
        }
        m_popup      = popup;
        m_popupShown = popup->isVisible();
        m_popup->installEventFilter( this );
    }

    void installApplicationEventFilter()
    {
        if ( !m_applicationFilterInstalled )
        {
            qApp->installEventFilter( this );
            m_applicationFilterInstalled = true;
        }
    }

    void removeApplicationEventFilter()
    {
        if ( m_applicationFilterInstalled )
        {
            qApp->removeEventFilter( this );
            m_applicationFilterInstalled = false;
        }
    }

    void animatePopup( QWidget* popup )
    {
        stopAnimation( popup );

        if ( !beginPopupAnimation( popup ) )
        {
            return;
        }

        installApplicationEventFilter();
        startPopupAnimation( popup );
    }

    bool beginPopupAnimation( QWidget* popup )
    {
        auto* popupView           = m_comboBox->view();
        auto* popupLayout         = popup->layout();
        auto* popupBoxLayout      = qobject_cast<QBoxLayout*>( popupLayout );
        const int viewLayoutIndex = popupLayout ? popupLayout->indexOf( popupView ) : -1;
        if ( !popupView || !popupBoxLayout || viewLayoutIndex < 0 )
        {
            return false;
        }

        m_isOpening     = true;
        m_finalGeometry = popup->geometry();
        m_opensAbove    = popup->property( ComboBoxPopupOpensAboveProperty ).toBool();

        m_popupLayout       = popupLayout;
        m_viewLayoutIndex   = viewLayoutIndex;
        m_finalViewPosition = popupView->pos();
        m_viewBottomMargin  = qMax( 0, m_finalGeometry.height() - ( popupView->geometry().bottom() + 1 ) );
        m_originalViewMask  = popupView->mask();

        popupLayout->removeWidget( popupView );
        m_viewDetached = true;
        return true;
    }

    void startPopupAnimation( QWidget* popup )
    {
        auto* popupView          = m_comboBox->view();
        QPoint startViewPosition = m_finalViewPosition;
        if ( m_opensAbove )
        {
            startViewPosition.setY( 1 );
        }
        else
        {
            startViewPosition.ry() -= popupView->height();
        }

        popup->setFixedHeight( 1 );
        if ( m_opensAbove )
        {
            popup->move( m_finalGeometry.x(), m_finalGeometry.bottom() );
        }
        else
        {
            popup->move( m_finalGeometry.topLeft() );
        }
        popupView->move( startViewPosition );
        updateViewClip( popup, popupView );

        m_animationGroup = new QParallelAnimationGroup( popup );

        auto* heightAnimation = new QVariantAnimation( m_animationGroup );
        heightAnimation->setStartValue( 1 );
        heightAnimation->setEndValue( m_finalGeometry.height() );
        heightAnimation->setDuration( AnimationDuration );
        heightAnimation->setEasingCurve( QEasingCurve::OutCubic );

        connect( heightAnimation,
                 &QVariantAnimation::valueChanged,
                 this,
                 [ this, popup ]( const QVariant& value )
                 {
                     const int height = value.toInt();
                     popup->setFixedHeight( height );

                     if ( m_opensAbove )
                     {
                         popup->move( m_finalGeometry.x(), m_finalGeometry.bottom() - height + 1 );
                     }
                     else
                     {
                         popup->move( m_finalGeometry.topLeft() );
                     }
                     updateViewClip( popup, m_comboBox->view() );
                 } );

        auto* viewAnimation = new QPropertyAnimation( popupView, "pos", m_animationGroup );
        viewAnimation->setStartValue( startViewPosition );
        viewAnimation->setEndValue( m_finalViewPosition );
        viewAnimation->setDuration( AnimationDuration );
        viewAnimation->setEasingCurve( QEasingCurve::OutCubic );
        connect( viewAnimation,
                 &QPropertyAnimation::valueChanged,
                 this,
                 [ this, popup, popupView ]( const QVariant& ) { updateViewClip( popup, popupView ); } );

        m_animationGroup->addAnimation( heightAnimation );
        m_animationGroup->addAnimation( viewAnimation );

        connect( m_animationGroup,
                 &QParallelAnimationGroup::finished,
                 this,
                 [ this, popup ]
                 {
                     restorePopup( popup );
                     restoreHeightConstraints( popup );
                     m_animationGroup = nullptr;
                     m_isOpening      = false;
                     removeApplicationEventFilter();
                     ensurePopupViewInteractive( m_comboBox );
                 } );

        m_animationGroup->start( QAbstractAnimation::DeleteWhenStopped );
    }

    void stopAnimation( QWidget* popup )
    {
        const bool hadAnimation = m_animationGroup;

        if ( m_animationGroup )
        {
            m_animationGroup->stop();
            m_animationGroup = nullptr;
        }

        m_isOpening = false;
        removeApplicationEventFilter();
        if ( hadAnimation || m_viewDetached )
        {
            restorePopup( popup );
        }
        restoreHeightConstraints( popup );
        ensurePopupViewInteractive( m_comboBox );
    }

    void restorePopup( QWidget* popup )
    {
        if ( m_finalGeometry.isValid() )
        {
            popup->setFixedHeight( m_finalGeometry.height() );
            popup->move( m_finalGeometry.topLeft() );
        }

        if ( !m_viewDetached || !m_comboBox || !m_popupLayout )
        {
            return;
        }

        auto* popupView = m_comboBox->view();
        popupView->move( m_finalViewPosition );
        if ( m_originalViewMask.isEmpty() )
        {
            popupView->clearMask();
        }
        else
        {
            popupView->setMask( m_originalViewMask );
        }

        auto* boxLayout = qobject_cast<QBoxLayout*>( m_popupLayout.data() );
        if ( !boxLayout )
        {
            return;
        }

        boxLayout->insertWidget( m_viewLayoutIndex, popupView );
        m_viewDetached = false;
    }

    void updateViewClip( QWidget* popup, QWidget* popupView )
    {
        if ( !popup || !popupView || !m_viewDetached )
        {
            return;
        }

        // The final layout position defines the actual content area.  Clip
        // the detached moving view to that area instead of to popup->rect(),
        // whose top and bottom also contain the transparent shadow reserve.
        const int contentHeight = popup->height() - m_finalViewPosition.y() - m_viewBottomMargin;
        if ( contentHeight <= 0 )
        {
            popupView->setMask( QRegion( QRect( -1, -1, 1, 1 ) ) );
            return;
        }

        const QRect contentRect( m_finalViewPosition.x(), m_finalViewPosition.y(), popupView->width(), contentHeight );
        const QRect visibleRect = contentRect.intersected( popupView->geometry() );
        if ( visibleRect.isEmpty() )
        {
            popupView->setMask( QRegion( QRect( -1, -1, 1, 1 ) ) );
            return;
        }

        const QRect localVisibleRect = visibleRect.translated( -popupView->pos() );
        if ( localVisibleRect == popupView->rect() )
        {
            popupView->clearMask();
        }
        else
        {
            popupView->setMask( QRegion( localVisibleRect ) );
        }
    }

    void restoreHeightConstraints( QWidget* popup )
    {
        popup->setMinimumHeight( 0 );
        popup->setMaximumHeight( QWIDGETSIZE_MAX );
    }

    QPointer<QComboBox> m_comboBox;
    QPointer<QWidget> m_popup;
    QPointer<QParallelAnimationGroup> m_animationGroup;
    QPointer<QLayout> m_popupLayout;
    QRect m_finalGeometry;
    QPoint m_finalViewPosition;
    QRegion m_originalViewMask;
    int m_viewBottomMargin            = 0;
    int m_viewLayoutIndex             = -1;
    bool m_isOpening                  = false;
    bool m_viewDetached               = false;
    bool m_opensAbove                 = false;
    bool m_popupShown                 = false;
    bool m_applicationFilterInstalled = false;
};

ComboBoxPopupAnimator::ComboBoxPopupAnimator( QComboBox* comboBox, QObject* parent )
    : QObject( parent ? parent : comboBox )
    , m_impl( new ComboBoxPopupAnimatorImpl( comboBox, this ) )
{
}

ComboBoxPopupAnimator::~ComboBoxPopupAnimator() = default;

void ComboBoxPopupAnimator::stop()
{
    m_impl->stop();
}

bool ComboBoxPopupAnimator::isEnabled( const QComboBox* comboBox )
{
    return comboBoxPopupAnimationEnabled( comboBox );
}

QComboBox* ComboBoxPopupAnimator::comboBoxForPopup( const QWidget* popup )
{
    const QWidget* parent = popup;
    while ( parent )
    {
        if ( auto* comboBox = qobject_cast<const QComboBox*>( parent ) )
        {
            return const_cast<QComboBox*>( comboBox );
        }
        parent = parent->parentWidget();
    }
    return nullptr;
}

void ComboBoxPopupAnimator::positionPopupForShadow( QWidget* popup )
{
    QComboBox* comboBox = comboBoxForPopup( popup );
    if ( !popup || !comboBox || popup->property( ComboBoxPopupShadowPositionedProperty ).toBool() )
    {
        return;
    }

    QRect geometry = popup->geometry();
    if ( !geometry.isValid() )
    {
        return;
    }

    QGraphicsProxyWidget* comboProxy = nullptr;
    for ( QWidget* ancestor = comboBox;
          ancestor && !comboProxy;
          ancestor = ancestor->parentWidget() )
    {
        comboProxy = ancestor->graphicsProxyWidget();
    }

    QGraphicsProxyWidget* popupProxy = popup->graphicsProxyWidget();
    auto* popupProxyParent =
        popupProxy
            ? qobject_cast<QGraphicsProxyWidget*>( popupProxy->parentWidget() )
            : nullptr;
    QWidget* popupParentWidget = popup->parentWidget();
    const QRectF popupParentRect =
        popupProxyParent && popupParentWidget
            ? popupProxyParent->subWidgetRect( popupParentWidget )
            : QRectF();
    if ( comboProxy && popupProxy && popupProxyParent && popupParentWidget
         && popupParentRect.isValid() )
    {
        const QRectF comboRectInParent =
            popupProxyParent->mapRectFromItem(
                comboProxy,
                comboProxy->subWidgetRect( comboBox ) );
        QRectF proxyGeometry = popupProxy->geometry();
        if ( comboRectInParent.isValid() && proxyGeometry.isValid() )
        {
            const QRectF comboFrameRect =
                comboRectInParent.adjusted(
                    ComboBoxControlFrameHorizontalInset,
                    0,
                    -ComboBoxControlFrameHorizontalInset,
                    0 );
            const bool opensAbove =
                proxyGeometry.center().y() < comboRectInParent.center().y();
            const qreal shadow = FlyoutShadowBorderWidth;

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
            constexpr int panelHorizontalExpansion = 2;
#else
            constexpr int panelHorizontalExpansion = 2;
#endif
            const qreal panelWidth =
                comboFrameRect.width() + panelHorizontalExpansion;
            const qreal panelHeight =
                qMax( 1.0, proxyGeometry.height() - 2.0 * shadow );
            proxyGeometry.setSize(
                QSizeF( panelWidth + 2.0 * shadow,
                        panelHeight + 2.0 * shadow ) );

            if ( comboBox->layoutDirection() == Qt::RightToLeft )
            {
                proxyGeometry.moveRight(
                    comboFrameRect.right() + shadow
                    + ( panelHorizontalExpansion - 1 ) );
            }
            else
            {
                proxyGeometry.moveLeft(
                    comboFrameRect.left() - shadow
                    - ( panelHorizontalExpansion - 1 ) );
            }

            if ( opensAbove )
            {
                const qreal panelBottom =
                    comboRectInParent.top() - FlyoutPopupOffset;
                proxyGeometry.moveBottom( panelBottom + shadow );
            }
            else
            {
                const qreal panelTop =
                    comboRectInParent.bottom() + FlyoutPopupOffset;
                proxyGeometry.moveTop( panelTop - shadow );
            }

            // Nested popup widgets keep a fake-global geometry for painting under
            // QGraphicsView transforms.  Keep that path for visibility, and sync
            // the proxy item separately so mouse delivery follows the visual rect.
            const QPointF popupParentTopLeft = popupParentRect.topLeft();
            const QPoint popupLocalPosition =
                ( proxyGeometry.topLeft() - popupParentTopLeft ).toPoint();
            const QPoint popupWidgetPosition =
                popupParentWidget->mapToGlobal( popupLocalPosition );

            popup->setGeometry(
                QRect( popupWidgetPosition,
                       QSize( qRound( proxyGeometry.width() ),
                              qRound( proxyGeometry.height() ) ) ) );

            if ( popupProxy->parentItem() == popupProxyParent )
            {
                popupProxy->setGeometry( proxyGeometry );
            }

            if ( comboProxy )
            {
                popupProxy->setZValue( comboProxy->zValue() + 1.0 );
            }

            popup->setProperty( ComboBoxPopupOpensAboveProperty, opensAbove );
            popup->setProperty( ComboBoxPopupShadowPositionedProperty, true );
            ensurePopupViewInteractive( comboBox );
            popup->update();
            return;
        }
    }

    const QRect comboGeometry( comboBox->mapToGlobal( QPoint( 0, 0 ) ), comboBox->size() );
    const QRect comboFrameGeometry =
        comboGeometry.adjusted( ComboBoxControlFrameHorizontalInset, 0, -ComboBoxControlFrameHorizontalInset, 0 );
    const bool opensAbove = geometry.center().y() < comboGeometry.center().y();

    // Keep the visible popup panel exactly as wide as the painted ComboBox.
    // QComboBoxPrivateContainer may otherwise widen it from the item sizeHint.
    // The transparent shadow reserve belongs to the popup window outside that
    // visible width.
    const int shadow = FlyoutShadowBorderWidth;
    // The ComboBox body uses a centered 1 px QPen.  Qt 5 rasterizes its
    // outside edge one pixel beyond both sides of the geometric frame, while
    // the filled popup path starts at the frame edge.  Expand the popup panel
    // by one more pixel on Qt 5 so their visible edges match.
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    constexpr int panelHorizontalExpansion = 2;
#else
    constexpr int panelHorizontalExpansion = 1;
#endif
    const int panelWidth  = comboFrameGeometry.width() + panelHorizontalExpansion;
    const int panelHeight = qMax( 1, geometry.height() - 2 * shadow );
    geometry.setSize( QSize( panelWidth + 2 * shadow, panelHeight + 2 * shadow ) );

    if ( comboBox->layoutDirection() == Qt::RightToLeft )
    {
        geometry.moveRight( comboFrameGeometry.right() + shadow + ( panelHorizontalExpansion - 1 ) );
    }
    else
    {
        geometry.moveLeft( comboFrameGeometry.left() - shadow - ( panelHorizontalExpansion - 1 ) );
    }

    if ( opensAbove )
    {
        const int panelBottom = comboGeometry.top() - FlyoutPopupOffset - 1;
        geometry.moveBottom( panelBottom + shadow );
    }
    else
    {
        const int panelTop = comboGeometry.bottom() + 1 + FlyoutPopupOffset;
        geometry.moveTop( panelTop - shadow );
    }

    // Before its first native show the popup can still report the primary
    // screen even when its ComboBox is on another monitor.  Clamp against the
    // anchor widget's screen so the corrected geometry stays on that monitor.
    QScreen* targetScreen = comboBox->screen();
    if ( !targetScreen )
    {
        targetScreen = popup->screen();
    }
    if ( targetScreen )
    {
        const QRect available = targetScreen->availableGeometry();
        if ( geometry.width() <= available.width() )
        {
            geometry.moveLeft( qBound( available.left(), geometry.left(), available.right() - geometry.width() + 1 ) );
        }
        if ( geometry.height() <= available.height() )
        {
            geometry.moveTop( qBound( available.top(), geometry.top(), available.bottom() - geometry.height() + 1 ) );
        }
    }

    popup->setGeometry( geometry );
    popup->setProperty( ComboBoxPopupOpensAboveProperty, opensAbove );
    popup->setProperty( ComboBoxPopupShadowPositionedProperty, true );
    ensurePopupViewInteractive( comboBox );
    popup->update();
}
