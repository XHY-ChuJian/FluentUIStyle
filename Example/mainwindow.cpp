//=============================================================================
// MainWindow Implementation
// FluentUI3 Style Demo Application
//=============================================================================

#include "mainwindow.h"

#include "applanguage.h"
#include "ui_mainwindow.h"

#include <algorithm>

// Qt Core Headers
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QEasingCurve>
#include <QEvent>
#include <QFile>
#include <QDate>
#include <QKeySequence>
#include <QPointer>
#include <QPainter>
#include <QSettings>
#include <QScreen>
#include <QSvgRenderer>
#include <QSet>
#include <QTextStream>
#include <QTimer>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QDir>

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
#include <QAbstractButton>
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
#include <QButtonGroup>

// Project Headers
#include <exrangeslider.h>
#include <exstackedwidget.h>
#include <exnavtreewidget.h>
#include <exmessagebox.h>
#include <exwinuinavigationview.h>
#include "font-icon/fonticon.h"
#include "segoeicongallerywidget.h"
#include "aboutprojectwidget.h"
#include "tabshowcasewidget.h"
#include "dialogshowcasewidget.h"
#include "colorshowcasewidget.h"
#include "../FluentUI3Style/fluentui3styleproperties.h"

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

struct InstalledSoftwareInfo
{
    QString name;
    QString version;
    QString publisher;
    QString installDate;
    QString source;
    QString displayIcon;
    QString uninstallString;
    QString quietUninstallString;
    QString installLocation;
};

static QString formatInstallDate(const QString &rawDate)
{
    if (rawDate.size() == 8)
    {
        const QDate date = QDate::fromString(rawDate, QStringLiteral("yyyyMMdd"));
        if (date.isValid())
        {
            return date.toString(QStringLiteral("yyyy-MM-dd"));
        }
    }
    return rawDate;
}

static QString normalizeDisplayIconPath(QString value)
{
    value = value.trimmed();
    if (value.isEmpty())
    {
        return {};
    }

    // Common formats:
    //  - "C:\Path\App.exe",0
    //  - C:\Path\App.exe
    //  - C:\Path\App.ico
    //  - "C:\Path\App.exe"
    if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
    {
        value = value.mid(1, value.size() - 2);
    }

    const int comma = value.lastIndexOf(QLatin1Char(','));
    if (comma > 0)
    {
        const QString maybeIndex = value.mid(comma + 1).trimmed();
        bool ok = false;
        maybeIndex.toInt(&ok);
        if (ok)
        {
            value = value.left(comma).trimmed();
            if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
            {
                value = value.mid(1, value.size() - 2);
            }
        }
    }

    return value;
}

static QIcon iconFromDisplayIcon(const QString &displayIcon)
{
    const QString path = normalizeDisplayIconPath(displayIcon);
    if (path.isEmpty())
    {
        return {};
    }

    // Some entries are command lines; only accept existing files.
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile())
    {
        return {};
    }

    QFileIconProvider provider;
    return provider.icon(fi);
}

static QString executablePathFromCommandLine(QString value)
{
    value = value.trimmed();
    if (value.isEmpty())
    {
        return {};
    }

    // Strip leading/trailing quotes first.
    if (value.startsWith(QLatin1Char('"')))
    {
        const int endQuote = value.indexOf(QLatin1Char('"'), 1);
        if (endQuote > 1)
        {
            value = value.mid(1, endQuote - 1);
        }
    }
    else
    {
        const int firstSpace = value.indexOf(QLatin1Char(' '));
        if (firstSpace > 0)
        {
            value = value.left(firstSpace);
        }
    }

    value = value.trimmed();
    if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')) && value.size() >= 2)
    {
        value = value.mid(1, value.size() - 2);
    }

    const QFileInfo fi(value);
    if (!fi.exists() || !fi.isFile())
    {
        return {};
    }
    // Not strictly limited to .exe: some entries point to .ico, .dll, etc.
    return fi.absoluteFilePath();
}

static QIcon iconFromSoftwareInfo(const InstalledSoftwareInfo &info)
{
    if (const QIcon icon = iconFromDisplayIcon(info.displayIcon); !icon.isNull())
    {
        return icon;
    }

    const QString uninstallExe = executablePathFromCommandLine(info.uninstallString);
    if (!uninstallExe.isEmpty())
    {
        QFileIconProvider provider;
        return provider.icon(QFileInfo(uninstallExe));
    }

    const QString quietExe = executablePathFromCommandLine(info.quietUninstallString);
    if (!quietExe.isEmpty())
    {
        QFileIconProvider provider;
        return provider.icon(QFileInfo(quietExe));
    }

    const QString loc = info.installLocation.trimmed();
    if (!loc.isEmpty())
    {
        const QDir dir(loc);
        if (dir.exists())
        {
            const QFileInfoList exeList = dir.entryInfoList(QStringList() << QStringLiteral("*.exe"),
                                                            QDir::Files | QDir::NoDotAndDotDot);
            if (!exeList.isEmpty())
            {
                QFileIconProvider provider;
                return provider.icon(exeList.first());
            }
        }
    }

    return {};
}

