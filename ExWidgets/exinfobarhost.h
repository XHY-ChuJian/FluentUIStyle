#pragma once

#include "exinfobar.h"
#include "exwidgets_global.h"

#include <QObject>

class QWidget;

/**
 * \brief 将 ExInfoBar 作为窗口级通知显示，并管理位置、堆叠、排队与超时。
 *
 * InfoBar 本身仍是页面内控件；Host 只负责弹出式使用场景。传给 addInfoBar()
 * 的控件会被重新设置父对象，并由 Host 在关闭后销毁。
 */
class EXWIDGETS_EXPORT ExInfoBarHost final : public QObject
{
    Q_OBJECT

public:
    enum Position
    {
        TopLeft,
        Top,
        TopRight,
        BottomLeft,
        Bottom,
        BottomRight
    };
    Q_ENUM( Position )

    Q_PROPERTY( int margin READ margin WRITE setMargin )
    Q_PROPERTY( int spacing READ spacing WRITE setSpacing )
    Q_PROPERTY( int maximumWidth READ maximumWidth WRITE setMaximumWidth )
    Q_PROPERTY( int defaultTimeout READ defaultTimeout WRITE setDefaultTimeout )

    explicit ExInfoBarHost( QWidget* target, QObject* parent = nullptr );
    ~ExInfoBarHost() override;

    // 设置应用的默认主窗口。默认 Host 按需创建并跟随该窗口销毁。
    static void setDefaultTarget( QWidget* target );
    [[nodiscard]] static QWidget* defaultTarget();
    [[nodiscard]] static ExInfoBarHost* defaultHost();

    [[nodiscard]] QWidget* target() const;

    [[nodiscard]] int margin() const;
    void setMargin( int margin );
    [[nodiscard]] int spacing() const;
    void setSpacing( int spacing );
    [[nodiscard]] int maximumWidth() const;
    void setMaximumWidth( int width );
    [[nodiscard]] int defaultTimeout() const;
    void setDefaultTimeout( int milliseconds );

    // timeout < 0 使用 defaultTimeout；timeout == 0 表示不自动关闭。
    [[nodiscard]] ExInfoBar* showInfoBar( ExInfoBar::Severity severity,
                                          const QString& title,
                                          const QString& message,
                                          Position position = TopRight,
                                          int timeout = -1 );
    void addInfoBar( ExInfoBar* infoBar,
                     Position position = TopRight,
                     int timeout = -1 );

public Q_SLOTS:
    void dismissAll();
    void dismissAll( ExInfoBarHost::Position position );

Q_SIGNALS:
    void infoBarShown( ExInfoBar* infoBar, ExInfoBarHost::Position position );
    void infoBarClosed( ExInfoBar* infoBar, ExInfoBarHost::Position position );

protected:
    bool eventFilter( QObject* watched, QEvent* event ) override;

private:
    class Private;
    Private* d = nullptr;
};
