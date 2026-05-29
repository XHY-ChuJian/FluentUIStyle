#include "excolorpickerbutton.h"

#include "excolorpicker.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidgetAction>

namespace
{
constexpr int kCheckerSize = 4;
constexpr QColor kCheckerColor(0x19, 0x80, 0x80, 0x80);

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
} // namespace

ExColorPickerButton::ExColorPickerButton(QWidget *parent)
    : QToolButton(parent)
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setPopupMode(QToolButton::InstantPopup);
    setArrowType(Qt::DownArrow);
    setFixedSize(56, 32);
    ensurePopup();
    syncButtonAppearance();
}

QColor ExColorPickerButton::selectedColor() const
{
    return m_selectedColor;
}

void ExColorPickerButton::setSelectedColor(const QColor &color)
{
    if (m_selectedColor == color)
        return;

    m_selectedColor = color;
    if (m_picker && m_picker->color() != color)
        m_picker->setColor(color);
    syncButtonAppearance();
    Q_EMIT selectedColorChanged(m_selectedColor);
}

ExColorPicker *ExColorPickerButton::colorPicker() const
{
    return m_picker;
}

void ExColorPickerButton::syncButtonAppearance()
{
    const int swatchSize = 18;
    QPixmap pix(swatchSize, swatchSize);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRect r(0, 0, swatchSize, swatchSize);
    paintCheckerboard(&painter, r);
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_selectedColor);
    painter.drawRoundedRect(r.adjusted(1, 1, -1, -1), 3, 3);
    painter.setPen(QPen(palette().color(QPalette::Mid), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(r.adjusted(1, 1, -1, -1), 3, 3);

    setIcon(QIcon(pix));
    setIconSize(QSize(swatchSize, swatchSize));
    setText(QString());
}

void ExColorPickerButton::ensurePopup()
{
    if (m_picker)
        return;

    auto *menu = new QMenu(this);
    menu->setWindowFlag(Qt::NoDropShadowWindowHint, false);

    auto *container = new QFrame(menu);
    container->setFrameShape(QFrame::NoFrame);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    m_picker = new ExColorPicker(container);
    m_picker->setColor(m_selectedColor);
    layout->addWidget(m_picker);

    auto *action = new QWidgetAction(menu);
    action->setDefaultWidget(container);
    menu->addAction(action);

    setMenu(menu);

    connect(m_picker, &ExColorPicker::colorChanged, this, [this](const QColor &color)
            {
                if (m_selectedColor == color)
                    return;
                m_selectedColor = color;
                syncButtonAppearance();
                Q_EMIT selectedColorChanged(m_selectedColor);
            });
}
