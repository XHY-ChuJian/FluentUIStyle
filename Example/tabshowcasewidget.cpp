#include "tabshowcasewidget.h"

#include "font-icon/fonticon.h"
#include "../FluentUI3Style/fluentui3styleproperties.h"

#include <extabwidget.h>

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QApplication>
#include <QEasingCurve>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

static constexpr int kTabBarFluentIconPx = 16;

static void applySectionHeadingFont(QLabel *label, int pixelSize)
{
    QFont f = label->font();
    f.setBold(true);
    f.setPixelSize(pixelSize);
    label->setFont(f);
}

static void applySectionDescriptionStyle(QLabel *label)
{
    label->setStyleSheet("color: gray; font-size: 12px;");
}

static void styleTabPageLabel(QLabel *page, const QColor &background)
{
    QPalette pal = page->palette();
    pal.setColor(QPalette::Window, background);
    pal.setColor(QPalette::WindowText, Qt::black);
    pal.setColor(QPalette::Text, Qt::black);
    page->setAutoFillBackground(true);
    page->setPalette(pal);
}

TabShowcaseWidget::TabShowcaseWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(11, 11, 11, 11);
    mainLayout->setSpacing(15);

    QLabel *lab = new QLabel("TabBar多种样式示例");
    {
        QFont f = lab->font();
        f.setBold(true);
        f.setPointSize(18);
        lab->setFont(f);
    }
    mainLayout->addWidget(lab);

    setupPivotTabs(mainLayout);
    setupSegmentedTabs(mainLayout);
    setupPillTabs(mainLayout);
    setupCapsuleTabs(mainLayout);
    setupNavigationTabs(mainLayout);
    mainLayout->addStretch();
}

void TabShowcaseWidget::setupPivotTabs(QVBoxLayout *mainLayout)
{
    QWidget *pivotWidget = createTabWidgetContainer();
    QVBoxLayout *pivotLayout = static_cast<QVBoxLayout *>(pivotWidget->layout());

    addTabBarSection(pivotLayout, "Pivot Grow TabBar", "特点：选中时会有一个生长动画效果。", Pivot_Grow);
    addTabBarSection(pivotLayout, "Pivot Slide TabBar", "特点：选中时会有一个滑动动画效果。", Pivot_Slide);
    addTabBarSection(pivotLayout, "Pivot Stretch TabBar", "特点：选中时会有一个拉伸动画效果。", Pivot_Stretch);

    pivotLayout->addStretch();
    mainLayout->addWidget(pivotWidget, 1);
}

void TabShowcaseWidget::setupSegmentedTabs(QVBoxLayout *mainLayout)
{
    QWidget *segmentedWidget = createTabWidgetContainer();
    QVBoxLayout *segmentedLayout = static_cast<QVBoxLayout *>(segmentedWidget->layout());

    addTabBarSection(segmentedLayout, "Segmented Slide TabBar", "特点：Segmented风格，选中时会有一个滑动动画效果。", Segmented_Slide, &m_segmentedBar);
    addTabBarSection(segmentedLayout, "Segmented Fade TabBar", "特点：选中时会有一个淡入淡出动画效果。", Segmented_Fade, &m_segmentedFadeBar);
    addTabBarSection(segmentedLayout,
                     "Segmented WinUI3 TabBar",
                     "特点：Segmented风格，使用更接近 WinUI3 的选中指示器效果。",
                     Segmented_WinUI3,
                     &m_winui3Bar);

    segmentedLayout->addStretch();
    mainLayout->addWidget(segmentedWidget, 1);
}

void TabShowcaseWidget::setupPillTabs(QVBoxLayout *mainLayout)
{
    QWidget *pillWidget = createTabWidgetContainer();
    QVBoxLayout *pillLayout = static_cast<QVBoxLayout *>(pillWidget->layout());

    QLabel *pillLabel = new QLabel("Pill TabBar");
    applySectionHeadingFont(pillLabel, 14);
    pillLayout->addWidget(pillLabel);

    QTabBar *pillBar = new QTabBar();
    pillBar->setTabsClosable(true);
    pillBar->setExpanding(false);
    pillBar->setProperty(TabBarStyleProperty, PillTabs);
    pillBar->addTab("Home");
    pillBar->addTab("Search");
    pillBar->addTab("Settings");
    pillBar->addTab("Help");
    pillBar->addTab("About");
    pillLayout->addWidget(pillBar);

    pillLayout->addStretch();
    mainLayout->addWidget(pillWidget, 1);
}

