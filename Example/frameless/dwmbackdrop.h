#pragma once

class QWidget;

namespace DwmBackdrop {

// Win10: ACCENT_ENABLE_BLURBEHIND
// Win11: ACCENT_ENABLE_ACRYLICBLURBEHIND (+ optional DWM system backdrop)
bool enable(QWidget *window);
void disable(QWidget *window);

} // namespace DwmBackdrop
