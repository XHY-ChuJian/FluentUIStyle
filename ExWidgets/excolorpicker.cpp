#include "excolorpicker.h"
#include "colorgradientslider.h"

#include "fluentui3styleproperties.h"

#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>
#include <QtMath>
#include <QPainterPath>
#include <functional>

#include "exstackedwidget.h"

namespace
{
    constexpr int kFlyoutWidth = 360;
    constexpr int kSpectrumHeight = 240;
    constexpr int kChannelThickness = 24;
    constexpr int kAlphaThickness = 22;
    constexpr int kShadeStripHeight = 28;
    constexpr int kPaletteCell = 38;
    constexpr int kPaletteColumns = 8;
    constexpr int kPaletteRows = 6;
    constexpr int kTabBarHeight = 36;
    constexpr int kCheckerSize = 4;
    constexpr QColor kCheckerColor(0x19, 0x80, 0x80, 0x80);
    constexpr int kCornerRadius = 8;
    constexpr int kShadowMargin = 8;

    bool isDarkMode()
    {
        return QApplication::palette().window().color().lightness() < 128;
    }

    QColor dialogBorderColor(bool dark)
    {
        return dark ? QColor(255, 255, 255, 20) : QColor(0, 0, 0, 15);
    }

    enum ColorPickerPage
    {
        SpectrumPage = 0,
        PalettePage = 1,
        SlidersPage = 2
    };

    // Segoe Fluent Icons 字符（Window 11 内置字体）
    // Spectrum: InkingTool (取色/滴管), Palette: Color (颜色板), Sliders: Equalizer (滑条)
    static const QChar kSegoeIconSpectrum = QChar(0xE76D);
    static const QChar kSegoeIconPalette = QChar(0xE790);
    static const QChar kSegoeIconSliders = QChar(0xE9E9);

    QFont segoeIconFont(int pixelSize = 16)
    {
        QFont font(QStringLiteral("Segoe Fluent Icons"), pixelSize);
        // font.setStyleStrategy(QFont::PreferMatch);
        return font;
    }

    // ============================================================================
    // 工具函数
    // ============================================================================

    void paintCheckerboard(QPainter *painter, const QRect &rect, const QColor &checkerColor = kCheckerColor)
    {
        painter->save();
        for (int y = rect.top(); y < rect.bottom(); y += kCheckerSize)
        {
            for (int x = rect.left(); x < rect.right(); x += kCheckerSize)
            {
                const bool blank = (((x / kCheckerSize) + (y / kCheckerSize)) % 2) == 0;
                painter->fillRect(x, y, kCheckerSize, kCheckerSize, blank ? Qt::transparent : checkerColor);
            }
        }
        painter->restore();
    }