static void appendInstalledSoftwareFromRegistry(const QString &rootPath,
                                                const QString &sourceLabel,
                                                QList<InstalledSoftwareInfo> &items,
                                                QSet<QString> &dedupeKeys)
{
    QSettings reg(rootPath, QSettings::NativeFormat);
    const QStringList subKeys = reg.childGroups();
    for (const QString &subKey : subKeys)
    {
        reg.beginGroup(subKey);

        const QString displayName = reg.value(QStringLiteral("DisplayName")).toString().trimmed();
        const bool systemComponent = reg.value(QStringLiteral("SystemComponent"), QVariant(0)).toInt() == 1;
        const QString releaseType = reg.value(QStringLiteral("ReleaseType")).toString();
        const QString parentKeyName = reg.value(QStringLiteral("ParentKeyName")).toString();

        if (displayName.isEmpty() || systemComponent || !parentKeyName.isEmpty() || releaseType.contains(QStringLiteral("Update"), Qt::CaseInsensitive) || releaseType.contains(QStringLiteral("Hotfix"), Qt::CaseInsensitive))
        {
            reg.endGroup();
            continue;
        }

        const QString version = reg.value(QStringLiteral("DisplayVersion")).toString().trimmed();
        const QString publisher = reg.value(QStringLiteral("Publisher")).toString().trimmed();
        const QString installDate = formatInstallDate(reg.value(QStringLiteral("InstallDate")).toString().trimmed());
        const QString displayIcon = reg.value(QStringLiteral("DisplayIcon")).toString().trimmed();
        const QString uninstallString = reg.value(QStringLiteral("UninstallString")).toString().trimmed();
        const QString quietUninstallString = reg.value(QStringLiteral("QuietUninstallString")).toString().trimmed();
        const QString installLocation = reg.value(QStringLiteral("InstallLocation")).toString().trimmed();

        const QString dedupeKey = displayName + QLatin1Char('|') + version + QLatin1Char('|') + publisher;
        if (dedupeKeys.contains(dedupeKey))
        {
            reg.endGroup();
            continue;
        }
        dedupeKeys.insert(dedupeKey);

        InstalledSoftwareInfo item;
        item.name = displayName;
        item.version = version;
        item.publisher = publisher;
        item.installDate = installDate;
        item.source = sourceLabel;
        item.displayIcon = displayIcon;
        item.uninstallString = uninstallString;
        item.quietUninstallString = quietUninstallString;
        item.installLocation = installLocation;
        items.append(item);

        reg.endGroup();
    }
}

