#include "mainwindow.h"

#include "common/fluenthelpers.h"
#include "exnavtreewidget.h"
#include "exstackedwidget.h"
#include "exwinuinavigationview.h"
#include "pages/alarmpage.h"
#include "pages/clockpages.h"
#include "pages/settingspage.h"
#include "pages/timerpage.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("时钟"));
    resize(1460, 900);
    setMinimumSize(1050, 680);
    setupUi();
}

void MainWindow::setupUi()
{
    auto* centralWidget = new QWidget(this);
    centralWidget->setBackgroundRole(QPalette::Base);
    centralWidget->setProperty("MainBackground", true);
    setCentralWidget(centralWidget);

    auto* rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* navigationPane = new QFrame(centralWidget);
    navigationPane->setFrameShape(QFrame::NoFrame);
    navigationPane->setFixedWidth(254);
    navigationPane->setAutoFillBackground(true);

    auto* navigationLayout = new QVBoxLayout(navigationPane);
    navigationLayout->setContentsMargins(0, 0, 0, 0);
    navigationLayout->setSpacing(0);

    // auto* brandRow = new QWidget(navigationPane);
    // brandRow->setFixedHeight(54);
    // auto* brandLayout = new QHBoxLayout(brandRow);
    // brandLayout->setContentsMargins(20, 0, 8, 0);
    // brandLayout->setSpacing(12);
    // navigationLayout->addWidget(brandRow);

    m_navigation = new ExWinUINavigationView(navigationPane);
    navigationLayout->addWidget(m_navigation, 1);
    m_pages = new ExStackedWidget(centralWidget);
    m_pages->setVerticalMode(true);
    m_pages->setSpeed(230);
    m_pages->setAnimation(QEasingCurve::OutCubic);

    m_pages->addWidget(new FocusPage(m_pages));
    m_pages->addWidget(new TimerPage(m_pages));
    m_pages->addWidget(new AlarmPage(m_pages));
    m_pages->addWidget(new StopwatchPage(m_pages));
    m_pages->addWidget(new WorldClockPage(m_pages));
    m_pages->addWidget(new AccountPage(m_pages));
    auto* settingsPage = new SettingsPage(m_pages);
    m_pages->addWidget(settingsPage);

    rootLayout->addWidget(navigationPane);
    rootLayout->addWidget(m_pages, 1);

    setupNavigation();
    connect(settingsPage,
            &SettingsPage::appearanceChanged,
            this,
            &MainWindow::refreshNavigationIcons);
}

void MainWindow::setupNavigation()
{
    m_navigation->setStackedWidget(m_pages);
    m_navigation->addMainNavigationItem(tr("专注时段"),
                                        FocusPageIndex,
                                        QStringLiteral("\uE9D9"));
    m_navigation->addMainNavigationItem(tr("计时器"),
                                        TimerPageIndex,
                                        QStringLiteral("\uE121"));
    m_navigation->addMainNavigationItem(tr("闹钟"),
                                        AlarmPageIndex,
                                        QStringLiteral("\uE7ED"));
    m_navigation->addMainNavigationItem(tr("秒表"),
                                        StopwatchPageIndex,
                                        QStringLiteral("\uE916"));
    m_navigation->addMainNavigationItem(tr("世界时钟"),
                                        WorldClockPageIndex,
                                        QStringLiteral("\uE909"));
    m_navigation->addFooterNavigationItem(tr("登录"),
                                          AccountPageIndex,
                                          QStringLiteral("\uE77B"));
    m_navigation->addFooterNavigationItem(tr("设置"),
                                          SettingsPageIndex,
                                          QStringLiteral("\uE713"));

    if (ExNavTreeWidget* mainNav = m_navigation->mainNavView())
    {
        mainNav->setExpandedWidth(242);
        mainNav->setCompactWidth(52);
    }
    if (ExNavTreeWidget* footerNav = m_navigation->footerNavView())
    {
        footerNav->setExpandedWidth(242);
        footerNav->setCompactWidth(52);
    }
    m_navigation->setNavigationExpanded(true, false);
    m_navigation->setSelectedPageIndex(TimerPageIndex);
}

void MainWindow::refreshNavigationIcons()
{
    if (m_navigation->mainNavView())
        m_navigation->mainNavView()->refreshNavigationIcons();
    if (m_navigation->footerNavView())
        m_navigation->footerNavView()->refreshNavigationIcons();
    update();
}
