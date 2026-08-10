#pragma once

#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

#include <QColor>
#include <QDial>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QString>

class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QVariantAnimation;
class QWheelEvent;

class EXWIDGETS_EXPORT ExRadialGaugeRange final : public QObject
{
    Q_OBJECT

public:
    // 区间的起始数值。
    EXWIDGETS_DECLARE_PROPERTY( int, fromValue, fromValue, setFromValue, 0 )

    // 区间的结束数值。
    EXWIDGETS_DECLARE_PROPERTY( int, toValue, toValue, setToValue, 0 )

    // 区间在刻度环上的颜色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, color, color, setColor, QColor() )

    explicit ExRadialGaugeRange( QObject* parent = nullptr );
    ExRadialGaugeRange( int fromValue, int toValue, const QColor& color, QObject* parent = nullptr );

    Q_SIGNAL void rangeChanged();
};

class EXWIDGETS_EXPORT ExRadialGauge final : public QDial
{
    Q_OBJECT

public:
    enum ScaleMode
    {
        TrackScale,
        ProgressScale,
        RangeScale
    };
    Q_ENUM( ScaleMode )

    enum NeedleStyle
    {
        NoNeedle,
        LineNeedle,
        TriangleNeedle
    };
    Q_ENUM( NeedleStyle )

    enum ValuePosition
    {
        CenterValue,
        BottomValue
    };
    Q_ENUM( ValuePosition )

    //是否允许通过鼠标、键盘和滚轮修改数值。
    EXWIDGETS_DECLARE_PROPERTY( bool, interactive, isInteractive, setInteractive, true )

    // 数值变化动画时长，单位为毫秒；0 表示关闭动画。
    EXWIDGETS_DECLARE_PROPERTY( int, valueAnimationDuration, valueAnimationDuration, setValueAnimationDuration, 500 )

    // 刻度环的绘制模式：纯 Track、数值进度或彩色区间。
    EXWIDGETS_DECLARE_PROPERTY( ScaleMode, scaleMode, scaleMode, setScaleMode, ProgressScale )

    // 刻度环的起始角度，正上方为 0°，顺时针为正。
    EXWIDGETS_DECLARE_PROPERTY( qreal, minimumAngle, minimumAngle, setMinimumAngle, -135.0 )

    // 刻度环的结束角度，正上方为 0°，顺时针为正。
    EXWIDGETS_DECLARE_PROPERTY( qreal, maximumAngle, maximumAngle, setMaximumAngle, 135.0 )

    // 整段圆弧上的主刻度数量，包含起点和终点。
    EXWIDGETS_DECLARE_PROPERTY( int, majorTickCount, majorTickCount, setMajorTickCount, 11 )

    // 每两个相邻主刻度之间绘制的次刻度数量。
    EXWIDGETS_DECLARE_PROPERTY( int, minorTickCount, minorTickCount, setMinorTickCount, 4 )

    // 刻度环的线宽，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, scaleWidth, scaleWidth, setScaleWidth, 8.0 )

    // 刻度环与控件外边缘之间的间距
    EXWIDGETS_DECLARE_PROPERTY( qreal, scalePadding, scalePadding, setScalePadding, 12.0 )

    // Track 圆弧的端点样式；Progress 模式默认使用圆头，其他模式默认使用平头。
    EXWIDGETS_DECLARE_PROPERTY( Qt::PenCapStyle, trackCapStyle, trackCapStyle, setTrackCapStyle, Qt::RoundCap )

    // 进度环和彩色区间圆弧的端点样式；Progress 模式默认使用圆头，其他模式默认使用平头。
    EXWIDGETS_DECLARE_PROPERTY( Qt::PenCapStyle, ringCapStyle, ringCapStyle, setRingCapStyle, Qt::RoundCap )

    // Progress 模式下，已扫过的圆弧是否沿扫描方向使用渐变色。
    EXWIDGETS_DECLARE_PROPERTY( bool, progressGradientEnabled, isProgressGradientEnabled, setProgressGradientEnabled, false )

    // Progress 模式下，是否显示从起点到当前指针位置的渐变扇形。
    EXWIDGETS_DECLARE_PROPERTY( bool, sweepAreaVisible, isSweepAreaVisible, setSweepAreaVisible, false )

    // 指针扫过扇形的不透明度，取值范围为 [0.0, 1.0]。
    EXWIDGETS_DECLARE_PROPERTY( qreal, sweepAreaOpacity, sweepAreaOpacity, setSweepAreaOpacity, 0.16 )

    // 进度环及扫过扇形的渐变起点颜色，无效颜色表示使用较亮的调色板强调色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, progressGradientStartColor, progressGradientStartColor, setProgressGradientStartColor, QColor() )

    // 进度环及扫过扇形的渐变终点颜色，无效颜色表示使用调色板强调色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, progressGradientEndColor, progressGradientEndColor, setProgressGradientEndColor, QColor() )

    // 指针的线宽
    EXWIDGETS_DECLARE_PROPERTY( qreal, needleWidth, needleWidth, setNeedleWidth, 10.0 )

