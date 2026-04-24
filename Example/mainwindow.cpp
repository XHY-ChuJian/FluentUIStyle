//=============================================================================
// MainWindow Implementation
// FluentUI3 Style Demo Application
//=============================================================================

#include "mainwindow.h"

#include "ui_mainwindow.h"

// Qt Core Headers
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QEasingCurve>
#include <QEvent>
#include <QFile>
#include <QKeySequence>
#include <QPainter>
#include <QScreen>
#include <QSvgRenderer>
#include <QTextStream>
#include <QTimer>

// Qt GUI Headers
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>

// Qt Widgets Headers
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QListView>
#include <QMap>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QStyleOptionComboBox>
#include <QTabBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

// Project Headers
#include <exstackedwidget.h>
#include <extabwidget.h>

#ifndef FLUENT_USE_QT_STYLE
#include <fluentui3style.h>
#include <palettemanager.h>

#include "fluentuiappearance.h"
#endif

// Private Qt Headers (for custom context menu)
#include <private/qabstractspinbox_p.h>
#include <private/qlineedit_p.h>

//=============================================================================
// Forward Declarations
//=============================================================================

void applyStandardMenuIcons(QMenu *menu, QWidget *widget);
static inline void emulateLeaveEvent(QWidget *widget);

//=============================================================================
// Global Variables
//=============================================================================

// Icon maps for menu and action icons
static QMap<QAction *, QString> g_actionIconMap;
static QMap<QMenu *, QString> g_menuIconMap;

//=============================================================================
// Helper Classes
//=============================================================================

// Menu position adjustment filter
class MenuOffsetFilter : public QObject
{
public:
    explicit MenuOffsetFilter(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::Show)
        {
            if (QMenu *menu = qobject_cast<QMenu *>(obj))
            {
                QWidget *menuParent = menu->parentWidget();
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

//=============================================================================
// MinGW Context Menu Hooks
//=============================================================================

#ifdef __MINGW32__
static void hookContextMenu(QWidget *widget)
{
    if (!widget)
    {
        return;
    }

    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(widget,
                     &QWidget::customContextMenuRequested,
                     widget,
                     [widget](const QPoint &pos)
                     {
                         QMenu *menu = nullptr;

                         if (QLineEdit *edit = qobject_cast<QLineEdit *>(widget))
                         {
                             menu = edit->createStandardContextMenu();
                         }
                         else if (QTextEdit *textEdit = qobject_cast<QTextEdit *>(widget))
                         {
                             menu = textEdit->createStandardContextMenu();
                         }
                         else if (QPlainTextEdit *plainEdit = qobject_cast<QPlainTextEdit *>(widget))
                         {
                             menu = plainEdit->createStandardContextMenu();
                         }
                         else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget))
                         {
                             if (comboBox->isEditable() && comboBox->lineEdit())
                             {
                                 menu = comboBox->lineEdit()->createStandardContextMenu();
                             }
                         }
                         else if (QAbstractSpinBox *spinBox = qobject_cast<QAbstractSpinBox *>(widget))
                         {
                             if (QLineEdit *edit = spinBox->findChild<QLineEdit *>())
                             {
                                 menu = edit->createStandardContextMenu();
                             }
                         }

                         if (!menu)
                         {
                             return;
                         }

                         applyStandardMenuIcons(menu, widget);
                         menu->exec(widget->mapToGlobal(pos));
                         delete menu;
                     });
}
#endif

//=============================================================================
// Fluent Icon Creation
//=============================================================================

QIcon createFluentIcon(const QString &unicode, QColor color = QColor())
{
    const int pixelSize = 25;
    const qreal devicePixelRatio = 1.0;
    QFont iconFont("Segoe Fluent Icons");
    iconFont.setPixelSize(pixelSize * devicePixelRatio);

    // Determine if we're using dark theme
    bool isDarkTheme = false;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
    isDarkTheme = qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
#ifdef FLUENT_USE_QT_STYLE
    isDarkTheme = qApp->property("_q_colorscheme").toInt() == 1;
#else
    isDarkTheme = fluentUIAppearance.theme() == Theme::Dark;
#endif
#endif

    // Create pixmap
    QPixmap pixmap(30 * devicePixelRatio, 30 * devicePixelRatio);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);

    // Draw icon
    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setFont(iconFont);
    painter.setPen(color.isValid() ? color : (isDarkTheme ? Qt::white : Qt::black));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, unicode);

    return QIcon(pixmap);
}

//=============================================================================
// MainWindow Constructor & Destructor
//=============================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_menuBar(nullptr), m_toolBar(nullptr), m_capsuleTabWidget(nullptr), m_segmentedBar(nullptr), m_segmentedFadeBar(nullptr), m_navigationTabWidget(nullptr), m_searchAction(nullptr), m_tabBarWidgetBg(nullptr), m_widgetBgMode(WidgetBgMode::None)
{
    // Setup window attributes
    setAttribute(Qt::WA_StyledBackground);

    // Setup UI
    ui->setupUi(this);

    setWindowTitle(QString("FluentUI Demo - QStyle [Qt-Version %1]").arg(QT_VERSION_STR));

    // Create menu bar
    m_menuBar = new QMenuBar();
    setMenuBar(m_menuBar);

    // Install menu offset filter
    qApp->installEventFilter(new MenuOffsetFilter(qApp));

    // Initialize widgets with fluent border style
    initializeFluentBorderWidgets();

    // Setup table widget
    ui->tableWidget->verticalHeader()->setMinimumSectionSize(32);

    // Initialize main components
    initializeComponents();

    // Setup combo box
    setupComboBox();

    // Load changelog
    loadChangelog();

    // Update icons
    updateActionIcons();

#ifdef __MINGW32__
    // Hook context menus for MinGW
    for (QWidget *widget : findChildren<QWidget *>())
    {
        hookContextMenu(widget);
    }
#endif

    resetPalette();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//=============================================================================
// Event Handlers
//=============================================================================

void MainWindow::paintEvent(QPaintEvent *event)
{
    if (m_widgetBgMode == WidgetBgMode::None)
    {
        QMainWindow::paintEvent(event);
        return;
    }

    if (m_bgLight.isNull() || m_bgDark.isNull())
    {
        return;
    }

    QPainter painter(this);
    const bool isDark = qApp->property("_q_colorscheme").toInt() == 1;
    painter.drawPixmap(rect(), isDark ? m_bgDark : m_bgLight);
}

void MainWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    // Empty implementation - kept for potential future use
}

//=============================================================================
// Initialization Functions
//=============================================================================

