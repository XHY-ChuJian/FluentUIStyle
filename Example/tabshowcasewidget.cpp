#include "tabshowcasewidget.h"

#include "font-icon/fonticon.h"
#include "fluentui3styleproperties.h"

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

    QLabel *lab = new QLabel(tr("TabBar多种样式示例"));
    lab->setObjectName(QStringLiteral("tsw_main_title"));
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

    addTabBarSection(pivotLayout, "pivot_grow", tr("Pivot Grow TabBar"), tr("特点：选中时会有一个生长动画效果。"), Pivot_Grow);
    addTabBarSection(pivotLayout, "pivot_slide", tr("Pivot Slide TabBar"), tr("特点：选中时会有一个滑动动画效果。"), Pivot_Slide);
    addTabBarSection(pivotLayout, "pivot_stretch", tr("Pivot Stretch TabBar"), tr("特点：选中时会有一个拉伸动画效果。"), Pivot_Stretch);

    pivotLayout->addStretch();
    mainLayout->addWidget(pivotWidget, 1);
}

void TabShowcaseWidget::setupSegmentedTabs(QVBoxLayout *mainLayout)
{
    QWidget *segmentedWidget = createTabWidgetContainer();
    QVBoxLayout *segmentedLayout = static_cast<QVBoxLayout *>(segmentedWidget->layout());

    addTabBarSection(segmentedLayout,
                     "seg_slide",
                     tr("Segmented Slide TabBar"),
                     tr("特点：Segmented风格，选中时会有一个滑动动画效果。"),
                     Segmented_Slide,
                     &m_segmentedBar);
    addTabBarSection(segmentedLayout,
                     "seg_fade",
                     tr("Segmented Fade TabBar"),
                     tr("特点：选中时会有一个淡入淡出动画效果。"),
                     Segmented_Fade,
                     &m_segmentedFadeBar);
    addTabBarSection(segmentedLayout,
                     "seg_winui3",
                     tr("Segmented WinUI3 TabBar"),
                     tr("特点：Segmented风格，WinUI3 的选中指示器效果。"),
                     Segmented_WinUI3,
                     &m_winui3Bar);

    QTabBar *winUi3IconOnlyBar = new QTabBar();
    winUi3IconOnlyBar->setAttribute(Qt::WA_StyledBackground, true);
    winUi3IconOnlyBar->setDrawBase(false);
    winUi3IconOnlyBar->setExpanding(false);
    winUi3IconOnlyBar->setProperty(TabBarStyleProperty, Segmented_WinUI3);
    for (int i = 0; i < 5; ++i)
    {
        winUi3IconOnlyBar->addTab(QString());
    }
    winUi3IconOnlyBar->setCurrentIndex(0);
    winUi3IconOnlyBar->setObjectName(QStringLiteral("tsw_seg_winui3_icon_bar"));
    m_winui3IconOnlyBar = winUi3IconOnlyBar;
    segmentedLayout->addWidget(winUi3IconOnlyBar);

    QLabel *customLabel = new QLabel(tr("Segmented Gallery Style"));
    customLabel->setObjectName(QStringLiteral("tsw_seg_gallery_title"));
    applySectionHeadingFont(customLabel, 14);
    segmentedLayout->addWidget(customLabel);
    QLabel *customDesc = new QLabel(tr("特点：半圆胶囊 + 自定义背景/选中/悬停/按下色"));
    customDesc->setObjectName(QStringLiteral("tsw_seg_gallery_desc"));
    applySectionDescriptionStyle(customDesc);
    segmentedLayout->addWidget(customDesc);

    QTabBar *defaultGalleryBar = new QTabBar();
    defaultGalleryBar->setAttribute(Qt::WA_StyledBackground, true);
    defaultGalleryBar->setDrawBase(false);
    defaultGalleryBar->setProperty(TabBarStyleProperty, Segmented_Slide);
    defaultGalleryBar->setProperty(SegmentedSemiRoundProperty, true);
    defaultGalleryBar->setProperty(SegmentedBackgroundColorProperty, QColor("#D9D9DD"));
    defaultGalleryBar->setProperty(SegmentedBackgroundColorDarkProperty, QColor("#3F3F46"));
    defaultGalleryBar->setProperty(SegmentedSelectedColorProperty, QColor("#FFFFFF"));
    defaultGalleryBar->setProperty(SegmentedSelectedColorDarkProperty, QColor("#5C5C64"));
    defaultGalleryBar->setProperty(SegmentedHoverColorProperty, QColor("#E6E6EA"));
    defaultGalleryBar->setProperty(SegmentedHoverColorDarkProperty, QColor("#4A4A52"));
    defaultGalleryBar->setProperty(SegmentedPressedColorProperty, QColor("#D0D0D4"));
    defaultGalleryBar->setProperty(SegmentedPressedColorDarkProperty, QColor("#55555D"));
    defaultGalleryBar->setObjectName(QStringLiteral("tsw_seg_demo_gallery_bar"));
    defaultGalleryBar->addTab(tr("Weekly"));
    defaultGalleryBar->addTab(tr("Daily"));
    defaultGalleryBar->addTab(tr("Monthly"));
    defaultGalleryBar->setCurrentIndex(1);
    segmentedLayout->addWidget(defaultGalleryBar);
    
    // 设置最小宽度, qss的方式有点问题；当前也可以在Style里设置
    //利用expanding和maximumWidth来实现
    defaultGalleryBar->setExpanding(true);
    defaultGalleryBar->setMaximumWidth(150 * defaultGalleryBar->count());

    QTabBar *purpleGalleryBar = new QTabBar();
    purpleGalleryBar->setAttribute(Qt::WA_StyledBackground, true);
    purpleGalleryBar->setDrawBase(false);
    purpleGalleryBar->setProperty(TabBarStyleProperty, Segmented_Slide);
    purpleGalleryBar->setProperty(SegmentedSemiRoundProperty, true);
    purpleGalleryBar->setProperty(SegmentedBackgroundColorProperty, QColor("#D9D9DD"));
    purpleGalleryBar->setProperty(SegmentedBackgroundColorDarkProperty, QColor("#3F3F46"));
    purpleGalleryBar->setProperty(SegmentedSelectedColorProperty, QColor("#7E57E8"));
    purpleGalleryBar->setProperty(SegmentedSelectedColorDarkProperty, QColor("#6E4FD6"));
    purpleGalleryBar->setProperty(SegmentedHoverColorProperty, QColor("#E6E6EA"));
    purpleGalleryBar->setProperty(SegmentedHoverColorDarkProperty, QColor("#4A4A52"));
    purpleGalleryBar->setProperty(SegmentedPressedColorProperty, QColor("#D0D0D4"));
    purpleGalleryBar->setProperty(SegmentedPressedColorDarkProperty, QColor("#55555D"));
    purpleGalleryBar->setObjectName(QStringLiteral("tsw_seg_purple_gallery_bar"));
    purpleGalleryBar->addTab(tr("Overview"));
    purpleGalleryBar->addTab(tr("Stats"));
    purpleGalleryBar->addTab(tr("Goals"));
    purpleGalleryBar->addTab(tr("History"));
    purpleGalleryBar->setCurrentIndex(0);
    segmentedLayout->addWidget(purpleGalleryBar);

    purpleGalleryBar->setExpanding(true);
    purpleGalleryBar->setMaximumWidth(150 * purpleGalleryBar->count());

    QTabBar *iconOnlyGalleryBar = new QTabBar();
    iconOnlyGalleryBar->setAttribute(Qt::WA_StyledBackground, true);
    iconOnlyGalleryBar->setDrawBase(false);
    iconOnlyGalleryBar->setProperty(TabBarStyleProperty, Segmented_Slide);
    iconOnlyGalleryBar->setProperty(SegmentedSemiRoundProperty, true);
    iconOnlyGalleryBar->setProperty(SegmentedBackgroundColorProperty, QColor("#D9D9DD"));
    iconOnlyGalleryBar->setProperty(SegmentedBackgroundColorDarkProperty, QColor("#3F3F46"));
    iconOnlyGalleryBar->setProperty(SegmentedSelectedColorProperty, QColor("#FFFFFF"));
    iconOnlyGalleryBar->setProperty(SegmentedSelectedColorDarkProperty, QColor("#5C5C64"));
    iconOnlyGalleryBar->setProperty(SegmentedHoverColorProperty, QColor("#E6E6EA"));
    iconOnlyGalleryBar->setProperty(SegmentedHoverColorDarkProperty, QColor("#4A4A52"));
    iconOnlyGalleryBar->setProperty(SegmentedPressedColorProperty, QColor("#D0D0D4"));
    iconOnlyGalleryBar->setProperty(SegmentedPressedColorDarkProperty, QColor("#55555D"));
    iconOnlyGalleryBar->addTab(QString());
    iconOnlyGalleryBar->addTab(QString());
    iconOnlyGalleryBar->addTab(QString());
    iconOnlyGalleryBar->addTab(QString());
    iconOnlyGalleryBar->setCurrentIndex(1);
    iconOnlyGalleryBar->setExpanding(false);
    iconOnlyGalleryBar->setMaximumWidth(56 * iconOnlyGalleryBar->count());
    iconOnlyGalleryBar->setObjectName(QStringLiteral("tsw_seg_icononly_bar"));
    m_segmentedIconOnlyBar = iconOnlyGalleryBar;
    segmentedLayout->addWidget(iconOnlyGalleryBar);

    segmentedLayout->addStretch();
    mainLayout->addWidget(segmentedWidget, 1);
}

