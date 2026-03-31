#pragma once

#include <QMainWindow>
#include <QTabBar>

class QTabBar;
class SlidingStackedWidget;

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
    // void on_horizonalBtn_clicked();
    // void on_verticalBtn_clicked();

    void on_checkBox_4_clicked(bool checked);
    void on_checkBox_5_checkStateChanged(Qt::CheckState state);
    void on_checkBox_5_stateChanged(int arg1);
    void on_radioButton_7_clicked();
    void on_radioButton_4_clicked();
    void on_radioButton_5_clicked();
    void on_radioButton_6_clicked();

private:
    void init();
    void initNavigationView();
    void initMenuAndToolBar();
    void setupAnimatedCapsuleTabs();

    void updateActionIcons();
    void loadChangelog();

private:
    Ui::MainWindow *ui;

    QToolBar* m_toolBar{nullptr};
    QTabBar* m_capsuleTabBar{nullptr};
    SlidingStackedWidget* m_capsuleStack{nullptr};
    QTabBar* m_growTabBar{nullptr};
    SlidingStackedWidget* m_growStack{nullptr};
    QTabBar* m_slideTabBar{nullptr};
    SlidingStackedWidget* m_slideStack{nullptr};
    QTabBar* m_stretchTabBar{nullptr};
    SlidingStackedWidget* m_stretchStack{nullptr};
};
