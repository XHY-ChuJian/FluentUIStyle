---
name: qt-fluentui3
description: >-
  FluentUI3Style、Frameless 与 ExWidgets 现代化 WinUI 3 风格界面的 AI 专属开发指南。
  基于 Gallery 生产示例走查整理，涵盖项目初始化与 CMake 集成、
  无边框窗口与 DWM 材质外壳、导航布局体系、底层背景透视机制（去底色与要底色原理）、
  fluentui3styleproperties.h 全量动态属性、ExWidgets 全家族控件精确 API，
  以及 Gallery 高级实战技巧与避坑清单。
---

# FluentUI3Style 与 ExWidgets 现代化界面开发使用指南 (AI 专属 Skill)

本指南基于 Gallery 示例源码走查提炼，面向在 Qt 项目中使用 **FluentUI3Style** 样式引擎、**Frameless** 无边框外壳与 **ExWidgets** 控件库构建 WinUI 3 风格界面的 AI 助手与开发者。

---

## 1. 工程集成与启动初始化 (`CMakeLists.txt` & `main.cpp`)

### 1.1 CMake 目标链接

参考 `CMakeLists.txt` 与 `Gallery` 示例的 `CMakeLists.txt`：

- **核心控件库**：`ExWidgets::ExWidgets`（内部已包含各子目录头文件路径与 `kissfft` 依赖）。
- **样式引擎**：Windows 下通常编译为 Qt Style Plugin（`"FluentUI3"`）；非 Windows 或需直接实例化时链接 `FluentUI3Style` 目标（或 `FluentUI3Style::FluentUI3Style`）。
- **无边框与 DWM 材质（可选，非 Android 桌面端）**：链接 `Frameless::Frameless`。
- **Qt 6.10+ 私有库**：当启用 Style 源码直连且使用私有头时按需附带 `${FLUENT_QT_PRIVATE_LIBS}`。

### 1.2 `main.cpp` 标准启动顺序

参考 `main.cpp`：

```cpp
int main( int argc, char* argv[] )
{
    // 1. Qt 5 高 DPI 适配（必须在 QApplication 构造前设置）
#if ( QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 ) )
    QGuiApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
    QGuiApplication::setAttribute( Qt::AA_UseHighDpiPixmaps );
#    if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough );
#    endif
#endif

    QApplication a( argc, argv );

    // 2. 优先加载本地 plugins / styles 目录，避免跨环境插件漫游
    const QString appDir = QCoreApplication::applicationDirPath();
    QApplication::addLibraryPath( appDir + QStringLiteral( "/plugins" ) );
    QApplication::addLibraryPath( appDir + QStringLiteral( "/styles" ) );

    // 3. 在 setStyle 之前配置全局样式属性
    qApp->setProperty( "secondLevelRoundingRadius", 3 );
    qApp->setProperty( "_q_scrollHint_center", false ); // false=WinUI3 下拉动画，true=居中弹出
    qApp->setProperty( "_q_themestyle", 0 );            // 0=Fluent 配色，1=Teams 配色

    // 4. 应用 FluentUI3 样式
#ifdef Q_OS_WIN
    qApp->setStyle( "FluentUI3" );
#elif defined( HAS_FLUENTUI3_STYLE_LIB )
    qApp->setStyle( new FluentUI3Style );
#endif

    // 5. 推荐字体配置（禁 hinting 更贴近现代渲染）
    QFont font = a.font();
    font.setPixelSize( 13 );
    font.setFamily( "微软雅黑" );
    font.setHintingPreference( QFont::PreferNoHinting );
    a.setFont( font );

    MainWindow w;
    w.show();
    return a.exec();
}
```

---

## 2. 外层 UI 框架、无边框外壳与导航体系

Gallery 采用典型的现代 WinUI 3 沉浸式分层架构，视觉层级严格如下：

```text
QMainWindow (顶层宿主：可选挂载 FluentWindowFrame 实现无边框 + DWM Mica/Blur，或自绘壁纸)
 ├─ [可选无边框顶栏] FluentWindowFrame -> m_chromeHeader (FluentTitleBar + QMenuBar)
 └─ centralwidget (WA_TranslucentBackground=true, WA_StyledBackground=true)
     │
     ├─ [顶层外壳] 菜单栏 / 工具栏 (无底色，直接透出窗口底层背景/DWM 材质)
     │   - setAutoFillBackground(false)
     │   - setAttribute(Qt::WA_TranslucentBackground, true)
     │   - setAttribute(Qt::WA_StyledBackground, false)
     │
     ├─ [左侧导航] ExWinUINavigationView (透出底层材质，自适应收起/展开)
     │   - 内部持有 mainNavView() 与 footerNavView() 两个 ExNavTreeWidget
     │
     └─ [右侧主工作区] ExStackedWidget (带平滑滑动动画的多页面容器)
         └─ 各子页面 PageXXX (继承自 QFrame，设为 QFrame::StyledPanel)
             │   (FluentUI3Style 在此层绘制 QPalette::Base 底色与 6px 圆角)
             └─ QScrollArea (滚动视口，必须彻底剥除底色)
                 └─ contentWidget (内容画布，禁用 autoFillBackground)
                     └─ 若干 Card 控件 (二级交互卡片：isCard=true + WA_StyledBackground=true)
```

### 2.1 无边框窗口与 DWM 材质 (`FluentWindowFrame` & `FluentTitleBar`)

参考 `fluentwindowframe.h`、`fluenttitlebar.h` 与 `mainwindow.cpp`：

```cpp
// 1. 构造期前置设置（避免创建原生子窗口句柄导致 DWM 透明异常）
setAttribute( Qt::WA_DontCreateNativeAncestors );
ui->setupUi( this );

// 2. 安装无边框外壳与自定义标题栏（自动接管标题栏拖拽与最小化/最大化/关闭按钮）
m_menuBar = new QMenuBar( this );
m_windowFrame = new FluentWindowFrame( this, this );
m_windowFrame->installChromeHeader( m_menuBar );

// 3. 连接标题栏内置的主题、置顶与强调色交互
FluentTitleBar* titleBar = m_windowFrame->titleBar();
titleBar->setThemeDark( qApp->property( "_q_colorscheme" ).toInt() == 1 );
connect( titleBar->themeButton(), &QToolButton::clicked, this, ... );
connect( titleBar->pinButton(), &QToolButton::toggled, this, ... );
connect( titleBar, &FluentTitleBar::accentColorChanged, this, ... );
```

