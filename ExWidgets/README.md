# ExWidgets 使用手册

**English:** [README_EN.md](README_EN.md)

ExWidgets 是本仓库在 **FluentUI3Style 样式库之外** 提供的一组 WinUI3 / CommunityToolkit 风格 **Qt Widgets 扩展控件**。  
它们可以独立编译链接，但 **推荐与 `app.setStyle("FluentUI3")` 一起使用**，以获得一致的 Fluent 视觉与交互。

Gallery 程序中 **ExWidgets** 导航分组下有各控件的交互演示，可作为活文档参考。

---

## 接入方式

### CMake

```cmake
add_subdirectory(path/to/Window11Style/ExWidgets)  # 或通过本仓库顶层工程

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE ExWidgets::ExWidgets Qt6::Widgets)
```

安装后可使用 `find_package(ExWidgets CONFIG REQUIRED)` 和同名 target。

无边框窗口（`FluentTitleBar` / `FluentWindowFrame`）是独立静态库，安装后：

```cmake
find_package(Frameless CONFIG REQUIRED)
target_link_libraries(MyApp PRIVATE Frameless::Frameless)
```

不通过 `ExWidgets` 链接，也不链接 `FluentUI3Style`。

头文件示例：

```cpp
#include "exspectrumwidget.h"
#include "exrangeslider.h"
```

### qmake

```qmake
include($$PWD/ExWidgets/ExWidgets.pri)   # 若项目提供；或参考 examples/Gallery/Gallery.pro
LIBS += -lExWidgets
```

### 运行依赖

- Qt **5.14+** / **6.x**（Widgets）
- 链接 `ExWidgets` 共享库（或静态链接）
- 样式：建议启用 FluentUI3 插件，否则部分控件仅呈现默认 Fusion/Windows 绘制

---

## 控件一览

| 控件 | 头文件 | 说明 | Gallery 演示 |
|------|--------|------|--------------|
| `ExRangeSlider` | `exrangeslider.h` | WinUI3 双端范围滑块 | ExWidgets → ExRangeSlider |
| `ExBorderBeam` / `ExBorderBeamButton` | `exborderbeam.h` / `exborderbeambutton.h` | 支持 Light/Dark 配置的动画渐变边框容器和按钮 | ExWidgets → ExBorderBeam |
| `ExAudioLevelMeter` | `exaudiolevelmeter.h` | 支持单声道/立体声、dBFS 刻度、峰值保持和衰减的音频电平表 | ExWidgets → ExAudioLevelMeter |
| `ExColorPicker` | `excolorpicker.h` | CommunityToolkit 取色器 | ExWidgets → ExColorPicker |
| `ExColorPickerButton` | `excolorpickerbutton.h` | 带 Flyout 的取色按钮 | ExColorPicker 页内 |
| `ExColorPickerDialog` | `excolorpickerdialog.h` | 带标题及底部等宽大按钮的颜色对话框 | ExColorPicker 页内 |
| `ExMessageBox` | `exmessagebox.h` | Fluent 风格 `QMessageBox` | Dialogs 页 |
| `ExContentDialog` | `excontentdialog.h` | WinUI3 ContentDialog | Dialogs 页 |
| `ExInfoBar` | `exinfobar.h` | 页面内非阻塞通知，支持四种严重级别、操作与关闭动画 | ExWidgets → ExInfoBar / ExExpander |
| `ExInfoBarHost` | `exinfobarhost.h` | 窗口级 InfoBar 弹出、六向定位、多条堆叠与超时管理 | ExWidgets → ExInfoBar / ExExpander |
| `ExExpander` | `exexpander.h` | 可向上或向下展开的 Header/Content 折叠容器 | ExWidgets → ExInfoBar / ExExpander |
| `ExCarousel` | `excarousel.h` | 水平轮播图，支持自动播放、循环、指示点和左右切换 | ExWidgets → ExCarousel |
| `ExBreadcrumbBar` | `exbreadcrumbbar.h` | 路径导航、前缀溢出菜单、文本省略、键盘与 RTL 支持 | ExWidgets → ExBreadcrumbBar / ExTeachingTip |
| `ExTeachingTip` | `exteachingtip.h` | 窗口内非模态引导气泡，支持目标跟随、自动换边、操作按钮与外部关闭 | ExWidgets → ExBreadcrumbBar / ExTeachingTip |
| `ExTimerDial` | `extimerdial.h` | 剩余时间、环形进度与预计完成时刻 | Win11Clock 计时器 |
| `ExTimeline` / `ExTimelineEvent` | `extimeline.h` | 事件时间轴，支持水平/垂直、单侧/交错布局、状态节点和动画 | ExWidgets → ExTimeline |
| `ExLiquidGauge` | `exliquidgauge.h` | Ant Design 风格水波图，支持四种形状、双层波浪和中心文本 | ExWidgets → ExLiquidGauge |
| `ExMultiProgressRing` / `ExMultiProgressRingItem` | `exmultiprogressring.h` | 轻量多环进度图，支持独立数值、颜色、中央详情和动画 | ExWidgets → ExRadialGauge |
| `ExMultiRadialGauge` / `ExMultiRadialGaugeItem` | `exmultiradialgauge.h` | 多数据项径向仪表盘，共享刻度并支持重叠进度、多指针、标题详情和动画 | ExWidgets → ExRadialGauge |
| `ExProgressRing` | `exprogressring.h` | 基于 QProgressBar Ring 的单环进度控件，支持标题、独立文本样式和自定义中心控件 | ExWidgets → ProgressRing |
| `ExRadialGauge` / `ExRadialGaugeRange` | `exradialgauge.h` | 可交互的径向仪表盘，支持自定义角度、刻度、指针和彩色区间 | ExWidgets → ExRadialGauge |
| `ExSpectrumWidget` | `exspectrumwidget.h` | 实时音频频谱（Push PCM） | Qt5: ExSpectrumWidget；Qt6: Audiomatic Mini |
| `ExWinUINavigationView` | `exwinuinavigationview.h` | 主导航 + 页脚导航 | MainWindow 左侧导航 |
| `ExNavTreeWidget` | `exnavtreewidget.h` | 可折叠导航树 | 由 NavigationView 内部使用 |
| `ExStackedWidget` | `exstackedwidget.h` | 带动画的 `QStackedWidget` | MainWindow 中央区域 |
| `ExTabWidget` | `extabwidget.h` | 带动画的 `QTabWidget` | Tab 演示 |
| `ColorGradientSlider` | `colorgradientslider.h` | 渐变轨道滑条（ExColorPicker 内部） | ExColorPicker 内部 |

