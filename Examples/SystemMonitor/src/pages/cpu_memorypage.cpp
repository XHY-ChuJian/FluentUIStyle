#include "cpu_memorypage.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
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

CpuMemoryPage::CpuMemoryPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    connect(&SystemProvider::instance(), &SystemProvider::snapshotUpdated,
            this, &CpuMemoryPage::onSnapshotUpdated);

    onSnapshotUpdated(SystemProvider::instance().lastSnapshot());
}

void CpuMemoryPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget(scrollArea);
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(18);

    // 页面主标题
    auto *titleLabel = new QLabel(QStringLiteral("处理器与内存深度监控"), container);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // 1. CPU 核心总览卡片
    auto *cpuCard = createCardFrame(container);
    auto *cpuLayout = new QVBoxLayout(cpuCard);
    cpuLayout->setContentsMargins(18, 16, 18, 18);
    cpuLayout->setSpacing(12);

    cpuLayout->addWidget(createCardHeader(QStringLiteral("⚡ 处理器总体负载与各逻辑核心"), cpuCard));

    m_cpuSpecLabel = new QLabel(QStringLiteral("处理器: 读取中..."), cpuCard);
    cpuLayout->addWidget(m_cpuSpecLabel);

    auto *totalBarLayout = new QHBoxLayout;
    auto *totalTxt = new QLabel(QStringLiteral("总体利用率:"), cpuCard);
    m_cpuTotalUsageLabel = new QLabel(QStringLiteral("0.0%"), cpuCard);
    QFont boldFont = m_cpuTotalUsageLabel->font();
    boldFont.setBold(true);
    m_cpuTotalUsageLabel->setFont(boldFont);

    m_cpuTotalBar = new QProgressBar(cpuCard);
    m_cpuTotalBar->setRange(0, 100);
    m_cpuTotalBar->setValue(0);
    m_cpuTotalBar->setFixedHeight(12);

    totalBarLayout->addWidget(totalTxt);
    totalBarLayout->addWidget(m_cpuTotalBar, 1);
    totalBarLayout->addWidget(m_cpuTotalUsageLabel);
    cpuLayout->addLayout(totalBarLayout);

    // 核心网格容器
    m_coresContainer = new QWidget(cpuCard);
    m_coresLayout = new QGridLayout(m_coresContainer);
    m_coresLayout->setContentsMargins(0, 8, 0, 0);
    m_coresLayout->setSpacing(10);
    cpuLayout->addWidget(m_coresContainer);

    mainLayout->addWidget(cpuCard);

    // 2. 内存分析卡片
    auto *memCard = createCardFrame(container);
    auto *memLayout = new QVBoxLayout(memCard);
    memLayout->setContentsMargins(18, 16, 18, 18);
    memLayout->setSpacing(14);

    memLayout->addWidget(createCardHeader(QStringLiteral("🧠 物理与虚拟内存深度分析"), memCard));

    // 物理内存
    auto *physBox = new QWidget(memCard);
    auto *physLayout = new QVBoxLayout(physBox);
    physLayout->setContentsMargins(0, 0, 0, 0);
    physLayout->setSpacing(6);

    auto *physHeaderLayout = new QHBoxLayout;
    physHeaderLayout->addWidget(new QLabel(QStringLiteral("物理内存 (RAM):"), physBox));
    physHeaderLayout->addStretch();
    m_physMemUsedLabel = new QLabel(QStringLiteral("已用: 0 GB"), physBox);
    m_physMemTotalLabel = new QLabel(QStringLiteral("总计: 0 GB"), physBox);
    physHeaderLayout->addWidget(m_physMemUsedLabel);
    physHeaderLayout->addWidget(new QLabel(QStringLiteral("/"), physBox));
    physHeaderLayout->addWidget(m_physMemTotalLabel);

    m_physMemBar = new QProgressBar(physBox);
    m_physMemBar->setRange(0, 100);
    m_physMemBar->setValue(0);
    m_physMemBar->setFixedHeight(14);

    physLayout->addLayout(physHeaderLayout);
    physLayout->addWidget(m_physMemBar);
    memLayout->addWidget(physBox);

    // 虚拟内存 / 交换文件
    auto *virtBox = new QWidget(memCard);
    auto *virtLayout = new QVBoxLayout(virtBox);
    virtLayout->setContentsMargins(0, 0, 0, 0);
    virtLayout->setSpacing(6);

    auto *virtHeaderLayout = new QHBoxLayout;
    virtHeaderLayout->addWidget(new QLabel(QStringLiteral("虚拟内存 / 页面文件 (Commit/Swap):"), virtBox));
    virtHeaderLayout->addStretch();
    m_virtMemUsedLabel = new QLabel(QStringLiteral("已用: 0 GB"), virtBox);
    m_virtMemTotalLabel = new QLabel(QStringLiteral("总计: 0 GB"), virtBox);
    virtHeaderLayout->addWidget(m_virtMemUsedLabel);
    virtHeaderLayout->addWidget(new QLabel(QStringLiteral("/"), virtBox));
    virtHeaderLayout->addWidget(m_virtMemTotalLabel);

    m_virtMemBar = new QProgressBar(virtBox);
    m_virtMemBar->setRange(0, 100);
    m_virtMemBar->setValue(0);
    m_virtMemBar->setFixedHeight(14);

    virtLayout->addLayout(virtHeaderLayout);
    virtLayout->addWidget(m_virtMemBar);
    memLayout->addWidget(virtBox);

    mainLayout->addWidget(memCard);
    mainLayout->addStretch();

    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea);
}

