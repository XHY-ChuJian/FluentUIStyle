#ifndef FLUENTUI3STYLE_H
#define FLUENTUI3STYLE_H

#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QStyleOption>
#include <QSvgRenderer>
#include <QTabBar>

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
};

enum class ControlType
{
    Control,
    ControlAlt
};

enum SpinBoxButtonLayout
{
    ArrowsVertical,
    ArrowsHorizontalSides,
    ArrowsHorizontalRight,
    PlusMinusHorizontalSides
};

class FluentUI3Style : public QProxyStyle
{
    Q_OBJECT

public:
    FluentUI3Style( QStyle* style = nullptr );
    ~FluentUI3Style();

    // QStyle interface

public:
    void drawComplexControl( ComplexControl control,
                             const QStyleOptionComplex* option,
                             QPainter* painter,
                             const QWidget* widget ) const override;

    void drawPrimitive( PrimitiveElement element,
                        const QStyleOption* option,
                        QPainter* painter,
                        const QWidget* widget = nullptr ) const override;

    QRect subElementRect( SubElement element, const QStyleOption* option, const QWidget* widget = nullptr ) const override;

    QRect subControlRect( ComplexControl control,
                          const QStyleOptionComplex* option,
                          SubControl subControl,
                          const QWidget* widget ) const override;

    void drawControl( ControlElement element,
                      const QStyleOption* option,
                      QPainter* painter,
                      const QWidget* widget = nullptr ) const override;
    int styleHint( StyleHint hint,
                   const QStyleOption* opt      = nullptr,
                   const QWidget* widget        = nullptr,
                   QStyleHintReturn* returnData = nullptr ) const override;

    void polish( QWidget* widget ) override;

    QSize sizeFromContents( ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget ) const override;

    int pixelMetric( PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr ) const override;

    void polish( QPalette& result ) override;
    void unpolish( QWidget* widget ) override;

    QIcon standardIcon( StandardPixmap sp, const QStyleOption* option, const QWidget* widget ) const override;

private:
    void drawCheckBox( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const;
    void drawSwitchButton( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const;

    QPen borderPenControlAlt( const QStyleOption* option ) const;

    QColor calculateAccentColor( const QStyleOption* option ) const;

    QBrush controlFillBrush( const QStyleOption* option, ControlType controlType ) const;

    QColor controlTextColor( const QStyleOption* option, QPalette::ColorRole role = QPalette::ButtonText ) const;

    void drawLineEditFrame( QPainter* p, const QRectF& rect, const QStyleOption* o, bool isEditable = true ) const;
    QColor winUI3Color( enum WINUI3Color col ) const;

    QIcon fluentIcon( const QChar& ch ) const;

    void drawFluentShadow( QPainter* painter, QRect rect, int shadowWidth, int radius ) const;
    void drawEffectShadow( QPainter* painter, QRect widgetRect, int shadowBorderWidth, int borderRadius ) const;

private:
    mutable QFont assetFont;
    bool highContrastTheme;
    int colorSchemeIndex = 0;
};

#endif  // FLUENTUI3STYLE_H