void MainWindow::initializeFluentBorderWidgets()
{
    QList<QWidget *> fluentWidgets;
    fluentWidgets << ui->widget << ui->widget_2 << ui->widget_3 << ui->widget_4 << ui->widget_5 << ui->widget_6 << ui->widget_7
                  << ui->widget_8 << ui->widget_9 << ui->widget_10 << ui->widget_12 << ui->widget_13 << ui->widget_14;

    for (QWidget *widget : std::as_const(fluentWidgets))
    {
        widget->setAttribute(Qt::WA_StyledBackground);
        widget->setProperty("isCard", true);
    }
}

void MainWindow::initializeComponents()
{
    // Load background images
    m_bgLight = QPixmap(":/images/bg3.png");
    m_bgDark = QPixmap(":/images/bg2.png");

    // Configure scroll areas
    ui->scrollArea_2->viewport()->setAutoFillBackground(false);
    ui->scrollAreaWidgetContents->setAutoFillBackground(false);
    ui->scrollAreaWidgetContents_4->setAutoFillBackground(false);

    // Configure search line edit
    ui->lineEditSerach->setPlaceholderText("搜索...");
    ui->lineEditSerach->setClearButtonEnabled(true);
    m_searchAction = ui->lineEditSerach->addAction(createFluentIcon("\ue721"), QLineEdit::TrailingPosition);

    // Configure stacked widget
    ui->stackedWidget->setVerticalMode(true);
    ui->stackedWidget->setAnimation(QEasingCurve::Type::InOutSine);
    ui->stackedWidget->setSpeed(300);

    // Initialize sub-components
    initializeMenuAndToolBar();
    initializeNavigationView();
    initializeTableView();

    // Configure background properties
    setProperty("MainBackground", true);
    ui->centralwidget->setProperty("MainBackground", true);
    ui->centralwidget->setAttribute(Qt::WA_TranslucentBackground, true);
    ui->centralwidget->setAttribute(Qt::WA_StyledBackground, true);
    menuBar()->setAttribute(Qt::WA_TranslucentBackground, true);
    menuBar()->setAttribute(Qt::WA_StyledBackground, false);
    menuBar()->setAutoFillBackground(false);
    m_toolBar->setAttribute(Qt::WA_TranslucentBackground, true);
    m_toolBar->setAttribute(Qt::WA_StyledBackground, false);
    m_toolBar->setAutoFillBackground(false);

    // Configure control properties
    ui->progressBar->setProperty("progressBarStyle", 1);
    ui->spinBox->setProperty("spinBoxButtonLayout", 0);
    ui->checkBox_5->setText("Off");
    ui->treeWidget->setProperty("ItemHeight", 32);
    ui->treeWidget->setIndentation(20);

    // Setup tabs
    setupTabs();

    // Setup buttons and icons
    setupButtonsAndIcons();

    // Setup MDI area
    setupMdiArea();

    // Set initial page
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::setupComboBox()
{
    ui->comboBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    ui->comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    QStyleOptionComboBox option;
    option.editable = false;
    if (style()->styleHint(QStyle::SH_ComboBox_Popup, &option, nullptr) > 0)
    {
        ui->comboBox->setView(new QListView());
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ui->comboBox->setView(new QListView());
#endif
}

//=============================================================================
// Menu & Toolbar Functions
//=============================================================================

static void initializeMenu(QMenuBar *menuBar)
{
    // File Menu
    QMenu *fileMenu = menuBar->addMenu("文件");
    QAction *newFileAction = fileMenu->addAction(createFluentIcon("\ue8a5"), "新建文件");
    newFileAction->setShortcut(QKeySequence("Ctrl+N"));
    g_actionIconMap[newFileAction] = "\ue8a5";

    QAction *newProjectAction = fileMenu->addAction(createFluentIcon("\ue8b5"), "新建项目");
    g_actionIconMap[newProjectAction] = "\ue8b5";

    QMenu *recentMenu = fileMenu->addMenu(createFluentIcon("\ue8c3"), "最近打开");
    g_menuIconMap[recentMenu] = "\ue8c3";
    recentMenu->addAction("project1");
    recentMenu->addAction("project2");
    recentMenu->addAction("example.cpp");

    QAction *openFileAction = fileMenu->addAction(createFluentIcon("\ue8a5"), "打开文件");
    openFileAction->setShortcut(QKeySequence("Ctrl+O"));
    g_actionIconMap[openFileAction] = "\ue8a5";

    QAction *openProjectAction = fileMenu->addAction(createFluentIcon("\ue8b5"), "打开项目");
    g_actionIconMap[openProjectAction] = "\ue8b5";

    fileMenu->addSeparator();

    QAction *saveAction = fileMenu->addAction(createFluentIcon("\ue74e"), "保存");
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    g_actionIconMap[saveAction] = "\ue74e";

    QAction *saveAsAction = fileMenu->addAction(createFluentIcon("\ue74e"), "另存为");
    saveAsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    g_actionIconMap[saveAsAction] = "\ue74e";

    fileMenu->addSeparator();

    QAction *closeFileAction = fileMenu->addAction(createFluentIcon("\ue8bb"), "关闭文件");
    g_actionIconMap[closeFileAction] = "\ue8bb";

    QAction *exitAction = fileMenu->addAction(createFluentIcon("\ue8bb"), "退出");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    g_actionIconMap[exitAction] = "\ue8bb";

    // Edit Menu
    QMenu *editMenu = menuBar->addMenu("编辑");
    QAction *undoAction = editMenu->addAction(createFluentIcon("\ue7a7"), "撤销");
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));
    g_actionIconMap[undoAction] = "\ue7a7";

    QAction *redoAction = editMenu->addAction(createFluentIcon("\ue7a6"), "重做");
    redoAction->setShortcut(QKeySequence("Ctrl+Y"));
    g_actionIconMap[redoAction] = "\ue7a6";

    editMenu->addSeparator();

    QAction *cutAction = editMenu->addAction(createFluentIcon("\ue8c6"), "剪切");
    cutAction->setShortcut(QKeySequence("Ctrl+X"));
    g_actionIconMap[cutAction] = "\ue8c6";

    QAction *copyAction = editMenu->addAction(createFluentIcon("\ue8c8"), "复制");
    copyAction->setShortcut(QKeySequence("Ctrl+C"));
    g_actionIconMap[copyAction] = "\ue8c8";

    QAction *pasteAction = editMenu->addAction(createFluentIcon("\ue8c7"), "粘贴");
    pasteAction->setShortcut(QKeySequence("Ctrl+V"));
    g_actionIconMap[pasteAction] = "\ue8c7";

    editMenu->addSeparator();

    QAction *findAction = editMenu->addAction(createFluentIcon("\ue721"), "查找");
    findAction->setShortcut(QKeySequence("Ctrl+F"));
    g_actionIconMap[findAction] = "\ue721";

    QAction *replaceAction = editMenu->addAction(createFluentIcon("\ue8ac"), "替换");
    replaceAction->setShortcut(QKeySequence("Ctrl+H"));
    g_actionIconMap[replaceAction] = "\ue8ac";

    QMenu *advancedMenu = editMenu->addMenu(createFluentIcon("\ue713"), "高级");
    g_menuIconMap[advancedMenu] = "\ue713";
    QAction *autoFormatAction = advancedMenu->addAction(createFluentIcon("\ue930"), "自动格式化");
    autoFormatAction->setCheckable(true);
    g_actionIconMap[autoFormatAction] = "\ue930";

    QAction *sortLinesAction = advancedMenu->addAction(createFluentIcon("\ue930"), "排序行");
    g_actionIconMap[sortLinesAction] = "\ue930";

    QAction *deleteEmptyLinesAction = advancedMenu->addAction(createFluentIcon("\ue8bb"), "删除空行");
    g_actionIconMap[deleteEmptyLinesAction] = "\ue8bb";

    // View Menu
    QMenu *viewMenu = menuBar->addMenu("视图");
    QAction *showToolbarAction = viewMenu->addAction(createFluentIcon("\ue728"), "显示工具栏");
    showToolbarAction->setCheckable(true);
    showToolbarAction->setChecked(true);
    g_actionIconMap[showToolbarAction] = "\ue728";

    QAction *showStatusBarAction = viewMenu->addAction(createFluentIcon("\ue9d9"), "显示状态栏");
    showStatusBarAction->setCheckable(true);
    showStatusBarAction->setChecked(true);
    g_actionIconMap[showStatusBarAction] = "\ue9d9";

    viewMenu->addSeparator();

    QAction *showSidebarAction = viewMenu->addAction(createFluentIcon("\ue728"), "显示侧边栏");
    showSidebarAction->setCheckable(true);
    showSidebarAction->setChecked(true);
    g_actionIconMap[showSidebarAction] = "\ue728";

    QAction *showOutputAction = viewMenu->addAction(createFluentIcon("\ue7e8"), "显示输出窗口");
    showOutputAction->setCheckable(true);
    g_actionIconMap[showOutputAction] = "\ue7e8";

    QMenu *zoomMenu = viewMenu->addMenu(createFluentIcon("\ue71e"), "缩放");
    g_menuIconMap[zoomMenu] = "\ue71e";
    zoomMenu->addAction("放大");
    zoomMenu->addAction("缩小");
    zoomMenu->addAction("恢复默认");

    // Build Menu
    QMenu *buildMenu = menuBar->addMenu("构建");
    QAction *buildProjectAction = buildMenu->addAction(createFluentIcon("\ue7b8"), "构建项目");
    buildProjectAction->setShortcut(QKeySequence("Ctrl+B"));
    g_actionIconMap[buildProjectAction] = "\ue7b8";

    QAction *rebuildAction = buildMenu->addAction(createFluentIcon("\ue7b8"), "重新构建");
    g_actionIconMap[rebuildAction] = "\ue7b8";

    buildMenu->addSeparator();

    QAction *runAction = buildMenu->addAction(createFluentIcon("\ue768"), "运行");
    g_actionIconMap[runAction] = "\ue768";

    QAction *debugAction = buildMenu->addAction(createFluentIcon("\ue7a6"), "调试");
    g_actionIconMap[debugAction] = "\ue7a6";

    QMenu *buildTargetMenu = buildMenu->addMenu(createFluentIcon("\ue8b5"), "构建目标");
    g_menuIconMap[buildTargetMenu] = "\ue8b5";
    buildTargetMenu->addAction("Debug");
    buildTargetMenu->addAction("Release");

    // Help Menu
    QMenu *helpMenu = menuBar->addMenu("帮助");
    QAction *docsAction = helpMenu->addAction(createFluentIcon("\ue8a5"), "文档");
    g_actionIconMap[docsAction] = "\ue8a5";

    QAction *apiAction = helpMenu->addAction(createFluentIcon("\ue8a5"), "API参考");
    g_actionIconMap[apiAction] = "\ue8a5";

    helpMenu->addSeparator();

    QAction *updateAction = helpMenu->addAction(createFluentIcon("\ue7b8"), "检查更新");
    g_actionIconMap[updateAction] = "\ue7b8";

    helpMenu->addSeparator();

    QAction *aboutAction = helpMenu->addAction(createFluentIcon("\ue946"), "关于");
    g_actionIconMap[aboutAction] = "\ue946";
}

