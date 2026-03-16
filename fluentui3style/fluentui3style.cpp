#include "fluentui3style.h"

#include <QApplication>
#include <QBitmap>
#include <QDockWidget>
#include <QFontMetrics>
#include <QIcon>
#include <QObject>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QString>
#include <QStyleHints>
#include <QSysInfo>
#include <QTextLayout>
#include <QToolButton>
#include <QTreeView>

#include <array>

#include "fluentuiappearance.h"
#include "qapplication.h"
#include "qhexstring_p.h"
#include "qstyleanimation_p.h"
#include "qstylehelper_p.h"
#include "qstyleoption.h"

QPainterPath buildRoundedPolyline(const QList<QPointF> &points, qreal radius)
{
    QPainterPath path;

    if (points.size() < 2)
        return path;

    path.moveTo(points[0]);

    for (int i = 1; i < points.size() - 1; ++i)
    {
        const QPointF &p0 = points[i - 1];
        const QPointF &p1 = points[i];
        const QPointF &p2 = points[i + 1];

        QVector2D v1(p1 - p0);
        QVector2D v2(p2 - p1);

        QVector2D v1n = v1.normalized();
        QVector2D v2n = v2.normalized();

        // 共线检测
        if (QVector2D::dotProduct(v1n, v2n) > 0.999)
        {
            path.lineTo(p1);
            continue;
        }

        qreal maxAllowedRadius = std::min(v1.length(), v2.length()) / 2;
        qreal r = std::min(radius, maxAllowedRadius);

        QPointF p1_before = p1 - r * v1n.toPointF();
        QPointF p1_after  = p1 + r * v2n.toPointF();

        path.lineTo(p1_before);
        path.quadTo(p1, p1_after);
    }

    path.lineTo(points.last());

    return path;
}

#if QT_VERSION <= QT_VERSION_CHECK( 6, 8, 0 )
#    include "palettemanager.h"
#endif

static constexpr int topLevelRoundingRadius    = 8;      // Radius for toplevel items like popups for round corners
static constexpr int secondLevelRoundingRadius = 4;      // Radius for second level items like hovered menu item round corners
static constexpr int contentItemHMargin        = 4;      // margin between content items (e.g. text and icon)
static constexpr int contentHMargin            = 2 * 3;  // margin between rounded border and content (= rounded border
                                                         // margin * 3)

enum
{
    windowsItemFrame      = 2,   // menu item frame width
    windowsSepHeight      = 9,   // separator item height
    windowsItemHMargin    = 3,   // menu item hor text margin
    windowsItemVMargin    = 2,   // menu item ver text margin
    windowsArrowHMargin   = 6,   // arrow horizontal margin
    windowsRightBorder    = 15,  // right border on windows
    windowsCheckMarkWidth = 12   // checkmarks width on windows
};

#define AcceptMedium QStringLiteral( "\uF78C" )
// QStringLiteral(u"\uE73C")
#define Dash12           QStringLiteral( "\uE629" )
#define CheckMark        QStringLiteral( "\uE73E" )

#define CaretLeftSolid8  QStringLiteral( "\uEDD9" )
#define CaretRightSolid8 QStringLiteral( "\uEDDA" )
#define CaretUpSolid8    QStringLiteral( "\uEDDB" )
#define CaretDownSolid8  QStringLiteral( "\uEDDC" )

#define ChevronDown      QStringLiteral( "\uE70D" )
#define ChevronUp        QStringLiteral( "\uE70E" )

#define Add              QStringLiteral( "\uE710" )
#define Remove           QStringLiteral( "\uE738" )

#define ChevronDownMed   QStringLiteral( "\uE972" )
#define ChevronLeftMed   QStringLiteral( "\uE973" )
#define ChevronRightMed  QStringLiteral( "\uE974" )

#define ChevronUpSmall   QStringLiteral( "\uE96D" )
#define ChevronDownSmall QStringLiteral( "\uE96E" )

#define ChromeMinimize   QStringLiteral( "\uE921" )
#define ChromeMaximize   QStringLiteral( "\uE922" )
#define ChromeRestore    QStringLiteral( "\uE923" )
#define ChromeClose      QStringLiteral( "\uE8BB" )

#define Help             QStringLiteral( "\uE897" )

template <typename R, typename P, typename B>
static inline void drawRoundedRect( QPainter* p, R&& rect, P&& pen, B&& brush )
{
    p->setPen( pen );
    p->setBrush( brush );
    p->drawRoundedRect( rect, secondLevelRoundingRadius, secondLevelRoundingRadius );
}

static constexpr int percentToAlpha( double percent )
{
    return qRound( percent * 255. / 100. );
}

static constexpr std::array<QColor, 34> WINUI3ColorsLight {
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 3.73 ) ),   // subtleHighlightColor (fillSubtleSecondary)
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 2.41 ) ),   // subtlePressedColor (fillSubtleTertiary)
    QColor( 0x00, 0x00, 0x00, 0x0F ),                     // frameColorLight
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 60.63 ) ),  // frameColorStrong
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 21.69 ) ),  // frameColorStrongDisabled
    QColor( 0x00, 0x00, 0x00, 0x72 ),                     // controlStrongFill
    QColor( 0x00, 0x00, 0x00, 0x29 ),                     // controlStrokeSecondary
    QColor( 0x00, 0x00, 0x00, 0x14 ),                     // controlStrokePrimary
    QColor( 0xFF, 0xFF, 0xFF, 0xFF ),                     // menuPanelFill
    QColor( 0x00, 0x00, 0x00, 0x66 ),                     // controlStrokeOnAccentSecondary
    QColor( 0xFF, 0xFF, 0xFF, 0xFF ),                     // controlFillSolid
    QColor( 0x75, 0x75, 0x75, 0x66 ),                     // surfaceStroke
    QColor( 0xFF, 0xFF, 0xFF, 0xFF ),                     // focusFrameInnerStroke
    QColor( 0x00, 0x00, 0x00, 0xFF ),                     // focusFrameOuterStroke
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 70 ) ),     // fillControlDefault
    QColor( 0xF9, 0xF9, 0xF9, percentToAlpha( 50 ) ),     // fillControlSecondary
    QColor( 0xF9, 0xF9, 0xF9, percentToAlpha( 30 ) ),     // fillControlTertiary
    QColor( 0xF9, 0xF9, 0xF9, percentToAlpha( 30 ) ),     // fillControlDisabled
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 100 ) ),    // fillControlInputActive
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 2.41 ) ),   // fillControlAltSecondary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 5.78 ) ),   // fillControlAltTertiary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 9.24 ) ),   // fillControlAltQuarternary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 0.00 ) ),   // fillControlAltDisabled
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 100 ) ),    // fillAccentDefault
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 90 ) ),     // fillAccentSecondary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 80 ) ),     // fillAccentTertiary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 21.69 ) ),  // fillAccentDisabled
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 89.56 ) ),  // textPrimary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 60.63 ) ),  // textSecondary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 36.14 ) ),  // textDisabled
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 100 ) ),    // textOnAccentPrimary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 70 ) ),     // textOnAccentSecondary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 100 ) ),    // textOnAccentDisabled
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 8.03 ) ),   // dividerStrokeDefault
};

static std::array<QColor, 34> WINUI3ColorsDark {
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 6.05 ) ),   // subtleHighlightColor (fillSubtleSecondary)
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 4.19 ) ),   // subtlePressedColor (fillSubtleTertiary)
    QColor( 0xFF, 0xFF, 0xFF, 0x12 ),                     // frameColorLight
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 60.47 ) ),  // frameColorStrong
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 15.81 ) ),  // frameColorStrongDisabled
    QColor( 0xFF, 0xFF, 0xFF, 0x8B ),                     // controlStrongFill
    QColor( 0xFF, 0xFF, 0xFF, 0x18 ),                     // controlStrokeSecondary
    QColor( 0xFF, 0xFF, 0xFF, 0x12 ),                     // controlStrokePrimary
    // QColor( 0x0F, 0x0F, 0x0F, 0xFF ),                     // menuPanelFill
    QColor( 45, 45, 45 ),                                 // menuPanelFill
    QColor( 0xFF, 0xFF, 0xFF, 0x14 ),                     // controlStrokeOnAccentSecondary
    QColor( 0x45, 0x45, 0x45, 0xFF ),                     // controlFillSolid
    QColor( 0x75, 0x75, 0x75, 0x66 ),                     // surfaceStroke
    QColor( 0x00, 0x00, 0x00, 0xFF ),                     // focusFrameInnerStroke
    QColor( 0xFF, 0xFF, 0xFF, 0xFF ),                     // focusFrameOuterStroke
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 6.05 ) ),   // fillControlDefault
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 8.37 ) ),   // fillControlSecondary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 3.26 ) ),   // fillControlTertiary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 4.19 ) ),   // fillControlDisabled
    QColor( 0x1E, 0x1E, 0x1E, percentToAlpha( 70 ) ),     // fillControlInputActive
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 10.0 ) ),   // fillControlAltDefault
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 4.19 ) ),   // fillControlAltSecondary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 6.98 ) ),   // fillControlAltTertiafillCy
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 0.00 ) ),   // controlAltDisabled
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 100 ) ),    // fillAccentDefault
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 90 ) ),     // fillAccentSecondary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 80 ) ),     // fillAccentTertiary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 15.81 ) ),  // fillAccentDisabled
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 100 ) ),    // textPrimary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 78.6 ) ),   // textSecondary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 36.28 ) ),  // textDisabled
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 100 ) ),    // textOnAccentPrimary
    QColor( 0x00, 0x00, 0x00, percentToAlpha( 70 ) ),     // textOnAccentSecondary
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 53.02 ) ),  // textOnAccentDisabled
    QColor( 0xFF, 0xFF, 0xFF, percentToAlpha( 8.37 ) ),   // dividerStrokeDefault
};

// Color of close Button in Titlebar (default + hover)
static constexpr QColor shellCaptionCloseFillColorPrimary( 0xC4, 0x2B, 0x1C, 0xFF );
static constexpr QColor shellCaptionCloseTextFillColorPrimary( 0xFF, 0xFF, 0xFF, 0xFF );
// Color of close Button in Titlebar (pressed + disabled)
static constexpr QColor shellCaptionCloseFillColorSecondary( 0xC4, 0x2B, 0x1C, 0xE6 );
static constexpr QColor shellCaptionCloseTextFillColorSecondary( 0xFF, 0xFF, 0xFF, 0xB3 );

static std::array<std::array<QColor, 34>, 2> WINUI3Colors { WINUI3ColorsLight, WINUI3ColorsDark };

static qreal radioButtonInnerRadius( int state, const QStyleOption* option, const QWidget* widget, int indicatorSize )
{
    Q_UNUSED( widget )
    Q_UNUSED( option )

    qreal outerRadius = indicatorSize / 2.0;

    // Fluent UI 标准：选中时内圆半径 = 外圈半径的一半
    qreal innerRadius = outerRadius / 2.0;

    if ( state & QStyle::State_Sunken )
    {
        // 按下时稍微缩小
        innerRadius = outerRadius / 2.5;
    }
    else if ( state & QStyle::State_MouseOver && !( state & QStyle::State_On ) )
    {
        // 悬停但未选中，保持外圈描边，不显示内圆
        innerRadius = 0.0;
    }
    else if ( state & QStyle::State_MouseOver && ( state & QStyle::State_On ) )
    {
        // 悬停且选中，内圆稍微放大
        innerRadius = outerRadius / 1.8;
    }
    else if ( state & QStyle::State_On )
    {
        // 选中状态，标准半径
        innerRadius = outerRadius / 2.0;
    }
    else
    {
        // 未选中状态，不显示内圆
        innerRadius = 0.0;
    }

    return innerRadius;
}

static qreal sliderInnerRadius( QStyle::State state, bool insideHandle )
{
    if ( state & QStyle::State_Sunken )
    {
        return 0.45;
    }
    else if ( insideHandle )
    {
        return 0.75;
    }
    return 0.65;
}

namespace StyleOptionHelper {
inline bool isChecked( const QStyleOption* option )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 2, 0 )
    return option->state.testAnyFlags( QStyle::State_On | QStyle::State_NoChange );
#endif
    return option->state & ( QStyle::State_On | QStyle::State_NoChange );
}

inline bool isDisabled( const QStyleOption* option )
{
    return !option->state.testFlag( QStyle::State_Enabled );
}

inline bool isPressed( const QStyleOption* option )
{
    return option->state.testFlag( QStyle::State_Sunken );
}

inline bool isHover( const QStyleOption* option )
{
    return option->state.testFlag( QStyle::State_MouseOver );
}

inline bool isAutoRaise( const QStyleOption* option )
{
    return option->state.testFlag( QStyle::State_AutoRaise );
}
enum class ControlState
{
    Normal,
    Hover,
    Pressed,
    Disabled
};

inline ControlState calcControlState( const QStyleOption* option )
{
    if ( isDisabled( option ) )
    {
        return ControlState::Disabled;
    }
    if ( isPressed( option ) )
    {
        return ControlState::Pressed;
    }
    if ( isHover( option ) )
    {
        return ControlState::Hover;
    }
    return ControlState::Normal;
};

}  // namespace StyleOptionHelper

static QSizeF viewItemTextLayout( QTextLayout& textLayout, int lineWidth, int maxHeight = -1, int* lastVisibleLine = nullptr )
{
    if ( lastVisibleLine )
    {
        *lastVisibleLine = -1;
    }
    qreal height    = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    int i = 0;
    while ( true )
    {
        QTextLine line = textLayout.createLine();
        if ( !line.isValid() )
        {
            break;
        }
        line.setLineWidth( lineWidth );
        line.setPosition( QPointF( 0, height ) );
        height += line.height();
        widthUsed = qMax( widthUsed, line.naturalTextWidth() );
        // we assume that the height of the next line is the same as the current
        // one
        if ( maxHeight > 0 && lastVisibleLine && height + line.height() > maxHeight )
        {
            const QTextLine nextLine = textLayout.createLine();
            *lastVisibleLine         = nextLine.isValid() ? i : -1;
            break;
        }
        ++i;
    }
    textLayout.endLayout();
    return QSizeF( widthUsed, height );
}

#if QT_CONFIG( toolbutton )

QString calculateElidedText( const QString& text,
                             const QTextOption& textOption,
                             const QFont& font,
                             const QRect& textRect,
                             const Qt::Alignment valign,
                             Qt::TextElideMode textElideMode,
                             int flags,
                             bool lastVisibleLineShouldBeElided,
                             QPointF* paintStartPosition )
{
    Q_UNUSED( flags )

    QTextLayout textLayout( text, font );
    textLayout.setTextOption( textOption );

    // In AlignVCenter mode when more than one line is displayed and the height
    // only allows some of the lines it makes no sense to display those. From a
    // users perspective it makes more sense to see the start of the text
    // instead something inbetween.
    const bool vAlignmentOptimization = paintStartPosition && valign.testFlag( Qt::AlignVCenter );

    int lastVisibleLine = -1;
    viewItemTextLayout( textLayout, textRect.width(), vAlignmentOptimization ? textRect.height() : -1, &lastVisibleLine );

    const QRectF boundingRect = textLayout.boundingRect();
    // don't care about LTR/RTL here, only need the height
    const QRect layoutRect = QStyle::alignedRect( Qt::LayoutDirectionAuto, valign, boundingRect.size().toSize(), textRect );

    if ( paintStartPosition )
    {
        *paintStartPosition = QPointF( textRect.x(), layoutRect.top() );
    }

    QString ret;
    qreal height        = 0;
    const int lineCount = textLayout.lineCount();
    for ( int i = 0; i < lineCount; ++i )
    {
        const QTextLine line = textLayout.lineAt( i );
        height += line.height();

        // above visible rect
        if ( height + layoutRect.top() <= textRect.top() )
        {
            if ( paintStartPosition )
            {
                paintStartPosition->ry() += line.height();
            }
            continue;
        }

        const int start           = line.textStart();
        const int length          = line.textLength();
        const bool drawElided     = line.naturalTextWidth() > textRect.width();
        bool elideLastVisibleLine = lastVisibleLine == i;
        if ( !drawElided && i + 1 < lineCount && lastVisibleLineShouldBeElided )
        {
            const QTextLine nextLine = textLayout.lineAt( i + 1 );
            const int nextHeight     = height + nextLine.height() / 2;
            // elide when less than the next half line is visible
            if ( nextHeight + layoutRect.top() > textRect.height() + textRect.top() )
            {
                elideLastVisibleLine = true;
            }
        }

        QString text = textLayout.text().mid( start, length );
        if ( drawElided || elideLastVisibleLine )
        {
            if ( elideLastVisibleLine )
            {
                if ( text.endsWith( QChar::LineSeparator ) )
                {
                    text.chop( 1 );
                }
                text += QChar( 0x2026 );
            }
            // Q_DECL_UNINITIALIZED const QStackTextEngine engine(text, font);
            // ret += engine.elidedText(textElideMode, textRect.width(), flags);
            QFontMetrics fm( font );
            ret += fm.elidedText( text, textElideMode, textRect.width() );

            // no newline for the last line (last visible or real)
            // sometimes drawElided is true but no eliding is done so the text
            // ends with QChar::LineSeparator - don't add another one. This
            // happened with arabic text in the testcase for QTBUG-72805
            if ( i < lineCount - 1 && !ret.endsWith( QChar::LineSeparator ) )
            {
                ret += QChar::LineSeparator;
            }
        }
        else
        {
            ret += text;
        }

        // below visible text, can stop
        if ( ( height + layoutRect.top() >= textRect.bottom() ) || ( lastVisibleLine >= 0 && lastVisibleLine == i ) )
        {
            break;
        }
    }
    return ret;
}

void viewItemDrawText( QPainter* p, const QStyleOptionViewItem* option, const QRect& rect )
{
    const QWidget* widget = option->widget;
    const int textMargin  = qApp->style()->pixelMetric( QStyle::PM_FocusFrameHMargin, nullptr, widget ) + 1;

    QRect textRect      = rect.adjusted( textMargin, 0, -textMargin, 0 );  // remove width padding
    const bool wrapText = option->features & QStyleOptionViewItem::WrapText;
    QTextOption textOption;
    textOption.setWrapMode( wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap );
    textOption.setTextDirection( option->direction );
    textOption.setAlignment( QStyle::visualAlignment( option->direction, option->displayAlignment ) );

    QPointF paintPosition;
    const QString newText = calculateElidedText(
        option->text, textOption, option->font, textRect, option->displayAlignment, option->textElideMode, 0, true, &paintPosition );

    QTextLayout textLayout( newText, option->font );
    textLayout.setTextOption( textOption );
    viewItemTextLayout( textLayout, textRect.width() );
    textLayout.draw( p, paintPosition );
}

QString toolButtonElideText( const QStyleOptionToolButton* option, const QRect& textRect, int flags )
{
    if ( option->fontMetrics.horizontalAdvance( option->text ) <= textRect.width() )
    {
        return option->text;
    }

    QString text = option->text;
    text.replace( u'\n', QChar::LineSeparator );
    QTextOption textOption;
    textOption.setWrapMode( QTextOption::ManualWrap );
    textOption.setTextDirection( option->direction );

    return calculateElidedText( text, textOption, option->font, textRect, Qt::AlignTop, Qt::ElideMiddle, flags, false, nullptr );
}

static void drawArrow( const QStyle* style,
                       const QStyleOptionToolButton* toolbutton,
                       const QRect& rect,
                       QPainter* painter,
                       const QWidget* widget = nullptr )
{
    QStyle::PrimitiveElement pe;
    switch ( toolbutton->arrowType )
    {
        case Qt::LeftArrow :
            pe = QStyle::PE_IndicatorArrowLeft;
            break;
        case Qt::RightArrow :
            pe = QStyle::PE_IndicatorArrowRight;
            break;
        case Qt::UpArrow :
            pe = QStyle::PE_IndicatorArrowUp;
            break;
        case Qt::DownArrow :
            pe = QStyle::PE_IndicatorArrowDown;
            break;
        default :
            return;
    }
    QStyleOption arrowOpt = *toolbutton;
    arrowOpt.rect         = rect;
    style->drawPrimitive( pe, &arrowOpt, painter, widget );
}

void drawRestore( QPainter* p, const QRect& rect )
{
    p->save();

    QPen pen( Qt::black, 1.5 );
    p->setPen( pen );
    p->setBrush( Qt::NoBrush );

    QRect r1 = rect.adjusted( 6, 4, -2, -6 );  // 前面的窗口
    QRect r2 = rect.adjusted( 4, 6, -6, -4 );  // 后面的窗口

    p->drawRect( r2 );
    p->drawRect( r1 );

    p->restore();
}

#endif  // QT_CONFIG(toolbutton)

#include <QComboBox>
#include <QCommandLinkButton>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsView>
#include <QHash>
#include <QLineEdit>
#include <QListView>
#include <QMdiArea>
#include <QMenu>
#include <QScreen>
#include <QTableView>
#include <QTextEdit>
#include <QTextLayout>
QHash<QObject*, QStyleAnimation*> animations;

template <typename K, typename V>
V takeValue( QHash<K, V>& h, K key )
{
    V val = h.value( key );
    h.remove( key );
    return val;
}

void removeAnimation( QObject* target )
{
    animations.remove( target );
}

void stopAnimation( QObject* target )
{
    QStyleAnimation* animation = takeValue( animations, target );
    if ( animation )
    {
        animation->stop();
        delete animation;
    }
}

void startAnimation( QStyleAnimation* animation )
{
    const auto target = animation->target();
    stopAnimation( target );
    QObject::connect( animation, &QStyleAnimation::destroyed, [ target ]() { removeAnimation( target ); } );
    animations.insert( target, animation );
    animation->start();
}

QStyleAnimation* getAnimation( QObject* target )
{
    return animations.value( target );
}

bool transitionsEnabled()
{
    return true;
}

class PainterStateGuard
{
public:
    PainterStateGuard( QPainter* painter )
    {
        m_painter = painter;
        m_painter->save();
    }

    ~PainterStateGuard() { m_painter->restore(); }

private:
    QPainter* m_painter;
};