    // Qt5: getHsvF(qreal*); Qt6: getHsvF(float*)
    void colorGetHsvF(const QColor &color, qreal *h, qreal *s, qreal *v, qreal *a = nullptr)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        float fh = 0, fs = 0, fv = 0, fa = 0;
        color.getHsvF(&fh, &fs, &fv, a ? &fa : nullptr);
        *h = fh;
        *s = fs;
        *v = fv;
        if (a)
            *a = fa;
#else
        color.getHsvF(h, s, v, a);
#endif
    }

    // ============================================================================
    // Fluent 标准色板（对齐 WinUI3 FluentColorPalette）
    // ============================================================================
    QVector<QColor> fluentPaletteColors()
    {
        static const QRgb table[kPaletteRows][kPaletteColumns] = {
            {0xFFB900, 0xD13438, 0xE3008C, 0x8E8CD8, 0x0099BC, 0x00CC6A, 0x567C73, 0x69797E},
            {0xFF8C00, 0xFF4343, 0xBF0077, 0x6B69D6, 0x2D7D9A, 0x10893E, 0x486860, 0x4A5459},
            {0xF7630C, 0xE74856, 0xC239B3, 0x8764B8, 0x00B7C3, 0x7A7574, 0x498205, 0x647C64},
            {0xCA5010, 0xE81123, 0x9A0089, 0x744DA9, 0x038387, 0x5D5A58, 0x107C10, 0x525E54},
            {0xDA3B01, 0xEA005E, 0x0078D4, 0xB146C2, 0x00B294, 0x68768A, 0x767676, 0x847545},
            {0xEF6950, 0xC30052, 0x0063B1, 0x881798, 0x018574, 0x515C6B, 0x4C4A48, 0x7E735F},
        };

        QVector<QColor> colors;
        colors.reserve(kPaletteRows * kPaletteColumns);
        for (int row = 0; row < kPaletteRows; ++row)
        {
            for (int col = 0; col < kPaletteColumns; ++col)
                colors.append(QColor(table[row][col]));
        }
        return colors;
    }

    struct RingSpectrumGeometry
    {
        QPointF center;
        qreal radius = 0.0;
    };

    RingSpectrumGeometry ringSpectrumGeometry(const QRectF &bounds)
    {
        return {
            bounds.center(),
            qMin(bounds.width(), bounds.height()) / 2.0,
        };
    }

    // ============================================================================
    // 渐变图像生成（对齐 WinUI3 ColorPickerRenderingHelpers）
    // ============================================================================
    QImage buildHueSaturationImage(int width, int height, ExColorPicker::ColorSpectrumShape shape)
    {
        QImage image(qMax(1, width), qMax(1, height), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        const qreal maxX = qMax(1.0, qreal(width - 1));
        const qreal maxY = qMax(1.0, qreal(height - 1));

        if (shape == ExColorPicker::Box)
        {
            for (int y = 0; y < height; ++y)
            {
                QRgb *scanLine = reinterpret_cast<QRgb *>(image.scanLine(y));
                const qreal saturation = 1.0 - (y / maxY);
                for (int x = 0; x < width; ++x)
                {
                    const qreal hue = x / maxX;
                    scanLine[x] = QColor::fromHsvF(hue, saturation, 1.0).rgb();
                }
            }
        }
        else
        {
            const RingSpectrumGeometry geo = ringSpectrumGeometry(QRectF(0, 0, width, height));
            for (int y = 0; y < height; ++y)
            {
                QRgb *scanLine = reinterpret_cast<QRgb *>(image.scanLine(y));
                for (int x = 0; x < width; ++x)
                {
                    const qreal dx = (x + 0.5) - geo.center.x();
                    const qreal dy = (y + 0.5) - geo.center.y();
                    const qreal distance = std::hypot(dx, dy);
                    if (distance > geo.radius)
                    {
                        scanLine[x] = Qt::transparent;
                        continue;
                    }

                    qreal angle = std::atan2(dy, dx);
                    if (angle < 0)
                        angle += 2 * M_PI;
                    const qreal hue = angle / (2 * M_PI);
                    const qreal saturation = qMin(1.0, distance / geo.radius);
                    scanLine[x] = QColor::fromHsvF(hue, saturation, 1.0).rgb();
                }
            }
        }
        return image;
    }

    QImage buildThirdDimensionImage(qreal hue, qreal saturation, int width, int height)
    {
        QImage image(qMax(1, width), qMax(1, height), QImage::Format_RGB32);
        const qreal maxY = qMax(1.0, qreal(height - 1));
        for (int y = 0; y < height; ++y)
        {
            const QRgb color = QColor::fromHsvF(hue, saturation, 1.0 - (y / maxY)).rgb();
            for (int x = 0; x < width; ++x)
                image.setPixel(x, y, color);
        }
        return image;
    }

    QImage buildChannelImage(int width, int height, int channel, const QColor &base)
    {
        QImage image(qMax(1, width), qMax(1, height), QImage::Format_RGB32);
        const qreal maxX = qMax(1.0, qreal(width - 1));
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const int t = qRound((x / maxX) * 255.0);
                QColor c = base;
                switch (channel)
                {
                case 0:
                    c.setRed(t);
                    break;
                case 1:
                    c.setGreen(t);
                    break;
                case 2:
                    c.setBlue(t);
                    break;
                case 3:
                    c.setAlpha(t);
                    break;
                }
                image.setPixel(x, y, c.rgb());
            }
        }
        return image;
    }

    QImage buildHsvChannelImage(int width, int height, int channel, qreal h, qreal s, qreal v)
    {
        QImage image(qMax(1, width), qMax(1, height), QImage::Format_RGB32);
        const qreal maxX = qMax(1.0, qreal(width - 1));
        for (int y = 0; y < height; ++y)
        {
            QRgb *scanLine = reinterpret_cast<QRgb *>(image.scanLine(y));
            for (int x = 0; x < width; ++x)
            {
                qreal t = x / maxX;
                qreal ch = h, cs = s, cv = v;
                if (channel == 0)
                    ch = t;
                else if (channel == 1)
                    cs = t;
                else if (channel == 2)
                    cv = t;
                scanLine[x] = QColor::fromHsvF(ch, cs, cv).rgb();
            }
        }
        return image;
    }

    QImage buildAlphaChannelImage(int width, int height, const QColor &base, bool isHorizontal = true)
    {
        QImage image(qMax(1, width), qMax(1, height), QImage::Format_ARGB32);
        const qreal maxX = qMax(1.0, qreal(width - 1));
        const qreal maxY = qMax(1.0, qreal(height - 1));
        image.fill(Qt::transparent);
        // 绘制棋盘格
        {
            QPainter p(&image);
            paintCheckerboard(&p, image.rect());
        }
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const int alpha = isHorizontal ? qRound((x / maxX) * 255.0)
                                               : qRound((1.0 - y / maxY) * 255.0);
                QColor c = base;
                c.setAlpha(alpha);
                // 使用 premultiplied alpha 混合到棋盘格上
                QColor existing = image.pixelColor(x, y);
                const qreal a = alpha / 255.0;
                QColor blended;
                blended.setRed(qRound(existing.red() * (1.0 - a) + c.red() * a));
                blended.setGreen(qRound(existing.green() * (1.0 - a) + c.green() * a));
                blended.setBlue(qRound(existing.blue() * (1.0 - a) + c.blue() * a));
                image.setPixelColor(x, y, blended);
            }
        }
        return image;
    }

    // ============================================================================
    // 颜色预览（顶部，所有 tab 共用） — 替代 WinUI3 顶部色阶预览
    // ============================================================================
    class AccentShadeStripWidget : public QWidget
    {
    public:
        std::function<void(const QColor &)> onColorSelected;

        explicit AccentShadeStripWidget(QWidget *parent = nullptr)
            : QWidget(parent)
        {
            setFixedHeight(kShadeStripHeight);
        }

        void setColor(const QColor &color)
        {
            if (m_color == color)
                return;
            m_color = color;
            update();
        }

    protected:
        void paintEvent(QPaintEvent *) override
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);

            const QRect cell(0, 0, width(), height());

            QPainterPath path;
            path.addRoundedRect(cell, 4, 4);

            painter.setClipPath(path);
            paintCheckerboard(&painter, cell);

            painter.setPen(Qt::NoPen);
            painter.setBrush(m_color);
            painter.drawRoundedRect(cell, 4, 4);
        }

        void mousePressEvent(QMouseEvent *) override
        {
            if (onColorSelected)
                onColorSelected(m_color);
        }

    private:
        QColor m_color;
    };

    // ============================================================================
    // Hue×Saturation 色域（对齐 WinUI3 ColorSpectrum Box 模式）
    // ============================================================================
    class HueSaturationMapWidget : public QWidget
    {
    public:
        std::function<void(qreal, qreal)> onHueSaturationChanged;

        explicit HueSaturationMapWidget(QWidget *parent = nullptr)
            : QWidget(parent)
        {
            setMinimumHeight(kSpectrumHeight);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }

        void setHueSaturation(qreal hue, qreal saturation)
        {
            hue = qBound(0.0, hue, 1.0);
            saturation = qBound(0.0, saturation, 1.0);
            if (qFuzzyCompare(m_hue, hue) && qFuzzyCompare(m_saturation, saturation))
                return;
            m_hue = hue;
            m_saturation = saturation;
            update();
        }

        void setShape(ExColorPicker::ColorSpectrumShape shape)
        {
            if (m_shape == shape)
                return;
            m_shape = shape;
            m_image = QImage();
            update();
        }

        qreal hue() const { return m_hue; }
        qreal saturation() const { return m_saturation; }

    protected:
        void paintEvent(QPaintEvent *) override
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            const QRect r = rect().adjusted(0, 0, 0, 0);
            if (m_image.isNull() || m_image.size() != r.size())
                m_image = buildHueSaturationImage(r.width(), r.height(), m_shape);

            QPainterPath path;
            RingSpectrumGeometry ringGeo;
            if (m_shape == ExColorPicker::Box)
            {
                path.addRoundedRect(r, 6, 6);
            }
            else
            {
                ringGeo = ringSpectrumGeometry(QRectF(r));
                path.addEllipse(ringGeo.center, ringGeo.radius, ringGeo.radius);
            }
            painter.setClipPath(path);
            painter.drawImage(r, m_image);

            //绘制边框
            painter.setPen(QPen(palette().color(QPalette::Mid), 1));
            painter.setBrush(Qt::NoBrush);
            if (m_shape == ExColorPicker::Box)
            {
                painter.drawRoundedRect(r, 6, 6);
            }
            else
            {
                painter.drawEllipse(ringGeo.center, ringGeo.radius, ringGeo.radius);
            }

            const QPointF marker = markerPosition(r);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Qt::black, 1));
            painter.drawEllipse(marker, 8, 8);
        }

        void resizeEvent(QResizeEvent *event) override
        {
            QWidget::resizeEvent(event);
            m_image = QImage();
            update();
        }

        void mousePressEvent(QMouseEvent *event) override
        {
            if (event->button() != Qt::LeftButton)
                return;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            const QPoint pos = event->position().toPoint();
#else
            const QPoint pos = event->pos();
#endif
            m_dragging = true;
            grabMouse();
            updateFromPos(pos);
        }

        void mouseMoveEvent(QMouseEvent *event) override
        {
            if (!m_dragging || !(event->buttons() & Qt::LeftButton))
                return;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            const QPoint pos = event->position().toPoint();
#else
            const QPoint pos = event->pos();
#endif
            updateFromPos(pos);
        }

        void mouseReleaseEvent(QMouseEvent *event) override
        {
            if (event->button() != Qt::LeftButton || !m_dragging)
                return;

            m_dragging = false;
            if (mouseGrabber() == this)
                releaseMouse();
        }

    private:
        QPointF markerPosition(const QRect &area) const
        {
            if (m_shape == ExColorPicker::Box)
            {
                return QPointF(area.left() + m_hue * area.width(),
                               area.top() + (1.0 - m_saturation) * area.height());
            }
            else
            {
                const RingSpectrumGeometry geo = ringSpectrumGeometry(QRectF(area));
                const qreal angle = m_hue * 2 * M_PI;
                const qreal distance = m_saturation * geo.radius;
                return QPointF(geo.center.x() + distance * std::cos(angle),
                               geo.center.y() + distance * std::sin(angle));
            }
        }

        void updateFromPos(const QPoint &pos)
        {
            const QRect area = rect().adjusted(0, 0, -1, -1);
            qreal hue = 0.0, saturation = 0.0;

            if (m_shape == ExColorPicker::Box)
            {
                hue = qBound(0.0, (pos.x() - area.left()) / qMax<qreal>(1.0, area.width()), 1.0);
                saturation = qBound(0.0, 1.0 - (pos.y() - area.top()) / qMax<qreal>(1.0, area.height()), 1.0);
            }
            else
            {
                const RingSpectrumGeometry geo = ringSpectrumGeometry(QRectF(area));
                const qreal dx = pos.x() - geo.center.x();
                const qreal dy = pos.y() - geo.center.y();
                const qreal distance = std::hypot(dx, dy);
                qreal angle = std::atan2(dy, dx);
                if (angle < 0)
                    angle += 2 * M_PI;
                hue = angle / (2 * M_PI);
                saturation = qBound(0.0, distance / geo.radius, 1.0);
            }

            if (qFuzzyCompare(m_hue, hue) && qFuzzyCompare(m_saturation, saturation))
                return;
            m_hue = hue;
            m_saturation = saturation;
            update();
            if (onHueSaturationChanged)
                onHueSaturationChanged(m_hue, m_saturation);
        }

        qreal m_hue = 0.0;
        qreal m_saturation = 1.0;
        bool m_dragging = false;
        ExColorPicker::ColorSpectrumShape m_shape = ExColorPicker::Box;
        QImage m_image;
    };

    // ============================================================================
    // 色板网格（对齐 WinUI3 PaletteItemGridView）
    // ============================================================================
    class PaletteWidget : public QWidget
    {
    public:
        explicit PaletteWidget(QWidget *parent = nullptr)
            : QWidget(parent)
        {
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }

        std::function<void(QColor)> onColorSelected;

        void setColors(const QVector<QColor> &colors)
        {
            m_colors = colors;
            update();
        }

        void setSelectedColor(const QColor &color)
        {
            m_selectedColor = color;
            update();
        }

    protected:
        void resizeEvent(QResizeEvent *event) override
        {
            QWidget::resizeEvent(event);
            update();
        }

        void paintEvent(QPaintEvent *) override
        {
            if (m_colors.isEmpty())
                return;

            const auto rects = buildRects(rect());
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);

            for (int i = 0; i < rects.size() && i < m_colors.size(); ++i)
            {
                const QRect &cell = rects[i];
                painter.setPen(Qt::NoPen);
                painter.setBrush(m_colors[i]);

                const bool isSelected = m_colors[i] == m_selectedColor;
                const int radius = isSelected ? 2 : 3;
                painter.drawRoundedRect(cell, radius, radius);

                if (isSelected)
                {
                    painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawRoundedRect(cell.adjusted(1, 1, -1, -1), 1, 1);
                }
            }
        }

        void mousePressEvent(QMouseEvent *event) override
        {
            if (event->button() != Qt::LeftButton || m_colors.isEmpty())
                return;
            const auto rects = buildRects(rect());
            for (int i = 0; i < rects.size(); ++i)
            {
                if (rects[i].contains(event->pos()))
                {
                    m_selectedColor = m_colors[i];
                    update();
                    if (onColorSelected)
                        onColorSelected(m_selectedColor);
                    break;
                }
            }
        }

    private:
        QVector<QRect> buildRects(const QRect &area) const
        {
            QVector<QRect> rects;
            if (m_colors.isEmpty() || area.width() <= 0 || area.height() <= 0)
                return rects;

            const int cols = qMin(kPaletteColumns, m_colors.size());
            const int rows = (m_colors.size() + cols - 1) / cols;
            const int gap = 3;
            const int cellByW = qMax(1, (area.width() - (cols - 1) * gap) / cols);
            const int cellByH = qMax(1, (area.height() - (rows - 1) * gap) / rows);
            const int cellSize = qMin(cellByW, cellByH);

            rects.reserve(m_colors.size());
            for (int i = 0; i < m_colors.size(); ++i)
            {
                const int col = i % cols;
                const int row = i / cols;
                rects.append(QRect(area.left() + col * (cellSize + gap),
                                   area.top() + row * (cellSize + gap),
                                   cellSize, cellSize));
            }
            return rects;
        }

        QVector<QColor> m_colors;
        QColor m_selectedColor;
    };

} // namespace