- **窗口材质切换**：调用 `m_windowFrame->setWindowBackdrop( key )` 或 `clearWindowBackdrop()`。支持关键字包括 `"dwm-blur"`、`"acrylic-material"`、`"mica"`、`"mica-alt"`、`"none"`，并需同步设置 `qApp->setProperty( "_q_widget_mode", ... )` 使卡片与面板进入半透明融合模式。
- **Windows 无闪烁置顶**：不要调用 `setWindowFlag(Qt::WindowStaysOnTopHint)`（会销毁重建 HWND 导致黑屏闪烁），请直接使用 Win32 API：
  ```cpp
  #ifdef Q_OS_WIN
  SetWindowPos( (HWND)w->winId(), topMost ? HWND_TOPMOST : HWND_NOTOPMOST,
                0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
  #endif
  ```

### 2.2 左侧导航 `ExWinUINavigationView` 使用实战

参考 `exwinuinavigationview.h`、`exnavtreewidget.h` 与 `mainwindow.cpp`：

```cpp
#include "exwinuinavigationview.h"
#include "exnavtreewidget.h"
#include "exstackedwidget.h"
#include "exfonticon.h"

// 1. 创建导航控件并加入主布局
auto* navView = new ExWinUINavigationView( this );
navView->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Expanding );

// 2. 添加主菜单项（支持 SegoeIcon::Type 或 QString iconCode）
navView->addNavigationItem( tr( "基础控件" ), 0, SegoeIcon::Home );
navView->addNavigationItem( tr( "表格控件" ), 1, QStringLiteral( "\uE99A" ) );

// 3. 添加多级折叠子菜单（通过 mainNavView() 获取 ExNavTreeWidget*）
ExNavTreeWidget* mainTree = navView->mainNavView();
auto* groupRoot = new QTreeWidgetItem();
mainTree->configureNavigationItem( groupRoot, tr( "ExWidgets" ), 5, QStringLiteral( "\uE8F1" ) );
mainTree->addTopLevelItem( groupRoot );

auto* childItem = new QTreeWidgetItem( groupRoot );
mainTree->configureNavigationItem( childItem, tr( "轮播图" ), 6, QStringLiteral( "\uE91B" ) );
groupRoot->setExpanded( true );

// 4. 添加底部常驻项（如系统监视、关于、设置）
navView->addFooterNavigationItem( tr( "关于" ), 8, SegoeIcon::Contact );
QTreeWidgetItem* settingsItem = navView->addFooterNavigationItem( tr( "设置" ), 9, SegoeIcon::Settings );
if ( settingsItem ) {
    // 特殊角色标记：开启设置图标在点击/悬停时的 WinUI 3 旋转动效
    settingsItem->setData( 0, Qt::UserRole + 1001, true );
}

// 5. 绑定页面容器并控制展开状态
navView->setStackedWidget( ui->stackedWidget );
navView->setNavigationExpanded( true, /*animated=*/false );
navView->clearFooterSelection();

// 6. 代码驱动切页并联动高亮
navView->setSelectedPageIndex( 8 );
```

---

## 3. 背景与底色机制深度解析：如何“不要底色”与如何“要底色”

WinUI 3 的美感源自**清晰的材质层次**（窗口透视 -> 主面板底板 -> 卡片微凸起）。错误地开启或遗漏底色会导致大面积死板灰色方块或内容层叠发黑。

### 3.1 源码绘制机理速懂

1. **一级主面板：`QFrame::StyledPanel`**（`fluentui3style.cpp` `PE_Frame`）
   - 样式引擎会用 `option->palette.brush( QPalette::Base )` 绘制底色，并自动裁剪 **6px** 圆角（除非设置了 `NoRoundedCorners` 属性）。
   - 子 Page 最外层继承 `QFrame` 并设置 `setFrameShape( QFrame::StyledPanel )`，即可获得标准的洁白/深灰内容面板底色与 6px 圆角。
2. **二级卡片容器：`isCard` 属性**（`fluentui3style.cpp` `PE_Widget`）
   - 任何 `QWidget`（或 `QFrame`）只需设置 `card->setAttribute( Qt::WA_StyledBackground, true )` 和 `card->setProperty( "isCard", true )`，样式引擎就会为其绘制 WinUI 3 卡片（4px 圆角、`cardStrokeColorBalanced` 描边、`winUI3CardBackgroundColor` 亮暗及壁纸半透明自适应背景）。
3. **普通 `QWidget` 的底色开关：`Qt::WA_StyledBackground`**（`fluentui3style.cpp` `PE_Widget`）
   - 普通 `QWidget` 默认透明不绘底；一旦开启 `WA_StyledBackground(true)` 且**没有** `isCard`（也非 `QTabBar` / `QMessageBox`），样式引擎会取 `widget->backgroundRole()`（默认 `QPalette::Window`）**直接用实心画刷填充整个矩形**！
   - **去底色时大忌**：用于页面排版、网格布局或滚动的中间层普通 `QWidget`，**严禁开启 `WA_StyledBackground(true)`**！
   - **要底色时神器**：需要纯色底板时，配合 `setBackgroundRole(...)` + `setAttribute(Qt::WA_StyledBackground, true)` 即可，无需手写 QSS。
4. **界面灰块的头号元凶：`autoFillBackground(true)`**
   - 开启后 Qt 会在 `paintEvent` 前强制用 `backgroundRole()` 擦除矩形。
   - **Qt Designer 暗坑**：在 `.ui` 中拖入的 `QScrollArea`，其内部 `scrollAreaWidgetContents` 在生成的 `ui_xxx.h` 里默认带有 `autoFillBackground(true)`（参见 `mainwindow.ui`），必须在 `setupUi(this)` 后手动 `setAutoFillBackground(false)`！

