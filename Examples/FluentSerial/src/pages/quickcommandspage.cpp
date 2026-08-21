#include "quickcommandspage.h"
#include "core/crcutils.h"

#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

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

QuickCommandsPage::QuickCommandsPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    auto &engine = SerialEngine::instance();
    connect(&engine, &SerialEngine::sequenceProgress, this, &QuickCommandsPage::onSequenceProgress);
    connect(&engine, &SerialEngine::sequenceFinished, this, &QuickCommandsPage::onSequenceFinished);

    loadDefaultPresets();
}

void QuickCommandsPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(12);

    auto *card = createCardFrame(this);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 12, 12, 12);
    cardLayout->setSpacing(10);

    // 顶部操作栏
    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(createCardHeader(QStringLiteral("⚡ 多指令快捷面板 & 自动化序列"), card));
    topLayout->addStretch();

    m_addRowBtn = new QPushButton(QStringLiteral("➕ 新增指令"), card);
    connect(m_addRowBtn, &QPushButton::clicked, this, &QuickCommandsPage::onAddRowClicked);

    m_importBtn = new QPushButton(QStringLiteral("📂 导入 JSON"), card);
    connect(m_importBtn, &QPushButton::clicked, this, &QuickCommandsPage::onImportJsonClicked);

    m_exportBtn = new QPushButton(QStringLiteral("💾 导出 JSON"), card);
    connect(m_exportBtn, &QPushButton::clicked, this, &QuickCommandsPage::onExportJsonClicked);

    m_clearAllBtn = new QPushButton(QStringLiteral("🗑️ 清空所有"), card);
    connect(m_clearAllBtn, &QPushButton::clicked, this, &QuickCommandsPage::onClearAllClicked);

    topLayout->addWidget(m_addRowBtn);
    topLayout->addWidget(m_importBtn);
    topLayout->addWidget(m_exportBtn);
    topLayout->addWidget(m_clearAllBtn);
    cardLayout->addLayout(topLayout);

    // 指令表格
    m_table = new QTableWidget(card);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("启用"),
        QStringLiteral("指令名称/备注"),
        QStringLiteral("格式"),
        QStringLiteral("发送数据内容"),
        QStringLiteral("延时(ms)"),
        QStringLiteral("快捷发送"),
        QStringLiteral("操作")
    });

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);

    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    cardLayout->addWidget(m_table, 1);

    // 底部自动化调度栏
    auto *bottomLayout = new QHBoxLayout();
    m_singleRunBtn = new QPushButton(QStringLiteral("▶️ 单次顺序执行"), card);
    m_singleRunBtn->setProperty("accent", true);
    connect(m_singleRunBtn, &QPushButton::clicked, this, &QuickCommandsPage::onStartSingleRunClicked);

    m_loopRunBtn = new QPushButton(QStringLiteral("🔁 循环轮询执行"), card);
    connect(m_loopRunBtn, &QPushButton::clicked, this, &QuickCommandsPage::onStartLoopRunClicked);

    m_stopRunBtn = new QPushButton(QStringLiteral("⏹️ 停止"), card);
    m_stopRunBtn->setEnabled(false);
    connect(m_stopRunBtn, &QPushButton::clicked, this, &QuickCommandsPage::onStopRunClicked);

    m_progressBar = new QProgressBar(card);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(8);

    m_statusLabel = new QLabel(QStringLiteral("就绪"), card);

    bottomLayout->addWidget(m_singleRunBtn);
    bottomLayout->addWidget(m_loopRunBtn);
    bottomLayout->addWidget(m_stopRunBtn);
    bottomLayout->addSpacing(16);
    bottomLayout->addWidget(m_statusLabel);
    bottomLayout->addWidget(m_progressBar, 1);

    cardLayout->addLayout(bottomLayout);

    rootLayout->addWidget(card);
}

void QuickCommandsPage::loadDefaultPresets()
{
    m_table->setRowCount(0);

    QList<SerialCommandItem> defaults = {
        {QStringLiteral("1"), QStringLiteral("Modbus 读保持寄存器 0-3"), QStringLiteral("01 03 00 00 00 04 44 09"), true, 200, true},
        {QStringLiteral("2"), QStringLiteral("Modbus 读线圈 0-7"), QStringLiteral("01 01 00 00 00 08 3D CC"), true, 200, true},
        {QStringLiteral("3"), QStringLiteral("AT 模组测试握手"), QStringLiteral("AT\\r\\n"), false, 300, true},
        {QStringLiteral("4"), QStringLiteral("AT 查询信号强度"), QStringLiteral("AT+CSQ\\r\\n"), false, 500, true},
        {QStringLiteral("5"), QStringLiteral("单片机 LED 开"), QStringLiteral("LED_ON"), false, 200, false}
    };

    for (const auto &item : defaults) {
        addCommandRow(item);
    }
}