---

## ExCarousel

水平轮播。支持 `addSlide()` 放入任意 `QWidget`，支持 `addImage()` / `addPixmap()` 放入高清图片并搭配标题与副标题（自动渲染底部半透明渐变遮罩）。支持 WinUI 3 风格强减速丝滑滑动、左右键、滚轮、胶囊指示点以及悬停淡入淡出切换按钮。

```cpp
#include "excarousel.h"

auto *carousel = new ExCarousel(this);
carousel->addImage(":/images/carousel_1.jpg", tr("标题"), tr("副标题"));
carousel->addSlide(customWidget);
carousel->setAutoPlay(true);
carousel->setInterval(3500);
carousel->setWrap(true);
carousel->setNavigationButtonTrigger(ExCarousel::OnHover); // 切换按钮鼠标移入时淡入
carousel->setBorderRadius(8.0);                           // 视口圆角裁剪
connect(carousel, &ExCarousel::currentIndexChanged, this, [](int index) { ... });
```

悬停默认暂停自动播放。`setAnimationDuration(0)` 可关闭滑动动画。

---

## ExAudioLevelMeter

`ExAudioLevelMeter` 是只读电平表，不用于调节音量。可直接提交 dBFS，也可提交范围为 `0.0 ~ 1.0` 的线性幅度：

```cpp
#include "exaudiolevelmeter.h"

auto *meter = new ExAudioLevelMeter(this);
meter->setRange(-60.0, 0.0);

// 单声道只需持续提交当前电平
meter->setLevel(currentPeakDb);

// 立体声
meter->setScalePosition(ExAudioLevelMeter::CenterScale);
meter->setStereoLevels(leftPeakDb, rightPeakDb);
// 或：meter->setLinearLevels({leftPeak, rightPeak});
```

刻度可按间隔、固定数量或自定义数值生成：

```cpp
meter->setScaleMode(ExAudioLevelMeter::CustomScale);
meter->setCustomScaleValues({0, -3, -6, -12, -24, -48, -60});
meter->setScaleUnit("dBFS");
meter->setScaleUnitVisible(true);
meter->setScalePrecision(0);
```

`setLevel()` / `setLevels()` / `setLinearLevels()` 支持从普通工作线程调用，控件会把更新投递到 GUI 线程。多声道数据会自动调整 `channelCount`（最多 8 声道）。如果数据来自不允许分配内存的硬实时音频回调，应先写入原子/无锁状态，再由工作线程提交给控件。

---

## ExRangeSlider

WinUI3 风格 **范围滑块**（lower / upper 双端），API 接近 `QAbstractSlider`。

### 基本用法

```cpp
#include "exrangeslider.h"

auto *slider = new ExRangeSlider(Qt::Horizontal, this);
slider->setRange(0, 100);
slider->setValues(20, 80);
slider->setTracking(true);          // 拖动时实时发信号
slider->setTickPosition(true);
slider->setTickInterval(10);

connect(slider, &ExRangeSlider::lowerValueChanged, this, [](int v) { /* ... */ });
connect(slider, &ExRangeSlider::upperValueChanged, this, [](int v) { /* ... */ });
```

### 常用属性

