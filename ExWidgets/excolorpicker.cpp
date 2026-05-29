#include "excolorpicker.h"
#include "colorgradientslider.h"

#include "fluentui3styleproperties.h"

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
#include <functional>

namespace
{
constexpr int kFlyoutWidth = 320;
constexpr int kSpectrumHeight = 180;
constexpr int kChannelThickness = 24;
constexpr int kAlphaThickness = 22;
constexpr int kShadeStripHeight = 28;
constexpr int kPaletteCell = 32;
constexpr int kPaletteColumns = 8;
constexpr int kPaletteRows = 6;
constexpr int kTabBarHeight = 36;
constexpr int kCheckerSize = 4;
constexpr QColor kCheckerColor(0x19, 0x80, 0x80, 0x80);

enum ColorPickerPage
{
    SpectrumPage = 0,
    PalettePage = 1,
    SlidersPage = 2
};

// Segoe Fluent Icons 字符（Window 11 内置字体）
// Spectrum: InkingTool (取色/滴管), Palette: Color (颜色板), Sliders: Equalizer (滑条)
static const QChar kSegoeIconSpectrum = QChar(0xE76D);
static const QChar kSegoeIconPalette  = QChar(0xE790);
static const QChar kSegoeIconSliders  = QChar(0xE9E9);

QFont segoeIconFont(int pixelSize = 16)
{
    QFont font(QStringLiteral("Segoe Fluent Icons"), pixelSize);
    font.setStyleStrategy(QFont::PreferMatch);
    return font;
}

// ============================================================================
// 工具函数
// ============================================================================

void paintCheckerboard(QPainter *painter, const QRect &rect, const QColor &checkerColor = kCheckerColor)
{
    painter->save();
    painter->setClipRect(rect);
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

// ============================================================================
// 渐变图像生成（对齐 WinUI3 ColorPickerRenderingHelpers）
// ============================================================================

QImage buildHueSaturationImage(int width, int height)
{
    QImage image(qMax(1, width), qMax(1, height), QImage::Format_RGB32);
    const qreal maxX = qMax(1.0, qreal(width - 1));
    const qreal maxY = qMax(1.0, qreal(height - 1));
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
            case 0: c.setRed(t); break;
            case 1: c.setGreen(t); break;
            case 2: c.setBlue(t); break;
            case 3: c.setAlpha(t); break;
            }
            image.setPixel(x, y, c.rgb());
        }
    }
    return image;
}

QImage buildAlphaChannelImage(int width, int height, const QColor &base)
{
    QImage image(qMax(1, width), qMax(1, height), QImage::Format_ARGB32);
    const qreal maxX = qMax(1.0, qreal(width - 1));
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
            const int alpha = qRound((x / maxX) * 255.0);
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
// 强调色色条（顶部，所有 tab 共用） — 对齐 WinUI3 顶部色阶预览
// ============================================================================

QVector<QColor> shadeStripColors(const QColor &base, int count = 7)
{
    float h = 0.0f, s = 0.0f, v = 0.0f, a = 1.0f;
    base.getHsvF(&h, &s, &v, &a);

    QVector<QColor> colors;
    colors.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        const float t = count > 1 ? float(i) / float(count - 1) : 0.0f;
        colors.append(QColor::fromHsvF(h, qMax(0.05f, s * (0.3f + 0.7f * t)),
                                        qMin(1.0f, 0.3f + 0.7f * t), a));
    }
    return colors;
}

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
        m_colors = shadeStripColors(color);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        if (m_colors.isEmpty())
            return;

        const int gap = 2;
        const int cellW = qMax(1, (width() - gap * (m_colors.size() - 1)) / m_colors.size());
        int x = 0;
        for (int i = 0; i < m_colors.size(); ++i)
        {
            const QRect cell(x, 0, cellW, height() - 1);
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_colors.at(i));
            painter.drawRoundedRect(cell, 3, 3);
            x += cellW + gap;
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (m_colors.isEmpty())
            return;
        const int gap = 2;
        const int cellW = qMax(1, (width() - gap * (m_colors.size() - 1)) / m_colors.size());
        const int index = qBound(0, int(event->position().x()) / (cellW + gap), m_colors.size() - 1);
        if (onColorSelected)
            onColorSelected(m_colors.at(index));
    }

