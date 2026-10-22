---
name: fluentui3-exwidgets-dev
description: >-
  FluentUI3Style 与 ExWidgets 现代化 WinUI 3 风格界面的 AI 专属使用指南。
  指导如何使用 Gallery 外层框架与导航布局、掌握底层背景透视机制（去底色与要底色原理）、
  处理 QScrollArea 视口透明坑点、运用 isCard 卡片系统与动态属性、
  以及调用 ExWidgets 核心控件构建现代化界面。
---

# FluentUI3Style 与 ExWidgets 现代化界面开发使用指南 (AI 专属 Skill)

本指南面向在本仓库中使用 **FluentUI3Style** 样式引擎与 **ExWidgets** 控件库构建 WinUI 3 风格界面的 AI 助手与开发者。指导如何快速搭建界面、正确组织视觉层级、精准控制背景透明与卡片底色、并熟练调用各项组件。

---

## 1. 外层 UI 框架与导航体系 (Shell & Navigation Framework)

Gallery 采用典型的现代 WinUI 3 沉浸式分层架构。其外层结构与视觉层级严格如下：

```text
QMainWindow (顶层宿主：负责无边框、DWM Mica/Acrylic 亚克力材质或壁纸绘制)
 └─ centralwidget (WA_TranslucentBackground=true, WA_StyledBackground=true)
     │
     ├─ [顶层外壳] 标题栏 / 菜单栏 / 工具栏 (无底色，直接透出窗口底层背景)
     │   - setAutoFillBackground(false)
     │   - setAttribute(Qt::WA_TranslucentBackground, true)
     │   - setAttribute(Qt::WA_StyledBackground, false)
     │
     ├─ [左侧导航] ExWinUINavigationView (透出底层材质，自适应收起/展开)
     │   - ExNavTreeWidget: setFrameShape(QFrame::NoFrame)
     │   - viewport()->setAutoFillBackground(false)
     │   - viewport()->setAttribute(Qt::WA_StyledBackground, false)
     │
     └─ [右侧主工作区] QFrame::StyledPanel (一级内容面板 Surface)
         │   (FluentUI3Style 在此层绘制 QPalette::Base 浅白/深灰底色与 6px 圆角)
         │
         └─ ExStackedWidget (带平滑滑动动画的多页面容器)
             └─ 各子页面 PageXXX (继承自 QFrame，设为 StyledPanel)
                 └─ QScrollArea (滚动视口，必须彻底剥除底色)
                     └─ contentWidget (内容画布，无底色)
                         └─ 若干 Card 控件 (二级交互卡片：isCard=true)
```

### 1.1 左侧导航 `ExWinUINavigationView` 使用实战

`ExWinUINavigationView` 封装了左侧主菜单列表、分割线与底部固定操作区，并内置了 WinUI 3 样式的项选中指示器滑动动画。

```cpp
#include "exwinuinavigationview.h"
#include "exstackedwidget.h"
#include "exfonticon.h"

// 1. 创建导航控件并加入主布局
auto* navView = new ExWinUINavigationView( this );
navView->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Expanding );

// 2. 添加主菜单项 (文本, 对应页面索引, 矢量图标)
navView->addNavigationItem( tr( "基础控件" ), 0, SegoeIcon::ViewAll );
navView->addNavigationItem( tr( "多媒体" ),   1, SegoeIcon::Media );
navView->addNavigationItem( tr( "时间轴" ),   2, SegoeIcon::Timeline );

// 3. 添加底部常驻项 (关于、设置等)
navView->addFooterNavigationItem( tr( "关于" ), 3, SegoeIcon::Info );
navView->addFooterNavigationItem( tr( "设置" ), 4, SegoeIcon::Settings );

// 4. 绑定页面容器 (点击导航项自动切页)
navView->setStackedWidget( ui->stackedWidget );

// 5. 展开/收起控制
navView->setNavigationExpanded( true, /*animated=*/false );
```

---

## 2. 背景与底色机制深度解析：如何“不要底色”与如何“要底色”

WinUI 3 的核心美感在于**清晰的材质层次**（窗口透视 -> 主面板底板 -> 卡片微凸起）。错误地开启或遗漏底色会导致大面积死板灰色方块或内容层叠发黑。

### 2.1 源码绘制机理速懂

