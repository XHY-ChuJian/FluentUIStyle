#include "fluentwindowframe.h"

#include <QEvent>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QPointer>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <QWKWidgets/widgetwindowagent.h>

#include "dwmbackdrop.h"
#include "fluenttitlebar.h"

namespace {

void scheduleBackdropReapply(QMainWindow *window)
{
    const QPointer<QMainWindow> windowGuard(window);
    QTimer::singleShot(0, window, [windowGuard]() {
        if (windowGuard)
        {
            DwmBackdrop::enable(windowGuard);
        }
    });
}

} // namespace

FluentWindowFrame::FluentWindowFrame(QMainWindow *window, QObject *parent)
    : QObject(parent)
    , m_window(window)
{
    setupWindowAgent();
    m_window->installEventFilter(this);
}

QWK::WidgetWindowAgent *FluentWindowFrame::windowAgent() const
{
    return m_windowAgent;
}

FluentTitleBar *FluentWindowFrame::titleBar() const
{
    return m_titleBar;
}

void FluentWindowFrame::clearWindowBackdrop()
{
    m_backdropEnabled = false;

    if (m_windowAgent)
    {
        static const QStringList kBackdropAttributes = {
            QStringLiteral("dwm-blur"),
            QStringLiteral("acrylic-material"),
            QStringLiteral("mica"),
            QStringLiteral("mica-alt"),
        };

        for (const QString &attribute : kBackdropAttributes)
        {
            m_windowAgent->setWindowAttribute(attribute, false);
        }
    }

    if (m_window)
    {
        DwmBackdrop::disable(m_window);
    }
}

bool FluentWindowFrame::setWindowBackdrop(const QString &key)
{
    if (!m_window)
    {
        return false;
    }

    clearWindowBackdrop();

    if (key.isEmpty() || key == QStringLiteral("none"))
    {
        return true;
    }

    if (key == QStringLiteral("dwm-blur"))
    {
        m_backdropEnabled = DwmBackdrop::enable(m_window);
        return m_backdropEnabled;
    }

    if (!m_windowAgent)
    {
        return false;
    }

    return m_windowAgent->setWindowAttribute(key, true);
}

void FluentWindowFrame::setupWindowAgent()
{
    m_windowAgent = new QWK::WidgetWindowAgent(m_window);
    m_windowAgent->setup(m_window);
}

void FluentWindowFrame::installChromeHeader(QMenuBar *menuBar)
{
    m_menuBar = menuBar;

    if (!m_titleBar)
    {
        m_titleBar = new FluentTitleBar(m_window);
    }

    if (!m_chromeHeader)
    {
        m_chromeHeader = new QWidget(m_window);
        m_chromeHeader->setObjectName(QStringLiteral("fluent-chrome-header"));

        auto *layout = new QVBoxLayout(m_chromeHeader);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(m_titleBar);
        layout->addWidget(m_menuBar);
    }

    attachChromeHeader();
}

void FluentWindowFrame::attachChromeHeader()
{
    if (!m_chromeHeader || !m_titleBar)
    {
        return;
    }

    m_window->setMenuWidget(m_chromeHeader);
    m_chromeHeader->show();
    m_titleBar->show();
    if (m_menuBar)
    {
        m_menuBar->show();
    }

    m_windowAgent->setTitleBar(m_titleBar);
#ifndef Q_OS_MAC
    m_windowAgent->setHitTestVisible(m_titleBar->themeButton(), true);
    m_windowAgent->setHitTestVisible(m_titleBar->pinButton(), true);
    m_windowAgent->setHitTestVisible(m_titleBar->searchLineEdit(), true);
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Minimize, m_titleBar->minButton());
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize, m_titleBar->maxButton());
    m_windowAgent->setSystemButton(QWK::WindowAgentBase::Close, m_titleBar->closeButton());
#endif
    if (m_menuBar)
    {
        m_windowAgent->setHitTestVisible(m_menuBar, true);
    }
}

bool FluentWindowFrame::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window)
    {
        switch (event->type())
        {
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
            if (m_chromeHeader)
            {
                m_chromeHeader->setProperty("bar-active", event->type() == QEvent::WindowActivate);
                m_window->style()->polish(m_chromeHeader);
            }
            break;
        case QEvent::Show:
        case QEvent::WinIdChange:
            attachChromeHeader();
            if (m_backdropEnabled)
            {
                scheduleBackdropReapply(m_window);
            }
            break;
        default:
            break;
        }
    }

    return QObject::eventFilter(watched, event);
}