void TabShowcaseWidget::setupCapsuleTabs(QVBoxLayout *mainLayout)
{
    QWidget *capsuleWidget = createTabWidgetContainer();
    QVBoxLayout *capsuleLayout = static_cast<QVBoxLayout *>(capsuleWidget->layout());

    QLabel *capsuleLabel = new QLabel("Capsule TabBar");
    applySectionHeadingFont(capsuleLabel, 14);
    QLabel *capsuleDescLabel = new QLabel("特点：浏览器标签样式。");
    applySectionDescriptionStyle(capsuleDescLabel);
    capsuleLayout->addWidget(capsuleLabel);
    capsuleLayout->addWidget(capsuleDescLabel);

    m_capsuleTabWidget = new ExTabWidget();
    m_capsuleTabWidget->setMinimumHeight(200);
    m_capsuleTabWidget->setTabsClosable(true);
    m_capsuleTabWidget->setMovable(true);

    QTabBar *capTabBar = m_capsuleTabWidget->tabBar();
    capTabBar->setAttribute(Qt::WA_StyledBackground, true);
    capTabBar->setAutoFillBackground(false);
    capTabBar->setExpanding(false);
    capTabBar->setProperty("TextAlign", static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft));
    capTabBar->setProperty(TabBarStyleProperty, Capsule);
    capTabBar->setDrawBase(false);

    const QStringList pageNames = {"Home", "Search", "Settings", "Help", "About"};
    const QStringList fullNames = {"Home Page", "Search Page", "Settings Page", "Help Page", "About Page"};
    const QList<QColor> pageColors = {
        QColor(255, 228, 225), QColor(224, 255, 255), QColor(240, 255, 240), QColor(255, 250, 205), QColor(230, 230, 250)};

    for (int i = 0; i < pageNames.size(); ++i)
    {
        QLabel *page = new QLabel(fullNames[i]);
        page->setAlignment(Qt::AlignCenter);
        styleTabPageLabel(page, pageColors[i]);
        m_capsuleTabWidget->addTab(page, pageNames[i]);
    }

    capsuleLayout->addWidget(m_capsuleTabWidget, 1);
    mainLayout->addWidget(capsuleWidget, 1);
}

void TabShowcaseWidget::setupNavigationTabs(QVBoxLayout *mainLayout)
{
    QWidget *navigationWidget = createTabWidgetContainer();
    QVBoxLayout *navigationLayout = static_cast<QVBoxLayout *>(navigationWidget->layout());

    QLabel *navigationLabel = new QLabel("Navigation TabBar");
    applySectionHeadingFont(navigationLabel, 14);
    QLabel *navigationDescLabel = new QLabel("特点：适合用于侧边栏的导航菜单，选项卡垂直排列，选中时指示器有个变长效果");
    applySectionDescriptionStyle(navigationDescLabel);
    navigationLayout->addWidget(navigationLabel);
    navigationLayout->addWidget(navigationDescLabel);

    QHBoxLayout *bodyLayout = new QHBoxLayout();
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(2);

    m_navigationTabWidget = new ExTabWidget();
    m_navigationTabWidget->setTabPosition(QTabWidget::West);
    m_navigationTabWidget->setVerticalMode(true);
    m_navigationTabWidget->setSpeed(220);
    m_navigationTabWidget->setAnimation(QEasingCurve::OutCubic);
    m_navigationTabWidget->setMinimumHeight(300);

    QTabBar *navTabBar = m_navigationTabWidget->tabBar();
    navTabBar->setAttribute(Qt::WA_StyledBackground, true);
    navTabBar->setShape(QTabBar::RoundedWest);
    navTabBar->setDrawBase(false);
    m_navigationTabWidget->setMovable(false);
    navTabBar->setExpanding(false);
    navTabBar->setProperty("TextAlign", static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft));
    navTabBar->setProperty(TabBarStyleProperty, Navigation);

    const QStringList navFullNames = {"Overview Page", "Files Page", "History Page", "Insights Page", "Settings Page"};
    const QStringList navNames = {"Overview", "Files", "History", "Insights", "Settings"};
    const QList<QColor> navPageColors = {
        QColor(244, 248, 255), QColor(240, 251, 246), QColor(255, 248, 238), QColor(248, 243, 255), QColor(245, 245, 245)};

    for (int i = 0; i < navFullNames.size(); ++i)
    {
        QLabel *page = new QLabel(navFullNames[i]);
        page->setAlignment(Qt::AlignCenter);
        page->setMinimumHeight(220);
        styleTabPageLabel(page, navPageColors[i]);
        m_navigationTabWidget->addTab(page, navNames[i]);
    }

    bodyLayout->addWidget(m_navigationTabWidget, 1);
    navigationLayout->addLayout(bodyLayout);
    mainLayout->addWidget(navigationWidget, 1);
}

