#include "monitorpage.h"
#include "core/crcutils.h"
#include "core/serialengine.h"

#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollBar>
#include <QVBoxLayout>

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

MonitorPage::MonitorPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    auto &engine = SerialEngine::instance();
    connect(&engine, &SerialEngine::dataReceived, this, &MonitorPage::onDataReceived);
    connect(&engine, &SerialEngine::dataSent, this, &MonitorPage::onDataSent);

    m_autoSendTimer = new QTimer(this);
    connect(m_autoSendTimer, &QTimer::timeout, this, &MonitorPage::onSendClicked);
}

void MonitorPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(12);

    // 1. 接收区域卡片
    auto *rxCard = createCardFrame(this);
    auto *rxLayout = new QVBoxLayout(rxCard);
    rxLayout->setContentsMargins(12, 12, 12, 12);
    rxLayout->setSpacing(8);

    auto *rxTopLayout = new QHBoxLayout();
    rxTopLayout->addWidget(createCardHeader(QStringLiteral("📥 接收数据区"), rxCard));
    rxTopLayout->addStretch();

    m_rxHexCheck = new QCheckBox(QStringLiteral("HEX 显示"), rxCard);
    m_timestampCheck = new QCheckBox(QStringLiteral("时间戳"), rxCard);
    m_timestampCheck->setChecked(true);
    m_autoScrollCheck = new QCheckBox(QStringLiteral("自动滚屏"), rxCard);
    m_autoScrollCheck->setChecked(true);

    m_clearRxBtn = new QPushButton(QStringLiteral("清空接收"), rxCard);
    connect(m_clearRxBtn, &QPushButton::clicked, this, &MonitorPage::onClearRxClicked);

    m_exportLogBtn = new QPushButton(QStringLiteral("导出日志..."), rxCard);
    connect(m_exportLogBtn, &QPushButton::clicked, this, &MonitorPage::onExportLogClicked);

    rxTopLayout->addWidget(m_rxHexCheck);
    rxTopLayout->addWidget(m_timestampCheck);
    rxTopLayout->addWidget(m_autoScrollCheck);
    rxTopLayout->addWidget(m_clearRxBtn);
    rxTopLayout->addWidget(m_exportLogBtn);
    rxLayout->addLayout(rxTopLayout);

    m_rxTextEdit = new QPlainTextEdit(rxCard);
    m_rxTextEdit->setReadOnly(true);
    m_rxTextEdit->setMaximumBlockCount(2000);
    QFont monoFont(QStringLiteral("Consolas, Courier New"));
    monoFont.setPixelSize(13);
    m_rxTextEdit->setFont(monoFont);
    rxLayout->addWidget(m_rxTextEdit, 1);

    rootLayout->addWidget(rxCard, 3);

    // 2. 发送区域卡片
    auto *txCard = createCardFrame(this);
    auto *txLayout = new QVBoxLayout(txCard);
    txLayout->setContentsMargins(12, 12, 12, 12);
    txLayout->setSpacing(8);

    auto *txTopLayout = new QHBoxLayout();
    txTopLayout->addWidget(createCardHeader(QStringLiteral("📤 发送数据区"), txCard));
    txTopLayout->addStretch();

    m_txHexCheck = new QCheckBox(QStringLiteral("HEX 发送"), txCard);
    txTopLayout->addWidget(m_txHexCheck);

    txTopLayout->addWidget(new QLabel(QStringLiteral("换行符:"), txCard));
    m_suffixCombo = new QComboBox(txCard);
    m_suffixCombo->addItem(QStringLiteral("无"), QString());
    m_suffixCombo->addItem(QStringLiteral("\\r\\n (CRLF)"), QStringLiteral("\\r\\n"));
    m_suffixCombo->addItem(QStringLiteral("\\n (LF)"), QStringLiteral("\\n"));
    m_suffixCombo->addItem(QStringLiteral("\\r (CR)"), QStringLiteral("\\r"));
    m_suffixCombo->setCurrentIndex(1);
    txTopLayout->addWidget(m_suffixCombo);

    txTopLayout->addSpacing(8);

    m_crcTypeCombo = new QComboBox(txCard);
    m_crcTypeCombo->addItem(QStringLiteral("CRC16 Modbus"), 0);
    m_crcTypeCombo->addItem(QStringLiteral("CRC16 CCITT"), 1);
    m_crcTypeCombo->addItem(QStringLiteral("CRC32"), 2);
    m_crcTypeCombo->addItem(QStringLiteral("Sum8 累加和"), 3);
    m_crcTypeCombo->addItem(QStringLiteral("XOR 异或校验"), 4);
    txTopLayout->addWidget(m_crcTypeCombo);

    m_appendCrcBtn = new QPushButton(QStringLiteral("追加校验码"), txCard);
    connect(m_appendCrcBtn, &QPushButton::clicked, this, &MonitorPage::onAppendCrcClicked);
    txTopLayout->addWidget(m_appendCrcBtn);

    txLayout->addLayout(txTopLayout);

    // 中部输入框
    m_txTextEdit = new QPlainTextEdit(txCard);
    m_txTextEdit->setMaximumHeight(100);
    m_txTextEdit->setFont(monoFont);
    m_txTextEdit->setPlaceholderText(QStringLiteral("请输入要发送的指令或 HEX 数据（支持空格分隔，如: 01 03 00 00 00 02）..."));
    txLayout->addWidget(m_txTextEdit);

    // 底部发送工具条
    auto *txBottomLayout = new QHBoxLayout();
    m_autoSendCheck = new QCheckBox(QStringLiteral("定时发送"), txCard);
    connect(m_autoSendCheck, &QCheckBox::toggled, this, &MonitorPage::onAutoSendToggled);

    m_intervalSpin = new QSpinBox(txCard);
    m_intervalSpin->setRange(10, 60000);
    m_intervalSpin->setValue(1000);
    m_intervalSpin->setSuffix(QStringLiteral(" ms"));
    m_intervalSpin->setFixedHeight(34);

    m_clearTxBtn = new QPushButton(QStringLiteral("清空输入"), txCard);
    m_clearTxBtn->setFixedHeight(34);
    connect(m_clearTxBtn, &QPushButton::clicked, m_txTextEdit, &QPlainTextEdit::clear);

    m_sendBtn = new QPushButton(QStringLiteral("发 送 (Ctrl+Enter)"), txCard);
    m_sendBtn->setMinimumWidth(130);
    m_sendBtn->setFixedHeight(34);
    m_sendBtn->setProperty("accent", true);
    m_sendBtn->setShortcut(QKeySequence(QStringLiteral("Ctrl+Return")));
    connect(m_sendBtn, &QPushButton::clicked, this, &MonitorPage::onSendClicked);

    txBottomLayout->addWidget(m_autoSendCheck);
    txBottomLayout->addWidget(m_intervalSpin);
    txBottomLayout->addStretch();
    txBottomLayout->addWidget(m_clearTxBtn);
    txBottomLayout->addWidget(m_sendBtn);

    txLayout->addLayout(txBottomLayout);

    rootLayout->addWidget(txCard, 1);
}

