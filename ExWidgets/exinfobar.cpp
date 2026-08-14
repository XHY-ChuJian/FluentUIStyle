#include "exinfobar.h"

#include <QEasingCurve>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QStyle>
#include <QToolButton>
#include <QTextLayout>
#include <QVariantAnimation>
#include <qmath.h>

namespace
{
constexpr int CornerRadius = 4;
constexpr int InfoBarMinHeight = 48;
constexpr int PopupInfoBarMaxHeight = 160;
constexpr int PopupMessageMaxLines = 3;
constexpr int ContentLeftPadding = 16;
constexpr int IconSize = 16;
constexpr int IconTopMargin = 16;
constexpr int IconTrailingMargin = 14;
constexpr int PanelTrailingMargin = 0;
constexpr int CloseButtonSize = 32;
constexpr int CloseButtonMargin = 4;
constexpr int HorizontalTitleTop = 14;
constexpr int HorizontalMessageLeading = 12;
constexpr int HorizontalActionLeading = 16;
constexpr int HorizontalActionTop = 8;
constexpr int VerticalTopPadding = 14;
constexpr int VerticalMessageTop = 4;
constexpr int VerticalActionTop = 12;
constexpr int VerticalBottomPadding = 18;
constexpr const char* PopupSurfaceProperty = "_q_exInfoBarPopupSurface";

bool isDarkPalette( const QPalette& palette )
{
    return palette.color( QPalette::Window ).lightness() < 128;
}

QColor severityColor( ExInfoBar::Severity severity, const QPalette& palette )
{
    const bool dark = isDarkPalette( palette );
    switch ( severity )
    {
        case ExInfoBar::Success : return dark ? QColor( 108, 203, 95 ) : QColor( 15, 123, 15 );
        case ExInfoBar::Warning : return dark ? QColor( 252, 225, 0 ) : QColor( 157, 93, 0 );
        case ExInfoBar::Error : return dark ? QColor( 255, 153, 164 ) : QColor( 196, 43, 28 );
        case ExInfoBar::Informational :
        default : return palette.color( QPalette::Highlight );
    }
}

QColor severityBackground( ExInfoBar::Severity severity, const QPalette& palette )
{
    if ( isDarkPalette( palette ) )
    {
        switch ( severity )
        {
            case ExInfoBar::Success : return QColor( 57, 61, 27 );
            case ExInfoBar::Warning : return QColor( 67, 53, 25 );
            case ExInfoBar::Error : return QColor( 68, 39, 38 );
            case ExInfoBar::Informational :
            default : return QColor( 255, 255, 255, 8 );
        }
    }

    switch ( severity )
    {
        case ExInfoBar::Success : return QColor( 223, 246, 221 );
        case ExInfoBar::Warning : return QColor( 255, 244, 206 );
        case ExInfoBar::Error : return QColor( 253, 231, 233 );
        case ExInfoBar::Informational :
        default : return QColor( 246, 246, 246, 128 );
    }
}

QColor inverseTextColor( const QColor& background )
{
    const qreal luminance = 0.2126 * background.redF()
                            + 0.7152 * background.greenF()
                            + 0.0722 * background.blueF();
    return luminance > 0.58 ? QColor( 0, 0, 0 ) : QColor( 255, 255, 255 );
}

int itemHeightForWidth( QWidget* widget, int width )
{
    if ( !widget || widget->isHidden() )
    {
        return 0;
    }
    return widget->hasHeightForWidth() ? widget->heightForWidth( qMax( 0, width ) )
                                       : widget->sizeHint().height();
}

class InfoBarCloseButton final : public QToolButton
{
public:
    using QToolButton::QToolButton;

protected:
    void paintEvent( QPaintEvent* event ) override
    {
        QToolButton::paintEvent( event );
        QPainter painter( this );
        painter.setRenderHint( QPainter::Antialiasing );
        painter.setPen( QPen( palette().color( isEnabled() ? QPalette::Active : QPalette::Disabled,
                                               QPalette::ButtonText ),
                              1.35,
                              Qt::SolidLine,
                              Qt::RoundCap ) );
        const QPointF center = rect().center();
        painter.drawLine( center + QPointF( -4.0, -4.0 ), center + QPointF( 4.0, 4.0 ) );
        painter.drawLine( center + QPointF( 4.0, -4.0 ), center + QPointF( -4.0, 4.0 ) );
    }
};
} // namespace