| 属性 / API | 说明 |
|------------|------|
| `minimum` / `maximum` / `setRange` | 数值范围 |
| `lowerValue` / `upperValue` / `setValues` | 当前区间 |
| `snapMode` | `NoSnap` / `SnapAlways` / `SnapOnRelease` |
| `tickPosition` / `tickInterval` | 刻度线 |
| `tracking` | 拖动过程是否持续触发 value 变化 |
| `first()` / `second()` | 对应 QML 的 first/second 节点对象 |

---

## ExProgressRing

`ExProgressRing` 继承 `QProgressBar`，继续使用 FluentUI3Style 的 `ProgressBarRing` 绘制环和 Track。继承的 `format`、`textVisible`、范围和数值 API 保持不变，并增加标题、两套文本样式以及可选的自定义中心控件。

```cpp
#include "exprogressring.h"

auto *ring = new ExProgressRing(this);
ring->setRange(0, 100);
ring->setValue(65);
ring->setTitle(tr("已完成"));
ring->setFormat(QStringLiteral("%p%"));

// 需要任意中心内容时，控件会接管 customWidget 的所有权。
ring->setCenterWidget(customWidget);
```

设置 `centerWidget` 后不再绘制默认标题和数值；`takeCenterWidget()` 可解除并取回其所有权。`textVisible` 同时控制默认文字或自定义中心控件的可见性。

---

## ExLiquidGauge

`ExLiquidGauge` 继承 `QProgressBar`，直接复用范围、数值、格式和文本可见性 API。水位由当前进度决定，双层水波仅负责动态表现。

```cpp
#include "exliquidgauge.h"

auto *gauge = new ExLiquidGauge(this);
gauge->setRange(0, 100);
gauge->setValue(68);
gauge->setShape(ExLiquidGauge::CircleShape);
gauge->setWaveAmplitude(6.0);
gauge->setWaveCount(3);
gauge->setWaveAnimationDuration(2400);
```

`shape` 支持 `CircleShape`、`RectShape`、`PinShape` 和 `TriangleShape`。`waveColor`、`backgroundColor`、`outlineColor`、`textColor` 与 `submergedTextColor` 传入无效 `QColor` 时会自动读取调色板，主题切换后无需重新设置。控件隐藏、禁用或关闭 `animationEnabled` 时会停止动画计时器。

---

## ExMultiRadialGauge

`ExMultiRadialGauge` 面向 ECharts Multi Title Gauge 一类多数据仪表盘：控件只绘制一套范围、刻度、Track 和公共轴心，每个 `ExMultiRadialGaugeItem` 分别提供名称、数值、颜色、可见性以及标题/详情偏移，避免叠放多个完整 Gauge。

```cpp
#include "exmultiradialgauge.h"

auto *gauge = new ExMultiRadialGauge(this);
gauge->setRange(0.0, 100.0);
gauge->setProgressOverlap(true);
gauge->setNeedleStyle(ExMultiRadialGauge::LineNeedle);

auto *good = gauge->addItem(tr("Good"), 20.0, QColor("#5470C6"));
auto *better = gauge->addItem(tr("Better"), 40.0, QColor("#B8DE29"));
auto *perfect = gauge->addItem(tr("Perfect"), 60.0, QColor("#555672"));

good->setTitleOffset(QPointF(-0.4, 0.8));
good->setDetailOffset(QPointF(-0.4, 0.95));
better->setTitleOffset(QPointF(0.0, 0.8));
better->setDetailOffset(QPointF(0.0, 0.95));
perfect->setTitleOffset(QPointF(0.4, 0.8));
perfect->setDetailOffset(QPointF(0.4, 0.95));
```

偏移以刻度环半径为单位，`(-0.4, 0.8)` 对应 ECharts 的 `[-40%, 80%]`。`progressOverlap=true` 时，较小数值覆盖在较大数值之上，形成连续分色进度；关闭后各数据项改为同心弧。控件持有通过 `addItem()` 加入的数据项，`removeItem()` 与 `clearItems()` 会延迟删除它们。

---

## ExTimeline

`ExTimeline` 基于 `QListView` 的 Model/View 与自定义 Delegate，只创建和绘制可见行，适合订单流程、发布记录和系统事件。时间轴负责布局和公共视觉属性，每个 `ExTimelineEvent` 负责时间、标题、描述、状态、颜色、图标，以及交错布局下可选的左右位置。

```cpp
#include "extimeline.h"

auto *timeline = new ExTimeline(this);
timeline->setOrientation(Qt::Horizontal);
timeline->setLayoutMode(ExTimeline::ContentOnRight);
timeline->setHorizontalItemWidth(220);
timeline->setTimestampFormat(QStringLiteral("HH:mm"));

timeline->addEvent(QDateTime::currentDateTime(),
                   tr("任务已创建"),
                   tr("任务已经加入处理队列。"),
                   ExTimelineEvent::Completed);

auto *current = timeline->addEvent(QDateTime::currentDateTime(),
                                   tr("正在处理"),
                                   tr("正在生成结果。"),
                                   ExTimelineEvent::Current);
current->setIcon(SegoeIcon::Sync);
```

