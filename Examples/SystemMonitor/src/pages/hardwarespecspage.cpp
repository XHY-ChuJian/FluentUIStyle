#include "hardwarespecspage.h"

#include "exexpander.h"
#include "extimeline.h"

#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QScreen>
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

HardwareSpecsPage::HardwareSpecsPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    connect(&SystemProvider::instance(), &SystemProvider::snapshotUpdated,
            this, &HardwareSpecsPage::onSnapshotUpdated);

    onSnapshotUpdated(SystemProvider::instance().lastSnapshot());
}

void HardwareSpecsPage::setupUi()
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

    auto *titleLabel = new QLabel(QStringLiteral("硬件规格与系统事件日志"), container);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // 1. 硬件规格折叠面板
    auto *specsCard = createCardFrame(container);
    auto *specsLayout = new QVBoxLayout(specsCard);
    specsLayout->setContentsMargins(18, 16, 18, 18);
    specsLayout->setSpacing(10);

    specsLayout->addWidget(createCardHeader(QStringLiteral("🖥️ 系统与设备详细参数"), specsCard));

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(10);

    grid->addWidget(new QLabel(QStringLiteral("操作系统名称:"), specsCard), 0, 0);
    m_osLabel = new QLabel(QStringLiteral("-"), specsCard);
    grid->addWidget(m_osLabel, 0, 1);

    grid->addWidget(new QLabel(QStringLiteral("系统内核版本:"), specsCard), 0, 2);
    m_kernelLabel = new QLabel(QStringLiteral("-"), specsCard);
    grid->addWidget(m_kernelLabel, 0, 3);

    grid->addWidget(new QLabel(QStringLiteral("处理器完整型号:"), specsCard), 1, 0);
    m_cpuNameLabel = new QLabel(QStringLiteral("-"), specsCard);
    grid->addWidget(m_cpuNameLabel, 1, 1);

    grid->addWidget(new QLabel(QStringLiteral("处理器指令架构:"), specsCard), 1, 2);
    m_cpuArchLabel = new QLabel(QStringLiteral("-"), specsCard);
    grid->addWidget(m_cpuArchLabel, 1, 3);

    grid->addWidget(new QLabel(QStringLiteral("逻辑处理线程数:"), specsCard), 2, 0);
    m_coreCountLabel = new QLabel(QStringLiteral("-"), specsCard);
    grid->addWidget(m_coreCountLabel, 2, 1);

    grid->addWidget(new QLabel(QStringLiteral("本次系统启动时间:"), specsCard), 2, 2);
    m_bootTimeLabel = new QLabel(QStringLiteral("-"), specsCard);
    grid->addWidget(m_bootTimeLabel, 2, 3);

    grid->addWidget(new QLabel(QStringLiteral("主显示器分辨率:"), specsCard), 3, 0);
    m_screenInfoLabel = new QLabel(QStringLiteral("-"), specsCard);
    grid->addWidget(m_screenInfoLabel, 3, 1);

    specsLayout->addLayout(grid);
    mainLayout->addWidget(specsCard);

    // 2. 系统性能与事件时间轴
    auto *timelineCard = createCardFrame(container);
    auto *timelineLayout = new QVBoxLayout(timelineCard);
    timelineLayout->setContentsMargins(18, 16, 18, 18);
    timelineLayout->setSpacing(12);

    timelineLayout->addWidget(createCardHeader(QStringLiteral("🕒 系统性能与状态事件时间轴"), timelineCard));

    m_timeline = new ExTimeline(timelineCard);
    m_timeline->setLayoutMode(ExTimeline::ContentOnRight);
    m_timeline->setMinimumHeight(240);

    timelineLayout->addWidget(m_timeline);
    mainLayout->addWidget(timelineCard);

    // 初始添加系统启动与监视器启动事件
    const auto &snap = SystemProvider::instance().lastSnapshot();
    if (snap.bootTime.isValid()) {
        addTimelineEvent(
            QStringLiteral("系统开机启动"),
            QStringLiteral("操作系统加载完成并初始化系统时钟服务。"),
            (int)ExTimelineEvent::Completed
        );
    }
    addTimelineEvent(
        QStringLiteral("监控箱服务就绪"),
        QStringLiteral("FluentSysMon 性能采集模块已启动，开始实时捕获硬件指标。"),
        (int)ExTimelineEvent::Current
    );

    mainLayout->addStretch();

    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea);
}

void HardwareSpecsPage::addTimelineEvent(const QString &title, const QString &description, int status)
{
    auto *event = new ExTimelineEvent(m_timeline);
    event->setTimestamp(QDateTime::currentDateTime());
    event->setTitle(title);
    event->setDescription(description);
    event->setStatus((ExTimelineEvent::Status)status);
    m_timeline->addEvent(event);
}

void HardwareSpecsPage::onSnapshotUpdated(const SystemSnapshot &snapshot)
{
    m_osLabel->setText(snapshot.osVersion);
    m_kernelLabel->setText(snapshot.osKernel);
    m_cpuNameLabel->setText(snapshot.cpuName);
    m_cpuArchLabel->setText(snapshot.cpuArchitecture);
    m_coreCountLabel->setText(QStringLiteral("%1 逻辑线程").arg(snapshot.logicalCoreCount));
    m_bootTimeLabel->setText(snapshot.bootTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));

    if (auto *screen = QGuiApplication::primaryScreen()) {
        QRect geo = screen->geometry();
        qreal dpr = screen->devicePixelRatio();
        m_screenInfoLabel->setText(QStringLiteral("%1 × %2 (缩放: %3%)")
            .arg(geo.width())
            .arg(geo.height())
            .arg(qRound(dpr * 100)));
    }

    // 告警事件检测（CPU > 90%）
    if (snapshot.cpuUsageTotal >= 90.0) {
        if (!m_highCpuWarned) {
            m_highCpuWarned = true;
            addTimelineEvent(
                QStringLiteral("⚠️ CPU 处于高负载状态"),
                QStringLiteral("当前 CPU 总使用率已达 %1%，请留意系统散热与后台进程。").arg(snapshot.cpuUsageTotal, 0, 'f', 1),
                (int)ExTimelineEvent::Warning
            );
        }
    } else if (snapshot.cpuUsageTotal < 75.0) {
        m_highCpuWarned = false;
    }

    // 内存 > 90%
    if (snapshot.memoryUsagePercent >= 90.0) {
        if (!m_highMemWarned) {
            m_highMemWarned = true;
            addTimelineEvent(
                QStringLiteral("⚠️ 物理内存严重不足"),
                QStringLiteral("当前物理内存占用率达 %1%，可用空间低于 10%。").arg(snapshot.memoryUsagePercent, 0, 'f', 1),
                (int)ExTimelineEvent::Warning
            );
        }
    } else if (snapshot.memoryUsagePercent < 80.0) {
        m_highMemWarned = false;
    }
}
