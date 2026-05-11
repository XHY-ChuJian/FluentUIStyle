#include "fluentui3style.h"

#include <QApplication>
#include <QCoreApplication>
#include <QBitmap>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QDockWidget>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsView>
#include <QHash>
#include <QIcon>
#include <QHoverEvent>
#include <QLineEdit>
#include <QListView>
#include <QMdiArea>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QPointer>
#include <QScreen>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QString>
#include <QStyleHints>
#include <QSysInfo>
#include <QTabBar>
#include <QTableView>
#include <QTextEdit>
#include <QTextLayout>
#include <QToolButton>
#include <QTreeView>
#include <QCursor>
#include <QMouseEvent>
#include <QtMath>
#include <QtGlobal>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QVariant>

#include <array>

// #include <private/qcommonstyle_p.h>
// #include <private/qstylehelper_p.h>
// #include <private/qstyleanimation_p.h>
// #include <private/qhexstring_p.h>

#include "fluentui3colors.h"
#include "palettemanager.h"
#include "qapplication.h"
#include "qcheckbox.h"
#include "qhexstring_p.h"
#include "qstyleanimation_p.h"
#include "qstylehelper_p.h"
#include "qstyleoption.h"

#include "fluentui3styleproperties.h"

#if QT_VERSION <= QT_VERSION_CHECK(6, 8, 0)
#include "fluentuiappearance.h"
#include "palettemanager.h"
#endif

static constexpr int topLevelRoundingRadius = 6;    // Radius for toplevel items like popups for round corners
static constexpr int secondLevelRoundingRadius = 4; // Radius for second level items like hovered menu item round corners
static constexpr int contentItemHMargin = 4;        // margin between content items (e.g. text and icon)
static constexpr int contentHMargin = 2 * 3;        // margin between rounded border and content (= rounded border
                                                    // margin * 3)
static constexpr int pivotIndicatorPreferredWidth = 24; // Pivot_Grow / Slide / Stretch: fixed bar length, centered

static QColor segmentedColorFromVariant(const QVariant &v, const QColor &fallback)
{
    if (!v.isValid())
    {
        return fallback;
    }
    QColor c;
    if (v.canConvert<QColor>())
    {
        c = qvariant_cast<QColor>(v);
    }
    if (!c.isValid())
    {
        c = QColor(v.toString());
    }
    return c.isValid() ? c : fallback;
}

static QColor segmentedThemeColorProperty(const QWidget *widget, bool darkTheme, const char *lightName, const char *darkName, const QColor &fallback)
{
    if (!widget)
    {
        return fallback;
    }
    const QVariant v = widget->property(darkTheme ? darkName : lightName);
    return segmentedColorFromVariant(v, fallback);
}

static bool segmentedBoolProperty(const QWidget *widget, const char *name, bool fallback)
{
    if (!widget)
    {
        return fallback;
    }
    const QVariant v = widget->property(name);
    return v.isValid() ? v.toBool() : fallback;
}

/// 将半透明的 Fluent 颜色与底色预合成，返回完全不透明的颜色。
/// 用于 input 控件作为 delegate editor 时，防止半透明背景透底。
static QColor resolveOpaque(const QColor &fluentColor, const QColor &base)
{
    if (fluentColor.alpha() == 255)
        return fluentColor;
    const double a = fluentColor.alphaF();
    return QColor(qRound(base.red() * (1.0 - a) + fluentColor.red() * a),
                  qRound(base.green() * (1.0 - a) + fluentColor.green() * a),
                  qRound(base.blue() * (1.0 - a) + fluentColor.blue() * a));
}

static QColor opaqueBlendBase(const QPalette &palette, bool darkTheme)
{
    QColor base = palette.base().color();
    if (base.alpha() == 0)
        base = palette.window().color();
    if (base.alpha() == 0)
        base = darkTheme ? QColor(0x1E, 0x1E, 0x1E) : QColor(0xFF, 0xFF, 0xFF);
    return base;
}

static constexpr int menuItemVMargin = 3; // vertical margin for menu items
static constexpr int menuItemHMargin = 3; // horizontal margin for menu items

static constexpr int cBShadowBorderWidth = 2;
static constexpr int cBRoundingRadius = 4;

static constexpr int ProgressBarThickness = 4;
static constexpr int NavigationSettingsSpinRole = Qt::UserRole + 1001;
static constexpr int NavigationIconRole = Qt::UserRole + 1;
QStyleAnimation *getAnimationEx(QObject *target, const QByteArray &key);
void startAnimationEx(QStyleAnimation *animation, QObject *target, const QByteArray &key);

static QByteArray navigationSettingsAnimKey(const QTreeView *treeView, const QModelIndex &index)
{
    if (!treeView || !index.isValid())
    {
        return QByteArray();
    }
    QByteArray key = QByteArrayLiteral("_q_nav_settings_spin_");
    QModelIndex current = index;
    while (current.isValid())
    {
        key.append(QByteArray::number(current.row()));
        key.append('/');
        current = current.parent();
    }
    return key;
}

static QPixmap navigationGlyphPixmapCached(const QString &iconCode, int glyphSide, const QColor &color)
{
    static QHash<QString, QPixmap> cache;
    const QString cacheKey =
        iconCode + QLatin1Char('|') + QString::number(glyphSide) + QLatin1Char('|') + QString::number(color.rgba());
    auto it = cache.constFind(cacheKey);
    if (it != cache.constEnd())
    {
        return *it;
    }

    QPixmap glyphPixmap(glyphSide, glyphSide);
    glyphPixmap.fill(Qt::transparent);

    QPainter glyphPainter(&glyphPixmap);
    glyphPainter.setRenderHint(QPainter::Antialiasing, true);
    glyphPainter.setRenderHint(QPainter::TextAntialiasing, true);
    glyphPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QFont iconFont("Segoe Fluent Icons");
    iconFont.setPixelSize(qMax(12, qRound(glyphSide * 0.78)));
    iconFont.setHintingPreference(QFont::PreferNoHinting);
    glyphPainter.setFont(iconFont);
    glyphPainter.setPen(color);
    glyphPainter.drawText(glyphPixmap.rect(), Qt::AlignCenter, iconCode);

    if (cache.size() > 128)
    {
        cache.clear();
    }
    cache.insert(cacheKey, glyphPixmap);
    return glyphPixmap;
}

static void startNavigationSettingsSpin(QObject *target,
                                        const QTreeView *treeView,
                                        const QModelIndex &index,
                                        qreal deltaDegrees,
                                        int durationMs)
{
    if (!target || !treeView || !index.isValid() || durationMs <= 0)
    {
        return;
    }

    const QByteArray animKey = navigationSettingsAnimKey(treeView, index);
    qreal start = 0.0;
    if (QNumberStyleAnimation *existAnim = qobject_cast<QNumberStyleAnimation *>(getAnimationEx(target, animKey)))
    {
        start = existAnim->currentValue();
    }

    QNumberStyleAnimation *t = new QNumberStyleAnimation(target);
    t->setStartValue(start);
    t->setEndValue(start + deltaDegrees);
    t->setDuration(durationMs);
    t->setFrameRate(QStyleAnimation::DefaultFps);
    startAnimationEx(t, target, animKey);
}

enum
{
    windowsItemFrame = 2,      // menu item frame width
    windowsSepHeight = 9,      // separator item height
    windowsItemHMargin = 3,    // menu item hor text margin
    windowsItemVMargin = 2,    // menu item ver text margin
    windowsArrowHMargin = 6,   // arrow horizontal margin
    windowsRightBorder = 15,   // right border on windows
    windowsCheckMarkWidth = 12 // checkmarks width on windows
};

#define AcceptMedium QChar(0xF78C)
#define Dash12 QChar(0xE629)
#define CheckMark QChar(0xE73E)

// #define CaretLeftSolid8 QChar(0xEDD9)
// #define CaretRightSolid8 QChar(0xEDDA)
// #define CaretUpSolid8 QChar(0xEDDB)
// #define CaretDownSolid8 QChar(0xEDDC)
#define CaretLeftSolid8 QChar(0xF08D)
#define CaretRightSolid8 QChar(0xF08F)
#define CaretUpSolid8 QChar(0xF090)
#define CaretDownSolid8 QChar(0xF08E)

#define ChevronDown QChar(0xE70D)
#define ChevronUp QChar(0xE70E)

#define Add QChar(0xE710)
#define Remove QChar(0xE738)

#define ChevronDownMed QChar(0xE972)
#define ChevronLeftMed QChar(0xE973)
#define ChevronRightMed QChar(0xE974)

#define ChevronUpSmall QChar(0xE96D)
#define ChevronDownSmall QChar(0xE96E)

#define ChromeMinimize QChar(0xE921)
#define ChromeMaximize QChar(0xE922)
#define ChromeRestore QChar(0xE923)
#define ChromeClose QChar(0xE8BB)

#define Help QChar(0xE897)
#define InfoMessage QChar(0xE946)
#define WarningMessage QChar(0xE7BA)
#define CriticalMessage QChar(0xEA39)

template <typename R, typename P, typename B>
static inline void drawRoundedRect(QPainter *p, R &&rect, P &&pen, B &&brush)
{
    p->setPen(pen);
    p->setBrush(brush);
    p->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
}

// From QCommonStylePrivate::tabLayout
static inline void tabLayout(const QStyle *proxyStyle,
                             const QStyleOptionTab *opt,
                             const QWidget *widget,
                             QRect *textRect,
                             QRect *iconRect)
{
    Q_ASSERT(textRect);
    Q_ASSERT(iconRect);
    QRect tr = opt->rect;
    const bool isNavigationStyle = widget && widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Navigation;
    const bool isVerticalNavigation = isNavigationStyle && (opt->shape == QTabBar::RoundedEast || opt->shape == QTabBar::RoundedWest || opt->shape == QTabBar::TriangularEast || opt->shape == QTabBar::TriangularWest);
    bool verticalTabs = opt->shape == QTabBar::RoundedEast || opt->shape == QTabBar::RoundedWest || opt->shape == QTabBar::TriangularEast || opt->shape == QTabBar::TriangularWest;
    if (isVerticalNavigation)
    {
        constexpr int indicatorWidth = 3;
        constexpr int indicatorContentGap = 2;
        constexpr int contentLeadingInset = indicatorWidth + indicatorContentGap;
        constexpr int contentTrailingInset = 8;
        constexpr int listItemVerticalMargin = 4;
        constexpr int iconTextSpacing = 8;

        tr.adjust(contentLeadingInset, listItemVerticalMargin, -contentTrailingInset, -listItemVerticalMargin);

        if (!opt->leftButtonSize.isEmpty())
        {
            tr.setLeft(tr.left() + 4 + opt->leftButtonSize.width());
        }
        if (!opt->rightButtonSize.isEmpty())
        {
            tr.setRight(tr.right() - 4 - opt->rightButtonSize.width());
        }

        if (!opt->icon.isNull())
        {
            QSize iconSize = opt->iconSize;
            if (!iconSize.isValid())
            {
                const int iconExtent = proxyStyle->pixelMetric(QStyle::PM_SmallIconSize, opt, widget);
                iconSize = QSize(iconExtent, iconExtent);
            }
            QSize tabIconSize = opt->icon.actualSize(iconSize,
                                                     (opt->state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled,
                                                     (opt->state & QStyle::State_Selected) ? QIcon::On : QIcon::Off);
            tabIconSize = QSize(qMin(tabIconSize.width(), iconSize.width()), qMin(tabIconSize.height(), iconSize.height()));

            const int offsetX = (iconSize.width() - tabIconSize.width()) / 2;
            *iconRect = QRect(tr.left() + offsetX, tr.center().y() - tabIconSize.height() / 2, tabIconSize.width(), tabIconSize.height());
            *iconRect = QStyle::visualRect(opt->direction, opt->rect, *iconRect);
            tr.setLeft(tr.left() + tabIconSize.width() + iconTextSpacing);
        }
        else
        {
            int cachedIconWidth = widget ? widget->property("_q_navigation_widest_icon_width").toInt() : 0;
            if (cachedIconWidth > 0)
            {
                tr.setLeft(tr.left() + cachedIconWidth + iconTextSpacing);
            }
            else
            {
                tr.setLeft(tr.left() + iconTextSpacing);
            }
        }

        *textRect = QStyle::visualRect(opt->direction, opt->rect, tr);
        return;
    }

    if (verticalTabs)
    {
        tr.setRect(0, 0, tr.height(), tr.width()); // 0, 0 as we will have a translate transform
    }

    int verticalShift = proxyStyle->pixelMetric(QStyle::PM_TabBarTabShiftVertical, opt, widget);
    int horizontalShift = proxyStyle->pixelMetric(QStyle::PM_TabBarTabShiftHorizontal, opt, widget);
    int hpadding = proxyStyle->pixelMetric(QStyle::PM_TabBarTabHSpace, opt, widget) / 2;
    int vpadding = proxyStyle->pixelMetric(QStyle::PM_TabBarTabVSpace, opt, widget) / 2;
    if (opt->shape == QTabBar::RoundedSouth || opt->shape == QTabBar::TriangularSouth)
    {
        verticalShift = -verticalShift;
    }
    tr.adjust(hpadding, verticalShift + vpadding, horizontalShift - hpadding, -vpadding);
    bool selected = opt->state & QStyle::State_Selected;
    if (selected)
    {
        tr.setTop(tr.top() - verticalShift);
        tr.setRight(tr.right() - horizontalShift);
    }

    // left widget
    if (!opt->leftButtonSize.isEmpty())
    {
        tr.setLeft(tr.left() + 4 + (verticalTabs ? opt->leftButtonSize.height() : opt->leftButtonSize.width()));
    }
    // right widget
    if (!opt->rightButtonSize.isEmpty())
    {
        tr.setRight(tr.right() - 4 - (verticalTabs ? opt->rightButtonSize.height() : opt->rightButtonSize.width()));
    }

    // icon
    if (!opt->icon.isNull())
    {
        QSize iconSize = opt->iconSize;
        if (!iconSize.isValid())
        {
            int iconExtent = proxyStyle->pixelMetric(QStyle::PM_SmallIconSize, opt, widget);
            iconSize = QSize(iconExtent, iconExtent);
        }
        QSize tabIconSize = opt->icon.actualSize(iconSize,
                                                 (opt->state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled,
                                                 (opt->state & QStyle::State_Selected) ? QIcon::On : QIcon::Off);
        // High-dpi icons do not need adjustment; make sure tabIconSize is not larger than iconSize
        tabIconSize = QSize(qMin(tabIconSize.width(), iconSize.width()), qMin(tabIconSize.height(), iconSize.height()));

        const int offsetX = (iconSize.width() - tabIconSize.width()) / 2;
        *iconRect = QRect(tr.left() + offsetX, tr.center().y() - tabIconSize.height() / 2, tabIconSize.width(), tabIconSize.height());
        if (!verticalTabs)
        {
            *iconRect = QStyle::visualRect(opt->direction, opt->rect, *iconRect);
        }
        tr.setLeft(tr.left() + tabIconSize.width() + 4);
    }

    if (!verticalTabs)
    {
        tr = QStyle::visualRect(opt->direction, opt->rect, tr);
    }

    *textRect = tr;
}

static inline QPainterPath buildRoundedPolyline(const QList<QPointF> &points, qreal radius)
{
    QPainterPath path;

    if (points.size() < 2)
    {
        return path;
    }

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

        if (QVector2D::dotProduct(v1n, v2n) > 0.999)
        {
            path.lineTo(p1);
            continue;
        }

        qreal maxAllowedRadius = std::min(v1.length(), v2.length()) / 2;
        qreal r = std::min(radius, maxAllowedRadius);

        QPointF p1_before = p1 - r * v1n.toPointF();
        QPointF p1_after = p1 + r * v2n.toPointF();

        path.lineTo(p1_before);
        path.quadTo(p1, p1_after);
    }

    path.lineTo(points.last());

    return path;
}

static qreal radioButtonInnerRadius(int state, const QStyleOption *option, const QWidget *widget, int indicatorSize)
{
    Q_UNUSED(widget)
    Q_UNUSED(option)

    qreal outerRadius = indicatorSize / 2.0;
    qreal innerRadius = outerRadius / 2.0;

    if (state & QStyle::State_Sunken)
    {
        innerRadius = outerRadius / 2.5;
    }
    else if (state & QStyle::State_MouseOver && !(state & QStyle::State_On))
    {
        innerRadius = 0.0;
    }
    else if (state & QStyle::State_MouseOver && (state & QStyle::State_On))
    {
        innerRadius = outerRadius / 1.8;
    }
    else if (state & QStyle::State_On)
    {
        innerRadius = outerRadius / 2.0;
    }
    else
    {
        // 未选中状态，不显示内圆
        innerRadius = 0.0;
    }

    return innerRadius;
}

static qreal sliderInnerRadius(QStyle::State state, bool insideHandle)
{
    if (state & QStyle::State_Sunken)
    {
        return 0.40;
    }
    else if (insideHandle)
    {
        return 0.65;
    }
    return 0.55;
}

namespace StyleOptionHelper
{
    inline bool isChecked(const QStyleOption *option)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        return option->state.testAnyFlags(QStyle::State_On | QStyle::State_NoChange);
#endif
        return option->state & (QStyle::State_On | QStyle::State_NoChange);
    }

    inline bool isDisabled(const QStyleOption *option)
    {
        return !option->state.testFlag(QStyle::State_Enabled);
    }

    inline bool isPressed(const QStyleOption *option)
    {
        return option->state.testFlag(QStyle::State_Sunken);
    }

    inline bool isHover(const QStyleOption *option)
    {
        return option->state.testFlag(QStyle::State_MouseOver);
    }

    inline bool isAutoRaise(const QStyleOption *option)
    {
        return option->state.testFlag(QStyle::State_AutoRaise);
    }
    enum class ControlState
    {
        Normal,
        Hover,
        Pressed,
        Disabled
    };

    inline ControlState calcControlState(const QStyleOption *option)
    {
        if (isDisabled(option))
        {
            return ControlState::Disabled;
        }
        if (isPressed(option))
        {
            return ControlState::Pressed;
        }
        if (isHover(option))
        {
            return ControlState::Hover;
        }
        return ControlState::Normal;
    };

} // namespace StyleOptionHelper

static QSizeF viewItemTextLayout(QTextLayout &textLayout, int lineWidth, int maxHeight = -1, int *lastVisibleLine = nullptr)
{
    if (lastVisibleLine)
    {
        *lastVisibleLine = -1;
    }
    qreal height = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    int i = 0;
    while (true)
    {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
        {
            break;
        }
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
        // we assume that the height of the next line is the same as the current
        // one
        if (maxHeight > 0 && lastVisibleLine && height + line.height() > maxHeight)
        {
            const QTextLine nextLine = textLayout.createLine();
            *lastVisibleLine = nextLine.isValid() ? i : -1;
            break;
        }
        ++i;
    }
    textLayout.endLayout();
    return QSizeF(widthUsed, height);
}

#if QT_CONFIG(toolbutton)

QString calculateElidedText(const QString &text,
                            const QTextOption &textOption,
                            const QFont &font,
                            const QRect &textRect,
                            const Qt::Alignment valign,
                            Qt::TextElideMode textElideMode,
                            int flags,
                            bool lastVisibleLineShouldBeElided,
                            QPointF *paintStartPosition)
{
    Q_UNUSED(flags)

    QTextLayout textLayout(text, font);
    textLayout.setTextOption(textOption);

    // In AlignVCenter mode when more than one line is displayed and the height
    // only allows some of the lines it makes no sense to display those. From a
    // users perspective it makes more sense to see the start of the text
    // instead something inbetween.
    const bool vAlignmentOptimization = paintStartPosition && valign.testFlag(Qt::AlignVCenter);

    int lastVisibleLine = -1;
    viewItemTextLayout(textLayout, textRect.width(), vAlignmentOptimization ? textRect.height() : -1, &lastVisibleLine);

    const QRectF boundingRect = textLayout.boundingRect();
    // don't care about LTR/RTL here, only need the height
    const QRect layoutRect = QStyle::alignedRect(Qt::LayoutDirectionAuto, valign, boundingRect.size().toSize(), textRect);

    if (paintStartPosition)
    {
        *paintStartPosition = QPointF(textRect.x(), layoutRect.top());
    }

    QString ret;
    qreal height = 0;
    const int lineCount = textLayout.lineCount();
    for (int i = 0; i < lineCount; ++i)
    {
        const QTextLine line = textLayout.lineAt(i);
        height += line.height();

        // above visible rect
        if (height + layoutRect.top() <= textRect.top())
        {
            if (paintStartPosition)
            {
                paintStartPosition->ry() += line.height();
            }
            continue;
        }

        const int start = line.textStart();
        const int length = line.textLength();
        const bool drawElided = line.naturalTextWidth() > textRect.width();
        bool elideLastVisibleLine = lastVisibleLine == i;
        if (!drawElided && i + 1 < lineCount && lastVisibleLineShouldBeElided)
        {
            const QTextLine nextLine = textLayout.lineAt(i + 1);
            const int nextHeight = height + nextLine.height() / 2;
            // elide when less than the next half line is visible
            if (nextHeight + layoutRect.top() > textRect.height() + textRect.top())
            {
                elideLastVisibleLine = true;
            }
        }

        QString text = textLayout.text().mid(start, length);
        if (drawElided || elideLastVisibleLine)
        {
            if (elideLastVisibleLine)
            {
                if (text.endsWith(QChar::LineSeparator))
                {
                    text.chop(1);
                }
                text += QChar(0x2026);
            }
            // Q_DECL_UNINITIALIZED const QStackTextEngine engine(text, font);
            // ret += engine.elidedText(textElideMode, textRect.width(), flags);
            QFontMetrics fm(font);
            ret += fm.elidedText(text, textElideMode, textRect.width());

            // no newline for the last line (last visible or real)
            // sometimes drawElided is true but no eliding is done so the text
            // ends with QChar::LineSeparator - don't add another one. This
            // happened with arabic text in the testcase for QTBUG-72805
            if (i < lineCount - 1 && !ret.endsWith(QChar::LineSeparator))
            {
                ret += QChar::LineSeparator;
            }
        }
        else
        {
            ret += text;
        }

        // below visible text, can stop
        if ((height + layoutRect.top() >= textRect.bottom()) || (lastVisibleLine >= 0 && lastVisibleLine == i))
        {
            break;
        }
    }
    return ret;
}

void viewItemDrawText(QPainter *p, const QStyleOptionViewItem *option, const QRect &rect)
{
    const QWidget *widget = option->widget;
    const int textMargin = qApp->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, widget) + 1;

    QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
    const bool wrapText = option->features & QStyleOptionViewItem::WrapText;
    QTextOption textOption;
    textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
    textOption.setTextDirection(option->direction);
    textOption.setAlignment(QStyle::visualAlignment(option->direction, option->displayAlignment));

    QPointF paintPosition;
    const QString newText = calculateElidedText(
        option->text, textOption, option->font, textRect, option->displayAlignment, option->textElideMode, 0, true, &paintPosition);

    QTextLayout textLayout(newText, option->font);
    textLayout.setTextOption(textOption);
    viewItemTextLayout(textLayout, textRect.width());
    textLayout.draw(p, paintPosition);
}

QString toolButtonElideText(const QStyleOptionToolButton *option, const QRect &textRect, int flags)
{
    if (option->fontMetrics.horizontalAdvance(option->text) <= textRect.width())
    {
        return option->text;
    }

    QString text = option->text;
    text.replace(u'\n', QChar::LineSeparator);
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::ManualWrap);
    textOption.setTextDirection(option->direction);

    return calculateElidedText(text, textOption, option->font, textRect, Qt::AlignTop, Qt::ElideMiddle, flags, false, nullptr);
}

static void drawArrow(const QStyle *style,
                      const QStyleOptionToolButton *toolbutton,
                      const QRect &rect,
                      QPainter *painter,
                      const QWidget *widget = nullptr)
{
    QStyle::PrimitiveElement pe;
    switch (toolbutton->arrowType)
    {
    case Qt::LeftArrow:
        pe = QStyle::PE_IndicatorArrowLeft;
        break;
    case Qt::RightArrow:
        pe = QStyle::PE_IndicatorArrowRight;
        break;
    case Qt::UpArrow:
        pe = QStyle::PE_IndicatorArrowUp;
        break;
    case Qt::DownArrow:
        pe = QStyle::PE_IndicatorArrowDown;
        break;
    default:
        return;
    }
    QStyleOption arrowOpt = *toolbutton;
    arrowOpt.rect = rect;
    style->drawPrimitive(pe, &arrowOpt, painter, widget);
}

#endif // QT_CONFIG(toolbutton)

QColor blend(const QColor &fg, const QColor &bg, double alpha);

//------------------单动画-------------------------------//
QHash<QObject *, QStyleAnimation *> animations;

template <typename K, typename V>
V takeValue(QHash<K, V> &h, K key)
{
    V val = h.value(key);
    h.remove(key);
    return val;
}

void removeAnimation(QObject *target)
{
    animations.remove(target);
}

void stopAnimation(QObject *target)
{
    QStyleAnimation *animation = takeValue(animations, target);
    if (animation)
    {
        animation->stop();
        delete animation;
    }
}

void startAnimation(QStyleAnimation *animation)
{
    const auto target = animation->target();
    stopAnimation(target);
    QObject::connect(animation, &QStyleAnimation::destroyed, [target]()
                     { removeAnimation(target); });
    animations.insert(target, animation);
    animation->start();
}

QStyleAnimation *getAnimation(QObject *target)
{
    return animations.value(target);
}

//--------------------------------------------------//

//--------------------多动画------------------------------//
QHash<QObject *, QHash<QByteArray, QPointer<QStyleAnimation>>> extraAnimations;

QStyleAnimation *getAnimationEx(QObject *target, const QByteArray &key)
{
    if (!target)
    {
        return nullptr;
    }

    auto it = extraAnimations.find(target);
    if (it == extraAnimations.end())
    {
        return nullptr;
    }

    return it->value(key, nullptr);
}

void clearAnimationsEx(QObject *target)
{
    if (!target)
    {
        return;
    }

    auto it = extraAnimations.find(target);
    if (it == extraAnimations.end())
    {
        return;
    }

    const auto animationsForTarget = it.value();
    extraAnimations.erase(it);

    for (auto animation : animationsForTarget)
    {
        if (animation)
        {
            animation->stop();
            animation->deleteLater();
        }
    }
}

void removeAnimationEx(QObject *target, const QByteArray &key, QStyleAnimation *animation)
{
    auto it = extraAnimations.find(target);
    if (it == extraAnimations.end())
    {
        return;
    }

    const auto current = it->value(key);
    if (current != animation)
    {
        return;
    }

    it->remove(key);

    if (it->isEmpty())
    {
        extraAnimations.erase(it);
    }
}

void stopAnimationEx(QObject *target, const QByteArray &key)
{
    if (!target)
    {
        return;
    }

    auto it = extraAnimations.find(target);
    if (it == extraAnimations.end())
    {
        return;
    }

    QPointer<QStyleAnimation> animation = it->take(key);
    if (animation)
    {
        animation->stop();
        animation->deleteLater();
    }

    if (it->isEmpty())
    {
        extraAnimations.erase(it);
    }
}

void startAnimationEx(QStyleAnimation *animation, QObject *target, const QByteArray &key)
{
    if (!animation || !target)
    {
        return;
    }

    if (!extraAnimations.contains(target))
    {
        QObject::connect(target, &QObject::destroyed, [](QObject *obj)
                         { clearAnimationsEx(obj); });
    }

    // 停掉同 key 的旧动画
    stopAnimationEx(target, key);

    QObject::connect(animation, &QObject::destroyed, [target, key, animation]()
                     { removeAnimationEx(target, key, animation); });

    extraAnimations[target].insert(key, animation);
    animation->start();
}

float animationValue(QObject *target, const QByteArray &key, float defaultValue)
{
    auto anim = qobject_cast<QNumberStyleAnimation *>(getAnimationEx(target, key));

    return anim ? anim->currentValue() : defaultValue;
}

//--------------------------------------------------//

bool transitionsEnabled()
{
    return true;
}

class PainterStateGuard
{
public:
    PainterStateGuard(QPainter *painter)
    {
        m_painter = painter;
        m_painter->save();
    }

    ~PainterStateGuard() { m_painter->restore(); }

private:
    QPainter *m_painter;
};

inline void
qDrawPlainRoundedRect(QPainter *p, int x, int y, int w, int h, qreal rx, qreal ry, const QColor &c, int lineWidth, const QBrush *fill)
{
    if (w == 0 || h == 0)
    {
        return;
    }
    if (Q_UNLIKELY(w < 0 || h < 0 || lineWidth < 0))
    {
        qWarning("qDrawPlainRect: Invalid parameters");
        return;
    }

    PainterStateGuard painterGuard(p);
    if (lineWidth == 0 && !fill)
    {
        return;
    }
    if (lineWidth > 0)
    {
        p->setPen(QPen(c, lineWidth));
    }
    p->setBrush(fill ? *fill : Qt::NoBrush);
    const QRectF r(x, y, w, h);
    const auto lw2 = lineWidth / 2.;
    const QRectF rect = r.marginsRemoved(QMarginsF(lw2, lw2, lw2, lw2));
    p->drawRoundedRect(rect, rx, ry);
}

inline void qDrawPlainRoundedRect(QPainter *painter,
                                  const QRect &rect,
                                  qreal rx,
                                  qreal ry,
                                  const QColor &lineColor,
                                  int lineWidth = 1,
                                  const QBrush *fill = nullptr)
{
    qDrawPlainRoundedRect(painter, rect.x(), rect.y(), rect.width(), rect.height(), rx, ry, lineColor, lineWidth, fill);
}

void qDrawShadeRect(QPainter *p,
                    int x,
                    int y,
                    int w,
                    int h,
                    const QPalette &pal,
                    bool sunken = false,
                    int lineWidth = 1,
                    int midLineWidth = 0,
                    const QBrush *fill = nullptr)
{
    if (w == 0 || h == 0)
    {
        return;
    }
    if (Q_UNLIKELY(w < 0 || h < 0 || lineWidth < 0 || midLineWidth < 0))
    {
        qWarning("qDrawShadeRect: Invalid parameters");
        return;
    }

    PainterStateGuard painterGuard(p);
    const qreal devicePixelRatio = QStyleHelper::getDpr(p);
    if (!qFuzzyCompare(devicePixelRatio, qreal(1)))
    {
        const qreal inverseScale = qreal(1) / devicePixelRatio;
        p->scale(inverseScale, inverseScale);
        x = qRound(devicePixelRatio * x);
        y = qRound(devicePixelRatio * y);
        w = devicePixelRatio * w;
        h = devicePixelRatio * h;
        lineWidth = qRound(devicePixelRatio * lineWidth);
        midLineWidth = qRound(devicePixelRatio * midLineWidth);
        p->translate(0.5, 0.5);
    }
    if (sunken)
    {
        p->setPen(pal.dark().color());
    }
    else
    {
        p->setPen(pal.light().color());
    }
    int x1 = x, y1 = y, x2 = x + w - 1, y2 = y + h - 1;

    if (lineWidth == 1 && midLineWidth == 0)
    { // standard shade rectangle
        p->drawRect(x1, y1, w - 2, h - 2);
        if (sunken)
        {
            p->setPen(pal.light().color());
        }
        else
        {
            p->setPen(pal.dark().color());
        }
        QLineF lines[4] = {QLineF(x1 + 1, y1 + 1, x2 - 2, y1 + 1),
                           QLineF(x1 + 1, y1 + 2, x1 + 1, y2 - 2),
                           QLineF(x1, y2, x2, y2),
                           QLineF(x2, y1, x2, y2 - 1)};
        p->drawLines(lines, 4); // draw bottom/right lines
    }
    else
    { // more complicated
        int m = lineWidth + midLineWidth;
        int i, j = 0, k = m;
        for (i = 0; i < lineWidth; i++)
        { // draw top shadow
            QLineF lines[4] = {QLineF(x1 + i, y2 - i, x1 + i, y1 + i),
                               QLineF(x1 + i, y1 + i, x2 - i, y1 + i),
                               QLineF(x1 + k, y2 - k, x2 - k, y2 - k),
                               QLineF(x2 - k, y2 - k, x2 - k, y1 + k)};
            p->drawLines(lines, 4);
            k++;
        }
        p->setPen(pal.mid().color());
        j = lineWidth * 2;
        for (i = 0; i < midLineWidth; i++)
        { // draw lines in the middle
            p->drawRect(x1 + lineWidth + i, y1 + lineWidth + i, w - j - 1, h - j - 1);
            j += 2;
        }
        if (sunken)
        {
            p->setPen(pal.light().color());
        }
        else
        {
            p->setPen(pal.dark().color());
        }
        k = m;
        for (i = 0; i < lineWidth; i++)
        { // draw bottom shadow
            QLineF lines[4] = {QLineF(x1 + 1 + i, y2 - i, x2 - i, y2 - i),
                               QLineF(x2 - i, y2 - i, x2 - i, y1 + i + 1),
                               QLineF(x1 + k, y2 - k, x1 + k, y1 + k),
                               QLineF(x1 + k, y1 + k, x2 - k, y1 + k)};
            p->drawLines(lines, 4);
            k++;
        }
    }
    if (fill)
    {
        int tlw = lineWidth + midLineWidth;
        p->setPen(Qt::NoPen);
        p->setBrush(*fill);
        p->drawRect(x + tlw, y + tlw, w - 2 * tlw, h - 2 * tlw);
    }
}

void qDrawShadeRect(QPainter *p,
                    const QRect &r,
                    const QPalette &pal,
                    bool sunken = false,
                    int lineWidth = 1,
                    int midLineWidth = 0,
                    const QBrush *fill = nullptr)
{
    qDrawShadeRect(p, r.x(), r.y(), r.width(), r.height(), pal, sunken, lineWidth, midLineWidth, fill);
}

void qDrawShadePanel(QPainter *p,
                     int x,
                     int y,
                     int w,
                     int h,
                     const QPalette &pal,
                     bool sunken = false,
                     int lineWidth = 1,
                     const QBrush *fill = nullptr)
{
    if (w == 0 || h == 0)
    {
        return;
    }
    if (Q_UNLIKELY(w < 0 || h < 0 || lineWidth < 0))
    {
        qWarning("qDrawShadePanel: Invalid parameters");
    }

    PainterStateGuard painterGuard(p);
    const qreal devicePixelRatio = QStyleHelper::getDpr(p);
    bool isTranslated = false;
    if (!qFuzzyCompare(devicePixelRatio, qreal(1)))
    {
        const qreal inverseScale = qreal(1) / devicePixelRatio;
        p->scale(inverseScale, inverseScale);
        x = qRound(devicePixelRatio * x);
        y = qRound(devicePixelRatio * y);
        w = devicePixelRatio * w;
        h = devicePixelRatio * h;
        lineWidth = qRound(devicePixelRatio * lineWidth);
        p->translate(0.5, 0.5);
        isTranslated = true;
    }

    QColor shade = pal.dark().color();
    QColor light = pal.light().color();
    if (fill)
    {
        if (fill->color() == shade)
        {
            shade = pal.shadow().color();
        }
        if (fill->color() == light)
        {
            light = pal.midlight().color();
        }
    }
    QVector<QLineF> lines;
    lines.reserve(2 * lineWidth);

    if (sunken)
    {
        p->setPen(shade);
    }
    else
    {
        p->setPen(light);
    }
    int x1, y1, x2, y2;
    int i;
    x1 = x;
    y1 = y2 = y;
    x2 = x + w - 2;
    for (i = 0; i < lineWidth; i++)
    { // top shadow
        lines << QLineF(x1, y1++, x2--, y2++);
    }
    x2 = x1;
    y1 = y + h - 2;
    for (i = 0; i < lineWidth; i++)
    { // left shado
        lines << QLineF(x1++, y1, x2++, y2--);
    }
    p->drawLines(lines);
    lines.clear();
    if (sunken)
    {
        p->setPen(light);
    }
    else
    {
        p->setPen(shade);
    }
    x1 = x;
    y1 = y2 = y + h - 1;
    x2 = x + w - 1;
    for (i = 0; i < lineWidth; i++)
    { // bottom shadow
        lines << QLineF(x1++, y1--, x2, y2--);
    }
    x1 = x2;
    y1 = y;
    y2 = y + h - lineWidth - 1;
    for (i = 0; i < lineWidth; i++)
    { // right shadow
        lines << QLineF(x1--, y1++, x2--, y2);
    }
    p->drawLines(lines);
    if (fill)
    { // fill with fill color
        if (isTranslated)
        {
            p->translate(-0.5, -0.5);
        }
        p->fillRect(x + lineWidth, y + lineWidth, w - lineWidth * 2, h - lineWidth * 2, *fill);
    }
}

