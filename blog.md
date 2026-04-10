# CusProxyStyle：基于Qt的FluentUI3风格实现

## 项目简介

CusProxyStyle是一个基于Qt的自定义样式类，旨在模仿Windows 11的FluentUI3设计风格，为Qt应用程序提供现代化的界面外观。

## 核心特性

- **完整的FluentUI3风格实现**：包括按钮、滑块、复选框、单选按钮、组合框等控件的样式
- **主题支持**：自动检测系统主题（Light/Dark）并应用相应的颜色方案
- **平滑动画**：控件状态变化时的平滑过渡效果
- **圆角设计**：符合FluentUI3的圆角美学
- **高对比度模式**：支持系统高对比度设置

## 技术实现

### 1. 代码来源

CusProxyStyle是基于Qt 6.10自带的Windows 11样式代码移植而来，在此基础上进行了大量的修复和优化：

- 修复了多个控件的显示问题
- 调整了控件大小和布局，使其更符合FluentUI3的设计规范
- 优化了动画效果和性能
- 增强了跨版本兼容性

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

### 按钮
具有圆角设计和状态变化效果的FluentUI3风格按钮：

![FluentUI3按钮示例](https://trae-api-cn.mchost.guru/api/ide/v1/text_to_image?prompt=Fluent%20UI%203%20style%20buttons%20with%20rounded%20corners%2C%20showing%20normal%2C%20hover%2C%20and%20pressed%20states%2C%20light%20theme%2C%20clean%20UI%20design&image_size=square_hd)

### 滑块
现代风格的滑块设计，带有平滑的拖动动画：

![FluentUI3滑块示例](https://trae-api-cn.mchost.guru/api/ide/v1/text_to_image?prompt=Fluent%20UI%203%20style%20slider%20with%20modern%20design%2C%20blue%20accent%20color%2C%20light%20theme%2C%20clean%20UI&image_size=square_hd)

### 复选框和单选按钮
符合FluentUI3风格的复选框和单选按钮：

![FluentUI3复选框和单选按钮示例](https://trae-api-cn.mchost.guru/api/ide/v1/text_to_image?prompt=Fluent%20UI%203%20style%20checkboxes%20and%20radio%20buttons%20in%20different%20states%2C%20light%20theme%2C%20clean%20UI%20design&image_size=square_hd)

### 组合框
现代风格的下拉框设计：

![FluentUI3组合框示例](https://trae-api-cn.mchost.guru/api/ide/v1/text_to_image?prompt=Fluent%20UI%203%20style%20combobox%20with%20dropdown%20menu%2C%20light%20theme%2C%20clean%20UI%20design&image_size=square_hd)

### 滚动条
简洁的滚动条设计：

![FluentUI3滚动条示例](https://trae-api-cn.mchost.guru/api/ide/v1/text_to_image?prompt=Fluent%20UI%203%20style%20scrollbar%20with%20modern%20design%2C%20light%20theme%2C%20clean%20UI&image_size=square_hd)

## 后续计划
1. 增加更多FluentUI3控件的支持
2. 进一步优化动画效果和性能
3. 提供更多主题定制选项

## 总结

CusProxyStyle是一个基于Qt 6.10 Windows 11样式代码移植并优化的FluentUI3风格实现。它不仅保留了原生样式的美观性，还通过修复问题和调整控件大小等方式，提供了更加完善和一致的用户体验。

通过使用CusProxyStyle，开发者可以轻松为Qt应用程序添加Windows 11风格的现代化界面，提升用户体验，同时享受跨版本兼容的便利。

---

**注意**：目前在Qt 5.15.2环境下，ComboBox下拉框存在Bug，正在修复中。建议在Qt 6.6.3环境下使用以获得最佳效果。