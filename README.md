# FluentUI3Style：基于Qt的FluentUI3风格实现

## 预览

<img width="1528" height="1148" alt="screenshot-20260323-092142" src="https://github.com/user-attachments/assets/730aa691-bc6e-4453-99de-9ad83ac0c3e0" />
<img width="1528" height="1148" alt="screenshot-20260323-092155" src="https://github.com/user-attachments/assets/b7a94eb7-a137-41fe-8590-5935adb167aa" />

## 项目简介

FluentUI3Style于继承QProxyStyle实现，实现FluentUI3UI风格，使用到项目中超简单。

```cpp
QApplication app(argc, argv);
app.setStyle(new FluentUI3Style);
```

[git地址](https://github.com/XHY-ChuJian/FluentUIStyle.git)

## 核心特性

- **完整的FluentUI3风格实现**：包括按钮、滑块、复选框、单选按钮、组合框等控件的样式
- **主题支持**：自动检测系统主题（Light/Dark）并应用相应的颜色方案
- **平滑动画**：控件状态变化时的平滑过渡效果
- **圆角设计**：符合FluentUI3的圆角美学

## 技术实现

### 1. 代码来源

FluentUI3Style是基于Qt 6.10自带的Windows 11样式代码移植而来，在此基础上进行了大量的修复和优化：

- 修复了多个控件的显示问题
- 调整了控件大小和布局，使其更符合FluentUI3的设计规范
- 优化了动画效果和性能
- 增强了跨版本兼容性
- 其他问题

### 2. 继承QProxyStyle

FluentUI3Style继承自QProxyStyle，通过重写以下核心方法实现自定义样式：

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
FluentUI3Style/
├── FluentUI3Style.h  # 类定义和颜色枚举
└── FluentUI3Style.cpp  # 实现文件
```

## 使用方法

1. 包含头文件：
   ```cpp
   #include "FluentUI3Style.h"
   ```
2. 创建样式实例并应用到应用程序：
   ```cpp
   QApplication app(argc, argv);
   app.setStyle(new FluentUI3Style);
   ```

## 示例效果

<img width="1783" height="1148" alt="screenshot-20260323-092936" src="https://github.com/user-attachments/assets/9e1391c5-fce2-490c-8689-0aef6411eb06" />
<img width="1783" height="1148" alt="screenshot-20260323-092954" src="https://github.com/user-attachments/assets/e692177d-c5b3-4661-8639-02f568471616" />
<img width="1783" height="1148" alt="screenshot-20260323-093001" src="https://github.com/user-attachments/assets/6cb5f5ce-e136-4c3d-863a-e00ec6164a70" />
<img width="1783" height="1148" alt="screenshot-20260323-093016" src="https://github.com/user-attachments/assets/a803a250-9b52-4029-b56b-6432898b75e8" />
<img width="1538" height="975" alt="screenshot-20260304-104007" src="https://github.com/user-attachments/assets/9dce1186-45d0-4fce-9ed4-bf0fd51c8a01" />