inline void
qDrawPlainRoundedRect( QPainter* p, int x, int y, int w, int h, qreal rx, qreal ry, const QColor& c, int lineWidth, const QBrush* fill )
{
    if ( w == 0 || h == 0 )
    {
        return;
    }
    if ( Q_UNLIKELY( w < 0 || h < 0 || lineWidth < 0 ) )
    {
        qWarning( "qDrawPlainRect: Invalid parameters" );
        return;
    }

    PainterStateGuard painterGuard( p );
    if ( lineWidth == 0 && !fill )
    {
        return;
    }
    if ( lineWidth > 0 )
    {
        p->setPen( QPen( c, lineWidth ) );
    }
    p->setBrush( fill ? *fill : Qt::NoBrush );
    const QRectF r( x, y, w, h );
    const auto lw2    = lineWidth / 2.;
    const QRectF rect = r.marginsRemoved( QMarginsF( lw2, lw2, lw2, lw2 ) );
    p->drawRoundedRect( rect, rx, ry );
}

inline void qDrawPlainRoundedRect( QPainter* painter,
                                   const QRect& rect,
                                   qreal rx,
                                   qreal ry,
                                   const QColor& lineColor,
                                   int lineWidth      = 1,
                                   const QBrush* fill = nullptr )
{
    qDrawPlainRoundedRect( painter, rect.x(), rect.y(), rect.width(), rect.height(), rx, ry, lineColor, lineWidth, fill );
}

void qDrawShadeRect( QPainter* p,
                     int x,
                     int y,
                     int w,
                     int h,
                     const QPalette& pal,
                     bool sunken        = false,
                     int lineWidth      = 1,
                     int midLineWidth   = 0,
                     const QBrush* fill = nullptr )
{
    if ( w == 0 || h == 0 )
    {
        return;
    }
    if ( Q_UNLIKELY( w < 0 || h < 0 || lineWidth < 0 || midLineWidth < 0 ) )
    {
        qWarning( "qDrawShadeRect: Invalid parameters" );
        return;
    }

    PainterStateGuard painterGuard( p );
    const qreal devicePixelRatio = QStyleHelper::getDpr( p );
    if ( !qFuzzyCompare( devicePixelRatio, qreal( 1 ) ) )
    {
        const qreal inverseScale = qreal( 1 ) / devicePixelRatio;
        p->scale( inverseScale, inverseScale );
        x            = qRound( devicePixelRatio * x );
        y            = qRound( devicePixelRatio * y );
        w            = devicePixelRatio * w;
        h            = devicePixelRatio * h;
        lineWidth    = qRound( devicePixelRatio * lineWidth );
        midLineWidth = qRound( devicePixelRatio * midLineWidth );
        p->translate( 0.5, 0.5 );
    }
    if ( sunken )
    {
        p->setPen( pal.dark().color() );
    }
    else
    {
        p->setPen( pal.light().color() );
    }
    int x1 = x, y1 = y, x2 = x + w - 1, y2 = y + h - 1;

    if ( lineWidth == 1 && midLineWidth == 0 )
    {  // standard shade rectangle
        p->drawRect( x1, y1, w - 2, h - 2 );
        if ( sunken )
        {
            p->setPen( pal.light().color() );
        }
        else
        {
            p->setPen( pal.dark().color() );
        }
        QLineF lines[ 4 ] = { QLineF( x1 + 1, y1 + 1, x2 - 2, y1 + 1 ),
                              QLineF( x1 + 1, y1 + 2, x1 + 1, y2 - 2 ),
                              QLineF( x1, y2, x2, y2 ),
                              QLineF( x2, y1, x2, y2 - 1 ) };
        p->drawLines( lines, 4 );  // draw bottom/right lines
    }
    else
    {  // more complicated
        int m = lineWidth + midLineWidth;
        int i, j = 0, k = m;
        for ( i = 0; i < lineWidth; i++ )
        {  // draw top shadow
            QLineF lines[ 4 ] = { QLineF( x1 + i, y2 - i, x1 + i, y1 + i ),
                                  QLineF( x1 + i, y1 + i, x2 - i, y1 + i ),
                                  QLineF( x1 + k, y2 - k, x2 - k, y2 - k ),
                                  QLineF( x2 - k, y2 - k, x2 - k, y1 + k ) };
            p->drawLines( lines, 4 );
            k++;
        }
        p->setPen( pal.mid().color() );
        j = lineWidth * 2;
        for ( i = 0; i < midLineWidth; i++ )
        {  // draw lines in the middle
            p->drawRect( x1 + lineWidth + i, y1 + lineWidth + i, w - j - 1, h - j - 1 );
            j += 2;
        }
        if ( sunken )
        {
            p->setPen( pal.light().color() );
        }
        else
        {
            p->setPen( pal.dark().color() );
        }
        k = m;
        for ( i = 0; i < lineWidth; i++ )
        {  // draw bottom shadow
            QLineF lines[ 4 ] = { QLineF( x1 + 1 + i, y2 - i, x2 - i, y2 - i ),
                                  QLineF( x2 - i, y2 - i, x2 - i, y1 + i + 1 ),
                                  QLineF( x1 + k, y2 - k, x1 + k, y1 + k ),
                                  QLineF( x1 + k, y1 + k, x2 - k, y1 + k ) };
            p->drawLines( lines, 4 );
            k++;
        }
    }
    if ( fill )
    {
        int tlw = lineWidth + midLineWidth;
        p->setPen( Qt::NoPen );
        p->setBrush( *fill );
        p->drawRect( x + tlw, y + tlw, w - 2 * tlw, h - 2 * tlw );
    }
}

void qDrawShadeRect( QPainter* p,
                     const QRect& r,
                     const QPalette& pal,
                     bool sunken        = false,
                     int lineWidth      = 1,
                     int midLineWidth   = 0,
                     const QBrush* fill = nullptr )
{
    qDrawShadeRect( p, r.x(), r.y(), r.width(), r.height(), pal, sunken, lineWidth, midLineWidth, fill );
}

void qDrawShadePanel( QPainter* p,
                      int x,
                      int y,
                      int w,
                      int h,
                      const QPalette& pal,
                      bool sunken        = false,
                      int lineWidth      = 1,
                      const QBrush* fill = nullptr )
{
    if ( w == 0 || h == 0 )
    {
        return;
    }
    if ( Q_UNLIKELY( w < 0 || h < 0 || lineWidth < 0 ) )
    {
        qWarning( "qDrawShadePanel: Invalid parameters" );
    }

    PainterStateGuard painterGuard( p );
    const qreal devicePixelRatio = QStyleHelper::getDpr( p );
    bool isTranslated            = false;
    if ( !qFuzzyCompare( devicePixelRatio, qreal( 1 ) ) )
    {
        const qreal inverseScale = qreal( 1 ) / devicePixelRatio;
        p->scale( inverseScale, inverseScale );
        x         = qRound( devicePixelRatio * x );
        y         = qRound( devicePixelRatio * y );
        w         = devicePixelRatio * w;
        h         = devicePixelRatio * h;
        lineWidth = qRound( devicePixelRatio * lineWidth );
        p->translate( 0.5, 0.5 );
        isTranslated = true;
    }

    QColor shade = pal.dark().color();
    QColor light = pal.light().color();
    if ( fill )
    {
        if ( fill->color() == shade )
        {
            shade = pal.shadow().color();
        }
        if ( fill->color() == light )
        {
            light = pal.midlight().color();
        }
    }
    QVector<QLineF> lines;
    lines.reserve( 2 * lineWidth );

    if ( sunken )
    {
        p->setPen( shade );
    }
    else
    {
        p->setPen( light );
    }
    int x1, y1, x2, y2;
    int i;
    x1 = x;
    y1 = y2 = y;
    x2      = x + w - 2;
    for ( i = 0; i < lineWidth; i++ )
    {  // top shadow
        lines << QLineF( x1, y1++, x2--, y2++ );
    }
    x2 = x1;
    y1 = y + h - 2;
    for ( i = 0; i < lineWidth; i++ )
    {  // left shado
        lines << QLineF( x1++, y1, x2++, y2-- );
    }
    p->drawLines( lines );
    lines.clear();
    if ( sunken )
    {
        p->setPen( light );
    }
    else
    {
        p->setPen( shade );
    }
    x1 = x;
    y1 = y2 = y + h - 1;
    x2      = x + w - 1;
    for ( i = 0; i < lineWidth; i++ )
    {  // bottom shadow
        lines << QLineF( x1++, y1--, x2, y2-- );
    }
    x1 = x2;
    y1 = y;
    y2 = y + h - lineWidth - 1;
    for ( i = 0; i < lineWidth; i++ )
    {  // right shadow
        lines << QLineF( x1--, y1++, x2--, y2 );
    }
    p->drawLines( lines );
    if ( fill )
    {  // fill with fill color
        if ( isTranslated )
        {
            p->translate( -0.5, -0.5 );
        }
        p->fillRect( x + lineWidth, y + lineWidth, w - lineWidth * 2, h - lineWidth * 2, *fill );
    }
}

void qDrawShadePanel( QPainter* p,
                      const QRect& r,
                      const QPalette& pal,
                      bool sunken        = false,
                      int lineWidth      = 1,
                      const QBrush* fill = nullptr )
{
    qDrawShadePanel( p, r.x(), r.y(), r.width(), r.height(), pal, sunken, lineWidth, fill );
}

static QScreen* screenOf( const QWidget* w )
{
    if ( w )
    {
        if ( auto screen = w->screen() )
        {
            return screen;
        }
    }
    return QGuiApplication::primaryScreen();
}

// Calculate the overall scale factor to obtain Qt Device Independent
// Pixels from a native Windows size.
qreal nativeMetricScaleFactor( const QWidget* widget )
{
    if ( !widget )
    {
        return 1.0;
    }

    QScreen* screen = screenOf( widget );
    return 1.0 / screen->devicePixelRatio();
}

static bool updateBrushOrigin_public( QPainter* painter, const QWidget* widget, const QBrush& brush )
{
    if ( !widget )
    {
        return false;
    }

    if ( brush.style() == Qt::NoBrush || brush.style() == Qt::SolidPattern )
    {
        return false;
    }

    auto viewport = qobject_cast<const QWidget*>( widget );
    if ( !viewport )
    {
        return false;
    }

    auto scrollArea = qobject_cast<const QAbstractScrollArea*>( viewport->parentWidget() );

    if ( !scrollArea || scrollArea->viewport() != viewport )
    {
        return false;
    }

    const QPoint offset = scrollArea->horizontalScrollBar()
                              ? QPoint( scrollArea->horizontalScrollBar()->value(), scrollArea->verticalScrollBar()->value() )
                              : QPoint();

    painter->setBrushOrigin( -offset );
    return true;
}

inline bool isWin11()
{
#ifdef Q_OS_WIN
    QSettings settings( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", QSettings::NativeFormat );
    int buildNumber = settings.value( "CurrentBuildNumber", 0 ).toInt();
    return buildNumber >= 22000;  // Win11 从 22000 build 开始
#else
    return false;
#endif
}

inline int getColorSchemeIndex()  // 0 = Light, 1 = Dark
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 8, 0 )
    Qt::ColorScheme scheme = qApp->styleHints()->colorScheme();
    return scheme == Qt::ColorScheme::Light ? 0 : 1;
#else
#    if 1
    return (int)fluentUIAppearance.theme();
#    else  // 直接读取系统设置，兼容性更好，可随系统变化
#        ifdef Q_OS_WIN
    if ( !isWin11() )
    {
        return 0;  // 默认 Light
    }

    QSettings settings( "HKEY_CURRENT_"
                        "USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Th"
                        "emes\\Personalize",
                        QSettings::NativeFormat );
    int value = settings.value( "AppsUseLightTheme", 1 ).toInt();
    return value == 1 ? 0 : 1;  // 0=Light,1=Dark
#        else
    return 0;  // 其他系统默认 Light
#        endif
#    endif
#endif
}

inline bool isHighContrastTheme()
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 5, 0 )
    return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Unknown;
#else
    // Qt5/Qt6.4 不支持 Unknown，统一返回 false
    return false;
#endif
}

FluentUI3Style::FluentUI3Style( QStyle* style )
    : QProxyStyle( style )
{
    QFontDatabase::addApplicationFont( ":/resource/Segoe Fluent Icons.ttf" );
    assetFont = QFont( QStringLiteral( "Segoe Fluent Icons" ) );
    assetFont.setStyleStrategy( QFont::NoFontMerging );

#if QT_VERSION >= QT_VERSION_CHECK( 6, 5, 0 )
    qDebug() << "Current colorscheme:" << qApp->styleHints()->colorScheme();
#endif

    highContrastTheme = isHighContrastTheme();
    colorSchemeIndex  = getColorSchemeIndex();
}

FluentUI3Style::~FluentUI3Style()
{
    qDebug() << "FluentUI3Style destroyed";
}

