#pragma once

#include <QEasingCurve>
#include <QPointer>
#include <QTabWidget>
#include "exwidgets_global.h"

class QLabel;
class QParallelAnimationGroup;

class EXWIDGETS_EXPORT ExTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit ExTabWidget( QWidget* parent = nullptr );

    void setVerticalMode( bool vertical );
    bool verticalMode() const;

    void setSpeed( int duration );
    int speed() const;

    void setAnimation( QEasingCurve::Type curve );
    QEasingCurve::Type animation() const;

private slots:
    void onCurrentChanged( int index );
    void finishAnimation();

private:
    bool m_verticalMode{ false };
    int m_duration{ 250 };
    QEasingCurve::Type m_curve{ QEasingCurve::OutCubic };
    bool m_animating{ false };
    int m_lastIndex{ -1 };
    int m_targetIndex{ -1 };
    QPointer<QLabel> m_currentOverlay;
    QPointer<QLabel> m_nextOverlay;
    QPointer<QParallelAnimationGroup> m_animationGroup;
};