static QList<InstalledSoftwareInfo> queryInstalledSoftwareList()
{
    QList<InstalledSoftwareInfo> items;
    QSet<QString> dedupeKeys;

    appendInstalledSoftwareFromRegistry(
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKLM 64-bit"), items, dedupeKeys);
    appendInstalledSoftwareFromRegistry(
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKLM 32-bit"), items, dedupeKeys);
    appendInstalledSoftwareFromRegistry(
        QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKCU"), items, dedupeKeys);

    std::sort(items.begin(), items.end(), [](const InstalledSoftwareInfo &a, const InstalledSoftwareInfo &b)
              { return a.name.localeAwareCompare(b.name) < 0; });
    return items;
}

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
#ifdef FLUENT_USE_QT_STYLE
    isDarkTheme = qApp->property("_q_colorscheme").toInt() == 1;
#else
    isDarkTheme = fluentUIAppearance.theme() == Theme::Dark;
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
    : QMainWindow(parent), ui(new Ui::MainWindow), m_menuBar(nullptr), m_toolBar(nullptr), m_tabShowcaseWidget(nullptr), m_searchAction(nullptr), m_tabBarWidgetBg(nullptr), m_widgetBgMode(WidgetBgMode::None)
{
    // Setup window attributes
    setAttribute(Qt::WA_StyledBackground);

    // Setup UI
    ui->setupUi(this);

    // Create menu bar
    m_menuBar = new QMenuBar();
    setMenuBar(m_menuBar);

    setWindowTitle(tr("FluentUI Demo - QStyle [Qt-Version %1]").arg(QT_VERSION_STR));

    // Install menu offset filter
    qApp->installEventFilter(new MenuOffsetFilter(qApp));

    // Initialize widgets with fluent border style
    initializeFluentBorderWidgets();

    // Initialize main components
    initializeComponents();

    // Setup combo box
    setupComboBox();

    // Load changelog
    loadChangelog();

    // Update icons
    updateActionIcons();

    syncLanguageRadios();

#ifdef __MINGW32__
    // Hook context menus for MinGW
    for (QWidget *widget : findChildren<QWidget *>())
    {
        hookContextMenu(widget);
    }
#endif
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
                  << ui->widget_8 << ui->widget_9 << ui->widget_10 << ui->widget_12 << ui->widget_13 << ui->widget_14
                  << ui->widgetWidgetMode << ui->widgetNavMode << ui->widgetColorSheme << ui->widgetAccentColor
                  << ui->widgetUiLanguage;

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

    ui->dial_3->setProperty("dialDrawValue", false);

    // Configure scroll areas
    ui->scrollArea_2->viewport()->setAutoFillBackground(false);
    ui->scrollAreaWidgetContents->setAutoFillBackground(false);
    ui->scrollAreaWidgetContents_4->setAutoFillBackground(false);

    // Configure search line edit
    ui->lineEditSerach->setPlaceholderText(tr("搜索..."));
    ui->lineEditSerach->setClearButtonEnabled(true);
    m_searchAction = ui->lineEditSerach->addAction(createFluentIcon("\ue721"), QLineEdit::TrailingPosition);

    setupSegoeIconGalleryPage();
    setupAboutPage();
    setupDialogsPage();
    setupColorPickerPage();

    // Configure stacked widget
    ui->stackedWidget->setVerticalMode(true);
    ui->stackedWidget->setAnimation(QEasingCurve::Type::InOutSine);
    ui->stackedWidget->setSpeed(300);

    // Initialize sub-components
    rebuildMenuAndToolBar();
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
    ui->progressBar->setProperty(ProgressBarStyleProperty, ProgressBarThick);
    ui->rangeSelector->setRange(0, 100);
    ui->rangeSelector->setValues(20, 80);
    ui->rangeSelector->setSingleStep(1);
    ui->rangeSelector->setPageStep(10);
    connect(ui->rangeSelector, &ExRangeSlider::valuesChanged, this, [this](int lower, int upper) {
        qDebug() << "Range slider values changed: " << lower << " " << upper;
    });

    ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsHorizontalRight);
    ui->checkBox_5->setText(tr("Off"));
    ui->treeWidget->setProperty("ItemHeight", 32);
    ui->treeWidget->setIndentation(20);

    // Setup tabs
    setupTabs();

    // Setup buttons and icons
    setupButtonsAndIcons();

    // Setup accent color widget
    setupAccentColorWidget();

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

void MainWindow::buildMainMenus()
{
    QMenuBar *menuBar = m_menuBar;
    // File Menu
    QMenu *fileMenu = menuBar->addMenu(tr("文件"));
    QAction *newFileAction = fileMenu->addAction(createFluentIcon("\ue8a5"), tr("新建文件"));
    newFileAction->setShortcut(QKeySequence("Ctrl+N"));
    g_actionIconMap[newFileAction] = "\ue8a5";

    QAction *newProjectAction = fileMenu->addAction(createFluentIcon("\ue8b5"), tr("新建项目"));
    g_actionIconMap[newProjectAction] = "\ue8b5";

    QMenu *recentMenu = fileMenu->addMenu(createFluentIcon("\ue8c3"), tr("最近打开"));
    g_menuIconMap[recentMenu] = "\ue8c3";
    recentMenu->addAction("project1");
    recentMenu->addAction("project2");
    recentMenu->addAction("example.cpp");

    QAction *openFileAction = fileMenu->addAction(createFluentIcon("\ue8a5"), tr("打开文件"));
    openFileAction->setShortcut(QKeySequence("Ctrl+O"));
    g_actionIconMap[openFileAction] = "\ue8a5";

    QAction *openProjectAction = fileMenu->addAction(createFluentIcon("\ue8b5"), tr("打开项目"));
    g_actionIconMap[openProjectAction] = "\ue8b5";

    fileMenu->addSeparator();

    QAction *saveAction = fileMenu->addAction(createFluentIcon("\ue74e"), tr("保存"));
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    g_actionIconMap[saveAction] = "\ue74e";

    QAction *saveAsAction = fileMenu->addAction(createFluentIcon("\ue74e"), tr("另存为"));
    saveAsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    g_actionIconMap[saveAsAction] = "\ue74e";

    fileMenu->addSeparator();

    QAction *closeFileAction = fileMenu->addAction(createFluentIcon("\ue8bb"), tr("关闭文件"));
    g_actionIconMap[closeFileAction] = "\ue8bb";

    QAction *exitAction = fileMenu->addAction(createFluentIcon("\ue8bb"), tr("退出"));
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    g_actionIconMap[exitAction] = "\ue8bb";

    // Edit Menu
    QMenu *editMenu = menuBar->addMenu(tr("编辑"));
    QAction *undoAction = editMenu->addAction(createFluentIcon("\ue7a7"), tr("撤销"));
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));
    g_actionIconMap[undoAction] = "\ue7a7";

    QAction *redoAction = editMenu->addAction(createFluentIcon("\ue7a6"), tr("重做"));
    redoAction->setShortcut(QKeySequence("Ctrl+Y"));
    g_actionIconMap[redoAction] = "\ue7a6";

    editMenu->addSeparator();

    QAction *cutAction = editMenu->addAction(createFluentIcon("\ue8c6"), tr("剪切"));
    cutAction->setShortcut(QKeySequence("Ctrl+X"));
    g_actionIconMap[cutAction] = "\ue8c6";

    QAction *copyAction = editMenu->addAction(createFluentIcon("\ue8c8"), tr("复制"));
    copyAction->setShortcut(QKeySequence("Ctrl+C"));
    g_actionIconMap[copyAction] = "\ue8c8";

    QAction *pasteAction = editMenu->addAction(createFluentIcon("\ue8c7"), tr("粘贴"));
    pasteAction->setShortcut(QKeySequence("Ctrl+V"));
    g_actionIconMap[pasteAction] = "\ue8c7";

    editMenu->addSeparator();

    QAction *findAction = editMenu->addAction(createFluentIcon("\ue721"), tr("查找"));
    findAction->setShortcut(QKeySequence("Ctrl+F"));
    g_actionIconMap[findAction] = "\ue721";

    QAction *replaceAction = editMenu->addAction(createFluentIcon("\ue8ac"), tr("替换"));
    replaceAction->setShortcut(QKeySequence("Ctrl+H"));
    g_actionIconMap[replaceAction] = "\ue8ac";

    QMenu *advancedMenu = editMenu->addMenu(createFluentIcon("\ue713"), tr("高级"));
    g_menuIconMap[advancedMenu] = "\ue713";
    QAction *autoFormatAction = advancedMenu->addAction(createFluentIcon("\ue930"), tr("自动格式化"));
    autoFormatAction->setCheckable(true);
    g_actionIconMap[autoFormatAction] = "\ue930";

    QAction *sortLinesAction = advancedMenu->addAction(createFluentIcon("\ue930"), tr("排序行"));
    g_actionIconMap[sortLinesAction] = "\ue930";

    QAction *deleteEmptyLinesAction = advancedMenu->addAction(createFluentIcon("\ue8bb"), tr("删除空行"));
    g_actionIconMap[deleteEmptyLinesAction] = "\ue8bb";

    // View Menu
    QMenu *viewMenu = menuBar->addMenu(tr("视图"));
    QAction *showToolbarAction = viewMenu->addAction(createFluentIcon("\ue728"), tr("显示工具栏"));
    showToolbarAction->setCheckable(true);
    showToolbarAction->setChecked(true);
    g_actionIconMap[showToolbarAction] = "\ue728";

    QAction *showStatusBarAction = viewMenu->addAction(createFluentIcon("\ue9d9"), tr("显示状态栏"));
    showStatusBarAction->setCheckable(true);
    showStatusBarAction->setChecked(true);
    g_actionIconMap[showStatusBarAction] = "\ue9d9";

    viewMenu->addSeparator();

    QAction *showSidebarAction = viewMenu->addAction(createFluentIcon("\ue728"), tr("显示侧边栏"));
    showSidebarAction->setCheckable(true);
    showSidebarAction->setChecked(true);
    g_actionIconMap[showSidebarAction] = "\ue728";

    QAction *showOutputAction = viewMenu->addAction(createFluentIcon("\ue7e8"), tr("显示输出窗口"));
    showOutputAction->setCheckable(true);
    g_actionIconMap[showOutputAction] = "\ue7e8";

    QMenu *zoomMenu = viewMenu->addMenu(createFluentIcon("\ue71e"), tr("缩放"));
    g_menuIconMap[zoomMenu] = "\ue71e";
    zoomMenu->addAction(tr("放大"));
    zoomMenu->addAction(tr("缩小"));
    zoomMenu->addAction(tr("恢复默认"));

    // Build Menu
    QMenu *buildMenu = menuBar->addMenu(tr("构建"));
    QAction *buildProjectAction = buildMenu->addAction(createFluentIcon("\ue7b8"), tr("构建项目"));
    buildProjectAction->setShortcut(QKeySequence("Ctrl+B"));
    g_actionIconMap[buildProjectAction] = "\ue7b8";

    QAction *rebuildAction = buildMenu->addAction(createFluentIcon("\ue7b8"), tr("重新构建"));
    g_actionIconMap[rebuildAction] = "\ue7b8";

    buildMenu->addSeparator();

    QAction *runAction = buildMenu->addAction(createFluentIcon("\ue768"), tr("运行"));
    g_actionIconMap[runAction] = "\ue768";

    QAction *debugAction = buildMenu->addAction(createFluentIcon("\ue7a6"), tr("调试"));
    g_actionIconMap[debugAction] = "\ue7a6";

    QMenu *buildTargetMenu = buildMenu->addMenu(createFluentIcon("\ue8b5"), tr("构建目标"));
    g_menuIconMap[buildTargetMenu] = "\ue8b5";
    buildTargetMenu->addAction("Debug");
    buildTargetMenu->addAction("Release");

    // Help Menu
    QMenu *helpMenu = menuBar->addMenu(tr("帮助"));
    QAction *docsAction = helpMenu->addAction(createFluentIcon("\ue8a5"), tr("文档"));
    g_actionIconMap[docsAction] = "\ue8a5";

    QAction *apiAction = helpMenu->addAction(createFluentIcon("\ue8a5"), tr("API参考"));
    g_actionIconMap[apiAction] = "\ue8a5";

    helpMenu->addSeparator();

    QAction *updateAction = helpMenu->addAction(createFluentIcon("\ue7b8"), tr("检查更新"));
    g_actionIconMap[updateAction] = "\ue7b8";

    helpMenu->addSeparator();

    QAction *aboutAction = helpMenu->addAction(createFluentIcon("\ue946"), tr("关于"));
    g_actionIconMap[aboutAction] = "\ue946";
}

void MainWindow::rebuildMenuAndToolBar()
{
    // Clear icon maps
    g_actionIconMap.clear();
    g_menuIconMap.clear();
    m_menuBar->clear();
    if (m_toolBar)
    {
        // Toolbar controls may be reparented to the main window; disconnect before teardown so
        // destructors cannot emit signals that run lambdas using member pointers we are about to clear.
        if (themeComboBox)
        {
            QObject::disconnect(themeComboBox, nullptr, this, nullptr);
        }
        if (m_colorSchemeCombo)
        {
            QObject::disconnect(m_colorSchemeCombo, nullptr, this, nullptr);
        }
        if (m_styleComboBox)
        {
            QObject::disconnect(m_styleComboBox, nullptr, this, nullptr);
        }
        if (m_tabBarWidgetBg)
        {
            QObject::disconnect(m_tabBarWidgetBg, nullptr, this, nullptr);
        }

        m_navigationToggleAction = nullptr;
        themeComboBox = nullptr;
        m_colorSchemeCombo = nullptr;
        m_styleComboBox = nullptr;
        m_tabBarWidgetBg = nullptr;

        removeToolBar(m_toolBar);
        delete m_toolBar;
        m_toolBar = nullptr;
    }

    // Initialize menu
    buildMainMenus();

    // Initialize toolbar
    m_toolBar = addToolBar(tr("工具栏"));

    m_navigationToggleAction = addToolBarAction(m_toolBar, QStringLiteral("\uE700"), QString(), QKeySequence());
    if (m_navigationToggleAction)
    {
        connect(m_navigationToggleAction,
                &QAction::triggered,
                this,
                [this](bool checked)
                {
                    Q_UNUSED(checked);
                    if (!m_winUINavigationView)
                    {
                        return;
                    }
                    const bool expand = m_winUINavigationView->navigationExpanded();
                    m_winUINavigationView->setNavigationExpanded(!expand);
                    ui->rBOnlyIcon->setChecked(expand);
                    ui->rBIconAndText->setChecked(!expand);
                });
    }
    m_toolBar->addSeparator();

    // Add file actions
    addToolBarAction(m_toolBar, "\ue8a5", tr("新建"), QKeySequence("Ctrl+N"));
    addToolBarAction(m_toolBar, "\ue8a5", tr("打开"), QKeySequence("Ctrl+O"));
    addToolBarAction(m_toolBar, "\ue74e", tr("保存"), QKeySequence("Ctrl+S"));

    m_toolBar->addSeparator();

    // Add edit actions
    addToolBarAction(m_toolBar, "\ue7a7", tr("撤销"), QKeySequence("Ctrl+Z"));
    addToolBarAction(m_toolBar, "\ue7a6", tr("重做"), QKeySequence("Ctrl+Y"));

    m_toolBar->addSeparator();

    addToolBarAction(m_toolBar, "\ue8c6", tr("剪切"), QKeySequence("Ctrl+X"));
    addToolBarAction(m_toolBar, "\ue8c8", tr("复制"), QKeySequence("Ctrl+C"));
    addToolBarAction(m_toolBar, "\ue8c7", tr("粘贴"), QKeySequence("Ctrl+V"));

    m_toolBar->addSeparator();

    // Add build actions
    addToolBarAction(m_toolBar, "\ue7b8", tr("构建"), QKeySequence("Ctrl+B"));
    addToolBarAction(m_toolBar, "\ue7b8", tr("重新构建"), QKeySequence());
    addToolBarAction(m_toolBar, "\ue768", tr("运行"), QKeySequence());

    m_toolBar->addSeparator();

    // Add toolbar controls
    setupToolBarControls(m_toolBar);

    m_toolBar->setAttribute(Qt::WA_TranslucentBackground, true);
    m_toolBar->setAttribute(Qt::WA_StyledBackground, false);
    m_toolBar->setAutoFillBackground(false);
}

QAction *MainWindow::addToolBarAction(QToolBar *toolBar, const QString &iconCode, const QString &text, const QKeySequence &shortcut)
{
    const QString label = text.isEmpty() ? QString() : tr(text.toUtf8().constData());
    QAction *action = toolBar->addAction(createFluentIcon(iconCode), label);
    if (!shortcut.isEmpty())
    {
        action->setShortcut(shortcut);
    }
    g_actionIconMap[action] = iconCode;
    return action;
}

void MainWindow::setupToolBarControls(QToolBar *toolBar)
{
    // Disable widget checkbox
    QCheckBox *disableCheckBox = new QCheckBox(tr("禁用"), toolBar);
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
    QLabel *themeLabel = new QLabel(tr("主题："), toolBar);
    toolBar->addWidget(themeLabel);

    themeComboBox = new QComboBox(toolBar);
    themeComboBox->blockSignals(true);
    themeComboBox->addItem(tr("浅色"));
    themeComboBox->addItem(tr("暗色"));
    themeComboBox->blockSignals(false);
    themeComboBox->setView(new QListView());

#ifdef FLUENT_USE_QT_STYLE
    themeComboBox->setCurrentIndex(qApp->property("_q_colorscheme").toInt() == 1 ? 1 : 0);
#else
    themeComboBox->setCurrentIndex(fluentUIAppearance.theme() == Theme::Dark ? 1 : 0);
#endif

    connect(themeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index)
            {
#ifdef FLUENT_USE_QT_STYLE
                qApp->setProperty("_q_colorscheme", index);
                qApp->setStyle("FluentUI3");
#else
            fluentUIAppearance.setTheme(index == 0 ? Theme::Light : Theme::Dark);
#endif
                updateActionIcons();

                if (index == 0)
                {
                    ui->rBLightTheme->setChecked(true);
                }
                else
                {
                    ui->rBDarkTheme->setChecked(true);
                }
            });

    toolBar->addWidget(themeComboBox);
}