void CpuMemoryPage::onSnapshotUpdated(const SystemSnapshot &snapshot)
{
    // CPU 总体
    m_cpuSpecLabel->setText(
        QStringLiteral("型号: %1 | 架构: %2 | 逻辑核心: %3 线程")
            .arg(snapshot.cpuName)
            .arg(snapshot.cpuArchitecture)
            .arg(snapshot.logicalCoreCount)
    );
    int totalCpu = qRound(snapshot.cpuUsageTotal);
    m_cpuTotalBar->setValue(totalCpu);
    m_cpuTotalUsageLabel->setText(QStringLiteral("%1%").arg(snapshot.cpuUsageTotal, 0, 'f', 1));

    // 各核心网格动态生成
    int coreCount = snapshot.cpuUsageCores.size();
    if (m_coreWidgets.size() != coreCount) {
        // 清理旧控件
        for (const auto &w : m_coreWidgets) {
            delete w.titleLabel;
            delete w.percentLabel;
            delete w.progressBar;
        }
        m_coreWidgets.clear();

        // 重新排布（每行 4 列）
        const int cols = 4;
        for (int i = 0; i < coreCount; ++i) {
            auto *cellWidget = new QFrame(m_coresContainer);
            cellWidget->setStyleSheet(QStringLiteral(
                "QFrame {"
                "  border: 1px solid rgba(128, 128, 128, 0.15);"
                "  border-radius: 6px;"
                "  padding: 4px;"
                "  background-color: palette(base);"
                "}"
            ));
            auto *cellLayout = new QVBoxLayout(cellWidget);
            cellLayout->setContentsMargins(8, 6, 8, 6);
            cellLayout->setSpacing(4);

            auto *headLayout = new QHBoxLayout;
            auto *titleLbl = new QLabel(QStringLiteral("核心 #%1").arg(i), cellWidget);
            auto *pctLbl = new QLabel(QStringLiteral("0%"), cellWidget);
            pctLbl->setAlignment(Qt::AlignRight);

            headLayout->addWidget(titleLbl);
            headLayout->addStretch();
            headLayout->addWidget(pctLbl);

            auto *bar = new QProgressBar(cellWidget);
            bar->setRange(0, 100);
            bar->setValue(0);
            bar->setFixedHeight(8);
            bar->setTextVisible(false);

            cellLayout->addLayout(headLayout);
            cellLayout->addWidget(bar);

            int r = i / cols;
            int c = i % cols;
            m_coresLayout->addWidget(cellWidget, r, c);

            m_coreWidgets.append({titleLbl, pctLbl, bar});
        }
    }

    // 更新各个核心数值
    for (int i = 0; i < coreCount && i < m_coreWidgets.size(); ++i) {
        double val = snapshot.cpuUsageCores.at(i);
        m_coreWidgets[i].percentLabel->setText(QStringLiteral("%1%").arg(val, 0, 'f', 0));
        m_coreWidgets[i].progressBar->setValue(qRound(val));
    }

    // 内存
    m_physMemUsedLabel->setText(QStringLiteral("已用: %1").arg(SystemProvider::formatBytes(snapshot.usedPhysicalMemory)));
    m_physMemTotalLabel->setText(QStringLiteral("总计: %1 (%2%)")
        .arg(SystemProvider::formatBytes(snapshot.totalPhysicalMemory))
        .arg(snapshot.memoryUsagePercent, 0, 'f', 1));
    m_physMemBar->setValue(qRound(snapshot.memoryUsagePercent));

    m_virtMemUsedLabel->setText(QStringLiteral("已用: %1").arg(SystemProvider::formatBytes(snapshot.usedVirtualMemory)));
    m_virtMemTotalLabel->setText(QStringLiteral("总计: %1 (%2%)")
        .arg(SystemProvider::formatBytes(snapshot.totalVirtualMemory))
        .arg(snapshot.virtualMemoryUsagePercent, 0, 'f', 1));
    m_virtMemBar->setValue(qRound(snapshot.virtualMemoryUsagePercent));
}
