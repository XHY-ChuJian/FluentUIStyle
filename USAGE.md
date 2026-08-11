# FluentUI3Style 使用方法

## 项目接入方式（推荐）

下面是把 `FluentUI3Style` 接入到你自己的 Qt 项目的常见流程。

### 1) 构建并部署插件

先使用 CMake 完成构建。由于默认开启了 `FLUENTUI3STYLE_COPY_TO_QT_DIR=ON` 选项，编译完成后插件会自动复制到 Qt 的样式插件目录：

- `QT_INSTALL_PLUGINS/styles`

同时会自动复制属性头文件到：

- `QT_INSTALL_HEADERS/FluentUI3Style/fluentui3styleproperties.h`

> 如果 Qt 安装目录在受保护路径（例如 `Program Files`），复制阶段可能需要管理员权限。

### 2) 项目中启用样式

应用启动后直接使用插件名启用：

```cpp
QApplication app(argc, argv);
app.setStyle("FluentUI3");
```

### 3) 在业务代码里使用属性枚举（推荐）

```cpp
#include <FluentUI3Style/fluentui3styleproperties.h>
// 或者 include "fluentui3styleproperties.h"
```

然后通过 `setProperty` 设置控件样式，不建议再写数字魔法值。

### 4) CMake 工程引用示例

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
add_executable(MyApp main.cpp mainwindow.cpp)
target_link_libraries(MyApp PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
```

只要运行环境能找到 `FluentUI3` 插件（`plugins/styles` 下），业务工程无需显式链接 `FluentUI3Style` 库即可使用 `app.setStyle("FluentUI3")`。

## 其他加载方式

### 方式 1：直接在代码中创建样式实例

```cpp
#include "fluentui3style.h"

QApplication app(argc, argv);
app.setStyle(new FluentUI3Style);
```

适用场景：

- 你以源码或库形式集成样式
- 不依赖 Qt 插件机制

### 方式 2：通过插件名加载（推荐）

```cpp
QApplication app(argc, argv);
app.setStyle("FluentUI3");
```

适用场景：

- 已构建并部署 `FluentUI3` 样式插件
- 希望主工程最少改动接入



## 控件属性设置说明

FluentUI3Style 提供了多种扩展属性以支持更加丰富的控件视觉风格。在代码中设置属性前，请包含定义文件：

```cpp
#include <FluentUI3Style/fluentui3styleproperties.h>
```

下面按控件类型为您列举所有的可用属性和设置示例：

### 1. 进度条 (QProgressBar)

可以通过 `ProgressBarStyleProperty` 改变进度条的粗细，甚至将其变为环形。

```cpp
// 进度条样式设置
ui->progressBar->setProperty(ProgressBarStyleProperty, ProgressBarThin);  // 细条样式（默认）
ui->progressBar->setProperty(ProgressBarStyleProperty, ProgressBarThick); // 粗条样式
ui->progressBar->setProperty(ProgressBarStyleProperty, ProgressBarRing);  // 环形样式

// 环形进度条专属属性（ProgressBarRing）
ui->progressBar->setProperty(ProgressBarThicknessProperty, 8); // 环形宽度，默认6
ui->progressBar->setProperty(ProgressBarRingIndeterminateDurationProperty, 1000); // 旋转一圈的时间(ms)，默认800
```

### 2. 选项卡 (QTabBar / QTabWidget)

利用 `TabBarStyleProperty` 能够将普通的 QTabBar 变为现代风格的导航控件。

```cpp
// Segmented（分段）样式
ui->tabBar->setProperty(TabBarStyleProperty, Segmented_WinUI3);
ui->tabBar->setProperty(TabBarStyleProperty, Segmented_Slide);
ui->tabBar->setProperty(TabBarStyleProperty, Segmented_Fade);

// Pivot（滑动文本）样式
ui->tabBar->setProperty(TabBarStyleProperty, Pivot_Grow);
ui->tabBar->setProperty(TabBarStyleProperty, Pivot_Slide);
ui->tabBar->setProperty(TabBarStyleProperty, Pivot_Stretch);

// 其他样式
ui->tabBar->setProperty(TabBarStyleProperty, Capsule);
ui->tabBar->setProperty(TabBarStyleProperty, PillTabs);
ui->tabBar->setProperty(TabBarStyleProperty, Navigation);

// Segmented 样式下的高级颜色定制（可选）
ui->tabBar->setProperty(SegmentedBackgroundColorProperty, QColor(240, 240, 240));
ui->tabBar->setProperty(SegmentedSemiRoundProperty, true); // 是否使用半圆角
```

### 3. 数字输入框 (QSpinBox / QDoubleSpinBox)

通过 `spinBoxButtonLayout`（枚举直接赋值）可改变加减按钮的排布方式。

```cpp
ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsVertical);           // 垂直上下箭头（默认）
ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsHorizontalSides);    // 水平分布在两侧的箭头
ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsHorizontalRight);    // 水平分布在右侧的箭头
ui->spinBox->setProperty("spinBoxButtonLayout", PlusMinusHorizontalSides); // 水平分布在两侧的加减号 (+/-)
```

### 4. 按钮与开关 (QPushButton / QCheckBox)

```cpp
// 强调按钮 (Accent Button)：应用主题强调色
ui->pushButton->setProperty(ButtonAccentStyleProperty, true);

// 将 CheckBox 变为 Switch 拨动开关
ui->checkBox->setProperty(SwitchStyleProperty, true);
```

### 5. 仪表盘与滑块 (QDial / QSlider)

```cpp
// QDial 样式
ui->dial->setProperty(DialStyleProperty, DialDots);  // 圆点刻度
ui->dial->setProperty(DialStyleProperty, DialRing);  // 环形
ui->dial->setProperty(DialStyleProperty, DialThumb); // 带把手的经典样式
ui->dial->setProperty(DialDrawValueProperty, true);  // 是否在中间绘制当前数值

// QSlider 滑块的值提示 (Value Tip)
ui->slider->setProperty(SliderValueTipProperty, true);       // 显示数值提示气泡
ui->slider->setProperty(SliderValueTipLabelProperty, " %");  // 附加文本后缀
```

### 6. 下拉框与菜单弹出动画 (QComboBox / QMenu)

可单独设置，也可设置在 `qApp` 作为全局开关。

```cpp
// 开启下拉框弹出动画
ui->comboBox->setProperty(ComboBoxPopupDropDownAnimationEnabledProperty, true);

// 开启菜单弹出动画
ui->menu->setProperty(MenuPopupAnimationEnabledProperty, true);
```

### 7. 树形组件导航指示器 (QTreeView)

```cpp
// 为 TreeView 开启类似 NavigationView 的选中指示器 (左侧小竖条)
ui->treeView->setProperty(NavigationViewStyleProperty, true);
```

### 8. 通用属性 (关闭圆角)

```cpp
// 对特定控件强行关闭 FluentUI 风格的圆角
ui->widget->setProperty(NoRoundedCorners, true);
```