void MainWindow::setupColorSchemeSelector(QToolBar *toolBar)
{
    QLabel *colorSchemeLabel = new QLabel(tr("配色："), toolBar);
    toolBar->addWidget(colorSchemeLabel);

    m_colorSchemeCombo = new QComboBox(toolBar);
    m_colorSchemeCombo->blockSignals(true);
    m_colorSchemeCombo->addItem(QStringLiteral("Fluent"));
    m_colorSchemeCombo->addItem(QStringLiteral("Teams"));
    m_colorSchemeCombo->blockSignals(false);
    m_colorSchemeCombo->setView(new QListView());

    connect(m_colorSchemeCombo,
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
            });

    toolBar->addWidget(m_colorSchemeCombo);
}

void MainWindow::setupStyleSelector(QToolBar *toolBar)
{
    QLabel *styleLabel = new QLabel(tr("样式："), toolBar);
    toolBar->addWidget(styleLabel);

    m_styleComboBox = new QComboBox(toolBar);
    m_styleComboBox->addItems(QStyleFactory::keys());
    m_styleComboBox->setView(new QListView());

    {
        const QPointer<QComboBox> comboGuard(m_styleComboBox);
        connect(m_styleComboBox,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                [this, comboGuard](int index)
                {
                    Q_UNUSED(index);
                    if (!comboGuard)
                    {
                        return;
                    }
                    qApp->setStyle(comboGuard->currentText());
                    updateActionIcons();
                });
    }

    toolBar->addWidget(m_styleComboBox);
}

