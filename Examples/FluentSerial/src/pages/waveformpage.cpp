#include "waveformpage.h"
#include "core/serialengine.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QtMath>

namespace {

QFrame *createCardFrame(QWidget *parent = nullptr)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("MonitorCard"));
    frame->setStyleSheet(QStringLiteral(
        "QFrame#MonitorCard {"
        "  border: 1px solid rgba(128, 128, 128, 0.22);"
        "  border-radius: 8px;"
        "  background-color: palette(base);"
        "}"
    ));
    return frame;
}

QLabel *createCardHeader(const QString &title, QWidget *parent = nullptr)
{
    auto *label = new QLabel(title, parent);
    QFont f = label->font();
    f.setPixelSize(15);
    f.setBold(true);
    label->setFont(f);
    return label;
}

} // namespace

WaveformPage::WaveformPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    connect(&SerialEngine::instance(), &SerialEngine::dataReceived, this, &WaveformPage::onDataReceived);

    m_simTimer = new QTimer(this);
    m_simTimer->setInterval(30);
    connect(m_simTimer, &QTimer::timeout, this, &WaveformPage::onSimulateTimer);
}

void WaveformPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(12);

    // 1. 主波形视窗卡片
    auto *plotCard = createCardFrame(this);
    auto *plotLayout = new QVBoxLayout(plotCard);
    plotLayout->setContentsMargins(12, 12, 12, 12);
    plotLayout->setSpacing(8);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(createCardHeader(QStringLiteral("📈 实时多通道波形示波器"), plotCard));
    topLayout->addStretch();

    topLayout->addWidget(new QLabel(QStringLiteral("采样深度:"), plotCard));
    m_pointsCombo = new QComboBox(plotCard);
    m_pointsCombo->addItem(QStringLiteral("100 点"), 100);
    m_pointsCombo->addItem(QStringLiteral("300 点"), 300);
    m_pointsCombo->addItem(QStringLiteral("500 点"), 500);
    m_pointsCombo->addItem(QStringLiteral("1000 点"), 1000);
    m_pointsCombo->setCurrentIndex(1);
    connect(m_pointsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        m_waveformWidget->setMaxPoints(m_pointsCombo->itemData(idx).toInt());
    });
    topLayout->addWidget(m_pointsCombo);

    m_pauseBtn = new QPushButton(QStringLiteral("⏸️ 暂停"), plotCard);
    connect(m_pauseBtn, &QPushButton::clicked, this, &WaveformPage::onTogglePauseClicked);
    topLayout->addWidget(m_pauseBtn);

    m_clearBtn = new QPushButton(QStringLiteral("🗑️ 清屏"), plotCard);
    connect(m_clearBtn, &QPushButton::clicked, this, &WaveformPage::onClearDataClicked);
    topLayout->addWidget(m_clearBtn);

    m_simulateBtn = new QPushButton(QStringLiteral("✨ 模拟正弦波演示"), plotCard);
    connect(m_simulateBtn, &QPushButton::clicked, this, &WaveformPage::onToggleSimulateClicked);
    topLayout->addWidget(m_simulateBtn);

    plotLayout->addLayout(topLayout);

    m_waveformWidget = new WaveformWidget(plotCard);
    plotLayout->addWidget(m_waveformWidget, 1);

    rootLayout->addWidget(plotCard, 1);

    // 2. 通道控制面板卡片
    auto *chCard = createCardFrame(this);
    auto *chLayout = new QVBoxLayout(chCard);
    chLayout->setContentsMargins(12, 10, 12, 10);
    chLayout->setSpacing(8);

    chLayout->addWidget(createCardHeader(QStringLiteral("🎛️ 通道参数与过滤设置 (支持接收格式: 'val1, val2, val3' 或 'ch1:10 ch2:20')"), chCard));

    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(6);

    const auto &channels = m_waveformWidget->channels();
    for (int i = 0; i < channels.size(); ++i) {
        int row = i / 4;
        int col = (i % 4) * 3;

        auto *chk = new QCheckBox(QStringLiteral("CH %1").arg(i + 1), chCard);
        chk->setChecked(channels[i].visible);
        chk->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(channels[i].color.name()));

        connect(chk, &QCheckBox::toggled, this, [this, i](bool checked) {
            m_waveformWidget->setChannelVisible(i, checked);
        });

        auto *nameEdit = new QLineEdit(channels[i].name, chCard);
        nameEdit->setMaximumWidth(100);
        connect(nameEdit, &QLineEdit::textChanged, this, [this, i](const QString &text) {
            m_waveformWidget->setChannelName(i, text);
        });

        grid->addWidget(chk, row, col);
        grid->addWidget(nameEdit, row, col + 1);

        m_channelChecks.append(chk);
        m_channelNames.append(nameEdit);
    }

    chLayout->addLayout(grid);
    rootLayout->addWidget(chCard);
}

void WaveformPage::onDataReceived(const QByteArray &data, const QDateTime &timestamp)
{
    Q_UNUSED(timestamp);
    if (m_waveformWidget->isPaused()) {
        return;
    }

    QString text = QString::fromUtf8(data);
    parseIncomingText(text);
}

void WaveformPage::parseIncomingText(const QString &text)
{
    // 支持按换行拆分多帧
    QStringList lines = text.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        // 匹配所有浮点数或整数，如 -12.34, 56, 0.78
        static QRegularExpression numRegex(QStringLiteral("[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?"));
        auto matchIterator = numRegex.globalMatch(line);

        QVector<double> values;
        while (matchIterator.hasNext()) {
            auto match = matchIterator.next();
            bool ok = false;
            double val = match.captured(0).toDouble(&ok);
            if (ok) {
                values.append(val);
            }
        }

        if (!values.isEmpty()) {
            m_waveformWidget->addDataSample(values);
        }
    }
}

void WaveformPage::onTogglePauseClicked()
{
    bool paused = !m_waveformWidget->isPaused();
    m_waveformWidget->setPaused(paused);
    m_pauseBtn->setText(paused ? QStringLiteral("▶️ 恢复") : QStringLiteral("⏸️ 暂停"));
}

void WaveformPage::onClearDataClicked()
{
    m_waveformWidget->clearData();
}

void WaveformPage::onToggleSimulateClicked()
{
    if (m_simTimer->isActive()) {
        m_simTimer->stop();
        m_simulateBtn->setText(QStringLiteral("✨ 模拟正弦波演示"));
    } else {
        m_simTimer->start();
        m_simulateBtn->setText(QStringLiteral("⏹️ 停止模拟"));
    }
}

void WaveformPage::onSimulateTimer()
{
    m_simAngle += 0.08;
    if (m_simAngle > 2 * M_PI) {
        m_simAngle -= 2 * M_PI;
    }

    double ch1 = 50.0 * qSin(m_simAngle);
    double ch2 = 30.0 * qCos(m_simAngle * 1.5) + 10.0;
    double ch3 = 20.0 * qSin(m_simAngle * 3.0) - 15.0;

    QVector<double> sample = {ch1, ch2, ch3};
    m_waveformWidget->addDataSample(sample);
}

void WaveformPage::onPointsChanged(int points)
{
    m_waveformWidget->setMaxPoints(points);
}