### 3.2 权威编码范式

#### 规范 A：彻底“不要底色”（容器、滚动区与透明外壳）

```cpp
// 1. 手写 QScrollArea 标准配置
scrollArea->setWidgetResizable( true );
scrollArea->setFrameShape( QFrame::NoFrame );
scrollArea->setAutoFillBackground( false );
scrollArea->viewport()->setAutoFillBackground( false );
scrollArea->viewport()->setAttribute( Qt::WA_StyledBackground, false );

contentWidget->setAutoFillBackground( false );
contentWidget->setAttribute( Qt::WA_StyledBackground, false );

// 2. Qt Designer UI 初始化后立即覆盖默认灰底
ui->scrollArea_2->viewport()->setAutoFillBackground( false );
ui->scrollAreaWidgetContents->setAutoFillBackground( false );
ui->scrollAreaWidgetContents_4->setAutoFillBackground( false );

// 3. 菜单栏、工具栏透出底层材质/壁纸
m_menuBar->setAttribute( Qt::WA_TranslucentBackground, true );
m_menuBar->setAttribute( Qt::WA_StyledBackground, false );
m_menuBar->setAutoFillBackground( false );
```

#### 规范 B：规范“要底色”（主面板、卡片容器与弱化文本）

```cpp
// 1. 页面整页主底板（一级面板）
setFrameShape( QFrame::StyledPanel );

// 2. 功能分组卡片（二级 Card）
auto* card = new QWidget( content );
card->setProperty( "isCard", true );
card->setAttribute( Qt::WA_StyledBackground, true );

// 3. 弱化次级文本（不依赖外部属性，直接用调色板 QPalette::Mid）
QPalette descPal = descLabel->palette();
descPal.setColor( QPalette::WindowText, descPal.color( QPalette::Mid ) );
descLabel->setPalette( descPal );

// 4. 状态微交互与强调底色（复用 FluentUI3Colors）
#include "fluentui3colors.h"
const bool isDark = palette().color( QPalette::Window ).lightness() < 128;
const QColor hoverBg = WINUI3Colors[ isDark ? 1 : 0 ][ subtleHighlightColor ];
const QColor pressBg = WINUI3Colors[ isDark ? 1 : 0 ][ subtlePressedColor ];
```

---

## 4. FluentUI3Style 动态属性与全局主题开关全表

极力推荐引入 `#include "fluentui3styleproperties.h"`（见 `fluentui3styleproperties.h`），优先使用常量与枚举，避免手写魔法字符串。

### 4.1 控件级动态属性字典

| 头文件常量 (`fluentui3styleproperties.h`) | 属性名字符串 | 适用控件 | 有效值 / 枚举 | 视觉与交互效果说明 |
| :--- | :--- | :--- | :--- | :--- |
| `ButtonAccentStyleProperty` | `"accent"` | `QPushButton` / `QToolButton` | `true` | 强调色主按钮（高对比度文字反色）。**注意：不要写成 `"isAccent"`**。 |
| `ButtonShapeProperty` | `"buttonShape"` | `QPushButton` / `QToolButton` | `ButtonShapeCapsule` (`"capsule"`)<br>`ButtonShapeCircle` (`"circle"`) | 按钮轮廓：胶囊型或正圆形。 |
| *(通用约定属性)* | `"isCard"` | `QWidget` / `QFrame` | `true` | 需配合 `WA_StyledBackground(true)`，绘制 4px 圆角、微描边与自适应底色卡片。 |
| `SwitchStyleProperty` | `"isSwitchButton"` | `QCheckBox` | `true` | 将复选框绘制为带滑块拖拽与动画的 WinUI 3 ToggleSwitch。 |
| `NavigationViewStyleProperty` | `"navigationViewIndicator"` | `QTreeWidget` / `QTreeView` | `true` | 激活左侧导航树选中时的竖向胶囊指示器与滑动过渡动画。 |
| *(视图辅助属性)* | `"ItemHeight"` | `QTreeView` / `QTreeWidget` | `int` (如 `32`、`34`) | 自定义树形控件单行高度（像素）。 |
| `ProgressBarStyleProperty` | `"progressBarStyle"` | `QProgressBar` | `ProgressBarThin` (0)<br>`ProgressBarThick` (1)<br>`ProgressBarRing` (2) | 切换细线进度条、粗条进度条或圆环进度环。 |
| `ProgressBarThicknessProperty` | `"progressBarThickness"` | `QProgressBar` | `qreal` (默认 `6`) | `ProgressBarRing` 模式下的圆环线条宽度（px）。 |
| `ProgressBarRingIndeterminateDurationProperty` | `"progressBarRingIndeterminateDuration"` | `QProgressBar` | `int` (默认 `800`) | 圆环不确定进度（`range(0,0)`）旋转一圈耗时（ms）。 |
| `TabBarStyleProperty` | `"tabBarStyle"` | `QTabBar` | `TabBarStyle` 枚举：<br>`Capsule`(1), `Pivot_Grow`(2), `Pivot_Slide`(3), `Pivot_Stretch`(4), `PillTabs`(5), `Segmented_Slide`(6), `Segmented_Fade`(7), `Navigation`(8), `Segmented_WinUI3`(9) | 切换 TabBar 视觉风格；`Segmented_*` 需配合 `WA_StyledBackground(true)` 和 `setDrawBase(false)`。 |
| `SegmentedSemiRoundProperty` | `"segmentedSemiRound"` | `QTabBar` | `true` / `false` | `Segmented_Slide` / `Segmented_Fade` 下开启半圆胶囊外观。 |
| `SegmentedBackgroundColorProperty`<br>`SegmentedBackgroundColorDarkProperty` | `"segmentedBackgroundColor"`<br>`"segmentedBackgroundColorDark"` | `QTabBar` | `QColor` | 分段控件轨道浅色/暗色自定义背景（同理支持 `SegmentedSelectedColor*`、`SegmentedHoverColor*`、`SegmentedPressedColor*`）。 |
| *(Tab 文本对齐)* | `"TextAlign"` | `QTabBar` | `static_cast<int>(Qt::AlignVCenter \| Qt::AlignLeft)` | 垂直 `Navigation` 或左侧对齐 Tab 文本对齐方式。 |
| `SliderValueTipProperty` | `"showValueTip"` | `QSlider` / `ExRangeSlider` | `true` | 拖拽手柄时在上方显示当前数值气泡。 |
| *(气泡数值格式化)* | `"scale"` / `"precision"` | `QSlider` | `double` / `int` | 气泡显示 `value / scale`，并保留 `precision` 位小数（如 `scale=100, precision=2`）。 |
| `DialStyleProperty` | `"dialStyle"` | `QDial` | `DialDots` (1), `DialRing` (2), `DialThumb` (3) | 仪表盘刻度与旋钮样式。 |
| `DialDrawValueProperty` | `"dialDrawValue"` | `QDial` | `true` / `false` | 是否在表盘中心绘制当前数值。 |
| *(枚举属性)* | `"spinBoxButtonLayout"` | `QSpinBox` / `QDoubleSpinBox` | `ArrowsHorizontalRight`, `ArrowsVertical`, `ArrowsHorizontalSides`, `PlusMinusHorizontalSides` | 控制加减按钮位置与排布形式。 |
| `NoRoundedCorners` | `"noRoundedCorners"` | `QWidget` / `QFrame` | `true` | 禁用 Style 默认圆角绘制，恢复纯直角。 |
| `ComboBoxPopupDropDownAnimationEnabledProperty` | `"comboBoxPopupDropDownAnimationEnabled"` | `qApp` 或 `QComboBox` | `true` / `false` | 控制普通 `QComboBox` 下拉弹窗动画（`ExComboBox` 不受此属性控制）。 |
| `MenuPopupAnimationEnabledProperty` | `"menuPopupAnimationEnabled"` | `qApp` 或 `QMenu` | `true` / `false` | 控制 `QMenu` 弹出过渡动效开关。 |