void MainWindow::setupWidgetBackgroundSelector(QToolBar *toolBar)
{
    QLabel *widgetBgLabel = new QLabel(tr("窗口背景："), toolBar);
    toolBar->addWidget(widgetBgLabel);

    m_tabBarWidgetBg = new QTabBar(toolBar);
    m_tabBarWidgetBg->setProperty(TabBarStyleProperty, Segmented_WinUI3);
    m_tabBarWidgetBg->addTab(tr("无"));
    m_tabBarWidgetBg->addTab(tr("图片"));

    toolBar->addWidget(m_tabBarWidgetBg);

    connect(m_tabBarWidgetBg,
            &QTabBar::currentChanged,
            this,
            [this](int index)
            {
                m_widgetBgMode = static_cast<WidgetBgMode>(index);
                qApp->setProperty("_q_widget_mode", index);
                qApp->setStyle("FluentUI3");

                if (index == 0)
                {
                    ui->rBWidgtModeNormal->setChecked(true);
                }
                else
                {
                    ui->rBWidgetModePixmap->setChecked(true);
                }
            });
}

//=============================================================================
// Navigation View
//=============================================================================

void MainWindow::initializeNavigationView()
{
    if (!m_winUINavigationView)
    {
        m_winUINavigationView = new ExWinUINavigationView(this);
        m_winUINavigationView->setObjectName(QStringLiteral("winUINavigationView"));
        m_winUINavigationView->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        ui->navigationPaneLayout->addWidget(m_winUINavigationView, 0, 0);
    }

    m_navView = m_winUINavigationView->mainNavView();

    m_mainNavItems.clear();
    m_mainNavItems.push_back(m_winUINavigationView->addNavigationItem(tr("基础控件"), 0, QStringLiteral("\uE80F")));
    m_mainNavItems.push_back(m_winUINavigationView->addNavigationItem(tr("表格控件"), 1, QStringLiteral("\uE99A")));
    m_mainNavItems.push_back(m_winUINavigationView->addNavigationItem(tr("列表控件"), 2, QStringLiteral("\uE71D")));
    m_mainNavItems.push_back(m_winUINavigationView->addNavigationItem(tr("树形控件"), 3, QStringLiteral("\uED28")));
    m_mainNavItems.push_back(m_winUINavigationView->addNavigationItem(tr("导航控件"), 4, QStringLiteral("\uE8B0")));
    m_mainNavItems.push_back(m_winUINavigationView->addNavigationItem(QStringLiteral("Mdi"), 5, QStringLiteral("\uE9D9")));
    m_mainNavItems.push_back(m_winUINavigationView->addNavigationItem(tr("图标库"), 7, QStringLiteral("\uE8FD")));
    m_mainNavItems.push_back(m_winUINavigationView->addNavigationItem(tr("对话框"), 9, QStringLiteral("\uE8F2")));
    m_mainNavItems.push_back(m_winUINavigationView->addNavigationItem(tr("颜色选择器"), 10, QStringLiteral("\uE790")));
    addTestNavigationTree();

    m_navAboutItem = m_winUINavigationView->addFooterNavigationItem(tr("关于"), 8, QStringLiteral("\uE77B"));
    if (QTreeWidgetItem *settingsItem = m_winUINavigationView->addFooterNavigationItem(tr("设置"), 6, QStringLiteral("\uE713")))
    {
        m_navSettingsItem = settingsItem;
        settingsItem->setData(0, Qt::UserRole + 1001, true);
    }

    m_winUINavigationView->setStackedWidget(ui->stackedWidget);

    m_winUINavigationView->setNavigationExpanded(false, false);

    m_winUINavigationView->clearFooterSelection();
}