void qDrawShadePanel(QPainter *p,
                     const QRect &r,
                     const QPalette &pal,
                     bool sunken = false,
                     int lineWidth = 1,
                     const QBrush *fill = nullptr)
{
    qDrawShadePanel(p, r.x(), r.y(), r.width(), r.height(), pal, sunken, lineWidth, fill);
}

static QScreen *screenOf(const QWidget *w)
{
    if (w)
    {
        if (auto screen = w->screen())
        {
            return screen;
        }
    }
    return QGuiApplication::primaryScreen();
}

// Calculate the overall scale factor to obtain Qt Device Independent
// Pixels from a native Windows size.
qreal nativeMetricScaleFactor(const QWidget *widget)
{
    if (!widget)
    {
        return 1.0;
    }

    QScreen *screen = screenOf(widget);
    return 1.0 / screen->devicePixelRatio();
}

static bool updateBrushOrigin_public(QPainter *painter, const QWidget *widget, const QBrush &brush)
{
    if (!widget)
    {
        return false;
    }

    if (brush.style() == Qt::NoBrush || brush.style() == Qt::SolidPattern)
    {
        return false;
    }

    auto viewport = qobject_cast<const QWidget *>(widget);
    if (!viewport)
    {
        return false;
    }

    auto scrollArea = qobject_cast<const QAbstractScrollArea *>(viewport->parentWidget());

    if (!scrollArea || scrollArea->viewport() != viewport)
    {
        return false;
    }

    const QPoint offset = scrollArea->horizontalScrollBar()
                              ? QPoint(scrollArea->horizontalScrollBar()->value(), scrollArea->verticalScrollBar()->value())
                              : QPoint();

    painter->setBrushOrigin(-offset);
    return true;
}

inline bool isWin11()
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", QSettings::NativeFormat);
    int buildNumber = settings.value("CurrentBuildNumber", 0).toInt();
    return buildNumber >= 22000; // Win11 从 22000 build 开始
#else
    return false;
#endif
}

inline int getColorSchemeIndex() // 0 = Light, 1 = Dark
{
#if !defined(FLUENT_USE_QT_STYLE)
    return static_cast<int>(fluentUIAppearance.theme());
#else
    const QVariant pColorScheme = qApp->property("_q_colorscheme");
    if (pColorScheme.isValid())
    {
        qDebug() << "[FluentUI3Style] Get _q_colorscheme:" << pColorScheme.toInt();
        return pColorScheme.toInt();
    }

    // 如果没有设置，则根据系统主题色返回（QStyleHints::colorScheme / Qt::ColorScheme 仅 Qt 6.5+）
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    qDebug() << "[FluentUI3Style] No set _q_colorscheme, use system theme color" << qApp->styleHints()->colorScheme();
    return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark ? 1 : 0;
#else
    return 0;
#endif

#endif
}

inline bool isHighContrastTheme()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Unknown;
#else
    // Qt5/Qt6.4 不支持 Unknown，统一返回 false
    return false;
#endif
}

FluentUI3Style::FluentUI3Style(QStyle *style)
    : QProxyStyle(style)
{
    static bool resourceInit = false;
    static int fontId = -1;
    if (!resourceInit)
    {
        Q_INIT_RESOURCE(resource);
        resourceInit = true;
        fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/resource/Segoe Fluent Icons.ttf"));
    }

    if (fontId != -1)
    {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty())
        {
            assetFont = QFont(families.first());
        }
    }

    if (assetFont.family().isEmpty())
    {
        qWarning() << "[FluentUI3Style] Failed to load Segoe Fluent Icons font from resource. Falling back to system font.";
        assetFont = QFont(QStringLiteral("Segoe Fluent Icons"));
    }
    assetFont.setStyleStrategy(QFont::NoFontMerging);

    highContrastTheme = isHighContrastTheme();
    colorSchemeIndex = getColorSchemeIndex();

#ifdef FLUENT_USE_QT_STYLE
    bool ok = false;
    int themeStyle = qApp->property("_q_themestyle").toInt(&ok);
    if (ok)
    {
        PaletteManager::instance().setThemeStyle((ThemeStyle)themeStyle);
    }
#endif
    qDebug() << "[FluentUI3Style] color scheme index:" << colorSchemeIndex;
}

FluentUI3Style::~FluentUI3Style()
{
    qDebug() << "[FluentUI3Style] destroyed";
}

void FluentUI3Style::drawComplexControl(ComplexControl control,
                                        const QStyleOptionComplex *option,
                                        QPainter *painter,
                                        const QWidget *widget) const
{
    const auto drawTitleBarButton = [&](ComplexControl control, SubControl sc, const QString &str)
    {
        using namespace StyleOptionHelper;
        const QRect buttonRect = proxy()->subControlRect(control, option, sc, widget);
        if (buttonRect.isValid())
        {
            const bool hover = option->activeSubControls == sc && isHover(option);
            if (hover)
            {
                painter->fillRect(buttonRect, winUI3Color(subtleHighlightColor));
            }
            painter->setPen(option->palette.color(QPalette::WindowText));
            painter->drawText(buttonRect, Qt::AlignCenter, str);
        }
    };
    const auto drawTitleBarCloseButton = [&](ComplexControl control, SubControl sc, const QString &str)
    {
        using namespace StyleOptionHelper;
        const QRect buttonRect = proxy()->subControlRect(control, option, sc, widget);
        if (buttonRect.isValid())
        {
            const auto state = (option->activeSubControls == sc) ? calcControlState(option) : ControlState::Normal;
            QPen pen;
            switch (state)
            {
            case ControlState::Hover:
                painter->fillRect(buttonRect, shellCaptionCloseFillColorPrimary);
                pen = shellCaptionCloseTextFillColorPrimary;
                break;
            case ControlState::Pressed:
                painter->fillRect(buttonRect, shellCaptionCloseFillColorSecondary);
                pen = shellCaptionCloseTextFillColorSecondary;
                break;
            case ControlState::Disabled:
            case ControlState::Normal:
                pen = option->palette.color(QPalette::WindowText);
                break;
            }
            painter->setPen(pen);
            painter->drawText(buttonRect, Qt::AlignCenter, str);
        }
    };

    State state = option->state;
    SubControls sub = option->subControls;
    State flags = option->state;
    if (widget && widget->testAttribute(Qt::WA_UnderMouse) && widget->isActiveWindow())
    {
        flags |= State_MouseOver;
    }

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    if (transitionsEnabled() && option->styleObject)
    {
        if (control == CC_Slider)
        {
            if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option))
            {
                QObject *styleObject = option->styleObject; // Can be widget or qquickitem

                QRectF thumbRect = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);
                const qreal outerRadius =
                    qMin(8.0, (slider->orientation == Qt::Horizontal ? thumbRect.height() / 2.0 : thumbRect.width() / 2.0) - 1);
                bool isInsideHandle = option->activeSubControls == SC_SliderHandle;

                bool oldIsInsideHandle = styleObject->property("_q_insidehandle").toBool();
                State oldState = State(styleObject->property("_q_stylestate").toInt());
                SubControls oldActiveControls = SubControls(styleObject->property("_q_stylecontrols").toInt());

                QRectF oldRect = styleObject->property("_q_stylerect").toRect();
                styleObject->setProperty("_q_insidehandle", isInsideHandle);
                styleObject->setProperty("_q_stylestate", int(state));
                styleObject->setProperty("_q_stylecontrols", int(option->activeSubControls));
                styleObject->setProperty("_q_stylerect", option->rect);
                if (option->styleObject->property("_q_end_radius").isNull())
                {
                    option->styleObject->setProperty("_q_end_radius", outerRadius * 0.43);
                }

                bool doTransition = (((state & State_Sunken) != (oldState & State_Sunken) || (oldIsInsideHandle != isInsideHandle) || (oldActiveControls != option->activeSubControls)) && state & State_Enabled);

                if (oldRect != option->rect)
                {
                    doTransition = false;
                    stopAnimation(styleObject);
                    styleObject->setProperty("_q_inner_radius", outerRadius * 0.43);
                }

                if (doTransition)
                {
                    QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                    t->setStartValue(styleObject->property("_q_inner_radius").toFloat());
                    t->setEndValue(outerRadius * sliderInnerRadius(state, isInsideHandle));
                    styleObject->setProperty("_q_end_radius", t->endValue());
                    t->setDuration(150);
                    startAnimation(t);
                }
            }
        }
        if (control == CC_ToolButton)
        {
            const QStyleOptionToolButton *tb = qstyleoption_cast<const QStyleOptionToolButton *>(option);

            if (!tb)
            {
                return;
            }

            QObject *obj = option->styleObject;
            int state = option->state;
            int oldState = obj->property("_q_stylestate").toInt();
            obj->setProperty("_q_stylestate", state);

            bool doTransition = (state & State_Sunken) != (oldState & State_Sunken) || (state & State_On) != (oldState & State_On);
            if (doTransition && (state & State_Enabled) && (tb->features & QStyleOptionToolButton::HasMenu))
            {
                QNumberStyleAnimation *t = new QNumberStyleAnimation(obj);
                // t->setEasingCurve( QEasingCurve::InOutSine );
                qreal start = (state & State_Sunken) ? 0 : 180;
                qreal end = (state & State_Sunken) ? 180.0 : 0.0;
                t->setStartValue(start);
                t->setEndValue(end);
                t->setDuration(180);
                t->setFrameRate(QStyleAnimation::DefaultFps);
                startAnimation(t);
            }
        }
        if (control == CC_ComboBox)
        {
            const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option);

            if (!cb)
            {
                return;
            }

            QObject *obj = option->styleObject;
            int state = option->state;
            int oldState = obj->property("_q_stylestate").toInt();
            obj->setProperty("_q_stylestate", state);

            bool doTransition = (state & State_On) != (oldState & State_On);
            if (doTransition && (state & State_Enabled))
            {
                QNumberStyleAnimation *t = new QNumberStyleAnimation(obj);
                // t->setEasingCurve( QEasingCurve::InOutSine );
                qreal start = (state & State_On) ? 0 : 180;
                qreal end = (state & State_On) ? 180.0 : 0.0;
                t->setStartValue(start);
                t->setEndValue(end);
                t->setFrameRate(QStyleAnimation::DefaultFps);
                t->setDuration(180);
                startAnimation(t);
            }
        }
    }

    switch (control)
    {
#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(option))
        {
            SpinBoxButtonLayout buttonLayout =
                widget ? static_cast<SpinBoxButtonLayout>(widget->property("spinBoxButtonLayout").toInt())
                       : SpinBoxButtonLayout::ArrowsVertical;

            QCachedPainter cp(
                painter,
                QLatin1String("win11_spinbox") % HexString<uint8_t>(colorSchemeIndex) % HexString<uint8_t>(buttonLayout),
                sb,
                sb->rect.size());
            if (cp.needsPainting())
            {
                const auto frameRect = QRectF(option->rect).marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));
                if (sb->frame && (sub & SC_SpinBoxFrame))
                {
                    QStyleOptionFrame panel;
                    panel.QStyleOption::operator=(*option);
                    panel.rect = option->rect;
                    panel.lineWidth = 1;
                    panel.midLineWidth = 0;

                    const auto fillColor = [&]() -> WINUI3Color
                    {
                        if (state & State_HasFocus)
                            return fillControlInputActive;
                        if (state & State_MouseOver)
                            return fillControlSecondary;
                        return fillControlDefault;
                    }();
                    const QColor bg = resolveOpaque(winUI3Color(fillColor), sb->palette.base().color());
                    drawRoundedRect(cp.painter(), frameRect, Qt::NoPen, bg);

                    proxy()->drawPrimitive(PE_FrameLineEdit, &panel, cp.painter(), widget);
                }

                const bool isMouseOver = state & State_MouseOver;
                const bool hasFocus = state & State_HasFocus;
                if (isMouseOver && !hasFocus && !highContrastTheme)
                {
                    drawRoundedRect(cp.painter(), frameRect, Qt::NoPen, winUI3Color(subtleHighlightColor));
                }

                const auto drawUpDown = [&](QStyle::SubControl sc)
                {
                    const bool isUp = sc == SC_SpinBoxUp;
                    const QRect rect = proxy()->subControlRect(CC_SpinBox, option, sc, widget);
                    if (sb->activeSubControls & sc)
                    {
                        drawRoundedRect(cp.painter(), rect.adjusted(1, 1, -1, -2), Qt::NoPen, winUI3Color(subtleHighlightColor));
                    }

                    bool horizonal = buttonLayout != SpinBoxButtonLayout::ArrowsVertical;
                    QString c = isUp ? ChevronUp : ChevronDown;
                    if (buttonLayout == SpinBoxButtonLayout::PlusMinusHorizontalSides)
                    {
                        c = isUp ? Add : Remove;
                    }

                    auto _font = assetFont;
                    _font.setPixelSize(horizonal ? 17 : 15);
                    cp->setFont(_font);
                    cp->setPen(sb->palette.buttonText().color());
                    cp->setBrush(Qt::NoBrush);
                    cp->drawText(rect, Qt::AlignCenter, c);
                };
                if (sub & SC_SpinBoxUp)
                {
                    drawUpDown(SC_SpinBoxUp);
                }
                if (sub & SC_SpinBoxDown)
                {
                    drawUpDown(SC_SpinBoxDown);
                }
                if (state & State_KeyboardFocusChange && state & State_HasFocus)
                {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*option);
                    proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, cp.painter(), widget);
                }
            }
        }
        break;
#endif // QT_CONFIG(spinbox)
#if QT_CONFIG(slider)
    case CC_Slider:
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option))
        {
            const auto &slrect = slider->rect;
            const bool isHorizontal = slider->orientation == Qt::Horizontal;
            const QRectF handleRect(proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget));
            const QPointF handleCenter(handleRect.center());

            if (sub & SC_SliderGroove)
            {
                QRectF rect = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
                QRectF leftRect;
                QRectF rightRect;

                if (isHorizontal)
                {
                    rect = QRectF(rect.left() + 2, rect.center().y() - 2, rect.width() - 2, 4);
                    leftRect = QRectF(rect.left(), rect.top(), handleCenter.x() - rect.left(), rect.height());
                    rightRect = QRectF(handleCenter.x(), rect.top(), rect.width() - handleCenter.x(), rect.height());
                }
                else
                {
                    rect = QRect(rect.center().x() - 2, rect.top() + 2, 4, rect.height() - 2);
                    leftRect = QRectF(rect.left(), rect.top(), rect.width(), handleCenter.y() - rect.top());
                    rightRect = QRectF(rect.left(), handleCenter.y(), rect.width(), rect.height() - handleCenter.y());
                }
                if (slider->upsideDown)
                {
                    qSwap(leftRect, rightRect);
                }

                painter->setPen(Qt::NoPen);
                painter->setBrush(calculateAccentColor(option));
                painter->drawRoundedRect(leftRect, 2, 2);
                painter->setBrush(WINUI3Colors[colorSchemeIndex][controlStrongFill]);
                painter->drawRoundedRect(rightRect, 2, 2);
            }
            if (sub & SC_SliderTickmarks)
            {
                int tickOffset = proxy()->pixelMetric(PM_SliderTickmarkOffset, slider, widget);
                int ticks = slider->tickPosition;
                int thickness = proxy()->pixelMetric(PM_SliderControlThickness, slider, widget);
                int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
                int available = proxy()->pixelMetric(PM_SliderSpaceAvailable, slider, widget);
                int interval = slider->tickInterval;
                if (interval <= 0)
                {
                    interval = slider->singleStep;
                    if (QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, interval, available) - QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, 0, available) < 3)
                    {
                        interval = slider->pageStep;
                    }
                }
                if (!interval)
                {
                    interval = 1;
                }
                int fudge = len / 2;
                painter->setPen(slider->palette.text().color());
                QVarLengthArray<QLineF, 32> lines;
                int v = slider->minimum;
                while (v <= slider->maximum + 1)
                {
                    if (v == slider->maximum + 1 && interval == 1)
                    {
                        break;
                    }
                    const int v_ = qMin(v, slider->maximum);
                    int tickLength = (v_ == slider->minimum || v_ >= slider->maximum) ? 4 : 3;
                    int pos = QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, v_, available, slider->upsideDown);
                    pos += fudge;
                    if (isHorizontal)
                    {
                        if (ticks & QSlider::TicksAbove)
                        {
                            lines.append(QLineF(pos, tickOffset - 0.5, pos, tickOffset - tickLength - 0.5));
                        }

                        if (ticks & QSlider::TicksBelow)
                        {
                            lines.append(QLineF(pos, tickOffset + thickness + 0.5, pos, tickOffset + thickness + tickLength + 0.5));
                        }
                    }
                    else
                    {
                        if (ticks & QSlider::TicksAbove)
                        {
                            lines.append(QLineF(tickOffset - 0.5, pos, tickOffset - tickLength - 0.5, pos));
                        }

                        if (ticks & QSlider::TicksBelow)
                        {
                            lines.append(QLineF(tickOffset + thickness + 0.5, pos, tickOffset + thickness + tickLength + 0.5, pos));
                        }
                    }
                    // in the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                    {
                        break;
                    }
                    v = nextInterval;
                }
                if (!lines.isEmpty())
                {
                    painter->save();
                    painter->translate(slrect.topLeft());
                    painter->drawLines(lines.constData(), lines.size());
                    painter->restore();
                }
            }
            if (sub & SC_SliderHandle)
            {
                const qreal outerRadius = qMin(10.0, (isHorizontal ? handleRect.height() / 2.0 : handleRect.width() / 2.0) - 1);
                float innerRadius = outerRadius * 0.43;

                if (option->styleObject)
                {
                    const QNumberStyleAnimation *animation =
                        qobject_cast<QNumberStyleAnimation *>(getAnimation(option->styleObject));
                    if (animation != nullptr)
                    {
                        innerRadius = animation->currentValue();
                        option->styleObject->setProperty("_q_inner_radius", innerRadius);
                    }
                    else
                    {
                        bool isInsideHandle = option->activeSubControls == SC_SliderHandle;
                        innerRadius = outerRadius * sliderInnerRadius(state, isInsideHandle);
                    }
                }

                painter->setPen(Qt::NoPen);
                painter->setBrush(winUI3Color(controlFillSolid));
                painter->drawEllipse(handleCenter, outerRadius, outerRadius);
                painter->setBrush(calculateAccentColor(option));
                painter->drawEllipse(handleCenter, innerRadius, innerRadius);

                painter->setPen(winUI3Color(controlStrokeSecondary));
                painter->setBrush(Qt::NoBrush);
                painter->drawEllipse(handleCenter, outerRadius + 0.5, outerRadius + 0.5);
            }
            if (slider->state & State_HasFocus)
            {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*slider);
                fropt.rect = subElementRect(SE_SliderFocusRect, slider, widget);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
            }
        }
        break;
#endif
#if QT_CONFIG(combobox)
    case CC_ComboBox:
    {
        if (const QStyleOptionComboBox *combobox = qstyleoption_cast<const QStyleOptionComboBox *>(option))
        {
            const QRectF frameRect =
                QRectF(option->rect).marginsRemoved(QMarginsF(cBShadowBorderWidth, 1, cBShadowBorderWidth, 1));
            QStyleOption opt(*option);
            opt.state.setFlag(QStyle::State_On, false);

            auto _drawRoundedRect = [](QPainter *p, QRectF rect, QPen pen, QBrush brush)
            {
                p->setPen(pen);
                p->setBrush(brush);
                p->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            };
            const QBrush rawBrush = combobox->editable
                                        ? inputFillBrush(option, widget)
                                        : controlFillBrush(&opt, ControlType::Control);
            const QColor blendBase = combobox->editable
                                         ? option->palette.base().color()
                                         : option->palette.button().color();
            const QColor opaqueBg = resolveOpaque(rawBrush.color(), blendBase);
            _drawRoundedRect(painter, frameRect, Qt::NoPen, opaqueBg);

            if (combobox->frame)
            {
                const bool isHovered = option->state & State_MouseOver;
                const auto frameCol = highContrastTheme ?
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
                                                        option->palette.color(isHovered ? QPalette::Accent : QPalette::ButtonText)
#else
                                                        option->palette.color(isHovered ? QPalette::Highlight
                                                                                        : QPalette::ButtonText) // Qt5/Qt6.5 fallback
#endif
                                                        : winUI3Color(frameColorLight);

                const QRectF pFrameRect = frameRect.marginsRemoved(QMarginsF(0, 1.5, 0, 1.5));
                painter->setPen(frameCol);
                painter->setBrush(Qt::NoBrush);
                painter->drawRoundedRect(frameRect, secondLevelRoundingRadius, secondLevelRoundingRadius);

                if (combobox->editable)
                {
                    const bool hasFocus = option->state & State_HasFocus;
                    const qreal penWidth = hasFocus ? 2.0 : 1.0;
                    const qreal halfPen = penWidth / 2.0;

                    auto pRect = pFrameRect;
                    QRectF underlineClip(pRect.left(), pRect.bottom() - halfPen, pRect.width(), penWidth);

                    PainterStateGuard guard(painter);
                    painter->setClipRect(underlineClip);

                    const auto underlineCol = hasFocus
                                                  ? accentColor(option)
                                                  : (colorSchemeIndex == 0 ? QColor(0x80, 0x80, 0x80) : QColor(0xa0, 0xa0, 0xa0));

                    const auto penUnderline = QPen(underlineCol, hasFocus ? 2 : 1);

                    QPen pen(underlineCol, penWidth);
                    painter->setPen(pen);
                    painter->setBrush(Qt::NoBrush);
                    drawRoundedRect(painter, pRect, penUnderline, Qt::NoBrush);
                }
            }

            if (sub & SC_ComboBoxArrow)
            {
                QRectF arrowRect = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget).adjusted(4, 0, -4, 0);
                static QFont f = assetFont;
                f.setPixelSize(14);
                painter->setFont(f);
                painter->setPen(controlTextColor(option));
                QNumberStyleAnimation *animation = qobject_cast<QNumberStyleAnimation *>(getAnimation(option->styleObject));
                qreal angle = 0;
                if (state & State_On)
                {
                    angle = 180;
                }
                if (animation)
                {
                    angle = animation->currentValue();
                }
                painter->save();

                QPointF c = arrowRect.center();
                painter->translate(c);
                painter->rotate(angle);

                QRect r(-arrowRect.width() / 2, -arrowRect.height() / 2, arrowRect.width(), arrowRect.height());
                painter->drawText(r, Qt::AlignCenter, ChevronDownMed);
                painter->restore();
            }

            const bool hasFocus = state & State_HasFocus;
            if (state & State_KeyboardFocusChange && hasFocus)
            {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*option);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
            }
        }
        break;
    }
#endif // QT_CONFIG(combobox)
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option))
        {
            qreal progress = (state & State_MouseOver) ? 1.0 : 0.0;
            if (transitionsEnabled() && scrollbar->styleObject)
            {
                QObject *styleObject = scrollbar->styleObject;
                State oldState = State(styleObject->property("_q_stylestate").toInt());
                styleObject->setProperty("_q_stylestate", int(state));

                if ((state & State_MouseOver) != (oldState & State_MouseOver) && (state & State_Enabled))
                {
                    QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                    t->setStartValue((state & State_MouseOver) ? 0.0 : 1.0);
                    t->setEndValue((state & State_MouseOver) ? 1.0 : 0.0);
                    t->setDuration(167);
                    t->setEasingCurve(QEasingCurve::OutCubic);
                    startAnimation(t);
                }

                if (QNumberStyleAnimation *animation = qobject_cast<QNumberStyleAnimation *>(getAnimation(styleObject)))
                {
                    progress = animation->currentValue();
                }
            }

            int progressInt = qRound(progress * 100);

            QCachedPainter cp(painter,
                              QLatin1String("win11_scrollbar") % HexString<uint8_t>(colorSchemeIndex) % HexString<int>(scrollbar->minimum) % HexString<int>(scrollbar->maximum) % HexString<int>(scrollbar->sliderPosition) % HexString<int>(progressInt),
                              scrollbar,
                              scrollbar->rect.size());
            if (cp.needsPainting())
            {
                const bool vertical = scrollbar->orientation == Qt::Vertical;
                const bool horizontal = scrollbar->orientation == Qt::Horizontal;
                const bool isRtl = option->direction == Qt::RightToLeft;

                if (progress > 0.0)
                {
                    QRectF rect = scrollbar->rect;
                    const QPointF center = rect.center();
                    if (vertical && rect.width() > 24)
                    {
                        rect = rect.marginsRemoved(QMarginsF(0, 2, 2, 2));
                        rect.setWidth(rect.width() / 2);
                    }
                    else if (horizontal && rect.height() > 24)
                    {
                        rect = rect.marginsRemoved(QMarginsF(2, 0, 2, 2));
                        rect.setHeight(rect.height() / 2);
                    }
                    rect.moveCenter(center);

                    QColor baseColor = scrollbar->palette.base().color();
                    baseColor.setAlphaF(baseColor.alphaF() * progress);
                    cp->setBrush(baseColor);
                    cp->setPen(Qt::NoPen);
                    cp->drawRoundedRect(rect, topLevelRoundingRadius, topLevelRoundingRadius);

                    rect = rect.marginsRemoved(QMarginsF(0.5, 0.5, 0.5, 0.5));
                    cp->setBrush(Qt::NoBrush);
                    QColor frameColor = winUI3Color(frameColorLight);
                    frameColor.setAlphaF(frameColor.alphaF() * progress);
                    cp->setPen(frameColor);
                    cp->drawRoundedRect(rect, topLevelRoundingRadius + 0.5, topLevelRoundingRadius + 0.5);
                }
                if (sub & SC_ScrollBarSlider)
                {
                    QRectF rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                    const QPointF center = rect.center();
                    const qreal _width = 2.5;
                    if (vertical)
                    {
                        rect.setWidth(_width + (rect.width() / 2 - _width) * progress);
                    }
                    else
                    {
                        rect.setHeight(_width + (rect.height() / 2 - _width) * progress);
                    }
                    rect.moveCenter(center);
                    cp->setBrush(Qt::gray);
                    cp->setPen(Qt::NoPen);
                    qreal cornerRadius = _width / 2 + (secondLevelRoundingRadius - _width / 2) * progress;
                    cp->drawRoundedRect(rect, cornerRadius, cornerRadius);
                }
                if (sub & SC_ScrollBarAddLine)
                {
                    if (progress > 0.0)
                    {
                        const QRectF rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddLine, widget);
                        QFont f = QFont(assetFont);
                        f.setPointSize(6);
                        cp->setFont(f);
                        QColor penColor = Qt::gray;
                        penColor.setAlphaF(penColor.alphaF() * progress);
                        cp->setPen(penColor);
                        const auto str = vertical ? CaretDownSolid8 : (isRtl ? CaretLeftSolid8 : CaretRightSolid8);
                        cp->drawText(rect, Qt::AlignCenter, str);
                    }
                }
                if (sub & SC_ScrollBarSubLine)
                {
                    if (progress > 0.0)
                    {
                        const QRectF rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubLine, widget);
                        QFont f = QFont(assetFont);
                        f.setPointSize(6);
                        cp->setFont(f);
                        QColor penColor = Qt::gray;
                        penColor.setAlphaF(penColor.alphaF() * progress);
                        cp->setPen(penColor);
                        const auto str = vertical ? CaretUpSolid8 : (isRtl ? CaretRightSolid8 : CaretLeftSolid8);
                        cp->drawText(rect, Qt::AlignCenter, str);
                    }
                }
            }
        }
        break;
    case CC_MdiControls:
    {
        QFont buttonFont = QFont(assetFont);
        buttonFont.setPointSize(8);
        painter->setFont(buttonFont);
        drawTitleBarCloseButton(CC_MdiControls, SC_MdiCloseButton, ChromeClose);
        drawTitleBarButton(CC_MdiControls, SC_MdiNormalButton, ChromeRestore);
        drawTitleBarButton(CC_MdiControls, SC_MdiMinButton, ChromeMinimize);
    }
    break;
    case CC_TitleBar:
        if (const auto *titlebar = qstyleoption_cast<const QStyleOptionTitleBar *>(option))
        {
            painter->setPen(Qt::NoPen);
            painter->setPen(WINUI3Colors[colorSchemeIndex][surfaceStroke]);
            painter->setBrush(titlebar->palette.button());
            painter->drawRect(titlebar->rect);

            // draw title
            QRect textRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarLabel, widget);
            QColor textColor = titlebar->palette.color(
                titlebar->titleBarState & Qt::WindowActive ? QPalette::Active : QPalette::Disabled, QPalette::WindowText);
            painter->setPen(textColor);
            // Note workspace also does elliding but it does not use the
            // correct font
            QString title = painter->fontMetrics().elidedText(titlebar->text, Qt::ElideRight, textRect.width() - 14);
            painter->drawText(textRect.adjusted(1, 1, -1, -1), title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));

            QFont buttonFont = QFont(assetFont);
            buttonFont.setPointSize(8);
            painter->setFont(buttonFont);
            auto shouldDrawButton = [titlebar](SubControl sc, Qt::WindowType flag)
            { return (titlebar->subControls & sc) && (titlebar->titleBarFlags & flag); };

            // min button
            if (shouldDrawButton(SC_TitleBarMinButton, Qt::WindowMinimizeButtonHint) && !(titlebar->titleBarState & Qt::WindowMinimized))
            {
                drawTitleBarButton(CC_TitleBar, SC_TitleBarMinButton, ChromeMinimize);
            }

            // max button
            if (shouldDrawButton(SC_TitleBarMaxButton, Qt::WindowMaximizeButtonHint) && !(titlebar->titleBarState & Qt::WindowMaximized))
            {
                drawTitleBarButton(CC_TitleBar, SC_TitleBarMaxButton, ChromeMaximize);
            }

            // close button
            if (shouldDrawButton(SC_TitleBarCloseButton, Qt::WindowSystemMenuHint))
            {
                drawTitleBarCloseButton(CC_TitleBar, SC_TitleBarCloseButton, ChromeClose);
            }

            // normalize button
            if ((titlebar->subControls & SC_TitleBarNormalButton) && (((titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint) && (titlebar->titleBarState & Qt::WindowMinimized)) || ((titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint) && (titlebar->titleBarState & Qt::WindowMaximized))))
            {
                drawTitleBarButton(CC_TitleBar, SC_TitleBarNormalButton, ChromeRestore);
            }

            // context help button
            if (shouldDrawButton(SC_TitleBarContextHelpButton, Qt::WindowContextHelpButtonHint))
            {
                drawTitleBarButton(CC_TitleBar, SC_TitleBarContextHelpButton, Help);
            }

            // shade button
            if (shouldDrawButton(SC_TitleBarShadeButton, Qt::WindowShadeButtonHint))
            {
                drawTitleBarButton(CC_TitleBar, SC_TitleBarShadeButton, ChevronUpSmall);
            }

            // unshade button
            if (shouldDrawButton(SC_TitleBarUnshadeButton, Qt::WindowShadeButtonHint))
            {
                drawTitleBarButton(CC_TitleBar, SC_TitleBarUnshadeButton, ChevronDownSmall);
            }

            // window icon for system menu
            if (shouldDrawButton(SC_TitleBarSysMenu, Qt::WindowSystemMenuHint))
            {
                const QRect iconRect = proxy()->subControlRect(CC_TitleBar, titlebar, SC_TitleBarSysMenu, widget);
                if (iconRect.isValid())
                {
                    if (!titlebar->icon.isNull())
                    {
                        titlebar->icon.paint(painter, iconRect);
                    }
                    else
                    {
                        QStyleOption tool = *titlebar;
                        const auto extent = proxy()->pixelMetric(PM_SmallIconSize, &tool, widget);
                        const auto dpr = QStyleHelper::getDpr(widget);
                        const auto icon = proxy()->standardIcon(SP_TitleBarMenuButton, &tool, widget);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                        QPixmap pm = icon.pixmap(QSize(extent, extent), dpr);
#else
                        QPixmap pm = icon.pixmap(QSize(extent, extent));
                        pm.setDevicePixelRatio(dpr);
#endif
                        proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pm);
                    }
                }
            }
        }
        break;
#if QT_CONFIG(toolbutton)
    case CC_ToolButton:
        if (const auto *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(option))
        {
            QRect button, menuarea;
            button = proxy()->subControlRect(control, toolbutton, SC_ToolButton, widget);
            menuarea = proxy()->subControlRect(control, toolbutton, SC_ToolButtonMenu, widget);

            State bflags = toolbutton->state & ~State_Sunken;
            State mflags = bflags;
            bool autoRaise = flags & State_AutoRaise;
            if (autoRaise)
            {
                if (!(bflags & State_MouseOver) || !(bflags & State_Enabled))
                {
                    bflags &= ~State_Raised;
                }
            }

            if (toolbutton->state & State_Sunken)
            {
                if (toolbutton->activeSubControls & SC_ToolButton)
                {
                    bflags |= State_Sunken;
                    mflags |= State_MouseOver | State_Sunken;
                }
                else if (toolbutton->activeSubControls & SC_ToolButtonMenu)
                {
                    mflags |= State_Sunken;
                    bflags |= State_MouseOver;
                }
            }

            QStyleOption tool = *toolbutton;
            if (toolbutton->subControls & SC_ToolButton)
            {
                if (flags & (State_Sunken | State_On | State_Raised) || !autoRaise)
                {
                    tool.rect = option->rect;
                    tool.state = bflags;
                    if (autoRaise) // for tool bars
                    {
                        proxy()->drawPrimitive(PE_PanelButtonTool, &tool, painter, widget);
                    }
                    else
                    {
                        proxy()->drawPrimitive(PE_PanelButtonBevel, &tool, painter, widget);
                    }
                }
            }

            if (toolbutton->state & State_HasFocus)
            {
                QStyleOptionFocusRect fr;
                fr.QStyleOption::operator=(*toolbutton);
                fr.rect.adjust(3, 3, -3, -3);
                if (toolbutton->features & QStyleOptionToolButton::MenuButtonPopup)
                {
                    fr.rect.adjust(0, 0, -proxy()->pixelMetric(QStyle::PM_MenuButtonIndicator, toolbutton, widget), 0);
                }
                proxy()->drawPrimitive(PE_FrameFocusRect, &fr, painter, widget);
            }
            QStyleOptionToolButton label = *toolbutton;
            label.state = bflags;
            int fw = 2;
            if (!autoRaise)
            {
                label.state &= ~State_Sunken;
            }
            label.rect = button.adjusted(fw, fw, -fw, -fw);
            proxy()->drawControl(CE_ToolButtonLabel, &label, painter, widget);

            if (toolbutton->subControls & SC_ToolButtonMenu)
            {
                tool.rect = menuarea;
                tool.state = mflags;
                if (autoRaise)
                {
                    proxy()->drawPrimitive(PE_IndicatorButtonDropDown, &tool, painter, widget);
                }
                else
                {
                    tool.state = mflags;
                    menuarea.adjust(-2, 0, 0, 0);
                    // Draw menu button
                    if ((bflags & State_Sunken) != (mflags & State_Sunken))
                    {
                        painter->save();
                        painter->setClipRect(menuarea);
                        tool.rect = option->rect;
                        proxy()->drawPrimitive(PE_PanelButtonBevel, &tool, painter, nullptr);
                        painter->restore();
                    }
                    // Draw arrow
                    painter->save();
                    painter->setPen(option->palette.dark().color());
                    painter->drawLine(menuarea.left(), menuarea.top() + 3, menuarea.left(), menuarea.bottom() - 3);
                    painter->setPen(option->palette.light().color());
                    painter->drawLine(menuarea.left() - 1, menuarea.top() + 3, menuarea.left() - 1, menuarea.bottom() - 3);

                    tool.rect = menuarea.adjusted(2, 3, -2, -1);
                    proxy()->drawPrimitive(PE_IndicatorArrowDown, &tool, painter, widget);
                    painter->restore();
                }
            }
            else if (toolbutton->features & QStyleOptionToolButton::HasMenu)
            {
                QRect arrowRect = menuarea;

                static QFont font(assetFont);
                font.setPixelSize(12);
                painter->setFont(font);
                painter->setPen(controlTextColor(option));

                QNumberStyleAnimation *animation = qobject_cast<QNumberStyleAnimation *>(getAnimation(option->styleObject));
                qreal angle = 0;
                if (state & State_Sunken)
                {
                    angle = 180;
                }
                if (animation)
                {
                    angle = animation->currentValue();
                }
                painter->save();
                QPoint c = arrowRect.center();
                painter->translate(c);
                painter->rotate(angle);

                QRect r(-arrowRect.width() / 2, -arrowRect.height() / 2, arrowRect.width(), arrowRect.height());
                painter->drawText(r, Qt::AlignCenter, ChevronDown);
                painter->restore();
            }
        }
        break;
