#pragma once

#include "fluentui3style_global.h"

#include <QPalette>
#include <QObject>
#include <memory>

enum ThemeStyle {
    Fluent,
    Teams
};

class ColorSchemeStrategy {
public:
    virtual ~ColorSchemeStrategy() = default;
    virtual void applyLight(QPalette& palette) const = 0;
    virtual void applyDark(QPalette& palette) const = 0;
    virtual QColor accentColor(bool isDark) const = 0;
};

class FluentColorScheme : public ColorSchemeStrategy {
public:
    void applyLight(QPalette& palette) const override;
    void applyDark(QPalette& palette) const override;
    QColor accentColor(bool isDark) const override;
};

class TeamsColorScheme : public ColorSchemeStrategy {
public:
    void applyLight(QPalette& palette) const override;
    void applyDark(QPalette& palette) const override;
    QColor accentColor(bool isDark) const override;
};

class FLUENTUI3STYLE_EXPORT PaletteManager: public QObject
{
    Q_OBJECT
public:
    static PaletteManager& instance();
    void setThemeStyle(ThemeStyle scheme);
    void applyPalette(QPalette& palette, bool isDark = true) const;
    QColor accentColor(bool isDark = true) const;

private:
    explicit PaletteManager(): QObject(nullptr) {}
    
    ThemeStyle currentScheme = ThemeStyle::Fluent;
    std::unique_ptr<ColorSchemeStrategy> strategy = std::make_unique<FluentColorScheme>();
};