`orientation` 可切换水平/垂直时间轴。水平模式下，左侧、右侧和交错布局分别映射为上方、下方和上下交错，`horizontalItemWidth` 控制节点间距。`reverse` 只改变显示顺序，不改变 `events()` 的存储顺序。`timeText` 非空时覆盖 `timestamp` 的格式化结果，适合显示“刚刚”“昨天”等相对时间。`Current` 节点可播放呼吸动画，隐藏或禁用控件时动画会自动停止。

控件接管通过 `addEvent()` 加入的事件对象；`removeEvent()` / `clearEvents()` 会延迟删除，`takeEvent()` 可在不删除的情况下转移事件所有权。

---

## ExRadialGauge

`ExRadialGauge` 继承 `QDial`，因此可直接使用 `setRange()`、`setValue()`、`valueChanged()`、键盘、滚轮和无障碍能力；它只接管径向仪表盘的绘制和鼠标角度映射。

```cpp
#include "exradialgauge.h"

auto *gauge = new ExRadialGauge(this);
gauge->setRange(0, 240);
gauge->setValue(210);
gauge->setValueAnimationDuration(180);
gauge->setMinimumAngle(-135.0);
gauge->setMaximumAngle(135.0);
gauge->setMajorTickCount(11);
gauge->setMinorTickCount(4);
gauge->setTickLength(4.0);
gauge->setMajorTickLength(8.0);
gauge->setLabelsVisible(true);
gauge->setNeedleStyle(ExRadialGauge::TriangleNeedle);
```

角度以正上方为 `0°`、顺时针为正；起止角度相同表示完整的 `360°`。`majorTickCount` 是整段圆弧上的主刻度总数（包含首尾），`minorTickCount` 是每两个主刻度之间的次刻度数量，数字标签与主刻度一一对应。`needleLength` 是相对刻度环半径的比例，其余宽度、长度和边距属性使用逻辑像素。强调色和 Track 色分别读取 `QPalette::Accent`（Qt 6.6 以前使用 `Highlight`）与 `QPalette::Mid`。

`scaleMode` 提供三种通用结构：`TrackScale` 只绘制 Track，`ProgressScale` 绘制当前数值进度，`RangeScale` 使用彩色区间。彩色区间通过独立对象配置：

```cpp
gauge->setScaleMode(ExRadialGauge::RangeScale);
gauge->addRange(0, 60, QColor("#21BCE2"));
gauge->addRange(60, 80, QColor("#FFB900"));
gauge->addRange(80, 100, QColor("#FF6475"));
gauge->setUnit(QStringLiteral("km/h"));
```

`setValue()` 会在 `valueAnimationDuration` 内逐帧改变真实 `value()`，因此 `valueChanged`、进度环、指针和数值文本同步变化；时长为 `0` 时关闭动画。Track 与前景环的端点分别由 `trackCapStyle`、`ringCapStyle` 控制；`ProgressScale` 默认使用 `Qt::RoundCap`，其他模式默认使用 `Qt::FlatCap`，切换模式后仍可单独修改。非完整圆的整体起止端各向外延伸 `1°`，让首尾主刻度完整落在圆弧范围内。彩色区间如果需要严格对齐，建议保持 `ringCapStyle` 为 `Qt::FlatCap`。`needleStyle` 可选 `NoNeedle`、`LineNeedle` 和 `TriangleNeedle`，两种可见指针都支持圆形轴心；数值可通过 `valuePosition` 放在中心或底部，并可配合 `title`、`unit`、刻度标签与自定义颜色组成不同仪表外观。

---

## ExColorPicker / ExColorPickerButton / ExColorPickerDialog

对齐 **WinUI3 CommunityToolkit ColorPicker**：Spectrum / Palette / Sliders 三页，支持 Alpha、Hex、RGB/HSV。

### ExColorPicker（内嵌）

```cpp
#include "excolorpicker.h"

auto *picker = new ExColorPicker(parent);
picker->setColor(Qt::blue);
picker->setAlphaEnabled(true);
layout->addWidget(picker);
```

内嵌到页面时构造 `ExColorPicker(parent, /*popup=*/false)`，无需 `exec()`，监听 `colorChanged` 即可。

### ExColorPickerDialog

标准带边框 `QDialog`：系统标题栏显示 `title()`，内容区顶部标题，底部独立操作区：左侧主题色「确定」、右侧「取消」，两个按钮等宽填满可用宽度，最小高度 40px。支持浅色和深色主题。无遮罩层、无无边框窗口。

```cpp
#include "excolorpickerdialog.h"
#include "excolorpicker.h"

ExColorPickerDialog dialog(this);
dialog.setTitle(tr("编辑颜色"));
dialog.setColor(Qt::blue);
dialog.colorPicker()->setAlphaEnabled(true);
if (dialog.exec() == QDialog::Accepted) {
    const QColor selected = dialog.color();
}
```

