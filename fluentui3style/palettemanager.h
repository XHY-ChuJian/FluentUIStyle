#pragma once

#include <QPalette>
#include <QObject>
#include <memory>

enum class ColorScheme {
    Fluent,
    Teams
};

class ColorSchemeStrategy {
public:
    virtual ~ColorSchemeStrategy() = default;
    virtual void applyLight(QPalette& palette) const = 0;
    virtual void applyDark(QPalette& palette) const = 0;
};

class FluentColorScheme : public ColorSchemeStrategy {
public:
    void applyLight(QPalette& palette) const override;
    void applyDark(QPalette& palette) const override;
};

class TeamsColorScheme : public ColorSchemeStrategy {
public:
    void applyLight(QPalette& palette) const override;
    void applyDark(QPalette& palette) const override;
};

class PaletteManager: public QObject
{
    Q_OBJECT
public:
    static PaletteManager& instance();
    void setColorScheme(ColorScheme scheme);
    void applyPalette(QPalette& palette, bool isDark = true) const;

private:
    explicit PaletteManager(): QObject(nullptr) {}
    
    ColorScheme currentScheme = ColorScheme::Fluent;
    std::unique_ptr<ColorSchemeStrategy> strategy = std::make_unique<FluentColorScheme>();
};