// ============================================================================
// ExColorPickerPrivate
// ============================================================================

class ExColorPickerPrivate
{
    Q_DECLARE_PUBLIC(ExColorPicker)
public:
    explicit ExColorPickerPrivate(ExColorPicker *q);

    void applyColor(const QColor &color, bool emitSignal, bool updateInputs);
    void updateColorFromSpectrum(bool emitSignal);
    void updateColorFromRgb(bool emitSignal);
    void updateColorFromAlphaChange(int alpha);
    void syncSpectrumWidgets();
    void syncRgbWidgets();
    void syncAlphaWidget();
    void refreshShadeStrip();
    void updateTabIcons();
    void updatePanelVisibility();
    QColor outputColor() const;
    void clampAlphaIfDisabled();

    void setupUi();
    void setupShadeStrip(QVBoxLayout *root);
    void setupTabBar(QVBoxLayout *root);
    void setupSpectrumPage();
    void setupPalettePage();
    void setupSlidersPage();
    void addChannelRow(QWidget *parent, QVBoxLayout *layout, const QString &label,
                       QLabel **labelOut, QSpinBox **spinOut, ColorGradientSlider **sliderOut,
                       int sliderHeight = kChannelThickness);
    void setupConnections();
    void setupChromeConnections();
    void setupSpectrumConnections();
    void setupPaletteConnections();
    void setupSlidersConnections();
    void initializeState();