也可以用 `open()` 非阻塞打开，监听 `colorSelected` 提交结果；`colorChanged` 用于实时预览。取消、Esc 或关闭窗口会恢复本次打开前的颜色，不发出 `colorSelected`。通过 `colorPicker()` 配置色域形状、Alpha 和自定义色板等选项。Gallery 的 ExColorPicker 页顶部提供弹窗演示。

### ExColorPickerButton

```cpp
#include "excolorpickerbutton.h"

auto *btn = new ExColorPickerButton(this);
btn->setSelectedColor(QColor(0, 120, 212));
connect(btn, &ExColorPickerButton::selectedColorChanged, this, [](const QColor &c) {
    // 用户从 Flyout 选色完成
});
```

---

## ExBreadcrumbBar

按 WinUI3 的 BreadcrumbBar/BreadcrumbLayout 实现核心交互：从末项向前保留可见后缀，前面的路径折叠到省略号菜单，菜单按“最近的上级在前”逆序排列。行内背景透明，默认 14 像素、普通字重；末项不加粗，也没有普通按钮的悬停底色。支持 Enter、Space、方向键、Home、End、Tab 和 RTL。

`ItemsSource` 对应 Qt 的 `QVariantList`；`ItemTemplate` 对应 `QAbstractItemDelegate*`；点击信号直接提供 WinUI 事件中的 Index 和 Item，不另建事件参数对象。

```cpp
#include "exbreadcrumbbar.h"

auto* breadcrumb = new ExBreadcrumbBar(this);
breadcrumb->setItemsSource({tr("主页"), tr("设备"), tr("音频设置")});
connect(breadcrumb, &ExBreadcrumbBar::itemClicked, this,
        [breadcrumb](int index, const QVariant& item) {
    // 包括溢出菜单，index 始终是完整 ItemsSource 的索引。
    // item 是原始 QVariant，不会强制转换成字符串。
    Q_UNUSED(item);
    breadcrumb->setItemsSource(breadcrumb->itemsSource().mid(0, index + 1));
});
```

控件只发送点击信号，**不会自动修改路径或执行导航**。末项过长时使用 Qt 文本省略，完整文本保留在工具提示中。

自定义项可以传入 QVariantMap 或自定义元类型，再通过 `setItemTemplate(delegate)` 设置 Qt 委托。委托的 `sizeHint()` 和 `paint()` 同时用于行内项与溢出菜单；`index.data(Qt::UserRole)` 是原始数据，`DisplayRole` 默认来自 `QVariant::toString()`。委托不转移所有权，建议以 BreadcrumbBar 为父对象；委托被删除时自动回退到默认显示。

## ExTeachingTip

窗口内非模态引导提示，具有缓存的柔和阴影和尾巴。不使用 `exec()`、遮罩或顶层弹窗，不主动抢走显示前的焦点。关闭叉号根据当前 `QPalette::ButtonText` 绘制，不使用平台标题栏图标，因此随明暗主题、禁用状态变化。

```cpp
#include "exteachingtip.h"

auto* tip = new ExTeachingTip(this);
tip->setTitle(tr("快捷配置"));
tip->setSubtitle(tr("在这里调整设备参数。"));
tip->setActionButtonContent(tr("了解更多"));
tip->setCloseButtonContent(tr("知道了")); // 非空时使用底部关闭按钮。
tip->setPreferredPlacement(ExTeachingTip::Bottom);
tip->setTarget(settingsButton);
tip->setIsOpen(true); // 目标必须已经显示；也可使用 showAt(settingsButton)。
```

### 与 WinUI3 的接口对应

| WinUI3 属性/事件 | Qt 接口与调整 |
|---|---|
| Title / Subtitle | `setTitle(QString)` / `setSubtitle(QString)`，使用纯文本 |
| IsOpen / Target | `setIsOpen(bool)` / `setTarget(QWidget*)`；提供 `isOpenChanged(bool)` |
| ActionButtonContent / CloseButtonContent | 使用 QString 按钮文字；底部关闭内容为空时使用标题区叉号 |
| ActionButtonStyle / CloseButtonStyle / Command | 不照搬 XAML；操作按钮默认使用 accent，可通过 `setActionButtonAccent(false)` 关闭。通过 `actionButton()`、`closeButton()` 配置原生 QPushButton 的样式、图标、启用状态，通过 Qt 信号连接业务逻辑 |
| Content / HeroContent | `setContent(QWidget*)` / `setHeroContent(QWidget*)`，接管新控件所有权；替换时延迟删除旧控件 |
| HeroContentPlacement | `setHeroContentPlacement(HeroContentPlacement::Auto/Top/Bottom)` |
| IconSource | `setIconSource(QIcon)` |
| TailVisibility | `setTailVisibility(TailVisibility::Auto/Visible/Collapsed)` |
| PlacementMargin | `setPlacementMargin(QMargins)` |
| PreferredPlacement | Auto、Top、Bottom、Left、Right、TopRight、TopLeft、BottomRight、BottomLeft、LeftTop、LeftBottom、RightTop、RightBottom、Center |
| IsLightDismissEnabled | `setIsLightDismissEnabled(bool)`，外部点击或滚动可关闭，关闭按钮隐藏；原输入事件仍交给页面处理 |
| ActionButtonClick / CloseButtonClick | `actionButtonClick()` / `closeButtonClick()`；操作按钮不会自动关闭 |
| 生命周期 | `opened()`、`closing(CloseReason, bool*)`、`closed(CloseReason)` |
| CloseReason | CloseButton、LightDismiss、Programmatic |