#### 1. 一级主面板：`QFrame::StyledPanel`（`fluentui3style.cpp:3106` `PE_Frame`）
```cpp
// FluentUI3Style 源码绘制逻辑：
painter->setBrush( option->palette.brush( QPalette::Base ) );
painter->drawRoundedRect( rect, 6, 6 ); // 填充 Base 浅白/深灰底色，裁剪 6px 优雅圆角
```
* **结论**：只要控件是 `QFrame` 且将 `frameShape` 设为 `StyledPanel`，样式引擎就会自动将其作为**主内容承载面板（Surface）**绘制洁白底色与 6px 圆角。右侧主工作区和所有子 Page 的最外层均应采用此模式。

#### 2. 二级卡片容器：`isCard` 属性（`fluentui3style.cpp:3322` `PE_Widget`）
```cpp
// FluentUI3Style 源码绘制逻辑：
if ( widget && widget->property( "isCard" ).toBool() ) {
    painter->setPen( winUI3Color( cardStrokeColorBalanced ) );
    painter->setBrush( winUI3CardBackgroundColor( option->palette, isDark, wallpaperMode ) );
    painter->drawRoundedRect( r, 4, 4 ); // 4px 微圆角与自适应卡片底色
    return;
}
```
* **结论**：任何普通 `QWidget` 只需设置 `card->setAttribute(Qt::WA_StyledBackground, true)` 和 `card->setProperty("isCard", true)`，样式引擎就会将其绘制为标准的 WinUI 3 卡片（亮暗自适应底色、自适应壁纸透明度、4px 圆角与柔和描边）。

#### 3. 普通 QWidget 的底色开关：`Qt::WA_StyledBackground`（`fluentui3style.cpp:3390` `PE_Widget`）
```cpp
// FluentUI3Style 源码绘制逻辑 (PE_Widget)：
if ( widget && widget->palette().isBrushSet( QPalette::Active, widget->backgroundRole() ) ) {
    const QBrush bg = option->palette.brush( widget->backgroundRole() );
    updateBrushOrigin_public( painter, widget, bg );
    painter->fillRect( option->rect, bg ); // 直接填充整个部件矩形！
}
```
* **核心机制**：在 Qt 体系中，普通 `QWidget` 默认不会由 QStyle 绘制背景（`paintEvent` 默认透明并透出父级）。**但一旦开启 `widget->setAttribute(Qt::WA_StyledBackground, true)`**，Qt 就会将绘制委托给样式引擎的 `PE_Widget`：
  - 若带有 `isCard` 属性，则绘制带边框的 4px 圆角卡片；
  - 若**无** `isCard` 属性，样式引擎会检查其 `backgroundRole()`（默认通常为 `QPalette::Window`），**直接用实心画刷填充整个部件矩形**！
* **双刃剑效应与使用准则**：
  - **去底色时大忌**：用于页面排版、网格布局或滚动的中间层普通 QWidget，**严禁设置 `WA_StyledBackground(true)`**！必须确保其为 `false`，否则就会无意中触发 `PE_Widget` 刷出一大块实心底色，彻底破坏底层的圆角或 Mica 透视。
  - **要底色时神器**：如果希望某个普通的 QWidget 成为一块标准的纯色底板，只需设置其 `setBackgroundRole(...)` 或修改 Palette，然后开启 `setAttribute(Qt::WA_StyledBackground, true)`，FluentUI3Style 就会自动绘制出原生底色，**完全不需要写硬编码 QSS**！

#### 4. 界面灰块的头号元凶：`autoFillBackground(true)`
* **底层根因**：若控件的 `autoFillBackground` 为 `true`，Qt 在调用 `paintEvent` 前会强制使用 `widget->backgroundRole()`（通常是 `QPalette::Window`，在 Windows 浅色下是生硬的不透明死灰 `#F0F0F0`）粗暴擦除整个矩形。
* **灾难后果**：子容器一旦擦除，外层 `StyledPanel` 的洁白底色、6px 圆角裁剪、或者底层的 Mica 亚克力材质全被这一层实心灰块遮蔽。
* **Qt Designer 的暗坑**：在 Qt Designer 中拖入的每一个 `QScrollArea`，其内部的 `scrollAreaWidgetContents` 在编译生成的 `ui_xxx.h` 中**默认都被写死了 `autoFillBackground(true)`**！如果不显式覆盖，界面必出丑陋灰色方块。

---

### 2.2 权威编码范式