void MainWindow::initializeMenuAndToolBar()
{
    // Clear icon maps
    g_actionIconMap.clear();
    g_menuIconMap.clear();

    // Initialize menu
    initializeMenu(m_menuBar);

    // Initialize toolbar
    m_toolBar = addToolBar("工具栏");

    addToolBarAction(m_toolBar, "\uE700", "", QKeySequence(""));
    connect(g_actionIconMap.key("\uE700"), &QAction::triggered, [=]()
            { m_navView->setNavigationExpanded(!m_navView->navigationExpanded()); });
    m_toolBar->addSeparator();

    // Add file actions
    addToolBarAction(m_toolBar, "\ue8a5", "新建", QKeySequence("Ctrl+N"));
    addToolBarAction(m_toolBar, "\ue8a5", "打开", QKeySequence("Ctrl+O"));
    addToolBarAction(m_toolBar, "\ue74e", "保存", QKeySequence("Ctrl+S"));

    m_toolBar->addSeparator();

    // Add edit actions
    addToolBarAction(m_toolBar, "\ue7a7", "撤销", QKeySequence("Ctrl+Z"));
    addToolBarAction(m_toolBar, "\ue7a6", "重做", QKeySequence("Ctrl+Y"));

    m_toolBar->addSeparator();

    addToolBarAction(m_toolBar, "\ue8c6", "剪切", QKeySequence("Ctrl+X"));
    addToolBarAction(m_toolBar, "\ue8c8", "复制", QKeySequence("Ctrl+C"));
    addToolBarAction(m_toolBar, "\ue8c7", "粘贴", QKeySequence("Ctrl+V"));

    m_toolBar->addSeparator();

    // Add build actions
    addToolBarAction(m_toolBar, "\ue7b8", "构建", QKeySequence("Ctrl+B"));
    addToolBarAction(m_toolBar, "\ue7b8", "重新构建");
    addToolBarAction(m_toolBar, "\ue768", "运行");

    m_toolBar->addSeparator();

    // Add toolbar controls
    setupToolBarControls(m_toolBar);
}

void MainWindow::addToolBarAction(QToolBar *toolBar, const QString &iconCode, const QString &text, const QKeySequence &shortcut)
{
    QAction *action = toolBar->addAction(createFluentIcon(iconCode), text);
    if (!shortcut.isEmpty())
    {
        action->setShortcut(shortcut);
    }
    g_actionIconMap[action] = iconCode;
}

