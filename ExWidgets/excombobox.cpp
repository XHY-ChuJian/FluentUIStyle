#include "excombobox.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QBoxLayout>
#include <QEasingCurve>
#include <QGuiApplication>
#include <QHideEvent>
#include <QStyleOptionComboBox>
#include <QStylePainter>
#include <QVariantAnimation>
#include <QWidget>
#include <QtMath>

namespace {
constexpr const char* PopupOpensAboveProperty = "_q_fluent_combo_popup_opens_above";
constexpr int AnimationDuration               = 300;
}  // namespace

ExComboBox::ExComboBox( QWidget* parent )
    : QComboBox( parent )
{
    connect( qApp,
             &QGuiApplication::applicationStateChanged,
             this,
             [ this ]( Qt::ApplicationState state )
             {
                 if ( state != Qt::ApplicationActive && m_state != AnimationState::Idle )
                 {
                     stopAnimation();
                     hidePopupImmediately();
                 }
             } );
}

ExComboBox::~ExComboBox()
{
    stopAnimation();
}

void ExComboBox::showPopup()
{
    if ( m_state == AnimationState::Opening )
    {
        return;
    }

    if ( m_state == AnimationState::Closing )
    {
        stopAnimation();
        hidePopupImmediately();
    }

    const bool oldEffect = qApp->isEffectEnabled( Qt::UI_AnimateCombo );
    qApp->setEffectEnabled( Qt::UI_AnimateCombo, false );
    QComboBox::showPopup();
    qApp->setEffectEnabled( Qt::UI_AnimateCombo, oldEffect );

    QWidget* popup = popupContainer();
    if ( popup && popup->isVisible() )
    {
        startAnimation( popup, AnimationState::Opening );
    }
}

void ExComboBox::hidePopup()
{
    // 展开完成前不允许关闭，避免破坏正在变化的布局和 Geometry。
    if ( m_state != AnimationState::Idle )
    {
        return;
    }

    QWidget* popup = popupContainer();
    const bool canAnimate =
        popup && popup->isVisible() && window()->isVisible() && QGuiApplication::applicationState() == Qt::ApplicationActive;

    if ( canAnimate && startAnimation( popup, AnimationState::Closing ) )
    {
        return;
    }

    QComboBox::hidePopup();
}

void ExComboBox::hideEvent( QHideEvent* event )
{
    if ( m_state != AnimationState::Idle )
    {
        stopAnimation();
        hidePopupImmediately();
    }

    QComboBox::hideEvent( event );
}

void ExComboBox::paintEvent( QPaintEvent* )
{
    QStyleOptionComboBox opt;
    initStyleOption( &opt );
    if ( m_state == AnimationState::Closing )
    {
        opt.state &= ~QStyle::State_On;
    }

    QStylePainter painter( this );
    painter.setPen( palette().color( QPalette::Text ) );

    // draw the combobox frame, focusrect and selected etc.
    painter.drawComplexControl( QStyle::CC_ComboBox, opt );

#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    if ( currentIndex() < 0 && !placeholderText().isEmpty() )
    {
        opt.palette.setBrush( QPalette::ButtonText, opt.palette.placeholderText() );
        opt.currentText = placeholderText();
    }
#endif

    // draw the icon and text
    painter.drawControl( QStyle::CE_ComboBoxLabel, opt );
}

QWidget* ExComboBox::popupContainer() const
{
    QAbstractItemView* popupView = view();
    QWidget* popup               = popupView ? popupView->window() : nullptr;
    return popup && popup->inherits( "QComboBoxPrivateContainer" ) ? popup : nullptr;
}