void FluentUI3Style::drawComplexControl( ComplexControl control,
                                         const QStyleOptionComplex* option,
                                         QPainter* painter,
                                         const QWidget* widget ) const
{
    const auto drawTitleBarButton = [ & ]( ComplexControl control, SubControl sc, const QString& str )
    {
        using namespace StyleOptionHelper;
        const QRect buttonRect = proxy()->subControlRect( control, option, sc, widget );
        if ( buttonRect.isValid() )
        {
            const bool hover = option->activeSubControls == sc && isHover( option );
            if ( hover )
            {
                painter->fillRect( buttonRect, winUI3Color( subtleHighlightColor ) );
            }
            painter->setPen( option->palette.color( QPalette::WindowText ) );
            painter->drawText( buttonRect, Qt::AlignCenter, str );
        }
    };
    const auto drawTitleBarCloseButton = [ & ]( ComplexControl control, SubControl sc, const QString& str )
    {
        using namespace StyleOptionHelper;
        const QRect buttonRect = proxy()->subControlRect( control, option, sc, widget );
        if ( buttonRect.isValid() )
        {
            const auto state = ( option->activeSubControls == sc ) ? calcControlState( option ) : ControlState::Normal;
            QPen pen;
            switch ( state )
            {
                case ControlState::Hover :
                    painter->fillRect( buttonRect, shellCaptionCloseFillColorPrimary );
                    pen = shellCaptionCloseTextFillColorPrimary;
                    break;
                case ControlState::Pressed :
                    painter->fillRect( buttonRect, shellCaptionCloseFillColorSecondary );
                    pen = shellCaptionCloseTextFillColorSecondary;
                    break;
                case ControlState::Disabled :
                case ControlState::Normal :
                    pen = option->palette.color( QPalette::WindowText );
                    break;
            }
            painter->setPen( pen );
            painter->drawText( buttonRect, Qt::AlignCenter, str );
        }
    };

    State state     = option->state;
    SubControls sub = option->subControls;
    State flags     = option->state;
    if ( widget && widget->testAttribute( Qt::WA_UnderMouse ) && widget->isActiveWindow() )
    {
        flags |= State_MouseOver;
    }

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    if ( transitionsEnabled() && option->styleObject )
    {
        if ( control == CC_Slider )
        {
            if ( const auto* slider = qstyleoption_cast<const QStyleOptionSlider*>( option ) )
            {
                QObject* styleObject = option->styleObject;  // Can be widget or qquickitem

                QRectF thumbRect = proxy()->subControlRect( CC_Slider, option, SC_SliderHandle, widget );
                const qreal outerRadius =
                    qMin( 8.0, ( slider->orientation == Qt::Horizontal ? thumbRect.height() / 2.0 : thumbRect.width() / 2.0 ) - 1 );
                bool isInsideHandle = option->activeSubControls == SC_SliderHandle;

                bool oldIsInsideHandle        = styleObject->property( "_q_insidehandle" ).toBool();
                State oldState                = State( styleObject->property( "_q_stylestate" ).toInt() );
                SubControls oldActiveControls = SubControls( styleObject->property( "_q_stylecontrols" ).toInt() );

                QRectF oldRect = styleObject->property( "_q_stylerect" ).toRect();
                styleObject->setProperty( "_q_insidehandle", isInsideHandle );
                styleObject->setProperty( "_q_stylestate", int( state ) );
                styleObject->setProperty( "_q_stylecontrols", int( option->activeSubControls ) );
                styleObject->setProperty( "_q_stylerect", option->rect );
                if ( option->styleObject->property( "_q_end_radius" ).isNull() )
                {
                    option->styleObject->setProperty( "_q_end_radius", outerRadius * 0.43 );
                }

                bool doTransition = ( ( ( state & State_Sunken ) != ( oldState & State_Sunken ) || ( oldIsInsideHandle != isInsideHandle )
                                        || ( oldActiveControls != option->activeSubControls ) )
                                      && state & State_Enabled );

                if ( oldRect != option->rect )
                {
                    doTransition = false;
                    stopAnimation( styleObject );
                    styleObject->setProperty( "_q_inner_radius", outerRadius * 0.43 );
                }

                if ( doTransition )
                {
                    QNumberStyleAnimation* t = new QNumberStyleAnimation( styleObject );
                    t->setStartValue( styleObject->property( "_q_inner_radius" ).toFloat() );
                    t->setEndValue( outerRadius * sliderInnerRadius( state, isInsideHandle ) );
                    styleObject->setProperty( "_q_end_radius", t->endValue() );

                    // t->setStartTime( animationTime() );
                    t->setDuration( 150 );
                    startAnimation( t );
                }
            }
        }
    }

    switch ( control )
    {
#if QT_CONFIG( spinbox )
        case CC_SpinBox :
            if ( const QStyleOptionSpinBox* sb = qstyleoption_cast<const QStyleOptionSpinBox*>( option ) )
            {
                SpinBoxButtonLayout buttonLayout =
                    widget ? static_cast<SpinBoxButtonLayout>( widget->property( "spinBoxButtonLayout" ).toInt() )
                           : SpinBoxButtonLayout::ArrowsVertical;

                QCachedPainter cp(
                    painter, QLatin1String( "win11_spinbox" ) % HexString<uint8_t>( colorSchemeIndex ) % HexString<uint8_t>( buttonLayout ), sb, sb->rect.size() );
                if ( cp.needsPainting() )
                {
                    [[maybe_unused]] const auto frameRect = QRectF( option->rect ).marginsRemoved( QMarginsF( 1.5, 1.5, 1.5, 1.5 ) );
                    if ( sb->frame && ( sub & SC_SpinBoxFrame ) )
                    {
                        QStyleOptionFrame panel;
                        panel.QStyleOption::operator=( *option );
                        panel.rect         = option->rect;
                        panel.lineWidth    = 1;
                        panel.midLineWidth = 0;

                        proxy()->drawPrimitive( PE_FrameLineEdit, &panel, cp.painter(), widget );
                    }

                    const bool isMouseOver = state & State_MouseOver;
                    const bool hasFocus    = state & State_HasFocus;
                    if ( isMouseOver && !hasFocus && !highContrastTheme )
                    {
                        drawRoundedRect( cp.painter(), frameRect, Qt::NoPen, winUI3Color( subtleHighlightColor ) );
                    }

                    const auto drawUpDown = [ & ]( QStyle::SubControl sc )
                    {
                        const bool isUp  = sc == SC_SpinBoxUp;
                        const QRect rect = proxy()->subControlRect( CC_SpinBox, option, sc, widget );
                        if ( sb->activeSubControls & sc )
                        {
                            drawRoundedRect( cp.painter(), rect.adjusted( 1, 1, -1, -2 ), Qt::NoPen, winUI3Color( subtleHighlightColor ) );
                        }

                        bool horizonal = buttonLayout != SpinBoxButtonLayout::ArrowsVertical;
                        QString c = isUp ? ChevronUp : ChevronDown;
                        if ( buttonLayout == SpinBoxButtonLayout::PlusMinusHorizontalSides )
                        {
                            c = isUp ? Add : Remove;
                        }

                        auto _font = assetFont;
                        _font.setPixelSize( horizonal ? 17 : 15 );
                        cp->setFont( _font );
                        cp->setPen( sb->palette.buttonText().color() );
                        cp->setBrush( Qt::NoBrush );
                        cp->drawText( rect, Qt::AlignCenter, c );
                    };
                    if ( sub & SC_SpinBoxUp )
                    {
                        drawUpDown( SC_SpinBoxUp );
                    }
                    if ( sub & SC_SpinBoxDown )
                    {
                        drawUpDown( SC_SpinBoxDown );
                    }
                    if ( state & State_KeyboardFocusChange && state & State_HasFocus )
                    {
                        QStyleOptionFocusRect fropt;
                        fropt.QStyleOption::operator=( *option );
                        proxy()->drawPrimitive( PE_FrameFocusRect, &fropt, cp.painter(), widget );
                    }
                }
            }
            break;
#endif  // QT_CONFIG(spinbox)
#if QT_CONFIG( slider )
        case CC_Slider :
            if ( const auto* slider = qstyleoption_cast<const QStyleOptionSlider*>( option ) )
            {
                const auto& slrect      = slider->rect;
                const bool isHorizontal = slider->orientation == Qt::Horizontal;
                const QRectF handleRect( proxy()->subControlRect( CC_Slider, option, SC_SliderHandle, widget ) );
                const QPointF handleCenter( handleRect.center() );

                if ( sub & SC_SliderGroove )
                {
                    QRectF rect = proxy()->subControlRect( CC_Slider, option, SC_SliderGroove, widget );
                    QRectF leftRect;
                    QRectF rightRect;

                    if ( isHorizontal )
                    {
                        rect      = QRectF( rect.left() + 2, rect.center().y() - 2, rect.width() - 2, 4 );
                        leftRect  = QRectF( rect.left(), rect.top(), handleCenter.x() - rect.left(), rect.height() );
                        rightRect = QRectF( handleCenter.x(), rect.top(), rect.width() - handleCenter.x(), rect.height() );
                    }
                    else
                    {
                        rect      = QRect( rect.center().x() - 2, rect.top() + 2, 4, rect.height() - 2 );
                        leftRect  = QRectF( rect.left(), rect.top(), rect.width(), handleCenter.y() - rect.top() );
                        rightRect = QRectF( rect.left(), handleCenter.y(), rect.width(), rect.height() - handleCenter.y() );
                    }
                    if ( slider->upsideDown )
                    {
                        qSwap( leftRect, rightRect );
                    }

                    painter->setPen( Qt::NoPen );
                    painter->setBrush( calculateAccentColor( option ) );
                    painter->drawRoundedRect( leftRect, 2, 2 );
                    painter->setBrush( WINUI3Colors[ colorSchemeIndex ][ controlStrongFill ] );
                    painter->drawRoundedRect( rightRect, 2, 2 );
                }
                if ( sub & SC_SliderTickmarks )
                {
                    int tickOffset = proxy()->pixelMetric( PM_SliderTickmarkOffset, slider, widget );
                    int ticks      = slider->tickPosition;
                    int thickness  = proxy()->pixelMetric( PM_SliderControlThickness, slider, widget );
                    int len        = proxy()->pixelMetric( PM_SliderLength, slider, widget );
                    int available  = proxy()->pixelMetric( PM_SliderSpaceAvailable, slider, widget );
                    int interval   = slider->tickInterval;
                    if ( interval <= 0 )
                    {
                        interval = slider->singleStep;
                        if ( QStyle::sliderPositionFromValue( slider->minimum, slider->maximum, interval, available )
                                 - QStyle::sliderPositionFromValue( slider->minimum, slider->maximum, 0, available )
                             < 3 )
                        {
                            interval = slider->pageStep;
                        }
                    }
                    if ( !interval )
                    {
                        interval = 1;
                    }
                    int fudge = len / 2;
                    painter->setPen( slider->palette.text().color() );
                    QVarLengthArray<QLineF, 32> lines;
                    int v = slider->minimum;
                    while ( v <= slider->maximum + 1 )
                    {
                        if ( v == slider->maximum + 1 && interval == 1 )
                        {
                            break;
                        }
                        const int v_   = qMin( v, slider->maximum );
                        int tickLength = ( v_ == slider->minimum || v_ >= slider->maximum ) ? 4 : 3;
                        int pos = QStyle::sliderPositionFromValue( slider->minimum, slider->maximum, v_, available, slider->upsideDown );
                        pos += fudge;
                        if ( isHorizontal )
                        {
                            if ( ticks & QSlider::TicksAbove )
                            {
                                lines.append( QLineF( pos, tickOffset - 0.5, pos, tickOffset - tickLength - 0.5 ) );
                            }

                            if ( ticks & QSlider::TicksBelow )
                            {
                                lines.append( QLineF( pos, tickOffset + thickness + 0.5, pos, tickOffset + thickness + tickLength + 0.5 ) );
                            }
                        }
                        else
                        {
                            if ( ticks & QSlider::TicksAbove )
                            {
                                lines.append( QLineF( tickOffset - 0.5, pos, tickOffset - tickLength - 0.5, pos ) );
                            }

                            if ( ticks & QSlider::TicksBelow )
                            {
                                lines.append( QLineF( tickOffset + thickness + 0.5, pos, tickOffset + thickness + tickLength + 0.5, pos ) );
                            }
                        }
                        // in the case where maximum is max int
                        int nextInterval = v + interval;
                        if ( nextInterval < v )
                        {
                            break;
                        }
                        v = nextInterval;
                    }
                    if ( !lines.isEmpty() )
                    {
                        painter->save();
                        painter->translate( slrect.topLeft() );
                        painter->drawLines( lines.constData(), lines.size() );
                        painter->restore();
                    }
                }
                if ( sub & SC_SliderHandle )
                {
                    const qreal outerRadius = qMin( 10.0, ( isHorizontal ? handleRect.height() / 2.0 : handleRect.width() / 2.0 ) - 1 );
                    float innerRadius       = outerRadius * 0.43;

                    if ( option->styleObject )
                    {
                        const QNumberStyleAnimation* animation =
                            qobject_cast<QNumberStyleAnimation*>( getAnimation( option->styleObject ) );
                        if ( animation != nullptr )
                        {
                            innerRadius = animation->currentValue();
                            option->styleObject->setProperty( "_q_inner_radius", innerRadius );
                        }
                        else
                        {
                            bool isInsideHandle = option->activeSubControls == SC_SliderHandle;
                            innerRadius         = outerRadius * sliderInnerRadius( state, isInsideHandle );
                        }
                    }

                    painter->setPen( Qt::NoPen );
                    painter->setBrush( winUI3Color( controlFillSolid ) );
                    painter->drawEllipse( handleCenter, outerRadius, outerRadius );
                    painter->setBrush( calculateAccentColor( option ) );
                    painter->drawEllipse( handleCenter, innerRadius, innerRadius );

                    painter->setPen( winUI3Color( controlStrokeSecondary ) );
                    painter->setBrush( Qt::NoBrush );
                    painter->drawEllipse( handleCenter, outerRadius + 0.5, outerRadius + 0.5 );
                }
                if ( slider->state & State_HasFocus )
                {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=( *slider );
                    fropt.rect = subElementRect( SE_SliderFocusRect, slider, widget );
                    proxy()->drawPrimitive( PE_FrameFocusRect, &fropt, painter, widget );
                }
            }
            break;
#endif
#if QT_CONFIG( combobox )
        case CC_ComboBox :
        {
            if ( const QStyleOptionComboBox* combobox = qstyleoption_cast<const QStyleOptionComboBox*>( option ) )
            {
                QRectF rect = option->rect;  // 不再 shrink

                const bool hasFocus = state & State_HasFocus;

                QStyleOption opt( *option );
                opt.state.setFlag( QStyle::State_On, false );

                painter->setPen( Qt::NoPen );
                painter->setBrush( controlFillBrush( &opt, ControlType::Control ) );
                painter->drawRoundedRect( rect, 6, 6 );

                if ( combobox->frame )
                {
                    QRectF borderRect = rect.adjusted( 0.5, 0.5, -0.5, -0.5 );
                    drawLineEditFrame( painter, borderRect, combobox, combobox->editable );
                }

                if ( sub & SC_ComboBoxArrow )
                {
                    QRectF arrowRect = proxy()->subControlRect( CC_ComboBox, option, SC_ComboBoxArrow, widget ).adjusted( 4, 0, -4, 0 );

                    painter->setFont( assetFont );
                    painter->setPen( controlTextColor( option ) );
                    painter->drawText( arrowRect, Qt::AlignCenter, ChevronDownMed );
                }

                if ( state & State_KeyboardFocusChange && hasFocus )
                {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=( *option );
                    proxy()->drawPrimitive( PE_FrameFocusRect, &fropt, painter, widget );
                }
            }
            break;
        }
        // if ( const QStyleOptionComboBox* combobox = qstyleoption_cast<const QStyleOptionComboBox*>( option ) )
        // {
        //     const auto frameRect = QRectF( option->rect ).marginsRemoved( QMarginsF( 1.5, 1.5, 1.5, 1.5 ) );

        //     const bool hasFocus = state & State_HasFocus;
        //     QStyleOption opt( *option );
        //     opt.state.setFlag( QStyle::State_On, false );
        //     drawRoundedRect( painter, frameRect, Qt::NoPen, controlFillBrush( &opt, ControlType::Control ) );

        //     if ( combobox->frame )
        //     {
        //         drawLineEditFrame( painter, frameRect, combobox, combobox->editable );
        //     }

        //     if ( sub & SC_ComboBoxArrow )
        //     {
        //         QRectF rect = proxy()
        //                           ->subControlRect( CC_ComboBox, option, SC_ComboBoxArrow, widget )
        //                           .adjusted( 4, 0, -4, 1 );
        //         painter->setFont( assetFont );
        //         painter->setPen( controlTextColor( option ) );
        //         painter->drawText( rect, Qt::AlignCenter, ChevronDownMed );
        //     }
        //     if ( state & State_KeyboardFocusChange && hasFocus )
        //     {
        //         QStyleOptionFocusRect fropt;
        //         fropt.QStyleOption::operator=( *option );
        //         proxy()->drawPrimitive( PE_FrameFocusRect, &fropt, painter, widget );
        //     }
        // }
        // break;
#endif  // QT_CONFIG(combobox)
        case CC_ScrollBar :
            if ( const QStyleOptionSlider* scrollbar = qstyleoption_cast<const QStyleOptionSlider*>( option ) )
            {
                QCachedPainter cp( painter,
                                   QLatin1String( "win11_scrollbar" ) % HexString<uint8_t>( colorSchemeIndex )
                                       % HexString<int>( scrollbar->minimum ) % HexString<int>( scrollbar->maximum )
                                       % HexString<int>( scrollbar->sliderPosition ),
                                   scrollbar,
                                   scrollbar->rect.size() );
                if ( cp.needsPainting() )
                {
                    const bool vertical    = scrollbar->orientation == Qt::Vertical;
                    const bool horizontal  = scrollbar->orientation == Qt::Horizontal;
                    const bool isMouseOver = state & State_MouseOver;
                    const bool isRtl       = option->direction == Qt::RightToLeft;

                    if ( isMouseOver )
                    {
                        QRectF rect          = scrollbar->rect;
                        const QPointF center = rect.center();
                        if ( vertical && rect.width() > 24 )
                        {
                            rect.marginsRemoved( QMargins( 0, 2, 2, 2 ) );
                            rect.setWidth( rect.width() / 2 );
                        }
                        else if ( horizontal && rect.height() > 24 )
                        {
                            rect.marginsRemoved( QMargins( 2, 0, 2, 2 ) );
                            rect.setHeight( rect.height() / 2 );
                        }
                        rect.moveCenter( center );
                        cp->setBrush( scrollbar->palette.base() );
                        cp->setPen( Qt::NoPen );
                        cp->drawRoundedRect( rect, topLevelRoundingRadius, topLevelRoundingRadius );
                        rect = rect.marginsRemoved( QMarginsF( 0.5, 0.5, 0.5, 0.5 ) );
                        cp->setBrush( Qt::NoBrush );
                        cp->setPen( WINUI3Colors[ colorSchemeIndex ][ frameColorLight ] );
                        cp->drawRoundedRect( rect, topLevelRoundingRadius + 0.5, topLevelRoundingRadius + 0.5 );
                    }
                    if ( sub & SC_ScrollBarSlider )
                    {
                        QRectF rect          = proxy()->subControlRect( CC_ScrollBar, option, SC_ScrollBarSlider, widget );
                        const QPointF center = rect.center();
                        if ( vertical )
                        {
                            rect.setWidth( isMouseOver ? rect.width() / 2 : 3 );
                        }
                        else
                        {
                            rect.setHeight( isMouseOver ? rect.height() / 2 : 3 );
                        }
                        rect.moveCenter( center );
                        cp->setBrush( Qt::gray );
                        cp->setPen( Qt::NoPen );
                        cp->drawRoundedRect(
                            rect, isMouseOver ? secondLevelRoundingRadius : 1.5, isMouseOver ? secondLevelRoundingRadius : 1.5 );
                    }
                    if ( sub & SC_ScrollBarAddLine )
                    {
                        if ( isMouseOver )
                        {
                            const QRectF rect = proxy()->subControlRect( CC_ScrollBar, option, SC_ScrollBarAddLine, widget );
                            QFont f           = QFont( assetFont );
                            f.setPointSize( 6 );
                            cp->setFont( f );
                            cp->setPen( Qt::gray );
                            const auto str = vertical ? CaretDownSolid8 : ( isRtl ? CaretLeftSolid8 : CaretRightSolid8 );
                            cp->drawText( rect, Qt::AlignCenter, str );
                        }
                    }
                    if ( sub & SC_ScrollBarSubLine )
                    {
                        if ( isMouseOver )
                        {
                            const QRectF rect = proxy()->subControlRect( CC_ScrollBar, option, SC_ScrollBarSubLine, widget );
                            QFont f           = QFont( assetFont );
                            f.setPointSize( 6 );
                            cp->setFont( f );
                            cp->setPen( Qt::gray );
                            const auto str = vertical ? CaretUpSolid8 : ( isRtl ? CaretRightSolid8 : CaretLeftSolid8 );
                            cp->drawText( rect, Qt::AlignCenter, str );
                        }
                    }
                }
            }
            break;
        case CC_MdiControls :
        {
            QFont buttonFont = QFont( assetFont );
            buttonFont.setPointSize( 8 );
            painter->setFont( buttonFont );
            drawTitleBarCloseButton( CC_MdiControls, SC_MdiCloseButton, ChromeClose );
            drawTitleBarButton( CC_MdiControls, SC_MdiNormalButton, ChromeRestore );
            drawTitleBarButton( CC_MdiControls, SC_MdiMinButton, ChromeMinimize );
        }
        break;
        case CC_TitleBar :
            if ( const auto* titlebar = qstyleoption_cast<const QStyleOptionTitleBar*>( option ) )
            {
                painter->setPen( Qt::NoPen );
                painter->setPen( WINUI3Colors[ colorSchemeIndex ][ surfaceStroke ] );
                painter->setBrush( titlebar->palette.button() );
                painter->drawRect( titlebar->rect );

                // draw title
                QRect textRect   = proxy()->subControlRect( CC_TitleBar, titlebar, SC_TitleBarLabel, widget );
                QColor textColor = titlebar->palette.color(
                    titlebar->titleBarState & Qt::WindowActive ? QPalette::Active : QPalette::Disabled, QPalette::WindowText );
                painter->setPen( textColor );
                // Note workspace also does elliding but it does not use the
                // correct font
                QString title = painter->fontMetrics().elidedText( titlebar->text, Qt::ElideRight, textRect.width() - 14 );
                painter->drawText( textRect.adjusted( 1, 1, -1, -1 ), title, QTextOption( Qt::AlignHCenter | Qt::AlignVCenter ) );

                QFont buttonFont = QFont( assetFont );
                buttonFont.setPointSize( 8 );
                painter->setFont( buttonFont );
                auto shouldDrawButton = [ titlebar ]( SubControl sc, Qt::WindowType flag )
                { return ( titlebar->subControls & sc ) && ( titlebar->titleBarFlags & flag ); };

                // min button
                if ( shouldDrawButton( SC_TitleBarMinButton, Qt::WindowMinimizeButtonHint )
                     && !( titlebar->titleBarState & Qt::WindowMinimized ) )
                {
                    drawTitleBarButton( CC_TitleBar, SC_TitleBarMinButton, ChromeMinimize );
                }

                // max button
                if ( shouldDrawButton( SC_TitleBarMaxButton, Qt::WindowMaximizeButtonHint )
                     && !( titlebar->titleBarState & Qt::WindowMaximized ) )
                {
                    drawTitleBarButton( CC_TitleBar, SC_TitleBarMaxButton, ChromeMaximize );
                }

                // close button
                if ( shouldDrawButton( SC_TitleBarCloseButton, Qt::WindowSystemMenuHint ) )
                {
                    drawTitleBarCloseButton( CC_TitleBar, SC_TitleBarCloseButton, ChromeClose );
                }

                // normalize button
                if ( ( titlebar->subControls & SC_TitleBarNormalButton )
                     && ( ( ( titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint )
                            && ( titlebar->titleBarState & Qt::WindowMinimized ) )
                          || ( ( titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint )
                               && ( titlebar->titleBarState & Qt::WindowMaximized ) ) ) )
                {
                    drawTitleBarButton( CC_TitleBar, SC_TitleBarNormalButton, ChromeRestore );
                }

                // context help button
                if ( shouldDrawButton( SC_TitleBarContextHelpButton, Qt::WindowContextHelpButtonHint ) )
                {
                    drawTitleBarButton( CC_TitleBar, SC_TitleBarContextHelpButton, Help );
                }

                // shade button
                if ( shouldDrawButton( SC_TitleBarShadeButton, Qt::WindowShadeButtonHint ) )
                {
                    drawTitleBarButton( CC_TitleBar, SC_TitleBarShadeButton, ChevronUpSmall );
                }

                // unshade button
                if ( shouldDrawButton( SC_TitleBarUnshadeButton, Qt::WindowShadeButtonHint ) )
                {
                    drawTitleBarButton( CC_TitleBar, SC_TitleBarUnshadeButton, ChevronDownSmall );
                }

                // window icon for system menu
                if ( shouldDrawButton( SC_TitleBarSysMenu, Qt::WindowSystemMenuHint ) )
                {
                    const QRect iconRect = proxy()->subControlRect( CC_TitleBar, titlebar, SC_TitleBarSysMenu, widget );
                    if ( iconRect.isValid() )
                    {
                        if ( !titlebar->icon.isNull() )
                        {
                            titlebar->icon.paint( painter, iconRect );
                        }
                        else
                        {
                            QStyleOption tool = *titlebar;
                            const auto extent = proxy()->pixelMetric( PM_SmallIconSize, &tool, widget );
                            const auto dpr    = QStyleHelper::getDpr( widget );
                            const auto icon   = proxy()->standardIcon( SP_TitleBarMenuButton, &tool, widget );
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
                            QPixmap pm = icon.pixmap( QSize( extent, extent ), dpr );
#else
                            QPixmap pm = icon.pixmap( QSize( extent, extent ) );
                            pm.setDevicePixelRatio( dpr );
#endif
                            proxy()->drawItemPixmap( painter, iconRect, Qt::AlignCenter, pm );
                        }
                    }
                }
            }
            break;
        case CC_ToolButton :
            QProxyStyle::drawComplexControl( control, option, painter, widget );
            break;
        default :
            QProxyStyle::drawComplexControl( control, option, painter, widget );
            break;
    }
    painter->restore();
}

