#include "extabwidget.h"

#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPixmap>
#include <QPropertyAnimation>

ExTabWidget::ExTabWidget( QWidget* parent )
    : QTabWidget( parent )
{
    connect( this, &QTabWidget::currentChanged, this, &ExTabWidget::onCurrentChanged );
}

void ExTabWidget::setVerticalMode( bool vertical )
{
    m_verticalMode = vertical;
}

bool ExTabWidget::verticalMode() const
{
    return m_verticalMode;
}

void ExTabWidget::setSpeed( int duration )
{
    m_duration = qMax( 0, duration );
}

int ExTabWidget::speed() const
{
    return m_duration;
}

void ExTabWidget::setAnimation( QEasingCurve::Type curve )
{
    m_curve = curve;
}

QEasingCurve::Type ExTabWidget::animation() const
{
    return m_curve;
}

void ExTabWidget::onCurrentChanged( int index )
{
    if ( index < 0 || index >= count() )
    {
        return;
    }

    if ( m_animating )
    {
        finishAnimation();
    }

    if ( !isVisible() || m_lastIndex < 0 || m_lastIndex == index || m_duration <= 0 )
    {
        m_lastIndex = index;
        return;
    }

    QWidget* currentWidget = this->widget( m_lastIndex );
    QWidget* nextWidget    = this->widget( index );

    if ( !currentWidget || !nextWidget )
    {
        m_lastIndex = index;
        return;
    }

    QWidget* stack = currentWidget->parentWidget();
    if ( !stack )
    {
        stack = this;
    }

    const QRect area = stack->rect();

    currentWidget->resize( area.size() );
    nextWidget->resize( area.size() );

    QPixmap currentPixmap( area.size() );
    currentPixmap.fill( Qt::transparent );
    currentWidget->render( &currentPixmap );

    QPixmap nextPixmap( area.size() );
    nextPixmap.fill( Qt::transparent );
    nextWidget->render( &nextPixmap );

    auto* currentOverlay = new QLabel( stack );
    currentOverlay->setPixmap( currentPixmap );
    currentOverlay->setScaledContents( false );
    currentOverlay->setGeometry( area );
    currentOverlay->setAttribute( Qt::WA_TransparentForMouseEvents, true );
    currentOverlay->show();
    currentOverlay->raise();

    auto* nextOverlay = new QLabel( stack );
    nextOverlay->setPixmap( nextPixmap );
    nextOverlay->setScaledContents( false );
    nextOverlay->setAttribute( Qt::WA_TransparentForMouseEvents, true );

    const bool forward = index > m_lastIndex;
    QPoint currentEnd;
    QPoint nextStart;
    if ( m_verticalMode )
    {
        const int offset = area.height();
        currentEnd = QPoint( 0, forward ? -offset : offset );
        nextStart  = QPoint( 0, forward ? offset : -offset );
    }
    else
    {
        const int offset = area.width();
        currentEnd = QPoint( forward ? -offset : offset, 0 );
        nextStart  = QPoint( forward ? offset : -offset, 0 );
    }

    nextOverlay->setGeometry( QRect( nextStart, area.size() ) );
    nextOverlay->show();
    nextOverlay->raise();

    nextWidget->hide();

    auto* currentAnimation = new QPropertyAnimation( currentOverlay, "pos", this );
    currentAnimation->setDuration( m_duration );
    currentAnimation->setEasingCurve( m_curve );
    currentAnimation->setStartValue( QPoint( 0, 0 ) );
    currentAnimation->setEndValue( currentEnd );

    auto* nextAnimation = new QPropertyAnimation( nextOverlay, "pos", this );
    nextAnimation->setDuration( m_duration );
    nextAnimation->setEasingCurve( m_curve );
    nextAnimation->setStartValue( nextStart );
    nextAnimation->setEndValue( QPoint( 0, 0 ) );

    auto* group = new QParallelAnimationGroup( this );
    group->addAnimation( currentAnimation );
    group->addAnimation( nextAnimation );

    m_animating      = true;
    m_targetIndex    = index;
    m_currentOverlay = currentOverlay;
    m_nextOverlay    = nextOverlay;
    m_animationGroup = group;

    connect( group, &QParallelAnimationGroup::finished, this, &ExTabWidget::finishAnimation );
    group->start();

    m_lastIndex = index;
}

void ExTabWidget::finishAnimation()
{
    if ( m_animationGroup )
    {
        m_animationGroup->stop();
        m_animationGroup->deleteLater();
        m_animationGroup = nullptr;
    }

    if ( QWidget* current = currentWidget() )
    {
        current->show();
    }

    if ( m_currentOverlay )
    {
        m_currentOverlay->deleteLater();
        m_currentOverlay = nullptr;
    }
    if ( m_nextOverlay )
    {
        m_nextOverlay->deleteLater();
        m_nextOverlay = nullptr;
    }

    m_animating   = false;
    m_targetIndex = -1;
}