class ExInfoBar::AdaptivePanel final : public QWidget
{
public:
    explicit AdaptivePanel( QWidget* parent )
        : QWidget( parent )
    {
        QSizePolicy policy( QSizePolicy::Expanding, QSizePolicy::Preferred );
        policy.setHeightForWidth( true );
        setSizePolicy( policy );
    }

    void setItems( QWidget* title, QWidget* message, QWidget* action )
    {
        m_title = title;
        m_message = message;
        m_action = action;
        updateGeometry();
        updateItemGeometries();
    }

    void refreshLayout()
    {
        updateGeometry();
        updateItemGeometries();
    }

    QSize sizeHint() const override
    {
        const int width = horizontalNaturalWidth();
        return QSize( width, horizontalHeight() );
    }

    QSize minimumSizeHint() const override
    {
        int width = PanelTrailingMargin;
        for ( QWidget* item : { m_title, m_message, m_action } )
        {
            if ( item && !item->isHidden() )
            {
                width = qMax( width, item->minimumSizeHint().width() + PanelTrailingMargin );
            }
        }
        // 高度不能用极窄的 minimum width 计算，否则长文本会在该宽度下
        // 逐字换行，并被外层布局误认为是控件的最小高度。
        return QSize( width, InfoBarMinHeight );
    }

    bool hasHeightForWidth() const override
    {
        return true;
    }

    int heightForWidth( int width ) const override
    {
        return useHorizontalLayout( width ) ? horizontalHeight()
                                            : verticalHeightForWidth( width );
    }

protected:
    void resizeEvent( QResizeEvent* event ) override
    {
        QWidget::resizeEvent( event );
        updateItemGeometries();
    }

    void changeEvent( QEvent* event ) override
    {
        QWidget::changeEvent( event );
        if ( event->type() == QEvent::LayoutDirectionChange )
        {
            updateItemGeometries();
        }
    }

private:
    int horizontalNaturalWidth() const
    {
        int width = PanelTrailingMargin;
        bool hasPrevious = false;
        if ( m_title && !m_title->isHidden() )
        {
            width += m_title->sizeHint().width();
            hasPrevious = true;
        }
        if ( m_message && !m_message->isHidden() )
        {
            width += ( hasPrevious ? HorizontalMessageLeading : 0 ) + m_message->sizeHint().width();
            hasPrevious = true;
        }
        if ( m_action && !m_action->isHidden() )
        {
            width += ( hasPrevious ? HorizontalActionLeading : 0 ) + m_action->sizeHint().width();
        }
        return width;
    }

    int horizontalHeight() const
    {
        int height = 0;
        if ( m_title && !m_title->isHidden() )
        {
            height = qMax( height, HorizontalTitleTop + m_title->sizeHint().height() );
        }
        if ( m_message && !m_message->isHidden() )
        {
            height = qMax( height, HorizontalTitleTop + m_message->sizeHint().height() );
        }
        if ( m_action && !m_action->isHidden() )
        {
            height = qMax( height, HorizontalActionTop + m_action->sizeHint().height() );
        }
        return qMax( height, InfoBarMinHeight );
    }

    int verticalHeightForWidth( int width ) const
    {
        const int contentWidth = qMax( 0, width - PanelTrailingMargin );
        int height = VerticalTopPadding;
        bool hasPrevious = false;
        if ( m_title && !m_title->isHidden() )
        {
            height += itemHeightForWidth( m_title, contentWidth );
            hasPrevious = true;
        }
        if ( m_message && !m_message->isHidden() )
        {
            height += ( hasPrevious ? VerticalMessageTop : 0 )
                      + itemHeightForWidth( m_message, contentWidth );
            hasPrevious = true;
        }
        if ( m_action && !m_action->isHidden() )
        {
            height += ( hasPrevious ? VerticalActionTop : 0 )
                      + itemHeightForWidth( m_action, contentWidth );
            hasPrevious = true;
        }
        return hasPrevious ? height + VerticalBottomPadding : InfoBarMinHeight;
    }

