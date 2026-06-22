#include "exrangeslider.h"

#include "fluentui3colors.h"
#include "fluentui3styleproperties.h"

#include <QApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QHoverEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QResizeEvent>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QtMath>
#include <QFrame>
#include <QLabel>
#include <QStyle>
#include <QStyleOptionFrame>
#include <QStylePainter>

namespace
{
// Match FluentUI3Style: PM_SliderLength adds 20; CC_Slider handle uses outerRadius = qMin(10, dim/2 - 1) → 9 for 20×20.
constexpr int kSliderHandlePmHalf = 10;
constexpr qreal kSliderHandlePaintRadius = 9.0;
constexpr int kGrooveThickness = 4;
constexpr int kTickThickness = 4;
constexpr int kTickGap = 6;
constexpr int kAnimDuration = 300;
constexpr int kGapAboveHandle = 4;
constexpr int kMinExtent = 24;
constexpr int kCrossExtent = 32;

constexpr qreal kInnerRadiusNormal = kSliderHandlePaintRadius * 0.55;
constexpr qreal kInnerRadiusHovered = kSliderHandlePaintRadius * 0.65;
constexpr qreal kInnerRadiusPressed = kSliderHandlePaintRadius * 0.40;

bool isDarkPalette(const QPalette &palette)
{
    return palette.color(QPalette::Window).lightness() < 128;
}

QColor winColor(const QPalette &palette, WINUI3Color which)
{
    const int idx = isDarkPalette(palette) ? 1 : 0;
    return WINUI3Colors[idx][which];
}

QColor accentColor(const QPalette &palette)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    return palette.color(QPalette::Accent);
#else
    return palette.color(QPalette::Highlight);
#endif
}

int clampInt(int value, int lo, int hi)
{
    return qBound(qMin(lo, hi), value, qMax(lo, hi));
}

void drawSliderHandleShadow(QPainter *painter, const QPointF &center, qreal outerRadius, const QPalette &palette)
{
    painter->setPen(Qt::NoPen);
    const qreal shadowStrength = isDarkPalette(palette) ? 1.0 : 0.7;
    for (int i = 5; i >= 1; --i)
    {
        const int alpha = qRound((40.0 / i) * shadowStrength);
        painter->setBrush(QColor(0, 0, 0, alpha));
        const qreal offset = i * 0.8;
        painter->drawEllipse(center, outerRadius + offset, outerRadius + offset);
    }
}
} // namespace

// =============================================================================
// ExRangeSliderNode
// =============================================================================

ExRangeSliderNode::ExRangeSliderNode(bool isFirst, ExRangeSlider *slider)
    : QObject(slider)
    , m_slider(slider)
    , m_isFirst(isFirst)
    , m_value(isFirst ? 0 : 100)
{
    m_innerAnim = new QVariantAnimation(this);
    m_innerAnim->setDuration(kAnimDuration);
    m_innerAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_innerAnim->setStartValue(kInnerRadiusNormal);
    m_innerAnim->setEndValue(kInnerRadiusNormal);
    QObject::connect(m_innerAnim, &QVariantAnimation::valueChanged, slider,
                     [slider]() { slider->update(); });
}

ExRangeSliderNode::~ExRangeSliderNode() = default;

ExRangeSliderNode *ExRangeSliderNode::peer() const
{
    return m_isFirst ? m_slider->second() : m_slider->first();
}

int ExRangeSliderNode::value() const
{
    return m_value;
}

void ExRangeSliderNode::setValue(int value)
{
    const int mn = m_slider->minimum();
    const int mx = m_slider->maximum();
    const int lo = qMin(mn, mx);
    const int hi = qMax(mn, mx);
    value = qBound(lo, value, hi);

    if (m_isFirst)
        value = qMin(value, peer()->value());
    else
        value = qMax(value, peer()->value());

    if (m_value == value)
        return;

    const qreal oldPos = position();
    m_value = value;
    emit valueChanged();
    if (!qFuzzyCompare(oldPos, position()))
    {
        emit positionChanged();
        emit visualPositionChanged();
    }
    m_slider->update();
}

void ExRangeSliderNode::setValueUnchecked(int value, bool emitValueSignals)
{
    if (m_value == value)
        return;
    m_value = value;
    if (emitValueSignals)
    {
        emit valueChanged();
        emit positionChanged();
        emit visualPositionChanged();
    }
    m_slider->update();
}

void ExRangeSliderNode::emitValuePositionVisualChanged()
{
    emit valueChanged();
    emit positionChanged();
    emit visualPositionChanged();
    m_slider->update();
}

qreal ExRangeSliderNode::position() const
{
    const int mn = m_slider->minimum();
    const int mx = m_slider->maximum();
    if (mx == mn)
        return 0.0;
    return qreal(m_value - mn) / qreal(mx - mn);
}

qreal ExRangeSliderNode::visualPosition() const
{
    const qreal p = position();
    if (m_slider->orientation() == Qt::Vertical || m_slider->layoutDirection() == Qt::RightToLeft)
        return 1.0 - p;
    return p;
}

bool ExRangeSliderNode::isPressed() const
{
    return m_pressed;
}

