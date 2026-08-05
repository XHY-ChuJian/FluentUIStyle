#include "progressringshowcasewidget.h"

#include "fluentui3styleproperties.h"

#include <excolorpickerbutton.h>

#include <QCheckBox>
#include <QColor>
#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVariant>

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
    QFont font = label->font();
    font.setBold(true);
    font.setPixelSize(14);
    label->setFont(font);
    return label;
}

QProgressBar *makeRing(QWidget *parent, int value, int side = 80)
{
    auto *ring = new QProgressBar(parent);
    ring->setProperty(ProgressBarStyleProperty, ProgressBarRing);
    ring->setRange(0, 100);
    ring->setValue(value);
    ring->setAlignment(Qt::AlignCenter);
    ring->setFixedSize(side, side);
    return ring;
}

QColor ringAccentColor(const QWidget *widget)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    return widget->palette().color(QPalette::Accent);
#else
    return widget->palette().color(QPalette::Highlight);
#endif
}

void setRingAccentColor(QWidget *widget, const QColor &color)
{
    QPalette palette = widget->palette();
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    palette.setColor(QPalette::Accent, color);
#else
    palette.setColor(QPalette::Highlight, color);
#endif
    widget->setPalette(palette);
}

void setRingTrackColor(QWidget *widget, const QColor &color)
{
    QPalette palette = widget->palette();
    palette.setColor(QPalette::All, QPalette::Mid, color);
    widget->setPalette(palette);
}

} // namespace