void FluentUI3Style::drawPrimitive( PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
{
    PainterStateGuard guard( painter );
    painter->setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );

    int state = option->state;
    if ( transitionsEnabled() && option->styleObject && ( element == PE_IndicatorCheckBox || element == PE_IndicatorRadioButton ) )
    {
        QObject* styleObject = option->styleObject;  // Can be widget or qquickitem
        int oldState         = styleObject->property( "_q_stylestate" ).toInt();
        styleObject->setProperty( "_q_stylestate", int( option->state ) );
        styleObject->setProperty( "_q_stylerect", option->rect );
        bool doTransition =
            ( ( ( state & State_Sunken ) != ( oldState & State_Sunken ) || ( ( state & State_MouseOver ) != ( oldState & State_MouseOver ) )
                || ( state & State_On ) != ( oldState & State_On ) )
              && state & State_Enabled );
        if ( doTransition )
        {
            if ( element == PE_IndicatorRadioButton )
            {
                QNumberStyleAnimation* t = new QNumberStyleAnimation( styleObject );
                t->setStartValue( styleObject->property( "_q_inner_radius" ).toFloat() );
                int indicatorSize = proxy()->pixelMetric( QStyle::PM_ExclusiveIndicatorWidth, option, widget );
                t->setEndValue( radioButtonInnerRadius( state, option, widget, indicatorSize ) );
                styleObject->setProperty( "_q_end_radius", t->endValue() );
                // t->setStartTime(d->animationTime());
                t->setDuration( 150 );
                startAnimation( t );
            }
            else if ( element == PE_IndicatorCheckBox )
            {
                if ( ( oldState & State_Off && state & State_On ) || ( oldState & State_NoChange && state & State_On ) )
                {
                    QNumberStyleAnimation* t = new QNumberStyleAnimation( styleObject );
                    t->setStartValue( 0.0f );
                    t->setEndValue( 1.0f );
                    // t->setStartTime(d->animationTime());
                    t->setDuration( 150 );
                    startAnimation( t );
                }
            }
        }
    }

    switch ( element )
    {
        case PE_FrameFocusRect :
        {
            if ( const QStyleOptionFocusRect* fropt = qstyleoption_cast<const QStyleOptionFocusRect*>( option ) )
            {
                if ( !( fropt->state & State_KeyboardFocusChange ) )
                {
                    break;
                }
                QRectF focusRect = option->rect;
                focusRect        = focusRect.marginsRemoved( QMarginsF( 1.5, 1.5, 1.5, 1.5 ) );
                painter->setPen( winUI3Color( focusFrameInnerStroke ) );
                painter->drawRoundedRect( focusRect, 4, 4 );

                focusRect = focusRect.marginsAdded( QMarginsF( 1.0, 1.0, 1.0, 1.0 ) );
                painter->setPen( QPen( winUI3Color( focusFrameOuterStroke ), 1 ) );
                painter->drawRoundedRect( focusRect, 4, 4 );
            }
            break;
        }
        case PE_PanelTipLabel :
        {
            const auto rect = QRectF( option->rect ).marginsRemoved( QMarginsF( 0.5, 0.5, 0.5, 0.5 ) );
            const auto pen  = highContrastTheme ? option->palette.buttonText().color() : winUI3Color( frameColorLight );
            drawRoundedRect( painter, rect, pen, option->palette.toolTipBase() );
            break;
        }
        case PE_FrameTabWidget :
#if QT_CONFIG( tabwidget )
            if ( const QStyleOptionTabWidgetFrame* frame = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>( option ) )
            {
                const auto rect = QRectF( option->rect ).marginsRemoved( QMarginsF( 0.5, 0.5, 0.5, 0.5 ) );
                const auto pen  = highContrastTheme ? frame->palette.buttonText().color() : winUI3Color( frameColorLight );
                // drawRoundedRect( painter, rect, pen, frame->palette.base() );
                painter->setPen( pen );
                painter->setBrush( frame->palette.base() );
                // painter->drawRoundedRect( rect, secondLevelRoundingRadius, secondLevelRoundingRadius );
                painter->drawRect(rect);
            }
#endif  // QT_CONFIG(tabwidget)
            break;
        case PE_FrameGroupBox :
            if ( const QStyleOptionFrame* frame = qstyleoption_cast<const QStyleOptionFrame*>( option ) )
            {
                const auto pen = highContrastTheme ? frame->palette.buttonText().color() : winUI3Color( frameColorStrong );
                if ( frame->features & QStyleOptionFrame::Flat )
                {
                    painter->setBrush( Qt::NoBrush );
                    painter->setPen( pen );
                    const QRect& fr = frame->rect;
                    QPoint p1( fr.x(), fr.y() + 1 );
                    QPoint p2( fr.x() + fr.width(), p1.y() );
                    painter->drawLine( p1, p2 );
                }
                else
                {
                    const auto frameRect = QRectF( frame->rect ).marginsRemoved( QMarginsF( 1.5, 1.5, 1.5, 1.5 ) );
                    drawRoundedRect( painter, frameRect, pen, Qt::NoBrush );
                }
            }
            break;
        case PE_IndicatorHeaderArrow :
            if ( const QStyleOptionHeader* header = qstyleoption_cast<const QStyleOptionHeader*>( option ) )
            {
                QFont f( assetFont );
                f.setPointSize( 6 );
                painter->setFont( f );
                painter->setPen( header->palette.text().color() );
                QRectF rect = option->rect;
                if ( header->sortIndicator & QStyleOptionHeader::SortUp )
                {
                    painter->drawText( rect, Qt::AlignCenter, ChevronUpSmall );
                }
                else if ( header->sortIndicator & QStyleOptionHeader::SortDown )
                {
                    painter->drawText( rect, Qt::AlignCenter, ChevronDownSmall );
                }
            }
            break;
        case PE_IndicatorCheckBox :
            drawCheckBox( option, painter, widget );
            return;
        case PE_IndicatorBranch :
        {
            if ( option->state & State_Children )
            {
                const bool isReverse = option->direction == Qt::RightToLeft;
                const bool isOpen    = option->state & QStyle::State_Open;
                QFont f( assetFont );
                f.setPointSize( 12 );
                painter->setFont( f );
                painter->setPen( option->palette.color( isOpen ? QPalette::Active : QPalette::Disabled, QPalette::WindowText ) );
                const auto str = isOpen ? ChevronDownMed : ( isReverse ? ChevronLeftMed : ChevronRightMed );
                painter->drawText( option->rect, Qt::AlignCenter, str );
            }
        }
        break;
        case PE_IndicatorRadioButton :
        {
            const bool isOn   = option->state & State_On;
            int indicatorSize = proxy()->pixelMetric( QStyle::PM_ExclusiveIndicatorWidth, option, widget );
            qreal outerRadius = indicatorSize / 2.0;
            qreal innerRadius = radioButtonInnerRadius( state, option, widget, indicatorSize );
            if ( transitionsEnabled() && option->styleObject )
            {
                if ( option->styleObject->property( "_q_end_radius" ).isNull() )
                {
                    option->styleObject->setProperty( "_q_end_radius", innerRadius );
                }
                QNumberStyleAnimation* animation = qobject_cast<QNumberStyleAnimation*>( getAnimation( option->styleObject ) );
                innerRadius = animation ? animation->currentValue() : option->styleObject->property( "_q_end_radius" ).toFloat();
                option->styleObject->setProperty( "_q_inner_radius", innerRadius );
            }

            const QRectF rect    = option->rect;
            const QPointF center = rect.center();

            painter->setPen( borderPenControlAlt( option ) );
            painter->setBrush( controlFillBrush( option, ControlType::ControlAlt ) );
            if ( isOn )
            {
                QPainterPath path;
                path.addEllipse( center, outerRadius, outerRadius );
                path.addEllipse( center, innerRadius, innerRadius );
                painter->drawPath( path );
                // Text On Accent/Primary
                painter->setBrush( option->palette.window().color() );
                painter->drawEllipse( center, innerRadius, innerRadius );
            }
            else
            {
                painter->drawEllipse( center, outerRadius, outerRadius );
            }
            return;
        }
        break;
        case PE_PanelButtonTool :
        {
            PainterStateGuard guard( painter );
            const bool isHover = option->state & State_MouseOver;
            const bool isDown  = option->state & State_Sunken;
            QRect r            = option->rect;
            painter->setPen( Qt::NoPen );
            if ( widget && widget->objectName() == "qt_dockwidget_closebutton" )
            {
                if ( isHover )
                {
                    painter->setBrush( QColor( 232, 17, 35 ) );  // Win11 close hover 红色
                }
                else if ( isDown )
                {
                    painter->setBrush( QColor( 241, 112, 122 ) );
                }
                else
                {
                    return;
                }

                painter->setRenderHint( QPainter::Antialiasing );
                painter->drawRect( r );
                return;
            }
            else if ( widget && widget->objectName() == "qt_dockwidget_floatbutton" )
            {
                if ( isHover )
                {
                    painter->setBrush( QColor( 120, 120, 120, 40 ) );  // Fluent hover 灰
                }
                else if ( isDown )
                {
                    painter->setBrush( QColor( 120, 120, 120, 80 ) );
                }
                else
                {
                    return;
                }

                painter->setRenderHint( QPainter::Antialiasing );
                painter->drawRect( r );
                return;
            }
        }
        case PE_PanelButtonBevel :
        {
            const bool isEnabled   = state & QStyle::State_Enabled;
            const bool isMouseOver = state & QStyle::State_MouseOver;
            const bool isRaised    = state & QStyle::State_Raised;
            const QRectF rect      = option->rect.marginsRemoved( QMargins( 2, 2, 2, 2 ) );
            if ( element == PE_PanelButtonTool && ( ( !isMouseOver && !isRaised ) || !isEnabled ) )
            {
                painter->setPen( Qt::NoPen );
            }
            else
            {
                painter->setPen( WINUI3Colors[ colorSchemeIndex ][ controlStrokePrimary ] );
            }
            drawEffectShadow( painter, option->rect, 2, secondLevelRoundingRadius );
            painter->setBrush( controlFillBrush( option, ControlType::Control ) );
            painter->drawRoundedRect( rect, secondLevelRoundingRadius, secondLevelRoundingRadius );

            if ( isRaised )
            {
                const qreal sublineOffset = secondLevelRoundingRadius - 0.5;
                painter->setPen( WINUI3Colors[ colorSchemeIndex ][ controlStrokeSecondary ] );
                painter->drawLine( rect.bottomLeft() + QPointF( sublineOffset, 0.5 ), rect.bottomRight() + QPointF( -sublineOffset, 0.5 ) );
            }
        }
        break;
        case PE_FrameDefaultButton :
#if QT_VERSION >= QT_VERSION_CHECK( 6, 5, 0 )
            painter->setPen( option->palette.accent().color() );
#else
            painter->setPen( option->palette.color( QPalette::Highlight ) );
#endif

            painter->setBrush( Qt::NoBrush );
            painter->drawRoundedRect( option->rect, secondLevelRoundingRadius, secondLevelRoundingRadius );
            break;
        case PE_FrameMenu :
            break;
        case PE_PanelMenu :
        {
            drawFluentShadow( painter, option->rect, 2, topLevelRoundingRadius );
            const QRect rect = option->rect.marginsRemoved( QMargins( 2, 2, 2, 2 ) );
            painter->setPen( highContrastTheme ? QPen( option->palette.windowText().color(), 2 ) : winUI3Color( frameColorLight ) );
            painter->setBrush( winUI3Color( menuPanelFill ) );
            painter->drawRoundedRect( rect, topLevelRoundingRadius, topLevelRoundingRadius );
            break;
        }
        case PE_PanelLineEdit :
            if ( const auto* panel = qstyleoption_cast<const QStyleOptionFrame*>( option ) )
            {
                const bool isInSpinBox  = widget && qobject_cast<const QAbstractSpinBox*>( widget->parent() ) != nullptr;
                const bool isInComboBox = widget && qobject_cast<const QComboBox*>( widget->parent() ) != nullptr;
                if ( !isInSpinBox && !isInComboBox )
                {
                    const auto frameRect = QRectF( option->rect ).marginsRemoved( QMarginsF( 1.5, 1.5, 1.5, 1.5 ) );

                    auto inputFillBrush = [ this ]( const QStyleOption* option, const QWidget* widget ) -> QBrush
                    {
                        // slightly different states than in controlFillBrush
                        using namespace StyleOptionHelper;
                        const auto role = widget ? widget->backgroundRole() : QPalette::Window;
                        if ( option->palette.isBrushSet( QPalette::Current, role ) )
                        {
                            return option->palette.button();
                        }

                        if ( isDisabled( option ) )
                        {
                            return winUI3Color( fillControlDisabled );
                        }
                        if ( option->state & State_HasFocus )
                        {
                            return winUI3Color( fillControlInputActive );
                        }
                        if ( isHover( option ) )
                        {
                            return winUI3Color( fillControlSecondary );
                        }
                        return winUI3Color( fillControlDefault );
                    };

                    drawRoundedRect( painter, frameRect, Qt::NoPen, inputFillBrush( option, widget ) );
                    if ( panel->lineWidth > 0 )
                    {
                        proxy()->drawPrimitive( PE_FrameLineEdit, panel, painter, widget );
                    }
                }
            }
            break;
        case PE_FrameLineEdit :
        {
            const auto frameRect = QRectF( option->rect ).marginsRemoved( QMarginsF( 1.5, 1.5, 1.5, 1.5 ) );
            drawLineEditFrame( painter, frameRect, option, !( option->state & State_ReadOnly ) );
            if ( state & State_KeyboardFocusChange && state & State_HasFocus )
            {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=( *option );
                proxy()->drawPrimitive( PE_FrameFocusRect, &fropt, painter, widget );
            }
            break;
        }
#if 1
        case PE_Frame :
        {
            if ( const auto* frame = qstyleoption_cast<const QStyleOptionFrame*>( option ) )
            {
                QRectF rect = option->rect;

                const bool isComboPopup = widget && widget->inherits( "QComboBoxPrivateContainer" );

                painter->setPen( Qt::NoPen );

                if ( isComboPopup )
                {
                    QBrush c = WINUI3Colors[ colorSchemeIndex ][ menuPanelFill ];
                    painter->setBrush( c );
                    painter->drawRoundedRect( rect, 6, 6 );
                }
                else
                {
                    painter->setBrush( option->palette.brush( QPalette::Base ) );
                    painter->drawRoundedRect( rect, 6, 6 );
                }

                if ( frame->frameShape == QFrame::NoFrame )
                {
                    break;
                }

                QRectF borderRect = rect.adjusted( 0.5, 0.5, -0.5, -0.5 );
                drawLineEditFrame( painter, borderRect, option, qobject_cast<const QTextEdit*>( widget ) != nullptr );
            }
            break;
        }
#else
        case PE_Frame :
        {
            if ( const auto* frame = qstyleoption_cast<const QStyleOptionFrame*>( option ) )
            {
                const auto rect = QRectF( option->rect ).marginsRemoved( QMarginsF( 1.5, 1.5, 1.5, 1.5 ) );

                const bool isComboPopup = widget && widget->inherits( "QComboBoxPrivateContainer" );
                if ( isComboPopup )
                {
                    QPen pen;
                    if ( highContrastTheme )
                    {
                        pen = QPen( option->palette.windowText().color(), 2 );
                    }
                    else
                    {
                        pen = Qt::NoPen;
                    }

                    QColor c = WINUI3Colors[ colorSchemeIndex ][ menuPanelFill ];
                    drawRoundedRect( painter, rect, pen, c );
                }
                else
                {
                    drawRoundedRect( painter, rect, Qt::NoPen, option->palette.brush( QPalette::Base ) );
                }

                if ( frame->frameShape == QFrame::NoFrame )
                {
                    break;
                }

                drawLineEditFrame( painter, rect, option, qobject_cast<const QTextEdit*>( widget ) != nullptr );
            }
            break;
        }
#endif
        case PE_PanelItemViewItem :
            if ( const QStyleOptionViewItem* vopt = qstyleoption_cast<const QStyleOptionViewItem*>( option ) )
            {
                if ( vopt->backgroundBrush.style() != Qt::NoBrush )
                {
                    PainterStateGuard psg( painter );
                    painter->setBrushOrigin( vopt->rect.topLeft() );
                    painter->fillRect( vopt->rect, vopt->backgroundBrush );
                }
            }
            break;
        case PE_PanelItemViewRow :
            if ( const QStyleOptionViewItem* vopt = qstyleoption_cast<const QStyleOptionViewItem*>( option ) )
            {
                // this is only called from a QTreeView to paint
                //  - the tree branch decoration (incl. selected/hovered or not)
                //  - the (alternate) background of the item in always
                //  unselected state
                const QRect& rect = vopt->rect;
                const bool isRtl  = option->direction == Qt::RightToLeft;
                if ( rect.width() <= 0 )
                {
                    break;
                }

                if ( vopt->features & QStyleOptionViewItem::Alternate )
                {
                    QPalette::ColorGroup cg =
                        ( widget ? widget->isEnabled() : ( vopt->state & QStyle::State_Enabled ) ) ? QPalette::Normal : QPalette::Disabled;
                    if ( cg == QPalette::Normal && !( vopt->state & QStyle::State_Active ) )
                    {
                        cg = QPalette::Inactive;
                    }
                    painter->fillRect( rect, option->palette.brush( cg, QPalette::AlternateBase ) );
                }

                if ( const QTableView* tv = qobject_cast<const QTableView*>( widget );
                     !tv && option->state & State_Selected && !highContrastTheme )
                {
                    // keep in sync with CE_ItemViewItem QListView indicator
                    // painting
#if QT_VERSION >= QT_VERSION_CHECK( 6, 5, 0 )
                    const auto col = option->palette.accent().color();
#else
                    const auto col = option->palette.color( QPalette::Highlight );
#endif

                    painter->setBrush( col );
                    painter->setPen( col );
                    const auto xPos = isRtl ? rect.right() - 4.5f : rect.left() + 3.5f;
                    const auto yOfs = rect.height() / 4.;
                    QRectF r( QPointF( xPos, rect.y() + yOfs ), QPointF( xPos + 1, rect.y() + rect.height() - yOfs ) );
                    painter->drawRoundedRect( r, 1, 1 );
                }

                const bool isTreeDecoration = /*vopt->features.testFlag(
                    QStyleOptionViewItem::IsDecorationForRootColumn)*/
                    false;
                if ( isTreeDecoration && ( ( vopt->state & ( State_Selected | State_MouseOver ) ) != 0 ) && vopt->showDecorationSelected )
                {
                    const bool onlyOne = vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne
                                         || vopt->viewItemPosition == QStyleOptionViewItem::Invalid;
                    bool isFirst = vopt->viewItemPosition == QStyleOptionViewItem::Beginning;
                    bool isLast  = vopt->viewItemPosition == QStyleOptionViewItem::End;

                    if ( onlyOne )
                    {
                        isFirst = true;
                    }

                    if ( isRtl )
                    {
                        isFirst = !isFirst;
                        isLast  = !isLast;
                    }

                    const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>( widget );
                    painter->setBrush( view->alternatingRowColors() ? vopt->palette.highlight()
                                                                    : WINUI3Colors[ colorSchemeIndex ][ subtleHighlightColor ] );
                    painter->setPen( Qt::NoPen );
                    if ( isFirst )
                    {
                        painter->save();
                        painter->setClipRect( rect );
                        painter->drawRoundedRect( rect.marginsRemoved( QMargins( 2, 2, -secondLevelRoundingRadius, 2 ) ),
                                                  secondLevelRoundingRadius,
                                                  secondLevelRoundingRadius );
                        painter->restore();
                    }
                    else if ( isLast )
                    {
                        painter->save();
                        painter->setClipRect( rect );
                        painter->drawRoundedRect( rect.marginsRemoved( QMargins( -secondLevelRoundingRadius, 2, 2, 2 ) ),
                                                  secondLevelRoundingRadius,
                                                  secondLevelRoundingRadius );
                        painter->restore();
                    }
                    else
                    {
                        painter->drawRect( vopt->rect.marginsRemoved( QMargins( 0, 2, 0, 2 ) ) );
                    }
                }
            }
            break;
        case PE_Widget :
        {
            if ( widget->property( "fluentBorder" ).toBool() )
            {
                painter->save();
                painter->setRenderHint( QPainter::Antialiasing );

                QRect r = option->rect.adjusted( 1, 1, -1, -1 );

                QColor borderColor = winUI3Color( frameColorStrongDisabled );

                QPen pen( borderColor );
                pen.setWidth( 1 );
                painter->setPen( pen );

                painter->setBrush( Qt::NoBrush );
                painter->drawRoundedRect( r, 4, 4 );

                painter->restore();
                return;
            }

            if ( widget && widget->palette().isBrushSet( QPalette::Active, widget->backgroundRole() ) )
            {
                const QBrush bg = widget->palette().brush( widget->backgroundRole() );
                updateBrushOrigin_public( painter, widget, bg );
                painter->fillRect( option->rect, bg );
            }
            break;
        }
        case PE_FrameWindow :
            if ( const auto* frm = qstyleoption_cast<const QStyleOptionFrame*>( option ) )
            {
                QRectF rect = option->rect;
                int fwidth  = int( ( frm->lineWidth + frm->midLineWidth ) / nativeMetricScaleFactor( widget ) );

                QRectF bottomLeftCorner  = QRectF( rect.left() + 1.0,
                                                  rect.bottom() - 1.0 - secondLevelRoundingRadius,
                                                  secondLevelRoundingRadius,
                                                  secondLevelRoundingRadius );
                QRectF bottomRightCorner = QRectF( rect.right() - 1.0 - secondLevelRoundingRadius,
                                                   rect.bottom() - 1.0 - secondLevelRoundingRadius,
                                                   secondLevelRoundingRadius,
                                                   secondLevelRoundingRadius );

                // Draw Mask
                if ( widget != nullptr )
                {
                    QBitmap mask( widget->width(), widget->height() );
                    mask.clear();

                    QPainter maskPainter( &mask );
                    maskPainter.setRenderHint( QPainter::Antialiasing );
                    maskPainter.setBrush( Qt::color1 );
                    maskPainter.setPen( Qt::NoPen );
                    maskPainter.drawRoundedRect( option->rect, secondLevelRoundingRadius, secondLevelRoundingRadius );
                    const_cast<QWidget*>( widget )->setMask( mask );
                }

                // Draw Window
                painter->setPen( QPen( frm->palette.base(), fwidth ) );
                painter->drawLine( QPointF( rect.left(), rect.top() ), QPointF( rect.left(), rect.bottom() - fwidth ) );
                painter->drawLine( QPointF( rect.left() + fwidth, rect.bottom() ), QPointF( rect.right() - fwidth, rect.bottom() ) );
                painter->drawLine( QPointF( rect.right(), rect.top() ), QPointF( rect.right(), rect.bottom() - fwidth ) );

                painter->setPen( WINUI3Colors[ colorSchemeIndex ][ surfaceStroke ] );
                painter->drawLine( QPointF( rect.left() + 0.5, rect.top() + 0.5 ),
                                   QPointF( rect.left() + 0.5, rect.bottom() - 0.5 - secondLevelRoundingRadius ) );
                painter->drawLine( QPointF( rect.left() + 0.5 + secondLevelRoundingRadius, rect.bottom() - 0.5 ),
                                   QPointF( rect.right() - 0.5 - secondLevelRoundingRadius, rect.bottom() - 0.5 ) );
                painter->drawLine( QPointF( rect.right() - 0.5, rect.top() + 1.5 ),
                                   QPointF( rect.right() - 0.5, rect.bottom() - 0.5 - secondLevelRoundingRadius ) );

                painter->setPen( Qt::NoPen );
                painter->setBrush( frm->palette.base() );
                painter->drawPie( bottomRightCorner.marginsAdded( QMarginsF( 2.5, 2.5, 0.0, 0.0 ) ), 270 * 16, 90 * 16 );
                painter->drawPie( bottomLeftCorner.marginsAdded( QMarginsF( 0.0, 2.5, 2.5, 0.0 ) ), -90 * 16, -90 * 16 );

                painter->setPen( WINUI3Colors[ colorSchemeIndex ][ surfaceStroke ] );
                painter->setBrush( Qt::NoBrush );
                painter->drawArc( bottomRightCorner, 0 * 16, -90 * 16 );
                painter->drawArc( bottomLeftCorner, -90 * 16, -90 * 16 );
            }
            break;
        default :
            QProxyStyle::drawPrimitive( element, option, painter, widget );
            break;
    }
}

QRect FluentUI3Style::subElementRect( SubElement element, const QStyleOption* option, const QWidget* widget ) const
{
    QRect ret;
    switch ( element )
    {
        case SE_RadioButtonIndicator :
        case SE_CheckBoxIndicator :
            ret = QProxyStyle::subElementRect( element, option, widget );
            ret.moveLeft( ret.left() + contentItemHMargin );
            break;
        case SE_ComboBoxFocusRect :
        case SE_CheckBoxFocusRect :
        case SE_RadioButtonFocusRect :
        case SE_PushButtonFocusRect :
            ret = option->rect;
            break;
        case SE_LineEditContents :
            ret = option->rect.adjusted( 4, 0, -4, 0 );
            break;
        case SE_ItemViewItemCheckIndicator :
        case SE_ItemViewItemDecoration :
        case SE_ItemViewItemText :
        {
            ret = QProxyStyle::subElementRect( element, option, widget );
            if ( !ret.isValid() || highContrastTheme )
            {
                return ret;
            }

            if ( const QListView* lv = qobject_cast<const QListView*>( widget ); lv && lv->viewMode() != QListView::IconMode )
            {
                const int xOfs   = contentHMargin;
                const bool isRtl = option->direction == Qt::RightToLeft;
                if ( isRtl )
                {
                    ret.moveRight( ret.right() - xOfs );
                    if ( ret.left() < option->rect.left() )
                    {
                        ret.setLeft( option->rect.left() );
                    }
                }
                else
                {
                    ret.moveLeft( ret.left() + xOfs );
                    if ( ret.right() > option->rect.right() )
                    {
                        ret.setRight( option->rect.right() );
                    }
                }
            }
            // Add left margin for QTreeView
            else if ( [[maybe_unused]] const QTreeView* tv = qobject_cast<const QTreeView*>( widget ) )
            {
                const int xOfs   = contentHMargin;
                const bool isRtl = option->direction == Qt::RightToLeft;
                if ( isRtl )
                {
                    ret.moveRight( ret.right() - xOfs );
                    if ( ret.left() < option->rect.left() )
                    {
                        ret.setLeft( option->rect.left() );
                    }
                }
                else
                {
                    ret.moveLeft( ret.left() + xOfs );
                    if ( ret.right() > option->rect.right() )
                    {
                        ret.setRight( option->rect.right() );
                    }
                }
            }
            break;
        }
#if QT_CONFIG( progressbar )
        case SE_ProgressBarGroove :
        case SE_ProgressBarContents :
        case SE_ProgressBarLabel :
            if ( const auto* pb = qstyleoption_cast<const QStyleOptionProgressBar*>( option ) )
            {
                QStyleOptionProgressBar optCopy( *pb );
                // we only support label right from content
                optCopy.textAlignment = Qt::AlignRight;
                return QProxyStyle::subElementRect( element, &optCopy, widget );
            }
            break;
#endif  // QT_CONFIG(progressbar)
        case SE_HeaderLabel :
        case SE_HeaderArrow :
            ret = QProxyStyle::subElementRect( element, option, widget );
            break;
        case SE_PushButtonContents :
        {
            int border = proxy()->pixelMetric( PM_DefaultFrameWidth, option, widget );
            ret        = option->rect.marginsRemoved( QMargins( border, border, border, border ) );
            break;
        }
        default :
            ret = QProxyStyle::subElementRect( element, option, widget );
    }
    return ret;
}

