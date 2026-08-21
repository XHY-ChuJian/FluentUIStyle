#include "serialcontrolbar.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>

SerialControlBar::SerialControlBar(QWidget *parent)
    : QFrame(parent)
{
    setupUi();

    auto &engine = SerialEngine::instance();
    connect(&engine, &SerialEngine::availablePortsChanged, this, &SerialControlBar::onPortsChanged);
    connect(&engine, &SerialEngine::portOpened, this, &SerialControlBar::onPortOpened);
    connect(&engine, &SerialEngine::portClosed, this, &SerialControlBar::onPortClosed);
    connect(&engine, &SerialEngine::countersUpdated, this, &SerialControlBar::onCountersUpdated);

    refreshPorts();
    updateControlsEnabled(false);
}

void SerialControlBar::setupUi()
{
    setObjectName(QStringLiteral("SerialControlBar"));
    setStyleSheet(QStringLiteral(
        "QFrame#SerialControlBar {"
        "  border: 1px solid rgba(128, 128, 128, 0.22);"
        "  border-radius: 8px;"
        "  background-color: palette(base);"
        "  padding: 6px;"
        "}"
    ));

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(12);

    // 状态指示器
    auto *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(6);
    m_statusDot = new QLabel(this);
    m_statusDot->setFixedSize(10, 10);
    m_statusDot->setStyleSheet(QStringLiteral("border-radius: 5px; background-color: #A0A0A0;"));
    m_statusText = new QLabel(QStringLiteral("未连接"), this);
    QFont stFont = m_statusText->font();
    stFont.setBold(true);
    m_statusText->setFont(stFont);
    statusLayout->addWidget(m_statusDot);
    statusLayout->addWidget(m_statusText);
    mainLayout->addLayout(statusLayout);

    mainLayout->addSpacing(8);

    // 端口选择
    mainLayout->addWidget(new QLabel(QStringLiteral("端口:"), this));
    m_portCombo = new QComboBox(this);
    m_portCombo->setMinimumWidth(160);
    mainLayout->addWidget(m_portCombo);

    m_refreshBtn = new QPushButton(QStringLiteral("🔄"), this);
    m_refreshBtn->setToolTip(QStringLiteral("刷新串口列表"));
    m_refreshBtn->setFixedWidth(32);
    connect(m_refreshBtn, &QPushButton::clicked, this, &SerialControlBar::refreshPorts);
    mainLayout->addWidget(m_refreshBtn);

    // 波特率
    mainLayout->addWidget(new QLabel(QStringLiteral("波特率:"), this));
    m_baudCombo = new QComboBox(this);
    m_baudCombo->setEditable(true);
    m_baudCombo->setMinimumWidth(95);
    const QList<qint32> bauds = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1000000, 2000000};
    for (auto b : bauds) {
        m_baudCombo->addItem(QString::number(b), b);
    }
    m_baudCombo->setCurrentText(QStringLiteral("115200"));
    mainLayout->addWidget(m_baudCombo);

    // 数据位
    mainLayout->addWidget(new QLabel(QStringLiteral("数据位:"), this));
    m_dataBitsCombo = new QComboBox(this);
    m_dataBitsCombo->addItem(QStringLiteral("8"), (int)QSerialPort::Data8);
    m_dataBitsCombo->addItem(QStringLiteral("7"), (int)QSerialPort::Data7);
    m_dataBitsCombo->addItem(QStringLiteral("6"), (int)QSerialPort::Data6);
    m_dataBitsCombo->addItem(QStringLiteral("5"), (int)QSerialPort::Data5);
    m_dataBitsCombo->setCurrentIndex(0);
    mainLayout->addWidget(m_dataBitsCombo);

    // 校验位
    mainLayout->addWidget(new QLabel(QStringLiteral("校验:"), this));
    m_parityCombo = new QComboBox(this);
    m_parityCombo->addItem(QStringLiteral("None"), (int)QSerialPort::NoParity);
    m_parityCombo->addItem(QStringLiteral("Even"), (int)QSerialPort::EvenParity);
    m_parityCombo->addItem(QStringLiteral("Odd"), (int)QSerialPort::OddParity);
    m_parityCombo->addItem(QStringLiteral("Mark"), (int)QSerialPort::MarkParity);
    m_parityCombo->addItem(QStringLiteral("Space"), (int)QSerialPort::SpaceParity);
    mainLayout->addWidget(m_parityCombo);

    // 停止位
    mainLayout->addWidget(new QLabel(QStringLiteral("停止位:"), this));
    m_stopBitsCombo = new QComboBox(this);
    m_stopBitsCombo->addItem(QStringLiteral("1"), (int)QSerialPort::OneStop);
    m_stopBitsCombo->addItem(QStringLiteral("1.5"), (int)QSerialPort::OneAndHalfStop);
    m_stopBitsCombo->addItem(QStringLiteral("2"), (int)QSerialPort::TwoStop);
    mainLayout->addWidget(m_stopBitsCombo);

    // 打开/关闭按钮
    m_openBtn = new QPushButton(QStringLiteral("打开串口"), this);
    m_openBtn->setMinimumWidth(100);
    m_openBtn->setProperty("accent", true);
    connect(m_openBtn, &QPushButton::clicked, this, &SerialControlBar::onTogglePortClicked);
    mainLayout->addWidget(m_openBtn);

    mainLayout->addStretch();

    // 计数与复位
    m_rxCountLabel = new QLabel(QStringLiteral("Rx: 0 B"), this);
    m_txCountLabel = new QLabel(QStringLiteral("Tx: 0 B"), this);
    m_resetCountersBtn = new QPushButton(QStringLiteral("复位"), this);
    m_resetCountersBtn->setToolTip(QStringLiteral("清零收发计数"));
    connect(m_resetCountersBtn, &QPushButton::clicked, this, &SerialControlBar::onResetCountersClicked);

    mainLayout->addWidget(m_rxCountLabel);
    mainLayout->addWidget(m_txCountLabel);
    mainLayout->addWidget(m_resetCountersBtn);
}

