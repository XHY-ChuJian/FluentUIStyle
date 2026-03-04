
# CusProxyStyle：基于Qt的FluentUI3风格实现
## 预览
<img width="1538" height="975" alt="screenshot-20260304-101705" src="https://github.com/user-attachments/assets/bd4cbee8-e3f1-4972-b1d7-9fc178a13c8f" />

## 项目简介

CusProxyStyle是一个基于Qt的自定义样式类，旨在模仿Windows 11的FluentUI3设计风格，为Qt应用程序提供现代化的界面外观。
继承QProxyStyle类实现样式，使用到项目中超简单

```cpp
QApplication app(argc, argv);
app.setStyle(new CusProxyStyle);
```

[git地址](https://github.com/XuHongYun-ChuJian/Window11Style.git)

## 提示
实测Qt5(5.15.2) 在控件右键菜单存在阴影Bug和QDockWidget按钮图标显示不清。
- 右键菜单阴影Bug，main.cpp中有解决办法，使用Qt5的可以自行修改
- QDockWidget按钮图标显示不清目前只想到可以给dock->setTitleBar的办法，同上
- Qt5还存在其他问题，比如QComboBox必须setView,样式才生效等
- TODO

## 核心特性

- **完整的FluentUI3风格实现**：包括按钮、滑块、复选框、单选按钮、组合框等控件的样式
- **主题支持**：自动检测系统主题（Light/Dark）并应用相应的颜色方案
- **平滑动画**：控件状态变化时的平滑过渡效果
- **圆角设计**：符合FluentUI3的圆角美学

## 技术实现

### 1. 代码来源

CusProxyStyle是基于Qt 6.10自带的Windows 11样式代码移植而来，在此基础上进行了大量的修复和优化：

- 修复了多个控件的显示问题
- 调整了控件大小和布局，使其更符合FluentUI3的设计规范
- 优化了动画效果和性能
- 增强了跨版本兼容性
- 其他问题

### 2. 继承QProxyStyle

CusProxyStyle继承自QProxyStyle，通过重写以下核心方法实现自定义样式：

- `drawComplexControl`：绘制复杂控件（如滑块、组合框）
- `drawPrimitive`：绘制基本元素（如边框、指示器）
- `drawControl`：绘制控件
- `subElementRect`：计算子元素位置
- `subControlRect`：计算子控件位置
- `styleHint`：提供样式提示
- `sizeFromContents`：计算控件大小
- `pixelMetric`：提供像素度量
- `polish`：应用样式到控件

### 3. 颜色体系

定义了完整的FluentUI3颜色体系，包括：

- Light主题颜色方案
- Dark主题颜色方案
- 各种状态下的控件颜色（默认、悬停、按下、禁用）
- 文本颜色
- 边框颜色

### 4. 图标支持

使用Segoe Fluent Icons字体作为图标源，实现了FluentUI3风格的图标显示。

### 5. 主题检测

自动检测系统主题设置，在Windows 11上使用系统API获取当前主题，在其他系统上使用默认Light主题。

## 代码结构

```
CusFluentUI3/
├── cusproxystyle.h  # 类定义和颜色枚举
└── cusproxystyle.cpp  # 实现文件
```

## 使用方法

1. 包含头文件：
   ```cpp
   #include "cusproxystyle.h"
   ```

2. 创建样式实例并应用到应用程序：
   ```cpp
   QApplication app(argc, argv);
   app.setStyle(new CusProxyStyle);
   ```

## 兼容性

- **Qt 6.6.3**：完全兼容，运行正常
- **Qt 5.15.2**：基本兼容，但ComboBox下拉框存在Bug，后续将修复

## 示例效果
<img width="1538" height="975" alt="screenshot-20260304-101705" src="https://github.com/user-attachments/assets/2cdc3f2d-d1a2-4c1b-87f3-156bfe3fe0ca" />
<img width="1538" height="975" alt="screenshot-20260304-103936" src="https://github.com/user-attachments/assets/fd89ce3d-9188-476e-a860-d02019489ca7" />
<img width="1538" height="975" alt="screenshot-20260304-103943" src="https://github.com/user-attachments/assets/eba2706f-e1f2-4921-b2f0-b498b4930bc8" />
<img width="1538" height="975" alt="screenshot-20260304-103954" src="https://github.com/user-attachments/assets/ae30836d-d765-48e3-b062-b90323508f21" />
<img width="1538" height="975" alt="screenshot-20260304-104007" src="https://github.com/user-attachments/assets/9dce1186-45d0-4fce-9ed4-bf0fd51c8a01" />

## 总结

CusProxyStyle是一个基于Qt 6.10 Windows 11样式代码移植并优化的FluentUI3风格实现。它不仅保留了原生样式的美观性，还通过修复问题和调整控件大小等方式，提供了更加完善和一致的用户体验。

通过使用CusProxyStyle，开发者可以轻松为Qt应用程序添加Windows 11风格的现代化界面，提升用户体验，同时享受跨版本兼容的便利。

---

**注意**：目前在Qt 5.15.2环境下，ComboBox下拉框存在Bug，后续修复。建议在Qt 6.6.3环境下使用以获得最佳效果。
