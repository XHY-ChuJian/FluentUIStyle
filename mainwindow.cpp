#include "mainwindow.h"

#include <QMdiArea>
#include <QStyleFactory>
#include <QStyleHints>
#include <QAbstractItemView>

#include "cusproxystyle.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow( QWidget* parent )
    : QMainWindow( parent )
    , ui( new Ui::MainWindow )
{
    ui->setupUi( this );

    setWindowTitle("FluentUI Demo - QStyle");
    setAttribute( Qt::WA_StyledBackground );
    setStyleSheet( "MainWindow{background: white;}" );  // rgb(243,243,243);

    ui->tableWidget->verticalHeader()->setMinimumSectionSize( 32 );

    ui->widget->setAttribute( Qt::WA_StyledBackground );
    QList<QWidget*> widgetList;
    widgetList << ui->widget << ui->widget_2 << ui->widget_3 << ui->widget_4 << ui->widget_5 << ui->widget_6
               << ui->widget_7 << ui->widget_8 << ui->widget_9 << ui->widget_10;
    for ( QWidget* w : widgetList )
    {
        //draw border in style
        w->setAttribute( Qt::WA_StyledBackground );
        w->setProperty( "fluentBorder", true );
    }

    init();

    // 用一首七言绝句初始化comboBox
    ui->comboBox->addItem( "窗含西岭千秋雪" );
    ui->comboBox->addItem( "门泊东吴万里船" );
    ui->comboBox->addItem( "日照香炉生紫烟" );
    ui->comboBox->addItem( "遥看瀑布挂前川" );
    ui->comboBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    ui->comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ui->comboBox->setView(new QListView());
#endif

    //ui->log 添加日志规范
    //更新5.15.2 下图形字体错误的Bug，QComboBox下拉ItemBug，右键菜单menu.popup()仍有Bug
    ui->log->append("【更新内容】");
    ui->log->append("1. 修复 Qt 5.15.2 下图形字体渲染错误的问题");
    ui->log->append("2. 修复 QComboBox 下拉项显示异常的 Bug");
    ui->log->append("3. 调整 Demo 布局，优化界面展示");
    ui->log->append("4. 右键菜单 menu.popup() 仍存在显示问题（待修复）");
    ui->log->append("");

    ui->log->append("【Qt5 版本说明】");
    ui->log->append("1. 右键菜单阴影问题为 Qt 5.15.2 已知缺陷");
    ui->log->append("   main.cpp 中已提供可选修复方案，Qt5 用户可自行启用。");

    ui->log->append("2. QDockWidget 标题栏按钮图标在高 DPI 下存在缩放失真问题");
    ui->log->append("   当前建议通过自定义 TitleBarWidget 方式规避。");

    ui->log->append("3. Qt5 存在若干样式兼容问题：");
    ui->log->append("   - QComboBox 需手动 setView() 才能完全应用样式");
    ui->log->append("   - 部分控件在高 DPI 下表现不一致");
    ui->log->append("   - Qt 5.15 后续不再进行兼容性修复");
    ui->log->append("");
}

MainWindow::~MainWindow()
{
    delete ui;
}

#include <QAction>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>

void MainWindow::init()
{
    // ===== 菜单栏 =====
    QMenu* fileMenu = menuBar()->addMenu( "文件(&F)" );
    QMenu* editMenu = menuBar()->addMenu( "编辑(&E)" );
    QMenu* helpMenu = menuBar()->addMenu( "帮助(&H)" );

    // ===== 文件菜单 =====
    QAction* newAct  = new QAction( "新建", this );
    QAction* openAct = new QAction( "打开", this );
    QAction* saveAct = new QAction( "保存", this );
    QAction* exitAct = new QAction( "退出", this );

    fileMenu->addAction( newAct );
    fileMenu->addAction( openAct );
    fileMenu->addAction( saveAct );
    fileMenu->addSeparator();
    fileMenu->addAction( exitAct );

    // ===== 编辑菜单 =====
    QAction* undoAct  = new QAction( "撤销", this );
    QAction* redoAct  = new QAction( "重做", this );
    QAction* copyAct  = new QAction( "复制", this );
    QAction* pasteAct = new QAction( "粘贴", this );

    editMenu->addAction( undoAct );
    editMenu->addAction( redoAct );
    editMenu->addSeparator();
    editMenu->addAction( copyAct );
    editMenu->addAction( pasteAct );

    // ===== 帮助菜单 =====
    QAction* aboutAct = new QAction( "关于", this );
    helpMenu->addAction( aboutAct );

    // ===== 工具栏 =====
    QToolBar* toolBar = ui->toolBar;
    toolBar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );

    toolBar->addAction( newAct );
    toolBar->addAction( openAct );
    toolBar->addAction( saveAct );
    toolBar->addSeparator();
    toolBar->addAction( copyAct );
    toolBar->addAction( pasteAct );

    QCheckBox* cb = new QCheckBox( "禁用", this );
    toolBar->addWidget( cb );
    connect(cb, &QCheckBox::clicked, this, [=](bool checked){
        centralWidget()->setEnabled(!checked);
    } );

    // ===== 状态栏（仅展示）=====
    statusBar()->showMessage( "Ready" );

    {
        // ui->page_5 添加QMdiArea，以及添加按钮控制ViewMode { SubWindowView, TabbedView }
        QMdiArea* mdiArea = new QMdiArea( ui->page_5 );
        mdiArea->setViewMode( QMdiArea::SubWindowView );
        mdiArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
        mdiArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );

        // 添加一些子窗口
        for ( int i = 0; i < 3; ++i )
        {
            QMdiSubWindow* subWindow = new QMdiSubWindow();
            subWindow->setWidget( new QTextEdit() );
            subWindow->setAttribute( Qt::WA_DeleteOnClose );
            subWindow->setWindowTitle( QString( "子窗口 %1" ).arg( i + 1 ) );
            mdiArea->addSubWindow( subWindow );
            subWindow->show();
        }

        // 添加切换视图模式的按钮
        QPushButton* switchViewBtn = new QPushButton( "切换视图模式", ui->page_5 );
        switchViewBtn->move( 10, 10 );
        switchViewBtn->raise();  // 确保按钮在mdiArea之上
        connect( switchViewBtn,
                 &QPushButton::clicked,
                 this,
                 [ mdiArea ]()
                 {
                     if ( mdiArea->viewMode() == QMdiArea::SubWindowView )
                     {
                         mdiArea->setViewMode( QMdiArea::TabbedView );
                     }
                     else
                     {
                         mdiArea->setViewMode( QMdiArea::SubWindowView );
                     }
                 } );

        QVBoxLayout* layout = new QVBoxLayout( ui->page_5 );
        layout->addWidget( switchViewBtn );
        layout->addWidget( mdiArea );
    }
}
