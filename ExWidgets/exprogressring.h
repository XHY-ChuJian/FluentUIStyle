#pragma once

#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

#include <QColor>
#include <QFont>
#include <QMetaObject>
#include <QPointer>
#include <QProgressBar>

class QEvent;
class QPaintEvent;
class QResizeEvent;

class EXWIDGETS_EXPORT ExProgressRing final : public QProgressBar
{
    Q_OBJECT

public:
    // 显示在数值上方的标题；空字符串表示只绘制数值。
    EXWIDGETS_DECLARE_PROPERTY( QString, title, title, setTitle, QString() )

    // 标题字体；默认 QFont() 表示根据控件字体和尺寸自动生成。
    EXWIDGETS_DECLARE_PROPERTY( QFont, titleFont, titleFont, setTitleFont, QFont() )

    // 数值字体；默认 QFont() 表示根据控件字体和尺寸自动生成。
    EXWIDGETS_DECLARE_PROPERTY( QFont, valueFont, valueFont, setValueFont, QFont() )

    // 标题颜色；无效颜色表示使用当前调色板的次要文本颜色。
    EXWIDGETS_DECLARE_PROPERTY( QColor, titleColor, titleColor, setTitleColor, QColor() )

    // 数值颜色；无效颜色表示使用 QPalette::Text。
    EXWIDGETS_DECLARE_PROPERTY( QColor, valueColor, valueColor, setValueColor, QColor() )

    // 标题与数值之间的垂直间距，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( int, textSpacing, textSpacing, setTextSpacing, 4 )

    Q_PROPERTY( QWidget* centerWidget READ centerWidget WRITE setCenterWidget NOTIFY centerWidgetChanged )

    explicit ExProgressRing( QWidget* parent = nullptr );
    ~ExProgressRing() override;

    [[nodiscard]] QWidget* centerWidget() const;
    // 控件接管 widget 的所有权；替换时会删除原中心控件。
    void setCenterWidget( QWidget* widget );
    // 解除并返回中心控件的所有权，调用方负责后续释放。
    [[nodiscard]] QWidget* takeCenterWidget();

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

    Q_SIGNAL void centerWidgetChanged( QWidget* widget );

protected:
    bool event( QEvent* event ) override;
    void paintEvent( QPaintEvent* event ) override;
    void resizeEvent( QResizeEvent* event ) override;
    void changeEvent( QEvent* event ) override;

private:
    [[nodiscard]] QRectF centerContentRect() const;
    [[nodiscard]] QFont resolvedTitleFont() const;
    [[nodiscard]] QFont resolvedValueFont() const;
    [[nodiscard]] QColor resolvedTextColor( const QColor& customColor, bool secondary ) const;
    void updateCenterWidgetGeometry();

    QPointer<QWidget> m_centerWidget;
    QMetaObject::Connection m_centerWidgetDestroyedConnection;
};
