#pragma once

#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

#include <QColor>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QWidget>

class QPaintEvent;
class QVariantAnimation;

class EXWIDGETS_EXPORT ExMultiRadialGaugeItem final : public QObject
{
    Q_OBJECT

public:
    // 数据项名称，对应 ECharts gauge.data.name。
    EXWIDGETS_DECLARE_PROPERTY( QString, label, label, setLabel, QString() )

    // 数据项数值，对应 ECharts gauge.data.value。
    EXWIDGETS_DECLARE_PROPERTY( qreal, value, value, setValue, 0.0 )

    // 进度弧、指针和数值徽标使用的颜色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, color, color, setColor, QColor() )

    // 是否绘制当前数据项。
    EXWIDGETS_DECLARE_PROPERTY( bool, visible, isVisible, setVisible, true )

    // 名称相对仪表半径的中心偏移，例如 (-0.4, 0.8) 对应 ECharts 的 [-40%, 80%]。
    EXWIDGETS_DECLARE_PROPERTY( QPointF, titleOffset, titleOffset, setTitleOffset, QPointF( 0.0, 0.2 ) )

    // 数值相对仪表半径的中心偏移，例如 (-0.4, 0.95) 对应 ECharts 的 [-40%, 95%]。
    EXWIDGETS_DECLARE_PROPERTY( QPointF, detailOffset, detailOffset, setDetailOffset, QPointF( 0.0, 0.4 ) )

    explicit ExMultiRadialGaugeItem( QObject* parent = nullptr );
    ExMultiRadialGaugeItem( const QString& label,
                            qreal value,
                            const QColor& color,
                            QObject* parent = nullptr );

    Q_SIGNAL void itemChanged();
};

class EXWIDGETS_EXPORT ExMultiRadialGauge final : public QWidget
{
    Q_OBJECT

public:
    enum NeedleStyle
    {
        NoNeedle,
        LineNeedle,
        TriangleNeedle
    };
    Q_ENUM( NeedleStyle )

    // 所有数据项共用的最小值。
    EXWIDGETS_DECLARE_PROPERTY( qreal, minimum, minimum, setMinimum, 0.0 )

    // 所有数据项共用的最大值。
    EXWIDGETS_DECLARE_PROPERTY( qreal, maximum, maximum, setMaximum, 100.0 )

    // 刻度环起始角度，正上方为 0°，顺时针为正。
    EXWIDGETS_DECLARE_PROPERTY( qreal, minimumAngle, minimumAngle, setMinimumAngle, -135.0 )

    // 刻度环结束角度，正上方为 0°，顺时针为正。
    EXWIDGETS_DECLARE_PROPERTY( qreal, maximumAngle, maximumAngle, setMaximumAngle, 135.0 )

    // 整段圆弧上的主刻度数量，包含起点和终点。
    EXWIDGETS_DECLARE_PROPERTY( int, majorTickCount, majorTickCount, setMajorTickCount, 11 )

    // 每两个相邻主刻度之间的次刻度数量。
    EXWIDGETS_DECLARE_PROPERTY( int, minorTickCount, minorTickCount, setMinorTickCount, 4 )

    // 是否绘制 Track。
    EXWIDGETS_DECLARE_PROPERTY( bool, trackVisible, isTrackVisible, setTrackVisible, true )

    // Track 宽度。
    EXWIDGETS_DECLARE_PROPERTY( qreal, trackWidth, trackWidth, setTrackWidth, 8.0 )

    // Track 颜色，无效颜色表示使用 QPalette::Mid。
    EXWIDGETS_DECLARE_PROPERTY( QColor, trackColor, trackColor, setTrackColor, QColor() )

    // Track 的端点样式。
    EXWIDGETS_DECLARE_PROPERTY( Qt::PenCapStyle, trackCapStyle, trackCapStyle, setTrackCapStyle, Qt::RoundCap )

    // 是否绘制每个数据项的进度弧。
    EXWIDGETS_DECLARE_PROPERTY( bool, progressVisible, isProgressVisible, setProgressVisible, true )

    // 多条进度弧是否重叠；关闭后按数据项顺序绘制为同心弧。
    EXWIDGETS_DECLARE_PROPERTY( bool, progressOverlap, isProgressOverlap, setProgressOverlap, true )

    // 每条进度弧的宽度。
    EXWIDGETS_DECLARE_PROPERTY( qreal, progressWidth, progressWidth, setProgressWidth, 8.0 )

    // 不重叠时，相邻进度弧边缘之间的距离。
    EXWIDGETS_DECLARE_PROPERTY( qreal, progressSpacing, progressSpacing, setProgressSpacing, 2.0 )

    // 进度弧端点样式。
    EXWIDGETS_DECLARE_PROPERTY( Qt::PenCapStyle, progressCapStyle, progressCapStyle, setProgressCapStyle, Qt::RoundCap )

    // 刻度环与控件外边缘之间的距离。
    EXWIDGETS_DECLARE_PROPERTY( qreal, scalePadding, scalePadding, setScalePadding, 12.0 )

    // 次刻度长度。
    EXWIDGETS_DECLARE_PROPERTY( qreal, tickLength, tickLength, setTickLength, 4.0 )

    // 次刻度宽度。
    EXWIDGETS_DECLARE_PROPERTY( qreal, tickWidth, tickWidth, setTickWidth, 1.0 )

    // 主刻度长度。
    EXWIDGETS_DECLARE_PROPERTY( qreal, majorTickLength, majorTickLength, setMajorTickLength, 8.0 )

    // 主刻度宽度。
    EXWIDGETS_DECLARE_PROPERTY( qreal, majorTickWidth, majorTickWidth, setMajorTickWidth, 1.8 )

