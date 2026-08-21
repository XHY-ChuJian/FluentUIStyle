#include "toolboxpage.h"
#include "core/crcutils.h"
#include "core/serialengine.h"

#include <QClipboard>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
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

ToolboxPage::ToolboxPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onCrcInputChanged();
    onFloatToHexClicked();
    onBuildModbusFrameClicked();
}

void ToolboxPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget(scrollArea);
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(14);

    // ==========================================
    // 1. CRC 与校验码实时计算器卡片
    // ==========================================
    auto *crcCard = createCardFrame(container);
    auto *crcLayout = new QVBoxLayout(crcCard);
    crcLayout->setContentsMargins(14, 12, 14, 12);
    crcLayout->setSpacing(10);

    auto *crcHead = new QHBoxLayout();
    crcHead->addWidget(createCardHeader(QStringLiteral("🧮 CRC 与校验码多功能计算器"), crcCard));
    crcHead->addStretch();
    m_crcInputHexRadio = new QRadioButton(QStringLiteral("HEX 模式"), crcCard);
    m_crcInputHexRadio->setChecked(true);
    m_crcInputTextRadio = new QRadioButton(QStringLiteral("文本 ASCII"), crcCard);
    connect(m_crcInputHexRadio, &QRadioButton::toggled, this, &ToolboxPage::onCrcInputChanged);
    crcHead->addWidget(m_crcInputHexRadio);
    crcHead->addWidget(m_crcInputTextRadio);
    crcLayout->addLayout(crcHead);

    m_crcInputEdit = new QPlainTextEdit(crcCard);
    m_crcInputEdit->setMaximumHeight(70);
    m_crcInputEdit->setFont(QFont(QStringLiteral("Consolas, Courier New"), 12));
    m_crcInputEdit->setPlainText(QStringLiteral("01 03 00 00 00 02"));
    m_crcInputEdit->setPlaceholderText(QStringLiteral("请输入要计算校验码的 HEX 或文本数据..."));
    connect(m_crcInputEdit, &QPlainTextEdit::textChanged, this, &ToolboxPage::onCrcInputChanged);
    crcLayout->addWidget(m_crcInputEdit);

    auto *crcGrid = new QGridLayout();
    crcGrid->setHorizontalSpacing(12);
    crcGrid->setVerticalSpacing(8);

    auto addCrcResultField = [this, crcGrid, crcCard](int row, int col, const QString &label, QLineEdit *&editPtr) {
        crcGrid->addWidget(new QLabel(label, crcCard), row, col);
        editPtr = new QLineEdit(crcCard);
        editPtr->setReadOnly(true);
        editPtr->setFont(QFont(QStringLiteral("Consolas, Courier New"), 12));
        crcGrid->addWidget(editPtr, row, col + 1);

        auto *copyBtn = new QPushButton(QStringLiteral("📋"), crcCard);
        copyBtn->setToolTip(QStringLiteral("复制到剪贴板"));
        copyBtn->setFixedWidth(30);
        connect(copyBtn, &QPushButton::clicked, this, [editPtr]() {
            QGuiApplication::clipboard()->setText(editPtr->text());
        });
        crcGrid->addWidget(copyBtn, row, col + 2);
    };

    addCrcResultField(0, 0, QStringLiteral("CRC-16 Modbus (低前高后):"), m_crc16ModbusLeEdit);
    addCrcResultField(0, 3, QStringLiteral("CRC-16 Modbus (高前低后):"), m_crc16ModbusBeEdit);

    addCrcResultField(1, 0, QStringLiteral("CRC-16 CCITT:"), m_crc16CcittEdit);
    addCrcResultField(1, 3, QStringLiteral("CRC-16 XMODEM:"), m_crc16XmodemEdit);

    addCrcResultField(2, 0, QStringLiteral("CRC-32 (IEEE 802.3):"), m_crc32Edit);
    addCrcResultField(2, 3, QStringLiteral("Sum8 累加和:"), m_sum8Edit);

    addCrcResultField(3, 0, QStringLiteral("XOR 异或校验 (BCC):"), m_xor8Edit);
    addCrcResultField(3, 3, QStringLiteral("LRC 纵向校验:"), m_lrcEdit);

    crcLayout->addLayout(crcGrid);
    mainLayout->addWidget(crcCard);

    // ==========================================
    // 2. IEEE-754 浮点数与十六进制互转卡片
    // ==========================================
    auto *floatCard = createCardFrame(container);
    auto *floatLayout = new QVBoxLayout(floatCard);
    floatLayout->setContentsMargins(14, 12, 14, 12);
    floatLayout->setSpacing(10);

    floatLayout->addWidget(createCardHeader(QStringLiteral("🔢 IEEE-754 32位单精度浮点数 ↔ 十六进制 (HEX) 转换"), floatCard));

    auto *fRow1 = new QHBoxLayout();
    fRow1->addWidget(new QLabel(QStringLiteral("浮点数值 (Float):"), floatCard));
    m_floatInputEdit = new QLineEdit(QStringLiteral("123.456"), floatCard);
    fRow1->addWidget(m_floatInputEdit);

    m_floatToHexBtn = new QPushButton(QStringLiteral("转换 ➡️"), floatCard);
    m_floatToHexBtn->setProperty("accent", true);
    connect(m_floatToHexBtn, &QPushButton::clicked, this, &ToolboxPage::onFloatToHexClicked);
    fRow1->addWidget(m_floatToHexBtn);

    fRow1->addWidget(new QLabel(QStringLiteral("大端 (Big-Endian):"), floatCard));
    m_hexOutputBeEdit = new QLineEdit(floatCard);
    m_hexOutputBeEdit->setReadOnly(true);
    m_hexOutputBeEdit->setFont(QFont(QStringLiteral("Consolas, Courier New"), 12));
    fRow1->addWidget(m_hexOutputBeEdit);

    fRow1->addWidget(new QLabel(QStringLiteral("小端 (Little-Endian):"), floatCard));
    m_hexOutputLeEdit = new QLineEdit(floatCard);
    m_hexOutputLeEdit->setReadOnly(true);
    m_hexOutputLeEdit->setFont(QFont(QStringLiteral("Consolas, Courier New"), 12));
    fRow1->addWidget(m_hexOutputLeEdit);
    floatLayout->addLayout(fRow1);

    auto *fRow2 = new QHBoxLayout();
    fRow2->addWidget(new QLabel(QStringLiteral("十六进制 (HEX 32-bit):"), floatCard));
    m_hexInputEdit = new QLineEdit(QStringLiteral("42F6E979"), floatCard);
    m_hexInputEdit->setFont(QFont(QStringLiteral("Consolas, Courier New"), 12));
    fRow2->addWidget(m_hexInputEdit);

    m_hexToFloatBtn = new QPushButton(QStringLiteral("转换 ➡️"), floatCard);
    connect(m_hexToFloatBtn, &QPushButton::clicked, this, &ToolboxPage::onHexToFloatClicked);
    fRow2->addWidget(m_hexToFloatBtn);

    fRow2->addWidget(new QLabel(QStringLiteral("解析浮点数结果:"), floatCard));
    m_floatOutputEdit = new QLineEdit(floatCard);
    m_floatOutputEdit->setReadOnly(true);
    m_floatOutputEdit->setFont(QFont(QStringLiteral("Consolas, Courier New"), 12));
    fRow2->addWidget(m_floatOutputEdit);
    floatLayout->addLayout(fRow2);

    mainLayout->addWidget(floatCard);

    // ==========================================
    // 3. Modbus RTU 快速组帧器
    // ==========================================
    auto *mbCard = createCardFrame(container);
    auto *mbLayout = new QVBoxLayout(mbCard);
    mbLayout->setContentsMargins(14, 12, 14, 12);
    mbLayout->setSpacing(10);

    mbLayout->addWidget(createCardHeader(QStringLiteral("📦 Modbus RTU 快速组帧器"), mbCard));

    auto *mbRow1 = new QHBoxLayout();
    mbRow1->addWidget(new QLabel(QStringLiteral("从站地址:"), mbCard));
    m_mbSlaveSpin = new QSpinBox(mbCard);
    m_mbSlaveSpin->setRange(1, 247);
    m_mbSlaveSpin->setValue(1);
    m_mbSlaveSpin->setFixedHeight(34);
    mbRow1->addWidget(m_mbSlaveSpin);

    mbRow1->addWidget(new QLabel(QStringLiteral("功能码:"), mbCard));
    m_mbFuncCombo = new QComboBox(mbCard);
    m_mbFuncCombo->addItem(QStringLiteral("01H 读线圈 (Read Coils)"), 0x01);
    m_mbFuncCombo->addItem(QStringLiteral("02H 读离散输入 (Read Discrete Inputs)"), 0x02);
    m_mbFuncCombo->addItem(QStringLiteral("03H 读保持寄存器 (Read Holding Registers)"), 0x03);
    m_mbFuncCombo->addItem(QStringLiteral("04H 读输入寄存器 (Read Input Registers)"), 0x04);
    m_mbFuncCombo->addItem(QStringLiteral("05H 写单个线圈 (Write Single Coil)"), 0x05);
    m_mbFuncCombo->addItem(QStringLiteral("06H 写单个寄存器 (Write Single Register)"), 0x06);
    m_mbFuncCombo->setCurrentIndex(2); // 03H
    m_mbFuncCombo->setFixedHeight(34);
    mbRow1->addWidget(m_mbFuncCombo);

    mbRow1->addWidget(new QLabel(QStringLiteral("起始地址:"), mbCard));
    m_mbRegAddrSpin = new QSpinBox(mbCard);
    m_mbRegAddrSpin->setRange(0, 65535);
    m_mbRegAddrSpin->setValue(0);
    m_mbRegAddrSpin->setFixedHeight(34);
    mbRow1->addWidget(m_mbRegAddrSpin);

    mbRow1->addWidget(new QLabel(QStringLiteral("数量/数值:"), mbCard));
    m_mbCountSpin = new QSpinBox(mbCard);
    m_mbCountSpin->setRange(1, 65535);
    m_mbCountSpin->setValue(2);
    m_mbCountSpin->setFixedHeight(34);
    mbRow1->addWidget(m_mbCountSpin);

    m_mbBuildBtn = new QPushButton(QStringLiteral("生成报文 ⚡"), mbCard);
    m_mbBuildBtn->setFixedHeight(34);
    connect(m_mbBuildBtn, &QPushButton::clicked, this, &ToolboxPage::onBuildModbusFrameClicked);
    mbRow1->addWidget(m_mbBuildBtn);

    mbLayout->addLayout(mbRow1);

    auto *mbRow2 = new QHBoxLayout();
    mbRow2->addWidget(new QLabel(QStringLiteral("完整 RTU 帧 (含CRC):"), mbCard));
    m_mbResultEdit = new QLineEdit(mbCard);
    m_mbResultEdit->setReadOnly(true);
    m_mbResultEdit->setFont(QFont(QStringLiteral("Consolas, Courier New"), 12));
    mbRow2->addWidget(m_mbResultEdit, 1);

    auto *copyMbBtn = new QPushButton(QStringLiteral("复制"), mbCard);
    connect(copyMbBtn, &QPushButton::clicked, this, [this]() {
        QGuiApplication::clipboard()->setText(m_mbResultEdit->text());
    });
    mbRow2->addWidget(copyMbBtn);

    m_mbSendBtn = new QPushButton(QStringLiteral("🚀 直接发送至串口"), mbCard);
    m_mbSendBtn->setProperty("accent", true);
    connect(m_mbSendBtn, &QPushButton::clicked, this, &ToolboxPage::onSendModbusToSerial);
    mbRow2->addWidget(m_mbSendBtn);

    mbLayout->addLayout(mbRow2);

    mainLayout->addWidget(mbCard);
    mainLayout->addStretch();

    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea);
}

