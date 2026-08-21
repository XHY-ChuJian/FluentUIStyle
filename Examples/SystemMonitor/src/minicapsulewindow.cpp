#include "minicapsulewindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QToolButton>
#include <QVBoxLayout>

MiniCapsuleWindow::MiniCapsuleWindow(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(260, 48);

    setupUi();

    connect(&SystemProvider::instance(), &SystemProvider::snapshotUpdated,
            this, &MiniCapsuleWindow::onSnapshotUpdated);

    onSnapshotUpdated(SystemProvider::instance().lastSnapshot());
}

void MiniCapsuleWindow::setupUi()
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 10, 6);
    layout->setSpacing(8);

    // CPU 区
    auto *cpuBox = new QVBoxLayout;
    cpuBox->setSpacing(2);
    m_cpuLabel = new QLabel(QStringLiteral("CPU 0%"), this);
    QFont f = m_cpuLabel->font();
    f.setPixelSize(11);
    f.setBold(true);
    m_cpuLabel->setFont(f);
    m_cpuBar = new QProgressBar(this);
    m_cpuBar->setRange(0, 100);
    m_cpuBar->setValue(0);
    m_cpuBar->setFixedSize(46, 5);
    m_cpuBar->setTextVisible(false);
    cpuBox->addWidget(m_cpuLabel, 0, Qt::AlignCenter);
    cpuBox->addWidget(m_cpuBar, 0, Qt::AlignCenter);
    layout->addLayout(cpuBox);

    // RAM 区
    auto *memBox = new QVBoxLayout;
    memBox->setSpacing(2);
    m_memLabel = new QLabel(QStringLiteral("RAM 0%"), this);
    m_memLabel->setFont(f);
    m_memBar = new QProgressBar(this);
    m_memBar->setRange(0, 100);
    m_memBar->setValue(0);
    m_memBar->setFixedSize(46, 5);
    m_memBar->setTextVisible(false);
    memBox->addWidget(m_memLabel, 0, Qt::AlignCenter);
    memBox->addWidget(m_memBar, 0, Qt::AlignCenter);
    layout->addLayout(memBox);

    // 网络上下行区
    auto *netBox = new QVBoxLayout;
    netBox->setSpacing(1);
    m_netDownLabel = new QLabel(QStringLiteral("↓ 0K/s"), this);
    m_netUpLabel = new QLabel(QStringLiteral("↑ 0K/s"), this);
    QFont netFont = m_netDownLabel->font();
    netFont.setPixelSize(10);
    m_netDownLabel->setFont(netFont);
    m_netUpLabel->setFont(netFont);
    netBox->addWidget(m_netDownLabel);
    netBox->addWidget(m_netUpLabel);
    layout->addLayout(netBox);

    layout->addStretch();

    // 展开/还原按钮
    auto *restoreBtn = new QToolButton(this);
    restoreBtn->setText(QStringLiteral("🗖"));
    restoreBtn->setToolTip(QStringLiteral("恢复主窗口"));
    restoreBtn->setFixedSize(22, 22);
    restoreBtn->setCursor(Qt::PointingHandCursor);
    connect(restoreBtn, &QToolButton::clicked, this, &MiniCapsuleWindow::restoreMainWindowRequested);
    layout->addWidget(restoreBtn);

    // 关闭按钮
    auto *closeBtn = new QToolButton(this);
    closeBtn->setText(QStringLiteral("✕"));
    closeBtn->setToolTip(QStringLiteral("隐藏悬浮窗"));
    closeBtn->setFixedSize(22, 22);
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QToolButton::clicked, this, &QWidget::hide);
    layout->addWidget(closeBtn);
}

void MiniCapsuleWindow::onSnapshotUpdated(const SystemSnapshot &snapshot)
{
    m_cpuLabel->setText(QStringLiteral("CPU %1%").arg(qRound(snapshot.cpuUsageTotal)));
    m_cpuBar->setValue(qRound(snapshot.cpuUsageTotal));

    m_memLabel->setText(QStringLiteral("RAM %1%").arg(qRound(snapshot.memoryUsagePercent)));
    m_memBar->setValue(qRound(snapshot.memoryUsagePercent));

    m_netDownLabel->setText(QStringLiteral("↓ %1").arg(SystemProvider::formatSpeed(snapshot.downloadSpeedBytesPerSec)));
    m_netUpLabel->setText(QStringLiteral("↑ %1").arg(SystemProvider::formatSpeed(snapshot.uploadSpeedBytesPerSec)));
}

void MiniCapsuleWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MiniCapsuleWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void MiniCapsuleWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit restoreMainWindowRequested();
        event->accept();
    }
}

void MiniCapsuleWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF r = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(r, 22, 22);

    // 胶囊背景（磨砂质感深色/浅色自适应半透明）
    QColor bgColor(30, 32, 36, 215);
    QColor borderColor(255, 255, 255, 45);

    painter.fillPath(path, bgColor);
    painter.setPen(QPen(borderColor, 1.2));
    painter.drawPath(path);
}
