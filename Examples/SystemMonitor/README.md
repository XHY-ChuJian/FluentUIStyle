# FluentSysMon 现代系统性能与硬件监控箱

基于 **Qt + FluentUI3Style + ExWidgets** 的现代化、跨平台系统性能与硬件监控独立实用软件。

---

## 📸 核心模块与功能一览

1. **🏠 性能总览 (Overview)**：
   - `ExRadialGauge` 动态彩色 CPU 利用率仪表盘
   - `ExLiquidGauge` 物理内存水波液体球（动态波浪效果）
   - `ExMultiProgressRing` 存储驱动器多段占比环
   - 实时上下行网速计与吞吐指示
   - 操作系统与硬件规格核心摘要
2. **⚡ 处理器与内存 (CPU & Memory)**：
   - 逻辑核心自适应多卡片实时负载网格
   - 物理内存 (RAM) 与虚拟内存 / 交换文件 (Commit / Swap) 深度分析
3. **💾 磁盘与存储 (Storage)**：
   - 全盘挂载点自动检测与可用余量进度
   - `ExExpander` 折叠面板展示驱动器详情，支持一键在文件资源管理器中打开
4. **🌐 实时网络 (Network)**：
   - 实时下载与上传速率、累计流量统计
   - 网络适配器硬件 MAC、IPv4、连接状态卡片
5. **📊 进程管理器 (Processes)**：
   - 实时系统进程列表（PID、内存占用、线程数）
   - 模糊搜索与排序
   - 一键强制终止进程（`ExContentDialog` 二次确认 + `ExInfoBarHost` 状态提示）
6. **🖥️ 硬件规格与时间轴 (Hardware & Events)**：
   - 操作系统、内核、处理器完整型号与架构、屏幕分辨率
   - `ExTimeline` 记录开机与高负载告警事件流水线
7. **📌 桌面置顶迷你悬浮窗 (Mini Capsule)**：
   - 极简毛玻璃置顶胶囊，实时显示 CPU/内存/网速，支持全屏拖拽与双击切换回主窗口

---

## 🛠️ 构建与运行

在根目录下使用 CMake 构建：

```bash
cmake -B build -S .
cmake --build build --target SystemMonitor
```