ProgressRingShowcaseWidget::ProgressRingShowcaseWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setAutoFillBackground(false);
    scrollArea->viewport()->setAutoFillBackground(false);
    rootLayout->addWidget(scrollArea);

    auto *content = new QWidget(scrollArea);
    content->setAutoFillBackground(false);
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    auto *title = new QLabel(tr("ProgressRing"), content);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    mainLayout->addWidget(title);

    auto *description = new QLabel(
        tr("使用标准 QProgressBar 的范围、数值和文本能力，仅通过 progressBarStyle 属性切换为环形外观。"),
        content);
    description->setWordWrap(true);
    mainLayout->addWidget(description);

    auto *statesCard = makeCard(content);
    auto *statesLayout = new QVBoxLayout(statesCard);
    statesLayout->setContentsMargins(16, 16, 16, 16);
    statesLayout->setSpacing(12);
    statesLayout->addWidget(makeSectionTitle(tr("基本状态"), statesCard));

    auto *ringsLayout = new QHBoxLayout;
    ringsLayout->setSpacing(24);
    ringsLayout->addStretch();

    const QList<int> values{0, 25, 50, 75, 100};
    for (int value : values)
    {
        auto *sample = new QWidget(statesCard);
        auto *sampleLayout = new QVBoxLayout(sample);
        sampleLayout->setContentsMargins(0, 0, 0, 0);
        sampleLayout->setSpacing(8);

        auto *ring = makeRing(sample, value);
        auto *label = new QLabel(tr("%1%").arg(value), sample);
        label->setAlignment(Qt::AlignCenter);
        sampleLayout->addWidget(ring, 0, Qt::AlignHCenter);
        sampleLayout->addWidget(label);
        ringsLayout->addWidget(sample);
    }

    auto *busySample = new QWidget(statesCard);
    auto *busyLayout = new QVBoxLayout(busySample);
    busyLayout->setContentsMargins(0, 0, 0, 0);
    busyLayout->setSpacing(8);
    auto *busyRing = makeRing(busySample, 0);
    busyRing->setRange(0, 0);
    busyRing->setTextVisible(false);
    auto *busyLabel = new QLabel(tr("不确定"), busySample);
    busyLabel->setAlignment(Qt::AlignCenter);
    busyLayout->addWidget(busyRing, 0, Qt::AlignHCenter);
    busyLayout->addWidget(busyLabel);
    ringsLayout->addWidget(busySample);
    ringsLayout->addStretch();

    statesLayout->addLayout(ringsLayout);
    mainLayout->addWidget(statesCard);

    auto *propertiesCard = makeCard(content);
    auto *propertiesLayout = new QVBoxLayout(propertiesCard);
    propertiesLayout->setContentsMargins(16, 16, 16, 16);
    propertiesLayout->setSpacing(12);
    propertiesLayout->addWidget(makeSectionTitle(tr("属性"), propertiesCard));

    auto *previewLayout = new QHBoxLayout;
    previewLayout->setSpacing(28);
    auto *previewRing = makeRing(propertiesCard, 65, 156);
    previewLayout->addWidget(previewRing, 0, Qt::AlignCenter);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(10);

    auto *valueSlider = new QSlider(Qt::Horizontal, propertiesCard);
    valueSlider->setRange(0, 100);
    valueSlider->setValue(previewRing->value());

    auto *indeterminateDurationSpin = new QSpinBox(propertiesCard);
    indeterminateDurationSpin->setRange(100, 5000);
    indeterminateDurationSpin->setSingleStep(100);
    indeterminateDurationSpin->setSuffix(tr(" ms"));
    indeterminateDurationSpin->setValue(ProgressBarRingDefaultIndeterminateDuration);

    auto *progressThicknessSpin = new QDoubleSpinBox(propertiesCard);
    progressThicknessSpin->setRange(1.0, 32.0);
    progressThicknessSpin->setDecimals(1);
    progressThicknessSpin->setSingleStep(0.5);
    progressThicknessSpin->setSuffix(tr(" px"));
    progressThicknessSpin->setValue(ProgressBarRingDefaultThickness);

    auto *ringColorButton = new ExColorPickerButton(propertiesCard);
    ringColorButton->setSelectedColor(ringAccentColor(previewRing));
    auto *trackColorButton = new ExColorPickerButton(propertiesCard);
    trackColorButton->setSelectedColor(QColor(Qt::gray));

    auto *busyCheck = new QCheckBox(tr("不确定进度"), propertiesCard);
    auto *textVisibleCheck = new QCheckBox(tr("显示百分比"), propertiesCard);
    textVisibleCheck->setChecked(previewRing->isTextVisible());
    auto *disabledCheck = new QCheckBox(tr("禁用状态"), propertiesCard);
    auto *resetButton = new QPushButton(tr("恢复默认属性"), propertiesCard);

    form->addRow(tr("进度"), valueSlider);
    form->addRow(tr("不确定动画周期"), indeterminateDurationSpin);
    form->addRow(tr("环与 Track 宽度"), progressThicknessSpin);
    form->addRow(tr("进度环颜色"), ringColorButton);
    form->addRow(tr("Track 颜色"), trackColorButton);
    form->addRow(busyCheck);
    form->addRow(textVisibleCheck);
    form->addRow(disabledCheck);
    form->addRow(resetButton);
    previewLayout->addLayout(form, 1);
    propertiesLayout->addLayout(previewLayout);

    auto refreshRing = [previewRing]
    {
        previewRing->updateGeometry();
        previewRing->update();
    };

    connect(valueSlider, &QSlider::valueChanged, previewRing, &QProgressBar::setValue);
    connect(indeterminateDurationSpin,
            QOverload<int>::of(&QSpinBox::valueChanged),
            previewRing,
            [previewRing](int duration)
            {
                previewRing->setProperty(ProgressBarRingIndeterminateDurationProperty, duration);
            });
    connect(progressThicknessSpin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            previewRing,
            [previewRing, refreshRing](double value)
            {
                previewRing->setProperty(ProgressBarThicknessProperty, value);
                refreshRing();
            });
    connect(ringColorButton,
            &ExColorPickerButton::selectedColorChanged,
            previewRing,
            [previewRing, refreshRing](const QColor &color)
            {
                setRingAccentColor(previewRing, color);
                refreshRing();
            });
    connect(trackColorButton,
            &ExColorPickerButton::selectedColorChanged,
            previewRing,
            [previewRing, refreshRing](const QColor &color)
            {
                setRingTrackColor(previewRing, color);
                refreshRing();
            });
    connect(busyCheck, &QCheckBox::toggled, previewRing, [previewRing, valueSlider](bool busy)
            {
                if (busy)
                {
                    previewRing->setRange(0, 0);
                }
                else
                {
                    previewRing->setRange(0, 100);
                    previewRing->setValue(valueSlider->value());
                }
                valueSlider->setEnabled(!busy);
            });
    connect(textVisibleCheck, &QCheckBox::toggled, previewRing, &QProgressBar::setTextVisible);
    connect(disabledCheck, &QCheckBox::toggled, previewRing, [previewRing](bool disabled)
            { previewRing->setEnabled(!disabled); });
    connect(resetButton,
            &QPushButton::clicked,
            previewRing,
            [previewRing, indeterminateDurationSpin, progressThicknessSpin, ringColorButton, trackColorButton, refreshRing]
            {
                const QSignalBlocker durationBlocker(indeterminateDurationSpin);
                const QSignalBlocker progressBlocker(progressThicknessSpin);
                const QSignalBlocker ringColorBlocker(ringColorButton);
                const QSignalBlocker trackColorBlocker(trackColorButton);
                indeterminateDurationSpin->setValue(ProgressBarRingDefaultIndeterminateDuration);
                progressThicknessSpin->setValue(ProgressBarRingDefaultThickness);
                previewRing->setProperty(ProgressBarRingIndeterminateDurationProperty,
                                         ProgressBarRingDefaultIndeterminateDuration);
                previewRing->setProperty(ProgressBarThicknessProperty, QVariant());
                previewRing->setPalette(QPalette());
                setRingTrackColor(previewRing, QColor(Qt::gray));
                ringColorButton->setSelectedColor(ringAccentColor(previewRing));
                trackColorButton->setSelectedColor(QColor(Qt::gray));
                refreshRing();
            });

    auto *code = new QPlainTextEdit(propertiesCard);
    code->setReadOnly(true);
    code->setMaximumHeight(128);
    code->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    code->setPlainText(QStringLiteral(
        "bar->setProperty(ProgressBarStyleProperty, ProgressBarRing);\n"
        "bar->setProperty(ProgressBarRingIndeterminateDurationProperty, 800);\n"
        "bar->setProperty(ProgressBarThicknessProperty, 8.0);\n"
        "auto palette = bar->palette();\n"
        "palette.setColor(QPalette::Accent, QColor(\"#7F5AF0\"));\n"
        "palette.setColor(QPalette::Mid, QColor(\"#60808080\"));\n"
        "bar->setPalette(palette);"));
    propertiesLayout->addWidget(code);
    mainLayout->addWidget(propertiesCard);
    mainLayout->addStretch();

    scrollArea->setWidget(content);
}
