#pragma once

#include <QMainWindow>
#include <QTabBar>
#include <QVector>

#include <Windows.h>

#include "qboxlayout.h"
#include "qsvgrenderer.h"
#include "qtreewidget.h"

class QTabBar;
class ExNavTreeWidget;
class ExWinUINavigationView;
class ExStackedWidget;
class QComboBox;
class QTreeWidgetItem;
class TabShowcaseWidget;

QT_BEGIN_NAMESPACE

namespace Ui
{
    class MainWindow;
} // namespace Ui

QT_END_NAMESPACE

enum class WidgetBgMode
{
    None,
    Pixmap
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // QWidget interface

protected:
    void paintEvent(QPaintEvent *event);
    void showEvent(QShowEvent *event);

private slots:
    void on_checkBox_4_clicked(bool checked);
    void on_checkBox_5_stateChanged(int arg1);
    void on_radioButton_7_clicked();
    void on_radioButton_4_clicked();
    void on_radioButton_5_clicked();
    void on_radioButton_6_clicked();

    void on_rBLightTheme_clicked(bool checked);
    void on_rBDarkTheme_clicked(bool checked);

    void on_rBWidgtModeNormal_clicked(bool checked);
    void on_rBWidgetModePixmap_clicked(bool checked);

    void on_rBOnlyIcon_clicked(bool checked);
    void on_rBIconAndText_clicked(bool checked);

    void on_rBLangZh_CN_clicked(bool checked);
    void on_rBLangEn_US_clicked(bool checked);
    void on_rBLangSystem_clicked(bool checked);

private:
    void initializeFluentBorderWidgets();
    void initializeComponents();
    void setupComboBox();
    void initializeTableView();
    void initializeNavigationView();
    void rebuildMenuAndToolBar();
    void buildMainMenus();
    void setupTabs();
    void setupButtonsAndIcons();
    void setupAccentColorWidget();
    void setupMdiArea();
    void setupSegoeIconGalleryPage();
    void setupAboutPage();

    void updateActionIcons();
    void loadChangelog();

    // Menu and Toolbar helpers
    QAction *addToolBarAction(QToolBar *toolBar,
                              const QString &iconCode,
                              const QString &text,
                              const QKeySequence &shortcut = QKeySequence());
    void setupToolBarControls(QToolBar *toolBar);
    void setupThemeSelector(QToolBar *toolBar);
    void setupColorSchemeSelector(QToolBar *toolBar);
    void setupStyleSelector(QToolBar *toolBar);
    void setupWidgetBackgroundSelector(QToolBar *toolBar);

    // Navigation View helpers
    void addTestNavigationTree();

    // Button helpers
    void setupToolButtonWithMenu();

    // Icon update helpers
    void updateButtonIcons();
    void updateMenuActionIcons();
    void updateMenuIcons();
    void updateNavigationItemIcons();

    void syncLanguageRadios();
    void promptRestartAfterLanguageChange();

private:
    Ui::MainWindow *ui;

    QMenuBar *m_menuBar{nullptr};
    QToolBar *m_toolBar{nullptr};

    TabShowcaseWidget *m_tabShowcaseWidget{nullptr};
    ExNavTreeWidget *m_navView{nullptr};
    ExWinUINavigationView *m_winUINavigationView{nullptr};
    QAction *m_searchAction{nullptr};

    QTabBar *m_tabBarWidgetBg{nullptr};
    WidgetBgMode m_widgetBgMode{WidgetBgMode::None};
    QAction *m_navigationToggleAction{nullptr};

    QComboBox *themeComboBox{nullptr};
    QComboBox *m_colorSchemeCombo{nullptr};
    QComboBox *m_styleComboBox{nullptr};

    QVector<QTreeWidgetItem *> m_mainNavItems;
    QTreeWidgetItem *m_navTestRoot{nullptr};
    QTreeWidgetItem *m_navAboutItem{nullptr};
    QTreeWidgetItem *m_navSettingsItem{nullptr};

private:
    QPixmap m_bgLight;
    QPixmap m_bgDark;
};
