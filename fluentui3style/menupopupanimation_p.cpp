#include "menupopupanimation_p.h"

#include <QApplication>
#include <QCursor>
#include <QEasingCurve>
#include <QEvent>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QRegion>
#include <QVariantAnimation>
#include <QWidgetAction>
#include <QtMath>

#include "fluentui3styleproperties.h"

class MenuSnapshotOverlay final : public QWidget
{
public:
    MenuSnapshotOverlay( const QPixmap& snapshot, QWidget* parent )
        : QWidget( parent )
        , m_snapshot( snapshot )
    {
        setAttribute( Qt::WA_TransparentForMouseEvents );
        setAttribute( Qt::WA_NoSystemBackground );
        setAttribute( Qt::WA_TranslucentBackground );
    }

protected:
    void paintEvent( QPaintEvent* ) override
    {
        QPainter painter( this );
        painter.drawPixmap( 0, 0, m_snapshot );
    }

private:
    QPixmap m_snapshot;
};

static bool menuContainsWidgetAction( const QMenu* menu )
{
    if ( !menu )
    {
        return false;
    }

    for ( QAction* action : menu->actions() )
    {
        if ( qobject_cast<QWidgetAction*>( action ) )
        {
            return true;
        }
    }

    return false;
}

static bool menuPopupAnimationEnabled( const QMenu* menu )
{
    const QVariant globalValue = qApp->property( MenuPopupAnimationEnabledProperty );
    if ( globalValue.isValid() && !globalValue.toBool() )
    {
        return false;
    }

    if ( menu )
    {
        const QVariant localValue = menu->property( MenuPopupAnimationEnabledProperty );
        if ( localValue.isValid() )
        {
            if ( !localValue.toBool() )
            {
                return false;
            }
        }

        if ( menuContainsWidgetAction( menu ) )
        {
            return false;
        }
    }

    return true;
}

class MenuPopupAnimatorImpl final : public QObject
{
public:
    explicit MenuPopupAnimatorImpl( QMenu* menu, QObject* parent )
        : QObject( parent ? parent : menu )
        , m_menu( menu )
    {
        menu->installEventFilter( this );
        connect( menu,
                 &QMenu::aboutToShow,
                 this,
                 [ this ] { prepareForShow( true ); } );
    }

    ~MenuPopupAnimatorImpl() override { stop( false ); }

    void setDuration( int duration ) { m_duration = qMax( 0, duration ); }

    void stop() { stop( true ); }