void TabShowcaseWidget::setupPillTabs(QVBoxLayout *mainLayout)
{
    QWidget *pillWidget = createTabWidgetContainer();
    QVBoxLayout *pillLayout = static_cast<QVBoxLayout *>(pillWidget->layout());

    QLabel *pillLabel = new QLabel(tr("Pill TabBar"));
    pillLabel->setObjectName(QStringLiteral("tsw_pill_title"));
    applySectionHeadingFont(pillLabel, 14);
    pillLayout->addWidget(pillLabel);

    QTabBar *pillBar = new QTabBar();
    pillBar->setTabsClosable(true);
    pillBar->setExpanding(false);
    pillBar->setProperty(TabBarStyleProperty, PillTabs);
    pillBar->setObjectName(QStringLiteral("tsw_pill_bar"));
    pillBar->addTab(tr("Home"));
    pillBar->addTab(tr("Search"));
    pillBar->addTab(tr("Settings"));
    pillBar->addTab(tr("Help"));
    pillBar->addTab(tr("About"));
    pillLayout->addWidget(pillBar);

    pillLayout->addStretch();
    mainLayout->addWidget(pillWidget, 1);
}

void TabShowcaseWidget::setupCapsuleTabs(QVBoxLayout *mainLayout)
{
    QWidget *capsuleWidget = createTabWidgetContainer();
    QVBoxLayout *capsuleLayout = static_cast<QVBoxLayout *>(capsuleWidget->layout());

    QLabel *capsuleLabel = new QLabel(tr("Capsule TabBar"));
    capsuleLabel->setObjectName(QStringLiteral("tsw_cap_title"));
    applySectionHeadingFont(capsuleLabel, 14);
    QLabel *capsuleDescLabel = new QLabel(tr("特点：浏览器标签样式。"));
    capsuleDescLabel->setObjectName(QStringLiteral("tsw_cap_desc"));
    applySectionDescriptionStyle(capsuleDescLabel);
    capsuleLayout->addWidget(capsuleLabel);
    capsuleLayout->addWidget(capsuleDescLabel);

    m_capsuleTabWidget = new ExTabWidget();
    m_capsuleTabWidget->setMinimumHeight(200);
    m_capsuleTabWidget->setTabsClosable(true);
    m_capsuleTabWidget->setMovable(true);

    QTabBar *capTabBar = m_capsuleTabWidget->tabBar();
    capTabBar->setAutoFillBackground(false);
    capTabBar->setExpanding(false);
    capTabBar->setProperty("TextAlign", static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft));
    capTabBar->setProperty(TabBarStyleProperty, Capsule);
    capTabBar->setDrawBase(false);

    const QStringList pageNames = {tr("Home"), tr("Search"), tr("Settings"), tr("Help"), tr("About")};
    const QStringList fullNames = {tr("Home Page"), tr("Search Page"), tr("Settings Page"), tr("Help Page"), tr("About Page")};
    const QList<QColor> pageColors = {
        QColor(255, 228, 225), QColor(224, 255, 255), QColor(240, 255, 240), QColor(255, 250, 205), QColor(230, 230, 250)};

    for (int i = 0; i < pageNames.size(); ++i)
    {
        QLabel *page = new QLabel(fullNames.at(i));
        page->setObjectName(QStringLiteral("tsw_cap_body_%1").arg(i));
        page->setAlignment(Qt::AlignCenter);
        styleTabPageLabel(page, pageColors[i]);
        m_capsuleTabWidget->addTab(page, pageNames.at(i));
    }

    capsuleLayout->addWidget(m_capsuleTabWidget, 1);
    mainLayout->addWidget(capsuleWidget, 1);
}

