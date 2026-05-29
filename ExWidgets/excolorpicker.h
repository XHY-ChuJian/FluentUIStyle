#pragma once

#include "exwidgets_global.h"

#include <QColor>
#include <QVector>
#include <QWidget>

class ExColorPickerPrivate;

/**
 * @brief WinUI3 CommunityToolkit ColorPicker 风格的取色控件。
 *
 * 对齐 WinUI3 CommunityToolkit ColorPicker 的 API 与布局：
 * - 顶部：ColorPreviewer（当前色预览 + 强调色色板）
 * - 分段控件切换：Spectrum / Palette / Sliders
 * - Spectrum：ColorSpectrum（Hue×Saturation 色域 + 第三维滑条） + Alpha 滑条
 * - Palette：Fluent 标准色板 / 自定义色板
 * - Sliders：颜色模式下拉（RGB/HSV）+ Hex 输入 + 4 通道（含 Alpha）滑条 + 数值输入
 *
 * 对应 WinUI3 属性：
 * - IsAlphaEnabled          → isAlphaEnabled
 * - IsColorSpectrumVisible  → isColorSpectrumVisible
 * - IsColorPaletteVisible   → isColorPaletteVisible
 * - IsColorPreviewVisible   → isColorPreviewVisible
 * - IsAlphaSliderVisible    → isAlphaSliderVisible
 * - IsColorChannelTextInputVisible → isColorChannelTextInputVisible
 * - ColorRepresentation     → colorRepresentation (Hsva / Rgba)
 * - CustomPaletteColors     → customPaletteColors
 */
class EXWIDGETS_EXPORT ExColorPicker : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(bool isAlphaEnabled READ isAlphaEnabled WRITE setAlphaEnabled NOTIFY alphaEnabledChanged FINAL)
    Q_PROPERTY(bool isColorSpectrumVisible READ isColorSpectrumVisible WRITE setColorSpectrumVisible NOTIFY colorSpectrumVisibleChanged FINAL)
    Q_PROPERTY(bool isColorPaletteVisible READ isColorPaletteVisible WRITE setColorPaletteVisible NOTIFY colorPaletteVisibleChanged FINAL)
    Q_PROPERTY(bool isColorPreviewVisible READ isColorPreviewVisible WRITE setColorPreviewVisible NOTIFY colorPreviewVisibleChanged FINAL)
    Q_PROPERTY(bool isAlphaSliderVisible READ isAlphaSliderVisible WRITE setAlphaSliderVisible NOTIFY alphaSliderVisibleChanged FINAL)
    Q_PROPERTY(bool isColorChannelTextInputVisible READ isColorChannelTextInputVisible WRITE setColorChannelTextInputVisible NOTIFY colorChannelTextInputVisibleChanged FINAL)

public:
    enum ColorRepresentation
    {
        Hsva,
        Rgba
    };
    Q_ENUM(ColorRepresentation)

    /// 对应 WinUI3 ColorSpectrumShape
    enum ColorSpectrumShape
    {
        Box,
        Ring
    };
    Q_ENUM(ColorSpectrumShape)

    explicit ExColorPicker(QWidget *parent = nullptr);
    ~ExColorPicker() override;

    // ---- 核心属性 ----

    QColor color() const;
    void setColor(const QColor &color);

    bool isAlphaEnabled() const;
    void setAlphaEnabled(bool enabled);

    ColorRepresentation colorRepresentation() const;
    void setColorRepresentation(ColorRepresentation representation);

    // ---- 面板可见性 ----

    bool isColorSpectrumVisible() const;
    void setColorSpectrumVisible(bool visible);

    bool isColorPaletteVisible() const;
    void setColorPaletteVisible(bool visible);

    bool isColorPreviewVisible() const;
    void setColorPreviewVisible(bool visible);

    bool isAlphaSliderVisible() const;
    void setAlphaSliderVisible(bool visible);

    bool isColorChannelTextInputVisible() const;
    void setColorChannelTextInputVisible(bool visible);

    // ---- 色板 ----

    QVector<QColor> customPaletteColors() const;
    void setCustomPaletteColors(const QVector<QColor> &colors);

    // ---- 色域形状 ----

    ColorSpectrumShape colorSpectrumShape() const;
    void setColorSpectrumShape(ColorSpectrumShape shape);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

Q_SIGNALS:
    void colorChanged(const QColor &color);
    void alphaEnabledChanged(bool enabled);
    void colorSpectrumVisibleChanged(bool visible);
    void colorPaletteVisibleChanged(bool visible);
    void colorPreviewVisibleChanged(bool visible);
    void alphaSliderVisibleChanged(bool visible);
    void colorChannelTextInputVisibleChanged(bool visible);

protected:
    void changeEvent(QEvent *event) override;

private:
    Q_DECLARE_PRIVATE(ExColorPicker)
    ExColorPickerPrivate *d_ptr;
};