### 4.2 应用级全局主题与配色动态属性 (`qApp`)

参考 `main.cpp` 与 `mainwindow.cpp`：

| `qApp` 属性名 | 类型与有效值 | 作用说明 |
| :--- | :--- | :--- |
| `"_q_colorscheme"` | `int`: `0` = Light，`1` = Dark | 切换浅色/深色主题（未设置时跟随系统 `styleHints()->colorScheme()`）。 |
| `"_q_themestyle"` | `int`: `0` = Fluent，`1` = Teams | 切换整体配色策略（见 `PaletteManager`）。 |
| `"_q_widget_mode"` | `int`: `0` = 实心普通模式，`1` = 图片壁纸半透明，`2` = DWM Blur 材质 | 控制 `StyledPanel` / `isCard` 等表面是否启用半透明融合。 |
| `"_q_accent_color"` | `QColor` 或 `QVariant()` | 覆盖全局强调色；传入无效 `QVariant()` 恢复系统/配色方案默认。 |
| `"_q_scrollHint_center"` | `bool` | `true` 时 `QComboBox` 在中心弹出且不启用下拉动画；默认 `false`。 |
| `"secondLevelRoundingRadius"` | `int` (Gallery 为 `3`) | 二级元素圆角半径配置。 |

> [!IMPORTANT]
> 运行时修改 `_q_colorscheme`、`_q_themestyle`、`_q_widget_mode` 或 `_q_accent_color` 后，需调用一次样式刷新以重建全局调色板并触发重绘（`qApp->setStyle("FluentUI3")` 或 `qApp->setStyle(new FluentUI3Style)`），并在自定义页面中通过 `changeEvent(QEvent::PaletteChange / ApplicationPaletteChange)` 同步更新自定义图标或自绘色值。

### 4.3 `TabBarStyle` 组合配置实战（来自 `pagetab.cpp`）

```cpp
// 1. Segmented_WinUI3 / Segmented_Slide 分段控件（必须加 WA_StyledBackground + setDrawBase(false)）
auto* segBar = new QTabBar( parent );
segBar->setAttribute( Qt::WA_StyledBackground, true );
segBar->setDrawBase( false );
segBar->setExpanding( false );
segBar->setProperty( TabBarStyleProperty, Segmented_WinUI3 ); // 或 Segmented_Slide + SegmentedSemiRoundProperty
segBar->addTab( tr( "无" ) );
segBar->addTab( tr( "图片" ) );

// 2. ExTabWidget 配合 Pivot_Slide 作为卡片内分栏（如 PageLiquidGauge / PageTimeline）
auto* tabs = new ExTabWidget( card );
tabs->tabBar()->setProperty( TabBarStyleProperty, TabBarStyle::Pivot_Slide );
tabs->setStyleSheet( QStringLiteral( "QTabWidget::pane{border:none;}" ) ); // 需要无边框内容区时仅剥离 pane 边框

// 3. 左侧垂直 Navigation 风格 TabWidget
auto* navTabs = new ExTabWidget( parent );
navTabs->setTabPosition( QTabWidget::West );
navTabs->setVerticalMode( true );
navTabs->setSpeed( 220 );
navTabs->setAnimation( QEasingCurve::OutCubic );
navTabs->tabBar()->setShape( QTabBar::RoundedWest );
navTabs->tabBar()->setDrawBase( false );
navTabs->tabBar()->setExpanding( false );
navTabs->tabBar()->setProperty( "TextAlign", static_cast<int>( Qt::AlignVCenter | Qt::AlignLeft ) );
navTabs->tabBar()->setProperty( TabBarStyleProperty, Navigation );
```

---

## 5. ExWidgets 核心控件实战手册 (Exact APIs)

以下所有类、方法与信号均已对照 `ExWidgets` 头文件与 Gallery 示例 实际调用逐一核对。

### 5.1 导航与容器类