    bool useHorizontalLayout( int width ) const
    {
        return width >= horizontalNaturalWidth();
    }

    QRect visualRect( const QRect& logical ) const
    {
        return QStyle::visualRect( layoutDirection(), rect(), logical );
    }

    void updateItemGeometries()
    {
        const int contentWidth = qMax( 0, width() - PanelTrailingMargin );
        if ( useHorizontalLayout( width() ) )
        {
            int x = 0;
            bool hasPrevious = false;
            if ( m_title && !m_title->isHidden() )
            {
                const QSize hint = m_title->sizeHint();
                m_title->setGeometry( visualRect( QRect( x, HorizontalTitleTop,
                                                         hint.width(), hint.height() ) ) );
                x += hint.width();
                hasPrevious = true;
            }
            if ( m_message && !m_message->isHidden() )
            {
                x += hasPrevious ? HorizontalMessageLeading : 0;
                const int actionWidth = m_action && !m_action->isHidden()
                                            ? HorizontalActionLeading + m_action->sizeHint().width()
                                            : 0;
                // Message 是横向 InfoBar 的弹性区域：占满标题之后、
                // Action（若存在）之前的全部剩余宽度。
                const int messageWidth = qMax( 0, contentWidth - x - actionWidth );
                const int messageHeight = itemHeightForWidth( m_message, messageWidth );
                m_message->setGeometry( visualRect( QRect( x, HorizontalTitleTop,
                                                           messageWidth, messageHeight ) ) );
                x += messageWidth;
                hasPrevious = true;
            }
            if ( m_action && !m_action->isHidden() )
            {
                x += hasPrevious ? HorizontalActionLeading : 0;
                const QSize hint = m_action->sizeHint();
                m_action->setGeometry( visualRect( QRect( x, HorizontalActionTop,
                                                          hint.width(), hint.height() ) ) );
            }
            return;
        }

        int y = VerticalTopPadding;
        bool hasPrevious = false;
        int messageHeightLimit = height();
        if ( m_message && !m_message->isHidden() )
        {
            messageHeightLimit -= VerticalTopPadding + VerticalBottomPadding;
            if ( m_title && !m_title->isHidden() )
            {
                messageHeightLimit -= itemHeightForWidth( m_title, contentWidth )
                                      + VerticalMessageTop;
            }
            if ( m_action && !m_action->isHidden() )
            {
                messageHeightLimit -= itemHeightForWidth( m_action, contentWidth )
                                      + VerticalActionTop;
            }
            messageHeightLimit = qMax( 0, messageHeightLimit );
        }
        for ( QWidget* item : { m_title, m_message, m_action } )
        {
            if ( !item || item->isHidden() )
            {
                continue;
            }
            if ( hasPrevious )
            {
                y += item == m_action ? VerticalActionTop : VerticalMessageTop;
            }
            int itemHeight = itemHeightForWidth( item, contentWidth );
            if ( item == m_message )
            {
                // 超长消息只使用 Title 和 Action 之外的剩余高度，避免
                // 把操作区域推出 InfoBar 的最大可见范围。
                itemHeight = qMin( itemHeight, messageHeightLimit );
            }
            item->setGeometry( visualRect( QRect( 0, y, contentWidth, itemHeight ) ) );
            y += itemHeight;
            hasPrevious = true;
        }
    }

    QWidget* m_title = nullptr;
    QWidget* m_message = nullptr;
    QWidget* m_action = nullptr;
};

class ExInfoBar::MessageLabel final : public QLabel
{
public:
    explicit MessageLabel( ExInfoBar* infoBar )
        : QLabel( infoBar->m_adaptivePanel )
        , m_infoBar( infoBar )
    {
    }

    void setFullText( const QString& text )
    {
        if ( m_fullText == text )
        {
            return;
        }
        m_fullText = text;
        refreshElision();
    }

