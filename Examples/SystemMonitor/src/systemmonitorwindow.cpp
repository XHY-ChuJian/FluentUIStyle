#include "systemmonitorwindow.h"

#include "core/systemprovider.h"
#include "exinfobarhost.h"
#include "exnavtreewidget.h"
#include "exstackedwidget.h"
#include "exwinuinavigationview.h"
#include "minicapsulewindow.h"

#include "pages/cpu_memorypage.h"
#include "pages/hardwarespecspage.h"
#include "pages/networkpage.h"
#include "pages/overviewpage.h"
#include "pages/processespage.h"
#include "pages/settingspage.h"
#include "pages/storagepage.h"

#ifdef SYSTEMMONITOR_ENABLE_FRAMELESS
#include <fluenttitlebar.h>
#include <fluentwindowframe.h>
#include <QApplication>
#include <QLineEdit>
#include <QToolButton>
#endif

#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QVBoxLayout>

SystemMonitorWindow::SystemMonitorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("FluentSysMon 现代系统性能与硬件监控箱"));
    setWindowIcon(QIcon(QStringLiteral(":/appicon.ico")));
    resize(1280, 820);
    setMinimumSize(980, 620);

#ifdef SYSTEMMONITOR_ENABLE_FRAMELESS
    setAttribute(Qt::WA_DontCreateNativeAncestors);
#endif

    setupUi();

#ifdef SYSTEMMONITOR_ENABLE_FRAMELESS
    m_windowFrame = new FluentWindowFrame(this, this);
    m_windowFrame->installChromeHeader(nullptr);
    if (FluentTitleBar *titleBar = m_windowFrame->titleBar()) {
        titleBar->searchLineEdit()->hide();
        titleBar->pinButton()->hide();
        titleBar->setThemeDark(qApp->property("_q_colorscheme").toInt() == 1);
        connect(titleBar->themeButton(), &QToolButton::clicked, this, [this]() {
            const bool isDark = qApp->property("_q_colorscheme").toInt() == 1;
            m_settingsPage->setDarkTheme(!isDark);
        });
    }
#endif

    // 初始化注册默认 InfoBarHost
    ExInfoBarHost::setDefaultTarget(this);

    // 启动全局数据采集服务
    SystemProvider::instance().start(1000);
}

SystemMonitorWindow::~SystemMonitorWindow()
{
    SystemProvider::instance().stop();
}

void SystemMonitorWindow::setupUi()
{
    auto *centralWidget = new QWidget(this);
    centralWidget->setBackgroundRole(QPalette::Window);
    setCentralWidget(centralWidget);

    auto *rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 侧边导航栏
    auto *navPane = new QFrame(centralWidget);
    navPane->setFrameShape(QFrame::NoFrame);
    navPane->setFixedWidth(240);
    navPane->setAutoFillBackground(true);

    auto *navLayout = new QVBoxLayout(navPane);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(0);

    m_navigation = new ExWinUINavigationView(navPane);
    navLayout->addWidget(m_navigation, 1);

    // 页面栈
    m_pages = new ExStackedWidget(centralWidget);
    m_pages->setFrameShape(QFrame::NoFrame);
    m_pages->setVerticalMode(true);
    m_pages->setSpeed(220);
    m_pages->setAnimation(QEasingCurve::OutCubic);

    m_overviewPage = new OverviewPage(m_pages);
    m_cpuMemoryPage = new CpuMemoryPage(m_pages);
    m_storagePage = new StoragePage(m_pages);
    m_networkPage = new NetworkPage(m_pages);
    m_processesPage = new ProcessesPage(m_pages);
    m_hardwareSpecsPage = new HardwareSpecsPage(m_pages);
    m_settingsPage = new SettingsPage(m_pages);

    m_pages->addWidget(m_overviewPage);
    m_pages->addWidget(m_cpuMemoryPage);
    m_pages->addWidget(m_storagePage);
    m_pages->addWidget(m_networkPage);
    m_pages->addWidget(m_processesPage);
    m_pages->addWidget(m_hardwareSpecsPage);
    m_pages->addWidget(m_settingsPage);

    rootLayout->addWidget(navPane);
    rootLayout->addWidget(m_pages, 1);

    setupNavigation();

    // 悬浮胶囊窗
    m_miniCapsule = new MiniCapsuleWindow(nullptr);
    connect(m_overviewPage, &OverviewPage::openMiniCapsuleRequested, this, &SystemMonitorWindow::switchToMiniCapsule);
    connect(m_miniCapsule, &MiniCapsuleWindow::restoreMainWindowRequested, this, &SystemMonitorWindow::restoreFromMiniCapsule);

    connect(m_settingsPage, &SettingsPage::appearanceChanged, this, &SystemMonitorWindow::refreshAppearance);
}

void SystemMonitorWindow::setupNavigation()
{
    m_navigation->setStackedWidget(m_pages);

    m_navigation->addMainNavigationItem(QStringLiteral("性能总览"), OverviewPageIndex, QStringLiteral("\uE80F"));
    m_navigation->addMainNavigationItem(QStringLiteral("处理器与内存"), CpuMemoryPageIndex, QStringLiteral("\uE950"));
    m_navigation->addMainNavigationItem(QStringLiteral("磁盘与存储"), StoragePageIndex, QStringLiteral("\uEDA2"));
    m_navigation->addMainNavigationItem(QStringLiteral("实时网络"), NetworkPageIndex, QStringLiteral("\uE968"));
    m_navigation->addMainNavigationItem(QStringLiteral("进程管理"), ProcessesPageIndex, QStringLiteral("\uE823"));
    m_navigation->addMainNavigationItem(QStringLiteral("硬件与日志"), HardwareSpecsPageIndex, QStringLiteral("\uEC4A"));

    m_navigation->addFooterNavigationItem(QStringLiteral("设置"), SettingsPageIndex, QStringLiteral("\uE713"));

    if (ExNavTreeWidget *mainNav = m_navigation->mainNavView()) {
        mainNav->setExpandedWidth(230);
        mainNav->setCompactWidth(52);
    }
    if (ExNavTreeWidget *footerNav = m_navigation->footerNavView()) {
        footerNav->setExpandedWidth(230);
        footerNav->setCompactWidth(52);
    }

    m_navigation->setNavigationExpanded(true, false);
    m_navigation->setSelectedPageIndex(OverviewPageIndex);
}

void SystemMonitorWindow::refreshAppearance()
{
    if (m_navigation->mainNavView()) {
        m_navigation->mainNavView()->refreshNavigationIcons();
    }
    if (m_navigation->footerNavView()) {
        m_navigation->footerNavView()->refreshNavigationIcons();
    }
#ifdef SYSTEMMONITOR_ENABLE_FRAMELESS
    if (FluentTitleBar *titleBar = m_windowFrame ? m_windowFrame->titleBar() : nullptr) {
        titleBar->setThemeDark(qApp->property("_q_colorscheme").toInt() == 1);
    }
#endif
    update();
}

void SystemMonitorWindow::switchToMiniCapsule()
{
    hide();
    if (m_miniCapsule) {
        m_miniCapsule->show();
        m_miniCapsule->raise();
        m_miniCapsule->activateWindow();
    }
}

void SystemMonitorWindow::restoreFromMiniCapsule()
{
    if (m_miniCapsule) {
        m_miniCapsule->hide();
    }
    show();
    raise();
    activateWindow();
}
