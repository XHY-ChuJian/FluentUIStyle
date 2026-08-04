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
#include <QVariant>
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

static void clearPopupViewMasks( const QPointer<QAbstractItemView>& popupView )
{
    // 必须用已持有的 QPointer：析构期 comboBox->view() 可能返回悬空裸指针。
    if ( !popupView )
    {
        return;
    }

    popupView->clearMask();
    if ( QWidget* viewport = popupView->viewport() )
    {
        viewport->clearMask();
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
    // 该 Qt 私有属性选择“当前项对齐 ComboBox”的 popup 行为；
    // 开启时不叠加任何 FluentUI3Style 自定义动画。
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
        , m_view( comboBox ? comboBox->view() : nullptr )
    {
        if ( m_view )
        {
            attachPopup( m_view->window() );
        }
    }

    ~ComboBoxPopupAnimatorImpl() override { stop(); }

    void stop()
    {
        removeApplicationEventFilter();
        if ( m_popup )
        {
            m_popup->removeEventFilter( this );
            if ( hasActiveAnimationWork() )
            {
                stopAnimation( m_popup );
            }
        }
        resetAnimationState();
    }

protected:
    bool eventFilter( QObject* watched, QEvent* event ) override
    {
        if ( !m_comboBox || !m_popup )
        {
            return QObject::eventFilter( watched, event );
        }

        // popup 正在销毁：只摘过滤器，绝不再碰 view/mask。
        if ( watched == m_popup
             && ( event->type() == QEvent::Destroy || event->type() == QEvent::DeferredDelete ) )
        {
            m_popupShown = false;
            resetAnimationState();
            removeApplicationEventFilter();
            m_popup.clear();
            return QObject::eventFilter( watched, event );
        }

        const bool animEnabled = comboBoxPopupAnimationEnabled( m_comboBox );

        // 普通下拉模式校正阴影定位；居中 popup 模式只校正
        // 当前项的视觉中心，动画相关逻辑仍由 animEnabled 完全禁用。
        if ( watched == m_popup && event->type() == QEvent::Show )
        {
            if ( !m_popupShown )
            {
                m_popupShown = true;
                if ( !m_view && m_comboBox )
                {
                    m_view = m_comboBox->view();
                }
                ComboBoxPopupAnimator::positionPopupForShadow( m_popup );
                if ( animEnabled )
                {
                    animatePopup( m_popup );
                }
            }
            return QObject::eventFilter( watched, event );
        }

        if ( watched == m_popup && event->type() == QEvent::Hide )
        {
            m_popupShown = false;
            m_popup->setProperty( ComboBoxPopupShadowPositionedProperty, QVariant() );
            if ( hasActiveAnimationWork() )
            {
                stopAnimation( m_popup );
            }
            removeApplicationEventFilter();
            return QObject::eventFilter( watched, event );
        }

        if ( !animEnabled )
        {
            // 运行中被关掉动画时收尾一次，其余事件不再进入动画分支。
            if ( hasActiveAnimationWork() )
            {
                stopAnimation( m_popup );
                removeApplicationEventFilter();
            }
            return QObject::eventFilter( watched, event );
        }

        if ( event->type() == QEvent::ApplicationDeactivate
             || ( watched == m_comboBox->window() && ( event->type() == QEvent::Hide || event->type() == QEvent::Close ) ) )
        {
            if ( hasActiveAnimationWork() )
            {
                stopAnimation( m_popup );
            }
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
    bool hasActiveAnimationWork() const
    {
        return m_isOpening || m_viewDetached || m_animationGroup;
    }

    void resetAnimationState()
    {
        m_isOpening     = false;
        m_viewDetached  = false;
        m_animationGroup = nullptr;
    }

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
        if ( !m_view && m_comboBox )
        {
            m_view = m_comboBox->view();
        }
        auto* popupView           = m_view.data();
        auto* popupLayout         = popup->layout();
        auto* popupBoxLayout      = qobject_cast<QBoxLayout*>( popupLayout );
        const int viewLayoutIndex = popupLayout && popupView ? popupLayout->indexOf( popupView ) : -1;
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

        // 动画开始前清掉残留 mask；过程中由 updateViewClip 临时裁剪。
        clearPopupViewMasks( m_view );

        popupLayout->removeWidget( popupView );
        m_viewDetached = true;
        return true;
    }

    void startPopupAnimation( QWidget* popup )
    {
        auto* popupView = m_view.data();
        if ( !popupView )
        {
            return;
        }

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
                     if ( !m_view )
                     {
                         return;
                     }
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
                     updateViewClip( popup, m_view );
                 } );

        auto* viewAnimation = new QPropertyAnimation( popupView, "pos", m_animationGroup );
        viewAnimation->setStartValue( startViewPosition );
        viewAnimation->setEndValue( m_finalViewPosition );
        viewAnimation->setDuration( AnimationDuration );
        viewAnimation->setEasingCurve( QEasingCurve::OutCubic );
        connect( viewAnimation,
                 &QPropertyAnimation::valueChanged,
                 this,
                 [ this, popup, popupView ]( const QVariant& )
                 {
                     if ( m_view )
                     {
                         updateViewClip( popup, popupView );
                     }
                 } );

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
    }

    void restorePopup( QWidget* popup )
    {
        if ( popup && m_finalGeometry.isValid() )
        {
            popup->setFixedHeight( m_finalGeometry.height() );
            popup->move( m_finalGeometry.topLeft() );
        }

        if ( !m_viewDetached || !m_view || !m_popupLayout )
        {
            m_viewDetached = false;
            clearPopupViewMasks( m_view );
            return;
        }

        auto* popupView = m_view.data();
        popupView->move( m_finalViewPosition );
        // 动画结束：清掉裁剪 mask，恢复可点选。
        clearPopupViewMasks( m_view );

        auto* boxLayout = qobject_cast<QBoxLayout*>( m_popupLayout.data() );
        if ( !boxLayout )
        {
            m_viewDetached = false;
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
        if ( !popup )
        {
            return;
        }
        popup->setMinimumHeight( 0 );
        popup->setMaximumHeight( QWIDGETSIZE_MAX );
    }

    QPointer<QComboBox> m_comboBox;
    QPointer<QAbstractItemView> m_view;
    QPointer<QWidget> m_popup;
    QPointer<QParallelAnimationGroup> m_animationGroup;
    QPointer<QLayout> m_popupLayout;
    QRect m_finalGeometry;
    QPoint m_finalViewPosition;
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
    if ( !popup || !comboBox )
    {
        return;
    }

    const bool useCenteredPopup =
        qApp && qApp->property( "_q_scrollHint_center" ).toBool();
    if ( !useCenteredPopup && popup->property( ComboBoxPopupShadowPositionedProperty ).toBool() )
    {
        return;
    }

    QRect geometry = popup->geometry();
    if ( !geometry.isValid() )
    {
        return;
    }

    // 与 QComboBox::showPopup() 相同：锚点用角点 mapToGlobal，不用
    // topLeft + size()。GraphicsView 缩放时后者会把未缩放高度混进 Y。
    const QPoint above = comboBox->mapToGlobal( QPoint( 0, 0 ) );
    const QPoint below = comboBox->mapToGlobal( QPoint( 0, comboBox->height() ) );
    const int comboBoxCenterY = ( above.y() + below.y() ) / 2;
    const bool opensAbove = geometry.center().y() < ( above.y() + below.y() ) / 2;

    // QGraphicsProxyWidget 里 Qt 已在同一变换空间算好 popup；再改 geometry
    // 容易让绘制和命中错位。这里只记录方向，保留 showPopup() 结果。
    if ( comboBoxGraphicsProxy( comboBox ) )
    {
        if ( QGraphicsProxyWidget* popupProxy = popup->graphicsProxyWidget() )
        {
            if ( QGraphicsProxyWidget* comboProxy = comboBoxGraphicsProxy( comboBox ) )
            {
                popupProxy->setZValue( comboProxy->zValue() + 1.0 );
            }
        }
        if ( !useCenteredPopup )
        {
            popup->setProperty( ComboBoxPopupOpensAboveProperty, opensAbove );
            popup->setProperty( ComboBoxPopupShadowPositionedProperty, true );
        }
        popup->update();
        return;
    }

    if ( useCenteredPopup )
    {
        // Qt 用 currentItemRect 对齐当前项，但它不知道 popup 窗口内部
        // 还有一圈透明阴影和内边距，所以内容视觉上会偏下。
        // 直接对齐当前项与 ComboBox 的实际中心，自动包含阴影、
        // layout margin 和不同的 item 高度。这里只移动窗口，不创建动画。
        QAbstractItemView* view = comboBox->view();
        if ( view && view->viewport() && view->currentIndex().isValid() )
        {
            const QRect currentItemRect = view->visualRect( view->currentIndex() );
            if ( currentItemRect.isValid() )
            {
                const int currentItemCenterY =
                    view->viewport()->mapToGlobal( currentItemRect.center() ).y();
                geometry.translate( 0, comboBoxCenterY - currentItemCenterY );
            }
        }
    }
    else
    {
        // 普通下拉模式：宽度/水平位置已由
        // SC_ComboBoxListBoxPopup + showPopup() 定好，这里只补
        // FlyoutPopupOffset 与阴影预留。
        const int shadow = FlyoutShadowBorderWidth;
        if ( opensAbove )
        {
            geometry.moveBottom( above.y() - FlyoutPopupOffset - 1 + shadow );
        }
        else
        {
            geometry.moveTop( below.y() + FlyoutPopupOffset - shadow );
        }
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
    if ( !useCenteredPopup )
    {
        popup->setProperty( ComboBoxPopupOpensAboveProperty, opensAbove );
        popup->setProperty( ComboBoxPopupShadowPositionedProperty, true );
    }
    popup->update();
}
