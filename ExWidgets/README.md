# ExWidgets 使用手册

**English:** [README_EN.md](README_EN.md)

ExWidgets 是本仓库在 **FluentUI3Style 样式库之外** 提供的一组 WinUI3 / CommunityToolkit 风格 **Qt Widgets 扩展控件**。  
它们可以独立编译链接，但 **推荐与 `app.setStyle("FluentUI3")` 一起使用**，以获得一致的 Fluent 视觉与交互。

Example 程序中 **ExWidgets** 导航分组下有各控件的交互演示，可作为活文档参考。

---

## 接入方式

### CMake

```cmake
add_subdirectory(path/to/Window11Style/ExWidgets)  # 或通过本仓库顶层工程

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE ExWidgets Qt6::Widgets)
```

头文件示例：

```cpp
#include "exspectrumwidget.h"
#include "exrangeslider.h"
```

### qmake

```qmake
include($$PWD/ExWidgets/ExWidgets.pri)   # 若项目提供；或参考 Example/Example.pro
LIBS += -lExWidgets
```

### 运行依赖

- Qt **5.14+** / **6.x**（Widgets）
- 链接 `ExWidgets` 共享库（或静态链接）
- 样式：建议启用 FluentUI3 插件，否则部分控件仅呈现默认 Fusion/Windows 绘制

---

## 控件一览

| 控件 | 头文件 | 说明 | Example 演示 |
|------|--------|------|--------------|
| `ExRangeSlider` | `exrangeslider.h` | WinUI3 双端范围滑块 | ExWidgets → ExRangeSlider |
| `ExColorPicker` | `excolorpicker.h` | CommunityToolkit 取色器 | ExWidgets → ExColorPicker |
| `ExColorPickerButton` | `excolorpickerbutton.h` | 带 Flyout 的取色按钮 | ExColorPicker 页内 |
| `ExMessageBox` | `exmessagebox.h` | Fluent 风格 `QMessageBox` | Dialogs 页 |
| `ExContentDialog` | `excontentdialog.h` | WinUI3 ContentDialog | Dialogs 页 |
| `ExSpectrumWidget` | `exspectrumwidget.h` | 实时音频频谱（Push PCM） | Qt5: ExSpectrumWidget；Qt6: Audiomatic Mini |
| `ExWinUINavigationView` | `exwinuinavigationview.h` | 主导航 + 页脚导航 | MainWindow 左侧导航 |
| `ExNavTreeWidget` | `exnavtreewidget.h` | 可折叠导航树 | 由 NavigationView 内部使用 |
| `ExStackedWidget` | `exstackedwidget.h` | 带动画的 `QStackedWidget` | MainWindow 中央区域 |
| `ExTabWidget` | `extabwidget.h` | 带动画的 `QTabWidget` | Tab 演示 |
| `ColorGradientSlider` | `colorgradientslider.h` | 渐变轨道滑条（ExColorPicker 内部） | ExColorPicker 内部 |

---

## ExRangeSlider

WinUI3 风格 **范围滑块**（lower / upper 双端），API 接近 `QAbstractSlider`。

### 基本用法

```cpp
#include "exrangeslider.h"

auto *slider = new ExRangeSlider(Qt::Horizontal, this);
slider->setRange(0, 100);
slider->setValues(20, 80);
slider->setTracking(true);          // 拖动时实时发信号
slider->setTickPosition(true);
slider->setTickInterval(10);

connect(slider, &ExRangeSlider::lowerValueChanged, this, [](int v) { /* ... */ });
connect(slider, &ExRangeSlider::upperValueChanged, this, [](int v) { /* ... */ });
```

### 常用属性

| 属性 / API | 说明 |
|------------|------|
| `minimum` / `maximum` / `setRange` | 数值范围 |
| `lowerValue` / `upperValue` / `setValues` | 当前区间 |
| `snapMode` | `NoSnap` / `SnapAlways` / `SnapOnRelease` |
| `tickPosition` / `tickInterval` | 刻度线 |
| `tracking` | 拖动过程是否持续触发 value 变化 |
| `first()` / `second()` | 对应 QML 的 first/second 节点对象 |

---

## ExColorPicker / ExColorPickerButton

对齐 **WinUI3 CommunityToolkit ColorPicker**：Spectrum / Palette / Sliders 三页，支持 Alpha、Hex、RGB/HSV。

### ExColorPicker（对话框 / 内嵌）

```cpp
#include "excolorpicker.h"

ExColorPicker picker(parent);
picker.setColor(Qt::blue);
picker.setAlphaEnabled(true);
picker.setColorSpectrumVisible(true);
picker.setColorPaletteVisible(true);

if (picker.exec() == QDialog::Accepted) {
    const QColor c = picker.color();
}
```

内嵌到页面时构造 `ExColorPicker(parent, /*popup=*/false)`，无需 `exec()`，监听 `colorChanged` 即可。

### ExColorPickerButton

```cpp
#include "excolorpickerbutton.h"

auto *btn = new ExColorPickerButton(this);
btn->setSelectedColor(QColor(0, 120, 212));
connect(btn, &ExColorPickerButton::selectedColorChanged, this, [](const QColor &c) {
    // 用户从 Flyout 选色完成
});
```

---

## ExMessageBox

基于 **`QMessageBox`** 子类，WinUI3 视觉；API 与静态方法兼容 Qt 习惯。

```cpp
#include "exmessagebox.h"

ExMessageBox::information(this, tr("标题"), tr("内容"));
ExMessageBox::warning(this, tr("警告"), tr("请注意"));
ExMessageBox::critical(this, tr("错误"), tr("发生错误"));

const auto ret = ExMessageBox::question(
    this, tr("确认"), tr("是否继续？"),
    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
```

