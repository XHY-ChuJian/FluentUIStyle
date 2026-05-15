#pragma once

#include "exwidgets_global.h"

#include <QObject>
#include <QVariantAnimation>
#include <QWidget>

class ExRangeSlider;

/**
 * @brief 范围滑块的一个端点（对应 Qt Quick 的 QQuickRangeSliderNode / QML first、second）。
 *
 * 每个节点持有自己的 value、pressed、hovered，以及用于手柄缩放的动画；
 * position / visualPosition 由所属 ExRangeSlider 的 minimum、maximum、orientation、layoutDirection 推导，
 * 语义与 qquickrangeslider.cpp 中 QQuickRangeSliderNode 一致。
 */
class EXWIDGETS_EXPORT ExRangeSliderNode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed WRITE setPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(bool hovered READ isHovered WRITE setHovered NOTIFY hoveredChanged FINAL)
    Q_PROPERTY(qreal implicitHandleWidth READ implicitHandleWidth NOTIFY implicitHandleWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHandleHeight READ implicitHandleHeight NOTIFY implicitHandleHeightChanged FINAL)

public:
    explicit ExRangeSliderNode(bool isFirst, ExRangeSlider *slider);
    ~ExRangeSliderNode() override;

    int value() const;
    void setValue(int value);

    qreal position() const;
    qreal visualPosition() const;

    bool isPressed() const;
    void setPressed(bool pressed);

    bool isHovered() const;
    void setHovered(bool hovered);

    qreal implicitHandleWidth() const;
    qreal implicitHandleHeight() const;

    ExRangeSliderNode *peer() const;

    void invalidateVisualPosition();

public Q_SLOTS:
    void increase();
    void decrease();

Q_SIGNALS:
    void valueChanged();
    void positionChanged();
    void visualPositionChanged();
    void pressedChanged();
    void hoveredChanged();
    void moved();
    void implicitHandleWidthChanged();
    void implicitHandleHeightChanged();

private:
    friend class ExRangeSlider;
    friend class ExRangeSliderPrivate;
    void setValueUnchecked(int value, bool emitValueSignals);
    void emitValuePositionVisualChanged();
    void syncInnerAnimation();

    ExRangeSlider *m_slider = nullptr;
    bool m_isFirst = false;
    int m_value = 0;
    bool m_pressed = false;
    bool m_hovered = false;
    QVariantAnimation *m_innerAnim = nullptr;
};

class ExRangeSliderPrivate;

/**
 * @brief WinUI3 风格的范围选择控件（QWidget 版本的 RangeSlider）。
 *
 * 设计参考自 Qt Quick Controls 的 FluentWinUI3 RangeSlider（见 ExWidgets/RangeSlider.qml）。
 * 颜色完全来自 QPalette（accent/highlight）以及 FluentUI3Colors 中定义的 WinUI3 颜色，
 *
 * 使用整型值（min/max/lower/upper/singleStep/pageStep），API 风格贴近 QAbstractSlider，
 * 同时提供 SnapMode、Tick、Live 拖动等
 */
class EXWIDGETS_EXPORT ExRangeSlider : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(int lowerValue READ lowerValue WRITE setLowerValue NOTIFY lowerValueChanged)
    Q_PROPERTY(int upperValue READ upperValue WRITE setUpperValue NOTIFY upperValueChanged)
    Q_PROPERTY(int singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int pageStep READ pageStep WRITE setPageStep)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode)
    Q_PROPERTY(bool tickPosition READ hasTickPosition WRITE setTickPosition)
    Q_PROPERTY(int tickInterval READ tickInterval WRITE setTickInterval)
    Q_PROPERTY(bool tracking READ hasTracking WRITE setTracking)
    Q_PROPERTY(bool handleFocusRingEnabled READ handleFocusRingEnabled WRITE setHandleFocusRingEnabled)

public:
    enum SnapMode
    {
        NoSnap,
        SnapAlways,
        SnapOnRelease,
    };
    Q_ENUM(SnapMode)

    enum Handle
    {
        NoHandle = 0,
        LowerHandle,
        UpperHandle,
    };
    Q_ENUM(Handle)

    explicit ExRangeSlider(QWidget *parent = nullptr);
    explicit ExRangeSlider(Qt::Orientation orientation, QWidget *parent = nullptr);
    ~ExRangeSlider() override;

    int minimum() const;
    int maximum() const;
    void setMinimum(int min);
    void setMaximum(int max);
    void setRange(int min, int max);

    int lowerValue() const;
    int upperValue() const;
    void setLowerValue(int value);
    void setUpperValue(int value);
    void setValues(int lower, int upper);

    int singleStep() const;
    int pageStep() const;
    void setSingleStep(int step);
    void setPageStep(int step);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    SnapMode snapMode() const;
    void setSnapMode(SnapMode mode);

    bool hasTickPosition() const;
    void setTickPosition(bool enabled);

    int tickInterval() const;
    void setTickInterval(int interval);

    bool hasTracking() const;
    void setTracking(bool enable);

    bool handleFocusRingEnabled() const;
    /** 为 true 且控件有焦点时，在当前激活手柄外绘制 accent 焦点环（默认 false，与 QML 按下无额外描边一致）。 */
    void setHandleFocusRingEnabled(bool enable);

    Handle activeHandle() const;

    ExRangeSliderNode *first() const;
    ExRangeSliderNode *second() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

Q_SIGNALS:
    void lowerValueChanged(int value);
    void upperValueChanged(int value);
    void rangeChanged(int minimum, int maximum);
    void valuesChanged(int lower, int upper);
    void sliderPressed(ExRangeSlider::Handle handle);
    void sliderReleased(ExRangeSlider::Handle handle);
    void sliderMoved(int lower, int upper);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool event(QEvent *event) override;

private:
    Q_DECLARE_PRIVATE(ExRangeSlider)
    ExRangeSliderPrivate *d_ptr;
};