#endif // QT_CONFIG(toolbutton)
    default:
        QProxyStyle::drawComplexControl(control, option, painter, widget);
        break;
    }
    painter->restore();
}

void FluentUI3Style::drawSpecialButton(QPainter *painter, const QStyleOption *option, const QWidget *widget, bool &isReturn) const
{
    isReturn = false;
    if (!widget)
    {
        return;
    }

    const QString objectName = widget->objectName();
    const bool isHover = option->state & State_MouseOver;
    const bool isDown = option->state & State_Sunken;

    if (objectName == "qt_dockwidget_closebutton")
    {
        isReturn = true;
        if (isDown)
        {
            painter->setBrush(shellCaptionCloseFillColorSecondary);
        }
        else if (isHover)
        {
            painter->setBrush(shellCaptionCloseFillColorPrimary);
        }
        else
        {
            return;
        }
    }
    else if (objectName == "qt_dockwidget_floatbutton")
    {
        isReturn = true;
        if (isDown)
        {
            painter->setBrush(winUI3Color(subtleHighlightColor).darker(110));
        }
        else if (isHover)
        {
            painter->setBrush(winUI3Color(subtleHighlightColor));
        }
        else
        {
            return;
        }
    }
    else if (objectName == "ScrollLeftButton" || objectName == "ScrollRightButton")
    {
        isReturn = true;
        return; // 无绘制
    }
    else
    {
        isReturn = false;
        return; // 不处理其他按钮
    }

    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawRect(option->rect);
}

void FluentUI3Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    PainterStateGuard guard(painter);
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    int state = option->state;
    if (transitionsEnabled() && option->styleObject && (element == PE_IndicatorCheckBox || element == PE_IndicatorRadioButton))
    {
        QObject *styleObject = option->styleObject; // Can be widget or qquickitem
        int oldState = styleObject->property("_q_stylestate").toInt();
        styleObject->setProperty("_q_stylestate", int(option->state));
        styleObject->setProperty("_q_stylerect", option->rect);
        bool doTransition =
            (((state & State_Sunken) != (oldState & State_Sunken) || ((state & State_MouseOver) != (oldState & State_MouseOver)) || (state & State_On) != (oldState & State_On)) && state & State_Enabled);
        if (doTransition)
        {
            if (element == PE_IndicatorRadioButton)
            {
                QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                t->setStartValue(styleObject->property("_q_inner_radius").toFloat());
                int indicatorSize = proxy()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, option, widget);
                t->setEndValue(radioButtonInnerRadius(state, option, widget, indicatorSize));
                styleObject->setProperty("_q_end_radius", t->endValue());
                t->setDuration(150);
                startAnimation(t);
            }
            else if (element == PE_IndicatorCheckBox && widget && widget->property(SwitchStyleProperty).toBool() == false)
            {
                if ((oldState & State_Off && state & State_On) || (oldState & State_NoChange && state & State_On))
                {
                    QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                    t->setStartValue(0.0f);
                    t->setEndValue(1.0f);
                    t->setDuration(150);
                    startAnimation(t);
                }
            }
            else if (element == PE_IndicatorCheckBox && widget && widget->property(SwitchStyleProperty).toBool() == true)
            {
                if ((state & State_On) != (oldState & State_On))
                {
                    QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                    t->setStartValue(animationValue(styleObject, "_q_thumb_pos", oldState & State_On ? 1.0f : 0.0f));
                    t->setEndValue(state & State_On ? 1.0f : 0.0f);
                    t->setDuration(150);
                    t->setFrameRate(QStyleAnimation::DefaultFps);
                    startAnimationEx(t, styleObject, "_q_thumb_pos");
                }
                if ((state & State_MouseOver) != (oldState & State_MouseOver))
                {
                    QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                    t->setStartValue(animationValue(styleObject, "_q_thumb_scale", oldState & State_MouseOver ? 1.1f : 0.9f));
                    t->setEndValue(state & State_MouseOver ? 1.1f : 0.9f);
                    t->setDuration(180);
                    startAnimationEx(t, styleObject, "_q_thumb_scale");
                }
                if ((state & State_Sunken) != (oldState & State_Sunken))
                {
                    QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                    t->setStartValue(animationValue(styleObject, "_q_thumb_stretch", oldState & State_Sunken ? 1.3f : 1.0f));
                    t->setEndValue(state & State_Sunken ? 1.3f : 1.0f);
                    t->setDuration(120);
                    startAnimationEx(t, styleObject, "_q_thumb_stretch");
                }
            }
        }
    }

    switch (element)
    {
    case PE_FrameFocusRect:
    {
        if (const QStyleOptionFocusRect *fropt = qstyleoption_cast<const QStyleOptionFocusRect *>(option))
        {
            if (!(fropt->state & State_KeyboardFocusChange))
            {
                break;
            }
            QRectF focusRect = option->rect;
            focusRect = focusRect.marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));
            painter->setPen(winUI3Color(focusFrameInnerStroke));
            painter->drawRoundedRect(focusRect, 4, 4);

            focusRect = focusRect.marginsAdded(QMarginsF(1.0, 1.0, 1.0, 1.0));
            painter->setPen(QPen(winUI3Color(focusFrameOuterStroke), 1));
            painter->drawRoundedRect(focusRect, 4, 4);
        }
        break;
    }
    case PE_PanelTipLabel:
    {
        // QProxyStyle::drawPrimitive( element, option, painter, widget );
        // return;
        const auto rect = QRectF(option->rect).marginsRemoved(QMarginsF(0.5, 0.5, 0.5, 0.5));
        const auto pen = highContrastTheme ? option->palette.buttonText().color() : winUI3Color(frameColorLight);
        // drawRoundedRect( painter, rect, pen, option->palette.toolTipBase() );
        painter->drawRect(option->rect);
        break;
    }
    case PE_FrameTabWidget:
#if QT_CONFIG(tabwidget)
        if (const QStyleOptionTabWidgetFrame *frame = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option))
        {
            const auto rect = QRectF(option->rect).marginsRemoved(QMarginsF(0.5, 0.5, 0.5, 0.5));
            const auto pen = highContrastTheme ? frame->palette.buttonText().color() : winUI3Color(frameColorLight);
            painter->setPen(pen);
            painter->setBrush(frame->palette.base());
            painter->drawRect(rect);
        }
#endif // QT_CONFIG(tabwidget)
        break;
    case PE_FrameGroupBox:
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(option))
        {
            const auto pen = highContrastTheme ? frame->palette.buttonText().color() : winUI3Color(frameColorStrong);
            if (frame->features & QStyleOptionFrame::Flat)
            {
                painter->setBrush(Qt::NoBrush);
                painter->setPen(pen);
                const QRect &fr = frame->rect;
                QPoint p1(fr.x(), fr.y() + 1);
                QPoint p2(fr.x() + fr.width(), p1.y());
                painter->drawLine(p1, p2);
            }
            else
            {
                const auto frameRect = QRectF(frame->rect).marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));
                drawRoundedRect(painter, frameRect, pen, Qt::NoBrush);
            }
        }
        break;
    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option))
        {
            QFont f(assetFont);
            f.setPointSize(6);
            painter->setFont(f);
            painter->setPen(header->palette.text().color());
            QRectF rect = option->rect;
            if (header->sortIndicator & QStyleOptionHeader::SortUp)
            {
                painter->drawText(rect, Qt::AlignCenter, ChevronUpSmall);
            }
            else if (header->sortIndicator & QStyleOptionHeader::SortDown)
            {
                painter->drawText(rect, Qt::AlignCenter, ChevronDownSmall);
            }
        }
        break;
    case PE_IndicatorCheckBox:
        if (widget && widget->property(SwitchStyleProperty).toBool())
        {
            drawSwitchButton(option, painter, widget);
        }
        else
        {
            drawCheckBox(option, painter, widget);
        }
        break;
    case PE_IndicatorBranch:
    {
        if (const QTreeView *treeView = qobject_cast<const QTreeView *>(widget);
            treeView && treeView->property(NavigationViewStyleProperty).toBool())
        {
            break;
        }

        if (option->state & State_Children)
        {
            const bool isReverse = option->direction == Qt::RightToLeft;
            const bool isOpen = option->state & QStyle::State_Open;
            QFont f(assetFont);
            f.setPointSize(11);
            painter->setFont(f);
            painter->setPen(option->palette.color(isOpen ? QPalette::Active : QPalette::Disabled, QPalette::WindowText));
            // 用每行唯一的 key 读取动画值，避免节点间动画状态共享
            const QByteArray animKey = QByteArrayLiteral("_q_branch_anim_") + QByteArray::number(option->rect.y());
            QNumberStyleAnimation *animation = widget
                                                   ? qobject_cast<QNumberStyleAnimation *>(getAnimationEx(const_cast<QWidget *>(widget), animKey))
                                                   : nullptr;
            qreal angle = isOpen ? (isReverse ? -90.0 : 90.0) : 0.0;
            if (animation)
            {
                angle = animation->currentValue();
            }
            painter->save();
            // 向右偏移 4px 以增大与左侧蓝色 indicator 竖条的间距
            const QPointF c = option->rect.center() + QPointF(isReverse ? -4.0 : 4.0, 0.0);
            painter->translate(c);
            painter->rotate(angle);
            const int half = qMin(option->rect.width(), option->rect.height()) / 2;
            QRect r(-half, -half, half * 2, half * 2);
            painter->drawText(r, Qt::AlignCenter, isReverse ? ChevronLeftMed : ChevronRightMed);
            painter->restore();
        }
    }
    break;
    case PE_IndicatorRadioButton:
    {
        const bool isOn = option->state & State_On;
        int indicatorSize = proxy()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, option, widget);
        qreal outerRadius = indicatorSize / 2.0;
        qreal innerRadius = radioButtonInnerRadius(state, option, widget, indicatorSize);
        if (transitionsEnabled() && option->styleObject)
        {
            if (option->styleObject->property("_q_end_radius").isNull())
            {
                option->styleObject->setProperty("_q_end_radius", innerRadius);
            }
            QNumberStyleAnimation *animation = qobject_cast<QNumberStyleAnimation *>(getAnimation(option->styleObject));
            innerRadius = animation ? animation->currentValue() : option->styleObject->property("_q_end_radius").toFloat();
            option->styleObject->setProperty("_q_inner_radius", innerRadius);
        }

        const QRectF rect = option->rect;
        const QPointF center = rect.center();

        painter->setPen(borderPenControlAlt(option));
        painter->setBrush(controlFillBrush(option, ControlType::ControlAlt));
        if (isOn)
        {
            QPainterPath path;
            path.addEllipse(center, outerRadius, outerRadius);
            path.addEllipse(center, innerRadius, innerRadius);
            painter->drawPath(path);
            // Text On Accent/Primary
            painter->setBrush(option->palette.window().color());
            painter->drawEllipse(center, innerRadius, innerRadius);
        }
        else
        {
            painter->drawEllipse(center, outerRadius, outerRadius);
        }
        return;
    }
    break;
    case PE_PanelButtonTool:
    {
        painter->save();
        painter->setPen(Qt::NoPen);

        bool isReturn = false;
        drawSpecialButton(painter, option, widget, isReturn);
        if (isReturn)
        {
            painter->restore();
            return;
        }
        painter->restore();

        const bool isEnabled = state & QStyle::State_Enabled;
        const bool isMouseOver = state & QStyle::State_MouseOver;
        const bool isRaised = state & QStyle::State_Raised;
        const QRectF rect = option->rect.marginsRemoved(QMargins(2, 2, 2, 2));

        if (state & State_AutoRaise)
        {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::NoBrush);

            if (state & QStyle::State_Sunken && isEnabled)
            {
                painter->setBrush(winUI3Color(subtlePressedColor));
            }
            else if (isMouseOver && isEnabled)
            {
                painter->setBrush(winUI3Color(subtleHighlightColor));
            }

            if (auto btn = qobject_cast<const QAbstractButton *>(widget); btn->isChecked())
            {
                painter->setBrush(winUI3Color(subtlePressedColor));
            }

            painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            return;
        }

        if ((!isMouseOver && !isRaised) || !isEnabled)
        {
            painter->setPen(Qt::NoPen);
        }
        else
        {
            painter->setPen(winUI3Color(controlStrokePrimary));
        }

        drawEffectShadow(painter, option->rect, 2, secondLevelRoundingRadius);
        painter->setBrush(controlFillBrush(option, ControlType::Control));
        painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

        if (isRaised)
        {
            const qreal sublineOffset = secondLevelRoundingRadius - 0.5;
            painter->setPen(winUI3Color(controlStrokeSecondary));
            painter->drawLine(rect.bottomLeft() + QPointF(sublineOffset, 0.5), rect.bottomRight() + QPointF(-sublineOffset, 0.5));
        }
    }
    break;
    case PE_PanelButtonBevel:
    {
        const bool isEnabled = state & QStyle::State_Enabled;
        const bool isMouseOver = state & QStyle::State_MouseOver;
        const bool isRaised = state & QStyle::State_Raised;
        const QRectF rect = option->rect.marginsRemoved(QMargins(2, 2, 2, 2));
        if (element == PE_PanelButtonTool && ((!isMouseOver && !isRaised) || !isEnabled))
        {
            painter->setPen(Qt::NoPen);
        }
        else
        {
            painter->setPen(winUI3Color(controlStrokePrimary));
        }

        drawEffectShadow(painter, option->rect, 2, secondLevelRoundingRadius);
        painter->setBrush(controlFillBrush(option, ControlType::Control));
        painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
    }
    break;
    case PE_FrameDefaultButton:
        painter->setPen(accentColor(option));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(option->rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
        break;
    case PE_FrameMenu:
        break;
    case PE_PanelMenu:
    {
        drawFluentShadow(painter, option->rect, 2, topLevelRoundingRadius);
        const QRect rect = option->rect.marginsRemoved(QMargins(2, 2, 2, 2));
        painter->setPen(highContrastTheme ? QPen(option->palette.windowText().color(), 2) : winUI3Color(frameColorLight));
        painter->setBrush(winUI3Color(menuPanelFill));
        painter->drawRoundedRect(rect, topLevelRoundingRadius, topLevelRoundingRadius);
        break;
    }
    case PE_PanelLineEdit:
        if (const auto *panel = qstyleoption_cast<const QStyleOptionFrame *>(option))
        {
            const bool isInSpinBox = widget && qobject_cast<const QAbstractSpinBox *>(widget->parent()) != nullptr;
            const bool isInComboBox = widget && qobject_cast<const QComboBox *>(widget->parent()) != nullptr;
            if (!isInSpinBox && !isInComboBox)
            {
                const auto frameRect = QRectF(option->rect).marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));

                auto inputFillBrush = [this](const QStyleOption *option, const QWidget *widget) -> QBrush
                {
                    // slightly different states than in controlFillBrush
                    using namespace StyleOptionHelper;
                    const auto role = widget ? widget->backgroundRole() : QPalette::Window;
                    if (option->palette.isBrushSet(QPalette::Current, role))
                    {
                        return option->palette.button();
                    }

                    if (isDisabled(option))
                    {
                        return winUI3Color(fillControlDisabled);
                    }
                    if (option->state & State_HasFocus)
                    {
                        return winUI3Color(fillControlInputActive);
                    }
                    if (isHover(option))
                    {
                        return winUI3Color(fillControlSecondary);
                    }
                    return winUI3Color(fillControlDefault);
                };

                const QColor opaqueBg =
                    resolveOpaque(inputFillBrush(option, widget).color(), option->palette.base().color());
                drawRoundedRect(painter, frameRect, Qt::NoPen, opaqueBg);
                if (panel->lineWidth > 0)
                {
                    proxy()->drawPrimitive(PE_FrameLineEdit, panel, painter, widget);
                }
            }
        }
        break;
    case PE_FrameLineEdit:
    {
        const auto frameRect = QRectF(option->rect).marginsRemoved(QMarginsF(1.5, 1.5, 1.5, 1.5));
        drawLineEditFrame(painter, frameRect, option, !(option->state & State_ReadOnly));
        if (state & State_KeyboardFocusChange && state & State_HasFocus)
        {
            QStyleOptionFocusRect fropt;
            fropt.QStyleOption::operator=(*option);
            proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
        }
        break;
    }
    case PE_Frame:
    {
        if (const auto *frame = qstyleoption_cast<const QStyleOptionFrame *>(option))
        {
            QRectF rect = option->rect;

            const bool isComboPopup = widget && widget->inherits("QComboBoxPrivateContainer");

            painter->setPen(Qt::NoPen);

            bool noRoundedCorners = widget && widget->property(NoRoundedCorners).toBool();
            if (isComboPopup)
            {
                drawFluentShadow(painter, rect.toRect(), cBShadowBorderWidth, cBRoundingRadius);
                painter->setBrush(winUI3Color(menuPanelFill));
                auto pRect = QRectF(rect).marginsRemoved(
                    QMarginsF(cBShadowBorderWidth, cBShadowBorderWidth, cBShadowBorderWidth, cBShadowBorderWidth));
                painter->drawRoundedRect(pRect, cBRoundingRadius, cBRoundingRadius);
            }
            else
            {
                painter->setBrush(option->palette.brush(QPalette::Base));
                if (noRoundedCorners)
                {
                    painter->drawRect(rect);
                }
                else
                {
                    painter->drawRoundedRect(rect, 6, 6);
                }
            }

            if (frame->frameShape == QFrame::NoFrame)
            {
                break;
            }

            QRectF borderRect = rect.adjusted(0.5, 0.5, -0.5, -0.5);

            if (noRoundedCorners)
            {
                const bool isHovered = option->state & State_MouseOver;
                const auto frameCol = highContrastTheme ?
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
                                                        option->palette.color(isHovered ? QPalette::Accent : QPalette::ButtonText)
#else
                                                        option->palette.color(isHovered ? QPalette::Highlight : QPalette::ButtonText)
#endif
                                                        : winUI3Color(frameColorLight);

                painter->setPen(frameCol);
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(borderRect);
            }
            else
            {
                drawLineEditFrame(painter, borderRect, option, qobject_cast<const QTextEdit *>(widget) != nullptr);
            }
        }
        break;
    }
    case PE_PanelItemViewItem:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option))
        {
            if (vopt->backgroundBrush.style() != Qt::NoBrush)
            {
                PainterStateGuard psg(painter);
                painter->setBrushOrigin(vopt->rect.topLeft());

                auto rect = vopt->rect;
                painter->fillRect(rect, vopt->backgroundBrush);
            }
        }
        break;
    case PE_PanelItemViewRow:
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option))
        {
            // this is only called from a QTreeView to paint
            //  - the tree branch decoration (incl. selected/hovered or not)
            //  - the (alternate) background of the item in always
            //  unselected state
            const QRect &rect = vopt->rect;
            const bool isRtl = option->direction == Qt::RightToLeft;
            if (rect.width() <= 0)
            {
                break;
            }

            if (vopt->features & QStyleOptionViewItem::Alternate)
            {
                QPalette::ColorGroup cg =
                    (widget ? widget->isEnabled() : (vopt->state & QStyle::State_Enabled)) ? QPalette::Normal : QPalette::Disabled;
                if (cg == QPalette::Normal && !(vopt->state & QStyle::State_Active))
                {
                    cg = QPalette::Inactive;
                }
                painter->fillRect(rect, option->palette.brush(cg, QPalette::AlternateBase));
            }

            bool hightlightCurrent = (vopt->state & (State_Selected | State_MouseOver)) != 0;
            const bool isNonTableHighlight = !qobject_cast<const QTableView *>(widget) && hightlightCurrent && !highContrastTheme;
            const bool isNavView = qobject_cast<const QTreeView *>(widget) && widget->property(NavigationViewStyleProperty).toBool();
            const bool isPlainTreeView = qobject_cast<const QTreeView *>(widget) && !isNavView;

            if (isPlainTreeView && isNonTableHighlight)
            {
                // 普通 QTreeView：只画蓝色 indicator 竖条，背景由 CE_ItemViewItem 负责
                drawTreeViewIndicator(vopt, painter, widget);
            }

            if (isNonTableHighlight)
            {
                bool isDecorationColumn = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                isDecorationColumn = vopt->features.testAnyFlags(QStyleOptionViewItem::IsDecorationForRootColumn);
#else
                if (qobject_cast<const QTreeView *>(widget))
                {
                    // isDecorationColumn = !vopt->index.isValid() || vopt->index.column() == 0;
                }
#endif
                if (isDecorationColumn && vopt->showDecorationSelected)
                {
                    const bool onlyOne = vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne || vopt->viewItemPosition == QStyleOptionViewItem::Invalid;
                    bool isFirst = vopt->viewItemPosition == QStyleOptionViewItem::Beginning;
                    bool isLast = vopt->viewItemPosition == QStyleOptionViewItem::End;

                    if (onlyOne)
                    {
                        isFirst = true;
                    }

                    if (isRtl)
                    {
                        isFirst = !isFirst;
                        isLast = !isLast;
                    }

                    const bool isSelected = (vopt->state & State_Selected) != 0;
                    const QColor itemHighlight = isSelected ? winUI3Color(subtlePressedColor)
                                                            : winUI3Color(subtleHighlightColor);
                    painter->setBrush(itemHighlight);
                    painter->setPen(Qt::NoPen);
                    if (isFirst)
                    {
                        PainterStateGuard psg(painter);
                        painter->setClipRect(rect);
                        painter->drawRoundedRect(rect.marginsRemoved(QMargins(2, 2, -secondLevelRoundingRadius, 2)),
                                                 secondLevelRoundingRadius,
                                                 secondLevelRoundingRadius);
                    }
                    else if (isLast)
                    {
                        PainterStateGuard psg(painter);
                        painter->setClipRect(rect);
                        painter->drawRoundedRect(rect.marginsRemoved(QMargins(-secondLevelRoundingRadius, 2, 2, 2)),
                                                 secondLevelRoundingRadius,
                                                 secondLevelRoundingRadius);
                    }
                    else
                    {
                        painter->drawRect(vopt->rect.marginsRemoved(QMargins(0, 2, 0, 2)));
                    }
                }
            }

        }
        break;
    case PE_Widget:
    {
#if QT_CONFIG(dialogbuttonbox)
        const QDialogButtonBox *buttonBox = nullptr;
        if (qobject_cast<const QMessageBox *> (widget))
            buttonBox = widget->findChild<const QDialogButtonBox *>(QLatin1String("qt_msgbox_buttonbox"));

        if (buttonBox) {
            QRect toprect = option->rect;
            toprect.setBottom(buttonBox->geometry().top());
            painter->fillRect(toprect, option->palette.brush(QPalette::Base));

            QRect buttonRect = option->rect;
            buttonRect.setTop(buttonBox->geometry().top());
            painter->fillRect(buttonRect, option->palette.brush(QPalette::Window));
        }
#endif
        if (widget && widget->property("isCard").toBool())
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);

            QRect r = option->rect.adjusted(1, 1, -1, -1);

            const bool isHovered = option->state & State_MouseOver;

            const auto frameCol = highContrastTheme ?
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
                                                    option->palette.color(isHovered ? QPalette::Accent : QPalette::ButtonText)
#else
                                                    option->palette.color(isHovered ? QPalette::Highlight : QPalette::ButtonText)
#endif
                                                    : winUI3Color(cardStrokeColorDefault);

            QPen pen(frameCol);
            pen.setWidth(1);
            painter->setPen(pen);

            QColor brColor;
            if (highContrastTheme)
            {
                brColor = option->palette.base().color();
            }
            else
            {
                const QColor cardColor = winUI3Color(cardBackgroundFillColorDefault);
                const QColor blendBase = opaqueBlendBase(option->palette, colorSchemeIndex == 1);
                brColor = resolveOpaque(cardColor, blendBase);

                // Example项目:
                // WidgetBgMode::Pixmap 会把 Base/Window 设为透明，这里保留最低透明度避免 card 过深或过透。
                //不需要的话，直接使用cardColor即可
                const bool wallpaperMode = qApp && qApp->property("_q_widget_mode").toBool();
                if (wallpaperMode)
                {
                    const int minAlpha = (colorSchemeIndex == 1) ? 72 : 92;
                    const int keepAlpha = qMax(minAlpha, qMin(160, blendBase.alpha()));
                    brColor.setAlpha(keepAlpha);
                }
            }
            painter->setBrush(brColor);
            painter->drawRoundedRect(r, 4, 4);

            painter->restore();
            return;
        }

        if (const QTabBar *tabBar = qobject_cast<const QTabBar *>(widget))
        {
            const QColor backgroundColor = segmentedThemeColorProperty(
                tabBar,
                colorSchemeIndex == 1,
                SegmentedBackgroundColorProperty,
                SegmentedBackgroundColorDarkProperty,
                highContrastTheme ? option->palette.button().color() : winUI3Color(fillControlAltSecondary));
            const bool segmentedSemiRound = segmentedBoolProperty(tabBar, SegmentedSemiRoundProperty, false);
            const QColor borderColor = highContrastTheme ? option->palette.buttonText().color() : winUI3Color(controlStrokePrimary);

            if (tabBar->count() > 0)
            {
                QRectF bgRect = tabBar->tabRect(0);
                for (int i = 1; i < tabBar->count(); ++i)
                {
                    bgRect = bgRect.united(tabBar->tabRect(i));
                }
                bgRect.adjust(0.5, 0.5, -0.5, -0.5);
                if (bgRect.isValid())
                {
                    const qreal radius = segmentedSemiRound ? bgRect.height() / 2.0 : secondLevelRoundingRadius;
                    painter->setPen(QPen(borderColor, 1));
                    painter->setBrush(backgroundColor);
                    painter->drawRoundedRect(bgRect, radius, radius);
                    return;
                }
            }
        }

        if (widget && widget->palette().isBrushSet(QPalette::Active, widget->backgroundRole()))
        {
            const QBrush bg = option->palette.brush(widget->backgroundRole());
            updateBrushOrigin_public(painter, widget, bg);
            painter->fillRect(option->rect, bg);
        }
        break;
    }
    case PE_FrameWindow:
        if (const auto *frm = qstyleoption_cast<const QStyleOptionFrame *>(option))
        {
            QRectF rect = option->rect;
            int fwidth = int((frm->lineWidth + frm->midLineWidth) / nativeMetricScaleFactor(widget));

            QRectF bottomLeftCorner = QRectF(rect.left() + 1.0,
                                             rect.bottom() - 1.0 - secondLevelRoundingRadius,
                                             secondLevelRoundingRadius,
                                             secondLevelRoundingRadius);
            QRectF bottomRightCorner = QRectF(rect.right() - 1.0 - secondLevelRoundingRadius,
                                              rect.bottom() - 1.0 - secondLevelRoundingRadius,
                                              secondLevelRoundingRadius,
                                              secondLevelRoundingRadius);

            // Draw Mask
            if (widget != nullptr)
            {
                QBitmap mask(widget->width(), widget->height());
                mask.clear();

                QPainter maskPainter(&mask);
                maskPainter.setRenderHint(QPainter::Antialiasing);
                maskPainter.setBrush(Qt::color1);
                maskPainter.setPen(Qt::NoPen);
                maskPainter.drawRoundedRect(option->rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
                const_cast<QWidget *>(widget)->setMask(mask);
            }

            // Draw Window
            painter->setPen(QPen(frm->palette.base(), fwidth));
            painter->drawLine(QPointF(rect.left(), rect.top()), QPointF(rect.left(), rect.bottom() - fwidth));
            painter->drawLine(QPointF(rect.left() + fwidth, rect.bottom()), QPointF(rect.right() - fwidth, rect.bottom()));
            painter->drawLine(QPointF(rect.right(), rect.top()), QPointF(rect.right(), rect.bottom() - fwidth));

            painter->setPen(winUI3Color(surfaceStroke));
            painter->drawLine(QPointF(rect.left() + 0.5, rect.top() + 0.5),
                              QPointF(rect.left() + 0.5, rect.bottom() - 0.5 - secondLevelRoundingRadius));
            painter->drawLine(QPointF(rect.left() + 0.5 + secondLevelRoundingRadius, rect.bottom() - 0.5),
                              QPointF(rect.right() - 0.5 - secondLevelRoundingRadius, rect.bottom() - 0.5));
            painter->drawLine(QPointF(rect.right() - 0.5, rect.top() + 1.5),
                              QPointF(rect.right() - 0.5, rect.bottom() - 0.5 - secondLevelRoundingRadius));

            painter->setPen(Qt::NoPen);
            painter->setBrush(frm->palette.base());
            painter->drawPie(bottomRightCorner.marginsAdded(QMarginsF(2.5, 2.5, 0.0, 0.0)), 270 * 16, 90 * 16);
            painter->drawPie(bottomLeftCorner.marginsAdded(QMarginsF(0.0, 2.5, 2.5, 0.0)), -90 * 16, -90 * 16);

            painter->setPen(winUI3Color(surfaceStroke));
            painter->setBrush(Qt::NoBrush);
            painter->drawArc(bottomRightCorner, 0 * 16, -90 * 16);
            painter->drawArc(bottomLeftCorner, -90 * 16, -90 * 16);
        }
        break;
    case PE_IndicatorTabTearLeft:
    case PE_IndicatorTabTearRight:
        break;
    case PE_FrameTabBarBase:
        break;
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
    case PE_IndicatorTabClose:
    {
        QIcon tabBarcloseButtonIcon = fluentIcon(QChar(0xE894));
        const int size = proxy()->pixelMetric(QStyle::PM_SmallIconSize, option);
        QIcon::Mode mode = option->state & State_Enabled ? (option->state & State_Raised ? QIcon::Active : QIcon::Normal)
                                                         : QIcon::Disabled;
        if (!(option->state & State_Raised) && !(option->state & State_Sunken) && !(option->state & QStyle::State_Selected))
        {
            mode = QIcon::Disabled;
        }

        QIcon::State state = option->state & State_Sunken ? QIcon::On : QIcon::Off;
        QPixmap pixmap = tabBarcloseButtonIcon.pixmap(size, mode, state);
        proxy()->drawItemPixmap(painter, option->rect, Qt::AlignCenter, pixmap);
        break;
    }
#endif
    case PE_IndicatorToolBarSeparator:
    {
        QPen pen = painter->pen();
        int margin = 3;
        painter->setPen(winUI3Color(dividerStrokeDefault));
        if (option->state & State_Horizontal)
        {
            int x1 = option->rect.center().x();
            painter->drawLine(QPoint(x1, option->rect.top() + margin), QPoint(x1, option->rect.bottom() - margin));
        }
        else
        {
            int y1 = option->rect.center().y();
            painter->drawLine(QPoint(option->rect.left() + margin, y1), QPoint(option->rect.right() - margin, y1));
        }
        painter->setPen(pen);
        return;
    }
    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        break;
    }
}

QRect FluentUI3Style::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    QRect ret;
    switch (element)
    {

    case SE_RadioButtonIndicator:
    case SE_CheckBoxIndicator:
    {
        ret = QProxyStyle::subElementRect(element, option, widget);
        ret.moveLeft(ret.left() + contentItemHMargin);
    }
    break;
    case SE_ComboBoxFocusRect:
    case SE_CheckBoxFocusRect:
    case SE_RadioButtonFocusRect:
    case SE_PushButtonFocusRect:
        ret = option->rect;
        break;
    case SE_LineEditContents:
        ret = option->rect.adjusted(4, 0, -4, 0);
        break;
    case SE_ItemViewItemCheckIndicator:
    case SE_ItemViewItemDecoration:
    case SE_ItemViewItemText:
    {
        ret = QProxyStyle::subElementRect(element, option, widget);
        if (!ret.isValid() || highContrastTheme)
        {
            return ret;
        }

        if (const QListView *lv = qobject_cast<const QListView *>(widget); lv && lv->viewMode() != QListView::IconMode)
        {
            const int xOfs = contentHMargin;
            const bool isRtl = option->direction == Qt::RightToLeft;
            if (isRtl)
            {
                ret.moveRight(ret.right() - xOfs);
                if (ret.left() < option->rect.left())
                {
                    ret.setLeft(option->rect.left());
                }
            }
            else
            {
                ret.moveLeft(ret.left() + xOfs);
                if (ret.right() > option->rect.right())
                {
                    ret.setRight(option->rect.right());
                }
            }
        }
        // Add left margin for QTreeView
        else if (const QTreeView *tv = qobject_cast<const QTreeView *>(widget))
        {
            const int xOfs = contentHMargin;
            const bool isRtl = option->direction == Qt::RightToLeft;
            if (isRtl)
            {
                ret.moveRight(ret.right() - xOfs);
                if (ret.left() < option->rect.left())
                {
                    ret.setLeft(option->rect.left());
                }
            }
            else
            {
                ret.moveLeft(ret.left() + xOfs);
                if (ret.right() > option->rect.right())
                {
                    ret.setRight(option->rect.right());
                }
            }
        }
        break;
    }
#if QT_CONFIG(progressbar)
    case SE_ProgressBarGroove:
    case SE_ProgressBarContents:
    case SE_ProgressBarLabel:
        if (const auto *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(option))
        {
            QStyleOptionProgressBar optCopy(*pb);
            // we only support label right from content
            if (widget && widget->property(ProgressBarStyleProperty).toInt() != ProgressBarRing)
            {
                optCopy.textAlignment = Qt::AlignRight;
            }

            return QProxyStyle::subElementRect(element, &optCopy, widget);
        }
        break;
#endif // QT_CONFIG(progressbar)
    case SE_HeaderLabel:
    case SE_HeaderArrow:
        ret = QProxyStyle::subElementRect(element, option, widget);
        break;
    case SE_PushButtonContents:
    {
        int border = proxy()->pixelMetric(PM_DefaultFrameWidth, option, widget);
        ret = option->rect.marginsRemoved(QMargins(border, border, border, border));
        break;
    }
    default:
        ret = QProxyStyle::subElementRect(element, option, widget);
    }
    return ret;
}

