# ExWidgets User Guide

**中文:** [README.md](README.md)

ExWidgets are **WinUI3 / CommunityToolkit–style Qt Widget extensions** shipped alongside FluentUI3Style. They can be used standalone, but **`app.setStyle("FluentUI3")` is recommended** for a consistent look.

The **Example** app includes live demos under the **ExWidgets** navigation group.

---

## Linking (CMake)

```cmake
target_link_libraries(MyApp PRIVATE ExWidgets Qt6::Widgets)
```

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
| `ExSpectrumWidget` | `exspectrumwidget.h` | Real-time spectrum (push mono int16 PCM) |
| `ExWinUINavigationView` | `exwinuinavigationview.h` | Navigation pane + footer |
| `ExNavTreeWidget` | `exnavtreewidget.h` | Collapsible nav tree |
| `ExStackedWidget` | `exstackedwidget.h` | Animated stacked pages |
| `ExTabWidget` | `extabwidget.h` | Animated tab widget |
| `ColorGradientSlider` | `colorgradientslider.h` | Gradient groove slider |

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
| Example Audiomatic Mini player | No | Yes |
| Example spectrum (simulated PCM) | Yes | Yes |

Build instructions: [README_EN.md](../README_EN.md)