    ExColorPicker *q_ptr;
    QColor m_color = QColor(0x94, 0x4E, 0x9B);

    // 属性
    bool m_alphaEnabled = true;
    bool m_spectrumVisible = true;
    bool m_paletteVisible = true;
    bool m_previewVisible = true;
    bool m_alphaSliderVisible = true;
    bool m_colorSliderVisible = true;
    bool m_channelTextInputVisible = true;
    ExColorPicker::ColorRepresentation m_representation = ExColorPicker::Rgba;
    ExColorPicker::ColorSpectrumShape m_spectrumShape = ExColorPicker::Box;
    QVector<QColor> m_customPaletteColors;
    bool m_updating = false;
    bool m_popupMode = false;

    // HSVA 缓存
    qreal m_h = 0.0;
    qreal m_s = 1.0;
    qreal m_v = 1.0;
    qreal m_a = 1.0;

    // Widget 成员
    AccentShadeStripWidget *shadeStrip = nullptr;
    QTabBar *tabBar = nullptr;
    ExStackedWidget *stack = nullptr;

    // Spectrum 页面
    HueSaturationMapWidget *hueSatMap = nullptr;
    ColorGradientSlider *thirdDimSlider = nullptr;
    ColorGradientSlider *spectrumAlphaSlider = nullptr;

    // Palette 页面
    PaletteWidget *palette = nullptr;

    // Sliders 页面
    QComboBox *repCombo = nullptr;
    QLineEdit *hexEdit = nullptr;

    QSpinBox *redSpin = nullptr;
    QSpinBox *greenSpin = nullptr;
    QSpinBox *blueSpin = nullptr;
    QSpinBox *alphaSpin = nullptr;

    ColorGradientSlider *redSlider = nullptr;
    ColorGradientSlider *greenSlider = nullptr;
    ColorGradientSlider *blueSlider = nullptr;
    ColorGradientSlider *alphaSlider = nullptr;

    QLabel *redLabel = nullptr;
    QLabel *greenLabel = nullptr;
    QLabel *blueLabel = nullptr;
    QLabel *alphaLabel = nullptr;
};

ExColorPickerPrivate::ExColorPickerPrivate(ExColorPicker *q)
    : q_ptr(q)
{
}

QColor ExColorPickerPrivate::outputColor() const
{
    if (m_alphaEnabled)
        return m_color;
    QColor color = m_color;
    color.setAlpha(255);
    return color;
}

void ExColorPickerPrivate::clampAlphaIfDisabled()
{
    if (m_alphaEnabled || m_color.alpha() == 255)
        return;
    m_color.setAlpha(255);
    m_a = 1.0;
}

void ExColorPickerPrivate::syncSpectrumWidgets()
{
    if (!hueSatMap || !thirdDimSlider)
        return;
    hueSatMap->setHueSaturation(m_h, m_s);

    // 更新第三维滑条渐变图并设置值
    thirdDimSlider->setImageBuilder([this](QSize sz)
                                    { return buildThirdDimensionImage(m_h, m_s, sz.width(), sz.height()); });
    thirdDimSlider->blockSignals(true);
    // 垂直滑条默认最大值在上，与渐变图顶端 Value=1 对齐
    thirdDimSlider->setValue(qRound(m_v * 255));
    thirdDimSlider->blockSignals(false);

    if (spectrumAlphaSlider)
    {
        spectrumAlphaSlider->setImageBuilder([this](QSize sz)
                                             { return buildAlphaChannelImage(sz.width(), sz.height(),
                                                                             QColor::fromHsvF(m_h, m_s, m_v, 1.0f), false); });
        spectrumAlphaSlider->blockSignals(true);
        spectrumAlphaSlider->setValue(qRound(m_a * 255));
        spectrumAlphaSlider->blockSignals(false);
    }
}

void ExColorPickerPrivate::syncRgbWidgets()
{
    if (!redSpin)
        return;

    const bool isHsv = (m_representation == ExColorPicker::Hsva);

    redSpin->blockSignals(true);
    greenSpin->blockSignals(true);
    blueSpin->blockSignals(true);
    redSlider->blockSignals(true);
    greenSlider->blockSignals(true);
    blueSlider->blockSignals(true);

    if (isHsv)
    {
        redLabel->setText(QStringLiteral("H"));
        greenLabel->setText(QStringLiteral("S"));
        blueLabel->setText(QStringLiteral("V"));

        redSpin->setRange(0, 359);
        greenSpin->setRange(0, 100);
        blueSpin->setRange(0, 100);
        redSlider->setRange(0, 359);
        greenSlider->setRange(0, 100);
        blueSlider->setRange(0, 100);

        redSpin->setValue(qRound(m_h * 359.0f));
        greenSpin->setValue(qRound(m_s * 100.0f));
        blueSpin->setValue(qRound(m_v * 100.0f));
        redSlider->setValue(qRound(m_h * 359.0f));
        greenSlider->setValue(qRound(m_s * 100.0f));
        blueSlider->setValue(qRound(m_v * 100.0f));

        redSlider->setImageBuilder([this](QSize sz)
                                   { return buildHsvChannelImage(sz.width(), sz.height(), 0, m_h, m_s, m_v); });
        greenSlider->setImageBuilder([this](QSize sz)
                                     { return buildHsvChannelImage(sz.width(), sz.height(), 1, m_h, m_s, m_v); });
        blueSlider->setImageBuilder([this](QSize sz)
                                    { return buildHsvChannelImage(sz.width(), sz.height(), 2, m_h, m_s, m_v); });
    }
    else
    {
        redLabel->setText(QStringLiteral("R"));
        greenLabel->setText(QStringLiteral("G"));
        blueLabel->setText(QStringLiteral("B"));

        redSpin->setRange(0, 255);
        greenSpin->setRange(0, 255);
        blueSpin->setRange(0, 255);
        redSlider->setRange(0, 255);
        greenSlider->setRange(0, 255);
        blueSlider->setRange(0, 255);

        redSpin->setValue(m_color.red());
        greenSpin->setValue(m_color.green());
        blueSpin->setValue(m_color.blue());
        redSlider->setValue(m_color.red());
        greenSlider->setValue(m_color.green());
        blueSlider->setValue(m_color.blue());

        redSlider->setImageBuilder([this](QSize sz)
                                   { return buildChannelImage(sz.width(), sz.height(), 0,
                                                              QColor(0, m_color.green(), m_color.blue())); });
        greenSlider->setImageBuilder([this](QSize sz)
                                     { return buildChannelImage(sz.width(), sz.height(), 1,
                                                                QColor(m_color.red(), 0, m_color.blue())); });
        blueSlider->setImageBuilder([this](QSize sz)
                                    { return buildChannelImage(sz.width(), sz.height(), 2,
                                                               QColor(m_color.red(), m_color.green(), 0)); });
    }

    redSpin->blockSignals(false);
    greenSpin->blockSignals(false);
    blueSpin->blockSignals(false);
    redSlider->blockSignals(false);
    greenSlider->blockSignals(false);
    blueSlider->blockSignals(false);

    syncAlphaWidget();
}