void MainWindow::setupToolBarControls(QToolBar *toolBar)
{
    // Disable widget checkbox
    QCheckBox *disableCheckBox = new QCheckBox("禁用", this);
    disableCheckBox->setProperty("isSwitchButton", true);
    connect(disableCheckBox, &QCheckBox::clicked, this, [this](bool checked)
            { centralWidget()->setEnabled(!checked); });
    toolBar->addWidget(disableCheckBox);
    toolBar->addSeparator();

    // Theme selector
    setupThemeSelector(toolBar);
    toolBar->addSeparator();

    // Color scheme selector
    setupColorSchemeSelector(toolBar);
    toolBar->addSeparator();

    // Style selector
    setupStyleSelector(toolBar);
    toolBar->addSeparator();

    // Widget background mode selector
    setupWidgetBackgroundSelector(toolBar);
}

void MainWindow::setupThemeSelector(QToolBar *toolBar)
{
    QLabel *themeLabel = new QLabel("主题：", this);
    toolBar->addWidget(themeLabel);

    QComboBox *themeComboBox = new QComboBox(this);
    themeComboBox->blockSignals(true);
    themeComboBox->addItem("浅色");
    themeComboBox->addItem("暗色");
    themeComboBox->blockSignals(false);
    themeComboBox->setView(new QListView());

#ifdef FLUENT_USE_QT_STYLE
#if QT_VERSION > QT_VERSION_CHECK(6, 8, 0)
    themeComboBox->setCurrentIndex(static_cast<int>(qApp->styleHints()->colorScheme()) - 1);
#else
    themeComboBox->setCurrentIndex(qApp->property("_q_colorscheme").toInt() == 1 ? 1 : 0);
#endif
#else
    themeComboBox->setCurrentIndex(fluentUIAppearance.theme() == Theme::Dark ? 1 : 0);
#endif

    connect(themeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index)
            {
#ifdef FLUENT_USE_QT_STYLE
#if QT_VERSION > QT_VERSION_CHECK(6, 8, 0)
                qApp->styleHints()->setColorScheme(Qt::ColorScheme(index + 1));
#else
            qApp->setProperty("_q_colorscheme", index);
#endif
                qApp->setStyle("FluentUI3");
#else
            fluentUIAppearance.setTheme(index == 0 ? Theme::Light : Theme::Dark);
#endif
                updateActionIcons();
                resetPalette();
            });

    toolBar->addWidget(themeComboBox);
}

void MainWindow::setupColorSchemeSelector(QToolBar *toolBar)
{
    QLabel *colorSchemeLabel = new QLabel("配色：", this);
    toolBar->addWidget(colorSchemeLabel);

    QComboBox *colorSchemeComboBox = new QComboBox(this);
    colorSchemeComboBox->blockSignals(true);
    colorSchemeComboBox->addItem("Fluent");
    colorSchemeComboBox->addItem("Teams");
    colorSchemeComboBox->blockSignals(false);
    colorSchemeComboBox->setView(new QListView());

    connect(colorSchemeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index)
            {
#ifdef FLUENT_USE_QT_STYLE
                qApp->setProperty("_q_themestyle", index);
                qApp->setStyle("FluentUI3");
#else
            PaletteManager::instance().setThemeStyle(index == 0 ? ThemeStyle::Fluent : ThemeStyle::Teams);
            fluentUIAppearance.setTheme(fluentUIAppearance.theme());
#endif
                updateActionIcons();
                resetPalette();
            });

    toolBar->addWidget(colorSchemeComboBox);
}

void MainWindow::setupStyleSelector(QToolBar *toolBar)
{
    QLabel *styleLabel = new QLabel("样式：", this);
    toolBar->addWidget(styleLabel);

    QComboBox *styleComboBox = new QComboBox(this);
    styleComboBox->addItems(QStyleFactory::keys());
    styleComboBox->setView(new QListView());

    connect(styleComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [styleComboBox, this](int index)
            {
                Q_UNUSED(index);
                const QString styleName = styleComboBox->currentText();
                qApp->setStyle(styleName);
                updateActionIcons();
            });

    toolBar->addWidget(styleComboBox);
}

void MainWindow::setupWidgetBackgroundSelector(QToolBar *toolBar)
{
    QLabel *widgetBgLabel = new QLabel("窗口背景：", this);
    toolBar->addWidget(widgetBgLabel);

    m_tabBarWidgetBg = new QTabBar();
    m_tabBarWidgetBg->setProperty("tabBarStyle", 6);
    m_tabBarWidgetBg->addTab("None");
    m_tabBarWidgetBg->addTab("图片");

    toolBar->addWidget(m_tabBarWidgetBg);

    connect(m_tabBarWidgetBg,
            &QTabBar::currentChanged,
            this,
            [this](int index)
            {
                m_widgetBgMode = static_cast<WidgetBgMode>(index);
                resetPalette();
                update();
            });
}

//=============================================================================
// Navigation View
//=============================================================================

void MainWindow::initializeNavigationView()
{
    m_navView = ui->navView;
    m_navView->setProperty("ItemHeight", 38);
    QTreeWidgetItem *basicItem = m_navView->addNavigationItem("基础控件", 0, "\uE80F");
    m_navView->addNavigationItem("表格控件", 1, "\uE99A");
    m_navView->addNavigationItem("列表控件", 2, "\uE71D");
    m_navView->addNavigationItem("树形控件", 3, "\uED28");
    m_navView->addNavigationItem("导航控件", 4, "\uE8B0");
    m_navView->addNavigationItem("Mdi", 5, "\uE9D9");
    m_navView->addNavigationItem("设置", 6, "\uE9F5");
    addTestNavigationTree();
    connect(m_navView,
            &ExNavTreeWidget::pageIndexChanged,
            this,
            [this](int pageIndex)
            { ui->stackedWidget->setCurrentIndex(pageIndex); });

    m_navView->setCurrentItem(basicItem);
    m_navView->setNavigationExpanded(false, false);
}

void MainWindow::addTestNavigationTree()
{
    QTreeWidgetItem *testItem = new QTreeWidgetItem(ui->navView);
    ui->navView->configureNavigationItem(testItem, "测试节点", 6, "\uE9F5");

    for (int i = 0; i < 5; ++i)
    {
        QTreeWidgetItem *childItem = new QTreeWidgetItem(testItem);
        ui->navView->configureNavigationItem(childItem, QString("子节点%1").arg(i + 1), 6);

        for (int j = 0; j < 3; ++j)
        {
            QTreeWidgetItem *subChildItem = new QTreeWidgetItem(childItem);
            ui->navView->configureNavigationItem(subChildItem, QString("子节点%1-%2").arg(i + 1).arg(j + 1), 6);
        }
    }
}

