#include "palettemanager.h"
#include "qapplication.h"

PaletteManager &PaletteManager::instance()
{
    static PaletteManager inst;
    return inst;
}

void PaletteManager::setThemeStyle(ThemeStyle scheme)
{
    currentScheme = scheme;
    switch (scheme)
    {
    case ThemeStyle::Fluent:
        strategy = std::make_unique<FluentColorScheme>();
        break;
    case ThemeStyle::Teams:
        strategy = std::make_unique<TeamsColorScheme>();
        break;
    }
}

void PaletteManager::applyPalette(QPalette &palette, bool isDark) const
{
    if (isDark)
    {
        strategy->applyDark(palette);
    }
    else
    {
        strategy->applyLight(palette);
    }

    // Example项目
    // 通过属性设置强调色，供样式使用，实际项目中可以根据需要选择是否使用
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    const QVariant accentColorVariant = qApp->property("_q_accent_color");
    if (accentColorVariant.isValid())
    {
        QColor c = accentColorVariant.value<QColor>();
        palette.setColor(QPalette::Active, QPalette::Accent, c);
        palette.setColor(QPalette::Inactive, QPalette::Accent, c);
        palette.setColor(QPalette::Disabled, QPalette::Accent, c.darker(150));
    }

    // Keep selection semantics aligned with Fluent: Highlight follows Accent.
    // This avoids hardcoded system blue and automatically tracks theme/app accent.
    palette.setColor(QPalette::Active, QPalette::Highlight,
                     palette.color(QPalette::Active, QPalette::Accent));
    palette.setColor(QPalette::Inactive, QPalette::Highlight,
                     palette.color(QPalette::Inactive, QPalette::Accent));
    palette.setColor(QPalette::Disabled, QPalette::Highlight,
                     palette.color(QPalette::Disabled, QPalette::Accent));
#endif

    // Example项目
    // WidgetBgMode::Pixmap需要Base和Window有透明度，不然看不见背景
    //无此需求屏蔽以下代码
    if (qApp->property("_q_widget_mode").toInt() >= 1)
    {
        auto setAlpha = [&](QPalette::ColorGroup group, QPalette::ColorRole colorRole, int a)
        {
            QColor cr1 = palette.color(group, colorRole);
            if (cr1.alpha() == a)
            {
                return;
            }

            cr1.setAlpha(a);
            palette.setColor(group, colorRole, cr1);
        };
        setAlpha(QPalette::Active, QPalette::Base, 0);
        setAlpha(QPalette::Active, QPalette::Window, 160);
        setAlpha(QPalette::Disabled, QPalette::Base, 0);
        setAlpha(QPalette::Disabled, QPalette::Window, 160);
        setAlpha(QPalette::Inactive, QPalette::Base, 0);
        setAlpha(QPalette::Inactive, QPalette::Window, 160);
    }
}

