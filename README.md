# FluentUI3Style：基于Qt的FluentUI3风格实现

## 预览
<img width="2004" height="1338" alt="screenshot-20260410-173023" src="https://github.com/user-attachments/assets/11e20b1e-2b6f-47eb-9197-00ae41066d87" />
<img width="2004" height="1338" alt="screenshot-20260410-173008" src="https://github.com/user-attachments/assets/3db1a598-2adf-4492-9417-7ffbb50044bf" />
<img width="2004" height="1338" alt="screenshot-20260410-173052" src="https://github.com/user-attachments/assets/320f385b-68bf-4320-aa23-cebbb0b3c056" />

## 项目简介

FluentUI3Style基于QProxyStyle实现，完整实现了FluentUI3 UI风格，使用到项目中超简单。通过编译成Qt样式插件，可直接在项目中使用`app.setStyle("FluentUI3")`来应用样式，无需手动加载库或链接源码。

### 插件优势
- **自动部署**：CMake或QMake编译时会自动将样式插件拷贝到对应的Qt目录
- **即插即用**：项目中使用时直接调用`app.setStyle("FluentUI3")`
- **无需依赖**：不需要在项目中链接源码或手动加载库文件

### Qt版本兼容性

| Qt版本 | 状态 | 备注 |
|--------|------|------|
| Qt5.15.2 | ✅ 支持 | 已测试 |
| Qt6.6.3 | ✅ 支持 | 已测试 |
| Qt6.10 | ✅ 支持 | 已测试 |

## 支持的控件样式

FluentUI3Style通过属性设置的方式支持以下控件的FluentUI3风格：

| 控件类型 | 控件名称 | 说明 | 属性设置 |
|---------|---------|------|---------|
| 按钮 | QPushButton | 普通按钮 | 支持悬停、按下效果 |
| 按钮 | QCheckBox | 复选框 | `switchButton=true`：启用开关按钮样式 |
| 按钮 | QRadioButton | 单选按钮 | |
| 输入控件 | QLineEdit | 文本框 | 支持底边线动画 |
| 输入控件 | QTextEdit | 文本编辑框 | |
| 输入控件 | QPlainTextEdit | 纯文本编辑框 | |
| 输入控件 | QSpinBox | 数字输入框 | `spinBoxButtonLayout`属性：<br>`0`：垂直箭头（默认）<br>`1`：水平两侧箭头<br>`2`：水平右侧箭头<br>`3`：水平两侧加减号 |
| 输入控件 | QDoubleSpinBox | 浮点数输入框 | `spinBoxButtonLayout`属性：<br>`0`：垂直箭头（默认）<br>`1`：水平两侧箭头<br>`2`：水平右侧箭头<br>`3`：水平两侧加减号 |
| 选择控件 | QComboBox | 下拉组合框 | 支持下拉动画和阴影效果 |
| 选择控件 | QListWidget | 列表框 | 支持选中指示器动画 |
| 选择控件 | QListView | 列表视图 | 支持选中指示器动画 |
| 滑块 | QSlider | 滑块 | 支持水平和垂直方向 |
| 进度条 | QProgressBar | 进度条 | `progressBarStyle`属性：<br>`0`：细条样式（默认）<br>`1`：粗条样式<br>`2`：环形样式 |
| 标签页 | QTabBar | 标签栏 | `tabBarStyle`属性：<br>`1`：胶囊标签<br>`2`：Pivot_Grow<br>`3`：Pivot_Slide<br>`4`：Pivot_Stretch<br>`5`：PillTabs<br>`6`：Segmented_Slide<br>`7`：Segmented_Fade<br>`8`：Navigation |
| 标签页 | QTabWidget | 标签页组件 | 继承QTabBar的样式设置 |
| 滚动条 | QScrollBar | 滚动条 | 支持水平和垂直方向 |
| 滚动条 | QScrollArea | 滚动区域 | |
| 菜单 | QMenu | 上下文菜单 | 支持阴影效果 |
| 菜单 | QMenuBar | 菜单栏 | |
| 对话框 | QMessageBox | 消息框 | |
| 对话框 | QFileDialog | 文件对话框 | |
| 工具栏 | QToolButton | 工具按钮 | 支持菜单箭头动画 |
| 工具栏 | QToolBar | 工具栏 | |
| 树形控件 | QTreeView | 树型视图 | 支持FluentUI导航控件样式 |
| 表格控件 | QTableView | 表格视图 | |
| 表格控件 | QTableWidget | 表格组件 | |

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

### 方法3：使用FluentUIAppearance

```cpp
#include "fluentuiappearance.h"

// 初始化FluentUI外观
FluentUIAppearance::instance()->initialize();

QApplication app(argc, argv);
app.setStyle("FluentUI3");
```

[git地址](https://github.com/XHY-ChuJian/FluentUIStyle.git)

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

<img width="1783" height="1148" alt="screenshot-20260323-092936" src="https://github.com/user-attachments/assets/9e1391c5-fce2-490c-8689-0aef6411eb06" />
<img width="1783" height="1148" alt="screenshot-20260323-092954" src="https://github.com/user-attachments/assets/e692177d-c5b3-4661-8639-02f568471616" />
<img width="1783" height="1148" alt="screenshot-20260323-093001" src="https://github.com/user-attachments/assets/6cb5f5ce-e136-4c3d-863a-e00ec6164a70" />
<img width="1783" height="1148" alt="screenshot-20260323-093016" src="https://github.com/user-attachments/assets/a803a250-9b52-4029-b56b-6432898b75e8" />
<img width="1538" height="975" alt="screenshot-20260304-104007" src="https://github.com/user-attachments/assets/9dce1186-45d0-4fce-9ed4-bf0fd51c8a01" />

## 未来计划

- 支持更多FluentUI3控件
- 增强自定义主题能力
- 优化性能和动画效果
- 提供更多配色方案

## 协议说明

FluentUI3Style 采用 MIT 许可证开源，允许所有类型项目使用，但要求所有分发的软件中必须保留本项目的MIT授权许可；所有未保留授权分发的商业行为均将被视为侵权行为。