//=============================================================================
// Table View
//=============================================================================

void MainWindow::initializeTableView()
{
    QTableWidget *table = ui->tableWidget;
    table->clear();
    table->setRowCount(5);
    table->setColumnCount(4);

    QStringList headers;
    headers << "Name"
            << "Age"
            << "Type"
            << "Score";
    table->setHorizontalHeaderLabels(headers);

    table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);

    // Populate name column
    for (int row = 0; row < 5; ++row)
    {
        QTableWidgetItem *item = new QTableWidgetItem(QString("User_%1").arg(row));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        table->setItem(row, 0, item);
    }

    // Populate age column with spin boxes
    for (int row = 0; row < 5; ++row)
    {
        QSpinBox *spinBox = new QSpinBox(table);
        spinBox->setRange(0, 120);
        spinBox->setValue(20 + row);
        spinBox->setAlignment(Qt::AlignCenter);
        table->setCellWidget(row, 1, spinBox);
    }

    // Populate type column with combo boxes
    QStringList typeList = {"Admin", "User", "Guest"};
    for (int row = 0; row < 5; ++row)
    {
        QComboBox *comboBox = new QComboBox(table);
        comboBox->addItems(typeList);
        comboBox->setCurrentIndex(row % typeList.size());
        table->setCellWidget(row, 2, comboBox);
    }

    // Populate score column
    for (int row = 0; row < 5; ++row)
    {
        QTableWidgetItem *item = new QTableWidgetItem(QString::number(60.5 + row));
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        table->setItem(row, 3, item);
    }
}

//=============================================================================
// Tab Setup
//=============================================================================

void MainWindow::setupTabs()
{
    QVBoxLayout *pageLayout = new QVBoxLayout(ui->page_4);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea(ui->page_4);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::StyledPanel);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->viewport()->setAutoFillBackground(false);
    scrollArea->viewport()->setAttribute(Qt::WA_StyledBackground, false);

    QWidget *contentWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(11, 11, 11, 11);
    mainLayout->setSpacing(15);

    scrollArea->setWidget(contentWidget);
    contentWidget->setAutoFillBackground(false);
    pageLayout->addWidget(scrollArea);

    QLabel* lab = new QLabel("TabBar多种样式示例");
    lab->setStyleSheet("font-size:18pt; font-weight:bold;");
    mainLayout->addWidget(lab);

    // Setup various tab styles
    setupPivotTabs(mainLayout);
    setupSegmentedTabs(mainLayout);
    setupPillTabs(mainLayout);
    setupCapsuleTabs(mainLayout);
    setupNavigationTabs(mainLayout);

    mainLayout->addStretch();
}

void MainWindow::setupPivotTabs(QVBoxLayout *mainLayout)
{
    QWidget *pivotWidget = createTabWidgetContainer();
    QVBoxLayout *pivotLayout = static_cast<QVBoxLayout *>(pivotWidget->layout());

    // Pivot Grow
    addTabBarSection(pivotLayout, "Pivot Grow TabBar", "特点：选中时会有一个生长动画效果。", 2);

    // Pivot Slide
    addTabBarSection(pivotLayout, "Pivot Slide TabBar", "特点：选中时会有一个滑动动画效果。", 3);

    // Pivot Stretch
    addTabBarSection(pivotLayout, "Pivot Stretch TabBar", "特点：选中时会有一个拉伸动画效果。", 4);

    pivotLayout->addStretch();
    mainLayout->addWidget(pivotWidget, 1);
}

void MainWindow::setupSegmentedTabs(QVBoxLayout *mainLayout)
{
    QWidget *segmentedWidget = createTabWidgetContainer();
    QVBoxLayout *segmentedLayout = static_cast<QVBoxLayout *>(segmentedWidget->layout());

    // Segmented Slide
    addTabBarSection(segmentedLayout, "Segmented Slide TabBar", "特点：Segmented风格，选中时会有一个滑动动画效果。", 6, &m_segmentedBar);

    // Segmented Fade
    addTabBarSection(segmentedLayout, "Segmented Fade TabBar", "特点：选中时会有一个淡入淡出动画效果。", 7, &m_segmentedFadeBar);

    segmentedLayout->addStretch();
    mainLayout->addWidget(segmentedWidget, 1);
}

void MainWindow::setupPillTabs(QVBoxLayout *mainLayout)
{
    QWidget *pillWidget = createTabWidgetContainer();
    QVBoxLayout *pillLayout = static_cast<QVBoxLayout *>(pillWidget->layout());

    QLabel *pillLabel = new QLabel("Pill TabBar");
    pillLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    pillLayout->addWidget(pillLabel);

    QTabBar *pillBar = new QTabBar();
    pillBar->setTabsClosable(true);
    pillBar->setExpanding(false);
    pillBar->setProperty("tabBarStyle", 5);
    pillBar->addTab("Home");
    pillBar->addTab("Search");
    pillBar->addTab("Settings");
    pillBar->addTab("Help");
    pillBar->addTab("About");
    pillLayout->addWidget(pillBar);

    pillLayout->addStretch();
    mainLayout->addWidget(pillWidget, 1);
}

void MainWindow::setupCapsuleTabs(QVBoxLayout *mainLayout)
{
    QWidget *capsuleWidget = createTabWidgetContainer();
    QVBoxLayout *capsuleLayout = static_cast<QVBoxLayout *>(capsuleWidget->layout());

    QLabel *capsuleLabel = new QLabel("Capsule TabBar");
    capsuleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    QLabel *capsuleDescLabel = new QLabel("特点：浏览器标签样式。");
    capsuleDescLabel->setStyleSheet("color: gray; font-size: 12px;");
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
    capTabBar->setProperty("tabBarStyle", 1);
    capTabBar->setDrawBase(false);

    const QStringList pageNames = {"Home", "Search", "Settings", "Help", "About"};
    const QStringList fullNames = {"Home Page", "Search Page", "Settings Page", "Help Page", "About Page"};
    const QList<QColor> pageColors = {
        QColor(255, 228, 225), QColor(224, 255, 255), QColor(240, 255, 240), QColor(255, 250, 205), QColor(230, 230, 250)};

    for (int i = 0; i < pageNames.size(); ++i)
    {
        QLabel *page = new QLabel(fullNames[i]);
        page->setAlignment(Qt::AlignCenter);
        page->setStyleSheet(QString("background-color: %1;color:black;").arg(pageColors[i].name()));
        m_capsuleTabWidget->addTab(page, pageNames[i]);
    }

    capsuleLayout->addWidget(m_capsuleTabWidget, 1);
    mainLayout->addWidget(capsuleWidget, 1);
}