`closing` 的取消参数只对**同线程直接连接**有效，不可保存指针或使用队列连接，不移植 WinRT 的异步 Deferral：

```cpp
connect(tip, &ExTeachingTip::closing, tip,
        [](ExTeachingTip::CloseReason reason, bool* cancel) {
    // 按需要设置 *cancel = true；这里只处理同步关闭决策。
    Q_UNUSED(reason);
    Q_UNUSED(cancel);
}, Qt::DirectConnection);
```

Qt 侧边界与生命周期：

- **使用 new 创建，不加入布局。** 首次显示时由目标所在窗口持有；无 Target 时使用构造时给定父控件所在窗口，默认放在右下角。跨窗口保存指针推荐使用 `QPointer<ExTeachingTip>`。
- 目标使用弱引用，移动、缩放后重新定位，滚出可视区域、隐藏、被删除或重设父控件时强制关闭。这类关闭不能取消，避免遗留浮层；无目标提示所在页面切换时，调用方应显式关闭。
- 窗口太小而无法容纳时不显示；内容过高时内部滚动，恢复短内容时重新计算高度。
- 保持窗口内子控件实现，因此不提供 `ShouldConstrainToRootBounds=false` 的跨窗口浮层模式；也不暴露 XAML DependencyProperty、TemplateSettings。方向名称与语义对齐 WinUI，但边缘回退与限界由 Qt 的宿主尺寸决定。
- `actionButtonAccent` 是 Qt 侧的便捷属性，默认 `true`，转发到操作按钮的 `accent` 动态属性，由 FluentUI3Style 绘制；关闭按钮不受影响，也不会因此改变默认按钮或焦点。其他 QStyle 是否呈现强调色取决于该样式是否支持 `accent`。
- 打开动画参考 WinUI TeachingTip 的缩放方式：以尾巴连接面板的位置为原点，在 167ms 内将完整快照从 1% 缩放至 100%，使用 Qt 内置的 `OutCubic` 快速减速曲线；无目标时从中心缩放。真实控件几何和布局保持不变，关闭不播放动画。
- `animationEnabled` 默认启用，可通过 `setAnimationEnabled(false)` 关闭打开动画；播放期间关闭会立即恢复真实内容。Gallery 下方的分步操作引导示例关闭动画。
- 阴影使用与本项目 QToolTip 相同的四层颜色、透明度和扩散范围，沿包含尾巴的轮廓外扩；只缓存轮廓，不给整个控件树设置 QGraphicsEffect。
- 边框使用 1 个逻辑像素的线宽：Light 沿用 `surfaceStrokeFlyout`，Dark 使用按钮较亮的 `controlStrokeSecondary`，避免描边与深色面板背景融在一起；以 `QPen` 绘制，不依赖 QToolTip 的复合边框实现。