void ExRangeSliderNode::setPressed(bool pressed)
{
    if (m_pressed == pressed)
        return;
    m_pressed = pressed;
    emit pressedChanged();
    syncInnerAnimation();
    m_slider->update();
}

bool ExRangeSliderNode::isHovered() const
{
    return m_hovered;
}

void ExRangeSliderNode::setHovered(bool hovered)
{
    if (m_hovered == hovered)
        return;
    m_hovered = hovered;
    emit hoveredChanged();
    syncInnerAnimation();
    m_slider->update();
}

qreal ExRangeSliderNode::implicitHandleWidth() const
{
    return qreal(2 * kSliderHandlePmHalf);
}

qreal ExRangeSliderNode::implicitHandleHeight() const
{
    return qreal(2 * kSliderHandlePmHalf);
}

void ExRangeSliderNode::invalidateVisualPosition()
{
    emit visualPositionChanged();
}

void ExRangeSliderNode::increase()
{
    const int step = qMax(1, m_slider->singleStep());
    setValue(m_value + step);
}

void ExRangeSliderNode::decrease()
{
    const int step = qMax(1, m_slider->singleStep());
    setValue(m_value - step);
}

void ExRangeSliderNode::syncInnerAnimation()
{
    qreal target = kInnerRadiusNormal;
    if (!m_slider->isEnabled())
        target = kInnerRadiusNormal;
    else if (m_pressed)
        target = kInnerRadiusPressed;
    else if (m_hovered)
        target = kInnerRadiusHovered;

    const qreal current = m_innerAnim->currentValue().isValid() ? m_innerAnim->currentValue().toReal()
                                                                : kInnerRadiusNormal;
    if (qFuzzyCompare(current, target))
        return;

    m_innerAnim->stop();
    m_innerAnim->setStartValue(current);
    m_innerAnim->setEndValue(target);
    m_innerAnim->start();
}

// =============================================================================
// RangeSliderTipLabel
// =============================================================================

class RangeSliderTipLabel : public QLabel
{
public:
    explicit RangeSliderTipLabel(QWidget *parent)
        : QLabel(parent)
    {
        setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::BypassGraphicsProxyWidget);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setFrameStyle(QFrame::NoFrame);
        setAlignment(Qt::AlignCenter);

        QFont tipFont = QApplication::font();
        tipFont.setHintingPreference(QFont::PreferNoHinting);
        setFont(tipFont);
        setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, this));
    }

protected:
    void paintEvent(QPaintEvent *ev) override
    {
        QStylePainter p(this);
        QStyleOptionFrame opt;
        opt.initFrom(this);
        p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
        p.end();

        QLabel::paintEvent(ev);
    }
};

// =============================================================================
// ExRangeSliderPrivate
// =============================================================================

class ExRangeSliderPrivate
{
    Q_DECLARE_PUBLIC(ExRangeSlider)
public:
    explicit ExRangeSliderPrivate(ExRangeSlider *q);

    qreal availableLength() const;
    qreal positionOf(int value) const;
    int valueOfPosition(qreal pos) const;
    int snapValue(int value) const;
    QPointF handleCenter(int value) const;
    QRectF grooveRect() const;
    ExRangeSlider::Handle handleAt(const QPointF &pos) const;

    void setLowerValueInternal(int value, bool fromUser);
    void setUpperValueInternal(int value, bool fromUser);
    void setActiveHandle(ExRangeSlider::Handle h);
    void setHoveredHandle(ExRangeSlider::Handle h);
    void setPressedHandle(ExRangeSlider::Handle h);
    void showPressedHandleToolTip(int value);
    void hidePressedHandleToolTip();
    qreal currentInnerRadius(ExRangeSlider::Handle h) const;
    void updateAccessibility();
    void paintHandle(QPainter *painter, const QPointF &center, qreal inner, bool focusRing,
                     ExRangeSlider::Handle handle) const;

    ExRangeSliderNode *nodeForHandle(ExRangeSlider::Handle h) const;

    void bindSignals();

    ExRangeSlider *q_ptr;

    int m_minimum = 0;
    int m_maximum = 100;
    int m_singleStep = 1;
    int m_pageStep = 10;

    Qt::Orientation m_orientation = Qt::Horizontal;
    ExRangeSlider::SnapMode m_snapMode = ExRangeSlider::NoSnap;
    bool m_tickPosition = false;
    int m_tickInterval = 0;
    bool m_tracking = true;
    bool m_handleFocusRingEnabled = false;

    ExRangeSlider::Handle m_pressedHandle = ExRangeSlider::NoHandle;
    ExRangeSlider::Handle m_hoveredHandle = ExRangeSlider::NoHandle;
    ExRangeSlider::Handle m_activeHandle = ExRangeSlider::LowerHandle;

    qreal m_pressOffset = 0.0;

    ExRangeSliderNode *m_first = nullptr;
    ExRangeSliderNode *m_second = nullptr;
    RangeSliderTipLabel *m_valueTip = nullptr;
};

ExRangeSliderPrivate::ExRangeSliderPrivate(ExRangeSlider *q)
    : q_ptr(q)
{
    m_second = new ExRangeSliderNode(false, q);
    m_first = new ExRangeSliderNode(true, q);
    bindSignals();
}

