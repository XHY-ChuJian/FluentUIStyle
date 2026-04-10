# FluentUI3Style：基于Qt的FluentUI3风格实现

## 预览
<img width="1657" height="1157" alt="screenshot-20260402-110700" src="https://github.com/user-attachments/assets/ab3ba199-c188-4fd3-8082-fcdf7e4a0f9e" />
<img width="1657" height="1157" alt="screenshot-20260402-110726" src="https://github.com/user-attachments/assets/b275696a-77f1-4541-8747-7c2d802166aa" />
<img width="1657" height="1157" alt="screenshot-20260402-110739" src="https://github.com/user-attachments/assets/a10f0eea-0ed8-4a7c-bad5-5c1e48b68946" />

## 项目简介

FluentUI3Style基于QProxyStyle实现，完整实现了FluentUI3 UI风格，使用到项目中超简单。

## 支持的控件样式

FluentUI3Style支持以下控件的FluentUI3风格：

- **按钮**：普通按钮、复选框、单选按钮、开关按钮
- **输入控件**：文本框、密码框、数字输入框
- **选择控件**：下拉组合框、列表框
- **滑块**：水平滑块、垂直滑块
- **进度条**：线性进度条、环形进度条
- **标签页**：胶囊标签、Pivot标签、分段控件
- **滚动条**：水平滚动条、垂直滚动条
- **菜单**：上下文菜单、菜单栏
- **对话框**：消息框、文件对话框
- **工具栏**：工具栏按钮、分隔符

## 使用方法

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

### 方法3：使用配色方案

```cpp
// 设置为Teams配色方案
PaletteManager::instance().setThemeStyle(ThemeStyle::Teams);

QApplication app(argc, argv);
app.setStyle("FluentUI3");
```

[git地址](https://github.com/XHY-ChuJian/FluentUIStyle.git)

## 核心特性

- **完整的FluentUI3风格实现**：包括按钮、滑块、复选框、单选按钮、组合框等控件的样式
- **主题支持**：自动检测系统主题（Light/Dark）并应用相应的颜色方案
- **平滑动画**：控件状态变化时的平滑过渡效果
- **圆角设计**：符合FluentUI3的圆角美学
- **配色方案**：支持Fluent和Teams两种配色方案
- **Qt版本兼容**：支持Qt5和Qt6，在Qt6.6以下版本也能使用accent颜色

## 技术实现

### 1. 代码来源

FluentUI3Style是基于Qt 6.10自带的Windows 11样式代码移植而来，在此基础上进行了大量的修复和优化：

- 修复了多个控件的显示问题
- 调整了控件大小和布局，使其更符合FluentUI3的设计规范
- 优化了动画效果和性能
- 增强了跨版本兼容性
- 其他问题

### 2. 继承QProxyStyle

FluentUI3Style继承自QProxyStyle，通过重写以下核心方法实现自定义样式：

- `drawComplexControl`：绘制复杂控件（如滑块、组合框）
- `drawPrimitive`：绘制基本元素（如边框、指示器）
- `drawControl`：绘制控件
- `subElementRect`：计算子元素位置
- `subControlRect`：计算子控件位置
- `styleHint`：提供样式提示
- `sizeFromContents`：计算控件大小
- `pixelMetric`：提供像素度量
- `polish`：应用样式到控件

### 3. 颜色体系

定义了完整的FluentUI3颜色体系，包括：

- Light主题颜色方案
- Dark主题颜色方案
- 各种状态下的控件颜色（默认、悬停、按下、禁用）
- 文本颜色
- 边框颜色
- 支持Fluent和Teams两种配色方案

### 4. 图标支持

使用Segoe Fluent Icons字体作为图标源，实现了FluentUI3风格的图标显示。

### 5. 主题检测

自动检测系统主题设置，在Windows 11上使用系统API获取当前主题，在其他系统上使用默认Light主题。

## 代码结构

```
Window11Style/
├── CMakeLists.txt          # 外层CMake配置
├── Example/                # 示例应用
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── mainwindow.h
│   └── mainwindow.ui
├── fluentui3style/         # 核心库
│   ├── CMakeLists.txt
│   ├── fluentui3style.cpp
│   ├── fluentui3style.h
│   ├── fluentui3colors.cpp
│   ├── fluentui3colors.h
│   ├── fluentuiappearance.cpp
│   ├── fluentuiappearance.h
│   ├── palettemanager.cpp
│   ├── palettemanager.h
│   └── plugin/             # 插件子工程
│       ├── CMakeLists.txt
│       ├── fluentui3styleplugin.cpp
│       ├── fluentui3styleplugin.h
│       └── fluentui3styleplugin.json
└── ExWidgets/              # 扩展控件库
    ├── CMakeLists.txt
    ├── exstackedwidget.cpp
    └── exstackedwidget.h
```

## 编译选项

- **BUILD_LIBRARY**：编译为静态库（默认ON）
- **BUILD_PLUGIN**：编译为Qt插件（默认OFF）
- **BUILD_EXAMPLE**：编译示例应用（默认ON）

### 编译命令

```bash
# 默认编译（库+示例）
cmake -B build
cmake --build build

# 编译库+示例+插件
cmake -B build -DBUILD_PLUGIN=ON
cmake --build build
```

## 示例效果

<img width="1783" height="1148" alt="screenshot-20260323-092936" src="https://github.com/user-attachments/assets/9e1391c5-fce2-490c-8689-0aef6411eb06" />
<img width="1783" height="1148" alt="screenshot-20260323-092954" src="https://github.com/user-attachments/assets/e692177d-c5b3-4661-8639-02f568471616" />
<img width="1783" height="1148" alt="screenshot-20260323-093001" src="https://github.com/user-attachments/assets/6cb5f5ce-e136-4c3d-863a-e00ec6164a70" />
<img width="1783" height="1148" alt="screenshot-20260323-093016" src="https://github.com/user-attachments/assets/a803a250-9b52-4029-b56b-6432898b75e8" />
<img width="1538" height="975" alt="screenshot-20260304-104007" src="https://github.com/user-attachments/assets/9dce1186-45d0-4fce-9ed4-bf0fd51c8a01" />

## 注意事项

- **Qt版本**：支持Qt5.15+和Qt6.0+
- **平台支持**：主要针对Windows平台优化，其他平台也可使用
- **插件使用**：编译插件后，需要将插件文件放在`plugins/styles`目录下
- **字体**：使用Segoe Fluent Icons字体，Windows 11默认已安装
- **主题**：自动检测系统主题，也可手动设置

## 未来计划

- 支持更多FluentUI3控件
- 增强自定义主题能力
- 优化性能和动画效果
- 提供更多配色方案
- 完善文档和示例
