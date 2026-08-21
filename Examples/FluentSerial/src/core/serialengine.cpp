#include "serialengine.h"
#include "crcutils.h"

#include <QDebug>

SerialEngine &SerialEngine::instance()
{
    static SerialEngine s_instance;
    return s_instance;
}

SerialEngine::SerialEngine(QObject *parent)
    : QObject(parent)
{
    m_serialPort = new QSerialPort(this);

    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialEngine::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
            emit portErrorOccurred(m_serialPort->errorString());
            if (!m_serialPort->isOpen()) {
                emit portClosed();
            }
        }
    });

    // 节流刷新定时器：每 25ms 刷新一次接收缓冲区，合并 UI 渲染
    m_flushTimer = new QTimer(this);
    m_flushTimer->setInterval(25);
    m_flushTimer->setSingleShot(false);
    connect(m_flushTimer, &QTimer::timeout, this, &SerialEngine::flushRxBuffer);

    // 端口热插拔监听定时器
    m_scanTimer = new QTimer(this);
    m_scanTimer->setInterval(1200);
    connect(m_scanTimer, &QTimer::timeout, this, &SerialEngine::onPortScanTimer);
    m_scanTimer->start();
    onPortScanTimer();

    // 自动化序列调度定时器
    m_sequenceTimer = new QTimer(this);
    m_sequenceTimer->setSingleShot(true);
    connect(m_sequenceTimer, &QTimer::timeout, this, &SerialEngine::onSequenceStep);
}

SerialEngine::~SerialEngine()
{
    closePort();
}

bool SerialEngine::openPort(const SerialPortConfig &config)
{
    if (m_serialPort->isOpen()) {
        closePort();
    }

    m_config = config;
    m_serialPort->setPortName(config.portName);
    m_serialPort->setBaudRate(config.baudRate);
    m_serialPort->setDataBits(config.dataBits);
    m_serialPort->setParity(config.parity);
    m_serialPort->setStopBits(config.stopBits);
    m_serialPort->setFlowControl(config.flowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_flushTimer->start();
        emit portOpened(config.portName, config.baudRate);
        return true;
    }

    emit portErrorOccurred(m_serialPort->errorString());
    return false;
}

void SerialEngine::closePort()
{
    stopSequence();
    if (m_flushTimer->isActive()) {
        m_flushTimer->stop();
    }
    flushRxBuffer();

    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit portClosed();
    }
}

bool SerialEngine::isOpen() const
{
    return m_serialPort && m_serialPort->isOpen();
}

SerialPortConfig SerialEngine::currentConfig() const
{
    return m_config;
}

QString SerialEngine::currentPortName() const
{
    return m_serialPort->portName();
}

bool SerialEngine::sendData(const QByteArray &data)
{
    if (!isOpen() || data.isEmpty()) {
        return false;
    }

    qint64 written = m_serialPort->write(data);
    if (written > 0) {
        m_txBytes += written;
        m_txFrames += 1;
        emit dataSent(data, QDateTime::currentDateTime());
        emit countersUpdated(m_rxBytes, m_txBytes, m_rxFrames, m_txFrames);
        return true;
    }
    return false;
}

bool SerialEngine::sendString(const QString &text, bool isHex, const QString &suffix)
{
    QByteArray payload;
    if (isHex) {
        payload = CrcUtils::hexStringToByteArray(text);
    } else {
        payload = text.toUtf8();
    }

    if (!suffix.isEmpty()) {
        if (suffix == QStringLiteral("\\r\\n")) {
            payload.append("\r\n");
        } else if (suffix == QStringLiteral("\\n")) {
            payload.append("\n");
        } else if (suffix == QStringLiteral("\\r")) {
            payload.append("\r");
        } else {
            payload.append(suffix.toUtf8());
        }
    }

    return sendData(payload);
}

void SerialEngine::resetCounters()
{
    m_rxBytes = 0;
    m_txBytes = 0;
    m_rxFrames = 0;
    m_txFrames = 0;
    emit countersUpdated(m_rxBytes, m_txBytes, m_rxFrames, m_txFrames);
}

void SerialEngine::onReadyRead()
{
    QByteArray chunk = m_serialPort->readAll();
    if (!chunk.isEmpty()) {
        m_rxBuffer.append(chunk);
        m_rxBytes += chunk.size();
    }
}

void SerialEngine::flushRxBuffer()
{
    if (m_rxBuffer.isEmpty()) {
        return;
    }

    QByteArray dataToSend = m_rxBuffer;
    m_rxBuffer.clear();

    m_rxFrames += 1;
    emit dataReceived(dataToSend, QDateTime::currentDateTime());
    emit countersUpdated(m_rxBytes, m_txBytes, m_rxFrames, m_txFrames);
}

void SerialEngine::onPortScanTimer()
{
    const auto currentList = QSerialPortInfo::availablePorts();
    bool changed = false;

    if (currentList.size() != m_cachedPorts.size()) {
        changed = true;
    } else {
        for (int i = 0; i < currentList.size(); ++i) {
            if (currentList[i].portName() != m_cachedPorts[i].portName()) {
                changed = true;
                break;
            }
        }
    }

    if (changed) {
        m_cachedPorts = currentList;
        emit availablePortsChanged(m_cachedPorts);
    }
}

void SerialEngine::startSequence(const QList<SerialCommandItem> &commands, bool loop)
{
    if (commands.isEmpty() || !isOpen()) {
        return;
    }

    m_sequenceCommands = commands;
    m_sequenceLoop = loop;
    m_sequenceIndex = 0;
    m_sequenceRunning = true;

    onSequenceStep();
}

void SerialEngine::stopSequence()
{
    m_sequenceRunning = false;
    if (m_sequenceTimer->isActive()) {
        m_sequenceTimer->stop();
    }
    emit sequenceFinished();
}

void SerialEngine::onSequenceStep()
{
    if (!m_sequenceRunning || !isOpen() || m_sequenceCommands.isEmpty()) {
        stopSequence();
        return;
    }

    if (m_sequenceIndex >= m_sequenceCommands.size()) {
        if (m_sequenceLoop) {
            m_sequenceIndex = 0;
        } else {
            stopSequence();
            return;
        }
    }

    const auto &cmd = m_sequenceCommands[m_sequenceIndex];
    if (cmd.isLoopChecked) {
        sendString(cmd.payload, cmd.isHex);
        emit sequenceProgress(m_sequenceIndex + 1, m_sequenceCommands.size(), cmd.name.isEmpty() ? cmd.payload : cmd.name);
    }

    int delay = qMax(10, cmd.delayMs);
    m_sequenceIndex++;
    m_sequenceTimer->start(delay);
}