源码对照：[BreadcrumbLayout](https://github.com/microsoft/microsoft-ui-xaml/blob/main/controls/dev/Breadcrumb/BreadcrumbLayout.cpp)、[BreadcrumbBar 样式](https://github.com/microsoft/microsoft-ui-xaml/blob/main/controls/dev/Breadcrumb/BreadcrumbBar.xaml)、[BreadcrumbBarItem 溢出与键盘处理](https://github.com/microsoft/microsoft-ui-xaml/blob/main/controls/dev/Breadcrumb/BreadcrumbBarItem.cpp)、[TeachingTip 接口](https://github.com/microsoft/microsoft-ui-xaml/blob/main/controls/dev/TeachingTip/TeachingTip.idl)。

### 分步操作引导示例

维护提示：TeachingTip 的子控件、状态和阴影缓存集中在源文件的 `ExTeachingTipPrivate`，公共头文件通过 `EXWIDGETS_DECLARE_PROPERTY_D` 声明转发属性，不重复存储标签和按钮内容。圆角统一调整 `paintEvent()` 中的局部常量 `cornerRadius`，边框、阴影和尾巴避让会跟随该值。

Gallery 的 BreadcrumbBar / TeachingTip 页面下方提供“分步操作引导”：点击“开始引导 / 重新开始”，依次介绍输出设备、输出音量和应用配置。示例只修改页面中的演示数据，不修改系统设置。

实现见 `Examples/Gallery/pagenavigationhint.cpp` 的 `showTourStep()` 和 `stopTour()`：

- 用步骤列表保存目标控件、标题和说明，复用一个 `ExTeachingTip`。
- 在 `actionButtonClick` 中推进步骤；先 `dismiss()`，再滚动目标到可视区、更新内容并 `showAt()`。最后一步将按钮文字改为“完成”。
- 切换步骤前清除当前步骤标记，使这次主动关闭不被当成用户退出；`closed` 只处理退出，不负责推进。
- 叉号、Esc、目标不可见或切换页面会结束引导；再次开始时回到第一步。

手动验收：连续走完三步、每一步按 Esc 或叉号退出后重启、引导中切换页面、缩小窗口并滚动页面，确认不遗留浮层或跳过步骤。

## ExMessageBox

基于 **`QMessageBox`** 子类，WinUI3 视觉；API 与静态方法兼容 Qt 习惯。

```cpp
#include "exmessagebox.h"

ExMessageBox::information(this, tr("标题"), tr("内容"));
ExMessageBox::warning(this, tr("警告"), tr("请注意"));
ExMessageBox::critical(this, tr("错误"), tr("发生错误"));

const auto ret = ExMessageBox::question(
    this, tr("确认"), tr("是否继续？"),
    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
```

可像普通 `QMessageBox` 一样设置 `setInformativeText`、`setCheckBox` 等（参见 Gallery → Dialogs）。

---

## ExContentDialog

WinUI3 **ContentDialog** 模态框：标题、正文、Primary / Secondary / Close 按钮。

```cpp
#include "excontentdialog.h"

ExContentDialog dlg(parent);
dlg.setTitle(tr("删除文件"));
dlg.setContent(tr("此操作不可撤销。"));
dlg.setPrimaryButtonText(tr("删除"));
dlg.setSecondaryButtonText(tr("取消"));
dlg.setCloseButtonText(tr("关闭"));
dlg.setDefaultButton(ExContentDialog::PrimaryBtn);

const ExContentDialog::ContentDialogResult r = dlg.showDialog();
if (r == ExContentDialog::Primary) { /* 确认 */ }
```

---

## ExInfoBar

`ExInfoBar` 是参与页面布局的非阻塞通知，不会像对话框一样覆盖或锁定其他内容。默认处于关闭状态：

```cpp
#include "exinfobar.h"

auto *bar = new ExInfoBar(this);
bar->setSeverity(ExInfoBar::Warning);
bar->setTitle(tr("网络连接不稳定"));
bar->setMessage(tr("部分内容可能无法及时更新。"));
bar->setActionButtonText(tr("重试"));
connect(bar, &ExInfoBar::actionTriggered, this, &Page::retry);
bar->setOpen(true);
```

`setActionWidget()` 可用任意 `QWidget` 替换默认操作按钮；`takeActionWidget()` 可在不删除控件的情况下转移所有权。关闭按钮、图标和动画时长均可配置。

窗口级弹出通知使用 `ExInfoBarHost`。它支持左上、顶部、右上、左下、底部和右下六个锚点。顶部锚点的新通知依次排在已有通知下方，底部锚点则从底边向上堆叠：

```cpp
#include "exinfobarhost.h"

// 主窗口初始化时设置一次。
ExInfoBarHost::setDefaultTarget(mainWindow);

ExInfoBarHost::defaultHost()->showInfoBar(ExInfoBar::Success,
                                          tr("保存成功"),
                                          tr("所有更改均已保存。"),
                                          ExInfoBarHost::TopRight);
```

默认窗口和默认 Host 使用 `QPointer` 跟踪生命周期。需要在其他顶层窗口中
使用独立通知队列时，仍可直接构造 `ExInfoBarHost(window, window)`。

默认在 4.5 秒后自动关闭，鼠标悬停会暂停计时。不设置固定数量上限，但只显示当前窗口高度能够完整容纳的通知，其余通知按加入顺序等待，前面的通知关闭后自动补位。传入 `timeout == 0` 可创建常驻通知。

---

## ExExpander

`ExExpander` 的 Header 始终可见，展开后的 Content 会推开相邻内容而不是覆盖它：

```cpp
#include "exexpander.h"

auto *expander = new ExExpander(this);
expander->setHeader(tr("高级设置"));
expander->addContentWidget(settingsWidget);
expander->setExpanded(true);
```

Header 和 Content 都可以是任意 `QWidget`。`expandDirection` 支持 `Down` 和 `Up`；Header 支持鼠标和键盘操作，并显示焦点状态。
可多次调用 `addContentWidget()` 追加 Content 面板；各项与 Header 连成一个整体，中间使用分隔线，只有最外侧 Content 保留圆角。

---

## ExSpectrumWidget

**通用 Push 型频谱控件**：接收 mono **16-bit PCM**（little-endian），内部 FFT + 对数频带 + Attack/Decay 绘制。  
**不依赖 Qt Multimedia**，Qt 5 / 6 行为一致。

### 数据契约

| 项目 | 约定 |
|------|------|
| 格式 | 16-bit PCM，**单声道**，little-endian |
| 调用 | `pushAudioData()` **追加**数据（非整段替换） |
| 线程 | **建议在 GUI 线程**调用 |
| 采样率 | 必须 `setSampleRate()` 与实际 PCM 一致（默认 44100） |
| 输出 | `bars()` / `barsChanged()` 为 0.0 ~ 1.0 归一化柱高 |

### 标准用法

```cpp
#include "exspectrumwidget.h"

auto *spectrum = new ExSpectrumWidget(this);
spectrum->setSampleRate(44100);
spectrum->setBarCount(64);                    // 可选，默认 64
spectrum->setRefreshIntervalMs(16);           // 可选，~60 FPS

// 周期性推送 PCM（例如每 16ms 一帧）
connect(source, &MyAudioSource::pcmReady,
        spectrum, &ExSpectrumWidget::pushAudioData);

// 兼容 slot 名（信号槽 / Designer）
spectrum->setAudioData(pcmChunk);

// 停止播放时
spectrum->clear();
```

### Q_PROPERTY（Designer / QML 绑定）

- `sampleRate`、`barCount`、`barColor`、`refreshIntervalMs`

### 演示数据来源

- **Gallery（Qt 5）**：`SineWaveGenerator` 合成动画 PCM → `pushAudioData`
- **Gallery（Qt 6）**：Audiomatic Mini 播放器解码后推送
- **独立程序**：`SpectrumDemo`（Qt 6，见 `examples/AudiomaticMini/`）

---

## ExWinUINavigationView / ExNavTreeWidget

Gallery 主窗口左侧 **WinUI3 导航窗格**：主菜单 + 分隔线 + 页脚项，配合 `QStackedWidget` 切页。

```cpp
#include "exwinuinavigationview.h"

auto *nav = new ExWinUINavigationView(this);
nav->setStackedWidget(ui->stackedWidget);

nav->addMainNavigationItem(tr("首页"), 0, SegoeIcon::Home);
nav->addFooterNavigationItem(tr("设置"), 5, SegoeIcon::Settings);

connect(nav, &ExWinUINavigationView::pageIndexChanged, ui->stackedWidget, &QStackedWidget::setCurrentIndex);
nav->setNavigationExpanded(true, /*animated=*/true);
```

`ExNavTreeWidget` 通常由 `ExWinUINavigationView` 内部创建；若单独使用，可调用 `addNavigationItem` / `configureNavigationItem`。

---

## ExStackedWidget / ExTabWidget

在 `QStackedWidget` / `QTabWidget` 基础上增加 **滑动切换动画**（Gallery 中央 `stackedWidget` 使用）。

```cpp
#include "exstackedwidget.h"

auto *stack = new ExStackedWidget(this);
stack->setVerticalMode(true);              // true=上下滑动，false=左右
stack->setSpeed(300);                      // 毫秒
stack->setAnimation(QEasingCurve::InOutSine);
stack->setCurrentIndex(1);
```

`ExTabWidget` API 类似，在 `currentChanged` 时自动播放过渡动画。

---

## ColorGradientSlider

`QSlider` 子类，轨道绘制为 **动态渐变图像**（主要用于 `ExColorPicker` 内部 Hue/Saturation 滑条）。  
若单独使用：

```cpp
#include "colorgradientslider.h"

auto *s = new ColorGradientSlider(Qt::Horizontal, this);
s->setImageBuilder([](QSize size) {
    QImage img(size, QImage::Format_RGB32);
    // 填充渐变...
    return img;
});
```

---

## 与 FluentUI3Style 的关系

- ExWidgets 负责 **控件逻辑与自绘**；标准 Qt 控件的外观仍由 **FluentUI3Style** 绘制。
- ExWidgets 内部分控件（如 `ExRangeSlider`、`ExSpectrumWidget`）使用 `QPalette` / Fluent 色板自绘，**不依赖** Style 的 `QStyle` 分支，但配色与 WinUI3 一致。
- 项目定位见根目录 [README.md](../README.md)「项目定位说明」。

---

## 版本与平台说明

| 能力 | Qt 5 | Qt 6 |
|------|------|------|
| ExWidgets 全部控件 | ✅ | ✅ |
| Gallery → Audiomatic Mini 播放器 | ❌ | ✅（需 Multimedia） |
| Gallery → ExSpectrumWidget 模拟频谱 | ✅ | ✅（Qt6 为完整播放器页） |

---

## 更多信息

- 构建与插件部署：根目录 [README.md](../README.md)
- 交互演示：编译并运行 `Gallery`，打开左侧 **ExWidgets** 分组
- 变更记录：`examples/Gallery/changelog.txt`
