#pragma once

#include "exwidgets_global.h"

#include <QColor>
#include <QToolButton>

class ExColorPicker;

/**
 * @brief 带 Flyout 的颜色选择按钮（对应 CommunityToolkit ColorPickerButton）。
 *
 * 按钮 Content 显示当前颜色（含透明棋盘格背景），点击弹出 ExColorPicker。
 */
class EXWIDGETS_EXPORT ExColorPickerButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(QColor selectedColor READ selectedColor WRITE setSelectedColor NOTIFY selectedColorChanged FINAL)

public:
    explicit ExColorPickerButton(QWidget *parent = nullptr);

    QColor selectedColor() const;
    void setSelectedColor(const QColor &color);

    ExColorPicker *colorPicker() const;

Q_SIGNALS:
    void selectedColorChanged(const QColor &color);

private:
    void syncButtonAppearance();
    void ensurePopup();

    ExColorPicker *m_picker = nullptr;
    QColor m_selectedColor = Qt::blue;
};
