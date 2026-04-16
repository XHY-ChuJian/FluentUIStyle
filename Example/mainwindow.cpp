#include "mainwindow.h"

#include <QAbstractItemView>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QFile>
#include <QGridLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QStatusBar>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTabBar>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include <exstackedwidget.h>
#include <extabwidget.h>
#include <private/qabstractspinbox_p.h>
#include <private/qlineedit_p.h>

#include "qevent.h"
#include "qpainter.h"
#ifndef FLUENT_USE_QT_STYLE
#    include <fluentui3style.h>
#    include <palettemanager.h>

#    include "fluentuiappearance.h"
#endif

#include "qdebug.h"
#include "qeasingcurve.h"
#include "ui_mainwindow.h"

void applyStandardMenuIcons( QMenu* menu, QWidget* widget );
class MenuOffsetFilter : public QObject
{
public:
    explicit MenuOffsetFilter(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::Show)
        {
            if (auto menu = qobject_cast<QMenu*>(obj))
            {
                auto menuParent = menu->parentWidget();
                if (menuParent && menuParent->inherits("QMenuBar"))
                {
                    QPoint pos = menu->pos();
                    menu->move(pos + QPoint(2, 0));
                }
            }
        }

        return QObject::eventFilter(obj, event);
    }
};

#ifdef __MINGW32__
void hookContextMenu(QWidget* w)
{
    if (!w) return;

    w->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(w, &QWidget::customContextMenuRequested, w,
        [w](const QPoint& pos)
        {
            QMenu* menu = nullptr;

            if (auto edit = qobject_cast<QLineEdit*>(w))
            {
                menu = edit->createStandardContextMenu();
            }
            else if (auto text = qobject_cast<QTextEdit*>(w))
            {
                menu = text->createStandardContextMenu();
            }
            else if (auto plain = qobject_cast<QPlainTextEdit*>(w))
            {
                menu = plain->createStandardContextMenu();
            }
            else if (auto combo = qobject_cast<QComboBox*>(w))
            {
                if (combo->isEditable() && combo->lineEdit())
                    menu = combo->lineEdit()->createStandardContextMenu();
            }
            else if (auto spin = qobject_cast<QAbstractSpinBox*>(w))
            {
                if (auto edit = spin->findChild<QLineEdit*>())
                    menu = edit->createStandardContextMenu();
            }

            if (!menu) return;

            applyStandardMenuIcons(menu, w);
            menu->exec(w->mapToGlobal(pos));
            delete menu;
        });
}
#endif
// 存储所有需要更新图标的action及其对应的图标代码
QMap<QAction*, QString> actionIconMap;
// 存储所有需要更新图标的menu及其对应的图标代码
QMap<QMenu*, QString> menuIconMap;

QIcon createFluentIcon( const QString& unicode, QColor color = QColor() )
{
    int pixelSize = 25;
    qreal dpr     = /*qApp->devicePixelRatio()*/ 1;
    QFont iconFont( "Segoe Fluent Icons" );
    iconFont.setPixelSize( pixelSize * dpr );

    // 检查当前主题
    bool isDarkTheme = false;
#if ( QT_VERSION >= QT_VERSION_CHECK( 6, 8, 0 ) )
    isDarkTheme = qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
#    ifdef FLUENT_USE_QT_STYLE
    isDarkTheme = qApp->property( "_q_colorscheme" ).toInt() == 1;
#    else
    isDarkTheme = fluentUIAppearance.theme() == Theme::Dark;
#    endif
#endif

    QPixmap pixmap( 30 * dpr, 30 * dpr );

    pixmap.setDevicePixelRatio( dpr );
    pixmap.fill( Qt::transparent );

    QPainter painter( &pixmap );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
    painter.setFont( iconFont );
    painter.setPen( !color.isValid() ? isDarkTheme ? Qt::white : Qt::black : color );

    painter.drawText( pixmap.rect(), Qt::AlignCenter, unicode );

    return QIcon( pixmap );
}