void MonitorPage::appendLog(const QByteArray &data, bool isRx, const QDateTime &time)
{
    if (data.isEmpty()) {
        return;
    }

    QString formattedContent;
    if (m_rxHexCheck->isChecked()) {
        formattedContent = CrcUtils::byteArrayToHexString(data, true, QStringLiteral(" "));
    } else {
        formattedContent = QString::fromUtf8(data);
    }

    QString line;
    if (m_timestampCheck->isChecked()) {
        QString tag = isRx ? QStringLiteral("<span style='color:#00A4EF;'>[Rx %1]</span> ")
                           : QStringLiteral("<span style='color:#107C41;'>[Tx %1]</span> ");
        tag = tag.arg(time.toString(QStringLiteral("HH:mm:ss.zzz")));
        line = tag + formattedContent.toHtmlEscaped();
        m_rxTextEdit->appendHtml(line);
    } else {
        m_rxTextEdit->appendPlainText(formattedContent);
    }

    if (m_autoScrollCheck->isChecked()) {
        m_rxTextEdit->verticalScrollBar()->setValue(m_rxTextEdit->verticalScrollBar()->maximum());
    }
}

void MonitorPage::onDataReceived(const QByteArray &data, const QDateTime &timestamp)
{
    appendLog(data, true, timestamp);
}

