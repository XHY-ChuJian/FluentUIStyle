#include "fonticon.h"
#include "qapplication.h"
#include "qpalette.h"

#include <QDebug>
#include <QFontDatabase>
#include <QMap>
#include <QPainter>

FontIcon::FontIcon()
{
    init();
}

void FontIcon::init()
{
    QFontDatabase fontDB;
    QStringList systemFamilies = fontDB.families();
    // Win11系统默认包含Segoe Fluent Icons
    if ( !systemFamilies.contains( SegoeFontName ) )
    {
        loadFont( QStringLiteral( ":/resource/Segoe Fluent Icons.ttf" ) );
    }
}

FontIcon::~FontIcon()
{
}

FontIcon* FontIcon::instance()
{
    static FontIcon _instance;
    return &_instance;
}

bool FontIcon::loadFont( const QString& path )
{
    int id = QFontDatabase::addApplicationFont( path );
    if ( -1 == id )
    {
        qDebug() << path << "load failed.";
        return false;
    }

    const QStringList names = QFontDatabase::applicationFontFamilies( id );
    if ( !names.isEmpty() )
    {
        return true;
    }

    return false;
}

QIcon FontIcon::getIcon( int unicode, const QString& family )
{
    QFont iconFont( family );
    QPixmap pix( 30, 30 );
    pix.fill( Qt::transparent );
    QPainter painter;
    painter.begin( &pix );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
    iconFont.setPixelSize( 25 );
    painter.setFont( iconFont );
    painter.setPen(qApp->palette().text().color());
    painter.drawText( pix.rect(), Qt::AlignCenter, QChar( (unsigned short)unicode ) );
    painter.end();
    return QIcon( pix );
}

QIcon FontIcon::getIcon( int unicode, QColor iconColor, const QString& family )
{
    QFont iconFont( family );
    QPixmap pix( 30, 30 );
    pix.fill( Qt::transparent );
    QPainter painter;
    painter.begin( &pix );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
    iconFont.setPixelSize( 25 );
    painter.setPen( iconColor );
    painter.setFont( iconFont );
    painter.drawText( pix.rect(), Qt::AlignCenter, QChar( (unsigned short)unicode ) );
    painter.end();
    return QIcon( pix );
}

QIcon FontIcon::getIcon( int unicode, int pixelSize, const QString& family )
{
    QFont iconFont( family );
    QPixmap pix( 30, 30 );
    pix.fill( Qt::transparent );
    QPainter painter;
    painter.begin( &pix );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
    iconFont.setPixelSize( pixelSize );
    painter.setFont( iconFont );
    painter.setPen(qApp->palette().text().color());
    painter.drawText( pix.rect(), Qt::AlignCenter, QChar( (unsigned short)unicode ) );
    painter.end();
    return QIcon( pix );
}

QIcon FontIcon::getIcon( int unicode, int pixelSize, QColor iconColor, const QString& family )
{
    QFont iconFont( family );
    QPixmap pix( 30, 30 );
    pix.fill( Qt::transparent );
    QPainter painter;
    painter.begin( &pix );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
    iconFont.setPixelSize( pixelSize );
    painter.setPen( iconColor );
    painter.setFont( iconFont );
    painter.drawText( pix.rect(), Qt::AlignCenter, QChar( (unsigned short)unicode ) );
    painter.end();
    return QIcon( pix );
}

QIcon FontIcon::getIcon( int unicode, int pixelSize, int fixedWidth, int fixedHeight, const QString& family )
{
    QFont iconFont( family );
    QPixmap pix( fixedWidth, fixedHeight );
    pix.fill( Qt::transparent );
    QPainter painter;
    painter.begin( &pix );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
    iconFont.setPixelSize( pixelSize );
    iconFont.setWeight( QFont::Normal );
    painter.setFont( iconFont );
    painter.setPen(qApp->palette().text().color());
    painter.drawText( pix.rect(), Qt::AlignCenter, QChar( (unsigned short)unicode ) );
    painter.end();
    return QIcon( pix );
}

QIcon FontIcon::getIcon( int unicode, int pixelSize, int fixedWidth, int fixedHeight, QColor iconColor, const QString& family )
{
    QFont iconFont( family );
    QPixmap pix( fixedWidth, fixedHeight );
    pix.fill( Qt::transparent );
    QPainter painter;
    painter.begin( &pix );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
    iconFont.setPixelSize( pixelSize );
    painter.setPen( iconColor );
    painter.setFont( iconFont );
    painter.drawText( pix.rect(), Qt::AlignCenter, QChar( (unsigned short)unicode ) );
    painter.end();
    return QIcon( pix );
}