ExRangeSliderNode *ExRangeSliderPrivate::nodeForHandle(ExRangeSlider::Handle h) const
{
    if (h == ExRangeSlider::LowerHandle)
        return m_first;
    if (h == ExRangeSlider::UpperHandle)
        return m_second;
    return nullptr;
}

void ExRangeSliderPrivate::bindSignals()
{
    Q_Q(ExRangeSlider);
    QObject::connect(m_first, &ExRangeSliderNode::valueChanged, q,
                     [q]()
                     {
                         emit q->lowerValueChanged(q->first()->value());
                         emit q->valuesChanged(q->first()->value(), q->second()->value());
                     });
    QObject::connect(m_second, &ExRangeSliderNode::valueChanged, q,
                     [q]()
                     {
                         emit q->upperValueChanged(q->second()->value());
                         emit q->valuesChanged(q->first()->value(), q->second()->value());
                     });
}

qreal ExRangeSliderPrivate::availableLength() const
{
    Q_Q(const ExRangeSlider);
    const QSize sz = q->size();
    const qreal raw = m_orientation == Qt::Horizontal ? sz.width() : sz.height();
    return qMax<qreal>(0, raw - 2 * kSliderHandlePmHalf);
}

qreal ExRangeSliderPrivate::positionOf(int value) const
{
    Q_Q(const ExRangeSlider);
    const qreal extent = availableLength();
    if (extent <= 0 || m_maximum == m_minimum)
        return kSliderHandlePmHalf;

    const qreal t = qreal(value - m_minimum) / qreal(m_maximum - m_minimum);
    if (m_orientation == Qt::Horizontal)
    {
        const bool reversed = q->layoutDirection() == Qt::RightToLeft;
        return reversed ? (kSliderHandlePmHalf + (1.0 - t) * extent) : (kSliderHandlePmHalf + t * extent);
    }
    return kSliderHandlePmHalf + (1.0 - t) * extent;
}

int ExRangeSliderPrivate::valueOfPosition(qreal pos) const
{
    Q_Q(const ExRangeSlider);
    const qreal extent = availableLength();
    if (extent <= 0 || m_maximum == m_minimum)
        return m_minimum;

    qreal t = (pos - kSliderHandlePmHalf) / extent;
    if (m_orientation == Qt::Horizontal)
    {
        if (q->layoutDirection() == Qt::RightToLeft)
            t = 1.0 - t;
    }
    else
    {
        t = 1.0 - t;
    }
    t = qBound<qreal>(0.0, t, 1.0);
    return qRound(m_minimum + t * (m_maximum - m_minimum));
}

int ExRangeSliderPrivate::snapValue(int value) const
{
    if (m_singleStep <= 0)
        return value;
    const int rel = value - m_minimum;
    const int snapped = (rel / m_singleStep) * m_singleStep
                        + ((rel % m_singleStep) * 2 >= m_singleStep ? m_singleStep : 0);
    return clampInt(m_minimum + snapped, m_minimum, m_maximum);
}

QPointF ExRangeSliderPrivate::handleCenter(int value) const
{
    Q_Q(const ExRangeSlider);
    const qreal pos = positionOf(value);
    if (m_orientation == Qt::Horizontal)
        return QPointF(pos, q->height() * 0.5);
    return QPointF(q->width() * 0.5, pos);
}

void ExRangeSliderPrivate::showPressedHandleToolTip(int value)
{
    Q_Q(ExRangeSlider);
    const QVariant showTip = q->property(SliderValueTipProperty);
    if (!showTip.isValid() || !showTip.toBool())
        return;
    if (m_pressedHandle == ExRangeSlider::NoHandle || !q->isEnabled() || !q->isVisible())
        return;

    if (!m_valueTip)
    {
        m_valueTip = new RangeSliderTipLabel(nullptr);
        QObject::connect(q, &QObject::destroyed, m_valueTip, &QObject::deleteLater);
        QApplication::style()->polish(m_valueTip);
    }

    m_valueTip->setText(QString::number(value));
    m_valueTip->adjustSize();

    const QPointF center = handleCenter(value);
    const QPoint centerGlobal = q->mapToGlobal(QPoint(qRound(center.x()), qRound(center.y())));
    int x = 0;
    int y = 0;
    if (m_orientation == Qt::Horizontal)
    {
        x = centerGlobal.x() - m_valueTip->width() / 2;
        y = centerGlobal.y() - static_cast<int>(kSliderHandlePaintRadius) - kGapAboveHandle - m_valueTip->height();
    }
    else
    {
        x = centerGlobal.x() - static_cast<int>(kSliderHandlePaintRadius) - kGapAboveHandle - m_valueTip->width();
        y = centerGlobal.y() - m_valueTip->height() / 2;
    }
    m_valueTip->move(x, y);
    m_valueTip->raise();
    m_valueTip->show();
}

void ExRangeSliderPrivate::hidePressedHandleToolTip()
{
    if (m_valueTip)
        m_valueTip->hide();
}