    void refreshElision()
    {
        const QString displayText = isPopup() ? elidedText( width(), visibleLineCount() )
                                              : m_fullText;
        if ( QLabel::text() != displayText )
        {
            QLabel::setText( displayText );
        }
        setToolTip( displayText == m_fullText ? QString() : m_fullText );
    }

    QSize sizeHint() const override
    {
        if ( !isPopup() || m_fullText.isEmpty() )
        {
            return QLabel::sizeHint();
        }
        const QFontMetrics metrics( font() );
        return metrics.boundingRect( m_fullText ).size();
    }

    int heightForWidth( int width ) const override
    {
        if ( !isPopup() )
        {
            return QLabel::heightForWidth( width );
        }
        return wrappedHeight( qMax( 0, width ) );
    }

protected:
    void resizeEvent( QResizeEvent* event ) override
    {
        QLabel::resizeEvent( event );
        refreshElision();
    }

    void changeEvent( QEvent* event ) override
    {
        QLabel::changeEvent( event );
        if ( event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange )
        {
            refreshElision();
        }
    }

private:
    bool isPopup() const
    {
        return m_infoBar->property( PopupSurfaceProperty ).toBool();
    }

    QTextOption textOption() const
    {
        QTextOption option;
        option.setWrapMode( QTextOption::WrapAtWordBoundaryOrAnywhere );
        option.setTextDirection( layoutDirection() );
        return option;
    }

    int visibleLineCount() const
    {
        if ( height() <= 0 )
        {
            return PopupMessageMaxLines;
        }
        const int lineSpacing = qMax( 1, QFontMetrics( font() ).lineSpacing() );
        return qBound( 1, height() / lineSpacing, PopupMessageMaxLines );
    }

    int wrappedHeight( int width ) const
    {
        if ( m_fullText.isEmpty() || width <= 0 )
        {
            return 0;
        }
        QTextLayout layout( m_fullText, font() );
        layout.setTextOption( textOption() );
        layout.beginLayout();
        qreal height = 0.0;
        for ( int index = 0; index < PopupMessageMaxLines; ++index )
        {
            QTextLine line = layout.createLine();
            if ( !line.isValid() )
            {
                break;
            }
            line.setLineWidth( width );
            height += line.height();
        }
        layout.endLayout();
        return qCeil( height );
    }

    QString elidedText( int width, int maximumLines ) const
    {
        if ( m_fullText.isEmpty() || width <= 0 )
        {
            return m_fullText;
        }

        QTextLayout layout( m_fullText, font() );
        layout.setTextOption( textOption() );
        layout.beginLayout();
        QTextLine lastLine;
        for ( int index = 0; index < maximumLines; ++index )
        {
            lastLine = layout.createLine();
            if ( !lastLine.isValid() )
            {
                break;
            }
            lastLine.setLineWidth( width );
        }
        layout.endLayout();

        if ( !lastLine.isValid()
             || lastLine.textStart() + lastLine.textLength() >= m_fullText.size() )
        {
            return m_fullText;
        }

        const int lastLineStart = lastLine.textStart();
        QString remainder = m_fullText.mid( lastLineStart );
        remainder.replace( QChar( '\n' ), QChar( ' ' ) );
        const QString lastLineText = QFontMetrics( font() ).elidedText(
            remainder, Qt::ElideRight, width );
        return m_fullText.left( lastLineStart ) + lastLineText;
    }

    ExInfoBar* m_infoBar = nullptr;
    QString m_fullText;
};