    void stop( bool repaintRealMenu )
    {
        restoreNativeMenuEffect();

        if ( m_animation )
        {
            m_animation->stop();
            m_animation->deleteLater();
            m_animation = nullptr;
        }

        restoreMenu( repaintRealMenu );
    }

protected:
    bool eventFilter( QObject* watched, QEvent* event ) override
    {
        if ( watched != m_menu )
        {
            return QObject::eventFilter( watched, event );
        }

        if ( event->type() == QEvent::Show )
        {
            // QMenu 在发送 Show 前已经完成 UI_AnimateMenu 判断，此时可以
            // 立即恢复全局原值，不影响当前 popup。
            restoreNativeMenuEffect();
            offsetMenuForShadow();
        }

        if ( menuContainsWidgetAction( m_menu ) )
        {
            return QObject::eventFilter( watched, event );
        }

        if ( event->type() == QEvent::Show )
        {
            if ( m_preparedForShow
                 || ( menuPopupAnimationEnabled( m_menu )
                      && m_menu->windowType() == Qt::Popup ) )
            {
                if ( !m_preparedForShow )
                {
                    prepareForShow( false );
                }

                if ( m_preparedForShow )
                {
                    animate();
                }
            }
        }
        else if ( event->type() == QEvent::Hide )
        {
            // QMenu 收起时只恢复真实绘制，不播放动画。
            stop( false );
        }
        else if ( m_isOpening )
        {
            switch ( event->type() )
            {
                case QEvent::MouseButtonPress :
                case QEvent::MouseButtonRelease :
                case QEvent::MouseButtonDblClick :
                case QEvent::MouseMove :
                case QEvent::Wheel :
                case QEvent::ContextMenu :
                case QEvent::KeyPress :
                case QEvent::KeyRelease :
                case QEvent::ShortcutOverride :
                    return true;
                default :
                    break;
            }
        }

        return QObject::eventFilter( watched, event );
    }

private:
    void offsetMenuForShadow()
    {
        if ( !m_menu || !m_menu->geometry().isValid() )
        {
            return;
        }

        constexpr int menuBarItemInset = 5;
        constexpr int buttonFrameInset = 2;
        constexpr int anchoredPopupPixelCorrection = 1;

        const QWidget* menuParent = m_menu->parentWidget();
        const int shadow          = FlyoutShadowBorderWidth;
        const bool rtl =
            menuParent
            && menuParent->layoutDirection() == Qt::RightToLeft;

        int horizontalOffset = rtl ? shadow : -shadow;
        int verticalOffset   = 0;

        if ( menuParent && menuParent->inherits( "QMenuBar" ) )
        {
            // Qt aligns the popup window to the complete MenuBar item.  Align
            // the visible menu panel to the item's frame, which is inset 5 px.
            horizontalOffset =
                rtl ? shadow - menuBarItemInset
                    : menuBarItemInset - shadow;
            horizontalOffset +=
                rtl ? anchoredPopupPixelCorrection
                    : -anchoredPopupPixelCorrection;
            const bool opensAbove =
                m_menu->geometry().center().y()
                < menuParent->mapToGlobal(
                      menuParent->rect().center() ).y();
            // verticalOffset = opensAbove ? shadow : -shadow;
            verticalOffset = 0;
        }
        else if ( menuParent
                  && menuParent->inherits( "QAbstractButton" ) )
        {
            // PushButton/ToolButton frames are inset 2 px.  The visible menu
            // edge must line up with that frame rather than the widget rect.
            horizontalOffset =
                rtl ? shadow - buttonFrameInset
                    : buttonFrameInset - shadow;
            horizontalOffset +=
                rtl ? anchoredPopupPixelCorrection
                    : -anchoredPopupPixelCorrection;
            const bool opensAbove =
                m_menu->geometry().center().y()
                < menuParent->mapToGlobal(
                      menuParent->rect().center() ).y();
            // verticalOffset = opensAbove ? shadow : -shadow;
            verticalOffset = 0;
        }
        else if ( menuParent && menuParent->inherits( "QMenu" ) )
        {
            // A submenu can open on either side of its parent menu.
            const bool opensLeft =
                m_menu->geometry().center().x()
                < menuParent->geometry().center().x();
            horizontalOffset = opensLeft ? shadow : -shadow;
            verticalOffset   = -shadow;
        }
        else
        {
            // Context menus have no visual anchor to align with.  Keep the
            // visible panel at Qt's requested popup point by compensating the
            // transparent shadow reserve directly.
            const QPoint anchor = QCursor::pos();
            const bool opensAbove =
                m_menu->geometry().center().y() < anchor.y();
            verticalOffset = opensAbove ? shadow : -shadow;
        }

        m_menu->move( m_menu->pos()
                      + QPoint( horizontalOffset, verticalOffset ) );
    }

    void suppressNativeMenuEffect()
    {
        if ( !qApp || m_nativeMenuEffectSuppressed )
        {
            return;
        }

        m_nativeMenuEffectBeforeSuppression =
            qApp->isEffectEnabled( Qt::UI_AnimateMenu );
        m_nativeMenuEffectSuppressed = true;
        qApp->setEffectEnabled( Qt::UI_AnimateMenu, false );
    }