void MainWindow::addTestNavigationTree()
{
    m_navTestRoot = new QTreeWidgetItem(m_navView);
    m_navView->configureNavigationItem(m_navTestRoot, tr("测试节点"), 6, QStringLiteral("\uE9F5"));

    for (int i = 0; i < 5; ++i)
    {
        QTreeWidgetItem *childItem = new QTreeWidgetItem(m_navTestRoot);
        m_navView->configureNavigationItem(childItem, tr("子节点%1").arg(i + 1), 6);

        for (int j = 0; j < 3; ++j)
        {
            QTreeWidgetItem *subChildItem = new QTreeWidgetItem(childItem);
            m_navView->configureNavigationItem(subChildItem, tr("子节点%1-%2").arg(i + 1).arg(j + 1), 6);
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
    constexpr int kTableColumnCount = 5;
    table->setColumnCount(kTableColumnCount);

    QStringList headers;
    headers << tr("软件名称") << tr("版本") << tr("发布商") << tr("安装日期") << tr("来源");
    table->setHorizontalHeaderLabels(headers);

    table->verticalHeader()->setMinimumSectionSize(50);
    table->verticalHeader()->setDefaultSectionSize(50);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    // table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->horizontalHeader()->setFixedHeight(50);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setIconSize(QSize(20, 20));

    {
        QFont headerFont = table->horizontalHeader()->font();
        headerFont.setPixelSize(14);
        table->horizontalHeader()->setFont(headerFont);
    }

    const QList<InstalledSoftwareInfo> softwareList = queryInstalledSoftwareList();
    table->setRowCount(softwareList.size());

    for (int row = 0; row < softwareList.size(); ++row)
    {
        const InstalledSoftwareInfo &app = softwareList.at(row);
        QTableWidgetItem *nameItem = new QTableWidgetItem(app.name);
        const QIcon appIcon = iconFromSoftwareInfo(app);
        if (!appIcon.isNull())
        {
            nameItem->setIcon(appIcon);
        }
        table->setItem(row, 0, nameItem);
        table->setItem(row, 1, new QTableWidgetItem(app.version.isEmpty() ? QStringLiteral("-") : app.version));
        table->setItem(row, 2, new QTableWidgetItem(app.publisher.isEmpty() ? QStringLiteral("-") : app.publisher));
        table->setItem(row, 3, new QTableWidgetItem(app.installDate.isEmpty() ? QStringLiteral("-") : app.installDate));
        table->setItem(row, 4, new QTableWidgetItem(app.source));
    }

    table->resizeColumnsToContents();
    if (!softwareList.isEmpty())
    {
        table->selectRow(0);
    }
    else
    {
        table->setRowCount(1);
        table->setItem(0, 0, new QTableWidgetItem(tr("未读取到安装软件信息")));
        for (int col = 1; col < table->columnCount(); ++col)
        {
            table->setItem(0, col, new QTableWidgetItem(QStringLiteral("-")));
        }
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
    m_tabShowcaseWidget = new TabShowcaseWidget(scrollArea);
    scrollArea->setWidget(m_tabShowcaseWidget);
    m_tabShowcaseWidget->setAutoFillBackground(false);
    m_tabShowcaseWidget->updateTabIcons();
    pageLayout->addWidget(scrollArea);
}

//=============================================================================
// Buttons & Icons
//=============================================================================

void MainWindow::setupButtonsAndIcons()
{
    // Setup tool button
    ui->toolButton->setIcon(createFluentIcon("\ue8c3"));
    ui->pushButton_10->setText(tr("工具按钮"));
    ui->pushButton_10->setIcon(createFluentIcon("\ue713"));

    // Setup tool button 3 with menu
    setupToolButtonWithMenu();

    // Setup tool button 4 with icon and text
    ui->toolButton_4->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButton_4->setText(tr("上下按钮"));
    ui->toolButton_4->setIcon(createFluentIcon("\uE804"));

    // Setup auto-raise button
    ui->tBtnAutoRaise->setIcon(createFluentIcon("\ue804"));
}

void MainWindow::setupToolButtonWithMenu()
{
    ui->toolButton_3->setAutoRaise(false);
    ui->toolButton_3->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui->toolButton_3->setPopupMode(QToolButton::InstantPopup);
    ui->toolButton_3->setText(tr("菜单按钮"));

    QMenu *menu = new QMenu(ui->toolButton_3);
    QAction *aNewFile = menu->addAction(createFluentIcon("\ue8a5"), tr("新建文件"));
    g_actionIconMap[aNewFile] = "\ue8a5";

    QAction *aNewProj = menu->addAction(createFluentIcon("\ue8b5"), tr("新建项目"));
    g_actionIconMap[aNewProj] = "\ue8b5";

    QAction *aRecent = menu->addAction(createFluentIcon("\ue8c3"), tr("最近打开"));
    g_actionIconMap[aRecent] = "\ue8c3";

    QAction *aOpenFile = menu->addAction(createFluentIcon("\ue8a5"), tr("打开文件"));
    g_actionIconMap[aOpenFile] = "\ue8a5";

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
        subWindow->setWindowTitle(tr("子窗口 %1").arg(i + 1));
        mdiArea->addSubWindow(subWindow);
        subWindow->show();
    }

    // Add view mode switch button
    QPushButton *switchViewBtn = new QPushButton(tr("切换视图模式"), ui->page_5);
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

void MainWindow::setupSegoeIconGalleryPage()
{
    if (!ui->stackedWidget)
    {
        return;
    }

    SegoeIconGalleryWidget *galleryWidget = new SegoeIconGalleryWidget(ui->stackedWidget);
    galleryWidget->setObjectName(QStringLiteral("pageSegoeIconGallery"));
    ui->stackedWidget->addWidget(galleryWidget);
}

void MainWindow::setupAboutPage()
{
    if (!ui->stackedWidget)
    {
        return;
    }

    AboutProjectWidget *aboutPage = new AboutProjectWidget(ui->stackedWidget);
    aboutPage->setObjectName(QStringLiteral("pageAboutProject"));
    ui->stackedWidget->addWidget(aboutPage);
}

void MainWindow::setupDialogsPage()
{
    if (!ui->stackedWidget)
    {
        return;
    }
    auto *page = new DialogShowcaseWidget(ui->stackedWidget);
    page->setObjectName(QStringLiteral("pageDialogs"));
    ui->stackedWidget->addWidget(page);
}

void MainWindow::setupColorPickerPage()
{
    if (!ui->stackedWidget)
    {
        return;
    }
    auto *page = new ColorShowcaseWidget(ui->stackedWidget);
    page->setObjectName(QStringLiteral("pageColorPicker"));
    ui->stackedWidget->addWidget(page);
}

//=============================================================================
// Utility Functions
//=============================================================================

void MainWindow::updateActionIcons()
{
    // Update search icon
    if (m_searchAction)
    {
        ui->lineEditSerach->removeAction(m_searchAction);
    }
    m_searchAction = ui->lineEditSerach->addAction(createFluentIcon("\ue721"), QLineEdit::TrailingPosition);

    if (m_tabShowcaseWidget)
    {
        m_tabShowcaseWidget->updateTabIcons();
    }

    // Update button icons
    updateButtonIcons();

    // Update action icons
    updateMenuActionIcons();

    // Update menu icons
    updateMenuIcons();

    // Update navigation item icons
    updateNavigationItemIcons();
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
    if (!m_navView)
    {
        return;
    }

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

    for (int i = 0; i < m_navView->topLevelItemCount(); ++i)
    {
        updateItemIcon(m_navView->topLevelItem(i));
    }
}

void MainWindow::loadChangelog()
{
    ui->log->clear();
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
        ui->log->append(tr("无法打开changelog.txt, %1").arg(file.errorString()));
    }

    ui->log->moveCursor(QTextCursor::Start);
}

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
    ui->checkBox_5->setText(state == Qt::Checked ? tr("On") : tr("Off"));
}

void MainWindow::on_radioButton_7_clicked()
{
    ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsVertical);
    ui->spinBox->setFrame(ui->spinBox->hasFrame());
}