void QuickCommandsPage::addCommandRow(const SerialCommandItem &item)
{
    int row = m_table->rowCount();
    m_table->insertRow(row);

    // 0. 勾选框
    auto *checkWidget = new QWidget(m_table);
    auto *checkLayout = new QHBoxLayout(checkWidget);
    checkLayout->setContentsMargins(4, 0, 4, 0);
    checkLayout->setAlignment(Qt::AlignCenter);
    auto *check = new QCheckBox(checkWidget);
    check->setChecked(item.isLoopChecked);
    checkLayout->addWidget(check);
    m_table->setCellWidget(row, 0, checkWidget);

    // 1. 备注名称
    auto *nameEdit = new QLineEdit(item.name, m_table);
    nameEdit->setPlaceholderText(QStringLiteral("备注名称"));
    m_table->setCellWidget(row, 1, nameEdit);

    // 2. HEX / 文本
    auto *typeCombo = new QComboBox(m_table);
    typeCombo->addItem(QStringLiteral("HEX"), true);
    typeCombo->addItem(QStringLiteral("TEXT"), false);
    typeCombo->setCurrentIndex(item.isHex ? 0 : 1);
    m_table->setCellWidget(row, 2, typeCombo);

    // 3. 内容
    auto *payloadEdit = new QLineEdit(item.payload, m_table);
    payloadEdit->setFont(QFont(QStringLiteral("Consolas, Courier New"), 12));
    payloadEdit->setPlaceholderText(QStringLiteral("指令内容"));
    m_table->setCellWidget(row, 3, payloadEdit);

    // 4. 延时 (ms)
    auto *delaySpin = new QSpinBox(m_table);
    delaySpin->setRange(10, 60000);
    delaySpin->setValue(item.delayMs > 0 ? item.delayMs : 100);
    delaySpin->setSuffix(QStringLiteral(" ms"));
    delaySpin->setFixedHeight(34);
    m_table->setCellWidget(row, 4, delaySpin);

    // 5. 单条发送按钮
    auto *sendBtn = new QPushButton(QStringLiteral("🚀 发送"), m_table);
    connect(sendBtn, &QPushButton::clicked, this, [this, sendBtn]() {
        for (int r = 0; r < m_table->rowCount(); ++r) {
            if (m_table->cellWidget(r, 5) == sendBtn) {
                onSendSingleRow(r);
                break;
            }
        }
    });
    m_table->setCellWidget(row, 5, sendBtn);

    // 6. 删除按钮
    auto *delBtn = new QPushButton(QStringLiteral("✖"), m_table);
    delBtn->setFixedWidth(30);
    connect(delBtn, &QPushButton::clicked, this, [this, delBtn]() {
        for (int r = 0; r < m_table->rowCount(); ++r) {
            if (m_table->cellWidget(r, 6) == delBtn) {
                m_table->removeRow(r);
                break;
            }
        }
    });
    m_table->setCellWidget(row, 6, delBtn);
}

void QuickCommandsPage::onAddRowClicked()
{
    SerialCommandItem empty;
    empty.name = QStringLiteral("自定义指令 %1").arg(m_table->rowCount() + 1);
    empty.payload = QStringLiteral("01 03 00 00 00 01");
    empty.isHex = true;
    empty.delayMs = 200;
    empty.isLoopChecked = true;
    addCommandRow(empty);
}

void QuickCommandsPage::onClearAllClicked()
{
    if (QMessageBox::question(this, QStringLiteral("确认清空"), QStringLiteral("确定要清空所有快捷指令吗？")) == QMessageBox::Yes) {
        m_table->setRowCount(0);
    }
}

QList<SerialCommandItem> QuickCommandsPage::collectCommandsFromTable() const
{
    QList<SerialCommandItem> list;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        SerialCommandItem item;

        auto *checkWidget = m_table->cellWidget(r, 0);
        auto *check = checkWidget ? checkWidget->findChild<QCheckBox *>() : nullptr;
        item.isLoopChecked = check ? check->isChecked() : true;

        auto *nameEdit = qobject_cast<QLineEdit *>(m_table->cellWidget(r, 1));
        item.name = nameEdit ? nameEdit->text() : QString();

        auto *typeCombo = qobject_cast<QComboBox *>(m_table->cellWidget(r, 2));
        item.isHex = typeCombo ? typeCombo->currentData().toBool() : false;

        auto *payloadEdit = qobject_cast<QLineEdit *>(m_table->cellWidget(r, 3));
        item.payload = payloadEdit ? payloadEdit->text() : QString();

        auto *delaySpin = qobject_cast<QSpinBox *>(m_table->cellWidget(r, 4));
        item.delayMs = delaySpin ? delaySpin->value() : 100;

        if (!item.payload.isEmpty()) {
            list.append(item);
        }
    }
    return list;
}