可像普通 `QMessageBox` 一样设置 `setInformativeText`、`setCheckBox` 等（参见 Example → Dialogs）。

---

## ExContentDialog

WinUI3 **ContentDialog** 模态框：标题、正文、Primary / Secondary / Close 按钮。

```cpp
#include "excontentdialog.h"

ExContentDialog dlg(parent);
dlg.setTitle(tr("删除文件"));
dlg.setContent(tr("此操作不可撤销。"));
dlg.setPrimaryButtonText(tr("删除"));
dlg.setSecondaryButtonText(tr("取消"));
dlg.setCloseButtonText(tr("关闭"));
dlg.setDefaultButton(ExContentDialog::PrimaryBtn);

const ExContentDialog::ContentDialogResult r = dlg.showDialog();
if (r == ExContentDialog::Primary) { /* 确认 */ }
```

---

## ExSpectrumWidget

**通用 Push 型频谱控件**：接收 mono **16-bit PCM**（little-endian），内部 FFT + 对数频带 + Attack/Decay 绘制。  
**不依赖 Qt Multimedia**，Qt 5 / 6 行为一致。

### 数据契约

| 项目 | 约定 |
|------|------|
| 格式 | 16-bit PCM，**单声道**，little-endian |
| 调用 | `pushAudioData()` **追加**数据（非整段替换） |
| 线程 | **建议在 GUI 线程**调用 |
| 采样率 | 必须 `setSampleRate()` 与实际 PCM 一致（默认 44100） |
| 输出 | `bars()` / `barsChanged()` 为 0.0 ~ 1.0 归一化柱高 |

### 标准用法

```cpp
#include "exspectrumwidget.h"

auto *spectrum = new ExSpectrumWidget(this);
spectrum->setSampleRate(44100);
spectrum->setBarCount(64);                    // 可选，默认 64
spectrum->setRefreshIntervalMs(16);           // 可选，~60 FPS

// 周期性推送 PCM（例如每 16ms 一帧）
connect(source, &MyAudioSource::pcmReady,
        spectrum, &ExSpectrumWidget::pushAudioData);

// 兼容 slot 名（信号槽 / Designer）
spectrum->setAudioData(pcmChunk);

// 停止播放时
spectrum->clear();
```

### Q_PROPERTY（Designer / QML 绑定）

- `sampleRate`、`barCount`、`barColor`、`refreshIntervalMs`

### 演示数据来源

- **Example（Qt 5）**：`SineWaveGenerator` 合成动画 PCM → `pushAudioData`
- **Example（Qt 6）**：Audiomatic Mini 播放器解码后推送
- **独立程序**：`SpectrumDemo`（Qt 6，见 `AudiomaticMini/`）

---

## ExWinUINavigationView / ExNavTreeWidget

Example 主窗口左侧 **WinUI3 导航窗格**：主菜单 + 分隔线 + 页脚项，配合 `QStackedWidget` 切页。

```cpp
#include "exwinuinavigationview.h"

auto *nav = new ExWinUINavigationView(this);
nav->setStackedWidget(ui->stackedWidget);

nav->addMainNavigationItem(tr("首页"), 0, QStringLiteral("\uE80F"));
nav->addFooterNavigationItem(tr("设置"), 5, QStringLiteral("\uE713"));

connect(nav, &ExWinUINavigationView::pageIndexChanged, ui->stackedWidget, &QStackedWidget::setCurrentIndex);
nav->setNavigationExpanded(true, /*animated=*/true);
```

`ExNavTreeWidget` 通常由 `ExWinUINavigationView` 内部创建；若单独使用，可调用 `addNavigationItem` / `configureNavigationItem`。

---

## ExStackedWidget / ExTabWidget

在 `QStackedWidget` / `QTabWidget` 基础上增加 **滑动切换动画**（Example 中央 `stackedWidget` 使用）。

```cpp
#include "exstackedwidget.h"

auto *stack = new ExStackedWidget(this);
stack->setVerticalMode(true);              // true=上下滑动，false=左右
stack->setSpeed(300);                      // 毫秒
stack->setAnimation(QEasingCurve::InOutSine);
stack->setCurrentIndex(1);
```

`ExTabWidget` API 类似，在 `currentChanged` 时自动播放过渡动画。

---

## ColorGradientSlider

`QSlider` 子类，轨道绘制为 **动态渐变图像**（主要用于 `ExColorPicker` 内部 Hue/Saturation 滑条）。  
若单独使用：

```cpp
#include "colorgradientslider.h"

auto *s = new ColorGradientSlider(Qt::Horizontal, this);
s->setImageBuilder([](QSize size) {
    QImage img(size, QImage::Format_RGB32);
    // 填充渐变...
    return img;
});
```

---

## 与 FluentUI3Style 的关系

- ExWidgets 负责 **控件逻辑与自绘**；标准 Qt 控件的外观仍由 **FluentUI3Style** 绘制。
- ExWidgets 内部分控件（如 `ExRangeSlider`、`ExSpectrumWidget`）使用 `QPalette` / Fluent 色板自绘，**不依赖** Style 的 `QStyle` 分支，但配色与 WinUI3 一致。
- 项目定位见根目录 [README.md](../README.md)「项目定位说明」。

---

## 版本与平台说明

| 能力 | Qt 5 | Qt 6 |
|------|------|------|
| ExWidgets 全部控件 | ✅ | ✅ |
| Example → Audiomatic Mini 播放器 | ❌ | ✅（需 Multimedia） |
| Example → ExSpectrumWidget 模拟频谱 | ✅ | ✅（Qt6 为完整播放器页） |

---

## 更多信息

- 构建与插件部署：根目录 [README.md](../README.md)
- 交互演示：编译并运行 `Example`，打开左侧 **ExWidgets** 分组
- 变更记录：`Example/changelog.txt`
