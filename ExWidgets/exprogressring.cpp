#include "exprogressring.h"

#include "fluentui3styleproperties.h"

#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFontMetricsF>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QStyleOptionProgressBar>
#include <QStylePainter>
#include <QtMath>

namespace
{
bool isDefaultFont( const QFont& font )
{
    return font == QFont();
}
}

ExProgressRing::ExProgressRing( QWidget* parent )
    : QProgressBar( parent )
{
    setProperty( ProgressBarStyleProperty, ProgressBarRing );
    setAlignment( Qt::AlignCenter );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
}

#define EX_PROGRESS_RING_SIMPLE_SETTER( Type, Property, Setter ) \
    void ExProgressRing::Setter( Type value ) \
    { \
        if ( m_##Property == value ) \
        { \
            return; \
        } \
        m_##Property = value; \
        update(); \
        emit Property##Changed( value ); \
    }

EX_PROGRESS_RING_SIMPLE_SETTER( QString, title, setTitle )
EX_PROGRESS_RING_SIMPLE_SETTER( QFont, titleFont, setTitleFont )
EX_PROGRESS_RING_SIMPLE_SETTER( QFont, valueFont, setValueFont )
EX_PROGRESS_RING_SIMPLE_SETTER( QColor, titleColor, setTitleColor )
EX_PROGRESS_RING_SIMPLE_SETTER( QColor, valueColor, setValueColor )

#undef EX_PROGRESS_RING_SIMPLE_SETTER

void ExProgressRing::setTextSpacing( int spacing )
{
    spacing = qBound( 0, spacing, 100 );
    if ( m_textSpacing == spacing )
    {
        return;
    }

    m_textSpacing = spacing;
    update();
    emit textSpacingChanged( spacing );
}

QWidget* ExProgressRing::centerWidget() const
{
    return m_centerWidget.data();
}

void ExProgressRing::setCenterWidget( QWidget* widget )
{
    if ( widget == this
         || widget == m_centerWidget.data()
         || ( widget && m_centerWidget && m_centerWidget->isAncestorOf( widget ) )
         || ( widget && widget->isAncestorOf( this ) ) )
    {
        return;
    }

    if ( m_centerWidget )
    {
        disconnect( m_centerWidgetDestroyedConnection );
        delete m_centerWidget.data();
    }

    m_centerWidget = widget;
    if ( widget )
    {
        widget->setParent( this );
        widget->raise();
        m_centerWidgetDestroyedConnection = connect( widget,
                                                     &QObject::destroyed,
                                                     this,
                                                     [this]
                                                     {
                                                         m_centerWidget = nullptr;
                                                         emit centerWidgetChanged( nullptr );
                                                         update();
                                                     } );
        updateCenterWidgetGeometry();
    }

    emit centerWidgetChanged( widget );
    update();
}

QWidget* ExProgressRing::takeCenterWidget()
{
    QWidget* widget = m_centerWidget.data();
    if ( !widget )
    {
        return nullptr;
    }

    disconnect( m_centerWidgetDestroyedConnection );
    m_centerWidget = nullptr;
    widget->hide();
    widget->setParent( nullptr );
    emit centerWidgetChanged( nullptr );
    update();
    return widget;
}

QSize ExProgressRing::sizeHint() const
{
    return QSize( 120, 120 );
}

QSize ExProgressRing::minimumSizeHint() const
{
    return QSize( 48, 48 );
}

bool ExProgressRing::event( QEvent* event )
{
    const bool handled = QProgressBar::event( event );
    if ( event->type() == QEvent::DynamicPropertyChange )
    {
        const auto* propertyEvent = static_cast<QDynamicPropertyChangeEvent*>( event );
        if ( propertyEvent->propertyName() == ProgressBarThicknessProperty
             || propertyEvent->propertyName() == ProgressBarStyleProperty )
        {
            updateCenterWidgetGeometry();
            update();
        }
    }
    return handled;
}

void ExProgressRing::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event )

    {
        QStylePainter stylePainter( this );
        QStyleOptionProgressBar option;
        initStyleOption( &option );
        option.textVisible = false;
        stylePainter.drawControl( QStyle::CE_ProgressBar, option );
    }

    updateCenterWidgetGeometry();
    if ( m_centerWidget )
    {
        m_centerWidget->setVisible( isTextVisible() );
        return;
    }

    if ( !isTextVisible() )
    {
        return;
    }

    const QString valueText = text();
    const bool hasTitle = !m_title.isEmpty();
    const bool hasValue = !valueText.isEmpty();
    if ( !hasTitle && !hasValue )
    {
        return;
    }

    QPainter painter( this );
    painter.setRenderHint( QPainter::TextAntialiasing, true );

    const QRectF contentRect = centerContentRect();
    const QFont titleFont = resolvedTitleFont();
    const QFont valueFont = resolvedValueFont();
    const QFontMetricsF titleMetrics( titleFont );
    const QFontMetricsF valueMetrics( valueFont );
    const qreal titleHeight = hasTitle ? titleMetrics.height() : 0.0;
    const qreal valueHeight = hasValue ? valueMetrics.height() : 0.0;
    const qreal spacing = hasTitle && hasValue ? m_textSpacing : 0.0;
    const qreal totalHeight = titleHeight + spacing + valueHeight;
    qreal top = contentRect.center().y() - totalHeight / 2.0;

    if ( hasTitle )
    {
        const QRectF titleRect( contentRect.left(), top, contentRect.width(), titleHeight );
        painter.setFont( titleFont );
        painter.setPen( resolvedTextColor( m_titleColor, true ) );
        painter.drawText( titleRect,
                          Qt::AlignCenter | Qt::TextSingleLine,
                          titleMetrics.elidedText( m_title, Qt::ElideRight, qRound( contentRect.width() ) ) );
        top += titleHeight + spacing;
    }

    if ( hasValue )
    {
        const QRectF valueRect( contentRect.left(), top, contentRect.width(), valueHeight );
        painter.setFont( valueFont );
        painter.setPen( resolvedTextColor( m_valueColor, false ) );
        painter.drawText( valueRect,
                          Qt::AlignCenter | Qt::TextSingleLine,
                          valueMetrics.elidedText( valueText, Qt::ElideRight, qRound( contentRect.width() ) ) );
    }
}