QRectF ExRangeSliderPrivate::grooveRect() const
{
    Q_Q(const ExRangeSlider);
    const QSize sz = q->size();
    const auto offset = /*kSliderHandlePmHalf*/2;
    if (m_orientation == Qt::Horizontal)
    {
        const qreal cy = sz.height() * 0.5;
        return QRectF(offset, cy - kGrooveThickness * 0.5,
                      qMax<qreal>(0, sz.width() - 2 * offset), kGrooveThickness);
    }
    const qreal cx = sz.width() * 0.5;
    return QRectF(cx - kGrooveThickness * 0.5, offset,
                  kGrooveThickness, qMax<qreal>(0, sz.height() - 2 * offset));
}

ExRangeSlider::Handle ExRangeSliderPrivate::handleAt(const QPointF &pos) const
{
    const QPointF lowerC = handleCenter(m_first->value());
    const QPointF upperC = handleCenter(m_second->value());
    const qreal r2 = kSliderHandlePaintRadius * kSliderHandlePaintRadius;

    const QPointF dl = pos - lowerC;
    const QPointF du = pos - upperC;
    const qreal distLower = dl.x() * dl.x() + dl.y() * dl.y();
    const qreal distUpper = du.x() * du.x() + du.y() * du.y();
    const bool inLower = distLower <= r2;
    const bool inUpper = distUpper <= r2;
    if (inLower && inUpper)
    {
        if (qFuzzyCompare(qreal(distLower), qreal(distUpper)))
        {
            const qreal lowerPos = positionOf(m_first->value());
            const qreal pressPos = m_orientation == Qt::Horizontal ? pos.x() : pos.y();
            return pressPos < lowerPos ? ExRangeSlider::LowerHandle : ExRangeSlider::UpperHandle;
        }
        return distLower < distUpper ? ExRangeSlider::LowerHandle : ExRangeSlider::UpperHandle;
    }
    if (inLower)
        return ExRangeSlider::LowerHandle;
    if (inUpper)
        return ExRangeSlider::UpperHandle;
    return ExRangeSlider::NoHandle;
}

void ExRangeSliderPrivate::setLowerValueInternal(int value, bool fromUser)
{
    Q_Q(ExRangeSlider);
    const qreal oldPos = m_first->position();
    m_first->setValue(value);
    if (fromUser)
    {
        emit q->sliderMoved(m_first->value(), m_second->value());
        if (!qFuzzyCompare(oldPos, m_first->position()))
            emit m_first->moved();
    }
    updateAccessibility();
}

void ExRangeSliderPrivate::setUpperValueInternal(int value, bool fromUser)
{
    Q_Q(ExRangeSlider);
    const qreal oldPos = m_second->position();
    m_second->setValue(value);
    if (fromUser)
    {
        emit q->sliderMoved(m_first->value(), m_second->value());
        if (!qFuzzyCompare(oldPos, m_second->position()))
            emit m_second->moved();
    }
    updateAccessibility();
}

void ExRangeSliderPrivate::setActiveHandle(ExRangeSlider::Handle h)
{
    if (h == ExRangeSlider::NoHandle || m_activeHandle == h)
        return;
    m_activeHandle = h;
}

void ExRangeSliderPrivate::setHoveredHandle(ExRangeSlider::Handle h)
{
    if (m_hoveredHandle == h)
        return;
    m_hoveredHandle = h;
    m_first->setHovered(h == ExRangeSlider::LowerHandle);
    m_second->setHovered(h == ExRangeSlider::UpperHandle);
}

void ExRangeSliderPrivate::setPressedHandle(ExRangeSlider::Handle h)
{
    if (m_pressedHandle == h)
        return;
    m_pressedHandle = h;
    m_first->setPressed(h == ExRangeSlider::LowerHandle);
    m_second->setPressed(h == ExRangeSlider::UpperHandle);
}

qreal ExRangeSliderPrivate::currentInnerRadius(ExRangeSlider::Handle h) const
{
    ExRangeSliderNode *n = nodeForHandle(h);
    if (!n)
        return kInnerRadiusNormal;
    const QVariant v = n->m_innerAnim->currentValue();
    return v.isValid() ? v.toReal() : kInnerRadiusNormal;
}

void ExRangeSliderPrivate::updateAccessibility()
{
}

void ExRangeSliderPrivate::paintHandle(QPainter *painter,
                                         const QPointF &center,
                                         qreal inner,
                                         bool focusRing,
                                         ExRangeSlider::Handle handle) const
{
    Q_Q(const ExRangeSlider);
    const QPalette pal = q->palette();
    const bool enabled = q->isEnabled();
    ExRangeSliderNode *node = nodeForHandle(handle);

    const QColor outerFill = winColor(pal, controlFillSolid);
    const QColor outerStroke = winColor(pal, controlStrokeSecondary);
    QColor inner_color = accentColor(pal);

    if (!enabled)
        inner_color = winColor(pal, fillAccentDisabled);
    else if (node)
    {
        // 与 RangeSlider.qml 中 indicator 的 alpha 一致
        if (node->isPressed())
            inner_color.setAlphaF(0.8);
        else if (node->isHovered())
            inner_color.setAlphaF(0.9020);
    }

    drawSliderHandleShadow(painter, center, kSliderHandlePaintRadius, pal);

    painter->setPen(Qt::NoPen);
    painter->setBrush(outerFill);
    painter->drawEllipse(center, kSliderHandlePaintRadius, kSliderHandlePaintRadius);

    painter->setBrush(inner_color);
    painter->drawEllipse(center, inner, inner);

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(outerStroke, 1.0));
    painter->drawEllipse(center, kSliderHandlePaintRadius + 0.5, kSliderHandlePaintRadius + 0.5);

    if (focusRing && q->hasFocus() && m_handleFocusRingEnabled)
    {
        QColor ring = accentColor(pal);
        ring.setAlpha(isDarkPalette(pal) ? 200 : 130);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(ring, 2.0));
        painter->drawEllipse(center, kSliderHandlePaintRadius + 2.0, kSliderHandlePaintRadius + 2.0);
    }
}

