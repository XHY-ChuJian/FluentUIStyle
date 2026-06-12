#include "dwmbackdrop.h"

#include <QApplication>
#include <QSettings>
#include <QWidget>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

namespace DwmBackdrop {

#ifdef Q_OS_WIN

namespace {

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

enum class SystemBackdropType : DWORD {
    Auto = 0,
    None = 1,
    MainWindow = 2,
    TransientWindow = 3,
    TabbedWindow = 4,
};

enum class AccentState : DWORD {
    Disabled = 0,
    EnableBlurBehind = 3,
    EnableAcrylicBlurBehind = 4,
};

enum class WindowCompositionAttrib : DWORD {
    AccentPolicy = 19,
};

struct AccentPolicy {
    AccentState accentState;
    DWORD accentFlags;
    DWORD gradientColor;
    DWORD animationId;
};

struct WindowCompositionAttribData {
    WindowCompositionAttrib attrib;
    PVOID data;
    ULONG dataSize;
};

using SetWindowCompositionAttributePtr = BOOL(WINAPI *)(HWND, WindowCompositionAttribData *);

SetWindowCompositionAttributePtr resolveSetWindowCompositionAttribute()
{
    static const auto proc = reinterpret_cast<SetWindowCompositionAttributePtr>(
        ::GetProcAddress(::GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
    return proc;
}

bool isWindows11OrGreater()
{
    const QSettings settings(
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        QSettings::NativeFormat);
    return settings.value(QStringLiteral("CurrentBuildNumber"), 0).toInt() >= 22000;
}

bool isDarkTheme()
{
    const QVariant scheme = qApp->property("_q_colorscheme");
    if (scheme.isValid())
    {
        return scheme.toInt() == 1;
    }

    const QSettings settings(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return settings.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
}

HWND hwndFor(QWidget *window)
{
    if (!window)
    {
        return nullptr;
    }

    window->winId();
    return reinterpret_cast<HWND>(window->winId());
}

bool setAccentPolicy(HWND hwnd, AccentState state, DWORD gradientColor = 0)
{
    const auto setWindowCompositionAttribute = resolveSetWindowCompositionAttribute();
    if (!setWindowCompositionAttribute)
    {
        return false;
    }

    AccentPolicy policy{};
    policy.accentState = state;
    if (state == AccentState::EnableAcrylicBlurBehind)
    {
        policy.accentFlags = 0x2; // ACCENT_ENABLE_ACRYLIC_WITH_LUMINOSITY
        policy.gradientColor = gradientColor;
    }

    WindowCompositionAttribData data{};
    data.attrib = WindowCompositionAttrib::AccentPolicy;
    data.data = &policy;
    data.dataSize = sizeof(policy);
    return setWindowCompositionAttribute(hwnd, &data) != FALSE;
}

bool setSystemBackdrop(HWND hwnd, SystemBackdropType type)
{
    const auto backdropType = static_cast<DWORD>(type);
    return SUCCEEDED(::DwmSetWindowAttribute(hwnd,
                                               DWMWA_SYSTEMBACKDROP_TYPE,
                                               &backdropType,
                                               sizeof(backdropType)));
}

bool extendFrame(HWND hwnd, const MARGINS &margins)
{
    return SUCCEEDED(::DwmExtendFrameIntoClientArea(hwnd, &margins));
}

DWORD acrylicGradientColor(bool darkTheme)
{
    // DWM accent gradient uses 0xAABBGGRR.
    return darkTheme ? 0x99202020u : 0x99F3F3F3u;
}

bool enableWin10Blur(HWND hwnd)
{
    static constexpr MARGINS kZeroMargins{0, 0, 0, 0};
    extendFrame(hwnd, kZeroMargins);
    setSystemBackdrop(hwnd, SystemBackdropType::None);
    return setAccentPolicy(hwnd, AccentState::EnableBlurBehind);
}

bool enableWin11Acrylic(HWND hwnd)
{
    static constexpr MARGINS kFullMargins{-1, -1, -1, -1};
    extendFrame(hwnd, kFullMargins);

    const BOOL useDarkMode = isDarkTheme() ? TRUE : FALSE;
    ::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

    setSystemBackdrop(hwnd, SystemBackdropType::TransientWindow);

    return setAccentPolicy(hwnd,
                           AccentState::EnableAcrylicBlurBehind,
                           acrylicGradientColor(isDarkTheme()));
}

void disableBackdrop(HWND hwnd)
{
    static constexpr MARGINS kShadowMargins{1, 1, 1, 1};
    extendFrame(hwnd, kShadowMargins);
    setSystemBackdrop(hwnd, SystemBackdropType::Auto);
    setAccentPolicy(hwnd, AccentState::Disabled);
}

} // namespace

#endif // Q_OS_WIN

bool enable(QWidget *window)
{
#ifdef Q_OS_WIN
    const HWND hwnd = hwndFor(window);
    if (!hwnd)
    {
        return false;
    }

    if (isWindows11OrGreater())
    {
        return enableWin11Acrylic(hwnd);
    }

    return enableWin10Blur(hwnd);
#else
    Q_UNUSED(window)
    return false;
#endif
}

void disable(QWidget *window)
{
#ifdef Q_OS_WIN
    const HWND hwnd = hwndFor(window);
    if (!hwnd)
    {
        return;
    }

    disableBackdrop(hwnd);
#else
    Q_UNUSED(window)
#endif
}

} // namespace DwmBackdrop
