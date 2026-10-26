#include "playerwindow.h"

#include "audiomaticplayerwidget.h"

#include <QEvent>
#include <QShowEvent>
#include <QTimer>

#ifdef Q_OS_WIN
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#endif

#ifdef AUDIOMATIC_ENABLE_FRAMELESS
#    include <QApplication>
#    include <QLineEdit>
#    include <QToolButton>

#    include <fluenttitlebar.h>
#    include <fluentwindowframe.h>

#    ifdef HAS_FLUENTUI3_STYLE_LIB
#        include "fluentui3style.h"
#    endif
#endif

#include <QMenuBar>

PlayerWindow::PlayerWindow( QWidget* parent )
    : QMainWindow( parent )
{
    setObjectName( QStringLiteral( "AudiomaticMini" ) );
    setAttribute( Qt::WA_DontCreateNativeAncestors );
    resize( 650, 680 );
    setMinimumSize( 380, 200 );

    auto* menuBar = new QMenuBar( this );
    menuBar->setObjectName( QStringLiteral( "win-menu-bar" ) );
    menuBar->hide();

#ifdef AUDIOMATIC_ENABLE_FRAMELESS
    m_windowFrame = new FluentWindowFrame( this, this );
    m_windowFrame->installChromeHeader( menuBar );
    setupTitleBar();
#else
    setMenuBar( menuBar );
#endif

    m_playerWidget = new AudiomaticPlayerWidget( this );
    setCentralWidget( m_playerWidget );
    connect( m_playerWidget, &AudiomaticPlayerWidget::windowChromeChanged, this, &PlayerWindow::applyWindowChrome );

    setWindowTitle( tr( "Audiomatic Mini" ) );
    setWindowIcon( QIcon( QStringLiteral( ":/music.ico" ) ) );
}

void PlayerWindow::showEvent( QShowEvent* event )
{
    QMainWindow::showEvent( event );
    // 无边框标题栏在 Show 时才创建原生窗口，构造期间设的透明度留不住。
    applyWindowChrome();
    QTimer::singleShot( 0, this, [ this ]() { applyWindowChrome(); } );
}

void PlayerWindow::changeEvent( QEvent* event )
{
    QMainWindow::changeEvent( event );
    if ( event->type() == QEvent::WinIdChange )
    {
        applyWindowChrome();
    }
}

void PlayerWindow::applyWindowChrome()
{
    if ( !m_playerWidget || m_applyingChrome )
    {
        return;
    }

    m_applyingChrome = true;

    const bool top = m_playerWidget->isAlwaysOnTop();
#ifdef Q_OS_WIN
    if ( testAttribute( Qt::WA_WState_Created ) )
    {
        HWND hwnd = reinterpret_cast<HWND>( winId() );
        if ( hwnd )
        {
            SetWindowPos( hwnd,
                          top ? HWND_TOPMOST : HWND_NOTOPMOST,
                          0, 0, 0, 0,
                          SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
        }
    }
#else
    const bool stayedOnTop = windowFlags().testFlag( Qt::WindowStaysOnTopHint );
    if ( stayedOnTop != top )
    {
        setWindowFlag( Qt::WindowStaysOnTopHint, top );
        show();
    }
#endif

    // 置顶会动到原生窗口。透明度放在后面设，避免被这次窗口重建清掉。
    const double opacity = m_playerWidget->isTranslucentEnabled()
        ? ( qBound( 30, m_playerWidget->translucentOpacity(), 100 ) / 100.0 )
        : 1.0;
    setWindowOpacity( opacity );

    m_applyingChrome = false;
}

PlayerWindow::~PlayerWindow() = default;

#ifdef AUDIOMATIC_ENABLE_FRAMELESS
void PlayerWindow::setupTitleBar()
{
    if ( FluentTitleBar* titleBar = m_windowFrame->titleBar() )
    {
        titleBar->searchLineEdit()->hide();
        titleBar->pinButton()->hide();

        const bool isDark = qApp->property( "_q_colorscheme" ).toInt() == 1;
        titleBar->setThemeDark( isDark );
        titleBar->themeButton()->show();

        connect( titleBar->themeButton(),
                 &QToolButton::clicked,
                 this,
                 [ titleBar ]()
                 {
                     const int current   = qApp->property( "_q_colorscheme" ).toInt();
                     const int nextTheme = ( current == 1 ? 0 : 1 );
                     qApp->setProperty( "_q_colorscheme", nextTheme );
                     qApp->setStyle( QStringLiteral( "FluentUI3" ) );
                     titleBar->setThemeDark( nextTheme == 1 );
                 } );

        connect( titleBar,
                 &FluentTitleBar::accentColorChanged,
                 this,
                 []( const QColor& color )
                 {
                     qApp->setProperty( "_q_accent_color", color.isValid() ? QVariant( color ) : QVariant() );
                     qApp->setStyle( QStringLiteral( "FluentUI3" ) );
                 } );
    }
}
#endif
