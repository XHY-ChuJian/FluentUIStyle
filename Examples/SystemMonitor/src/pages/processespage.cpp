#include "processespage.h"

#include "excontentdialog.h"
#include "exinfobarhost.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

ProcessesPage::ProcessesPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    refreshProcessList();
}

void ProcessesPage::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(14);

    // 顶部标题
    auto *titleLayout = new QHBoxLayout;
    auto *titleLabel = new QLabel(QStringLiteral("系统运行进程管理器"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    m_countLabel = new QLabel(QStringLiteral("共 0 个进程"), this);
    titleLayout->addWidget(m_countLabel);
    mainLayout->addLayout(titleLayout);

    // 工具栏
    auto *toolbarLayout = new QHBoxLayout;
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("🔍 搜索进程名称或 PID..."));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ProcessesPage::onFilterTextChanged);
    toolbarLayout->addWidget(m_searchEdit, 1);

    m_refreshBtn = new QPushButton(QStringLiteral("🔄 刷新列表"), this);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ProcessesPage::refreshProcessList);
    toolbarLayout->addWidget(m_refreshBtn);

    m_killBtn = new QPushButton(QStringLiteral("⛔ 结束进程"), this);
    m_killBtn->setCursor(Qt::PointingHandCursor);
    m_killBtn->setEnabled(false);
    connect(m_killBtn, &QPushButton::clicked, this, &ProcessesPage::onKillProcessClicked);
    toolbarLayout->addWidget(m_killBtn);

    mainLayout->addLayout(toolbarLayout);

    // 进程表格
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(4);
    m_tableWidget->setHorizontalHeaderLabels({
        QStringLiteral("PID"),
        QStringLiteral("进程名称"),
        QStringLiteral("物理内存 (Working Set)"),
        QStringLiteral("线程数")
    });

    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setShowGrid(false);
    m_tableWidget->setSortingEnabled(true);

    connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, [this]() {
        m_killBtn->setEnabled(!m_tableWidget->selectedItems().isEmpty());
    });

    mainLayout->addWidget(m_tableWidget);
}

void ProcessesPage::refreshProcessList()
{
    m_cachedProcesses = SystemProvider::instance().refreshProcesses();
    updateTableData();
}

void ProcessesPage::onFilterTextChanged(const QString &text)
{
    Q_UNUSED(text);
    updateTableData();
}

void ProcessesPage::updateTableData()
{
    QString query = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : QString();

    m_tableWidget->setSortingEnabled(false);
    m_tableWidget->setRowCount(0);

    int count = 0;
    for (const auto &p : m_cachedProcesses) {
        if (!query.isEmpty()) {
            if (!p.name.toLower().contains(query) && !QString::number(p.pid).contains(query)) {
                continue;
            }
        }

        int row = m_tableWidget->rowCount();
        m_tableWidget->insertRow(row);

        auto *pidItem = new QTableWidgetItem;
        pidItem->setData(Qt::DisplayRole, (qlonglong)p.pid);
        m_tableWidget->setItem(row, 0, pidItem);

        auto *nameItem = new QTableWidgetItem(p.name);
        m_tableWidget->setItem(row, 1, nameItem);

        auto *memItem = new QTableWidgetItem;
        memItem->setData(Qt::DisplayRole, (qlonglong)p.memoryWorkingSetBytes);
        memItem->setText(SystemProvider::formatBytes(p.memoryWorkingSetBytes));
        m_tableWidget->setItem(row, 2, memItem);

        auto *threadItem = new QTableWidgetItem;
        threadItem->setData(Qt::DisplayRole, p.threadCount);
        m_tableWidget->setItem(row, 3, threadItem);

        count++;
    }

    m_tableWidget->setSortingEnabled(true);
    m_countLabel->setText(QStringLiteral("显示 %1 / 共 %2 个进程").arg(count).arg(m_cachedProcesses.size()));
    m_killBtn->setEnabled(false);
}

void ProcessesPage::onKillProcessClicked()
{
    auto selected = m_tableWidget->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    int row = selected.first()->row();
    qint64 pid = m_tableWidget->item(row, 0)->data(Qt::DisplayRole).toLongLong();
    QString name = m_tableWidget->item(row, 1)->text();

    ExContentDialog dialog(this);
    dialog.setTitle(QStringLiteral("结束进程确认"));
    dialog.setContent(QStringLiteral("确定要强制终止进程【%1】 (PID: %2) 吗？\n强制结束进程可能导致未保存的数据丢失。")
                          .arg(name)
                          .arg(pid));
    dialog.setPrimaryButtonText(QStringLiteral("终止进程"));
    dialog.setCloseButtonText(QStringLiteral("取消"));

    if (dialog.exec() == QDialog::Accepted) {
        bool ok = SystemProvider::instance().killProcess(pid);
        if (ok) {
            ExInfoBarHost::defaultHost()->showInfoBar(
                ExInfoBar::Success,
                QStringLiteral("进程已终止"),
                QStringLiteral("已成功结束进程【%1】 (PID: %2)").arg(name).arg(pid)
            );
            refreshProcessList();
        } else {
            ExInfoBarHost::defaultHost()->showInfoBar(
                ExInfoBar::Error,
                QStringLiteral("操作失败"),
                QStringLiteral("无法终止进程【%1】 (PID: %2)，可能是权限不足。").arg(name).arg(pid)
            );
        }
    }
}
