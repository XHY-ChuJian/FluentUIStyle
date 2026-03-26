#include "fluentui3colors.h"

const std::array<QColor, WINUI3Color::count> WINUI3ColorsLight {
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 3.73 ) ),   // subtleHighlightColor (fillSubtleSecondary)
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 2.41 ) ),   // subtlePressedColor (fillSubtleTertiary)
    QColor( 0x00, 0x00, 0x00, 0x0F ),                     // frameColorLight
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 60.63 ) ),  // frameColorStrong
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 21.69 ) ),  // frameColorStrongDisabled
    QColor( 0x00, 0x00, 0x00, 0x72 ),                     // controlStrongFill
    QColor( 0x00, 0x00, 0x00, 0x29 ),                     // controlStrokeSecondary
    QColor( 0x00, 0x00, 0x00, 0x14 ),                     // controlStrokePrimary
    QColor( 0xFF, 0xFF, 0xFF, 0xFF ),                     // menuPanelFill
    QColor( 0x00, 0x00, 0x00, 0x66 ),                     // controlStrokeOnAccentSecondary
    QColor( 0xFF, 0xFF, 0xFF, 0xFF ),                     // controlFillSolid
    QColor( 0x75, 0x75, 0x75, 0x66 ),                     // surfaceStroke
    QColor( 0xFF, 0xFF, 0xFF, 0xFF ),                     // focusFrameInnerStroke
    QColor( 0x00, 0x00, 0x00, 0xFF ),                     // focusFrameOuterStroke
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 70 ) ),     // fillControlDefault
    QColor( 0xF9, 0xF9, 0xF9, percentToAlpha( 50 ) ),     // fillControlSecondary
    QColor( 0xF9, 0xF9, 0xF9, percentToAlpha( 30 ) ),     // fillControlTertiary
    QColor( 0xF9, 0xF9, 0xF9, percentToAlpha( 30 ) ),     // fillControlDisabled
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 100 ) ),    // fillControlInputActive
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 2.41 ) ),   // fillControlAltSecondary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 5.78 ) ),   // fillControlAltTertiary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 9.24 ) ),   // fillControlAltQuarternary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 0.00 ) ),   // fillControlAltDisabled
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 100 ) ),    // fillAccentDefault
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 90 ) ),     // fillAccentSecondary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 80 ) ),     // fillAccentTertiary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 21.69 ) ),  // fillAccentDisabled
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 89.56 ) ),  // textPrimary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 60.63 ) ),  // textSecondary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 36.14 ) ),  // textDisabled
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 100 ) ),    // textOnAccentPrimary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 70 ) ),     // textOnAccentSecondary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 100 ) ),    // textOnAccentDisabled
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 8.03 ) ),   // dividerStrokeDefault
    QColor( 0xFF, 0xFF, 0xFF, 0xFF ),                     // switchThumbOn
    QColor( 0x5A, 0x5A, 0x5A, 0xFF ),                     // switchThumbOff
    QColor( 0x5A, 0x5A, 0x5A, percentToAlpha( 40 ) ),     // switchThumbDisabled
    QColor( 230, 230, 230 ),                               // tabBarSelectedBackground
    QColor( 0, 0, 0, 18 )                                  // tabBarHoverBackground

};

const std::array<QColor, WINUI3Color::count> WINUI3ColorsDark {
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 6.05 ) ),   // subtleHighlightColor (fillSubtleSecondary)
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 4.19 ) ),   // subtlePressedColor (fillSubtleTertiary)
    QColor( 0xFF, 0xFF, 0xFF, 0x12 ),                     // frameColorLight
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 60.47 ) ),  // frameColorStrong
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 15.81 ) ),  // frameColorStrongDisabled
    QColor( 0xFF, 0xFF, 0xFF, 0x8B ),                     // controlStrongFill
    QColor( 0xFF, 0xFF, 0xFF, 0x18 ),                     // controlStrokeSecondary
    QColor( 0xFF, 0xFF, 0xFF, 0x12 ),                     // controlStrokePrimary
    QColor( 45, 45, 45 ),                                 // menuPanelFill
    QColor( 0xFF, 0xFF, 0xFF, 0x14 ),                     // controlStrokeOnAccentSecondary
    QColor( 0x45, 0x45, 0x45, 0xFF ),                     // controlFillSolid
    QColor( 0x75, 0x75, 0x75, 0x66 ),                     // surfaceStroke
    QColor( 0x00, 0x00, 0x00, 0xFF ),                     // focusFrameInnerStroke
    QColor( 0xFF, 0xFF, 0xFF, 0xFF ),                     // focusFrameOuterStroke
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 6.05 ) ),   // fillControlDefault
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 8.37 ) ),   // fillControlSecondary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 3.26 ) ),   // fillControlTertiary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 4.19 ) ),   // fillControlDisabled
    QColor( 0x1E, 0x1E, 0x1E, percentToAlpha( 70 ) ),     // fillControlInputActive
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 10.0 ) ),   // fillControlAltDefault
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 4.19 ) ),   // fillControlAltSecondary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 6.98 ) ),   // fillControlAltTertiafillCy
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 0.00 ) ),   // controlAltDisabled
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 100 ) ),    // fillAccentDefault
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 90 ) ),     // fillAccentSecondary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 80 ) ),     // fillAccentTertiary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 15.81 ) ),  // fillAccentDisabled
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 100 ) ),    // textPrimary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 78.6 ) ),   // textSecondary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 36.28 ) ),  // textDisabled
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 100 ) ),    // textOnAccentPrimary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 70 ) ),     // textOnAccentSecondary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 53.02 ) ),  // textOnAccentDisabled
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 8.37 ) ),   // dividerStrokeDefault
    QColor( 0x1F, 0x1F, 0x1F, 0xFF ),                     // switchThumbOn
    QColor( 0x5A, 0x5A, 0x5A, 0xFF ),                     // switchThumbOff
    QColor( 0x5A, 0x5A, 0x5A, percentToAlpha( 40 ) ),     // switchThumbDisabled
    QColor( 60, 60, 60 ),                                 // tabBarSelectedBackground
    QColor( 255, 255, 255, 18 )                            // tabBarHoverBackground
};

// Color of close Button in Titlebar (default + hover)
const QColor shellCaptionCloseFillColorPrimary( 0xC4, 0x2B, 0x1C, 0xFF );
const QColor shellCaptionCloseTextFillColorPrimary( 0xFF, 0xFF, 0xFF, 0xFF );
// Color of close Button in Titlebar (pressed + disabled)
const QColor shellCaptionCloseFillColorSecondary( 0xC4, 0x2B, 0x1C, 0xE6 );
const QColor shellCaptionCloseTextFillColorSecondary( 0xFF, 0xFF, 0xFF, 0xB3 );

const std::array<std::array<QColor, WINUI3Color::count>, 2> WINUI3Colors { WINUI3ColorsLight, WINUI3ColorsDark };
