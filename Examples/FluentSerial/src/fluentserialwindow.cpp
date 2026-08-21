#include "fluentserialwindow.h"

#include "core/serialengine.h"
#include "widgets/serialcontrolbar.h"
#include "pages/monitorpage.h"
#include "pages/quickcommandspage.h"
#include "pages/waveformpage.h"
#include "pages/toolboxpage.h"
#include "pages/settingspage.h"

#include "exnavtreewidget.h"
#include "exstackedwidget.h"
#include "exwinuinavigationview.h"

#ifdef FLUENTSERIAL_ENABLE_FRAMELESS
#include "fluenttitlebar.h"
#include "fluentwindowframe.h"
#endif

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>

FluentSerialWindow::FluentSerialWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("FluentSerial 现代串口调试与协议工作台"));
    setWindowIcon(QIcon(QStringLiteral(":/appicon.ico")));
    resize(1200, 800);
    setMinimumSize(960, 620);

#ifdef FLUENTSERIAL_ENABLE_FRAMELESS
    setAttribute(Qt::WA_DontCreateNativeAncestors);
#endif

    setupUi();

#ifdef FLUENTSERIAL_ENABLE_FRAMELESS
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
}

void FluentSerialWindow::setupUi()
{
    auto *centralWidget = new QWidget(this);
    centralWidget->setBackgroundRole(QPalette::Window);
    setCentralWidget(centralWidget);

    auto *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(12, 8, 12, 12);
    rootLayout->setSpacing(10);

    // 1. 顶部常驻串口控制栏
    m_controlBar = new SerialControlBar(centralWidget);
    rootLayout->addWidget(m_controlBar);

    // 2. 下方主体：左侧导航 + 右侧多页面路由
    auto *bodyLayout = new QHBoxLayout();
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(10);

    auto *navPane = new QFrame(centralWidget);
    navPane->setFrameShape(QFrame::NoFrame);
    navPane->setFixedWidth(240);
    navPane->setAutoFillBackground(true);

    auto *navLayout = new QVBoxLayout(navPane);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(0);

    m_navigation = new ExWinUINavigationView(navPane);
    navLayout->addWidget(m_navigation, 1);

    // 右侧页面 Stack
    m_pages = new ExStackedWidget(centralWidget);
    m_pages->setFrameShape(QFrame::NoFrame);
    m_pages->setVerticalMode(true);
    m_pages->setSpeed(220);
    m_pages->setAnimation(QEasingCurve::OutCubic);

    m_monitorPage = new MonitorPage(m_pages);
    m_quickCmdPage = new QuickCommandsPage(m_pages);
    m_waveformPage = new WaveformPage(m_pages);
    m_toolboxPage = new ToolboxPage(m_pages);
    m_settingsPage = new SettingsPage(m_pages);

    connect(m_settingsPage, &SettingsPage::appearanceChanged, this, &FluentSerialWindow::refreshAppearance);

    m_pages->addWidget(m_monitorPage);     // index 0: Monitor
    m_pages->addWidget(m_quickCmdPage);    // index 1: QuickCmd
    m_pages->addWidget(m_waveformPage);    // index 2: Waveform
    m_pages->addWidget(m_toolboxPage);     // index 3: Toolbox
    m_pages->addWidget(m_settingsPage);    // index 4: Settings

    bodyLayout->addWidget(navPane);
    bodyLayout->addWidget(m_pages, 1);
    rootLayout->addLayout(bodyLayout, 1);

    setupNavigation();
}

void FluentSerialWindow::setupNavigation()
{
    m_navigation->setStackedWidget(m_pages);

    m_navigation->addMainNavigationItem(QStringLiteral("基础收发监控"), MonitorPageIndex, QStringLiteral("\uE8BD"));
    m_navigation->addMainNavigationItem(QStringLiteral("多指令与自动化"), QuickCmdPageIndex, QStringLiteral("\uE756"));
    m_navigation->addMainNavigationItem(QStringLiteral("实时波形示波器"), WaveformPageIndex, QStringLiteral("\uEC16"));
    m_navigation->addMainNavigationItem(QStringLiteral("协议与计算工具箱"), ToolboxPageIndex, QStringLiteral("\uE8EF"));

    m_navigation->addFooterNavigationItem(QStringLiteral("设置与关于"), SettingsPageIndex, QStringLiteral("\uE713"));

    if (ExNavTreeWidget *mainNav = m_navigation->mainNavView()) {
        mainNav->setExpandedWidth(230);
        mainNav->setCompactWidth(52);
    }
    if (ExNavTreeWidget *footerNav = m_navigation->footerNavView()) {
        footerNav->setExpandedWidth(230);
        footerNav->setCompactWidth(52);
    }

    m_navigation->setNavigationExpanded(true, false);
    m_navigation->setSelectedPageIndex(MonitorPageIndex);
}

void FluentSerialWindow::refreshAppearance()
{
    if (m_navigation->mainNavView()) {
        m_navigation->mainNavView()->refreshNavigationIcons();
    }
    if (m_navigation->footerNavView()) {
        m_navigation->footerNavView()->refreshNavigationIcons();
    }
#ifdef FLUENTSERIAL_ENABLE_FRAMELESS
    if (FluentTitleBar *titleBar = m_windowFrame ? m_windowFrame->titleBar() : nullptr) {
        titleBar->setThemeDark(qApp->property("_q_colorscheme").toInt() == 1);
    }
#endif
    update();
}

void FluentSerialWindow::closeEvent(QCloseEvent *event)
{
    SerialEngine::instance().closePort();
    QMainWindow::closeEvent(event);
}
