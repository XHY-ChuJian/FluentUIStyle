#ifndef FLUENTUI3STYLE_H
#define FLUENTUI3STYLE_H

#include "fluentui3style_global.h"

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

class FLUENTUI3STYLE_EXPORT FluentUI3Style : public QProxyStyle
{
    Q_OBJECT

public:
    FluentUI3Style(QStyle *style = nullptr);
    ~FluentUI3Style();

public:
    void drawComplexControl(ComplexControl control,
                            const QStyleOptionComplex *option,
                            QPainter *painter,
                            const QWidget *widget) const override;

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override;

    void drawControl(ControlElement element,
                     const QStyleOption *option,
                     QPainter *painter,
                     const QWidget *widget = nullptr) const override;

    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget = nullptr) const override;

    QRect subControlRect(ComplexControl control,
                         const QStyleOptionComplex *option,
                         SubControl subControl,
                         const QWidget *widget) const override;

    int styleHint(StyleHint hint,
                  const QStyleOption *opt = nullptr,
                  const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const override;

    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;

    void polish(QWidget *widget) override;
    void polish(QPalette &pal) override;
    void polish(QApplication *app) override;

    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *app) override;

    QIcon standardIcon(StandardPixmap sp, const QStyleOption *option, const QWidget *widget) const override;
    QPalette standardPalette() const override;

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void drawCheckBox(const QStyleOption *option, QPainter *painter, const QWidget *widget) const;
    void drawSwitchButton(const QStyleOption *option, QPainter *painter, const QWidget *widget) const;
    void drawSpecialButton(QPainter *painter, const QStyleOption *option, const QWidget *widget, bool &isReturn) const;

    void drawTabBarTabLabel(const QStyleOption *option, QPainter *painter, const QWidget *widget) const;

    void drawTabBarTabShape(const QStyleOption *option, QPainter *painter, const QWidget *widget) const;
    void drawCapsuleTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const;
    void drawPivotGrowingTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const;
    void drawPivotStretchingTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const;
    void drawPivotSlidingTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const;

    void drawPillTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const;
    void drawSegmentedSlideTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const;
    void drawSegmentedFadeTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const;
    void drawSegmentedWinUI3Tab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const;
    void drawNavigationTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const;

    void drawProgressRing(const QStyleOptionProgressBar *option,
                          QPainter *painter,
                          const QWidget *widget,
                          bool drawTrack) const;

    void drawFluentDial(const QStyleOptionSlider *dial, QPainter *painter, const QWidget *widget, int style) const;

    void drawListViewIndicator(const QStyleOptionViewItem *option, QPainter *painter, const QWidget *widget) const;
    void drawNavigationViewIndicator(const QStyleOptionViewItem *option, QPainter *painter, const QWidget *widget) const;
    void drawTreeViewIndicator(const QStyleOptionViewItem *option, QPainter *painter, const QWidget *widget) const;

    void drawLineEditFrame(QPainter *painter, const QRectF &rect, const QStyleOption *option, bool isEditable = true, int roundingRadius = 4) const;

    void drawFluentShadow(QPainter *painter, QRect rect, int shadowWidth, int radius) const;
    void drawToolTipShadow(QPainter *painter, const QRect &panelRect, int radius) const;
    void drawEffectShadow(QPainter *painter, QRect widgetRect, int shadowBorderWidth, int borderRadius) const;
    void drawSliderHandleShadow(QPainter *painter, const QPointF &center, qreal outerRadius) const;

    QPen borderPenControlAlt(const QStyleOption *option) const;
    QColor calculateAccentColor(const QStyleOption *option) const;
    QBrush controlFillBrush(const QStyleOption *option, ControlType controlType) const;
    QBrush inputFillBrush(const QStyleOption *option, const QWidget *widget) const;
    QColor controlTextColor(const QStyleOption *option, QPalette::ColorRole role = QPalette::ButtonText) const;
    QColor winUI3Color(WINUI3Color col) const;
    QColor accentColor(const QStyleOption *option) const;

    QIcon fluentIcon(const QChar &ch, const QColor &color = QColor()) const;

private:
    mutable QFont assetFont;
    bool highContrastTheme;
    mutable int colorSchemeIndex = 0;
};

#endif // FLUENTUI3STYLE_H
