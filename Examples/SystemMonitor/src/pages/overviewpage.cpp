#include "overviewpage.h"

#include "exliquidgauge.h"
#include "exmultiprogressring.h"
#include "exradialgauge.h"

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
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

OverviewPage::OverviewPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    connect(&SystemProvider::instance(), &SystemProvider::snapshotUpdated,
            this, &OverviewPage::onSnapshotUpdated);

    // 初始读取一次
    onSnapshotUpdated(SystemProvider::instance().lastSnapshot());
}

void OverviewPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget(scrollArea);
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(16);

    // 顶部工具栏
    auto *topBarLayout = new QHBoxLayout;
    auto *titleLabel = new QLabel(QStringLiteral("系统性能概览"), container);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    topBarLayout->addWidget(titleLabel);
    topBarLayout->addStretch();

    auto *intervalLabel = new QLabel(QStringLiteral("采样频率:"), container);
    m_intervalCombo = new QComboBox(container);
    m_intervalCombo->addItem(QStringLiteral("极速 (500ms)"), 500);
    m_intervalCombo->addItem(QStringLiteral("标准 (1秒)"), 1000);
    m_intervalCombo->addItem(QStringLiteral("节能 (2秒)"), 2000);
    m_intervalCombo->addItem(QStringLiteral("低频 (5秒)"), 5000);
    m_intervalCombo->setCurrentIndex(1);

    connect(m_intervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        int ms = m_intervalCombo->itemData(idx).toInt();
        SystemProvider::instance().setInterval(ms);
    });

    m_miniCapsuleBtn = new QPushButton(QStringLiteral("📌 开启桌面悬浮窗"), container);
    m_miniCapsuleBtn->setCursor(Qt::PointingHandCursor);
    connect(m_miniCapsuleBtn, &QPushButton::clicked, this, &OverviewPage::openMiniCapsuleRequested);

    topBarLayout->addWidget(intervalLabel);
    topBarLayout->addWidget(m_intervalCombo);
    topBarLayout->addSpacing(8);
    topBarLayout->addWidget(m_miniCapsuleBtn);

    mainLayout->addLayout(topBarLayout);

    // 核心四宫格仪表盘
    auto *cardsGrid = new QGridLayout;
    cardsGrid->setSpacing(16);

    // 1. CPU 卡片
    auto *cpuCard = createCardFrame(container);
    auto *cpuLayout = new QVBoxLayout(cpuCard);
    cpuLayout->setContentsMargins(16, 14, 16, 16);
    cpuLayout->setSpacing(8);
    cpuLayout->addWidget(createCardHeader(QStringLiteral("⚡ 处理器 (CPU)"), cpuCard));

    m_cpuGauge = new ExRadialGauge(cpuCard);
    m_cpuGauge->setMinimumSize(180, 180);
    m_cpuGauge->setScaleMode(ExRadialGauge::ProgressScale);
    m_cpuGauge->setNeedleStyle(ExRadialGauge::TriangleNeedle);
    m_cpuGauge->setSweepAreaVisible(true);
    m_cpuGauge->setTitle(QStringLiteral("利用率"));
    m_cpuGauge->setUnit(QStringLiteral("%"));
    m_cpuGauge->setRange(0, 100);
    m_cpuGauge->setValue(0);
    m_cpuGauge->setInteractive(false);

    cpuLayout->addWidget(m_cpuGauge, 0, Qt::AlignCenter);

    m_cpuSummaryLabel = new QLabel(QStringLiteral("加载中..."), cpuCard);
    m_cpuSummaryLabel->setAlignment(Qt::AlignCenter);
    cpuLayout->addWidget(m_cpuSummaryLabel);

    cardsGrid->addWidget(cpuCard, 0, 0);

    // 2. 内存卡片
    auto *memCard = createCardFrame(container);
    auto *memLayout = new QVBoxLayout(memCard);
    memLayout->setContentsMargins(16, 14, 16, 16);
    memLayout->setSpacing(8);
    memLayout->addWidget(createCardHeader(QStringLiteral("🧠 物理内存 (RAM)"), memCard));

    m_memoryLiquidGauge = new ExLiquidGauge(memCard);
    m_memoryLiquidGauge->setMinimumSize(160, 160);
    m_memoryLiquidGauge->setShape(ExLiquidGauge::CircleShape);
    m_memoryLiquidGauge->setRange(0, 100);
    m_memoryLiquidGauge->setValue(0);
    m_memoryLiquidGauge->setWaveAmplitude(5.0);
    m_memoryLiquidGauge->setWaveCount(2);

    memLayout->addWidget(m_memoryLiquidGauge, 0, Qt::AlignCenter);

    m_memorySummaryLabel = new QLabel(QStringLiteral("加载中..."), memCard);
    m_memorySummaryLabel->setAlignment(Qt::AlignCenter);
    memLayout->addWidget(m_memorySummaryLabel);

    cardsGrid->addWidget(memCard, 0, 1);

    // 3. 存储空间卡片
    auto *diskCard = createCardFrame(container);
    auto *diskLayout = new QVBoxLayout(diskCard);
    diskLayout->setContentsMargins(16, 14, 16, 16);
    diskLayout->setSpacing(8);
    diskLayout->addWidget(createCardHeader(QStringLiteral("💾 磁盘存储 (Storage)"), diskCard));

    m_diskMultiRing = new ExMultiProgressRing(diskCard);
    m_diskMultiRing->setMinimumSize(160, 160);
    m_diskMultiRing->setRingWidth(10.0);
    m_diskMultiRing->setRingSpacing(4.0);

    diskLayout->addWidget(m_diskMultiRing, 0, Qt::AlignCenter);

    m_diskSummaryLabel = new QLabel(QStringLiteral("计算中..."), diskCard);
    m_diskSummaryLabel->setAlignment(Qt::AlignCenter);
    diskLayout->addWidget(m_diskSummaryLabel);

    cardsGrid->addWidget(diskCard, 1, 0);

    // 4. 网络实时卡片
    auto *netCard = createCardFrame(container);
    auto *netLayout = new QVBoxLayout(netCard);
    netLayout->setContentsMargins(16, 14, 16, 16);
    netLayout->setSpacing(8);
    netLayout->addWidget(createCardHeader(QStringLiteral("🌐 实时网络 (Network)"), netCard));

    auto *speedBox = new QWidget(netCard);
    auto *speedBoxLayout = new QVBoxLayout(speedBox);
    speedBoxLayout->setContentsMargins(8, 8, 8, 8);
    speedBoxLayout->setSpacing(10);

    auto *downLayout = new QHBoxLayout;
    auto *downIcon = new QLabel(QStringLiteral("⬇️ 下载速率:"), speedBox);
    m_downloadSpeedLabel = new QLabel(QStringLiteral("0.0 KB/s"), speedBox);
    QFont speedFont = m_downloadSpeedLabel->font();
    speedFont.setBold(true);
    speedFont.setPixelSize(14);
    m_downloadSpeedLabel->setFont(speedFont);
    downLayout->addWidget(downIcon);
    downLayout->addStretch();
    downLayout->addWidget(m_downloadSpeedLabel);

    m_downSpeedBar = new QProgressBar(speedBox);
    m_downSpeedBar->setRange(0, 100);
    m_downSpeedBar->setValue(0);
    m_downSpeedBar->setTextVisible(false);
    m_downSpeedBar->setFixedHeight(8);

    auto *upLayout = new QHBoxLayout;
    auto *upIcon = new QLabel(QStringLiteral("⬆️ 上传速率:"), speedBox);
    m_uploadSpeedLabel = new QLabel(QStringLiteral("0.0 KB/s"), speedBox);
    m_uploadSpeedLabel->setFont(speedFont);
    upLayout->addWidget(upIcon);
    upLayout->addStretch();
    upLayout->addWidget(m_uploadSpeedLabel);

    m_upSpeedBar = new QProgressBar(speedBox);
    m_upSpeedBar->setRange(0, 100);
    m_upSpeedBar->setValue(0);
    m_upSpeedBar->setTextVisible(false);
    m_upSpeedBar->setFixedHeight(8);

    speedBoxLayout->addLayout(downLayout);
    speedBoxLayout->addWidget(m_downSpeedBar);
    speedBoxLayout->addLayout(upLayout);
    speedBoxLayout->addWidget(m_upSpeedBar);

    netLayout->addWidget(speedBox);

    m_netSummaryLabel = new QLabel(QStringLiteral("累计收发统计中..."), netCard);
    m_netSummaryLabel->setAlignment(Qt::AlignCenter);
    netLayout->addWidget(m_netSummaryLabel);

    cardsGrid->addWidget(netCard, 1, 1);

    mainLayout->addLayout(cardsGrid);

    // 下方系统基本信息摘要卡片
    auto *sysInfoCard = createCardFrame(container);
    auto *sysInfoLayout = new QVBoxLayout(sysInfoCard);
    sysInfoLayout->setContentsMargins(18, 14, 18, 16);
    sysInfoLayout->setSpacing(10);

    sysInfoLayout->addWidget(createCardHeader(QStringLiteral("🖥️ 系统与硬件规格摘要"), sysInfoCard));

    auto *infoGridLayout = new QGridLayout;
    infoGridLayout->setHorizontalSpacing(24);
    infoGridLayout->setVerticalSpacing(8);

    infoGridLayout->addWidget(new QLabel(QStringLiteral("操作系统:"), sysInfoCard), 0, 0);
    m_osInfoLabel = new QLabel(QStringLiteral("-"), sysInfoCard);
    infoGridLayout->addWidget(m_osInfoLabel, 0, 1);

    infoGridLayout->addWidget(new QLabel(QStringLiteral("处理器型号:"), sysInfoCard), 0, 2);
    m_cpuModelLabel = new QLabel(QStringLiteral("-"), sysInfoCard);
    infoGridLayout->addWidget(m_cpuModelLabel, 0, 3);

    infoGridLayout->addWidget(new QLabel(QStringLiteral("计算机与用户:"), sysInfoCard), 1, 0);
    m_hostUserLabel = new QLabel(QStringLiteral("-"), sysInfoCard);
    infoGridLayout->addWidget(m_hostUserLabel, 1, 1);

    infoGridLayout->addWidget(new QLabel(QStringLiteral("系统运行时间:"), sysInfoCard), 1, 2);
    m_uptimeLabel = new QLabel(QStringLiteral("-"), sysInfoCard);
    infoGridLayout->addWidget(m_uptimeLabel, 1, 3);

    sysInfoLayout->addLayout(infoGridLayout);
    mainLayout->addWidget(sysInfoCard);

    mainLayout->addStretch();

    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea);
}