// =============================================================================
// ExRangeSlider
// =============================================================================

ExRangeSlider::ExRangeSlider(QWidget *parent)
    : ExRangeSlider(Qt::Horizontal, parent)
{
}

ExRangeSlider::ExRangeSlider(Qt::Orientation orientation, QWidget *parent)
    : QWidget(parent), d_ptr(new ExRangeSliderPrivate(this))
{
    Q_D(ExRangeSlider);
    d->m_orientation = orientation;

    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    setCursor(Qt::ArrowCursor);
    setTickPosition(true);
    setTickInterval(10);

    if (orientation == Qt::Horizontal)
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    else
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    d->updateAccessibility();

    connect(this, &ExRangeSlider::sliderPressed, this, [this, d](ExRangeSlider::Handle handle) {
        if (handle == NoHandle)
            return;
        const int v = handle == LowerHandle ? lowerValue() : upperValue();
        d->showPressedHandleToolTip(v);
    });
    connect(this, &ExRangeSlider::sliderMoved, this, [this, d](int lower, int upper) {
        if (d->m_pressedHandle == NoHandle)
            return;
        const int v = d->m_pressedHandle == LowerHandle ? lower : upper;
        d->showPressedHandleToolTip(v);
    });
    connect(this, &ExRangeSlider::sliderReleased, this, [d](ExRangeSlider::Handle) {
        d->hidePressedHandleToolTip();
    });
}

ExRangeSlider::~ExRangeSlider()
{
    delete d_ptr;
}

ExRangeSliderNode *ExRangeSlider::first() const
{
    Q_D(const ExRangeSlider);
    return d->m_first;
}

ExRangeSliderNode *ExRangeSlider::second() const
{
    Q_D(const ExRangeSlider);
    return d->m_second;
}

int ExRangeSlider::minimum() const
{
    Q_D(const ExRangeSlider);
    return d->m_minimum;
}

int ExRangeSlider::maximum() const
{
    Q_D(const ExRangeSlider);
    return d->m_maximum;
}

void ExRangeSlider::setMinimum(int min)
{
    setRange(min, qMax(min, maximum()));
}

void ExRangeSlider::setMaximum(int max)
{
    setRange(qMin(minimum(), max), max);
}

void ExRangeSlider::setRange(int min, int max)
{
    Q_D(ExRangeSlider);
    if (min > max)
        qSwap(min, max);
    if (d->m_minimum == min && d->m_maximum == max)
        return;

    d->m_minimum = min;
    d->m_maximum = max;

    emit rangeChanged(min, max);
    d->m_second->setValue(d->m_second->value());
    d->m_first->setValue(d->m_first->value());
    update();
}

int ExRangeSlider::lowerValue() const
{
    Q_D(const ExRangeSlider);
    return d->m_first->value();
}

int ExRangeSlider::upperValue() const
{
    Q_D(const ExRangeSlider);
    return d->m_second->value();
}

void ExRangeSlider::setLowerValue(int value)
{
    Q_D(ExRangeSlider);
    d->setLowerValueInternal(value, false);
}

void ExRangeSlider::setUpperValue(int value)
{
    Q_D(ExRangeSlider);
    d->setUpperValueInternal(value, false);
}

void ExRangeSlider::setValues(int lower, int upper)
{
    Q_D(ExRangeSlider);
    if (lower > upper)
        qSwap(lower, upper);

    lower = clampInt(lower, d->m_minimum, d->m_maximum);
    upper = clampInt(upper, d->m_minimum, d->m_maximum);

    const int oldF = d->m_first->value();
    const int oldS = d->m_second->value();

    d->m_first->blockSignals(true);
    d->m_second->blockSignals(true);
    d->m_first->setValueUnchecked(lower, false);
    d->m_second->setValueUnchecked(upper, false);
    d->m_first->blockSignals(false);
    d->m_second->blockSignals(false);

    if (d->m_first->value() != oldF)
        d->m_first->emitValuePositionVisualChanged();
    if (d->m_second->value() != oldS)
        d->m_second->emitValuePositionVisualChanged();

    if (d->m_first->value() != oldF || d->m_second->value() != oldS)
    {
        if (d->m_first->value() != oldF)
            emit lowerValueChanged(d->m_first->value());
        if (d->m_second->value() != oldS)
            emit upperValueChanged(d->m_second->value());
        emit valuesChanged(d->m_first->value(), d->m_second->value());
        update();
    }
}

int ExRangeSlider::singleStep() const
{
    Q_D(const ExRangeSlider);
    return d->m_singleStep;
}

int ExRangeSlider::pageStep() const
{
    Q_D(const ExRangeSlider);
    return d->m_pageStep;
}

