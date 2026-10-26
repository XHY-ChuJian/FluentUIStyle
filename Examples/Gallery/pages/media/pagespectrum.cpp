#include "pagespectrum.h"

#include "exspectrumwidget.h"
#include "sinewavegenerator.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>

PageSpectrum::PageSpectrum(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);
    setObjectName(QStringLiteral("pageExSpectrum"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *hint = new QLabel(
        tr("ExSpectrumWidget 演示：模拟 PCM 驱动（各频带 LFO + 节拍脉冲，支持实时参数调节）。"),
        this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    m_spectrum = new ExSpectrumWidget(this);
    m_spectrum->setSampleRate(44100);
    m_spectrum->setMinimumHeight(160);
    layout->addWidget(m_spectrum, 1);

    // --- 频谱参数动态控制卡片 ---
    auto *controlCard = new QFrame(this);
    controlCard->setProperty("isCard", true);
    auto *cardLayout = new QGridLayout(controlCard);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setHorizontalSpacing(16);
    cardLayout->setVerticalSpacing(12);

    // 1. 倒影与峰值帽开关
    auto *chkReflection = new QCheckBox(tr("显示水波倒影 (reflection)"), controlCard);
    chkReflection->setChecked(m_spectrum->isReflectionEnabled());
    connect(chkReflection, &QCheckBox::toggled, m_spectrum, &ExSpectrumWidget::setReflectionEnabled);
    cardLayout->addWidget(chkReflection, 0, 0);

    auto *chkPeakCap = new QCheckBox(tr("显示浮动峰值帽 (peakCap)"), controlCard);
    chkPeakCap->setChecked(m_spectrum->isPeakCapEnabled());
    connect(chkPeakCap, &QCheckBox::toggled, m_spectrum, &ExSpectrumWidget::setPeakCapEnabled);
    cardLayout->addWidget(chkPeakCap, 0, 1, 1, 2);

    // 2. 倒影透明度 (reflectionOpacity: 0 ~ 100%)
    auto *lblOpacityTitle = new QLabel(tr("倒影透明度 (Opacity):"), controlCard);
    auto *sliderOpacity = new QSlider(Qt::Horizontal, controlCard);
    sliderOpacity->setRange(0, 100);
    sliderOpacity->setValue(int(m_spectrum->reflectionOpacity() * 100.0f));
    auto *lblOpacityVal = new QLabel(QStringLiteral("%1%").arg(sliderOpacity->value()), controlCard);
    lblOpacityVal->setMinimumWidth(50);
    connect(sliderOpacity, &QSlider::valueChanged, this, [this, lblOpacityVal](int val) {
        m_spectrum->setReflectionOpacity(val / 100.0f);
        lblOpacityVal->setText(QStringLiteral("%1%").arg(val));
    });
    connect(chkReflection, &QCheckBox::toggled, sliderOpacity, &QSlider::setEnabled);
    connect(chkReflection, &QCheckBox::toggled, lblOpacityTitle, &QLabel::setEnabled);
    connect(chkReflection, &QCheckBox::toggled, lblOpacityVal, &QLabel::setEnabled);
    cardLayout->addWidget(lblOpacityTitle, 1, 0);
    cardLayout->addWidget(sliderOpacity, 1, 1);
    cardLayout->addWidget(lblOpacityVal, 1, 2);

    // 3. 柱子数量 (barCount: 16 ~ 128)
    auto *lblBarsTitle = new QLabel(tr("柱子数量 (barCount):"), controlCard);
    auto *sliderBars = new QSlider(Qt::Horizontal, controlCard);
    sliderBars->setRange(16, 128);
    sliderBars->setSingleStep(8);
    sliderBars->setValue(m_spectrum->barCount());
    auto *lblBarsVal = new QLabel(QString(tr("%1 根")).arg(sliderBars->value()), controlCard);
    lblBarsVal->setMinimumWidth(50);
    connect(sliderBars, &QSlider::valueChanged, this, [this, lblBarsVal](int val) {
        m_spectrum->setBarCount(val);
        lblBarsVal->setText(QString(tr("%1 根")).arg(val));
    });
    cardLayout->addWidget(lblBarsTitle, 2, 0);
    cardLayout->addWidget(sliderBars, 2, 1);
    cardLayout->addWidget(lblBarsVal, 2, 2);

    // 4. 柱子间距 (barGap: 0 ~ 8px)
    auto *lblGapTitle = new QLabel(tr("柱子间距 (barGap):"), controlCard);
    auto *sliderGap = new QSlider(Qt::Horizontal, controlCard);
    sliderGap->setRange(0, 8);
    sliderGap->setValue(m_spectrum->barGap());
    auto *lblGapVal = new QLabel(QStringLiteral("%1 px").arg(sliderGap->value()), controlCard);
    lblGapVal->setMinimumWidth(50);
    connect(sliderGap, &QSlider::valueChanged, this, [this, lblGapVal](int val) {
        m_spectrum->setBarGap(val);
        lblGapVal->setText(QStringLiteral("%1 px").arg(val));
    });
    cardLayout->addWidget(lblGapTitle, 3, 0);
    cardLayout->addWidget(sliderGap, 3, 1);
    cardLayout->addWidget(lblGapVal, 3, 2);

    // 5. 柱顶圆角 (barRadius: 0 ~ 10px)
    auto *lblRadiusTitle = new QLabel(tr("柱顶圆角 (barRadius):"), controlCard);
    auto *sliderRadius = new QSlider(Qt::Horizontal, controlCard);
    sliderRadius->setRange(0, 10);
    sliderRadius->setValue(m_spectrum->barRadius());
    auto *lblRadiusVal = new QLabel(QStringLiteral("%1 px").arg(sliderRadius->value()), controlCard);
    lblRadiusVal->setMinimumWidth(50);
    connect(sliderRadius, &QSlider::valueChanged, this, [this, lblRadiusVal](int val) {
        m_spectrum->setBarRadius(val);
        lblRadiusVal->setText(QStringLiteral("%1 px").arg(val));
    });
    cardLayout->addWidget(lblRadiusTitle, 4, 0);
    cardLayout->addWidget(sliderRadius, 4, 1);
    cardLayout->addWidget(lblRadiusVal, 4, 2);

    // 6. 最高分析频率 (maxFrequency: 8000 ~ 20000Hz)
    auto *lblFreqTitle = new QLabel(tr("最高分析频率 (maxFreq):"), controlCard);
    auto *sliderFreq = new QSlider(Qt::Horizontal, controlCard);
    sliderFreq->setRange(8, 20);
    sliderFreq->setValue(int(m_spectrum->maxFrequency() / 1000.0f));
    auto *lblFreqVal = new QLabel(QStringLiteral("%1 kHz").arg(sliderFreq->value()), controlCard);
    lblFreqVal->setMinimumWidth(50);
    connect(sliderFreq, &QSlider::valueChanged, this, [this, lblFreqVal](int val) {
        m_spectrum->setMaxFrequency(float(val * 1000));
        lblFreqVal->setText(QStringLiteral("%1 kHz").arg(val));
    });
    cardLayout->addWidget(lblFreqTitle, 5, 0);
    cardLayout->addWidget(sliderFreq, 5, 1);
    cardLayout->addWidget(lblFreqVal, 5, 2);

    // 7. 波形颜色 (Accent)
    auto *lblColorTitle = new QLabel(tr("波形主题色 (Accent):"), controlCard);
    auto *comboColor = new QComboBox(controlCard);
    comboColor->addItem(tr("五颜六色 (从左至右)"), QColor());
    comboColor->addItem(tr("微软蓝 (Fluent Blue)"), QColor(QStringLiteral("#0078D4")));
    comboColor->addItem(tr("青空翠 (Teal Cyan)"), QColor(QStringLiteral("#00B7C3")));
    comboColor->addItem(tr("薄荷绿 (Mint Green)"), QColor(QStringLiteral("#107C41")));
    comboColor->addItem(tr("极光紫 (Aurora Purple)"), QColor(QStringLiteral("#8764B8")));
    comboColor->addItem(tr("火焰红 (Crimson Red)"), QColor(QStringLiteral("#E81123")));
    comboColor->addItem(tr("活力橙 (Vibrant Orange)"), QColor(QStringLiteral("#F7630C")));
    comboColor->addItem(tr("绚丽粉 (Hot Pink)"), QColor(QStringLiteral("#E3008C")));
    comboColor->addItem(tr("暖金黄 (Amber Gold)"), QColor(QStringLiteral("#FFB900")));
    connect(comboColor, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, comboColor](int idx) {
        const QColor c = comboColor->itemData(idx).value<QColor>();
        m_spectrum->setRainbowEnabled(!c.isValid());
        m_spectrum->setAccentColor(c);
    });
    cardLayout->addWidget(lblColorTitle, 6, 0);
    cardLayout->addWidget(comboColor, 6, 1, 1, 2);

    layout->addWidget(controlCard);

    m_generator = new SineWaveGenerator(44100);
    m_generator->setAmplitude(0.55f);
    m_generator->setAnimated(true);

    m_feedTimer = new QTimer(this);
    m_feedTimer->setTimerType(Qt::PreciseTimer);
    connect(m_feedTimer, &QTimer::timeout, this, &PageSpectrum::feedSimulatedAudio);
    m_feedTimer->start(ExSpectrumWidget::DefaultRefreshIntervalMs);
}

PageSpectrum::~PageSpectrum() = default;

void PageSpectrum::feedSimulatedAudio()
{
    if (!m_spectrum || !m_generator)
    {
        return;
    }

    const int sampleCount = qMax(
        256,
        44100 * ExSpectrumWidget::DefaultRefreshIntervalMs / 1000);
    m_spectrum->pushAudioData(m_generator->generate(sampleCount));
}
