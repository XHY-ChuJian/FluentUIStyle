#pragma once

#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

#include <QColor>
#include <QElapsedTimer>
#include <QFrame>
#include <QPainterPath>

class QEvent;
class QHideEvent;
class QPaintEvent;
class QShowEvent;
class QTimer;

class EXWIDGETS_EXPORT ExBorderBeam final : public QFrame
{
    Q_OBJECT

public:
    enum Direction
    {
        Clockwise,
        CounterClockwise
    };
    Q_ENUM( Direction )

    enum ThemeMode
    {
        AutoTheme,
        LightTheme,
        DarkTheme
    };
    Q_ENUM( ThemeMode )

    struct ThemeConfig
    {
        QColor backgroundColor;
        QColor borderColor;
        QColor startColor;
        QColor endColor;

        [[nodiscard]] bool operator==( const ThemeConfig& other ) const
        {
            return backgroundColor == other.backgroundColor && borderColor == other.borderColor
                   && startColor == other.startColor && endColor == other.endColor;
        }
        [[nodiscard]] bool operator!=( const ThemeConfig& other ) const { return !( *this == other ); }
    };

    // 光束沿边框路径占用的长度，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, beamLength, beamLength, setBeamLength, 60.0 )

    // 光束线宽，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, beamWidth, beamWidth, setBeamWidth, 2.0 )

    // 外边框圆角半径。
    EXWIDGETS_DECLARE_PROPERTY( qreal, cornerRadius, cornerRadius, setCornerRadius, 8.0 )

    // 背景颜色；无效颜色表示使用调色板 Window 颜色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, backgroundColor, backgroundColor, setBackgroundColor, QColor() )

    // 边框颜色；无效颜色表示使用调色板 Mid 颜色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, borderColor, borderColor, setBorderColor, QColor() )

    // 光束尾部颜色；无效颜色表示使用调色板强调色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, startColor, startColor, setStartColor, QColor() )

    // 光束头部颜色；无效颜色表示从调色板强调色自动生成。
    EXWIDGETS_DECLARE_PROPERTY( QColor, endColor, endColor, setEndColor, QColor() )

    // 光束绕边框一周的时间，单位为毫秒。
    EXWIDGETS_DECLARE_PROPERTY( int, animationDuration, animationDuration, setAnimationDuration, 6000 )

    // 动画的初始位置，取值范围为 [0, 1]。
    EXWIDGETS_DECLARE_PROPERTY( qreal, initialProgress, initialProgress, setInitialProgress, 0.0 )

    // 光束运动方向。
    EXWIDGETS_DECLARE_PROPERTY( Direction, direction, direction, setDirection, Clockwise )

    // 同时显示的光束数量，多束光沿路径均匀分布。
    EXWIDGETS_DECLARE_PROPERTY( int, beamCount, beamCount, setBeamCount, 1 )

    // 是否播放光束动画。
    EXWIDGETS_DECLARE_PROPERTY( bool, animationEnabled, isAnimationEnabled, setAnimationEnabled, true )

    // AutoTheme 会在应用切换调色板时自动选择 Light/Dark 配置。
    EXWIDGETS_DECLARE_PROPERTY( ThemeMode, themeMode, themeMode, setThemeMode, AutoTheme )

    Q_PROPERTY( bool running READ isRunning NOTIFY runningChanged )

    explicit ExBorderBeam( QWidget* parent = nullptr );

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

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

Q_DECLARE_METATYPE( ExBorderBeam::ThemeConfig )