void ExRangeSlider::setSingleStep(int step)
{
    Q_D(ExRangeSlider);
    d->m_singleStep = qMax(0, step);
}

void ExRangeSlider::setPageStep(int step)
{
    Q_D(ExRangeSlider);
    d->m_pageStep = qMax(0, step);
}

Qt::Orientation ExRangeSlider::orientation() const
{
    Q_D(const ExRangeSlider);
    return d->m_orientation;
}

void ExRangeSlider::setOrientation(Qt::Orientation orientation)
{
    Q_D(ExRangeSlider);
    if (d->m_orientation == orientation)
        return;
    d->m_orientation = orientation;
    if (orientation == Qt::Horizontal)
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    else
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    d->m_first->invalidateVisualPosition();
    d->m_second->invalidateVisualPosition();
    updateGeometry();
    update();
}

ExRangeSlider::SnapMode ExRangeSlider::snapMode() const
{
    Q_D(const ExRangeSlider);
    return d->m_snapMode;
}

void ExRangeSlider::setSnapMode(SnapMode mode)
{
    Q_D(ExRangeSlider);
    d->m_snapMode = mode;
}

bool ExRangeSlider::hasTickPosition() const
{
    Q_D(const ExRangeSlider);
    return d->m_tickPosition;
}

void ExRangeSlider::setTickPosition(bool enabled)
{
    Q_D(ExRangeSlider);
    if (d->m_tickPosition == enabled)
        return;
    d->m_tickPosition = enabled;
    update();
}

int ExRangeSlider::tickInterval() const
{
    Q_D(const ExRangeSlider);
    return d->m_tickInterval;
}

void ExRangeSlider::setTickInterval(int interval)
{
    Q_D(ExRangeSlider);
    if (d->m_tickInterval == interval)
        return;
    d->m_tickInterval = qMax(0, interval);
    update();
}

bool ExRangeSlider::hasTracking() const
{
    Q_D(const ExRangeSlider);
    return d->m_tracking;
}

void ExRangeSlider::setTracking(bool enable)
{
    Q_D(ExRangeSlider);
    d->m_tracking = enable;
}

bool ExRangeSlider::handleFocusRingEnabled() const
{
    Q_D(const ExRangeSlider);
    return d->m_handleFocusRingEnabled;
}

void ExRangeSlider::setHandleFocusRingEnabled(bool enable)
{
    Q_D(ExRangeSlider);
    if (d->m_handleFocusRingEnabled == enable)
        return;
    d->m_handleFocusRingEnabled = enable;
    update();
}

ExRangeSlider::Handle ExRangeSlider::activeHandle() const
{
    Q_D(const ExRangeSlider);
    return d->m_activeHandle;
}

QSize ExRangeSlider::sizeHint() const
{
    Q_D(const ExRangeSlider);
    if (d->m_orientation == Qt::Horizontal)
        return QSize(160, kCrossExtent);
    return QSize(kCrossExtent, 160);
}

QSize ExRangeSlider::minimumSizeHint() const
{
    Q_D(const ExRangeSlider);
    if (d->m_orientation == Qt::Horizontal)
        return QSize(kMinExtent + 2 * kSliderHandlePmHalf, kCrossExtent);
    return QSize(kCrossExtent, kMinExtent + 2 * kSliderHandlePmHalf);
}

void ExRangeSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    Q_D(ExRangeSlider);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    const QPalette pal = palette();
    const bool enabled = isEnabled();

    const QRectF groove = d->grooveRect();
    QColor grooveColor = winColor(pal, controlStrongFill);
    if (!enabled)
    {
        QColor c = grooveColor;
        c.setAlphaF(c.alphaF() * 0.6);
        grooveColor = c;
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(grooveColor);
    painter.drawRoundedRect(groove, kGrooveThickness * 0.5, kGrooveThickness * 0.5);

    const QPointF lowerC = d->handleCenter(d->m_first->value());
    const QPointF upperC = d->handleCenter(d->m_second->value());

    QRectF activeTrack;
    if (d->m_orientation == Qt::Horizontal)
    {
        const qreal x1 = qMin(lowerC.x(), upperC.x());
        const qreal x2 = qMax(lowerC.x(), upperC.x());
        activeTrack = QRectF(x1, groove.top(), x2 - x1, groove.height());
    }
    else
    {
        const qreal y1 = qMin(lowerC.y(), upperC.y());
        const qreal y2 = qMax(lowerC.y(), upperC.y());
        activeTrack = QRectF(groove.left(), y1, groove.width(), y2 - y1);
    }

    QColor trackColor = accentColor(pal);
    if (!enabled)
        trackColor = winColor(pal, controlStrongFill);
    painter.setBrush(trackColor);
    painter.drawRoundedRect(activeTrack, kGrooveThickness * 0.5, kGrooveThickness * 0.5);

    if (d->m_tickPosition)
    {
        int interval = d->m_tickInterval;
        if (interval <= 0)
            interval = qMax(1, d->m_singleStep);
        if (d->m_maximum != d->m_minimum && interval > 0)
        {
            painter.setPen(Qt::NoPen);
            const QColor tickColor = /*winColor(pal, controlStrongFill)*/palette().text().color();
            painter.setBrush(tickColor);

            for (int v = d->m_minimum; v <= d->m_maximum; v += interval)
            {
                const QPointF c = d->handleCenter(v);
                if (d->m_orientation == Qt::Horizontal)
                {
                    QRectF rTop(c.x() - 0.5, groove.top() - kTickGap - kTickThickness, 1.0, kTickThickness);
                    QRectF rBottom(c.x() - 0.5, groove.bottom() + kTickGap, 1.0, kTickThickness);
                    painter.drawRect(rTop);
                    painter.drawRect(rBottom);
                }
                else
                {
                    QRectF rLeft(groove.left() - kTickGap - kTickThickness, c.y() - 0.5, kTickThickness, 1.0);
                    QRectF rRight(groove.right() + kTickGap, c.y() - 0.5, kTickThickness, 1.0);
                    painter.drawRect(rLeft);
                    painter.drawRect(rRight);
                }
                if (v == d->m_maximum)
                    break;
            }
        }
    }

    const ExRangeSlider::Handle topHandle = (d->m_pressedHandle != NoHandle) ? d->m_pressedHandle
                                                                               : d->m_activeHandle;

    auto drawOne = [&](ExRangeSlider::Handle h)
    {
        const QPointF c = (h == LowerHandle) ? lowerC : upperC;
        const qreal inner = d->currentInnerRadius(h);
        const bool focusRing = (h == topHandle) && hasFocus();
        d->paintHandle(&painter, c, inner, focusRing, h);
    };

    if (topHandle == LowerHandle)
    {
        drawOne(UpperHandle);
        drawOne(LowerHandle);
    }
    else
    {
        drawOne(LowerHandle);
        drawOne(UpperHandle);
    }
}