class ExInfoBar::IconWidget final : public QWidget
{
public:
    explicit IconWidget( ExInfoBar* infoBar )
        : QWidget( infoBar )
        , m_infoBar( infoBar )
    {
        setFixedWidth( IconSize + IconTrailingMargin );
        setMinimumHeight( InfoBarMinHeight );
        setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
        setAttribute( Qt::WA_TransparentForMouseEvents );
    }

protected:
    void paintEvent( QPaintEvent* ) override
    {
        QPainter painter( this );
        painter.setRenderHint( QPainter::Antialiasing );

        const QColor background = severityColor( m_infoBar->severity(), palette() );
        const QColor foreground = inverseTextColor( background );
        const QPointF center( IconSize * 0.5, IconTopMargin + IconSize * 0.5 );
        painter.setPen( Qt::NoPen );
        painter.setBrush( background );
        painter.drawEllipse( center, IconSize * 0.5, IconSize * 0.5 );
        painter.setBrush( Qt::NoBrush );
        painter.setPen( QPen( foreground, 1.45, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );

        painter.save();
        painter.translate( center );
        switch ( m_infoBar->severity() )
        {
            case ExInfoBar::Success :
                painter.drawLine( QPointF( -3.5, 0.0 ), QPointF( -1.0, 2.5 ) );
                painter.drawLine( QPointF( -1.0, 2.5 ), QPointF( 4.0, -2.5 ) );
                break;
            case ExInfoBar::Warning :
                painter.drawLine( QPointF( 0.0, -3.5 ), QPointF( 0.0, 1.0 ) );
                painter.drawPoint( QPointF( 0.0, 3.5 ) );
                break;
            case ExInfoBar::Error :
                painter.drawLine( QPointF( -2.8, -2.8 ), QPointF( 2.8, 2.8 ) );
                painter.drawLine( QPointF( 2.8, -2.8 ), QPointF( -2.8, 2.8 ) );
                break;
            case ExInfoBar::Informational :
            default :
                painter.drawLine( QPointF( 0.0, -0.5 ), QPointF( 0.0, 3.5 ) );
                painter.drawPoint( QPointF( 0.0, -3.2 ) );
                break;
        }
        painter.restore();
    }

private:
    ExInfoBar* m_infoBar = nullptr;
};

ExInfoBar::ExInfoBar( QWidget* parent )
    : QWidget( parent )
    , m_heightAnimation( new QVariantAnimation( this ) )
{
    QSizePolicy policy( QSizePolicy::Preferred, QSizePolicy::Maximum );
    policy.setHeightForWidth( true );
    setSizePolicy( policy );
    setAttribute( Qt::WA_StyledBackground, false );

    auto* root = new QHBoxLayout( this );
    root->setContentsMargins( ContentLeftPadding, 0, 0, 0 );
    root->setSpacing( 0 );

    m_iconWidget = new IconWidget( this );
    root->addWidget( m_iconWidget );

    m_adaptivePanel = new AdaptivePanel( this );
    root->addWidget( m_adaptivePanel, 1 );

    m_titleLabel = new QLabel( m_adaptivePanel );
    m_titleLabel->setWordWrap( true );
    QFont titleFont = m_titleLabel->font();
    titleFont.setPixelSize( 14 );
    titleFont.setWeight( QFont::DemiBold );
    m_titleLabel->setFont( titleFont );

    m_messageLabel = new MessageLabel( this );
    m_messageLabel->setWordWrap( true );
    QFont messageFont = m_messageLabel->font();
    messageFont.setPixelSize( 13 );
    messageFont.setWeight( QFont::Normal );
    m_messageLabel->setFont( messageFont );
    m_messageLabel->setTextInteractionFlags( Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse );
    m_messageLabel->setOpenExternalLinks( false );

    m_actionContainer = new QWidget( m_adaptivePanel );
    m_actionLayout = new QHBoxLayout( m_actionContainer );
    m_actionLayout->setContentsMargins( 0, 0, 0, 0 );
    m_actionLayout->setSpacing( 8 );

    m_actionButton = new QPushButton( m_actionContainer );
    m_actionButton->setMinimumSize( 96, 24 );
    m_actionLayout->addWidget( m_actionButton );
    m_adaptivePanel->setItems( m_titleLabel, m_messageLabel, m_actionContainer );

    auto* closeArea = new QWidget( this );
    closeArea->setFixedSize( CloseButtonSize + CloseButtonMargin * 2,
                             InfoBarMinHeight );
    auto* closeLayout = new QHBoxLayout( closeArea );
    const int closeButtonVerticalMargin = ( InfoBarMinHeight - CloseButtonSize ) / 2;
    closeLayout->setContentsMargins( CloseButtonMargin, closeButtonVerticalMargin,
                                     CloseButtonMargin, closeButtonVerticalMargin );
    closeLayout->setSpacing( 0 );
    m_closeButton = new InfoBarCloseButton( closeArea );
    m_closeButton->setToolTip( tr( "关闭" ) );
    m_closeButton->setAccessibleName( tr( "关闭通知" ) );
    m_closeButton->setAutoRaise( true );
    m_closeButton->setFixedSize( CloseButtonSize, CloseButtonSize );
    closeLayout->addWidget( m_closeButton );
    root->addWidget( closeArea, 0, Qt::AlignTop );

    connect( m_actionButton, &QPushButton::clicked, this, &ExInfoBar::actionTriggered );
    connect( m_closeButton, &QToolButton::clicked, this, [this]()
    {
        emit closeButtonClicked();
        dismiss();
    } );
    connect( m_heightAnimation, &QVariantAnimation::valueChanged, this, [this]( const QVariant& value )
    {
        setMaximumHeight( value.toInt() );
        updateGeometry();
    } );
    connect( m_heightAnimation, &QVariantAnimation::finished, this, &ExInfoBar::finishTransition );

    updateTextVisibility();
    updateActionVisibility();
    setMaximumHeight( 0 );
    hide();
}

ExInfoBar::~ExInfoBar()
{
    // QWidget 的子控件在派生类析构体之后才会被逐个删除。自定义 Action
    // 析构时不能再回调并访问已经进入析构流程的兄弟控件和布局。
    disconnect( m_actionWidgetDestroyedConnection );
}

ExInfoBar::Severity ExInfoBar::severity() const
{
    return m_severity;
}

void ExInfoBar::setSeverity( Severity severity )
{
    if ( m_severity == severity )
    {
        return;
    }
    m_severity = severity;
    m_iconWidget->update();
    update();
    emit severityChanged( severity );
}

QString ExInfoBar::title() const
{
    return m_title;
}

void ExInfoBar::setTitle( const QString& title )
{
    if ( m_title == title )
    {
        return;
    }
    m_title = title;
    m_titleLabel->setText( title );
    updateTextVisibility();
    updateAccessibleText();
    emit titleChanged( title );
}

QString ExInfoBar::message() const
{
    return m_message;
}

void ExInfoBar::setMessage( const QString& message )
{
    if ( m_message == message )
    {
        return;
    }
    m_message = message;
    m_messageLabel->setFullText( message );
    updateTextVisibility();
    updateAccessibleText();
    emit messageChanged( message );
}

QString ExInfoBar::actionButtonText() const
{
    return m_actionButtonText;
}

void ExInfoBar::setActionButtonText( const QString& text )
{
    if ( m_actionButtonText == text )
    {
        return;
    }
    m_actionButtonText = text;
    m_actionButton->setText( text );
    updateActionVisibility();
    emit actionButtonTextChanged( text );
}

bool ExInfoBar::isOpen() const
{
    return m_open;
}

bool ExInfoBar::isClosable() const
{
    return m_closable;
}

void ExInfoBar::setClosable( bool closable )
{
    if ( m_closable == closable )
    {
        return;
    }
    m_closable = closable;
    m_closeButton->parentWidget()->setVisible( closable );
    updateGeometry();
    emit closableChanged( closable );
}

bool ExInfoBar::isIconVisible() const
{
    return m_iconVisible;
}

void ExInfoBar::setIconVisible( bool visible )
{
    if ( m_iconVisible == visible )
    {
        return;
    }
    m_iconVisible = visible;
    m_iconWidget->setVisible( visible );
    updateGeometry();
    emit iconVisibleChanged( visible );
}

bool ExInfoBar::isAnimationEnabled() const
{
    return m_animationEnabled;
}

void ExInfoBar::setAnimationEnabled( bool enabled )
{
    if ( m_animationEnabled == enabled )
    {
        return;
    }
    m_animationEnabled = enabled;
    if ( !enabled && m_heightAnimation->state() == QAbstractAnimation::Running )
    {
        m_heightAnimation->stop();
        finishTransition();
    }
    emit animationEnabledChanged( enabled );
}

int ExInfoBar::animationDuration() const
{
    return m_animationDuration;
}

void ExInfoBar::setAnimationDuration( int duration )
{
    duration = qMax( 0, duration );
    if ( m_animationDuration == duration )
    {
        return;
    }
    m_animationDuration = duration;
    emit animationDurationChanged( duration );
}

QPushButton* ExInfoBar::actionButton() const
{
    return m_actionButton;
}

QWidget* ExInfoBar::actionWidget() const
{
    return m_actionWidget.data();
}

void ExInfoBar::setActionWidget( QWidget* widget )
{
    if ( widget == this
         || widget == m_actionButton
         || widget == m_closeButton
         || widget == m_iconWidget
         || widget == m_adaptivePanel
         || widget == m_actionContainer
         || widget == m_titleLabel
         || widget == m_messageLabel
         || widget == m_actionWidget.data()
         || ( widget && m_actionWidget && m_actionWidget->isAncestorOf( widget ) )
         || ( widget && widget->isAncestorOf( this ) ) )
    {
        return;
    }
    if ( m_actionWidget )
    {
        disconnect( m_actionWidgetDestroyedConnection );
        m_actionLayout->removeWidget( m_actionWidget );
        delete m_actionWidget.data();
    }
    m_actionWidget = widget;
    if ( widget )
    {
        widget->setParent( m_actionContainer );
        m_actionLayout->insertWidget( 0, widget );
        widget->show();
        m_actionWidgetDestroyedConnection = connect( widget, &QObject::destroyed, this, [this]
        {
            m_actionWidget = nullptr;
            updateActionVisibility();
            updateGeometry();
        } );
    }
    updateActionVisibility();
    updateGeometry();
}

QWidget* ExInfoBar::takeActionWidget()
{
    QWidget* widget = m_actionWidget.data();
    if ( widget )
    {
        disconnect( m_actionWidgetDestroyedConnection );
        m_actionLayout->removeWidget( widget );
        m_actionWidget = nullptr;
        widget->setParent( nullptr );
        updateActionVisibility();
        updateGeometry();
    }
    return widget;
}

QSize ExInfoBar::sizeHint() const
{
    QSize result = layout() ? layout()->sizeHint().expandedTo( QSize( 0, InfoBarMinHeight ) )
                            : QWidget::sizeHint();
    if ( property( PopupSurfaceProperty ).toBool() )
    {
        result.setHeight( qMin( PopupInfoBarMaxHeight, result.height() ) );
    }
    return result;
}

QSize ExInfoBar::minimumSizeHint() const
{
    return layout() ? layout()->minimumSize().expandedTo( QSize( 0, InfoBarMinHeight ) )
                    : QWidget::minimumSizeHint();
}

bool ExInfoBar::hasHeightForWidth() const
{
    return layout() && layout()->hasHeightForWidth();
}

int ExInfoBar::heightForWidth( int width ) const
{
    int height = sizeHint().height();
    if ( layout() && layout()->hasHeightForWidth() )
    {
        height = qMax( InfoBarMinHeight, layout()->heightForWidth( width ) );
    }
    if ( property( PopupSurfaceProperty ).toBool() )
    {
        height = qMin( PopupInfoBarMaxHeight, height );
    }
    return height;
}

void ExInfoBar::setOpen( bool open )
{
    if ( m_open == open )
    {
        return;
    }
    m_open = open;
    emit openChanged( open );

    if ( property( PopupSurfaceProperty ).toBool() )
    {
        m_heightAnimation->stop();
        if ( open )
        {
            setMaximumHeight( PopupInfoBarMaxHeight );
            show();
            updateGeometry();
            emit opened();
        }
        // 弹出关闭由 ExInfoBarHost 完成 Element 风格淡出后收尾。
        return;
    }

    m_heightAnimation->stop();
    const bool canAnimate = m_animationEnabled && m_animationDuration > 0
                            && parentWidget() && parentWidget()->isVisible();
    if ( !canAnimate )
    {
        setMaximumHeight( open ? QWIDGETSIZE_MAX : 0 );
        setVisible( open );
        updateGeometry();
        if ( open )
        {
            emit opened();
        }
        else
        {
            emit closed();
        }
        return;
    }

    int startHeight = isVisible() ? height() : 0;
    int endHeight = 0;
    if ( open )
    {
        const int currentHeight = startHeight;
        setMaximumHeight( QWIDGETSIZE_MAX );
        show();
        if ( layout() )
        {
            layout()->activate();
        }
        endHeight = hasHeightForWidth() ? heightForWidth( width() )
                                        : sizeHint().height();
        startHeight = qMin( currentHeight, endHeight );
        setMaximumHeight( startHeight );
    }

    m_heightAnimation->setDuration( m_animationDuration );
    m_heightAnimation->setEasingCurve( open ? QEasingCurve::OutCubic : QEasingCurve::InCubic );
    m_heightAnimation->setStartValue( startHeight );
    m_heightAnimation->setEndValue( endHeight );
    m_heightAnimation->start();
}

void ExInfoBar::dismiss()
{
    setOpen( false );
}

void ExInfoBar::paintEvent( QPaintEvent* )
{
    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );

