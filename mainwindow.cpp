#include "mainwindow.h"

#include <QAbstractItemView>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QFile>
#include <QKeySequence>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>

#include <fluentui3style.h>
#include <palettemanager.h>

#include "fluentuiappearance.h"
#include "ui_mainwindow.h"

// 存储所有需要更新图标的action及其对应的图标代码
QMap<QAction*, QString> actionIconMap;

// 存储所有需要更新图标的menu及其对应的图标代码
QMap<QMenu*, QString> menuIconMap;

QIcon createFluentIcon( const QString& unicode, int pixelSize = 25 )
{
    qreal dpr = /*qApp->devicePixelRatio()*/ 1;
    QFont iconFont( "Segoe Fluent Icons" );
    iconFont.setPixelSize( pixelSize * dpr );

    // 检查当前主题
    bool isDarkTheme = false;
#if ( QT_VERSION >= QT_VERSION_CHECK( 6, 8, 0 ) )
    isDarkTheme = qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    isDarkTheme = fluentUIAppearance.theme() == Theme::Dark;
#endif

    QPixmap pixmap( 30 * dpr, 30 * dpr );

    pixmap.setDevicePixelRatio( dpr );
    pixmap.fill( Qt::transparent );

    QPainter painter( &pixmap );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
    painter.setFont( iconFont );
    painter.setPen( isDarkTheme ? Qt::white : Qt::black );

    painter.drawText( pixmap.rect(), Qt::AlignCenter, unicode );

    return QIcon( pixmap );
}

MainWindow::MainWindow( QWidget* parent )
    : QMainWindow( parent )
    , ui( new Ui::MainWindow )
{
    setAttribute( Qt::WA_StyledBackground );
    ui->setupUi( this );

    setWindowTitle( QString( "FluentUI Demo - QStyle [Qt-Verison %1]" ).arg( QT_VERSION_STR ) );
    const QString strStyleSheet = R"(
    #widget, #widget_2,#widget_3,#widget_4,#widget_5,#widget_6,#widget_7,#widget_8,#widget_9,#widget_10
    {border: 2px solid rgb(226, 226, 226); border-radius: 5px;}
    )";
    // setStyleSheet( strStyleSheet );
    QList<QWidget*> widgetList;
    widgetList << ui->widget << ui->widget_2 << ui->widget_3 << ui->widget_4 << ui->widget_5 << ui->widget_6
               << ui->widget_7 << ui->widget_8 << ui->widget_9 << ui->widget_10 << ui->widget_12;
    for ( QWidget* w : widgetList )
    {
        // draw border in style
        w->setAttribute( Qt::WA_StyledBackground );
        w->setProperty( "fluentBorder", true );
    }

    ui->tableWidget->verticalHeader()->setMinimumSectionSize( 32 );

    init();
    ui->stackedWidget->setCurrentIndex(0);

    ui->comboBox->addItem( "窗含西岭千秋雪" );
    ui->comboBox->addItem( "门泊东吴万里船" );
    ui->comboBox->addItem( "日照香炉生紫烟" );
    ui->comboBox->addItem( "遥看瀑布挂前川" );
    ui->comboBox->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
    ui->comboBox->setSizeAdjustPolicy( QComboBox::AdjustToContents );

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    ui->comboBox->setView( new QListView() );
#endif

    loadChangelog();

    // adjustSize();
}