void OverviewPage::onSnapshotUpdated(const SystemSnapshot &snapshot)
{
    // 1. CPU
    int cpuVal = qRound(snapshot.cpuUsageTotal);
    m_cpuGauge->setValue(cpuVal);
    m_cpuSummaryLabel->setText(
        QStringLiteral("核心: %1 逻辑线程 | 负载: %2%")
            .arg(snapshot.logicalCoreCount)
            .arg(snapshot.cpuUsageTotal, 0, 'f', 1)
    );

    // 2. 内存
    int memVal = qRound(snapshot.memoryUsagePercent);
    m_memoryLiquidGauge->setValue(memVal);
    m_memorySummaryLabel->setText(
        QStringLiteral("已用: %1 / %2 (%3%)")
            .arg(SystemProvider::formatBytes(snapshot.usedPhysicalMemory))
            .arg(SystemProvider::formatBytes(snapshot.totalPhysicalMemory))
            .arg(snapshot.memoryUsagePercent, 0, 'f', 1)
    );

    // 3. 磁盘环
    if (m_diskMultiRing->items().isEmpty() && !snapshot.disks.isEmpty()) {
        static const QList<QColor> diskColors = {
            QColor(0, 120, 215),
            QColor(16, 137, 62),
            QColor(232, 17, 35),
            QColor(255, 140, 0),
            QColor(136, 23, 152)
        };
        for (int i = 0; i < snapshot.disks.size() && i < 5; ++i) {
            const auto &d = snapshot.disks.at(i);
            QColor c = diskColors.at(i % diskColors.size());
            m_diskMultiRing->addItem(d.displayName, d.usagePercent, c);
        }
    } else {
        const auto &items = m_diskMultiRing->items();
        for (int i = 0; i < items.size() && i < snapshot.disks.size(); ++i) {
            items.at(i)->setValue(snapshot.disks.at(i).usagePercent);
        }
    }
    m_diskSummaryLabel->setText(
        QStringLiteral("全盘已用: %1 / 总计: %2")
            .arg(SystemProvider::formatBytes(snapshot.totalDiskUsed))
            .arg(SystemProvider::formatBytes(snapshot.totalDiskSpace))
    );

    // 4. 网络
    m_downloadSpeedLabel->setText(SystemProvider::formatSpeed(snapshot.downloadSpeedBytesPerSec));
    m_uploadSpeedLabel->setText(SystemProvider::formatSpeed(snapshot.uploadSpeedBytesPerSec));

    // 动态换算进度条（最大按 50MB/s 标定）
    int downPct = qBound(0, (int)(snapshot.downloadSpeedBytesPerSec * 100.0 / (50.0 * 1024 * 1024)), 100);
    int upPct = qBound(0, (int)(snapshot.uploadSpeedBytesPerSec * 100.0 / (10.0 * 1024 * 1024)), 100);
    m_downSpeedBar->setValue(downPct);
    m_upSpeedBar->setValue(upPct);

    m_netSummaryLabel->setText(
        QStringLiteral("总接收: %1 | 总发送: %2")
            .arg(SystemProvider::formatBytes(snapshot.totalBytesReceived))
            .arg(SystemProvider::formatBytes(snapshot.totalBytesSent))
    );

    // 5. 底部系统信息
    m_osInfoLabel->setText(snapshot.osVersion + QStringLiteral(" (") + snapshot.cpuArchitecture + QStringLiteral(")"));
    m_cpuModelLabel->setText(snapshot.cpuName);
    m_hostUserLabel->setText(snapshot.hostName + QStringLiteral(" / ") + snapshot.userName);
    m_uptimeLabel->setText(SystemProvider::formatUptime(snapshot.uptimeSeconds));
}