void ExRangeSlider::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

void ExRangeSlider::mousePressEvent(QMouseEvent *event)
{
    Q_D(ExRangeSlider);
    if (event->button() != Qt::LeftButton || !isEnabled())
    {
        QWidget::mousePressEvent(event);
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF pos = event->position();
#else
    QPointF pos = event->pos();
#endif

    ExRangeSlider::Handle hit = d->handleAt(pos);

    if (hit == NoHandle)
    {
        const qreal pressPos = d->m_orientation == Qt::Horizontal ? pos.x() : pos.y();
        const qreal lowerPos = d->positionOf(d->m_first->value());
        const qreal upperPos = d->positionOf(d->m_second->value());
        const qreal distLower = qAbs(pressPos - lowerPos);
        const qreal distUpper = qAbs(pressPos - upperPos);
        if (qFuzzyCompare(distLower, distUpper))
        {
            const bool reversed = (d->m_orientation == Qt::Horizontal && layoutDirection() == Qt::RightToLeft)
                                  || d->m_orientation == Qt::Vertical;
            const bool towardsLower = reversed ? (pressPos > lowerPos) : (pressPos < lowerPos);
            hit = towardsLower ? LowerHandle : UpperHandle;
        }
        else
        {
            hit = distLower < distUpper ? LowerHandle : UpperHandle;
        }
    }

    d->setPressedHandle(hit);
    d->setActiveHandle(hit);

    const QPointF handleC = d->handleCenter(hit == LowerHandle ? d->m_first->value() : d->m_second->value());
    d->m_pressOffset = (d->m_orientation == Qt::Horizontal ? pos.x() - handleC.x() : pos.y() - handleC.y());

    if (d->handleAt(pos) == NoHandle)
    {
        const qreal trackPos = (d->m_orientation == Qt::Horizontal ? pos.x() : pos.y());
        int value = d->valueOfPosition(trackPos);
        if (d->m_snapMode == SnapAlways)
            value = d->snapValue(value);
        if (hit == LowerHandle)
            d->setLowerValueInternal(value, true);
        else
            d->setUpperValueInternal(value, true);
        d->m_pressOffset = 0.0;
    }

    emit sliderPressed(hit);
    event->accept();
    update();
}

void ExRangeSlider::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(ExRangeSlider);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF pos = event->position();
#else
    QPointF pos = event->pos();
#endif

    if (d->m_pressedHandle == NoHandle)
    {
        d->setHoveredHandle(d->handleAt(pos));
        QWidget::mouseMoveEvent(event);
        return;
    }

    if (!isEnabled())
        return;

    const qreal coord = (d->m_orientation == Qt::Horizontal ? pos.x() : pos.y()) - d->m_pressOffset;
    int value = d->valueOfPosition(coord);
    if (d->m_snapMode == SnapAlways)
        value = d->snapValue(value);

    if (d->m_tracking)
    {
        if (d->m_pressedHandle == LowerHandle)
            d->setLowerValueInternal(value, true);
        else
            d->setUpperValueInternal(value, true);
    }
    else
    {
        emit sliderMoved(d->m_pressedHandle == LowerHandle ? value : d->m_first->value(),
                         d->m_pressedHandle == UpperHandle ? value : d->m_second->value());
        update();
    }
    event->accept();
}

void ExRangeSlider::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(ExRangeSlider);
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    const ExRangeSlider::Handle released = d->m_pressedHandle;
    if (released == NoHandle)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF pos = event->position();
#else
    QPointF pos = event->pos();