    // 指针的绘制样式：隐藏、线形或三角形。
    EXWIDGETS_DECLARE_PROPERTY( NeedleStyle, needleStyle, needleStyle, setNeedleStyle, LineNeedle )

    // 指针长度相对于刻度环半径的比例，取值范围为 [0.05, 1.0]。
    EXWIDGETS_DECLARE_PROPERTY( qreal, needleLength, needleLength, setNeedleLength, 0.62 )

    // 次刻度的长度。
    EXWIDGETS_DECLARE_PROPERTY( qreal, tickLength, tickLength, setTickLength, 7.0 )

    // 次刻度的线宽。
    EXWIDGETS_DECLARE_PROPERTY( qreal, tickWidth, tickWidth, setTickWidth, 1.5 )

    // 主刻度的长度。
    EXWIDGETS_DECLARE_PROPERTY( qreal, majorTickLength, majorTickLength, setMajorTickLength, 10.0 )

    // 主刻度的线宽。
    EXWIDGETS_DECLARE_PROPERTY( qreal, majorTickWidth, majorTickWidth, setMajorTickWidth, 2.0 )

    // 外圈刻度与刻度环之间的间距。
    EXWIDGETS_DECLARE_PROPERTY( qreal, tickPadding, tickPadding, setTickPadding, 8.0 )

    // 是否绘制刻度对应的数值标签。
    EXWIDGETS_DECLARE_PROPERTY( bool, labelsVisible, areLabelsVisible, setLabelsVisible, false )

    // 数值标签与刻度环内边缘之间的距离。
    EXWIDGETS_DECLARE_PROPERTY( qreal, labelPadding, labelPadding, setLabelPadding, 28.0 )

    // 数值标签的字体像素大小。
    EXWIDGETS_DECLARE_PROPERTY( int, labelFontPixelSize, labelFontPixelSize, setLabelFontPixelSize, 11 )

    // 是否在指针中心绘制圆形轴心，线形和三角形指针均支持。
    EXWIDGETS_DECLARE_PROPERTY( bool, hubVisible, isHubVisible, setHubVisible, false )

    // 指针轴心的半径。
    EXWIDGETS_DECLARE_PROPERTY( qreal, hubRadius, hubRadius, setHubRadius, 11.0 )

    // 是否在仪表盘底部显示当前数值。
    EXWIDGETS_DECLARE_PROPERTY( bool, valueVisible, isValueVisible, setValueVisible, true )

    // 当前数值显示在仪表盘中心或底部。
    EXWIDGETS_DECLARE_PROPERTY( ValuePosition, valuePosition, valuePosition, setValuePosition, BottomValue )

    // 显示在当前数值上方的标题。
    EXWIDGETS_DECLARE_PROPERTY( QString, title, title, setTitle, QString() )

    // 追加在当前数值后的单位文本。
    EXWIDGETS_DECLARE_PROPERTY( QString, unit, unit, setUnit, QString() )

    // 当前数值的字体像素大小，0 表示根据控件尺寸自动计算。
    EXWIDGETS_DECLARE_PROPERTY( int, valueFontPixelSize, valueFontPixelSize, setValueFontPixelSize, 0 )

    // 指针颜色，无效颜色表示使用调色板强调色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, needleColor, needleColor, setNeedleColor, QColor() )

    // 刻线颜色，无效颜色表示使用调色板文本色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, tickColor, tickColor, setTickColor, QColor() )

    // 刻度数值标签颜色，无效颜色表示使用调色板文本色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, labelColor, labelColor, setLabelColor, QColor() )

    // 标题、当前数值和单位颜色，无效颜色表示使用调色板文本色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, valueColor, valueColor, setValueColor, QColor() )

    explicit ExRadialGauge( QWidget* parent = nullptr );
    ~ExRadialGauge() override;

    Q_SLOT void setValue( int value );
    [[nodiscard]] bool isValueAnimating() const;

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

    [[nodiscard]] QList<ExRadialGaugeRange*> ranges() const;
    ExRadialGaugeRange* addRange( int fromValue, int toValue, const QColor& color );
    void removeRange( ExRadialGaugeRange* range );
    void clearRanges();

    Q_SIGNAL void rangesChanged();

protected:
    void paintEvent( QPaintEvent* event ) override;
    void mousePressEvent( QMouseEvent* event ) override;
    void mouseMoveEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;
    void wheelEvent( QWheelEvent* event ) override;
    void keyPressEvent( QKeyEvent* event ) override;

private:
    [[nodiscard]] qreal sweepAngle() const;
    [[nodiscard]] qreal valueFraction( qreal value ) const;
    [[nodiscard]] qreal positionFraction() const;
    [[nodiscard]] int positionFromPoint( const QPointF& point ) const;
    void updatePositionFromPoint( const QPointF& point );

    QList<ExRadialGaugeRange*> m_ranges;
    QVariantAnimation* m_valueAnimation = nullptr;
    Qt::FocusPolicy m_interactiveFocusPolicy = Qt::StrongFocus;
};
