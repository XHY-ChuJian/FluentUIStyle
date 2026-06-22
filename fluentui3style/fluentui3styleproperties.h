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