#### `ExStackedWidget`（平滑滑动翻页容器，`exstackedwidget.h`）
```cpp
#include <exstackedwidget.h>

auto* stack = new ExStackedWidget( this );
stack->setVerticalMode( true );                     // WinUI 3 推荐垂直切页
stack->setAnimation( QEasingCurve::Type::InOutSine );
stack->setSpeed( 300 );
stack->setCurrentIndex( 0 );
```

#### `ExExpander`（折叠面板，`exexpander.h` & `pagefeedback.cpp`）
> [!NOTE]
> `ExExpander` 使用 `setHeader()` / `setHeaderWidget()` 与 `addContentWidget()`（可追加多个独立内容面板），**没有** `setHeaderText()` 或 `setContentWidget()`。

```cpp
#include <exexpander.h>

auto* expander = new ExExpander( parent );
expander->setHeader( tr( "高级设置" ) );                  // 纯文本标题，或使用 setHeaderWidget(customHeader)
expander->addContentWidget( new QCheckBox( tr( "启用自动保存" ) ) ); // 支持多次调用添加多个独立面板
expander->addContentWidget( customPanelWidget );
expander->setExpandDirection( ExExpander::Down );         // 可选 ExExpander::Up（向上展开）
expander->setAnimationEnabled( true );
expander->setAnimationDuration( 167 );                    // 动画时长(ms)
expander->setExpanded( false );                           // 默认折叠
// 信号：expanding()、collapsed()、expansionFinished(bool expanded)、expandedChanged(bool)
```

#### `ExCarousel`（轮播图，`excarousel.h` & `pagecarousel.cpp`）
```cpp
#include <excarousel.h>

auto* carousel = new ExCarousel( parent );
carousel->setFixedHeight( 280 );
carousel->setBorderRadius( 6.0 );
carousel->addImage( QStringLiteral( ":/images/cliff.jpg" ), tr( "Cliff" ), tr( "海岸峭壁" ) );
carousel->addPixmap( pixmap, tr( "标题" ), tr( "副标题" ) ); // 传 QPixmap 须用 addPixmap()
carousel->addSlide( customWidget );                       // 支持任意自定义 QWidget
carousel->setAutoPlay( true );                            // 注意驼峰大小写 setAutoPlay
carousel->setInterval( 3500 );
carousel->setAnimationDuration( 600 );
carousel->setWrap( true );
carousel->setShowIndicators( true );
carousel->setShowNavigationButtons( true );
carousel->setNavigationButtonTrigger( ExCarousel::OnHover ); // 或 ExCarousel::AlwaysVisible
connect( carousel, &ExCarousel::currentIndexChanged, this, [ carousel ]( int idx ) { ... } );
```

#### `ExBreadcrumbBar`（面包屑导航，`exbreadcrumbbar.h` & `pagenavigationhint.cpp`）
```cpp
#include <exbreadcrumbbar.h>

auto* breadcrumb = new ExBreadcrumbBar( parent );
breadcrumb->setItems( { tr( "主页" ), tr( "项目" ), tr( "设置" ) } ); // QStringList 用 setItems；QVariantList 用 setItemsSource
connect( breadcrumb, &ExBreadcrumbBar::itemClicked, this,
         [ breadcrumb ]( int index, const QVariant& item ) {
             // 注意：点击不会自动截断路径，由调用方自行截断维护
             breadcrumb->setItemsSource( breadcrumb->itemsSource().mid( 0, index + 1 ) );
         } );
```

#### `ExComboBox` & `ExRangeSlider` & `ExBorderBeam`
- **`ExComboBox`**（`excombobox.h`）：内置弹窗滑出裁剪动画的 `QComboBox`，不受 `ComboBoxPopupDropDownAnimationEnabledProperty` 控制；配合 `combo->setView(new QListView(combo))` 使用。
- **`ExRangeSlider`**（`exrangeslider.h` & `pagerangeslider.cpp`）：双滑块范围选择控件：
  ```cpp
  auto* range = new ExRangeSlider( Qt::Horizontal, parent );
  range->setProperty( SliderValueTipProperty, true );
  range->setRange( 0, 100 );
  range->setValues( 20, 80 );
  range->setTickPosition( true );
  range->setTickInterval( 10 );
  connect( range, &ExRangeSlider::valuesChanged, this, []( int lower, int upper ) { ... } );
  ```
- **`ExBorderBeam` / `ExBorderBeamButton`**（`exborderbeam.h` & `exborderbeambutton.h`）：沿圆角周长巡游渐变光束容器（继承 `QFrame`）与光束按钮（继承 `QPushButton`）。`startColor`/`endColor` 留空时自动跟随调色板强调色；支持 `setBeamCount()`、`setBeamLength()`、`setCornerRadius()`、`setDirection(ExBorderBeam::CounterClockwise)`。

### 5.2 对话框、通知与引导家族

#### `ExContentDialog`（WinUI 3 三按钮模态对话框，`excontentdialog.h` & `pagedialog.cpp`）
```cpp
#include <excontentdialog.h>

ExContentDialog dlg( window() );
dlg.setTitle( tr( "永久删除文件？" ) );
dlg.setContent( tr( "删除后将无法恢复，确定要继续吗？" ) ); // 或 dlg.setContentWidget(customWidget)
dlg.setPrimaryButtonText( tr( "删除" ) );
dlg.setSecondaryButtonText( tr( "移入回收站" ) );
dlg.setCloseButtonText( tr( "取消" ) );
dlg.setDefaultButton( ExContentDialog::CloseBtn );        // PrimaryBtn / SecondaryBtn / CloseBtn
if ( dlg.showDialog() == ExContentDialog::Primary ) {
    // 处理主按钮确认
}
```

#### `ExMessageBox`（WinUI 3 样式消息框，`exmessagebox.h`）
- 静态便捷调用：`ExMessageBox::information(...)`、`warning(...)`、`critical(...)`、`question(...)`。
- 进阶用法：支持 `setInformativeText(...)`、`setCheckBox(...)`、`setContentWidget(lineEdit)` 嵌入密码输入框等自定义控件。

