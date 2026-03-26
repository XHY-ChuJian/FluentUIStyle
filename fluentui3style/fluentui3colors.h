#ifndef FLUENTUI3COLORS_H
#define FLUENTUI3COLORS_H

#include <QColor>
#include <array>

enum WINUI3Color
{
    subtleHighlightColor,      // Subtle highlight based on alpha used for hovered elements
    subtlePressedColor,        // Subtle highlight based on alpha used for pressed elements
    frameColorLight,           // Color of frame around flyouts and controls except for Checkbox and Radiobutton
    frameColorStrong,          // Color of frame around Checkbox and Radiobuttons (normal and hover)
    frameColorStrongDisabled,  // Color of frame around Checkbox and Radiobuttons (pressed and disabled)
    controlStrongFill,         // Color of controls with strong filling such as the right side of a slider
    controlStrokeSecondary,
    controlStrokePrimary,
    menuPanelFill,                   // Color of menu panel
    controlStrokeOnAccentSecondary,  // Color of frame around Buttons in accent color
    controlFillSolid,                // Color for solid fill
    surfaceStroke,                   // Color of MDI window frames
    focusFrameInnerStroke,
    focusFrameOuterStroke,
    fillControlDefault,         // button default color (alpha)
    fillControlSecondary,       // button hover color (alpha)
    fillControlTertiary,        // button pressed color (alpha)
    fillControlDisabled,        // button disabled color (alpha)
    fillControlInputActive,     // input active
    fillControlAltSecondary,    // checkbox/RadioButton default color (alpha)
    fillControlAltTertiary,     // checkbox/RadioButton hover color (alpha)
    fillControlAltQuarternary,  // checkbox/RadioButton pressed color (alpha)
    fillControlAltDisabled,     // checkbox/RadioButton disabled color (alpha)
    fillAccentDefault,          // button default color (alpha)
    fillAccentSecondary,        // button hover color (alpha)
    fillAccentTertiary,         // button pressed color (alpha)
    fillAccentDisabled,         // button disabled color (alpha)
    textPrimary,                // text of default/hovered control
    textSecondary,              // text of pressed control
    textDisabled,               // text of disabled control
    textOnAccentPrimary,        // text of default/hovered control on accent color
    textOnAccentSecondary,      // text of pressed control on accent color
    textOnAccentDisabled,       // text of disabled control on accent color
    dividerStrokeDefault,       // divider color (alpha)
    switchThumbOn,              // thumb color for checked switch
    switchThumbOff,             // thumb color for unchecked switch
    switchThumbDisabled,        // thumb color for disabled switch
    // TabBar background states for FluentUI3
    tabBarSelectedBackground,
    tabBarHoverBackground,
    count
};


static constexpr int percentToAlpha(double percent)
{
    return qRound(percent * 255. / 100.);
}

extern const std::array<QColor, WINUI3Color::count> WINUI3ColorsLight;
extern const std::array<QColor, WINUI3Color::count> WINUI3ColorsDark;
extern const std::array<std::array<QColor, WINUI3Color::count>, 2> WINUI3Colors;

// Color of close Button in Titlebar (default + hover)
extern const QColor shellCaptionCloseFillColorPrimary;
extern const QColor shellCaptionCloseTextFillColorPrimary;
// Color of close Button in Titlebar (pressed + disabled)
extern const QColor shellCaptionCloseFillColorSecondary;
extern const QColor shellCaptionCloseTextFillColorSecondary;

#endif // FLUENTUI3COLORS_H