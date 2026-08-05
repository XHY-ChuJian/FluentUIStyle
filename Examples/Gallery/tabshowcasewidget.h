#pragma once

#include <QWidget>

class ExTabWidget;
class QTabBar;
class QVBoxLayout;

class TabShowcaseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TabShowcaseWidget(QWidget *parent = nullptr);
    void updateTabIcons();

private:
    void setupPivotTabs(QVBoxLayout *mainLayout);
    void setupSegmentedTabs(QVBoxLayout *mainLayout);
    void setupPillTabs(QVBoxLayout *mainLayout);
    void setupCapsuleTabs(QVBoxLayout *mainLayout);
    void setupNavigationTabs(QVBoxLayout *mainLayout);
    QWidget *createTabWidgetContainer();
    void addTabBarSection(QVBoxLayout *layout,
                          const char *sectionId,
                          const QString &title,
                          const QString &description,
                          int tabStyle,
                          QTabBar **outTabBar = nullptr);

private:
    ExTabWidget *m_capsuleTabWidget{nullptr};
    QTabBar *m_segmentedBar{nullptr};
    QTabBar *m_segmentedFadeBar{nullptr};
    QTabBar *m_winui3Bar{nullptr};
    QTabBar *m_winui3IconOnlyBar{nullptr};
    QTabBar *m_segmentedIconOnlyBar{nullptr};
    ExTabWidget *m_navigationTabWidget{nullptr};
};
