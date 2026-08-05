#pragma once

#include <QObject>
#include <QString>

#include "exwidgetsframeless_global.h"

class FluentTitleBar;
class QMainWindow;
class QMenuBar;
class QWidget;

namespace QWK {
class WidgetWindowAgent;
}  // namespace QWK

class EXWIDGETS_FRAMELESS_EXPORT FluentWindowFrame : public QObject
{
    Q_OBJECT

public:
    explicit FluentWindowFrame( QMainWindow* window, QObject* parent = nullptr );
    ~FluentWindowFrame() override;

    void installChromeHeader( QMenuBar* menuBar );

    FluentTitleBar* titleBar() const;

    void clearWindowBackdrop();
    bool setWindowBackdrop(const QString& key);

protected:
    bool eventFilter( QObject* watched, QEvent* event ) override;

private:
    void setupWindowAgent();
    void attachChromeHeader();

    QMainWindow* m_window { nullptr };
    QWK::WidgetWindowAgent* m_windowAgent { nullptr };
    FluentTitleBar* m_titleBar { nullptr };
    QWidget* m_chromeHeader { nullptr };
    QMenuBar* m_menuBar { nullptr };
};
