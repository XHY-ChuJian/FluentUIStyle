# 支持 Qt 5.12（Qt 5.12.12 / MSVC2017）

## Why

当前 FluentUI3Style 样式库实测支持 Qt 5.14.2+ / Qt 5.15.2+ / Qt 6.x（MSVC），但许多存量业务项目仍运行在 Qt 5.12.12 (MSVC2017) 上，无法升级。让本项目在 Qt 5.12 下可编译、可运行，可以覆盖这些存量用户。全项目源码扫描确认：除 1 处 Qt 5.14 API 误保护外，代码已具备良好的版本条件编译，适配成本低。QWindowKit 官方声明最低支持 Qt 5.12，项目自加的 5.15.2 门槛属于保守限制，应一并移除。

差异化视角：同类 Qt FluentUI C++ 库 ElaWidgetTools 从 Qt 5.15 起步，本项目支持到 Qt 5.12 可形成对 5.12/5.14 存量项目的覆盖优势。

## What Changes

- 修复 Example/main.cpp:38：`QGuiApplication::setHighDpiScaleFactorRoundingPolicy` 是 Qt 5.14+ API，被 `QT_VERSION < 6.0.0` 误保护导致 Qt 5.12 编译失败，改为 `QT_VERSION >= 5.14.0` 条件。
- 移除根 CMakeLists.txt:60-65 对 QWindowKit 的 `QT_VERSION < 5.15.2` 禁用门槛，允许 Qt 5.12 起启用 QWindowKit 无边框窗口（qwindowkit 官方支持 Qt 5.12+，源码含 `< 5.15` 的 startSystemMove/Resize 模拟分支）。
- 清理 Example/Example.pro:3 多余的 `-private` qmake 模块依赖（项目代码零 `<private/...>` include，CMake 侧 Qt < 6.10 也不链接 private 模块）。
- 补充头文件：mainwindow.cpp 显式 include `<utility>`、`<functional>`；segoeicongallerywidget.cpp:156 的 QTextStream 增加 Qt5 `setCodec("UTF-8")` 守卫（与 mainwindow.cpp:1412 一致）。
- 保持现状不变的部分：AudiomaticMini 维持 Qt6-only（三重条件保护），qmake 路径不集成 QWindowKit（与当前 CMake/qmake 分工一致，README 已注明 qmake 遗留）。
- 新增 Qt 5.12.12 MSVC2017 的构建/测试验证流程（README 兼容性表格更新 + 验证命令）。
- 构建输出隔离：CMake 与 qmake 均使用 out-of-source 构建目录（如 `build-qt512-msvc2017/`），构建产物不写入源码目录；该目录已被 `.gitignore` 的 `*build-*` 模式覆盖。
- **（建议扩展，评审项）** CI 多版本构建矩阵（GitHub Actions + aqtinstall）：5.12.12 / 5.15.2 / 6.6.3 三版本全量构建，防止多版本手工验证矩阵随时间腐化。默认不在本次核心范围，作为可选任务（见 tasks 第 10 组）。

## Capabilities

### New Capabilities

- `qt-5-12-build`: 全项目（ExWidgets、FluentUI3Style 库、插件、Example）在 Qt 5.12.12 / MSVC2017 下可编译、可运行，CMake 与 qmake 两条构建路径均支持。
- `qwindowkit-qt5-12`: QWindowKit 无边框窗口能力在 Qt 5.12 下可用（CMake 构建、Windows 平台），与 Qt 5.15.2+ 行为一致。

### Modified Capabilities

<!-- 无：openspec/specs/ 目前为空，全部为新增能力 -->

## Impact

- **构建系统**：根 CMakeLists.txt（QWK 门槛）、Example/CMakeLists.txt（无改动或微调）、Example/Example.pro（private 模块）、common.pri（无改动）。
- **示例代码**：Example/main.cpp（HighDPI 守卫）、mainwindow.cpp（头文件）、segoeicongallerywidget.cpp（setCodec）。
- **第三方依赖**：3rd/qwindowkit（子模块，已含 Qt 5.12 兼容代码，无改动）；AudiomaticMini 保持 Qt6-only。
- **测试环境**：Qt 5.12.12 (MSVC2017 15.9)，已安装于 `C:\Qt\Qt5.12.12\5.12.12\msvc2017_64`；Qt 5.14.2/5.15.2（回归）、Qt 6.6.3+（回归）。
- **文档**：README.md / README_EN.md 的 Qt 版本兼容性表格与 QWindowKit 说明。
