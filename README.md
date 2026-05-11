
# FluentUI3Style：基于Qt的FluentUI3风格实现

## 预览
<img width="1920" height="1094" alt="QQ20260509-112403-HD" src="https://github.com/user-attachments/assets/2e5d044d-7f2a-41cd-a76d-e5faf3cbc140" />

<img width="2438" height="1390" alt="screenshot-20260509-112513" src="https://github.com/user-attachments/assets/d1180649-0722-4824-8158-3771debb4a8f" />

<img width="2438" height="1390" alt="screenshot-20260509-112526" src="https://github.com/user-attachments/assets/5d813ac2-3dd8-4ba9-8e60-c823fcf26c5f" />

<img width="2438" height="1390" alt="screenshot-20260509-112552" src="https://github.com/user-attachments/assets/6e297f08-d381-4725-891d-3e9c057563f3" />

<img width="2247" height="1407" alt="screenshot-20260511-094643" src="https://github.com/user-attachments/assets/e59e6846-a9f5-4349-9a30-09251308f00c" />

<img width="2468" height="1390" alt="screenshot-20260509-112806" src="https://github.com/user-attachments/assets/4f0c8084-864e-409c-afc2-98918ff2ec36" />

## 项目简介

FluentUI3Style基于QProxyStyle实现，完整实现了FluentUI3 UI风格，使用到项目中超简单。通过编译成Qt样式插件，可直接在项目中使用`app.setStyle("FluentUI3")`来应用样式，无需手动加载库或链接源码。

### 项目定位说明

本项目定位为样式库，目标是将 Qt 现有控件呈现为 FluentUI（WinUI3）风格。
由于 Qt 组件边界限制，部分 FluentUI 控件无法完全复刻；但会尽量基于现有控件，通过 Style 中的定制逻辑实现接近 FluentUI 的交互与视觉效果，例如：

- SwitchButton
- TabBar实现"Pivot"和"Segmented"控件

为了让 "Example" 展示更完整，会在 ExWidgets 下实现一些组件；这些组件的样式仍由 Style 统一绘制。  
后续可能会增加其他控件，但都会保证这些组件能独立于Style之外运行。

若想修改控件样式，只能通过以往使用QSS的方式，那样会使控件的QStyle样式消失。  
如需深度定制，建议像本项目一样重写对应的 QStyle 逻辑，但深度定制已经是「组件库 [ElaWidgetTools]」的功能了，对于本项目不是很合适。

所以如果需要统一样式，或者在统一样式下做一些小改动，推荐本项目。  
最后，本项目是为了在现有项目中，或者希望简单集成FluentUI样式时使用而实现。

PS:关于本项目自定义控件的问题，如果不改源码的话，本项目没有提供自定义组件的功能，因为本项目写的样式Style与Qt自带的"windowsvista", "Windows", "Fusion"使用方式完全一致，
所以要自定义的话，就跟我们平常对Qt组件改样式的方法一样，qss或者定制qstyle。后续会使用对控件 setProperty的方式，放开一些属性可设。

祝各位Qter能做出完美的Qt程序。

### 插件优势

- **自动部署**：CMake或QMake编译时会自动将样式插件拷贝到对应的Qt目录
- **即插即用**：项目中使用时直接调用`app.setStyle("FluentUI3")`
- **无需依赖**：不需要在项目中链接源码或手动加载库文件

### 使用说明

- **版本兼容性**：目前在Qt5.14.2、Qt5.15.2、Qt6.5.3（MSVC环境）下测试正常
- **MinGW注意**：在MinGW环境下，菜单弹出可能需要特殊处理
- **版本差异**：不同Qt版本间的差异主要体现在右键菜单的显示效果上，可能存在渲染或布局的细微差别
- **兼容性建议**：由于Qt版本众多且自身兼容性差异，建议在使用时针对具体版本进行适当调整。完全兼容所有Qt版本不现实，但会确保对Qt最新稳定版的支持

### Qt版本兼容性


| Qt版本     | 状态   | 备注  |
| -------- | ---- | --- |
| Qt5.14.2 | ✅ 支持 | 已测试 |
| Qt5.15.2 | ✅ 支持 | 已测试 |
| Qt6.6.3  | ✅ 支持 | 已测试 |
| Qt6.10   | ✅ 支持 | 已测试 |


## 编译步骤（详细）

### 一、准备环境

- 操作系统：建议 Windows 10/11
- 编译器：MSVC（与 Qt 套件保持一致）
- Qt：建议使用已测试版本（如 Qt 6.6.3）
- CMake：建议 3.16+

> 说明：请确保 `cmake`、`ninja`（如使用）和 Qt 对应工具链在同一套环境中，避免出现“头文件版本与 moc 版本不一致”问题。

---

### 二、使用 CMake 编译（推荐）

#### 1) 进入项目目录

```powershell
cd D:/workspace/Code/Github/Window11Style
```

