#include "storagepage.h"

#include "exexpander.h"

#include <QDesktopServices>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
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

StoragePage::StoragePage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    connect(&SystemProvider::instance(), &SystemProvider::snapshotUpdated,
            this, &StoragePage::onSnapshotUpdated);

    onSnapshotUpdated(SystemProvider::instance().lastSnapshot());
}

void StoragePage::setupUi()
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

    auto *titleLabel = new QLabel(QStringLiteral("磁盘与存储驱动器监控"), container);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // 1. 全盘总览卡片
    auto *summaryCard = createCardFrame(container);
    auto *summaryLayout = new QVBoxLayout(summaryCard);
    summaryLayout->setContentsMargins(18, 16, 18, 18);
    summaryLayout->setSpacing(10);

    summaryLayout->addWidget(createCardHeader(QStringLiteral("💾 存储总体利用率"), summaryCard));

    auto *barHeadLayout = new QHBoxLayout;
    barHeadLayout->addWidget(new QLabel(QStringLiteral("全盘总体空间:"), summaryCard));
    barHeadLayout->addStretch();
    m_totalSpaceSummaryLabel = new QLabel(QStringLiteral("计算中..."), summaryCard);
    QFont boldFont = m_totalSpaceSummaryLabel->font();
    boldFont.setBold(true);
    m_totalSpaceSummaryLabel->setFont(boldFont);
    barHeadLayout->addWidget(m_totalSpaceSummaryLabel);

    m_totalDiskBar = new QProgressBar(summaryCard);
    m_totalDiskBar->setRange(0, 100);
    m_totalDiskBar->setValue(0);
    m_totalDiskBar->setFixedHeight(14);

    summaryLayout->addLayout(barHeadLayout);
    summaryLayout->addWidget(m_totalDiskBar);
    mainLayout->addWidget(summaryCard);

    // 2. 各驱动器列表容器
    auto *listHeaderLabel = new QLabel(QStringLiteral("📁 各驱动器与挂载点明细"), container);
    QFont hFont = listHeaderLabel->font();
    hFont.setPixelSize(16);
    hFont.setBold(true);
    listHeaderLabel->setFont(hFont);
    mainLayout->addWidget(listHeaderLabel);

    m_disksListContainer = new QWidget(container);
    m_disksListLayout = new QVBoxLayout(m_disksListContainer);
    m_disksListLayout->setContentsMargins(0, 0, 0, 0);
    m_disksListLayout->setSpacing(12);

    mainLayout->addWidget(m_disksListContainer);
    mainLayout->addStretch();

    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea);
}

void StoragePage::onSnapshotUpdated(const SystemSnapshot &snapshot)
{
    // 更新总览
    m_totalSpaceSummaryLabel->setText(
        QStringLiteral("已用: %1 / 总计: %2 (%3%)")
            .arg(SystemProvider::formatBytes(snapshot.totalDiskUsed))
            .arg(SystemProvider::formatBytes(snapshot.totalDiskSpace))
            .arg(snapshot.totalDiskUsagePercent, 0, 'f', 1)
    );
    m_totalDiskBar->setValue(qRound(snapshot.totalDiskUsagePercent));

    // 检查盘符列表是否有变动
    QStringList currentPaths;
    for (const auto &d : snapshot.disks) {
        currentPaths << d.rootPath;
    }

    if (currentPaths != m_lastDiskPaths) {
        m_lastDiskPaths = currentPaths;
        rebuildDiskList(snapshot.disks);
    }
}

void StoragePage::rebuildDiskList(const QList<DiskInfo> &disks)
{
    // 清空现有控件
    QLayoutItem *child = nullptr;
    while ((child = m_disksListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    if (disks.isEmpty()) {
        auto *emptyLabel = new QLabel(QStringLiteral("未检测到有效存储驱动器"), m_disksListContainer);
        m_disksListLayout->addWidget(emptyLabel);
        return;
    }

    for (const auto &d : disks) {
        auto *expander = new ExExpander(m_disksListContainer);
        expander->setHeader(QStringLiteral("%1 (%2)  —  已用: %3 / %4 (%5%)")
            .arg(d.displayName)
            .arg(d.fileSystemType)
            .arg(SystemProvider::formatBytes(d.usedBytes))
            .arg(SystemProvider::formatBytes(d.totalBytes))
            .arg(d.usagePercent, 0, 'f', 1));
        expander->setExpanded(true);

        // 展开面板内容
        auto *contentWidget = new QWidget;
        auto *cLayout = new QVBoxLayout(contentWidget);
        cLayout->setContentsMargins(16, 12, 16, 12);
        cLayout->setSpacing(8);

        auto *bar = new QProgressBar(contentWidget);
        bar->setRange(0, 100);
        bar->setValue(qRound(d.usagePercent));
        bar->setFixedHeight(10);
        cLayout->addWidget(bar);

        auto *grid = new QGridLayout;
        grid->setSpacing(8);

        grid->addWidget(new QLabel(QStringLiteral("挂载路径:"), contentWidget), 0, 0);
        grid->addWidget(new QLabel(d.rootPath, contentWidget), 0, 1);

        grid->addWidget(new QLabel(QStringLiteral("文件系统:"), contentWidget), 0, 2);
        grid->addWidget(new QLabel(d.fileSystemType, contentWidget), 0, 3);

        grid->addWidget(new QLabel(QStringLiteral("可用空间:"), contentWidget), 1, 0);
        grid->addWidget(new QLabel(SystemProvider::formatBytes(d.availableBytes), contentWidget), 1, 1);

        grid->addWidget(new QLabel(QStringLiteral("总容量:"), contentWidget), 1, 2);
        grid->addWidget(new QLabel(SystemProvider::formatBytes(d.totalBytes), contentWidget), 1, 3);

        cLayout->addLayout(grid);

        auto *btnLayout = new QHBoxLayout;
        btnLayout->addStretch();
        auto *openFolderBtn = new QPushButton(QStringLiteral("📂 在文件管理器中打开"), contentWidget);
        openFolderBtn->setCursor(Qt::PointingHandCursor);
        QString rootP = d.rootPath;
        connect(openFolderBtn, &QPushButton::clicked, [rootP]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(rootP));
        });
        btnLayout->addWidget(openFolderBtn);
        cLayout->addLayout(btnLayout);

        expander->addContentWidget(contentWidget);
        m_disksListLayout->addWidget(expander);
    }
}
