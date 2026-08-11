
# FluentUI3Style：基于Qt的FluentUI3风格实现

**English:** [README_EN.md](README_EN.md) | **使用方法:** [USAGE.md](USAGE.md)

## 预览
<img width="1280" height="854" alt="QQ20260811-115406" src="https://github.com/user-attachments/assets/ca6f40e0-f1f5-4671-8f84-fa5a463e60d2" />
<img width="2081" height="1335" alt="QQ20260713-173357" src="https://github.com/user-attachments/assets/be4a2d53-adee-408e-b837-0e8f37ee418f" />

## 自定义控件预览
<img width="2697" height="1279" alt="QQ20260811-114012" src="https://github.com/user-attachments/assets/744b67cb-6f5b-4f6a-8ed7-ea3b72637644" />
<img width="2081" height="1335" alt="QQ20260713-173544" src="https://github.com/user-attachments/assets/3438b9bc-cfc8-4b81-a199-ee635cfcb7bb" />
<img width="1972" height="1329" alt="QQ20260713-173612" src="https://github.com/user-attachments/assets/856a6e6c-ec98-46f6-9c6d-7657b361dc63" />
<img width="2081" height="1335" alt="QQ20260713-173534" src="https://github.com/user-attachments/assets/e7ec0db1-617f-4ee3-a157-4b1192581e21" />

## QDesigner展示
<img width="2616" height="1234" alt="image" src="https://github.com/user-attachments/assets/9f3526d0-cd38-46ee-a325-0c667a11d341" />

## 项目简介

FluentUI3Style基于QProxyStyle实现，完整实现了FluentUI3 UI风格，使用到项目中超简单。通过编译成Qt样式插件，可直接在项目中使用`app.setStyle("FluentUI3")`来应用样式，无需手动加载库或链接源码。

### 基础样式选择

FluentUI3Style 基于 `QProxyStyle` 实现，没有重写的尺寸指标、子控件布局、默认绘制和交互行为仍由基础样式提供。实测表明，Qt 的 `windowsvista` 与 `fusion` 在这些细节上并不完全一致，因此即使使用同一套 FluentUI3Style 代码，部分控件的尺寸、位置和最终视觉效果仍可能存在细微差别。

为尽量保持 Windows 下的预期效果，FluentUI3Style 默认优先使用 `windowsvista` 作为基础样式；当前 Qt 环境不提供 `windowsvista` 时（例如部分非 Windows 平台），再自动回退到 `fusion`。通过构造函数显式传入 `QStyle` 时，则以传入的样式为准。通过 `app.setStyle("FluentUI3")` 加载插件时也会进入相同的选择逻辑。

### 项目定位说明

本项目定位为样式库，目标是将 Qt 现有控件呈现为 FluentUI（WinUI3）风格。
由于 Qt 组件边界限制，部分 FluentUI 控件无法完全复刻；但会尽量基于现有控件，通过 Style 中的定制逻辑实现接近 FluentUI 的交互与视觉效果，例如：

- SwitchButton
- TabBar实现"Pivot"和"Segmented"控件

为了让 "Gallery" 展示更完整，会在 ExWidgets 下实现一些组件；这些组件的样式仍由 Style 统一绘制。
后续可能会增加其他控件，但都会保证这些组件能独立于Style之外运行。

**ExWidgets 各控件的使用说明（接入方式、API 示例、数据格式约定）见：[ExWidgets/README.md](ExWidgets/README.md)**（[English](ExWidgets/README_EN.md)）

若想修改控件样式，只能通过以往使用QSS的方式，那样会使控件的QStyle样式消失。  
如需深度定制，建议像本项目一样重写对应的 QStyle 逻辑，不过深度定制是【ElaWidgetTools】组件库的功能了，对于本项目不是很合适，本项目只是样式库。

所以如果需要统一样式，或者在统一样式下做一些小改动，推荐本项目。  
最后，本项目是为了在现有项目中，或者希望简单集成FluentUI样式时使用而实现。

