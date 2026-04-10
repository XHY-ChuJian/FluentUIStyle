#ifndef FLUENTUIAPPEARANCE_H
#define FLUENTUIAPPEARANCE_H

#include "fluentui3style_global.h"

#include <QObject>

enum class Theme
{
    Light,
    Dark
};

#define fluentUIAppearance FluentUIAppearance::instance()

class FluentUIAppearancePrivate;
class FLUENTUI3STYLE_EXPORT FluentUIAppearance : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(FluentUIAppearance)

public:
    static FluentUIAppearance& instance();

    void initialize();

    void setTheme(Theme theme);
    Theme theme() const;
    void setMainWindow(QWidget* mainWindow);

signals:
    void themeChanged(Theme theme);

private:
    explicit FluentUIAppearance(QObject *parent = nullptr);

    FluentUIAppearancePrivate* d_ptr;
};

#endif // FLUENTUIAPPEARANCE_H
