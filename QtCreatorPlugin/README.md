# FluentUI3Style Qt Creator 专属扩展插件

本项目通过实现 Qt Creator 原生的 `ExtensionSystem::IPlugin` 插件架构，将 `FluentUI3Style` 作为全局样式直接注入到 Qt Creator IDE 内部，从而绕过 Qt Creator 对普通 Qt 样式插件和命令行 `-style` 的隔离机制。

本项目已**内嵌轻量级 `extensionsystem/iplugin.h` 兼容接口**，无需下载庞大的 Qt Creator 源码树即可通过标准 Qt 开发环境直接编译。

---

## 目录结构

```
QtCreatorPlugin/
├── CMakeLists.txt                 # 插件 CMake 构建脚本
├── fluentui3creatorplugin.h       # 插件类声明 (继承 ExtensionSystem::IPlugin)
├── fluentui3creatorplugin.cpp     # 插件实现 (注入并刷新全局 FluentUI3Style)
├── FluentUI3CreatorPlugin.json    # Qt Creator 插件元数据配置
├── extensionsystem/
│   └── iplugin.h                 # 自包含的 IPlugin 兼容接口定义
└── README.md                      # 本说明文档
```

---

## 编译与部署步骤

### 1. 确认 Qt Creator 运行环境

> **关键前提**：编译 Qt Creator 插件所用的 **Qt 大版本/工具链（如 Qt 6.x MSVC 64bit）以及 Release 模式** 需与 Qt Creator 保持兼容。

1. 打开 Qt Creator，点击顶部菜单栏 **「帮助 (Help)」 -> 「关于 Qt Creator (About Qt Creator)」**。
2. 查看其基于的编译器版本（如 MSVC 2022 64bit Release）。

---

### 2. 通过 CMake 配置并编译插件

在构建工程时，开启 `-DBUILD_QTCREATOR_PLUGIN=ON`，并指定你的 Qt Creator 路径（例如 `D:/Qt/Qt6.10/Tools/QtCreator`）：

```bash
# 切换到 build 目录
mkdir build && cd build

# 通过 CMake 配置（替换为你本地实际的 Qt Creator 路径）
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_QTCREATOR_PLUGIN=ON \
  -DQTCREATOR_DIR="D:/Qt/Qt6.10/Tools/QtCreator"

# 编译插件（会自动部署到 Qt Creator 插件目录）
cmake --build . --config Release --target FluentUI3CreatorPlugin
```

---

### 3. 手动部署（如未指定自动部署）

如果手动复制，将生成的动态库复制到 Qt Creator 的插件目录：

- **Windows**:
  - 将 `FluentUI3CreatorPlugin.dll` 和 `FluentUI3Style.dll` 复制到：
    `D:\Qt\Qt6.10\Tools\QtCreator\lib\qtcreator\plugins` （或 `bin\plugins`）

---

### 4. 启动与管理插件

1. 启动 Qt Creator。
2. 点击菜单栏 **「帮助 (Help)」 -> 「关于插件 (About Plugins...)」**。
3. 在插件列表中找到 **「FluentUI3CreatorPlugin」**，确认其处于已勾选（已启用）状态。
4. 重启 Qt Creator 即可在 IDE 界面中看到 FluentUI3 控件样式生效。
