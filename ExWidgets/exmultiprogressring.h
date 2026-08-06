#pragma once

#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

#include <QColor>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QWidget>

class QPaintEvent;
class QVariantAnimation;

class EXWIDGETS_EXPORT ExMultiProgressRingItem final : public QObject
{
    Q_OBJECT

public:
    // 环对应的名称，显示在中央详情区域。
    EXWIDGETS_DECLARE_PROPERTY( QString, label, label, setLabel, QString() )

    // 环的当前数值，由所属控件的 minimum 和 maximum 共同解释。
    EXWIDGETS_DECLARE_PROPERTY( qreal, value, value, setValue, 0.0 )

    // 环的前景颜色，无效颜色表示使用调色板强调色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, color, color, setColor, QColor() )

    explicit ExMultiProgressRingItem( QObject* parent = nullptr );
    ExMultiProgressRingItem( const QString& label,
                             qreal value,
                             const QColor& color,
                             QObject* parent = nullptr );

    Q_SIGNAL void itemChanged();
};

class EXWIDGETS_EXPORT ExMultiProgressRing final : public QWidget
{
    Q_OBJECT

public:
    // 所有环共用的最小值。
    EXWIDGETS_DECLARE_PROPERTY( qreal, minimum, minimum, setMinimum, 0.0 )

    // 所有环共用的最大值。
    EXWIDGETS_DECLARE_PROPERTY( qreal, maximum, maximum, setMaximum, 100.0 )

    // 圆弧起始角度，正上方为 0°，顺时针为正。
    EXWIDGETS_DECLARE_PROPERTY( qreal, startAngle, startAngle, setStartAngle, 0.0 )

    // 圆弧沿顺时针方向扫过的角度，取值范围为 [0°, 360°]。
    EXWIDGETS_DECLARE_PROPERTY( qreal, sweepAngle, sweepAngle, setSweepAngle, 360.0 )

    // 每条环的线宽，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, ringWidth, ringWidth, setRingWidth, 8.0 )

    // 相邻两条环边缘之间的距离，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, ringSpacing, ringSpacing, setRingSpacing, 4.0 )

    // 最外层环与控件边缘之间的距离，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, ringPadding, ringPadding, setRingPadding, 12.0 )

    // 进度环和 Track 的端点样式。
    EXWIDGETS_DECLARE_PROPERTY( Qt::PenCapStyle, capStyle, capStyle, setCapStyle, Qt::RoundCap )

    // 是否在每条进度环后方绘制完整 Track。
    EXWIDGETS_DECLARE_PROPERTY( bool, trackVisible, isTrackVisible, setTrackVisible, false )

    // Track 颜色，无效颜色表示使用 QPalette::Mid。
    EXWIDGETS_DECLARE_PROPERTY( QColor, trackColor, trackColor, setTrackColor, QColor() )

    // 是否绘制中央的名称和数值详情。
    EXWIDGETS_DECLARE_PROPERTY( bool, detailsVisible, areDetailsVisible, setDetailsVisible, true )

    // 数值是否绘制带颜色边框的圆角徽标。
    EXWIDGETS_DECLARE_PROPERTY( bool, valueBadgeVisible, isValueBadgeVisible, setValueBadgeVisible, true )

    // 中央名称颜色，无效颜色表示使用 QPalette::Text。
    EXWIDGETS_DECLARE_PROPERTY( QColor, labelColor, labelColor, setLabelColor, QColor() )

    // 追加在每个数值后的文本，例如百分号。
    EXWIDGETS_DECLARE_PROPERTY( QString, valueSuffix, valueSuffix, setValueSuffix, QStringLiteral( "%" ) )

    // 数值显示的小数位数，取值范围为 [0, 6]。
    EXWIDGETS_DECLARE_PROPERTY( int, valueDecimals, valueDecimals, setValueDecimals, 0 )

    // 中央名称的字体像素大小，0 表示根据控件尺寸自动计算。
    EXWIDGETS_DECLARE_PROPERTY( int, labelFontPixelSize, labelFontPixelSize, setLabelFontPixelSize, 0 )

    // 中央数值的字体像素大小，0 表示根据控件尺寸自动计算。
    EXWIDGETS_DECLARE_PROPERTY( int, valueFontPixelSize, valueFontPixelSize, setValueFontPixelSize, 0 )

    // 数值变化动画时长，单位为毫秒；0 表示关闭动画。
    EXWIDGETS_DECLARE_PROPERTY( int,
                                valueAnimationDuration,
                                valueAnimationDuration,
                                setValueAnimationDuration,
                                500 )

    explicit ExMultiProgressRing( QWidget* parent = nullptr );

    void setRange( qreal minimum, qreal maximum );
    [[nodiscard]] QList<ExMultiProgressRingItem*> items() const;
    ExMultiProgressRingItem* addItem( const QString& label, qreal value, const QColor& color = QColor() );
    void addItem( ExMultiProgressRingItem* item );
    void removeItem( ExMultiProgressRingItem* item );
    void clearItems();

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

    Q_SIGNAL void itemsChanged();

protected:
    void paintEvent( QPaintEvent* event ) override;

private:
    [[nodiscard]] qreal itemFraction( qreal value ) const;
    [[nodiscard]] qreal displayedValue( const ExMultiProgressRingItem* item ) const;
    void connectItem( ExMultiProgressRingItem* item );
    void startValueAnimation();
    void synchronizeDisplayedValues();

    QList<ExMultiProgressRingItem*> m_items;
    QHash<const ExMultiProgressRingItem*, qreal> m_displayedValues;
    QHash<const ExMultiProgressRingItem*, qreal> m_animationStartValues;
    QVariantAnimation* m_valueAnimation = nullptr;
};
