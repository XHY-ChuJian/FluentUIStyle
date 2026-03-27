#ifndef FLUENTUI3STYLE_H
#define FLUENTUI3STYLE_H

#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QStyleOption>
#include <QSvgRenderer>
#include <QTabBar>

#include "fluentui3colors.h"

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

public:
    void drawComplexControl( ComplexControl control,
                             const QStyleOptionComplex* option,
                             QPainter* painter,
                             const QWidget* widget ) const override;

    void drawPrimitive( PrimitiveElement element,
                        const QStyleOption* option,
                        QPainter* painter,
                        const QWidget* widget = nullptr ) const override;

    void drawControl( ControlElement element,
                     const QStyleOption* option,
                     QPainter* painter,
                     const QWidget* widget = nullptr ) const override;

    QRect subElementRect( SubElement element, const QStyleOption* option, const QWidget* widget = nullptr ) const override;

    QRect subControlRect( ComplexControl control,
                          const QStyleOptionComplex* option,
                          SubControl subControl,
                          const QWidget* widget ) const override;

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
    void drawSpecialButton( QPainter* painter, const QStyleOption* option, const QWidget* widget, bool& isReturn ) const;

    void drawTabBarTabLabel( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const;

    void drawTabBarTabShape( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const;
    void drawCapsuleTab( const QStyleOptionTab* tab, QPainter* painter, const QWidget* widget ) const;
    void drawPivotTab( const QStyleOptionTab* tab, QPainter* painter, const QWidget* widget ) const;
    void drawPivotGrowingIndicator( const QStyleOptionTab* tab, QPainter* painter, const QWidget* widget ) const;
    void drawPivotSlidingIndicator( const QStyleOptionTab* tab, QPainter* painter, const QWidget* widget ) const;

    QPen borderPenControlAlt( const QStyleOption* option ) const;

    QColor calculateAccentColor( const QStyleOption* option ) const;

    QBrush controlFillBrush( const QStyleOption* option, ControlType controlType ) const;

    QColor controlTextColor( const QStyleOption* option, QPalette::ColorRole role = QPalette::ButtonText ) const;

    void drawLineEditFrame( QPainter* p, const QRectF& rect, const QStyleOption* o, bool isEditable = true ) const;
    QColor winUI3Color( WINUI3Color col ) const;

    QIcon fluentIcon( const QChar& ch ) const;

    void drawFluentShadow( QPainter* painter, QRect rect, int shadowWidth, int radius ) const;
    void drawEffectShadow( QPainter* painter, QRect widgetRect, int shadowBorderWidth, int borderRadius ) const;

private:
    mutable QFont assetFont;
    bool highContrastTheme;
    int colorSchemeIndex = 0;
};

#endif  // FLUENTUI3STYLE_H
