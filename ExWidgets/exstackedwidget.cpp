#include "exstackedwidget.h"

#include <QLabel>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPixmap>
#include <QPropertyAnimation>

ExStackedWidget::ExStackedWidget( QWidget* parent )
    : QStackedWidget( parent )
{}

void ExStackedWidget::setVerticalMode( bool vertical )
{
    m_verticalMode = vertical;
}

bool ExStackedWidget::verticalMode() const
{
    return m_verticalMode;
}

void ExStackedWidget::setSpeed( int duration )
{
    m_duration = qMax( 0, duration );
}

int ExStackedWidget::speed() const
{
    return m_duration;
}

void ExStackedWidget::setAnimation( QEasingCurve::Type curve )
{
    m_curve = curve;
}

QEasingCurve::Type ExStackedWidget::animation() const
{
    return m_curve;
}

int ExStackedWidget::addWidget(QWidget *w)
{
    w->setBackgroundRole(QPalette::Base);
    return QStackedWidget::addWidget(w);
}

void ExStackedWidget::setCurrentIndex( int index )
{
    slideToIndex( index );
}

void ExStackedWidget::setCurrentWidget( QWidget* widget )
{
    slideToIndex( indexOf( widget ) );
}

void ExStackedWidget::slideToIndex( int index )
{
    if ( index < 0 || index >= count() )
    {
        return;
    }

    if ( m_animating )
    {
        // Do not snap an in-flight animation to its end. Queue the latest
        // destination and start it after the current transition completes.
        m_pendingIndex = index == m_targetIndex ? -1 : index;
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

    // QStackedWidget is a QFrame: its pages occupy contentsRect(), which may
    // start at a non-zero position when a frame is present. Using rect() here
    // makes the overlays jump when the real page is restored by the layout.
    const QRect area = contentsRect();
    if ( area.isEmpty() )
    {
        QStackedWidget::setCurrentIndex( index );
        return;
    }

    currentWidget->setGeometry( area );
    nextWidget->setGeometry( area );

    const auto renderPage = [ &area ]( QWidget* page )
    {
        QPixmap pixmap( area.size() );
        QPainter painter( &pixmap );
        painter.fillRect( pixmap.rect(), page->palette().brush( page->backgroundRole() ) );
        page->render( &painter, QPoint(), QRegion(), QWidget::DrawChildren );
        return pixmap;
    };

    const QPixmap currentPixmap = renderPage( currentWidget );
    const QPixmap nextPixmap    = renderPage( nextWidget );

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
    const QPoint origin = area.topLeft();
    QPoint currentEnd;
    QPoint nextStart;
    if ( m_verticalMode )
    {
        const int offset = area.height();
        currentEnd = origin + QPoint( 0, forward ? -offset : offset );
        nextStart  = origin + QPoint( 0, forward ? offset : -offset );
    }
    else
    {
        const int offset = area.width();
        currentEnd = origin + QPoint( forward ? -offset : offset, 0 );
        nextStart  = origin + QPoint( forward ? offset : -offset, 0 );
    }

    nextOverlay->setGeometry( QRect( nextStart, area.size() ) );
    nextOverlay->show();
    nextOverlay->raise();

    currentWidget->hide();
    nextWidget->hide();

    auto* currentAnimation = new QPropertyAnimation( currentOverlay, "pos", this );
    currentAnimation->setDuration( m_duration );
    currentAnimation->setEasingCurve( m_curve );
    currentAnimation->setStartValue( origin );
    currentAnimation->setEndValue( currentEnd );

    auto* nextAnimation = new QPropertyAnimation( nextOverlay, "pos", this );
    nextAnimation->setDuration( m_duration );
    nextAnimation->setEasingCurve( m_curve );
    nextAnimation->setStartValue( nextStart );
    nextAnimation->setEndValue( origin );

    auto* group = new QParallelAnimationGroup( this );
    group->addAnimation( currentAnimation );
    group->addAnimation( nextAnimation );

    m_animating      = true;
    m_targetIndex    = index;
    m_currentOverlay = currentOverlay;
    m_nextOverlay    = nextOverlay;
    m_animationGroup = group;

    connect( group, &QParallelAnimationGroup::finished, this, &ExStackedWidget::finishAnimation );
    group->start();
}

void ExStackedWidget::finishAnimation()
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
        current->setGeometry( contentsRect() );
        current->show();
    }

    if ( m_currentOverlay )
    {
        m_currentOverlay->hide();
        m_currentOverlay->deleteLater();
        m_currentOverlay = nullptr;
    }
    if ( m_nextOverlay )
    {
        m_nextOverlay->hide();
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