QRect FluentUI3Style::subControlRect( ComplexControl control,
                                      const QStyleOptionComplex* option,
                                      SubControl subControl,
                                      const QWidget* widget ) const
{
    QRect ret;
    switch ( control )
    {
#if QT_CONFIG( spinbox )
        case CC_SpinBox :
            if ( const QStyleOptionSpinBox* spinbox = qstyleoption_cast<const QStyleOptionSpinBox*>( option ) )
            {
                const bool hasButtons  = spinbox->buttonSymbols != QAbstractSpinBox::NoButtons;
                const int fw           = spinbox->frame ? proxy()->pixelMetric( PM_SpinBoxFrameWidth, spinbox, widget ) : 0;
                const int buttonHeight = hasButtons ? qMin( spinbox->rect.height() - 3 * fw, spinbox->fontMetrics.height() * 5 / 4 ) : 0;
                const QSize buttonSize( buttonHeight * 6 / 5, buttonHeight );
                const int textFieldLength = spinbox->rect.width() - 2 * fw - 2 * buttonSize.width();

                SpinBoxButtonLayout buttonLayout =
                    widget ? static_cast<SpinBoxButtonLayout>( widget->property( "spinBoxButtonLayout" ).toInt() )
                           : SpinBoxButtonLayout::ArrowsVertical;
                bool horizonal = buttonLayout != SpinBoxButtonLayout::ArrowsVertical;
                QRect r = spinbox->rect.marginsRemoved( QMargins( fw, fw, fw, fw ) );
                if ( !horizonal )
                {
                    int btnHeight = qMin( r.height() / 2, spinbox->fontMetrics.height() );
                    if ( btnHeight == r.height() / 2 )
                    {
                        btnHeight -= 2;
                    }

                    if ( subControl == SC_SpinBoxUp )
                    {
                        return QRect( QPoint( r.topRight().x() - buttonSize.width(), r.topRight().y() + fw * 2 ),
                                      QSize( buttonSize.width(), btnHeight ) );
                    }
                    if ( subControl == SC_SpinBoxDown )
                    {
                        return QRect( QPoint( r.topRight().x() - buttonSize.width(), r.bottomRight().y() - btnHeight - fw ),
                                      QSize( buttonSize.width(), btnHeight ) );
                    }
                    if ( subControl == SC_SpinBoxEditField )
                    {
                        return r.marginsRemoved( QMargins( 0, 0, buttonSize.width(), 0 ) );
                    }
                }

                switch ( subControl )
                {
                    case SC_SpinBoxUp :
                    case SC_SpinBoxDown :
                    {
                        if ( !hasButtons )
                        {
                            return QRect();
                        }
                        const int yOfs = ( r.height() - buttonSize.height() ) / 2;
                        if ( buttonLayout == SpinBoxButtonLayout::ArrowsHorizontalRight )
                        {
                            ret = QRect( r.x() + textFieldLength, r.y() + yOfs, buttonSize.width(), buttonSize.height() );
                            if ( subControl == SC_SpinBoxDown )
                            {
                                ret.moveRight( ret.right() + buttonSize.width() - 2 * fw );
                            }
                            break;
                        }
                        else  // ArrowsHorizontalSides or PlusMinusHorizontalSides
                        {
                            ret = QRect( r.x() + 1, r.y() + ( r.height() - buttonSize.height() ) / 2, buttonSize.width(), buttonSize.height() );
                            if ( subControl == SC_SpinBoxDown )
                            {
                                ret = QRect( ret.right() + textFieldLength - 1, ret.y(), buttonSize.width(), buttonSize.height() );
                            }
                            break;
                        }
                    }
                    case SC_SpinBoxEditField :
                        if ( buttonLayout == SpinBoxButtonLayout::ArrowsHorizontalRight )
                        {
                            ret = QRect( r.x(), r.y(), r.width() - 2 * buttonSize.width(), r.height() );
                        }
                        else
                        {
                            ret = QRect( r.x() + buttonSize.width(), r.y(), r.width() - 2 * buttonSize.width(), r.height() );
                        }
                        break;
                    case SC_SpinBoxFrame :
                        ret = spinbox->rect;
                    default :
                        break;
                }
                ret = visualRect( spinbox->direction, spinbox->rect, ret );
            }
            break;
        case CC_TitleBar :
            if ( const QStyleOptionTitleBar* titlebar = qstyleoption_cast<const QStyleOptionTitleBar*>( option ) )
            {
                SubControl sc                           = subControl;
                ret                                     = QProxyStyle::subControlRect( control, option, subControl, widget );
                static constexpr int indent             = 3;
                static constexpr int controlWidthMargin = 2;
                const int controlHeight                 = titlebar->rect.height();
                const int controlWidth                  = 46;
                const int iconSize                      = proxy()->pixelMetric( QStyle::PM_TitleBarButtonIconSize, option, widget );
                int offset                              = -( controlWidthMargin + indent );

                bool isMinimized = titlebar->titleBarState & Qt::WindowMinimized;
                bool isMaximized = titlebar->titleBarState & Qt::WindowMaximized;

                switch ( sc )
                {
                    case SC_TitleBarLabel :
                        if ( titlebar->titleBarFlags & ( Qt::WindowTitleHint | Qt::WindowSystemMenuHint ) )
                        {
                            ret = titlebar->rect;
                            if ( titlebar->titleBarFlags & Qt::WindowSystemMenuHint )
                            {
                                ret.adjust( iconSize + controlWidthMargin + indent, 0, -controlWidth, 0 );
                            }
                            if ( titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint )
                            {
                                ret.adjust( 0, 0, -controlWidth, 0 );
                            }
                            if ( titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint )
                            {
                                ret.adjust( 0, 0, -controlWidth, 0 );
                            }
                            if ( titlebar->titleBarFlags & Qt::WindowShadeButtonHint )
                            {
                                ret.adjust( 0, 0, -controlWidth, 0 );
                            }
                            if ( titlebar->titleBarFlags & Qt::WindowContextHelpButtonHint )
                            {
                                ret.adjust( 0, 0, -controlWidth, 0 );
                            }
                        }
                        break;
                    case SC_TitleBarContextHelpButton :
                        if ( titlebar->titleBarFlags & Qt::WindowContextHelpButtonHint )
                        {
                            offset += controlWidth;
                        }
                        Q_FALLTHROUGH();
                    case SC_TitleBarMinButton :
                        if ( !isMinimized && ( titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint ) )
                        {
                            offset += controlWidth;
                        }
                        else if ( sc == SC_TitleBarMinButton )
                        {
                            break;
                        }
                        Q_FALLTHROUGH();
                    case SC_TitleBarNormalButton :
                        if ( isMinimized && ( titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint ) )
                        {
                            offset += controlWidth;
                        }
                        else if ( isMaximized && ( titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint ) )
                        {
                            offset += controlWidth;
                        }
                        else if ( sc == SC_TitleBarNormalButton )
                        {
                            break;
                        }
                        Q_FALLTHROUGH();
                    case SC_TitleBarMaxButton :
                        if ( !isMaximized && ( titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint ) )
                        {
                            offset += controlWidth;
                        }
                        else if ( sc == SC_TitleBarMaxButton )
                        {
                            break;
                        }
                        Q_FALLTHROUGH();
                    case SC_TitleBarShadeButton :
                        if ( !isMinimized && ( titlebar->titleBarFlags & Qt::WindowShadeButtonHint ) )
                        {
                            offset += controlWidth;
                        }
                        else if ( sc == SC_TitleBarShadeButton )
                        {
                            break;
                        }
                        Q_FALLTHROUGH();
                    case SC_TitleBarUnshadeButton :
                        if ( isMinimized && ( titlebar->titleBarFlags & Qt::WindowShadeButtonHint ) )
                        {
                            offset += controlWidth;
                        }
                        else if ( sc == SC_TitleBarUnshadeButton )
                        {
                            break;
                        }
                        Q_FALLTHROUGH();
                    case SC_TitleBarCloseButton :
                        if ( titlebar->titleBarFlags & Qt::WindowSystemMenuHint )
                        {
                            offset += controlWidth;
                        }
                        else if ( sc == SC_TitleBarCloseButton )
                        {
                            break;
                        }
                        ret.setRect( titlebar->rect.right() - offset, titlebar->rect.top(), controlWidth, controlHeight );
                        break;
                    case SC_TitleBarSysMenu :
                        if ( titlebar->titleBarFlags & Qt::WindowSystemMenuHint )
                        {
                            const auto yOfs = titlebar->rect.top() + ( titlebar->rect.height() - iconSize ) / 2;
                            ret.setRect( titlebar->rect.left() + controlWidthMargin + indent, yOfs, iconSize, iconSize );
                        }
                        break;
                    default :
                        break;
                }
                if ( widget && isMinimized && titlebar->rect.width() < offset )
                {
                    const_cast<QWidget*>( widget )->resize( controlWidthMargin + indent + offset + iconSize + controlWidthMargin,
                                                            controlWidth );
                }
                ret = visualRect( titlebar->direction, titlebar->rect, ret );
            }
            break;
#endif  // Qt_NO_SPINBOX
        case CC_ScrollBar :
        {
            ret = QProxyStyle::subControlRect( control, option, subControl, widget );

            if ( subControl == SC_ScrollBarAddLine || subControl == SC_ScrollBarSubLine )
            {
                if ( const QStyleOptionSlider* scrollbar = qstyleoption_cast<const QStyleOptionSlider*>( option ) )
                {
                    if ( scrollbar->orientation == Qt::Vertical )
                    {
                        ret = ret.adjusted( 2, 2, -2, -3 );
                    }
                    else
                    {
                        ret = ret.adjusted( 3, 2, -2, -2 );
                    }
                }
            }
            break;
        }
        case CC_ComboBox :
        {
            if ( subControl == SC_ComboBoxArrow )
            {
                const auto indicatorWidth = proxy()->pixelMetric( PM_MenuButtonIndicator, option, widget );
                const int endX            = option->rect.right() - contentHMargin - 2;
                const int startX          = endX - indicatorWidth;
                const QRect rect( QPoint( startX, option->rect.top() ), QPoint( endX, option->rect.bottom() ) );
                ret = visualRect( option->direction, option->rect, rect );
            }
            else
            {
                ret = QProxyStyle::subControlRect( control, option, subControl, widget );
            }
            break;
        }
#if QT_CONFIG( groupbox )
        case CC_GroupBox :
        {
            ret = QProxyStyle::subControlRect( control, option, subControl, widget );
            switch ( subControl )
            {
                case SC_GroupBoxCheckBox :
                    ret.moveTop( 1 );
                    break;
                default :
                    break;
            }
            break;
        }
#endif  // QT_CONFIG(groupbox)
        default :
            ret = QProxyStyle::subControlRect( control, option, subControl, widget );
    }
    return ret;
}