PS:关于本项目自定义控件的问题，如果不改源码的话，本项目没有提供自定义组件的功能，因为本项目写的样式Style与Qt自带的"windowsvista", "Windows", "Fusion"使用方式完全一致，
所以要自定义的话，就跟我们平常对Qt组件改样式的方法一样，qss或者定制qstyle。后续会使用对控件 setProperty的方式，放开一些属性可设。

祝各位Qter能做出完美的Qt程序。

### 插件优势

- **自动部署**：CMake 编译时可将样式插件拷贝到对应的 Qt 目录（见下方 CMake 选项 `FLUENTUI3STYLE_COPY_TO_QT_DIR`）
- **即插即用**：项目中使用时直接调用`app.setStyle("FluentUI3")`
- **无需依赖**：不需要在项目中链接源码或手动加载库文件

### 构建方式说明

目前仅对 **CMake** 进行维护更新，推荐使用 CMake 构建本项目。
### 使用说明

- **版本兼容性**：样式库在 Qt 5.14.2、Qt 5.15.2、Qt 6.5.3、Qt 6.6.3（MSVC 环境）下测试正常
- **可选无边框组件**：Qt ≥ 5.15.2 时默认构建 `ExWidgets::Frameless`（需 CMake ≥ 3.19）；旧版 Qt 自动关闭。可显式设置 `EXWIDGETS_BUILD_FRAMELESS=OFF` 使基础 `ExWidgets` 不引入 QWindowKit。
- **MinGW注意**：在MinGW环境下，菜单弹出可能需要特殊处理
- **版本差异**：不同Qt版本间的差异主要体现在右键菜单的显示效果上，可能存在渲染或布局的细微差别
- **兼容性建议**：由于Qt版本众多且自身兼容性差异，建议在使用时针对具体版本进行适当调整。完全兼容所有Qt版本不现实，但会确保对Qt最新稳定版的支持

### Qt版本兼容性


| Qt版本     | 样式库 | Gallery（CMake） | 窗口边框 |
| -------- | ---- | -------------- | ---- |
| Qt5.14.2 | ✅ 支持 | ✅ 支持 | **系统边框**（无 QWindowKit 无边框） |
| Qt5.15.2 | ✅ 支持 | ✅ 支持 | 可选 QWindowKit 无边框 + DWM 背景 |
| Qt6.6.3  | ✅ 支持 | ✅ 支持 | 可选 QWindowKit 无边框 + DWM 背景 |
| Qt6.8+   | ✅ 支持 | ✅ 支持 | 可选 QWindowKit 无边框 + DWM 背景 |
| Qt6.10   | ✅ 支持 | ✅ 支持 | 样式代码基于 Qt 6.10 Win11 样式移植 |


## 编译步骤

### 1. 获取源码（含子模块）

若需要无边框组件支持（依赖 QWindowKit），请在克隆时初始化子模块：

```powershell
git clone --recursive https://github.com/XHY-ChuJian/FluentUIStyle.git
```

### 2. 编译工程

推荐直接使用支持 CMake 的 IDE（如 Qt Creator、Visual Studio 或 CLion）打开根目录的 `CMakeLists.txt` 进行配置与编译。

### 3. CMake 构建选项

- `BUILD_LIBRARY`：编译样式库（默认 ON）
- `BUILD_PLUGIN`：编译 Qt Style 插件（默认 ON）
- `BUILD_GALLERY`：编译 Gallery（默认 ON）
- `EXWIDGETS_BUILD_FRAMELESS`：编译 `ExWidgets::Frameless` 并引入 QWindowKit（Qt ≥ 5.15.2 默认 **ON**，旧版 Qt 默认 **OFF**）
- `FLUENTUI3STYLE_COPY_TO_QT_DIR`：构建后将插件复制到 Qt 的 `plugins/styles`（默认 **ON**）

## 使用方法

完整的使用说明、控件属性设置以及不同的接入方式，请参阅单独的文档：**[使用方法 (USAGE.md)](USAGE.md)**。

**最简单的加载方式：**
若已将插件部署到 Qt 目录，在项目启动后直接调用即可应用样式：

```cpp
QApplication app(argc, argv);
app.setStyle("FluentUI3");
```

## 支持的控件样式