#### `ExInfoBar` 与 `ExInfoBarHost`（页面内 & 窗口级悬浮通知，`exinfobar.h`、`exinfobarhost.h`）
```cpp
// 1. 页面内嵌通知条
auto* bar = new ExInfoBar( parent );
bar->setSeverity( ExInfoBar::Warning ); // Informational / Success / Warning / Error
bar->setTitle( tr( "警告" ) );
bar->setMessage( tr( "网络连接不稳定。" ) );
bar->setActionButtonText( tr( "重试" ) );
bar->setOpen( true );

// 2. 窗口级悬浮堆叠通知（主窗口构造中先绑定默认宿主目标）
ExInfoBarHost::setDefaultTarget( this ); // 在 MainWindow 构造函数中调用
ExInfoBarHost::defaultHost()->showInfoBar(
    ExInfoBar::Success, tr( "已保存" ), tr( "配置已更新。" ), ExInfoBarHost::TopRight, 4500 );
// 若脱离主窗口独立使用：new ExInfoBarHost( window(), this )
```

#### `ExTeachingTip` 与 `ExTour`（引导气泡与聚光灯向导，`exteachingtip.h`、`extour.h`）
```cpp
// 1. ExTeachingTip：非模态带小尾巴浮层（用 new 创建，不加入布局）
auto* tip = new ExTeachingTip( this );
tip->setTitle( tr( "快捷配置" ) );
tip->setSubtitle( tr( "点击外部或按 Esc 关闭。" ) );
tip->setPreferredPlacement( ExTeachingTip::Bottom );
tip->setIsLightDismissEnabled( true );
tip->setActionButtonContent( tr( "知道了" ) );
connect( tip, &ExTeachingTip::actionButtonClick, tip, &ExTeachingTip::dismiss );
tip->showAt( targetButton ); // 传 nullptr 可显示无目标居中提示

// 2. ExTour：全屏聚光灯镂空漫游向导
auto* tour = new ExTour( this );
tour->addStep( themeButton, tr( "切换主题" ), tr( "支持浅色与深色模式。" ), ExTourStep::Bottom );
tour->addStep( navTarget, tr( "折叠导航" ), tr( "切换紧凑图标与展开文本。" ), ExTourStep::Bottom );
tour->start( 0 );
// 注意：页面隐藏或重建工具栏前应检查 if ( tour->isRunning() ) tour->exit();
```

### 5.3 颜色选择器家族（`pagecolor.cpp`）

- **`ExColorPickerButton`**（`excolorpickerbutton.h`）：带 Flyout 飞层的拾色按钮：
  ```cpp
  auto* colorBtn = new ExColorPickerButton( parent );
  colorBtn->setSelectedColor( QColor( "#0078D4" ) ); // 注意是 setSelectedColor
  connect( colorBtn, &ExColorPickerButton::selectedColorChanged, this, []( const QColor& c ) { ... } );
  ```
- **`ExColorPicker`**（`excolorpicker.h`）：内嵌拾色器，使用 `setColor()` / `colorChanged`，支持 `setColorSpectrumShape(ExColorPicker::Box / Ring)`、`setAlphaEnabled(bool)`、`setColorPaletteVisible(bool)`。
- **`ExColorPickerDialog`**（`excolorpickerdialog.h`）：带标题与确认/取消按钮的对话框，实时预览发出 `colorChanged`，点击确认发出 `colorSelected`，取消自动还原打开前颜色。

### 5.4 仪表盘、进度环与计时器家族

- **`ExProgressRing`**（`exprogressring.h` & `pageprogressring.cpp`）：继承 `QProgressBar`，默认启用 `ProgressBarRing` 样式；额外支持 `setTitle()`、`setTitleFont()`、`setValueFont()`、`setTitleColor()`、`setValueColor()`、`setCenterWidget(customWidget)`，不确定状态使用 `setRange(0, 0)`。
- **`ExMultiProgressRing`**（`exmultiprogressring.h`）：多层同心环，通过 `addItem(label, value, color)` 添加并动态更新各环。
- **`ExLiquidGauge`**（`exliquidgauge.h` & `pageliquidgauge.cpp`）：水波进度球（继承 `QProgressBar`），支持 `CircleShape` / `RectShape` / `PinShape` / `TriangleShape` 及 `setWaveAmplitude()`、`setWaveCount()`、`setWaveColor()`。
- **`ExRadialGauge` & `ExMultiRadialGauge`**（`exradialgauge.h`、`exmultiradialgauge.h`）：径向指针仪表盘，支持 `TrackScale` / `ProgressScale` / `RangeScale`（`addRange(from, to, color)`）及多指针。
- **`ExTimerDial`**（`extimerdial.h`）：倒计时圆环表盘，通过 `setTime(remainingMs, totalMs, running)` 驱动。

### 5.5 时间轴、多媒体与矢量图标

- **`ExTimeline`**（`extimeline.h` & `pagetimeline.cpp`）：支持垂直/水平、单侧（`ContentOnRight`/`ContentOnLeft`）与交错布局（`Alternating`/`AlternatingReverse`）、倒序（`setReverse(true)`）。通过 `addEvent(timestamp, title, desc, ExTimelineEvent::Completed / Current / Pending / Warning / Error)` 添加节点，支持 `event->setTimeText()` 和 `event->setIcon(SegoeIcon::Type)`。
- **`ExAudioLevelMeter`**（`exaudiolevelmeter.h` & `pageaudiolevelmeter.cpp`）：只读多声道实时电平表，支持 `setLevel(dB)`、`setStereoLevels(left, right)`、`setLevels(vector)`，可从普通工作线程安全调用。
- **`ExSpectrumWidget`**（`exspectrumwidget.h` & `pagespectrum.cpp`）：2048 点 FFT 实时频谱控件，仅需输入 16-bit PCM 流：`pushAudioData(pcmData)`（或连接信号到 `setAudioData`），支持 `setSampleRate()`、`setBarCount()`、`setReflectionEnabled()`、`setPeakCapEnabled()`、`setRainbowEnabled()`、`setAccentColor()`。
- **`ExLyricsWidget`**（`exlyricswidget.h`）：平滑滚动歌词与单行卡拉 OK 视图，支持 `setLyrics(lrcText)`、`setCurrentPositionMs(ms)`、`setViewMode(ExLyricsWidget::ViewMode::MultiLine / SingleLine)`。
- **`ExFontIcon`**（`exfonticon.h`）：矢量字体图标库，切勿硬编码魔法字符，优先使用 `SegoeIcon::Type`（即 `SegoeIcon::Icon`）：
  ```cpp
  #include <exfonticon.h>

  // 方式 1：获取 QIcon（适用于 QAction / QTabBar / QLineEdit::addAction / QTreeWidgetItem）
  QIcon icon = EXFONTICON->getIcon( SegoeIcon::Folder, /*pixelSize=*/20 );

  // 方式 2（小按钮强烈推荐）：用矢量字体直接绘字，高分屏绝对清晰且自动随 QPalette 变色
  auto* btn = new QToolButton( parent );
  btn->setToolButtonStyle( Qt::ToolButtonTextOnly );
  btn->setFont( ExFontIcon::iconFont( 12 ) );
  btn->setText( ExFontIcon::iconString( SegoeIcon::ChromeClose ) );
  ```

