#pragma once

#include <QComboBox>
#include <QPointer>
#include <QRegion>

#include "exwidgets_global.h"

class QHideEvent;
class QPaintEvent;
class QVariantAnimation;

// 可直接接管 showPopup()/hidePopup() 的动画版 QComboBox。
// 它不受 FluentUI3Style 的全局或局部 ComboBox 动画属性控制。
class EXWIDGETS_EXPORT ExComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit ExComboBox( QWidget* parent = nullptr );
    ~ExComboBox() override;

    void showPopup() override;
    void hidePopup() override;

protected:
    void hideEvent( QHideEvent* event ) override;
    void paintEvent( QPaintEvent* event ) override;

private:
    enum class AnimationState
    {
        Idle,
        Opening,
        Closing
    };

    QWidget* popupContainer() const;
    bool startAnimation( QWidget* popup, AnimationState state );
    void setAnimationProgress( QWidget* popup, qreal progress );
    void finishAnimation( QWidget* popup );
    void stopAnimation();
    void restorePopup( QWidget* popup );
    void updateViewClip( QWidget* popup, QWidget* popupView );
    void hidePopupImmediately();

    QPointer<QWidget> m_popup;
    QPointer<QVariantAnimation> m_animation;
    QRect m_finalGeometry;
    QPoint m_finalViewPosition;
    QPoint m_collapsedViewPosition;
    QRegion m_originalViewMask;
    int m_viewBottomMargin = 0;
    int m_viewLayoutIndex  = -1;
    bool m_viewDetached    = false;
    bool m_opensAbove      = false;
    AnimationState m_state = AnimationState::Idle;
};