    QColor border = palette().color( QPalette::Mid );
    border.setAlpha( isDarkPalette( palette() ) ? 150 : 88 );
    const QRectF bounds = QRectF( rect() ).adjusted( 0.5, 0.5, -0.5, -0.5 );
    QColor background = severityBackground( m_severity, palette() );
    if ( m_severity == Informational && property( PopupSurfaceProperty ).toBool() )
    {
        // 页面内 InfoBar 使用半透明 AttentionBackground；窗口级弹出必须使用
        // SolidAttentionBackground，避免直接透出主窗口内容。
        background = isDarkPalette( palette() ) ? QColor( 46, 46, 46 )
                                                : QColor( 247, 247, 247 );
    }
    painter.setPen( QPen( border, 1.0 ) );
    painter.setBrush( background );
    painter.drawRoundedRect( bounds, CornerRadius, CornerRadius );
}

bool ExInfoBar::event( QEvent* event )
{
    const bool result = QWidget::event( event );
    if ( event->type() == QEvent::DynamicPropertyChange && m_messageLabel )
    {
        m_messageLabel->refreshElision();
        updateGeometry();
    }
    return result;
}

void ExInfoBar::changeEvent( QEvent* event )
{
    QWidget::changeEvent( event );
    if ( event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange
         || event->type() == QEvent::StyleChange || event->type() == QEvent::EnabledChange )
    {
        m_iconWidget->update();
        update();
    }
}

