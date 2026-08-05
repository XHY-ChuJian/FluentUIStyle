#include "rangeslidershowcasewidget.h"

#include "fluentui3styleproperties.h"

#include <exrangeslider.h>

#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

namespace
{

QWidget *makeCard(QWidget *parent)
{
    auto *card = new QWidget(parent);
    card->setProperty("isCard", true);
    card->setAttribute(Qt::WA_StyledBackground, true);
    return card;
}

QLabel *makeSectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    QFont f = label->font();
    f.setBold(true);
    f.setPixelSize(14);
    label->setFont(f);
    return label;
}

} // namespace

RangeSliderShowcaseWidget::RangeSliderShowcaseWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);

    auto *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(16, 16, 16, 16);
    mainLay->setSpacing(16);

    auto *title = new QLabel(tr("ExRangeSlider"), this);
    {
        QFont f = title->font();
        f.setPointSize(16);
        f.setBold(true);
        title->setFont(f);
    }
    mainLay->addWidget(title);

    auto *hint = new QLabel(
        tr("WinUI3 风格双滑块范围选择控件。可调整刻度显示、刻度间隔与 tracking（拖动时是否实时更新数值）。"),
        this);
    hint->setWordWrap(true);
    mainLay->addWidget(hint);

    auto *demoCard = makeCard(this);
    auto *demoLay = new QVBoxLayout(demoCard);
    demoLay->setContentsMargins(12, 12, 12, 12);
    demoLay->setSpacing(12);
    demoLay->addWidget(makeSectionTitle(tr("演示"), demoCard));

    auto *rangeSlider = new ExRangeSlider(Qt::Horizontal, demoCard);
    rangeSlider->setObjectName(QStringLiteral("rangeSelector"));
    rangeSlider->setMinimumHeight(32);
    rangeSlider->setProperty(SliderValueTipProperty, true);
    rangeSlider->setRange(0, 100);
    rangeSlider->setValues(20, 80);
    rangeSlider->setSingleStep(1);
    rangeSlider->setPageStep(10);
    rangeSlider->setTickPosition(true);
    rangeSlider->setTickInterval(10);
    rangeSlider->setTracking(true);

    auto *valueLabel = new QLabel(demoCard);
    valueLabel->setWordWrap(true);

    const auto refreshValueLabel = [valueLabel](int lower, int upper)
    {
        valueLabel->setText(tr("当前范围：%1 – %2").arg(lower).arg(upper));
    };
    refreshValueLabel(rangeSlider->lowerValue(), rangeSlider->upperValue());

    connect(rangeSlider, &ExRangeSlider::valuesChanged, demoCard, refreshValueLabel);
    connect(rangeSlider, &ExRangeSlider::sliderMoved, demoCard, refreshValueLabel);

    demoLay->addWidget(rangeSlider);
    demoLay->addWidget(valueLabel);
    mainLay->addWidget(demoCard);

    auto *optionsCard = makeCard(this);
    auto *optionsLay = new QVBoxLayout(optionsCard);
    optionsLay->setContentsMargins(12, 12, 12, 12);
    optionsLay->setSpacing(12);
    optionsLay->addWidget(makeSectionTitle(tr("属性"), optionsCard));

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(10);

    auto *tickCheck = new QCheckBox(tr("显示刻度"), optionsCard);
    tickCheck->setChecked(rangeSlider->hasTickPosition());

    auto *tickIntervalSpin = new QSpinBox(optionsCard);
    tickIntervalSpin->setRange(1, 50);
    tickIntervalSpin->setValue(qMax(1, rangeSlider->tickInterval()));
    tickIntervalSpin->setEnabled(tickCheck->isChecked());

    auto *trackingCheck = new QCheckBox(tr("Tracking（拖动时实时更新）"), optionsCard);
    trackingCheck->setChecked(rangeSlider->hasTracking());

    auto *valueTipCheck = new QCheckBox(tr("拖动数值 tooltip"), optionsCard);
    valueTipCheck->setChecked(rangeSlider->property(SliderValueTipProperty).toBool());

    form->addRow(tickCheck);
    form->addRow(tr("刻度间隔"), tickIntervalSpin);
    form->addRow(trackingCheck);
    form->addRow(valueTipCheck);
    optionsLay->addLayout(form);
    mainLay->addWidget(optionsCard);

    connect(tickCheck, &QCheckBox::toggled, rangeSlider, &ExRangeSlider::setTickPosition);
    connect(tickCheck, &QCheckBox::toggled, tickIntervalSpin, &QSpinBox::setEnabled);

    connect(tickIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), rangeSlider, &ExRangeSlider::setTickInterval);

    connect(trackingCheck, &QCheckBox::toggled, rangeSlider, &ExRangeSlider::setTracking);

    connect(valueTipCheck, &QCheckBox::toggled, rangeSlider, [rangeSlider](bool checked)
            { rangeSlider->setProperty(SliderValueTipProperty, checked); });

    mainLay->addStretch();
}
