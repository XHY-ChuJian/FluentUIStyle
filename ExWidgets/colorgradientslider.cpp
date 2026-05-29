#include "colorgradientslider.h"

#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionSlider>

ColorGradientSlider::ColorGradientSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
    setRange(0, 255);
}

void ColorGradientSlider::setImageBuilder(ImageBuilder builder)
{
    m_imageBuilder = std::move(builder);
    m_cachedImage = QImage();
    update();
}

void ColorGradientSlider::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QStyleOptionSlider opt;
    initStyleOption(&opt);

    const bool isHorizontal = (orientation() == Qt::Horizontal);

    QRectF groove = style()->subControlRect(QStyle::CC_Slider, &opt,
                                             QStyle::SC_SliderGroove, this);
    if (groove.isEmpty())
    {
        QSlider::paintEvent(nullptr);
        return;
    }

    const qreal grooveThickness = 10;
    const qreal radius = grooveThickness / 2;
    if (isHorizontal)
    {
        groove = QRectF(groove.left() + 2,
                        groove.center().y() - grooveThickness / 2.0,
                        groove.width() - 4,
                        grooveThickness);
    }
    else
    {
        groove = QRectF(groove.center().x() - grooveThickness / 2.0,
                        groove.top() + 2,
                        grooveThickness,
                        groove.height() - 4);
    }

    const QRect grooveR = groove.toRect();
    if (grooveR.isEmpty())
    {
        QSlider::paintEvent(nullptr);
        return;
    }

    // 构建/缓存渐变图
    if (m_imageBuilder)
    {
        if (m_cachedImage.isNull() || m_cachedImage.size() != grooveR.size())
            m_cachedImage = m_imageBuilder(grooveR.size());

        p.save();
        QPainterPath clipPath;
        clipPath.addRoundedRect(groove, radius, radius);
        p.setClipPath(clipPath);
        p.drawImage(grooveR.topLeft(), m_cachedImage);
        p.restore();
    }

    opt.subControls = QStyle::SC_SliderHandle;
    style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);
}

void ColorGradientSlider::resizeEvent(QResizeEvent *event)
{
    QSlider::resizeEvent(event);
    m_cachedImage = QImage();
}
