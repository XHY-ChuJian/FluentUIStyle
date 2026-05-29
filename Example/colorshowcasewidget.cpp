#include "colorshowcasewidget.h"

#include <excolorpicker.h>
#include <excolorpickerbutton.h>

#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace
{
QHBoxLayout *addCardSection(QVBoxLayout *mainLay, QWidget *contentParent, const QString &sectionTitle)
{
    auto *card = new QWidget(contentParent);
    card->setProperty("isCard", true);
    card->setAttribute(Qt::WA_StyledBackground, true);

    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(12, 12, 12, 12);
    v->setSpacing(10);

    auto *title = new QLabel(sectionTitle, card);
    {
        QFont f = title->font();
        f.setBold(true);
        f.setPixelSize(14);
        title->setFont(f);
    }
    v->addWidget(title);

    auto *row = new QHBoxLayout;
    row->setSpacing(8);
    row->addStretch(0);
    v->addLayout(row);

    mainLay->addWidget(card);
    return row;
}
} // namespace

ColorShowcaseWidget::ColorShowcaseWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setAutoFillBackground(false);

    auto *content = new QWidget(scroll);
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(scroll);

    auto *mainLay = new QVBoxLayout(content);
    mainLay->setContentsMargins(16, 16, 16, 16);
    mainLay->setSpacing(16);

    auto *title = new QLabel(tr("ColorPicker（CommunityToolkit 风格）"), content);
    {
        QFont f = title->font();
        f.setPointSize(16);
        f.setBold(true);
        title->setFont(f);
    }
    mainLay->addWidget(title);

    auto *hint = new QLabel(
        tr("布局对齐 WinUI3 ColorPicker：顶部色阶条、Spectrum / Palette / Sliders 三页 Tab，"
           "Spectrum 为左侧 Value 滑条 + 右侧 Hue×Saturation 色域。"),
        content);
    hint->setWordWrap(true);
    mainLay->addWidget(hint);

    ExColorPicker *inlinePicker = nullptr;

    {
        auto *card = new QWidget(content);
        card->setProperty("isCard", true);
        card->setAttribute(Qt::WA_StyledBackground, true);
        auto *v = new QVBoxLayout(card);
        v->setContentsMargins(12, 12, 12, 12);
        v->setSpacing(10);

        auto *sectionTitle = new QLabel(tr("内嵌 ColorPicker"), card);
        {
            QFont f = sectionTitle->font();
            f.setBold(true);
            f.setPixelSize(14);
            sectionTitle->setFont(f);
        }
        v->addWidget(sectionTitle);

        auto *resultLabel = new QLabel(card);
        resultLabel->setWordWrap(true);

        inlinePicker = new ExColorPicker(card);
        inlinePicker->setColor(QColor(0x94, 0x4E, 0x9B));
        connect(inlinePicker, &ExColorPicker::colorChanged, card, [resultLabel](const QColor &color)
                {
                    resultLabel->setText(tr("当前颜色：%1").arg(color.name(QColor::HexArgb).toUpper()));
                });
        resultLabel->setText(tr("当前颜色：%1").arg(inlinePicker->color().name(QColor::HexArgb).toUpper()));

        v->addWidget(inlinePicker);
        v->addWidget(resultLabel);
        mainLay->addWidget(card);
    }

    QHBoxLayout *rowBtn = addCardSection(mainLay, content, tr("ColorPickerButton（Flyout）"));
    auto *btnPicker = new ExColorPickerButton(content);
    btnPicker->setSelectedColor(QColor(255, 185, 0));
    auto *btnResult = new QLabel(content);
    btnResult->setWordWrap(true);
    btnResult->setText(tr("选中：%1").arg(btnPicker->selectedColor().name(QColor::HexArgb).toUpper()));
    connect(btnPicker, &ExColorPickerButton::selectedColorChanged, this, [btnResult](const QColor &color)
            {
                btnResult->setText(tr("选中：%1").arg(color.name(QColor::HexArgb).toUpper()));
            });
    rowBtn->insertWidget(0, btnPicker);
    rowBtn->insertWidget(1, btnResult);

    QHBoxLayout *rowOpts = addCardSection(mainLay, content, tr("显示选项"));
    auto *btnTogglePalette = new QPushButton(tr("切换标准色板"), content);
    auto *btnToggleAccent = new QPushButton(tr("切换颜色预览区"), content);
    connect(btnTogglePalette, &QPushButton::clicked, this, [inlinePicker]()
            {
                if (inlinePicker)
                    inlinePicker->setColorPaletteVisible(!inlinePicker->isColorPaletteVisible());
            });
    connect(btnToggleAccent, &QPushButton::clicked, this, [inlinePicker]()
            {
                if (inlinePicker)
                    inlinePicker->setColorPreviewVisible(!inlinePicker->isColorPreviewVisible());
            });
    rowOpts->insertWidget(0, btnTogglePalette);
    rowOpts->insertWidget(1, btnToggleAccent);

    mainLay->addStretch(1);
    scroll->setWidget(content);
    content->setAutoFillBackground(false);
}