void FluentColorScheme::applyLight(QPalette &p) const
{
    // Active
    p.setColor(QPalette::Active, QPalette::WindowText, QColor(0, 0, 0, 255));
    p.setColor(QPalette::Active, QPalette::Button, QColor(240, 240, 240, 255));
    p.setColor(QPalette::Active, QPalette::Light, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Active, QPalette::Midlight, QColor(227, 227, 227, 255));
    p.setColor(QPalette::Active, QPalette::Dark, QColor(160, 160, 160, 255));
    p.setColor(QPalette::Active, QPalette::Mid, QColor(160, 160, 160, 255));
    p.setColor(QPalette::Active, QPalette::Text, QColor(0, 0, 0, 255));
    p.setColor(QPalette::Active, QPalette::BrightText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Active, QPalette::ButtonText, QColor(0, 0, 0, 255));

    p.setColor(QPalette::Active, QPalette::Base, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Active, QPalette::Window, QColor(250, 250, 250, 255));

    //Fluent测试
    p.setColor(QPalette::Active, QPalette::Window, QColor(243, 243, 243));
    p.setColor(QPalette::Active, QPalette::Base, QColor(249, 249, 249));

    p.setColor(QPalette::Active, QPalette::Shadow, QColor(105, 105, 105, 255));
    p.setColor(QPalette::Active, QPalette::Highlight, QColor(0, 90, 158, 255));
    p.setColor(QPalette::Active, QPalette::HighlightedText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Active, QPalette::Link, QColor(0, 66, 117, 255));
    p.setColor(QPalette::Active, QPalette::LinkVisited, QColor(0, 38, 66, 255));
    p.setColor(QPalette::Active, QPalette::AlternateBase, QColor(245, 245, 245, 255));
    p.setColor(QPalette::Active, QPalette::ToolTipBase, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Active, QPalette::ToolTipText, QColor(0, 0, 0, 230));
    p.setColor(QPalette::Active, QPalette::PlaceholderText, QColor(0, 0, 0, 128));

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    p.setColor(QPalette::Active, QPalette::Accent, QColor(0, 90, 158, 255));
#else
#endif

    // Disabled
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120, 255));
    p.setColor(QPalette::Disabled, QPalette::Button, QColor(240, 240, 240, 255));
    p.setColor(QPalette::Disabled, QPalette::Light, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Disabled, QPalette::Midlight, QColor(247, 247, 247, 255));
    p.setColor(QPalette::Disabled, QPalette::Dark, QColor(160, 160, 160, 255));
    p.setColor(QPalette::Disabled, QPalette::Mid, QColor(160, 160, 160, 255));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 120, 255));
    p.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120, 255));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor(240, 240, 240, 255));
    p.setColor(QPalette::Disabled, QPalette::Window, QColor(240, 240, 240, 255));
    p.setColor(QPalette::Disabled, QPalette::Shadow, QColor(0, 0, 0, 255));
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(120, 120, 120, 255));
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Disabled, QPalette::Link, QColor(0, 66, 117, 128));
    p.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(0, 38, 66, 128));
    p.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(245, 245, 245, 255));
    p.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(0, 0, 0, 230));
    p.setColor(QPalette::Disabled, QPalette::PlaceholderText, QColor(0, 0, 0, 128));

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    p.setColor(QPalette::Disabled, QPalette::Accent, QColor(120, 120, 120, 255));
#endif

    // Inactive
    p.setColor(QPalette::Inactive, QPalette::WindowText, QColor(0, 0, 0, 255));
    p.setColor(QPalette::Inactive, QPalette::Button, QColor(240, 240, 240, 255));
    p.setColor(QPalette::Inactive, QPalette::Light, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::Midlight, QColor(227, 227, 227, 255));
    p.setColor(QPalette::Inactive, QPalette::Dark, QColor(160, 160, 160, 255));
    p.setColor(QPalette::Inactive, QPalette::Mid, QColor(160, 160, 160, 255));
    p.setColor(QPalette::Inactive, QPalette::Text, QColor(0, 0, 0, 255));
    p.setColor(QPalette::Inactive, QPalette::BrightText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(0, 0, 0, 255));
    p.setColor(QPalette::Inactive, QPalette::Base, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::Window, QColor(/*240,240,240,255*/ 255, 255, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::Shadow, QColor(105, 105, 105, 255));
    p.setColor(QPalette::Inactive, QPalette::Highlight, QColor(0, 90, 158, 255));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::Link, QColor(0, 66, 117, 255));
    p.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(0, 38, 66, 255));
    p.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(245, 245, 245, 255));
    p.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(0, 0, 0, 230));
    p.setColor(QPalette::Inactive, QPalette::PlaceholderText, QColor(0, 0, 0, 128));

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    // p.setColor( QPalette::Inactive, QPalette::Accent, QColor( 240, 240, 240, 255 ) );
    p.setColor(QPalette::Inactive, QPalette::Accent, QColor(0, 90, 158, 255));
#endif
}