void TabShowcaseWidget::setupNavigationTabs(QVBoxLayout *mainLayout)
{
    QWidget *navigationWidget = createTabWidgetContainer();
    QVBoxLayout *navigationLayout = static_cast<QVBoxLayout *>(navigationWidget->layout());

    QLabel *navigationLabel = new QLabel(tr("Navigation TabBar"));
    navigationLabel->setObjectName(QStringLiteral("tsw_nav_title"));
    applySectionHeadingFont(navigationLabel, 14);
    QLabel *navigationDescLabel = new QLabel(tr("特点：适合用于侧边栏的导航菜单，选项卡垂直排列，选中时指示器有个变长效果"));
    navigationDescLabel->setObjectName(QStringLiteral("tsw_nav_desc"));
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
    navTabBar->setShape(QTabBar::RoundedWest);
    navTabBar->setDrawBase(false);
    m_navigationTabWidget->setMovable(false);
    navTabBar->setExpanding(false);
    navTabBar->setProperty("TextAlign", static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft));
    navTabBar->setProperty(TabBarStyleProperty, Navigation);

    const QStringList navFullNames = {tr("Overview Page"),
                                      tr("Files Page"),
                                      tr("History Page"),
                                      tr("Insights Page"),
                                      tr("Settings Page")};
    const QStringList navNames = {tr("Overview"), tr("Files"), tr("History"), tr("Insights"), tr("Settings")};
    const QList<QColor> navPageColors = {
        QColor(244, 248, 255), QColor(240, 251, 246), QColor(255, 248, 238), QColor(248, 243, 255), QColor(245, 245, 245)};

    for (int i = 0; i < navFullNames.size(); ++i)
    {
        QLabel *page = new QLabel(navFullNames.at(i));
        page->setObjectName(QStringLiteral("tsw_nav_body_%1").arg(i));
        page->setAlignment(Qt::AlignCenter);
        page->setMinimumHeight(220);
        styleTabPageLabel(page, navPageColors[i]);
        m_navigationTabWidget->addTab(page, navNames.at(i));
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
                                         const char *sectionId,
                                         const QString &title,
                                         const QString &description,
                                         int tabStyle,
                                         QTabBar **outTabBar)
{
    const QString sid = QString::fromLatin1(sectionId);

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("tsw_%1_title").arg(sid));
    applySectionHeadingFont(titleLabel, 14);
    layout->addWidget(titleLabel);

    if (!description.isEmpty())
    {
        QLabel *descLabel = new QLabel(description);
        descLabel->setObjectName(QStringLiteral("tsw_%1_desc").arg(sid));
        applySectionDescriptionStyle(descLabel);
        layout->addWidget(descLabel);
    }

    QTabBar *tabBar = new QTabBar();
    tabBar->setObjectName(QStringLiteral("tsw_%1_bar").arg(sid));
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
        tabBar->setAttribute(Qt::WA_StyledBackground, true);
    }
    tabBar->addTab(tr("Home"));
    tabBar->addTab(tr("Search"));
    tabBar->addTab(tr("Settings"));
    tabBar->addTab(tr("Help"));
    tabBar->addTab(tr("About"));
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
    if (m_winui3IconOnlyBar)
    {
        for (int i = 0; i < nSeg && i < m_winui3IconOnlyBar->count(); ++i)
        {
            m_winui3IconOnlyBar->setTabIcon(i, FONTICON->getIcon(segmentedIcons[i]));
        }
    }

    static const int segmentedIconOnlyIcons[] = {
        static_cast<int>(SegoeIcon::Camera),
        static_cast<int>(SegoeIcon::Video),
        static_cast<int>(SegoeIcon::MusicInfo),
        static_cast<int>(SegoeIcon::Cloud),
    };
    if (m_segmentedIconOnlyBar)
    {
        for (int i = 0;
             i < int(sizeof(segmentedIconOnlyIcons) / sizeof(segmentedIconOnlyIcons[0])) && i < m_segmentedIconOnlyBar->count();
             ++i)
        {
            m_segmentedIconOnlyBar->setTabIcon(i, FONTICON->getIcon(segmentedIconOnlyIcons[i]));
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