void ExColorPickerPrivate::syncAlphaWidget()
{
    if (alphaSpin)
    {
        alphaSpin->blockSignals(true);
        alphaSpin->setValue(m_color.alpha());
        alphaSpin->blockSignals(false);
    }
    if (alphaSlider)
    {
        alphaSlider->setImageBuilder([this](QSize sz)
                                     { return buildAlphaChannelImage(sz.width(), sz.height(),
                                                                     QColor(m_color.red(), m_color.green(), m_color.blue(), 255), true); });
        alphaSlider->blockSignals(true);
        alphaSlider->setValue(m_color.alpha());
        alphaSlider->blockSignals(false);
    }
}

void ExColorPickerPrivate::refreshShadeStrip()
{
    if (shadeStrip)
        shadeStrip->setColor(m_color);
}

void ExColorPickerPrivate::updateTabIcons()
{
    if (!tabBar)
        return;

    QFont iconFont = segoeIconFont(12);
    tabBar->setFont(iconFont);

    tabBar->setTabText(0, kSegoeIconSpectrum);
    tabBar->setTabText(1, kSegoeIconPalette);
    tabBar->setTabText(2, kSegoeIconSliders);
}

void ExColorPickerPrivate::updatePanelVisibility()
{
//Qt 5.15以下没有setTabVisible接口
#if QT_VERSION >= QT_VERSION_CHECK(5,15,2)
    if (!tabBar || !stack)
        return;

    // Spectrum tab
    tabBar->setTabVisible(0, m_spectrumVisible);

    // Palette tab
    tabBar->setTabVisible(1, m_paletteVisible);

    // Sliders tab
    tabBar->setTabVisible(2, m_channelTextInputVisible);

    // 如果当前选中的 tab 被隐藏了，切换到第一个可见的
    const int cur = tabBar->currentIndex();
    if (!tabBar->isTabVisible(cur))
    {
        for (int i = 0; i < tabBar->count(); ++i)
        {
            if (tabBar->isTabVisible(i))
            {
                tabBar->setCurrentIndex(i);
                break;
            }
        }
    }
#endif
}

void ExColorPickerPrivate::applyColor(const QColor &color, bool emitSignal, bool updateInputs)
{
    Q_Q(ExColorPicker);
    QColor normalized = color;
    if (!m_alphaEnabled)
        normalized.setAlpha(255);
    if (!normalized.isValid() || (m_color == normalized && m_updating))
        return;

    m_updating = true;
    m_color = normalized;
    colorGetHsvF(m_color, &m_h, &m_s, &m_v, &m_a);
    clampAlphaIfDisabled();

    syncSpectrumWidgets();
    if (updateInputs)
    {
        syncRgbWidgets();
        if (hexEdit)
            hexEdit->setText(m_alphaEnabled
                                 ? m_color.name(QColor::HexArgb).mid(1).toUpper()
                                 : m_color.name(QColor::HexRgb).toUpper());
        if (palette)
            palette->setSelectedColor(m_color);
    }
    refreshShadeStrip();

    m_updating = false;
    if (emitSignal)
        Q_EMIT q->colorChanged(q->color());
}

void ExColorPickerPrivate::updateColorFromSpectrum(bool emitSignal)
{
    Q_Q(ExColorPicker);
    if (m_updating || !hueSatMap || !thirdDimSlider)
        return;

    m_h = hueSatMap->hue();
    m_s = hueSatMap->saturation();
    // 垂直滑条顶端 Value=1，底端 Value=0
    m_v = thirdDimSlider->value() / 255.0;

    const qreal alpha = m_alphaEnabled ? m_a : 1.0;
    const QColor color = QColor::fromHsvF(m_h, m_s, m_v, alpha);
    if (m_color == color)
        return;

    m_updating = true;
    m_color = color;
    clampAlphaIfDisabled();

    // Update spectrum sliders images since h/s/v changed
    thirdDimSlider->setImageBuilder([this](QSize sz)
                                    { return buildThirdDimensionImage(m_h, m_s, sz.width(), sz.height()); });
    if (spectrumAlphaSlider)
    {
        spectrumAlphaSlider->setImageBuilder([this](QSize sz)
                                             { return buildAlphaChannelImage(sz.width(), sz.height(),
                                                                             QColor::fromHsvF(m_h, m_s, m_v, 1.0f), false); });
    }

    syncRgbWidgets();
    if (hexEdit)
        hexEdit->setText(m_alphaEnabled
                             ? m_color.name(QColor::HexArgb).mid(1).toUpper()
                             : m_color.name(QColor::HexRgb).toUpper());
    if (palette)
        palette->setSelectedColor(m_color);
    refreshShadeStrip();
    m_updating = false;

    if (emitSignal)
        Q_EMIT q->colorChanged(q->color());
}

void ExColorPickerPrivate::updateColorFromRgb(bool emitSignal)
{
    Q_Q(ExColorPicker);
    if (m_updating || !redSpin || !greenSpin || !blueSpin)
        return;

    const bool isHsv = (m_representation == ExColorPicker::Hsva);
    QColor color;
    if (isHsv)
    {
        color = QColor::fromHsvF(redSpin->value() / 359.0f,
                                 greenSpin->value() / 100.0f,
                                 blueSpin->value() / 100.0f,
                                 m_alphaEnabled && alphaSpin ? alphaSpin->value() / 255.0f : 1.0f);
    }
    else
    {
        color = QColor(redSpin->value(), greenSpin->value(), blueSpin->value(),
                       m_alphaEnabled && alphaSpin ? alphaSpin->value() : 255);
    }

    if (m_color == color)
        return;

    m_updating = true;
    m_color = color;
    clampAlphaIfDisabled();
    colorGetHsvF(m_color, &m_h, &m_s, &m_v, &m_a);
    syncSpectrumWidgets();
    syncRgbWidgets();
    if (hexEdit)
        hexEdit->setText(m_alphaEnabled
                             ? m_color.name(QColor::HexArgb).mid(1).toUpper()
                             : m_color.name(QColor::HexRgb).toUpper());
    if (palette)
        palette->setSelectedColor(m_color);
    refreshShadeStrip();
    m_updating = false;

    if (emitSignal)
        Q_EMIT q->colorChanged(q->color());
}

