#include "palettemanager.h"

PaletteManager& PaletteManager::instance()
{
    static PaletteManager inst;
    return inst;
}

void PaletteManager::applyPalette(QPalette& palette, bool isDark) const
{
    if (isDark)
        applyDark(palette);
    else
        applyLight(palette);
}

void PaletteManager::applyLight(QPalette& p) const
{
    // Active
    p.setColor(QPalette::Active, QPalette::WindowText, QColor(0,0,0,255));
    p.setColor(QPalette::Active, QPalette::Button, QColor(240,240,240,255));
    p.setColor(QPalette::Active, QPalette::Light, QColor(255,255,255,255));
    p.setColor(QPalette::Active, QPalette::Midlight, QColor(227,227,227,255));
    p.setColor(QPalette::Active, QPalette::Dark, QColor(160,160,160,255));
    p.setColor(QPalette::Active, QPalette::Mid, QColor(160,160,160,255));
    p.setColor(QPalette::Active, QPalette::Text, QColor(0,0,0,255));
    p.setColor(QPalette::Active, QPalette::BrightText, QColor(255,255,255,255));
    p.setColor(QPalette::Active, QPalette::ButtonText, QColor(0,0,0,255));
    p.setColor(QPalette::Active, QPalette::Base, QColor(255,255,255,255));
    p.setColor(QPalette::Active, QPalette::Window, QColor(255,255,255,255));
    p.setColor(QPalette::Active, QPalette::Shadow, QColor(105,105,105,255));
    p.setColor(QPalette::Active, QPalette::Highlight, QColor(0,120,215,255));
    p.setColor(QPalette::Active, QPalette::HighlightedText, QColor(255,255,255,255));
    p.setColor(QPalette::Active, QPalette::Link, QColor(0,66,117,255));
    p.setColor(QPalette::Active, QPalette::LinkVisited, QColor(0,38,66,255));
    p.setColor(QPalette::Active, QPalette::AlternateBase, QColor(245,245,245,255));
    p.setColor(QPalette::Active, QPalette::ToolTipBase, QColor(255,255,220,255));
    p.setColor(QPalette::Active, QPalette::ToolTipText, QColor(0,0,0,255));
    p.setColor(QPalette::Active, QPalette::PlaceholderText, QColor(0,0,0,128));

#if QT_VERSION >= QT_VERSION_CHECK(6,6,0)
    p.setColor(QPalette::Active, QPalette::Accent, QColor(0,90,158,255));
#else
#endif

    // Disabled
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120,120,120,255));
    p.setColor(QPalette::Disabled, QPalette::Button, QColor(240,240,240,255));
    p.setColor(QPalette::Disabled, QPalette::Light, QColor(255,255,255,255));
    p.setColor(QPalette::Disabled, QPalette::Midlight, QColor(247,247,247,255));
    p.setColor(QPalette::Disabled, QPalette::Dark, QColor(160,160,160,255));
    p.setColor(QPalette::Disabled, QPalette::Mid, QColor(160,160,160,255));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(120,120,120,255));
    p.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255,255,255,255));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120,120,120,255));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor(240,240,240,255));
    p.setColor(QPalette::Disabled, QPalette::Window, QColor(240,240,240,255));
    p.setColor(QPalette::Disabled, QPalette::Shadow, QColor(0,0,0,255));
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(0,120,215,255));
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(255,255,255,255));
    p.setColor(QPalette::Disabled, QPalette::Link, QColor(0,0,255,255));
    p.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(255,0,255,255));
    p.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(245,245,245,255));
    p.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(255,255,220,255));
    p.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(0,0,0,255));
    p.setColor(QPalette::Disabled, QPalette::PlaceholderText, QColor(0,0,0,128));

#if QT_VERSION >= QT_VERSION_CHECK(6,6,0)
    p.setColor(QPalette::Disabled, QPalette::Accent, QColor(120,120,120,255));
#endif

    // Inactive
    p.setColor(QPalette::Inactive, QPalette::WindowText, QColor(0,0,0,255));
    p.setColor(QPalette::Inactive, QPalette::Button, QColor(240,240,240,255));
    p.setColor(QPalette::Inactive, QPalette::Light, QColor(255,255,255,255));
    p.setColor(QPalette::Inactive, QPalette::Midlight, QColor(227,227,227,255));
    p.setColor(QPalette::Inactive, QPalette::Dark, QColor(160,160,160,255));
    p.setColor(QPalette::Inactive, QPalette::Mid, QColor(160,160,160,255));
    p.setColor(QPalette::Inactive, QPalette::Text, QColor(0,0,0,255));
    p.setColor(QPalette::Inactive, QPalette::BrightText, QColor(255,255,255,255));
    p.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(0,0,0,255));
    p.setColor(QPalette::Inactive, QPalette::Base, QColor(255,255,255,255));
    p.setColor(QPalette::Inactive, QPalette::Window, QColor(/*240,240,240,255*/255,255,255,255));
    p.setColor(QPalette::Inactive, QPalette::Shadow, QColor(105,105,105,255));
    p.setColor(QPalette::Inactive, QPalette::Highlight, QColor(240,240,240,255));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(0,0,0,255));
    p.setColor(QPalette::Inactive, QPalette::Link, QColor(0,66,117,255));
    p.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(0,38,66,255));
    p.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(245,245,245,255));
    p.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(255,255,220,255));
    p.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(0,0,0,255));
    p.setColor(QPalette::Inactive, QPalette::PlaceholderText, QColor(0,0,0,128));