#### 规范 A：彻底“不要底色”（用于容器、滚动区与透明过渡部件）
如果一个部件只起布局、滚动或占位作用，必须**彻底剔除自身背景绘制**：

```cpp
// 1. 手写 QScrollArea 必备的“透明五件套”：
scrollArea->setFrameShape( QFrame::NoFrame );                  // 禁用自身的边框
scrollArea->setAutoFillBackground( false );                    // 禁用滚动区背景
scrollArea->viewport()->setAutoFillBackground( false );        // 核心：禁用视口画布擦除
scrollArea->viewport()->setAttribute( Qt::WA_StyledBackground, false ); // 禁用 QStyle 对视口绘底
contentWidget->setAutoFillBackground( false );                 // 核心：禁用内容承载部件擦除
contentWidget->setAttribute( Qt::WA_StyledBackground, false ); // 禁用内容部件自绘背景

// 2. Qt Designer UI 实例初始化必备（在 setupUi 后立即覆盖默认灰底）：
ui->scrollArea_2->viewport()->setAutoFillBackground( false );
ui->scrollAreaWidgetContents->setAutoFillBackground( false );
ui->scrollAreaWidgetContents_4->setAutoFillBackground( false );

// 3. 标题栏、菜单栏、工具栏透出窗口背景必备：
m_menuBar->setAttribute( Qt::WA_TranslucentBackground, true );
m_menuBar->setAttribute( Qt::WA_StyledBackground, false );
m_menuBar->setAutoFillBackground( false );
```

#### 规范 B：规范“要底色”（用于主面板、卡片容器与纯色底板）
需要赋予底色的四大标准场景：

1. **页面整页主底板（一级面板）**：
   ```cpp
   // 页面直接继承 QFrame，并在构造函数中设置
   setFrameShape( QFrame::StyledPanel ); // FluentUI3Style 自动绘制 QPalette::Base 与 6px 圆角
   ```
2. **功能分组卡片 (二级 Card)**：
   ```cpp
   auto* card = new QWidget( parent );
   card->setProperty( "isCard", true );
   card->setAttribute( Qt::WA_StyledBackground, true ); // 触发 PE_Widget 绘制 WinUI 3 卡片
   ```
3. **普通 QWidget 纯色背景底板（非卡片形态）**：
   ```cpp
   auto* solidWidget = new QWidget( parent );
   solidWidget->setBackgroundRole( QPalette::Base ); // 或 QPalette::Window / 自定义
   solidWidget->setAttribute( Qt::WA_StyledBackground, true ); // 触发 PE_Widget 绘制当前背景角色底色
   ```