void ExColorPickerPrivate::updateColorFromAlphaChange(int alpha)
{
    Q_Q(ExColorPicker);
    if (m_updating || !m_alphaEnabled)
        return;

    m_updating = true;
    m_color.setAlpha(alpha);
    m_a = alpha / 255.0;
    syncRgbWidgets();
    if (hexEdit)
        hexEdit->setText(m_alphaEnabled
                             ? m_color.name(QColor::HexArgb).mid(1).toUpper()
                             : m_color.name(QColor::HexRgb).toUpper());
    refreshShadeStrip();
    m_updating = false;

    Q_EMIT q->colorChanged(q->color());
}

void ExColorPickerPrivate::setupUi()
{
    Q_Q(ExColorPicker);
    auto *root = new QVBoxLayout(q);
    if (m_popupMode)
        root->setContentsMargins(kShadowMargin + 12, kShadowMargin + 8, kShadowMargin + 12, kShadowMargin + 12);
    else
        root->setContentsMargins(12, 8, 12, 12);
    root->setSpacing(8);

    setupShadeStrip(root);
    setupTabBar(root);

    stack = new ExStackedWidget(q);
    root->addWidget(stack);

    setupSpectrumPage();
    setupPalettePage();
    setupSlidersPage();
    setupConnections();
    initializeState();
}

void ExColorPickerPrivate::setupShadeStrip(QVBoxLayout *root)
{
    Q_Q(ExColorPicker);
    shadeStrip = new AccentShadeStripWidget(q);
    root->addWidget(shadeStrip);
}

void ExColorPickerPrivate::setupTabBar(QVBoxLayout *root)
{
    Q_Q(ExColorPicker);
    tabBar = new QTabBar(q);
    tabBar->setObjectName(QStringLiteral("exColorPickerTabBar"));
    tabBar->setAttribute(Qt::WA_StyledBackground, true);
    tabBar->setDrawBase(false);
    tabBar->setExpanding(true);
    tabBar->setProperty(TabBarStyleProperty, Segmented_WinUI3);
    tabBar->addTab(QString());
    tabBar->addTab(QString());
    tabBar->addTab(QString());
    tabBar->setTabToolTip(0, ExColorPicker::tr("Spectrum"));
    tabBar->setTabToolTip(1, ExColorPicker::tr("Palette"));
    tabBar->setTabToolTip(2, ExColorPicker::tr("Sliders"));
    updateTabIcons();
    root->addWidget(tabBar);
}

void ExColorPickerPrivate::setupSpectrumPage()
{
    auto *spectrumPage = new QWidget(stack);
    auto *spectrumMainLay = new QVBoxLayout(spectrumPage);
    spectrumMainLay->setContentsMargins(0, 5, 0, 0);
    spectrumMainLay->setSpacing(6);

    auto *spectrumRow = new QHBoxLayout;
    spectrumRow->setSpacing(8);

    thirdDimSlider = new ColorGradientSlider(Qt::Vertical, spectrumPage);
    thirdDimSlider->setFixedWidth(kChannelThickness);
    hueSatMap = new HueSaturationMapWidget(spectrumPage);
    spectrumRow->addWidget(thirdDimSlider);
    spectrumRow->addWidget(hueSatMap, 1);

    spectrumAlphaSlider = new ColorGradientSlider(Qt::Vertical, spectrumPage);
    spectrumAlphaSlider->setFixedWidth(kChannelThickness);
    spectrumRow->addWidget(spectrumAlphaSlider);

    spectrumMainLay->addLayout(spectrumRow);
    stack->addWidget(spectrumPage);
}

void ExColorPickerPrivate::setupPalettePage()
{
    auto *palettePage = new QWidget(stack);
    auto *paletteLay = new QVBoxLayout(palettePage);
    paletteLay->setContentsMargins(0, 0, 0, 0);

    palette = new PaletteWidget(palettePage);
    palette->setColors(m_customPaletteColors.isEmpty() ? fluentPaletteColors() : m_customPaletteColors);
    paletteLay->addWidget(palette, 1);
    stack->addWidget(palettePage);
}

void ExColorPickerPrivate::addChannelRow(QWidget *parent, QVBoxLayout *layout, const QString &label,
                                        QLabel **labelOut, QSpinBox **spinOut, ColorGradientSlider **sliderOut,
                                        int sliderHeight)
{
    auto *row = new QHBoxLayout;
    row->setSpacing(6);

    auto *labelWidget = new QLabel(label, parent);
    labelWidget->setFixedWidth(16);
    labelWidget->setAlignment(Qt::AlignCenter);
    row->addWidget(labelWidget);

    auto *spinBox = new QSpinBox(parent);
    spinBox->setRange(0, 255);
    spinBox->setFixedWidth(56);
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    row->addWidget(spinBox);

    auto *sliderWidget = new ColorGradientSlider(Qt::Horizontal, parent);
    sliderWidget->setFixedHeight(sliderHeight);
    row->addWidget(sliderWidget, 1);

    layout->addLayout(row);
    *labelOut = labelWidget;
    *spinOut = spinBox;
    *sliderOut = sliderWidget;
}

void ExColorPickerPrivate::setupSlidersPage()
{
    auto *slidersPage = new QWidget(stack);
    auto *slidersLay = new QVBoxLayout(slidersPage);
    slidersLay->setContentsMargins(0, 0, 0, 0);
    slidersLay->setSpacing(6);

    auto *headerRow = new QHBoxLayout;
    repCombo = new QComboBox(slidersPage);
    repCombo->addItem(QStringLiteral("RGB"), ExColorPicker::Rgba);
    repCombo->addItem(QStringLiteral("HSV"), ExColorPicker::Hsva);
    repCombo->setCurrentIndex(0);
    repCombo->setFixedWidth(80);
    headerRow->addWidget(repCombo);

    hexEdit = new QLineEdit(slidersPage);
    hexEdit->setPlaceholderText(QStringLiteral("944E9B"));
    hexEdit->setMaxLength(8);
    hexEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[0-9A-Fa-f]{6,8}")), hexEdit));
    headerRow->addWidget(hexEdit, 1);
    slidersLay->addLayout(headerRow);

    addChannelRow(slidersPage, slidersLay, QStringLiteral("R"), &redLabel, &redSpin, &redSlider);
    addChannelRow(slidersPage, slidersLay, QStringLiteral("G"), &greenLabel, &greenSpin, &greenSlider);
    addChannelRow(slidersPage, slidersLay, QStringLiteral("B"), &blueLabel, &blueSpin, &blueSlider);
    addChannelRow(slidersPage, slidersLay, QStringLiteral("A"), &alphaLabel, &alphaSpin, &alphaSlider, kAlphaThickness);

    slidersLay->addStretch();
    stack->addWidget(slidersPage);
}