#endif

    const qreal coord = (d->m_orientation == Qt::Horizontal ? pos.x() : pos.y()) - d->m_pressOffset;
    int value = d->valueOfPosition(coord);
    if (d->m_snapMode != NoSnap)
        value = d->snapValue(value);

    if (released == LowerHandle)
        d->setLowerValueInternal(value, true);
    else
        d->setUpperValueInternal(value, true);

    d->setPressedHandle(NoHandle);
    d->setHoveredHandle(d->handleAt(pos));

    emit sliderReleased(released);
    event->accept();
    update();
}

void ExRangeSlider::wheelEvent(QWheelEvent *event)
{
    Q_D(ExRangeSlider);
    if (!isEnabled())
    {
        QWidget::wheelEvent(event);
        return;
    }

    const int delta = event->angleDelta().y();
    if (delta == 0)
    {
        event->ignore();
        return;
    }

    const int step = qMax(1, d->m_singleStep);
    const int direction = delta > 0 ? 1 : -1;
    if (d->m_activeHandle == LowerHandle)
        d->setLowerValueInternal(d->m_first->value() + direction * step, true);
    else
        d->setUpperValueInternal(d->m_second->value() + direction * step, true);
    event->accept();
}

void ExRangeSlider::keyPressEvent(QKeyEvent *event)
{
    Q_D(ExRangeSlider);
    if (!isEnabled())
    {
        QWidget::keyPressEvent(event);
        return;
    }

    const bool horizontal = (d->m_orientation == Qt::Horizontal);
    const int step = qMax(1, d->m_singleStep);
    const int page = qMax(step, d->m_pageStep);
    int delta = 0;
    bool switchHandle = false;
    ExRangeSlider::Handle nextHandle = d->m_activeHandle;

    switch (event->key())
    {
    case Qt::Key_Left:
        if (horizontal)
            delta = (layoutDirection() == Qt::RightToLeft) ? +step : -step;
        else
            switchHandle = true, nextHandle = LowerHandle;
        break;
    case Qt::Key_Right:
        if (horizontal)
            delta = (layoutDirection() == Qt::RightToLeft) ? -step : +step;
        else
            switchHandle = true, nextHandle = UpperHandle;
        break;
    case Qt::Key_Up:
        if (horizontal)
            switchHandle = true, nextHandle = UpperHandle;
        else
            delta = +step;
        break;
    case Qt::Key_Down:
        if (horizontal)
            switchHandle = true, nextHandle = LowerHandle;
        else
            delta = -step;
        break;
    case Qt::Key_PageUp:
        delta = +page;
        break;
    case Qt::Key_PageDown:
        delta = -page;
        break;
    case Qt::Key_Home:
        if (d->m_activeHandle == LowerHandle)
            d->setLowerValueInternal(d->m_minimum, true);
        else
            d->setUpperValueInternal(d->m_first->value(), true);
        event->accept();
        return;
    case Qt::Key_End:
        if (d->m_activeHandle == LowerHandle)
            d->setLowerValueInternal(d->m_second->value(), true);
        else
            d->setUpperValueInternal(d->m_maximum, true);
        event->accept();
        return;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        QWidget::keyPressEvent(event);
        return;
    default:
        QWidget::keyPressEvent(event);
        return;
    }

    if (switchHandle)
    {
        d->setActiveHandle(nextHandle);
        update();
        event->accept();
        return;
    }

    if (delta != 0)
    {
        d->setPressedHandle(d->m_activeHandle);
        if (d->m_activeHandle == LowerHandle)
            d->setLowerValueInternal(d->m_first->value() + delta, true);
        else
            d->setUpperValueInternal(d->m_second->value() + delta, true);
        event->accept();
    }
}

void ExRangeSlider::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(ExRangeSlider);
    if (!event->isAutoRepeat())
    {
        if (d->m_pressedHandle != NoHandle)
            d->setPressedHandle(NoHandle);
    }
    QWidget::keyReleaseEvent(event);
}

void ExRangeSlider::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    update();
}

void ExRangeSlider::focusOutEvent(QFocusEvent *event)
{
    Q_D(ExRangeSlider);
    QWidget::focusOutEvent(event);
    d->setPressedHandle(NoHandle);
    update();
}

void ExRangeSlider::changeEvent(QEvent *event)
{
    Q_D(ExRangeSlider);
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->m_first->invalidateVisualPosition();
        d->m_second->invalidateVisualPosition();
    }
    if (event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange
        || event->type() == QEvent::StyleChange || event->type() == QEvent::ThemeChange)
    {
        d->m_first->syncInnerAnimation();
        d->m_second->syncInnerAnimation();
        update();
    }
}

bool ExRangeSlider::event(QEvent *event)
{
    Q_D(ExRangeSlider);
    switch (event->type())
    {
    case QEvent::HoverEnter:
    case QEvent::HoverMove:
    {
        auto *he = static_cast<QHoverEvent *>(event);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QPointF pos = he->position();
#else
        QPointF pos = he->pos();
#endif

        d->setHoveredHandle(d->handleAt(pos));
        break;
    }
    case QEvent::HoverLeave:
        d->setHoveredHandle(NoHandle);
        break;
    default:
        break;
    }
    return QWidget::event(event);
}
