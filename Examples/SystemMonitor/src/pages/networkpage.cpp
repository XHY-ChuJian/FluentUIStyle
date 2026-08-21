#include "networkpage.h"

#include "exexpander.h"

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

NetworkPage::NetworkPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    connect(&SystemProvider::instance(), &SystemProvider::snapshotUpdated,
            this, &NetworkPage::onSnapshotUpdated);

    onSnapshotUpdated(SystemProvider::instance().lastSnapshot());
}

void NetworkPage::setupUi()
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

    auto *titleLabel = new QLabel(QStringLiteral("网络流量与适配器监控"), container);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // 1. 实时吞吐速率卡片
    auto *speedCard = createCardFrame(container);
    auto *speedLayout = new QVBoxLayout(speedCard);
    speedLayout->setContentsMargins(18, 16, 18, 18);
    speedLayout->setSpacing(12);

    speedLayout->addWidget(createCardHeader(QStringLiteral("🌐 实时网络吞吐量 (Throughput)"), speedCard));

    auto *downHead = new QHBoxLayout;
    downHead->addWidget(new QLabel(QStringLiteral("⬇️ 实时下载速率:"), speedCard));
    downHead->addStretch();
    m_downSpeedLabel = new QLabel(QStringLiteral("0.0 KB/s"), speedCard);
    QFont bFont = m_downSpeedLabel->font();
    bFont.setBold(true);
    bFont.setPixelSize(15);
    m_downSpeedLabel->setFont(bFont);
    downHead->addWidget(m_downSpeedLabel);

    m_downSpeedBar = new QProgressBar(speedCard);
    m_downSpeedBar->setRange(0, 100);
    m_downSpeedBar->setValue(0);
    m_downSpeedBar->setFixedHeight(10);
    m_downSpeedBar->setTextVisible(false);

    auto *upHead = new QHBoxLayout;
    upHead->addWidget(new QLabel(QStringLiteral("⬆️ 实时上传速率:"), speedCard));
    upHead->addStretch();
    m_upSpeedLabel = new QLabel(QStringLiteral("0.0 KB/s"), speedCard);
    m_upSpeedLabel->setFont(bFont);
    upHead->addWidget(m_upSpeedLabel);

    m_upSpeedBar = new QProgressBar(speedCard);
    m_upSpeedBar->setRange(0, 100);
    m_upSpeedBar->setValue(0);
    m_upSpeedBar->setFixedHeight(10);
    m_upSpeedBar->setTextVisible(false);

    speedLayout->addLayout(downHead);
    speedLayout->addWidget(m_downSpeedBar);
    speedLayout->addLayout(upHead);
    speedLayout->addWidget(m_upSpeedBar);

    auto *totalLayout = new QHBoxLayout;
    m_totalRecvLabel = new QLabel(QStringLiteral("累计下载流量: 0 B"), speedCard);
    m_totalSentLabel = new QLabel(QStringLiteral("累计上传流量: 0 B"), speedCard);
    totalLayout->addWidget(m_totalRecvLabel);
    totalLayout->addStretch();
    totalLayout->addWidget(m_totalSentLabel);
    speedLayout->addLayout(totalLayout);

    mainLayout->addWidget(speedCard);

    // 2. 适配器列表
    auto *adaptersHeaderLabel = new QLabel(QStringLiteral("🔌 网络接口与适配器详情"), container);
    QFont ahFont = adaptersHeaderLabel->font();
    ahFont.setPixelSize(16);
    ahFont.setBold(true);
    adaptersHeaderLabel->setFont(ahFont);
    mainLayout->addWidget(adaptersHeaderLabel);

    m_adaptersContainer = new QWidget(container);
    m_adaptersLayout = new QVBoxLayout(m_adaptersContainer);
    m_adaptersLayout->setContentsMargins(0, 0, 0, 0);
    m_adaptersLayout->setSpacing(12);
    mainLayout->addWidget(m_adaptersContainer);

    mainLayout->addStretch();

    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea);
}

void NetworkPage::onSnapshotUpdated(const SystemSnapshot &snapshot)
{
    m_downSpeedLabel->setText(SystemProvider::formatSpeed(snapshot.downloadSpeedBytesPerSec));
    m_upSpeedLabel->setText(SystemProvider::formatSpeed(snapshot.uploadSpeedBytesPerSec));

    int downPct = qBound(0, (int)(snapshot.downloadSpeedBytesPerSec * 100.0 / (50.0 * 1024 * 1024)), 100);
    int upPct = qBound(0, (int)(snapshot.uploadSpeedBytesPerSec * 100.0 / (10.0 * 1024 * 1024)), 100);
    m_downSpeedBar->setValue(downPct);
    m_upSpeedBar->setValue(upPct);

    m_totalRecvLabel->setText(QStringLiteral("累计下载: %1").arg(SystemProvider::formatBytes(snapshot.totalBytesReceived)));
    m_totalSentLabel->setText(QStringLiteral("累计上传: %1").arg(SystemProvider::formatBytes(snapshot.totalBytesSent)));

    if (snapshot.adapters.size() != m_lastAdapterCount) {
        m_lastAdapterCount = snapshot.adapters.size();
        rebuildAdaptersList(snapshot.adapters);
    }
}

void NetworkPage::rebuildAdaptersList(const QList<NetworkAdapterInfo> &adapters)
{
    QLayoutItem *child = nullptr;
    while ((child = m_adaptersLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    if (adapters.isEmpty()) {
        auto *emptyLabel = new QLabel(QStringLiteral("未检测到活动网络接口"), m_adaptersContainer);
        m_adaptersLayout->addWidget(emptyLabel);
        return;
    }

    for (const auto &ad : adapters) {
        auto *expander = new ExExpander(m_adaptersContainer);
        QString statusText = ad.isUp ? QStringLiteral("🟢 已连接") : QStringLiteral("⚪ 未连接");
        expander->setHeader(QStringLiteral("%1  (%2)  —  %3")
            .arg(ad.name)
            .arg(ad.description.isEmpty() ? QStringLiteral("网络接口") : ad.description)
            .arg(statusText));
        expander->setExpanded(ad.isUp);

        auto *contentWidget = new QWidget;
        auto *cLayout = new QGridLayout(contentWidget);
        cLayout->setContentsMargins(16, 12, 16, 12);
        cLayout->setSpacing(8);

        cLayout->addWidget(new QLabel(QStringLiteral("适配器名称:"), contentWidget), 0, 0);
        cLayout->addWidget(new QLabel(ad.name, contentWidget), 0, 1);

        cLayout->addWidget(new QLabel(QStringLiteral("硬件 MAC:"), contentWidget), 0, 2);
        cLayout->addWidget(new QLabel(ad.macAddress.isEmpty() ? QStringLiteral("未知") : ad.macAddress, contentWidget), 0, 3);

        cLayout->addWidget(new QLabel(QStringLiteral("IPv4 地址:"), contentWidget), 1, 0);
        cLayout->addWidget(new QLabel(ad.ipAddress.isEmpty() ? QStringLiteral("未分配") : ad.ipAddress, contentWidget), 1, 1);

        cLayout->addWidget(new QLabel(QStringLiteral("接口状态:"), contentWidget), 1, 2);
        cLayout->addWidget(new QLabel(statusText, contentWidget), 1, 3);

        expander->addContentWidget(contentWidget);
        m_adaptersLayout->addWidget(expander);
    }
}
