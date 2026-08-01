# Design：Qt 5.12.12 (MSVC2017) 兼容性适配

## Context

FluentUI3Style 是基于 QProxyStyle 的 FluentUI3 样式库，当前官方测试版本为 Qt 5.14.2 / 5.15.2 / 6.5.3 / 6.6.3（MSVC）。代码已进行大量 `QT_VERSION` 条件编译（60+ 处守卫），覆盖 Qt6 独有 API（`QPalette::Accent`、`QMouseEvent::position()`、`icon.pixmap(dpr,...)` 等）。全项目源码扫描结论：

- **唯一编译失败点**：Example/main.cpp:38 的 `QGuiApplication::setHighDpiScaleFactorRoundingPolicy`（Qt 5.14+ API）被 `#if QT_VERSION < 6.0.0` 误保护。
- **QWindowKit**：qwindowkit 子模块官方支持 Qt 5.12+（README 明确 "Qt 5.12 or later"），源码含 `QT_VERSION < 5.15.0` 的 `startSystemMove`/`startSystemResize` 模拟分支（WindowMoveManipulator / WindowResizeManipulator）。根 CMakeLists.txt:60-65 的 5.15.2 门槛是自加保守限制。
- **Qt private headers**：项目零 `<private/...>` include（fluentui3style.cpp 中的 private 头 include 全为注释；本地 `*_p.h` 是自包含 Qt 源码副本），因此 Qt 5.12 下无需链接 private 模块。
- **C++17 特性**：项目用到的 if-init、`std::as_const`、`[[maybe_unused]]`、`std::make_unique` 等均被 MSVC2017 15.9（Qt 5.12.12 官方工具链）支持。qwindowkit 源码的 `std::optional` 等仅 Qt ≥ 5.15.2 才编译，不受影响。
- **构建系统**：CMake 已用 `find_package(QT NAMES Qt6 Qt5)` + `QT_VERSION_MAJOR` 分支；qmake 侧 `.pro` 全部版本中立，唯一问题是 Example.pro:3 的 `-private` 模块（qmake 能解析但引入误导）。

## Goals / Non-Goals

**Goals:**

- Qt 5.12.12 (MSVC2017) 下 CMake 与 qmake 双路径编译通过（ExWidgets、FluentUI3Style、插件、Example）。
- Qt 5.12 下启用 QWindowKit 无边框窗口，核心交互（拖动、缩放、窗口按钮）可用。
- 不破坏现有 Qt 5.14.2 / 5.15.2 / 6.x 支持。

**Non-Goals:**

- 不做代码风格重构、不引入新的抽象层（如统一的版本宏头文件）——改动面应最小化。
- 不保证 Qt 5.12 下所有视觉效果与 Qt 6.10 完全一致（README 已声明版本间存在渲染细微差异）。
- 不在 qmake 路径集成 QWindowKit（qmake 为遗留路径，README 已注明不再同步维护）。
- 不修改 3rd/qwindowkit 子模块源码。
- AudiomaticMini 维持 Qt6-only（已有三重条件保护）。

## Decisions

### D1. HighDPI 守卫修复：最小局部改动

Example/main.cpp:35-39 改为：

```cpp
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)) && (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
```

- **理由**：Qt 5.14+ 才引入该 API 与枚举；Qt 6 起默认行为已变化且不需要（现有 `QT_VERSION < 6.0.0` 块内其余两个 setAttribute 在 5.12 可用）。
- **备选**：`#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)` 反向排除——语义等价，但正向区间更直观且与全项目守卫风格一致。

### D2. QWindowKit 门槛：从 5.15.2 降到 5.12

根 CMakeLists.txt:60-65 改为：

```cmake
if(QT_VERSION VERSION_LESS "5.12.0")
    set(FLUENT_ENABLE_QWINDOWKIT OFF)
    message(STATUS "QWindowKit disabled: Qt ${QT_VERSION} < 5.12")
else()
    set(FLUENT_ENABLE_QWINDOWKIT ON)
endif()
```

- **理由**：qwindowkit 官方最低支持 Qt 5.12（README 声明 + 源码 `< 5.15` 模拟分支）；Example 的 frameless 集成代码本身版本中立（`#include <QWKWidgets/widgetwindowagent.h>`）。
- **验证点**：Qt 5.12 下 QWK 的 Windows 平台上下文（win32windowcontext.cpp）有 `QT_VERSION < 5.14.0` 分支处理 QStringView 注册表访问，5.12 编译路径已就位。实施时需在 5.12.12 实机确认 QWKCore/QWKWidgets 产出 DLL 正常。
- **默认值回退（评审 HIGH 项）**：实机验证（tasks 3.3）是决定默认 ON/OFF 的门禁。若 Qt 5.12 下 QWK 模拟路径（WindowMoveManipulator/WindowResizeManipulator）的拖动/缩放体验异常，则保持 `FLUENT_ENABLE_QWINDOWKIT` 在 Qt < 5.15.2 下默认 OFF，README 告知手动 `-DFLUENT_ENABLE_QWINDOWKIT=ON` 开启方式与已知限制——不为未验证的交互承诺默认交付。
- **备选方案**：
  - 维持 5.15.2 门槛、文档标注"5.12 需手动开启"——不符合用户"QWindowKit 在 qt5.12 下应该也能使用"的意图，且隐藏真实状态；作为回退基线。
  - 仅 qmake 路径支持 5.12（CMake 保持高门槛）——拒绝：CMake 是官方推荐路径（README 声明），只支持遗留 qmake 路径无实际意义。
  - 提供预编译二进制包给 5.12 用户——拒绝：本项目是源码样式库不发布二进制，超出本次范围（记入 NOT in scope）。

