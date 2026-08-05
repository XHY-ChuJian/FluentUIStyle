# ExWidgets User Guide

**中文:** [README.md](README.md)

ExWidgets are **WinUI3 / CommunityToolkit–style Qt Widget extensions** shipped alongside FluentUI3Style. They can be used standalone, but **`app.setStyle("FluentUI3")` is recommended** for a consistent look.

The **Gallery** app includes live demos under the **ExWidgets** navigation group.

---

## Linking (CMake)

```cmake
target_link_libraries(MyApp PRIVATE ExWidgets::ExWidgets Qt6::Widgets)
```

For the optional QWindowKit-based window chrome:

```cmake
set(EXWIDGETS_BUILD_FRAMELESS ON CACHE BOOL "" FORCE)
add_subdirectory(path/to/Window11Style/ExWidgets)
target_link_libraries(MyApp PRIVATE ExWidgets::Frameless)
```

Installed packages support `find_package(ExWidgets CONFIG REQUIRED COMPONENTS Frameless)`. The option defaults to ON with Qt ≥ 5.15.2 and OFF with older Qt; explicitly setting it OFF keeps base `ExWidgets` free of QWindowKit.

```cpp
#include "exspectrumwidget.h"
#include "exrangeslider.h"
```

---

## Widget overview

| Widget | Header | Description |
|--------|--------|-------------|
| `ExRangeSlider` | `exrangeslider.h` | Dual-handle range slider |
| `ExColorPicker` | `excolorpicker.h` | Full color picker dialog |
| `ExColorPickerButton` | `excolorpickerbutton.h` | Tool button + flyout picker |
| `ExMessageBox` | `exmessagebox.h` | Fluent-styled `QMessageBox` |
| `ExContentDialog` | `excontentdialog.h` | WinUI3 ContentDialog |
| `ExTimerDial` | `extimerdial.h` | Remaining time, circular progress, and optional finish time |
| `ExLiquidGauge` | `exliquidgauge.h` | Ant Design-inspired liquid gauge with four shapes, layered waves, and centered text |
| `ExRadialGauge` / `ExRadialGaugeRange` | `exradialgauge.h` | Interactive radial gauge with configurable angles, ticks, needle, and colored ranges |
| `ExSpectrumWidget` | `exspectrumwidget.h` | Real-time spectrum (push mono int16 PCM) |
| `ExWinUINavigationView` | `exwinuinavigationview.h` | Navigation pane + footer |
| `ExNavTreeWidget` | `exnavtreewidget.h` | Collapsible nav tree |
| `ExStackedWidget` | `exstackedwidget.h` | Animated stacked pages |
| `ExTabWidget` | `extabwidget.h` | Animated tab widget |
| `ColorGradientSlider` | `colorgradientslider.h` | Gradient groove slider |
| `FluentTitleBar` / `FluentWindowFrame` | `fluenttitlebar.h` / `fluentwindowframe.h` | Optional QWindowKit window chrome |

See [README.md](README.md) for detailed API examples (Chinese).

---

## ExSpectrumWidget (quick reference)

**Input:** mono **16-bit PCM**, little-endian, appended via `pushAudioData()`.

**Threading:** call from the **GUI thread**.

```cpp
auto *spectrum = new ExSpectrumWidget(this);
spectrum->setSampleRate(44100);
connect(source, &MySource::pcmReady, spectrum, &ExSpectrumWidget::pushAudioData);
spectrum->clear();  // on stop
```

---

## Platform notes

| Feature | Qt 5 | Qt 6 |
|---------|------|------|
| All ExWidgets | Yes | Yes |
| Gallery Audiomatic Mini player | No | Yes |
| Gallery spectrum (simulated PCM) | Yes | Yes |

Build instructions: [README_EN.md](../README_EN.md)
