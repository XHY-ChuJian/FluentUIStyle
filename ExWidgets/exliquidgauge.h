#pragma once

#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

#include <QColor>
#include <QElapsedTimer>
#include <QPainterPath>
#include <QProgressBar>

class QEvent;
class QHideEvent;
class QPaintEvent;
class QShowEvent;
class QTimer;

class EXWIDGETS_EXPORT ExLiquidGauge final : public QProgressBar
{
    Q_OBJECT

public:
    enum Shape
    {
        CircleShape,
        RectShape,
        PinShape,
        TriangleShape
    };
    Q_ENUM( Shape )

    // 水波图的外轮廓形状。
    EXWIDGETS_DECLARE_PROPERTY( Shape, shape, shape, setShape, CircleShape )

    // 波峰相对液面上下浮动的高度，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, waveAmplitude, waveAmplitude, setWaveAmplitude, 6.0 )

    // 控件宽度范围内包含的完整波形数量。
    EXWIDGETS_DECLARE_PROPERTY( int, waveCount, waveCount, setWaveCount, 3 )

    // 水波横向移动一个完整周期所需的时间，单位为毫秒。
    EXWIDGETS_DECLARE_PROPERTY( int,
                                waveAnimationDuration,
                                waveAnimationDuration,
                                setWaveAnimationDuration,
                                2400 )

    // 是否播放水波的横向移动动画。
    EXWIDGETS_DECLARE_PROPERTY( bool, animationEnabled, isAnimationEnabled, setAnimationEnabled, true )

    // 后层水波相对于主水波的不透明度，取值范围为 [0, 1]。
    EXWIDGETS_DECLARE_PROPERTY( qreal,
                                secondaryWaveOpacity,
                                secondaryWaveOpacity,
                                setSecondaryWaveOpacity,
                                0.45 )

    // 外轮廓线宽，0 表示不绘制轮廓。
    EXWIDGETS_DECLARE_PROPERTY( qreal, outlineWidth, outlineWidth, setOutlineWidth, 2.0 )

    // 外轮廓与内部液体区域之间的透明间距。
    EXWIDGETS_DECLARE_PROPERTY( qreal, outlineDistance, outlineDistance, setOutlineDistance, 3.0 )

    // 主水波颜色，无效颜色表示使用调色板强调色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, waveColor, waveColor, setWaveColor, QColor() )

    // 内部未填充区域颜色，无效颜色表示使用 QPalette::Base。
    EXWIDGETS_DECLARE_PROPERTY( QColor, backgroundColor, backgroundColor, setBackgroundColor, QColor() )

    // 外轮廓颜色，无效颜色表示使用水波颜色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, outlineColor, outlineColor, setOutlineColor, QColor() )

    // 未被液体覆盖的文字颜色，无效颜色表示使用 QPalette::Text。
    EXWIDGETS_DECLARE_PROPERTY( QColor, textColor, textColor, setTextColor, QColor() )

    // 被液体覆盖的文字颜色，无效颜色表示使用 QPalette::HighlightedText。
    EXWIDGETS_DECLARE_PROPERTY( QColor,
                                submergedTextColor,
                                submergedTextColor,
                                setSubmergedTextColor,
                                QColor() )

    // 中心文字字体像素大小，0 表示根据控件尺寸自动计算。
    EXWIDGETS_DECLARE_PROPERTY( int,
                                contentFontPixelSize,
                                contentFontPixelSize,
                                setContentFontPixelSize,
                                0 )

    explicit ExLiquidGauge( QWidget* parent = nullptr );

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent( QPaintEvent* event ) override;
    void showEvent( QShowEvent* event ) override;
    void hideEvent( QHideEvent* event ) override;
    void changeEvent( QEvent* event ) override;

private:
    [[nodiscard]] qreal valueFraction() const;
    [[nodiscard]] QPainterPath shapePath( const QRectF& bounds ) const;
    [[nodiscard]] QPainterPath wavePath( const QRectF& bounds,
                                         qreal baseline,
                                         qreal amplitude,
                                         qreal phase ) const;
    [[nodiscard]] QColor resolvedWaveColor() const;
    [[nodiscard]] QColor resolvedColor( const QColor& configuredColor,
                                        QPalette::ColorRole fallbackRole ) const;
    void updateAnimationState();

    QTimer* m_waveTimer = nullptr;
    QElapsedTimer m_waveElapsed;
    qreal m_wavePhase = 0.0;
};
