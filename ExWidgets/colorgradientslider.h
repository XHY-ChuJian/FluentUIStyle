#pragma once

#include "exwidgets_global.h"

#include <QImage>
#include <QSlider>
#include <functional>

/**
 * @brief QSlider 子类，轨道（groove）绘制为颜色渐变图像，手柄由 Qt 原生绘制。
 *
 * 通过 setImageBuilder() 设置一个回调函数，在每次 paint 时根据 groove 尺寸
 * 动态生成渐变 QImage，并使用圆角矩形裁剪后绘制到轨道区域。
 */
class EXWIDGETS_EXPORT ColorGradientSlider : public QSlider
{
public:
    using ImageBuilder = std::function<QImage(QSize)>;

    explicit ColorGradientSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

    void setImageBuilder(ImageBuilder builder);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    ImageBuilder m_imageBuilder;
    QImage m_cachedImage;
};