QWidget *TabShowcaseWidget::createTabWidgetContainer()
{
    QWidget *widget = new QWidget();
    widget->setProperty("isCard", true);
    widget->setAttribute(Qt::WA_StyledBackground);

    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    return widget;
}

void TabShowcaseWidget::addTabBarSection(QVBoxLayout *layout,
                                         const QString &title,
                                         const QString &description,
                                         int tabStyle,
                                         QTabBar **outTabBar)
{
    QLabel *titleLabel = new QLabel(title);
    applySectionHeadingFont(titleLabel, 14);
    layout->addWidget(titleLabel);

    if (!description.isEmpty())
    {
        QLabel *descLabel = new QLabel(description);
        applySectionDescriptionStyle(descLabel);
        layout->addWidget(descLabel);
    }

    QTabBar *tabBar = new QTabBar();
    tabBar->setExpanding(false);
    tabBar->setProperty(TabBarStyleProperty, tabStyle);
    // Match style pivotIndicatorPreferredWidth (32) + horizontal margins (Stretch uses 20 each side)
    if (tabStyle == Pivot_Grow || tabStyle == Pivot_Slide || tabStyle == Pivot_Stretch)
    {
        QFont pivotFont = tabBar->font();
        pivotFont.setPixelSize(15);
        pivotFont.setWeight(QFont::Bold);
        tabBar->setFont(pivotFont);
    }
    else if (tabStyle == Segmented_Slide || tabStyle == Segmented_Fade || tabStyle == Segmented_WinUI3)
    {
        // tabBar->setAttribute(Qt::WA_StyledBackground);
    }
    tabBar->addTab("Home");
    tabBar->addTab("Search");
    tabBar->addTab("Settings");
    tabBar->addTab("Help");
    tabBar->addTab("About");
    layout->addWidget(tabBar);

    if (outTabBar)
    {
        *outTabBar = tabBar;
    }
}

void TabShowcaseWidget::updateTabIcons()
{
    const QColor iconColor = QApplication::palette().color(QPalette::WindowText);

    static const int capsuleIcons[] = {
        static_cast<int>(SegoeIcon::Home),
        static_cast<int>(SegoeIcon::Search),
        static_cast<int>(SegoeIcon::Settings),
        static_cast<int>(SegoeIcon::Help),
        static_cast<int>(SegoeIcon::Info),
    };
    if (m_capsuleTabWidget)
    {
        for (int i = 0; i < int(sizeof(capsuleIcons) / sizeof(capsuleIcons[0])) && i < m_capsuleTabWidget->count(); ++i)
        {
            m_capsuleTabWidget->setTabIcon(
                i, FONTICON->getIcon(capsuleIcons[i]));
        }
    }

    static const int segmentedIcons[] = {
        static_cast<int>(SegoeIcon::CompanionApp),
        static_cast<int>(SegoeIcon::PlayerSettings),
        static_cast<int>(SegoeIcon::Robot),
        static_cast<int>(SegoeIcon::RingerSilent),
        static_cast<int>(SegoeIcon::TrafficCongestionSolid),
    };
    const int nSeg = int(sizeof(segmentedIcons) / sizeof(segmentedIcons[0]));
    if (m_segmentedBar)
    {
        for (int i = 0; i < nSeg && i < m_segmentedBar->count(); ++i)
        {
            m_segmentedBar->setTabIcon(i, FONTICON->getIcon(segmentedIcons[i]));
        }
    }
    if (m_segmentedFadeBar)
    {
        for (int i = 0; i < nSeg && i < m_segmentedFadeBar->count(); ++i)
        {
            m_segmentedFadeBar->setTabIcon(i, FONTICON->getIcon(segmentedIcons[i]));
        }
    }
    if (m_winui3Bar)
    {
        for (int i = 0; i < nSeg && i < m_winui3Bar->count(); ++i)
        {
            m_winui3Bar->setTabIcon(i, FONTICON->getIcon(segmentedIcons[i]));
        }
    }

    static const int navigationIcons[] = {
        static_cast<int>(SegoeIcon::CompanionApp),
        static_cast<int>(SegoeIcon::Folder),
        static_cast<int>(SegoeIcon::History),
        static_cast<int>(SegoeIcon::Unknown),
        static_cast<int>(SegoeIcon::Settings),
    };
    if (m_navigationTabWidget)
    {
        for (int i = 0; i < int(sizeof(navigationIcons) / sizeof(navigationIcons[0])) && i < m_navigationTabWidget->count();
             ++i)
        {
            m_navigationTabWidget->setTabIcon(
                i, FONTICON->getIcon(navigationIcons[i]));
        }
    }
}