void QuickCommandsPage::onSendSingleRow(int row)
{
    auto &engine = SerialEngine::instance();
    if (!engine.isOpen()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("串口未打开，无法发送！"));
        return;
    }

    auto *typeCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 2));
    auto *payloadEdit = qobject_cast<QLineEdit *>(m_table->cellWidget(row, 3));

    if (!payloadEdit || payloadEdit->text().isEmpty()) {
        return;
    }

    bool isHex = typeCombo ? typeCombo->currentData().toBool() : false;
    engine.sendString(payloadEdit->text(), isHex);
}

void QuickCommandsPage::onStartSingleRunClicked()
{
    auto &engine = SerialEngine::instance();
    if (!engine.isOpen()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先打开串口后再启动自动化序列！"));
        return;
    }

    auto cmds = collectCommandsFromTable();
    if (cmds.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先添加有效指令！"));
        return;
    }

    m_singleRunBtn->setEnabled(false);
    m_loopRunBtn->setEnabled(false);
    m_stopRunBtn->setEnabled(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText(QStringLiteral("正在执行单次序列..."));

    engine.startSequence(cmds, false);
}

void QuickCommandsPage::onStartLoopRunClicked()
{
    auto &engine = SerialEngine::instance();
    if (!engine.isOpen()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先打开串口后再启动轮询！"));
        return;
    }

    auto cmds = collectCommandsFromTable();
    if (cmds.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先添加有效指令！"));
        return;
    }

    m_singleRunBtn->setEnabled(false);
    m_loopRunBtn->setEnabled(false);
    m_stopRunBtn->setEnabled(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText(QStringLiteral("正在循环轮询执行..."));

    engine.startSequence(cmds, true);
}

void QuickCommandsPage::onStopRunClicked()
{
    SerialEngine::instance().stopSequence();
    onSequenceFinished();
}

void QuickCommandsPage::onSequenceProgress(int current, int total, const QString &cmdName)
{
    if (total > 0) {
        m_progressBar->setValue((current * 100) / total);
    }
    m_statusLabel->setText(QStringLiteral("已发送 [%1/%2]: %3").arg(current).arg(total).arg(cmdName));
}

void QuickCommandsPage::onSequenceFinished()
{
    m_singleRunBtn->setEnabled(true);
    m_loopRunBtn->setEnabled(true);
    m_stopRunBtn->setEnabled(false);
    m_statusLabel->setText(QStringLiteral("执行已停止"));
}

void QuickCommandsPage::onExportJsonClicked()
{
    auto cmds = collectCommandsFromTable();
    if (cmds.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("当前表格中无指令数据。"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出快捷指令配置"),
        QStringLiteral("FluentSerial_Commands.json"),
        QStringLiteral("JSON 文件 (*.json)")
    );

    if (fileName.isEmpty()) return;

    QJsonArray arr;
    for (const auto &c : cmds) {
        QJsonObject obj;
        obj[QStringLiteral("name")] = c.name;
        obj[QStringLiteral("payload")] = c.payload;
        obj[QStringLiteral("isHex")] = c.isHex;
        obj[QStringLiteral("delayMs")] = c.delayMs;
        obj[QStringLiteral("checked")] = c.isLoopChecked;
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("已成功导出指令配置！"));
    }
}

void QuickCommandsPage::onImportJsonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("导入快捷指令配置"),
        QString(),
        QStringLiteral("JSON 文件 (*.json)")
    );

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("无法读取文件！"));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        QMessageBox::warning(this, QStringLiteral("格式错误"), QStringLiteral("JSON 内容非指令列表数组！"));
        return;
    }

    m_table->setRowCount(0);
    QJsonArray arr = doc.array();
    for (const auto &val : arr) {
        QJsonObject obj = val.toObject();
        SerialCommandItem item;
        item.name = obj[QStringLiteral("name")].toString();
        item.payload = obj[QStringLiteral("payload")].toString();
        item.isHex = obj[QStringLiteral("isHex")].toBool();
        item.delayMs = obj[QStringLiteral("delayMs")].toInt(100);
        item.isLoopChecked = obj[QStringLiteral("checked")].toBool(true);
        addCommandRow(item);
    }

    QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("已成功导入 %1 条指令！").arg(arr.size()));
}