QRect FluentUI3Style::subControlRect(ComplexControl control,
                                     const QStyleOptionComplex *option,
                                     SubControl subControl,
                                     const QWidget *widget) const
{
    QRect ret;
    switch (control)
    {
#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(option))
        {
            const bool hasButtons = spinbox->buttonSymbols != QAbstractSpinBox::NoButtons;
            const int fw = spinbox->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) : 0;
            const int buttonHeight = hasButtons ? qMin(spinbox->rect.height() - 3 * fw, spinbox->fontMetrics.height() * 5 / 4) : 0;
            const QSize buttonSize(buttonHeight * 6 / 5, buttonHeight);
            const int textFieldLength = spinbox->rect.width() - 2 * fw - 2 * buttonSize.width();

            SpinBoxButtonLayout buttonLayout =
                widget ? static_cast<SpinBoxButtonLayout>(widget->property("spinBoxButtonLayout").toInt())
                       : SpinBoxButtonLayout::ArrowsVertical;
            bool horizonal = buttonLayout != SpinBoxButtonLayout::ArrowsVertical;
            QRect r = spinbox->rect.marginsRemoved(QMargins(fw, fw, fw, fw));
            if (!horizonal)
            {
                int btnHeight = qMin(r.height() / 2, spinbox->fontMetrics.height());
                if (btnHeight == r.height() / 2)
                {
                    btnHeight -= 2;
                }

                if (subControl == SC_SpinBoxUp)
                {
                    return QRect(QPoint(r.topRight().x() - buttonSize.width(), r.topRight().y() + fw * 2),
                                 QSize(buttonSize.width(), btnHeight));
                }
                if (subControl == SC_SpinBoxDown)
                {
                    return QRect(QPoint(r.topRight().x() - buttonSize.width(), r.bottomRight().y() - btnHeight - fw),
                                 QSize(buttonSize.width(), btnHeight));
                }
                if (subControl == SC_SpinBoxEditField)
                {
                    return r.marginsRemoved(QMargins(0, 0, buttonSize.width(), 0));
                }
            }

            switch (subControl)
            {
            case SC_SpinBoxUp:
            case SC_SpinBoxDown:
            {
                if (!hasButtons)
                {
                    return QRect();
                }
                const int yOfs = (r.height() - buttonSize.height()) / 2;
                if (buttonLayout == SpinBoxButtonLayout::ArrowsHorizontalRight)
                {
                    ret = QRect(r.x() + textFieldLength, r.y() + yOfs, buttonSize.width(), buttonSize.height());
                    if (subControl == SC_SpinBoxDown)
                    {
                        ret.moveRight(ret.right() + buttonSize.width() - 2 * fw);
                    }
                    break;
                }
                else // ArrowsHorizontalSides or PlusMinusHorizontalSides
                {
                    ret = QRect(
                        r.x() + 1, r.y() + (r.height() - buttonSize.height()) / 2, buttonSize.width(), buttonSize.height());
                    if (subControl == SC_SpinBoxDown)
                    {
                        ret = QRect(ret.right() + textFieldLength - 1, ret.y(), buttonSize.width(), buttonSize.height());
                    }
                    break;
                }
            }
            case SC_SpinBoxEditField:
                if (buttonLayout == SpinBoxButtonLayout::ArrowsHorizontalRight)
                {
                    ret = QRect(r.x(), r.y(), r.width() - 2 * buttonSize.width(), r.height());
                }
                else
                {
                    ret = QRect(r.x() + buttonSize.width(), r.y(), r.width() - 2 * buttonSize.width(), r.height());
                }
                break;
            case SC_SpinBoxFrame:
                ret = spinbox->rect;
            default:
                break;
            }
            ret = visualRect(spinbox->direction, spinbox->rect, ret);
        }
        break;
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *titlebar = qstyleoption_cast<const QStyleOptionTitleBar *>(option))
        {
            SubControl sc = subControl;
            ret = QProxyStyle::subControlRect(control, option, subControl, widget);
            static constexpr int indent = 3;
            static constexpr int controlWidthMargin = 2;
            const int controlHeight = titlebar->rect.height();
            const int controlWidth = 46;
            const int iconSize = proxy()->pixelMetric(QStyle::PM_TitleBarButtonIconSize, option, widget);
            int offset = -(controlWidthMargin + indent);

            bool isMinimized = titlebar->titleBarState & Qt::WindowMinimized;
            bool isMaximized = titlebar->titleBarState & Qt::WindowMaximized;

            switch (sc)
            {
            case SC_TitleBarLabel:
                if (titlebar->titleBarFlags & (Qt::WindowTitleHint | Qt::WindowSystemMenuHint))
                {
                    ret = titlebar->rect;
                    if (titlebar->titleBarFlags & Qt::WindowSystemMenuHint)
                    {
                        ret.adjust(iconSize + controlWidthMargin + indent, 0, -controlWidth, 0);
                    }
                    if (titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint)
                    {
                        ret.adjust(0, 0, -controlWidth, 0);
                    }
                    if (titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint)
                    {
                        ret.adjust(0, 0, -controlWidth, 0);
                    }
                    if (titlebar->titleBarFlags & Qt::WindowShadeButtonHint)
                    {
                        ret.adjust(0, 0, -controlWidth, 0);
                    }
                    if (titlebar->titleBarFlags & Qt::WindowContextHelpButtonHint)
                    {
                        ret.adjust(0, 0, -controlWidth, 0);
                    }
                }
                break;
            case SC_TitleBarContextHelpButton:
                if (titlebar->titleBarFlags & Qt::WindowContextHelpButtonHint)
                {
                    offset += controlWidth;
                }
                Q_FALLTHROUGH();
            case SC_TitleBarMinButton:
                if (!isMinimized && (titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint))
                {
                    offset += controlWidth;
                }
                else if (sc == SC_TitleBarMinButton)
                {
                    break;
                }
                Q_FALLTHROUGH();
            case SC_TitleBarNormalButton:
                if (isMinimized && (titlebar->titleBarFlags & Qt::WindowMinimizeButtonHint))
                {
                    offset += controlWidth;
                }
                else if (isMaximized && (titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint))
                {
                    offset += controlWidth;
                }
                else if (sc == SC_TitleBarNormalButton)
                {
                    break;
                }
                Q_FALLTHROUGH();
            case SC_TitleBarMaxButton:
                if (!isMaximized && (titlebar->titleBarFlags & Qt::WindowMaximizeButtonHint))
                {
                    offset += controlWidth;
                }
                else if (sc == SC_TitleBarMaxButton)
                {
                    break;
                }
                Q_FALLTHROUGH();
            case SC_TitleBarShadeButton:
                if (!isMinimized && (titlebar->titleBarFlags & Qt::WindowShadeButtonHint))
                {
                    offset += controlWidth;
                }
                else if (sc == SC_TitleBarShadeButton)
                {
                    break;
                }
                Q_FALLTHROUGH();
            case SC_TitleBarUnshadeButton:
                if (isMinimized && (titlebar->titleBarFlags & Qt::WindowShadeButtonHint))
                {
                    offset += controlWidth;
                }
                else if (sc == SC_TitleBarUnshadeButton)
                {
                    break;
                }
                Q_FALLTHROUGH();
            case SC_TitleBarCloseButton:
                if (titlebar->titleBarFlags & Qt::WindowSystemMenuHint)
                {
                    offset += controlWidth;
                }
                else if (sc == SC_TitleBarCloseButton)
                {
                    break;
                }
                ret.setRect(titlebar->rect.right() - offset, titlebar->rect.top(), controlWidth, controlHeight);
                break;
            case SC_TitleBarSysMenu:
                if (titlebar->titleBarFlags & Qt::WindowSystemMenuHint)
                {
                    const auto yOfs = titlebar->rect.top() + (titlebar->rect.height() - iconSize) / 2;
                    ret.setRect(titlebar->rect.left() + controlWidthMargin + indent, yOfs, iconSize, iconSize);
                }
                break;
            default:
                break;
            }
            if (widget && isMinimized && titlebar->rect.width() < offset)
            {
                const_cast<QWidget *>(widget)->resize(controlWidthMargin + indent + offset + iconSize + controlWidthMargin,
                                                      controlWidth);
            }
            ret = visualRect(titlebar->direction, titlebar->rect, ret);
        }
        break;
#endif // Qt_NO_SPINBOX
    case CC_ScrollBar:
    {
        ret = QProxyStyle::subControlRect(control, option, subControl, widget);

        if (subControl == SC_ScrollBarAddLine || subControl == SC_ScrollBarSubLine)
        {
            if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option))
            {
                if (scrollbar->orientation == Qt::Vertical)
                {
                    ret = ret.adjusted(2, 2, -2, -3);
                }
                else
                {
                    ret = ret.adjusted(3, 2, -2, -2);
                }
            }
        }
        break;
    }
#if 0
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
#else
    case CC_ComboBox:
    {
        if (const auto *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option))
        {
            const auto indicatorWidth = proxy()->pixelMetric(PM_MenuButtonIndicator, option, widget);
            switch (subControl)
            {
            case SC_ComboBoxArrow:
            {
                const int fw = cb->frame ? proxy()->pixelMetric(PM_ComboBoxFrameWidth, cb, widget) : 0;
                const int buttonHeight = qMin(cb->rect.height() - 3 * fw, cb->fontMetrics.height() * 5 / 4);
                const QSize buttonSize(buttonHeight * 6 / 5, buttonHeight);
                const int textFieldLength = cb->rect.width() - 2 * fw - buttonSize.width();
                const QPoint topLeft(cb->rect.topLeft() + QPoint(fw, fw));
                const int yOfs = ((cb->rect.height() - 2 * fw) - buttonSize.height()) / 2;
                ret = QRect(topLeft.x() + textFieldLength, topLeft.y() + yOfs, buttonSize.width(), buttonSize.height());
                ret = visualRect(option->direction, option->rect, ret);
                break;
            }
            case SC_ComboBoxEditField:
            {
                ret = option->rect;
                if (cb->frame)
                {
                    const int fw = proxy()->pixelMetric(PM_ComboBoxFrameWidth, cb, widget);
                    ret = ret.marginsRemoved(QMargins(fw, fw, fw, fw));
                }
                ret.setWidth(ret.width() - indicatorWidth - contentHMargin * 2);
                ret = visualRect(option->direction, option->rect, ret);
                break;
            }
            default:
                ret = QProxyStyle::subControlRect(control, option, subControl, widget);
                break;
            }
        }
        break;
    }
#endif
#if QT_CONFIG(groupbox)
    case CC_GroupBox:
    {
        ret = QProxyStyle::subControlRect(control, option, subControl, widget);
        switch (subControl)
        {
        case SC_GroupBoxCheckBox:
            ret.moveTop(1);
            break;
        default:
            break;
        }
        break;
    }
#endif // QT_CONFIG(groupbox)
#if QT_CONFIG(toolbutton)
    case CC_ToolButton:
        if (const QStyleOptionToolButton *tb = qstyleoption_cast<const QStyleOptionToolButton *>(option))
        {
            int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, tb, widget);
            ret = tb->rect;
            switch (subControl)
            {
            case SC_ToolButton:
                if ((tb->features & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay)) == QStyleOptionToolButton::MenuButtonPopup)
                {
                    ret.adjust(0, 0, -mbi, 0);
                }
                break;
            case SC_ToolButtonMenu:
                if ((tb->features & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay)) == QStyleOptionToolButton::MenuButtonPopup)
                {
                    ret.adjust(ret.width() - mbi, 0, 0, 0);
                }
                else if (tb->features & QStyleOptionToolButton::HasMenu)
                {
                    ret.adjust(ret.width() - mbi - 12, 0, 0, 0);
                }
                break;
            default:
                break;
            }
            ret = visualRect(tb->direction, tb->rect, ret);
        }
        break;
#endif // QT_CONFIG(toolbutton)
    default:
        ret = QProxyStyle::subControlRect(control, option, subControl, widget);
    }
    return ret;
}

void FluentUI3Style::drawTabBarTabShape(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option))
    {
        TabBarStyle tabBarStyle = widget ? static_cast<TabBarStyle>(widget->property("tabBarStyle").toInt()) : TabBarStyle::Capsule;
        if (tabBarStyle == TabBarStyle::Pivot_Grow)
        {
            drawPivotGrowingTab(tab, painter, widget);
        }
        else if (tabBarStyle == TabBarStyle::Pivot_Stretch)
        {
            drawPivotStretchingTab(tab, painter, widget);
        }
        else if (tabBarStyle == TabBarStyle::Pivot_Slide)
        {
            drawPivotSlidingTab(tab, painter, widget);
        }
        else if (tabBarStyle == TabBarStyle::Capsule)
        {
            drawCapsuleTab(tab, painter, widget);
        }
        else if (tabBarStyle == TabBarStyle::PillTabs)
        {
            drawPillTab(tab, painter, widget);
        }
        else if (tabBarStyle == TabBarStyle::Segmented_Slide)
        {
            drawSegmentedSlideTab(tab, painter, widget);
        }
        else if (tabBarStyle == TabBarStyle::Segmented_Fade)
        {
            drawSegmentedFadeTab(tab, painter, widget);
        }
        else if (tabBarStyle == TabBarStyle::Segmented_WinUI3)
        {
            drawSegmentedWinUI3Tab(tab, painter, widget);
        }
        else if (tabBarStyle == TabBarStyle::Navigation)
        {
            drawNavigationTab(tab, painter, widget);
        }
        else
        {
            drawCapsuleTab(tab, painter, widget);
        }
    }
}

void FluentUI3Style::drawCapsuleTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const
{
    Q_UNUSED(widget)
#if QT_CONFIG(tabbar)
    // QRectF tabRect = tab->rect.marginsRemoved( QMargins( 2, 2, 0, 0 ) );
    QRectF tabRect = tab->rect.marginsRemoved(QMargins(0, 0, 0, 0));
    painter->setPen(Qt::NoPen);
    painter->setBrush(tab->palette.base());

    const qreal radius = 7;
    if (tab->state & State_Selected)
    {
        painter->setBrush(winUI3Color(tabBarSelectedBackground));

        const int arcLength = 14;
        QList<QPointF> pts;
        pts << QPointF(tabRect.bottomLeft().x() - arcLength, tabRect.bottomLeft().y()) << tabRect.bottomLeft() << tabRect.topLeft()
            << tabRect.topRight() << tabRect.bottomRight() << QPointF(tabRect.bottomRight().x() + arcLength, tabRect.bottomRight().y());

        QPainterPath path = buildRoundedPolyline(pts, radius);
        painter->fillPath(path, painter->brush());

        // QRectF indicatorRect(tabRect.x() + 7.f, tabRect.y() + 7.0f, 2, tabRect.height() - 14.0f);
        // const QColor col = accentColor(tab);
        // painter->setBrush(col);
        // painter->setPen(col);
        // painter->drawRoundedRect(indicatorRect, 1.0, 1.0);
    }
    else if (tab->state & State_MouseOver)
    {
        painter->setBrush(winUI3Color(tabBarHoverBackground));

        QList<QPointF> pts;
        pts << tabRect.bottomLeft() << tabRect.topLeft() << tabRect.topRight() << tabRect.bottomRight();

        QPainterPath path = buildRoundedPolyline(pts, radius);
        pts << tabRect.bottomLeft();
        painter->drawPath(path);
    }
    else
    {
        painter->setBrush(tab->palette.window());
    }

    painter->setBrush(Qt::NoBrush);
    painter->setPen(highContrastTheme == true ? tab->palette.buttonText().color() : WINUI3Colors[colorSchemeIndex][frameColorLight]);
#endif // QT_CONFIG(tabbar)
}

void FluentUI3Style::drawPivotGrowingTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const
{
#if QT_CONFIG(tabbar)
    if (!tab || !tab->state.testFlag(QStyle::State_Selected))
    {
        return;
    }

    QObject *styleObject = tab->styleObject;
    if (!styleObject)
    {
        return;
    }

    constexpr qreal indicatorHeight = 3.0;
    constexpr int indicatorMargin = 15;
    const QByteArray animKey = "_q_pivot_indicator_grow";

    const QRect r = tab->rect;
    const int innerWidth = qMax(0, r.width() - indicatorMargin * 2);
    const int targetWidth = qMin(pivotIndicatorPreferredWidth, innerWidth);
    const qreal contentLeft = r.left() + indicatorMargin + (innerWidth - targetWidth) / 2.0;

    int currentTabIndex = -1;
    if (const QTabBar *tabBar = qobject_cast<const QTabBar *>(widget))
    {
        currentTabIndex = tabBar->tabAt(r.center());
    }

    const int previousTabIndex = styleObject->property("_q_pivot_grow_selected_tab_index").toInt();

    bool isMovingTab = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    isMovingTab = tab->position == QStyleOptionTab::TabPosition::Moving;
#endif

    bool selectionChanged = (currentTabIndex >= 0 && previousTabIndex >= 0 && currentTabIndex != previousTabIndex);
    if (isMovingTab)
    {
        selectionChanged = false;
    }

    if (selectionChanged)
    {
        QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
        t->setStartValue(0.0);
        t->setEndValue(1.0);
        t->setDuration(300);
        startAnimationEx(t, styleObject, animKey);
    }

    qreal progress = animationValue(styleObject, animKey, 1.0f);
    if (progress < 0.0)
    {
        progress = 0.0;
    }
    else if (progress > 1.0)
    {
        progress = 1.0;
    }

    const qreal currentWidth = targetWidth * progress;
    const QRect indicator(qRound(contentLeft + (targetWidth - currentWidth) / 2.0),
                          qRound(r.bottom() - indicatorHeight),
                          qRound(currentWidth),
                          qRound(indicatorHeight));

    styleObject->setProperty("_q_pivot_grow_selected_tab_index", currentTabIndex);

    painter->setPen(Qt::NoPen);
    painter->setBrush(calculateAccentColor(tab));
    painter->drawRoundedRect(indicator, indicatorHeight / 2.0, indicatorHeight / 2.0);
#else
    Q_UNUSED(tab)
    Q_UNUSED(painter)
    Q_UNUSED(widget)
#endif
}

void FluentUI3Style::drawPivotStretchingTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const
{
#if QT_CONFIG(tabbar)
    if (!tab || !tab->state.testFlag(QStyle::State_Selected))
    {
        return;
    }

    QObject *styleObject = tab->styleObject;
    if (!styleObject)
    {
        return;
    }

    constexpr qreal indicatorHeight = 3.0;
    constexpr int indicatorMargin = 20;
    const QByteArray animKey = "_q_pivot_indicator_slide";
    const auto clamp01 = [](qreal value)
    { return qBound(0.0, value, 1.0); };
    const auto lerp = [](qreal a, qreal b, qreal t)
    { return a + (b - a) * t; };
    const auto smoothStep = [&](qreal t)
    {
        const qreal clamped = clamp01(t);
        return clamped * clamped * (3.0 - 2.0 * clamped);
    };
    const auto easeOutCubic = [&](qreal t)
    {
        const qreal clamped = clamp01(t);
        const qreal inv = 1.0 - clamped;
        return 1.0 - inv * inv * inv;
    };
    const auto easeOutBack = [&](qreal t)
    {
        const qreal clamped = clamp01(t);
        const qreal x = clamped - 1.0;
        constexpr qreal s = 1.15;
        return 1.0 + (s + 1.0) * x * x * x + s * x * x;
    };
    const auto delayedProgress = [&](qreal t, qreal delayPortion)
    {
        const qreal clamped = clamp01(t);
        if (clamped <= delayPortion)
        {
            return 0.0;
        }

        return clamp01((clamped - delayPortion) / (1.0 - delayPortion));
    };
    const auto releaseProgress = [&](qreal t, qreal delayPortion)
    {
        const qreal delayed = delayedProgress(t, delayPortion);
        if (delayed <= 0.0)
        {
            return 0.0;
        }

        return easeOutCubic(delayed * delayed);
    };
    const auto settleProgress = [&](qreal t, qreal delayPortion)
    {
        const qreal delayed = delayedProgress(t, delayPortion);
        if (delayed <= 0.0)
        {
            return 0.0;
        }

        const qreal shaped = delayed * delayed * delayed;
        return smoothStep(shaped);
    };

    const QRect r = tab->rect;
    const int innerWidth = qMax(0, r.width() - indicatorMargin * 2);
    const int targetWidth = qMin(pivotIndicatorPreferredWidth, innerWidth);

    const qreal targetLeft = r.left() + indicatorMargin + (innerWidth - targetWidth) / 2.0;
    const qreal targetTop = r.bottom() - indicatorHeight;
    const qreal targetW = targetWidth;
    const qreal targetH = indicatorHeight;
    const auto indicatorRect = [&](qreal startLeft,
                                   qreal startTop,
                                   qreal startWidth,
                                   qreal startHeight,
                                   qreal endLeft,
                                   qreal endTop,
                                   qreal endWidth,
                                   qreal endHeight,
                                   qreal progressValue)
    {
        const qreal p = clamp01(progressValue);
        const qreal startRight = startLeft + startWidth;
        const qreal endRight = endLeft + endWidth;
        const bool movingRight = endLeft >= startLeft;

        const qreal startTabLeft = startLeft - indicatorMargin;
        const qreal startTabRight = startRight + indicatorMargin;
        [[maybe_unused]] const qreal endTabLeft = endLeft - indicatorMargin;
        [[maybe_unused]] const qreal endTabRight = endRight + indicatorMargin;

        qreal drawLeft = startLeft;
        qreal drawRight = startRight;

        if (qAbs(endLeft - startLeft) < 0.5)
        {
            const qreal t = smoothStep(p);
            drawLeft = lerp(startLeft, endLeft, t);
            drawRight = lerp(startRight, endRight, t);
        }
        else if (p < 0.66)
        {
            const qreal stretchT = releaseProgress(p / 0.66, 0.34);

            if (movingRight)
            {
                drawLeft = startLeft;
                drawRight = lerp(startRight, startTabRight, stretchT);
            }
            else
            {
                drawLeft = lerp(startLeft, startTabLeft, stretchT);
                drawRight = startRight;
            }
        }
        else
        {
            const qreal settleT = settleProgress((p - 0.66) / 0.34, 0.04);
            const qreal widthT = easeOutCubic(settleT);
            const qreal settleBackT = easeOutBack(settleT);
            const qreal retractSpan = qMin(endWidth * 0.32, qreal(20.0));
            const qreal microBounce = qSin(settleT * M_PI) * (1.0 - settleT) * 0.9;

            if (movingRight)
            {
                drawLeft = lerp(endLeft - retractSpan, endLeft, widthT);
                drawRight = lerp(endRight + microBounce, endRight, settleBackT);
            }
            else
            {
                drawLeft = lerp(endLeft - microBounce, endLeft, settleBackT);
                drawRight = lerp(endRight + retractSpan, endRight, widthT);
            }
        }

        if (drawRight < drawLeft)
        {
            qSwap(drawLeft, drawRight);
        }

        return QRectF(drawLeft,
                      lerp(startTop, endTop, smoothStep(p)),
                      qMax(0.0, drawRight - drawLeft),
                      lerp(startHeight, endHeight, smoothStep(p)));
    };

    int currentTabIndex = -1;
    if (const QTabBar *tabBar = qobject_cast<const QTabBar *>(widget))
    {
        currentTabIndex = tabBar->tabAt(r.center());
    }

    const int previousTabIndex = styleObject->property("_q_pivot_selected_tab_index").toInt();
    bool isMovingTab = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    isMovingTab = tab->position == QStyleOptionTab::TabPosition::Moving;
#endif

    qreal fromLeft = styleObject->property("_q_pivot_indicator_from_left").toReal();
    qreal fromTop = styleObject->property("_q_pivot_indicator_from_top").toReal();
    qreal fromW = styleObject->property("_q_pivot_indicator_from_width").toReal();
    qreal fromH = styleObject->property("_q_pivot_indicator_from_height").toReal();
    qreal toLeft = styleObject->property("_q_pivot_indicator_to_left").toReal();
    qreal toTop = styleObject->property("_q_pivot_indicator_to_top").toReal();
    qreal toW = styleObject->property("_q_pivot_indicator_to_width").toReal();
    qreal toH = styleObject->property("_q_pivot_indicator_to_height").toReal();

    if (toW <= 0.0)
    {
        fromLeft = targetLeft;
        fromTop = targetTop;
        fromW = targetW;
        fromH = targetH;
        toLeft = targetLeft;
        toTop = targetTop;
        toW = targetW;
        toH = targetH;
    }

    bool selectionChanged = (currentTabIndex >= 0 && previousTabIndex >= 0 && currentTabIndex != previousTabIndex);
    if (isMovingTab)
    {
        selectionChanged = false;
    }

    if (selectionChanged)
    {
        fromLeft = toLeft;
        fromTop = toTop;
        fromW = toW;
        fromH = toH;
        toLeft = targetLeft;
        toTop = targetTop;
        toW = targetW;
        toH = targetH;

        QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
        t->setStartValue(0.0);
        t->setEndValue(1.0);
        t->setDuration(450);
        startAnimationEx(t, styleObject, animKey);
    }
    else if (currentTabIndex < 0 || previousTabIndex < 0)
    {
        fromLeft = targetLeft;
        fromTop = targetTop;
        fromW = targetW;
        fromH = targetH;
        toLeft = targetLeft;
        toTop = targetTop;
        toW = targetW;
        toH = targetH;
    }

    const qreal progress = clamp01(animationValue(styleObject, animKey, 1.0f));
    const QRectF indicatorRectF = indicatorRect(fromLeft, fromTop, fromW, fromH, toLeft, toTop, toW, toH, progress);

    styleObject->setProperty("_q_pivot_selected_tab_index", currentTabIndex);
    styleObject->setProperty("_q_pivot_indicator_from_left", fromLeft);
    styleObject->setProperty("_q_pivot_indicator_from_top", fromTop);
    styleObject->setProperty("_q_pivot_indicator_from_width", fromW);
    styleObject->setProperty("_q_pivot_indicator_from_height", fromH);
    styleObject->setProperty("_q_pivot_indicator_to_left", toLeft);
    styleObject->setProperty("_q_pivot_indicator_to_top", toTop);
    styleObject->setProperty("_q_pivot_indicator_to_width", toW);
    styleObject->setProperty("_q_pivot_indicator_to_height", toH);

    QRect indicator(qRound(indicatorRectF.left()),
                    qRound(indicatorRectF.top()),
                    qRound(indicatorRectF.width()),
                    qRound(indicatorRectF.height()));
    painter->setPen(Qt::NoPen);
    painter->setBrush(calculateAccentColor(tab));
    painter->drawRoundedRect(indicator, indicatorHeight / 2.0, indicatorHeight / 2.0);
#else
    Q_UNUSED(tab)
    Q_UNUSED(painter)
    Q_UNUSED(widget)
#endif
}

void FluentUI3Style::drawPivotSlidingTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const
{
#if QT_CONFIG(tabbar)
    if (!tab || !tab->state.testFlag(QStyle::State_Selected))
    {
        return;
    }

    QObject *styleObject = tab->styleObject;
    if (!styleObject)
    {
        return;
    }

    constexpr qreal indicatorHeight = 3.0;
    constexpr int indicatorMargin = 15;
    const QByteArray animKey = "_q_pivot_indicator_slide_tab";
    const auto clamp01 = [](qreal value)
    { return qBound(0.0, value, 1.0); };
    const auto lerp = [](qreal a, qreal b, qreal t)
    { return a + (b - a) * t; };
    const auto smoothStep = [&](qreal t)
    {
        const qreal clamped = clamp01(t);
        return clamped * clamped * (3.0 - 2.0 * clamped);
    };
    const auto easeOutCubic = [&](qreal t)
    {
        const qreal clamped = clamp01(t);
        const qreal inv = 1.0 - clamped;
        return 1.0 - inv * inv * inv;
    };

    const QRect r = tab->rect;
    const int innerWidth = qMax(0, r.width() - indicatorMargin * 2);
    const int targetWidth = qMin(pivotIndicatorPreferredWidth, innerWidth);

    const qreal targetLeft = r.left() + indicatorMargin + (innerWidth - targetWidth) / 2.0;
    const qreal targetTop = r.bottom() - indicatorHeight;
    const qreal targetRight = targetLeft + targetWidth;

    int currentTabIndex = -1;
    if (const QTabBar *tabBar = qobject_cast<const QTabBar *>(widget))
    {
        currentTabIndex = tabBar->tabAt(r.center());
    }

    const int previousTabIndex = styleObject->property("_q_pivot_slide_selected_tab_index").toInt();
    bool isMovingTab = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    isMovingTab = tab->position == QStyleOptionTab::TabPosition::Moving;
#endif

    qreal fromLeft = styleObject->property("_q_pivot_slide_from_left").toReal();
    qreal fromRight = styleObject->property("_q_pivot_slide_from_right").toReal();
    qreal fromTop = styleObject->property("_q_pivot_slide_from_top").toReal();
    qreal toLeft = styleObject->property("_q_pivot_slide_to_left").toReal();
    qreal toRight = styleObject->property("_q_pivot_slide_to_right").toReal();
    qreal toTop = styleObject->property("_q_pivot_slide_to_top").toReal();

    if (toRight <= toLeft)
    {
        fromLeft = targetLeft;
        fromRight = targetRight;
        fromTop = targetTop;
        toLeft = targetLeft;
        toRight = targetRight;
        toTop = targetTop;
    }

    bool selectionChanged = (currentTabIndex >= 0 && previousTabIndex >= 0 && currentTabIndex != previousTabIndex);
    if (isMovingTab)
    {
        selectionChanged = false;
    }

    if (selectionChanged)
    {
        fromLeft = toLeft;
        fromRight = toRight;
        fromTop = toTop;
        toLeft = targetLeft;
        toRight = targetRight;
        toTop = targetTop;

        QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
        t->setStartValue(0.0);
        t->setEndValue(1.0);
        t->setDuration(200);
        startAnimationEx(t, styleObject, animKey);
    }
    else if (currentTabIndex < 0 || previousTabIndex < 0)
    {
        fromLeft = targetLeft;
        fromRight = targetRight;
        fromTop = targetTop;
        toLeft = targetLeft;
        toRight = targetRight;
        toTop = targetTop;
    }

    const qreal progress = clamp01(animationValue(styleObject, animKey, 1.0f));
    const qreal fromWidth = qMax(0.0, fromRight - fromLeft);
    const qreal toWidth = qMax(0.0, toRight - toLeft);
    const qreal fromCenter = fromLeft + fromWidth / 2.0;
    const qreal toCenter = toLeft + toWidth / 2.0;
    // Fixed bar width while sliding (no stretch / pulse); min keeps bar inside both tab slots if widths differ
    const qreal barW = qMin(fromWidth, toWidth);
    const qreal center = lerp(fromCenter, toCenter, easeOutCubic(progress));
    const qreal drawLeft = center - barW / 2.0;

    const QRectF indicatorRectF(drawLeft, lerp(fromTop, toTop, smoothStep(progress)), barW, indicatorHeight);

    styleObject->setProperty("_q_pivot_slide_selected_tab_index", currentTabIndex);
    styleObject->setProperty("_q_pivot_slide_from_left", fromLeft);
    styleObject->setProperty("_q_pivot_slide_from_right", fromRight);
    styleObject->setProperty("_q_pivot_slide_from_top", fromTop);
    styleObject->setProperty("_q_pivot_slide_to_left", toLeft);
    styleObject->setProperty("_q_pivot_slide_to_right", toRight);
    styleObject->setProperty("_q_pivot_slide_to_top", toTop);

    painter->setPen(Qt::NoPen);
    painter->setBrush(calculateAccentColor(tab));
    painter->drawRoundedRect(indicatorRectF, indicatorHeight / 2.0, indicatorHeight / 2.0);
#else
    Q_UNUSED(tab)
    Q_UNUSED(painter)
    Q_UNUSED(widget)
#endif
}

void FluentUI3Style::drawPillTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const
{
    Q_UNUSED(widget)
#if QT_CONFIG(tabbar)
    const bool isSelected = tab->state.testFlag(QStyle::State_Selected);
    const bool isHover = tab->state.testFlag(QStyle::State_MouseOver);
    [[maybe_unused]] const bool isEnabled = tab->state.testFlag(QStyle::State_Enabled);
    const QRectF tabRect = tab->rect.marginsRemoved(QMargins(2, 2, 2, 2));
    const qreal radius = 2;

    QColor hoverColor = winUI3Color(tabBarHoverBackground);
    QColor selectedColor = winUI3Color(tabBarSelectedBackground);
    QColor fillColor = isSelected ? selectedColor : (isHover ? hoverColor : tab->palette.window().color());

    painter->setPen(WINUI3Colors[colorSchemeIndex][controlStrokePrimary]);
    painter->setBrush(fillColor);
    painter->drawRoundedRect(tabRect, radius, radius);
#else
    Q_UNUSED(tab)
    Q_UNUSED(painter)
    Q_UNUSED(widget)
#endif
}