    void restoreNativeMenuEffect()
    {
        if ( !m_nativeMenuEffectSuppressed )
        {
            return;
        }

        if ( qApp )
        {
            qApp->setEffectEnabled(
                Qt::UI_AnimateMenu,
                m_nativeMenuEffectBeforeSuppression );
        }
        m_nativeMenuEffectSuppressed = false;
    }

    void prepareForShow( bool suppressNativeEffect )
    {
        if ( !m_menu
             || !menuPopupAnimationEnabled( m_menu )
             || m_menu->windowType() != Qt::Popup )
        {
            return;
        }

        if ( m_isOpening
             || m_preparedForShow
             || m_animation
             || m_overlay
             || m_revealMaskActive
             || !m_hiddenChildren.isEmpty() )
        {
            stop( false );
        }

        if ( suppressNativeEffect )
        {
            suppressNativeMenuEffect();
        }

        // QMenu::aboutToShow 发生在原生菜单窗口显示前。先禁止真实菜单
        // 绘制，避免 Qt 5 的按钮菜单先提交一帧完整内容。
        m_preparedForShow = true;
        m_menu->setProperty(
            MenuPopupSuppressPaintingProperty, true );
    }

    QPixmap renderMenuSnapshot()
    {
        if ( !m_menu || m_menu->size().isEmpty() )
        {
            return {};
        }

        const qreal ratio = m_menu->devicePixelRatioF();
        QPixmap snapshot(
            qCeil( m_menu->width() * ratio ),
            qCeil( m_menu->height() * ratio ) );
        snapshot.setDevicePixelRatio( ratio );
        snapshot.fill( Qt::transparent );

        // 临时允许 Style 绘制，但只渲染到离屏 QPixmap；这里不调用
        // QWidget::grab()，避免 Qt 5 将完整菜单刷新到真实 backing store。
        m_menu->setProperty(
            MenuPopupSuppressPaintingProperty, QVariant() );
        {
            QPainter painter( &snapshot );
            m_menu->render(
                &painter,
                QPoint(),
                QRegion( m_menu->rect() ),
                QWidget::DrawWindowBackground
                    | QWidget::DrawChildren );
        }
        m_menu->setProperty(
            MenuPopupSuppressPaintingProperty, true );
        return snapshot;
    }

    void animate()
    {
        if ( !m_menu || m_menu->height() <= 1 || m_menu->width() <= 0 )
        {
            restoreMenu();
            return;
        }

        const QPixmap snapshot = renderMenuSnapshot();
        if ( snapshot.isNull() )
        {
            restoreMenu();
            return;
        }

        const auto children = m_menu->findChildren<QWidget*>( QString(), Qt::FindDirectChildrenOnly );
        for ( QWidget* child : children )
        {
            if ( child->isVisible() )
            {
                m_hiddenChildren.append( child );
                child->hide();
            }
        }

        m_overlay = new MenuSnapshotOverlay( snapshot, m_menu );
        m_overlay->resize( m_menu->size() );

        const bool opensAbove = m_menu->geometry().center().y() < QCursor::pos().y();
        m_startSnapshotY      = opensAbove ? m_menu->height() : -m_menu->height();
        m_originalMenuMask    = m_menu->mask();
        m_hadOriginalMenuMask = !m_originalMenuMask.isEmpty();
        m_revealMaskActive    = true;
        updateRevealMask( 1 );
        m_overlay->move( 0, m_startSnapshotY );
        // Qt 5 会在 show() 时立即提交子窗口首帧。必须先放到动画
        // 起点再显示，否则完整截图会在 (0, 0) 闪现一帧。
        m_overlay->show();
        m_overlay->raise();

        m_isOpening = true;
        m_preparedForShow = false;

        // 立即清掉 Qt 5 可能已经准备好的真实菜单 backing store。此时
        // overlay 位于裁剪区外，所以首帧保持透明。
        m_menu->repaint();

        m_animation = new QVariantAnimation( m_menu );
        m_animation->setStartValue( 0.0 );
        m_animation->setEndValue( 1.0 );
        m_animation->setDuration( m_duration );
        m_animation->setEasingCurve( QEasingCurve::OutCubic );

        connect( m_animation,
                 &QVariantAnimation::valueChanged,
                 this,
                 [ this ]( const QVariant& value )
                 {
                     if ( !m_menu || !m_overlay )
                     {
                         return;
                     }

                     const qreal progress = value.toReal();
                     const int snapshotY  = qRound( m_startSnapshotY * ( 1.0 - progress ) );
                     const int visibleHeight =
                         qBound( 1,
                                 qRound( m_menu->height() * progress ),
                                 m_menu->height() );
                     m_overlay->move( 0, snapshotY );
                     updateRevealMask( visibleHeight );
                 } );

        connect( m_animation,
                 &QVariantAnimation::finished,
                 this,
                 [ this ]
                 {
                     m_animation = nullptr;
                     restoreMenu();
                 } );

        m_animation->start( QAbstractAnimation::DeleteWhenStopped );
    }

