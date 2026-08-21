#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

struct SerialPortConfig {
    QString portName;
    qint32 baudRate{115200};
    QSerialPort::DataBits dataBits{QSerialPort::Data8};
    QSerialPort::Parity parity{QSerialPort::NoParity};
    QSerialPort::StopBits stopBits{QSerialPort::OneStop};
    QSerialPort::FlowControl flowControl{QSerialPort::NoFlowControl};
};

struct SerialCommandItem {
    QString id;
    QString name;
    QString payload;
    bool isHex{false};
    int delayMs{100};
    bool isLoopChecked{true};
};

class SerialEngine : public QObject
{
    Q_OBJECT

public:
    static SerialEngine &instance();

    bool openPort(const SerialPortConfig &config);
    void closePort();
    bool isOpen() const;

    SerialPortConfig currentConfig() const;
    QString currentPortName() const;

    // 数据发送
    bool sendData(const QByteArray &data);
    bool sendString(const QString &text, bool isHex = false, const QString &suffix = QString());

    // 统计计数
    qint64 rxBytes() const { return m_rxBytes; }
    qint64 txBytes() const { return m_txBytes; }
    qint64 rxFrames() const { return m_rxFrames; }
    qint64 txFrames() const { return m_txFrames; }
    void resetCounters();

    // 自动化指令序列
    void startSequence(const QList<SerialCommandItem> &commands, bool loop = false);
    void stopSequence();
    bool isSequenceRunning() const { return m_sequenceRunning; }

    QList<QSerialPortInfo> availablePorts() const { return m_cachedPorts; }

signals:
    void portOpened(const QString &portName, qint32 baudRate);
    void portClosed();
    void portErrorOccurred(const QString &errorString);
    void availablePortsChanged(const QList<QSerialPortInfo> &ports);

    void dataReceived(const QByteArray &data, const QDateTime &timestamp);
    void dataSent(const QByteArray &data, const QDateTime &timestamp);
    void countersUpdated(qint64 rxBytes, qint64 txBytes, qint64 rxFrames, qint64 txFrames);

    void sequenceProgress(int currentIndex, int totalCount, const QString &cmdName);
    void sequenceFinished();

private slots:
    void onReadyRead();
    void onPortScanTimer();
    void flushRxBuffer();
    void onSequenceStep();

private:
    explicit SerialEngine(QObject *parent = nullptr);
    ~SerialEngine() override;

    QSerialPort *m_serialPort{nullptr};
    SerialPortConfig m_config;

    QTimer *m_scanTimer{nullptr};
    QList<QSerialPortInfo> m_cachedPorts;

    // 接收防卡顿节流缓冲区
    QTimer *m_flushTimer{nullptr};
    QByteArray m_rxBuffer;

    // 计数
    qint64 m_rxBytes{0};
    qint64 m_txBytes{0};
    qint64 m_rxFrames{0};
    qint64 m_txFrames{0};

    // 自动化序列
    QTimer *m_sequenceTimer{nullptr};
    QList<SerialCommandItem> m_sequenceCommands;
    int m_sequenceIndex{0};
    bool m_sequenceLoop{false};
    bool m_sequenceRunning{false};
};