void FluentUI3Style::drawSegmentedSlideTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const
{
#if QT_CONFIG(tabbar)
    if (!tab || !painter)
    {
        return;
    }

    QObject *styleObject = tab->styleObject;
    if (!styleObject)
    {
        return;
    }

    const auto clamp01 = [](qreal value)
    { return qBound(0.0, value, 1.0); };
    const auto lerp = [](qreal a, qreal b, qreal t)
    { return a + (b - a) * t; };
    const auto lerpRect = [&](const QRectF &from, const QRectF &to, qreal t)
    {
        return QRectF(lerp(from.left(), to.left(), t),
                      lerp(from.top(), to.top(), t),
                      lerp(from.width(), to.width(), t),
                      lerp(from.height(), to.height(), t));
    };

    const bool isSelected = tab->state.testFlag(QStyle::State_Selected);
    const bool isPressed = tab->state.testFlag(QStyle::State_Sunken) && !isSelected;
    const bool isHover = tab->state.testFlag(QStyle::State_MouseOver) && !isSelected && !isPressed;
    const bool isFirstSegment = tab->position == QStyleOptionTab::Beginning || tab->position == QStyleOptionTab::OnlyOneTab;
    const bool isLastSegment = tab->position == QStyleOptionTab::End || tab->position == QStyleOptionTab::OnlyOneTab;
    constexpr int selectionOuterMarginH = 4;
    constexpr int selectionInnerSpacingH = 4;
    constexpr int selectionMarginV = 4;

    const bool segmentedSemiRound = segmentedBoolProperty(widget, SegmentedSemiRoundProperty, false);
    const QColor hoverColor = segmentedThemeColorProperty(
        widget, colorSchemeIndex == 1, SegmentedHoverColorProperty, SegmentedHoverColorDarkProperty, winUI3Color(subtleHighlightColor));
    const QColor pressedColor = segmentedThemeColorProperty(
        widget, colorSchemeIndex == 1, SegmentedPressedColorProperty, SegmentedPressedColorDarkProperty, winUI3Color(subtlePressedColor));
    const QColor selectedColor = segmentedThemeColorProperty(
        widget,
        colorSchemeIndex == 1,
        SegmentedSelectedColorProperty,
        SegmentedSelectedColorDarkProperty,
        highContrastTheme ? tab->palette.highlight().color() : winUI3Color(tabBarSelectedBackground));

    PainterStateGuard guard(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);

    auto beginRect = styleObject->property("BeginRect").toRect();
    if (isFirstSegment && beginRect != tab->rect)
    {
        styleObject->setProperty("BeginRect", tab->rect);
    }

    auto endRect = styleObject->property("EndRect").toRect();
    if (isLastSegment && endRect != tab->rect)
    {
        styleObject->setProperty("EndRect", tab->rect);
    }

    const int leftInset = isFirstSegment ? selectionOuterMarginH : selectionInnerSpacingH / 2;
    const int rightInset = isLastSegment ? selectionOuterMarginH : selectionInnerSpacingH / 2;
    const QRectF adjustedTabRect = QRectF(tab->rect).adjusted(leftInset, selectionMarginV, -rightInset, -selectionMarginV);

    painter->setPen(Qt::NoPen);
    if (isPressed)
    {
        painter->setBrush(pressedColor);
        const qreal tabRadius = segmentedSemiRound ? adjustedTabRect.height() / 2.0 : secondLevelRoundingRadius;
        painter->drawRoundedRect(adjustedTabRect, tabRadius, tabRadius);
    }
    else if (isHover)
    {
        painter->setBrush(hoverColor);
        const qreal tabRadius = segmentedSemiRound ? adjustedTabRect.height() / 2.0 : secondLevelRoundingRadius;
        painter->drawRoundedRect(adjustedTabRect, tabRadius, tabRadius);
    }

    const QTabBar *tabBar = qobject_cast<const QTabBar *>(widget);
    if (!tabBar)
    {
        return;
    }

    const int currentTabIndex = tabBar->currentIndex();
    const int previousTabIndex = styleObject->property("_q_segmented_selected_tab_index").toInt();
    if (currentTabIndex < 0)
    {
        return;
    }

    const bool targetIsFirst = currentTabIndex == 0;
    const bool targetIsLast = currentTabIndex == tabBar->count() - 1;
    const int targetLeftInset = targetIsFirst ? selectionOuterMarginH : selectionInnerSpacingH / 2;
    const int targetRightInset = targetIsLast ? selectionOuterMarginH : selectionInnerSpacingH / 2;
    const QRect targetRect =
        tabBar->tabRect(currentTabIndex).adjusted(targetLeftInset, selectionMarginV, -targetRightInset, -selectionMarginV);
    QRect previousRect = styleObject->property("_q_segmented_previous_selected_rect").toRect();
    QRect currentRect = styleObject->property("_q_segmented_current_selected_rect").toRect();
    const QByteArray animKey = "_q_segmented_slide_rect";

    if (!currentRect.isValid())
    {
        currentRect = targetRect;
    }
    if (!previousRect.isValid())
    {
        previousRect = currentRect;
    }

    bool selectionChanged = (currentTabIndex >= 0 && previousTabIndex >= 0 && currentTabIndex != previousTabIndex);
    if (selectionChanged)
    {
        previousRect = currentRect;
        currentRect = targetRect;

        QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
        t->setStartValue(0.0);
        t->setEndValue(1.0);
        t->setDuration(300);
        startAnimationEx(t, styleObject, animKey);
    }
    else if (previousTabIndex < 0)
    {
        previousRect = targetRect;
        currentRect = targetRect;
    }
    else
    {
        currentRect = targetRect;
    }

    const qreal progress = clamp01(animationValue(styleObject, animKey, 1.0f));
    QRectF drawRect = lerpRect(QRectF(previousRect), QRectF(currentRect), progress);
    styleObject->setProperty("_q_segmented_selected_tab_index", currentTabIndex);
    styleObject->setProperty("_q_segmented_previous_selected_rect", previousRect);
    styleObject->setProperty("_q_segmented_current_selected_rect", currentRect);

    const QColor selectedBrush = selectedColor;
    painter->setBrush(selectedBrush);
    drawRect.adjust(0.5, 0.5, -0.5, -0.5);
    
    painter->save();
    painter->setClipRect(QRect(tab->rect).adjusted(-2, 0, 2, 0));
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    const qreal selectedRadius = segmentedSemiRound ? drawRect.height() / 2.0 : secondLevelRoundingRadius;
    painter->drawRoundedRect(drawRect, selectedRadius, selectedRadius);
    painter->restore();
#else
    Q_UNUSED(tab)
    Q_UNUSED(painter)
    Q_UNUSED(widget)
#endif
}

void FluentUI3Style::drawSegmentedFadeTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const
{
#if QT_CONFIG(tabbar)
    if (!tab || !painter)
    {
        return;
    }

    QObject *styleObject = tab->styleObject;
    if (!styleObject)
    {
        return;
    }

    const auto clamp01 = [](qreal value)
    { return qBound(0.0, value, 1.0); };

    const bool isSelected = tab->state.testFlag(QStyle::State_Selected);
    const bool isPressed = tab->state.testFlag(QStyle::State_Sunken) && !isSelected;
    const bool isHover = tab->state.testFlag(QStyle::State_MouseOver) && !isSelected && !isPressed;
    const bool isFirstSegment = tab->position == QStyleOptionTab::Beginning || tab->position == QStyleOptionTab::OnlyOneTab;
    const bool isLastSegment = tab->position == QStyleOptionTab::End || tab->position == QStyleOptionTab::OnlyOneTab;
    constexpr int selectionOuterMarginH = 4;
    constexpr int selectionInnerSpacingH = 4;
    constexpr int selectionMarginV = 4;

    const bool segmentedSemiRound = segmentedBoolProperty(widget, SegmentedSemiRoundProperty, false);
    const QColor hoverColor = segmentedThemeColorProperty(
        widget, colorSchemeIndex == 1, SegmentedHoverColorProperty, SegmentedHoverColorDarkProperty, winUI3Color(subtleHighlightColor));
    const QColor pressedColor = segmentedThemeColorProperty(
        widget, colorSchemeIndex == 1, SegmentedPressedColorProperty, SegmentedPressedColorDarkProperty, winUI3Color(subtlePressedColor));
    const QColor selectedColor = segmentedThemeColorProperty(
        widget,
        colorSchemeIndex == 1,
        SegmentedSelectedColorProperty,
        SegmentedSelectedColorDarkProperty,
        highContrastTheme ? tab->palette.highlight().color() : winUI3Color(tabBarSelectedBackground));

    PainterStateGuard guard(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);

    const int leftInset = isFirstSegment ? selectionOuterMarginH : selectionInnerSpacingH / 2;
    const int rightInset = isLastSegment ? selectionOuterMarginH : selectionInnerSpacingH / 2;
    const QRectF adjustedTabRect = QRectF(tab->rect).adjusted(leftInset, selectionMarginV, -rightInset, -selectionMarginV);

    painter->setPen(Qt::NoPen);
    if (isPressed)
    {
        painter->setBrush(pressedColor);
        const qreal tabRadius = segmentedSemiRound ? adjustedTabRect.height() / 2.0 : secondLevelRoundingRadius;
        painter->drawRoundedRect(adjustedTabRect, tabRadius, tabRadius);
    }
    else if (isHover)
    {
        painter->setBrush(hoverColor);
        const qreal tabRadius = segmentedSemiRound ? adjustedTabRect.height() / 2.0 : secondLevelRoundingRadius;
        painter->drawRoundedRect(adjustedTabRect, tabRadius, tabRadius);
    }

    const QTabBar *tabBar = qobject_cast<const QTabBar *>(widget);
    const int tabIndex = tabBar ? tabBar->tabAt(tab->rect.center()) : -1;
    const int currentTabIndex = tabBar ? tabBar->currentIndex() : -1;
    int selectedTabIndex = styleObject->property("_q_segmented_fade_selected_tab_index").toInt();
    int fadingFromIndex = styleObject->property("_q_segmented_fade_from_index").toInt();
    const QByteArray animKey = "_q_segmented_fade_fill";

    if (isSelected && currentTabIndex >= 0 && selectedTabIndex < 0)
    {
        selectedTabIndex = currentTabIndex;
        fadingFromIndex = -1;
    }
    else if (isSelected && currentTabIndex >= 0 && selectedTabIndex >= 0 && currentTabIndex != selectedTabIndex)
    {
        fadingFromIndex = selectedTabIndex;
        selectedTabIndex = currentTabIndex;

        QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
        t->setStartValue(0.0);
        t->setEndValue(1.0);
        t->setDuration(600);
        t->setFrameRate(QStyleAnimation::DefaultFps);
        t->setEasingCurve(QEasingCurve::InOutCubic);
        startAnimationEx(t, styleObject, animKey);
    }

    const qreal progress = clamp01(animationValue(styleObject, animKey, 1.0f));
    const bool isFadingBetweenTabs = fadingFromIndex >= 0 && progress < 0.999;

    bool shouldDrawSelectedFill = false;
    qreal opacity = 1.0;

    if (tabIndex == selectedTabIndex)
    {
        shouldDrawSelectedFill = true;
        opacity = isFadingBetweenTabs ? progress : 1.0;
    }
    else if (tabIndex == fadingFromIndex && isFadingBetweenTabs)
    {
        shouldDrawSelectedFill = true;
        opacity = 1.0 - progress;
    }

    if (!shouldDrawSelectedFill)
    {
        if (!isFadingBetweenTabs)
        {
            styleObject->setProperty("_q_segmented_fade_from_index", -1);
        }
        styleObject->setProperty("_q_segmented_fade_selected_tab_index", selectedTabIndex);
        return;
    }

    const QColor selectedBrush = selectedColor;
    styleObject->setProperty("_q_segmented_fade_selected_tab_index", selectedTabIndex);
    if (!isFadingBetweenTabs)
    {
        styleObject->setProperty("_q_segmented_fade_from_index", -1);
    }
    else
    {
        styleObject->setProperty("_q_segmented_fade_from_index", fadingFromIndex);
    }

    painter->setOpacity(opacity);
    painter->setBrush(selectedBrush);
    const qreal selectedRadius = segmentedSemiRound ? adjustedTabRect.height() / 2.0 : secondLevelRoundingRadius;
    painter->drawRoundedRect(adjustedTabRect, selectedRadius, selectedRadius);
#else
    Q_UNUSED(tab)
    Q_UNUSED(painter)
    Q_UNUSED(widget)
#endif
}

