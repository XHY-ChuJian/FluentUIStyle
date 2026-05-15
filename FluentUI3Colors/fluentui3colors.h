#ifndef FLUENTUI3COLORS_H
#define FLUENTUI3COLORS_H

#include <QtGlobal>
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
    tabBarSelectedBackground,
    tabBarHoverBackground,
    cardBackgroundFillColorDefault,
    cardStrokeColorDefault,
    count
};

inline int percentToAlpha(double percent)
{
    return qRound(percent * 255. / 100.);
}

inline const std::array<QColor, WINUI3Color::count> WINUI3ColorsLight{
    QColor(0x00, 0x00, 0x00, percentToAlpha(4.0)),    // subtleHighlightColor (fillSubtleSecondary)
    QColor(0x00, 0x00, 0x00, percentToAlpha(5.5)),    // subtlePressedColor (fillSubtleTertiary)
    QColor(0x00, 0x00, 0x00, 0x0F),                   // frameColorLight
    QColor(0x00, 0x00, 0x00, percentToAlpha(60.63)),  // frameColorStrong
    QColor(0x00, 0x00, 0x00, percentToAlpha(21.69)),   // frameColorStrongDisabled
    QColor(0x00, 0x00, 0x00, 0x72),                   // controlStrongFill
    QColor(0x00, 0x00, 0x00, 0x29),                   // controlStrokeSecondary
    QColor(0x00, 0x00, 0x00, 0x14),                   // controlStrokePrimary
    QColor(0xFF, 0xFF, 0xFF, 0xFF),                   // menuPanelFill
    QColor(0x00, 0x00, 0x00, 0x66),                   // controlStrokeOnAccentSecondary
    QColor(0xFF, 0xFF, 0xFF, 0xFF),                   // controlFillSolid
    QColor(0x75, 0x75, 0x75, 0x66),                   // surfaceStroke
    QColor(0xFF, 0xFF, 0xFF, 0xFF),                   // focusFrameInnerStroke
    QColor(0x00, 0x00, 0x00, 0xFF),                   // focusFrameOuterStroke
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(70)),     // fillControlDefault
    QColor(0xF0, 0xF0, 0xF0, percentToAlpha(80)),     // fillControlSecondary
    QColor(0xF9, 0xF9, 0xF9, percentToAlpha(30)),     // fillControlTertiary
    QColor(0xF9, 0xF9, 0xF9, percentToAlpha(30)),     // fillControlDisabled
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(100)),    // fillControlInputActive
    QColor(0x00, 0x00, 0x00, percentToAlpha(2.41)),   // fillControlAltSecondary
    QColor(0x00, 0x00, 0x00, percentToAlpha(5.78)),   // fillControlAltTertiary
    QColor(0x00, 0x00, 0x00, percentToAlpha(9.24)),   // fillControlAltQuarternary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(0.00)),   // fillControlAltDisabled
    QColor(0x00, 0x00, 0x00, percentToAlpha(100)),     // fillAccentDefault
    QColor(0x00, 0x00, 0x00, percentToAlpha(90)),     // fillAccentSecondary
    QColor(0x00, 0x00, 0x00, percentToAlpha(80)),     // fillAccentTertiary
    QColor(0x00, 0x00, 0x00, percentToAlpha(21.69)),  // fillAccentDisabled
    QColor(0x00, 0x00, 0x00, percentToAlpha(89.56)),  // textPrimary
    QColor(0x00, 0x00, 0x00, percentToAlpha(60.63)),  // textSecondary
    QColor(0x00, 0x00, 0x00, percentToAlpha(36.14)),  // textDisabled
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(100)),    // textOnAccentPrimary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(70)),     // textOnAccentSecondary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(100)),     // textOnAccentDisabled
    QColor(0x00, 0x00, 0x00, percentToAlpha(8.03)),   // dividerStrokeDefault
    QColor(0xFF, 0xFF, 0xFF, 0xFF),                   // switchThumbOn
    QColor(0x5A, 0x5A, 0x5A, 0xFF),                  // switchThumbOff
    QColor(0x5A, 0x5A, 0x5A, percentToAlpha(40)),     // switchThumbDisabled
    QColor(230, 230, 230),                            // tabBarSelectedBackground
    QColor(0, 0, 0, 18),                              // tabBarHoverBackground
    QColor(255, 255, 255, 179),                       // cardBackgroundFillColorDefault
    QColor(0, 0, 0, 15)                               // cardStrokeColorDefault
};