void MainWindow::setupNavigationTabs(QVBoxLayout *mainLayout)
{
    QWidget *navigationWidget = createTabWidgetContainer();
    QVBoxLayout *navigationLayout = static_cast<QVBoxLayout *>(navigationWidget->layout());

    QLabel *navigationLabel = new QLabel("Navigation TabBar");
    navigationLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    QLabel *navigationDescLabel = new QLabel("特点：适合用于侧边栏的导航菜单，选项卡垂直排列，选中时指示器有个变长效果");
    navigationDescLabel->setStyleSheet("color: gray; font-size: 12px;");
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
    navTabBar->setProperty("tabBarStyle", 8);

    const QStringList navFullNames = {"Overview Page", "Files Page", "History Page", "Insights Page", "Settings Page"};
    const QStringList navNames = {"Overview", "Files", "History", "Insights", "Settings"};
    const QList<QColor> navPageColors = {
        QColor(244, 248, 255), QColor(240, 251, 246), QColor(255, 248, 238), QColor(248, 243, 255), QColor(245, 245, 245)};

    for (int i = 0; i < navFullNames.size(); ++i)
    {
        QLabel *page = new QLabel(navFullNames[i]);
        page->setAlignment(Qt::AlignCenter);
        page->setMinimumHeight(220);
        page->setStyleSheet(QString("background-color:%1;color:black;").arg(navPageColors[i].name()));
        m_navigationTabWidget->addTab(page, navNames[i]);
    }

    bodyLayout->addWidget(m_navigationTabWidget, 1);
    navigationLayout->addLayout(bodyLayout);
    mainLayout->addWidget(navigationWidget, 1);
}

QWidget *MainWindow::createTabWidgetContainer()
{
    QWidget *widget = new QWidget();
    widget->setProperty("isCard", true);
    widget->setAttribute(Qt::WA_StyledBackground);

    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    return widget;
}

