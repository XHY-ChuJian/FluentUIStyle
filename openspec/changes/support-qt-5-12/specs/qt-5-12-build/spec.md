## ADDED Requirements

### Requirement: 全项目在 Qt 5.12 下可编译

系统 SHALL 支持在 Qt 5.12.12 (MSVC2017 15.9, Windows) 环境下编译 ExWidgets、FluentUI3Style 库、样式插件与 Example 示例应用，CMake 与 qmake 两条构建路径均需通过。所有 Qt 5.14+ / Qt 6 独有 API 的使用点 SHALL 由 `QT_VERSION` 条件编译守卫，不得依赖 Qt 5.12 不存在的符号。

#### Scenario: CMake 构建 ExWidgets / FluentUI3Style / 插件

- **WHEN** 使用 CMake 配置 `CMAKE_PREFIX_PATH` 指向 Qt 5.12.12 (msvc2017_64) 并构建 `ExWidgets`、`FluentUI3Style`、`FluentUI3StylePlugin` 目标
- **THEN** 三个目标全部编译、链接成功，无未解析符号错误

#### Scenario: CMake 构建 Example

- **WHEN** 在 Qt 5.12.12 下构建 `Example` 目标
- **THEN** Example 编译链接成功，`spectrumshowcasewidget`/`sinewavegenerator` 等 Qt5 分支源码正常参与构建，AudiomaticMini 不参与构建

#### Scenario: qmake 构建全项目

- **WHEN** 使用 Qt 5.12.12 的 qmake 依次构建 ExWidgets、FluentUI3Style、插件、Example
- **THEN** 各 `.pro` 构建成功，且不因 qmake 解析 `-private` 模块或缺少 `Qt::AA_*` 属性而失败

#### Scenario: Qt 5.14+ API 误用防护

- **WHEN** 在 Qt 5.12 下编译包含 `QGuiApplication::setHighDpiScaleFactorRoundingPolicy` 调用的源码
- **THEN** 该调用被 `QT_VERSION >= 5.14.0` 条件排除，编译不报错；在 Qt 5.14+ / Qt 6 下仍生效

### Requirement: Qt 5.12 下运行不退化

系统 SHALL 保证在 Qt 5.12 下运行时样式绘制、控件交互与 Qt 5.14.2 版本行为一致，不出现编译期不可见的行为退化。文本/编码相关的文件读取 SHALL 在 Qt 5 下显式指定 UTF-8 编码，避免依赖 locale 导致乱码。

#### Scenario: Example 启动并应用 FluentUI3 样式

- **WHEN** 在 Qt 5.12.12 环境运行编译出的 Example，调用 `app.setStyle("FluentUI3")`
- **THEN** 主窗口正常显示 FluentUI3 风格控件，无崩溃、无样式缺失

#### Scenario: Segoe 图标枚举文件读取

- **WHEN** 在 Qt 5 下打开 Segoe 图标画廊页并读取 `SegoeFluentIconsEnum.txt`
- **THEN** 文件按 UTF-8 读取，图标列表完整显示，无乱码

#### Scenario: 右键菜单中文文本编码

- **WHEN** 在中文 Windows (CP936) + Qt 5.12 下弹出右键菜单
- **THEN** 菜单文本通过 UTF-8 流式读取显示正常，无乱码

### Requirement: 构建产物安装与打包兼容 Qt 5.12

系统 SHALL 保证 CMake 安装规则（install 头文件、库、CMake package 配置）在 Qt 5.12 下生成可用的 `FluentUI3StyleConfig.cmake` / `ExWidgetsConfig.cmake`，下游项目可 `find_package` 后直接链接使用。

#### Scenario: 安装包配置可被下游消费

- **WHEN** 在 Qt 5.12.12 下 `cmake --install` 后，一个独立项目通过 `find_package(FluentUI3Style)` 链接
- **THEN** 依赖解析正确（`find_dependency(Qt5 ...)`），链接成功且运行正常

### Requirement: 版本兼容性文档更新

系统 SHALL 将 Qt 5.12.12 的支持状态写入 README.md / README_EN.md 的 Qt 版本兼容性表格与 QWindowKit 说明，明确标注验证环境与已知限制。

#### Scenario: 文档反映真实支持状态

- **WHEN** 用户查阅 README 的兼容性表格
- **THEN** 表格包含 Qt 5.12.12 行，标注样式库 / Example / 窗口边框支持状态，与实测一致

## MODIFIED Requirements

<!-- 无既有 spec，无 MODIFIED -->