#### 2) 配置工程

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="D:/app/Qt/Qt6.6.3/6.6.3/msvc2019_64"
```

如果你希望更明确地指定 Qt 包目录，也可以加上：

```powershell
-DQt6_DIR="D:/app/Qt/Qt6.6.3/6.6.3/msvc2019_64/lib/cmake/Qt6"
```

#### 3) 编译

```powershell
cmake --build build --config Debug
```

或 Release：

```powershell
cmake --build build --config Release
```

#### 4) 运行示例

```powershell
./build/bin/Exampled.exe   # Debug
./build/bin/Example.exe    # Release
```

#### 5) 关键构建选项（CMake）

- `BUILD_LIBRARY`：编译样式库（默认 ON）
- `BUILD_PLUGIN`：编译 Qt Style 插件（默认 ON）
- `BUILD_EXAMPLE`：编译示例程序（默认 ON）

例如只编译库和插件、不编译示例：

```powershell
cmake -S . -B build -DBUILD_EXAMPLE=OFF
```

---

### 三、使用 qmake 编译

项目根目录提供了 `fluentw3uistyle.pro`（`subdirs` 工程）：

```powershell
cd D:/workspace/Code/Github/Window11Style
qmake fluentw3uistyle.pro
nmake
```

或在 Qt Creator 中直接打开 `fluentw3uistyle.pro`，选择对应 Kit 后编译。

---

### 四、插件部署说明

当启用插件构建时，构建脚本会自动把样式插件拷贝到 Qt 的 `plugins/styles` 目录。  
如果你在自定义环境中使用，也可以手动将生成的插件复制到目标 Qt 环境的 `plugins/styles` 下。

## 如何使用

## 项目接入方式（推荐）

下面是把 `FluentUI3Style` 接入到你自己的 Qt 项目的常见流程。

### 1) 构建并部署插件

先按上面的 CMake 或 qmake 步骤完成构建。构建完成后，插件会自动复制到 Qt 的样式插件目录：

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

### 4) CMake / qmake 工程引用示例

#### CMake（示例）

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
add_executable(MyApp main.cpp mainwindow.cpp)
target_link_libraries(MyApp PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
```

#### qmake（示例）

```qmake
QT += core gui widgets
SOURCES += main.cpp mainwindow.cpp
```

只要运行环境能找到 `FluentUI3` 插件（`plugins/styles` 下），业务工程无需显式链接 `FluentUI3Style` 库即可使用 `app.setStyle("FluentUI3")`。

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

### 方式 3：配合 FluentUIAppearance 初始化（可选）

```cpp
#include "fluentuiappearance.h"

FluentUIAppearance::instance()->initialize();

QApplication app(argc, argv);
```

适用场景：

- 需要更完整的外观初始化流程
- 需要统一主题/配色相关行为

## 支持的控件样式

FluentUI3Style通过属性设置的方式支持以下控件的FluentUI3风格：


| 控件类型 | 控件名称           | 说明     | 属性设置                                                                                                                                                                  |
| ---- | -------------- | ------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 按钮   | QPushButton    | 普通按钮   |                                                                                                                                                                       |
| 按钮   | QCheckBox      | 复选框    | `switchButton=true`：启用开关按钮样式                                                                                                                                          |
| 按钮   | QRadioButton   | 单选按钮   |                                                                                                                                                                       |
| 输入控件 | QLineEdit      | 文本框    | 支持底边线动画                                                                                                                                                               |
| 输入控件 | QTextEdit      | 文本编辑框  |                                                                                                                                                                       |
| 输入控件 | QPlainTextEdit | 纯文本编辑框 |                                                                                                                                                                       |
| 输入控件 | QSpinBox       | 数字输入框  | `spinBoxButtonLayout`属性： `0`：垂直箭头（默认） `1`：水平两侧箭头 `2`：水平右侧箭头 `3`：水平两侧加减号                                                                                               |
| 输入控件 | QDoubleSpinBox | 浮点数输入框 | `spinBoxButtonLayout`属性： `0`：垂直箭头（默认） `1`：水平两侧箭头 `2`：水平右侧箭头 `3`：水平两侧加减号                                                                                               |
| 选择控件 | QComboBox      | 下拉组合框  | 支持下拉动画和阴影效果                                                                                                                                                           |
| 选择控件 | QListWidget    | 列表框    | 支持选中指示器动画                                                                                                                                                             |
| 选择控件 | QListView      | 列表视图   | 支持选中指示器动画                                                                                                                                                             |
| 滑块   | QSlider        | 滑块     | 支持水平和垂直方向                                                                                                                                                             |
| 进度条  | QProgressBar   | 进度条    | `progressBarStyle`属性： `0`：细条样式（默认） `1`：粗条样式 `2`：环形样式                                                                                                                  |
| 标签页  | QTabBar        | 标签栏    | `tabBarStyle`属性： `1`：Capsule `2`：Pivot_Grow `3`：Pivot_Slide `4`：Pivot_Stretch `5`：PillTabs `6`：Segmented_Slide `7`：Segmented_Fade `8`：Navigation `9`：Segmented_WinUI3 |
| 标签页  | QTabWidget     | 标签页组件  | 继承QTabBar的样式设置                                                                                                                                                        |
| 滚动条  | QScrollBar     | 滚动条    | 支持水平和垂直方向                                                                                                                                                             |
| 滚动条  | QScrollArea    | 滚动区域   |                                                                                                                                                                       |
| 菜单   | QMenu          | 上下文菜单  | 支持阴影效果                                                                                                                                                                |
| 菜单   | QMenuBar       | 菜单栏    |                                                                                                                                                                       |
| 对话框  | QMessageBox    | 消息框    |                                                                                                                                                                       |
| 对话框  | QFileDialog    | 文件对话框  |                                                                                                                                                                       |
| 工具栏  | QToolButton    | 工具按钮   | 支持菜单箭头动画                                                                                                                                                              |
| 工具栏  | QToolBar       | 工具栏    |                                                                                                                                                                       |
| 树形控件 | QTreeView      | 树型视图   | 支持FluentUI导航控件样式                                                                                                                                                      |
| 表格控件 | QTableView     | 表格视图   |                                                                                                                                                                       |
| 表格控件 | QTableWidget   | 表格组件   |                                                                                                                                                                       |


