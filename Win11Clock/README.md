# Win11 Clock

基于 Qt Widgets 与 `FluentUI3Style` Qt 样式插件实现的 Windows 11 时钟示例。

运行前请确保 `FluentUI3Style` 插件已经安装到当前 Qt 的
`plugins/styles` 目录；应用通过 `qApp->setStyle("FluentUI3")` 加载它，
不会再次编译或链接样式库源码。

## 功能

- 专注时段快捷启动
- 多计时器、新建/编辑/删除、暂停与重置
- 闹钟、新建/编辑/删除、重复日期与开关
- 秒表计时与计次
- 多时区世界时钟
- 深色/浅色主题切换

## 构建

可以随仓库根工程一起构建：

```powershell
cmake -S .. -B ../build
cmake --build ../build --target Win11Clock
```

也可以单独配置本目录：

```powershell
cmake -S . -B build
cmake --build build
```