void MainWindow::addTabBarSection(QVBoxLayout *layout,
                                  const QString &title,
                                  const QString &description,
                                  int tabStyle,
                                  QTabBar **outTabBar)
{
    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(titleLabel);

    if (!description.isEmpty())
    {
        QLabel *descLabel = new QLabel(description);
        descLabel->setStyleSheet("color: gray; font-size: 12px;");
        layout->addWidget(descLabel);
    }

    QTabBar *tabBar = new QTabBar();
    tabBar->setExpanding(false);
    tabBar->setProperty("tabBarStyle", tabStyle);
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

//=============================================================================
// Buttons & Icons
//=============================================================================

void MainWindow::setupButtonsAndIcons()
{
    // Setup tool button
    ui->toolButton->setIcon(createFluentIcon("\ue8c3"));
    ui->pushButton_10->setText("工具按钮");
    ui->pushButton_10->setIcon(createFluentIcon("\ue713"));

    // Setup tool button 3 with menu
    setupToolButtonWithMenu();

    // Setup tool button 4 with icon and text
    ui->toolButton_4->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButton_4->setText("上下按钮");
    ui->toolButton_4->setIcon(createFluentIcon("\uE804"));

    // Setup auto-raise button
    ui->tBtnAutoRaise->setIcon(createFluentIcon("\ue804"));
}

void MainWindow::setupToolButtonWithMenu()
{
    ui->toolButton_3->setAutoRaise(false);
    ui->toolButton_3->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui->toolButton_3->setPopupMode(QToolButton::InstantPopup);
    ui->toolButton_3->setText("菜单按钮");

    QMenu *menu = new QMenu(ui->toolButton_3);
    g_actionIconMap[menu->addAction(createFluentIcon("\ue8a5"), "新建文件")] = "\ue8a5";
    g_actionIconMap[menu->addAction(createFluentIcon("\ue8b5"), "新建项目")] = "\ue8b5";
    g_actionIconMap[menu->addAction(createFluentIcon("\ue8c3"), "最近打开")] = "\ue8c3";
    g_actionIconMap[menu->addAction(createFluentIcon("\ue8a5"), "打开文件")] = "\ue8a5";
    ui->toolButton_3->setMenu(menu);
    ui->toolButton_4->setMenu(menu);
}

//=============================================================================
// MDI Area
//=============================================================================

void MainWindow::setupMdiArea()
{
    QMdiArea *mdiArea = new QMdiArea(ui->page_5);
    mdiArea->setViewMode(QMdiArea::SubWindowView);
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Add sub-windows
    for (int i = 0; i < 3; ++i)
    {
        QMdiSubWindow *subWindow = new QMdiSubWindow();
        subWindow->setWidget(new QTextEdit());
        subWindow->setAttribute(Qt::WA_DeleteOnClose);
        subWindow->setWindowTitle(QString("子窗口 %1").arg(i + 1));
        mdiArea->addSubWindow(subWindow);
        subWindow->show();
    }

    // Add view mode switch button
    QPushButton *switchViewBtn = new QPushButton("切换视图模式", ui->page_5);
    switchViewBtn->move(10, 10);
    switchViewBtn->raise();
    connect(switchViewBtn,
            &QPushButton::clicked,
            this,
            [mdiArea]()
            { mdiArea->setViewMode(mdiArea->viewMode() == QMdiArea::SubWindowView ? QMdiArea::TabbedView : QMdiArea::SubWindowView); });

    QVBoxLayout *layout = new QVBoxLayout(ui->page_5);
    layout->addWidget(switchViewBtn);
    layout->addWidget(mdiArea);
}

//=============================================================================
// Utility Functions
//=============================================================================

void MainWindow::updateActionIcons()
{
    // Update search icon
    ui->lineEditSerach->removeAction(m_searchAction);
    m_searchAction = ui->lineEditSerach->addAction(createFluentIcon("\ue721"), QLineEdit::TrailingPosition);

    // Update capsule tab icons
    updateCapsuleTabIcons();

    // Update segmented bar icons
    updateSegmentedBarIcons();

    // Update navigation tab icons
    updateNavigationTabIcons();

    // Update button icons
    updateButtonIcons();

    // Update action icons
    updateMenuActionIcons();

    // Update menu icons
    updateMenuIcons();

    // Update navigation item icons
    updateNavigationItemIcons();
}

void MainWindow::updateCapsuleTabIcons()
{
    if (!m_capsuleTabWidget)
    {
        return;
    }

    const QStringList iconCodes = {"\uEA86", "\uE7F3", "\ue8c3", "\uE836", "\uE9F5"};
    for (int i = 0; i < iconCodes.size() && i < m_capsuleTabWidget->count(); ++i)
    {
        m_capsuleTabWidget->setTabIcon(i, createFluentIcon(iconCodes[i]));
    }
}

void MainWindow::updateSegmentedBarIcons()
{
    const QStringList iconCodes = {"\uEC64", "\uEF58", "\uE99A", "\uE7ED", "\uF163"};

    if (m_segmentedBar)
    {
        for (int i = 0; i < iconCodes.size() && i < m_segmentedBar->count(); ++i)
        {
            m_segmentedBar->setTabIcon(i, createFluentIcon(iconCodes[i]));
        }
    }

    if (m_segmentedFadeBar)
    {
        for (int i = 0; i < iconCodes.size() && i < m_segmentedFadeBar->count(); ++i)
        {
            m_segmentedFadeBar->setTabIcon(i, createFluentIcon(iconCodes[i]));
        }
    }
}

void MainWindow::updateNavigationTabIcons()
{
    if (!m_navigationTabWidget)
    {
        return;
    }

    const QStringList iconCodes = {"\uEC64", "\uE8B7", "\uE81C", "\uE9CE", "\uE713"};
    for (int i = 0; i < iconCodes.size() && i < m_navigationTabWidget->count(); ++i)
    {
        m_navigationTabWidget->setTabIcon(i, createFluentIcon(iconCodes[i]));
    }
}

void MainWindow::updateButtonIcons()
{
    ui->toolButton->setIcon(createFluentIcon("\ue8c3"));
    ui->pushButton_10->setIcon(createFluentIcon("\ue713"));
    ui->toolButton_4->setIcon(createFluentIcon("\uE804"));
    ui->tBtnAutoRaise->setIcon(createFluentIcon("\ue804"));
}

void MainWindow::updateMenuActionIcons()
{
    for (auto it = g_actionIconMap.begin(); it != g_actionIconMap.end(); ++it)
    {
        QAction *action = it.key();
        const QString iconCode = it.value();
        if (action)
        {
            action->setIcon(createFluentIcon(iconCode));
        }
    }
}

void MainWindow::updateMenuIcons()
{
    for (auto it = g_menuIconMap.begin(); it != g_menuIconMap.end(); ++it)
    {
        QMenu *menu = it.key();
        const QString iconCode = it.value();
        if (menu)
        {
            menu->setIcon(createFluentIcon(iconCode));
        }
    }
}

void MainWindow::updateNavigationItemIcons()
{
    std::function<void(QTreeWidgetItem *)> updateItemIcon = [&](QTreeWidgetItem *item)
    {
        if (!item)
        {
            return;
        }
        const QString iconCode = item->data(0, Qt::UserRole + 1).toString();
        if (!iconCode.isEmpty())
        {
            item->setIcon(0, createFluentIcon(iconCode));
        }
        for (int i = 0; i < item->childCount(); ++i)
        {
            updateItemIcon(item->child(i));
        }
    };

    for (int i = 0; i < ui->navView->topLevelItemCount(); ++i)
    {
        updateItemIcon(ui->navView->topLevelItem(i));
    }
}

void MainWindow::loadChangelog()
{
    QFile file(":/changelog.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        in.setCodec("UTF-8");
#endif
        const QString content = in.readAll();
        file.close();
        const QStringList lines = content.split("\n");
        for (const QString &line : std::as_const(lines))
        {
            ui->log->append(line);
        }
    }
    else
    {
        ui->log->append("无法打开changelog.txt, " + file.errorString());
    }
}

void MainWindow::resetPalette()
{
    QPalette originPalette = qApp->palette();
    auto setAlpha = [&](QPalette::ColorGroup group, QPalette::ColorRole colorRole, int a)
    {
        QColor cr1 = originPalette.color(group, colorRole);
        if (cr1.alpha() == a)
        {
            return;
        }

        cr1.setAlpha(a);
        originPalette.setColor(group, colorRole, cr1);
    };

    //Example项目
    //WidgetBgMode::Pixmap需要Base和Window有透明度，不然看不见背景
    if (m_widgetBgMode == WidgetBgMode::Pixmap)
    {
        setAlpha(QPalette::Active, QPalette::Base, 0);
        setAlpha(QPalette::Active, QPalette::Window, 160);

        setAlpha(QPalette::Disabled, QPalette::Base, 0);
        setAlpha(QPalette::Disabled, QPalette::Window, 160);

        setAlpha(QPalette::Inactive, QPalette::Base, 0);
        setAlpha(QPalette::Inactive, QPalette::Window, 160);
    }
    else
    {
        setAlpha(QPalette::Active, QPalette::Base, 255);
        setAlpha(QPalette::Active, QPalette::Window, 255);

        setAlpha(QPalette::Disabled, QPalette::Base, 255);
        setAlpha(QPalette::Disabled, QPalette::Window, 255);

        setAlpha(QPalette::Inactive, QPalette::Base, 255);
        setAlpha(QPalette::Inactive, QPalette::Window, 255);
    }
    qApp->setPalette(originPalette);
}

//=============================================================================
// Slot Functions
//=============================================================================

enum class SpinBoxButtonStyle
{
    ArrowsVertical = 0,
    ArrowsHorizontalSides = 1,
    ArrowsHorizontalRight = 2,
    PlusMinusHorizontalSides = 3
};

void MainWindow::on_checkBox_4_clicked(bool checked)
{
    QList<QCheckBox *> checkBoxList;
    checkBoxList << ui->checkBox_5;
    for (QCheckBox *checkBox : std::as_const(checkBoxList))
    {
        checkBox->setChecked(checked);
    }
}

void MainWindow::on_checkBox_5_stateChanged(int state)
{
    ui->checkBox_5->setText(state == Qt::Checked ? "On" : "Off");
}

void MainWindow::on_radioButton_7_clicked()
{
    ui->spinBox->setProperty("spinBoxButtonLayout", static_cast<int>(SpinBoxButtonStyle::ArrowsVertical));
    ui->spinBox->setFrame(ui->spinBox->hasFrame());
}

void MainWindow::on_radioButton_4_clicked()
{
    ui->spinBox->setProperty("spinBoxButtonLayout", static_cast<int>(SpinBoxButtonStyle::ArrowsHorizontalSides));
    ui->spinBox->setFrame(ui->spinBox->hasFrame());
}

void MainWindow::on_radioButton_5_clicked()
{
    ui->spinBox->setProperty("spinBoxButtonLayout", static_cast<int>(SpinBoxButtonStyle::ArrowsHorizontalRight));
    ui->spinBox->setFrame(ui->spinBox->hasFrame());
}

void MainWindow::on_radioButton_6_clicked()
{
    ui->spinBox->setProperty("spinBoxButtonLayout", static_cast<int>(SpinBoxButtonStyle::PlusMinusHorizontalSides));
    ui->spinBox->setFrame(ui->spinBox->hasFrame());
}

//=============================================================================
// Standard Menu Icons
//=============================================================================

void applyStandardMenuIcons(QMenu *menu, QWidget *widget)
{
    if (!menu)
    {
        return;
    }

    const QColor iconColor = widget ? widget->palette().color(QPalette::Text) : QApplication::palette().color(QPalette::Text);
    Q_UNUSED(iconColor);

    // Segoe Fluent glyph definitions
    constexpr QChar GLYPH_UNDO(0xE7A7);
    constexpr QChar GLYPH_REDO(0xE7A6);
    constexpr QChar GLYPH_CUT(0xE8C6);
    constexpr QChar GLYPH_COPY(0xE8C8);
    constexpr QChar GLYPH_PASTE(0xE77F);
    constexpr QChar GLYPH_SELECT_ALL(0xE8B3);
    constexpr QChar GLYPH_DELETE(0xE74D);
    constexpr QChar GLYPH_UP(0xE70E);
    constexpr QChar GLYPH_DOWN(0xE70D);

    const QList<QAction *> actions = menu->actions();
    for (QAction *action : actions)
    {
        if (!action || action->isSeparator())
        {
            continue;
        }

        if (QMenu *subMenu = action->menu())
        {
            applyStandardMenuIcons(subMenu, widget);
            continue;
        }

        if (!action->icon().isNull())
        {
            continue;
        }

        const QString text = action->text().remove('&').toLower();

        if (text.contains("undo"))
        {
            action->setIcon(createFluentIcon(GLYPH_UNDO));
        }
        else if (text.contains("redo"))
        {
            action->setIcon(createFluentIcon(GLYPH_REDO));
        }
        else if (text.contains("cut"))
        {
            action->setIcon(createFluentIcon(GLYPH_CUT));
        }
        else if (text.contains("copy"))
        {
            action->setIcon(createFluentIcon(GLYPH_COPY));
        }
        else if (text.contains("paste"))
        {
            action->setIcon(createFluentIcon(GLYPH_PASTE));
        }
        else if (text.contains("select all") || text.contains("selectall"))
        {
            action->setIcon(createFluentIcon(GLYPH_SELECT_ALL));
        }
        else if (text.contains("delete") || text.contains("clear"))
        {
            action->setIcon(createFluentIcon(GLYPH_DELETE));
        }
        else if (text.contains("step up"))
        {
            action->setIcon(createFluentIcon(GLYPH_UP));
        }
        else if (text.contains("step down"))
        {
            action->setIcon(createFluentIcon(GLYPH_DOWN));
        }
    }
}

//=============================================================================
// Context Menu Hooks (Non-MinGW)
//=============================================================================

#ifndef __MINGW32__
void QLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    if (QMenu *menu = createStandardContextMenu())
    {
        applyStandardMenuIcons(menu, this);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->exec(event->globalPos());
    }
}

void QTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    if (QMenu *menu = createStandardContextMenu())
    {
        applyStandardMenuIcons(menu, this);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->exec(event->globalPos());
    }
}

void QComboBox::contextMenuEvent(QContextMenuEvent *event)
{
    if (lineEdit())
    {
        if (QMenu *menu = lineEdit()->createStandardContextMenu())
        {
            applyStandardMenuIcons(menu, lineEdit());
            menu->setAttribute(Qt::WA_DeleteOnClose);
            menu->exec(event->globalPos());
        }
    }
}

void QAbstractSpinBox::contextMenuEvent(QContextMenuEvent *event)
{
    Q_D(QAbstractSpinBox);
    QPointer<QMenu> menu = d->edit->createStandardContextMenu();
    if (!menu)
    {
        return;
    }

    QAction *selectAllAction = new QAction(tr("&Select All"), menu);
#if QT_CONFIG(shortcut)
    selectAllAction->setShortcut(QKeySequence::SelectAll);
#endif
    menu->insertAction(d->edit->d_func()->selectAllAction, selectAllAction);
    menu->removeAction(d->edit->d_func()->selectAllAction);
    menu->addSeparator();

    const uint se = stepEnabled();
    QAction *stepUpAction = menu->addAction(tr("&Step up"));
    stepUpAction->setEnabled(se & StepUpEnabled);
    QAction *stepDownAction = menu->addAction(tr("Step &down"));
    stepDownAction->setEnabled(se & StepDownEnabled);
    menu->addSeparator();

    applyStandardMenuIcons(menu, this);

    const QPointer<QAbstractSpinBox> spinBox(this);
    const QPoint pos = (event->reason() == QContextMenuEvent::Mouse)
                           ? event->globalPos()
                           : mapToGlobal(QPoint(event->pos().x(), 0)) + QPoint(width() / 2, height() / 2);
    const QAction *action = menu->exec(pos);
    delete static_cast<QMenu *>(menu);
    if (spinBox && action)
    {
        if (action == stepUpAction)
        {
            stepBy(1);
        }
        else if (action == stepDownAction)
        {
            stepBy(-1);
        }
        else if (action == selectAllAction)
        {
            selectAll();
        }
    }
    event->accept();
}
#endif

//=============================================================================
// Leave Event Helper
//=============================================================================

static inline void emulateLeaveEvent(QWidget *widget)
{
    Q_ASSERT(widget);
    if (!widget)
    {
        return;
    }

    QTimer::singleShot(0,
                       widget,
                       [widget]()
                       {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                           const QScreen *screen = widget->screen();
#else
            const QScreen *screen = widget->windowHandle()->screen();
#endif
                           const QPoint globalPos = QCursor::pos(screen);
                           if (!QRect(widget->mapToGlobal(QPoint{0, 0}), widget->size()).contains(globalPos))
                           {
                               QCoreApplication::postEvent(widget, new QEvent(QEvent::Leave));
                               if (widget->testAttribute(Qt::WA_Hover))
                               {
                                   const QPoint localPos = widget->mapFromGlobal(globalPos);
                                   const QPoint scenePos = widget->window()->mapFromGlobal(globalPos);
                                   static constexpr const QPoint oldPos;
                                   const Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
                                   const auto hoverEvent = new QHoverEvent(QEvent::HoverLeave, scenePos, globalPos, oldPos, modifiers);
                                   Q_UNUSED(localPos);
#elif (QT_VERSION >= QT_VERSION_CHECK(6, 3, 0))
                    const auto hoverEvent = new QHoverEvent(QEvent::HoverLeave, localPos, globalPos, oldPos, modifiers);
                    Q_UNUSED(scenePos);
#else
                    const auto hoverEvent = new QHoverEvent(QEvent::HoverLeave, localPos, oldPos, modifiers);
                    Q_UNUSED(scenePos);
#endif
                                   QCoreApplication::postEvent(widget, hoverEvent);
                               }
                           }
                       });
}

//=============================================================================
// End of File
//=============================================================================
