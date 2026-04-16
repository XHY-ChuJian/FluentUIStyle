#pragma once

#include <QMainWindow>
#include <QTabBar>

class QTabBar;
class ExStackedWidget;
class ExTabWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_checkBox_4_clicked(bool checked);
    void on_checkBox_5_stateChanged(int arg1);
    void on_radioButton_7_clicked();
    void on_radioButton_4_clicked();
    void on_radioButton_5_clicked();
    void on_radioButton_6_clicked();

private:
    void init();
    void initTableView();
    void initNavigationView();
    void initMenuAndToolBar();
    void setupTabs();

    void updateActionIcons();
    void loadChangelog();

private:
    Ui::MainWindow *ui;

    QToolBar* m_toolBar{nullptr};

    ExTabWidget* m_capsuleTabWidget{nullptr};
    QTabBar* m_segmentedBar{nullptr};
    QTabBar* m_segmentedFadeBar{nullptr};
    ExTabWidget* m_navigationTabWidget{nullptr};
    QAction* m_searchAction{nullptr};
};
