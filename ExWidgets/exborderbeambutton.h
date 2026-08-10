#pragma once

#include "exborderbeam.h"
#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

#include <QColor>
#include <QElapsedTimer>
#include <QPainterPath>
#include <QPushButton>

class QEvent;
class QHideEvent;
class QPaintEvent;
class QShowEvent;
class QTimer;

// 这个类继承 QPushButton，并使用与 ExBorderBeam 相同的绘制逻辑。
class EXWIDGETS_EXPORT ExBorderBeamButton final : public QPushButton
{
    Q_OBJECT

public:
    enum Direction
    {
        Clockwise = ExBorderBeam::Clockwise,
        CounterClockwise = ExBorderBeam::CounterClockwise
    };
    Q_ENUM( Direction )

    enum ThemeMode
    {
        AutoTheme = ExBorderBeam::AutoTheme,
        LightTheme = ExBorderBeam::LightTheme,
        DarkTheme = ExBorderBeam::DarkTheme
    };
    Q_ENUM( ThemeMode )

    using ThemeConfig = ExBorderBeam::ThemeConfig;

    EXWIDGETS_DECLARE_PROPERTY( qreal, beamLength, beamLength, setBeamLength, 60.0 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, beamWidth, beamWidth, setBeamWidth, 2.0 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, cornerRadius, cornerRadius, setCornerRadius, 8.0 )
    EXWIDGETS_DECLARE_PROPERTY( QColor, backgroundColor, backgroundColor, setBackgroundColor, QColor() )
    EXWIDGETS_DECLARE_PROPERTY( QColor, borderColor, borderColor, setBorderColor, QColor() )
    EXWIDGETS_DECLARE_PROPERTY( QColor, startColor, startColor, setStartColor, QColor() )
    EXWIDGETS_DECLARE_PROPERTY( QColor, endColor, endColor, setEndColor, QColor() )
    EXWIDGETS_DECLARE_PROPERTY( int, animationDuration, animationDuration, setAnimationDuration, 6000 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, initialProgress, initialProgress, setInitialProgress, 0.0 )
    EXWIDGETS_DECLARE_PROPERTY( Direction, direction, direction, setDirection, Clockwise )
    EXWIDGETS_DECLARE_PROPERTY( int, beamCount, beamCount, setBeamCount, 1 )
    EXWIDGETS_DECLARE_PROPERTY( bool, animationEnabled, isAnimationEnabled, setAnimationEnabled, true )
    EXWIDGETS_DECLARE_PROPERTY( ThemeMode, themeMode, themeMode, setThemeMode, AutoTheme )

    Q_PROPERTY( bool running READ isRunning NOTIFY runningChanged )

    explicit ExBorderBeamButton( QWidget* parent = nullptr );
    explicit ExBorderBeamButton( const QString& text, QWidget* parent = nullptr );

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] QSize sizeHint() const override;

    void restartAnimation();

    [[nodiscard]] static ThemeConfig defaultLightTheme();
    [[nodiscard]] static ThemeConfig defaultDarkTheme();
    [[nodiscard]] ThemeConfig lightTheme() const;
    [[nodiscard]] ThemeConfig darkTheme() const;
    [[nodiscard]] ThemeConfig activeTheme() const;
    void setLightTheme( const ThemeConfig& config );
    void setDarkTheme( const ThemeConfig& config );

    Q_SIGNAL void runningChanged( bool running );
    Q_SIGNAL void lightThemeChanged();
    Q_SIGNAL void darkThemeChanged();

protected:
    void paintEvent( QPaintEvent* event ) override;
    void showEvent( QShowEvent* event ) override;
    void hideEvent( QHideEvent* event ) override;
    void changeEvent( QEvent* event ) override;

private:
    void initialize();
    void rebuildBorderPath();
    void updateAnimationState();
    [[nodiscard]] QColor resolvedStartColor() const;
    [[nodiscard]] QColor resolvedEndColor( const QColor& start ) const;
    [[nodiscard]] QColor resolvedBackgroundColor() const;
    [[nodiscard]] QColor resolvedBorderColor() const;
    [[nodiscard]] bool usesDarkTheme() const;

    QTimer* m_animationTimer = nullptr;
    QElapsedTimer m_elapsed;
    QPainterPath m_borderPath;
    qreal m_borderLength = 0.0;
    qreal m_progress = 0.0;
    QSize m_pathSize;
    bool m_pathDirty = true;
    ThemeConfig m_lightTheme = defaultLightTheme();
    ThemeConfig m_darkTheme = defaultDarkTheme();
};