    void updateRevealMask( int visibleHeight )
    {
        if ( !m_menu || !m_revealMaskActive )
        {
            return;
        }

        const int height = qBound( 1, visibleHeight, m_menu->height() );
        const int y = m_startSnapshotY > 0
                          ? m_menu->height() - height
                          : 0;
        QRegion revealRegion(
            QRect( 0, y, m_menu->width(), height ) );
        if ( m_hadOriginalMenuMask )
        {
            revealRegion &= m_originalMenuMask;
        }
        m_menu->setMask( revealRegion );
    }

    void restoreMenu( bool repaintRealMenu = true )
    {
        m_isOpening      = false;
        m_preparedForShow = false;

        if ( m_menu )
        {
            m_menu->setProperty( MenuPopupSuppressPaintingProperty, QVariant() );
        }

        for ( const QPointer<QWidget>& child : m_hiddenChildren )
        {
            if ( child )
            {
                child->show();
            }
        }
        m_hiddenChildren.clear();

        if ( m_menu )
        {
            if ( repaintRealMenu )
            {
                // overlay 仍覆盖在最上层时先准备好真实菜单的最终帧，
                // 再移除 overlay，避免动画结束处出现空白尾帧。
                m_menu->repaint();
            }

            if ( m_revealMaskActive )
            {
                if ( m_hadOriginalMenuMask )
                {
                    m_menu->setMask( m_originalMenuMask );
                }
                else
                {
                    m_menu->clearMask();
                }
            }
        }
        m_originalMenuMask    = QRegion();
        m_hadOriginalMenuMask = false;
        m_revealMaskActive    = false;

        if ( m_overlay )
        {
            m_overlay->hide();
            m_overlay->deleteLater();
            m_overlay = nullptr;
        }

        if ( m_menu && repaintRealMenu )
        {
            m_menu->update();
        }
    }

    QPointer<QMenu> m_menu;
    QPointer<MenuSnapshotOverlay> m_overlay;
    QPointer<QVariantAnimation> m_animation;
    QList<QPointer<QWidget>> m_hiddenChildren;
    QRegion m_originalMenuMask;
    int m_startSnapshotY = 0;
    int m_duration       = 400;
    bool m_isOpening     = false;
    bool m_preparedForShow = false;
    bool m_hadOriginalMenuMask = false;
    bool m_revealMaskActive = false;
    bool m_nativeMenuEffectSuppressed = false;
    bool m_nativeMenuEffectBeforeSuppression = false;
};

MenuPopupAnimator::MenuPopupAnimator( QMenu* menu, QObject* parent )
    : QObject( parent ? parent : menu )
    , m_impl( new MenuPopupAnimatorImpl( menu, this ) )
{
}

MenuPopupAnimator::~MenuPopupAnimator() = default;

void MenuPopupAnimator::stop()
{
    m_impl->stop();
}

void MenuPopupAnimator::setDuration( int duration )
{
    m_impl->setDuration( duration );
}