MainWindow::MainWindow( QWidget* parent )
    : QMainWindow( parent )
    , ui( new Ui::MainWindow )
{
    setAttribute( Qt::WA_StyledBackground );
    ui->setupUi( this );
    qApp->installEventFilter( new MenuOffsetFilter( qApp ) );

    setWindowTitle( QString( "FluentUI Demo - QStyle [Qt-Verison %1]" ).arg( QT_VERSION_STR ) );
    QList<QWidget*> widgetList;
    widgetList << ui->widget << ui->widget_2 << ui->widget_3 << ui->widget_4 << ui->widget_5 << ui->widget_6 << ui->widget_7 << ui->widget_8
               << ui->widget_9 << ui->widget_10 << ui->widget_12 << ui->widget_13 << ui->widget_14;
    for ( QWidget* w : std::as_const( widgetList ) )
    {
        // draw border in style
        w->setAttribute( Qt::WA_StyledBackground );
        w->setProperty( "fluentBorder", true );
    }

    // ui->tableWidget->verticalHeader()->setMinimumSectionSize( 32 );

    init();
    ui->stackedWidget->setCurrentIndex( 0 );

    ui->comboBox->setEditable( false );
    ui->comboBox->clear();
    ui->comboBox->addItem( "窗含西岭千秋雪" );
    ui->comboBox->addItem( "门泊东吴万里船" );
    ui->comboBox->addItem( "日照香炉生紫烟" );
    ui->comboBox->addItem( "遥看瀑布挂前川" );
    ui->comboBox->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
    ui->comboBox->setSizeAdjustPolicy( QComboBox::AdjustToContents );

    QStyleOptionComboBox opt;
    opt.editable = false;
    if ( style()->styleHint( QStyle::SH_ComboBox_Popup, &opt, nullptr ) > 0 )
    {
        ui->comboBox->setView( new QListView() );
    }

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    ui->comboBox->setView( new QListView() );
#endif

    loadChangelog();
    updateActionIcons();

#ifdef __MINGW32__
    for (auto w : findChildren<QWidget*>())
    {
        hookContextMenu(w);
    }
#endif
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
        for ( const QString& line : std::as_const( lines ) )
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
    cb->setProperty( "isSwitchButton", true );
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
#ifdef FLUENT_USE_QT_STYLE
    themeComboBox->setCurrentIndex( qApp->property( "_q_colorscheme" ).toInt() == 1 ? 1 : 0 );

#    if QT_VERSION > QT_VERSION_CHECK( 6, 8, 0 )
    themeComboBox->setCurrentIndex( (int)( qApp->styleHints()->colorScheme() ) - 1 );
#    else
    themeComboBox->setCurrentIndex( qApp->property( "_q_colorscheme" ).toInt() == 1 ? 1 : 0 );
#    endif

#else
    themeComboBox->setCurrentIndex( fluentUIAppearance.theme() == Theme::Dark ? 1 : 0 );
#endif
    connect( themeComboBox,
             QOverload<int>::of( &QComboBox::currentIndexChanged ),
             this,
             [ = ]( int index )
             {
#ifdef FLUENT_USE_QT_STYLE

#    if QT_VERSION > QT_VERSION_CHECK( 6, 8, 0 )
                 qApp->styleHints()->setColorScheme( Qt::ColorScheme( index + 1 ) );
#    else
    qApp->setProperty("_q_colorscheme", index);
#    endif
                 qApp->setStyle( "FluentUI3" );
#else
                fluentUIAppearance.setTheme( index == 0 ? Theme::Light : Theme::Dark );
#endif

                 updateActionIcons();
             } );
    toolBar->addWidget( themeComboBox );
    toolBar->addSeparator();
    QLabel* colorSchemeLabel = new QLabel( "配色：", this );
    toolBar->addWidget( colorSchemeLabel );
    QComboBox* colorSchemeComboBox = new QComboBox( this );
    colorSchemeComboBox->blockSignals( true );
    colorSchemeComboBox->addItem( "Fluent" );
    colorSchemeComboBox->addItem( "Teams" );
    colorSchemeComboBox->blockSignals( false );
    colorSchemeComboBox->setView( new QListView );
    connect( colorSchemeComboBox,
             QOverload<int>::of( &QComboBox::currentIndexChanged ),
             this,
             [ = ]( int index )
             {
#ifdef FLUENT_USE_QT_STYLE
                 qApp->setProperty( "_q_themestyle", index );
                 qApp->setStyle( "FluentUI3" );
#else
        PaletteManager::instance().setThemeStyle( index == 0 ? ThemeStyle::Fluent : ThemeStyle::Teams );
        fluentUIAppearance.setTheme( fluentUIAppearance.theme() );
#endif

                 updateActionIcons();
             } );
    toolBar->addWidget( colorSchemeComboBox );

    toolBar->addSeparator();
    // QLabel(样式) + QcomboBox(切换QStyle QStyleFactory::keys())
    QLabel* styleLabel = new QLabel( "样式：", this );
    toolBar->addWidget( styleLabel );
    QComboBox* styleComboBox = new QComboBox( this );
    styleComboBox->addItems( QStyleFactory::keys() );
    styleComboBox->setView( new QListView );
    connect( styleComboBox,
             QOverload<int>::of( &QComboBox::currentIndexChanged ),
             this,
             [ = ]( int index )
             {
                 QString styleName = styleComboBox->itemText( index );
                 qApp->setStyle( styleName );
                 updateActionIcons();
             } );
    toolBar->addWidget( styleComboBox );
}