void FluentUI3Style::drawControl( ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
{
    State flags = option->state;

    painter->save();
    painter->setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
    switch ( element )
    {
        case QStyle::CE_ComboBoxLabel :
#if QT_CONFIG( combobox )
            if ( const QStyleOptionComboBox* cb = qstyleoption_cast<const QStyleOptionComboBox*>( option ) )
            {
                painter->setPen( controlTextColor( option ) );
                QStyleOptionComboBox newOption = *cb;
                newOption.rect.adjust( 4, 0, -4, 0 );
                QProxyStyle::drawControl( element, &newOption, painter, widget );
            }
#endif  // QT_CONFIG(combobox)
            break;
        case CE_TabBarTabShape :
#if QT_CONFIG( tabbar )
            if ( const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>( option ) )
            {
                // QRectF tabRect = tab->rect.marginsRemoved( QMargins( 2, 2, 0, 0 ) );
                QRectF tabRect = tab->rect.marginsRemoved( QMargins( 0, 0, 0, 0 ) );
                painter->setPen( Qt::NoPen );
                painter->setBrush( tab->palette.base() );
                if (tab->state & State_Selected)
                {
                    painter->setBrush(colorSchemeIndex == 0
                        ? QColor(230,230,230)
                        : QColor(60,60,60));

                    const int arcLength = 16;
                    QList<QPointF> pts;
                    pts << QPointF(tabRect.bottomLeft().x() - arcLength, tabRect.bottomLeft().y())
                        << tabRect.bottomLeft()
                        << tabRect.topLeft()
                        << tabRect.topRight()
                        << tabRect.bottomRight()
                        << QPointF(tabRect.bottomRight().x() + arcLength, tabRect.bottomRight().y());

                    auto path = buildRoundedPolyline(pts, 7);
                    painter->fillPath(path, painter->brush());
                }
                else if (tab->state & State_MouseOver)
                {
                    painter->setBrush(colorSchemeIndex == 0
                        ? QColor(0,0,0,18)
                        : QColor(255,255,255,18));

                    painter->drawRect(tabRect);
                }
                else
                {
                    painter->setBrush(tab->palette.window());
                }

                painter->setBrush( Qt::NoBrush );
                painter->setPen( highContrastTheme == true ? tab->palette.buttonText().color()
                                                           : WINUI3Colors[ colorSchemeIndex ][ frameColorLight ] );
            }
#endif  // QT_CONFIG(tabbar)
            break;
        case CE_ToolButtonLabel :
#if QT_CONFIG( toolbutton )
#    if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
            QProxyStyle::drawControl( element, option, painter, widget );
            break;
#    endif
            if ( const QStyleOptionToolButton* toolbutton = qstyleoption_cast<const QStyleOptionToolButton*>( option ) )
            {
                QRect rect = toolbutton->rect;
                int shiftX = 0;
                int shiftY = 0;
                if ( toolbutton->state & ( State_Sunken | State_On ) )
                {
                    shiftX = proxy()->pixelMetric( PM_ButtonShiftHorizontal, toolbutton, widget );
                    shiftY = proxy()->pixelMetric( PM_ButtonShiftVertical, toolbutton, widget );
                }
                // Arrow type always overrules and is always shown
                bool hasArrow = toolbutton->features & QStyleOptionToolButton::Arrow;
                if ( ( ( !hasArrow && toolbutton->icon.isNull() ) && !toolbutton->text.isEmpty() )
                     || toolbutton->toolButtonStyle == Qt::ToolButtonTextOnly )
                {
                    int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
                    if ( !proxy()->styleHint( SH_UnderlineShortcut, toolbutton, widget ) )
                    {
                        alignment |= Qt::TextHideMnemonic;
                    }
                    rect.translate( shiftX, shiftY );
                    painter->setFont( toolbutton->font );
                    const QString text = toolButtonElideText( toolbutton, rect, alignment );
                    // option->state has no State_Sunken here,
                    // windowsvistastyle/CC_ToolButton removes it
                    painter->setPen( controlTextColor( option ) );
                    proxy()->drawItemText( painter, rect, alignment, toolbutton->palette, toolbutton->state & State_Enabled, text );
                }
                else
                {
                    QPixmap pm;
                    QSize pmSize = toolbutton->iconSize;
                    if ( !toolbutton->icon.isNull() )
                    {
                        QIcon::State state = toolbutton->state & State_On ? QIcon::On : QIcon::Off;
                        QIcon::Mode mode;
                        if ( !( toolbutton->state & State_Enabled ) )
                        {
                            mode = QIcon::Disabled;
                        }
                        else if ( ( toolbutton->state & State_MouseOver ) && ( toolbutton->state & State_AutoRaise ) )
                        {
                            mode = QIcon::Active;
                        }
                        else
                        {
                            mode = QIcon::Normal;
                        }

                        auto tBtnIconSize = toolbutton->iconSize;
                        if ( widget && QString( widget->metaObject()->className() ) == "QDockWidgetTitleButton" )
                        {
                            tBtnIconSize = QSize( 15, 15 );
                        }
#    if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
                        pm = toolbutton->icon.pixmap(
                            toolbutton->rect.size().boundedTo( tBtnIconSize ), painter->device()->devicePixelRatio(), mode, state );
#    else
                        pm = toolbutton->icon.pixmap( toolbutton->rect.size().boundedTo( tBtnIconSize ), mode, state );
                        pm.setDevicePixelRatio( painter->device()->devicePixelRatio() );
#    endif

                        pmSize = pm.size() / pm.devicePixelRatio();
                    }

                    if ( toolbutton->toolButtonStyle != Qt::ToolButtonIconOnly )
                    {
                        painter->setFont( toolbutton->font );
                        QRect pr = rect, tr = rect;
                        int alignment = Qt::TextShowMnemonic;
                        if ( !proxy()->styleHint( SH_UnderlineShortcut, toolbutton, widget ) )
                        {
                            alignment |= Qt::TextHideMnemonic;
                        }

                        if ( toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon )
                        {
                            pr.setHeight( pmSize.height() + 4 );  // ### 4 is currently hardcoded in
                                                                  // QToolButton::sizeHint()
                            tr.adjust( 0, pr.height() - 1, 0, -1 );
                            pr.translate( shiftX, shiftY );
                            if ( !hasArrow )
                            {
                                proxy()->drawItemPixmap( painter, pr, Qt::AlignCenter, pm );
                            }
                            else
                            {
                                drawArrow( proxy(), toolbutton, pr, painter, widget );
                            }
                            alignment |= Qt::AlignCenter;
                        }
                        else
                        {
                            pr.setWidth( pmSize.width() + 4 );  // ### 4 is currently hardcoded
                                                                // in QToolButton::sizeHint()
                            tr.adjust( pr.width(), 0, 0, 0 );
                            pr.translate( shiftX, shiftY );
                            if ( !hasArrow )
                            {
                                proxy()->drawItemPixmap(
                                    painter, QStyle::visualRect( toolbutton->direction, rect, pr ), Qt::AlignCenter, pm );
                            }
                            else
                            {
                                drawArrow( proxy(), toolbutton, pr, painter, widget );
                            }
                            alignment |= Qt::AlignLeft | Qt::AlignVCenter;
                        }
                        tr.translate( shiftX, shiftY );
                        const QString text = toolButtonElideText( toolbutton, tr, alignment );
                        painter->setPen( controlTextColor( option ) );
                        proxy()->drawItemText( painter,
                                               QStyle::visualRect( toolbutton->direction, rect, tr ),
                                               alignment,
                                               toolbutton->palette,
                                               toolbutton->state & State_Enabled,
                                               text );
                    }
                    else
                    {
                        rect.translate( shiftX, shiftY );
                        if ( hasArrow )
                        {
                            drawArrow( proxy(), toolbutton, rect, painter, widget );
                        }
                        else
                        {
                            proxy()->drawItemPixmap( painter, rect, Qt::AlignCenter, pm );
                        }
                    }
                }
            }
#endif  // QT_CONFIG(toolbutton)
            break;
        case CE_ShapedFrame :
            if ( const QStyleOptionFrame* f = qstyleoption_cast<const QStyleOptionFrame*>( option ) )
            {
                int frameShape  = f->frameShape;
                int frameShadow = QFrame::Plain;
                if ( f->state & QStyle::State_Sunken )
                {
                    frameShadow = QFrame::Sunken;
                }
                else if ( f->state & QStyle::State_Raised )
                {
                    frameShadow = QFrame::Raised;
                }

                int lw  = f->lineWidth;
                int mlw = f->midLineWidth;

                switch ( frameShape )
                {
                    case QFrame::Box :
                        if ( frameShadow == QFrame::Plain )
                        {
                            qDrawPlainRoundedRect( painter,
                                                   f->rect,
                                                   secondLevelRoundingRadius,
                                                   secondLevelRoundingRadius,
                                                   highContrastTheme == true ? f->palette.buttonText().color()
                                                                             : WINUI3Colors[ colorSchemeIndex ][ frameColorStrong ],
                                                   lw );
                        }
                        else
                        {
                            qDrawShadeRect( painter, f->rect, f->palette, frameShadow == QFrame::Sunken, lw, mlw );
                        }
                        break;
                    case QFrame::Panel :
                        if ( frameShadow == QFrame::Plain )
                        {
                            qDrawPlainRoundedRect( painter,
                                                   f->rect,
                                                   secondLevelRoundingRadius,
                                                   secondLevelRoundingRadius,
                                                   highContrastTheme == true ? f->palette.buttonText().color()
                                                                             : WINUI3Colors[ colorSchemeIndex ][ frameColorStrong ],
                                                   lw );
                        }
                        else
                        {
                            qDrawShadePanel( painter, f->rect, f->palette, frameShadow == QFrame::Sunken, lw );
                        }
                        break;
                    default :
                        QProxyStyle::drawControl( element, option, painter, widget );
                }
            }
            break;
#if QT_CONFIG( progressbar )
        case CE_ProgressBarGroove :
            if ( const auto baropt = qstyleoption_cast<const QStyleOptionProgressBar*>( option ) )
            {
                QRect rect     = option->rect;
                QPointF center = rect.center();
                if ( baropt->state & QStyle::State_Horizontal )
                {
                    rect.setHeight( 1 );
                    rect.moveTop( center.y() );
                }
                else
                {
                    rect.setWidth( 1 );
                    rect.moveLeft( center.x() );
                }
                painter->setPen( Qt::NoPen );
                painter->setBrush( Qt::gray );
                painter->drawRect( rect );
            }
            break;
        case CE_ProgressBarContents :
            if ( const auto baropt = qstyleoption_cast<const QStyleOptionProgressBar*>( option ) )
            {
                PainterStateGuard psg( painter );
                QRectF rect = option->rect;
                painter->translate( rect.topLeft() );
                rect.translate( -rect.topLeft() );

                constexpr qreal progressBarThickness     = 3;
                constexpr qreal progressBarHalfThickness = progressBarThickness / 2.0;

                const auto isIndeterminate = baropt->maximum == 0 && baropt->minimum == 0;
                const auto orientation     = ( baropt->state & QStyle::State_Horizontal ) ? Qt::Horizontal : Qt::Vertical;
                const auto inverted        = baropt->invertedAppearance;
                const auto reverse         = ( baropt->direction == Qt::RightToLeft ) ^ inverted;
                // If the orientation is vertical, we use a transform to rotate
                // the progress bar 90 degrees (counter)clockwise. This way we
                // can use the same rendering code for both orientations.
                if ( orientation == Qt::Vertical )
                {
                    rect = QRectF( rect.left(), rect.top(), rect.height(),
                                   rect.width() );  // flip width and height
                    QTransform m;
                    if ( inverted )
                    {
                        m.rotate( 90 );
                        m.translate( 0, -rect.height() + 1 );
                    }
                    else
                    {
                        m.rotate( -90 );
                        m.translate( -rect.width(), 0 );
                    }
                    painter->setTransform( m, true );
                }
                else if ( reverse )
                {
                    QTransform m = QTransform::fromScale( -1, 1 );
                    m.translate( -rect.width(), 0 );
                    painter->setTransform( m, true );
                }
                const qreal offset = ( int( rect.height() ) % 2 == 0 ) ? 0.5f : 0.0f;

                if ( isIndeterminate )
                {
                    constexpr auto loopDurationMSec = 4000;
                    const auto elapsedTime  = std::chrono::time_point_cast<std::chrono::milliseconds>( std::chrono::system_clock::now() );
                    const auto elapsed      = elapsedTime.time_since_epoch().count();
                    const auto handleCenter = ( elapsed % loopDurationMSec ) / float( loopDurationMSec );
                    const auto isLongHandle = ( elapsed / loopDurationMSec ) % 2 == 0;
                    const auto lengthFactor = ( isLongHandle ? 33.0f : 25.0f ) / 100.0f;
                    const auto begin        = qMax( handleCenter * ( 1 + lengthFactor ) - lengthFactor, 0.0f );
                    const auto end          = qMin( handleCenter * ( 1 + lengthFactor ), 1.0f );
                    const auto barBegin     = begin * rect.width();
                    const auto barEnd       = end * rect.width();
                    rect = QRectF( QPointF( rect.left() + barBegin, rect.top() ), QPointF( rect.left() + barEnd, rect.bottom() ) );
                    const_cast<QWidget*>( widget )->update();
                }
                else
                {
                    const auto fillPercentage =
                        ( float( baropt->progress - baropt->minimum ) ) / ( float( baropt->maximum - baropt->minimum ) );
                    rect.setWidth( rect.width() * fillPercentage );
                }
                const QPointF center = rect.center();
                rect.setHeight( progressBarThickness );
                rect.moveTop( center.y() - progressBarHalfThickness - offset );

#    if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
                QBrush br = baropt->palette.accent();
#    else
                QBrush br = baropt->palette.color( QPalette::Highlight );
#    endif
                painter->setPen( Qt::NoPen );
                painter->setBrush( br );
                painter->drawRoundedRect( rect, 1.5, 1.5 );
            }
            break;
        case CE_ProgressBarLabel :
            if ( const auto baropt = qstyleoption_cast<const QStyleOptionProgressBar*>( option ) )
            {
                const bool vertical = !( baropt->state & QStyle::State_Horizontal );
                if ( !vertical )
                {
                    proxy()->drawItemText( painter,
                                           baropt->rect,
                                           Qt::AlignCenter | Qt::TextSingleLine,
                                           baropt->palette,
                                           baropt->state & State_Enabled,
                                           baropt->text,
                                           QPalette::Text );
                }
            }
            break;
#endif  // QT_CONFIG(progressbar)
        case CE_PushButton :
            if ( const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>( option ) )
            {
                proxy()->drawControl( CE_PushButtonBevel, btn, painter, widget );
                QStyleOptionButton subopt = *btn;
                subopt.rect               = subElementRect( SE_PushButtonContents, btn, widget );
                proxy()->drawControl( CE_PushButtonLabel, &subopt, painter, widget );
                if ( btn->state & State_HasFocus )
                {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=( *btn );
                    fropt.rect = subElementRect( SE_PushButtonFocusRect, btn, widget );
                    proxy()->drawPrimitive( PE_FrameFocusRect, &fropt, painter, widget );
                }
            }
            break;
        case CE_PushButtonLabel :
            if ( const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>( option ) )
            {
                using namespace StyleOptionHelper;
                const bool isEnabled = !isDisabled( option );

                QRect textRect = btn->rect.marginsRemoved( QMargins( contentHMargin, 0, contentHMargin, 0 ) );
                int tf         = Qt::AlignCenter | Qt::TextShowMnemonic;
                if ( !proxy()->styleHint( SH_UnderlineShortcut, btn, widget ) )
                {
                    tf |= Qt::TextHideMnemonic;
                }

                if ( !btn->icon.isNull() )
                {
                    // Center both icon and text
                    QIcon::Mode mode = isEnabled ? QIcon::Normal : QIcon::Disabled;
                    if ( mode == QIcon::Normal && btn->state & State_HasFocus )
                    {
                        mode = QIcon::Active;
                    }
                    QIcon::State state = isChecked( btn ) ? QIcon::On : QIcon::Off;

                    int iconSpacing = 4;  // ### 4 is currently hardcoded in
                                          // QPushButton::sizeHint()

                    QRect iconRect  = QRect( textRect.x(), textRect.y(), btn->iconSize.width(), textRect.height() );
                    QRect vIconRect = visualRect( btn->direction, btn->rect, iconRect );
                    textRect.setLeft( textRect.left() + iconRect.width() + iconSpacing );

                    if ( isChecked( btn ) || isPressed( btn ) )
                    {
                        vIconRect.translate( proxy()->pixelMetric( PM_ButtonShiftHorizontal, option, widget ),
                                             proxy()->pixelMetric( PM_ButtonShiftVertical, option, widget ) );
                    }
                    btn->icon.paint( painter, vIconRect, Qt::AlignCenter, mode, state );
                }

                auto vTextRect = visualRect( btn->direction, btn->rect, textRect );
                bool accent    = widget && widget->property( "accent" ).toBool();
                if ( accent )
                {
                    painter->setPen( WINUI3Colors[ colorSchemeIndex ][ textOnAccentPrimary ] );
                }
                else
                {
                    painter->setPen( controlTextColor( option ) );
                }

                // painter->setPen( controlTextColor( option, QPalette::ButtonText, widget ) );
                proxy()->drawItemText( painter, vTextRect, tf, option->palette, isEnabled, btn->text );
            }
            break;
        case CE_PushButtonBevel :
            if ( const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>( option ) )
            {
                using namespace StyleOptionHelper;

                drawEffectShadow( painter, option->rect, 2, secondLevelRoundingRadius );
                QRectF rect = btn->rect.marginsRemoved( QMargins( 2, 2, 2, 2 ) );
                painter->setPen( Qt::NoPen );
                if ( btn->features.testFlag( QStyleOptionButton::Flat ) )
                {
                    painter->setBrush( btn->palette.button() );
                    painter->drawRoundedRect( rect, secondLevelRoundingRadius, secondLevelRoundingRadius );
                    if ( flags & ( State_Sunken | State_On ) )
                    {
                        painter->setBrush( WINUI3Colors[ colorSchemeIndex ][ subtlePressedColor ] );
                    }
                    else if ( flags & State_MouseOver )
                    {
                        painter->setBrush( WINUI3Colors[ colorSchemeIndex ][ subtleHighlightColor ] );
                    }
                    painter->drawRoundedRect( rect, secondLevelRoundingRadius, secondLevelRoundingRadius );
                }
                else
                {
                    bool accent = widget && widget->property( "accent" ).toBool();
                    if ( accent )
                    {
                        painter->setBrush( calculateAccentColor( option ) );
                        painter->setPen( Qt::NoPen );
                    }
                    else
                    {
                        painter->setBrush( controlFillBrush( option, ControlType::Control ) );
                    }

                    painter->drawRoundedRect( rect, secondLevelRoundingRadius, secondLevelRoundingRadius );

                    rect.adjust( 0.5, 0.5, -0.5, -0.5 );
                    const bool defaultButton = btn->features.testFlag( QStyleOptionButton::DefaultButton );
                    painter->setBrush( Qt::NoBrush );
                    painter->setPen(
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
                        defaultButton ? option->palette.accent().color() : WINUI3Colors[ colorSchemeIndex ][ controlStrokePrimary ]
#else
                        defaultButton ? option->palette.color( QPalette::Highlight )
                                      : WINUI3Colors[ colorSchemeIndex ][ controlStrokePrimary ]
#endif
                    );

                    painter->drawRoundedRect( rect, secondLevelRoundingRadius, secondLevelRoundingRadius );

                    painter->setPen( defaultButton ? WINUI3Colors[ colorSchemeIndex ][ controlStrokeOnAccentSecondary ]
                                                   : WINUI3Colors[ colorSchemeIndex ][ controlStrokeSecondary ] );
                }
                if ( btn->features.testFlag( QStyleOptionButton::HasMenu ) )
                {
                    PainterStateGuard psg( painter );

                    const bool isEnabled = !isDisabled( option );
                    QRect textRect       = btn->rect.marginsRemoved( QMargins( contentHMargin, 0, contentHMargin, 0 ) );
                    const auto indSize   = proxy()->pixelMetric( PM_MenuButtonIndicator, btn, widget );
                    const auto indRect   = QRect( btn->rect.right() - indSize - contentItemHMargin,
                                                textRect.top(),
                                                indSize + contentItemHMargin,
                                                btn->rect.height() );
                    const auto vindRect  = visualRect( btn->direction, btn->rect, indRect );
                    textRect.setWidth( textRect.width() - indSize );

                    int fontSize = painter->font().pointSize();
                    QFont f( assetFont );
                    f.setPointSize( qRound( fontSize * 0.9f ) );  // a little bit smaller
                    painter->setFont( f );
                    QColor penColor = option->palette.color( isEnabled ? QPalette::Active : QPalette::Disabled, QPalette::Text );
                    if ( isEnabled )
                    {
                        penColor.setAlpha( percentToAlpha( 60.63 ) );  // fillColorTextSecondary
                    }
                    painter->setPen( penColor );
                    painter->drawText( vindRect, Qt::AlignCenter, ChevronDownMed );
                }
            }
            break;
        case CE_RadioButton :
        case CE_CheckBox :
            if ( const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>( option ) )
            {
                bool isRadio              = ( element == CE_RadioButton );
                QStyleOptionButton subopt = *btn;
                subopt.rect               = subElementRect( isRadio ? SE_RadioButtonIndicator : SE_CheckBoxIndicator, btn, widget );
                proxy()->drawPrimitive( isRadio ? PE_IndicatorRadioButton : PE_IndicatorCheckBox, &subopt, painter, widget );
                subopt.rect = subElementRect( isRadio ? SE_RadioButtonContents : SE_CheckBoxContents, btn, widget );
                proxy()->drawControl( isRadio ? CE_RadioButtonLabel : CE_CheckBoxLabel, &subopt, painter, widget );
                if ( btn->state & State_HasFocus )
                {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=( *btn );
                    fropt.rect = subElementRect( isRadio ? SE_RadioButtonFocusRect : SE_CheckBoxFocusRect, btn, widget );
                    proxy()->drawPrimitive( PE_FrameFocusRect, &fropt, painter, widget );
                }
            }
            break;
        case CE_RadioButtonLabel :
        case CE_CheckBoxLabel :
            if ( const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>( option ) )
            {
                int alignment = visualAlignment( btn->direction, Qt::AlignLeft | Qt::AlignVCenter );

                if ( !proxy()->styleHint( SH_UnderlineShortcut, btn, widget ) )
                {
                    alignment |= Qt::TextHideMnemonic;
                }
                QRect textRect = btn->rect;
                if ( !btn->icon.isNull() )
                {
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
                    const auto pix = btn->icon.pixmap(
                        btn->iconSize, QStyleHelper::getDpr( painter ), ( btn->state & State_Enabled ) ? QIcon::Normal : QIcon::Disabled );
#else
                    QPixmap pix = btn->icon.pixmap( btn->iconSize, ( btn->state & State_Enabled ) ? QIcon::Normal : QIcon::Disabled );
                    pix.setDevicePixelRatio( QStyleHelper::getDpr( painter ) );
#endif

                    proxy()->drawItemPixmap( painter, btn->rect, alignment, pix );
                    if ( btn->direction == Qt::RightToLeft )
                    {
                        textRect.setRight( textRect.right() - btn->iconSize.width() - 4 );
                    }
                    else
                    {
                        textRect.setLeft( textRect.left() + btn->iconSize.width() + 4 );
                    }
                }
                if ( !btn->text.isEmpty() )
                {
                    proxy()->drawItemText( painter,
                                           textRect,
                                           alignment | Qt::TextShowMnemonic,
                                           btn->palette,
                                           btn->state & State_Enabled,
                                           btn->text,
                                           QPalette::WindowText );
                }
            }
            break;
        case CE_MenuBarItem :
            if ( const auto* mbi = qstyleoption_cast<const QStyleOptionMenuItem*>( option ) )
            {
                using namespace StyleOptionHelper;

                constexpr int hPadding      = 11;
                constexpr int topPadding    = 4;
                constexpr int bottomPadding = 6;
                QStyleOptionMenuItem newMbi = *mbi;

                newMbi.font.setPointSize( 10 );
                newMbi.palette.setColor( QPalette::ButtonText, controlTextColor( &newMbi ) );
                if ( !isDisabled( &newMbi ) )
                {
                    QPen pen( Qt::NoPen );
                    QBrush brush( Qt::NoBrush );
                    if ( highContrastTheme )
                    {
                        pen   = QPen( newMbi.palette.highlight().color(), 2 );
                        brush = newMbi.palette.window();
                    }
                    else if ( isPressed( &newMbi ) )
                    {
                        brush = winUI3Color( subtlePressedColor );
                    }
                    else if ( isHover( &newMbi ) )
                    {
                        brush = winUI3Color( subtleHighlightColor );
                    }
                    if ( pen != Qt::NoPen || brush != Qt::NoBrush )
                    {
                        // 调整菜单栏MenuItem的绘制区域，弹窗Menu更靠近MenuItem的左边界，符合WinUI3设计规范 QMargins( 2,
                        // 0, 2, 0 )
                        const QRect rect = mbi->rect.marginsRemoved( QMargins( 5, 0, 5, 0 ) );
                        drawRoundedRect( painter, rect, pen, brush );
                    }
                }
                newMbi.rect.adjust( hPadding, topPadding, -hPadding, -bottomPadding );
                painter->setFont( newMbi.font );
                QCommonStyle::drawControl( element, &newMbi, painter, widget );
            }
            break;

#if QT_CONFIG( menu )
        case CE_MenuEmptyArea :
            break;

        case CE_MenuItem :
            if ( const auto* menuitem = qstyleoption_cast<const QStyleOptionMenuItem*>( option ) )
            {
                const auto visualMenuRect = [ & ]( const QRect& rect ) { return visualRect( option->direction, menuitem->rect, rect ); };
                bool dis                  = !( menuitem->state & State_Enabled );
                bool checked              = menuitem->checkType != QStyleOptionMenuItem::NotCheckable ? menuitem->checked : false;
                bool act                  = menuitem->state & State_Selected;

                const QRect rect = menuitem->rect.marginsRemoved( QMargins( 2, 2, 2, 2 ) );
                if ( act && dis == false )
                {
                    //实际绘制时，右边界会有1px，调整后可以覆盖掉这个
                    QRectF r = QRectF(rect).adjusted(0, 0, 1, 0);
                    drawRoundedRect( painter,
                                     r,
                                     Qt::NoPen,
                                     highContrastTheme ? menuitem->palette.brush( QPalette::Highlight ) : QBrush( winUI3Color( subtleHighlightColor ) ) );
                }

                if ( menuitem->menuItemType == QStyleOptionMenuItem::Separator )
                {
                    constexpr int yoff = 1;
                    painter->setPen( highContrastTheme ? menuitem->palette.buttonText().color() : winUI3Color( dividerStrokeDefault ) );
                    painter->drawLine( menuitem->rect.topLeft() + QPoint( 0, yoff ), menuitem->rect.topRight() + QPoint( 0, yoff ) );
                    break;
                }

                int xOffset = contentHMargin;
                // WinUI3 draws, in contrast to former windows styles, the
                // checkmark and icon separately
                const auto checkMarkWidth = proxy()->pixelMetric( PM_IndicatorWidth, option, widget );
                if ( checked )
                {
                    QRect vRect( visualMenuRect( QRect( rect.x() + xOffset, rect.y(), checkMarkWidth, rect.height() ) ) );
                    PainterStateGuard psg( painter );
                    painter->setFont( assetFont );
                    painter->setPen( option->palette.text().color() );
                    const auto textToDraw = QStringLiteral( u"\uE73E" );
                    painter->drawText( vRect, Qt::AlignCenter, textToDraw );
                }
                if ( menuitem->menuHasCheckableItems )
                {
                    xOffset += checkMarkWidth + contentItemHMargin;
                }
                if ( !menuitem->icon.isNull() )
                {
                    // 4 is added to maxIconWidth in qmenu.cpp to
                    // PM_SmallIconSize
                    QRect vRect( visualMenuRect( QRect( rect.x() + xOffset, rect.y(), menuitem->maxIconWidth - 4, rect.height() ) ) );
                    QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                    if ( act && !dis )
                    {
                        mode = QIcon::Active;
                    }
                    const auto size = proxy()->pixelMetric( PM_SmallIconSize, option, widget );
                    QRect pmr( QPoint( 0, 0 ), QSize( size, size ) );
                    pmr.moveCenter( vRect.center() );
                    menuitem->icon.paint( painter, pmr, Qt::AlignCenter, mode, checked ? QIcon::On : QIcon::Off );
                }
                if ( menuitem->maxIconWidth > 0 )
                {
                    xOffset += menuitem->maxIconWidth - 4 + contentItemHMargin;
                }

                QStringView s( menuitem->text );
                if ( !s.isEmpty() )
                {  // draw text
                    QPoint tl( rect.left() + xOffset, rect.top() );
#    if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
                    int shortcutWidth = menuitem->reservedShortcutWidth;
#    else
                    int shortcutWidth = menuitem->tabWidth;
#    endif

                    QPoint br( rect.right() - shortcutWidth - contentHMargin, rect.bottom() );
                    QRect textRect( tl, br );
                    QRect vRect( visualMenuRect( textRect ) );

                    qsizetype t    = s.indexOf( u'\t' );
                    int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                    if ( !proxy()->styleHint( SH_UnderlineShortcut, menuitem, widget ) )
                    {
                        text_flags |= Qt::TextHideMnemonic;
                    }
                    text_flags |= Qt::AlignLeft;
                    // a submenu doesn't paint a possible shortcut in WinUI3
                    if ( t >= 0 && menuitem->menuItemType != QStyleOptionMenuItem::SubMenu )
                    {
                        QRect shortcutRect( QPoint( textRect.right(), textRect.top() ), QPoint( rect.right(), textRect.bottom() ) );
                        QRect vShortcutRect( visualMenuRect( shortcutRect ) );
                        QColor penColor;
                        if ( highContrastTheme )
                        {
                            penColor = menuitem->palette.color( act ? QPalette::HighlightedText : QPalette::Text );
                        }
                        else
                        {
                            penColor = menuitem->palette.color( dis ? QPalette::Disabled : QPalette::Active, QPalette::Text );
                            if ( !dis )
                            {
                                penColor.setAlpha( percentToAlpha( 60.63 ) );  // fillColorTextSecondary
                            }
                        }
                        painter->setPen( penColor );
                        const QString textToDraw = s.mid( t + 1 ).toString();
                        painter->drawText( vShortcutRect, text_flags, textToDraw );
                        s = s.left( t );
                    }
                    QFont font = menuitem->font;
                    if ( menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem )
                    {
                        font.setBold( true );
                    }
                    painter->setFont( font );
                    QColor penColor;
                    if ( highContrastTheme && act )
                    {
                        penColor = menuitem->palette.color( QPalette::HighlightedText );
                    }
                    else
                    {
                        penColor = menuitem->palette.color( dis ? QPalette::Disabled : QPalette::Current, QPalette::Text );
                    }
                    painter->setPen( penColor );
                    const QString textToDraw = s.left( t ).toString();
                    painter->drawText( vRect, text_flags, textToDraw );
                }
                if ( menuitem->menuItemType == QStyleOptionMenuItem::SubMenu )
                {  // draw sub menu arrow
                    int fontSize = menuitem->font.pointSize();
                    QFont f( assetFont );
                    f.setPointSize( qRound( fontSize * 0.9f ) );  // a little bit smaller
                    painter->setFont( f );
                    int yOfs = qRound( fontSize / 3.0f );  // an offset to align the '>' with
                                                           // the baseline of the text
                    QPoint tl( rect.right() - 2 * windowsArrowHMargin - contentItemHMargin, rect.top() + yOfs );
                    QRect submenuRect( tl, rect.bottomRight() );
                    QRect vSubMenuRect = visualMenuRect( submenuRect );
                    painter->setPen( option->palette.text().color() );
                    const bool isReverse = option->direction == Qt::RightToLeft;
                    const auto str       = isReverse ? ChevronLeftMed : ChevronRightMed;
                    painter->drawText( vSubMenuRect, Qt::AlignCenter, str );
                }
            }
            break;
#endif  // QT_CONFIG(menu)
        case CE_MenuBarEmptyArea :
        {
            break;
        }
        case CE_HeaderEmptyArea :
            break;
        case CE_HeaderSection :
        {
            if ( const QStyleOptionHeader* header = qstyleoption_cast<const QStyleOptionHeader*>( option ) )
            {
                painter->setPen( Qt::NoPen );
                painter->setBrush( header->palette.button() );
                painter->drawRect( header->rect );

                painter->setPen( highContrastTheme == true ? header->palette.buttonText().color()
                                                           : WINUI3Colors[ colorSchemeIndex ][ frameColorLight ] );
                painter->setBrush( Qt::NoBrush );

                if ( header->position == QStyleOptionHeader::OnlyOneSection )
                {
                    break;
                }
                else if ( header->position == QStyleOptionHeader::Beginning )
                {
                    painter->drawLine( QPointF( option->rect.topRight() ) + QPointF( 0.5, 0.0 ),
                                       QPointF( option->rect.bottomRight() ) + QPointF( 0.5, 0.0 ) );
                }
                else if ( header->position == QStyleOptionHeader::End )
                {
                    painter->drawLine( QPointF( option->rect.topLeft() ) - QPointF( 0.5, 0.0 ),
                                       QPointF( option->rect.bottomLeft() ) - QPointF( 0.5, 0.0 ) );
                }
                else
                {
                    painter->drawLine( QPointF( option->rect.topRight() ) + QPointF( 0.5, 0.0 ),
                                       QPointF( option->rect.bottomRight() ) + QPointF( 0.5, 0.0 ) );
                    painter->drawLine( QPointF( option->rect.topLeft() ) - QPointF( 0.5, 0.0 ),
                                       QPointF( option->rect.bottomLeft() ) - QPointF( 0.5, 0.0 ) );
                }
                painter->drawLine( QPointF( option->rect.bottomLeft() ) + QPointF( 0.0, 0.5 ),
                                   QPointF( option->rect.bottomRight() ) + QPointF( 0.0, 0.5 ) );
            }
            break;
        }
        case CE_ItemViewItem :
        {
            if ( const QStyleOptionViewItem* vopt = qstyleoption_cast<const QStyleOptionViewItem*>( option ) )
            {
                const auto p    = proxy();
                QRect checkRect = p->subElementRect( SE_ItemViewItemCheckIndicator, vopt, widget );
                QRect iconRect  = p->subElementRect( SE_ItemViewItemDecoration, vopt, widget );
                QRect textRect  = p->subElementRect( SE_ItemViewItemText, vopt, widget );

                // draw the background
                proxy()->drawPrimitive( PE_PanelItemViewItem, option, painter, widget );

                QRect rect       = vopt->rect /*.marginsRemoved( {3,3,3,3} )*/;
                const bool isRtl = option->direction == Qt::RightToLeft;
                bool onlyOne     = vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne
                               || vopt->viewItemPosition == QStyleOptionViewItem::Invalid;
                bool isFirst = vopt->viewItemPosition == QStyleOptionViewItem::Beginning;
                bool isLast  = vopt->viewItemPosition == QStyleOptionViewItem::End;

                // the tree decoration already painted the left side of the
                // rounded rect
                // if ( vopt->features.testFlag( QStyleOptionViewItem::IsDecoratedRootColumn )
                //      && vopt->showDecorationSelected )
                // {
                //     isFirst = false;
                //     if ( onlyOne )
                //     {
                //         onlyOne = false;
                //         isLast  = true;
                //     }
                // }

                if ( isRtl )
                {
                    if ( isFirst )
                    {
                        isFirst = false;
                        isLast  = true;
                    }
                    else if ( isLast )
                    {
                        isFirst = true;
                        isLast  = false;
                    }
                }
                const bool highlightCurrent = ( vopt->state & ( State_Selected | State_MouseOver ) ) != 0;

                if ( highlightCurrent )
                {
                    if ( highContrastTheme )
                    {
                        painter->setBrush( vopt->palette.highlight() );
                    }
                    else
                    {
                        const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>( widget );
                        painter->setBrush( view && view->alternatingRowColors() ? vopt->palette.highlight()
                                                                                : winUI3Color( subtleHighlightColor ) );
                    }
                }
                else
                {
                    painter->setBrush( vopt->backgroundBrush );
                }
                painter->setPen( Qt::NoPen );

                if ( onlyOne )
                {
                    painter->drawRoundedRect(
                        rect.marginsRemoved( QMargins( 2, 2, 2, 2 ) ), secondLevelRoundingRadius, secondLevelRoundingRadius );
                }
                else if ( isFirst )
                {
                    painter->save();
                    painter->setClipRect( rect );
                    painter->drawRoundedRect( rect.marginsRemoved( QMargins( 2, 2, -secondLevelRoundingRadius, 2 ) ),
                                              secondLevelRoundingRadius,
                                              secondLevelRoundingRadius );
                    painter->restore();
                }
                else if ( isLast )
                {
                    painter->save();
                    painter->setClipRect( rect );
                    painter->drawRoundedRect( rect.marginsRemoved( QMargins( -secondLevelRoundingRadius, 2, 2, 2 ) ),
                                              secondLevelRoundingRadius,
                                              secondLevelRoundingRadius );
                    painter->restore();
                }
                else
                {
                    painter->drawRect( rect.marginsRemoved( QMargins( 0, 2, 0, 2 ) ) );
                }

                // draw the check mark
                if ( vopt->features & QStyleOptionViewItem::HasCheckIndicator )
                {
                    QStyleOptionViewItem option( *vopt );
                    option.rect  = checkRect;
                    option.state = option.state & ~QStyle::State_HasFocus;

                    switch ( vopt->checkState )
                    {
                        case Qt::Unchecked :
                            option.state |= QStyle::State_Off;
                            break;
                        case Qt::PartiallyChecked :
                            option.state |= QStyle::State_NoChange;
                            break;
                        case Qt::Checked :
                            option.state |= QStyle::State_On;
                            break;
                    }
                    proxy()->drawPrimitive( QStyle::PE_IndicatorItemViewItemCheck, &option, painter, widget );
                }

                // draw the icon
                if ( iconRect.isValid() )
                {
                    QIcon::Mode mode = QIcon::Normal;
                    if ( !( vopt->state & QStyle::State_Enabled ) )
                    {
                        mode = QIcon::Disabled;
                    }
                    else if ( vopt->state & QStyle::State_Selected )
                    {
                        mode = QIcon::Selected;
                    }
                    QIcon::State state = vopt->state & QStyle::State_Open ? QIcon::On : QIcon::Off;
                    vopt->icon.paint( painter, iconRect, vopt->decorationAlignment, mode, state );
                }

                painter->setPen( highlightCurrent && highContrastTheme ? vopt->palette.base().color() : vopt->palette.text().color() );
                viewItemDrawText( painter, vopt, textRect );

                // paint a vertical marker for QListView
                if ( vopt->state & State_Selected && !highContrastTheme )
                {
                    if ( const QListView* lv = qobject_cast<const QListView*>( widget ); lv && lv->viewMode() != QListView::IconMode )
                    {
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
                        const auto col = vopt->palette.accent().color();
#else
                        const auto col = vopt->palette.color( QPalette::Highlight );  // Qt5/Qt6.5 fallback
#endif

                        painter->setBrush( col );
                        painter->setPen( col );
                        const auto xPos = isRtl ? rect.right() - 4.5f : rect.left() + 3.5f;
                        const auto yOfs = rect.height() / 4.;
                        QRectF r( QPointF( xPos, rect.y() + yOfs ), QPointF( xPos + 2, rect.y() + rect.height() - yOfs ) );
                        painter->drawRoundedRect( r, 1, 1 );
                    }
                }
            }
            break;
        }
        case CE_DockWidgetTitle :
            if ( const QStyleOptionDockWidget* dwOpt = qstyleoption_cast<const QStyleOptionDockWidget*>( option ) )
            {
                const QDockWidget* dockWidget = qobject_cast<const QDockWidget*>( widget );
                QRect rect                    = option->rect;
                if ( dockWidget && dockWidget->isFloating() )
                {
                    QCommonStyle::drawControl( element, option, painter, widget );
                    break;  // otherwise fall through
                }

                const bool verticalTitleBar = dwOpt->verticalTitleBar;

                if ( verticalTitleBar )
                {
                    rect = rect.transposed();

                    painter->translate( rect.left() - 1, rect.top() + rect.width() );
                    painter->rotate( -90 );
                    painter->translate( -rect.left() + 1, -rect.top() );
                }

                painter->setBrush( winUI3Color( fillControlAltSecondary ) );
                painter->setPen( Qt::NoPen );
                painter->drawRect( rect.adjusted( 0, 1, -1, -3 ) );

                int buttonMargin      = 4;
                int mw                = proxy()->pixelMetric( QStyle::PM_DockWidgetTitleMargin, dwOpt, widget );
                int fw                = proxy()->pixelMetric( PM_DockWidgetFrameWidth, dwOpt, widget );
                const QDockWidget* dw = qobject_cast<const QDockWidget*>( widget );
                bool isFloating       = dw && dw->isFloating();

                QRect r         = option->rect.adjusted( 0, 2, -1, -3 );
                QRect titleRect = r;

                if ( dwOpt->closable )
                {
                    QSize sz = proxy()->standardIcon( QStyle::SP_TitleBarCloseButton, dwOpt, widget ).actualSize( QSize( 10, 10 ) );
                    titleRect.adjust( 0, 0, -sz.width() - mw - buttonMargin, 0 );
                }

                if ( dwOpt->floatable )
                {
                    QSize sz = proxy()->standardIcon( QStyle::SP_TitleBarMaxButton, dwOpt, widget ).actualSize( QSize( 10, 10 ) );
                    titleRect.adjust( 0, 0, -sz.width() - mw - buttonMargin, 0 );
                }

                if ( isFloating )
                {
                    titleRect.adjust( 0, -fw, 0, 0 );
                    if ( widget && widget->windowIcon().cacheKey() != QApplication::windowIcon().cacheKey() )
                    {
                        titleRect.adjust( titleRect.height() + mw, 0, 0, 0 );
                    }
                }
                else
                {
                    titleRect.adjust( mw, 0, 0, 0 );
                    if ( !dwOpt->floatable && !dwOpt->closable )
                    {
                        titleRect.adjust( 0, 0, -mw, 0 );
                    }
                }
                if ( !verticalTitleBar )
                {
                    titleRect = visualRect( dwOpt->direction, r, titleRect );
                }

                if ( !dwOpt->title.isEmpty() )
                {
                    QString titleText = painter->fontMetrics().elidedText(
                        dwOpt->title, Qt::ElideRight, verticalTitleBar ? titleRect.height() : titleRect.width() );
                    const int indent = 4;
                    painter->setPen( controlTextColor( option ) );
                    drawItemText( painter,
                                  rect.adjusted( indent + 1, 1, -indent - 1, -1 ),
                                  Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                                  dwOpt->palette,
                                  dwOpt->state & State_Enabled,
                                  titleText,
                                  QPalette::WindowText );
                }
            }
            break;
        default :
            QProxyStyle::drawControl( element, option, painter, widget );
            break;
    }
    painter->restore();
}

int FluentUI3Style::styleHint( StyleHint hint, const QStyleOption* opt, const QWidget* widget, QStyleHintReturn* returnData ) const
{
    switch ( hint )
    {
        case SH_Menu_AllowActiveAndDisabled :
            return 0;
        case SH_GroupBox_TextLabelColor :
            if ( opt != nullptr && widget != nullptr )
            {
                return opt->palette.text().color().rgba();
            }
            return 0;
        case SH_ItemView_ShowDecorationSelected :
            return 1;
        case SH_Slider_AbsoluteSetButtons :
            return Qt::LeftButton;
        case SH_Slider_PageSetButtons :
            return 0;
        case SH_DockWidget_ButtonsHaveFrame :
            return 1;
        default :
            return QProxyStyle::styleHint( hint, opt, widget, returnData );
    }
}

static bool isComboBoxPopup( QWidget* w )
{
    if ( !w )
    {
        return false;
    }

    // ComboBox popup 一定是 Popup
    if ( w->windowType() != Qt::Popup )
    {
        return false;
    }

    // Menu 已单独处理
    if ( qobject_cast<QMenu*>( w ) )
    {
        return false;
    }

    // ScrollBar 绝对不是
    if ( qobject_cast<QScrollBar*>( w ) )
    {
        return false;
    }

    // parent 链中有 QComboBox
    QWidget* p = w->parentWidget();
    while ( p )
    {
        if ( qobject_cast<QComboBox*>( p ) )
        {
            return true;
        }
        p = p->parentWidget();
    }

    return false;
}

void FluentUI3Style::polish( QWidget* widget )
{
#if QT_CONFIG( commandlinkbutton )
    if ( !qobject_cast<QCommandLinkButton*>( widget ) )
#endif  // QT_CONFIG(commandlinkbutton)
        QProxyStyle::polish( widget );

    // 没有动态更新主题的需求，可屏蔽
    if ( auto le = qobject_cast<QLineEdit*>( widget ) )
    {
        if ( !le->isClearButtonEnabled() )
        {
            return;
        }

        QToolButton* btn = le->findChild<QToolButton*>();
        if ( btn )
        {
            QIcon icon = standardIcon( QStyle::SP_LineEditClearButton, nullptr, le );
            btn->setIcon( icon );
        }
    }
    if ( auto dock = qobject_cast<QDockWidget*>( widget ) )
    {
        auto buttons = dock->findChildren<QAbstractButton*>();

        for ( auto btn : buttons )
        {
            btn->setAttribute( Qt::WA_Hover, true );
            btn->setMouseTracking( true );
        }
    }

    const bool isScrollBar  = qobject_cast<QScrollBar*>( widget );
    const bool isMenu       = qobject_cast<QMenu*>( widget );
    const bool isComboPopup = isComboBoxPopup( widget );

    if ( isScrollBar )
    {
        bool wasCreated      = widget->testAttribute( Qt::WA_WState_Created );
        bool layoutDirection = widget->testAttribute( Qt::WA_RightToLeft );
        widget->setAttribute( Qt::WA_OpaquePaintEvent, false );
        widget->setAttribute( Qt::WA_TranslucentBackground );

        widget->setWindowFlag( Qt::NoDropShadowWindowHint );
        widget->setAttribute( Qt::WA_RightToLeft, layoutDirection );
        widget->setAttribute( Qt::WA_WState_Created, wasCreated );
    }
    else if ( isMenu || isComboPopup )
    {
        bool wasCreated      = widget->testAttribute( Qt::WA_WState_Created );
        bool layoutDirection = widget->testAttribute( Qt::WA_RightToLeft );

        widget->setAttribute( Qt::WA_OpaquePaintEvent, false );

        widget->setAttribute( Qt::WA_TranslucentBackground );
        widget->setWindowFlag( Qt::FramelessWindowHint );
        widget->setWindowFlag( Qt::Popup );

        widget->setWindowFlag( Qt::NoDropShadowWindowHint );

        widget->setAttribute( Qt::WA_RightToLeft, layoutDirection );
        widget->setAttribute( Qt::WA_WState_Created, wasCreated );
    }
    else if ( QComboBox* cb = qobject_cast<QComboBox*>( widget ) )
    {
        if ( cb->isEditable() )
        {
            QLineEdit* le = cb->lineEdit();
            le->setFrame( false );
        }
    }
    else if ( const auto* scrollarea = qobject_cast<QAbstractScrollArea*>( widget ); scrollarea && !qobject_cast<QGraphicsView*>( widget )
#if QT_CONFIG( mdiarea )
                                                                                     && !qobject_cast<QMdiArea*>( widget )
#endif
    )
    {
        if ( scrollarea->frameShape() == QFrame::StyledPanel )
        {
            const auto vp                   = scrollarea->viewport();
            const bool isAutoFillBackground = vp->autoFillBackground();
            const bool isStyledBackground   = vp->testAttribute( Qt::WA_StyledBackground );
            vp->setProperty( "_q_original_autofill_background", isAutoFillBackground );
            vp->setProperty( "_q_original_styled_background", isStyledBackground );
            vp->setAutoFillBackground( false );
            vp->setAttribute( Qt::WA_StyledBackground, true );
        }
        // QTreeView & QListView are already set in the base windowsvista style
        if ( auto table = qobject_cast<QTableView*>( widget ) )
        {
            table->viewport()->setAttribute( Qt::WA_Hover, true );
        }
    }
}

QSize FluentUI3Style::sizeFromContents( ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget ) const
{
    QSize contentSize( size );

    switch ( type )
    {
#if QT_CONFIG( menubar )
        case CT_MenuBarItem :
            if ( !contentSize.isEmpty() )
            {
                constexpr int hMargin    = 2 * 6;
                constexpr int hPadding   = 2 * 11;
                constexpr int itemHeight = 32;
                contentSize.setWidth( contentSize.width() + hMargin + hPadding );
#    if QT_CONFIG( tabwidget )
                if ( widget->parent() && !qobject_cast<const QTabWidget*>( widget->parent() ) )
#    endif
                    contentSize.setHeight( itemHeight );
            }
            break;
#endif
#if QT_CONFIG( menu )
        case CT_MenuItem :
            if ( const auto* menuItem = qstyleoption_cast<const QStyleOptionMenuItem*>( option ) )
            {
                int width = size.width();
                int height;
                if ( menuItem->menuItemType == QStyleOptionMenuItem::Separator )
                {
                    width  = 10;
                    height = 3;
                }
                else
                {
                    height                             = menuItem->fontMetrics.height() + 8;
                    constexpr int fluentMenuItemHeight = 32;
                    height                             = height < fluentMenuItemHeight ? fluentMenuItemHeight : height;
                    if ( !menuItem->icon.isNull() )
                    {
                        int iconExtent = proxy()->pixelMetric( PM_SmallIconSize, option, widget );
                        height         = qMax( height, menuItem->icon.actualSize( QSize( iconExtent, iconExtent ) ).height() + 4 );
                    }
                }
                if ( menuItem->text.contains( u'\t' ) )
                {
                    width += contentItemHMargin;  // the text width is already in
                }
                if ( menuItem->menuItemType == QStyleOptionMenuItem::SubMenu )
                {
                    width += 2 * windowsArrowHMargin + contentItemHMargin;
                }
                if ( menuItem->menuItemType == QStyleOptionMenuItem::DefaultItem )
                {
                    const QFontMetrics fm( menuItem->font );
                    QFont fontBold = menuItem->font;
                    fontBold.setBold( true );
                    const QFontMetrics fmBold( fontBold );
                    width += fmBold.horizontalAdvance( menuItem->text ) - fm.horizontalAdvance( menuItem->text );
                }
                // in contrast to windowsvista, the checkmark and icon are drawn
                // separately
                if ( menuItem->menuHasCheckableItems )
                {
                    const auto checkMarkWidth = proxy()->pixelMetric( PM_IndicatorWidth, option, widget );
                    width += checkMarkWidth + contentItemHMargin * 2;
                }
                // we have an icon and it's already in the given size, only add
                // margins 4 is added in qmenu.cpp to PM_SmallIconSize
                if ( menuItem->maxIconWidth > 0 )
                {
                    width += contentItemHMargin * 2 + menuItem->maxIconWidth - 4;
                }
                width += 2 * 2;  // margins for rounded border
                width += 2 * contentHMargin;
                if ( width < 100 )  // minimum size
                {
                    width = 100;
                }
                contentSize = QSize( width, height );
            }
            break;
#endif  // QT_CONFIG(menu)
#if QT_CONFIG( spinbox )
        case CT_SpinBox :
        {
            if ( const auto* spinBoxOpt = qstyleoption_cast<const QStyleOptionSpinBox*>( option ) )
            {
                // Add button + frame widths
                // const qreal dpi       = QStyleHelper::dpi( option );
                const bool hasButtons = ( spinBoxOpt->buttonSymbols != QAbstractSpinBox::NoButtons );
                const int margins     = 8;
                // const int buttonWidth = hasButtons ? qRound( QStyleHelper::dpiScaled( 16, dpi ) ) : 0;
                const int buttonWidth = hasButtons ? 16 + contentItemHMargin : 0;
                const int frameWidth  = spinBoxOpt->frame ? proxy()->pixelMetric( PM_SpinBoxFrameWidth, spinBoxOpt, widget ) : 0;

                contentSize += QSize( 2 * buttonWidth + 2 * frameWidth + 2 * margins, 2 * frameWidth );
            }
            break;
        }
#endif
#if QT_CONFIG( combobox )
        case CT_ComboBox :
            if ( const auto* comboBoxOpt = qstyleoption_cast<const QStyleOptionComboBox*>( option ) )
            {
                contentSize = QProxyStyle::sizeFromContents( type, option, size,
                                                             widget );  // don't rely on QWindowsThemeData
                contentSize += QSize( 4, 4 );                           // default win11 style margins
                if ( comboBoxOpt->subControls & SC_ComboBoxArrow )
                {
                    const auto w = proxy()->pixelMetric( PM_MenuButtonIndicator, option, widget );
                    contentSize.rwidth() += w + contentItemHMargin;
                }
            }
            break;
#endif
        case CT_HeaderSection :
            // windows vista does not honor the indicator (as it was drawn above
            // the text, not on the side) so call QWindowsStyle::styleHint
            // directly to get the correct size hint
            contentSize = QProxyStyle::sizeFromContents( type, option, size, widget );
            break;
        case CT_RadioButton :
        case CT_CheckBox :
            if ( const auto* buttonOpt = qstyleoption_cast<const QStyleOptionButton*>( option ) )
            {
                const auto p       = proxy();
                const bool isRadio = ( type == CT_RadioButton );

                const int width  = p->pixelMetric( isRadio ? PM_ExclusiveIndicatorWidth : PM_IndicatorWidth, option, widget );
                const int height = p->pixelMetric( isRadio ? PM_ExclusiveIndicatorHeight : PM_IndicatorHeight, option, widget );

                int margins = 2 * contentItemHMargin;
                if ( !buttonOpt->icon.isNull() || !buttonOpt->text.isEmpty() )
                {
                    margins += p->pixelMetric( isRadio ? PM_RadioButtonLabelSpacing : PM_CheckBoxLabelSpacing, option, widget );
                }

                contentSize += QSize( width + margins, 4 );
                contentSize.setHeight( qMax( size.height(), height + 2 * contentItemHMargin ) );
            }
            break;

            // the indicator needs 2px more in width when there is no text, not
            // needed when the style draws the text
            contentSize = QProxyStyle::sizeFromContents( type, option, size, widget );
            if ( size.width() == 0 )
            {
                contentSize.rwidth() += 2;
            }
            break;
        case CT_PushButton :
        {
            contentSize = QProxyStyle::sizeFromContents( type, option, size, widget );
            // we want our own horizontal spacing
            const int oldMargin = proxy()->pixelMetric( PM_ButtonMargin, option, widget );
            contentSize.rwidth() += 2 * contentHMargin - oldMargin;
            break;
        }
            // or ui setHeight
        case CT_ToolButton :
        {
            contentSize = QProxyStyle::sizeFromContents( type, option, size, widget );
            break;
        }
        case CT_ItemViewItem :
        {
            if ( auto view = qobject_cast<const QListView*>( widget ) )
            {
                if ( view->parentWidget() && view->parentWidget()->parentWidget()
                     && qobject_cast<const QComboBox*>( view->parentWidget()->parentWidget() ) )
                {
                    contentSize.setHeight( 32 );  // ComboBox dropdown item
                    break;
                }
            }

            // FluentUI 3 列表项标准常量
            const int FLUENT_H_MARGIN                             = 12;  // 水平边距（左右各12px）
            [[maybe_unused]] const int FLUENT_V_MARGIN            = 8;   // 常规模式垂直边距（上下各8px）
            [[maybe_unused]] const int FLUENT_V_MARGIN_COMPACT    = 4;   // 紧凑模式垂直边距
            const int FLUENT_ITEM_HEIGHT                          = 40;  // 常规列表项总高度
            [[maybe_unused]] const int FLUENT_ITEM_HEIGHT_COMPACT = 32;  // 紧凑列表项总高度

            if ( const auto* viewItemOpt = qstyleoption_cast<const QStyleOptionViewItem*>( option ) )
            {
                if ( const QListView* lv = qobject_cast<const QListView*>( widget ); lv && lv->viewMode() != QListView::IconMode )
                {
                    QStyleOptionViewItem vOpt( *viewItemOpt );
                    vOpt.rect.setRight( vOpt.rect.right() - FLUENT_H_MARGIN );
                    contentSize = QProxyStyle::sizeFromContents( type, &vOpt, size, widget );
                    contentSize.rwidth() += FLUENT_H_MARGIN;
                    contentSize.setHeight( FLUENT_ITEM_HEIGHT );
                }
                else if ( auto view = qobject_cast<const QListView*>( widget ) )
                {
                    if ( view->parentWidget() && view->parentWidget()->parentWidget()
                         && qobject_cast<const QComboBox*>( view->parentWidget()->parentWidget() ) )
                    {
                        contentSize = QProxyStyle::sizeFromContents( type, option, size, widget );
                        contentSize.setHeight( 32 );
                        return contentSize;
                    }
                }
                else
                {
                    contentSize = QProxyStyle::sizeFromContents( type, option, size, widget );
                    contentSize.setHeight( FLUENT_ITEM_HEIGHT );
                }
            }
            break;
        }
    case CT_TabBarTab:
    {
        contentSize = QProxyStyle::sizeFromContents( type, option, size, widget );
        contentSize.setHeight(32);
        contentSize.setWidth(contentSize.width()+20);
    }
        break;
        default :
            contentSize = QProxyStyle::sizeFromContents( type, option, size, widget );
            break;
    }

    switch ( type )
    {
        case CT_PushButton :
        case CT_ToolButton :
        case CT_CheckBox :
        case CT_RadioButton :
        case CT_ComboBox :
        case CT_LineEdit :
        case CT_SpinBox :
            if ( contentSize.height() < 31 )
            {
                contentSize.setHeight( 31 );
            }
            break;
        default :
            break;
    }

    return contentSize;
}

int FluentUI3Style::pixelMetric( PixelMetric metric, const QStyleOption* option, const QWidget* widget ) const
{
    int res = 0;

    switch ( metric )
    {
        case PM_IndicatorWidth :
        case PM_IndicatorHeight :
        case PM_ExclusiveIndicatorWidth :
        case PM_ExclusiveIndicatorHeight :
            res = 18;
            break;
        case PM_SliderThickness :  // full height of a slider
            if ( auto opt = qstyleoption_cast<const QStyleOptionSlider*>( option ) )
            {
                // hard-coded in qslider.cpp, but we need a little bit more
                constexpr auto TickSpace = 5;
                if ( opt->tickPosition & QSlider::TicksAbove )
                {
                    res += 6 - TickSpace;
                }
                if ( opt->tickPosition & QSlider::TicksBelow )
                {
                    res += 6 - TickSpace;
                }
            }
            Q_FALLTHROUGH();
        case PM_SliderControlThickness :  // size of the control handle
        case PM_SliderLength :            // same because handle is a circle with r=8
            res += 2 * 10;
            break;
        case PM_RadioButtonLabelSpacing :
        case PM_CheckBoxLabelSpacing :
            res = 2 * contentItemHMargin;
            break;
        case PM_TitleBarButtonIconSize :
            res = 16;
            break;
        case PM_TitleBarButtonSize :
            res = 32;
            break;
        case PM_ScrollBarExtent :
            res = 12;
            break;
        case PM_SubMenuOverlap :
            res = -1;
            break;
        case PM_MenuButtonIndicator :
        {
            res = contentItemHMargin;
            if ( widget )
            {
                const int fontSize = widget->font().pixelSize();
                QFont f( assetFont );
                f.setPixelSize( qRound( fontSize * 0.9f ) );  // a little bit smaller
                QFontMetrics fm( f );
                res += fm.horizontalAdvance( ChevronDownMed );
            }
            else
            {
                res += 12;
            }
            break;
        }
        case PM_DefaultFrameWidth :
        {
            res = 4;
            if ( qobject_cast<const QListView*>( widget ) )
            {
                res = 2;
            }
            if ( isComboBoxPopup( const_cast<QWidget*>( widget ) ) )
            {
                res = 2;
            }
        }
        break;
        case PM_ButtonShiftHorizontal :
        case PM_ButtonShiftVertical :
            res = 0;
            break;
        case PM_TreeViewIndentation :
            res = 30;
            break;
        case PM_DockWidgetTitleBarButtonMargin :
            res = 12;
            break;
        default :
            res = QProxyStyle::pixelMetric( metric, option, widget );
    }

    return res;
}

/*
The colors for Windows 11 are taken from the official WinUI3 Figma style at
https://www.figma.com/community/file/1159947337437047524
*/
#define SET_IF_UNRESOLVED( GROUP, ROLE, VALUE )                                \
    if ( !result.isBrushSet( QPalette::Inactive, ROLE ) || styleSheetChanged ) \
    result.setColor( GROUP, ROLE, VALUE )

static void populateLightSystemBasePalette( QPalette& result )
{
    static QString oldStyleSheet;
    const bool styleSheetChanged = oldStyleSheet != qApp->styleSheet();

    constexpr QColor textColor     = QColor( 0x00, 0x00, 0x00, 0xE4 );
    constexpr QColor textDisabled  = QColor( 0x00, 0x00, 0x00, 0x5C );
    constexpr QColor btnFace       = QColor( 0xFF, 0xFF, 0xFF, 0xB3 );
    constexpr QColor base          = QColor( 0xFF, 0xFF, 0xFF, 0xFF );
    constexpr QColor alternateBase = QColor( 0x00, 0x00, 0x00, 0x09 );
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    const QColor btnHighlight = result.accent().color();
#else
    const QColor btnHighlight = result.color( QPalette::Highlight );
#endif

    const QColor btnColor = result.button().color();

    SET_IF_UNRESOLVED( QPalette::Active, QPalette::Highlight, btnHighlight );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::WindowText, textColor );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::Button, btnFace );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::Light, btnColor.lighter( 150 ) );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::Dark, btnColor.darker( 200 ) );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::Mid, btnColor.darker( 150 ) );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::Text, textColor );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::BrightText, btnHighlight );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::Base, base );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::Window, /*QColor( 0xF3, 0xF3, 0xF3, 0xFF )*/ QColor( 249, 249, 249 ) );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::ButtonText, textColor );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::Midlight, btnColor.lighter( 125 ) );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::Shadow, Qt::black );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::ToolTipBase, result.window().color() );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::ToolTipText, result.windowText().color() );
    SET_IF_UNRESOLVED( QPalette::Active, QPalette::AlternateBase, alternateBase );

    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Highlight, btnHighlight );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::WindowText, textColor );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Button, btnFace );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Light, btnColor.lighter( 150 ) );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Dark, btnColor.darker( 200 ) );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Mid, btnColor.darker( 150 ) );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Text, textColor );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::BrightText, btnHighlight );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Base, base );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Window, /*QColor( 0xF3, 0xF3, 0xF3, 0xFF )*/ QColor( 249, 249, 249 ) );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::ButtonText, textColor );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Midlight, btnColor.lighter( 125 ) );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Shadow, Qt::black );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::ToolTipBase, result.window().color() );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::ToolTipText, result.windowText().color() );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::AlternateBase, alternateBase );

    result.setColor( QPalette::Disabled, QPalette::WindowText, textDisabled );

    if ( result.midlight() == result.button() )
    {
        result.setColor( QPalette::Midlight, btnColor.lighter( 110 ) );
    }
    oldStyleSheet = qApp->styleSheet();
}