void ExColorPickerPrivate::setupConnections()
{
    setupChromeConnections();
    setupSpectrumConnections();
    setupPaletteConnections();
    setupSlidersConnections();
}

void ExColorPickerPrivate::setupChromeConnections()
{
    Q_Q(ExColorPicker);

    shadeStrip->onColorSelected = [this](const QColor &color)
    {
        applyColor(color, true, true);
    };

    QObject::connect(tabBar, &QTabBar::currentChanged, q, [this](int index)
                     {
                         if (index < 0 || !stack)
                             return;
                         stack->setCurrentIndex(index);
                     });
}

void ExColorPickerPrivate::setupSpectrumConnections()
{
    Q_Q(ExColorPicker);

    hueSatMap->onHueSaturationChanged = [this](qreal, qreal)
    {
        if (m_updating)
            return;
        updateColorFromSpectrum(true);
    };

    QObject::connect(thirdDimSlider, &QSlider::valueChanged, q, [this](int)
                     { updateColorFromSpectrum(true); });

    QObject::connect(spectrumAlphaSlider, &QSlider::valueChanged, q, [this](int alpha)
                     { updateColorFromAlphaChange(alpha); });
}

void ExColorPickerPrivate::setupPaletteConnections()
{
    palette->onColorSelected = [this](const QColor &color)
    {
        applyColor(color, true, true);
    };
}

void ExColorPickerPrivate::setupSlidersConnections()
{
    Q_Q(ExColorPicker);

    auto channelChanged = [this](int)
    {
        updateColorFromRgb(true);
    };
    QObject::connect(redSpin, qOverload<int>(&QSpinBox::valueChanged), q, channelChanged);
    QObject::connect(greenSpin, qOverload<int>(&QSpinBox::valueChanged), q, channelChanged);
    QObject::connect(blueSpin, qOverload<int>(&QSpinBox::valueChanged), q, channelChanged);

    QObject::connect(redSlider, &QSlider::valueChanged, q, [this](int value)
                     {
                         if (m_updating)
                             return;
                         redSpin->setValue(value);
                     });
    QObject::connect(greenSlider, &QSlider::valueChanged, q, [this](int value)
                     {
                         if (m_updating)
                             return;
                         greenSpin->setValue(value);
                     });
    QObject::connect(blueSlider, &QSlider::valueChanged, q, [this](int value)
                     {
                         if (m_updating)
                             return;
                         blueSpin->setValue(value);
                     });

    QObject::connect(alphaSpin, qOverload<int>(&QSpinBox::valueChanged), q, [this](int alpha)
                     { updateColorFromAlphaChange(alpha); });
    QObject::connect(alphaSlider, &QSlider::valueChanged, q, [this](int alpha)
                     {
                         if (m_updating)
                             return;
                         alphaSpin->setValue(alpha);
                     });

    QObject::connect(hexEdit, &QLineEdit::editingFinished, q, [this]()
                     {
                         const QString text = hexEdit->text().trimmed();
                         const QColor parsed(QStringLiteral("#") + text);
                         if (parsed.isValid())
                             applyColor(parsed, true, true);
                     });

    QObject::connect(repCombo, qOverload<int>(&QComboBox::currentIndexChanged), q, [this](int idx)
                     {
                         m_representation = static_cast<ExColorPicker::ColorRepresentation>(repCombo->itemData(idx).toInt());
                         syncRgbWidgets();
                     });
}

void ExColorPickerPrivate::initializeState()
{
    applyColor(m_color, false, true);
    tabBar->setCurrentIndex(0);
    stack->setCurrentIndex(SpectrumPage);
}

// ============================================================================
// ExColorPicker 构造与析构
// ============================================================================

ExColorPicker::ExColorPicker(QWidget *parent, bool popup)
    : QDialog(parent)
    , d_ptr(new ExColorPickerPrivate(this))
{
    Q_D(ExColorPicker);
    d->m_popupMode = popup;
    if (popup)
    {
        setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setObjectName(QStringLiteral("exColorPickerFlyout"));
    }
    else
    {
        // 内嵌模式：作为普通子控件，避免 QDialog 默认窗口行为导致不显示
        setWindowFlags(Qt::Widget);
        setModal(false);
        setAttribute(Qt::WA_TranslucentBackground);
        setAutoFillBackground(false);
    }
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    d->setupUi();
    setFixedSize(sizeHint());
    if (!popup)
        QWidget::setVisible(true);
}

bool ExColorPicker::isPopupMode() const
{
    Q_D(const ExColorPicker);
    return d->m_popupMode;
}

void ExColorPicker::showPopup(QWidget *anchor)
{
    if (!d_ptr->m_popupMode || !anchor)
        return;

    if (isVisible())
    {
        hide();
        return;
    }

    adjustSize();
    const QPoint anchorTopLeft = anchor->mapToGlobal(QPoint(0, 0));
    move(anchorTopLeft.x() - 6, anchorTopLeft.y() - height());
    show();
    raise();
    activateWindow();
}

ExColorPicker::~ExColorPicker()
{
    delete d_ptr;
}

// ============================================================================
// 公共 API
// ============================================================================

QColor ExColorPicker::color() const
{
    Q_D(const ExColorPicker);
    return d->outputColor();
}

void ExColorPicker::setColor(const QColor &color)
{
    Q_D(ExColorPicker);
    if (!color.isValid())
        return;
    d->applyColor(color, true, true);
}

bool ExColorPicker::isAlphaEnabled() const
{
    Q_D(const ExColorPicker);
    return d->m_alphaEnabled;
}

void ExColorPicker::setAlphaEnabled(bool enabled)
{
    Q_D(ExColorPicker);
    if (d->m_alphaEnabled == enabled)
        return;
    d->m_alphaEnabled = enabled;
    if (d->alphaSpin)
        d->alphaSpin->setVisible(enabled && d->m_channelTextInputVisible);
    if (d->alphaSlider)
        d->alphaSlider->setVisible(enabled && d->m_alphaSliderVisible);
    if (d->alphaLabel)
        d->alphaLabel->setVisible(enabled && d->m_channelTextInputVisible);
    if (d->spectrumAlphaSlider)
        d->spectrumAlphaSlider->setVisible(enabled && d->m_alphaSliderVisible);
    if (!enabled)
        d->clampAlphaIfDisabled();
    d->syncSpectrumWidgets();
    d->syncRgbWidgets();
    if (d->hexEdit)
        d->hexEdit->setText(enabled
                                ? d->m_color.name(QColor::HexArgb).mid(1).toUpper()
                                : d->m_color.name(QColor::HexRgb).toUpper());
    Q_EMIT alphaEnabledChanged(enabled);
    Q_EMIT colorChanged(color());
}

ExColorPicker::ColorRepresentation ExColorPicker::colorRepresentation() const
{
    Q_D(const ExColorPicker);
    return d->m_representation;
}