void MainWindow::on_radioButton_4_clicked()
{
    ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsHorizontalSides);
    ui->spinBox->setFrame(ui->spinBox->hasFrame());
}

void MainWindow::on_radioButton_5_clicked()
{
    ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsHorizontalRight);
    ui->spinBox->setFrame(ui->spinBox->hasFrame());
}

void MainWindow::on_radioButton_6_clicked()
{
    ui->spinBox->setProperty("spinBoxButtonLayout", PlusMinusHorizontalSides);
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

void MainWindow::on_rBLightTheme_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (themeComboBox)
    {
        themeComboBox->setCurrentIndex(0);
    }
}

void MainWindow::on_rBDarkTheme_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (themeComboBox)
    {
        themeComboBox->setCurrentIndex(1);
    }
}

void MainWindow::on_rBWidgtModeNormal_clicked(bool checked)
{
    Q_UNUSED(checked)
    m_tabBarWidgetBg->setCurrentIndex(0);
}

void MainWindow::on_rBWidgetModePixmap_clicked(bool checked)
{
    Q_UNUSED(checked)
    m_tabBarWidgetBg->setCurrentIndex(1);
}

void MainWindow::on_rBOnlyIcon_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (m_winUINavigationView)
        m_winUINavigationView->setNavigationExpanded(false);
}

