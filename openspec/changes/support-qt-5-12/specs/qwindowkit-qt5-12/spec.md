## ADDED Requirements

### Requirement: QWindowKit 在 Qt 5.12 下启用

系统 SHALL 移除根 CMakeLists.txt 中 `QT_VERSION < 5.15.2` 禁用 QWindowKit 的限制，使 Qt 5.12 起的 CMake 构建默认启用 QWindowKit（`FLUENT_ENABLE_QWINDOWKIT`）。qwindowkit 源码已含 Qt 5.12 兼容实现（`QT_VERSION < 5.15.0` 时使用 WindowMoveManipulator / WindowResizeManipulator 模拟系统移动/缩放），无需修改第三方代码。

#### Scenario: Qt 5.12 构建启用 QWK

- **WHEN** 在 Qt 5.12.12 下执行 CMake 配置
- **THEN** `FLUENT_ENABLE_QWINDOWKIT` 为 ON，QWKCore / QWKWidgets 目标被加入构建，Example 的 frameless 源码参与编译

#### Scenario: Qt 5.15.2+ 行为不回归

- **WHEN** 在 Qt 5.15.2 / Qt 6.x 下执行 CMake 配置
- **THEN** QWK 启用逻辑与改动前一致，`qwindowkit` 子模块按各自 Qt 版本的既有分支编译

### Requirement: Qt 5.12 下无边框窗口功能可用

系统 SHALL 保证在 Qt 5.12 + QWindowKit 下，Example 的无边框主窗口具备与 Qt 5.15.2+ 一致的核心交互：拖动移动、边缘/角落缩放、系统菜单与最小化/最大化/关闭按钮可用。

#### Scenario: 无边框窗口拖拽移动

- **WHEN** 在 Qt 5.12.12 运行启用 QWK 的 Example，按住标题栏拖动
- **THEN** 窗口跟随鼠标移动，位置正确，无闪烁

#### Scenario: 无边框窗口边缘缩放

- **WHEN** 在 Qt 5.12.12 下拖动窗口右边缘或右下角
- **THEN** 窗口按鼠标方向缩放，尺寸正确，无卡顿

#### Scenario: 窗口控制按钮

- **WHEN** 点击最小化、最大化/还原、关闭按钮
- **THEN** 分别触发对应系统行为，窗口状态正确

### Requirement: Qt 5.12 无边框限制明确记录

系统 SHALL 在 README 中记录 Qt 5.12 下 QWindowKit 的已知差异与限制（如 `startSystemMove`/`startSystemResize` 不可用时走模拟路径，部分交互可能与 5.15.2+ 有细微差异），供用户决策。

#### Scenario: 已知差异文档化

- **WHEN** 用户查阅 README 的 QWindowKit 说明
- **THEN** 能了解到 Qt 5.12 下无边框窗口的验证状态与已知限制
