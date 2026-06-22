#include "playerwindow.h"

#include "audiomaticplayerwidget.h"
#include "fluenttitlebar.h"
#include "fluentwindowframe.h"

#include <QMenuBar>
#include <QLineEdit>
#include <QToolButton>

PlayerWindow::PlayerWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName(QStringLiteral("AudiomaticMini"));
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    resize(420, 680);
    setMinimumSize(380, 560);

    auto *menuBar = new QMenuBar(this);
    menuBar->setObjectName(QStringLiteral("win-menu-bar"));
    menuBar->hide();

    m_windowFrame = new FluentWindowFrame(this, this);
    m_windowFrame->installChromeHeader(menuBar);
    setupTitleBar();

    m_playerWidget = new AudiomaticPlayerWidget(this);
    setCentralWidget(m_playerWidget);

    setWindowTitle(tr("Audiomatic Mini"));
}

PlayerWindow::~PlayerWindow() = default;

void PlayerWindow::setupTitleBar()
{
    if (FluentTitleBar *titleBar = m_windowFrame->titleBar())
    {
        titleBar->searchLineEdit()->hide();
        titleBar->themeButton()->hide();
        titleBar->pinButton()->hide();
    }
}