static void populateDarkSystemBasePalette( QPalette& result )
{
    static QString oldStyleSheet;
    const bool styleSheetChanged = oldStyleSheet != qApp->styleSheet();

    constexpr QColor alternateBase = QColor( 0xFF, 0xFF, 0xFF, 0x0F );

    SET_IF_UNRESOLVED( QPalette::Active, QPalette::AlternateBase, alternateBase );

    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::AlternateBase, alternateBase );

    oldStyleSheet = qApp->styleSheet();
}

void FluentUI3Style::polish( QPalette& result )
{
    highContrastTheme = isHighContrastTheme();
    colorSchemeIndex  = getColorSchemeIndex();

    if ( !highContrastTheme && colorSchemeIndex == 0 )
    {
        populateLightSystemBasePalette( result );
    }
    else if ( !highContrastTheme && colorSchemeIndex == 1 )
    {
        populateDarkSystemBasePalette( result );
    }

    const bool styleSheetChanged = false;  // so the macro works

    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Button, result.button().color() );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Window, result.window().color() );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Light, result.light().color() );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Dark, result.dark().color() );
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Accent, result.accent().color() );
#else
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Highlight, result.color( QPalette::Highlight ) );
#endif

    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Highlight, result.highlight().color() );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::HighlightedText, result.highlightedText().color() );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::Text, result.text().color() );
    SET_IF_UNRESOLVED( QPalette::Inactive, QPalette::WindowText, result.windowText().color() );
}