void ExColorPicker::setColorRepresentation(ColorRepresentation representation)
{
    Q_D(ExColorPicker);
    d->m_representation = representation;
    if (d->repCombo)
        d->repCombo->setCurrentIndex(representation == Rgba ? 0 : 1);
}

bool ExColorPicker::isColorSpectrumVisible() const
{
    Q_D(const ExColorPicker);
    return d->m_spectrumVisible;
}

void ExColorPicker::setColorSpectrumVisible(bool visible)
{
    Q_D(ExColorPicker);
    if (d->m_spectrumVisible == visible)
        return;
    d->m_spectrumVisible = visible;
    d->updatePanelVisibility();
    if (visible && d->tabBar)
        d->tabBar->setCurrentIndex(0);
    Q_EMIT colorSpectrumVisibleChanged(visible);
}

bool ExColorPicker::isColorPaletteVisible() const
{
    Q_D(const ExColorPicker);
    return d->m_paletteVisible;
}

void ExColorPicker::setColorPaletteVisible(bool visible)
{
    Q_D(ExColorPicker);
    if (d->m_paletteVisible == visible)
        return;
    d->m_paletteVisible = visible;
    d->updatePanelVisibility();
    if (visible && d->tabBar)
        d->tabBar->setCurrentIndex(1);
    Q_EMIT colorPaletteVisibleChanged(visible);
}

bool ExColorPicker::isColorPreviewVisible() const
{
    Q_D(const ExColorPicker);
    return d->m_previewVisible;
}

void ExColorPicker::setColorPreviewVisible(bool visible)
{
    Q_D(ExColorPicker);
    if (d->m_previewVisible == visible)
        return;
    d->m_previewVisible = visible;
    if (d->shadeStrip)
        d->shadeStrip->setVisible(visible);
    Q_EMIT colorPreviewVisibleChanged(visible);
}

bool ExColorPicker::isAlphaSliderVisible() const
{
    Q_D(const ExColorPicker);
    return d->m_alphaSliderVisible;
}

void ExColorPicker::setAlphaSliderVisible(bool visible)
{
    Q_D(ExColorPicker);
    if (d->m_alphaSliderVisible == visible)
        return;
    d->m_alphaSliderVisible = visible;
    if (d->spectrumAlphaSlider)
        d->spectrumAlphaSlider->setVisible(visible && d->m_alphaEnabled);
    if (d->alphaSlider)
    {
        d->alphaSlider->setVisible(visible && d->m_alphaEnabled);
        d->alphaLabel->setVisible(visible && d->m_alphaEnabled);
        d->alphaSpin->setVisible(visible && d->m_alphaEnabled);
    }
    Q_EMIT alphaSliderVisibleChanged(visible);
}

bool ExColorPicker::isColorSliderVisible() const
{
    Q_D(const ExColorPicker);
    return d->m_colorSliderVisible;
}

void ExColorPicker::setColorSliderVisible(bool visible)
{
    Q_D(ExColorPicker);
    if (d->m_colorSliderVisible == visible)
        return;
    d->m_colorSliderVisible = visible;
    if (d->thirdDimSlider)
        d->thirdDimSlider->setVisible(visible);
    Q_EMIT colorSliderVisibleChanged(visible);
}

bool ExColorPicker::isColorChannelTextInputVisible() const
{
    Q_D(const ExColorPicker);
    return d->m_channelTextInputVisible;
}

void ExColorPicker::setColorChannelTextInputVisible(bool visible)
{
    Q_D(ExColorPicker);
    if (d->m_channelTextInputVisible == visible)
        return;
    d->m_channelTextInputVisible = visible;
    d->updatePanelVisibility();
    Q_EMIT colorChannelTextInputVisibleChanged(visible);
}

QVector<QColor> ExColorPicker::customPaletteColors() const
{
    Q_D(const ExColorPicker);
    return d->m_customPaletteColors;
}

void ExColorPicker::setCustomPaletteColors(const QVector<QColor> &colors)
{
    Q_D(ExColorPicker);
    d->m_customPaletteColors = colors;
    if (d->palette)
        d->palette->setColors(colors.isEmpty() ? fluentPaletteColors() : colors);
}

ExColorPicker::ColorSpectrumShape ExColorPicker::colorSpectrumShape() const
{
    Q_D(const ExColorPicker);
    return d->m_spectrumShape;
}

void ExColorPicker::setColorSpectrumShape(ColorSpectrumShape shape)
{
    Q_D(ExColorPicker);
    if (d->m_spectrumShape == shape)
        return;
    d->m_spectrumShape = shape;
    if (d->hueSatMap)
        d->hueSatMap->setShape(shape);
    Q_EMIT colorSpectrumShapeChanged(shape);
}

QSize ExColorPicker::sizeHint() const
{
    Q_D(const ExColorPicker);
    const int contentW = kFlyoutWidth;
    const int contentH = kShadeStripHeight + kTabBarHeight + kSpectrumHeight + kAlphaThickness + 60;
    const int shadow = d->m_popupMode ? 2 * kShadowMargin : 0;
    return {contentW + shadow, contentH + shadow};
}

QSize ExColorPicker::minimumSizeHint() const
{
    return sizeHint();
}

void ExColorPicker::setVisible(bool visible)
{
    Q_D(ExColorPicker);
    if (d->m_popupMode)
        QDialog::setVisible(visible);
    else
        QWidget::setVisible(visible);
}

void ExColorPicker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    Q_D(ExColorPicker);
    if (!d->m_popupMode)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect cardRect = rect();
    cardRect.adjust(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
    const bool dark = isDarkMode();

    painter.setPen(Qt::NoPen);
    constexpr int layers = 4;
    for (int i = layers; i > 0; --i)
    {
        const qreal expand = i * 1.5;
        const int alpha = dark ? (6 + (layers - i) * 4) : (3 + (layers - i) * 2);
        QRectF sr = cardRect.adjusted(-expand, -expand + 1, expand, expand + 1);
        QPainterPath sp;
        sp.addRoundedRect(sr, kCornerRadius + expand, kCornerRadius + expand);
        painter.setBrush(QColor(0, 0, 0, alpha));
        painter.drawPath(sp);
    }

    QPainterPath path;
    path.addRoundedRect(cardRect, kCornerRadius, kCornerRadius);
    painter.fillPath(path, palette().window());

    painter.setPen(QPen(dialogBorderColor(dark), 1.0));
    painter.setBrush(Qt::NoBrush);
    const QRectF cardRectF(cardRect);
    painter.drawRoundedRect(cardRectF.adjusted(0.5, 0.5, -0.5, -0.5), kCornerRadius, kCornerRadius);
}

void ExColorPicker::changeEvent(QEvent *event)
{
    QDialog::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
    {
        Q_D(ExColorPicker);
        d->refreshShadeStrip();
        d->updateTabIcons();
        update();
    }
}
