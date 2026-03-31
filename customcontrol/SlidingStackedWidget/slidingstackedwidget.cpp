#include "slidingstackedwidget.h"

#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPixmap>
#include <QPropertyAnimation>

SlidingStackedWidget::SlidingStackedWidget( QWidget* parent )
    : QStackedWidget( parent )
{}

void SlidingStackedWidget::setVerticalMode( bool vertical )
{
    m_verticalMode = vertical;
}

bool SlidingStackedWidget::verticalMode() const
{
    return m_verticalMode;
}

void SlidingStackedWidget::setSpeed( int duration )
{
    m_duration = qMax( 0, duration );
}

int SlidingStackedWidget::speed() const
{
    return m_duration;
}

void SlidingStackedWidget::setAnimation( QEasingCurve::Type curve )
{
    m_curve = curve;
}

QEasingCurve::Type SlidingStackedWidget::animation() const
{
    return m_curve;
}

void SlidingStackedWidget::setCurrentIndex( int index )
{
    slideToIndex( index );
}

void SlidingStackedWidget::setCurrentWidget( QWidget* widget )
{
    slideToIndex( indexOf( widget ) );
}

void SlidingStackedWidget::slideToIndex( int index )
{
    if ( index < 0 || index >= count() )
    {
        return;
    }

    if ( m_animating )
    {
        m_pendingIndex = index;
        finishAnimation();
        return;
    }

    const int current = QStackedWidget::currentIndex();
    if ( current == index )
    {
        QStackedWidget::setCurrentIndex( index );
        return;
    }

    if ( !isVisible() || current < 0 || m_duration <= 0 )
    {
        QStackedWidget::setCurrentIndex( index );
        return;
    }

    QWidget* currentWidget = this->currentWidget();
    QWidget* nextWidget    = this->widget( index );
    if ( !currentWidget || !nextWidget )
    {
        QStackedWidget::setCurrentIndex( index );
        return;
    }

    const QRect area = rect();
    currentWidget->resize( area.size() );
    nextWidget->resize( area.size() );

    QPixmap currentPixmap( area.size() );
    currentPixmap.fill( Qt::transparent );
    currentWidget->render( &currentPixmap );

    QPixmap nextPixmap( area.size() );
    nextPixmap.fill( Qt::transparent );
    nextWidget->render( &nextPixmap );

    auto* currentOverlay = new QLabel( this );
    currentOverlay->setPixmap( currentPixmap );
    currentOverlay->setScaledContents( false );
    currentOverlay->setGeometry( area );
    currentOverlay->setAttribute( Qt::WA_TransparentForMouseEvents, true );
    currentOverlay->show();
    currentOverlay->raise();

    auto* nextOverlay = new QLabel( this );
    nextOverlay->setPixmap( nextPixmap );
    nextOverlay->setScaledContents( false );
    nextOverlay->setAttribute( Qt::WA_TransparentForMouseEvents, true );

    const bool forward = index > current;
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

    currentWidget->hide();
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

    connect( group, &QParallelAnimationGroup::finished, this, &SlidingStackedWidget::finishAnimation );
    group->start();
}

void SlidingStackedWidget::finishAnimation()
{
    if ( m_animationGroup )
    {
        m_animationGroup->stop();
        m_animationGroup->deleteLater();
        m_animationGroup = nullptr;
    }

    if ( m_targetIndex >= 0 && m_targetIndex < count() )
    {
        QStackedWidget::setCurrentIndex( m_targetIndex );
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

    if ( m_pendingIndex >= 0 && m_pendingIndex != currentIndex() )
    {
        const int nextIndex = m_pendingIndex;
        m_pendingIndex      = -1;
        slideToIndex( nextIndex );
    }
    else
    {
        m_pendingIndex = -1;
    }
}