void FluentUI3Style::drawSegmentedWinUI3Tab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const
{
#if QT_CONFIG(tabbar)
    if (!tab || !painter)
        return;

    QObject *styleObject = tab->styleObject;
    if (!styleObject)
        return;

    const auto clamp01 = [](qreal value)
    { return qBound(0.0, value, 1.0); };

    const bool isSelected = tab->state.testFlag(QStyle::State_Selected);
    const bool isHover = tab->state.testFlag(QStyle::State_MouseOver);
    const bool isFirstSegment = tab->position == QStyleOptionTab::Beginning || tab->position == QStyleOptionTab::OnlyOneTab;
    const bool isLastSegment = tab->position == QStyleOptionTab::End || tab->position == QStyleOptionTab::OnlyOneTab;
    constexpr int selectionOuterMarginH = 0;
    constexpr int selectionInnerSpacingH = 0;
    constexpr int selectionMarginV = 0;

    PainterStateGuard guard(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QTabBar *tabBar = qobject_cast<const QTabBar *>(widget);
    int tabIndex = tabBar->tabAt(tab->rect.center());
    if (tabIndex < 0)
        return;

    const int currentTabIndex = tabBar->currentIndex();
    QVariant activeTabIndexVar = styleObject->property("_q_segmented_winui3_curr_sel");
    int activeTabIndex = activeTabIndexVar.isValid() ? activeTabIndexVar.toInt() : -1;

    QVariant previousTabIndexVar = styleObject->property("_q_segmented_winui3_prev_sel");
    int previousTabIndex = previousTabIndexVar.isValid() ? previousTabIndexVar.toInt() : -1;

    if (activeTabIndex < 0)
    {
        activeTabIndex = currentTabIndex;
        styleObject->setProperty("_q_segmented_winui3_curr_sel", activeTabIndex);
    }

    if (currentTabIndex != activeTabIndex)
    {
        previousTabIndex = activeTabIndex;
        activeTabIndex = currentTabIndex;
        styleObject->setProperty("_q_segmented_winui3_prev_sel", previousTabIndex);
        styleObject->setProperty("_q_segmented_winui3_curr_sel", activeTabIndex);

        QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
        t->setStartValue(0.0);
        t->setEndValue(1.0);
        t->setDuration(120);
        t->setEasingCurve(QEasingCurve::OutCubic);
        startAnimationEx(t, styleObject, "_q_segmented_winui3_sel_anim");
    }

    QVariant currentPressedIndexVar = styleObject->property("_q_segmented_winui3_pressed_index");
    int currentPressedIndex = currentPressedIndexVar.isValid() ? currentPressedIndexVar.toInt() : -1;

    QVariant activePressedIndexVar = styleObject->property("_q_segmented_winui3_curr_press");
    int activePressedIndex = activePressedIndexVar.isValid() ? activePressedIndexVar.toInt() : -1;

    QVariant previousPressedIndexVar = styleObject->property("_q_segmented_winui3_prev_press");
    int previousPressedIndex = previousPressedIndexVar.isValid() ? previousPressedIndexVar.toInt() : -1;

    if (currentPressedIndex != activePressedIndex)
    {
        previousPressedIndex = activePressedIndex;
        activePressedIndex = currentPressedIndex;
        styleObject->setProperty("_q_segmented_winui3_prev_press", previousPressedIndex);
        styleObject->setProperty("_q_segmented_winui3_curr_press", activePressedIndex);

        QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
        t->setStartValue(0.0);
        t->setEndValue(1.0);
        t->setDuration(120);
        t->setEasingCurve(QEasingCurve::OutCubic);
        startAnimationEx(t, styleObject, "_q_segmented_winui3_press_anim");
    }

    qreal selAnimProgress = clamp01(animationValue(styleObject, "_q_segmented_winui3_sel_anim", 1.0f));
    qreal pressAnimProgress = clamp01(animationValue(styleObject, "_q_segmented_winui3_press_anim", 1.0f));

    qreal selectProgress = 0.0;
    if (tabIndex == activeTabIndex)
        selectProgress = selAnimProgress;
    else if (tabIndex == previousTabIndex)
        selectProgress = 1.0 - selAnimProgress;

    qreal pressProgress = 0.0;
    if (tabIndex == activePressedIndex)
        pressProgress = pressAnimProgress;
    else if (tabIndex == previousPressedIndex)
        pressProgress = 1.0 - pressAnimProgress;

    const bool isPressedReal = (tabIndex == currentPressedIndex);

    const int leftInset = isFirstSegment ? selectionOuterMarginH : selectionInnerSpacingH / 2;
    const int rightInset = isLastSegment ? selectionOuterMarginH : selectionInnerSpacingH / 2;
    QRectF adjustedTabRect = QRectF(tab->rect).adjusted(leftInset, selectionMarginV, -rightInset, -selectionMarginV);

    if (!isSelected)
    {
        if (isHover || isPressedReal)
        {
            painter->setPen(Qt::NoPen);
            painter->setBrush(isPressedReal ? winUI3Color(subtlePressedColor) : winUI3Color(subtleHighlightColor));

            QRectF hoverRect = adjustedTabRect.marginsRemoved(QMarginsF(3, 3, 3, 3));
            painter->drawRoundedRect(hoverRect, secondLevelRoundingRadius, secondLevelRoundingRadius);
        }
    }

    if (selectProgress > 0.0 || isSelected)
    {
        if (isSelected)
        {
            const QColor selectedBrush = highContrastTheme ? tab->palette.highlight().color() : winUI3Color(fillControlDefault);
            painter->setBrush(selectedBrush);
            painter->setPen(QPen(highContrastTheme ? tab->palette.buttonText().color() : winUI3Color(frameColorLight), 1));

            QRectF drawRect = adjustedTabRect;
            drawRect.adjust(0.5, 0.5, -0.5, -0.5);
            painter->drawRoundedRect(drawRect, secondLevelRoundingRadius, secondLevelRoundingRadius);
        }

        qreal targetIndicatorWidth = 16.0;
        qreal currentIndicatorWidth = targetIndicatorWidth * selectProgress * (1.0 - 0.5 * pressProgress);

        if (currentIndicatorWidth > 0.5)
        {
            QRectF indicatorRect = QRectF(adjustedTabRect.x() + (adjustedTabRect.width() / 2.0 - currentIndicatorWidth / 2.0), adjustedTabRect.bottom() - 3.5, currentIndicatorWidth, 2);
            painter->setPen(calculateAccentColor(tab));
            painter->setBrush(calculateAccentColor(tab));
            painter->drawRoundedRect(indicatorRect, 1, 1);
        }
    }

#else
    Q_UNUSED(tab)
    Q_UNUSED(painter)
    Q_UNUSED(widget)
#endif
}

void FluentUI3Style::drawNavigationTab(const QStyleOptionTab *tab, QPainter *painter, const QWidget *widget) const
{
#if QT_CONFIG(tabbar)
    if (!tab || !painter)
    {
        return;
    }

    const bool isSelected = tab->state.testFlag(QStyle::State_Selected);
    const bool isHover = tab->state.testFlag(QStyle::State_MouseOver) && !isSelected;
    const bool isWest = tab->shape == QTabBar::RoundedWest || tab->shape == QTabBar::TriangularWest;
    const bool isEast = tab->shape == QTabBar::RoundedEast || tab->shape == QTabBar::TriangularEast;
    const QTabBar *tabBar = qobject_cast<const QTabBar *>(widget);
    const bool isFirstTab = tab->position == QStyleOptionTab::Beginning || tab->position == QStyleOptionTab::OnlyOneTab;
    const bool isLastTab = tab->position == QStyleOptionTab::End || tab->position == QStyleOptionTab::OnlyOneTab;

    if (!isWest && !isEast)
    {
        drawPillTab(tab, painter, widget);
        return;
    }

    PainterStateGuard guard(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);

    constexpr int navigationOuterMarginV = 0;
    constexpr int navigationSpacingV = 4;
    constexpr int navigationInsetH = 0;
    const int topInset = isFirstTab ? navigationOuterMarginV : navigationSpacingV / 2;
    const int bottomInset = isLastTab ? navigationOuterMarginV : navigationSpacingV / 2;
    const QRectF itemRect = QRectF(tab->rect).adjusted(navigationInsetH, topInset, -navigationInsetH, -bottomInset);
    if (isSelected)
    {
        painter->setBrush(winUI3Color(tabBarSelectedBackground));
        painter->drawRoundedRect(itemRect, secondLevelRoundingRadius, secondLevelRoundingRadius);
    }
    else if (isHover)
    {
        painter->setBrush(winUI3Color(tabBarHoverBackground));
        painter->drawRoundedRect(itemRect, secondLevelRoundingRadius, secondLevelRoundingRadius);
    }

    if (!isSelected)
    {
        return;
    }

    constexpr qreal indicatorWidth = 3.0;
    constexpr qreal indicatorInset = 8.0;
    const qreal indicatorX = isEast ? itemRect.right() - indicatorWidth : itemRect.left();
    qreal indicatorTop = itemRect.top() + indicatorInset;
    qreal indicatorHeight = qMax(0.0, itemRect.height() - indicatorInset * 2.0);

    if (tabBar && tab->styleObject)
    {
        const int currentTabIndex = tabBar->currentIndex();
        const int tabIndex = tabBar->tabAt(tab->rect.center());
        const int previousTabIndex = tab->styleObject->property("_q_navigation_selected_tab_index").toInt();
        const QByteArray animKey = "_q_tab_navigation_indicator_grow";

        if (currentTabIndex >= 0 && previousTabIndex >= 0 && currentTabIndex != previousTabIndex && tabIndex == currentTabIndex)
        {
            QNumberStyleAnimation *t = new QNumberStyleAnimation(tab->styleObject);
            t->setStartValue(0.0);
            t->setEndValue(1.0);
            t->setDuration(160);
            t->setEasingCurve(QEasingCurve::OutCubic);
            startAnimationEx(t, tab->styleObject, animKey);
        }

        const qreal progress = previousTabIndex < 0 ? 1.0 : qBound(0.0, qreal(animationValue(tab->styleObject, animKey, 1.0f)), 1.0);
        const qreal fullHeight = indicatorHeight;
        indicatorHeight = fullHeight * progress;
        indicatorTop = itemRect.top() + indicatorInset + (fullHeight - indicatorHeight) / 2.0;

        tab->styleObject->setProperty("_q_navigation_selected_tab_index", currentTabIndex);
    }

    const QRectF indicatorRect(indicatorX, indicatorTop, indicatorWidth, indicatorHeight);

    const QColor accent = calculateAccentColor(tab);
    painter->setBrush(accent);
    painter->drawRoundedRect(indicatorRect, indicatorWidth / 2.0, indicatorWidth / 2.0);
#else
    Q_UNUSED(tab)
    Q_UNUSED(painter)
    Q_UNUSED(widget)
#endif
}

void FluentUI3Style::drawListViewIndicator(const QStyleOptionViewItem *option, QPainter *painter, const QWidget *widget) const
{
    if (!option || !painter || highContrastTheme)
    {
        return;
    }

    const auto clamp01 = [](qreal value)
    { return qBound(0.0, value, 1.0); };

    const QListView *lv = qobject_cast<const QListView *>(widget);
    if (!lv && widget)
    {
        lv = qobject_cast<const QListView *>(widget->parentWidget());
    }
    if (!lv || lv->viewMode() == QListView::IconMode)
    {
        return;
    }

    if (!(option->state & State_Selected))
    {
        return;
    }

    QObject *stateObject = lv->viewport() ? static_cast<QObject *>(lv->viewport()) : const_cast<QListView *>(lv);
    const QByteArray animKey = "_q_list_indicator_grow";
    const int currentRow = option->index.row();
    const int storedRow = stateObject->property("_q_list_indicator_selected_row").toInt();

    if (currentRow >= 0 && currentRow != storedRow)
    {
        stateObject->setProperty("_q_list_indicator_selected_row", currentRow);

        QNumberStyleAnimation *t = new QNumberStyleAnimation(stateObject);
        t->setStartValue(0.0);
        t->setEndValue(1.0);
        t->setDuration(120);
        startAnimationEx(t, stateObject, animKey);
    }

    const qreal progress = clamp01(animationValue(stateObject, animKey, 1.0f));
    const qreal normalInset = option->rect.height() / 4.0;
    const qreal markerWidth = 2.0;
    const bool isRtl = option->direction == Qt::RightToLeft;
    const qreal xPos = isRtl ? option->rect.right() - 4.5f : option->rect.left() + 3.5f;
    const qreal halfHeight = (option->rect.height() - normalInset * 2.0) * progress * 0.5;
    const qreal centerY = option->rect.center().y() + 0.5;
    const QRectF indicatorRect(QPointF(xPos, centerY - halfHeight), QPointF(xPos + markerWidth, centerY + halfHeight));

    const QColor col = accentColor(option);

    painter->setBrush(col);
    painter->setPen(col);
    painter->drawRoundedRect(indicatorRect, 1.0, 1.0);
}

void FluentUI3Style::drawNavigationViewIndicator(const QStyleOptionViewItem *option, QPainter *painter, const QWidget *widget) const
{
    if (!option || !painter || highContrastTheme)
    {
        return;
    }

    const QTreeView *treeView = qobject_cast<const QTreeView *>(widget);
    if (!treeView && widget)
    {
        const QWidget *p = widget->parentWidget();
        while (p && !treeView)
        {
            treeView = qobject_cast<const QTreeView *>(p);
            p = p->parentWidget();
        }
    }
    if (!treeView)
    {
        return;
    }

    const auto clamp01 = [](qreal value)
    { return qBound(0.0, value, 1.0); };
    const auto lerp = [](qreal a, qreal b, qreal t)
    { return a + (b - a) * t; };
    const auto smoothStep = [&](qreal t)
    {
        const qreal clamped = clamp01(t);
        return clamped * clamped * (3.0 - 2.0 * clamped);
    };

    const QRect rect = option->rect;
    if (rect.width() <= 0 || rect.height() <= 0)
    {
        return;
    }

    QObject *stateObject = treeView->viewport() ? static_cast<QObject *>(treeView->viewport()) : const_cast<QTreeView *>(treeView);
    const QByteArray slideKey = "_q_tree_indicator_slide";

    const bool isRtl = option->direction == Qt::RightToLeft;
    const qreal targetX = isRtl ? rect.right() - 4.5f : rect.left() + 3.5f;
    const qreal normalInset = rect.height() / 3.5f;
    const qreal targetTop = rect.top() + normalInset;
    const qreal targetH = rect.height() - normalInset * 2.0;
    const qreal targetW = 2.0;

    const QRectF fromRect = stateObject->property("_q_tree_indicator_from_rect").toRectF();
    const QRectF toRect = stateObject->property("_q_tree_indicator_to_rect").toRectF();
    qreal fromX = fromRect.x();
    qreal fromTop = fromRect.y();
    qreal fromH = fromRect.height();
    qreal toX = toRect.x();
    qreal toTop = toRect.y();
    qreal toH = toRect.height();
    QModelIndex fromIndex = stateObject->property("_q_tree_indicator_from_index").toModelIndex();
    QModelIndex toIndex = stateObject->property("_q_tree_indicator_to_index").toModelIndex();
    QString phase = stateObject->property("_q_nav_phase").toString();
    bool wasSelected = stateObject->property("_q_nav_had_selection").toBool();
    const bool hasSelection = treeView->selectionModel() && treeView->selectionModel()->hasSelection();
    const bool inAnimation = getAnimationEx(stateObject, slideKey) != nullptr;

    if (!inAnimation && !phase.isEmpty())
    {
        if (phase == QStringLiteral("exiting"))
        {
            toIndex = QModelIndex();
            fromIndex = QModelIndex();
        }
        phase.clear();
    }

    if (!hasSelection && !wasSelected && phase.isEmpty())
    {
        if (toIndex.isValid())
        {
            stateObject->setProperty("_q_tree_indicator_to_index", QModelIndex());
            stateObject->setProperty("_q_tree_indicator_from_index", QModelIndex());
        }
        return;
    }

    auto kickSlideAnim = [&]() {
        QNumberStyleAnimation *t = new QNumberStyleAnimation(stateObject);
        t->setStartValue(0.0);
        t->setEndValue(1.0);
        t->setDuration(550);
        t->setFrameRate(QStyleAnimation::DefaultFps);
        startAnimationEx(t, stateObject, slideKey);
    };

    if (toH <= 0.0 && hasSelection && phase.isEmpty() && treeView->property("navigationDirection").toString().isEmpty())
    {
        QModelIndex seedIndex;
        const auto sel = treeView->selectionModel()->selectedIndexes();
        if (!sel.isEmpty())
            seedIndex = sel.first();
        if (!seedIndex.isValid())
            seedIndex = treeView->currentIndex();
        if (!seedIndex.isValid())
            return;

        const QRect seedVr = treeView->visualRect(seedIndex);
        if (seedVr.width() > 0 && seedVr.height() > 0)
        {
            const qreal sX = isRtl ? seedVr.right() - 4.5f : seedVr.left() + 3.5f;
            const qreal sInset = seedVr.height() / 3.5f;
            fromX = toX = sX;
            fromTop = toTop = seedVr.top() + sInset;
            fromH = toH = seedVr.height() - sInset * 2.0;
            fromIndex = toIndex = seedIndex;
            wasSelected = true;
            stateObject->setProperty("_q_nav_had_selection", true);
        }
        else if (seedIndex == option->index && (option->state & State_Selected))
        {
            fromX = toX = targetX;
            fromTop = toTop = targetTop;
            fromH = toH = targetH;
            fromIndex = toIndex = seedIndex;
            wasSelected = true;
            stateObject->setProperty("_q_nav_had_selection", true);
        }
        else
        {
            return;
        }
    }

    if (!inAnimation && phase.isEmpty())
    {
        const QString navDir = treeView->property("navigationDirection").toString();

        if (wasSelected && !hasSelection && toIndex.isValid() && toH > 0.0)
        {
            fromX = toX;
            fromTop = toTop;
            fromH = toH;
            fromIndex = toIndex;
            toTop = (navDir == QStringLiteral("up")) ? (fromTop - normalInset)
                                                    : (fromTop + fromH + normalInset);
            toH = 0.0;
            phase = QStringLiteral("exiting");
            kickSlideAnim();
            wasSelected = false;
            stateObject->setProperty("_q_nav_had_selection", false);
        }
        else if (!wasSelected && hasSelection && (option->state & State_Selected))
        {
            toX = targetX;
            toTop = targetTop;
            toH = targetH;
            toIndex = option->index;
            fromX = toX;
            fromTop = (navDir == QStringLiteral("up")) ? qreal(rect.top()) : qreal(rect.bottom());
            fromH = 0.0;
            fromIndex = toIndex;
            phase = QStringLiteral("entering");
            kickSlideAnim();
            wasSelected = true;
            stateObject->setProperty("_q_nav_had_selection", true);
        }
        else if (wasSelected && hasSelection && (option->state & State_Selected) && option->index != toIndex)
        {
            fromX = toX;
            fromTop = toTop;
            fromH = toH;
            toX = targetX;
            toTop = targetTop;
            toH = targetH;
            fromIndex = toIndex;
            toIndex = option->index;
            phase = QStringLiteral("sliding");
            kickSlideAnim();
        }
        else if ((option->state & State_Selected) && option->index == toIndex)
        {
            // 同一行刷新（处理滚动）：实时同步坐标
            toX = targetX;
            toTop = targetTop;
            toH = targetH;
            fromX = targetX;
            fromTop = targetTop;
            fromH = targetH;
        }
    }

    const qreal progress = clamp01(animationValue(stateObject, slideKey, 1.0f));

    qreal drawTop = toTop;
    qreal drawBottom = toTop + toH;
    qreal drawX = toX;
    
    //离场和入场时，只画前半段或后半段
    bool shouldDraw = false;

    const bool drawFirstHalf = (phase != QStringLiteral("entering"));
    const bool drawSecondHalf = (phase != QStringLiteral("exiting"));

    const bool movingDown = toTop >= fromTop;
    const qreal fromBottom = fromTop + fromH;
    const qreal toBottom = toTop + toH;
    const qreal fromItemTop = fromTop - normalInset;
    const qreal fromItemBottom = fromBottom + normalInset;
    const qreal toItemTop = toTop - normalInset;
    const qreal toItemBottom = toBottom + normalInset;

    drawX = (progress < 0.5) ? fromX : toX;
    const bool isFromItem = fromIndex.isValid() && option->index == fromIndex;
    const bool isToItem = toIndex.isValid() && option->index == toIndex;

    if (isFromItem || isToItem)
    {
        if (qAbs(toTop - fromTop) < 0.5)
        {
            shouldDraw = true;
            const qreal t = smoothStep(progress);
            drawTop = lerp(fromTop, toTop, t);
            drawBottom = lerp(fromBottom, toBottom, t);
        }
        else if (progress < 0.5 && drawFirstHalf && isFromItem)
        {
            shouldDraw = true;
            const qreal p = progress / 0.5;
            const qreal t = p * p;
            if (movingDown)
            {
                drawTop = fromTop;
                drawBottom = lerp(fromBottom, fromItemBottom, t);
            }
            else
            {
                drawTop = lerp(fromTop, fromItemTop, t);
                drawBottom = fromBottom;
            }
        }
        else if (progress >= 0.5 && drawSecondHalf && isToItem)
        {
            shouldDraw = true;
            const qreal p = (progress - 0.5) / 0.5;
            const qreal t = p * (2.0 - p);
            if (movingDown)
            {
                drawTop = lerp(toItemTop, toTop, t);
                drawBottom = toBottom;
            }
            else
            {
                drawTop = toTop;
                drawBottom = lerp(toItemBottom, toBottom, t);
            }
        }
    }

    stateObject->setProperty("_q_tree_indicator_from_rect", QRectF(fromX, fromTop, targetW, fromH));
    stateObject->setProperty("_q_tree_indicator_to_rect", QRectF(toX, toTop, targetW, toH));
    stateObject->setProperty("_q_tree_indicator_from_index", fromIndex);
    stateObject->setProperty("_q_tree_indicator_to_index", toIndex);
    stateObject->setProperty("_q_nav_phase", phase);

    if (!shouldDraw)
    {
        return;
    }

    const QRectF mark(QPointF(drawX, drawTop), QPointF(drawX + targetW, drawBottom));
    painter->setBrush(calculateAccentColor(option));
    painter->setPen(calculateAccentColor(option));
    painter->drawRoundedRect(mark, 1.0, 1.0);
}

void FluentUI3Style::drawTreeViewIndicator(const QStyleOptionViewItem *option, QPainter *painter, const QWidget *widget) const
{
    if (!option || !painter || highContrastTheme)
    {
        return;
    }

    const QTreeView *treeView = qobject_cast<const QTreeView *>(widget);
    if (!treeView || qobject_cast<const QTableView *>(widget))
    {
        return;
    }

    if (option->state & State_Selected)
    {
        const QColor col = accentColor(option);
        const QRect rect = option->rect;
        if (rect.width() <= 0 || rect.height() <= 0)
        {
            return;
        }

        const bool isRtl = option->direction == Qt::RightToLeft;
        const qreal xPos = isRtl ? rect.right() - 4.5f : rect.left() + 3.5f;
        const qreal yOfs = rect.height() / 4.0;
        const QRectF mark(QPointF(xPos, rect.y() + yOfs), QPointF(xPos + 1.0, rect.y() + rect.height() - yOfs));

        painter->setBrush(col);
        painter->setPen(col);
        painter->drawRoundedRect(mark, 1.0, 1.0);
    }
}

void FluentUI3Style::drawProgressRing(const QStyleOptionProgressBar *option,
                                      QPainter *painter,
                                      const QWidget *widget,
                                      bool drawTrack) const
{
    if (!option || !painter)
    {
        return;
    }

    qreal thickness = ProgressBarThickness + 2;
    // 暂时不需要了，后续如果需要调整进度环的粗细，可以再加上这个功能
    //  if ( widget && widget->property( ProgressBarThicknessProperty ).isValid() )
    //  {
    //      const int t = widget->property( ProgressBarThicknessProperty ).toInt();
    //      if ( t > 0 )
    //      {
    //          thickness = t;
    //      }
    //  }

    const QRectF rect = option->rect;
    const qreal diameter = qMin(rect.width(), rect.height());
    if (diameter <= 0.0)
    {
        return;
    }

    QRectF ringRect(0, 0, diameter, diameter);
    ringRect.moveCenter(rect.center());
    ringRect = ringRect.marginsRemoved(QMarginsF(thickness / 2.0 + 2, thickness / 2.0 + 2, thickness / 2.0 + 2, thickness / 2.0 + 2));

    if (drawTrack)
    {
        QPen pen(Qt::gray, thickness, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(ringRect);
        return;
    }

    const bool isIndeterminate = option->maximum == 0 && option->minimum == 0;
    constexpr qreal angleZeroOffset = 90.0; // make 0 degree point to 12 o'clock
    qreal startAngle = 0.0;
    qreal spanAngle = 0.0;
    if (isIndeterminate)
    {
        constexpr auto loopDurationMSec = 800;
        const auto elapsedTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        const auto elapsed = elapsedTime.time_since_epoch().count();
        const qreal t = (elapsed % loopDurationMSec) / float(loopDurationMSec);
        // Qt angles are CCW; use negative to rotate clockwise.
        startAngle = -360.0 * t;
        spanAngle = 90.0;
        const_cast<QWidget *>(widget)->update();
    }
    else
    {
        const auto fillPercentage = (float(option->progress - option->minimum)) / (float(option->maximum - option->minimum));
        spanAngle = 360.0 * qBound(0.0f, fillPercentage, 1.0f);
    }
    spanAngle = -spanAngle;

    QPen pen(accentColor(option), thickness, Qt::SolidLine, Qt::RoundCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawArc(ringRect, int((startAngle + angleZeroOffset) * 16), int(spanAngle * 16));
}

// From QCommonStyle::drawControl, but only for CE_TabBarTabLabel, and with some adjustments for FluentUI3
void FluentUI3Style::drawTabBarTabLabel(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option))
    {
        const bool isVerticalNavigation = widget && widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Navigation && (tab->shape == QTabBar::RoundedWest || tab->shape == QTabBar::RoundedEast || tab->shape == QTabBar::TriangularWest || tab->shape == QTabBar::TriangularEast);

        QRect tr = tab->rect;
        int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
        if (widget)
        {
            auto textAlign = widget->property("TextAlign").toString();
            if (!textAlign.isEmpty())
            {
                alignment = textAlign.toInt() | Qt::TextShowMnemonic;
            }
        }

        if (!proxy()->styleHint(SH_UnderlineShortcut, option, widget))
        {
            alignment |= Qt::TextHideMnemonic;
        }

        QRect iconRect;
        tabLayout(proxy(), tab, widget, &tr, &iconRect);

        if (isVerticalNavigation)
        {
            alignment = Qt::AlignVCenter | Qt::AlignLeft | Qt::TextShowMnemonic;
            if (!proxy()->styleHint(SH_UnderlineShortcut, option, widget))
            {
                alignment |= Qt::TextHideMnemonic;
            }
        }
        else
        {
            // compute tr again, unless tab is moving, because the style may override subElementRect
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            if (tab->position != QStyleOptionTab::TabPosition::Moving)
#endif
            {
                tr = proxy()->subElementRect(SE_TabBarTabText, option, widget);
            }
        }

        const bool isSegmentedWinUI3 = widget && widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Segmented_WinUI3;
        if (isSegmentedWinUI3)
        {
            int iconWidth = 0;
            int spacing = 0;
            if (!tab->icon.isNull())
            {
                iconWidth = tab->iconSize.width();
                if (!tab->text.isEmpty())
                {
                    spacing = 6;
                }
            }
            int textWidth = tab->fontMetrics.horizontalAdvance(tab->text);
            int totalWidth = iconWidth + spacing + textWidth;
            int startX = tab->rect.left() + (tab->rect.width() - totalWidth) / 2;
            if (!tab->icon.isNull())
            {
                iconRect = QRect(startX, tab->rect.center().y() - tab->iconSize.height() / 2, iconWidth, tab->iconSize.height());
                tr = QRect(iconRect.right() + spacing, tab->rect.top(), textWidth, tab->rect.height());
            }
            else
            {
                tr = QRect(startX, tab->rect.top(), textWidth, tab->rect.height());
            }
            alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
        }
        else if (!tab->icon.isNull() && tab->text.isEmpty())
        {
            // Icon-only tabs should be visually centered in each segment.
            const int iconW = tab->iconSize.width();
            const int iconH = tab->iconSize.height();
            iconRect = QRect(tab->rect.center().x() - iconW / 2,
                             tab->rect.center().y() - iconH / 2,
                             iconW,
                             iconH);
            tr = tab->rect;
            alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
        }

        if (!tab->icon.isNull())
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QPixmap tabIcon = tab->icon.pixmap(tab->iconSize,
                                               QStyleHelper::getDpr(painter),
                                               (tab->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled,
                                               (tab->state & State_Selected) ? QIcon::On : QIcon::Off);
#else
            QPixmap tabIcon = tab->icon.pixmap(tab->iconSize,
                                               (tab->state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled,
                                               (tab->state & QStyle::State_Selected) ? QIcon::On : QIcon::Off);
#endif
            painter->drawPixmap(iconRect.x(), iconRect.y(), tabIcon);
        }

        QPalette textPalette = tab->palette;
        const bool isEnabled = tab->state & State_Enabled;
        const bool isHot = tab->state & (State_Selected | State_MouseOver);

        QColor tabTextColor;
        const bool isSegmentedCustomColors = widget && (widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Segmented_Slide
                                                        || widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Segmented_Fade)
                                             && widget->property(SegmentedBackgroundColorProperty).isValid();
        if (!isEnabled)
        {
            tabTextColor = highContrastTheme ? tab->palette.buttonText().color() : winUI3Color(textDisabled);
        }
        else if (isHot)
        {
            tabTextColor = highContrastTheme ? tab->palette.buttonText().color() : winUI3Color(textPrimary);
        }
        else
        {
            tabTextColor = highContrastTheme ? tab->palette.buttonText().color()
                                             : (isSegmentedCustomColors ? winUI3Color(textPrimary) : winUI3Color(textSecondary));
        }

        textPalette.setColor(QPalette::WindowText, tabTextColor);
        textPalette.setColor(QPalette::ButtonText, tabTextColor);
        textPalette.setColor(QPalette::Text, tabTextColor);

        proxy()->drawItemText(painter, tr, alignment, textPalette, isEnabled, tab->text, QPalette::WindowText);

        if (tab->state & State_HasFocus)
        {
            const int OFFSET = 1 + pixelMetric(PM_DefaultFrameWidth, option, widget);

            int x1, x2;
            x1 = tab->rect.left();
            x2 = tab->rect.right() - 1;

            QStyleOptionFocusRect fropt;
            fropt.QStyleOption::operator=(*tab);
            fropt.rect.setRect(x1 + 1 + OFFSET, tab->rect.y() + OFFSET, x2 - x1 - 2 * OFFSET, tab->rect.height() - 2 * OFFSET);
            drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
        }
    }
}

void FluentUI3Style::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    State flags = option->state;
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    switch (element)
    {
    case CE_ComboBoxLabel:
#if QT_CONFIG(combobox)
        if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(option))
        {
            painter->setPen(controlTextColor(option));
            QStyleOptionComboBox newOption = *cb;
            newOption.rect.adjust(4, 0, -4, 0);
            QProxyStyle::drawControl(element, &newOption, painter, widget);
        }
#endif // QT_CONFIG(combobox)
        break;
    case CE_TabBarTabShape:
        drawTabBarTabShape(option, painter, widget);
        break;
    case CE_ToolButtonLabel:
#if QT_CONFIG(toolbutton)
        if (const QStyleOptionToolButton *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(option))
        {
            QRect rect = toolbutton->rect;
            int shiftX = 0;
            int shiftY = 0;
            int leftMargin = 0;
            if (toolbutton->features & QStyleOptionToolButton::HasMenu)
            {
                leftMargin = 4;
            }
            if (toolbutton->state & (State_Sunken | State_On))
            {
                shiftX = proxy()->pixelMetric(PM_ButtonShiftHorizontal, toolbutton, widget);
                shiftY = proxy()->pixelMetric(PM_ButtonShiftVertical, toolbutton, widget);
            }
            // Arrow type always overrules and is always shown
            // FluentUI don't have arrow type
            bool hasArrow = /*toolbutton->features & QStyleOptionToolButton::Arrow*/ false;
            if (((!hasArrow && toolbutton->icon.isNull()) && !toolbutton->text.isEmpty()) || toolbutton->toolButtonStyle == Qt::ToolButtonTextOnly)
            {
                int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
                if (!proxy()->styleHint(SH_UnderlineShortcut, toolbutton, widget))
                {
                    alignment |= Qt::TextHideMnemonic;
                }
                rect.translate(shiftX, shiftY);
                if (toolbutton->features & QStyleOptionToolButton::HasMenu)
                {
                    const int mbi = pixelMetric(PM_MenuButtonIndicator, option, widget);
                    rect.adjust(-mbi + 4, 0, 0, 0);
                }

                painter->setFont(toolbutton->font);
                const QString text = toolButtonElideText(toolbutton, rect, alignment);
                // option->state has no State_Sunken here,
                // windowsvistastyle/CC_ToolButton removes it
                painter->setPen(controlTextColor(option));
                proxy()->drawItemText(painter, rect, alignment, toolbutton->palette, toolbutton->state & State_Enabled, text);
            }
            else
            {
                QPixmap pm;
                QSize pmSize = toolbutton->iconSize;
                if (!toolbutton->icon.isNull())
                {
                    QIcon::State state = toolbutton->state & State_On ? QIcon::On : QIcon::Off;
                    QIcon::Mode mode;
                    if (!(toolbutton->state & State_Enabled))
                    {
                        mode = QIcon::Disabled;
                    }
                    else if ((toolbutton->state & State_MouseOver) && (toolbutton->state & State_AutoRaise))
                    {
                        mode = QIcon::Active;
                    }
                    else
                    {
                        mode = QIcon::Normal;
                    }

                    auto tBtnIconSize = toolbutton->iconSize;
                    // Note: 由于 Qt 源码中 QDockWidgetTitleButton::dockButtonIconSize() 有硬编码限制，
                    // 即使在 FluentUI3Style 中设置 PM_SmallIconSize 为 20，也会被 isWindowsStyle 检查限制为 10 像素左右。
                    // 这里直接硬编码为 15 像素，以获得更大的 dock widget title 按钮图标。
                    if (widget && QString(widget->metaObject()->className()) == "QDockWidgetTitleButton")
                    {
                        tBtnIconSize = QSize(15, 15);
                    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                    pm = toolbutton->icon.pixmap(
                        toolbutton->rect.size().boundedTo(tBtnIconSize), painter->device()->devicePixelRatio(), mode, state);
#else
                    auto qt_getWindow = [](const QWidget *widget)
                    { return widget ? widget->window()->windowHandle() : nullptr; };
                    pm = toolbutton->icon.pixmap(qt_getWindow(widget), toolbutton->rect.size().boundedTo(tBtnIconSize), mode, state);
#endif

                    pmSize = pm.size() / pm.devicePixelRatio();
                }

                if (toolbutton->toolButtonStyle != Qt::ToolButtonIconOnly)
                {
                    painter->setFont(toolbutton->font);
                    QRect pr = rect, tr = rect;
                    int alignment = Qt::TextShowMnemonic;
                    if (!proxy()->styleHint(SH_UnderlineShortcut, toolbutton, widget))
                    {
                        alignment |= Qt::TextHideMnemonic;
                    }

                    if (toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon)
                    {
                        pr.setHeight(pmSize.height() + 4); // ### 4 is currently hardcoded in
                                                           // QToolButton::sizeHint()
                        tr.adjust(0, pr.height() - 1, 0, -1);
                        pr.translate(shiftX, shiftY);

                        const int mbi = pixelMetric(PM_MenuButtonIndicator, option, widget);
                        if (toolbutton->features & QStyleOptionToolButton::HasMenu)
                        {
                            tr.translate(-mbi + 4, 0);
                        }

                        if (!hasArrow)
                        {
                            proxy()->drawItemPixmap(painter, pr, Qt::AlignCenter, pm);
                        }
                        else
                        {
                            drawArrow(proxy(), toolbutton, pr, painter, widget);
                        }
                        alignment |= Qt::AlignCenter;
                    }
                    else
                    {
                        pr.setWidth(pmSize.width() + 4); // ### 4 is currently hardcoded
                                                         // in QToolButton::sizeHint()
                        tr.adjust(pr.width() + 2, 0, 0, 0);
                        pr.translate(shiftX + leftMargin, shiftY);
                        if (!hasArrow)
                        {
                            proxy()->drawItemPixmap(painter, QStyle::visualRect(toolbutton->direction, rect, pr), Qt::AlignCenter, pm);
                        }
                        else
                        {
                            drawArrow(proxy(), toolbutton, pr, painter, widget);
                        }
                        alignment |= Qt::AlignLeft | Qt::AlignVCenter;
                    }
                    tr.translate(shiftX + leftMargin, shiftY);
                    const QString text = toolButtonElideText(toolbutton, tr, alignment);
                    painter->setPen(controlTextColor(option));
                    proxy()->drawItemText(painter,
                                          QStyle::visualRect(toolbutton->direction, rect, tr),
                                          alignment,
                                          toolbutton->palette,
                                          toolbutton->state & State_Enabled,
                                          text);
                }
                else
                {
                    rect.translate(shiftX, shiftY);
                    if (hasArrow)
                    {
                        drawArrow(proxy(), toolbutton, rect, painter, widget);
                    }
                    else
                    {
                        proxy()->drawItemPixmap(painter, rect, Qt::AlignCenter, pm);
                    }
                }
            }
        }
#endif // QT_CONFIG(toolbutton)
        break;
    case CE_TabBarTabLabel:
        drawTabBarTabLabel(option, painter, widget);
        break;
    case CE_ShapedFrame:
        if (const QStyleOptionFrame *f = qstyleoption_cast<const QStyleOptionFrame *>(option))
        {
            const bool isComboPopup = widget && widget->inherits("QComboBoxPrivateContainer");
            if (isComboPopup)
            {
                painter->setPen(highContrastTheme ? QPen(option->palette.windowText().color(), 1) : winUI3Color(frameColorLight));
                drawFluentShadow(painter, option->rect, cBShadowBorderWidth, cBRoundingRadius);
                painter->setBrush(winUI3Color(menuPanelFill));
                auto pRect = QRectF(option->rect)
                                 .marginsRemoved(
                                     QMarginsF(cBShadowBorderWidth, cBShadowBorderWidth, cBShadowBorderWidth, cBShadowBorderWidth));
                painter->drawRoundedRect(pRect, cBRoundingRadius, cBRoundingRadius);
                break;
            }

            int frameShape = f->frameShape;
            int frameShadow = QFrame::Plain;
            if (f->state & QStyle::State_Sunken)
            {
                frameShadow = QFrame::Sunken;
            }
            else if (f->state & QStyle::State_Raised)
            {
                frameShadow = QFrame::Raised;
            }

            int lw = f->lineWidth;
            int mlw = f->midLineWidth;

            switch (frameShape)
            {
            case QFrame::Box:
                if (frameShadow == QFrame::Plain)
                {
                    qDrawPlainRoundedRect(painter,
                                          f->rect,
                                          secondLevelRoundingRadius,
                                          secondLevelRoundingRadius,
                                          highContrastTheme == true ? f->palette.buttonText().color()
                                                                    : WINUI3Colors[colorSchemeIndex][frameColorStrong],
                                          lw);
                }
                else
                {
                    qDrawShadeRect(painter, f->rect, f->palette, frameShadow == QFrame::Sunken, lw, mlw);
                }
                break;
            case QFrame::Panel:
                if (frameShadow == QFrame::Plain)
                {
                    qDrawPlainRoundedRect(painter,
                                          f->rect,
                                          secondLevelRoundingRadius,
                                          secondLevelRoundingRadius,
                                          highContrastTheme == true ? f->palette.buttonText().color()
                                                                    : WINUI3Colors[colorSchemeIndex][frameColorStrong],
                                          lw);
                }
                else
                {
                    qDrawShadePanel(painter, f->rect, f->palette, frameShadow == QFrame::Sunken, lw);
                }
                break;
            default:
                QProxyStyle::drawControl(element, option, painter, widget);
            }
        }
        break;
#if QT_CONFIG(progressbar)
    case CE_ProgressBar:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(option))
        {
            QStyleOptionProgressBar subopt = *pb;
            subopt.rect = subElementRect(SE_ProgressBarGroove, pb, widget);
            drawControl(CE_ProgressBarGroove, &subopt, painter, widget);
            subopt.rect = subElementRect(SE_ProgressBarContents, pb, widget);
            drawControl(CE_ProgressBarContents, &subopt, painter, widget);
            if (pb->textVisible)
            {
                subopt.rect = subElementRect(SE_ProgressBarLabel, pb, widget);
                drawControl(CE_ProgressBarLabel, &subopt, painter, widget);
            }
        }
        break;
    case CE_ProgressBarGroove:
        if (const auto baropt = qstyleoption_cast<const QStyleOptionProgressBar *>(option))
        {
            QRectF rect = option->rect;
            QPointF center = rect.center();
            const int styleValue = widget ? widget->property(ProgressBarStyleProperty).toInt() : ProgressBarThin;
            const bool isThick = styleValue == ProgressBarThick;
            const bool isRing = styleValue == ProgressBarRing;
            if (isRing)
            {
                drawProgressRing(baropt, painter, widget, true);
                break;
            }
            if (isThick)
            {
                if (baropt->state & QStyle::State_Horizontal)
                {
                    rect.setLeft(rect.left() + 2);
                    rect.setWidth(qMax(0.0, rect.width() - 2.0));
                    rect.setHeight(ProgressBarThickness);
                    rect.moveTop(center.y() - rect.height() / 2.0);
                }
                else
                {
                    rect.setWidth(ProgressBarThickness);
                    rect.moveLeft(center.x() - rect.width() / 2.0);
                }
                const qreal radius = ProgressBarThickness / 2.0;
                painter->setPen(Qt::NoPen);
                painter->setBrush(Qt::gray);
                painter->drawRoundedRect(rect, radius, radius);
            }
            else
            {
                if (baropt->state & QStyle::State_Horizontal)
                {
                    rect.setLeft(rect.left() + 2);
                    rect.setWidth(qMax(0.0, rect.width() - 2.0));
                    rect.setHeight(1);
                    rect.moveTop(center.y() - 0.5);
                }
                else
                {
                    rect.setWidth(1);
                    rect.moveLeft(center.x() - 0.5);
                }
                painter->setPen(Qt::NoPen);
                painter->setBrush(Qt::gray);
                painter->drawRect(rect);
            }
        }
        break;
    case CE_ProgressBarContents:
        if (const auto baropt = qstyleoption_cast<const QStyleOptionProgressBar *>(option))
        {
            PainterStateGuard psg(painter);
            const int styleValue = widget ? widget->property(ProgressBarStyleProperty).toInt() : ProgressBarThin;
            const bool isRing = styleValue == ProgressBarRing;
            if (isRing)
            {
                drawProgressRing(baropt, painter, widget, false);
                break;
            }

            QRectF rect = option->rect;
            painter->translate(rect.topLeft());
            rect.translate(-rect.topLeft());
            const auto orientation = (baropt->state & QStyle::State_Horizontal) ? Qt::Horizontal : Qt::Vertical;
            if (orientation == Qt::Horizontal)
            {
                rect.setLeft(rect.left() + 2);
                rect.setWidth(qMax(0.0, rect.width() - 2.0));
            }
            const auto isIndeterminate = baropt->maximum == 0 && baropt->minimum == 0;
            const auto inverted = baropt->invertedAppearance;
            const auto reverse = (baropt->direction == Qt::RightToLeft) ^ inverted;
            // If the orientation is vertical, we use a transform to rotate
            // the progress bar 90 degrees (counter)clockwise. This way we
            // can use the same rendering code for both orientations.
            if (orientation == Qt::Vertical)
            {
                rect = QRectF(rect.left(), rect.top(), rect.height(),
                              rect.width()); // flip width and height
                QTransform m;
                if (inverted)
                {
                    m.rotate(90);
                    m.translate(0, -rect.height() + 1);
                }
                else
                {
                    m.rotate(-90);
                    m.translate(-rect.width(), 0);
                }
                painter->setTransform(m, true);
            }
            else if (reverse)
            {
                QTransform m = QTransform::fromScale(-1, 1);
                m.translate(-rect.width(), 0);
                painter->setTransform(m, true);
            }

            if (isIndeterminate)
            {
                constexpr auto loopDurationMSec = 3000;
                const auto elapsedTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
                const auto elapsed = elapsedTime.time_since_epoch().count();
                const auto handleCenter = (elapsed % loopDurationMSec) / float(loopDurationMSec);
                const auto isLongHandle = (elapsed / loopDurationMSec) % 2 == 0;
                const auto lengthFactor = (isLongHandle ? 33.0f : 25.0f) / 100.0f;
                const auto begin = qMax(handleCenter * (1 + lengthFactor) - lengthFactor, 0.0f);
                const auto end = qMin(handleCenter * (1 + lengthFactor), 1.0f);
                const auto barBegin = begin * rect.width();
                const auto barEnd = end * rect.width();
                rect = QRectF(QPointF(rect.left() + barBegin, rect.top()), QPointF(rect.left() + barEnd, rect.bottom()));
                const_cast<QWidget *>(widget)->update();
            }
            else
            {
                const auto fillPercentage =
                    (float(baropt->progress - baropt->minimum)) / (float(baropt->maximum - baropt->minimum));
                rect.setWidth(rect.width() * fillPercentage);
            }

            qreal progressBarThickness = 3.0;
            qreal offset = (int(rect.height()) % 2 == 0) ? 0.5f : 0.0f;
            if (widget && widget->property(ProgressBarStyleProperty).toInt() == ProgressBarThick)
            {
                progressBarThickness = ProgressBarThickness;
                offset = 0.0;
            }

            const QPointF center = rect.center();
            const qreal progressBarHalfThickness = progressBarThickness / 2.0;
            rect.setHeight(progressBarThickness);
            rect.moveTop(center.y() - progressBarHalfThickness - offset);
            if (progressBarThickness > 1.0 && rect.width() > 1.0)
            {
                // After the vertical-path transform, width is still along progress and height is thickness.
                // Cap insets belong on the progress axis only; do not shrink thickness for vertical bars.
                rect.adjust(0.5, 0.0, -0.5, 0.0);
            }

            painter->setPen(Qt::NoPen);
            painter->setBrush(accentColor(baropt));
            painter->drawRoundedRect(rect, progressBarHalfThickness, progressBarHalfThickness);
        }
        break;
    case CE_ProgressBarLabel:
        if (const auto baropt = qstyleoption_cast<const QStyleOptionProgressBar *>(option))
        {
            const bool vertical = !(baropt->state & QStyle::State_Horizontal);
            if (!vertical)
            {
                proxy()->drawItemText(painter,
                                      baropt->rect,
                                      Qt::AlignCenter | Qt::TextSingleLine,
                                      baropt->palette,
                                      baropt->state & State_Enabled,
                                      baropt->text,
                                      QPalette::Text);
            }
        }
        break;
#endif // QT_CONFIG(progressbar)
    case CE_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))
        {
            proxy()->drawControl(CE_PushButtonBevel, btn, painter, widget);
            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);
            proxy()->drawControl(CE_PushButtonLabel, &subopt, painter, widget);
            if (btn->state & State_HasFocus)
            {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*btn);
                fropt.rect = subElementRect(SE_PushButtonFocusRect, btn, widget);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
            }
        }
        break;
    case CE_PushButtonLabel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))
        {
            using namespace StyleOptionHelper;
            const bool isEnabled = !isDisabled(option);

            QRect textRect = btn->rect.marginsRemoved(QMargins(contentHMargin, 0, contentHMargin, 0));
            int tf = Qt::AlignCenter | Qt::TextShowMnemonic;
            if (!proxy()->styleHint(SH_UnderlineShortcut, btn, widget))
            {
                tf |= Qt::TextHideMnemonic;
            }

            if (!btn->icon.isNull())
            {
                // Center both icon and text
                QIcon::Mode mode = isEnabled ? QIcon::Normal : QIcon::Disabled;
                if (mode == QIcon::Normal && btn->state & State_HasFocus)
                {
                    mode = QIcon::Active;
                }
                QIcon::State state = isChecked(btn) ? QIcon::On : QIcon::Off;

                int iconSpacing = 4; // ### 4 is currently hardcoded in
                                     // QPushButton::sizeHint()

                QRect iconRect = QRect(textRect.x(), textRect.y(), btn->iconSize.width(), textRect.height());
                QRect vIconRect = visualRect(btn->direction, btn->rect, iconRect);
                textRect.setLeft(textRect.left() + iconRect.width() + iconSpacing);

                if (isChecked(btn) || isPressed(btn))
                {
                    vIconRect.translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                                        proxy()->pixelMetric(PM_ButtonShiftVertical, option, widget));
                }
                btn->icon.paint(painter, vIconRect, Qt::AlignCenter, mode, state);
            }

            auto vTextRect = visualRect(btn->direction, btn->rect, textRect);
            bool accent = widget && widget->property(ButtonAccentStyleProperty).toBool();

            bool checkable = false, checked = false;
            if (const auto *button = qobject_cast<const QPushButton *>(widget))
            {
                checkable = button->isCheckable();
                checked = button->isChecked();
            }

            if (accent || (checkable && checked))
            {
                painter->setPen(WINUI3Colors[colorSchemeIndex][textOnAccentPrimary]);
            }
            else
            {
                painter->setPen(controlTextColor(option));
            }
            proxy()->drawItemText(painter, vTextRect, tf, option->palette, isEnabled, btn->text);
        }
        break;
    case CE_PushButtonBevel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))
        {
            using namespace StyleOptionHelper;

            drawEffectShadow(painter, option->rect, 2, secondLevelRoundingRadius);
            QRectF rect = btn->rect.marginsRemoved(QMargins(2, 2, 2, 2));
            painter->setPen(Qt::NoPen);

            bool checkable = false, checked = false;
            if (const auto *button = qobject_cast<const QPushButton *>(widget))
            {
                checkable = button->isCheckable();
                checked = button->isChecked();
            }

            if (btn->features.testFlag(QStyleOptionButton::Flat))
            {
                painter->setBrush(btn->palette.button());
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
                if (flags & (State_Sunken | State_On))
                {
                    painter->setBrush(winUI3Color(subtlePressedColor));
                }
                else if (flags & State_MouseOver)
                {
                    painter->setBrush(winUI3Color(subtleHighlightColor));
                }
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);
            }
            else
            {
                bool accent = widget && widget->property("accent").toBool();
                const bool isCurrentChecked = checkable && checked;
                const int currentBtnState = option->state;

                QObject *styleObject = option->styleObject;
                QColor targetBgColor;
                if (accent)
                {
                    targetBgColor = calculateAccentColor(option);
                    painter->setPen(Qt::NoPen);
                }
                else if (isCurrentChecked)
                {
                    targetBgColor = calculateAccentColor(option);
                }
                else
                {
                    targetBgColor = controlFillBrush(option, ControlType::Control).color();
                }

                QColor bgColor = targetBgColor;
                if (styleObject)
                {
                    const int oldBtnState = styleObject->property("_q_btn_state").toInt();
                    if (oldBtnState != currentBtnState)
                    {
                        QColor fromColor;
                        if (QNumberStyleAnimation *prevAnim = qobject_cast<QNumberStyleAnimation *>(getAnimation(styleObject)))
                        {
                            const QColor prevFromColor = styleObject->property("_q_btn_from_color").value<QColor>();
                            const QColor prevTargetColor = styleObject->property("_q_btn_target_color").value<QColor>();
                            if (prevFromColor.isValid() && prevTargetColor.isValid())
                            {
                                fromColor = blend(prevTargetColor, prevFromColor, prevAnim->currentValue());
                            }
                            else
                            {
                                fromColor = prevFromColor.isValid() ? prevFromColor : targetBgColor;
                            }
                        }
                        else
                        {
                            fromColor = styleObject->property("_q_btn_from_color").value<QColor>();
                            if (!fromColor.isValid())
                            {
                                fromColor = targetBgColor;
                            }
                        }

                        styleObject->setProperty("_q_btn_state", currentBtnState);
                        styleObject->setProperty("_q_btn_from_color", fromColor);
                        styleObject->setProperty("_q_btn_target_color", targetBgColor);

                        QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                        t->setStartValue(0.0f);
                        t->setEndValue(1.0f);
                        t->setDuration(200);
                        t->setEasingCurve(QEasingCurve::OutCubic);
                        startAnimation(t);
                    }

                    if (QNumberStyleAnimation *anim = qobject_cast<QNumberStyleAnimation *>(getAnimation(styleObject)))
                    {
                        const qreal progress = anim->currentValue();
                        const QColor fromColor = styleObject->property("_q_btn_from_color").value<QColor>();
                        if (fromColor.isValid() && progress < 1.0f)
                        {
                            bgColor = blend(targetBgColor, fromColor, progress);
                        }
                    }
                    else
                    {
                        styleObject->setProperty("_q_btn_from_color", targetBgColor);
                        styleObject->setProperty("_q_btn_target_color", targetBgColor);
                    }
                }

                painter->setBrush(bgColor);
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

                rect.adjust(0.5, 0.5, -0.5, -0.5);
                const bool defaultButton = btn->features.testFlag(QStyleOptionButton::DefaultButton);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(defaultButton ? accentColor(option) : winUI3Color(controlStrokePrimary));
                painter->drawRoundedRect(rect, secondLevelRoundingRadius, secondLevelRoundingRadius);

                painter->setPen(defaultButton ? winUI3Color(controlStrokeOnAccentSecondary)
                                              : winUI3Color(controlStrokeSecondary));
            }

            if (btn->features.testFlag(QStyleOptionButton::HasMenu))
            {
                PainterStateGuard psg(painter);

                const bool isEnabled = !isDisabled(option);
                QRect textRect = btn->rect.marginsRemoved(QMargins(contentHMargin, 0, contentHMargin, 0));
                const auto indSize = proxy()->pixelMetric(PM_MenuButtonIndicator, btn, widget);
                const auto indRect = QRect(btn->rect.right() - indSize - contentItemHMargin,
                                           textRect.top(),
                                           indSize + contentItemHMargin,
                                           btn->rect.height());
                const auto vindRect = visualRect(btn->direction, btn->rect, indRect);
                textRect.setWidth(textRect.width() - indSize);

                int fontSize = painter->font().pointSize();
                QFont f(assetFont);
                f.setPointSize(qRound(fontSize * 0.9f)); // a little bit smaller
                painter->setFont(f);
                QColor penColor = option->palette.color(isEnabled ? QPalette::Active : QPalette::Disabled, QPalette::Text);
                if (isEnabled)
                {
                    penColor.setAlpha(percentToAlpha(60.63)); // fillColorTextSecondary
                }
                painter->setPen(penColor);
                painter->drawText(vindRect, Qt::AlignCenter, ChevronDownMed);
            }
        }
        break;
    case CE_RadioButton:
    case CE_CheckBox:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))
        {
            bool isRadio = (element == CE_RadioButton);
            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(isRadio ? SE_RadioButtonIndicator : SE_CheckBoxIndicator, btn, widget);
            proxy()->drawPrimitive(isRadio ? PE_IndicatorRadioButton : PE_IndicatorCheckBox, &subopt, painter, widget);
            subopt.rect = subElementRect(isRadio ? SE_RadioButtonContents : SE_CheckBoxContents, btn, widget);
            proxy()->drawControl(isRadio ? CE_RadioButtonLabel : CE_CheckBoxLabel, &subopt, painter, widget);
            if (btn->state & State_HasFocus)
            {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*btn);
                fropt.rect = subElementRect(isRadio ? SE_RadioButtonFocusRect : SE_CheckBoxFocusRect, btn, widget);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
            }
        }
        break;
    case CE_RadioButtonLabel:
    case CE_CheckBoxLabel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))
        {
            int alignment = visualAlignment(btn->direction, Qt::AlignLeft | Qt::AlignVCenter);

            if (!proxy()->styleHint(SH_UnderlineShortcut, btn, widget))
            {
                alignment |= Qt::TextHideMnemonic;
            }
            QRect textRect = btn->rect;
            if (!btn->icon.isNull())
            {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const auto pix = btn->icon.pixmap(
                    btn->iconSize, QStyleHelper::getDpr(painter), (btn->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled);
#else
                QPixmap pix = btn->icon.pixmap(btn->iconSize, (btn->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled);
                pix.setDevicePixelRatio(QStyleHelper::getDpr(painter));
#endif

                proxy()->drawItemPixmap(painter, btn->rect, alignment, pix);
                if (btn->direction == Qt::RightToLeft)
                {
                    textRect.setRight(textRect.right() - btn->iconSize.width() - 4);
                }
                else
                {
                    textRect.setLeft(textRect.left() + btn->iconSize.width() + 4);
                }
            }
            if (!btn->text.isEmpty())
            {
                proxy()->drawItemText(painter,
                                      textRect,
                                      alignment | Qt::TextShowMnemonic,
                                      btn->palette,
                                      btn->state & State_Enabled,
                                      btn->text,
                                      QPalette::WindowText);
            }
        }
        break;
    case CE_MenuBarItem:
        if (const auto *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            using namespace StyleOptionHelper;

            constexpr int hPadding = 11;
            constexpr int topPadding = 4;
            constexpr int bottomPadding = 6;
            QStyleOptionMenuItem newMbi = *mbi;

            newMbi.font.setPointSize(10);
            newMbi.palette.setColor(QPalette::ButtonText, controlTextColor(&newMbi));
            if (!isDisabled(&newMbi))
            {
                QPen pen(Qt::NoPen);
                QBrush brush(Qt::NoBrush);
                if (highContrastTheme)
                {
                    pen = QPen(newMbi.palette.highlight().color(), 2);
                    brush = newMbi.palette.window();
                }
                else if (isPressed(&newMbi))
                {
                    brush = winUI3Color(subtlePressedColor);
                }
                else if (isHover(&newMbi))
                {
                    brush = winUI3Color(subtleHighlightColor);
                }
                if (pen != Qt::NoPen || brush != Qt::NoBrush)
                {
                    // 调整菜单栏MenuItem的绘制区域，弹窗Menu更靠近MenuItem的左边界，符合WinUI3设计规范 QMargins( 2,
                    // 0, 2, 0 )
                    const QRect rect = mbi->rect.marginsRemoved(QMargins(5, 0, 5, 0));
                    drawRoundedRect(painter, rect, pen, brush);
                }
            }
            newMbi.rect.adjust(hPadding, topPadding, -hPadding, -bottomPadding);
            painter->setFont(newMbi.font);
            QCommonStyle::drawControl(element, &newMbi, painter, widget);
        }
        break;

#if QT_CONFIG(menu)
    case CE_MenuEmptyArea:
        break;

    case CE_MenuItem:
        if (const auto *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            const auto visualMenuRect = [&](const QRect &rect)
            { return visualRect(option->direction, menuitem->rect, rect); };
            bool dis = !(menuitem->state & State_Enabled);
            bool checked = menuitem->checkType != QStyleOptionMenuItem::NotCheckable ? menuitem->checked : false;
            bool act = menuitem->state & State_Selected;

            const QRect rect =
                menuitem->rect.marginsRemoved(QMargins(menuItemHMargin, menuItemVMargin, menuItemHMargin, menuItemVMargin));
            if (act && dis == false)
            {
                // 实际绘制时，右边界会有1px，调整后可以覆盖掉这个
                QRectF r = QRectF(rect).adjusted(0, 0, 1, 0);
                drawRoundedRect(painter,
                                r,
                                Qt::NoPen,
                                highContrastTheme ? menuitem->palette.brush(QPalette::Highlight)
                                                  : QBrush(winUI3Color(subtleHighlightColor)));
            }

            if (menuitem->menuItemType == QStyleOptionMenuItem::Separator)
            {
                constexpr int yoff = 1;
                painter->setPen(highContrastTheme ? menuitem->palette.buttonText().color() : winUI3Color(dividerStrokeDefault));
                painter->drawLine(menuitem->rect.topLeft() + QPoint(0, yoff), menuitem->rect.topRight() + QPoint(0, yoff));
                break;
            }

            int xOffset = contentHMargin;
            // WinUI3 draws, in contrast to former windows styles, the
            // checkmark and icon separately
            const auto checkMarkWidth = proxy()->pixelMetric(PM_IndicatorWidth, option, widget);
            if (checked)
            {
                QRect vRect(visualMenuRect(QRect(rect.x() + xOffset, rect.y(), checkMarkWidth, rect.height())));
                PainterStateGuard psg(painter);
                painter->setFont(assetFont);
                painter->setPen(option->palette.text().color());
                const auto textToDraw = QStringLiteral(u"\uE73E");
                painter->drawText(vRect, Qt::AlignCenter, textToDraw);
            }
            if (menuitem->menuHasCheckableItems)
            {
                xOffset += checkMarkWidth + contentItemHMargin;
            }
            if (!menuitem->icon.isNull())
            {
                // 4 is added to maxIconWidth in qmenu.cpp to
                // PM_SmallIconSize
                QRect vRect(visualMenuRect(QRect(rect.x() + xOffset, rect.y(), menuitem->maxIconWidth - 4, rect.height())));
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                {
                    mode = QIcon::Active;
                }
                const auto size = proxy()->pixelMetric(PM_SmallIconSize, option, widget);
                QRect pmr(QPoint(0, 0), QSize(size, size));
                pmr.moveCenter(vRect.center());
                menuitem->icon.paint(painter, pmr, Qt::AlignCenter, mode, checked ? QIcon::On : QIcon::Off);
            }
            if (menuitem->maxIconWidth > 0)
            {
                xOffset += menuitem->maxIconWidth - 4 + contentItemHMargin;
            }

            QStringView s(menuitem->text);
            if (!s.isEmpty())
            { // draw text
                QPoint tl(rect.left() + xOffset, rect.top());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                int shortcutWidth = menuitem->reservedShortcutWidth;
#else
                int shortcutWidth = menuitem->tabWidth;
#endif

                QPoint br(rect.right() - shortcutWidth - contentHMargin, rect.bottom());
                QRect textRect(tl, br);
                QRect vRect(visualMenuRect(textRect));

                qsizetype t = s.indexOf(u'\t');
                int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!proxy()->styleHint(SH_UnderlineShortcut, menuitem, widget))
                {
                    text_flags |= Qt::TextHideMnemonic;
                }
                text_flags |= Qt::AlignLeft;
                // a submenu doesn't paint a possible shortcut in WinUI3
                if (t >= 0 && menuitem->menuItemType != QStyleOptionMenuItem::SubMenu)
                {
                    QRect shortcutRect(QPoint(textRect.right(), textRect.top()), QPoint(rect.right(), textRect.bottom()));
                    QRect vShortcutRect(visualMenuRect(shortcutRect));
                    QColor penColor;
                    if (highContrastTheme)
                    {
                        penColor = menuitem->palette.color(act ? QPalette::HighlightedText : QPalette::Text);
                    }
                    else
                    {
                        penColor = menuitem->palette.color(dis ? QPalette::Disabled : QPalette::Active, QPalette::Text);
                        if (!dis)
                        {
                            penColor.setAlpha(percentToAlpha(60.63)); // fillColorTextSecondary
                        }
                    }
                    painter->setPen(penColor);
                    const QString textToDraw = s.mid(t + 1).toString();
                    painter->drawText(vShortcutRect, text_flags, textToDraw);
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                {
                    font.setBold(true);
                }
                painter->setFont(font);
                QColor penColor;
                if (highContrastTheme && act)
                {
                    penColor = menuitem->palette.color(QPalette::HighlightedText);
                }
                else
                {
                    penColor = menuitem->palette.color(dis ? QPalette::Disabled : QPalette::Current, QPalette::Text);
                }
                painter->setPen(penColor);
                //FIX: Qt5.14.2 t < 0, s.left assert failed
                const QString textToDraw = t >= 0 ? s.left(t).toString() : s.toString();
                painter->drawText(vRect, text_flags, textToDraw);
            }
            if (menuitem->menuItemType == QStyleOptionMenuItem::SubMenu)
            { // draw sub menu arrow
                int fontSize = menuitem->font.pointSize();
                QFont f(assetFont);
                f.setPointSize(qRound(fontSize * 0.9f)); // a little bit smaller
                painter->setFont(f);
                int yOfs = qRound(fontSize / 3.0f); // an offset to align the '>' with
                                                    // the baseline of the text
                QPoint tl(rect.right() - 2 * windowsArrowHMargin - contentItemHMargin, rect.top() + yOfs);
                QRect submenuRect(tl, rect.bottomRight());
                QRect vSubMenuRect = visualMenuRect(submenuRect);
                painter->setPen(option->palette.text().color());
                const bool isReverse = option->direction == Qt::RightToLeft;
                const auto str = isReverse ? ChevronLeftMed : ChevronRightMed;
                painter->drawText(vSubMenuRect, Qt::AlignCenter, str);
            }
        }
        break;
#endif // QT_CONFIG(menu)
    case CE_MenuBarEmptyArea:
    {
        break;
    }
    case CE_HeaderEmptyArea:
        break;
    case CE_HeaderLabel:
    {
        QProxyStyle::drawControl(element, option, painter, widget);
        break;
    }
    case CE_HeaderSection:
    {
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option))
        {
            painter->setPen(Qt::NoPen);
            painter->setBrush(header->palette.button());
            painter->drawRect(header->rect);

            painter->setPen(highContrastTheme == true ? header->palette.buttonText().color()
                                                      : WINUI3Colors[colorSchemeIndex][frameColorLight]);
            painter->setBrush(Qt::NoBrush);

            if (header->position == QStyleOptionHeader::OnlyOneSection)
            {
                break;
            }
            else if (header->position == QStyleOptionHeader::Beginning)
            {
                painter->drawLine(QPointF(option->rect.topRight()) + QPointF(0.5, 0.0),
                                  QPointF(option->rect.bottomRight()) + QPointF(0.5, 0.0));
            }
            else if (header->position == QStyleOptionHeader::End)
            {
                painter->drawLine(QPointF(option->rect.topLeft()) - QPointF(0.5, 0.0),
                                  QPointF(option->rect.bottomLeft()) - QPointF(0.5, 0.0));
            }
            else
            {
                painter->drawLine(QPointF(option->rect.topRight()) + QPointF(0.5, 0.0),
                                  QPointF(option->rect.bottomRight()) + QPointF(0.5, 0.0));
                painter->drawLine(QPointF(option->rect.topLeft()) - QPointF(0.5, 0.0),
                                  QPointF(option->rect.bottomLeft()) - QPointF(0.5, 0.0));
            }
            painter->drawLine(QPointF(option->rect.bottomLeft()) + QPointF(0.0, 0.5),
                              QPointF(option->rect.bottomRight()) + QPointF(0.0, 0.5));
        }
        break;
    }
    case CE_ItemViewItem:
    {
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option))
        {
            const auto p = proxy();
            QRect checkRect = p->subElementRect(SE_ItemViewItemCheckIndicator, vopt, widget);
            QRect iconRect = p->subElementRect(SE_ItemViewItemDecoration, vopt, widget);
            QRect textRect = p->subElementRect(SE_ItemViewItemText, vopt, widget);
            const QTreeView *itemTreeView = qobject_cast<const QTreeView *>(widget);
            if (!itemTreeView && widget)
            {
                const QWidget *pwidget = widget;
                while (pwidget && !itemTreeView)
                {
                    itemTreeView = qobject_cast<const QTreeView *>(pwidget);
                    pwidget = pwidget->parentWidget();
                }
            }

            const bool isNavigationTreeView =
                itemTreeView && itemTreeView->property(NavigationViewStyleProperty).toBool();

            if (isNavigationTreeView && (vopt->state & State_Children))
            {
                textRect.adjust(0, 0, -20, 0);
            }
            // draw the background
            proxy()->drawPrimitive(PE_PanelItemViewItem, option, painter, widget);

            QRect rect = vopt->rect;
            const bool isRtl = option->direction == Qt::RightToLeft;
            bool onlyOne = vopt->viewItemPosition == QStyleOptionViewItem::OnlyOne || vopt->viewItemPosition == QStyleOptionViewItem::Invalid;
            bool isFirst = vopt->viewItemPosition == QStyleOptionViewItem::Beginning;
            bool isLast = vopt->viewItemPosition == QStyleOptionViewItem::End;

            // the tree decoration already painted the left side of the
            // rounded rect
            bool isDecorationColumn = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
            isDecorationColumn = vopt->features.testFlag(QStyleOptionViewItem::IsDecoratedRootColumn);
#else
            if (qobject_cast<const QTreeView *>(widget))
            {
                // isDecorationColumn = !vopt->index.isValid() || vopt->index.column() == 0;
            }
#endif
            if (isDecorationColumn && vopt->showDecorationSelected)
            {
                isFirst = false;
                if (onlyOne)
                {
                    onlyOne = false;
                    isLast = true;
                }
            }

            if (isRtl)
            {
                if (isFirst)
                {
                    isFirst = false;
                    isLast = true;
                }
                else if (isLast)
                {
                    isFirst = true;
                    isLast = false;
                }
            }
            bool highlightCurrent = (vopt->state & (State_Selected | State_MouseOver)) != 0;
            const QTableView *tableView = qobject_cast<const QTableView *>(widget);
            bool tableCellHighlightHandled = false;
            const bool tableRowStrategy = tableView && !highContrastTheme && vopt->index.isValid()
                                          && tableView->selectionBehavior() == QAbstractItemView::SelectRows;

            if (tableRowStrategy)
            {
                bool rowSelected = (vopt->state & State_Selected) != 0;
                bool rowHovered = false;
                if (!rowSelected && tableView->viewport())
                {
                    const int hoverRow = tableView->viewport()->property("_q_table_hover_row").toInt();
                    rowHovered = (hoverRow >= 0 && hoverRow == vopt->index.row());
                }

                if (rowSelected || rowHovered)
                {
                    QColor accent = vopt->palette.highlight().color();
                    const bool isDark = colorSchemeIndex == 1;
                    accent.setAlpha(255);

                    const QRect cellRect = rect.marginsRemoved(QMargins(0, 1, 0, 1));
                    QColor fill = accent;
                    fill.setAlpha(rowSelected ? (isDark ? 56 : 40) : (isDark ? 40 : 28));
                    painter->fillRect(cellRect, fill);

                    // Selected row: draw stitched border segments across cells.
                    // While editing the current cell in SelectRows mode, keep fill only.
                    bool editingCurrentCell = false;
                    if (rowSelected && tableView->currentIndex().isValid()
                        && tableView->currentIndex().row() == vopt->index.row())
                    {
                        const QWidget *fw = QApplication::focusWidget();
                        editingCurrentCell = fw && fw != tableView && tableView->isAncestorOf(fw);
                    }

                    if (rowSelected && !editingCurrentCell)
                    {
                        const auto *model = tableView->model();
                        const int totalCols = model ? model->columnCount(vopt->index.parent()) : 0;
                        int firstVisibleCol = 0;
                        int lastVisibleCol = qMax(0, totalCols - 1);
                        for (int c = 0; c < totalCols; ++c)
                        {
                            if (!tableView->isColumnHidden(c))
                            {
                                firstVisibleCol = c;
                                break;
                            }
                        }
                        for (int c = totalCols - 1; c >= 0; --c)
                        {
                            if (!tableView->isColumnHidden(c))
                            {
                                lastVisibleCol = c;
                                break;
                            }
                        }

                        QColor border = accent;
                        border.setAlpha(isDark ? 210 : 190);
                        PainterStateGuard borderGuard(painter);
                        painter->setPen(QPen(border, 1));
                        painter->drawLine(cellRect.topLeft(), cellRect.topRight());
                        painter->drawLine(cellRect.bottomLeft(), cellRect.bottomRight());
                        if (vopt->index.column() == firstVisibleCol)
                        {
                            painter->drawLine(cellRect.topLeft(), cellRect.bottomLeft());
                        }
                        if (vopt->index.column() == lastVisibleCol)
                        {
                            painter->drawLine(cellRect.topRight(), cellRect.bottomRight());
                        }
                    }
                    tableCellHighlightHandled = true;
                }
            }
            //ListView
            else if (highlightCurrent)
            {
                if (highContrastTheme)
                {
                    painter->setBrush(vopt->palette.highlight());
                }
                else
                {
                    const bool isSelected = (vopt->state & State_Selected) != 0;
                    const QColor itemHighlight = isSelected ? winUI3Color(subtlePressedColor)
                                                            : winUI3Color(subtleHighlightColor);
                    painter->setBrush(itemHighlight);
                }
            }
            else
            {
                painter->setBrush(vopt->backgroundBrush);
            }
            painter->setPen(Qt::NoPen);

            // QTableView row strategy: selected/hover painted per-cell above.
            if (tableRowStrategy && tableCellHighlightHandled)
            {
                // no-op
            }
            else if (onlyOne)
            {
                painter->drawRoundedRect(
                    rect.marginsRemoved(QMargins(2, 2, 2, 2)), secondLevelRoundingRadius, secondLevelRoundingRadius);
            }
            else if (isFirst)
            {
                PainterStateGuard psg(painter);
                painter->setClipRect(rect);
                painter->drawRoundedRect(rect.marginsRemoved(QMargins(2, 2, -secondLevelRoundingRadius, 2)),
                                         secondLevelRoundingRadius,
                                         secondLevelRoundingRadius);
            }
            else if (isLast)
            {
                PainterStateGuard psg(painter);
                painter->setClipRect(rect);
                painter->drawRoundedRect(rect.marginsRemoved(QMargins(-secondLevelRoundingRadius, 2, 2, 2)),
                                         secondLevelRoundingRadius,
                                         secondLevelRoundingRadius);
            }
            else
            {
                painter->drawRect(rect.marginsRemoved(QMargins(0, 2, 0, 2)));
            }

            // draw the check mark
            if (vopt->features & QStyleOptionViewItem::HasCheckIndicator)
            {
                QStyleOptionViewItem option(*vopt);
                option.rect = checkRect;
                option.state = option.state & ~QStyle::State_HasFocus;

                switch (vopt->checkState)
                {
                case Qt::Unchecked:
                    option.state |= QStyle::State_Off;
                    break;
                case Qt::PartiallyChecked:
                    option.state |= QStyle::State_NoChange;
                    break;
                case Qt::Checked:
                    option.state |= QStyle::State_On;
                    break;
                }
                proxy()->drawPrimitive(PE_IndicatorItemViewItemCheck, &option, painter, widget);
            }

            // draw the icon
            if (iconRect.isValid())
            {
                QIcon::Mode mode = QIcon::Normal;
                if (!(vopt->state & QStyle::State_Enabled))
                {
                    mode = QIcon::Disabled;
                }
                QIcon::State state = vopt->state & QStyle::State_Open ? QIcon::On : QIcon::Off;
                const bool isSettingsSpinItem = vopt->index.isValid() && vopt->index.data(NavigationSettingsSpinRole).toBool();
                if (isNavigationTreeView && isSettingsSpinItem)
                {
                    PainterStateGuard psg(painter);
                    const QByteArray animKey = navigationSettingsAnimKey(itemTreeView, vopt->index);
                    qreal iconAngle = 0.0;
                    if (const QNumberStyleAnimation *animation =
                            qobject_cast<QNumberStyleAnimation *>(getAnimationEx(const_cast<QTreeView *>(itemTreeView), animKey)))
                    {
                        iconAngle = animation->currentValue();
                    }
                    const int side = qMin(iconRect.width(), iconRect.height());
                    const QRect squareRect(iconRect.x() + (iconRect.width() - side) / 2,
                                           iconRect.y() + (iconRect.height() - side) / 2,
                                           side,
                                           side);
                    const QPointF center = squareRect.center();
                    QRectF drawRect(-side / 2.0, -side / 2.0, side, side);
                    painter->translate(center);
                    painter->rotate(iconAngle);
                    const QString iconCode = vopt->index.data(NavigationIconRole).toString();
                    if (!iconCode.isEmpty())
                    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                        const int supersample = 2;
                        const int glyphSide = qMax(16, side * supersample);
                        const QPixmap glyphPixmap =
                            navigationGlyphPixmapCached(iconCode, glyphSide, vopt->palette.text().color());
                        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
                        painter->drawPixmap(drawRect, glyphPixmap, QRectF(glyphPixmap.rect()));
#else
                        QFont iconFont(assetFont);
                        iconFont.setPixelSize(17);
                        painter->setFont(iconFont);
                        painter->setPen(vopt->palette.text().color());
                        painter->setRenderHint(QPainter::TextAntialiasing, true);
                        painter->drawText(drawRect, Qt::AlignCenter, iconCode);
#endif
                    }
                    else
                    {
                        vopt->icon.paint(painter, drawRect.toRect(), vopt->decorationAlignment, mode, state);
                    }
                }
                else
                {
                    vopt->icon.paint(painter, iconRect, vopt->decorationAlignment, mode, state);
                }
            }

            painter->setPen(highlightCurrent && highContrastTheme ? vopt->palette.base().color() : vopt->palette.text().color());
            viewItemDrawText(painter, vopt, textRect);

            if (!isNavigationTreeView)
            {
                drawListViewIndicator(vopt, painter, widget);
            }
            if (isNavigationTreeView)
            {
                const bool isIconMode =
                    (widget && widget->property("navigationIconMode").toBool()) ||
                    (itemTreeView && itemTreeView->property("navigationIconMode").toBool());
                if ((vopt->state & State_Children) && !isIconMode)
                {
                    const bool isOpen = vopt->state & State_Open;
                    static QFont f = assetFont;
                    f.setPixelSize(15);
                    painter->setFont(f);
                    painter->setPen(vopt->palette.text().color());

                    const QRect arrowRect = rect.adjusted(rect.width() - 22, 0, -6, 0);

                    const QByteArray animKey = QByteArrayLiteral("_q_nav_branch_anim_") + QByteArray::number(vopt->rect.y());
                    const QNumberStyleAnimation *animation =
                        qobject_cast<QNumberStyleAnimation *>(getAnimationEx(const_cast<QWidget *>(widget), animKey));
                    qreal angle = isOpen ? 180.0 : 0.0;
                    if (animation)
                    {
                        angle = animation->currentValue();
                    }

                    PainterStateGuard arrowGuard(painter);
                    QRectF arrowRectF(arrowRect);

                    const QPointF center = arrowRectF.center();
                    painter->translate(center);
                    painter->rotate(angle);
                    QRectF r(-arrowRectF.width() / 2.0, -arrowRectF.height() / 2.0, arrowRectF.width(), arrowRectF.height());
                    painter->drawText(r, Qt::AlignCenter, ChevronDownMed);
                }
                drawNavigationViewIndicator(vopt, painter, widget);
            }
        }
        break;
    }
    case CE_DockWidgetTitle:
        if (const QStyleOptionDockWidget *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(option))
        {
            const QDockWidget *dockWidget = qobject_cast<const QDockWidget *>(widget);
            QRect rect = option->rect;
            if (dockWidget && dockWidget->isFloating())
            {
                QCommonStyle::drawControl(element, option, painter, widget);
                break; // otherwise fall through
            }

            const bool verticalTitleBar = dwOpt->verticalTitleBar;

            if (verticalTitleBar)
            {
                rect = rect.transposed();

                painter->translate(rect.left() - 1, rect.top() + rect.width());
                painter->rotate(-90);
                painter->translate(-rect.left() + 1, -rect.top());
            }

            painter->setBrush(winUI3Color(fillControlAltSecondary));
            painter->setPen(Qt::NoPen);
            painter->drawRect(rect.adjusted(0, 1, -1, -3));

            int buttonMargin = 4;
            int mw = proxy()->pixelMetric(PM_DockWidgetTitleMargin, dwOpt, widget);
            int fw = proxy()->pixelMetric(PM_DockWidgetFrameWidth, dwOpt, widget);
            const QDockWidget *dw = qobject_cast<const QDockWidget *>(widget);
            bool isFloating = dw && dw->isFloating();

            QRect r = option->rect.adjusted(0, 2, -1, -3);
            QRect titleRect = r;

            if (dwOpt->closable)
            {
                QSize sz = proxy()->standardIcon(QStyle::SP_TitleBarCloseButton, dwOpt, widget).actualSize(QSize(15, 15));
                titleRect.adjust(0, 0, -sz.width() - mw - buttonMargin, 0);
            }

            if (dwOpt->floatable)
            {
                QSize sz = proxy()->standardIcon(QStyle::SP_TitleBarMaxButton, dwOpt, widget).actualSize(QSize(15, 15));
                titleRect.adjust(0, 0, -sz.width() - mw - buttonMargin, 0);
            }

            if (isFloating)
            {
                titleRect.adjust(0, -fw, 0, 0);
                if (widget && widget->windowIcon().cacheKey() != QApplication::windowIcon().cacheKey())
                {
                    titleRect.adjust(titleRect.height() + mw, 0, 0, 0);
                }
            }
            else
            {
                titleRect.adjust(mw, 0, 0, 0);
                if (!dwOpt->floatable && !dwOpt->closable)
                {
                    titleRect.adjust(0, 0, -mw, 0);
                }
            }
            if (!verticalTitleBar)
            {
                titleRect = visualRect(dwOpt->direction, r, titleRect);
            }

            if (!dwOpt->title.isEmpty())
            {
                QString titleText = painter->fontMetrics().elidedText(
                    dwOpt->title, Qt::ElideRight, verticalTitleBar ? titleRect.height() : titleRect.width());
                const int indent = 4;
                painter->setPen(controlTextColor(option));
                drawItemText(painter,
                             rect.adjusted(indent + 1, 1, -indent - 1, -1),
                             Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                             dwOpt->palette,
                             dwOpt->state & State_Enabled,
                             titleText,
                             QPalette::WindowText);
            }
        }
        break;
    case CE_ToolBar:
        break;
    default:
        QProxyStyle::drawControl(element, option, painter, widget);
        break;
    }
    painter->restore();
}