void FluentUI3Style::unpolish( QWidget* widget )
{
#if QT_CONFIG( commandlinkbutton )
    if ( !qobject_cast<QCommandLinkButton*>( widget ) )
#endif  // QT_CONFIG(commandlinkbutton)
        QProxyStyle::unpolish( widget );

    if ( const auto* scrollarea = qobject_cast<QAbstractScrollArea*>( widget ); scrollarea
#if QT_CONFIG( mdiarea )
                                                                                && !qobject_cast<QMdiArea*>( widget )
#endif
    )
    {
        const auto vp                    = scrollarea->viewport();
        const auto wasAutoFillBackground = vp->property( "_q_original_autofill_background" ).toBool();
        vp->setAutoFillBackground( wasAutoFillBackground );
        vp->setProperty( "_q_original_autofill_background", QVariant() );
        const auto origStyledBackground = vp->property( "_q_original_styled_background" ).toBool();
        vp->setAttribute( Qt::WA_StyledBackground, origStyledBackground );
        vp->setProperty( "_q_original_styled_background", QVariant() );
    }
}

QIcon FluentUI3Style::standardIcon( StandardPixmap sp, const QStyleOption* option, const QWidget* widget ) const
{
    // if (sp == SP_TitleBarCloseButton) { return makeFluentIcon(QChar(0xE8BB)); }
    if ( sp == SP_LineEditClearButton || sp == SP_TitleBarCloseButton || sp == SP_TabCloseButton)
    {
        QIcon icon = fluentIcon( QChar( 0xE894 ) );
        return icon;
    }
    if ( sp == SP_TitleBarNormalButton )
    {
        return colorSchemeIndex ? QIcon( ":/resource/images/window-restore-s-light.png" )
                                : QIcon( ":/resource/images/window-restore-s-dark.png" );
    }

    return QProxyStyle::standardIcon( sp, option, widget );
}

void FluentUI3Style::drawCheckBox( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
{
    Q_UNUSED( widget )

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );

    const bool isOn      = option->state & QStyle::State_On;
    const bool isPartial = option->state & QStyle::State_NoChange;

    QRectF rect          = option->rect;
    const QPointF center = rect.center();

    drawRoundedRect( painter, rect, borderPenControlAlt( option ), controlFillBrush( option, ControlType::ControlAlt ) );

    if ( isOn )
    {
        painter->setFont( assetFont );
        painter->setPen( controlTextColor( option, QPalette::Window ) );
        qreal clipWidth = 1.0;
        QFontMetrics fm( assetFont );
        QRectF clipRect = fm.boundingRect( AcceptMedium );
        if ( transitionsEnabled() && option->styleObject )
        {
            QNumberStyleAnimation* animation = qobject_cast<QNumberStyleAnimation*>( getAnimation( option->styleObject ) );
            if ( animation )
            {
                clipWidth = animation->currentValue();
            }
        }

        clipRect.moveCenter( center );
        clipRect.setLeft( rect.x() + ( rect.width() - clipRect.width() ) / 2.0 + 0.5 );
        clipRect.setWidth( clipWidth * clipRect.width() );
        painter->drawText( clipRect, Qt::AlignVCenter | Qt::AlignLeft, AcceptMedium );
    }
    else if ( isPartial )
    {
        painter->setFont( assetFont );
        painter->setPen( controlTextColor( option, QPalette::Window ) );
        painter->drawText( rect, Qt::AlignCenter, Dash12 );
    }

    painter->restore();
}

QPen FluentUI3Style::borderPenControlAlt( const QStyleOption* option ) const
{
    using namespace StyleOptionHelper;
    if ( isChecked( option ) )
    {
        return Qt::NoPen;
    }

    if ( isDisabled( option ) || isPressed( option ) )
    {
        return winUI3Color( frameColorStrongDisabled );
    }
    return winUI3Color( frameColorStrong );
}

QColor blend( const QColor& fg, const QColor& bg, double alpha )
{
    return QColor( int( bg.red() + ( fg.red() - bg.red() ) * alpha ),
                   int( bg.green() + ( fg.green() - bg.green() ) * alpha ),
                   int( bg.blue() + ( fg.blue() - bg.blue() ) * alpha ),
                   fg.alpha() );
}

QColor FluentUI3Style::calculateAccentColor( const QStyleOption* option ) const
{
    using namespace StyleOptionHelper;
    if ( isDisabled( option ) )
    {
        return winUI3Color( fillAccentDisabled );
    }

    const auto alphaColor = isPressed( option ) ? fillAccentTertiary : isHover( option ) ? fillAccentSecondary : fillAccentDefault;
    const auto alpha      = winUI3Color( alphaColor );

#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    // accent 在 inactive 窗口下可能退化成灰色
    QColor col = /*option->palette.accent().color()*/ option->palette.color( QPalette::Active, QPalette::Accent );
#else
    QColor col = option->palette.color( QPalette::Highlight );
#endif
    col.setAlpha( alpha.alpha() );
    return col;
}

QBrush FluentUI3Style::controlFillBrush( const QStyleOption* option, ControlType controlType ) const
{
    using namespace StyleOptionHelper;
    static constexpr WINUI3Color colorEnums[ 2 ][ 4 ] = {
        // Light & Dark Control
        { fillControlDefault,      fillControlSecondary,   fillControlTertiary,       fillControlDisabled    },
        // Light & Dark Control Alt
        { fillControlAltSecondary, fillControlAltTertiary, fillControlAltQuarternary, fillControlAltDisabled },
    };

    if ( option->palette.isBrushSet( QPalette::Current, QPalette::Button ) )
    {
        return option->palette.button();
    }

    if ( !isChecked( option ) && isAutoRaise( option ) )
    {
        return Qt::NoBrush;
    }

    // checked is the same for Control (Buttons) and Control Alt (Radiobuttons/Checkboxes)
    if ( isChecked( option ) )
    {
        return calculateAccentColor( option );
    }

    const auto state = calcControlState( option );
    return winUI3Color( colorEnums[ int( controlType ) ][ int( state ) ] );
}

QColor FluentUI3Style::controlTextColor( const QStyleOption* option, QPalette::ColorRole role ) const
{
    using namespace StyleOptionHelper;
    static constexpr WINUI3Color colorEnums[ 2 ][ 4 ] = {
        // Control, unchecked
        { textPrimary,         textPrimary,         textSecondary,         textDisabled         },
        // Control, checked
        { textOnAccentPrimary, textOnAccentPrimary, textOnAccentSecondary, textOnAccentDisabled },
    };

    if ( option->palette.isBrushSet( QPalette::Current, QPalette::ButtonText ) )
    {
        return option->palette.buttonText().color();
    }

    const int colorIndex = isChecked( option ) ? 1 : 0;
    const auto state     = calcControlState( option );
    const auto alpha     = winUI3Color( colorEnums[ colorIndex ][ int( state ) ] );
    QColor col           = option->palette.color( role );
    col.setAlpha( alpha.alpha() );
    return col;
}

void drawBottomUnderline( QPainter* p, const QRectF& rect, const QStyleOption* option )
{
    const bool hasFocus = option->state & QStyle::State_HasFocus;
    const bool hover    = option->state & QStyle::State_MouseOver;
    const bool enabled  = option->state & QStyle::State_Enabled;

    QColor lineColor;

    if ( !enabled )
    {
        lineColor = QColor( 160, 160, 160 );
    }
    else if ( hasFocus )
    {
        lineColor = QColor( 0, 120, 215 );  // Accent
    }
    else if ( hover )
    {
        lineColor = QColor( 100, 100, 100 );
    }
    else
    {
        lineColor = QColor( 130, 130, 130 );
    }

    const qreal thickness = hasFocus ? 2.0 : 1.0;

    QPainterPath path;

    qreal y = rect.bottom() - thickness / 2.0;

    path.moveTo( rect.left(), y );
    path.lineTo( rect.right(), y );

    QPen pen( lineColor, thickness );
    pen.setCapStyle( Qt::FlatCap );

    p->save();
    p->setRenderHint( QPainter::Antialiasing, false );
    p->setPen( pen );
    p->setBrush( Qt::NoBrush );
    p->drawPath( path );
    p->restore();
}

void FluentUI3Style::drawLineEditFrame( QPainter* p, const QRectF& rect, const QStyleOption* o, bool isEditable ) const
{
    const bool isHovered = o->state & State_MouseOver;
    const auto frameCol  = highContrastTheme
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
                              ? o->palette.color( isHovered ? QPalette::Accent : QPalette::ButtonText )
#else
                              ? o->palette.color( isHovered ? QPalette::Highlight : QPalette::ButtonText )  // Qt5/Qt6.5 fallback
#endif
                              : winUI3Color( frameColorLight );

    drawRoundedRect( p, rect, frameCol, Qt::NoBrush );

    if ( !isEditable )
    {
        return;
    }

    PainterStateGuard psg( p );
    p->setClipRect( rect.marginsRemoved( QMarginsF( 0, rect.height() - 0.5, 0, -1 ) ) );

    const bool hasFocus     = o->state & State_HasFocus;
    const auto underlineCol = hasFocus
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
                                  ? o->palette.color( QPalette::Accent )
#else
                                  ? o->palette.color( QPalette::Highlight )  // Qt5/Qt6.5 fallback
#endif
                                  : ( colorSchemeIndex == 0 ? QColor( 0x80, 0x80, 0x80 ) : QColor( 0xa0, 0xa0, 0xa0 ) );
    const auto penUnderline = QPen( underlineCol, hasFocus ? 2 : 1 );
    if ( qobject_cast<QLineEdit*>( o->styleObject ) )
    {
        drawRoundedRect( p, rect, penUnderline, Qt::NoBrush );
    }
    else
    {
        const qreal penWidth = hasFocus ? 2.0 : 1.0;
        const qreal halfPen  = penWidth / 2.0;

        QRectF underlineClip( rect.left(), rect.bottom() - halfPen, rect.width(), penWidth );

        PainterStateGuard guard( p );
        p->setClipRect( underlineClip );

        QPen pen( underlineCol, penWidth );
        p->setPen( pen );
        p->setBrush( Qt::NoBrush );
        drawRoundedRect( p, rect, penUnderline, Qt::NoBrush );
    }
}

QColor FluentUI3Style::winUI3Color( WINUI3Color col ) const
{
    return WINUI3Colors[ colorSchemeIndex ][ col ];
}

QIcon FluentUI3Style::fluentIcon( const QChar& ch ) const
{
    QFont f( assetFont );
    f.setPixelSize( 27 );

    QPixmap pix( 30, 30 );
    pix.fill( Qt::transparent );

    QPainter p( &pix );
    p.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );

    p.setFont( f );
    p.setPen( winUI3Color( textPrimary ) );
    p.drawText( pix.rect(), Qt::AlignCenter, ch );

    return QIcon( pix );
}

void FluentUI3Style::drawFluentShadow( QPainter* painter, QRect rect, int shadowWidth, int radius ) const
{
    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    painter->setPen( Qt::NoPen );

    QColor color = colorSchemeIndex ? QColor( 0x70, 0x70, 0x70 ) : QColor( 0x9C, 0x9B, 0x9E );

    for ( int i = 0; i < shadowWidth; ++i )
    {
        QRect r   = rect.adjusted( i, i, -i, -i );
        int alpha = 25 * ( shadowWidth - i ) / shadowWidth;
        color.setAlpha( alpha );
        painter->setBrush( color );

        painter->drawRoundedRect( r, radius + shadowWidth - i, radius + shadowWidth - i );
    }

    painter->restore();
}

void FluentUI3Style::drawEffectShadow( QPainter* painter, QRect widgetRect, int shadowBorderWidth, int borderRadius ) const
{
    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    QPainterPath path;
    path.setFillRule( Qt::WindingFill );
    QColor color = colorSchemeIndex ? QColor( 0x70, 0x70, 0x70 ) : QColor( 0x9C, 0x9B, 0x9E );
    for ( int i = 0; i < shadowBorderWidth; i++ )
    {
        path.addRoundedRect( widgetRect.x() + shadowBorderWidth - i,
                             widgetRect.y() + shadowBorderWidth - i,
                             widgetRect.width() - ( shadowBorderWidth - i ) * 2,
                             widgetRect.height() - ( shadowBorderWidth - i ) * 2,
                             borderRadius + i,
                             borderRadius + i );
        int alpha = 2 * ( shadowBorderWidth - i + 1 );
        color.setAlpha( alpha > 255 ? 255 : alpha );
        painter->setPen( color );
        painter->drawPath( path );
    }
    painter->restore();
}
