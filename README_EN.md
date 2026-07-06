
# FluentUI3Style: Fluent UI 3 look for Qt

[中文版](README.md)

## Preview

<img width="1920" height="1094" alt="Preview 1" src="https://github.com/user-attachments/assets/2e5d044d-7f2a-41cd-a76d-e5faf3cbc140" />

<img width="2438" height="1390" alt="Preview 2" src="https://github.com/user-attachments/assets/d1180649-0722-4824-8158-3771debb4a8f" />

<img width="2438" height="1390" alt="Preview 3" src="https://github.com/user-attachments/assets/5d813ac2-3dd8-4ba9-8e60-c823fcf26c5f" />

<img width="2438" height="1390" alt="Preview 4" src="https://github.com/user-attachments/assets/6e297f08-d381-4725-891d-3e9c057563f3" />

<img width="2247" height="1407" alt="Preview 5" src="https://github.com/user-attachments/assets/e59e6846-a9f5-4349-9a30-09251308f00c" />

<img width="2468" height="1390" alt="Preview 6" src="https://github.com/user-attachments/assets/4f0c8084-864e-409c-afc2-98918ff2ec36" />

## Qt Designer

<img width="2616" height="1234" alt="Qt Designer" src="https://github.com/user-attachments/assets/9f3526d0-cd38-46ee-a325-0c667a11d341" />

## Overview

FluentUI3Style is built on `QProxyStyle` and implements a full Fluent UI 3 visual language for Qt. It is easy to adopt: build the project as a Qt style plugin and apply it with `app.setStyle("FluentUI3")`—no manual DLL loading or linking style sources into every app.

### What this project is

This repository is a **style library**. The goal is to make standard Qt widgets look and behave as close as practical to Fluent UI (WinUI 3).

Because of Qt’s widget model, not every WinUI control can be reproduced exactly; where possible, the style customizes painting and interaction on existing widgets—for example:

- Switch-style `QCheckBox` (“SwitchButton”)
- `QTabBar` variants such as **Pivot** and **Segmented**

The **Example** app may include extra widgets under **ExWidgets** for demonstration; those widgets are still drawn using the same style engine. More samples may be added over time, and those widgets are intended to remain usable without the style if needed.

**ExWidgets usage guide (linking, APIs, PCM format for ExSpectrumWidget):** [ExWidgets/README_EN.md](ExWidgets/README_EN.md) ([中文](ExWidgets/README.md))

If you try to restyle everything with **QSS** alone, you will often **override** the `QStyle` painting and lose this library’s look. For deep customization, extending or subclassing `QStyle` (as this project does) is the right level—but that is advanced and not the primary goal here.

If you want a **consistent** Fluent look with small tweaks on top, this project is a good fit. It targets teams that already have Qt apps and want Fluent UI styling with minimal integration cost.

**Note on “custom controls”:** Without modifying this repo’s sources, there is no separate “custom widget SDK”—usage matches built-in styles such as `windowsvista`, `Windows`, and `Fusion`. To customize further, use the same approaches you would for any Qt app (QSS or a custom `QStyle`). More tuning via `setProperty` on widgets is planned to expose additional knobs.

Happy coding.

### Plugin benefits

- **Auto deploy:** CMake / qmake builds copy the style plugin into the correct Qt `plugins/styles` directory.
- **Drop-in:** Call `app.setStyle("FluentUI3")` after `QApplication` starts.
- **No extra linkage:** Your app does not need to link the style sources directly when using the plugin.

### Notes