#if QT_VERSION >= QT_VERSION_CHECK(6,6,0)
    p.setColor(QPalette::Inactive, QPalette::Accent, QColor(240,240,240,255));
#endif
}

void PaletteManager::applyDark(QPalette& p) const
{
    // Active
    p.setColor(QPalette::Active, QPalette::WindowText, QColor(255,255,255,255));
    p.setColor(QPalette::Active, QPalette::Button, QColor(60,60,60,255));
    p.setColor(QPalette::Active, QPalette::Light, QColor(120,120,120,255));
    p.setColor(QPalette::Active, QPalette::Midlight, QColor(90,90,90,255));
    p.setColor(QPalette::Active, QPalette::Dark, QColor(30,30,30,255));
    p.setColor(QPalette::Active, QPalette::Mid, QColor(40,40,40,255));
    p.setColor(QPalette::Active, QPalette::Text, QColor(255,255,255,255));
    p.setColor(QPalette::Active, QPalette::BrightText, QColor(105,189,255,255));
    p.setColor(QPalette::Active, QPalette::ButtonText, QColor(255,255,255,255));
    p.setColor(QPalette::Active, QPalette::Base, QColor(45,45,45,255));
    p.setColor(QPalette::Active, QPalette::Window, QColor(32,32,32,255));
    p.setColor(QPalette::Active, QPalette::Shadow, QColor(0,0,0,255));
    p.setColor(QPalette::Active, QPalette::Highlight, QColor(0,120,215,255));
    p.setColor(QPalette::Active, QPalette::HighlightedText, QColor(255,255,255,255));
    p.setColor(QPalette::Active, QPalette::Link, QColor(105,189,255,255));
    p.setColor(QPalette::Active, QPalette::LinkVisited, QColor(54,166,255,255));
    p.setColor(QPalette::Active, QPalette::AlternateBase, QColor(255,255,255,15));
    p.setColor(QPalette::Active, QPalette::ToolTipBase, QColor(60,60,60,255));
    p.setColor(QPalette::Active, QPalette::ToolTipText, QColor(212,212,212,255));
    p.setColor(QPalette::Active, QPalette::PlaceholderText, QColor(255,255,255,128));

#if QT_VERSION >= QT_VERSION_CHECK(6,6,0)
    p.setColor(QPalette::Active, QPalette::Accent, QColor(54,166,255,255));
#endif


    // Disabled
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(157,157,157,255));
    p.setColor(QPalette::Disabled, QPalette::Button, QColor(60,60,60,255));
    p.setColor(QPalette::Disabled, QPalette::Light, QColor(120,120,120,255));
    p.setColor(QPalette::Disabled, QPalette::Midlight, QColor(90,90,90,255));
    p.setColor(QPalette::Disabled, QPalette::Dark, QColor(30,30,30,255));
    p.setColor(QPalette::Disabled, QPalette::Mid, QColor(40,40,40,255));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(157,157,157,255));
    p.setColor(QPalette::Disabled, QPalette::BrightText, QColor(105,189,255,255));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(157,157,157,255));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor(30,30,30,255));
    p.setColor(QPalette::Disabled, QPalette::Window, QColor(30,30,30,255));
    p.setColor(QPalette::Disabled, QPalette::Shadow, QColor(0,0,0,255));
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(0,120,215,255));
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(255,255,255,255));
    p.setColor(QPalette::Disabled, QPalette::Link, QColor(0,0,255,255));
    p.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(255,0,255,255));
    p.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(52,52,52,255));
    p.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(255,255,220,255));
    p.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(0,0,0,255));
    p.setColor(QPalette::Disabled, QPalette::PlaceholderText, QColor(255,255,255,128));

#if QT_VERSION >= QT_VERSION_CHECK(6,6,0)
    p.setColor(QPalette::Disabled, QPalette::Accent, QColor(157,157,157,255));
#endif


    // Inactive
    p.setColor(QPalette::Inactive, QPalette::WindowText, QColor(255,255,255,255));
    p.setColor(QPalette::Inactive, QPalette::Button, QColor(60,60,60,255));
    p.setColor(QPalette::Inactive, QPalette::Light, QColor(120,120,120,255));
    p.setColor(QPalette::Inactive, QPalette::Midlight, QColor(90,90,90,255));
    p.setColor(QPalette::Inactive, QPalette::Dark, QColor(30,30,30,255));
    p.setColor(QPalette::Inactive, QPalette::Mid, QColor(40,40,40,255));
    p.setColor(QPalette::Inactive, QPalette::Text, QColor(255,255,255,255));
    p.setColor(QPalette::Inactive, QPalette::BrightText, QColor(105,189,255,255));
    p.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(255,255,255,255));
    p.setColor(QPalette::Inactive, QPalette::Base, QColor(45,45,45,255));
    p.setColor(QPalette::Inactive, QPalette::Window, QColor(32,32,32,255));
    p.setColor(QPalette::Inactive, QPalette::Shadow, QColor(0,0,0,255));
    p.setColor(QPalette::Inactive, QPalette::Highlight, QColor(0,120,215,255));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(255,255,255,255));
    p.setColor(QPalette::Inactive, QPalette::Link, QColor(105,189,255,255));
    p.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(54,166,255,255));
    p.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(255,255,255,15));
    p.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(60,60,60,255));
    p.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(212,212,212,255));
    p.setColor(QPalette::Inactive, QPalette::PlaceholderText, QColor(255,255,255,128));

#if QT_VERSION >= QT_VERSION_CHECK(6,6,0)
    p.setColor(QPalette::Inactive, QPalette::Accent, QColor(54,166,255,255));
#endif
}
