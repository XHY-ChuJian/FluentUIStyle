#ifndef FLUENTUI3STYLEPROPERTIES_H
#define FLUENTUI3STYLEPROPERTIES_H

enum SpinBoxButtonLayout
{
    ArrowsHorizontalRight,
    ArrowsVertical,
    ArrowsHorizontalSides,
    PlusMinusHorizontalSides
};

[[maybe_unused]] constexpr const char *TabBarStyleProperty = "tabBarStyle";
enum TabBarStyle
{
    Capsule = 1,
    Pivot_Grow,
    Pivot_Slide,
    Pivot_Stretch,
    PillTabs,
    Segmented_Slide,
    Segmented_Fade,
    Navigation,
    Segmented_WinUI3
};

[[maybe_unused]] constexpr const char *ProgressBarThicknessProperty = "progressBarThickness";
[[maybe_unused]] constexpr const char *ProgressBarStyleProperty = "progressBarStyle";
enum ProgressBarStyle
{
    ProgressBarThin = 0,
    ProgressBarThick = 1,
    ProgressBarRing = 2
};

[[maybe_unused]] constexpr const char *DialStyleProperty = "dialStyle";
[[maybe_unused]] constexpr const char *DialDrawValueProperty = "dialDrawValue";
[[maybe_unused]] constexpr const char *SliderValueTipProperty = "showValueTip";
[[maybe_unused]] constexpr const char *SliderValueTipHooksProperty = "sliderValueTipHooks";
[[maybe_unused]] constexpr const char *SliderValueTipLabelProperty = "sliderValueTipLabel";
// 可同时设置在 qApp 和单个 QComboBox 上。qApp 属性是总开关；
// 全局开启时，单个 QComboBox 可通过局部属性选择是否启用动画。
// 仅控制普通 QComboBox 的展开动画；ExComboBox 不受该属性控制。
[[maybe_unused]] constexpr const char *ComboBoxPopupDropDownAnimationEnabledProperty =
    "comboBoxPopupDropDownAnimationEnabled";
// 可同时设置在 qApp 和单个 QMenu 上，规则同 ComboBox。
[[maybe_unused]] constexpr const char *MenuPopupAnimationEnabledProperty = "menuPopupAnimationEnabled";
// Flyout 阴影绘制在透明顶层窗口内部；布局和弹出位置必须共用同一预留宽度。
[[maybe_unused]] constexpr int FlyoutShadowBorderWidth = 4;
[[maybe_unused]] constexpr int FlyoutPopupOffset = 3;
// QComboBox 的可见主题边框相对控件 rect 左右各收进 2 px。
[[maybe_unused]] constexpr int ComboBoxControlFrameHorizontalInset = 2;
enum DialStyle
{
    DialDots = 1,
    DialRing = 2,
    DialThumb = 3
};

[[maybe_unused]] constexpr const char *ButtonAccentStyleProperty = "accent";
[[maybe_unused]] constexpr const char *SwitchStyleProperty = "isSwitchButton";
[[maybe_unused]] constexpr const char *NavigationViewStyleProperty = "navigationViewIndicator";
[[maybe_unused]] constexpr const char *NoRoundedCorners = "noRoundedCorners";
[[maybe_unused]] constexpr const char *SegmentedBackgroundColorProperty = "segmentedBackgroundColor";
[[maybe_unused]] constexpr const char *SegmentedBackgroundColorDarkProperty = "segmentedBackgroundColorDark";
[[maybe_unused]] constexpr const char *SegmentedSelectedColorProperty = "segmentedSelectedColor";
[[maybe_unused]] constexpr const char *SegmentedSelectedColorDarkProperty = "segmentedSelectedColorDark";
[[maybe_unused]] constexpr const char *SegmentedHoverColorProperty = "segmentedHoverColor";
[[maybe_unused]] constexpr const char *SegmentedHoverColorDarkProperty = "segmentedHoverColorDark";
[[maybe_unused]] constexpr const char *SegmentedPressedColorProperty = "segmentedPressedColor";
[[maybe_unused]] constexpr const char *SegmentedPressedColorDarkProperty = "segmentedPressedColorDark";
[[maybe_unused]] constexpr const char *SegmentedSemiRoundProperty = "segmentedSemiRound";

#endif // FLUENTUI3STYLEPROPERTIES_H