- **Tested versions:** Qt 5.14.2, Qt 5.15.2, Qt 6.5.3 / 6.6.3 (MSVC).
- **Qt 6.8+ known issue:** **Drop shadows** on **Popup** surfaces such as `QMenu` and `QComboBox` lists may look wrong on Qt **6.8 and newer** (missing, clipped, dirty edges, or misaligned). FluentUI3Style turns off the native shadow (`Qt::NoDropShadowWindowHint`) and uses `WA_TranslucentBackground` so the style can **paint multi-layer shadows in `QStyle`**. From Qt 6.8 onward, the Windows platform changed how **translucent popup windows** are composited (paintable region, alpha blending, stacking). That no longer matches the assumptions of the hand-drawn shadow path, so shadows that looked correct on Qt 6.6 and earlier can break. This is a **Qt platform + custom shadow** interaction, not a single-widget logic bug; adaptation for newer Qt releases is ongoing.
- **MinGW:** Menus may need extra handling in some MinGW setups.
- **Version differences:** Mostly visible in context menus (rendering/layout nuances).
- **Compatibility:** Qt has many versions; full parity everywhere is unrealistic, but recent stable Qt releases are the priority.

### Qt compatibility matrix

| Qt version | Status | Notes |
| ---------- | ------ | ----- |
| Qt 5.14.2  | Supported | Tested |
| Qt 5.15.2  | Supported | Tested |
| Qt 6.6.3   | Supported | Tested |
| Qt 6.8+    | Partial | Popup menu/dropdown **shadows** may be wrong (see note above) |
| Qt 6.10    | Partial | Same shadow issue; style sources are ported from Qt 6.10 Win11 style |

## Build (detailed)

### Get the source (including submodules)