4. **状态微交互与强调底色**：
   调用 [fluentui3colors.h](file:///d:/workspace/Code/Github/Window11Style/FluentUI3Colors/fluentui3colors.h) 的 `WINUI3Colors`，动态匹配浅色/深色主题：
   ```cpp
   #include "fluentui3colors.h"
   const bool isDark = palette().color( QPalette::Window ).lightness() < 128;
   const QColor hoverBg = WINUI3Colors[ isDark ? 1 : 0 ][ subtleHighlightColor ];
   const QColor pressBg = WINUI3Colors[ isDark ? 1 : 0 ][ subtlePressedColor ];
   ```

---

## 3. FluentUI3Style 动态属性与 `fluentui3styleproperties.h` 使用指南

FluentUI3Style 提供了高度灵活的动态属性机制。**极力推荐在代码中引入 `#include "fluentui3styleproperties.h"`**，优先使用头文件中定义的**属性常量与枚举**，彻底避免手写字面量字符串（Magic Strings）导致的拼写错误与重构困难。

### 3.1 核心属性常量与推荐编码范式

```cpp
#include "fluentui3styleproperties.h"

// 1. 强调色主按钮 (使用 ButtonAccentStyleProperty 代替字面量 "accent")
primaryBtn->setProperty( ButtonAccentStyleProperty, true );

// 2. 将复选框变为 WinUI 3 ToggleSwitch (使用 SwitchStyleProperty 代替 "isSwitchButton")
switchCheckBox->setProperty( SwitchStyleProperty, true );

// 3. 导航树高亮胶囊滑动条 (使用 NavigationViewStyleProperty)
navTreeWidget->setProperty( NavigationViewStyleProperty, true );

// 4. 进度条形态与环形 ProgressRing (使用常量与 ProgressBarStyle 枚举)
progressBar->setProperty( ProgressBarStyleProperty, ProgressBarThick ); // 粗条
// 圆环模式及参数：
ringBar->setProperty( ProgressBarStyleProperty, ProgressBarRing );
ringBar->setProperty( ProgressBarThicknessProperty, 8 ); // 环厚度 (默认 6px)
ringBar->setProperty( ProgressBarRingIndeterminateDurationProperty, 800 ); // 繁忙单圈旋转周期(ms)

// 5. 选项卡风格切换 (使用 TabBarStyleProperty 与 TabBarStyle 枚举)
tabBar->setProperty( TabBarStyleProperty, Segmented_WinUI3 ); 
// 可选枚举：Capsule(1), Pivot_Grow(2), Pivot_Slide(3), PillTabs(5), Navigation(7), Segmented_WinUI3(8)

// 6. 步进输入框加减按钮布局 (使用 SpinBoxButtonLayout 枚举)
spinBox->setProperty( "spinBoxButtonLayout", ArrowsHorizontalRight ); 
// 可选枚举：ArrowsHorizontalRight, ArrowsVertical, ArrowsHorizontalSides, PlusMinusHorizontalSides

// 7. 仪表盘与滑块 (使用常量与 DialStyle 枚举)
dial->setProperty( DialStyleProperty, DialDots ); // 或 DialRing(2), DialThumb(3)
dial->setProperty( DialDrawValueProperty, true ); // 在表盘中央绘制数值
slider->setProperty( SliderValueTipProperty, true ); // 开启滑块拖拽跟随浮动气泡

// 8. 按钮形态 (胶囊 / 正圆)
actionBtn->setProperty( ButtonShapeProperty, ButtonShapeCapsule ); // 胶囊型
iconBtn->setProperty( ButtonShapeProperty, ButtonShapeCircle );    // 正圆形

// 9. 强制纯直角 (禁用 QStyle 默认绘制的圆角)
widget->setProperty( NoRoundedCorners, true );

// 10. 下拉菜单与弹窗动画开关 (可作用于单个控件或全局 qApp)
qApp->setProperty( ComboBoxPopupDropDownAnimationEnabledProperty, true );
qApp->setProperty( MenuPopupAnimationEnabledProperty, true );
```

### 3.2 动态属性、头文件常量与枚举映射全字典

| 头文件常量 (`fluentui3styleproperties.h`) | 对应属性名 (`Property`) | 作用控件 | 推荐值 / 对应枚举 | 视觉与交互效果说明 |
| :--- | :--- | :--- | :--- | :--- |
| `ButtonAccentStyleProperty` | `"accent"` | `QPushButton` / `QToolButton` | `true` | 主强调色按钮（自动适配亮暗高对比度文本反色）。 |
| *(常规属性)* | `"isCard"` | `QWidget` | `true` | 赋予 4px 圆角、微描边与主题自适应背景底色的 WinUI 3 卡片。 |
| `SwitchStyleProperty` | `"isSwitchButton"` | `QCheckBox` | `true` | 将普通单选/复选框转为现代 ToggleSwitch 动画滑动开关。 |
| `NavigationViewStyleProperty` | `"navigationViewIndicator"` | `QTreeWidget` / `QTreeView` | `true` | 激活左侧导航树项选中时的竖向胶囊指示器与滑动过渡。 |
| `ProgressBarStyleProperty` | `"progressBarStyle"` | `QProgressBar` | `ProgressBarThin` (0)<br>`ProgressBarThick` (1)<br>`ProgressBarRing` (2) | 控制进度条形态为极细线、标准粗条或圆环进度环。 |
| `ProgressBarThicknessProperty` | `"progressBarThickness"` | `QProgressBar` | `int` (默认 6) | `ProgressBarRing` 圆环形态下的环形线条粗细（像素）。 |
| `ProgressBarRingIndeterminateDurationProperty` | `"progressBarRingIndeterminateDuration"` | `QProgressBar` | `int` (默认 800) | 圆环处于不确定状态（0~0）时旋转一圈的毫秒时长。 |
| `TabBarStyleProperty` | `"tabBarStyle"` | `QTabBar` | `TabBarStyle` 枚举：<br>`Capsule` (1)<br>`Pivot_Slide` (3)<br>`Segmented_WinUI3` (8) | 切换选项卡风格：药丸胶囊、滑动下划线枢轴、分段控件等。 |
| `ButtonShapeProperty` | `"buttonShape"` | `QPushButton` / `QToolButton` | `ButtonShapeCapsule` (`"capsule"`)<br>`ButtonShapeCircle` (`"circle"`) | 按钮轮廓形状：椭圆胶囊或正圆。 |
| `SliderValueTipProperty` | `"showValueTip"` | `QSlider` | `true` | 滑块拖拽时在手柄上方显示当前数值的浮动提示气泡。 |
| `DialStyleProperty` | `"dialStyle"` | `QDial` | `DialDots` (1)<br>`DialRing` (2)<br>`DialThumb` (3) | 仪表盘刻度样式：圆点、圆环或旋钮指针。 |
| `DialDrawValueProperty` | `"dialDrawValue"` | `QDial` | `true` / `false` | 是否在旋钮中央绘制当前数值文本。 |
| *(枚举类型)* | `"spinBoxButtonLayout"` | `QSpinBox` / `QDoubleSpinBox` | `ArrowsHorizontalRight`<br>`ArrowsHorizontalSides` 等 | 步进加减按钮排布：靠右并排或分布在输入框两侧。 |
| `NoRoundedCorners` | `"noRoundedCorners"` | `QWidget` / `QFrame` | `true` | 禁用圆角绘制，恢复为纯矩形直角。 |
| `ComboBoxPopupDropDownAnimationEnabledProperty` | `"comboBoxPopupDropDownAnimationEnabled"` | `qApp` 或 `QComboBox` | `true` / `false` | 控制下拉框弹出时的淡入下滑动效。 |
| `MenuPopupAnimationEnabledProperty` | `"menuPopupAnimationEnabled"` | `qApp` 或 `QMenu` | `true` / `false` | 控制右键/下拉菜单弹出时的 WinUI 3 过渡动效。 |

---

## 4. ExWidgets 核心控件实战调用指南 (ExWidgets Components)

### 4.1 `ExStackedWidget` 平滑动画切页
```cpp
#include "exstackedwidget.h"

auto* stack = new ExStackedWidget( this );
stack->setVerticalMode( true );                     // 垂直滑动翻页 (符合 WinUI 3 规范)
stack->setAnimation( QEasingCurve::Type::InOutSine ); // 平滑过渡曲线
stack->setSpeed( 300 );                             // 300ms 切换时长
stack->setCurrentIndex( 1 );
```

### 4.2 `ExExpander` 折叠面板
```cpp
#include "exexpander.h"

auto* expander = new ExExpander( parent );
expander->setHeaderText( tr( "高级设置" ) );
expander->setContentWidget( mySettingsWidget );
expander->setExpanded( false ); // 默认折叠
```

### 4.3 `ExCarousel` 走马灯轮播图
```cpp
#include "excarousel.h"

auto* carousel = new ExCarousel( parent );
carousel->addImage( QPixmap( ":/images/banner1.png" ) );
carousel->addImage( QPixmap( ":/images/banner2.png" ) );
carousel->setAutoplay( true );
carousel->setInterval( 4000 ); // 4秒自动轮播
carousel->setIndicatorPosition( ExCarousel::IndicatorPosition::BottomCenter );
```

### 4.4 `ExBreadcrumbBar` 面包屑导航
```cpp
#include "exbreadcrumbbar.h"

auto* breadcrumb = new ExBreadcrumbBar( parent );
breadcrumb->setItemsSource( { tr( "主页" ), tr( "控件库" ), tr( "按钮" ) } );
connect( breadcrumb, &ExBreadcrumbBar::itemClicked, this, []( int index, const QString& text ) {
    // 处理导航跳转
} );
```

### 4.5 `ExColorPickerButton` 现代拾色器
```cpp
#include "excolorpickerbutton.h"

auto* colorBtn = new ExColorPickerButton( parent );
colorBtn->setColor( QColor( "#0078D4" ) );
connect( colorBtn, &ExColorPickerButton::colorChanged, this, []( const QColor& newCol ) {
    // 颜色变化处理
} );
```

### 4.6 `ExFontIcon` 矢量字体图标调用规范
切勿手写 Unicode 十六进制字符串，统一使用 `ExFontIcon` 与 `SegoeIcon::Type`：

```cpp
#include "exfonticon.h"

// 方式 1：标准 QIcon 获取 (适用于 TableView、ListView 等需要 QIcon 的接口)
QIcon folderIcon = EXFONTICON->getIcon( SegoeIcon::Folder, /*pixelSize=*/20 );

// 方式 2 (强烈推荐)：小按钮与文本标签的超清晰亚像素矢量绘制 (高分屏永不发虚模糊)
auto* closeBtn = new QToolButton( parent );
closeBtn->setToolButtonStyle( Qt::ToolButtonTextOnly );
closeBtn->setFont( ExFontIcon::iconFont( 12 ) );
closeBtn->setText( ExFontIcon::iconString( SegoeIcon::ChromeClose ) );
```

---

## 5. Gallery 经典实战高级技巧 (Advanced Recipes from Gallery)

以下技巧提炼自 Gallery 生产级实战代码，能极大提升界面的现代感、交互细节与启动性能：

### 5.1 设置项卡片 (SettingsCard) 经典构建模式
WinUI 3 风格最具代表性的布局是“左侧图标 + 中间主副标题 + 右侧操作控件”的横向卡片：

```cpp
auto makeSettingsCard = []( int icon, const QString& title, const QString& description, 
                            QWidget* trailingControl, QWidget* parent ) -> QWidget*
{
    auto* card = new QWidget( parent );
    card->setAttribute( Qt::WA_StyledBackground, true );
    card->setProperty( "isCard", true );
    card->setMinimumHeight( 70 );
    card->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    auto* layout = new QHBoxLayout( card );
    layout->setContentsMargins( 18, 12, 18, 12 );
    layout->setSpacing( 16 );

    // 1. 左侧 36px 居中大图标 (22px SegoeFluentIcon)
    auto* iconLabel = new QLabel( card );
    iconLabel->setFont( ExFontIcon::iconFont( 22 ) );
    iconLabel->setText( ExFontIcon::iconString( static_cast<SegoeIcon::Type>( icon ) ) );
    iconLabel->setFixedWidth( 36 );
    iconLabel->setAlignment( Qt::AlignCenter );
    layout->addWidget( iconLabel );

    // 2. 中间垂直文本列 (主标题 + 弱化副标题)
    auto* textWidget = new QWidget( card );
    auto* textLayout = new QVBoxLayout( textWidget );
    textLayout->setContentsMargins( 0, 0, 0, 0 );
    textLayout->setSpacing( 2 );

    auto* titleLabel = new QLabel( title, textWidget );
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize( 15 );
    titleLabel->setFont( titleFont );
    textLayout->addWidget( titleLabel );

    if ( !description.isEmpty() ) {
        auto* descLabel = new QLabel( description, textWidget );
        QPalette p = descLabel->palette();
        p.setColor( QPalette::WindowText, p.color( QPalette::Mid ) ); // 弱化文本颜色
        descLabel->setPalette( p );
        descLabel->setWordWrap( true );
        textLayout->addWidget( descLabel );
    }
    layout->addWidget( textWidget, 1 );

    // 3. 右侧操作控件 (ToggleSwitch、ComboBox、Button 等)
    if ( trailingControl ) {
        layout->addWidget( trailingControl, 0, Qt::AlignVCenter );
    }
    return card;
};
```

### 5.2 QSlider 浮动气泡的高级缩放与小数精度 (`scale` 与 `precision`)
FluentUI3Style 原生支持在滑块气泡中将整数刻度自动格式化为带小数的物理值：

```cpp
#include "fluentui3styleproperties.h"

auto* slider = new QSlider( Qt::Horizontal, parent );
slider->setRange( 0, 100 ); // 原生整数范围 0 ~ 100
slider->setValue( 25 );

// 核心技巧：设置 scale 和 precision
slider->setProperty( SliderValueTipProperty, true ); // 开启拖拽气泡
slider->setProperty( "scale", 100.0 );               // 数值缩小 100 倍
slider->setProperty( "precision", 2 );               // 保留 2 位小数
// 效果：用户拖拽时，滑块上方气泡自动显示 "0.25"！
```

### 5.3 树形与列表控件行高控制 (`ItemHeight`)
无需手写复杂的 `QStyledItemDelegate`，只需赋予动态属性：

```cpp
// 宽敞舒适的 WinUI 3 树形列表
treeWidget->setProperty( "ItemHeight", 34 ); // FluentUI3Style 自动将项高度设为 34px
treeWidget->setIndentation( 20 );            // 子项层级缩进
```

### 5.4 单个控件专属 Accent 强调色定制 (`changeAccentPalette`)
如果不想改变全局强调色，而仅希望某些特定控件呈现特定颜色（如“成功状态”的绿色 CheckBox、“危险/警告”的橙色 RadioButton）：

```cpp
void setWidgetAccentColor( QWidget* widget, const QColor& color )
{
    auto p = widget->palette();
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    p.setColor( QPalette::Accent, color );
#else
    p.setColor( QPalette::Highlight, color );
#endif
    widget->setPalette( p );
}

// 使用：
setWidgetAccentColor( ui->checkBoxSuccess, QColor( "#4CAF50" ) ); // 纯绿
setWidgetAccentColor( ui->radioButtonWarn, QColor( "#FF8F00" ) ); // 亮橙
```

### 5.5 现代搜索输入框标准范式 (内嵌图标与一键清除)
```cpp
auto* searchEdit = new QLineEdit( parent );
searchEdit->setPlaceholderText( tr( "搜索..." ) );
searchEdit->setClearButtonEnabled( true ); // 右侧一键清除小叉号

// 利用 QLineEdit::addAction 将 Segoe 矢量放大镜图标无缝内嵌在末尾
searchEdit->addAction( EXFONTICON->getIcon( SegoeIcon::Search, 16 ), QLineEdit::TrailingPosition );
```

### 5.6 应用级全局主题与壁纸模式控制 (`qApp` 动态属性)
FluentUI3Style 响应 `qApp` 上的特殊内部属性，用于统领全局状态：

```cpp
// 1. 全局切换浅色 / 深色模式：
qApp->setProperty( "_q_colorscheme", 0 ); // 0 = Light 浅色, 1 = Dark 深色

// 2. 全局切换壁纸 / 亚克力半透明模式 (Wallpaper Mode)：
qApp->setProperty( "_q_widget_mode", 1 ); // 1 = 启用卡片与半透明材质融合，0 = 普通实心模式

// 3. 全局动态更新品牌强调色：
qApp->setProperty( "_q_accent_color", QColor( "#0078D4" ) );
```

### 5.7 复杂页面的冷启动秒开优化 (懒加载)
Gallery 拥有数十个子页面与重型表格，冷启动秒开的核心秘诀是**不要在主窗口构造期同步 new 出所有子页**：

```cpp
// 仅为当前激活页初始化；对于耗时页面，在用户首次切换到该页时再进行数据加载：
connect( ui->stackedWidget, &QStackedWidget::currentChanged, this, [ this ]( int index ) {
    if ( index == TABLE_PAGE_INDEX && !m_tableLoaded ) {
        m_tableLoaded = true;
        initHeavyTableData(); // 首次切页时延迟加载
    }
} );
```

### 5.8 引导提示组件 `ExTour` 实战
为复杂界面提供步骤式新手向导：

```cpp
#include "extour.h"

auto* tour = new ExTour( this );
QList<ExTourStep> steps;

ExTourStep step1;
step1.target = themeButton;
step1.title = tr( "切换深浅模式" );
step1.description = tr( "点击此处在浅色和深色主题之间自由切换。" );
step1.placement = ExTourStep::Bottom;
steps.append( step1 );

tour->setSteps( steps );
tour->start( 0 ); // 从第 0 步开始播放
```

---

## 6. 标准 Page 页面编写脚手架模板 (Page Scaffold)

当为应用新增一个功能展示或设置页面时，请直接复制套用以下模板：

### 6.1 头文件 (`pagedemo.h`)
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

### 6.2 源文件 (`pagedemo.cpp`)
```cpp
#include "pagedemo.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include "exfonticon.h"

namespace
{
// 辅助函数：创建标准的 WinUI 3 卡片容器
QWidget* makeCard( QWidget* parent )
{
    auto* card = new QWidget( parent );
    card->setProperty( "isCard", true );
    card->setAttribute( Qt::WA_StyledBackground, true );
    return card;
}

// 辅助函数：创建卡片组标题
QLabel* makeSectionTitle( const QString& text, QWidget* parent )
{
    auto* label = new QLabel( text, parent );
    QFont font = label->font();
    font.setBold( true );
    font.setPixelSize( 14 );
    label->setFont( font );
    return label;
}
} // namespace

PageDemo::PageDemo( QWidget* parent )
    : QFrame( parent )
{
    // 1. 设置为 StyledPanel：获得 FluentUI3Style 的主面板底色与 6px 优雅圆角
    setFrameShape( QFrame::StyledPanel );

    auto* rootLayout = new QVBoxLayout( this );
    rootLayout->setContentsMargins( 0, 0, 0, 0 );

    // 2. 嵌入滚动视口，彻底剥离背景擦除与额外边框
    auto* scrollArea = new QScrollArea( this );
    scrollArea->setWidgetResizable( true );
    scrollArea->setFrameShape( QFrame::NoFrame );
    scrollArea->setAutoFillBackground( false );
    scrollArea->viewport()->setAutoFillBackground( false );
    scrollArea->viewport()->setAttribute( Qt::WA_StyledBackground, false );
    rootLayout->addWidget( scrollArea );

    // 3. 嵌入内容承载部件，禁用自动背景擦除
    auto* content = new QWidget( scrollArea );
    content->setAutoFillBackground( false );
    content->setAttribute( Qt::WA_StyledBackground, false );
    scrollArea->setWidget( content );

    auto* mainLayout = new QVBoxLayout( content );
    mainLayout->setContentsMargins( 24, 24, 24, 24 );
    mainLayout->setSpacing( 16 );

    // 4. 页面大标题与描述
    auto* title = new QLabel( tr( "功能模块标题" ), content );
    QFont titleFont = title->font();
    titleFont.setPointSize( 16 );
    titleFont.setBold( true );
    title->setFont( titleFont );
    mainLayout->addWidget( title );

    auto* desc = new QLabel( tr( "这里是对该功能页面的详细说明与操作指引。" ), content );
    desc->setWordWrap( true );
    mainLayout->addWidget( desc );

    // 5. 添加交互卡片 (Card)
    auto* card = makeCard( content );
    auto* cardLayout = new QVBoxLayout( card );
    cardLayout->setContentsMargins( 16, 16, 16, 16 );
    cardLayout->setSpacing( 12 );

    cardLayout->addWidget( makeSectionTitle( tr( "操作选项" ), card ) );

    auto* btnRow = new QHBoxLayout;
    auto* primaryBtn = new QPushButton( tr( "主要操作 (Accent)" ), card );
    primaryBtn->setProperty( "accent", true ); // 强调按钮

    auto* standardBtn = new QPushButton( tr( "标准按钮" ), card );

    btnRow->addWidget( primaryBtn );
    btnRow->addWidget( standardBtn );
    btnRow->addStretch();
    cardLayout->addLayout( btnRow );

    mainLayout->addWidget( card );
    mainLayout->addStretch( 1 ); // 底部弹性支撑
}
```

---

## 7. 避坑清单 (Checklist)

- [ ] **杜绝手写覆盖背景的 QSS**：严禁写 `setStyleSheet("background-color: white; border: 1px solid gray;")`。这会直接破坏 FluentUI3Style 的亚克力、暗黑主题切换和悬停动画。
- [ ] **QScrollArea 必去底色**：凡是用到 `QScrollArea`，请立即检查是否设置了 `viewport()->setAutoFillBackground(false)` 与 `content->setAutoFillBackground(false)`。
- [ ] **Designer 自动生成的 UI 必加覆盖逻辑**：只要界面是从 `.ui` 文件加载的，务必在构造函数中加上 `ui->scrollAreaWidgetContents->setAutoFillBackground(false);`。
- [ ] **普通 QWidget 的 WA_StyledBackground 双刃剑**：普通 QWidget 默认透明；一旦开启该属性且没有 `isCard`，样式引擎会直接触发 `PE_Widget` 用实心底色填充整个矩形！中间布局与占位容器切勿随意开启；若确需纯色底板则可配合 `setBackgroundRole` 规范使用。
- [ ] **跨版本 Accent 色兼容**：如需手动提取强调色，注意 Qt 6.6+ 使用 `QPalette::Accent`，Qt 5 及旧版 Qt 6 使用 `QPalette::Highlight`：
  ```cpp
  #if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
      const QColor accent = palette().color( QPalette::Accent );
  #else
      const QColor accent = palette().color( QPalette::Highlight );
  #endif
  ```
- [ ] **小图标避免 QIcon 模糊**：对于尺寸小于 16px 的细线图标，优先使用 `setFont(ExFontIcon::iconFont(12))` + `setText(ExFontIcon::iconString(icon))`，字体引擎矢量渲染效果远胜位图缩放。