void MainWindow::on_rBIconAndText_clicked(bool checked)
{
    Q_UNUSED(checked)
    if (m_winUINavigationView)
        m_winUINavigationView->setNavigationExpanded(true);
}

void MainWindow::on_rBLangZh_CN_clicked(bool checked)
{
    if (!checked)
    {
        return;
    }
    AppLanguage::saveUiLanguage(AppUiLanguage::Zh_CN);
    syncLanguageRadios();
    promptRestartAfterLanguageChange();
}

void MainWindow::on_rBLangEn_US_clicked(bool checked)
{
    if (!checked)
    {
        return;
    }
    AppLanguage::saveUiLanguage(AppUiLanguage::En_US);
    syncLanguageRadios();
    promptRestartAfterLanguageChange();
}

void MainWindow::on_rBLangSystem_clicked(bool checked)
{
    if (!checked)
    {
        return;
    }
    AppLanguage::saveUiLanguage(AppUiLanguage::FollowSystem);
    syncLanguageRadios();
    promptRestartAfterLanguageChange();
}

void MainWindow::promptRestartAfterLanguageChange()
{
    ExMessageBox box(this);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("界面语言"));
    box.setText(tr("语言已保存。是否立即重启应用程序？"));
    QAbstractButton *restartBtn = box.addButton(tr("立即重启"), QMessageBox::AcceptRole);
    restartBtn->setProperty("accent", true);
    box.addButton(tr("稍后"), QMessageBox::RejectRole);
    box.setDefaultButton(qobject_cast<QPushButton *>(restartBtn));
    box.exec();
    if (box.clickedButton() == restartBtn)
    {
        if (!AppLanguage::restartApplication())
        {
            ExMessageBox::warning(this, tr("界面语言"), tr("无法重新启动应用程序，请手动关闭后再次打开。"));
        }
    }
}

void MainWindow::syncLanguageRadios()
{
    if (!ui->rBLangZh_CN || !ui->rBLangEn_US || !ui->rBLangSystem)
    {
        return;
    }
    const AppUiLanguage pref = AppLanguage::savedUiLanguage();
    ui->rBLangZh_CN->blockSignals(true);
    ui->rBLangEn_US->blockSignals(true);
    ui->rBLangSystem->blockSignals(true);
    ui->rBLangZh_CN->setChecked(pref == AppUiLanguage::Zh_CN);
    ui->rBLangEn_US->setChecked(pref == AppUiLanguage::En_US);
    ui->rBLangSystem->setChecked(pref == AppUiLanguage::FollowSystem);
    ui->rBLangZh_CN->blockSignals(false);
    ui->rBLangEn_US->blockSignals(false);
    ui->rBLangSystem->blockSignals(false);
}

void MainWindow::setupAccentColorWidget()
{
    if (ui->widgetAccentColor->layout())
    {
        delete ui->widgetAccentColor->layout();
    }
    QHBoxLayout *layout = new QHBoxLayout(ui->widgetAccentColor);

    QList<QColor> colors = {
        QColor(),          // Default (represented by invalid QColor)
        QColor("#FFB900"), // Yellow
        QColor("#FF8C00"), // Orange
        QColor("#E81123"), // Red
        QColor("#E3008C"), // Magenta
        QColor("#881798"), // Purple
        QColor("#0078D4"), // Blue
        QColor("#00B7C3"), // Teal
        QColor("#107C10")  // Green
    };

    QFont iconFont("Segoe Fluent Icons");
    iconFont.setPixelSize(20);

    QButtonGroup *btnGroup = new QButtonGroup(this);
    btnGroup->setExclusive(true);

    for (int i = 0; i < colors.size(); ++i)
    {
        QColor color = colors[i];
        QPushButton *btn = new QPushButton(ui->widgetAccentColor);
        btn->setFixedSize(40, 40);
        btn->setCheckable(true);
        btn->setFont(iconFont);
        btnGroup->addButton(btn, i);

        QColor bgColor = color.isValid() ? color : QColor("#0078D4");
        QString style = QString(
                            "QPushButton {"
                            "   background-color: %1;"
                            "   border: 1px solid rgba(0, 0, 0, 0.1);"
                            "   border-radius: 4px;"
                            "}"
                            "QPushButton:hover {"
                            "   background-color: %2;"
                            "}"
                            "QPushButton:pressed {"
                            "   background-color: %3;"
                            "}")
                            .arg(bgColor.name())
                            .arg(bgColor.lighter(110).name())
                            .arg(bgColor.darker(110).name());

        btn->setStyleSheet(style);

        if (!color.isValid())
        {
            btn->setToolTip(tr("恢复默认"));
        }
        else
        {
            btn->setToolTip(color.name());
        }
        layout->addWidget(btn);
    }
    layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Preferred));

    connect(btnGroup, &QButtonGroup::idClicked, this, [=](int id)
            {
                for (QAbstractButton *b : btnGroup->buttons())
                {
                    b->setText("");
                }

                QAbstractButton *clickedBtn = btnGroup->button(id);
                if (clickedBtn)
                {
                    clickedBtn->setText(QString::fromUtf16(u"\uE73E"));
                }

                QColor color = colors[id];
                if (color.isValid())
                {
                    qApp->setProperty("_q_accent_color", color);
                }
                else
                {
                    qApp->setProperty("_q_accent_color", QVariant());
                }
                qApp->setStyle("FluentUI3"); // Trigger repaint
            });

    QAbstractButton *defaultBtn = btnGroup->button(0);
    if (defaultBtn)
    {
        defaultBtn->setChecked(true);
        defaultBtn->setText(QString::fromUtf16(u"\uE73E"));
    }
}

//=============================================================================
// End of File
//=============================================================================