FluentUI3Style通过属性设置的方式支持以下控件的FluentUI3风格：


| 控件类型 | 控件名称           | 说明     | 属性设置                                                                                                                                                                  |
| ---- | -------------- | ------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 按钮   | QPushButton    | 普通按钮   |                                                                                                                                                                       |
| 按钮   | QCheckBox      | 复选框    | `switchButton=true`：启用开关按钮样式                                                                                                                                          |
| 按钮   | QRadioButton   | 单选按钮   |                                                                                                                                                                       |
| 输入控件 | QLineEdit      | 文本框    | 支持底边线动画                                                                                                                                                               |
| 输入控件 | QSpinBox       | 数字输入框  | `spinBoxButtonLayout`属性： `0`：垂直箭头（默认） `1`：水平两侧箭头 `2`：水平右侧箭头 `3`：水平两侧加减号                                                                                               |
| 输入控件 | QDoubleSpinBox | 浮点数输入框 | `spinBoxButtonLayout`属性： `0`：垂直箭头（默认） `1`：水平两侧箭头 `2`：水平右侧箭头 `3`：水平两侧加减号                                                                                               |
| 选择控件 | QComboBox      | 下拉组合框  | 支持下拉动画和阴影效果                                                                                                                                                           |
| 选择控件 | QListWidget    | 列表框    | 支持选中指示器动画                                                                                                                                                             |
| 选择控件 | QListView      | 列表视图   | 支持选中指示器动画                                                                                                                                                             |
| 滑块   | QSlider        | 滑块     | 支持水平和垂直方向                                                                                                                                                             |
| 进度条  | QProgressBar   | 进度条    | `progressBarStyle`属性： `0`：细条样式（默认） `1`：粗条样式 `2`：环形样式                                                                                                                  |
| 标签页  | QTabBar        | 标签栏    | `tabBarStyle`属性： `1`：Capsule `2`：Pivot_Grow `3`：Pivot_Slide `4`：Pivot_Stretch `5`：PillTabs `6`：Segmented_Slide `7`：Segmented_Fade `8`：Navigation `9`：Segmented_WinUI3 |
| 滚动条  | QScrollBar     | 滚动条    | 支持水平和垂直方向                                                                                                                                                             |
| 菜单   | QMenu          | 上下文菜单  | 支持阴影效果                                                                                                                                                                |
| 菜单   | QMenuBar       | 菜单栏    |                                                                                                                                                                       |
| 对话框  | QMessageBox    | 消息框    |                                                                                                                                                                       |
| 工具栏  | QToolButton    | 工具按钮   | 支持菜单箭头动画                                                                                                                                                              |
| 树形控件 | QTreeView      | 树型视图   | 支持FluentUI导航控件样式                                                                                                                                                      |
| 表格控件 | QTableView     | 表格视图   |                                                                                                                                                                       |


## 技术实现

### 1. 代码来源

FluentUI3Style是基于Qt 6.10自带的Windows 11样式代码移植而来，在此基础上进行了大量的修复和优化：

- 修复了多个控件的显示问题
- 调整了控件大小和布局，使其更符合FluentUI3的设计规范
- 优化了动画效果和性能
- 增强了跨版本兼容性
- 其他问题


## 示例效果

## 未来计划

- 支持更多FluentUI3控件
- 增强自定义主题能力
- 优化性能和动画效果
- 提供更多配色方案

## 致谢

本项目在界面布局与交互设计方面参考了以下优秀项目：
- [WinUI-Gallery](https://github.com/microsoft/WinUI-Gallery.git)
- [FluentUI](https://github.com/zhuzichu520/FluentUI)
- [ElaWidgetTools](https://github.com/Liniyous/ElaWidgetTools)

## 协议说明

FluentUI3Style 采用 MIT 许可证开源，允许所有类型项目使用，但要求所有分发的软件中必须保留本项目的MIT授权许可；所有未保留授权分发的商业行为均将被视为侵权行为。

本项目部分代码由 AI 工具辅助生成或修改。如认为相关内容侵犯了您的合法权益，请通过项目 Issue 联系维护者，我们会及时核查并处理。