void MainWindow::setupTabs()
{
    QVBoxLayout* pageLayout = new QVBoxLayout( ui->page_4 );
    pageLayout->setContentsMargins( 0, 0, 0, 0 );
    pageLayout->setSpacing( 0 );

    QScrollArea* scrollArea = new QScrollArea( ui->page_4 );
    scrollArea->setWidgetResizable( true );
    scrollArea->setFrameShape( QFrame::NoFrame );
    scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    QWidget* contentWidget  = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout( contentWidget );
    mainLayout->setContentsMargins( 10, 10, 10, 10 );
    mainLayout->setSpacing( 15 );

    scrollArea->setWidget( contentWidget );
    pageLayout->addWidget( scrollArea );

    // ============多种TabBar样式 (Pivot_Grow, Pivot_Slide, Pivot_Stretch) ============
    QWidget* pivotWidget = new QWidget();
    pivotWidget->setProperty( "fluentBorder", true );
    pivotWidget->setAttribute( Qt::WA_StyledBackground );

    QVBoxLayout* pivotLayout = new QVBoxLayout( pivotWidget );
    pivotLayout->setContentsMargins( 10, 10, 10, 10 );
    pivotLayout->setSpacing( 10 );

    // Pivot_Grow
    QLabel* pivotGrowLabel = new QLabel( "Pivot Grow TabBar" );
    pivotGrowLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
    QLabel* pivotGrowDescLabel = new QLabel( "特点：选中时会有一个生长动画效果。" );
    pivotGrowDescLabel->setStyleSheet( "color: gray; font-size: 12px;" );
    pivotLayout->addWidget( pivotGrowLabel );
    pivotLayout->addWidget( pivotGrowDescLabel );

    QTabBar* pivotGrowBar = new QTabBar();
    pivotGrowBar->setExpanding( false );
    pivotGrowBar->setProperty( "tabBarStyle", 2 );  // TabBarStyle::Pivot_Grow
    pivotGrowBar->addTab( "Home" );
    pivotGrowBar->addTab( "Search" );
    pivotGrowBar->addTab( "Settings" );
    pivotGrowBar->addTab( "Help" );
    pivotGrowBar->addTab( "About" );
    pivotLayout->addWidget( pivotGrowBar );

    // Pivot_Slide
    QLabel* pivotSlideLabel = new QLabel( "Pivot Slide TabBar" );
    pivotSlideLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
    QLabel* pivotSlideDescLabel = new QLabel( "特点：选中时会有一个滑动动画效果。" );
    pivotSlideDescLabel->setStyleSheet( "color: gray; font-size: 12px;" );
    pivotLayout->addWidget( pivotSlideLabel );
    pivotLayout->addWidget( pivotSlideDescLabel );

    QTabBar* pivotSlideBar = new QTabBar();
    pivotSlideBar->setExpanding( false );
    pivotSlideBar->setProperty( "tabBarStyle", 3 );  // TabBarStyle::Pivot_Slide
    pivotSlideBar->addTab( "Home" );
    pivotSlideBar->addTab( "Search" );
    pivotSlideBar->addTab( "Settings" );
    pivotSlideBar->addTab( "Help" );
    pivotSlideBar->addTab( "About" );
    pivotLayout->addWidget( pivotSlideBar );

    // Pivot_Stretch
    QLabel* pivotStretchLabel = new QLabel( "Pivot Stretch TabBar" );
    pivotStretchLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
    QLabel* pivotStretchDescLabel = new QLabel( "特点：选中时会有一个拉伸动画效果。" );
    pivotStretchDescLabel->setStyleSheet( "color: gray; font-size: 12px;" );
    pivotLayout->addWidget( pivotStretchLabel );
    pivotLayout->addWidget( pivotStretchDescLabel );

    QTabBar* pivotStretchBar = new QTabBar();
    pivotStretchBar->setExpanding( false );
    pivotStretchBar->setProperty( "tabBarStyle", 4 );  // TabBarStyle::Pivot_Stretch
    pivotStretchBar->addTab( "Home" );
    pivotStretchBar->addTab( "Search" );
    pivotStretchBar->addTab( "Settings" );
    pivotStretchBar->addTab( "Help" );
    pivotStretchBar->addTab( "About" );

    pivotLayout->addWidget( pivotStretchBar );

    pivotLayout->addStretch();
    mainLayout->addWidget( pivotWidget, 1 );

    // ============ Segmented TabBar ============
    {
        QWidget* segmentedWidget = new QWidget();
        segmentedWidget->setProperty( "fluentBorder", true );
        segmentedWidget->setAttribute( Qt::WA_StyledBackground );

        QVBoxLayout* segmentedLayout = new QVBoxLayout( segmentedWidget );
        segmentedLayout->setContentsMargins( 10, 10, 10, 10 );
        segmentedLayout->setSpacing( 10 );

        QLabel* segmentedLabel = new QLabel( "Segmented Slide TabBar" );
        segmentedLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
        QLabel* segmentedDescLabel = new QLabel( "特点：Segmented风格， 选中时会有一个滑动动画效果。" );
        segmentedDescLabel->setStyleSheet( "color: gray; font-size: 12px;" );
        segmentedLayout->addWidget( segmentedLabel );
        segmentedLayout->addWidget( segmentedDescLabel );

        m_segmentedBar = new QTabBar();
        m_segmentedBar->setTabsClosable( false );
        m_segmentedBar->setMovable( false );
        m_segmentedBar->setExpanding( false );
        m_segmentedBar->setProperty( "tabBarStyle", 6 );  // TabBarStyle::Segmented_Slide
        m_segmentedBar->addTab( "Home" );
        m_segmentedBar->addTab( "Search" );
        m_segmentedBar->addTab( "Settings" );
        m_segmentedBar->addTab( "Help" );
        m_segmentedBar->addTab( "About" );
        segmentedLayout->addWidget( m_segmentedBar );

        QLabel* segmentedFadeLabel = new QLabel( "Segmented Fade TabBar" );
        segmentedFadeLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
        segmentedLayout->addWidget( segmentedFadeLabel );
        QLabel* segmentedFadeDescLabel = new QLabel( "特点：选中时会有一个淡入淡出动画效果。" );
        segmentedFadeDescLabel->setStyleSheet( "color: gray; font-size: 12px;" );
        segmentedLayout->addWidget( segmentedFadeDescLabel );

        m_segmentedFadeBar = new QTabBar();
        m_segmentedFadeBar->setTabsClosable( false );
        m_segmentedFadeBar->setMovable( true );
        m_segmentedFadeBar->setExpanding( false );
        m_segmentedFadeBar->setProperty( "tabBarStyle", 7 );  // TabBarStyle::Segmented_Fade
        m_segmentedFadeBar->addTab( "Home" );
        m_segmentedFadeBar->addTab( "Search" );
        m_segmentedFadeBar->addTab( "Settings" );
        m_segmentedFadeBar->addTab( "Help" );
        m_segmentedFadeBar->addTab( "About" );
        segmentedLayout->addWidget( m_segmentedFadeBar );

        segmentedLayout->addStretch();
        mainLayout->addWidget( segmentedWidget, 1 );
    }
    //========PillTabs===========
    {
        QWidget* pillWidget = new QWidget();
        pillWidget->setProperty( "fluentBorder", true );
        pillWidget->setAttribute( Qt::WA_StyledBackground );

        QVBoxLayout* pillLayout = new QVBoxLayout( pillWidget );
        pillLayout->setContentsMargins( 10, 10, 10, 10 );
        pillLayout->setSpacing( 10 );

        QLabel* pillLabel = new QLabel( "Pill TabBar" );
        pillLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
        pillLayout->addWidget( pillLabel );

        QTabBar* pillBar = new QTabBar();
        pillBar->setTabsClosable( true );
        pillBar->setExpanding( false );
        pillBar->setProperty( "tabBarStyle", 5 );  // TabBarStyle::PillTabs
        pillBar->addTab( "Home" );
        pillBar->addTab( "Search" );
        pillBar->addTab( "Settings" );
        pillBar->addTab( "Help" );
        pillBar->addTab( "About" );
        pillLayout->addWidget( pillBar );

        pillLayout->addStretch();
        mainLayout->addWidget( pillWidget, 1 );
    }

    // ============ CapsuleTabBar + SlidingStackedWidget ============
    QWidget* capsuleWidget = new QWidget();
    capsuleWidget->setProperty( "fluentBorder", true );
    capsuleWidget->setAttribute( Qt::WA_StyledBackground );

    QVBoxLayout* capsuleLayout = new QVBoxLayout( capsuleWidget );
    capsuleLayout->setContentsMargins( 10, 10, 10, 10 );
    capsuleLayout->setSpacing( 2 );

    QLabel* capsuleLabel = new QLabel( "Capsule TabBar" );
    capsuleLabel->setStyleSheet( "font-weight: bold; font-size: 14px; " );
    capsuleLayout->addWidget( capsuleLabel );
    QLabel* capsuleDescLabel = new QLabel( "特点：浏览器标签样式。" );
    capsuleDescLabel->setStyleSheet( "color: gray; font-size: 12px;" );
    capsuleLayout->addWidget( capsuleDescLabel );

    m_capsuleTabWidget = new ExTabWidget();
    m_capsuleTabWidget->setMinimumHeight( 200 );
    m_capsuleTabWidget->setTabsClosable( true );
    m_capsuleTabWidget->setMovable( true );
    
    QTabBar* capTabBar = m_capsuleTabWidget->tabBar();
    capTabBar->setAttribute( Qt::WA_StyledBackground, true );
    capTabBar->setAutoFillBackground( false );
    capTabBar->setExpanding( false );
    capTabBar->setProperty( "TextAlign", static_cast<int>( Qt::AlignVCenter | Qt::AlignLeft ) );
    capTabBar->setProperty( "tabBarStyle", 1 );  // TabBarStyle::Capsule
    capTabBar->setDrawBase( false );

    QStringList pageNames    = { "Home", "Search", "Settings", "Help", "About" };
    QStringList fullNames    = { "Home Page", "Search Page", "Settings Page", "Help Page", "About Page" };
    QList<QColor> pageColors = {
        QColor( 255, 228, 225 ), QColor( 224, 255, 255 ), QColor( 240, 255, 240 ), QColor( 255, 250, 205 ), QColor( 230, 230, 250 )
    };
    for ( int i = 0; i < pageNames.size(); ++i )
    {
        QLabel* page = new QLabel( fullNames[ i ] );
        page->setAlignment( Qt::AlignCenter );
        page->setStyleSheet( QString( "background-color: %1;color:black;" ).arg( pageColors[ i ].name() ) );
        m_capsuleTabWidget->addTab( page, pageNames[ i ] );
    }

    capsuleLayout->addWidget( m_capsuleTabWidget, 1 );
    mainLayout->addWidget( capsuleWidget, 1 );

    // ============Navigation TabBar + SlidingStackedWidget ============
    {
        QWidget* navigationWidget = new QWidget();
        navigationWidget->setProperty( "fluentBorder", true );
        navigationWidget->setAttribute( Qt::WA_StyledBackground );

        QVBoxLayout* navigationLayout = new QVBoxLayout( navigationWidget );
        navigationLayout->setContentsMargins( 10, 10, 10, 10 );
        navigationLayout->setSpacing( 10 );

        QLabel* navigationLabel = new QLabel( "Navigation TabBar" );
        navigationLabel->setStyleSheet( "font-weight: bold; font-size: 14px;" );
        navigationLayout->addWidget( navigationLabel );
        QLabel* navigationDescLabel = new QLabel( "特点：适合用于侧边栏的导航菜单，选项卡垂直排列，选中时指示器有个变长效果" );
        navigationDescLabel->setStyleSheet( "color: gray; font-size: 12px;" );
        navigationLayout->addWidget( navigationDescLabel );

        QHBoxLayout* bodyLayout = new QHBoxLayout();
        bodyLayout->setContentsMargins( 0, 0, 0, 0 );
        bodyLayout->setSpacing( 2 );

        m_navigationTabWidget = new ExTabWidget();
        m_navigationTabWidget->setTabPosition(QTabWidget::West );
        m_navigationTabWidget->setVerticalMode( true );
        m_navigationTabWidget->setSpeed( 220 );
        m_navigationTabWidget->setAnimation( QEasingCurve::OutCubic );
        m_navigationTabWidget->setMinimumHeight( 300 );

        QTabBar* navTabBar = m_navigationTabWidget->tabBar();
        navTabBar->setAttribute( Qt::WA_StyledBackground, true );
        navTabBar->setShape( QTabBar::RoundedWest );
        navTabBar->setDrawBase( false );
        m_navigationTabWidget->setMovable( false );
        navTabBar->setExpanding( false );
        navTabBar->setProperty( "TextAlign", static_cast<int>( Qt::AlignVCenter | Qt::AlignLeft ) );
        navTabBar->setProperty( "tabBarStyle", 8 );  // TabBarStyle::Navigation

        const QStringList navFullNames    = { "Overview Page", "Files Page", "History Page", "Insights Page", "Settings Page" };
        const QStringList navNames        = { "Overview", "Files", "History", "Insights", "Settings" };
        const QList<QColor> navPageColors = {
            QColor( 244, 248, 255 ), QColor( 240, 251, 246 ), QColor( 255, 248, 238 ), QColor( 248, 243, 255 ), QColor( 245, 245, 245 )
        };
        for ( int i = 0; i < navFullNames.size(); ++i )
        {
            QLabel* page = new QLabel( navFullNames[ i ] );
            page->setAlignment( Qt::AlignCenter );
            page->setMinimumHeight( 220 );
            page->setStyleSheet( QString( "background-color:%1;color:black;" ).arg( navPageColors[ i ].name() ) );
            m_navigationTabWidget->addTab( page, navNames[ i ] );
        }

        bodyLayout->addWidget( m_navigationTabWidget, 1 );
        navigationLayout->addLayout( bodyLayout );
        mainLayout->addWidget( navigationWidget, 1 );
    }

    mainLayout->addStretch();
}