void ExProgressRing::resizeEvent( QResizeEvent* event )
{
    QProgressBar::resizeEvent( event );
    updateCenterWidgetGeometry();
}

void ExProgressRing::changeEvent( QEvent* event )
{
    QProgressBar::changeEvent( event );
    if ( event->type() == QEvent::FontChange
         || event->type() == QEvent::PaletteChange
         || event->type() == QEvent::StyleChange
         || event->type() == QEvent::EnabledChange )
    {
        updateCenterWidgetGeometry();
        update();
    }
}

QRectF ExProgressRing::centerContentRect() const
{
    const qreal side = qMin( width(), height() );
    QRectF square( ( width() - side ) / 2.0,
                   ( height() - side ) / 2.0,
                   side,
                   side );

    bool thicknessOk = false;
    qreal thickness = property( ProgressBarThicknessProperty ).toDouble( &thicknessOk );
    if ( !thicknessOk || !qIsFinite( thickness ) || thickness <= 0.0 )
    {
        thickness = ProgressBarRingDefaultThickness;
    }

    const qreal inset = qMin( side / 2.0, thickness + 6.0 );
    return square.marginsRemoved( QMarginsF( inset, inset, inset, inset ) );
}

QFont ExProgressRing::resolvedTitleFont() const
{
    if ( !isDefaultFont( m_titleFont ) )
    {
        return m_titleFont;
    }

    QFont result = font();
    result.setPixelSize( qBound( 9, qRound( qMin( width(), height() ) * 0.09 ), 14 ) );
    return result;
}

QFont ExProgressRing::resolvedValueFont() const
{
    if ( !isDefaultFont( m_valueFont ) )
    {
        return m_valueFont;
    }

    QFont result = font();
    result.setPixelSize( qBound( 12, qRound( qMin( width(), height() ) * 0.18 ), 32 ) );
    result.setWeight( QFont::DemiBold );
    return result;
}

QColor ExProgressRing::resolvedTextColor( const QColor& customColor, bool secondary ) const
{
    const QPalette::ColorGroup group = !isEnabled()
                                           ? QPalette::Disabled
                                           : isActiveWindow() ? QPalette::Active : QPalette::Inactive;
    QColor color = customColor.isValid()
                       ? customColor
                       : palette().color( group, QPalette::Text );
    if ( secondary && !customColor.isValid() )
    {
        color.setAlphaF( color.alphaF() * 0.72 );
    }
    if ( !isEnabled() && customColor.isValid() )
    {
        const QColor disabledText = palette().color( QPalette::Disabled, QPalette::Text );
        color.setRed( ( color.red() + disabledText.red() ) / 2 );
        color.setGreen( ( color.green() + disabledText.green() ) / 2 );
        color.setBlue( ( color.blue() + disabledText.blue() ) / 2 );
    }
    return color;
}

void ExProgressRing::updateCenterWidgetGeometry()
{
    if ( !m_centerWidget )
    {
        return;
    }

    m_centerWidget->setGeometry( centerContentRect().toAlignedRect() );
    m_centerWidget->setVisible( isTextVisible() );
    m_centerWidget->raise();
}