---

## 6. Gallery 经典实战高级技巧 (Advanced Recipes)

### 6.1 设置项卡片 (`SettingsCard`) 与折叠设置组 (`ExExpander`)

参考 `mainwindow.cpp`：
- 常规设置项采用“左侧 36px 图标列（22px `Segoe Fluent Icons`）+ 中间垂直主标题（15px）与 `QPalette::Mid` 弱化副标题 + 右侧操作控件（`ExComboBox` / Switch `QCheckBox` / `QToolButton`）”的 `isCard=true` 容器，最小高度 `72px`。
- 含子面板的复杂设置项（如强调色选择面板）直接使用 `ExExpander`，将上述左中右结构（不含 `isCard`）通过 `setHeaderWidget()` 注入，再把子控件通过 `addContentWidget()` 放入折叠区。

### 6.2 `QSlider` 浮动气泡的小数缩放 (`scale` 与 `precision`)

```cpp
slider->setRange( 0, 100 );
slider->setProperty( SliderValueTipProperty, true );
slider->setProperty( "scale", 100 );     // 气泡将整数值除以 scale
slider->setProperty( "precision", 2 );   // 保留 2 位小数（如 25 -> "0.25"）
```

### 6.3 单个控件专属 Accent 强调色（含失焦组 & 跨版本兼容）

参考 `pageprogressring.cpp` 与 `pageradialgauge.cpp`：

```cpp
void setWidgetAccentColor( QWidget* widget, const QColor& color )
{
    QPalette p = widget->palette();
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    p.setColor( QPalette::Active, QPalette::Accent, color );
    p.setColor( QPalette::Inactive, QPalette::Accent, color );
#else
    p.setColor( QPalette::Active, QPalette::Highlight, color );
    p.setColor( QPalette::Inactive, QPalette::Highlight, color );
#endif
    widget->setPalette( p );
}
```

### 6.4 现代搜索输入框标准范式

```cpp
auto* searchEdit = new QLineEdit( parent );
searchEdit->setPlaceholderText( tr( "搜索..." ) );
searchEdit->setClearButtonEnabled( true );
searchEdit->addAction( EXFONTICON->getIcon( SegoeIcon::Search, 16 ), QLineEdit::TrailingPosition );
```

### 6.5 表格、树形列表与日历的标准 WinUI 3 配置

- **表格 (`QTableWidget` / `QTableView`，`pageinstalledsoftware.cpp`)**：
  ```cpp
  table->verticalHeader()->setVisible( false );
  table->verticalHeader()->setDefaultSectionSize( 50 );
  table->horizontalHeader()->setFixedHeight( 50 );
  table->horizontalHeader()->setStretchLastSection( true );
  table->setSelectionBehavior( QAbstractItemView::SelectRows );
  table->setAlternatingRowColors( true );
  table->setShowGrid( false );
  ```
- **树形列表 (`QTreeWidget`)**：
  ```cpp
  treeWidget->setProperty( "ItemHeight", 32 ); // FluentUI3Style 读取该属性设置行高
  treeWidget->setIndentation( 20 );
  ```
- **日历 (`QCalendarWidget`)**：
  ```cpp
  calendar->setGridVisible( false );
  calendar->setVerticalHeaderFormat( QCalendarWidget::NoVerticalHeader );
  ```

### 6.6 复杂页面冷启动秒开（懒加载）与生命周期管理

- **按需懒加载**（`mainwindow.cpp`）：
  - 重型页面（如注册表软件列表、多媒体播放器与网络栈）不在主窗口构造函数中同步初始化，而是在 `QStackedWidget::currentChanged` 首次切入目标页时按需构造，或用 `QTimer::singleShot` 在冷启动空闲后预热。
- **页面显隐与浮层/采集器生命周期**（`pagesystemresources.cpp`、`pagenavigationhint.cpp`）：
  - 后台采样类页面在 `showEvent` 启动采集、在 `hideEvent` 停止定时器与 Provider，避免切换走后空耗 CPU。
  - 带有非模态浮层（`ExTeachingTip`、`ExTour`）的页面，在 `hideEvent` 中显式调用 `dismiss()` / `exit()`，切换页面时干净退场。
- **动态主题响应 (`changeEvent`)**：
  - 自定义绘制颜色或持有位图 `QIcon` 的页面，重写 `changeEvent(QEvent* event)` 监听 `QEvent::PaletteChange` 与 `QEvent::ApplicationPaletteChange`，实时刷新配色与图标。

---

## 7. 标准 Page 页面编写脚手架模板 (Page Scaffold)

新增功能展示或设置页面时，可直接套用以下模板：

### 7.1 头文件 (`pagedemo.h`)
```cpp
#pragma once

#include <QFrame>

class PageDemo : public QFrame
{
    Q_OBJECT

public:
    explicit PageDemo( QWidget* parent = nullptr );
    ~PageDemo() override = default;
};
```