### D3. qmake private 模块清理

Example/Example.pro:3 的 `QT += ... core-private widgets-private gui-private svg` 删除 private 三项（保留 `svg`）。

- **理由**：项目代码零 private 头 include，与 CMake（Qt < 6.10 不链接 private）保持一致；避免误导后续维护者。
- **风险**：若未来某处引入 private 头，qmake 下会编译失败——由新增代码评审把关，且 CMake 侧同步有版本门槛。

### D4. 头文件与编码补强（低风险防御）

- mainwindow.cpp 显式 `#include <utility>`（std::as_const）、`#include <functional>`（std::function）——消除对 MSVC 标准库传递包含的依赖。
- segoeicongallerywidget.cpp:156 的 QTextStream 增加 `#if QT_VERSION < 6.0.0 stream.setCodec("UTF-8"); #endif`，与 mainwindow.cpp:1412-1414 既有模式一致。

### D5. 不引入统一版本宏层

不使用 `#define` 封装或新头文件统一 Qt 版本差异，沿用全项目既有的 `#if QT_VERSION ...` 内联守卫风格。

- **理由**：项目已有 60+ 处该风格守卫，引入新抽象会增大改动面、降低可读性；本次只有 2-3 处新守卫需要添加。

### D6. 构建输出与源码目录隔离

所有 Qt 5.12 构建使用 out-of-source 目录：CMake 构建目录 `build-qt512-msvc2017/`（项目根下），qmake 构建目录 `build-qt512-qmake/`（或同风格命名），构建产物一律不写入源码目录。

- **理由**：用户明确要求；CMake 与 qmake 本身均天然支持 out-of-source（qmake 的 `DESTDIR = $$OUT_PWD/../bin` 已指向构建目录）；`.gitignore` 已有 `*build-*` 与 `/build` 模式，无需改动即被覆盖。
- **验证点**：构建后 `git status` 干净，无新增未跟踪产物文件；如需清空构建产物，直接删除对应 `build-*` 目录。

## Risks / Trade-offs

- [Qt 5.12 下 QWK 交互细节未知（模拟 move/resize 与系统 API 行为差异）] → 实施时在 5.12.12 实机验证拖动/缩放/按钮三项；README 记录验证状态；如发现不可用项，文档标注限制（spec 已含"限制明确记录"要求）。
- [Qt 5.12 与 5.14.2/6.x 渲染差异（菜单阴影、圆角等，README 已声明存在）] → 不视为 bug，回归时以 5.14.2 行为为基准，不做像素级对齐。
- [MSVC2017 的 C++17 实现差异（如 if-init 的 init 语句中变量作用域）] → 已扫描确认使用点均为 MSVC19.11+ 支持的常规用法；以实机构建为准。
- [Example.pro 删除 private 后 qmake 行为变化] → 仅为移除冗余模块，qmake 不链接它们时行为不变；通过 Qt 5.12 与 5.14.2 双版本 qmake 验证。
- [QWK 在 Qt 5.12 下编译耗时长（新增两个库目标）] → 属构建期一次性成本，无运行时影响。

## Migration Plan

1. 代码改动量小（2 处守卫 + 1 处 .pro + 2 处 include/编码），均为向后兼容修改，无破坏性变更；按常规 PR 流程合入即可。
2. 回滚：若 Qt 5.12 实机验证发现问题，可单独回退对应提交（D2 的 CMake 门槛改动与 D1 守卫彼此独立）。
3. 验证矩阵（合入前必须全绿；测试机已装 Qt 5.12.12 于 `C:\Qt\Qt5.12.12\5.12.12\msvc2017_64`）：
   - Qt 5.12.12 MSVC2017（构建目录 `build-qt512-msvc2017/`）：CMake 全量构建 + qmake 构建 + Example 运行（样式、无边框窗口三项交互）
   - Qt 5.14.2 MSVC2019：CMake 构建回归（HighDPI 守卫仍生效）
   - Qt 5.15.2：CMake 构建回归（QWK 原路径不受影响）
   - Qt 6.6.3：CMake 构建回归
   - 每轮构建后 `git status` 确认源码目录无产物污染
4. README 兼容性表格更新后与代码同一提交合入。

## Open Questions

- CI 多版本构建矩阵（GitHub Actions + aqtinstall）已列为可选任务（tasks 第 10 组，评审双模型独立指出的强信号项）：评审认为无 CI 支撑的多版本手工验证矩阵 6 个月后必然腐化。默认以本地验证为准，用户可在实施时决定是否启用 CI。