int FluentUI3Style::styleHint(StyleHint hint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const
{
    switch (hint)
    {
    case SH_Menu_AllowActiveAndDisabled:
        return 0;
    case SH_GroupBox_TextLabelColor:
        if (opt != nullptr && widget != nullptr)
        {
            return opt->palette.text().color().rgba();
        }
        return 0;
    case SH_ItemView_ShowDecorationSelected:
        return 1;
    case SH_TabBar_Alignment:
        if (widget && widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Navigation)
        {
            return Qt::AlignLeft;
        }
        break;
    case SH_Slider_AbsoluteSetButtons:
        return Qt::LeftButton;
    case SH_Slider_PageSetButtons:
        return 0;
    case SH_DockWidget_ButtonsHaveFrame:
        return 1;
    case SH_ComboBox_Popup:
    {
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
            cmb && qApp->property("_q_scrollHint_center").toBool())
        {
            return !cmb->editable;
        }
        return 0;
    }
    default:
        break;
    }
    return QProxyStyle::styleHint(hint, opt, widget, returnData);
}

static bool isComboBoxPopup(QWidget *w)
{
    if (!w)
    {
        return false;
    }

    return w->inherits("QComboBoxPrivateContainer");

    // ComboBox popup 一定是 Popup
    if (w->windowType() != Qt::Popup)
    {
        return false;
    }

    // Menu 已单独处理
    if (qobject_cast<QMenu *>(w))
    {
        return false;
    }

    // ScrollBar 绝对不是
    if (qobject_cast<QScrollBar *>(w))
    {
        return false;
    }

    // parent 链中有 QComboBox
    QWidget *p = w->parentWidget();
    while (p)
    {
        if (qobject_cast<QComboBox *>(p))
        {
            return true;
        }
        p = p->parentWidget();
    }

    return false;
}

QSize FluentUI3Style::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
    QSize contentSize(size);

    switch (type)
    {
#if QT_CONFIG(menubar)
    case CT_MenuBarItem:
        if (!contentSize.isEmpty())
        {
            constexpr int hMargin = 2 * 6;
            constexpr int hPadding = 2 * 11;
            constexpr int itemHeight = 32;
            contentSize.setWidth(contentSize.width() + hMargin + hPadding);
#if QT_CONFIG(tabwidget)
            if (widget->parent() && !qobject_cast<const QTabWidget *>(widget->parent()))
#endif
                contentSize.setHeight(itemHeight);
        }
        break;
#endif
#if QT_CONFIG(menu)
    case CT_MenuItem:
        if (const auto *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            int width = size.width();
            int height;
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator)
            {
                width = 10;
                height = 3;
            }
            else
            {
                height = menuItem->fontMetrics.height() + 8;
                constexpr int fluentMenuItemHeight = 34; // WinUI3 menu item height, including 3px top and bottom margins
                height = height < fluentMenuItemHeight ? fluentMenuItemHeight : height;
                if (!menuItem->icon.isNull())
                {
                    int iconExtent = proxy()->pixelMetric(PM_SmallIconSize, option, widget);
                    height = qMax(height, menuItem->icon.actualSize(QSize(iconExtent, iconExtent)).height() + 4);
                }
            }
            if (menuItem->text.contains(u'\t'))
            {
                width += contentItemHMargin; // the text width is already in
            }
            if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu)
            {
                width += 2 * windowsArrowHMargin + contentItemHMargin;
            }
            if (menuItem->menuItemType == QStyleOptionMenuItem::DefaultItem)
            {
                const QFontMetrics fm(menuItem->font);
                QFont fontBold = menuItem->font;
                fontBold.setBold(true);
                const QFontMetrics fmBold(fontBold);
                width += fmBold.horizontalAdvance(menuItem->text) - fm.horizontalAdvance(menuItem->text);
            }
            // in contrast to windowsvista, the checkmark and icon are drawn
            // separately
            if (menuItem->menuHasCheckableItems)
            {
                const auto checkMarkWidth = proxy()->pixelMetric(PM_IndicatorWidth, option, widget);
                width += checkMarkWidth + contentItemHMargin * 2;
            }
            // we have an icon and it's already in the given size, only add
            // margins 4 is added in qmenu.cpp to PM_SmallIconSize
            if (menuItem->maxIconWidth > 0)
            {
                width += contentItemHMargin * 2 + menuItem->maxIconWidth - 4;
            }
            width += 2 * 2; // margins for rounded border
            width += 2 * contentHMargin;
            if (width < 100) // minimum size
            {
                width = 100;
            }
            contentSize = QSize(width, height);
        }
        break;
#endif // QT_CONFIG(menu)
#if QT_CONFIG(spinbox)
    case CT_SpinBox:
    {
        if (const auto *spinBoxOpt = qstyleoption_cast<const QStyleOptionSpinBox *>(option))
        {
            // Add button + frame widths
            // const qreal dpi       = QStyleHelper::dpi( option );
            const bool hasButtons = (spinBoxOpt->buttonSymbols != QAbstractSpinBox::NoButtons);
            const int margins = 8;
            // const int buttonWidth = hasButtons ? qRound( QStyleHelper::dpiScaled( 16, dpi ) ) : 0;
            const int buttonWidth = hasButtons ? 16 + contentItemHMargin : 0;
            const int frameWidth = spinBoxOpt->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, spinBoxOpt, widget) : 0;

            contentSize += QSize(2 * buttonWidth + 2 * frameWidth + 2 * margins, 2 * frameWidth);
        }
        break;
    }
#endif
#if QT_CONFIG(combobox)
    case CT_ComboBox:
        if (const auto *comboBoxOpt = qstyleoption_cast<const QStyleOptionComboBox *>(option))
        {
            contentSize = QProxyStyle::sizeFromContents(type, option, size,
                                                        widget); // don't rely on QWindowsThemeData
            contentSize += QSize(4, 4);                          // default win11 style margins
            if (comboBoxOpt->subControls & SC_ComboBoxArrow)
            {
                const auto w = proxy()->pixelMetric(PM_MenuButtonIndicator, option, widget);
                contentSize.rwidth() += w + contentItemHMargin;
            }
        }
        break;
#endif
    case CT_ProgressBar:
        if (widget && widget->property(ProgressBarStyleProperty).toInt() == ProgressBarRing)
        {
            // Fluent UI guidance uses a 20x20 minimum for ProgressRing.
            constexpr int ringMinSize = 0;
            contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
            contentSize = contentSize.expandedTo(QSize(ringMinSize, ringMinSize));
        }
        else
        {
            contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
        }
        break;
    case CT_HeaderSection:
        contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
        break;
    case CT_RadioButton:
    case CT_CheckBox:
        if (const auto *buttonOpt = qstyleoption_cast<const QStyleOptionButton *>(option))
        {
            const auto p = proxy();
            const bool isRadio = (type == CT_RadioButton);

            const int width = p->pixelMetric(isRadio ? PM_ExclusiveIndicatorWidth : PM_IndicatorWidth, option, widget);
            const int height = p->pixelMetric(isRadio ? PM_ExclusiveIndicatorHeight : PM_IndicatorHeight, option, widget);

            int margins = 2 * contentItemHMargin;
            if (!buttonOpt->icon.isNull() || !buttonOpt->text.isEmpty())
            {
                margins += p->pixelMetric(isRadio ? PM_RadioButtonLabelSpacing : PM_CheckBoxLabelSpacing, option, widget);
            }

            contentSize += QSize(width + margins, 4);
            contentSize.setHeight(qMax(size.height(), height + 2 * contentItemHMargin));
        }
        break;

        // the indicator needs 2px more in width when there is no text, not
        // needed when the style draws the text
        contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
        if (size.width() == 0)
        {
            contentSize.rwidth() += 2;
        }
        break;
    case CT_PushButton:
    {
        contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
        // we want our own horizontal spacing
        const int oldMargin = proxy()->pixelMetric(PM_ButtonMargin, option, widget);
        contentSize.rwidth() += 2 * contentHMargin - oldMargin;
        break;
    }
        // or ui setHeight
    case CT_ToolButton:
    {
        contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
        if (auto optionTbtn = qstyleoption_cast<const QStyleOptionToolButton *>(option))
        {
            if (optionTbtn->toolButtonStyle == Qt::ToolButtonIconOnly)
            {
                contentSize += QSize(4, 4);
            }
            if (optionTbtn->features & QStyleOptionToolButton::HasMenu && optionTbtn->toolButtonStyle == Qt::ToolButtonTextBesideIcon)
            {
                const int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, option, widget);
                contentSize.rwidth() += mbi + 4;
            }
            else if (optionTbtn->features & QStyleOptionToolButton::HasMenu && optionTbtn->toolButtonStyle == Qt::ToolButtonTextUnderIcon)
            {
                const int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, option, widget);
                contentSize.rwidth() += mbi + 4;
            }
            else if (optionTbtn->features & QStyleOptionToolButton::HasMenu && optionTbtn->toolButtonStyle == Qt::ToolButtonTextOnly)
            {
                const int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, option, widget);
                contentSize.rwidth() += mbi + 4;
            }
        }
        break;
    }
    case CT_ItemViewItem:
    {
        if (auto view = qobject_cast<const QListView *>(widget))
        {
            if (view->parentWidget() && view->parentWidget()->parentWidget() && qobject_cast<const QComboBox *>(view->parentWidget()->parentWidget()))
            {
                contentSize.setHeight(32); // ComboBox dropdown item
                break;
            }
        }

        // FluentUI 3 列表项标准常量
        const int FLUENT_H_MARGIN = 12;    // 水平边距（左右各12px）
        const int FLUENT_ITEM_HEIGHT = 38; // 常规列表项总高度
        if (const auto *viewItemOpt = qstyleoption_cast<const QStyleOptionViewItem *>(option))
        {
            if (const QListView *lv = qobject_cast<const QListView *>(widget); lv && lv->viewMode() != QListView::IconMode)
            {
                QStyleOptionViewItem vOpt(*viewItemOpt);
                vOpt.rect.setRight(vOpt.rect.right() - FLUENT_H_MARGIN);
                contentSize = QProxyStyle::sizeFromContents(type, &vOpt, size, widget);
                contentSize.rwidth() += FLUENT_H_MARGIN;
                contentSize.setHeight(FLUENT_ITEM_HEIGHT);
            }
            else if (auto view = qobject_cast<const QListView *>(widget))
            {
                if (view->parentWidget() && view->parentWidget()->parentWidget() && qobject_cast<const QComboBox *>(view->parentWidget()->parentWidget()))
                {
                    contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                    contentSize.setHeight(32);
                    return contentSize;
                }
            }
            else if (auto view = qobject_cast<const QTreeView *>(widget); view && view->property("ItemHeight").toInt() > 0)
            {
                contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                int itemHeight = view->property("ItemHeight").toInt();
                if (itemHeight > 0)
                {
                    contentSize.setHeight(itemHeight);
                }
            }
            else
            {
                contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                contentSize.setHeight(FLUENT_ITEM_HEIGHT);
            }
        }
        break;
    }
    case CT_TabBarTab:
    {
        contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
        contentSize.setHeight(32);

        const int tabBarStyle = widget ? widget->property(TabBarStyleProperty).toInt() : 0;
        const bool isPivotStyle = tabBarStyle == TabBarStyle::Pivot_Grow || tabBarStyle == TabBarStyle::Pivot_Slide
                                  || tabBarStyle == TabBarStyle::Pivot_Stretch;
        if (isPivotStyle)
        {
            constexpr int pivotIndicatorHeight = 3;
            constexpr int pivotTextTopPadding = 6;
            constexpr int pivotIndicatorTopGap = 5;
            constexpr int pivotIndicatorBottomPadding = 4;

            int contentVisualHeight = option->fontMetrics.height();
            if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option))
            {
                contentVisualHeight = qMax(contentVisualHeight, tab->iconSize.height());
            }

            const int pivotHeight = pivotTextTopPadding + contentVisualHeight + pivotIndicatorTopGap + pivotIndicatorHeight
                                    + pivotIndicatorBottomPadding;
            constexpr int pivotMinTabWidth = 72;
            // contentSize.setWidth(qMax(contentSize.width(), pivotMinTabWidth));
            contentSize.setHeight(qMax(contentSize.height(), pivotHeight));
        }

        if (widget && tabBarStyle == TabBarStyle::Capsule)
        {
            contentSize.setWidth(contentSize.width() + 30);
        }
        if (widget &&
            (widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Segmented_Slide || widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Segmented_Fade))
        {
            contentSize.setHeight(contentSize.height() + 10);
        }
        else if (widget && widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Segmented_WinUI3)
        {
            if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option))
            {
                if (tab->text.isEmpty())
                {
                    // contentSize.setWidth(32);
                }
                else if (!tab->text.isEmpty() && tab->icon.isNull())
                {
                }
                else
                {
                    contentSize.setWidth(qMax(contentSize.width(), 130));
                }
            }
        }

        if (widget && widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Navigation)
        {
            if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option))
            {
                const bool isVerticalNavigation = tab->shape == QTabBar::RoundedWest || tab->shape == QTabBar::RoundedEast || tab->shape == QTabBar::TriangularWest || tab->shape == QTabBar::TriangularEast;
                if (isVerticalNavigation)
                {
                    constexpr int indicatorWidth = 3;
                    constexpr int indicatorContentGap = 2;
                    constexpr int contentLeadingInset = indicatorWidth + indicatorContentGap;
                    constexpr int contentTrailingInset = 8;
                    constexpr int iconTextSpacing = 8;

                    if (const auto *tabBar = qobject_cast<const QTabBar *>(widget))
                    {
                        QStringList cacheParts;
                        cacheParts.reserve(tabBar->count() + 2);
                        cacheParts << QString::number(tabBar->count()) << tabBar->font().toString();
                        for (int i = 0; i < tabBar->count(); ++i)
                        {
                            cacheParts << tabBar->tabText(i);
                            cacheParts << QString::number(tabBar->tabIcon(i).cacheKey());
                        }
                        const QString cacheKey = cacheParts.join(u'|');
                        const QString storedCacheKey = tabBar->property("_q_navigation_tab_width_cache_key").toString();

                        int cachedTextWidth = tabBar->property("_q_navigation_longest_text_width").toInt();
                        int cachedIconWidth = tabBar->property("_q_navigation_widest_icon_width").toInt();

                        if (storedCacheKey != cacheKey || cachedTextWidth <= 0)
                        {
                            int longestTextWidth = 0;
                            int widestIconWidth = 0;
                            QSize iconSize = tab->iconSize;
                            if (!iconSize.isValid())
                            {
                                const int iconExtent = proxy()->pixelMetric(QStyle::PM_SmallIconSize, option, widget);
                                iconSize = QSize(iconExtent, iconExtent);
                            }

                            for (int i = 0; i < tabBar->count(); ++i)
                            {
                                longestTextWidth = qMax(longestTextWidth, tab->fontMetrics.horizontalAdvance(tabBar->tabText(i)));

                                const QIcon icon = tabBar->tabIcon(i);
                                if (!icon.isNull())
                                {
                                    const QSize actualIconSize = icon.actualSize(iconSize, QIcon::Normal, QIcon::Off);
                                    widestIconWidth = qMax(widestIconWidth, qMax(16, actualIconSize.width()));
                                }
                            }

                            const_cast<QTabBar *>(tabBar)->setProperty("_q_navigation_tab_width_cache_key", cacheKey);
                            const_cast<QTabBar *>(tabBar)->setProperty("_q_navigation_longest_text_width", longestTextWidth);
                            const_cast<QTabBar *>(tabBar)->setProperty("_q_navigation_widest_icon_width", widestIconWidth);
                            cachedTextWidth = longestTextWidth;
                            cachedIconWidth = widestIconWidth;
                        }

                        const int contentWidth = contentLeadingInset + cachedIconWidth + iconTextSpacing + cachedTextWidth + contentTrailingInset;
                        contentSize.setWidth(contentWidth);
                    }
                    else
                    {
                        const int iconWidth = tab->icon.isNull() ? 0 : qMax(16, tab->iconSize.width());
                        const int textWidth = tab->fontMetrics.horizontalAdvance(tab->text);
                        const int contentWidth = contentLeadingInset + iconWidth + iconTextSpacing + textWidth + contentTrailingInset;
                        contentSize.setWidth(contentWidth);
                    }
                    contentSize.setHeight(qMax(40, contentSize.height()));
                }
            }
        }
    }
    break;
    default:
        contentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
        break;
    }

    switch (type)
    {
    case CT_PushButton:
    case CT_ToolButton:
        // 阴影高度
        if (contentSize.height() < 36)
        {
            contentSize.setHeight(36);
        }
        break;
    case CT_CheckBox:
    case CT_RadioButton:
    case CT_ComboBox:
        if (contentSize.height() < 34)
        {
            contentSize.setHeight(34);
        }
        break;
    case CT_LineEdit:
    case CT_SpinBox:
        if (contentSize.height() < 34)
        {
            contentSize.setHeight(34);
        }
        break;
    default:
        break;
    }

    return contentSize;
}