void ExInfoBar::updateTextVisibility()
{
    m_titleLabel->setVisible( !m_title.isEmpty() );
    m_messageLabel->setVisible( !m_message.isEmpty() );
    m_adaptivePanel->refreshLayout();
    updateGeometry();
}

void ExInfoBar::updateAccessibleText()
{
    const QString text = m_title.isEmpty() ? m_message
                                            : ( m_message.isEmpty() ? m_title
                                                                    : m_title + QStringLiteral( ". " ) + m_message );
    setAccessibleName( text );
}

void ExInfoBar::updateActionVisibility()
{
    m_actionButton->setVisible( !m_actionWidget && !m_actionButtonText.isEmpty() );
    if ( m_actionWidget )
    {
        m_actionWidget->show();
    }
    m_actionContainer->setVisible( m_actionWidget || !m_actionButtonText.isEmpty() );
    m_adaptivePanel->refreshLayout();
}

void ExInfoBar::finishTransition()
{
    if ( m_open )
    {
        setMaximumHeight( QWIDGETSIZE_MAX );
        show();
        emit opened();
    }
    else
    {
        setMaximumHeight( 0 );
        hide();
        emit closed();
    }
    updateGeometry();
}

void ExInfoBar::finishPopupClose()
{
    if ( m_open )
    {
        return;
    }
    setMaximumHeight( 0 );
    hide();
    updateGeometry();
    emit closed();
}