void MonitorPage::onDataSent(const QByteArray &data, const QDateTime &timestamp)
{
    appendLog(data, false, timestamp);
}

void MonitorPage::onSendClicked()
{
    auto &engine = SerialEngine::instance();
    if (!engine.isOpen()) {
        if (m_autoSendCheck->isChecked()) {
            m_autoSendCheck->setChecked(false);
        }
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("串口未打开，无法发送！"));
        return;
    }

    QString text = m_txTextEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        return;
    }

    bool isHex = m_txHexCheck->isChecked();
    QString suffix = m_suffixCombo->currentData().toString();
    engine.sendString(text, isHex, suffix);
}

void MonitorPage::onClearRxClicked()
{
    m_rxTextEdit->clear();
}

void MonitorPage::onExportLogClicked()
{
    QString content = m_rxTextEdit->toPlainText();
    if (content.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("当前接收区内容为空，无需导出。"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出接收日志"),
        QStringLiteral("SerialLog_%1.txt").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))),
        QStringLiteral("文本文件 (*.txt *.log);;所有文件 (*.*)")
    );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << content;
            file.close();
            QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("日志已成功导出到:\n%1").arg(fileName));
        } else {
            QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("无法写入文件: %1").arg(file.errorString()));
        }
    }
}

void MonitorPage::onAutoSendToggled(bool checked)
{
    if (checked) {
        if (!SerialEngine::instance().isOpen()) {
            m_autoSendCheck->setChecked(false);
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先打开串口后再开启定时发送！"));
            return;
        }
        m_autoSendTimer->start(m_intervalSpin->value());
    } else {
        m_autoSendTimer->stop();
    }
}

void MonitorPage::onAppendCrcClicked()
{
    QString current = m_txTextEdit->toPlainText().trimmed();
    if (current.isEmpty()) {
        return;
    }

    QByteArray bytes = m_txHexCheck->isChecked() ? CrcUtils::hexStringToByteArray(current) : current.toUtf8();
    int crcType = m_crcTypeCombo->currentData().toInt();

    QString crcStr;
    if (crcType == 0) { // CRC16 Modbus (低字节在前)
        quint16 crc = CrcUtils::crc16Modbus(bytes);
        crcStr = QStringLiteral("%1 %2")
                     .arg(crc & 0xFF, 2, 16, QLatin1Char('0'))
                     .arg((crc >> 8) & 0xFF, 2, 16, QLatin1Char('0'))
                     .toUpper();
    } else if (crcType == 1) { // CRC16 CCITT (高字节在前)
        quint16 crc = CrcUtils::crc16Ccitt(bytes);
        crcStr = QStringLiteral("%1 %2")
                     .arg((crc >> 8) & 0xFF, 2, 16, QLatin1Char('0'))
                     .arg(crc & 0xFF, 2, 16, QLatin1Char('0'))
                     .toUpper();
    } else if (crcType == 2) { // CRC32
        quint32 crc = CrcUtils::crc32(bytes);
        crcStr = QStringLiteral("%1").arg(crc, 8, 16, QLatin1Char('0')).toUpper();
    } else if (crcType == 3) { // Sum8
        quint8 sum = CrcUtils::checksum8(bytes);
        crcStr = QStringLiteral("%1").arg(sum, 2, 16, QLatin1Char('0')).toUpper();
    } else if (crcType == 4) { // XOR
        quint8 x = CrcUtils::xor8(bytes);
        crcStr = QStringLiteral("%1").arg(x, 2, 16, QLatin1Char('0')).toUpper();
    }

    if (m_txHexCheck->isChecked()) {
        m_txTextEdit->setPlainText(current + QStringLiteral(" ") + crcStr);
    } else {
        m_txTextEdit->setPlainText(current + crcStr);
    }
}