inline const std::array<QColor, WINUI3Color::count> WINUI3ColorsDark{
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(4.19)),   // subtleHighlightColor (fillSubtleSecondary)
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(6.05)),   // subtlePressedColor (fillSubtleTertiary)
    QColor(0xFF, 0xFF, 0xFF, 0x12),                   // frameColorLight
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(60.47)), // frameColorStrong
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(15.81)), // frameColorStrongDisabled
    QColor(0xFF, 0xFF, 0xFF, 0x8B),                   // controlStrongFill
    QColor(0xFF, 0xFF, 0xFF, 0x18),                   // controlStrokeSecondary
    QColor(0xFF, 0xFF, 0xFF, 0x12),                   // controlStrokePrimary
    QColor(45, 45, 45),                               // menuPanelFill
    QColor(0xFF, 0xFF, 0xFF, 0x14),                   // controlStrokeOnAccentSecondary
    QColor(0x45, 0x45, 0x45, 0xFF),                   // controlFillSolid
    QColor(0x75, 0x75, 0x75, 0x66),                   // surfaceStroke
    QColor(0x00, 0x00, 0x00, 0xFF),                   // focusFrameInnerStroke
    QColor(0xFF, 0xFF, 0xFF, 0xFF),                   // focusFrameOuterStroke
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(6.05)),   // fillControlDefault
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(15.0)),   // fillControlSecondary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(3.26)),   // fillControlTertiary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(4.19)),   // fillControlDisabled
    QColor(0x21, 0x21, 0x21, percentToAlpha(100)),    // fillControlInputActive (LineEdit/SpinBox focused)
    QColor(0x00, 0x00, 0x00, percentToAlpha(10.0)),   // fillControlAltSecondary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(4.19)),    // fillControlAltSecondary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(6.98)),   // fillControlAltTertiary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(0.00)),   // controlAltDisabled
    QColor(0x00, 0x00, 0x00, percentToAlpha(100)),    // fillAccentDefault
    QColor(0x00, 0x00, 0x00, percentToAlpha(90)),     // fillAccentSecondary
    QColor(0x00, 0x00, 0x00, percentToAlpha(80)),     // fillAccentTertiary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(15.81)),  // fillAccentDisabled
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(100)),    // textPrimary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(78.6)),  // textSecondary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(36.28)),  // textDisabled
    QColor(0x00, 0x00, 0x00, percentToAlpha(100)),    // textOnAccentPrimary
    QColor(0x00, 0x00, 0x00, percentToAlpha(70)),     // textOnAccentSecondary
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(53.02)),  // textOnAccentDisabled
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(8.37)),   // dividerStrokeDefault
    QColor(0x1F, 0x1F, 0x1F, 0xFF),                   // switchThumbOn
    QColor(0x5A, 0x5A, 0x5A, 0xFF),                   // switchThumbOff
    QColor(0x5A, 0x5A, 0x5A, percentToAlpha(40)),     // switchThumbDisabled
    QColor(82, 82, 84),                               // tabBarSelectedBackground
    QColor(0xFF, 0xFF, 0xFF, percentToAlpha(12.0)),  // tabBarHoverBackground
    QColor(255, 255, 255, 13),                        // cardBackgroundFillColorDefault
    QColor(0, 0, 0, 25)                               // cardStrokeColorDefault
};

inline const QColor shellCaptionCloseFillColorPrimary(0xC4, 0x2B, 0x1C, 0xFF);
inline const QColor shellCaptionCloseTextFillColorPrimary(0xFF, 0xFF, 0xFF, 0xFF);
inline const QColor shellCaptionCloseFillColorSecondary(0xC4, 0x2B, 0x1C, 0xE6);
inline const QColor shellCaptionCloseTextFillColorSecondary(0xFF, 0xFF, 0xFF, 0xB3);

inline const std::array<std::array<QColor, WINUI3Color::count>, 2> WINUI3Colors{WINUI3ColorsLight, WINUI3ColorsDark};

#endif // FLUENTUI3COLORS_H