bool ExComboBox::startAnimation( QWidget* popup, AnimationState state )
{
    auto* popupView           = view();
    auto* popupLayout         = popup->layout();
    auto* popupBoxLayout      = qobject_cast<QBoxLayout*>( popupLayout );
    const int viewLayoutIndex = popupLayout ? popupLayout->indexOf( popupView ) : -1;
    if ( !popupView || !popupBoxLayout || viewLayoutIndex < 0 )
    {
        return false;
    }

    m_popup         = popup;
    m_state         = state;
    m_finalGeometry = popup->geometry();

    if ( popup->property( PopupOpensAboveProperty ).isValid() )
    {
        m_opensAbove = popup->property( PopupOpensAboveProperty ).toBool();
    }
    else
    {
        const QPoint comboCenter = mapToGlobal( rect().center() );
        m_opensAbove             = m_finalGeometry.center().y() < comboCenter.y();
    }

    if ( state == AnimationState::Opening )
    {
        popup->setProperty( PopupOpensAboveProperty, m_opensAbove );
        popup->update();
    }
    else
    {
        update();
    }

    m_viewLayoutIndex       = viewLayoutIndex;
    m_finalViewPosition     = popupView->pos();
    m_viewBottomMargin      = qMax( 0, m_finalGeometry.height() - ( popupView->geometry().bottom() + 1 ) );
    m_originalViewMask      = popupView->mask();
    m_collapsedViewPosition = m_finalViewPosition;
    if ( m_opensAbove )
    {
        m_collapsedViewPosition.setY( 1 );
    }
    else
    {
        m_collapsedViewPosition.ry() -= popupView->height();
    }

    popupLayout->removeWidget( popupView );
    m_viewDetached            = true;
    const bool opening        = state == AnimationState::Opening;
    const qreal startProgress = opening ? 0.0 : 1.0;
    const qreal endProgress   = opening ? 1.0 : 0.0;

    setAnimationProgress( popup, startProgress );

    m_animation = new QVariantAnimation( popup );
    m_animation->setStartValue( startProgress );
    m_animation->setEndValue( endProgress );
    m_animation->setDuration( AnimationDuration );
    m_animation->setEasingCurve( opening ? QEasingCurve::OutCubic : QEasingCurve::InCubic );

    connect( m_animation,
             &QVariantAnimation::valueChanged,
             this,
             [ this, popup ]( const QVariant& value ) { setAnimationProgress( popup, value.toReal() ); } );
    connect( m_animation, &QVariantAnimation::finished, this, [ this, popup ] { finishAnimation( popup ); } );

    m_animation->start( QAbstractAnimation::DeleteWhenStopped );
    return true;
}

void ExComboBox::setAnimationProgress( QWidget* popup, qreal progress )
{
    const int finalHeight = m_finalGeometry.height();
    const int height      = qRound( 1 + ( finalHeight - 1 ) * progress );
    popup->setFixedHeight( height );

    if ( m_opensAbove )
    {
        popup->move( m_finalGeometry.x(), m_finalGeometry.bottom() - height + 1 );
    }
    else
    {
        popup->move( m_finalGeometry.topLeft() );
    }

    view()->move( m_collapsedViewPosition + ( m_finalViewPosition - m_collapsedViewPosition ) * progress );
    updateViewClip( popup, view() );
}

void ExComboBox::finishAnimation( QWidget* popup )
{
    const bool closing = m_state == AnimationState::Closing;
    m_animation        = nullptr;

    if ( closing )
    {
        hidePopupImmediately();
    }

    restorePopup( popup );
    m_state = AnimationState::Idle;
}

void ExComboBox::stopAnimation()
{
    if ( m_animation )
    {
        disconnect( m_animation, nullptr, this, nullptr );
        m_animation->stop();
        m_animation = nullptr;
    }

    if ( m_popup )
    {
        restorePopup( m_popup );
    }
    m_state = AnimationState::Idle;
}

void ExComboBox::restorePopup( QWidget* popup )
{
    if ( m_finalGeometry.isValid() )
    {
        popup->setFixedHeight( m_finalGeometry.height() );
        popup->move( m_finalGeometry.topLeft() );
    }

    if ( m_viewDetached )
    {
        if ( auto* boxLayout = qobject_cast<QBoxLayout*>( popup->layout() ) )
        {
            auto* popupView = view();
            popupView->move( m_finalViewPosition );
            if ( m_originalViewMask.isEmpty() )
            {
                popupView->clearMask();
            }
            else
            {
                popupView->setMask( m_originalViewMask );
            }
            boxLayout->insertWidget( m_viewLayoutIndex, popupView );
            m_viewDetached = false;
        }
    }

    popup->setMinimumHeight( 0 );
    popup->setMaximumHeight( QWIDGETSIZE_MAX );
}

void ExComboBox::updateViewClip( QWidget* popup, QWidget* popupView )
{
    if ( !popup || !popupView || !m_viewDetached )
    {
        return;
    }

    // The popup rectangle includes the transparent shadow reserve.  Reveal
    // the moving view only inside the final layout's actual content area.
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

void ExComboBox::hidePopupImmediately()
{
    const bool oldEffect = qApp->isEffectEnabled( Qt::UI_AnimateCombo );
    qApp->setEffectEnabled( Qt::UI_AnimateCombo, false );
    QComboBox::hidePopup();
    qApp->setEffectEnabled( Qt::UI_AnimateCombo, oldEffect );
}