## 使用方法

### 控件属性设置示例（来自 Example/MainWindow）

建议优先使用 `fluentui3styleproperties.h` 里的属性名常量和枚举值，而不是直接写数字。

```cpp
#include <FluentUI3Style/fluentui3styleproperties.h>

// ProgressBar：粗条样式
ui->progressBar->setProperty(ProgressBarStyleProperty, ProgressBarThick);

// TabBar：WinUI3 Segmented 样式
ui->tabBar->setProperty(TabBarStyleProperty, Segmented_WinUI3);

// TabBar：其他样式
pillBar->setProperty(TabBarStyleProperty, PillTabs);
capTabBar->setProperty(TabBarStyleProperty, Capsule);
navTabBar->setProperty(TabBarStyleProperty, Navigation);

// SpinBox 按钮布局（当前属性名仍是字符串）
ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsVertical);
ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsHorizontalSides);
ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsHorizontalRight);
ui->spinBox->setProperty("spinBoxButtonLayout", PlusMinusHorizontalSides);

// CheckBox 开关样式
checkBox->setProperty(SwitchStyleProperty, true);
```

常用属性速查：

- `TabBarStyleProperty`：`QTabBar` 风格（Capsule / Pivot / Segmented / Navigation）
- `ProgressBarStyleProperty`：`QProgressBar` 风格（Thin / Thick / Ring）
- `SwitchStyleProperty`：`QCheckBox` 切换开关样式
- `ButtonAccentStyleProperty`：按钮 Accent 色风格
- `NavigationViewStyleProperty`：导航指示器样式
- `NoRoundedCorners`：关闭圆角样式

### 方法1：直接创建实例

```cpp
#include "fluentui3style.h"

QApplication app(argc, argv);
app.setStyle(new FluentUI3Style);
```

### 方法2：通过插件加载（推荐）

```cpp
QApplication app(argc, argv);
app.setStyle("FluentUI3");
```

### 方法3：使用FluentUIAppearance

```cpp
#include "fluentuiappearance.h"

// 初始化FluentUI外观
FluentUIAppearance::instance()->initialize();

QApplication app(argc, argv);
```

## 技术实现

### 1. 代码来源

FluentUI3Style是基于Qt 6.10自带的Windows 11样式代码移植而来，在此基础上进行了大量的修复和优化：

- 修复了多个控件的显示问题
- 调整了控件大小和布局，使其更符合FluentUI3的设计规范
- 优化了动画效果和性能
- 增强了跨版本兼容性
- 其他问题

### 2. 颜色体系

定义了完整的FluentUI3颜色体系，包括：

- Light主题颜色方案
- Dark主题颜色方案
- 各种状态下的控件颜色（默认、悬停、按下、禁用）
- 文本颜色
- 边框颜色
- 支持Fluent和Teams两种配色方案

### 3. 图标支持

使用Segoe Fluent Icons字体作为图标源，实现了FluentUI3风格的图标显示。

### 4. 主题检测

自动检测系统主题设置，在Windows 11上使用系统API获取当前主题，在其他系统上使用默认Light主题。

## 编译选项

- **BUILD_LIBRARY**：编译为静态库（默认ON）
- **BUILD_PLUGIN**：编译为Qt插件（默认OFF）
- **BUILD_EXAMPLE**：编译示例应用（默认ON）

## 示例效果

## 未来计划

- 支持更多FluentUI3控件
- 增强自定义主题能力
- 优化性能和动画效果
- 提供更多配色方案

## 协议说明

FluentUI3Style 采用 MIT 许可证开源，允许所有类型项目使用，但要求所有分发的软件中必须保留本项目的MIT授权许可；所有未保留授权分发的商业行为均将被视为侵权行为。
