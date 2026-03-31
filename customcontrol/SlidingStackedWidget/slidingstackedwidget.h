#pragma once

#include <QEasingCurve>
#include <QPointer>
#include <QStackedWidget>

class QLabel;
class QParallelAnimationGroup;

class SlidingStackedWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit SlidingStackedWidget( QWidget* parent = nullptr );

    void setVerticalMode( bool vertical );
    bool verticalMode() const;

    void setSpeed( int duration );
    int speed() const;

    void setAnimation( QEasingCurve::Type curve );
    QEasingCurve::Type animation() const;

public slots:
    void setCurrentIndex( int index );
    void setCurrentWidget( QWidget* widget );

private:
    void slideToIndex( int index );
    void finishAnimation();

private:
    bool m_verticalMode{ false };
    int m_duration{ 250 };
    QEasingCurve::Type m_curve{ QEasingCurve::OutCubic };
    bool m_animating{ false };
    int m_pendingIndex{ -1 };
    int m_targetIndex{ -1 };
    QPointer<QLabel> m_currentOverlay;
    QPointer<QLabel> m_nextOverlay;
    QPointer<QParallelAnimationGroup> m_animationGroup;
};