### 7.2 源文件 (`pagedemo.cpp`)
```cpp
#include "pagedemo.h"

#include "fluentui3styleproperties.h"

#include <exfonticon.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace
{
QWidget* makeCard( QWidget* parent )
{
    auto* card = new QWidget( parent );
    card->setProperty( "isCard", true );
    card->setAttribute( Qt::WA_StyledBackground, true );
    return card;
}

QLabel* makeSectionTitle( const QString& text, QWidget* parent )
{
    auto* label = new QLabel( text, parent );
    QFont font  = label->font();
    font.setBold( true );
    font.setPixelSize( 14 );
    label->setFont( font );
    return label;
}
} // namespace

PageDemo::PageDemo( QWidget* parent )
    : QFrame( parent )
{
    // 1. 一级内容面板：自动获得 Base 底色与 6px 圆角
    setFrameShape( QFrame::StyledPanel );

    auto* rootLayout = new QVBoxLayout( this );
    rootLayout->setContentsMargins( 0, 0, 0, 0 );

    // 2. 滚动视口：必须剥离边框与自动擦除
    auto* scrollArea = new QScrollArea( this );
    scrollArea->setWidgetResizable( true );
    scrollArea->setFrameShape( QFrame::NoFrame );
    scrollArea->setAutoFillBackground( false );
    scrollArea->viewport()->setAutoFillBackground( false );
    scrollArea->viewport()->setAttribute( Qt::WA_StyledBackground, false );
    rootLayout->addWidget( scrollArea );

    // 3. 内容画布：禁用自动填充与样式自绘
    auto* content = new QWidget( scrollArea );
    content->setAutoFillBackground( false );
    content->setAttribute( Qt::WA_StyledBackground, false );
    scrollArea->setWidget( content );

    auto* mainLayout = new QVBoxLayout( content );
    mainLayout->setContentsMargins( 24, 24, 24, 24 );
    mainLayout->setSpacing( 16 );

    // 4. 页面大标题与弱化描述
    auto* title     = new QLabel( tr( "功能模块标题" ), content );
    QFont titleFont = title->font();
    titleFont.setPointSize( 16 );
    titleFont.setBold( true );
    title->setFont( titleFont );
    mainLayout->addWidget( title );

    auto* desc = new QLabel( tr( "这里是对该功能页面的详细说明与操作指引。" ), content );
    QPalette descPal = desc->palette();
    descPal.setColor( QPalette::WindowText, descPal.color( QPalette::Mid ) );
    desc->setPalette( descPal );
    desc->setWordWrap( true );
    mainLayout->addWidget( desc );

    // 5. 交互卡片 (Card)
    auto* card       = makeCard( content );
    auto* cardLayout = new QVBoxLayout( card );
    cardLayout->setContentsMargins( 16, 16, 16, 16 );
    cardLayout->setSpacing( 12 );

    cardLayout->addWidget( makeSectionTitle( tr( "操作选项" ), card ) );

    auto* btnRow      = new QHBoxLayout;
    auto* primaryBtn  = new QPushButton( tr( "主要操作 (Accent)" ), card );
    primaryBtn->setProperty( ButtonAccentStyleProperty, true );

    auto* standardBtn = new QPushButton( tr( "标准按钮" ), card );

    btnRow->addWidget( primaryBtn );
    btnRow->addWidget( standardBtn );
    btnRow->addStretch();
    cardLayout->addLayout( btnRow );

    mainLayout->addWidget( card );
    mainLayout->addStretch( 1 );
}
```

---

## 8. 避坑清单 (Checklist)

- [ ] **杜绝用 QSS 覆盖控件底色与边框**：严禁全局或卡片写 `background-color: white; border: 1px solid gray;`，否则会破坏 FluentUI3Style 的深浅主题、亚克力半透明与悬停动画。需要去掉 `QTabWidget` 内框线时，仅允许局部使用 `"QTabWidget::pane{border:none;}"`。
- [ ] **QScrollArea 必去底色**：凡是用到 `QScrollArea`，务必检查 `viewport()->setAutoFillBackground(false)` 与 `content->setAutoFillBackground(false)`。
- [ ] **`.ui` 文件必加覆盖逻辑**：Qt Designer 生成的代码会默认给 `scrollAreaWidgetContents` 设置 `autoFillBackground(true)`，必须在 `setupUi(this)` 后手动设为 `false`。
- [ ] **普通 `QWidget` 的 `WA_StyledBackground` 双刃剑**：中间布局与占位容器切勿开启 `WA_StyledBackground(true)`，否则 `PE_Widget` 会刷出实心矩形遮挡下层；只有 `isCard` 卡片或刻意做纯色底板时才开启。
- [ ] **强调色按钮只认 `ButtonAccentStyleProperty` (`"accent"`)**：不要误写为 `"isAccent"`（`FluentUI3Style` 不识别该属性）。
- [ ] **注意易混淆的 ExWidgets API**：
  - `ExExpander`：`setHeader()` / `setHeaderWidget()` / `addContentWidget()`（非 `setHeaderText` / `setContentWidget`）。
  - `ExCarousel`：`addPixmap(QPixmap)` / `addImage(QString)` / `setAutoPlay()` / `setShowIndicators()`（非 `addImage(QPixmap)` / `setAutoplay` / `setIndicatorPosition`）。
  - `ExColorPickerButton`：`setSelectedColor()` / `selectedColorChanged`（区别于 `ExColorPicker` 的 `setColor()` / `colorChanged`）。
  - `ExBreadcrumbBar`：`itemClicked(int index, const QVariant& item)` 不会自动截断路径，需自行维护 `itemsSource`。
- [ ] **Windows 窗口置顶用 `SetWindowPos`**：避免使用 `setWindowFlag(Qt::WindowStaysOnTopHint)` 触发 HWND 重建闪烁。
- [ ] **小图标优先矢量字体渲染**：尺寸小于 16px 的图标尽量使用 `setFont(ExFontIcon::iconFont(12))` + `setText(ExFontIcon::iconString(icon))`，无需切主题重建位图且高分屏不模糊。