void ToolboxPage::onCrcInputChanged()
{
    QString input = m_crcInputEdit->toPlainText().trimmed();
    QByteArray bytes = m_crcInputHexRadio->isChecked()
                           ? CrcUtils::hexStringToByteArray(input)
                           : input.toUtf8();

    if (bytes.isEmpty()) {
        m_crc16ModbusLeEdit->clear();
        m_crc16ModbusBeEdit->clear();
        m_crc16CcittEdit->clear();
        m_crc16XmodemEdit->clear();
        m_crc32Edit->clear();
        m_sum8Edit->clear();
        m_xor8Edit->clear();
        m_lrcEdit->clear();
        return;
    }

    quint16 crcModbus = CrcUtils::crc16Modbus(bytes);
    m_crc16ModbusLeEdit->setText(QStringLiteral("%1 %2")
                                     .arg(crcModbus & 0xFF, 2, 16, QLatin1Char('0'))
                                     .arg((crcModbus >> 8) & 0xFF, 2, 16, QLatin1Char('0'))
                                     .toUpper());
    m_crc16ModbusBeEdit->setText(QStringLiteral("%1 %2")
                                     .arg((crcModbus >> 8) & 0xFF, 2, 16, QLatin1Char('0'))
                                     .arg(crcModbus & 0xFF, 2, 16, QLatin1Char('0'))
                                     .toUpper());

    quint16 crcCcitt = CrcUtils::crc16Ccitt(bytes);
    m_crc16CcittEdit->setText(QStringLiteral("%1").arg(crcCcitt, 4, 16, QLatin1Char('0')).toUpper());

    quint16 crcXmodem = CrcUtils::crc16Xmodem(bytes);
    m_crc16XmodemEdit->setText(QStringLiteral("%1").arg(crcXmodem, 4, 16, QLatin1Char('0')).toUpper());

    quint32 c32 = CrcUtils::crc32(bytes);
    m_crc32Edit->setText(QStringLiteral("%1").arg(c32, 8, 16, QLatin1Char('0')).toUpper());

    quint8 s8 = CrcUtils::checksum8(bytes);
    m_sum8Edit->setText(QStringLiteral("%1").arg(s8, 2, 16, QLatin1Char('0')).toUpper());

    quint8 x8 = CrcUtils::xor8(bytes);
    m_xor8Edit->setText(QStringLiteral("%1").arg(x8, 2, 16, QLatin1Char('0')).toUpper());

    quint8 lrcVal = CrcUtils::lrc(bytes);
    m_lrcEdit->setText(QStringLiteral("%1").arg(lrcVal, 2, 16, QLatin1Char('0')).toUpper());
}

