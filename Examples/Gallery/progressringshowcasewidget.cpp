#include "progressringshowcasewidget.h"

#include "fluentui3styleproperties.h"

#include <excolorpickerbutton.h>
#include <exprogressring.h>

#include <QCheckBox>
#include <QColor>
#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPalette>
#include <QPlainTextEdit>
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

ExProgressRing *makeRing(QWidget *parent, int value, int side = 80)
{
    auto *ring = new ExProgressRing(parent);
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
        tr("ExProgressRing 继承 QProgressBar，复用范围、数值、格式和 Ring Style，并增加可独立设置的中央标题、数值样式与自定义中心控件。"),
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
    previewRing->setTitle(tr("已完成"));
    QFont previewTitleFont = previewRing->font();
    previewTitleFont.setPixelSize(12);
    previewRing->setTitleFont(previewTitleFont);
    QFont previewValueFont = previewRing->font();
    previewValueFont.setPixelSize(24);
    previewValueFont.setWeight(QFont::DemiBold);
    previewRing->setValueFont(previewValueFont);
    auto *previewHost = new QWidget(propertiesCard);
    auto *previewHostLayout = new QVBoxLayout(previewHost);
    previewHostLayout->setContentsMargins(0, 0, 0, 0);
    previewHostLayout->addWidget(previewRing, 0, Qt::AlignHCenter | Qt::AlignTop);
    previewHostLayout->addStretch();
    previewLayout->addWidget(previewHost, 1);

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

    auto *titleEdit = new QLineEdit(previewRing->title(), propertiesCard);
    auto *formatEdit = new QLineEdit(previewRing->format(), propertiesCard);
    auto *titleFontSizeSpin = new QSpinBox(propertiesCard);
    titleFontSizeSpin->setRange(6, 72);
    titleFontSizeSpin->setValue(previewRing->titleFont().pixelSize());
    auto *valueFontSizeSpin = new QSpinBox(propertiesCard);
    valueFontSizeSpin->setRange(6, 96);
    valueFontSizeSpin->setValue(previewRing->valueFont().pixelSize());
    auto *textSpacingSpin = new QSpinBox(propertiesCard);
    textSpacingSpin->setRange(0, 40);
    textSpacingSpin->setValue(previewRing->textSpacing());
    auto *titleColorButton = new ExColorPickerButton(propertiesCard);
    QColor defaultTitleColor = previewRing->palette().color(QPalette::Text);
    defaultTitleColor.setAlphaF(defaultTitleColor.alphaF() * 0.72);
    titleColorButton->setSelectedColor(defaultTitleColor);
    auto *valueColorButton = new ExColorPickerButton(propertiesCard);
    valueColorButton->setSelectedColor(previewRing->palette().color(QPalette::Text));

    auto *busyCheck = new QCheckBox(tr("不确定进度"), propertiesCard);
    auto *textVisibleCheck = new QCheckBox(tr("显示中央文字"), propertiesCard);
    textVisibleCheck->setChecked(previewRing->isTextVisible());
    auto *disabledCheck = new QCheckBox(tr("禁用状态"), propertiesCard);
    auto *resetButton = new QPushButton(tr("恢复默认属性"), propertiesCard);

    form->addRow(tr("进度"), valueSlider);
    form->addRow(tr("不确定动画周期"), indeterminateDurationSpin);
    form->addRow(tr("环与 Track 宽度"), progressThicknessSpin);
    form->addRow(tr("进度环颜色"), ringColorButton);
    form->addRow(tr("Track 颜色"), trackColorButton);
    form->addRow(tr("标题"), titleEdit);
    form->addRow(tr("数值格式"), formatEdit);
    form->addRow(tr("标题字号"), titleFontSizeSpin);
    form->addRow(tr("数值字号"), valueFontSizeSpin);
    form->addRow(tr("文字间距"), textSpacingSpin);
    form->addRow(tr("标题颜色"), titleColorButton);
    form->addRow(tr("数值颜色"), valueColorButton);
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
    connect(titleEdit, &QLineEdit::textChanged, previewRing, &ExProgressRing::setTitle);
    connect(formatEdit, &QLineEdit::textChanged, previewRing, &QProgressBar::setFormat);
    connect(titleFontSizeSpin,
            QOverload<int>::of(&QSpinBox::valueChanged),
            previewRing,
            [previewRing](int size)
            {
                QFont font = previewRing->titleFont();
                if (font == QFont())
                    font = previewRing->font();
                font.setPixelSize(size);
                previewRing->setTitleFont(font);
            });
    connect(valueFontSizeSpin,
            QOverload<int>::of(&QSpinBox::valueChanged),
            previewRing,
            [previewRing](int size)
            {
                QFont font = previewRing->valueFont();
                if (font == QFont())
                    font = previewRing->font();
                font.setPixelSize(size);
                previewRing->setValueFont(font);
            });
    connect(textSpacingSpin,
            QOverload<int>::of(&QSpinBox::valueChanged),
            previewRing,
            &ExProgressRing::setTextSpacing);
    connect(titleColorButton,
            &ExColorPickerButton::selectedColorChanged,
            previewRing,
            &ExProgressRing::setTitleColor);
    connect(valueColorButton,
            &ExColorPickerButton::selectedColorChanged,
            previewRing,
            &ExProgressRing::setValueColor);
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
            [previewRing,
             indeterminateDurationSpin,
             progressThicknessSpin,
             ringColorButton,
             trackColorButton,
             titleEdit,
             formatEdit,
             titleFontSizeSpin,
             valueFontSizeSpin,
             textSpacingSpin,
             titleColorButton,
             valueColorButton,
             refreshRing]
            {
                const QSignalBlocker durationBlocker(indeterminateDurationSpin);
                const QSignalBlocker progressBlocker(progressThicknessSpin);
                const QSignalBlocker ringColorBlocker(ringColorButton);
                const QSignalBlocker trackColorBlocker(trackColorButton);
                const QSignalBlocker titleBlocker(titleEdit);
                const QSignalBlocker formatBlocker(formatEdit);
                const QSignalBlocker titleFontBlocker(titleFontSizeSpin);
                const QSignalBlocker valueFontBlocker(valueFontSizeSpin);
                const QSignalBlocker spacingBlocker(textSpacingSpin);
                const QSignalBlocker titleColorBlocker(titleColorButton);
                const QSignalBlocker valueColorBlocker(valueColorButton);
                indeterminateDurationSpin->setValue(ProgressBarRingDefaultIndeterminateDuration);
                progressThicknessSpin->setValue(ProgressBarRingDefaultThickness);
                previewRing->setProperty(ProgressBarRingIndeterminateDurationProperty,
                                         ProgressBarRingDefaultIndeterminateDuration);
                previewRing->setProperty(ProgressBarThicknessProperty, QVariant());
                previewRing->setPalette(QPalette());
                setRingTrackColor(previewRing, QColor(Qt::gray));
                ringColorButton->setSelectedColor(ringAccentColor(previewRing));
                trackColorButton->setSelectedColor(QColor(Qt::gray));
                titleEdit->setText(ProgressRingShowcaseWidget::tr("已完成"));
                formatEdit->setText(QStringLiteral("%p%"));
                titleFontSizeSpin->setValue(12);
                valueFontSizeSpin->setValue(24);
                textSpacingSpin->setValue(4);
                QFont titleFont = previewRing->font();
                titleFont.setPixelSize(12);
                previewRing->setTitleFont(titleFont);
                QFont valueFont = previewRing->font();
                valueFont.setPixelSize(24);
                valueFont.setWeight(QFont::DemiBold);
                previewRing->setValueFont(valueFont);
                previewRing->setTitle(ProgressRingShowcaseWidget::tr("已完成"));
                previewRing->setFormat(QStringLiteral("%p%"));
                previewRing->setTextSpacing(4);
                previewRing->setTitleColor(QColor());
                previewRing->setValueColor(QColor());
                QColor titleColor = previewRing->palette().color(QPalette::Text);
                titleColor.setAlphaF(titleColor.alphaF() * 0.72);
                titleColorButton->setSelectedColor(titleColor);
                valueColorButton->setSelectedColor(previewRing->palette().color(QPalette::Text));
                refreshRing();
            });

    auto *code = new QPlainTextEdit(propertiesCard);
    code->setReadOnly(true);
    code->setMaximumHeight(128);
    code->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    code->setPlainText(QStringLiteral(
        "auto *ring = new ExProgressRing(parent);\n"
        "ring->setRange(0, 100);\n"
        "ring->setValue(65);\n"
        "ring->setTitle(tr(\"Completed\"));\n"
        "ring->setFormat(QStringLiteral(\"%p%\"));\n"
        "ring->setProperty(ProgressBarThicknessProperty, 8.0);\n"
        "// Optional: ring->setCenterWidget(customWidget);"));
    propertiesLayout->addWidget(code);
    mainLayout->addWidget(propertiesCard);
    mainLayout->addStretch();

    scrollArea->setWidget(content);
}