void SerialControlBar::refreshPorts()
{
    const QString currentSelected = m_portCombo->currentData().toString();
    m_portCombo->clear();

    const auto ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty()) {
        m_portCombo->addItem(QStringLiteral("未发现串口设备"), QString());
    } else {
        int selectIndex = 0;
        for (int i = 0; i < ports.size(); ++i) {
            const auto &p = ports[i];
            QString desc = p.description().isEmpty() ? p.portName() : QStringLiteral("%1 (%2)").arg(p.portName(), p.description());
            m_portCombo->addItem(desc, p.portName());
            if (p.portName() == currentSelected) {
                selectIndex = i;
            }
        }
        m_portCombo->setCurrentIndex(selectIndex);
    }
}

void SerialControlBar::onPortsChanged(const QList<QSerialPortInfo> &ports)
{
    Q_UNUSED(ports);
    if (!SerialEngine::instance().isOpen()) {
        refreshPorts();
    }
}

void SerialControlBar::onTogglePortClicked()
{
    auto &engine = SerialEngine::instance();
    if (engine.isOpen()) {
        engine.closePort();
    } else {
        QString portName = m_portCombo->currentData().toString();
        if (portName.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择有效的串口设备！"));
            return;
        }

        SerialPortConfig cfg;
        cfg.portName = portName;
        cfg.baudRate = m_baudCombo->currentText().toInt();
        cfg.dataBits = (QSerialPort::DataBits)m_dataBitsCombo->currentData().toInt();
        cfg.parity = (QSerialPort::Parity)m_parityCombo->currentData().toInt();
        cfg.stopBits = (QSerialPort::StopBits)m_stopBitsCombo->currentData().toInt();
        cfg.flowControl = QSerialPort::NoFlowControl;

        if (!engine.openPort(cfg)) {
            QMessageBox::critical(this, QStringLiteral("打开失败"), QStringLiteral("无法打开串口 %1，请检查端口是否被占用！").arg(portName));
        }
    }
}

void SerialControlBar::onResetCountersClicked()
{
    SerialEngine::instance().resetCounters();
}

void SerialControlBar::onPortOpened(const QString &portName, qint32 baud)
{
    m_statusDot->setStyleSheet(QStringLiteral("border-radius: 5px; background-color: #107C41;"));
    m_statusText->setText(QStringLiteral("已连接 %1 @ %2").arg(portName).arg(baud));
    m_openBtn->setText(QStringLiteral("关闭串口"));
    updateControlsEnabled(true);
}

void SerialControlBar::onPortClosed()
{
    m_statusDot->setStyleSheet(QStringLiteral("border-radius: 5px; background-color: #A0A0A0;"));
    m_statusText->setText(QStringLiteral("未连接"));
    m_openBtn->setText(QStringLiteral("打开串口"));
    updateControlsEnabled(false);
}

void SerialControlBar::onCountersUpdated(qint64 rx, qint64 tx, qint64 rxFrames, qint64 txFrames)
{
    Q_UNUSED(rxFrames);
    Q_UNUSED(txFrames);

    auto formatBytes = [](qint64 bytes) -> QString {
        if (bytes < 1024) return QStringLiteral("%1 B").arg(bytes);
        if (bytes < 1024 * 1024) return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
        return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
    };

    m_rxCountLabel->setText(QStringLiteral("Rx: %1").arg(formatBytes(rx)));
    m_txCountLabel->setText(QStringLiteral("Tx: %1").arg(formatBytes(tx)));
}

void SerialControlBar::updateControlsEnabled(bool opened)
{
    m_portCombo->setEnabled(!opened);
    m_refreshBtn->setEnabled(!opened);
    m_baudCombo->setEnabled(!opened);
    m_dataBitsCombo->setEnabled(!opened);
    m_parityCombo->setEnabled(!opened);
    m_stopBitsCombo->setEnabled(!opened);
}