void FluentColorScheme::applyDark(QPalette &p) const
{
    // Active
    p.setColor(QPalette::Active, QPalette::WindowText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Active, QPalette::Button, QColor(60, 60, 60, 255));
    p.setColor(QPalette::Active, QPalette::Light, QColor(120, 120, 120, 255));
    p.setColor(QPalette::Active, QPalette::Midlight, QColor(90, 90, 90, 255));
    p.setColor(QPalette::Active, QPalette::Dark, QColor(30, 30, 30, 255));
    p.setColor(QPalette::Active, QPalette::Mid, QColor(40, 40, 40, 255));
    p.setColor(QPalette::Active, QPalette::Text, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Active, QPalette::BrightText, QColor(105, 189, 255, 255));
    p.setColor(QPalette::Active, QPalette::ButtonText, QColor(255, 255, 255, 255));

    p.setColor(QPalette::Active, QPalette::Base, QColor(39, 39, 39, 255));
    p.setColor(QPalette::Active, QPalette::Window, QColor(32, 32, 32, 255));

    //测试
    p.setColor(QPalette::Active, QPalette::Base, QColor(46, 46, 46, 255));
    p.setColor(QPalette::Active, QPalette::Window, QColor(39, 39, 39, 255));

    p.setColor(QPalette::Active, QPalette::Shadow, QColor(0, 0, 0, 255));
    p.setColor(QPalette::Active, QPalette::Highlight, QColor(54, 166, 255, 255));
    p.setColor(QPalette::Active, QPalette::HighlightedText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Active, QPalette::Link, QColor(105, 189, 255, 255));
    p.setColor(QPalette::Active, QPalette::LinkVisited, QColor(54, 166, 255, 255));
    p.setColor(QPalette::Active, QPalette::AlternateBase, QColor(255, 255, 255, 15));
    p.setColor(QPalette::Active, QPalette::ToolTipBase, QColor(45, 45, 45, 255));
    p.setColor(QPalette::Active, QPalette::ToolTipText, QColor(255, 255, 255, 230));
    p.setColor(QPalette::Active, QPalette::PlaceholderText, QColor(255, 255, 255, 128));

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    // p.setColor(QPalette::Active, QPalette::Accent, QColor(54, 166, 255, 255));
    p.setColor(QPalette::Active, QPalette::Accent, QColor(76, 160, 224, 255));
#else
#endif

    // Disabled
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(157, 157, 157, 255));
    p.setColor(QPalette::Disabled, QPalette::Button, QColor(60, 60, 60, 255));
    p.setColor(QPalette::Disabled, QPalette::Light, QColor(120, 120, 120, 255));
    p.setColor(QPalette::Disabled, QPalette::Midlight, QColor(90, 90, 90, 255));
    p.setColor(QPalette::Disabled, QPalette::Dark, QColor(30, 30, 30, 255));
    p.setColor(QPalette::Disabled, QPalette::Mid, QColor(40, 40, 40, 255));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(157, 157, 157, 255));
    p.setColor(QPalette::Disabled, QPalette::BrightText, QColor(105, 189, 255, 255));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(157, 157, 157, 255));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor(39, 39, 39, 255));
    p.setColor(QPalette::Disabled, QPalette::Window, QColor(39, 39, 39, 255));
    p.setColor(QPalette::Disabled, QPalette::Shadow, QColor(0, 0, 0, 255));
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(90, 90, 90, 255));
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Disabled, QPalette::Link, QColor(105, 189, 255, 128));
    p.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(54, 166, 255, 128));
    p.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(52, 52, 52, 255));
    p.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(44, 44, 44, 255));
    p.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(255, 255, 255, 128));
    p.setColor(QPalette::Disabled, QPalette::PlaceholderText, QColor(255, 255, 255, 128));

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    p.setColor(QPalette::Disabled, QPalette::Accent, QColor(157, 157, 157, 255));
#endif

    // Inactive
    p.setColor(QPalette::Inactive, QPalette::WindowText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::Button, QColor(60, 60, 60, 255));
    p.setColor(QPalette::Inactive, QPalette::Light, QColor(120, 120, 120, 255));
    p.setColor(QPalette::Inactive, QPalette::Midlight, QColor(90, 90, 90, 255));
    p.setColor(QPalette::Inactive, QPalette::Dark, QColor(30, 30, 30, 255));
    p.setColor(QPalette::Inactive, QPalette::Mid, QColor(40, 40, 40, 255));
    p.setColor(QPalette::Inactive, QPalette::Text, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::BrightText, QColor(105, 189, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::Base, QColor(50, 50, 50, 255));
    p.setColor(QPalette::Inactive, QPalette::Window, QColor(39, 39, 39, 255));
    p.setColor(QPalette::Inactive, QPalette::Shadow, QColor(0, 0, 0, 255));
    p.setColor(QPalette::Inactive, QPalette::Highlight, QColor(54, 166, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::Link, QColor(105, 189, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(54, 166, 255, 255));
    p.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(255, 255, 255, 15));
    p.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(44, 44, 44, 255));
    p.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(255, 255, 255, 230));
    p.setColor(QPalette::Inactive, QPalette::PlaceholderText, QColor(255, 255, 255, 128));

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    // p.setColor( QPalette::Inactive, QPalette::Accent, QColor( 54, 166, 255, 255 ) );
    p.setColor(QPalette::Inactive, QPalette::Accent, QColor(54, 166, 255, 255));
#endif
}

void TeamsColorScheme::applyLight(QPalette &p) const
{
    auto defaultColorScheme = std::make_unique<FluentColorScheme>();
    defaultColorScheme->applyLight(p);

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    QColor accentActive(91, 95, 199);     // #5B5FC7
    QColor accentInactive(138, 141, 240); // #8A8DF0
    QColor accentDisabled(200, 200, 200); // #C8C8C8

    p.setColor(QPalette::Active, QPalette::Accent, accentActive);
    p.setColor(QPalette::Inactive, QPalette::Accent, accentInactive);
    p.setColor(QPalette::Disabled, QPalette::Accent, accentDisabled);
#endif
}

void TeamsColorScheme::applyDark(QPalette &p) const
{
    auto defaultColorScheme = std::make_unique<FluentColorScheme>();
    defaultColorScheme->applyDark(p);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)

    QColor accentActive(99, 102, 241);  // 主色（Dark优化）
    QColor accentInactive(70, 73, 160); // 弱化
    QColor accentDisabled(90, 90, 90);  // 灰

    p.setColor(QPalette::Active, QPalette::Accent, accentActive);
    p.setColor(QPalette::Inactive, QPalette::Accent, accentInactive);
    p.setColor(QPalette::Disabled, QPalette::Accent, accentDisabled);
#endif
}

QColor FluentColorScheme::accentColor(bool isDark) const
{
    if (isDark)
    {
        return QColor(54, 166, 255, 255);
    }
    else
    {
        return QColor(0, 90, 158, 255);
    }
}

QColor TeamsColorScheme::accentColor(bool isDark) const
{
    if (isDark)
    {
        return QColor(99, 102, 241, 255);
    }
    else
    {
        return QColor(91, 95, 199, 255);
    }
}

QColor PaletteManager::accentColor(bool isDark) const
{
    return strategy->accentColor(isDark);
}