private:
    QVector<QColor> m_colors;
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
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMouseTracking(true);
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

    qreal hue() const { return m_hue; }
    qreal saturation() const { return m_saturation; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRect r = rect().adjusted(0, 0, -1, -1);
        if (m_image.isNull() || m_image.size() != r.size())
            m_image = buildHueSaturationImage(r.width(), r.height());
        painter.drawImage(r, m_image);

        painter.setPen(QPen(palette().color(QPalette::Mid), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(r);

        const QPointF marker = markerPosition(r);
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(marker, 7, 7);
        painter.setPen(QPen(Qt::black, 1));
        painter.drawEllipse(marker, 7, 7);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        m_image = QImage();
        update();
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
            updateFromPos(event->position().toPoint());
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons() & Qt::LeftButton)
            updateFromPos(event->position().toPoint());
    }

private:
    QPointF markerPosition(const QRect &area) const
    {
        return QPointF(area.left() + m_hue * area.width(),
                       area.top() + (1.0 - m_saturation) * area.height());
    }

    void updateFromPos(const QPoint &pos)
    {
        const QRect area = rect().adjusted(0, 0, -1, -1);
        const qreal hue = qBound(0.0, (pos.x() - area.left()) / qMax<qreal>(1.0, area.width()), 1.0);
        const qreal saturation = qBound(0.0, 1.0 - (pos.y() - area.top()) / qMax<qreal>(1.0, area.height()), 1.0);
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
    QImage m_image;
};

// ============================================================================
// 色板网格（对齐 WinUI3 PaletteItemGridView）
// ============================================================================

class PaletteWidget : public QWidget
{
public:
    std::function<void(const QColor &)> onColorSelected;

    explicit PaletteWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        computeLayout();
    }

    void setColors(const QVector<QColor> &colors)
    {
        m_colors = colors;
        computeLayout();
        update();
    }

    void setSelectedColor(const QColor &color)
    {
        m_selectedColor = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        for (int i = 0; i < m_cellRects.size() && i < m_colors.size(); ++i)
        {
            const QRect &cell = m_cellRects[i];
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_colors[i]);

            // 高亮当前选中项
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
        const QPoint pos = event->position().toPoint();
        for (int i = 0; i < m_cellRects.size() && i < m_colors.size(); ++i)
        {
            if (m_cellRects[i].contains(pos))
            {
                m_selectedColor = m_colors[i];
                update();
                if (onColorSelected)
                    onColorSelected(m_colors[i]);
                return;
            }
        }
    }

private:
    void computeLayout()
    {
        m_cellRects.clear();
        if (m_colors.isEmpty())
        {
            setFixedSize(0, 0);
            return;
        }

        const int cols = qMin(kPaletteColumns, m_colors.size());
        const int rows = (m_colors.size() + cols - 1) / cols;
        const int gap = 3;

        const int totalW = cols * kPaletteCell + (cols - 1) * gap;
        const int totalH = rows * kPaletteCell + (rows - 1) * gap;
        setFixedSize(totalW, totalH);

        for (int i = 0; i < m_colors.size(); ++i)
        {
            const int col = i % cols;
            const int row = i / cols;
            m_cellRects.append(QRect(col * (kPaletteCell + gap), row * (kPaletteCell + gap),
                                     kPaletteCell, kPaletteCell));
        }
    }

    QVector<QColor> m_colors;
    QVector<QRect> m_cellRects;
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

    ExColorPicker *q_ptr;
    QColor m_color = QColor(0x94, 0x4E, 0x9B);

    // 属性
    bool m_alphaEnabled = true;
    bool m_spectrumVisible = true;
    bool m_paletteVisible = true;
    bool m_previewVisible = true;
    bool m_alphaSliderVisible = true;
    bool m_channelTextInputVisible = true;
    ExColorPicker::ColorRepresentation m_representation = ExColorPicker::Rgba;
    ExColorPicker::ColorSpectrumShape m_spectrumShape = ExColorPicker::Box;
    QVector<QColor> m_customPaletteColors;
    bool m_updating = false;

    // HSVA 缓存
    float m_h = 0.0f;
    float m_s = 1.0f;
    float m_v = 1.0f;
    float m_a = 1.0f;

    // Widget 成员
    AccentShadeStripWidget *shadeStrip = nullptr;
    QTabBar *tabBar = nullptr;
    QStackedWidget *stack = nullptr;

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

void ExColorPickerPrivate::syncSpectrumWidgets()
{
    if (!hueSatMap || !thirdDimSlider)
        return;
    hueSatMap->setHueSaturation(m_h, m_s);

    // 更新第三维滑条渐变图并设置值
    thirdDimSlider->setImageBuilder([this](QSize sz) {
        return buildThirdDimensionImage(m_h, m_s, sz.width(), sz.height());
    });
    thirdDimSlider->blockSignals(true);
    // 垂直滑条：最大值在上 (inverted)
    thirdDimSlider->setValue(qRound((1.0f - m_v) * 255));
    thirdDimSlider->blockSignals(false);

    if (spectrumAlphaSlider)
    {
        spectrumAlphaSlider->setImageBuilder([this](QSize sz) {
            return buildAlphaChannelImage(sz.width(), sz.height(),
                                          QColor::fromHsvF(m_h, m_s, m_v, 1.0f));
        });
        spectrumAlphaSlider->blockSignals(true);
        spectrumAlphaSlider->setValue(qRound(m_a * 255));
        spectrumAlphaSlider->blockSignals(false);
    }
}

void ExColorPickerPrivate::syncRgbWidgets()
{
    if (!redSpin || !greenSpin || !blueSpin || !redSlider || !greenSlider || !blueSlider)
        return;

    redSpin->blockSignals(true);
    greenSpin->blockSignals(true);
    blueSpin->blockSignals(true);
    redSpin->setValue(m_color.red());
    greenSpin->setValue(m_color.green());
    blueSpin->setValue(m_color.blue());
    redSpin->blockSignals(false);
    greenSpin->blockSignals(false);
    blueSpin->blockSignals(false);

    // 通道滑条：渐变图显示其他通道固定时当前通道的变化
    redSlider->setImageBuilder([this](QSize sz) {
        return buildChannelImage(sz.width(), sz.height(), 0,
                                 QColor(0, m_color.green(), m_color.blue()));
    });
    greenSlider->setImageBuilder([this](QSize sz) {
        return buildChannelImage(sz.width(), sz.height(), 1,
                                 QColor(m_color.red(), 0, m_color.blue()));
    });
    blueSlider->setImageBuilder([this](QSize sz) {
        return buildChannelImage(sz.width(), sz.height(), 2,
                                 QColor(m_color.red(), m_color.green(), 0));
    });
    redSlider->blockSignals(true);
    greenSlider->blockSignals(true);
    blueSlider->blockSignals(true);
    redSlider->setValue(m_color.red());
    greenSlider->setValue(m_color.green());
    blueSlider->setValue(m_color.blue());
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
        alphaSlider->setImageBuilder([this](QSize sz) {
            return buildAlphaChannelImage(sz.width(), sz.height(),
                                          QColor(m_color.red(), m_color.green(), m_color.blue(), 255));
        });
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

    QFont iconFont = segoeIconFont(16);
    tabBar->setFont(iconFont);

    tabBar->setTabText(0, kSegoeIconSpectrum);
    tabBar->setTabText(1, kSegoeIconPalette);
    tabBar->setTabText(2, kSegoeIconSliders);
}

void ExColorPickerPrivate::updatePanelVisibility()
{
    if (!tabBar || !stack)
        return;

    // Spectrum tab
    const int spectrumIdx = 0;
    tabBar->setTabVisible(spectrumIdx, m_spectrumVisible);

    // Palette tab
    const int paletteIdx = 1;
    tabBar->setTabVisible(paletteIdx, m_paletteVisible);

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
}

void ExColorPickerPrivate::applyColor(const QColor &color, bool emitSignal, bool updateInputs)
{
    Q_Q(ExColorPicker);
    if (!color.isValid() || (m_color == color && m_updating))
        return;

    m_updating = true;
    m_color = color;
    m_color.getHsvF(&m_h, &m_s, &m_v, &m_a);

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
        Q_EMIT q->colorChanged(m_color);
}

void ExColorPickerPrivate::updateColorFromSpectrum(bool emitSignal)
{
    Q_Q(ExColorPicker);
    if (m_updating || !hueSatMap || !thirdDimSlider)
        return;

    m_h = float(hueSatMap->hue());
    m_s = float(hueSatMap->saturation());
    // 垂直滑条 inverted: 顶端=0, 底端=255, 对应 Value=1→0
    m_v = float(255 - thirdDimSlider->value()) / 255.0f;

    const QColor color = QColor::fromHsvF(m_h, m_s, m_v, m_a);
    if (m_color == color)
        return;

    m_updating = true;
    m_color = color;
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
        Q_EMIT q->colorChanged(m_color);
}

void ExColorPickerPrivate::updateColorFromRgb(bool emitSignal)
{
    Q_Q(ExColorPicker);
    if (m_updating || !redSpin || !greenSpin || !blueSpin)
        return;

    const QColor color(redSpin->value(), greenSpin->value(), blueSpin->value(),
                       m_alphaEnabled && alphaSpin ? alphaSpin->value() : 255);
    if (m_color == color)
        return;

    m_updating = true;
    m_color = color;
    m_color.getHsvF(&m_h, &m_s, &m_v, &m_a);
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
        Q_EMIT q->colorChanged(m_color);
}

void ExColorPickerPrivate::updateColorFromAlphaChange(int alpha)
{
    Q_Q(ExColorPicker);
    if (m_updating)
        return;

    m_updating = true;
    m_color.setAlpha(alpha);
    m_a = alpha / 255.0f;
    syncRgbWidgets();
    if (hexEdit)
        hexEdit->setText(m_alphaEnabled
                             ? m_color.name(QColor::HexArgb).mid(1).toUpper()
                             : m_color.name(QColor::HexRgb).toUpper());
    refreshShadeStrip();
    m_updating = false;

    Q_EMIT q->colorChanged(m_color);
}

// ============================================================================
// ExColorPicker 构造与析构
// ============================================================================

ExColorPicker::ExColorPicker(QWidget *parent)
    : QWidget(parent), d_ptr(new ExColorPickerPrivate(this))
{
    Q_D(ExColorPicker);
    setFixedWidth(kFlyoutWidth);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 8, 12, 12);
    root->setSpacing(8);

    // ---- 强调色色条（顶部，对齐 WinUI3 shade strip）----
    d->shadeStrip = new AccentShadeStripWidget(this);
    root->addWidget(d->shadeStrip);

    // ---- 分段 TabBar（对齐 WinUI3 ColorPanelSelector Segmented）----
    d->tabBar = new QTabBar(this);
    d->tabBar->setObjectName(QStringLiteral("exColorPickerTabBar"));
    d->tabBar->setAttribute(Qt::WA_StyledBackground, true);
    d->tabBar->setDrawBase(false);
    d->tabBar->setExpanding(true);
    d->tabBar->setProperty(TabBarStyleProperty, Segmented_WinUI3);
    d->tabBar->addTab(QString());
    d->tabBar->addTab(QString());
    d->tabBar->addTab(QString());
    d->tabBar->setTabToolTip(0, tr("Spectrum"));
    d->tabBar->setTabToolTip(1, tr("Palette"));
    d->tabBar->setTabToolTip(2, tr("Sliders"));
    d->updateTabIcons();
    root->addWidget(d->tabBar);

    d->stack = new QStackedWidget(this);
    root->addWidget(d->stack);

    // ======== Spectrum 页面 ========
    auto *spectrumPage = new QWidget(d->stack);
    auto *spectrumMainLay = new QVBoxLayout(spectrumPage);
    spectrumMainLay->setContentsMargins(0, 0, 0, 0);
    spectrumMainLay->setSpacing(6);

    // Hue×Saturation + 第三维滑条（左右布局）
    auto *spectrumRow = new QHBoxLayout;
    spectrumRow->setSpacing(8);
    d->thirdDimSlider = new ColorGradientSlider(Qt::Vertical, spectrumPage);
    d->thirdDimSlider->setFixedWidth(kChannelThickness);
    d->thirdDimSlider->setFixedHeight(kSpectrumHeight);
    d->hueSatMap = new HueSaturationMapWidget(spectrumPage);
    d->hueSatMap->setFixedHeight(kSpectrumHeight);
    spectrumRow->addWidget(d->thirdDimSlider);
    spectrumRow->addWidget(d->hueSatMap, 1);
    spectrumMainLay->addLayout(spectrumRow);

    // Spectrum Alpha 滑条
    d->spectrumAlphaSlider = new ColorGradientSlider(Qt::Horizontal, spectrumPage);
    d->spectrumAlphaSlider->setFixedHeight(kAlphaThickness);
    spectrumMainLay->addWidget(d->spectrumAlphaSlider);

    d->stack->addWidget(spectrumPage);

    // ======== Palette 页面 ========
    auto *palettePage = new QWidget(d->stack);
    auto *paletteLay = new QHBoxLayout(palettePage);
    paletteLay->setContentsMargins(0, 0, 0, 0);
    paletteLay->addStretch(1);
    d->palette = new PaletteWidget(palettePage);
    d->palette->setColors(d->m_customPaletteColors.isEmpty() ? fluentPaletteColors() : d->m_customPaletteColors);
    paletteLay->addWidget(d->palette);
    paletteLay->addStretch(1);
    d->stack->addWidget(palettePage);

    // ======== Sliders 页面（对齐 WinUI3 4 通道滑条布局）========
    auto *slidersPage = new QWidget(d->stack);
    auto *slidersLay = new QVBoxLayout(slidersPage);
    slidersLay->setContentsMargins(0, 0, 0, 0);
    slidersLay->setSpacing(6);

    // 顶行：颜色模式下拉 + Hex 输入
    auto *headerRow = new QHBoxLayout;
    d->repCombo = new QComboBox(slidersPage);
    d->repCombo->addItem(QStringLiteral("RGB"));
    d->repCombo->addItem(QStringLiteral("HSV"));
    d->repCombo->setCurrentIndex(0);
    d->repCombo->setFixedWidth(72);
    headerRow->addWidget(d->repCombo);

    d->hexEdit = new QLineEdit(slidersPage);
    d->hexEdit->setPlaceholderText(QStringLiteral("944E9B"));
    d->hexEdit->setMaxLength(8);
    d->hexEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[0-9A-Fa-f]{6,8}")), d->hexEdit));
    headerRow->addWidget(d->hexEdit, 1);
    slidersLay->addLayout(headerRow);

    // 通道行构建辅助函数
    auto addChannelRow = [&](const QString &label, QLabel **lbl, QSpinBox **spin,
                             ColorGradientSlider **slider)
    {
        auto *row = new QHBoxLayout;
        row->setSpacing(6);

        auto *labelWidget = new QLabel(label, slidersPage);
        labelWidget->setFixedWidth(16);
        labelWidget->setAlignment(Qt::AlignCenter);
        row->addWidget(labelWidget);

        auto *spinBox = new QSpinBox(slidersPage);
        spinBox->setRange(0, 255);
        spinBox->setFixedWidth(56);
        spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        row->addWidget(spinBox);

        auto *sliderWidget = new ColorGradientSlider(Qt::Horizontal, slidersPage);
        sliderWidget->setFixedHeight(kChannelThickness);
        row->addWidget(sliderWidget, 1);

        slidersLay->addLayout(row);
        *lbl = labelWidget;
        *spin = spinBox;
        *slider = sliderWidget;
    };

    addChannelRow(QStringLiteral("R"), &d->redLabel, &d->redSpin, &d->redSlider);
    addChannelRow(QStringLiteral("G"), &d->greenLabel, &d->greenSpin, &d->greenSlider);
    addChannelRow(QStringLiteral("B"), &d->blueLabel, &d->blueSpin, &d->blueSlider);

    // Alpha 通道行
    {
        auto *alphaRow = new QHBoxLayout;
        alphaRow->setSpacing(6);

        d->alphaLabel = new QLabel(QStringLiteral("A"), slidersPage);
        d->alphaLabel->setFixedWidth(16);
        d->alphaLabel->setAlignment(Qt::AlignCenter);
        alphaRow->addWidget(d->alphaLabel);

        d->alphaSpin = new QSpinBox(slidersPage);
        d->alphaSpin->setRange(0, 255);
        d->alphaSpin->setFixedWidth(56);
        d->alphaSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
        alphaRow->addWidget(d->alphaSpin);

        d->alphaSlider = new ColorGradientSlider(Qt::Horizontal, slidersPage);
        d->alphaSlider->setFixedHeight(kAlphaThickness);
        alphaRow->addWidget(d->alphaSlider, 1);

        slidersLay->addLayout(alphaRow);
    }

    d->stack->addWidget(slidersPage);

    // ======== 信号连接 ========

    // shadeStrip: 点击色条色块
    d->shadeStrip->onColorSelected = [this](const QColor &color)
    {
        Q_D(ExColorPicker);
        d->applyColor(color, true, true);
    };

    // TabBar 切换
    connect(d->tabBar, &QTabBar::currentChanged, this, [this](int index)
            {
                Q_D(ExColorPicker);
                if (index < 0 || !d->stack)
                    return;
                d->stack->setCurrentIndex(index);
            });

    // Spectrum: 色域变化
    d->hueSatMap->onHueSaturationChanged = [this](qreal, qreal)
    {
        Q_D(ExColorPicker);
        if (d->m_updating)
            return;
        d->updateColorFromSpectrum(true);
    };

    connect(d->thirdDimSlider, &QSlider::valueChanged, this, [this](int)
    {
        Q_D(ExColorPicker);
        d->updateColorFromSpectrum(true);
    });

    // Spectrum Alpha 滑条
    connect(d->spectrumAlphaSlider, &QSlider::valueChanged, this, [this](int alpha)
    {
        Q_D(ExColorPicker);
        d->updateColorFromAlphaChange(alpha);
    });

    // Palette: 颜色选中
    d->palette->onColorSelected = [this](const QColor &color)
    {
        Q_D(ExColorPicker);
        d->applyColor(color, true, true);
    };

    // Sliders: RGB SpinBox 变化
    auto channelChanged = [this](int)
    {
        Q_D(ExColorPicker);
        d->updateColorFromRgb(true);
    };
    connect(d->redSpin, qOverload<int>(&QSpinBox::valueChanged), this, channelChanged);
    connect(d->greenSpin, qOverload<int>(&QSpinBox::valueChanged), this, channelChanged);
    connect(d->blueSpin, qOverload<int>(&QSpinBox::valueChanged), this, channelChanged);

    // Sliders: RGB 滑条拖动 → 更新对应的 SpinBox
    connect(d->redSlider, &QSlider::valueChanged, this, [this](int value)
    {
        Q_D(ExColorPicker);
        if (d->m_updating) return;
        d->redSpin->setValue(value);
    });
    connect(d->greenSlider, &QSlider::valueChanged, this, [this](int value)
    {
        Q_D(ExColorPicker);
        if (d->m_updating) return;
        d->greenSpin->setValue(value);
    });
    connect(d->blueSlider, &QSlider::valueChanged, this, [this](int value)
    {
        Q_D(ExColorPicker);
        if (d->m_updating) return;
        d->blueSpin->setValue(value);
    });

    // Sliders: Alpha 通道
    connect(d->alphaSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int alpha)
            {
                Q_D(ExColorPicker);
                d->updateColorFromAlphaChange(alpha);
            });
    connect(d->alphaSlider, &QSlider::valueChanged, this, [this](int alpha)
    {
        Q_D(ExColorPicker);
        if (d->m_updating) return;
        d->alphaSpin->setValue(alpha);
    });

    // Hex 输入
    connect(d->hexEdit, &QLineEdit::editingFinished, this, [this]()
            {
                Q_D(ExColorPicker);
                const QString text = d->hexEdit->text().trimmed();
                QColor parsed;
                if (text.length() == 8)
                    parsed = QColor(QStringLiteral("#") + text);
                else
                    parsed = QColor(QStringLiteral("#") + text);
                if (parsed.isValid())
                    d->applyColor(parsed, true, true);
            });

    // 颜色模式切换（后续可扩展 HSV 输入）
    connect(d->repCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx)
            {
                Q_D(ExColorPicker);
                d->m_representation = (idx == 0) ? Rgba : Hsva;
            });

    // 初始状态
    d->applyColor(d->m_color, false, true);
    d->tabBar->setCurrentIndex(0);
    d->stack->setCurrentIndex(SpectrumPage);
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
    return d->m_color;
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
    // 显示/隐藏 Alpha 相关控件
    if (d->alphaSpin)
        d->alphaSpin->setVisible(enabled);
    if (d->alphaSlider)
        d->alphaSlider->setVisible(enabled);
    if (d->alphaLabel)
        d->alphaLabel->setVisible(enabled);
    Q_EMIT alphaEnabledChanged(enabled);
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
        d->spectrumAlphaSlider->setVisible(visible);
    Q_EMIT alphaSliderVisibleChanged(visible);
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
    if (d->redSpin)
        d->redSpin->setVisible(visible);
    if (d->greenSpin)
        d->greenSpin->setVisible(visible);
    if (d->blueSpin)
        d->blueSpin->setVisible(visible);
    if (d->alphaSpin)
        d->alphaSpin->setVisible(visible && d->m_alphaEnabled);
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
    d->m_spectrumShape = shape;
    // Ring 模式暂未实现，保持 Box 行为
}

QSize ExColorPicker::sizeHint() const
{
    return {kFlyoutWidth, kShadeStripHeight + kTabBarHeight + kSpectrumHeight + kAlphaThickness + 60};
}

QSize ExColorPicker::minimumSizeHint() const
{
    return sizeHint();
}

void ExColorPicker::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
    {
        Q_D(ExColorPicker);
        d->refreshShadeStrip();
        d->updateTabIcons();
        update();
    }
}