void MainWindow::loadChangelog()
{
    QFile file( ":/changelog.txt" );

    if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        QTextStream in( &file );
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
        in.setCodec( "UTF-8" );
#endif
        QString content = in.readAll();
        file.close();
        QStringList lines = content.split( "\n" );
        for ( const QString& line : lines )
        {
            ui->log->append( line );
        }
    }
    else
    {
        ui->log->append( "无法打开changelog.txt, " + file.errorString() );
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void initMenu( QMainWindow* window )
{
    QMenuBar* menuBar = window->menuBar();

    // ================= 文件菜单 =================
    QMenu* fileMenu        = menuBar->addMenu( "文件" );
    QAction* newFileAction = fileMenu->addAction( createFluentIcon( "\ue8a5" ), "新建文件" );
    newFileAction->setShortcut( QKeySequence( "Ctrl+N" ) );
    actionIconMap[ newFileAction ] = "\ue8a5";

    QAction* newProjectAction         = fileMenu->addAction( createFluentIcon( "\ue8b5" ), "新建项目" );
    actionIconMap[ newProjectAction ] = "\ue8b5";

    QMenu* recentMenu         = fileMenu->addMenu( createFluentIcon( "\ue8c3" ), "最近打开" );
    menuIconMap[ recentMenu ] = "\ue8c3";
    recentMenu->addAction( "project1" );
    recentMenu->addAction( "project2" );
    recentMenu->addAction( "example.cpp" );

    QAction* openFileAction = fileMenu->addAction( createFluentIcon( "\ue8a5" ), "打开文件" );
    openFileAction->setShortcut( QKeySequence( "Ctrl+O" ) );
    actionIconMap[ openFileAction ] = "\ue8a5";

    QAction* openProjectAction         = fileMenu->addAction( createFluentIcon( "\ue8b5" ), "打开项目" );
    actionIconMap[ openProjectAction ] = "\ue8b5";

    fileMenu->addSeparator();

    QAction* saveAction = fileMenu->addAction( createFluentIcon( "\ue74e" ), "保存" );
    saveAction->setShortcut( QKeySequence( "Ctrl+S" ) );
    actionIconMap[ saveAction ] = "\ue74e";

    QAction* saveAsAction = fileMenu->addAction( createFluentIcon( "\ue74e" ), "另存为" );
    saveAsAction->setShortcut( QKeySequence( "Ctrl+Shift+S" ) );
    actionIconMap[ saveAsAction ] = "\ue74e";

    fileMenu->addSeparator();

    QAction* closeFileAction         = fileMenu->addAction( createFluentIcon( "\ue8bb" ), "关闭文件" );
    actionIconMap[ closeFileAction ] = "\ue8bb";

    QAction* exitAction = fileMenu->addAction( createFluentIcon( "\ue8bb" ), "退出" );
    exitAction->setShortcut( QKeySequence( "Ctrl+Q" ) );
    actionIconMap[ exitAction ] = "\ue8bb";

    // ================= 编辑菜单 =================
    QMenu* editMenu     = menuBar->addMenu( "编辑" );
    QAction* undoAction = editMenu->addAction( createFluentIcon( "\ue7a7" ), "撤销" );
    undoAction->setShortcut( QKeySequence( "Ctrl+Z" ) );
    actionIconMap[ undoAction ] = "\ue7a7";

    QAction* redoAction = editMenu->addAction( createFluentIcon( "\ue7a6" ), "重做" );
    redoAction->setShortcut( QKeySequence( "Ctrl+Y" ) );
    actionIconMap[ redoAction ] = "\ue7a6";

    editMenu->addSeparator();

    QAction* cutAction = editMenu->addAction( createFluentIcon( "\ue8c6" ), "剪切" );
    cutAction->setShortcut( QKeySequence( "Ctrl+X" ) );
    actionIconMap[ cutAction ] = "\ue8c6";

    QAction* copyAction = editMenu->addAction( createFluentIcon( "\ue8c8" ), "复制" );
    copyAction->setShortcut( QKeySequence( "Ctrl+C" ) );
    actionIconMap[ copyAction ] = "\ue8c8";

    QAction* pasteAction = editMenu->addAction( createFluentIcon( "\ue8c7" ), "粘贴" );
    pasteAction->setShortcut( QKeySequence( "Ctrl+V" ) );
    actionIconMap[ pasteAction ] = "\ue8c7";

    editMenu->addSeparator();

    QAction* findAction = editMenu->addAction( createFluentIcon( "\ue721" ), "查找" );
    findAction->setShortcut( QKeySequence( "Ctrl+F" ) );
    actionIconMap[ findAction ] = "\ue721";

    QAction* replaceAction = editMenu->addAction( createFluentIcon( "\ue8ac" ), "替换" );
    replaceAction->setShortcut( QKeySequence( "Ctrl+H" ) );
    actionIconMap[ replaceAction ] = "\ue8ac";

    QMenu* advancedMenu         = editMenu->addMenu( createFluentIcon( "\ue713" ), "高级" );
    menuIconMap[ advancedMenu ] = "\ue713";
    QAction* autoFormatAction   = advancedMenu->addAction( createFluentIcon( "\ue930" ), "自动格式化" );
    autoFormatAction->setCheckable( true );
    actionIconMap[ autoFormatAction ] = "\ue930";

    QAction* sortLinesAction         = advancedMenu->addAction( createFluentIcon( "\ue930" ), "排序行" );
    actionIconMap[ sortLinesAction ] = "\ue930";

    QAction* deleteEmptyLinesAction         = advancedMenu->addAction( createFluentIcon( "\ue8bb" ), "删除空行" );
    actionIconMap[ deleteEmptyLinesAction ] = "\ue8bb";

    // ================= 视图菜单 =================
    QMenu* viewMenu            = menuBar->addMenu( "视图" );
    QAction* showToolbarAction = viewMenu->addAction( createFluentIcon( "\ue728" ), "显示工具栏" );
    showToolbarAction->setCheckable( true );
    showToolbarAction->setChecked( true );
    actionIconMap[ showToolbarAction ] = "\ue728";

    QAction* showStatusBarAction = viewMenu->addAction( createFluentIcon( "\ue9d9" ), "显示状态栏" );
    showStatusBarAction->setCheckable( true );
    showStatusBarAction->setChecked( true );
    actionIconMap[ showStatusBarAction ] = "\ue9d9";

    viewMenu->addSeparator();

    QAction* showSidebarAction = viewMenu->addAction( createFluentIcon( "\ue728" ), "显示侧边栏" );
    showSidebarAction->setCheckable( true );
    showSidebarAction->setChecked( true );
    actionIconMap[ showSidebarAction ] = "\ue728";

    QAction* showOutputAction = viewMenu->addAction( createFluentIcon( "\ue7e8" ), "显示输出窗口" );
    showOutputAction->setCheckable( true );
    actionIconMap[ showOutputAction ] = "\ue7e8";

    QMenu* zoomMenu         = viewMenu->addMenu( createFluentIcon( "\ue71e" ), "缩放" );
    menuIconMap[ zoomMenu ] = "\ue71e";
    zoomMenu->addAction( "放大" );
    zoomMenu->addAction( "缩小" );
    zoomMenu->addAction( "恢复默认" );

    // ================= 构建菜单 =================
    QMenu* buildMenu            = menuBar->addMenu( "构建" );
    QAction* buildProjectAction = buildMenu->addAction( createFluentIcon( "\ue7b8" ), "构建项目" );
    buildProjectAction->setShortcut( QKeySequence( "Ctrl+B" ) );
    actionIconMap[ buildProjectAction ] = "\ue7b8";

    QAction* rebuildAction         = buildMenu->addAction( createFluentIcon( "\ue7b8" ), "重新构建" );
    actionIconMap[ rebuildAction ] = "\ue7b8";

    buildMenu->addSeparator();

    QAction* runAction         = buildMenu->addAction( createFluentIcon( "\ue768" ), "运行" );
    actionIconMap[ runAction ] = "\ue768";

    QAction* debugAction         = buildMenu->addAction( createFluentIcon( "\ue7a6" ), "调试" );
    actionIconMap[ debugAction ] = "\ue7a6";

    QMenu* buildTargetMenu         = buildMenu->addMenu( createFluentIcon( "\ue8b5" ), "构建目标" );
    menuIconMap[ buildTargetMenu ] = "\ue8b5";
    buildTargetMenu->addAction( "Debug" );
    buildTargetMenu->addAction( "Release" );

    // ================= 帮助菜单 =================
    QMenu* helpMenu             = menuBar->addMenu( "帮助" );
    QAction* docsAction         = helpMenu->addAction( createFluentIcon( "\ue8a5" ), "文档" );
    actionIconMap[ docsAction ] = "\ue8a5";

    QAction* apiAction         = helpMenu->addAction( createFluentIcon( "\ue8a5" ), "API参考" );
    actionIconMap[ apiAction ] = "\ue8a5";

    helpMenu->addSeparator();

    QAction* updateAction         = helpMenu->addAction( createFluentIcon( "\ue7b8" ), "检查更新" );
    actionIconMap[ updateAction ] = "\ue7b8";

    helpMenu->addSeparator();

    QAction* aboutAction         = helpMenu->addAction( createFluentIcon( "\ue946" ), "关于" );
    actionIconMap[ aboutAction ] = "\ue946";
}

void MainWindow::initMenuAndToolBar()
{
    // 清空图标映射
    actionIconMap.clear();
    menuIconMap.clear();

    // 如果工具栏不存在，创建一个
    if ( !m_toolBar )
    {
        m_toolBar = addToolBar( "工具栏" );
    }

    // ===== 初始化菜单 =====
    initMenu( this );

    // ===== 工具栏 =====
    auto toolBar = m_toolBar;

    // ================= 常用操作按钮 =================
    QAction* newAction = toolBar->addAction( createFluentIcon( "\ue8a5" ), "新建" );
    newAction->setShortcut( QKeySequence( "Ctrl+N" ) );
    actionIconMap[ newAction ] = "\ue8a5";

    QAction* openAction = toolBar->addAction( createFluentIcon( "\ue8a5" ), "打开" );
    openAction->setShortcut( QKeySequence( "Ctrl+O" ) );
    actionIconMap[ openAction ] = "\ue8a5";

    QAction* saveAction = toolBar->addAction( createFluentIcon( "\ue74e" ), "保存" );
    saveAction->setShortcut( QKeySequence( "Ctrl+S" ) );
    actionIconMap[ saveAction ] = "\ue74e";

    toolBar->addSeparator();

    // ================= 编辑操作按钮 =================
    QAction* undoAction = toolBar->addAction( createFluentIcon( "\ue7a7" ), "撤销" );
    undoAction->setShortcut( QKeySequence( "Ctrl+Z" ) );
    actionIconMap[ undoAction ] = "\ue7a7";

    QAction* redoAction = toolBar->addAction( createFluentIcon( "\ue7a6" ), "重做" );
    redoAction->setShortcut( QKeySequence( "Ctrl+Y" ) );
    actionIconMap[ redoAction ] = "\ue7a6";

    toolBar->addSeparator();

    QAction* cutAction = toolBar->addAction( createFluentIcon( "\ue8c6" ), "剪切" );
    cutAction->setShortcut( QKeySequence( "Ctrl+X" ) );
    actionIconMap[ cutAction ] = "\ue8c6";

    QAction* copyAction = toolBar->addAction( createFluentIcon( "\ue8c8" ), "复制" );
    copyAction->setShortcut( QKeySequence( "Ctrl+C" ) );
    actionIconMap[ copyAction ] = "\ue8c8";

    QAction* pasteAction = toolBar->addAction( createFluentIcon( "\ue8c7" ), "粘贴" );
    pasteAction->setShortcut( QKeySequence( "Ctrl+V" ) );
    actionIconMap[ pasteAction ] = "\ue8c7";

    toolBar->addSeparator();

    // ================= 构建操作按钮 =================
    QAction* buildAction = toolBar->addAction( createFluentIcon( "\ue7b8" ), "构建" );
    buildAction->setShortcut( QKeySequence( "Ctrl+B" ) );
    actionIconMap[ buildAction ] = "\ue7b8";

    QAction* rebuildAction         = toolBar->addAction( createFluentIcon( "\ue7b8" ), "重新构建" );
    actionIconMap[ rebuildAction ] = "\ue7b8";

    QAction* runAction         = toolBar->addAction( createFluentIcon( "\ue768" ), "运行" );
    actionIconMap[ runAction ] = "\ue768";

    toolBar->addSeparator();

    // ================= 其他控件 =================
    // 添加禁用按钮
    QCheckBox* cb = new QCheckBox( "禁用", this );
    connect( cb, &QCheckBox::clicked, this, [ = ]( bool checked ) { centralWidget()->setEnabled( !checked ); } );
    toolBar->addWidget( cb );
    toolBar->addSeparator();

    // 添加QLabel(主题:) + QComboBox Qt::ColorScheme切换
    QLabel* themeLabel = new QLabel( "主题：", this );
    toolBar->addWidget( themeLabel );
    QComboBox* themeComboBox = new QComboBox( this );
    themeComboBox->blockSignals( true );
    themeComboBox->addItem( "浅色" );
    themeComboBox->addItem( "暗色" );
    themeComboBox->blockSignals( false );

    themeComboBox->setView( new QListView );
    themeComboBox->setCurrentIndex( fluentUIAppearance.theme() == Theme::Dark ? 1 : 0 );
    connect( themeComboBox,
            QOverload<int>::of( &QComboBox::currentIndexChanged ),
            this,
            [ = ]( int index )
            {
                fluentUIAppearance.setTheme( index == 0 ? Theme::Light : Theme::Dark );
                updateActionIcons();
            } );
    toolBar->addWidget( themeComboBox );
}

void MainWindow::updateActionIcons()
{
    {
        //TabBar添加图标
        ui->tabWidget->setTabIcon( 0, createFluentIcon( "\ue8a5" ) );
        ui->tabWidget->setTabIcon( 1, createFluentIcon( "\ue8b5" ) );
        ui->tabWidget->setTabIcon( 2, createFluentIcon( "\ue8c3" ) );
    }

    {
        ui->toolButton->setIcon( createFluentIcon( "\ue8c3" ) );
        ui->toolButton_2->setIcon( createFluentIcon( "\ue713" ) );
        ui->toolButton_3->setIcon( createFluentIcon( "\uEA8E" ) );
        ui->toolButton_4->setIcon( createFluentIcon( "\uE804" ) );
    }

    // 更新action图标
    for ( auto it = actionIconMap.begin(); it != actionIconMap.end(); ++it )
    {
        QAction* action  = it.key();
        QString iconCode = it.value();
        if ( action )
        {
            action->setIcon( createFluentIcon( iconCode ) );

        }
    }

    // 更新menu图标
    for ( auto it = menuIconMap.begin(); it != menuIconMap.end(); ++it )
    {
        QMenu* menu      = it.key();
        QString iconCode = it.value();
        if ( menu )
        {
            menu->setIcon( createFluentIcon( iconCode ) );
        }
    }
}

void MainWindow::init()
{
    initMenuAndToolBar();

    ui->spinBox->setProperty( "spinBoxButtonLayout", ArrowsVertical );
    ui->spinBox_2->setProperty( "spinBoxButtonLayout", ArrowsHorizontalSides );
    ui->spinBox_3->setProperty( "spinBoxButtonLayout", ArrowsHorizontalRight );
    ui->doubleSpinBox->setProperty( "spinBoxButtonLayout", PlusMinusHorizontalSides );
    {
        ui->scrollAreaWidgetContents->setAutoFillBackground( false );
    }

    {
        //TabBar添加图标
        ui->tabWidget->setTabIcon( 0, createFluentIcon( "\ue8a5" ) );
        ui->tabWidget->setTabIcon( 1, createFluentIcon( "\ue8b5" ) );
        ui->tabWidget->setTabIcon( 2, createFluentIcon( "\ue8c3" ) );
    }

    {
        // pushButton、pushButton_2、toolButton添加图标
        ui->toolButton->setIcon( createFluentIcon( "\ue8c3" ) );

        // toolButton_2添加文字和图标
        ui->toolButton_2->setText( "工具按钮" );
        ui->toolButton_2->setIcon( createFluentIcon( "\ue713" ) );

        //toolButton_3 带文字和图标以及4个菜单
        ui->toolButton_3->setAutoRaise(false);
        ui->toolButton_3->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui->toolButton_3->setPopupMode(QToolButton::InstantPopup);
        ui->toolButton_3->setText( "菜单按钮" );
        ui->toolButton_3->setIcon( createFluentIcon( "\uEA8E" ) );
        QMenu* menu = new QMenu( ui->toolButton_3 );
        actionIconMap [menu->addAction( createFluentIcon( "\ue8a5" ), "新建文件" )] = "\ue8a5";
        actionIconMap [menu->addAction( createFluentIcon( "\ue8b5" ), "新建项目" )] = "\ue8b5";
        actionIconMap [menu->addAction( createFluentIcon( "\ue8c3" ), "最近打开" )] = "\ue8c3";
        actionIconMap [menu->addAction( createFluentIcon( "\ue8a5" ), "打开文件" )] = "\ue8a5";
        ui->toolButton_3->setMenu( menu );

        //toolButton_4添加图标和文字，上下
        ui->toolButton_4->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
        ui->toolButton_4->setText( "上下按钮" );
        ui->toolButton_4->setIcon( createFluentIcon( "\uE804" ) );

        ui->toolButton_4->setMenu( menu );
    }

    {
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

#include <QApplication>
// void MainWindow::on_horizonalBtn_clicked()
// {
//     ui->spinBox->setProperty( "spinBoxButtonLayout", ArrowsVertical );
//     QSize s = ui->spinBox->size();

//     ui->spinBox->resize(s.width() + 1, s.height());

//     // QTimer::singleShot(0, this, [=]{
//     //     ui->spinBox->resize(s);
//     // });
// }

// void MainWindow::on_verticalBtn_clicked()
// {
//     ui->spinBox->setProperty( "horizonalButtons", false );
//     QSize s = ui->spinBox->size();

//     ui->spinBox->resize(s.width() + 1, s.height());

//     // QTimer::singleShot(0, this, [=]{
//     //     ui->spinBox->resize(s);
//     // });
// }
