// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qpainterpath.h"
#include <QtCore/qpoint.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringbuilder.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpaintdevice.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpolygon.h>
#include <QtWidgets/qwidget.h>

#ifndef QSTYLEHELPER_P_H
#    define QSTYLEHELPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QColor;
class QObject;
class QPalette;
class QPixmap;
class QStyleOptionSlider;
class QStyleOption;
class QWindow;

namespace QStyleHelper {
 QString uniqueName( const QString& key, const QStyleOption* option, const QSize& size, qreal dpr );

 qreal dpi( const QStyleOption* option );

 qreal dpiScaled( qreal value, qreal dpi );
 qreal dpiScaled( qreal value, const QPaintDevice* device );
 qreal dpiScaled( qreal value, const QStyleOption* option );

#    if QT_CONFIG( dial )
qreal angle( const QPointF& p1, const QPointF& p2 );
QPolygonF calcLines( const QStyleOptionSlider* dial );
int calcBigLineSize( int radius );
 void drawDial( const QStyleOptionSlider* dial, QPainter* painter );
#    endif  // QT_CONFIG(dial)
 void
drawBorderPixmap( const QPixmap& pixmap, QPainter* painter, const QRect& rect, int left = 0, int top = 0, int right = 0, int bottom = 0 );
#    if QT_CONFIG( accessibility )
 bool isInstanceOf( QObject* obj, QAccessible::Role role );
 bool hasAncestor( QObject* obj, QAccessible::Role role );
#    endif
 QColor backgroundColor( const QPalette& pal, const QWidget* widget = nullptr );

enum WidgetSizePolicy
{
    SizeLarge   = 0,
    SizeSmall   = 1,
    SizeMini    = 2,
    SizeDefault = -1
};

 WidgetSizePolicy widgetSizePolicy( const QWidget* w, const QStyleOption* opt = nullptr );

// returns the device pixel ratio of the widget or the global one
// if widget is a nullptr
static inline qreal getDpr( const QWidget* widget )
{
    return widget ? widget->devicePixelRatio() : qApp->devicePixelRatio();
}

// returns the device pixel ratio of the painters underlying paint device
static inline qreal getDpr( const QPainter* painter )
{
    Q_ASSERT( painter && painter->device() );
    return painter->device()->devicePixelRatio();
}

static inline QPainterPath buildRoundedPolyline( const QList<QPointF>& points, qreal radius )
{
    QPainterPath path;

    if ( points.size() < 2 )
    {
        return path;
    }

    path.moveTo( points[ 0 ] );

    for ( int i = 1; i < points.size() - 1; ++i )
    {
        const QPointF& p0 = points[ i - 1 ];
        const QPointF& p1 = points[ i ];
        const QPointF& p2 = points[ i + 1 ];

        QVector2D v1( p1 - p0 );
        QVector2D v2( p2 - p1 );

        QVector2D v1n = v1.normalized();
        QVector2D v2n = v2.normalized();

        // 共线检测
        if ( QVector2D::dotProduct( v1n, v2n ) > 0.999 )
        {
            path.lineTo( p1 );
            continue;
        }

        qreal maxAllowedRadius = std::min( v1.length(), v2.length() ) / 2;
        qreal r                = std::min( radius, maxAllowedRadius );

        QPointF p1_before = p1 - r * v1n.toPointF();
        QPointF p1_after  = p1 + r * v2n.toPointF();

        path.lineTo( p1_before );
        path.quadTo( p1, p1_after );
    }

    path.lineTo( points.last() );

    return path;
}
}  // namespace QStyleHelper

// small helper to read out the pixmap to paint from QPixmapCache or re-draw
// it and put it into the QPixmapCache for later usage
class QCachedPainter
{
public:
    QCachedPainter( QPainter* painter, const QString& cachePrefix, const QStyleOption* option, QSize size = {}, QRect paintRect = {} );
    ~QCachedPainter();
    void finish();

    bool needsPainting() const { return !m_alreadyCached; }

    QRect pixmapRect() const
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        const auto sz = m_pixmap.deviceIndependentSize();
#else
        const auto sz = m_pixmap.size() / m_pixmap.devicePixelRatio();
#endif

        return QRect( 0, 0, sz.width(), sz.height() );
    }

    QPainter* operator->() { return painter(); }

    QPainter* painter()
    {
        Q_ASSERT_X( m_pixmapPainter, "painter()", "Must only be called when painting on a pixmap to cache" );
        return m_pixmapPainter.get();
    }

    // clean pixmap cache from all cached pixmaps (e.g. due to palette change)
    // to make sure the widgets are painted correctly afterwards
    static void cleanupPixmapCache();

private:
    QPainter* m_painter          = nullptr;
    const QStyleOption* m_option = nullptr;
    std::unique_ptr<QPainter> m_pixmapPainter;
    QString m_pixmapName;
    QPixmap m_pixmap;
    QRect m_paintRect;
    bool m_alreadyCached;
    bool m_pixmapDrawn = false;
    static QSet<QString> s_pixmapCacheKeys;
};

QT_END_NAMESPACE

#endif  // QSTYLEHELPER_P_H
