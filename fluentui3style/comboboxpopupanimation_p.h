#pragma once

#include <QObject>

class QComboBox;
class QWidget;
class ComboBoxPopupAnimatorImpl;

inline constexpr const char* ComboBoxPopupOpensAboveProperty =
    "_q_fluent_combo_popup_opens_above";
inline constexpr const char* ComboBoxPopupShadowPositionedProperty =
    "_q_fluent_combo_popup_shadow_positioned";

// FluentUI3Style 的 ComboBox popup 动画控制器。
class ComboBoxPopupAnimator final : public QObject
{
public:
    explicit ComboBoxPopupAnimator( QComboBox* comboBox, QObject* parent = nullptr );
    ~ComboBoxPopupAnimator() override;

    void stop();

    static bool isEnabled( const QComboBox* comboBox );
    static QComboBox* comboBoxForPopup( const QWidget* popup );
    static void positionPopupForShadow( QWidget* popup );

private:
    ComboBoxPopupAnimatorImpl* m_impl;
};