    // 刻度与刻度环内边缘之间的距离。
    EXWIDGETS_DECLARE_PROPERTY( qreal, tickPadding, tickPadding, setTickPadding, 8.0 )

    // 刻线颜色，无效颜色表示使用调色板文本色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, tickColor, tickColor, setTickColor, QColor() )

    // 是否显示主刻度数值标签。
    EXWIDGETS_DECLARE_PROPERTY( bool, labelsVisible, areLabelsVisible, setLabelsVisible, true )

    // 刻度数值标签与刻线之间的距离。
    EXWIDGETS_DECLARE_PROPERTY( qreal, labelPadding, labelPadding, setLabelPadding, 13.0 )

    // 刻度数值标签字号。
    EXWIDGETS_DECLARE_PROPERTY( int, labelFontPixelSize, labelFontPixelSize, setLabelFontPixelSize, 10 )

    // 刻度数值标签颜色，无效颜色表示使用调色板文本色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, labelColor, labelColor, setLabelColor, QColor() )

    // 指针样式，所有数据项共用。
    EXWIDGETS_DECLARE_PROPERTY( NeedleStyle, needleStyle, needleStyle, setNeedleStyle, LineNeedle )

    // 指针宽度。
    EXWIDGETS_DECLARE_PROPERTY( qreal, needleWidth, needleWidth, setNeedleWidth, 5.0 )

    // 指针长度相对于刻度环半径的比例。
    EXWIDGETS_DECLARE_PROPERTY( qreal, needleLength, needleLength, setNeedleLength, 0.72 )

    // 指针和轴心相对仪表半径的中心偏移。
    EXWIDGETS_DECLARE_PROPERTY( QPointF, needleOffset, needleOffset, setNeedleOffset, QPointF( 0.0, 0.08 ) )

    // 是否绘制公共轴心。
    EXWIDGETS_DECLARE_PROPERTY( bool, hubVisible, isHubVisible, setHubVisible, true )

    // 公共轴心半径。
    EXWIDGETS_DECLARE_PROPERTY( qreal, hubRadius, hubRadius, setHubRadius, 7.0 )

    // 公共轴心颜色，无效颜色表示使用调色板强调色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, hubColor, hubColor, setHubColor, QColor() )

    // 是否显示每个数据项的名称。
    EXWIDGETS_DECLARE_PROPERTY( bool, titleVisible, isTitleVisible, setTitleVisible, true )

    // 是否显示每个数据项的数值详情。
    EXWIDGETS_DECLARE_PROPERTY( bool, detailVisible, isDetailVisible, setDetailVisible, true )

    // 数值是否使用数据项颜色作为圆角徽标背景。
    EXWIDGETS_DECLARE_PROPERTY( bool, detailBadgeVisible, isDetailBadgeVisible, setDetailBadgeVisible, true )

    // 数据项名称字号。
    EXWIDGETS_DECLARE_PROPERTY( int, titleFontPixelSize, titleFontPixelSize, setTitleFontPixelSize, 12 )

    // 数据项数值字号。
    EXWIDGETS_DECLARE_PROPERTY( int, detailFontPixelSize, detailFontPixelSize, setDetailFontPixelSize, 12 )

    // 数据项名称颜色，无效颜色表示使用调色板文本色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, titleColor, titleColor, setTitleColor, QColor() )

    // 徽标内数值颜色，无效颜色表示自动选择。
    EXWIDGETS_DECLARE_PROPERTY( QColor, detailTextColor, detailTextColor, setDetailTextColor, QColor() )

    // 数值徽标的水平内边距。
    EXWIDGETS_DECLARE_PROPERTY( qreal, detailBadgePadding, detailBadgePadding, setDetailBadgePadding, 8.0 )

    // 追加在每个数值后的文本。
    EXWIDGETS_DECLARE_PROPERTY( QString, valueSuffix, valueSuffix, setValueSuffix, QStringLiteral( "%" ) )

    // 数值详情的小数位数。
    EXWIDGETS_DECLARE_PROPERTY( int, valueDecimals, valueDecimals, setValueDecimals, 0 )

    // 数据项数值变化动画时长，单位为毫秒；0 表示关闭动画。
    EXWIDGETS_DECLARE_PROPERTY( int, valueAnimationDuration, valueAnimationDuration, setValueAnimationDuration, 500 )

    explicit ExMultiRadialGauge( QWidget* parent = nullptr );
    ~ExMultiRadialGauge() override;

    void setRange( qreal minimum, qreal maximum );
    [[nodiscard]] QList<ExMultiRadialGaugeItem*> items() const;
    ExMultiRadialGaugeItem* addItem( const QString& label,
                                     qreal value,
                                     const QColor& color = QColor() );
    void addItem( ExMultiRadialGaugeItem* item );
    void removeItem( ExMultiRadialGaugeItem* item );
    void clearItems();

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

    Q_SIGNAL void itemsChanged();

protected:
    void paintEvent( QPaintEvent* event ) override;

private:
    [[nodiscard]] qreal sweepAngle() const;
    [[nodiscard]] qreal valueFraction( qreal value ) const;
    [[nodiscard]] qreal displayedValue( const ExMultiRadialGaugeItem* item ) const;
    void connectItem( ExMultiRadialGaugeItem* item );
    void startValueAnimation();
    void synchronizeDisplayedValues();

    QList<ExMultiRadialGaugeItem*> m_items;
    QHash<const ExMultiRadialGaugeItem*, qreal> m_displayedValues;
    QHash<const ExMultiRadialGaugeItem*, qreal> m_animationStartValues;
    QVariantAnimation* m_valueAnimation = nullptr;
};