void ToolboxPage::onFloatToHexClicked()
{
    bool ok = false;
    float val = m_floatInputEdit->text().toFloat(&ok);
    if (!ok) {
        m_hexOutputBeEdit->setText(QStringLiteral("无效数字"));
        m_hexOutputLeEdit->setText(QStringLiteral("无效数字"));
        return;
    }

    m_hexOutputBeEdit->setText(CrcUtils::floatToHex(val, true));
    m_hexOutputLeEdit->setText(CrcUtils::floatToHex(val, false));
}

void ToolboxPage::onHexToFloatClicked()
{
    bool ok = false;
    float f = CrcUtils::hexToFloat(m_hexInputEdit->text(), true, &ok);
    if (!ok) {
        m_floatOutputEdit->setText(QStringLiteral("HEX 格式错误"));
        return;
    }
    m_floatOutputEdit->setText(QString::number(f, 'g', 7));
}

void ToolboxPage::onBuildModbusFrameClicked()
{
    quint8 slave = static_cast<quint8>(m_mbSlaveSpin->value());
    quint8 func = static_cast<quint8>(m_mbFuncCombo->currentData().toInt());
    quint16 reg = static_cast<quint16>(m_mbRegAddrSpin->value());
    quint16 count = static_cast<quint16>(m_mbCountSpin->value());

    QByteArray frame = CrcUtils::buildModbusRtuRequest(slave, func, reg, count);
    m_mbResultEdit->setText(CrcUtils::byteArrayToHexString(frame, true, QStringLiteral(" ")));
}

void ToolboxPage::onSendModbusToSerial()
{
    auto &engine = SerialEngine::instance();
    if (!engine.isOpen()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("串口未打开，无法发送！"));
        return;
    }

    QString hexFrame = m_mbResultEdit->text().trimmed();
    if (hexFrame.isEmpty()) {
        onBuildModbusFrameClicked();
        hexFrame = m_mbResultEdit->text().trimmed();
    }

    engine.sendString(hexFrame, true);
    QMessageBox::information(this, QStringLiteral("已发送"), QStringLiteral("Modbus 报文已成功发送至串口！\n%1").arg(hexFrame));
}
