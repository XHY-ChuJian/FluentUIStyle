# Tasks：Qt 5.12.12 (MSVC2017) 兼容性适配

## 1. 环境准备

- [x] 1.1 确认 Qt 5.12.12 (msvc2017_64) 已安装于 `C:\Qt\Qt5.12.12\5.12.12\msvc2017_64`（已确认存在：bin/lib/include 齐全）
- [x] 1.2 确认 Visual Studio 2017 (15.9) 或兼容 MSVC2017 工具链可用（CMake Generator: Visual Studio 15 2017 Win64）
- [x] 1.3 构建输出隔离约定：CMake 构建目录 `build-qt512-msvc2017/`、qmake 构建目录 `build-qt512-qmake/`（项目根下，已被 `.gitignore` 的 `*build-*` 覆盖）；构建后 `git status` 确认源码目录无产物污染

## 2. 代码修复（Qt 5.12 编译失败点）

- [x] 2.1 修复 Example/main.cpp:38：`setHighDpiScaleFactorRoundingPolicy` 调用改为 `#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)) && (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))` 守卫（参考 design D1）
- [x] 2.2 验证其余 `QT_VERSION < 6.0.0` 守卫块内无其他 Qt 5.14+ API（编译通过即验证）

## 3. QWindowKit Qt 5.12 启用

- [x] 3.1 修改根 CMakeLists.txt:60-65：QWindowKit 门槛从 `5.15.2` 降为 `5.12`（参考 design D2）
- [ ] 3.2 在 Qt 5.12.12 下 CMake 配置构建，确认 `FLUENT_ENABLE_QWINDOWKIT` 为 ON、QWKCore/QWKWidgets 编译链接成功、DLL 拷贝到 Example 输出目录
- [ ] 3.3 **门禁（评审 HIGH 项）**：运行启用 QWK 的 Example，验证拖动移动、边缘缩放、窗口按钮三项交互。**若任一项异常**：将根 CMakeLists 的 `FLUENT_ENABLE_QWINDOWKIT` 默认置回 OFF（保留 5.12 可手动开启），并在 README 记录限制；**若全部正常**：维持默认 ON 并继续

## 4. 构建系统清理

- [x] 4.1 删除 Example/Example.pro:3 的 `core-private widgets-private gui-private`（保留 `svg`），与 CMake 的 private 策略保持一致（design D3）
- [x] 4.2 mainwindow.cpp 顶部显式 `#include <utility>`、`#include <functional>`（design D4）
- [x] 4.3 segoeicongallerywidget.cpp:156 的 QTextStream 增加 `#if QT_VERSION < 6.0.0 stream.setCodec("UTF-8"); #endif` 守卫（design D4）

## 5. Qt 5.12 构建验证（CMake）

- [ ] 5.1 配置（out-of-source）：`cmake -S . -B build-qt512-msvc2017 -G "Visual Studio 15 2017 Win64" -DCMAKE_PREFIX_PATH=C:/Qt/Qt5.12.12/5.12.12/msvc2017_64` 无配置错误
- [ ] 5.2 构建 ExWidgets、FluentUI3Style、FluentUI3StylePlugin、Example 全部目标成功（Release + Debug）
- [ ] 5.3 运行 Example：`app.setStyle("FluentUI3")` 后主窗口正常显示，各 showcase 页（Tab、Dialog、Color、RangeSlider、SegoeIconGallery）无崩溃
- [ ] 5.4 无边框窗口验证（QWK）：标题栏拖动移动、边缘/角落缩放、最小化/最大化/关闭按钮（对应 spec qwindowkit-qt5-12）
- [ ] 5.5 验证 Segoe 图标画廊读取 `SegoeFluentIconsEnum.txt` 无乱码

## 6. Qt 5.12 构建验证（qmake）

- [ ] 6.1 用 Qt 5.12.12 qmake 依次构建 ExWidgets → FluentUI3Style → plugin → Example（`qmake <pro> -o build-qt512-qmake/<subdir>/Makefile && nmake`，或 `shadow build` 目录方式），全部成功，产物在构建目录
- [ ] 6.2 运行 qmake 构建的 Example，确认样式应用正常

## 7. 回归验证（高版本 Qt）

- [ ] 7.1 Qt 5.14.2 (MSVC)：CMake 全量构建 + Example 运行回归（确认 HighDPI PassThrough 守卫仍生效、样式正常）
- [ ] 7.2 Qt 5.15.2 (MSVC)：CMake 全量构建回归（QWK 原路径不受 D2 影响）
- [ ] 7.3 Qt 6.6.3 (MSVC2019)：CMake 全量构建 + Example 运行回归（确认 Qt6 分支无回归）

## 8. 文档更新

- [ ] 8.1 README.md / README_EN.md：Qt 版本兼容性表格增加 Qt 5.12.12 行（样式库 ✅ / Example ✅ / 窗口边框 = QWK 无边框），更新 QWindowKit 说明段落（最低 5.12 起启用、5.12 下验证状态与已知限制）
- [ ] 8.2 同步更新"版本兼容性"与"使用说明"中 QWindowKit 的版本描述（如原"Qt ≥ 5.15.2 时启用"字样）

## 9. 收尾

- [ ] 9.1 代码评审：确认改动面最小（仅守卫/构建/头文件/文档），无新增依赖
- [ ] 9.2 提交：按仓库提交规范（feat:/fix:/refactor: 前缀）提交全部改动

## 10. CI 多版本构建矩阵（可选，评审建议项）

- [ ] 10.1 新增 `.github/workflows/build-qt-matrix.yml`：GitHub Actions + aqtinstall 安装 Qt 5.12.12（ubuntu/windows 任一）、5.15.2、6.6.3，全量 CMake 构建（ExWidgets/FluentUI3Style/Plugin/Example）
- [ ] 10.2 矩阵覆盖本方案全部关键路径：HighDPI 守卫（5.12 排除 / 5.14+ 生效）、QWK 门槛（5.12+ 启用）、Example Qt5 分支
- [ ] 10.3 本地先手动跑通一次 aqtinstall 流程验证工作流可复现，再提交 CI 配置
- [ ] 10.4 确认 CI 与本地验证结果一致后，README 增加"CI 验证"说明