The Example app’s frameless window and DWM backdrop use **[QWindowKit](https://github.com/stdware/qwindowkit)**, vendored as a Git **submodule** at `3rd/qwindowkit/`.  
A plain `git clone` does **not** fetch submodule contents; an empty or missing directory will break CMake.

#### First-time clone (recommended)

```powershell
git clone --recursive https://github.com/XHY-ChuJian/FluentUIStyle.git
cd FluentUIStyle
```

#### Already cloned the repo

```powershell
git pull
git submodule update --init --recursive
```

> **Note:** `git pull` alone does **not** update submodules. If CMake cannot find `3rd/qwindowkit`, run `git submodule update --init --recursive` again.

Other third-party code (e.g. `3rd/kissfft`) is committed directly in this repo.

---

### Prerequisites

- OS: Windows 10/11 recommended.
- Compiler: MSVC matching your Qt kit.
- Qt: a tested version (e.g. Qt 6.6.3) is recommended.
- CMake: 3.16+.

> Keep `cmake`, `ninja` (if used), and the Qt toolchain in one consistent environment to avoid “moc vs headers version mismatch” issues.

---

### Build with CMake (recommended)

#### 1) Open a shell in the repo

```powershell
cd D:/workspace/Code/Github/Window11Style
```

#### 2) Configure

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="D:/app/Qt/Qt6.6.3/6.6.3/msvc2019_64"
```

You can also point CMake at Qt explicitly:

```powershell
-DQt6_DIR="D:/app/Qt/Qt6.6.3/6.6.3/msvc2019_64/lib/cmake/Qt6"
```

#### 3) Build

```powershell
cmake --build build --config Debug
```

Release:

```powershell
cmake --build build --config Release
```

#### 4) Run the example

```powershell
./build/bin/Exampled.exe   # Debug
./build/bin/Example.exe    # Release
```

#### 5) Main CMake options

- `BUILD_LIBRARY` — build the style library (default **ON**).
- `BUILD_PLUGIN` — build the Qt style plugin (default **ON**).
- `BUILD_EXAMPLE` — build the demo app (default **ON**).

Example: library + plugin only, no demo:

```powershell
cmake -S . -B build -DBUILD_EXAMPLE=OFF
```

---

### Build with qmake

The repo provides `fluentw3uistyle.pro` (SUBDIRS project):

```powershell
cd D:/workspace/Code/Github/Window11Style
qmake fluentw3uistyle.pro
nmake
```

Or open `fluentw3uistyle.pro` in Qt Creator and build with your kit.

---

### Plugin deployment

When the plugin target is enabled, the build copies the style plugin into Qt’s `plugins/styles`. In custom setups you can also copy the built plugin manually into the target Qt’s `plugins/styles` folder.

## How to use

## Integrating into your project (recommended)

Typical flow to adopt **FluentUI3Style** in your own Qt application.

### 1) Build and deploy the plugin

Follow the CMake or qmake steps above. After a successful build, the plugin is copied to:

- `QT_INSTALL_PLUGINS/styles`

The property header is copied to:

- `QT_INSTALL_HEADERS/FluentUI3Style/fluentui3styleproperties.h`

> If Qt is installed under a protected path (e.g. `Program Files`), the copy step may require elevation.

### 2) Enable the style at runtime

```cpp
QApplication app(argc, argv);
app.setStyle("FluentUI3");
```

### 3) Use property enums in code (recommended)

```cpp
#include <FluentUI3Style/fluentui3styleproperties.h>
// or: #include "fluentui3styleproperties.h"
```

Prefer `setProperty` with named constants instead of raw numeric literals.

### 4) Minimal CMake / qmake snippets

#### CMake (sketch)

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
add_executable(MyApp main.cpp mainwindow.cpp)
target_link_libraries(MyApp PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
```

#### qmake (sketch)

```qmake
QT += core gui widgets
SOURCES += main.cpp mainwindow.cpp
```

As long as the `FluentUI3` plugin is discoverable under `plugins/styles`, your app does **not** need to link `FluentUI3Style` explicitly to use `app.setStyle("FluentUI3")`.

### Option A: Instantiate the style in code

```cpp
#include "fluentui3style.h"

QApplication app(argc, argv);
app.setStyle(new FluentUI3Style);
```

Use when:

- You integrate the style as source or a static/shared library.
- You do not rely on Qt’s plugin loader.

### Option B: Load by style name (recommended for plugins)

```cpp
QApplication app(argc, argv);
app.setStyle("FluentUI3");
```

Use when:

- The `FluentUI3` plugin is built and deployed.
- You want the smallest change in the host project.

### Option C: FluentUIAppearance (optional)

```cpp
#include "fluentuiappearance.h"

FluentUIAppearance::instance()->initialize();

QApplication app(argc, argv);
```

Use when:

- You want a richer initialization path for theme / color behavior.

## Supported widgets

FluentUI3 styling is driven mainly by **dynamic properties** on standard widgets:

| Category | Widget | Notes | Properties |
| -------- | ------ | ----- | ---------- |
| Buttons | `QPushButton` | Regular push button | |
| Buttons | `QCheckBox` | Check box | `switchButton=true` enables switch styling |
| Buttons | `QRadioButton` | Radio button | |
| Input | `QLineEdit` | Single-line edit | Bottom-line animation |
| Input | `QTextEdit` | Rich text editor | |
| Input | `QPlainTextEdit` | Plain text editor | |
| Input | `QSpinBox` | Integer spin box | `spinBoxButtonLayout`: `0` vertical arrows (default), `1` horizontal both sides, `2` horizontal right, `3` horizontal plus/minus both sides |
| Input | `QDoubleSpinBox` | Floating spin box | Same `spinBoxButtonLayout` as `QSpinBox` |
| Selection | `QComboBox` | Combo box | Popup animation and shadow |
| Selection | `QListWidget` | List widget | Selection indicator animation |
| Selection | `QListView` | List view | Selection indicator animation |
| Slider | `QSlider` | Slider | Horizontal and vertical |
| Progress | `QProgressBar` | Progress bar | `progressBarStyle`: `0` thin (default), `1` thick, `2` ring |
| Tabs | `QTabBar` | Tab bar | `tabBarStyle`: `1` Capsule, `2` Pivot_Grow, `3` Pivot_Slide, `4` Pivot_Stretch, `5` PillTabs, `6` Segmented_Slide, `7` Segmented_Fade, `8` Navigation, `9` Segmented_WinUI3 |
| Tabs | `QTabWidget` | Tab widget | Inherits tab bar styling |
| Scroll | `QScrollBar` | Scroll bar | Horizontal and vertical |
| Scroll | `QScrollArea` | Scroll area | |
| Menus | `QMenu` | Context menu | Shadow |
| Menus | `QMenuBar` | Menu bar | |
| Dialogs | `QMessageBox` | Message box | |
| Dialogs | `QFileDialog` | File dialog | |
| Toolbar | `QToolButton` | Tool button | Menu-arrow animation |
| Toolbar | `QToolBar` | Tool bar | |
| Tree | `QTreeView` | Tree / navigation | Fluent navigation look |
| Table | `QTableView` | Table view | |
| Table | `QTableWidget` | Table widget | |

## Usage examples

### Property examples (from Example / MainWindow)

Prefer constants from `fluentui3styleproperties.h` instead of magic numbers.

```cpp
#include <FluentUI3Style/fluentui3styleproperties.h>

// Progress bar: thick style
ui->progressBar->setProperty(ProgressBarStyleProperty, ProgressBarThick);

// Tab bar: WinUI3 segmented style
ui->tabBar->setProperty(TabBarStyleProperty, Segmented_WinUI3);

// Tab bar: other styles
pillBar->setProperty(TabBarStyleProperty, PillTabs);
capTabBar->setProperty(TabBarStyleProperty, Capsule);
navTabBar->setProperty(TabBarStyleProperty, Navigation);

// Spin box button layout (property name is still a string here)
ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsVertical);
ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsHorizontalSides);
ui->spinBox->setProperty("spinBoxButtonLayout", ArrowsHorizontalRight);
ui->spinBox->setProperty("spinBoxButtonLayout", PlusMinusHorizontalSides);

// Check box as switch
checkBox->setProperty(SwitchStyleProperty, true);
```

Quick reference:

- `TabBarStyleProperty` — `QTabBar` variants (Capsule / Pivot / Segmented / Navigation).
- `ProgressBarStyleProperty` — `QProgressBar` (thin / thick / ring).
- `SwitchStyleProperty` — switch-style `QCheckBox`.
- `ButtonAccentStyleProperty` — accent button styling.
- `NavigationViewStyleProperty` — navigation indicator styling.
- `NoRoundedCorners` — disable rounded corners where supported.

### Method 1: Direct style instance

```cpp
#include "fluentui3style.h"

QApplication app(argc, argv);
app.setStyle(new FluentUI3Style);
```

### Method 2: Plugin name

```cpp
QApplication app(argc, argv);
app.setStyle("FluentUI3");
```

### Method 3: FluentUIAppearance

```cpp
#include "fluentuiappearance.h"

FluentUIAppearance::instance()->initialize();

QApplication app(argc, argv);
```

## Technical notes

### 1. Code lineage

FluentUI3Style started from Qt 6.10’s built-in Windows 11 style sources, with substantial fixes and tuning:

- Visual fixes across multiple widgets.
- Sizes and layout aligned closer to Fluent UI 3.
- Animation and performance improvements.
- Broader Qt version compatibility work.

### 2. Color system

Includes a structured Fluent UI 3 palette:

- Light and dark schemes.
- State colors (normal, hover, pressed, disabled).
- Text and border colors.
- Fluent and Teams color schemes.

### 3. Icons

**Segoe Fluent Icons** is used as the icon font for Fluent-style glyphs.

### 4. Theme detection

The style can follow the OS theme on Windows 11 via system APIs; on other platforms it falls back to a sensible light default.

## CMake options (summary)

- **`BUILD_LIBRARY`** — build the static style library (default **ON**).
- **`BUILD_PLUGIN`** — build the Qt style plugin (default **ON**).
- **`BUILD_EXAMPLE`** — build the demo application (default **ON**).

## Demo gallery

(Refer to the screenshots at the top of this document.)

## Roadmap

- More Fluent UI 3 coverage on standard widgets.
- Stronger theme customization hooks.
- Further performance and animation polish.
- Additional color schemes.

## License

FluentUI3Style is released under the **MIT License**. You may use it in any kind of project, but **any redistribution must retain this project’s MIT license text**. Redistribution without the license notice is not permitted and may infringe the license terms.