void MainWindow::updateActionIcons()
{
    ui->lineEditSerach->removeAction( m_searchAction );
    m_searchAction = ui->lineEditSerach->addAction( createFluentIcon( "\ue721" ), QLineEdit::TrailingPosition );
    {
        if ( m_capsuleTabWidget )
        {
            m_capsuleTabWidget->setTabIcon( 0, createFluentIcon( "\ueA86" ) );
            m_capsuleTabWidget->setTabIcon( 1, createFluentIcon( "\uE7F3" ) );
            m_capsuleTabWidget->setTabIcon( 2, createFluentIcon( "\ue8c3" ) );
            m_capsuleTabWidget->setTabIcon( 3, createFluentIcon( "\uE836" ) );
            m_capsuleTabWidget->setTabIcon( 4, createFluentIcon( "\uE9F5" ) );
        }

        if ( m_segmentedBar )
        {
            m_segmentedBar->setTabIcon( 0, createFluentIcon( "\uEC64" ) );
            m_segmentedBar->setTabIcon( 1, createFluentIcon( "\uEF58" ) );
            m_segmentedBar->setTabIcon( 2, createFluentIcon( "\uE99A" ) );
            m_segmentedBar->setTabIcon( 3, createFluentIcon( "\uE7ED" ) );
            m_segmentedBar->setTabIcon( 4, createFluentIcon( "\uF163" ) );
        }
        if ( m_segmentedFadeBar )
        {
            m_segmentedFadeBar->setTabIcon( 0, createFluentIcon( "\uEC64" ) );
            m_segmentedFadeBar->setTabIcon( 1, createFluentIcon( "\uEF58" ) );
            m_segmentedFadeBar->setTabIcon( 2, createFluentIcon( "\uE99A" ) );
            m_segmentedFadeBar->setTabIcon( 3, createFluentIcon( "\uE7ED" ) );
            m_segmentedFadeBar->setTabIcon( 4, createFluentIcon( "\uF163" ) );
        }
        if ( m_navigationTabWidget )
        {
            m_navigationTabWidget->setTabIcon( 0, createFluentIcon( "\uEC64" ) );
            m_navigationTabWidget->setTabIcon( 1, createFluentIcon( "\uE8B7" ) );
            m_navigationTabWidget->setTabIcon( 2, createFluentIcon( "\uE81C" ) );
            m_navigationTabWidget->setTabIcon( 3, createFluentIcon( "\uE9CE" ) );
            m_navigationTabWidget->setTabIcon( 4, createFluentIcon( "\uE713" ) );
        }
    }

    {
        ui->toolButton->setIcon( createFluentIcon( "\ue8c3" ) );
        ui->pushButton_10->setIcon( createFluentIcon( "\ue713" ) );
        // ui->toolButton_3->setIcon( createFluentIcon( "\uEA8E" ) );
        ui->toolButton_4->setIcon( createFluentIcon( "\uE804" ) );
        ui->tBtnAutoRaise->setIcon( createFluentIcon( "\ue804" ) );
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

    const std::function<void( QTreeWidgetItem* )> updateNavItemIcons = [ & ]( QTreeWidgetItem* item )
    {
        if ( !item )
        {
            return;
        }

        const QString iconCode = item->data( 0, Qt::UserRole + 1 ).toString();
        if ( !iconCode.isEmpty() )
        {
            item->setIcon( 0, createFluentIcon( iconCode ) );
        }

        for ( int i = 0; i < item->childCount(); ++i )
        {
            updateNavItemIcons( item->child( i ) );
        }
    };

    for ( int i = 0; i < ui->navView->topLevelItemCount(); ++i )
    {
        updateNavItemIcons( ui->navView->topLevelItem( i ) );
    }
}

void MainWindow::init()
{
    ui->lineEditSerach->setPlaceholderText( "搜索..." );
    ui->lineEditSerach->setClearButtonEnabled( true );
    m_searchAction = ui->lineEditSerach->addAction( createFluentIcon( "\ue721" ), QLineEdit::TrailingPosition );

    ui->stackedWidget->setVerticalMode( true );
    ui->stackedWidget->setAnimation( QEasingCurve::Type::InOutSine );
    ui->stackedWidget->setSpeed( 300 );

    initMenuAndToolBar();
    initNavigationView();
    initTableView();

    {
        setProperty( "MainBackground", true );
        ui->centralwidget->setProperty( "MainBackground", true );

        ui->centralwidget->setAttribute( Qt::WA_TranslucentBackground, true );
        ui->centralwidget->setAttribute( Qt::WA_StyledBackground, true );

        menuBar()->setAttribute( Qt::WA_TranslucentBackground, true );
        menuBar()->setAttribute( Qt::WA_StyledBackground, false );
        menuBar()->setAutoFillBackground( false );

        m_toolBar->setAttribute( Qt::WA_TranslucentBackground, true );
        m_toolBar->setAttribute( Qt::WA_StyledBackground, false );
        m_toolBar->setAutoFillBackground( false );
    }

    ui->progressBar->setProperty( "progressBarStyle", 1 );

    ui->spinBox->setProperty( "spinBoxButtonLayout", 0 );  // ArrowsVertical
    ui->checkBox_5->setText( "Off" );
    ui->treeWidget->setProperty( "ItemHeight", 32 );

    {
        setupTabs();
    }

    {
        ui->scrollAreaWidgetContents->setAutoFillBackground( false );
    }

    {
        // pushButton、pushButton_2、toolButton添加图标
        ui->toolButton->setIcon( createFluentIcon( "\ue8c3" ) );

        // toolButton_2添加文字和图标
        ui->pushButton_10->setText( "工具按钮" );
        ui->pushButton_10->setIcon( createFluentIcon( "\ue713" ) );

        // toolButton_3 带文字和图标以及4个菜单
        ui->toolButton_3->setAutoRaise( false );
        ui->toolButton_3->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
        ui->toolButton_3->setPopupMode( QToolButton::InstantPopup );
        ui->toolButton_3->setText( "菜单按钮" );
        QMenu* menu                                                                  = new QMenu( ui->toolButton_3 );
        actionIconMap[ menu->addAction( createFluentIcon( "\ue8a5" ), "新建文件" ) ] = "\ue8a5";
        actionIconMap[ menu->addAction( createFluentIcon( "\ue8b5" ), "新建项目" ) ] = "\ue8b5";
        actionIconMap[ menu->addAction( createFluentIcon( "\ue8c3" ), "最近打开" ) ] = "\ue8c3";
        actionIconMap[ menu->addAction( createFluentIcon( "\ue8a5" ), "打开文件" ) ] = "\ue8a5";
        ui->toolButton_3->setMenu( menu );

        // toolButton_4添加图标和文字，上下
        ui->toolButton_4->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
        ui->toolButton_4->setText( "上下按钮" );
        ui->toolButton_4->setIcon( createFluentIcon( "\uE804" ) );

        ui->toolButton_4->setMenu( menu );

        ui->tBtnAutoRaise->setIcon( createFluentIcon( "\ue804" ) );
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

void MainWindow::initTableView()
{
      auto *table = ui->tableWidget;

    // 1️⃣ 清空并设置结构
    table->clear();
    table->setRowCount(5);
    table->setColumnCount(4);

    QStringList headers;
    headers << "Name" << "Age" << "Type" << "Score";
    table->setHorizontalHeaderLabels(headers);

    // 2️⃣ 基础属性（推荐）
    table->setEditTriggers(QAbstractItemView::DoubleClicked |
                           QAbstractItemView::EditKeyPressed);

    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);

    // =========================
    // 3️⃣ 第一列：可编辑文本
    // =========================
    for (int row = 0; row < 5; ++row)
    {
        auto *item = new QTableWidgetItem(QString("User_%1").arg(row));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        table->setItem(row, 0, item);
    }

    // =========================
    // 4️⃣ 第二列：SpinBox
    // =========================
    for (int row = 0; row < 5; ++row)
    {
        auto *spin = new QSpinBox(table);
        spin->setRange(0, 120);
        spin->setValue(20 + row);
        spin->setAlignment(Qt::AlignCenter);

        table->setCellWidget(row, 1, spin);
    }

    // =========================
    // 5️⃣ 第三列：ComboBox
    // =========================
    QStringList types = {"Admin", "User", "Guest"};

    for (int row = 0; row < 5; ++row)
    {
        auto *combo = new QComboBox(table);
        combo->addItems(types);
        combo->setCurrentIndex(row % types.size());

        table->setCellWidget(row, 2, combo);
    }

    // =========================
    // 6️⃣ 第四列：可编辑数字
    // =========================
    for (int row = 0; row < 5; ++row)
    {
        auto *item = new QTableWidgetItem(QString::number(60.5 + row));
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        table->setItem(row, 3, item);
    }
}

void MainWindow::initNavigationView()
{
    // ui->navView 字号 + 1
    QFont navFont = ui->navView->font();
    navFont.setPixelSize( navFont.pixelSize() + 1 );
    navFont.setHintingPreference( QFont::PreferFullHinting );

    ui->navView->setIconSize( QSize( 20, 20 ) );

    ui->navView->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->navView->setFont( navFont );
    ui->navView->setRootIsDecorated( false );
    ui->navView->setFrameShape( QFrame::NoFrame );
    ui->navView->setStyleSheet( "#navView{background:transparent;}" );

    ui->navView->setProperty( "navigationViewIndicator", true );
    // ui->navView 添加一级节点 {基础控件，List, Tree, TabWidget, SrollArea}
    QTreeWidgetItem* basicItem = new QTreeWidgetItem( ui->navView );
    basicItem->setText( 0, "基础控件" );
    basicItem->setData( 0, Qt::UserRole, 0 );
    basicItem->setData( 0, Qt::UserRole + 1, "\uE80F" );

    QTreeWidgetItem* tableItem = new QTreeWidgetItem( ui->navView );
    tableItem->setText( 0, "表格控件" );
    tableItem->setData( 0, Qt::UserRole, 1 );
    tableItem->setData( 0, Qt::UserRole + 1, "\uE99A" );

    QTreeWidgetItem* listItem = new QTreeWidgetItem( ui->navView );
    listItem->setText( 0, "列表控件" );
    listItem->setData( 0, Qt::UserRole, 2 );
    listItem->setData( 0, Qt::UserRole + 1, "\uE71D" );

    QTreeWidgetItem* treeItem = new QTreeWidgetItem( ui->navView );
    treeItem->setText( 0, "树形控件" );
    treeItem->setData( 0, Qt::UserRole, 3 );
    treeItem->setData( 0, Qt::UserRole + 1, "\uED28" );

    QTreeWidgetItem* tabItem = new QTreeWidgetItem( ui->navView );
    tabItem->setText( 0, "导航控件" );
    tabItem->setData( 0, Qt::UserRole, 4 );
    tabItem->setData( 0, Qt::UserRole + 1, "\uE8B0" );

    QTreeWidgetItem* mdiItem = new QTreeWidgetItem( ui->navView );
    mdiItem->setText( 0, "Mdi" );
    mdiItem->setData( 0, Qt::UserRole, 5 );
    mdiItem->setData( 0, Qt::UserRole + 1, "\uE9D9" );

    QTreeWidgetItem* settingItem = new QTreeWidgetItem( ui->navView );
    settingItem->setText( 0, "设置" );
    settingItem->setData( 0, Qt::UserRole, 6 );
    settingItem->setData( 0, Qt::UserRole + 1, "\uE9F5" );

    connect( ui->navView,
             &QTreeWidget::currentItemChanged,
             this,
             [ this ]( QTreeWidgetItem* current, QTreeWidgetItem* )
             {
                 if ( !current )
                 {
                     return;
                 }
                 const QVariant pageIndex = current->data( 0, Qt::UserRole );
                 if ( pageIndex.isValid() )
                 {
                     ui->stackedWidget->setCurrentIndex( pageIndex.toInt() );
                 }
             } );
    ui->navView->setCurrentItem( basicItem );

    // 最后在添加一个节点，有很多测试子节点，嵌套3次
    QTreeWidgetItem* testItem = new QTreeWidgetItem( ui->navView );
    testItem->setText( 0, "测试节点" );
    testItem->setData( 0, Qt::UserRole, 6 );
    testItem->setData( 0, Qt::UserRole + 1, "\uE9F5" );
    for ( int i = 0; i < 5; ++i )
    {
        QTreeWidgetItem* child = new QTreeWidgetItem( testItem );
        child->setText( 0, QString( "子节点%1" ).arg( i + 1 ) );
        child->setData( 0, Qt::UserRole, 6 );

        for ( int j = 0; j < 3; ++j )
        {
            QTreeWidgetItem* subChild = new QTreeWidgetItem( child );
            subChild->setText( 0, QString( "子节点%1-%2" ).arg( i + 1 ).arg( j + 1 ) );
            subChild->setData( 0, Qt::UserRole, 6 );
        }
    }
}

void MainWindow::on_checkBox_4_clicked( bool checked )
{
    QList<QCheckBox*> cbs;
    cbs << ui->checkBox_5 /*<< ui->checkBox_6 << ui->checkBox_7 << ui->checkBox_8 << ui->checkBox_9*/;
    for ( QCheckBox* cb : std::as_const( cbs ) )
    {
        cb->setChecked( checked );
    }
}

enum class SpinBoxButton
{
    ArrowsVertical,
    ArrowsHorizontalSides,
    ArrowsHorizontalRight,
    PlusMinusHorizontalSides
};

void MainWindow::on_checkBox_5_stateChanged( int arg1 )
{
    ui->checkBox_5->setText( arg1 == Qt::Checked ? "On" : "Off" );
}

void MainWindow::on_radioButton_7_clicked()
{
    ui->spinBox->setProperty( "spinBoxButtonLayout", (int)SpinBoxButton::ArrowsVertical );
    ui->spinBox->setFrame( ui->spinBox->hasFrame() );
}

void MainWindow::on_radioButton_4_clicked()
{
    ui->spinBox->setProperty( "spinBoxButtonLayout", (int)SpinBoxButton::ArrowsHorizontalSides );
    ui->spinBox->setFrame( ui->spinBox->hasFrame() );
}

void MainWindow::on_radioButton_5_clicked()
{
    ui->spinBox->setProperty( "spinBoxButtonLayout", (int)SpinBoxButton::ArrowsHorizontalRight );
    ui->spinBox->setFrame( ui->spinBox->hasFrame() );
}

void MainWindow::on_radioButton_6_clicked()
{
    ui->spinBox->setProperty( "spinBoxButtonLayout", (int)SpinBoxButton::PlusMinusHorizontalSides );
    ui->spinBox->setFrame( ui->spinBox->hasFrame() );
}

void applyStandardMenuIcons( QMenu* menu, QWidget* widget )
{
    if ( !menu )
    {
        return;
    }

    const QColor iconColor = widget ? widget->palette().color( QPalette::Text ) : QApplication::palette().color( QPalette::Text );

    // Segoe Fluent / MDL2 glyphs
    constexpr QChar GlyphUndo( 0xE7A7 );
    constexpr QChar GlyphRedo( 0xE7A6 );
    constexpr QChar GlyphCut( 0xE8C6 );
    constexpr QChar GlyphCopy( 0xE8C8 );
    constexpr QChar GlyphPaste( 0xE77F );
    constexpr QChar GlyphSelectAll( 0xE8B3 );
    constexpr QChar GlyphDelete( 0xE74D );
    constexpr QChar GlyphUp( 0xE70E );   // ChevronUp
    constexpr QChar GlyphDown( 0xE70D ); // ChevronDown

    const auto actions = menu->actions();
    for ( QAction* act : actions )
    {
        if ( !act || act->isSeparator() )
        {
            continue;
        }
        if ( QMenu* sub = act->menu() )
        {
            applyStandardMenuIcons( sub, widget );
            continue;
        }

        if ( !act->icon().isNull() )
        {
            continue;
        }

        QString text = act->text();
        text.remove( '&' );
        const QString t = text.toLower();

        if ( t.contains( "undo" ) )
        {
            act->setIcon( createFluentIcon( GlyphUndo ) );
        }
        else if ( t.contains( "redo" ) )
        {
            act->setIcon( createFluentIcon( GlyphRedo ) );
        }
        else if ( t.contains( "cut" ) )
        {
            act->setIcon( createFluentIcon( GlyphCut ) );
        }
        else if ( t.contains( "copy" ) )
        {
            act->setIcon( createFluentIcon( GlyphCopy ) );
        }
        else if ( t.contains( "paste" ) )
        {
            act->setIcon( createFluentIcon( GlyphPaste ) );
        }
        else if ( t.contains( "select all" ) || t.contains( "selectall" ) )
        {
            act->setIcon( createFluentIcon( GlyphSelectAll ) );
        }
        else if ( t.contains( "delete" ) || t.contains( "clear" ) )
        {
            act->setIcon( createFluentIcon( GlyphDelete ) );
        }
        else if ( t.contains( "step up" ) )
        {
            act->setIcon( createFluentIcon( GlyphUp ) );
        }
        else if ( t.contains( "step down" ) )
        {
            act->setIcon( createFluentIcon( GlyphDown ) );
        }
    }
}

#ifndef __MINGW32__
void QLineEdit::contextMenuEvent( QContextMenuEvent* event )
{
    if ( QMenu* menu = createStandardContextMenu() )
    {
        applyStandardMenuIcons( menu, this );
        menu->setAttribute( Qt::WA_DeleteOnClose );
        menu->exec( event->globalPos() );
    }
}

void QTextEdit::contextMenuEvent( QContextMenuEvent* e )
{
    if ( QMenu* menu = createStandardContextMenu() )
    {
        applyStandardMenuIcons( menu, this );
        menu->setAttribute( Qt::WA_DeleteOnClose );
        menu->exec( e->globalPos() );
    }
}

void QComboBox::contextMenuEvent( QContextMenuEvent* e )
{
    if ( lineEdit() )
    {
        if ( QMenu* menu = lineEdit()->createStandardContextMenu() )
        {
            applyStandardMenuIcons( menu, lineEdit() );
            menu->setAttribute( Qt::WA_DeleteOnClose );
            menu->exec( e->globalPos() );
        }
    }
}

void QAbstractSpinBox::contextMenuEvent( QContextMenuEvent* event )
{
    Q_D( QAbstractSpinBox );

    QPointer<QMenu> menu = d->edit->createStandardContextMenu();
    if ( !menu )
    {
        return;
    }

    // d->reset();

    QAction* selAll = new QAction( tr( "&Select All" ), menu );
#if QT_CONFIG( shortcut )
    selAll->setShortcut( QKeySequence::SelectAll );
#endif
    menu->insertAction( d->edit->d_func()->selectAllAction, selAll );
    menu->removeAction( d->edit->d_func()->selectAllAction );
    menu->addSeparator();
    const uint se = stepEnabled();
    QAction* up   = menu->addAction( tr( "&Step up" ) );
    up->setEnabled( se & StepUpEnabled );
    QAction* down = menu->addAction( tr( "Step &down" ) );
    down->setEnabled( se & StepDownEnabled );
    menu->addSeparator();

    applyStandardMenuIcons(menu, this);

    const QPointer<QAbstractSpinBox> that = this;
    const QPoint pos                      = ( event->reason() == QContextMenuEvent::Mouse )
                                                ? event->globalPos()
                                                : mapToGlobal( QPoint( event->pos().x(), 0 ) ) + QPoint( width() / 2, height() / 2 );
    const QAction* action = menu->exec( pos );
    delete static_cast<QMenu*>( menu );
    if ( that && action )
    {
        if ( action == up )
        {
            stepBy( 1 );
        }
        else if ( action == down )
        {
            stepBy( -1 );
        }
        else if ( action == selAll )
        {
            selectAll();
        }
    }
    event->accept();
}
#endif