int FluentUI3Style::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    int res = 0;

    switch (metric)
    {
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
    {
        if (widget && widget->property(SwitchStyleProperty).toBool())
        {
            return PM_IndicatorWidth == metric ? 40 : 20;
        }
    }
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
        res = 18;
        break;
    case PM_SliderThickness: // full height of a slider
        if (auto opt = qstyleoption_cast<const QStyleOptionSlider *>(option))
        {
            // hard-coded in qslider.cpp, but we need a little bit more
            constexpr auto TickSpace = 5;
            if (opt->tickPosition & QSlider::TicksAbove)
            {
                res += 6 - TickSpace;
            }
            if (opt->tickPosition & QSlider::TicksBelow)
            {
                res += 6 - TickSpace;
            }
        }
        Q_FALLTHROUGH();
    case PM_SliderControlThickness: // size of the control handle
    case PM_SliderLength:           // same because handle is a circle with r=8
        res += 2 * 10;
        break;
    case PM_RadioButtonLabelSpacing:
    case PM_CheckBoxLabelSpacing:
        res = 2 * contentItemHMargin;
        break;
    case PM_TitleBarButtonIconSize:
        res = 16;
        break;
    case PM_TitleBarButtonSize:
        res = 32;
        break;
    case PM_ScrollBarExtent:
        res = 12;
        break;
    case PM_SubMenuOverlap:
        res = -1;
        break;
    case PM_MenuButtonIndicator:
    {
        res = contentItemHMargin;
        if (widget)
        {
            const int fontSize = widget->font().pixelSize();
            QFont f(assetFont);
            f.setPixelSize(qRound(fontSize * 0.9f) <= 0 ? 1 : qRound(fontSize * 0.9f)); // a little bit smaller
            QFontMetrics fm(f);
            res += fm.horizontalAdvance(ChevronDownMed);
        }
        else
        {
            res += 12;
        }
        break;
    }
    case PM_DefaultFrameWidth:
    {
        res = 4;
        if (qobject_cast<const QTableView *>(widget))
        {
            // Keep table header visually flush with outer frame.
            res = 0;
        }
        if (qobject_cast<const QListView *>(widget))
        {
            res = 2;
        }
        if (isComboBoxPopup(const_cast<QWidget *>(widget)))
        {
            res = 2 + cBShadowBorderWidth;
        }
    }
    break;
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        res = 0;
        break;
    case PM_TreeViewIndentation:
        res = 30;
        break;
    case PM_DockWidgetTitleBarButtonMargin:
        res = 12;
        break;
    case PM_TabBarTabShiftVertical:
        res = 0;
        break;
    default:
        res = QProxyStyle::pixelMetric(metric, option, widget);
    }

    return res;
}

void FluentUI3Style::polish(QPalette &result)
{
    colorSchemeIndex = getColorSchemeIndex();
    PaletteManager::instance().applyPalette(result, colorSchemeIndex);
}

void FluentUI3Style::polish(QApplication *app)
{
    QProxyStyle::polish(app);
}

void FluentUI3Style::polish(QWidget *widget)
{
#if QT_CONFIG(commandlinkbutton)
    if (!qobject_cast<QCommandLinkButton *>(widget))
#endif // QT_CONFIG(commandlinkbutton)
        QProxyStyle::polish(widget);

    if (widget->inherits("QRollEffect"))
    {
        widget->setAttribute(Qt::WA_TranslucentBackground);
        widget->setWindowFlag(Qt::FramelessWindowHint);
        widget->setAttribute(Qt::WA_NoSystemBackground, false);
    }

    ///没有动态更新主题的需求，可屏蔽
    if (auto le = qobject_cast<QLineEdit *>(widget))
    {
        if (!le->isClearButtonEnabled())
        {
            return;
        }

        QToolButton *btn = le->findChild<QToolButton *>();
        if (btn)
        {
            QIcon icon = standardIcon(QStyle::SP_LineEditClearButton, nullptr, le);
            btn->setIcon(icon);
        }
    }
    if (auto tooBtn = qobject_cast<QToolButton *>(widget);
        tooBtn && (widget->objectName() == "ScrollLeftButton" || widget->objectName() == "ScrollRightButton"))
    {
        tooBtn->setAutoRaise(true);
    }

    if (qobject_cast<QTabBar *>(widget) && widget->property(TabBarStyleProperty).toInt() == TabBarStyle::Segmented_WinUI3)
    {
        widget->installEventFilter(this);
    }
    if (auto *treeView = qobject_cast<QTreeView *>(widget);
        treeView && treeView->property(NavigationViewStyleProperty).toBool())
    {
        if (treeView->viewport())
            treeView->viewport()->installEventFilter(this);
    }

    const bool isScrollBar = qobject_cast<QScrollBar *>(widget);
    const bool isMenu = qobject_cast<QMenu *>(widget);
    const bool isComboPopup = isComboBoxPopup(widget);

    if (isScrollBar)
    {
        bool wasCreated = widget->testAttribute(Qt::WA_WState_Created);
        bool layoutDirection = widget->testAttribute(Qt::WA_RightToLeft);
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
        widget->setAttribute(Qt::WA_TranslucentBackground);

        widget->setWindowFlag(Qt::NoDropShadowWindowHint);
        widget->setAttribute(Qt::WA_RightToLeft, layoutDirection);
        widget->setAttribute(Qt::WA_WState_Created, wasCreated);
    }
    else if (isMenu || isComboPopup)
    {
        bool wasCreated = widget->testAttribute(Qt::WA_WState_Created);
        bool layoutDirection = widget->testAttribute(Qt::WA_RightToLeft);

        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
        widget->setAttribute(Qt::WA_TranslucentBackground);

        widget->setWindowFlag(Qt::FramelessWindowHint);
        widget->setWindowFlag(Qt::Popup);
        widget->setWindowFlag(Qt::NoDropShadowWindowHint);

        widget->setAttribute(Qt::WA_RightToLeft, layoutDirection);
        widget->setAttribute(Qt::WA_WState_Created, wasCreated);
    }
    // else if ( isToolTip )
    // {
    //     // FluentUI ToolTip styling
    //     bool wasCreated      = widget->testAttribute( Qt::WA_WState_Created );
    //     bool layoutDirection = widget->testAttribute( Qt::WA_RightToLeft );

    //     widget->setAttribute( Qt::WA_OpaquePaintEvent, false );
    //     widget->setAttribute( Qt::WA_TranslucentBackground );
    //     widget->setWindowFlag( Qt::FramelessWindowHint );
    //     widget->setWindowFlag( Qt::ToolTip );
    //     widget->setWindowFlag( Qt::NoDropShadowWindowHint );

    //     widget->setAttribute( Qt::WA_RightToLeft, layoutDirection );
    //     widget->setAttribute( Qt::WA_WState_Created, wasCreated );
    // }
    else if (QComboBox *cb = qobject_cast<QComboBox *>(widget))
    {
        if (cb->isEditable())
        {
            QLineEdit *le = cb->lineEdit();
            le->setFrame(false);
        }
    }
    else if (const auto *scrollarea = qobject_cast<QAbstractScrollArea *>(widget); scrollarea && !qobject_cast<QGraphicsView *>(widget)
#if QT_CONFIG(mdiarea)
                                                                                   && !qobject_cast<QMdiArea *>(widget)
#endif
    )
    {
        if (scrollarea->frameShape() == QFrame::StyledPanel)
        {
            const auto vp = scrollarea->viewport();
            const bool isAutoFillBackground = vp->autoFillBackground();
            const bool isStyledBackground = vp->testAttribute(Qt::WA_StyledBackground);
            vp->setProperty("_q_original_autofill_background", isAutoFillBackground);
            vp->setProperty("_q_original_styled_background", isStyledBackground);
            vp->setAutoFillBackground(false);
            vp->setAttribute(Qt::WA_StyledBackground, true);
        }
        // QTreeView & QListView are already set in the base windowsvista style
        if (auto table = qobject_cast<QTableView *>(widget))
        {
            table->viewport()->setAttribute(Qt::WA_Hover, true);
            table->viewport()->setMouseTracking(true);
            table->viewport()->installEventFilter(this);
        }
    }

    // QTreeView 提前连接展开/折叠信号，在节点展开/折叠时立即启动箭头旋转动画，
    if (auto treeView = qobject_cast<QTreeView *>(widget); treeView && treeView->isAnimated())
    {
        auto *ctx = new QObject(treeView);
        treeView->setProperty("_q_branch_anim_ctx", QVariant::fromValue(ctx));
        auto triggerBranchAnimation = [treeView](const QModelIndex &index, bool opening)
        {
            if (!treeView || !transitionsEnabled())
            {
                return;
            }
            const QRect vr = treeView->visualRect(index);
            const bool isNavView = treeView->property(NavigationViewStyleProperty).toBool();
            const QByteArray animKey = (isNavView ? QByteArrayLiteral("_q_nav_branch_anim_")
                                                  : QByteArrayLiteral("_q_branch_anim_")) +
                                       QByteArray::number(vr.y());

            const bool isReverse = treeView->layoutDirection() == Qt::RightToLeft;
            const qreal closedAngle = 0.0;
            const qreal openAngle = isNavView ? 180.0 : (isReverse ? -90.0 : 90.0);

            QNumberStyleAnimation *existAnim =
                qobject_cast<QNumberStyleAnimation *>(getAnimationEx(treeView, animKey));
            qreal startAngle = existAnim ? existAnim->currentValue() : (opening ? closedAngle : openAngle);

            QNumberStyleAnimation *t = new QNumberStyleAnimation(treeView);
            t->setStartValue(startAngle);
            t->setEndValue(opening ? openAngle : closedAngle);
            t->setDuration(200);
            t->setFrameRate(QStyleAnimation::DefaultFps);
            startAnimationEx(t, treeView, animKey);
        };

        QObject::connect(treeView, &QTreeView::expanded, ctx,
                         [triggerBranchAnimation](const QModelIndex &index)
                         { triggerBranchAnimation(index, true); });
        QObject::connect(treeView, &QTreeView::collapsed, ctx,
                         [triggerBranchAnimation](const QModelIndex &index)
                         { triggerBranchAnimation(index, false); });
    }
}

void FluentUI3Style::unpolish(QWidget *widget)
{
#if QT_CONFIG(commandlinkbutton)
    if (!qobject_cast<QCommandLinkButton *>(widget))
#endif // QT_CONFIG(commandlinkbutton)
        QProxyStyle::unpolish(widget);

    if (qobject_cast<QTabBar *>(widget) && widget->property("TabBarStyle").toInt() == TabBarStyle::Segmented_WinUI3)
    {
        widget->removeEventFilter(this);
    }
    if (auto *treeView = qobject_cast<QTreeView *>(widget);
        treeView && treeView->property(NavigationViewStyleProperty).toBool())
    {
        if (treeView->viewport())
            treeView->viewport()->removeEventFilter(this);
    }
    if (auto *tableView = qobject_cast<QTableView *>(widget))
    {
        if (tableView->viewport())
            tableView->viewport()->removeEventFilter(this);
    }

    if (const auto *scrollarea = qobject_cast<QAbstractScrollArea *>(widget); scrollarea
#if QT_CONFIG(mdiarea)
                                                                              && !qobject_cast<QMdiArea *>(widget)
#endif
    )
    {
        const auto vp = scrollarea->viewport();
        const auto wasAutoFillBackground = vp->property("_q_original_autofill_background").toBool();
        vp->setAutoFillBackground(wasAutoFillBackground);
        vp->setProperty("_q_original_autofill_background", QVariant());
        const auto origStyledBackground = vp->property("_q_original_styled_background").toBool();
        vp->setAttribute(Qt::WA_StyledBackground, origStyledBackground);
        vp->setProperty("_q_original_styled_background", QVariant());
    }

    if (auto treeView = qobject_cast<QTreeView *>(widget))
    {
        auto *ctx = treeView->property("_q_branch_anim_ctx").value<QObject *>();
        if (ctx)
        {
            delete ctx;
            treeView->setProperty("_q_branch_anim_ctx", QVariant());
        }
    }
}

void FluentUI3Style::unpolish(QApplication *app)
{
    QProxyStyle::unpolish(app);
}

// void FluentUI3Style::unpolish( QWidget* widget )
// {
// #if QT_CONFIG( commandlinkbutton )
//     if ( !qobject_cast<QCommandLinkButton*>( widget ) )
// #endif  // QT_CONFIG(commandlinkbutton)
//     QProxyStyle::unpolish( widget );

//     if (auto tooBtn = qobject_cast<QToolButton*>(widget); tooBtn && ( widget->objectName() == "ScrollLeftButton" || widget->objectName()
//     == "ScrollRightButton" ) )
//     {
//         tooBtn->setAutoRaise( true );
//     }

//     const bool isScrollBar  = qobject_cast<QScrollBar*>( widget );
//     const bool isMenu       = qobject_cast<QMenu*>( widget );
//     const bool isComboPopup = isComboBoxPopup( widget );

//     if ( isScrollBar )
//     {
//         widget->setAttribute( Qt::WA_OpaquePaintEvent, true );
//         widget->setAttribute( Qt::WA_TranslucentBackground, false );
//     }
//     else if ( isMenu || isComboPopup )
//     {
//         widget->setAttribute( Qt::WA_OpaquePaintEvent, true );
//         widget->setAttribute( Qt::WA_TranslucentBackground, false );
//         widget->setWindowFlag( Qt::FramelessWindowHint, false );
//     }
//     else if ( QComboBox* cb = qobject_cast<QComboBox*>( widget ) )
//     {
//         if ( cb->isEditable() && cb->lineEdit() )
//         {
//             cb->lineEdit()->setFrame( true );
//         }
//     }
//     else if ( const auto* scrollarea = qobject_cast<QAbstractScrollArea*>( widget ); scrollarea && !qobject_cast<QGraphicsView*>( widget
//     )
// #if QT_CONFIG( mdiarea )
//                                                                                      && !qobject_cast<QMdiArea*>( widget )
// #endif
//     )
//     {
//         if ( scrollarea->frameShape() == QFrame::StyledPanel )
//         {
//             const auto vp                    = scrollarea->viewport();
//             const auto wasAutoFillBackground = vp->property( "_q_original_autofill_background" ).toBool();
//             vp->setAutoFillBackground( wasAutoFillBackground );
//             vp->setProperty( "_q_original_autofill_background", QVariant() );
//             const auto origStyledBackground = vp->property( "_q_original_styled_background" ).toBool();
//             vp->setAttribute( Qt::WA_StyledBackground, origStyledBackground );
//             vp->setProperty( "_q_original_styled_background", QVariant() );
//         }
//         if ( auto table = qobject_cast<QTableView*>( widget ) )
//         {
//             table->viewport()->setAttribute( Qt::WA_Hover, false );
//         }
//     }
// }

QIcon FluentUI3Style::standardIcon(StandardPixmap sp, const QStyleOption *option, const QWidget *widget) const
{
#if QT_VERSION > QT_VERSION_CHECK(6, 3, 0)
    if (sp == SP_LineEditClearButton || sp == SP_TitleBarCloseButton || sp == SP_TabCloseButton)
#else
    if (sp == SP_LineEditClearButton || sp == SP_TitleBarCloseButton)
#endif
    {
        QIcon icon = fluentIcon(QChar(0xE894));
        if (sp == SP_TitleBarCloseButton)
        {
            QIcon icon = fluentIcon(QChar(0xE894));
            QFont f(assetFont);
            f.setPixelSize(27);

            QPixmap pix(30, 30);
            pix.fill(Qt::transparent);

            QPainter p(&pix);
            p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

            p.setFont(f);
            p.setPen(widget ? widget->palette().color(QPalette::Window) : qApp->palette().color(QPalette::Window));
            p.drawText(pix.rect(), Qt::AlignCenter, QChar(0xE894));

            icon.addPixmap(pix, QIcon::Active);
            return icon;
        }
        return icon;
    }
    if (sp == SP_TitleBarNormalButton)
    {
        return colorSchemeIndex ? QIcon(":/resource/images/window-restore-s-light.png")
                                : QIcon(":/resource/images/window-restore-s-dark.png");
    }

    if (sp == SP_MessageBoxInformation)
    {
        return fluentIcon(InfoMessage, colorSchemeIndex ? QColor("#60CDFF") : QColor("#005FB8"));
    }
    if (sp == SP_MessageBoxWarning)
    {
        return fluentIcon(WarningMessage, colorSchemeIndex ? QColor("#FCE100") : QColor("#9D5D00"));
    }
    if (sp == SP_MessageBoxCritical)
    {
        return fluentIcon(CriticalMessage, colorSchemeIndex ? QColor("#FF99A4") : QColor("#C42B1C"));
    }
    if (sp == SP_MessageBoxQuestion)
    {
        return fluentIcon(Help, colorSchemeIndex ? QColor("#60CDFF") : QColor("#005FB8"));
    }

    return QProxyStyle::standardIcon(sp, option, widget);
}

QPalette FluentUI3Style::standardPalette() const
{
    QPalette palette;
    PaletteManager::instance().applyPalette(palette, colorSchemeIndex);
    return palette;
}

void FluentUI3Style::drawCheckBox(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    Q_UNUSED(widget)

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const bool isOn = option->state & QStyle::State_On;
    const bool isPartial = option->state & QStyle::State_NoChange;

    QRectF rect = option->rect;
    const QPointF center = rect.center();

    drawRoundedRect(painter, rect, borderPenControlAlt(option), controlFillBrush(option, ControlType::ControlAlt));

    if (isOn)
    {
        painter->setFont(assetFont);
        painter->setPen(controlTextColor(option, QPalette::Window));
        qreal clipWidth = 1.0;
        QFontMetrics fm(assetFont);
        QRectF clipRect = fm.boundingRect(AcceptMedium);
        if (transitionsEnabled() && option->styleObject)
        {
            QNumberStyleAnimation *animation = qobject_cast<QNumberStyleAnimation *>(getAnimation(option->styleObject));
            if (animation)
            {
                clipWidth = animation->currentValue();
            }
        }

        clipRect.moveCenter(center);
        clipRect.setLeft(rect.x() + (rect.width() - clipRect.width()) / 2.0 + 0.5);
        clipRect.setWidth(clipWidth * clipRect.width());
        painter->drawText(clipRect, Qt::AlignVCenter | Qt::AlignLeft, AcceptMedium);
    }
    else if (isPartial)
    {
        painter->setFont(assetFont);
        painter->setPen(controlTextColor(option, QPalette::Window));
        painter->drawText(rect, Qt::AlignCenter, Dash12);
    }

    painter->restore();
}

void FluentUI3Style::drawSwitchButton(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    Q_UNUSED(widget)

    const QStyleOptionButton *btn = static_cast<const QStyleOptionButton *>(option);

    painter->save();

    QRect rect = btn->rect.adjusted(1, 1, -1, -1);

    bool checked = btn->state & State_On;
    bool hovered = btn->state & State_MouseOver;
    bool pressed = btn->state & State_Sunken;
    bool enabled = btn->state & State_Enabled;

    QRect trackRect = rect.adjusted(0, 0, 0, 0);
    qreal radius = trackRect.height() / 2;
    qreal margin = 2.5f;
    qreal thumbRadius = radius - margin;

    float pos = animationValue(option->styleObject, "_q_thumb_pos", (checked ? 1.0f : 0.0f));
    float scale = animationValue(option->styleObject, "_q_thumb_scale", (hovered ? 1.1f : 0.9f));
    float stretch = animationValue(option->styleObject, "_q_thumb_stretch", (pressed ? 1.3f : 1.0f));

    QColor thumbColor;

    if (!enabled)
    {
        thumbColor = winUI3Color(switchThumbDisabled);
    }
    else if (checked)
    {
        thumbColor = winUI3Color(switchThumbOn);
    }
    else
    {
        thumbColor = winUI3Color(switchThumbOff);
    }

    // Hover / Pressed
    if (hovered)
    {
        thumbColor = thumbColor.lighter(110);
    }

    if (pressed)
    {
        thumbColor = thumbColor.darker(110);
    }

    QPen pen;
    pen.setColor(borderPenControlAlt(option).color());
    if (checked)
    {
        pen.setColor(controlFillBrush(option, ControlType::ControlAlt).color());
    }
    painter->setPen(pen);
    painter->setBrush(controlFillBrush(option, ControlType::ControlAlt));
    painter->drawRoundedRect(trackRect, radius, radius);

    const qreal baseRadius = thumbRadius;
    const qreal r = baseRadius * scale;

    // 椭圆尺寸（press 拉伸）
    const qreal w = r * 2.0 * stretch;
    const qreal h = r * 2.0;

    // 可移动范围（左右留 margin）
    QRect innerRect = rect.adjusted(margin, margin, -margin, -margin);
    const qreal minX = innerRect.left();
    const qreal maxX = innerRect.right() - w + 1.0;

    const qreal x = minX + pos * (maxX - minX);
    const qreal y = innerRect.top() + (innerRect.height() - h) / 2.0;

    const QRectF thumbRect(x, y, w, h);

    painter->setPen(Qt::NoPen);
    painter->setBrush(thumbColor);
    painter->drawRoundedRect(thumbRect, r, r);

    painter->restore();
}

QPen FluentUI3Style::borderPenControlAlt(const QStyleOption *option) const
{
    using namespace StyleOptionHelper;
    if (isChecked(option))
    {
        return Qt::NoPen;
    }

    if (isDisabled(option) || isPressed(option))
    {
        return winUI3Color(frameColorStrongDisabled);
    }
    return winUI3Color(frameColorStrong);
}

QColor blend(const QColor &fg, const QColor &bg, double alpha)
{
    return QColor(int(bg.red() + (fg.red() - bg.red()) * alpha),
                  int(bg.green() + (fg.green() - bg.green()) * alpha),
                  int(bg.blue() + (fg.blue() - bg.blue()) * alpha),
                  int(bg.alpha() + (fg.alpha() - bg.alpha()) * alpha));
}

QColor FluentUI3Style::calculateAccentColor(const QStyleOption *option) const
{
    using namespace StyleOptionHelper;
    if (isDisabled(option))
    {
        return winUI3Color(fillAccentDisabled);
    }

    const auto alphaColor = isPressed(option) ? fillAccentTertiary : isHover(option) ? fillAccentSecondary
                                                                                     : fillAccentDefault;
    const auto alpha = winUI3Color(alphaColor);
    QColor col = accentColor(option);
    col.setAlpha(alpha.alpha());
    return col;
}

QBrush FluentUI3Style::controlFillBrush(const QStyleOption *option, ControlType controlType) const
{
    using namespace StyleOptionHelper;
    static constexpr WINUI3Color colorEnums[2][4] = {
        // Light & Dark Control
        {fillControlDefault, fillControlSecondary, fillControlTertiary, fillControlDisabled},
        // Light & Dark Control Alt
        {fillControlAltSecondary, fillControlAltTertiary, fillControlAltQuarternary, fillControlAltDisabled},
    };

    if (option->palette.isBrushSet(QPalette::Current, QPalette::Button))
    {
        return option->palette.button();
    }

    if (!isChecked(option) && isAutoRaise(option))
    {
        return Qt::NoBrush;
    }

    // checked is the same for Control (Buttons) and Control Alt (Radiobuttons/Checkboxes)
    if (isChecked(option))
    {
        return calculateAccentColor(option);
    }

    const auto state = calcControlState(option);
    return winUI3Color(colorEnums[int(controlType)][int(state)]);
}

QBrush FluentUI3Style::inputFillBrush(const QStyleOption *option, const QWidget *widget) const
{
    // slightly different states than in controlFillBrush
    using namespace StyleOptionHelper;
    const auto role = widget ? widget->backgroundRole() : QPalette::Window;
    if (option->palette.isBrushSet(QPalette::Current, role))
    {
        return option->palette.button();
    }

    if (isDisabled(option))
    {
        return winUI3Color(fillControlDisabled);
    }
    if (option->state.testFlag(QStyle::State_HasFocus))
    {
        return winUI3Color(fillControlInputActive);
    }
    if (isHover(option))
    {
        return winUI3Color(fillControlSecondary);
    }
    return winUI3Color(fillControlDefault);
}

QColor FluentUI3Style::controlTextColor(const QStyleOption *option, QPalette::ColorRole role) const
{
    using namespace StyleOptionHelper;
    static constexpr WINUI3Color colorEnums[2][4] = {
        // Control, unchecked
        {textPrimary, textPrimary, textSecondary, textDisabled},
        // Control, checked
        {textOnAccentPrimary, textOnAccentPrimary, textOnAccentSecondary, textOnAccentDisabled},
    };

    if (option->palette.isBrushSet(QPalette::Current, QPalette::ButtonText))
    {
        return option->palette.buttonText().color();
    }

    const int colorIndex = isChecked(option) ? 1 : 0;
    const auto state = calcControlState(option);
    const auto alpha = winUI3Color(colorEnums[colorIndex][int(state)]);
    QColor col = option->palette.color(role);
    col.setAlpha(alpha.alpha());
    return col;
}

void FluentUI3Style::drawLineEditFrame(QPainter *painter,
                                       const QRectF &rect,
                                       const QStyleOption *option,
                                       bool isEditable,
                                       int roundingRadius) const
{
    const bool isHovered = option->state & State_MouseOver;
    const auto frameCol = highContrastTheme ?
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
                                            option->palette.color(isHovered ? QPalette::Accent : QPalette::ButtonText)
#else
                                            option->palette.color(isHovered ? QPalette::Highlight : QPalette::ButtonText)
#endif
                                            : winUI3Color(frameColorLight);

    painter->setPen(frameCol);
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(rect, roundingRadius, roundingRadius);

    if (!isEditable)
    {
        return;
    }

    PainterStateGuard psg(painter);
    // painter->setClipRect( rect.marginsRemoved( QMarginsF( 0, rect.height() - 0.5, 0, -1 ) ) );
    const qreal h = 1;
    QRectF underlineClip(rect.left(), rect.bottom() - 1, rect.width(), h + 2.0);

    painter->setClipRect(underlineClip);

    const bool hasFocus = option->state & State_HasFocus;

    const QColor focusColor = accentColor(option);

    const auto underlineCol = hasFocus ? accentColor(option)
                                       : (colorSchemeIndex == 0 ? QColor(0x80, 0x80, 0x80) : QColor(0xa0, 0xa0, 0xa0));
    const auto penUnderline = QPen(underlineCol, hasFocus ? 2 : 1);
    if (qobject_cast<QLineEdit *>(option->styleObject))
    {
        QObject *styleObject = option->styleObject;
        if (styleObject)
        {
            int state = option->state;
            int oldState = styleObject->property("_q_stylestate").toInt();
            styleObject->setProperty("_q_stylestate", state);
            if ((oldState & State_HasFocus) != (state & State_HasFocus))
            {
                QNumberStyleAnimation *t = new QNumberStyleAnimation(styleObject);
                t->setStartValue(state & State_HasFocus ? 0 : 1);
                t->setEndValue(state & State_HasFocus ? 1 : 0);
                t->setDuration(300);
                t->setEasingCurve(QEasingCurve::InOutSine);
                startAnimation(t);
            }
        }

        qreal progress = 1.0f;
        QNumberStyleAnimation *animation = qobject_cast<QNumberStyleAnimation *>(getAnimation(styleObject));
        if (animation)
        {
            progress = animation->currentValue();
            const qreal width = rect.width() * progress;
            const qreal left = rect.left() + (rect.width() - width) / 2.0;
            const QRectF animRect(left, rect.top(), width, rect.height());
            painter->setPen(QPen(focusColor, 1));

            if (!hasFocus)
            {
                drawRoundedRect(painter, rect, penUnderline, Qt::NoBrush);
            }
            drawRoundedRect(painter, animRect, QPen(focusColor, 2), Qt::NoBrush);
        }
        else
        {
            drawRoundedRect(painter, rect, penUnderline, Qt::NoBrush);
        }
    }
    else
    {
        const qreal penWidth = hasFocus ? 2.0 : 1.0;
        const qreal halfPen = penWidth / 2.0;

        auto pRect = rect;
        QRectF underlineClip(pRect.left(), pRect.bottom() - halfPen, pRect.width(), penWidth);

        PainterStateGuard guard(painter);
        painter->setClipRect(underlineClip);

        QPen pen(underlineCol, penWidth);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        drawRoundedRect(painter, rect, penUnderline, Qt::NoBrush);
    }
}

QColor FluentUI3Style::winUI3Color(WINUI3Color col) const
{
    return WINUI3Colors[colorSchemeIndex][col];
}

QColor FluentUI3Style::accentColor(const QStyleOption *option) const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    QBrush br = option->palette.accent();
    return br.color();
#else
    bool isDark = colorSchemeIndex == 1;
    QColor baseAccent = PaletteManager::instance().accentColor(isDark);
    if (!(option->state & QStyle::State_Enabled))
    {
        return baseAccent.darker(150).lighter(120);
    }
    if (!(option->state & QStyle::State_Active))
    {
        return baseAccent.darker(120);
    }

    return baseAccent;
#endif
}

QIcon FluentUI3Style::fluentIcon(const QChar &ch, const QColor &color) const
{
    QFont f(assetFont);
    f.setPixelSize(27);

    QPixmap pix(30, 30);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    p.setFont(f);
    p.setPen(color.isValid() ? color : winUI3Color(textPrimary));
    p.drawText(pix.rect(), Qt::AlignCenter, ch);

    return QIcon(pix);
}

void FluentUI3Style::drawFluentShadow(QPainter *painter, QRect rect, int shadowWidth, int radius) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);

    QColor color = colorSchemeIndex == 1 ? QColor(0x70, 0x70, 0x70) : QColor(0x9C, 0x9B, 0x9E);

    for (int i = 0; i < shadowWidth; ++i)
    {
        QRect r = rect.adjusted(i, i, -i, -i);
        int alpha = 25 * (shadowWidth - i) / shadowWidth;
        color.setAlpha(alpha);
        painter->setBrush(color);

        painter->drawRoundedRect(r, radius + shadowWidth - i, radius + shadowWidth - i);
    }

    painter->restore();
}

void FluentUI3Style::drawEffectShadow(QPainter *painter, QRect widgetRect, int shadowBorderWidth, int borderRadius) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    QColor color = colorSchemeIndex == 1 ? QColor(0x70, 0x70, 0x70) : QColor(0x9C, 0x9B, 0x9E);
    for (int i = 0; i < shadowBorderWidth; i++)
    {
        path.addRoundedRect(widgetRect.x() + shadowBorderWidth - i,
                            widgetRect.y() + shadowBorderWidth - i,
                            widgetRect.width() - (shadowBorderWidth - i) * 2,
                            widgetRect.height() - (shadowBorderWidth - i) * 2,
                            borderRadius + i,
                            borderRadius + i);
        int alpha = 2 * (shadowBorderWidth - i + 1);
        color.setAlpha(alpha > 255 ? 255 : alpha);
        painter->setPen(color);
        painter->drawPath(path);
    }
    painter->restore();
}

bool FluentUI3Style::eventFilter(QObject *watched, QEvent *event)
{
    if (auto tabBar = qobject_cast<QTabBar *>(watched))
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton)
            {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                int index = tabBar->tabAt(me->position().toPoint());
#else
                int index = tabBar->tabAt(me->pos());
#endif
                if (index >= 0)
                {
                    tabBar->setProperty("_q_segmented_winui3_pressed_index", index);
                    tabBar->update();
                }
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::Leave)
        {
            tabBar->setProperty("_q_segmented_winui3_pressed_index", -1);
            tabBar->update();
        }
    }
    else if (auto *viewport = qobject_cast<QWidget *>(watched))
    {
        auto *tableView = qobject_cast<QTableView *>(viewport->parent());
        if (tableView && tableView->viewport() == viewport)
        {
            auto updateHoverRow = [&](int newRow)
            {
                const int oldRow = viewport->property("_q_table_hover_row").toInt();
                if (oldRow == newRow)
                    return;
                viewport->setProperty("_q_table_hover_row", newRow);
                viewport->update();
            };

            if (event->type() == QEvent::MouseMove)
            {
                const auto *me = static_cast<QMouseEvent *>(event);
                const QModelIndex hoverIdx = tableView->indexAt(me->pos());
                updateHoverRow(hoverIdx.isValid() ? hoverIdx.row() : -1);
            }
            else if (event->type() == QEvent::HoverMove)
            {
                const auto *he = static_cast<QHoverEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QPoint pos = he->position().toPoint();
#else
                const QPoint pos = he->pos();
#endif
                const QModelIndex hoverIdx = tableView->indexAt(pos);
                updateHoverRow(hoverIdx.isValid() ? hoverIdx.row() : -1);
            }
            else if (event->type() == QEvent::Leave)
            {
                updateHoverRow(-1);
            }
        }

        auto *treeView = qobject_cast<QTreeView *>(viewport->parent());
        if (treeView && treeView->viewport() == viewport &&
            treeView->property(NavigationViewStyleProperty).toBool() &&
            (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease))
        {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton && transitionsEnabled())
            {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QPoint viewportPos = me->position().toPoint();
#else
                const QPoint viewportPos = me->pos();
#endif
                const QModelIndex index = treeView->indexAt(viewportPos);

                if (event->type() == QEvent::MouseButtonPress)
                {
                    if (index.isValid() && index.data(NavigationSettingsSpinRole).toBool())
                    {
                        treeView->setProperty("_q_nav_settings_spin_pressed", QVariant::fromValue(index));
                        startNavigationSettingsSpin(treeView, treeView, index, 30.0, 200);
                        viewport->update(treeView->visualRect(index));
                    }
                }
                else
                {
                    const QModelIndex pressed = treeView->property("_q_nav_settings_spin_pressed").toModelIndex();
                    treeView->setProperty("_q_nav_settings_spin_pressed", QModelIndex());
                    if (pressed.isValid() && pressed.data(NavigationSettingsSpinRole).toBool())
                    {
                        startNavigationSettingsSpin(treeView, treeView, pressed, 360.0, 600);
                        viewport->update(treeView->visualRect(pressed));
                    }
                }
            }
        }
    }
    return QProxyStyle::eventFilter(watched, event);
}
